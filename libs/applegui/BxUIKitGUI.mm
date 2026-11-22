#include "BxAppleGUI/BxAppleGUI.h"
#include "BxAppleGUI/BxGuiInputCoalescer.h"
#include "BxAppleGUI/BxMetalRenderer.h"
#include "BxCore/BxCore.h"
#include "BxEvents/BxEvents.h"

#import <CoreFoundation/CoreFoundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <mach/mach_time.h>

#include <crt_externs.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// **********************************************************************************************

@class BxGuiWindowHost;

struct BxGuiFrameTiming {
    uint64_t outputTimeNanoseconds;
    uint64_t targetTimeNanoseconds;
    uint64_t refreshIntervalNanoseconds;
    double eventTimeSeconds;
};

struct BxGuiInputStreamState {
    BxGuiInputCoalescer coalescer;
    bool canceled = false;
    uint64_t eventsReceived = 0;
    uint64_t eventsCoalesced = 0;
    uint64_t callbacksScheduled = 0;
    uint64_t callbacksServiced = 0;
    uint64_t barriersReceived = 0;
    uint64_t pendingCallbacks = 0;
    uint64_t pendingHighWaterMark = 0;

    bool tryMerge(const BxGuiInputEvent& event) {
        ++eventsReceived;
        if (event.inputKind == BxGuiInputKind::Barrier) {
            ++barriersReceived;
        }
        if (coalescer.tryMerge(event)) {
            ++eventsCoalesced;
            return true;
        }
        return false;
    }

    void append(BxGuiInputEvent& event) {
        coalescer.append(event);
        ++callbacksScheduled;
        ++pendingCallbacks;
        pendingHighWaterMark = std::max(pendingHighWaterMark, pendingCallbacks);
    }

    bool begin(BxGuiInputEvent& event) {
        coalescer.begin(event);
        if (pendingCallbacks) {
            --pendingCallbacks;
        }
        return !canceled;
    }

    void abandon(BxGuiInputEvent& event) {
        coalescer.abandon(event);
        if (pendingCallbacks) {
            --pendingCallbacks;
        }
    }

    void cancel() {
        canceled = true;
        coalescer.seal();
    }
};

static std::atomic<int32_t> guiAppVisibility(BxGuiPresentationHidden);
static bool guiAppLaunched = false;
static std::vector<BxGuiWindowHost*> guiWindows;
static BxGuiActionCallback guiAppQuitCallback = nullptr;
static void* guiAppQuitContext = nullptr;
static BxGuiActionCallback guiAppBackgroundCallback = nullptr;
static void* guiAppBackgroundContext = nullptr;
static BxGuiActionCallback guiAppSystemCallback = nullptr;
static void* guiAppSystemContext = nullptr;
static bool guiSystemObserving = false;
static UIBackgroundTaskIdentifier guiAppBackgroundTask = UIBackgroundTaskInvalid;
static bool guiAppKeepsDisplayAwake = false;

static void guiFrameSchedulerRequest(BxGuiWindowHost* guiWindow);
static void guiFrameSchedulerCancel(BxGuiWindowHost* guiWindow);
static void guiFrameSchedulerResume(void);

static void
guiReplaceRoot(void** slot, void* context) {
    void* root = context ? brixAddRoot(static_cast<char*>(context)) : nullptr;
    if (*slot) {
        brixRemoveRoot(*slot);
    }
    *slot = root;
}

// **********************************************************************************************

struct BxGuiCallbackRecord {
    explicit BxGuiCallbackRecord(void* aContext):
        context(aContext),
        contextRoot(aContext ? brixAddRoot(static_cast<char*>(aContext)) : nullptr)
    {}

    virtual ~BxGuiCallbackRecord() {
        if (contextRoot) {
            brixRemoveRoot(contextRoot);
        }
    }

    void* context;
    void* contextRoot;
};

static void
guiFinishCallbackTask(BxGuiCallbackRecord* record) {
    delete record;
}

struct BxGuiActionCallbackRecord : BxGuiCallbackRecord {
    BxGuiActionCallbackRecord(
        BxGuiActionCallback aCallback,
        void* aContext,
        uint64_t anIdleDeadlineNanoseconds,
        bool anIsDisplayFrame):
        BxGuiCallbackRecord(aContext),
        callback(aCallback),
        idleDeadlineNanoseconds(anIdleDeadlineNanoseconds),
        isDisplayFrame(anIsDisplayFrame) {}
    BxGuiActionCallback callback;
    uint64_t idleDeadlineNanoseconds;
    bool isDisplayFrame;
};

static const uint64_t kGuiOverrunFrameCollectorNanoseconds = 1000000ull;

static void
guiRunActionCallback(void* closure) {
    BxGuiActionCallbackRecord* record = static_cast<BxGuiActionCallbackRecord*>(closure);
    if (!brixProcessTerminationRequested()) {
        record->callback(record->context);
        uint64_t now = brixMonotonicTimeNanoseconds();
        if (record->idleDeadlineNanoseconds > now) {
            brixGCFrameBudget(record->idleDeadlineNanoseconds - now);
        } else if (record->isDisplayFrame) {
            brixGCFrameBudget(kGuiOverrunFrameCollectorNanoseconds);
        }
    }
    guiFinishCallbackTask(record);
}

static void
guiEndBackgroundTask(void) {
    if (guiAppBackgroundTask != UIBackgroundTaskInvalid) {
        [[UIApplication sharedApplication] endBackgroundTask:guiAppBackgroundTask];
        guiAppBackgroundTask = UIBackgroundTaskInvalid;
    }
}

static void
guiRunBackgroundCallback(void* closure) {
    BxGuiActionCallbackRecord* record = static_cast<BxGuiActionCallbackRecord*>(closure);
    if (!brixProcessTerminationRequested()) {
        record->callback(record->context);
    }
    guiEndBackgroundTask();
    guiFinishCallbackTask(record);
}

static void
guiScheduleBackgroundCallback(UIApplication* application) {
    if (!guiAppBackgroundCallback) {
        return;
    }
    guiEndBackgroundTask();
    guiAppBackgroundTask = [application beginBackgroundTaskWithExpirationHandler:^{
        guiEndBackgroundTask();
    }];
    BxGuiActionCallbackRecord* record =
        new BxGuiActionCallbackRecord(guiAppBackgroundCallback, guiAppBackgroundContext, 0, false);
    if (!brixScheduleMainTaskFromNative(guiRunBackgroundCallback, record)) {
        delete record;
        guiEndBackgroundTask();
    }
}

static void
guiApplyKeepsDisplayAwake(void) {
    if (guiAppLaunched) {
        [UIApplication sharedApplication].idleTimerDisabled = guiAppKeepsDisplayAwake;
    }
}

static void
guiScheduleActionCallback(
    BxGuiActionCallback callback,
    void* context,
    uint64_t idleDeadlineNanoseconds = 0,
    bool isDisplayFrame = false) {
    if (!callback) {
        return;
    }
    BxGuiActionCallbackRecord* record =
        new BxGuiActionCallbackRecord(callback, context, idleDeadlineNanoseconds, isDisplayFrame);
    if (!brixScheduleMainTaskFromNative(guiRunActionCallback, record)) {
        delete record;
    }
}

struct BxGuiPointerCallbackRecord : BxGuiCallbackRecord {
    BxGuiPointerCallbackRecord(
        BxGuiPointerCallback aCallback,
        void* aContext,
        std::shared_ptr<BxGuiInputStreamState> aStream,
        BxGuiInputEvent&& anEvent):
        BxGuiCallbackRecord(aContext),
        callback(aCallback),
        stream(std::move(aStream)),
        event(std::move(anEvent)),
        pending(true)
    {
        stream->append(event);
    }

    ~BxGuiPointerCallbackRecord() override {
        if (pending) {
            stream->abandon(event);
        }
    }

    BxGuiPointerCallback callback;
    std::shared_ptr<BxGuiInputStreamState> stream;
    BxGuiInputEvent event;
    bool pending;
};

static void
guiRunPointerCallback(void* closure) {
    BxGuiPointerCallbackRecord* record = static_cast<BxGuiPointerCallbackRecord*>(closure);
    record->pending = false;
    bool active = record->stream->begin(record->event);
    if (active && !brixProcessTerminationRequested()) {
        ++record->stream->callbacksServiced;
        record->callback(
            record->context,
            record->event.eventKind,
            record->event.pointerId,
            record->event.deviceKind,
            record->event.button,
            record->event.buttons,
            record->event.modifiers,
            record->event.x,
            record->event.y,
            record->event.deltaX,
            record->event.deltaY,
            record->event.orderingTime,
            record->event.clickCount,
            record->event.isPrecise,
            record->event.phase,
            record->event.momentumPhase,
            0
        );
    }
    guiFinishCallbackTask(record);
}

static void
guiSchedulePointerCallback(
    const std::shared_ptr<BxGuiInputStreamState>& stream,
    uintptr_t source,
    BxGuiPointerCallback callback,
    void* context,
    uint32_t eventKind,
    uint64_t pointerId,
    uint32_t deviceKind,
    uint64_t buttons,
    double x,
    double y,
    double deltaX,
    double deltaY,
    double orderingTime,
    uint32_t clickCount) {
    if (!stream || stream->canceled || !callback) {
        return;
    }
    BxGuiInputKind inputKind = eventKind == 0
        ? BxGuiInputKind::PointerMove
        : eventKind == 3
            ? BxGuiInputKind::PointerDrag
            : BxGuiInputKind::Barrier;
    BxGuiInputEvent event(inputKind, source);
    event.eventKind = eventKind;
    event.pointerId = pointerId;
    event.deviceKind = deviceKind;
    event.buttons = buttons;
    event.x = x;
    event.y = y;
    event.deltaX = deltaX;
    event.deltaY = deltaY;
    event.orderingTime = orderingTime;
    event.clickCount = clickCount;
    if (stream->tryMerge(event)) {
        return;
    }
    BxGuiPointerCallbackRecord* record =
        new BxGuiPointerCallbackRecord(callback, context, stream, std::move(event));
    if (!brixScheduleMainTaskFromNative(guiRunPointerCallback, record)) {
        delete record;
    }
}

struct BxGuiTouchRecord {
    uint32_t phase;
    uint64_t touchId;
    double x;
    double y;
    double orderingTime;
    uint32_t tapCount;
    double radius;
};

struct BxGuiTouchCallbackRecord : BxGuiCallbackRecord {
    BxGuiTouchCallbackRecord(
        BxGuiTouchCallback aCallback,
        void* aContext,
        std::shared_ptr<BxGuiInputStreamState> aStream,
        std::vector<BxGuiTouchRecord>&& aTouches):
        BxGuiCallbackRecord(aContext),
        callback(aCallback),
        stream(std::move(aStream)),
        touches(std::move(aTouches))
    {}

    BxGuiTouchCallback callback;
    std::shared_ptr<BxGuiInputStreamState> stream;
    std::vector<BxGuiTouchRecord> touches;
};

static void
guiRunTouchCallback(void* closure) {
    BxGuiTouchCallbackRecord* record = static_cast<BxGuiTouchCallbackRecord*>(closure);
    for (const BxGuiTouchRecord& touch : record->touches) {
        if (record->stream->canceled || brixProcessTerminationRequested()) {
            break;
        }
        record->callback(
            record->context,
            touch.phase,
            touch.touchId,
            touch.x,
            touch.y,
            touch.orderingTime,
            touch.tapCount,
            touch.radius
        );
    }
    guiFinishCallbackTask(record);
}

static void
guiScheduleTouchCallback(
    const std::shared_ptr<BxGuiInputStreamState>& stream,
    BxGuiTouchCallback callback,
    void* context,
    std::vector<BxGuiTouchRecord>&& touches) {
    if (!stream || stream->canceled || !callback || touches.empty()) {
        return;
    }
    BxGuiTouchCallbackRecord* record =
        new BxGuiTouchCallbackRecord(callback, context, stream, std::move(touches));
    if (!brixScheduleMainTaskFromNative(guiRunTouchCallback, record)) {
        delete record;
    }
}

struct BxGuiSizeCallbackRecord : BxGuiCallbackRecord {
    BxGuiSizeCallbackRecord(BxGuiWindowHost* aGuiWindow, BxGuiSizeCallback aCallback, void* aContext);
    ~BxGuiSizeCallbackRecord() override;
    BxGuiWindowHost* guiWindow;
    BxGuiSizeCallback callback;
};

struct BxGuiWindowEventCallbackRecord : BxGuiCallbackRecord {
    BxGuiWindowEventCallbackRecord(
        BxGuiWindowHost* aGuiWindow,
        BxGuiWindowEventCallback aCallback,
        void* aContext,
        uint32_t anEventKind);
    ~BxGuiWindowEventCallbackRecord() override;
    BxGuiWindowHost* guiWindow;
    BxGuiWindowEventCallback callback;
    uint32_t eventKind;
};

struct BxGuiRenderFrameCallbackRecord : BxGuiActionCallbackRecord {
    BxGuiRenderFrameCallbackRecord(
        BxGuiWindowHost* aGuiWindow,
        BxGuiActionCallback aCallback,
        void* aContext,
        uint64_t anIdleDeadlineNanoseconds);
    ~BxGuiRenderFrameCallbackRecord() override;
    BxGuiWindowHost* guiWindow;
};

// **********************************************************************************************

@interface BxGuiRootView : UIView

@property(nonatomic, weak) BxGuiWindowHost* guiWindow;

@end

@interface BxGuiRootViewController : UIViewController
@end

@interface BxGuiWindowHost : NSObject {
@public
    std::shared_ptr<BxGuiInputStreamState> inputStreamState;
    CAMetalLayer* metalLayer;
    UIWindow* window;
    BxGuiRootView* rootView;
    double contentWidth;
    double contentHeight;
    double scale;
    bool shown;
    bool closed;
    bool frameUpdateOpen;
    bool presentationPending;
    bool presentationHeldFrame;
    bool renderScheduled;
    uint64_t renderNotBeforeNanoseconds;
    bool contentSizePending;
    BxGuiActionCallback renderCallback;
    void* renderContext;
    void* renderContextRoot;
    BxGuiSizeCallback contentSizeCallback;
    void* contentSizeContext;
    void* contentSizeContextRoot;
    BxGuiActionCallback backingScaleCallback;
    void* backingScaleContext;
    void* backingScaleContextRoot;
    BxGuiPointerCallback pointerCallback;
    void* pointerContext;
    void* pointerContextRoot;
    BxGuiTouchCallback touchCallback;
    void* touchContext;
    void* touchContextRoot;
    BxGuiWindowEventCallback eventCallback;
    void* eventContext;
    void* eventContextRoot;
    std::unordered_map<void*, uint64_t> touchIds;
    uint64_t nextTouchId;
    UITouch* primaryTouch;
    CGPoint primaryTouchPoint;
    uint64_t lastFrameTargetTimeNanoseconds;
    uint64_t lastFrameRefreshIntervalNanoseconds;
    double lastFrameEventTimeSeconds;
}

- (void)attachToScreen;
- (void)viewDidLayout;
- (void)beginFrameUpdate;
- (void)finishFrameUpdate;
- (void)requestRenderFrameWithCallback:(BxGuiActionCallback)callback
                               context:(void*)context
                             notBefore:(uint64_t)notBeforeNanoseconds;
- (void)fireRenderFrameWithTiming:(BxGuiFrameTiming)timing;
- (void)dispatchWindowEventKind:(uint32_t)kind;
- (void)touchesBegan:(NSSet<UITouch*>*)touches;
- (void)touchesMoved:(NSSet<UITouch*>*)touches;
- (void)touchesEnded:(NSSet<UITouch*>*)touches;
- (void)touchesCancelled:(NSSet<UITouch*>*)touches;

@end

BxGuiSizeCallbackRecord::BxGuiSizeCallbackRecord(
    BxGuiWindowHost* aGuiWindow, BxGuiSizeCallback aCallback, void* aContext):
    BxGuiCallbackRecord(aContext),
    guiWindow(aGuiWindow),
    callback(aCallback)
{}

BxGuiSizeCallbackRecord::~BxGuiSizeCallbackRecord() {
}

BxGuiWindowEventCallbackRecord::BxGuiWindowEventCallbackRecord(
    BxGuiWindowHost* aGuiWindow,
    BxGuiWindowEventCallback aCallback,
    void* aContext,
    uint32_t anEventKind):
    BxGuiCallbackRecord(aContext),
    guiWindow(aGuiWindow),
    callback(aCallback),
    eventKind(anEventKind)
{}

BxGuiWindowEventCallbackRecord::~BxGuiWindowEventCallbackRecord() {
}

BxGuiRenderFrameCallbackRecord::BxGuiRenderFrameCallbackRecord(
    BxGuiWindowHost* aGuiWindow,
    BxGuiActionCallback aCallback,
    void* aContext,
    uint64_t anIdleDeadlineNanoseconds):
    BxGuiActionCallbackRecord(aCallback, aContext, anIdleDeadlineNanoseconds, true),
    guiWindow(aGuiWindow)
{}

BxGuiRenderFrameCallbackRecord::~BxGuiRenderFrameCallbackRecord() {
}

static void
guiRunRenderFrameCallback(void* closure) {
    BxGuiRenderFrameCallbackRecord* record = static_cast<BxGuiRenderFrameCallbackRecord*>(closure);
    BxGuiWindowHost* guiWindow = record->guiWindow;
    if (guiAppVisibility.load() == BxGuiPresentationHidden) {
        if (!guiWindow->closed && !guiWindow->renderScheduled) {
            [guiWindow requestRenderFrameWithCallback:record->callback context:record->context notBefore:0];
        }
        guiFinishCallbackTask(record);
        return;
    }
    guiRunActionCallback(closure);
}

static void
guiRunWindowEventCallback(void* closure) {
    BxGuiWindowEventCallbackRecord* record = static_cast<BxGuiWindowEventCallbackRecord*>(closure);
    if (!brixProcessTerminationRequested() && !record->guiWindow->closed) {
        record->callback(record->context, record->eventKind, 0, 0, 0, false, "");
    }
    guiFinishCallbackTask(record);
}

static void
guiRunSizeCallback(void* closure) {
    BxGuiSizeCallbackRecord* record = static_cast<BxGuiSizeCallbackRecord*>(closure);
    BxGuiWindowHost* guiWindow = record->guiWindow;
    guiWindow->contentSizePending = false;
    if (!brixProcessTerminationRequested() && !guiWindow->closed) {
        record->callback(record->context, guiWindow->contentWidth, guiWindow->contentHeight);
    }
    guiFinishCallbackTask(record);
}

static void
guiScheduleSizeCallback(BxGuiWindowHost* guiWindow) {
    if (!guiWindow->contentSizeCallback || guiWindow->closed || guiWindow->contentSizePending) {
        return;
    }
    guiWindow->contentSizePending = true;
    BxGuiSizeCallbackRecord* record = new BxGuiSizeCallbackRecord(
        guiWindow, guiWindow->contentSizeCallback, guiWindow->contentSizeContext);
    if (!brixScheduleMainTaskFromNative(guiRunSizeCallback, record)) {
        guiWindow->contentSizePending = false;
        delete record;
    }
}

@implementation BxGuiRootView

- (void)layoutSubviews {
    [super layoutSubviews];
    [self.guiWindow viewDidLayout];
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self.guiWindow touchesBegan:touches];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self.guiWindow touchesMoved:touches];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self.guiWindow touchesEnded:touches];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self.guiWindow touchesCancelled:touches];
}

@end

@implementation BxGuiRootViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    [self registerForTraitChanges:@[ UITraitUserInterfaceStyle.class ]
                      withHandler:^(__kindof id<UITraitEnvironment>, UITraitCollection*) {
        guiScheduleActionCallback(guiAppSystemCallback, guiAppSystemContext);
    }];
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    return UIRectEdgeAll;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskLandscape;
}

@end

@implementation BxGuiWindowHost

- (void)attachToScreen {
    if (window || closed) {
        return;
    }
    UIWindow* screenWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    BxGuiRootViewController* controller = [[BxGuiRootViewController alloc] init];
    BxGuiRootView* view = [[BxGuiRootView alloc] initWithFrame:[screenWindow bounds]];
    view.guiWindow = self;
    view.multipleTouchEnabled = YES;
    view.backgroundColor = [UIColor blackColor];
    [view.layer addSublayer:metalLayer];
    controller.view = view;
    screenWindow.rootViewController = controller;
    window = screenWindow;
    rootView = view;
    if (shown) {
        [window makeKeyAndVisible];
    }
}

- (void)detachFromScreen {
    if (!window) {
        return;
    }
    rootView.guiWindow = nil;
    [metalLayer removeFromSuperlayer];
    window.hidden = YES;
    window.rootViewController = nil;
    rootView = nil;
    window = nil;
}

- (void)dispatchWindowEventKind:(uint32_t)kind {
    if (!eventCallback || closed) {
        return;
    }
    BxGuiWindowEventCallbackRecord* record =
        new BxGuiWindowEventCallbackRecord(self, eventCallback, eventContext, kind);
    if (!brixScheduleMainTaskFromNative(guiRunWindowEventCallback, record)) {
        delete record;
    }
}

- (void)viewDidLayout {
    if (closed || !rootView) {
        return;
    }
    CGRect bounds = [rootView bounds];
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    metalLayer.frame = bounds;
    [CATransaction commit];

    CGFloat screenScale = rootView.window ? rootView.window.screen.scale : [UIScreen mainScreen].scale;
    if (screenScale > 0 && screenScale != scale) {
        scale = screenScale;
        guiScheduleActionCallback(backingScaleCallback, backingScaleContext);
    }
    if (bounds.size.width != contentWidth || bounds.size.height != contentHeight) {
        contentWidth = bounds.size.width;
        contentHeight = bounds.size.height;
        guiScheduleSizeCallback(self);
    }
}

- (void)beginFrameUpdate {
    if (closed || frameUpdateOpen) {
        return;
    }
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    frameUpdateOpen = true;
    metalLayer.presentsWithTransaction = !shown;
}

- (void)finishFrameUpdate {
    if (!frameUpdateOpen) {
        return;
    }
    frameUpdateOpen = false;
    [CATransaction commit];
}

- (void)requestRenderFrameWithCallback:(BxGuiActionCallback)callback
                               context:(void*)context
                             notBefore:(uint64_t)notBeforeNanoseconds {
    if (closed) {
        return;
    }
    renderCallback = callback;
    renderContext = context;
    guiReplaceRoot(&renderContextRoot, context);
    renderScheduled = true;
    renderNotBeforeNanoseconds = notBeforeNanoseconds;
    guiFrameSchedulerRequest(self);
}

- (void)cancelRenderFrame {
    renderScheduled = false;
    renderCallback = nullptr;
    renderContext = nullptr;
    guiReplaceRoot(&renderContextRoot, nullptr);
    guiFrameSchedulerCancel(self);
}

- (void)fireRenderFrameWithTiming:(BxGuiFrameTiming)timing {
    lastFrameTargetTimeNanoseconds = timing.targetTimeNanoseconds;
    lastFrameRefreshIntervalNanoseconds = timing.refreshIntervalNanoseconds;
    lastFrameEventTimeSeconds = timing.eventTimeSeconds;

    if (presentationPending && renderScheduled && !closed) {
        presentationHeldFrame = true;
        return;
    }
    BxGuiActionCallback callback = renderScheduled && !closed ? renderCallback : nullptr;
    void* context = renderContext;
    void* root = renderContextRoot;
    renderScheduled = false;
    renderCallback = nullptr;
    renderContext = nullptr;
    renderContextRoot = nullptr;

    if (callback) {
        uint64_t idleDeadlineNanoseconds = timing.targetTimeNanoseconds;
        if (timing.refreshIntervalNanoseconds) {
            idleDeadlineNanoseconds = timing.outputTimeNanoseconds + timing.refreshIntervalNanoseconds;
        }
        BxGuiRenderFrameCallbackRecord* record =
            new BxGuiRenderFrameCallbackRecord(self, callback, context, idleDeadlineNanoseconds);
        if (!brixScheduleMainTaskFromNative(guiRunRenderFrameCallback, record)) {
            delete record;
        }
    }
    if (root) {
        brixRemoveRoot(root);
    }
}

- (void)dispatchPointerKind:(uint32_t)kind
                      touch:(UITouch*)touch
                      point:(CGPoint)point
                     deltaX:(double)deltaX
                     deltaY:(double)deltaY
                    buttons:(uint64_t)buttons {
    if (!pointerCallback || closed) {
        return;
    }
    uint32_t deviceKind = touch.type == UITouchTypePencil
        ? 1
        : touch.type == UITouchTypeDirect
            ? 4
            : 0;
    auto found = touchIds.find((__bridge void*)touch);
    guiSchedulePointerCallback(
        inputStreamState,
        reinterpret_cast<uintptr_t>(self),
        pointerCallback,
        pointerContext,
        kind,
        found == touchIds.end() ? 0 : found->second,
        deviceKind,
        buttons,
        point.x,
        point.y,
        deltaX,
        deltaY,
        touch.timestamp,
        kind == 1 || kind == 2 ? static_cast<uint32_t>(touch.tapCount) : 0
    );
}

- (void)dispatchTouches:(NSSet<UITouch*>*)touches phase:(uint32_t)phase {
    std::vector<BxGuiTouchRecord> records;
    records.reserve(touches.count);
    for (UITouch* touch in touches) {
        auto found = touchIds.find((__bridge void*)touch);
        uint64_t touchId = found == touchIds.end() ? 0 : found->second;
        if (!touchId) {
            touchId = ++nextTouchId;
            touchIds[(__bridge void*)touch] = touchId;
        }
        if (phase >= 2) {
            touchIds.erase((__bridge void*)touch);
        }
        CGPoint point = [touch locationInView:rootView];
        records.push_back({
            phase,
            touchId,
            point.x,
            point.y,
            touch.timestamp,
            static_cast<uint32_t>(touch.tapCount),
            touch.majorRadius
        });
    }
    if (closed) {
        return;
    }
    guiScheduleTouchCallback(inputStreamState, touchCallback, touchContext, std::move(records));
}

- (void)dragPrimaryTouch {
    CGPoint point = [primaryTouch locationInView:rootView];
    double deltaX = point.x - primaryTouchPoint.x;
    double deltaY = point.y - primaryTouchPoint.y;
    if (deltaX == 0 && deltaY == 0) {
        return;
    }
    primaryTouchPoint = point;
    [self dispatchPointerKind:3 touch:primaryTouch point:point deltaX:deltaX deltaY:deltaY buttons:1];
}

- (void)cancelPrimaryTouch {
    UITouch* touch = primaryTouch;
    [self dispatchPointerKind:8 touch:touch point:primaryTouchPoint deltaX:0 deltaY:0 buttons:0];
    [self dispatchPointerKind:5 touch:touch point:primaryTouchPoint deltaX:0 deltaY:0 buttons:0];
    primaryTouch = nil;
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches {
    bool startsAlone = touchIds.empty() && touches.count == 1;
    if (primaryTouch) {
        [self cancelPrimaryTouch];
    }
    [self dispatchTouches:touches phase:0];
    if (!startsAlone) {
        return;
    }
    UITouch* touch = [touches anyObject];
    CGPoint point = [touch locationInView:rootView];
    primaryTouch = touch;
    primaryTouchPoint = point;
    [self dispatchPointerKind:0 touch:touch point:point deltaX:0 deltaY:0 buttons:0];
    [self dispatchPointerKind:1 touch:touch point:point deltaX:0 deltaY:0 buttons:1];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches {
    [self dispatchTouches:touches phase:1];
    if (primaryTouch && [touches containsObject:primaryTouch]) {
        [self dragPrimaryTouch];
    }
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches {
    bool endsPrimary = primaryTouch && [touches containsObject:primaryTouch];
    if (endsPrimary) {
        [self dragPrimaryTouch];
    }
    [self dispatchTouches:touches phase:2];
    if (!endsPrimary) {
        return;
    }
    UITouch* touch = primaryTouch;
    [self dispatchPointerKind:2 touch:touch point:primaryTouchPoint deltaX:0 deltaY:0 buttons:0];
    [self dispatchPointerKind:5 touch:touch point:primaryTouchPoint deltaX:0 deltaY:0 buttons:0];
    primaryTouch = nil;
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches {
    bool cancelsPrimary = primaryTouch && [touches containsObject:primaryTouch];
    [self dispatchTouches:touches phase:3];
    if (cancelsPrimary) {
        [self cancelPrimaryTouch];
    }
}

- (void)dealloc {
    [self detachFromScreen];
}

@end

// **********************************************************************************************

@interface BxGuiAppHost : NSObject
@end

@implementation BxGuiAppHost
@end

@interface BxGuiAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation BxGuiAppDelegate

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options {
    guiAppLaunched = true;
    guiAppVisibility.store(BxGuiPresentationUnoccluded);
    guiApplyKeepsDisplayAwake();
    std::vector<BxGuiWindowHost*> windows = guiWindows;
    for (BxGuiWindowHost* guiWindow : windows) {
        [guiWindow attachToScreen];
    }
    guiFrameSchedulerResume();
    return YES;
}

- (UIWindow*)window {
    return guiWindows.empty() ? nil : guiWindows.front()->window;
}

- (void)applicationDidBecomeActive:(UIApplication*)application {
    guiAppVisibility.store(BxGuiPresentationUnoccluded);
    guiApplyKeepsDisplayAwake();
    std::vector<BxGuiWindowHost*> windows = guiWindows;
    for (BxGuiWindowHost* guiWindow : windows) {
        if (guiWindow->shown) {
            [guiWindow dispatchWindowEventKind:9];
        }
    }
}

- (void)applicationWillResignActive:(UIApplication*)application {
    guiAppVisibility.store(BxGuiPresentationOccluded);
}

- (void)applicationDidEnterBackground:(UIApplication*)application {
    guiAppVisibility.store(BxGuiPresentationHidden);
    guiScheduleBackgroundCallback(application);
}

- (void)applicationWillEnterForeground:(UIApplication*)application {
    guiAppVisibility.store(BxGuiPresentationOccluded);
    guiFrameSchedulerResume();
}

- (void)applicationWillTerminate:(UIApplication*)application {
    if (guiAppQuitCallback) {
        guiScheduleActionCallback(guiAppQuitCallback, guiAppQuitContext);
    }
}

@end

// **********************************************************************************************

static const size_t kGuiMainQueueTaskBudget = 64;
static const uint64_t kGuiMainQueueNanosecondBudget = 2000000ull;
static const uint64_t kGuiIdleCollectNanoseconds = 1000000ull;
static const uint64_t kGuiIdleCollectQuietNanoseconds = 100000000ull;
static const uint64_t kGuiIdleCollectIntervalNanoseconds = 20000000ull;
static const uint64_t kGuiIdleReleaseQuietNanoseconds = 5000000000ull;
static std::mutex guiMainQueueSourceMutex;
static CFRunLoopSourceRef guiMainQueueSource = nullptr;
static CFRunLoopRef guiMainQueueRunLoop = nullptr;
static std::atomic_bool guiMainQueueStopRequested(false);

static void
guiMainQueueWake(void*) {
    CFRunLoopSourceRef source = nullptr;
    CFRunLoopRef runLoop = nullptr;
    {
        std::lock_guard<std::mutex> lock(guiMainQueueSourceMutex);
        if (guiMainQueueSource) {
            source = (CFRunLoopSourceRef)CFRetain(guiMainQueueSource);
        }
        if (guiMainQueueRunLoop) {
            runLoop = (CFRunLoopRef)CFRetain(guiMainQueueRunLoop);
        }
    }
    if (source && runLoop) {
        CFRunLoopSourceSignal(source);
        CFRunLoopWakeUp(runLoop);
    }
    if (source) {
        CFRelease(source);
    }
    if (runLoop) {
        CFRelease(runLoop);
    }
}

static void
guiMainQueueRequestStop(void) {
    guiMainQueueStopRequested.store(true, std::memory_order_release);
    guiMainQueueWake(nullptr);
}

static uint64_t guiMainActivityCount = 0;
static uint64_t guiMainActivityNanoseconds = 0;
static uint64_t guiIdleCollectArmedCount = 0;
static bool guiIdleCollectScheduled = false;
static bool guiIdleReleaseScheduled = false;

static void
guiMainNoteActivity(void) {
    ++guiMainActivityCount;
    guiMainActivityNanoseconds = brixMonotonicTimeNanoseconds();
}

static void
guiMainQueueScheduleIdleRelease(uint64_t delayNanoseconds) {
    if (guiIdleReleaseScheduled) {
        return;
    }
    guiIdleReleaseScheduled = true;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(delayNanoseconds)),
        dispatch_get_main_queue(),
        ^{
            guiIdleReleaseScheduled = false;
            if (guiMainQueueStopRequested.load(std::memory_order_acquire) ||
                    brixProcessTerminationRequested()) {
                return;
            }
            uint64_t quiet = brixMonotonicTimeNanoseconds() - guiMainActivityNanoseconds;
            if (quiet < kGuiIdleReleaseQuietNanoseconds) {
                guiMainQueueScheduleIdleRelease(kGuiIdleReleaseQuietNanoseconds - quiet);
                return;
            }
            brixGuiMetalReleaseIdle();
        });
}

static void
guiMainQueueScheduleIdleCollect(uint64_t delayNanoseconds) {
    if (guiIdleCollectScheduled) {
        return;
    }
    guiIdleCollectScheduled = true;
    guiIdleCollectArmedCount = guiMainActivityCount;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(delayNanoseconds)),
        dispatch_get_main_queue(),
        ^{
            guiIdleCollectScheduled = false;
            if (guiMainQueueStopRequested.load(std::memory_order_acquire) ||
                    brixProcessTerminationRequested()) {
                return;
            }
            if (guiMainActivityCount != guiIdleCollectArmedCount) {
                guiMainQueueScheduleIdleCollect(kGuiIdleCollectQuietNanoseconds);
                return;
            }
            if (brixGCRuntimeIdleCollect(kGuiIdleCollectNanoseconds)) {
                guiMainQueueScheduleIdleCollect(kGuiIdleCollectIntervalNanoseconds);
            }
            guiMainQueueScheduleIdleRelease(kGuiIdleReleaseQuietNanoseconds);
        });
}

static void
guiMainQueueSourcePerform(void*) {
    guiMainNoteActivity();
    uint64_t now = brixMonotonicTimeNanoseconds();
    uint32_t result = brixDrainMainTaskQueue(kGuiMainQueueTaskBudget, now + kGuiMainQueueNanosecondBudget);
    if (guiMainQueueStopRequested.load(std::memory_order_acquire) ||
        brixProcessTerminationRequested()) {
        std::exit(brixGetProcessExitCode());
    }
    if (result == 0) {
        guiMainQueueScheduleIdleCollect(kGuiIdleCollectQuietNanoseconds);
    }
    if (result == 1) {
        guiMainQueueWake(nullptr);
    }
}

static void
guiMainQueueRunHost(void*) {
    guiMainQueueStopRequested.store(false, std::memory_order_release);
    CFRunLoopSourceContext context = {};
    context.perform = guiMainQueueSourcePerform;
    CFRunLoopSourceRef source = CFRunLoopSourceCreate(nullptr, 0, &context);
    CFRunLoopRef runLoop = CFRunLoopGetMain();
    {
        std::lock_guard<std::mutex> lock(guiMainQueueSourceMutex);
        guiMainQueueSource = source;
        guiMainQueueRunLoop = (CFRunLoopRef)CFRetain(runLoop);
    }
    CFRunLoopAddSource(runLoop, source, kCFRunLoopCommonModes);
    guiMainQueueWake(nullptr);
    @autoreleasepool {
        UIApplicationMain(
            *_NSGetArgc(), *_NSGetArgv(), nil, NSStringFromClass([BxGuiAppDelegate class]));
    }
}

// **********************************************************************************************

@interface BxGuiDisplayLinkTarget : NSObject
- (void)tick:(CADisplayLink*)link;
@end

static CADisplayLink* guiDisplayLink = nil;
static std::vector<BxGuiWindowHost*> guiFramePendingWindows;
static mach_timebase_info_data_t guiMachTimebase = {0, 0};

static uint64_t
guiMachTimeToNanoseconds(uint64_t machTime) {
    if (!guiMachTimebase.denom) {
        mach_timebase_info(&guiMachTimebase);
    }
    return static_cast<uint64_t>(
        (static_cast<__uint128_t>(machTime) * guiMachTimebase.numer) / guiMachTimebase.denom);
}

static uint64_t
guiMediaTimeToContinuousNanoseconds(CFTimeInterval seconds) {
    uint64_t absolute = mach_absolute_time();
    uint64_t continuous = mach_continuous_time();
    return static_cast<uint64_t>(seconds * 1e9) + guiMachTimeToNanoseconds(continuous - absolute);
}

@implementation BxGuiDisplayLinkTarget

- (void)tick:(CADisplayLink*)link {
    guiMainNoteActivity();
    if (guiAppVisibility.load() == BxGuiPresentationHidden) {
        link.paused = YES;
        return;
    }
    BxGuiFrameTiming timing = {};
    timing.outputTimeNanoseconds = guiMediaTimeToContinuousNanoseconds(link.timestamp);
    timing.targetTimeNanoseconds = guiMediaTimeToContinuousNanoseconds(link.targetTimestamp);
    timing.refreshIntervalNanoseconds = link.targetTimestamp > link.timestamp
        ? static_cast<uint64_t>((link.targetTimestamp - link.timestamp) * 1e9)
        : 0;
    timing.eventTimeSeconds = link.timestamp;

    std::vector<BxGuiWindowHost*> windows;
    auto fired = std::stable_partition(
        guiFramePendingWindows.begin(), guiFramePendingWindows.end(), [&timing](BxGuiWindowHost* guiWindow) {
            return timing.targetTimeNanoseconds && guiWindow->renderNotBeforeNanoseconds > timing.targetTimeNanoseconds;
        });
    windows.assign(fired, guiFramePendingWindows.end());
    guiFramePendingWindows.erase(fired, guiFramePendingWindows.end());
    for (BxGuiWindowHost* guiWindow : windows) {
        [guiWindow fireRenderFrameWithTiming:timing];
    }
    if (windows.empty() && guiFramePendingWindows.empty()) {
        link.paused = YES;
    }
}

@end

static void
guiFrameSchedulerRequest(BxGuiWindowHost* guiWindow) {
    if (std::find(guiFramePendingWindows.begin(), guiFramePendingWindows.end(), guiWindow) ==
        guiFramePendingWindows.end()) {
        guiFramePendingWindows.push_back(guiWindow);
    }
    if (!guiDisplayLink) {
        BxGuiDisplayLinkTarget* target = [[BxGuiDisplayLinkTarget alloc] init];
        guiDisplayLink = [CADisplayLink displayLinkWithTarget:target selector:@selector(tick:)];
        float maximum = static_cast<float>([UIScreen mainScreen].maximumFramesPerSecond);
        guiDisplayLink.preferredFrameRateRange = CAFrameRateRangeMake(maximum, maximum, maximum);
        [guiDisplayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    }
    guiDisplayLink.paused = guiAppVisibility.load() == BxGuiPresentationHidden;
}

static void
guiFrameSchedulerResume(void) {
    if (guiDisplayLink && !guiFramePendingWindows.empty() &&
        guiAppVisibility.load() != BxGuiPresentationHidden) {
        guiDisplayLink.paused = NO;
    }
}

static void
guiFrameSchedulerCancel(BxGuiWindowHost* guiWindow) {
    auto found = std::find(guiFramePendingWindows.begin(), guiFramePendingWindows.end(), guiWindow);
    if (found != guiFramePendingWindows.end()) {
        guiFramePendingWindows.erase(found);
    }
}

// **********************************************************************************************

static BxGuiWindowHost*
getWindow(void* ptr) {
    return (__bridge BxGuiWindowHost*)ptr;
}

extern "C" void*
brixGuiAppCreate(void) {
    return (__bridge_retained void*)[[BxGuiAppHost alloc] init];
}

extern "C" void
brixGuiAppRelease(void* ptr) {
    guiAppQuitCallback = nullptr;
    guiAppQuitContext = nullptr;
    guiAppBackgroundCallback = nullptr;
    guiAppBackgroundContext = nullptr;
    guiAppSystemCallback = nullptr;
    guiAppSystemContext = nullptr;
    CFBridgingRelease(ptr);
}

extern "C" void
brixGuiAppRun(void* ptr) {
    if (!brixRunMainTaskQueueHost(guiMainQueueRunHost, guiMainQueueWake, ptr) &&
        !brixProcessTerminationRequested()) {
        std::fprintf(stderr, "App.run must own the active main Brix task queue\n");
        std::abort();
    }
}

extern "C" void
brixGuiAppStop(void* ptr) {
    guiMainQueueRequestStop();
}

extern "C" void
brixGuiAppScheduleStop(void* ptr, uint64_t delayNanoseconds) {
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(std::min<uint64_t>(delayNanoseconds, INT64_MAX))),
        dispatch_get_main_queue(),
        ^{
            guiMainQueueRequestStop();
        });
}

extern "C" void
brixGuiAppSetTitle(void* ptr, const char* title) {}

extern "C" void
brixGuiAppSetQuitCallback(void* ptr, BxGuiActionCallback callback, void* context) {
    guiAppQuitCallback = callback;
    guiAppQuitContext = context;
}

extern "C" void
brixGuiAppSetBackgroundCallback(void* ptr, BxGuiActionCallback callback, void* context) {
    guiAppBackgroundCallback = callback;
    guiAppBackgroundContext = context;
}

extern "C" void
brixGuiAppSetOpenCallback(void* ptr, BxGuiOpenCallback callback, void* context) {}

extern "C" void
brixGuiAppSetKeepsDisplayAwake(void* ptr, bool keeps) {
    guiAppKeepsDisplayAwake = keeps;
    guiApplyKeepsDisplayAwake();
}

extern "C" uint32_t
brixGuiSystemAppearance(void) {
    UITraitCollection* traits = UITraitCollection.currentTraitCollection;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:UIWindowScene.class]) {
            traits = static_cast<UIWindowScene*>(scene).traitCollection;
            break;
        }
    }
    return traits.userInterfaceStyle == UIUserInterfaceStyleDark ? 2 : 1;
}

extern "C" void
brixGuiAppSetSystemCallback(void* ptr, BxGuiActionCallback callback, void* context) {
    guiAppSystemCallback = callback;
    guiAppSystemContext = context;
    if (guiSystemObserving) {
        return;
    }
    guiSystemObserving = true;
    for (NSNotificationName name in @[
        UIAccessibilityReduceMotionStatusDidChangeNotification,
        UIAccessibilityDarkerSystemColorsStatusDidChangeNotification,
        UIAccessibilityReduceTransparencyStatusDidChangeNotification,
    ]) {
        [[NSNotificationCenter defaultCenter] addObserverForName:name
                                                          object:nil
                                                           queue:nil
                                                      usingBlock:^(NSNotification*) {
            guiScheduleActionCallback(guiAppSystemCallback, guiAppSystemContext);
        }];
    }
}

static uint32_t
guiPackedColor(UIColor* color) {
    CGFloat red = 0;
    CGFloat green = 0;
    CGFloat blue = 0;
    CGFloat alpha = 0;
    if (![color getRed:&red green:&green blue:&blue alpha:&alpha]) {
        return 0;
    }
    uint32_t packed = 0;
    for (CGFloat component : {alpha, blue, green, red}) {
        packed = (packed << 8) | static_cast<uint32_t>(lround(std::min(std::max(component, 0.0), 1.0) * 255.0));
    }
    return packed;
}

extern "C" uint32_t
brixGuiSystemColor(uint32_t color, uint32_t appearance) {
    UITraitCollection* traits = [UITraitCollection
        traitCollectionWithUserInterfaceStyle:appearance == 2 ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight];
    UIColor* named = nil;
    switch (color) {
        case 1: named = [UIColor tintColor]; break;
        case 2: named = [UIColor systemBlueColor]; break;
        case 3: named = [UIColor systemGray4Color]; break;
        default: break;
    }
    return named ? guiPackedColor([named resolvedColorWithTraitCollection:traits]) : 0;
}

extern "C" uint32_t
brixGuiSystemOptions(void) {
    uint32_t options = 0;
    if (UIAccessibilityIsReduceMotionEnabled()) {
        options |= 1;
    }
    if (UIAccessibilityDarkerSystemColorsEnabled()) {
        options |= 2;
    }
    if (UIAccessibilityIsReduceTransparencyEnabled()) {
        options |= 4;
    }
    return options;
}

// **********************************************************************************************

extern "C" void* brixGuiMenuBarCreate(void) { return nullptr; }
extern "C" void brixGuiMenuBarInstall(void* menuBar) {}
extern "C" void brixGuiMenuBarUninstall(void* menuBar) {}
extern "C" void* brixGuiMenuCreate(const char* title, BxGuiMenuCallback callback, void* context) { return nullptr; }
extern "C" void brixGuiMenuRelease(void* menu) {}
extern "C" void brixGuiMenuRemoveAllItems(void* menu) {}
extern "C" void brixGuiMenuAddItem(
    void* menu,
    const char* title,
    const char* keyCharacters,
    uint32_t keyKind,
    uint64_t modifiers,
    bool enabled,
    bool checked,
    uint32_t role,
    void* submenu
) {}
extern "C" void brixGuiMenuAddSeparator(void* menu) {}
extern "C" void brixGuiMenuPerformRole(uint32_t role) {}

// **********************************************************************************************

extern "C" void* brixGuiStatusItemCreate(void) { return nullptr; }
extern "C" void brixGuiStatusItemRelease(void* item) {}
extern "C" void brixGuiStatusItemSetTitle(void* item, const char* title) {}
extern "C" void brixGuiStatusItemSetAutosaveName(void* item, const char* name) {}
extern "C" void brixGuiStatusItemSetImage(void* item, void* canvas, double width, double height, bool isTemplate) {}
extern "C" void brixGuiStatusItemSetMenu(void* item, void* menu) {}
extern "C" void brixGuiStatusItemSetContextMenu(void* item, void* menu) {}
extern "C" void brixGuiStatusItemSetCallback(void* item, BxGuiStatusItemCallback callback, void* context) {}
extern "C" void brixGuiStatusItemSetPanel(void* item, void* window) {}
extern "C" void* brixGuiStatusItemCreatePopover(void* item, double width, double height) { return nullptr; }
extern "C" void brixGuiStatusItemShowPopover(void* item) {}
extern "C" void brixGuiStatusItemClosePopover(void* item) {}
extern "C" double brixGuiStatusItemGetBound(void* item, uint32_t component) { return 0; }
extern "C" double brixGuiStatusItemGetScreenBound(void* item, uint32_t component) { return 0; }

// **********************************************************************************************

extern "C" void*
brixGuiWindowCreate(const char* title, double width, double height) {
    UIScreen* screen = [UIScreen mainScreen];
    CGSize screenSize = screen.bounds.size;
    BxGuiWindowHost* guiWindow = [[BxGuiWindowHost alloc] init];
    guiWindow->inputStreamState = std::make_shared<BxGuiInputStreamState>();
    guiWindow->contentWidth = std::max(screenSize.width, screenSize.height);
    guiWindow->contentHeight = std::min(screenSize.width, screenSize.height);
    guiWindow->scale = screen.scale;
    CAMetalLayer* layer = [[CAMetalLayer alloc] init];
    layer.frame = CGRectMake(0, 0, guiWindow->contentWidth, guiWindow->contentHeight);
    layer.contentsScale = guiWindow->scale;
    layer.opaque = YES;
    layer.hidden = YES;
    guiWindow->metalLayer = layer;
    guiWindows.push_back(guiWindow);
    if (guiAppLaunched) {
        [guiWindow attachToScreen];
    }
    return (__bridge_retained void*)guiWindow;
}

extern "C" void
brixGuiWindowRetain(void* ptr) {
    if (ptr) {
        CFRetain(ptr);
    }
}

extern "C" void
brixGuiWindowRelease(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->closed = true;
    guiWindow->inputStreamState->cancel();
    [guiWindow cancelRenderFrame];
    [guiWindow finishFrameUpdate];
    guiReplaceRoot(&guiWindow->contentSizeContextRoot, nullptr);
    guiReplaceRoot(&guiWindow->backingScaleContextRoot, nullptr);
    guiReplaceRoot(&guiWindow->pointerContextRoot, nullptr);
    guiReplaceRoot(&guiWindow->touchContextRoot, nullptr);
    guiReplaceRoot(&guiWindow->eventContextRoot, nullptr);
    guiWindows.erase(std::remove(guiWindows.begin(), guiWindows.end(), guiWindow), guiWindows.end());
    [guiWindow detachFromScreen];
    CFBridgingRelease(ptr);
}

extern "C" void*
brixGuiWindowGetNativeWindow(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? (__bridge void*)guiWindow->window : nullptr;
}

extern "C" void
brixGuiWindowShow(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || guiWindow->closed) {
        return;
    }
    guiWindow->shown = true;
    if (guiWindow->window) {
        [guiWindow->window makeKeyAndVisible];
    }
}

extern "C" void
brixGuiWindowClose(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->shown = false;
    guiWindow->window.hidden = YES;
}

extern "C" void brixGuiWindowRejectDeferredClose(void* window) {}
extern "C" void brixGuiWindowSetTitle(void* window, const char* title) {}

extern "C" void
brixGuiWindowSetTransparent(void* ptr, bool windowTransparent, bool contentTransparent) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        guiWindow->metalLayer.opaque = !contentTransparent;
    }
}

extern "C" void brixGuiWindowSetAppearance(void* window, uint32_t appearance) {}
extern "C" void brixGuiWindowSetChrome(void* window, bool titlebarVisible, bool hasStoplightOffset, double x, double y, double zoom) {}
extern "C" double brixGuiWindowGetStoplightEdge(void* window) { return 0; }
extern "C" void brixGuiWindowSetKind(void* window, uint32_t kind) {}
extern "C" void brixGuiWindowSetAllowsZoom(void* window, bool allowsZoom) {}
extern "C" bool brixGuiWindowRequestDrag(void* window, uint64_t token) { return false; }
extern "C" void brixGuiWindowPerformTitlebarDoubleClick(void* window) {}
extern "C" void brixGuiWindowSetContentSize(void* window, double width, double height) {}
extern "C" double brixGuiWindowGetPosition(void* window, uint32_t component) { return 0; }
extern "C" void brixGuiWindowSetPosition(void* window, double x, double y) {}
extern "C" void brixGuiWindowSetMovedCallback(void* window, BxGuiActionCallback callback, void* context) {}

extern "C" double
brixGuiWindowGetContentWidth(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->contentWidth : 0;
}

extern "C" double
brixGuiWindowGetContentHeight(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->contentHeight : 0;
}

extern "C" void*
brixGuiWindowGetDisplayColorSpace(void* ptr) {
    static CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    return colorSpace;
}

extern "C" bool brixGuiWindowDisplayIsWideGamut(void* window) { return false; }

extern "C" void
brixGuiWindowSetContentSizeChangedCallback(void* ptr, BxGuiSizeCallback callback, void* context) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->contentSizeCallback = callback;
    guiWindow->contentSizeContext = context;
    guiReplaceRoot(&guiWindow->contentSizeContextRoot, context);
}

extern "C" double
brixGuiWindowGetBackingScaleFactor(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow->scale > 0 ? guiWindow->scale : 1.0;
}

extern "C" void
brixGuiWindowSetBackingScaleChangedCallback(void* ptr, BxGuiActionCallback callback, void* context) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->backingScaleCallback = callback;
    guiWindow->backingScaleContext = context;
    guiReplaceRoot(&guiWindow->backingScaleContextRoot, context);
}

extern "C" void
brixGuiWindowSetPointerCallback(void* ptr, BxGuiPointerCallback callback, void* context) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->pointerCallback = callback;
    guiWindow->pointerContext = context;
    guiReplaceRoot(&guiWindow->pointerContextRoot, context);
}

extern "C" void
brixGuiWindowSetTouchCallback(void* ptr, BxGuiTouchCallback callback, void* context) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->touchCallback = callback;
    guiWindow->touchContext = context;
    guiReplaceRoot(&guiWindow->touchContextRoot, context);
}

extern "C" void
brixGuiWindowSetEventCallback(void* ptr, BxGuiWindowEventCallback callback, void* context) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->eventCallback = callback;
    guiWindow->eventContext = context;
    guiReplaceRoot(&guiWindow->eventContextRoot, context);
}

extern "C" void brixGuiWindowSetTextActionCallback(void* window, BxGuiWindowTextActionCallback callback, void* context) {}
extern "C" void brixGuiWindowSetTextInputActive(void* window, bool active) {}
extern "C" void brixGuiWindowSetUndoState(void* window, bool canUndo, const char* undoTitle, bool canRedo, const char* redoTitle) {}
extern "C" void brixGuiWindowClearCommandShortcuts(void* window) {}
extern "C" void brixGuiWindowAddCommandShortcut(void* window, uint32_t key, const char* characters, uint64_t modifiers) {}
extern "C" void brixGuiWindowSetTextInputContext(
    void* window,
    const char* before,
    const char* selected,
    double x,
    double y,
    double width,
    double height
) {}
extern "C" void brixGuiWindowDiscardTextComposition(void* window) {}
extern "C" bool brixGuiWindowDebugTextInputActive(void* window) { return false; }
extern "C" void brixGuiWindowSetCursorStyle(void* window, uint32_t cursorStyle, void* cursorImage, bool captured) {}
extern "C" uint32_t brixGuiWindowDebugCursorStyle(void* window) { return 0; }
extern "C" void brixGuiWindowDebugSetPointerOverCanvas(void* window, bool value) {}
extern "C" uint64_t brixGuiWindowDebugCursorRectRefreshCount(void* window) { return 0; }
extern "C" bool brixGuiWindowDebugCursorRectsEnabled(void* window) { return false; }
extern "C" const char* brixGuiWindowGetEventText(void* window) { return ""; }
extern "C" bool brixGuiWindowFocusCanvas(void* window) { return true; }
extern "C" bool brixGuiWindowRequestPeerFocus(void* window, void* peer) { return false; }
extern "C" bool brixGuiWindowPeerHasFocus(void* window, void* peer) { return false; }
extern "C" bool brixGuiWindowDebugPeerAt(void* window, void* peer, double x, double y) { return false; }

extern "C" void
brixGuiWindowBeginFrame(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow beginFrameUpdate];
    }
}

extern "C" void
brixGuiWindowCommitFrame(void* ptr, void* canvas, double damageX, double damageY, double damageWidth, double damageHeight) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow finishFrameUpdate];
    }
}

extern "C" bool brixGuiWindowCopyPresentedPixels(void* window, void* canvas) { return false; }
extern "C" int64_t brixGuiWindowPresentedSurfaceCount(void* window) { return 0; }
extern "C" int64_t brixGuiWindowPresentationOrientationCheck(void* window) { return 0; }

extern "C" void
brixGuiWindowCommitMetalFrame(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow finishFrameUpdate];
    }
}

extern "C" void*
brixGuiWindowGetMetalLayer(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && !guiWindow->closed ? (__bridge void*)guiWindow->metalLayer : nullptr;
}

extern "C" bool
brixGuiWindowSetCanvasLayer(void* ptr, void* layerPointer) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    CALayer* layer = (__bridge CALayer*)layerPointer;
    if (!guiWindow || guiWindow->closed || layer != guiWindow->metalLayer) {
        return false;
    }
    layer.hidden = NO;
    return true;
}

extern "C" void
brixGuiWindowResetCanvasLayer(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        guiWindow->metalLayer.hidden = YES;
    }
}

extern "C" int32_t
brixGuiWindowPresentationVisibility(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow->window || !guiWindow->shown) {
        return BxGuiPresentationHidden;
    }
    return guiAppVisibility.load();
}

extern "C" void
brixGuiWindowSetPresentationPending(void* ptr, bool pending) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->presentationPending = pending;
    if (!pending && guiWindow->presentationHeldFrame) {
        guiWindow->presentationHeldFrame = false;
        if (guiWindow->renderScheduled && !guiWindow->closed) {
            guiFrameSchedulerRequest(guiWindow);
        }
    }
}

extern "C" int32_t
brixGuiWindowGetMaximumFramesPerSecond(void* window) {
    return static_cast<int32_t>([UIScreen mainScreen].maximumFramesPerSecond);
}

extern "C" void
brixGuiWindowAbortFrame(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow finishFrameUpdate];
    }
}

extern "C" void
brixGuiWindowRequestFrame(void* ptr, BxGuiActionCallback callback, void* context, uint64_t notBeforeNanoseconds) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow && callback) {
        [guiWindow requestRenderFrameWithCallback:callback context:context notBefore:notBeforeNanoseconds];
    }
}

extern "C" void
brixGuiWindowCancelFrame(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow cancelRenderFrame];
    }
}

extern "C" uint64_t
brixGuiWindowGetLastFrameTargetTimeNanoseconds(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->lastFrameTargetTimeNanoseconds : 0;
}

extern "C" uint64_t
brixGuiWindowGetLastFrameRefreshIntervalNanoseconds(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->lastFrameRefreshIntervalNanoseconds : 0;
}

extern "C" double
brixGuiWindowGetLastFrameEventTime(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->lastFrameEventTimeSeconds : 0;
}

extern "C" double
brixGuiEventClockSeconds(void) {
    return [[NSProcessInfo processInfo] systemUptime];
}

extern "C" uint64_t brixGuiWindowDebugViewCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugLayerCount(void* window) { return 0; }
extern "C" void brixGuiWindowDebugInjectContentSize(void* window, double width, double height) {}
extern "C" bool brixGuiWindowDebugInjectTransactionalContentSize(
    void* window,
    double width,
    double height,
    uint64_t budgetNanoseconds
) { return false; }
extern "C" void brixGuiWindowDebugInjectBackingScaleChange(void* window) {}
extern "C" void brixGuiWindowDebugSendMouseClick(void* window, double x, double y) {}
extern "C" void brixGuiWindowDebugPostMouseClick(void* window, double x, double y) {}
extern "C" void brixGuiAppDebugMainLoopProbeStart(uint64_t intervalNanoseconds) {}
extern "C" int64_t brixGuiAppDebugMainLoopProbeValue(int32_t field) { return 0; }
extern "C" void brixGuiWindowDebugInjectPointerEvent(
    void* window,
    uint32_t eventKind,
    uint64_t pointerId,
    uint32_t deviceKind,
    uint32_t button,
    uint64_t buttons,
    uint64_t modifiers,
    double x,
    double y,
    double deltaX,
    double deltaY,
    double orderingTime,
    uint32_t clickCount,
    bool isPrecise,
    uint64_t phase,
    uint64_t momentumPhase
) {}

extern "C" uint64_t
brixGuiWindowDebugInputEventCount(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->eventsReceived : 0;
}

extern "C" uint64_t
brixGuiWindowDebugInputCallbacksScheduled(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->callbacksScheduled : 0;
}

extern "C" uint64_t
brixGuiWindowDebugInputCallbacksServiced(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->callbacksServiced : 0;
}

extern "C" uint64_t
brixGuiWindowDebugInputCoalescedCount(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->eventsCoalesced : 0;
}

extern "C" uint64_t
brixGuiWindowDebugInputBarrierCount(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->barriersReceived : 0;
}

extern "C" uint64_t
brixGuiWindowDebugInputPendingCount(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->pendingCallbacks : 0;
}

extern "C" uint64_t
brixGuiWindowDebugInputPendingHighWaterMark(void* ptr) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->pendingHighWaterMark : 0;
}

extern "C" uint64_t brixGuiWindowDebugResizeNotificationCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugResizeCallbacksServiced(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugResizeCoalescedCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugResizePendingCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugResizePendingHighWaterMark(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugResizeFrameCommitCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugResizeFramesPerCallbackHighWaterMark(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugResizeObsoleteSizeRenderCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugBackingScaleNotificationCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugBackingScaleCallbacksServiced(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugBackingScaleCoalescedCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugBackingScalePendingCount(void* window) { return 0; }
extern "C" uint64_t brixGuiWindowDebugBackingScalePendingHighWaterMark(void* window) { return 0; }

// **********************************************************************************************

extern "C" void* brixGuiMaterialCreate(void) { return nullptr; }
extern "C" void brixGuiMaterialRelease(void* material) {}
extern "C" void brixGuiMaterialSetContainer(void* material, void* window, void* boxHost, int32_t depth) {}
extern "C" void brixGuiMaterialSetFrame(
    void* material,
    double x,
    double y,
    double width,
    double height,
    uint32_t kind,
    double opacity,
    bool visible
) {}
extern "C" void brixGuiMaterialSetMask(void* material, void* canvas, double width, double height, double cap) {}
extern "C" void* brixGuiBoxHostCreate(void* window) { return nullptr; }
extern "C" void brixGuiBoxHostRelease(void* host) {}
extern "C" void brixGuiBoxHostSetParent(void* host, void* parentHost) {}
extern "C" void brixGuiBoxHostSetFrame(
    void* host,
    double x,
    double y,
    double width,
    double height,
    double a,
    double b,
    double c,
    double d,
    double contentX,
    double contentY,
    double contentWidth,
    double contentHeight,
    double scrollX,
    double scrollY,
    bool visible
) {}
extern "C" void brixGuiBoxHostSetCornerRadii(void* host, double left, double top, double right, double bottom) {}
extern "C" bool brixGuiBoxHostHasParent(void* host, void* parentHost) { return false; }
extern "C" bool brixGuiBoxHostFrameEquals(
    void* host,
    double x,
    double y,
    double width,
    double height,
    double contentX,
    double contentY,
    double contentWidth,
    double contentHeight
) { return false; }
extern "C" bool brixGuiBoxHostContentOriginEquals(void* host, double x, double y) { return false; }
extern "C" bool brixGuiBoxHostClips(void* host) { return false; }
extern "C" bool brixGuiBoxHostHasRoundedClip(void* host) { return false; }
extern "C" bool brixGuiBoxHostIsAccessibilityTransparent(void* host) { return true; }

// **********************************************************************************************

extern "C" void brixGuiWindowAddPeer(void* window, void* peer, bool ownsKeys, bool ownsCursor) {}
extern "C" void brixGuiPeerSetContainer(void* window, void* peer, void* boxHost) {}
extern "C" void brixGuiPeerDetach(void* peer) {}
extern "C" void brixGuiPeerSetFrame(
    void* peer,
    double x,
    double y,
    double width,
    double height,
    double a,
    double b,
    double c,
    double d
) {}
extern "C" void brixGuiPeerSetVisible(void* peer, bool visible) {}
extern "C" void brixGuiPeerSetOpacity(void* peer, double opacity) {}
extern "C" void brixGuiPeerSetPassesScroll(void* peer, bool passesScroll) {}
extern "C" void brixGuiPeerDispatchKeyEvent(void* peer, void* event) {}

// **********************************************************************************************

extern "C" void* brixGuiPopupCreate(void* window) { return nullptr; }
extern "C" void brixGuiPopupRelease(void* popup) {}
extern "C" void* brixGuiPopupGetHost(void* popup) { return nullptr; }
extern "C" void* brixGuiPopupGetMetalLayer(void* popup) { return nullptr; }
extern "C" bool brixGuiPopupSetCanvasLayer(void* popup, void* layer) { return false; }
extern "C" void brixGuiPopupResetCanvasLayer(void* popup) {}
extern "C" int32_t brixGuiPopupPresentationVisibility(void* popup) { return BxGuiPresentationHidden; }
extern "C" void* brixGuiPopupGetWindow(void* popup) { return nullptr; }
extern "C" void brixGuiPopupSetFrame(void* popup, double x, double y, double width, double height) {}
extern "C" void brixGuiPopupSetShadow(void* popup, bool shadow) {}
extern "C" void brixGuiPopupSetPresentationPending(void* popup, bool pending) {}
extern "C" void brixGuiPopupSetShown(void* popup, bool shown) {}
extern "C" bool brixGuiPopupIsShown(void* popup) { return false; }
extern "C" void brixGuiPopupOrderFront(void* popup) {}
extern "C" void brixGuiPopupSetPassesPointer(void* popup, bool passesPointer) {}
extern "C" void brixGuiPopupSetConstrained(void* popup, bool constrained) {}
extern "C" void brixGuiPopupCommitFrame(void* popup, void* canvas) {}
extern "C" double brixGuiPopupGetScreenBound(void* popup, uint32_t component) { return 0; }

extern "C" double
brixGuiWindowGetScreenBound(void* ptr, uint32_t component) {
    BxGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return 0;
    }
    switch (component) {
        case 2: return guiWindow->contentWidth;
        case 3: return guiWindow->contentHeight;
        default: return 0;
    }
}

// **********************************************************************************************

extern "C" uint64_t brixGuiClipboardChangeCount(const char* name) { return 0; }
extern "C" uint64_t brixGuiClipboardItemCount(const char* name) { return 0; }
extern "C" const char* brixGuiClipboardItemTypes(const char* name, uint64_t index) { return nullptr; }
extern "C" const char* brixGuiClipboardItemString(const char* name, uint64_t index, const char* type) { return nullptr; }
extern "C" void* brixGuiClipboardItemData(const char* name, uint64_t index, const char* type) { return nullptr; }
extern "C" const char* brixGuiClipboardItemPath(const char* name, uint64_t index) { return nullptr; }
extern "C" uint64_t brixGuiClipboardDataLength(void* data) { return 0; }
extern "C" void brixGuiClipboardDataCopy(void* data, uint8_t* bytes, uint64_t length) {}
extern "C" void brixGuiClipboardDataRelease(void* data) {}
extern "C" void* brixGuiClipboardWriterCreate(void) { return nullptr; }
extern "C" void brixGuiClipboardWriterAddItem(void* writer) {}
extern "C" bool brixGuiClipboardWriterSetString(void* writer, const char* type, const char* value) { return false; }
extern "C" bool brixGuiClipboardWriterSetData(void* writer, const char* type, const uint8_t* bytes, uint64_t length) { return false; }
extern "C" bool brixGuiClipboardWriterSetPath(void* writer, const char* path) { return false; }
extern "C" bool brixGuiClipboardWriterCommit(void* writer, const char* name) { return false; }
extern "C" void brixGuiClipboardWriterRelease(void* writer) {}
extern "C" void brixGuiClipboardDiscard(const char* name) {}
extern "C" const char* brixGuiWindowDropPasteboardName(void*) { return nullptr; }
extern "C" void brixGuiWindowSetDropOperation(void* window, uint32_t operation) {}
extern "C" bool brixGuiWindowBeginExportDrag(void* window, void* writer, void* canvas, double x, double y, double width, double height) { return false; }
extern "C" uint64_t brixGuiDragWaitPromisedFiles(uint64_t sequence, uint64_t timeoutMilliseconds) { return 0; }
extern "C" const char* brixGuiDragPromisedFile(uint64_t sequence, uint64_t index) { return nullptr; }
extern "C" void brixGuiDragReleasePromisedFiles(uint64_t sequence) {}
extern "C" bool brixGuiTypeConforms(const char* type, const char* parent) { return false; }
extern "C" const char* brixGuiTypeOfPath(const char* path) { return nullptr; }

// **********************************************************************************************

extern "C" bool brixGuiTrashPath(const char* path) { return false; }
extern "C" bool brixGuiRevealPath(const char* path) { return false; }
extern "C" bool brixGuiShowPathInfo(const char* path) { return false; }

extern "C" bool
brixGuiOpenURL(const char* url) {
    NSURL* target = url ? [NSURL URLWithString:[NSString stringWithUTF8String:url]] : nil;
    if (!target) {
        return false;
    }
    [[UIApplication sharedApplication] openURL:target options:@{} completionHandler:nil];
    return true;
}

// **********************************************************************************************

extern "C" void* brixGuiTextInputCreate(
    void* window,
    double x,
    double y,
    double width,
    double height,
    const char* text,
    const char* placeholder
) { return nullptr; }
extern "C" void brixGuiTextInputRelease(void* input) {}
extern "C" void brixGuiTextInputSetFrame(
    void* input,
    double x,
    double y,
    double width,
    double height,
    double a,
    double b,
    double c,
    double d
) {}
extern "C" void brixGuiTextInputSetText(void* input, const char* text) {}
extern "C" const char* brixGuiTextInputGetText(void* input) { return ""; }
extern "C" void brixGuiTextInputSetEventCallback(void* input, BxGuiTextInputEventCallback callback, void* context) {}
extern "C" void brixGuiTextInputSetPlaceholder(void* input, const char* placeholder) {}
extern "C" void brixGuiTextInputSetTextColor(void* input, double red, double green, double blue, double alpha) {}
extern "C" void brixGuiTextInputSetFont(void* input, const char* family, double size, double weight) {}
extern "C" void brixGuiTextInputSetAlignment(void* input, int32_t alignment) {}
extern "C" void brixGuiTextInputSetEnabled(void* input, bool enabled) {}
extern "C" void brixGuiTextInputSetEditable(void* input, bool editable) {}
extern "C" void brixGuiTextInputSetSecure(void* input, bool secure) {}
extern "C" void brixGuiTextInputSetMaxLength(void* input, uint32_t maxLength) {}
extern "C" void brixGuiTextInputSetVisible(void* input, bool visible) {}
extern "C" void brixGuiTextInputSetOpacity(void* input, double opacity) {}
extern "C" double brixGuiTextInputGetOpacity(void* input) { return 0; }
extern "C" void brixGuiTextInputSetCursorStyle(void* input, uint32_t cursorStyle, void* cursorImage) {}
extern "C" uint32_t brixGuiTextInputDebugCursorStyle(void* input) { return 0; }
extern "C" void brixGuiTextInputSetContainer(void* input, void* boxHost) {}
extern "C" bool brixGuiTextInputHasContainer(void* input, void* boxHost) { return false; }
extern "C" double brixGuiTextInputGetFittingWidth(void* input) { return 0; }
extern "C" double brixGuiTextInputGetFittingHeight(void* input) { return 0; }
extern "C" double brixGuiTextInputGetFittingHeightForWidth(void* input, double width) { return 0; }
extern "C" void brixGuiTextInputSetWraps(void* input, bool wraps) {}
extern "C" void brixGuiTextInputSetSelection(void* input, const char* before, const char* selected) {}
extern "C" void brixGuiTextInputSetPassedKeys(void* input, uint32_t keys) {}
extern "C" void brixGuiTextInputSetLineMetrics(void* input, double lineHeight, double lineSpacing) {}
extern "C" void brixGuiTextInputSetTextInsets(void* input, double top, double bottom) {}
extern "C" void brixGuiTextInputSetHitRect(void* input, double x, double y, double width, double height) {}
extern "C" double brixGuiTextInputDebugLineMetric(void* input, uint32_t metric) { return 0; }
extern "C" void brixGuiTextInputReplaceText(void* input, const char* text) {}
extern "C" void brixGuiTextInputPasteText(void* input) {}
extern "C" void brixGuiTextInputMoveCaretToEnd(void* input) {}
extern "C" void brixGuiTextInputSyncText(void* input, const char* text) {}
extern "C" bool brixGuiTextInputCanUndo(void* input, bool redoes) { return false; }
extern "C" void brixGuiTextInputUndo(void* input, bool redoes) {}
extern "C" double brixGuiTextInputGetTextOriginX(void* input) { return 0; }
extern "C" double brixGuiTextInputGetTextBaseline(void* input) { return 0; }
extern "C" void brixGuiTextInputDebugInjectEvent(void* input, uint32_t eventKind, const char* text) {}

// **********************************************************************************************

extern "C" void* brixGuiCursorCreate(void* canvas, double hotspotX, double hotspotY) { return nullptr; }
extern "C" void brixGuiCursorRelease(void* cursor) {}
extern "C" double brixGuiCursorWidth(void* cursor) { return 0; }
extern "C" double brixGuiCursorHotspotX(void* cursor) { return 0; }
extern "C" double brixGuiCursorHotspotY(void* cursor) { return 0; }

extern "C" void* brixGuiWindowCreateWithKind(const char* title, double width, double height, uint32_t kind) {
    return brixGuiWindowCreate(title, width, height);
}
extern "C" void brixGuiWindowSetHasShadow(void* window, bool hasShadow) {}
extern "C" void brixGuiWindowHide(void* window) {}
extern "C" bool brixGuiWindowIsVisible(void* window) { return false; }
extern "C" double brixGuiMouseScreenBound(uint32_t component) { return 0; }
extern "C" void brixGuiMaterialSetCornerRadius(void* material, double radius) {}
extern "C" bool brixGuiOpenPath(const char* path) { return false; }
extern "C" bool brixGuiOpenPathWithApp(const char* path, const char* appPath) { return false; }
extern "C" const char* brixGuiPathDisplayName(const char* path) { return ""; }
extern "C" void* brixGuiPathIconCanvas(const char* path, double size, double scale) { return nullptr; }
extern "C" void* brixGuiHotKeyRegister(
    uint32_t nativeKeyCode,
    uint64_t modifiers,
    BxGuiActionCallback callback,
    void* context
) { return nullptr; }
extern "C" void brixGuiHotKeyUnregister(void* hotKey) {}
extern "C" void* brixGuiModifierDoubleTapRegister(
    uint64_t modifiers,
    BxGuiActionCallback callback,
    void* context
) { return nullptr; }
extern "C" void brixGuiModifierDoubleTapUnregister(void* monitor) {}
extern "C" void* brixGuiQuickLookCreate(void* window) { return nullptr; }
extern "C" void brixGuiQuickLookRelease(void* quickLook) {}
extern "C" void brixGuiQuickLookSetPath(void* quickLook, const char* path) {}
extern "C" void brixGuiQuickLookPanelShow(void* window, const char* path) {}
extern "C" void brixGuiQuickLookPanelUpdate(void* window, const char* path) {}
extern "C" void brixGuiQuickLookPanelClose(void* window) {}
extern "C" bool brixGuiQuickLookPanelIsVisible(void) { return false; }
extern "C" void brixGuiQuickLookExtensionRun(
    BxGuiQuickLookPreviewCallback previewCallback,
    BxGuiQuickLookCloseCallback closeCallback,
    void* context
) {}
extern "C" void brixGuiQuickLookExtensionFinish(
    void* completion,
    const char* errorMessage
) {}
