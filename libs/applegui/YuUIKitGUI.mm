#include "YuAppleGUI/YuAppleGUI.h"
#include "YuAppleGUI/YuGuiInputCoalescer.h"
#include "YuCore/YuCore.h"
#include "YuEvents/YuEvents.h"

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

@class YuGuiWindowHost;

struct YuGuiFrameTiming {
    uint64_t outputTimeNanoseconds;
    uint64_t targetTimeNanoseconds;
    uint64_t refreshIntervalNanoseconds;
    double eventTimeSeconds;
};

struct YuGuiInputStreamState {
    YuGuiInputCoalescer coalescer;
    bool canceled = false;
    uint64_t eventsReceived = 0;
    uint64_t eventsCoalesced = 0;
    uint64_t callbacksScheduled = 0;
    uint64_t callbacksServiced = 0;
    uint64_t barriersReceived = 0;
    uint64_t pendingCallbacks = 0;
    uint64_t pendingHighWaterMark = 0;

    bool tryMerge(const YuGuiInputEvent& event) {
        ++eventsReceived;
        if (event.inputKind == YuGuiInputKind::Barrier) {
            ++barriersReceived;
        }
        if (coalescer.tryMerge(event)) {
            ++eventsCoalesced;
            return true;
        }
        return false;
    }

    void append(YuGuiInputEvent& event) {
        coalescer.append(event);
        ++callbacksScheduled;
        ++pendingCallbacks;
        pendingHighWaterMark = std::max(pendingHighWaterMark, pendingCallbacks);
    }

    bool begin(YuGuiInputEvent& event) {
        coalescer.begin(event);
        if (pendingCallbacks) {
            --pendingCallbacks;
        }
        return !canceled;
    }

    void abandon(YuGuiInputEvent& event) {
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

static std::atomic<int32_t> guiAppVisibility(YuGuiPresentationHidden);
static bool guiAppLaunched = false;
static std::vector<YuGuiWindowHost*> guiWindows;
static YuGuiActionCallback guiAppQuitCallback = nullptr;
static void* guiAppQuitContext = nullptr;
static YuGuiActionCallback guiAppBackgroundCallback = nullptr;
static void* guiAppBackgroundContext = nullptr;
static YuGuiActionCallback guiAppSystemCallback = nullptr;
static void* guiAppSystemContext = nullptr;
static bool guiSystemObserving = false;
static UIBackgroundTaskIdentifier guiAppBackgroundTask = UIBackgroundTaskInvalid;
static bool guiAppKeepsDisplayAwake = false;

static void guiFrameSchedulerRequest(YuGuiWindowHost* guiWindow);
static void guiFrameSchedulerCancel(YuGuiWindowHost* guiWindow);
static void guiFrameSchedulerResume(void);

static void
guiReplaceRoot(void** slot, void* context) {
    void* root = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    if (*slot) {
        yumiRemoveRoot(*slot);
    }
    *slot = root;
}

// **********************************************************************************************

struct YuGuiCallbackRecord {
    explicit YuGuiCallbackRecord(void* aContext):
        context(aContext),
        contextRoot(aContext ? yumiAddRoot(static_cast<char*>(aContext)) : nullptr)
    {}

    virtual ~YuGuiCallbackRecord() {
        if (contextRoot) {
            yumiRemoveRoot(contextRoot);
        }
    }

    void* context;
    void* contextRoot;
};

static void
guiFinishCallbackTask(YuGuiCallbackRecord* record) {
    delete record;
    yumiTerminateActiveTask();
}

struct YuGuiActionCallbackRecord : YuGuiCallbackRecord {
    YuGuiActionCallbackRecord(
        YuGuiActionCallback aCallback,
        void* aContext,
        uint64_t anIdleDeadlineNanoseconds,
        bool anIsDisplayFrame):
        YuGuiCallbackRecord(aContext),
        callback(aCallback),
        idleDeadlineNanoseconds(anIdleDeadlineNanoseconds),
        isDisplayFrame(anIsDisplayFrame) {}
    YuGuiActionCallback callback;
    uint64_t idleDeadlineNanoseconds;
    bool isDisplayFrame;
};

static const uint64_t kGuiOverrunFrameCollectorNanoseconds = 1000000ull;

static void
guiRunActionCallback(void* closure) {
    YuGuiActionCallbackRecord* record = static_cast<YuGuiActionCallbackRecord*>(closure);
    if (!yumiProcessTerminationRequested()) {
        record->callback(record->context);
        uint64_t now = yumiMonotonicTimeNanoseconds();
        if (record->idleDeadlineNanoseconds > now) {
            yumiGCFrameBudget(record->idleDeadlineNanoseconds - now);
        } else if (record->isDisplayFrame) {
            yumiGCFrameBudget(kGuiOverrunFrameCollectorNanoseconds);
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
    YuGuiActionCallbackRecord* record = static_cast<YuGuiActionCallbackRecord*>(closure);
    if (!yumiProcessTerminationRequested()) {
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
    YuGuiActionCallbackRecord* record =
        new YuGuiActionCallbackRecord(guiAppBackgroundCallback, guiAppBackgroundContext, 0, false);
    if (!yumiScheduleMainTaskFromNative(guiRunBackgroundCallback, record)) {
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
    YuGuiActionCallback callback,
    void* context,
    uint64_t idleDeadlineNanoseconds = 0,
    bool isDisplayFrame = false) {
    if (!callback) {
        return;
    }
    YuGuiActionCallbackRecord* record =
        new YuGuiActionCallbackRecord(callback, context, idleDeadlineNanoseconds, isDisplayFrame);
    if (!yumiScheduleMainTaskFromNative(guiRunActionCallback, record)) {
        delete record;
    }
}

struct YuGuiPointerCallbackRecord : YuGuiCallbackRecord {
    YuGuiPointerCallbackRecord(
        YuGuiPointerCallback aCallback,
        void* aContext,
        std::shared_ptr<YuGuiInputStreamState> aStream,
        YuGuiInputEvent&& anEvent):
        YuGuiCallbackRecord(aContext),
        callback(aCallback),
        stream(std::move(aStream)),
        event(std::move(anEvent)),
        pending(true)
    {
        stream->append(event);
    }

    ~YuGuiPointerCallbackRecord() override {
        if (pending) {
            stream->abandon(event);
        }
    }

    YuGuiPointerCallback callback;
    std::shared_ptr<YuGuiInputStreamState> stream;
    YuGuiInputEvent event;
    bool pending;
};

static void
guiRunPointerCallback(void* closure) {
    YuGuiPointerCallbackRecord* record = static_cast<YuGuiPointerCallbackRecord*>(closure);
    record->pending = false;
    bool active = record->stream->begin(record->event);
    if (active && !yumiProcessTerminationRequested()) {
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
    const std::shared_ptr<YuGuiInputStreamState>& stream,
    uintptr_t source,
    YuGuiPointerCallback callback,
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
    YuGuiInputKind inputKind = eventKind == 0
        ? YuGuiInputKind::PointerMove
        : eventKind == 3
            ? YuGuiInputKind::PointerDrag
            : YuGuiInputKind::Barrier;
    YuGuiInputEvent event(inputKind, source);
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
    YuGuiPointerCallbackRecord* record =
        new YuGuiPointerCallbackRecord(callback, context, stream, std::move(event));
    if (!yumiScheduleMainTaskFromNative(guiRunPointerCallback, record)) {
        delete record;
    }
}

struct YuGuiTouchRecord {
    uint32_t phase;
    uint64_t touchId;
    double x;
    double y;
    double orderingTime;
    uint32_t tapCount;
    double radius;
};

struct YuGuiTouchCallbackRecord : YuGuiCallbackRecord {
    YuGuiTouchCallbackRecord(
        YuGuiTouchCallback aCallback,
        void* aContext,
        std::shared_ptr<YuGuiInputStreamState> aStream,
        std::vector<YuGuiTouchRecord>&& aTouches):
        YuGuiCallbackRecord(aContext),
        callback(aCallback),
        stream(std::move(aStream)),
        touches(std::move(aTouches))
    {}

    YuGuiTouchCallback callback;
    std::shared_ptr<YuGuiInputStreamState> stream;
    std::vector<YuGuiTouchRecord> touches;
};

static void
guiRunTouchCallback(void* closure) {
    YuGuiTouchCallbackRecord* record = static_cast<YuGuiTouchCallbackRecord*>(closure);
    for (const YuGuiTouchRecord& touch : record->touches) {
        if (record->stream->canceled || yumiProcessTerminationRequested()) {
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
    const std::shared_ptr<YuGuiInputStreamState>& stream,
    YuGuiTouchCallback callback,
    void* context,
    std::vector<YuGuiTouchRecord>&& touches) {
    if (!stream || stream->canceled || !callback || touches.empty()) {
        return;
    }
    YuGuiTouchCallbackRecord* record =
        new YuGuiTouchCallbackRecord(callback, context, stream, std::move(touches));
    if (!yumiScheduleMainTaskFromNative(guiRunTouchCallback, record)) {
        delete record;
    }
}

struct YuGuiSizeCallbackRecord : YuGuiCallbackRecord {
    YuGuiSizeCallbackRecord(YuGuiWindowHost* aGuiWindow, YuGuiSizeCallback aCallback, void* aContext);
    ~YuGuiSizeCallbackRecord() override;
    YuGuiWindowHost* guiWindow;
    YuGuiSizeCallback callback;
};

struct YuGuiWindowEventCallbackRecord : YuGuiCallbackRecord {
    YuGuiWindowEventCallbackRecord(
        YuGuiWindowHost* aGuiWindow,
        YuGuiWindowEventCallback aCallback,
        void* aContext,
        uint32_t anEventKind);
    ~YuGuiWindowEventCallbackRecord() override;
    YuGuiWindowHost* guiWindow;
    YuGuiWindowEventCallback callback;
    uint32_t eventKind;
};

struct YuGuiRenderFrameCallbackRecord : YuGuiActionCallbackRecord {
    YuGuiRenderFrameCallbackRecord(
        YuGuiWindowHost* aGuiWindow,
        YuGuiActionCallback aCallback,
        void* aContext,
        uint64_t anIdleDeadlineNanoseconds);
    ~YuGuiRenderFrameCallbackRecord() override;
    YuGuiWindowHost* guiWindow;
};

// **********************************************************************************************

@interface YuGuiRootView : UIView

@property(nonatomic, weak) YuGuiWindowHost* guiWindow;

@end

@interface YuGuiRootViewController : UIViewController
@end

@interface YuGuiWindowHost : NSObject {
@public
    std::shared_ptr<YuGuiInputStreamState> inputStreamState;
    CAMetalLayer* metalLayer;
    UIWindow* window;
    YuGuiRootView* rootView;
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
    YuGuiActionCallback renderCallback;
    void* renderContext;
    void* renderContextRoot;
    YuGuiSizeCallback contentSizeCallback;
    void* contentSizeContext;
    void* contentSizeContextRoot;
    YuGuiActionCallback backingScaleCallback;
    void* backingScaleContext;
    void* backingScaleContextRoot;
    YuGuiPointerCallback pointerCallback;
    void* pointerContext;
    void* pointerContextRoot;
    YuGuiTouchCallback touchCallback;
    void* touchContext;
    void* touchContextRoot;
    YuGuiWindowEventCallback eventCallback;
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
- (void)requestRenderFrameWithCallback:(YuGuiActionCallback)callback
                               context:(void*)context
                             notBefore:(uint64_t)notBeforeNanoseconds;
- (void)fireRenderFrameWithTiming:(YuGuiFrameTiming)timing;
- (void)dispatchWindowEventKind:(uint32_t)kind;
- (void)touchesBegan:(NSSet<UITouch*>*)touches;
- (void)touchesMoved:(NSSet<UITouch*>*)touches;
- (void)touchesEnded:(NSSet<UITouch*>*)touches;
- (void)touchesCancelled:(NSSet<UITouch*>*)touches;

@end

YuGuiSizeCallbackRecord::YuGuiSizeCallbackRecord(
    YuGuiWindowHost* aGuiWindow, YuGuiSizeCallback aCallback, void* aContext):
    YuGuiCallbackRecord(aContext),
    guiWindow(aGuiWindow),
    callback(aCallback)
{}

YuGuiSizeCallbackRecord::~YuGuiSizeCallbackRecord() {
}

YuGuiWindowEventCallbackRecord::YuGuiWindowEventCallbackRecord(
    YuGuiWindowHost* aGuiWindow,
    YuGuiWindowEventCallback aCallback,
    void* aContext,
    uint32_t anEventKind):
    YuGuiCallbackRecord(aContext),
    guiWindow(aGuiWindow),
    callback(aCallback),
    eventKind(anEventKind)
{}

YuGuiWindowEventCallbackRecord::~YuGuiWindowEventCallbackRecord() {
}

YuGuiRenderFrameCallbackRecord::YuGuiRenderFrameCallbackRecord(
    YuGuiWindowHost* aGuiWindow,
    YuGuiActionCallback aCallback,
    void* aContext,
    uint64_t anIdleDeadlineNanoseconds):
    YuGuiActionCallbackRecord(aCallback, aContext, anIdleDeadlineNanoseconds, true),
    guiWindow(aGuiWindow)
{}

YuGuiRenderFrameCallbackRecord::~YuGuiRenderFrameCallbackRecord() {
}

static void
guiRunRenderFrameCallback(void* closure) {
    YuGuiRenderFrameCallbackRecord* record = static_cast<YuGuiRenderFrameCallbackRecord*>(closure);
    YuGuiWindowHost* guiWindow = record->guiWindow;
    if (guiAppVisibility.load() == YuGuiPresentationHidden) {
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
    YuGuiWindowEventCallbackRecord* record = static_cast<YuGuiWindowEventCallbackRecord*>(closure);
    if (!yumiProcessTerminationRequested() && !record->guiWindow->closed) {
        record->callback(record->context, record->eventKind, 0, 0, 0, false, "");
    }
    guiFinishCallbackTask(record);
}

static void
guiRunSizeCallback(void* closure) {
    YuGuiSizeCallbackRecord* record = static_cast<YuGuiSizeCallbackRecord*>(closure);
    YuGuiWindowHost* guiWindow = record->guiWindow;
    guiWindow->contentSizePending = false;
    if (!yumiProcessTerminationRequested() && !guiWindow->closed) {
        record->callback(record->context, guiWindow->contentWidth, guiWindow->contentHeight);
    }
    guiFinishCallbackTask(record);
}

static void
guiScheduleSizeCallback(YuGuiWindowHost* guiWindow) {
    if (!guiWindow->contentSizeCallback || guiWindow->closed || guiWindow->contentSizePending) {
        return;
    }
    guiWindow->contentSizePending = true;
    YuGuiSizeCallbackRecord* record = new YuGuiSizeCallbackRecord(
        guiWindow, guiWindow->contentSizeCallback, guiWindow->contentSizeContext);
    if (!yumiScheduleMainTaskFromNative(guiRunSizeCallback, record)) {
        guiWindow->contentSizePending = false;
        delete record;
    }
}

@implementation YuGuiRootView

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

@implementation YuGuiRootViewController

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

@implementation YuGuiWindowHost

- (void)attachToScreen {
    if (window || closed) {
        return;
    }
    UIWindow* screenWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    YuGuiRootViewController* controller = [[YuGuiRootViewController alloc] init];
    YuGuiRootView* view = [[YuGuiRootView alloc] initWithFrame:[screenWindow bounds]];
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
    YuGuiWindowEventCallbackRecord* record =
        new YuGuiWindowEventCallbackRecord(self, eventCallback, eventContext, kind);
    if (!yumiScheduleMainTaskFromNative(guiRunWindowEventCallback, record)) {
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

- (void)requestRenderFrameWithCallback:(YuGuiActionCallback)callback
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

- (void)fireRenderFrameWithTiming:(YuGuiFrameTiming)timing {
    lastFrameTargetTimeNanoseconds = timing.targetTimeNanoseconds;
    lastFrameRefreshIntervalNanoseconds = timing.refreshIntervalNanoseconds;
    lastFrameEventTimeSeconds = timing.eventTimeSeconds;

    if (presentationPending && renderScheduled && !closed) {
        presentationHeldFrame = true;
        return;
    }
    YuGuiActionCallback callback = renderScheduled && !closed ? renderCallback : nullptr;
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
        YuGuiRenderFrameCallbackRecord* record =
            new YuGuiRenderFrameCallbackRecord(self, callback, context, idleDeadlineNanoseconds);
        if (!yumiScheduleMainTaskFromNative(guiRunRenderFrameCallback, record)) {
            delete record;
        }
    }
    if (root) {
        yumiRemoveRoot(root);
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
    std::vector<YuGuiTouchRecord> records;
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

@interface YuGuiAppHost : NSObject
@end

@implementation YuGuiAppHost
@end

@interface YuGuiAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation YuGuiAppDelegate

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options {
    guiAppLaunched = true;
    guiAppVisibility.store(YuGuiPresentationUnoccluded);
    guiApplyKeepsDisplayAwake();
    std::vector<YuGuiWindowHost*> windows = guiWindows;
    for (YuGuiWindowHost* guiWindow : windows) {
        [guiWindow attachToScreen];
    }
    guiFrameSchedulerResume();
    return YES;
}

- (UIWindow*)window {
    return guiWindows.empty() ? nil : guiWindows.front()->window;
}

- (void)applicationDidBecomeActive:(UIApplication*)application {
    guiAppVisibility.store(YuGuiPresentationUnoccluded);
    guiApplyKeepsDisplayAwake();
    std::vector<YuGuiWindowHost*> windows = guiWindows;
    for (YuGuiWindowHost* guiWindow : windows) {
        if (guiWindow->shown) {
            [guiWindow dispatchWindowEventKind:9];
        }
    }
}

- (void)applicationWillResignActive:(UIApplication*)application {
    guiAppVisibility.store(YuGuiPresentationOccluded);
}

- (void)applicationDidEnterBackground:(UIApplication*)application {
    guiAppVisibility.store(YuGuiPresentationHidden);
    guiScheduleBackgroundCallback(application);
}

- (void)applicationWillEnterForeground:(UIApplication*)application {
    guiAppVisibility.store(YuGuiPresentationOccluded);
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

static void
guiMainQueueSourcePerform(void*) {
    uint64_t now = yumiMonotonicTimeNanoseconds();
    uint32_t result = yumiDrainMainTaskQueue(kGuiMainQueueTaskBudget, now + kGuiMainQueueNanosecondBudget);
    if (guiMainQueueStopRequested.load(std::memory_order_acquire) ||
        yumiProcessTerminationRequested()) {
        std::exit(yumiGetProcessExitCode());
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
            *_NSGetArgc(), *_NSGetArgv(), nil, NSStringFromClass([YuGuiAppDelegate class]));
    }
}

// **********************************************************************************************

@interface YuGuiDisplayLinkTarget : NSObject
- (void)tick:(CADisplayLink*)link;
@end

static CADisplayLink* guiDisplayLink = nil;
static std::vector<YuGuiWindowHost*> guiFramePendingWindows;
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

@implementation YuGuiDisplayLinkTarget

- (void)tick:(CADisplayLink*)link {
    if (guiAppVisibility.load() == YuGuiPresentationHidden) {
        link.paused = YES;
        return;
    }
    YuGuiFrameTiming timing = {};
    timing.outputTimeNanoseconds = guiMediaTimeToContinuousNanoseconds(link.timestamp);
    timing.targetTimeNanoseconds = guiMediaTimeToContinuousNanoseconds(link.targetTimestamp);
    timing.refreshIntervalNanoseconds = link.targetTimestamp > link.timestamp
        ? static_cast<uint64_t>((link.targetTimestamp - link.timestamp) * 1e9)
        : 0;
    timing.eventTimeSeconds = link.timestamp;

    std::vector<YuGuiWindowHost*> windows;
    auto fired = std::stable_partition(
        guiFramePendingWindows.begin(), guiFramePendingWindows.end(), [&timing](YuGuiWindowHost* guiWindow) {
            return timing.targetTimeNanoseconds && guiWindow->renderNotBeforeNanoseconds > timing.targetTimeNanoseconds;
        });
    windows.assign(fired, guiFramePendingWindows.end());
    guiFramePendingWindows.erase(fired, guiFramePendingWindows.end());
    for (YuGuiWindowHost* guiWindow : windows) {
        [guiWindow fireRenderFrameWithTiming:timing];
    }
    if (windows.empty() && guiFramePendingWindows.empty()) {
        link.paused = YES;
    }
}

@end

static void
guiFrameSchedulerRequest(YuGuiWindowHost* guiWindow) {
    if (std::find(guiFramePendingWindows.begin(), guiFramePendingWindows.end(), guiWindow) ==
        guiFramePendingWindows.end()) {
        guiFramePendingWindows.push_back(guiWindow);
    }
    if (!guiDisplayLink) {
        YuGuiDisplayLinkTarget* target = [[YuGuiDisplayLinkTarget alloc] init];
        guiDisplayLink = [CADisplayLink displayLinkWithTarget:target selector:@selector(tick:)];
        float maximum = static_cast<float>([UIScreen mainScreen].maximumFramesPerSecond);
        guiDisplayLink.preferredFrameRateRange = CAFrameRateRangeMake(maximum, maximum, maximum);
        [guiDisplayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    }
    guiDisplayLink.paused = guiAppVisibility.load() == YuGuiPresentationHidden;
}

static void
guiFrameSchedulerResume(void) {
    if (guiDisplayLink && !guiFramePendingWindows.empty() &&
        guiAppVisibility.load() != YuGuiPresentationHidden) {
        guiDisplayLink.paused = NO;
    }
}

static void
guiFrameSchedulerCancel(YuGuiWindowHost* guiWindow) {
    auto found = std::find(guiFramePendingWindows.begin(), guiFramePendingWindows.end(), guiWindow);
    if (found != guiFramePendingWindows.end()) {
        guiFramePendingWindows.erase(found);
    }
}

// **********************************************************************************************

static YuGuiWindowHost*
getWindow(void* ptr) {
    return (__bridge YuGuiWindowHost*)ptr;
}

extern "C" void*
yumiGuiAppCreate(void) {
    return (__bridge_retained void*)[[YuGuiAppHost alloc] init];
}

extern "C" void
yumiGuiAppRelease(void* ptr) {
    guiAppQuitCallback = nullptr;
    guiAppQuitContext = nullptr;
    guiAppBackgroundCallback = nullptr;
    guiAppBackgroundContext = nullptr;
    guiAppSystemCallback = nullptr;
    guiAppSystemContext = nullptr;
    CFBridgingRelease(ptr);
}

extern "C" void
yumiGuiAppRun(void* ptr) {
    if (!yumiRunMainTaskQueueHost(guiMainQueueRunHost, guiMainQueueWake, ptr) &&
        !yumiProcessTerminationRequested()) {
        std::fprintf(stderr, "App.run must own the active main Yumi task queue\n");
        std::abort();
    }
}

extern "C" void
yumiGuiAppStop(void* ptr) {
    guiMainQueueRequestStop();
}

extern "C" void
yumiGuiAppScheduleStop(void* ptr, uint64_t delayNanoseconds) {
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(std::min<uint64_t>(delayNanoseconds, INT64_MAX))),
        dispatch_get_main_queue(),
        ^{
            guiMainQueueRequestStop();
        });
}

extern "C" void
yumiGuiAppSetTitle(void* ptr, const char* title) {}

extern "C" void
yumiGuiAppSetQuitCallback(void* ptr, YuGuiActionCallback callback, void* context) {
    guiAppQuitCallback = callback;
    guiAppQuitContext = context;
}

extern "C" void
yumiGuiAppSetBackgroundCallback(void* ptr, YuGuiActionCallback callback, void* context) {
    guiAppBackgroundCallback = callback;
    guiAppBackgroundContext = context;
}

extern "C" void
yumiGuiAppSetOpenCallback(void* ptr, YuGuiOpenCallback callback, void* context) {}

extern "C" void
yumiGuiAppSetKeepsDisplayAwake(void* ptr, bool keeps) {
    guiAppKeepsDisplayAwake = keeps;
    guiApplyKeepsDisplayAwake();
}

extern "C" uint32_t
yumiGuiSystemAppearance(void) {
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
yumiGuiAppSetSystemCallback(void* ptr, YuGuiActionCallback callback, void* context) {
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
yumiGuiSystemColor(uint32_t color, uint32_t appearance) {
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
yumiGuiSystemOptions(void) {
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

extern "C" void* yumiGuiMenuBarCreate(void) { return nullptr; }
extern "C" void yumiGuiMenuBarInstall(void* menuBar) {}
extern "C" void yumiGuiMenuBarUninstall(void* menuBar) {}
extern "C" void* yumiGuiMenuCreate(const char* title, YuGuiMenuCallback callback, void* context) { return nullptr; }
extern "C" void yumiGuiMenuRelease(void* menu) {}
extern "C" void yumiGuiMenuRemoveAllItems(void* menu) {}
extern "C" void yumiGuiMenuAddItem(
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
extern "C" void yumiGuiMenuAddSeparator(void* menu) {}
extern "C" void yumiGuiMenuPerformRole(uint32_t role) {}

// **********************************************************************************************

extern "C" void* yumiGuiStatusItemCreate(void) { return nullptr; }
extern "C" void yumiGuiStatusItemRelease(void* item) {}
extern "C" void yumiGuiStatusItemSetTitle(void* item, const char* title) {}
extern "C" void yumiGuiStatusItemSetMenu(void* item, void* menu) {}
extern "C" void yumiGuiStatusItemSetCallback(void* item, YuGuiStatusItemCallback callback, void* context) {}
extern "C" void yumiGuiStatusItemSetPanel(void* item, void* window) {}
extern "C" double yumiGuiStatusItemGetBound(void* item, uint32_t component) { return 0; }
extern "C" double yumiGuiStatusItemGetScreenBound(void* item, uint32_t component) { return 0; }

// **********************************************************************************************

extern "C" void*
yumiGuiWindowCreate(const char* title, double width, double height) {
    UIScreen* screen = [UIScreen mainScreen];
    CGSize screenSize = screen.bounds.size;
    YuGuiWindowHost* guiWindow = [[YuGuiWindowHost alloc] init];
    guiWindow->inputStreamState = std::make_shared<YuGuiInputStreamState>();
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
yumiGuiWindowRelease(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
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
yumiGuiWindowGetNativeWindow(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? (__bridge void*)guiWindow->window : nullptr;
}

extern "C" void
yumiGuiWindowShow(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || guiWindow->closed) {
        return;
    }
    guiWindow->shown = true;
    if (guiWindow->window) {
        [guiWindow->window makeKeyAndVisible];
    }
}

extern "C" void
yumiGuiWindowClose(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->shown = false;
    guiWindow->window.hidden = YES;
}

extern "C" void yumiGuiWindowRejectDeferredClose(void* window) {}
extern "C" void yumiGuiWindowSetTitle(void* window, const char* title) {}

extern "C" void
yumiGuiWindowSetTransparent(void* ptr, bool windowTransparent, bool contentTransparent) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        guiWindow->metalLayer.opaque = !contentTransparent;
    }
}

extern "C" void yumiGuiWindowSetAppearance(void* window, uint32_t appearance) {}
extern "C" void yumiGuiWindowSetChrome(void* window, bool titlebarVisible, bool hasStoplightOffset, double x, double y, double zoom) {}
extern "C" double yumiGuiWindowGetStoplightEdge(void* window) { return 0; }
extern "C" void yumiGuiWindowSetKind(void* window, uint32_t kind) {}
extern "C" bool yumiGuiWindowRequestDrag(void* window, uint64_t token) { return false; }
extern "C" void yumiGuiWindowPerformTitlebarDoubleClick(void* window) {}
extern "C" void yumiGuiWindowSetContentSize(void* window, double width, double height) {}
extern "C" double yumiGuiWindowGetPosition(void* window, uint32_t component) { return 0; }
extern "C" void yumiGuiWindowSetPosition(void* window, double x, double y) {}
extern "C" void yumiGuiWindowSetMovedCallback(void* window, YuGuiActionCallback callback, void* context) {}

extern "C" double
yumiGuiWindowGetContentWidth(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->contentWidth : 0;
}

extern "C" double
yumiGuiWindowGetContentHeight(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->contentHeight : 0;
}

extern "C" void*
yumiGuiWindowGetDisplayColorSpace(void* ptr) {
    static CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    return colorSpace;
}

extern "C" bool yumiGuiWindowDisplayIsWideGamut(void* window) { return false; }

extern "C" void
yumiGuiWindowSetContentSizeChangedCallback(void* ptr, YuGuiSizeCallback callback, void* context) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->contentSizeCallback = callback;
    guiWindow->contentSizeContext = context;
    guiReplaceRoot(&guiWindow->contentSizeContextRoot, context);
}

extern "C" double
yumiGuiWindowGetBackingScaleFactor(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow->scale > 0 ? guiWindow->scale : 1.0;
}

extern "C" void
yumiGuiWindowSetBackingScaleChangedCallback(void* ptr, YuGuiActionCallback callback, void* context) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->backingScaleCallback = callback;
    guiWindow->backingScaleContext = context;
    guiReplaceRoot(&guiWindow->backingScaleContextRoot, context);
}

extern "C" void
yumiGuiWindowSetPointerCallback(void* ptr, YuGuiPointerCallback callback, void* context) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->pointerCallback = callback;
    guiWindow->pointerContext = context;
    guiReplaceRoot(&guiWindow->pointerContextRoot, context);
}

extern "C" void
yumiGuiWindowSetTouchCallback(void* ptr, YuGuiTouchCallback callback, void* context) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->touchCallback = callback;
    guiWindow->touchContext = context;
    guiReplaceRoot(&guiWindow->touchContextRoot, context);
}

extern "C" void
yumiGuiWindowSetEventCallback(void* ptr, YuGuiWindowEventCallback callback, void* context) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->eventCallback = callback;
    guiWindow->eventContext = context;
    guiReplaceRoot(&guiWindow->eventContextRoot, context);
}

extern "C" void yumiGuiWindowSetTextActionCallback(void* window, YuGuiWindowTextActionCallback callback, void* context) {}
extern "C" void yumiGuiWindowSetTextInputActive(void* window, bool active) {}
extern "C" void yumiGuiWindowSetUndoState(void* window, bool canUndo, const char* undoTitle, bool canRedo, const char* redoTitle) {}
extern "C" void yumiGuiWindowSetTextInputContext(
    void* window,
    const char* before,
    const char* selected,
    double x,
    double y,
    double width,
    double height
) {}
extern "C" void yumiGuiWindowDiscardTextComposition(void* window) {}
extern "C" bool yumiGuiWindowDebugTextInputActive(void* window) { return false; }
extern "C" void yumiGuiWindowSetCursorStyle(void* window, uint32_t cursorStyle, void* cursorImage, bool captured) {}
extern "C" uint32_t yumiGuiWindowDebugCursorStyle(void* window) { return 0; }
extern "C" void yumiGuiWindowDebugSetPointerOverCanvas(void* window, bool value) {}
extern "C" uint64_t yumiGuiWindowDebugCursorRectRefreshCount(void* window) { return 0; }
extern "C" bool yumiGuiWindowDebugCursorRectsEnabled(void* window) { return false; }
extern "C" const char* yumiGuiWindowGetEventText(void* window) { return ""; }
extern "C" bool yumiGuiWindowFocusCanvas(void* window) { return true; }
extern "C" bool yumiGuiWindowRequestPeerFocus(void* window, void* peer) { return false; }
extern "C" bool yumiGuiWindowPeerHasFocus(void* window, void* peer) { return false; }
extern "C" bool yumiGuiWindowDebugPeerAt(void* window, void* peer, double x, double y) { return false; }

extern "C" void
yumiGuiWindowBeginFrame(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow beginFrameUpdate];
    }
}

extern "C" void
yumiGuiWindowCommitFrame(void* ptr, void* canvas, double damageX, double damageY, double damageWidth, double damageHeight) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow finishFrameUpdate];
    }
}

extern "C" bool yumiGuiWindowCopyPresentedPixels(void* window, void* canvas) { return false; }
extern "C" int64_t yumiGuiWindowPresentedSurfaceCount(void* window) { return 0; }
extern "C" int64_t yumiGuiWindowPresentationOrientationCheck(void* window) { return 0; }

extern "C" void
yumiGuiWindowCommitMetalFrame(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow finishFrameUpdate];
    }
}

extern "C" void*
yumiGuiWindowGetMetalLayer(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && !guiWindow->closed ? (__bridge void*)guiWindow->metalLayer : nullptr;
}

extern "C" bool
yumiGuiWindowSetCanvasLayer(void* ptr, void* layerPointer) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    CALayer* layer = (__bridge CALayer*)layerPointer;
    if (!guiWindow || guiWindow->closed || layer != guiWindow->metalLayer) {
        return false;
    }
    layer.hidden = NO;
    return true;
}

extern "C" void
yumiGuiWindowResetCanvasLayer(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        guiWindow->metalLayer.hidden = YES;
    }
}

extern "C" int32_t
yumiGuiWindowPresentationVisibility(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow->window || !guiWindow->shown) {
        return YuGuiPresentationHidden;
    }
    return guiAppVisibility.load();
}

extern "C" void
yumiGuiWindowSetPresentationPending(void* ptr, bool pending) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
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
yumiGuiWindowGetMaximumFramesPerSecond(void* window) {
    return static_cast<int32_t>([UIScreen mainScreen].maximumFramesPerSecond);
}

extern "C" void
yumiGuiWindowAbortFrame(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow finishFrameUpdate];
    }
}

extern "C" void
yumiGuiWindowRequestFrame(void* ptr, YuGuiActionCallback callback, void* context, uint64_t notBeforeNanoseconds) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow && callback) {
        [guiWindow requestRenderFrameWithCallback:callback context:context notBefore:notBeforeNanoseconds];
    }
}

extern "C" void
yumiGuiWindowCancelFrame(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow cancelRenderFrame];
    }
}

extern "C" uint64_t
yumiGuiWindowGetLastFrameTargetTimeNanoseconds(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->lastFrameTargetTimeNanoseconds : 0;
}

extern "C" uint64_t
yumiGuiWindowGetLastFrameRefreshIntervalNanoseconds(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->lastFrameRefreshIntervalNanoseconds : 0;
}

extern "C" double
yumiGuiWindowGetLastFrameEventTime(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->lastFrameEventTimeSeconds : 0;
}

extern "C" uint64_t yumiGuiWindowDebugViewCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugLayerCount(void* window) { return 0; }
extern "C" void yumiGuiWindowDebugInjectContentSize(void* window, double width, double height) {}
extern "C" bool yumiGuiWindowDebugInjectTransactionalContentSize(
    void* window,
    double width,
    double height,
    uint64_t budgetNanoseconds
) { return false; }
extern "C" void yumiGuiWindowDebugInjectBackingScaleChange(void* window) {}
extern "C" void yumiGuiWindowDebugSendMouseClick(void* window, double x, double y) {}
extern "C" void yumiGuiWindowDebugPostMouseClick(void* window, double x, double y) {}
extern "C" void yumiGuiAppDebugMainLoopProbeStart(uint64_t intervalNanoseconds) {}
extern "C" int64_t yumiGuiAppDebugMainLoopProbeValue(int32_t field) { return 0; }
extern "C" void yumiGuiWindowDebugInjectPointerEvent(
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
yumiGuiWindowDebugInputEventCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->eventsReceived : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugInputCallbacksScheduled(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->callbacksScheduled : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugInputCallbacksServiced(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->callbacksServiced : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugInputCoalescedCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->eventsCoalesced : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugInputBarrierCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->barriersReceived : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugInputPendingCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->pendingCallbacks : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugInputPendingHighWaterMark(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->inputStreamState->pendingHighWaterMark : 0;
}

extern "C" uint64_t yumiGuiWindowDebugResizeNotificationCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugResizeCallbacksServiced(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugResizeCoalescedCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugResizePendingCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugResizePendingHighWaterMark(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugResizeFrameCommitCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugResizeFramesPerCallbackHighWaterMark(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugResizeObsoleteSizeRenderCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugBackingScaleNotificationCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugBackingScaleCallbacksServiced(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugBackingScaleCoalescedCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugBackingScalePendingCount(void* window) { return 0; }
extern "C" uint64_t yumiGuiWindowDebugBackingScalePendingHighWaterMark(void* window) { return 0; }

// **********************************************************************************************

extern "C" void* yumiGuiMaterialCreate(void) { return nullptr; }
extern "C" void yumiGuiMaterialRelease(void* material) {}
extern "C" void yumiGuiMaterialSetContainer(void* material, void* window, void* boxHost, int32_t depth) {}
extern "C" void yumiGuiMaterialSetFrame(
    void* material,
    double x,
    double y,
    double width,
    double height,
    uint32_t kind,
    double opacity,
    bool visible
) {}
extern "C" void yumiGuiMaterialSetMask(void* material, void* canvas, double width, double height, double cap) {}
extern "C" void* yumiGuiBoxHostCreate(void* window) { return nullptr; }
extern "C" void yumiGuiBoxHostRelease(void* host) {}
extern "C" void yumiGuiBoxHostSetParent(void* host, void* parentHost) {}
extern "C" void yumiGuiBoxHostSetFrame(
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
extern "C" void yumiGuiBoxHostSetCornerRadii(void* host, double left, double top, double right, double bottom) {}
extern "C" bool yumiGuiBoxHostHasParent(void* host, void* parentHost) { return false; }
extern "C" bool yumiGuiBoxHostFrameEquals(
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
extern "C" bool yumiGuiBoxHostContentOriginEquals(void* host, double x, double y) { return false; }
extern "C" bool yumiGuiBoxHostClips(void* host) { return false; }
extern "C" bool yumiGuiBoxHostHasRoundedClip(void* host) { return false; }
extern "C" bool yumiGuiBoxHostIsAccessibilityTransparent(void* host) { return true; }

// **********************************************************************************************

extern "C" void yumiGuiWindowAddPeer(void* window, void* peer, bool ownsKeys, bool ownsCursor) {}
extern "C" void yumiGuiPeerSetContainer(void* window, void* peer, void* boxHost) {}
extern "C" void yumiGuiPeerDetach(void* peer) {}
extern "C" void yumiGuiPeerSetFrame(
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
extern "C" void yumiGuiPeerSetVisible(void* peer, bool visible) {}
extern "C" void yumiGuiPeerSetOpacity(void* peer, double opacity) {}
extern "C" void yumiGuiPeerSetPassesScroll(void* peer, bool passesScroll) {}
extern "C" void yumiGuiPeerDispatchKeyEvent(void* peer, void* event) {}

// **********************************************************************************************

extern "C" void* yumiGuiPopupCreate(void* window) { return nullptr; }
extern "C" void yumiGuiPopupRelease(void* popup) {}
extern "C" void* yumiGuiPopupGetHost(void* popup) { return nullptr; }
extern "C" void* yumiGuiPopupGetMetalLayer(void* popup) { return nullptr; }
extern "C" bool yumiGuiPopupSetCanvasLayer(void* popup, void* layer) { return false; }
extern "C" void yumiGuiPopupResetCanvasLayer(void* popup) {}
extern "C" int32_t yumiGuiPopupPresentationVisibility(void* popup) { return YuGuiPresentationHidden; }
extern "C" void* yumiGuiPopupGetWindow(void* popup) { return nullptr; }
extern "C" void yumiGuiPopupSetFrame(void* popup, double x, double y, double width, double height) {}
extern "C" void yumiGuiPopupSetShadow(void* popup, bool shadow) {}
extern "C" void yumiGuiPopupSetPresentationPending(void* popup, bool pending) {}
extern "C" void yumiGuiPopupSetShown(void* popup, bool shown) {}
extern "C" bool yumiGuiPopupIsShown(void* popup) { return false; }
extern "C" void yumiGuiPopupOrderFront(void* popup) {}
extern "C" void yumiGuiPopupSetPassesPointer(void* popup, bool passesPointer) {}
extern "C" void yumiGuiPopupSetConstrained(void* popup, bool constrained) {}
extern "C" void yumiGuiPopupCommitFrame(void* popup, void* canvas) {}
extern "C" double yumiGuiPopupGetScreenBound(void* popup, uint32_t component) { return 0; }

extern "C" double
yumiGuiWindowGetScreenBound(void* ptr, uint32_t component) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
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

extern "C" uint64_t yumiGuiClipboardChangeCount(const char* name) { return 0; }
extern "C" uint64_t yumiGuiClipboardItemCount(const char* name) { return 0; }
extern "C" const char* yumiGuiClipboardItemTypes(const char* name, uint64_t index) { return nullptr; }
extern "C" const char* yumiGuiClipboardItemString(const char* name, uint64_t index, const char* type) { return nullptr; }
extern "C" void* yumiGuiClipboardItemData(const char* name, uint64_t index, const char* type) { return nullptr; }
extern "C" const char* yumiGuiClipboardItemPath(const char* name, uint64_t index) { return nullptr; }
extern "C" uint64_t yumiGuiClipboardDataLength(void* data) { return 0; }
extern "C" void yumiGuiClipboardDataCopy(void* data, uint8_t* bytes, uint64_t length) {}
extern "C" void yumiGuiClipboardDataRelease(void* data) {}
extern "C" void* yumiGuiClipboardWriterCreate(void) { return nullptr; }
extern "C" void yumiGuiClipboardWriterAddItem(void* writer) {}
extern "C" bool yumiGuiClipboardWriterSetString(void* writer, const char* type, const char* value) { return false; }
extern "C" bool yumiGuiClipboardWriterSetData(void* writer, const char* type, const uint8_t* bytes, uint64_t length) { return false; }
extern "C" bool yumiGuiClipboardWriterSetPath(void* writer, const char* path) { return false; }
extern "C" bool yumiGuiClipboardWriterCommit(void* writer, const char* name) { return false; }
extern "C" void yumiGuiClipboardWriterRelease(void* writer) {}
extern "C" void yumiGuiClipboardDiscard(const char* name) {}
extern "C" const char* yumiGuiWindowDropPasteboardName(void*) { return nullptr; }
extern "C" void yumiGuiWindowSetDropOperation(void* window, uint32_t operation) {}
extern "C" bool yumiGuiWindowBeginExportDrag(void* window, void* writer, void* canvas, double x, double y, double width, double height) { return false; }
extern "C" uint64_t yumiGuiDragWaitPromisedFiles(uint64_t sequence, uint64_t timeoutMilliseconds) { return 0; }
extern "C" const char* yumiGuiDragPromisedFile(uint64_t sequence, uint64_t index) { return nullptr; }
extern "C" void yumiGuiDragReleasePromisedFiles(uint64_t sequence) {}
extern "C" bool yumiGuiTypeConforms(const char* type, const char* parent) { return false; }
extern "C" const char* yumiGuiTypeOfPath(const char* path) { return nullptr; }

// **********************************************************************************************

extern "C" bool yumiGuiTrashPath(const char* path) { return false; }
extern "C" bool yumiGuiRevealPath(const char* path) { return false; }

extern "C" bool
yumiGuiOpenURL(const char* url) {
    NSURL* target = url ? [NSURL URLWithString:[NSString stringWithUTF8String:url]] : nil;
    if (!target) {
        return false;
    }
    [[UIApplication sharedApplication] openURL:target options:@{} completionHandler:nil];
    return true;
}

// **********************************************************************************************

extern "C" void* yumiGuiTextInputCreate(
    void* window,
    double x,
    double y,
    double width,
    double height,
    const char* text,
    const char* placeholder
) { return nullptr; }
extern "C" void yumiGuiTextInputRelease(void* input) {}
extern "C" void yumiGuiTextInputSetFrame(
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
extern "C" void yumiGuiTextInputSetText(void* input, const char* text) {}
extern "C" const char* yumiGuiTextInputGetText(void* input) { return ""; }
extern "C" void yumiGuiTextInputSetEventCallback(void* input, YuGuiTextInputEventCallback callback, void* context) {}
extern "C" void yumiGuiTextInputSetPlaceholder(void* input, const char* placeholder) {}
extern "C" void yumiGuiTextInputSetTextColor(void* input, double red, double green, double blue, double alpha) {}
extern "C" void yumiGuiTextInputSetFont(void* input, const char* family, double size, double weight) {}
extern "C" void yumiGuiTextInputSetAlignment(void* input, int32_t alignment) {}
extern "C" void yumiGuiTextInputSetEnabled(void* input, bool enabled) {}
extern "C" void yumiGuiTextInputSetEditable(void* input, bool editable) {}
extern "C" void yumiGuiTextInputSetSecure(void* input, bool secure) {}
extern "C" void yumiGuiTextInputSetMaxLength(void* input, uint32_t maxLength) {}
extern "C" void yumiGuiTextInputSetVisible(void* input, bool visible) {}
extern "C" void yumiGuiTextInputSetOpacity(void* input, double opacity) {}
extern "C" double yumiGuiTextInputGetOpacity(void* input) { return 0; }
extern "C" void yumiGuiTextInputSetCursorStyle(void* input, uint32_t cursorStyle, void* cursorImage) {}
extern "C" uint32_t yumiGuiTextInputDebugCursorStyle(void* input) { return 0; }
extern "C" void yumiGuiTextInputSetContainer(void* input, void* boxHost) {}
extern "C" bool yumiGuiTextInputHasContainer(void* input, void* boxHost) { return false; }
extern "C" double yumiGuiTextInputGetFittingWidth(void* input) { return 0; }
extern "C" double yumiGuiTextInputGetFittingHeight(void* input) { return 0; }
extern "C" double yumiGuiTextInputGetFittingHeightForWidth(void* input, double width) { return 0; }
extern "C" void yumiGuiTextInputSetWraps(void* input, bool wraps) {}
extern "C" void yumiGuiTextInputSetSelection(void* input, const char* before, const char* selected) {}
extern "C" void yumiGuiTextInputSetPassedKeys(void* input, uint32_t keys) {}
extern "C" void yumiGuiTextInputSetLineMetrics(void* input, double lineHeight, double lineSpacing) {}
extern "C" void yumiGuiTextInputSetTextInsets(void* input, double top, double bottom) {}
extern "C" void yumiGuiTextInputSetHitRect(void* input, double x, double y, double width, double height) {}
extern "C" double yumiGuiTextInputDebugLineMetric(void* input, uint32_t metric) { return 0; }
extern "C" void yumiGuiTextInputReplaceText(void* input, const char* text) {}
extern "C" void yumiGuiTextInputPasteText(void* input) {}
extern "C" void yumiGuiTextInputMoveCaretToEnd(void* input) {}
extern "C" void yumiGuiTextInputSyncText(void* input, const char* text) {}
extern "C" bool yumiGuiTextInputCanUndo(void* input, bool redoes) { return false; }
extern "C" void yumiGuiTextInputUndo(void* input, bool redoes) {}
extern "C" double yumiGuiTextInputGetTextOriginX(void* input) { return 0; }
extern "C" double yumiGuiTextInputGetTextBaseline(void* input) { return 0; }
extern "C" void yumiGuiTextInputDebugInjectEvent(void* input, uint32_t eventKind, const char* text) {}

// **********************************************************************************************

extern "C" void* yumiGuiCursorCreate(void* canvas, double hotspotX, double hotspotY) { return nullptr; }
extern "C" void yumiGuiCursorRelease(void* cursor) {}
extern "C" double yumiGuiCursorWidth(void* cursor) { return 0; }
extern "C" double yumiGuiCursorHotspotX(void* cursor) { return 0; }
extern "C" double yumiGuiCursorHotspotY(void* cursor) { return 0; }
