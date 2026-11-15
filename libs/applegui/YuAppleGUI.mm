#include "YuAppleGUI/YuAppleGUI.h"
#include "YuAppleGUI/YuGuiInputCoalescer.h"
#include "YuAppleGUI/YuMetalRenderer.h"
#include "YuCore/YuCore.h"
#include "YuEvents/YuEvents.h"
#include "YuEvents/YuGCHandshake.h"
#include "YuGraphics/YuGraphics.h"

#import <AppKit/AppKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <IOSurface/IOSurface.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <mach/mach_time.h>
#import <objc/runtime.h>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

// **********************************************************************************************

@class YuGuiCanvasView;
@class YuGuiFlippedView;
@class YuGuiMetalView;
@class YuGuiWindowHost;
@class YuGuiStoplightView;
@class YuGuiPopupHost;

@interface YuGuiEventWindow : NSWindow

@property(nonatomic, weak) YuGuiWindowHost* guiWindow;
@property(nonatomic, strong) NSTextView* guiFieldEditor;

@end

struct YuGuiFrameTiming {
    uint64_t outputTimeNanoseconds;
    uint64_t targetTimeNanoseconds;
    uint64_t refreshIntervalNanoseconds;
    uint64_t sequence;
    double eventTimeSeconds;
};

struct YuGuiLatestCallbackState {
    bool pending;
    bool servicing;
    uint64_t notificationsReceived;
    uint64_t callbacksServiced;
    uint64_t notificationsCoalesced;
    uint64_t pendingHighWaterMark;
    uint64_t framesCommitted;
    uint64_t framesCommittedThisService;
    uint64_t framesCommittedPerServiceHighWaterMark;
    uint64_t obsoleteSizesRendered;
    double serviceWidth;
    double serviceHeight;
};

static const uint64_t kGuiResizeDefaultNanosecondBudget = 16666667ull;

static NSCursor*
guiSymbolCursor(uint32_t style) {
    static NSCursor* cursors[15] = {};
    if (cursors[style]) return cursors[style];
    NSImage* image = [[NSImage alloc] initWithSize:NSMakeSize(24, 24)];
    [image lockFocus];
    NSBezierPath* path = [NSBezierPath bezierPath];
    auto line = [&](CGFloat x1, CGFloat y1, CGFloat x2, CGFloat y2) {
        [path moveToPoint:NSMakePoint(x1, y1)];
        [path lineToPoint:NSMakePoint(x2, y2)];
    };
    if (style == 14) {
        [path appendBezierPathWithArcWithCenter:NSMakePoint(12, 12)
                                        radius:7 startAngle:40 endAngle:320 clockwise:NO];
        line(17, 7, 20, 7);
        line(20, 7, 20, 11);
    } else if (style == 5) {
        line(3, 12, 21, 12);
        line(12, 3, 12, 21);
        line(3, 12, 7, 16); line(3, 12, 7, 8);
        line(21, 12, 17, 16); line(21, 12, 17, 8);
        line(12, 3, 8, 7); line(12, 3, 16, 7);
        line(12, 21, 8, 17); line(12, 21, 16, 17);
    } else {
        bool descending = style == 12;
        CGFloat y1 = descending ? 20 : 4;
        CGFloat y2 = descending ? 4 : 20;
        line(4, y1, 20, y2);
        line(4, y1, 10, y1);
        line(4, y1, 4, descending ? 14 : 10);
        line(20, y2, 14, y2);
        line(20, y2, 20, descending ? 10 : 14);
    }
    [path setLineJoinStyle:NSLineJoinStyleRound];
    [[NSColor whiteColor] setStroke];
    [path setLineWidth:4];
    [path stroke];
    [[NSColor blackColor] setStroke];
    [path setLineWidth:2];
    [path stroke];
    [image unlockFocus];
    cursors[style] = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(12, 12)];
    return cursors[style];
}

static NSCursor*
guiCursorForStyle(uint32_t cursorStyle, bool nativeAutomatic, NSCursor* customCursor = nil) {
    switch (cursorStyle) {
        case 0:
            return nativeAutomatic ? nil : [NSCursor arrowCursor];
        case 1:
            return [NSCursor arrowCursor];
        case 2:
            return [NSCursor pointingHandCursor];
        case 3:
            return [NSCursor IBeamCursor];
        case 4:
            return [NSCursor crosshairCursor];
        case 5:
        case 12:
        case 13:
        case 14:
            return guiSymbolCursor(cursorStyle);
        case 15:
            return customCursor ?: [NSCursor arrowCursor];
        case 6:
            return [NSCursor openHandCursor];
        case 7:
            return [NSCursor closedHandCursor];
        case 8:
            return [NSCursor operationNotAllowedCursor];
        case 9:
            return [NSCursor resizeLeftRightCursor];
        case 10:
            return [NSCursor resizeUpDownCursor];
        case 11: {
            static NSCursor* hiddenCursor = nil;
            if (!hiddenCursor) {
                NSImage* image = [[NSImage alloc] initWithSize:NSMakeSize(1, 1)];
                hiddenCursor = [[NSCursor alloc] initWithImage:image hotSpot:NSZeroPoint];
            }
            return hiddenCursor;
        }
        default:
            return nativeAutomatic ? nil : [NSCursor arrowCursor];
    }
}

struct YuGuiPointerCallbackRecord;
static std::atomic_uint64_t guiNextWindowDragToken{1};

struct YuGuiInputStreamState {
    std::unordered_map<uint64_t, YuGuiPointerCallbackRecord*> dragRequests;
    uint64_t primaryPress = 0;
    uint64_t draggedPress = 0;
    bool primaryDown = false;
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
        if (event.inputKind == YuGuiInputKind::Barrier || event.isScrollTransition()) {
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

    void seal() {
        coalescer.seal();
    }

    void cancel() {
        canceled = true;
        primaryDown = false;
        seal();
    }
};

static void guiFrameSchedulerRequest(YuGuiWindowHost* guiWindow);
static void guiFrameSchedulerCancel(YuGuiWindowHost* guiWindow);

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

struct YuGuiInputCallbackRecord : YuGuiCallbackRecord {
    YuGuiInputCallbackRecord(
        void* aContext,
        std::shared_ptr<YuGuiInputStreamState> aStream,
        YuGuiInputEvent&& anEvent):
        YuGuiCallbackRecord(aContext),
        stream(std::move(aStream)),
        event(std::move(anEvent)),
        pending(true)
    {
        stream->append(event);
    }

    ~YuGuiInputCallbackRecord() override {
        if (pending) {
            stream->abandon(event);
        }
    }

    bool begin() {
        if (!pending) {
            return false;
        }
        pending = false;
        return stream->begin(event);
    }

    void serviced() {
        ++stream->callbacksServiced;
    }

    std::shared_ptr<YuGuiInputStreamState> stream;
    YuGuiInputEvent event;
    bool pending;
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
        bool anIsDisplayFrame = false):
        YuGuiCallbackRecord(aContext),
        callback(aCallback),
        idleDeadlineNanoseconds(anIdleDeadlineNanoseconds),
        isDisplayFrame(anIsDisplayFrame) {}
    YuGuiActionCallback callback;
    uint64_t idleDeadlineNanoseconds;
    bool isDisplayFrame;
};

static const uint64_t kGuiOverrunFrameCollectorNanoseconds = 1000000ull;

static void guiFrameSchedulerFrameEnded(void);

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
        if (record->isDisplayFrame) {
            guiFrameSchedulerFrameEnded();
        }
    }
    guiFinishCallbackTask(record);
}

static void
guiScheduleActionCallback(
    YuGuiActionCallback callback,
    void* context,
    uint64_t idleDeadlineNanoseconds,
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

struct YuGuiOpenCallbackRecord : YuGuiCallbackRecord {
    YuGuiOpenCallbackRecord(YuGuiOpenCallback aCallback, void* aContext, NSArray<NSURL*>* urls):
        YuGuiCallbackRecord(aContext),
        callback(aCallback)
    {
        for (NSURL* url in urls) {
            const char* path = [url isFileURL] ? [[url path] UTF8String] : nullptr;
            if (path) {
                paths.push_back(path);
            }
        }
    }

    YuGuiOpenCallback callback;
    std::vector<std::string> paths;
};

static void
guiRunOpenCallback(void* closure) {
    YuGuiOpenCallbackRecord* record = static_cast<YuGuiOpenCallbackRecord*>(closure);
    if (!yumiProcessTerminationRequested()) {
        std::vector<const char*> paths;
        for (const std::string& path : record->paths) {
            paths.push_back(path.c_str());
        }
        record->callback(record->context, paths.data(), static_cast<int32_t>(paths.size()));
    }
    guiFinishCallbackTask(record);
}

static void
guiScheduleOpenCallback(YuGuiOpenCallback callback, void* context, NSArray<NSURL*>* urls) {
    if (!callback) {
        return;
    }
    YuGuiOpenCallbackRecord* record = new YuGuiOpenCallbackRecord(callback, context, urls);
    if (record->paths.empty() || !yumiScheduleMainTaskFromNative(guiRunOpenCallback, record)) {
        delete record;
    }
}

struct YuGuiTextInputCallbackRecord : YuGuiInputCallbackRecord {
    YuGuiTextInputCallbackRecord(
        YuGuiTextInputEventCallback aCallback,
        void* aContext,
        std::shared_ptr<YuGuiInputStreamState> aStream,
        YuGuiInputEvent&& anEvent):
        YuGuiInputCallbackRecord(aContext, std::move(aStream), std::move(anEvent)),
        callback(aCallback)
    {}
    YuGuiTextInputEventCallback callback;
};

static void
guiRunTextInputCallback(void* closure) {
    YuGuiTextInputCallbackRecord* record =
        static_cast<YuGuiTextInputCallbackRecord*>(closure);
    bool active = record->begin();
    if (active && !yumiProcessTerminationRequested()) {
        record->serviced();
        record->callback(
            record->context,
            record->event.eventKind,
            record->event.text.c_str(),
            record->event.selectionStart,
            record->event.selectionEnd,
            record->event.modifiers);
    }
    guiFinishCallbackTask(record);
}

static uint32_t
guiCodepointCount(NSString* text, NSUInteger utf16Length) {
    NSString* value = text ?: @"";
    NSUInteger length = MIN(utf16Length, [value length]);
    NSString* prefix = [value substringToIndex:length];
    return static_cast<uint32_t>([prefix lengthOfBytesUsingEncoding:NSUTF32LittleEndianStringEncoding] / 4);
}

static void
guiScheduleTextInputCallback(
    const std::shared_ptr<YuGuiInputStreamState>& stream,
    uintptr_t source,
    YuGuiTextInputEventCallback callback,
    void* context,
    uint32_t eventKind,
    NSString* text,
    NSRange selection,
    uint64_t modifiers) {
    if (!stream || stream->canceled || !callback) {
        return;
    }
    YuGuiInputEvent event(
        eventKind == 0 || eventKind == 5 ? YuGuiInputKind::TextChange : YuGuiInputKind::Barrier,
        source);
    event.eventKind = eventKind;
    event.text = [text ?: @"" UTF8String];
    event.selectionStart = guiCodepointCount(text, selection.location);
    event.selectionEnd = guiCodepointCount(text, selection.location + selection.length);
    event.modifiers = modifiers;
    if (stream->tryMerge(event)) {
        return;
    }
    YuGuiTextInputCallbackRecord* record = new YuGuiTextInputCallbackRecord(
        callback,
        context,
        stream,
        std::move(event)
    );
    if (!yumiScheduleMainTaskFromNative(guiRunTextInputCallback, record)) {
        delete record;
    }
}

struct YuGuiPointerCallbackRecord : YuGuiInputCallbackRecord {
    YuGuiPointerCallbackRecord(
        YuGuiPointerCallback aCallback,
        void* aContext,
        std::shared_ptr<YuGuiInputStreamState> aStream,
        YuGuiInputEvent&& anEvent,
        NSEvent* aMouseDown):
        YuGuiInputCallbackRecord(aContext, std::move(aStream), std::move(anEvent)),
        callback(aCallback),
        mouseDown(aMouseDown),
        dragWindow([aMouseDown window]),
        press(stream->primaryPress),
        dragToken(aMouseDown ? guiNextWindowDragToken.fetch_add(1) : 0)
    {}
    ~YuGuiPointerCallbackRecord() override {
        stream->dragRequests.erase(dragToken);
    }
    bool canDrag() const {
        return mouseDown && !stream->canceled && stream->primaryDown &&
            stream->primaryPress == press && stream->draggedPress != press;
    }
    YuGuiPointerCallback callback;
    NSEvent* mouseDown;
    NSWindow* dragWindow;
    uint64_t press;
    uint64_t dragToken;
    bool dragRequested = false;
};

static void
guiRunPointerCallback(void* closure) {
    YuGuiPointerCallbackRecord* record = static_cast<YuGuiPointerCallbackRecord*>(closure);
    bool active = record->begin();
    if (record->event.button == 0 && (record->event.eventKind == 2 || record->event.eventKind == 3) &&
        record->press != 0 && record->press == record->stream->draggedPress) {
        active = false;
    }
    if (active && !yumiProcessTerminationRequested()) {
        record->serviced();
        if (record->dragToken) record->stream->dragRequests.emplace(record->dragToken, record);
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
            record->dragToken
        );
        if (record->dragRequested && record->canDrag()) {
            record->stream->draggedPress = record->press;
            record->stream->primaryDown = false;
            [record->dragWindow performWindowDragWithEvent:record->mouseDown];
        }
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
    uint64_t momentumPhase,
    NSEvent* mouseDown = nil) {
    if (!stream || stream->canceled || !callback) {
        return;
    }

    YuGuiInputKind inputKind = eventKind == 0
        ? YuGuiInputKind::PointerMove
        : eventKind == 3
            ? YuGuiInputKind::PointerDrag
            : eventKind == 4
                ? YuGuiInputKind::Scroll
                : eventKind == 7
                    ? YuGuiInputKind::Magnify
                    : eventKind == 10
                        ? YuGuiInputKind::DropOver
                        : YuGuiInputKind::Barrier;
    YuGuiInputEvent event(inputKind, source);
    event.eventKind = eventKind;
    event.pointerId = pointerId;
    event.deviceKind = deviceKind;
    event.button = button;
    event.buttons = buttons;
    event.modifiers = modifiers;
    event.x = x;
    event.y = y;
    event.deltaX = deltaX;
    event.deltaY = deltaY;
    event.orderingTime = orderingTime;
    event.clickCount = clickCount;
    event.isPrecise = isPrecise;
    event.phase = phase;
    event.momentumPhase = momentumPhase;
    if (stream->tryMerge(event)) {
        return;
    }

    YuGuiPointerCallbackRecord* record =
        new YuGuiPointerCallbackRecord(callback, context, stream, std::move(event), mouseDown);
    if (!yumiScheduleMainTaskFromNative(guiRunPointerCallback, record)) {
        delete record;
    }
}

struct YuGuiWindowEventCallbackRecord : YuGuiInputCallbackRecord {
    YuGuiWindowEventCallbackRecord(
        YuGuiWindowEventCallback aCallback,
        void* aContext,
        std::shared_ptr<YuGuiInputStreamState> aStream,
        YuGuiInputEvent&& anEvent,
        uint32_t anEventKind,
        uint32_t aKey,
        uint32_t aNativeKeyCode,
        uint64_t someModifiers,
        bool repeat,
        const char* someText):
        YuGuiInputCallbackRecord(aContext, std::move(aStream), std::move(anEvent)),
        callback(aCallback),
        eventKind(anEventKind),
        key(aKey),
        nativeKeyCode(aNativeKeyCode),
        modifiers(someModifiers),
        isRepeat(repeat),
        text(someText ? someText : "")
    {}
    YuGuiWindowEventCallback callback;
    uint32_t eventKind;
    uint32_t key;
    uint32_t nativeKeyCode;
    uint64_t modifiers;
    bool isRepeat;
    std::string text;
};

static void
guiRunWindowEventCallback(void* closure) {
    YuGuiWindowEventCallbackRecord* record =
        static_cast<YuGuiWindowEventCallbackRecord*>(closure);
    bool active = record->begin();
    if ((active || record->eventKind == 6) && !yumiProcessTerminationRequested()) {
        record->serviced();
        record->callback(
            record->context,
            record->eventKind,
            record->key,
            record->nativeKeyCode,
            record->modifiers,
            record->isRepeat,
            record->text.c_str()
        );
    }
    guiFinishCallbackTask(record);
}

static void
guiScheduleWindowEventCallback(
    const std::shared_ptr<YuGuiInputStreamState>& stream,
    uintptr_t source,
    YuGuiWindowEventCallback callback,
    void* context,
    uint32_t eventKind,
    uint32_t key,
    uint32_t nativeKeyCode,
    uint64_t modifiers,
    bool isRepeat,
    NSString* text) {
    if (!stream || (stream->canceled && eventKind != 6) || !callback) {
        return;
    }
    YuGuiInputEvent event(YuGuiInputKind::Barrier, source);
    event.eventKind = eventKind;
    if (stream->tryMerge(event)) {
        return;
    }
    YuGuiWindowEventCallbackRecord* record = new YuGuiWindowEventCallbackRecord(
        callback,
        context,
        stream,
        std::move(event),
        eventKind,
        key,
        nativeKeyCode,
        modifiers,
        isRepeat,
        [text ?: @"" UTF8String]
    );
    if (!yumiScheduleMainTaskFromNative(guiRunWindowEventCallback, record)) {
        delete record;
    }
}

struct YuGuiTextAction {
    uint32_t kind;
    std::string text;
    uint32_t start;
    uint32_t length;
};

struct YuGuiTextActionCallbackRecord : YuGuiInputCallbackRecord {
    YuGuiTextActionCallbackRecord(
        YuGuiWindowTextActionCallback aCallback,
        void* aContext,
        std::shared_ptr<YuGuiInputStreamState> aStream,
        YuGuiInputEvent&& anEvent,
        std::vector<YuGuiTextAction>&& someActions,
        bool isKeyed):
        YuGuiInputCallbackRecord(aContext, std::move(aStream), std::move(anEvent)),
        callback(aCallback),
        actions(std::move(someActions)),
        keyed(isKeyed)
    {}
    YuGuiWindowTextActionCallback callback;
    std::vector<YuGuiTextAction> actions;
    bool keyed;
};

static void
guiRunTextActionCallback(void* closure) {
    YuGuiTextActionCallbackRecord* record =
        static_cast<YuGuiTextActionCallbackRecord*>(closure);
    bool active = record->begin();
    if (active && !yumiProcessTerminationRequested()) {
        record->serviced();
        for (const YuGuiTextAction& action : record->actions) {
            record->callback(
                record->context,
                action.kind,
                action.text.c_str(),
                action.start,
                action.length,
                record->keyed
            );
        }
    }
    guiFinishCallbackTask(record);
}

static void
guiScheduleTextActionCallback(
    const std::shared_ptr<YuGuiInputStreamState>& stream,
    uintptr_t source,
    YuGuiWindowTextActionCallback callback,
    void* context,
    std::vector<YuGuiTextAction>&& actions,
    bool keyed) {
    if (!stream || stream->canceled || !callback || actions.empty()) {
        return;
    }
    YuGuiInputEvent event(YuGuiInputKind::Barrier, source);
    if (stream->tryMerge(event)) {
        return;
    }
    YuGuiTextActionCallbackRecord* record = new YuGuiTextActionCallbackRecord(
        callback,
        context,
        stream,
        std::move(event),
        std::move(actions),
        keyed
    );
    if (!yumiScheduleMainTaskFromNative(guiRunTextActionCallback, record)) {
        delete record;
    }
}

static uint32_t
guiCodePointCount(NSString* text) {
    return static_cast<uint32_t>([text lengthOfBytesUsingEncoding:NSUTF32LittleEndianStringEncoding] / 4);
}

static NSString*
guiPlainString(id value) {
    if ([value isKindOfClass:[NSAttributedString class]]) {
        return [(NSAttributedString*)value string];
    }
    return [value isKindOfClass:[NSString class]] ? (NSString*)value : @"";
}

@interface YuGuiAppHost : NSObject {
@public
    std::atomic_uint64_t scheduledStopGeneration;
}

@end

struct YuGuiPresentSurface {
    IOSurfaceRef surface = nullptr;
    int32_t staleLeft = 0;
    int32_t staleTop = 0;
    int32_t staleRight = 0;
    int32_t staleBottom = 0;
};

struct YuGuiCanvasPresenter {
    std::vector<YuGuiPresentSurface> surfaces;
    int32_t width = 0;
    int32_t height = 0;
    CGColorSpaceRef colorSpace = nullptr;
    int32_t front = -1;
};

static const size_t yuGuiPresentSurfaceLimit = 3;

@interface YuGuiWindowHost : NSObject <NSWindowDelegate, NSDraggingSource> {
@public
    YuGuiCanvasPresenter canvasPresenter;
    YuGuiLatestCallbackState resizeCallbackState;
    YuGuiLatestCallbackState backingScaleCallbackState;
    YuGuiLatestCallbackState moveCallbackState;
    std::shared_ptr<YuGuiInputStreamState> inputStreamState;
    std::vector<YuGuiTextAction>* collectingTextActions;
    bool undoAvailable;
    bool redoAvailable;
    std::string undoTitle;
    std::string redoTitle;
}

@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, strong) YuGuiStoplightView* stoplightView;
@property(nonatomic, assign) BOOL titlebarVisible;
@property(nonatomic, assign) BOOL panel;
@property(nonatomic, assign) BOOL hasStoplightOffset;
@property(nonatomic, assign) NSPoint stoplightOffset;
@property(nonatomic, assign) CGFloat stoplightZoom;
@property(nonatomic, assign) CGFloat stoplightEdge;
@property(nonatomic, assign) BOOL allowsZoom;
@property(nonatomic, assign) BOOL stoplightsSuspended;
@property(nonatomic, assign) BOOL fullScreenTransitioning;
@property(nonatomic, assign) BOOL presentationPending;
@property(nonatomic, assign) BOOL presentationHeldFrame;
@property(nonatomic, strong) NSView* contentView;
@property(nonatomic, strong) YuGuiCanvasView* canvasView;
@property(nonatomic, strong) YuGuiMetalView* metalView;
@property(nonatomic, strong) NSMutableArray* popupHosts;
@property(nonatomic, assign) BOOL closed;
@property(nonatomic, assign) BOOL renderScheduled;
@property(nonatomic, assign) BOOL frameUpdateOpen;
@property(nonatomic, assign) BOOL shadowStale;
@property(nonatomic, assign) BOOL couplesPresentation;
@property(nonatomic, assign) BOOL closeDecisionPending;
@property(nonatomic, assign) void* renderContext;
@property(nonatomic, assign) void* renderContextRoot;
@property(nonatomic, assign) YuGuiActionCallback renderCallback;
@property(nonatomic, assign) void* backingScaleContext;
@property(nonatomic, assign) void* backingScaleContextRoot;
@property(nonatomic, assign) YuGuiActionCallback backingScaleCallback;
@property(nonatomic, assign) void* moveContext;
@property(nonatomic, assign) void* moveContextRoot;
@property(nonatomic, assign) YuGuiActionCallback moveCallback;
@property(nonatomic, assign) void* contentSizeContext;
@property(nonatomic, assign) void* contentSizeContextRoot;
@property(nonatomic, assign) YuGuiSizeCallback contentSizeCallback;
@property(nonatomic, assign) void* pointerContext;
@property(nonatomic, assign) void* pointerContextRoot;
@property(nonatomic, assign) YuGuiPointerCallback pointerCallback;
@property(nonatomic, assign) void* eventContext;
@property(nonatomic, assign) void* eventContextRoot;
@property(nonatomic, assign) YuGuiWindowEventCallback eventCallback;
@property(nonatomic, strong) NSString* eventText;
@property(nonatomic, assign) void* textActionContext;
@property(nonatomic, assign) void* textActionContextRoot;
@property(nonatomic, assign) YuGuiWindowTextActionCallback textActionCallback;
@property(nonatomic, assign) BOOL textInputActive;
@property(nonatomic, copy) NSString* textInputBefore;
@property(nonatomic, copy) NSString* textInputSelected;
@property(nonatomic, assign) NSRect textInputCaret;
@property(nonatomic, copy) NSString* markedText;
@property(nonatomic, assign) NSRange markedSelection;
@property(nonatomic, assign) BOOL pointerOverCanvas;
@property(nonatomic, assign) BOOL cursorCaptured;
@property(nonatomic, assign) uint64_t canvasCapturedButtons;
@property(nonatomic, assign) uint32_t dropOperation;
@property(nonatomic, strong) NSPasteboard* dropPasteboard;
@property(nonatomic, strong) NSEvent* pressEvent;
@property(nonatomic, assign) uint32_t cursorStyle;
@property(nonatomic, assign) uint64_t cursorRectRefreshCount;
@property(nonatomic, assign) uint64_t lastFrameTargetTimeNanoseconds;
@property(nonatomic, assign) uint64_t renderNotBeforeNanoseconds;
@property(nonatomic, assign) uint64_t lastFrameRefreshIntervalNanoseconds;
@property(nonatomic, assign) double lastFrameEventTimeSeconds;

- (void)requestRenderFrameWithCallback:(YuGuiActionCallback)callback
                               context:(void*)context
                             notBefore:(uint64_t)notBeforeNanoseconds;
- (void)cancelRenderFrame;
- (void)fireRenderFrameWithTiming:(YuGuiFrameTiming)timing;
- (void)beginFrameUpdate;
- (void)finishFrameUpdate;
- (void)markShadowStale;
- (void)scheduleShadowSettle;
- (void)settleShadow;
- (void)setPresentsWithTransaction:(BOOL)presentsWithTransaction;
- (void)couplePresentation;
- (void)flushNativeChange;
- (void)dispatchPointerEvent:(NSEvent*)event kind:(uint32_t)kind;
- (NSDragOperation)dispatchDrop:(id<NSDraggingInfo>)info kind:(uint32_t)kind windowPoint:(NSPoint)windowPoint;
- (BOOL)performDrop:(id<NSDraggingInfo>)info windowPoint:(NSPoint)windowPoint;
- (void)handleWindowEvent:(NSEvent*)event;
- (BOOL)handleKeyEvent:(NSEvent*)event;
- (void)handleKeyReleaseEvent:(NSEvent*)event;
- (void)dispatchPeerKeyEvent:(NSEvent*)event;
- (void)dispatchPeerFocusExit:(BOOL)backward;
- (void)dispatchWindowEventKind:(uint32_t)kind
                            key:(uint32_t)key
                  nativeKeyCode:(uint32_t)nativeKeyCode
                      modifiers:(uint64_t)modifiers
                       isRepeat:(BOOL)isRepeat;
- (BOOL)isCanvasTargetAtWindowPoint:(NSPoint)windowPoint;
- (BOOL)isCanvasAreaAtWindowPoint:(NSPoint)windowPoint;
- (BOOL)isCanvasAtWindowPoint:(NSPoint)windowPoint controlsBlock:(BOOL)controlsBlock;
- (BOOL)ownsWindow:(NSWindow*)window;
- (void)settleKeyResignation;
- (void)closePopups;
- (NSPoint)windowPointForEvent:(NSEvent*)event;
- (NSPoint)currentWindowPointerPoint;
- (void)restoreStoplights;
- (void)updateStoplights;
- (BOOL)hasMarkedText;
- (void)emitTextActionKind:(uint32_t)kind text:(NSString*)text start:(uint32_t)start length:(uint32_t)length;
- (void)dispatchTextActions:(std::vector<YuGuiTextAction>&)actions keyed:(BOOL)keyed;
- (void)textInputInsert:(NSString*)text replacementRange:(NSRange)replacementRange;
- (void)textInputCommand:(SEL)selector;
- (void)textInputSetMarkedText:(NSString*)text selectedRange:(NSRange)selectedRange;
- (void)textInputUnmark;
- (NSRange)textInputSelectedRange;
- (NSRange)textInputMarkedRange;
- (NSAttributedString*)textInputSubstring:(NSRange)range actualRange:(NSRangePointer)actualRange;
- (NSRect)textInputScreenRect;
- (NSTextInputContext*)activeTextInputContext;
- (void)finishTextComposition;
- (void)discardTextComposition;

@end

struct YuGuiWindowLatestCallbackRecord : YuGuiCallbackRecord {
    YuGuiWindowLatestCallbackRecord(
        YuGuiWindowHost* aGuiWindow,
        void* aContext,
        YuGuiLatestCallbackState* aState):
        YuGuiCallbackRecord(aContext),
        guiWindow(aGuiWindow),
        state(aState)
    {}

    ~YuGuiWindowLatestCallbackRecord() override {
    }

    YuGuiWindowHost* guiWindow;
    YuGuiLatestCallbackState* state;
};

struct YuGuiLatestSizeCallbackRecord : YuGuiWindowLatestCallbackRecord {
    YuGuiLatestSizeCallbackRecord(
        YuGuiWindowHost* aGuiWindow,
        YuGuiSizeCallback aCallback,
        void* aContext):
        YuGuiWindowLatestCallbackRecord(
            aGuiWindow,
            aContext,
            &aGuiWindow->resizeCallbackState),
        callback(aCallback)
    {}

    YuGuiSizeCallback callback;
};

struct YuGuiLatestActionCallbackRecord : YuGuiWindowLatestCallbackRecord {
    YuGuiLatestActionCallbackRecord(
        YuGuiWindowHost* aGuiWindow,
        YuGuiActionCallback aCallback,
        void* aContext,
        YuGuiLatestCallbackState* aState):
        YuGuiWindowLatestCallbackRecord(aGuiWindow, aContext, aState),
        callback(aCallback)
    {}

    YuGuiActionCallback callback;
};

static void
guiBeginLatestCallbackService(YuGuiWindowLatestCallbackRecord* record) {
    record->state->servicing = true;
    record->state->framesCommittedThisService = 0;
    ++record->state->callbacksServiced;
}

static void
guiEndLatestCallbackService(YuGuiWindowLatestCallbackRecord* record) {
    record->state->servicing = false;
}

static void
guiRunLatestSizeCallback(void* closure) {
    YuGuiLatestSizeCallbackRecord* record =
        static_cast<YuGuiLatestSizeCallbackRecord*>(closure);
    record->state->pending = false;
    if (!yumiProcessTerminationRequested() &&
        !record->guiWindow.closed &&
        record->guiWindow.contentView) {
        guiBeginLatestCallbackService(record);
        NSSize size = [record->guiWindow.contentView bounds].size;
        record->state->serviceWidth = static_cast<double>(size.width);
        record->state->serviceHeight = static_cast<double>(size.height);
        record->callback(
            record->context,
            record->state->serviceWidth,
            record->state->serviceHeight
        );
        guiEndLatestCallbackService(record);
    }
    guiFinishCallbackTask(record);
}

static void
guiRunLatestActionCallback(void* closure) {
    YuGuiLatestActionCallbackRecord* record =
        static_cast<YuGuiLatestActionCallbackRecord*>(closure);
    record->state->pending = false;
    if (!yumiProcessTerminationRequested() && !record->guiWindow.closed) {
        guiBeginLatestCallbackService(record);
        record->callback(record->context);
        guiEndLatestCallbackService(record);
    }
    guiFinishCallbackTask(record);
}

static bool
guiObserveLatestCallbackNotification(YuGuiLatestCallbackState* state) {
    ++state->notificationsReceived;
    if (state->pending) {
        ++state->notificationsCoalesced;
        return false;
    }
    state->pending = true;
    state->pendingHighWaterMark = std::max<uint64_t>(state->pendingHighWaterMark, 1);
    return true;
}

static uint32_t
guiScheduleLatestSizeCallback(
    YuGuiWindowHost* guiWindow,
    YuGuiSizeCallback callback,
    void* context,
    uint64_t deadlineNanoseconds) {
    YuGuiLatestCallbackState* state = &guiWindow->resizeCallbackState;
    if (!guiObserveLatestCallbackNotification(state)) {
        return YuMainTaskDeadlineDeferred;
    }

    YuGuiLatestSizeCallbackRecord* record =
        new YuGuiLatestSizeCallbackRecord(guiWindow, callback, context);
    uint32_t result = deadlineNanoseconds
        ? yumiRunMainTaskFromNativeUntilDeadline(
            guiRunLatestSizeCallback, record, deadlineNanoseconds)
        : (yumiScheduleMainTaskFromNative(guiRunLatestSizeCallback, record)
            ? YuMainTaskDeadlineDeferred
            : YuMainTaskDeadlineFailed);
    if (result == YuMainTaskDeadlineFailed) {
        state->pending = false;
        delete record;
    }
    return result;
}

static void
guiScheduleLatestActionCallback(
    YuGuiWindowHost* guiWindow,
    YuGuiActionCallback callback,
    void* context,
    YuGuiLatestCallbackState* state) {
    if (!guiObserveLatestCallbackNotification(state)) {
        return;
    }

    YuGuiLatestActionCallbackRecord* record =
        new YuGuiLatestActionCallbackRecord(guiWindow, callback, context, state);
    if (!yumiScheduleMainTaskFromNative(guiRunLatestActionCallback, record)) {
        state->pending = false;
        delete record;
    }
}

static uint32_t
guiReceiveContentSizeNotification(
    YuGuiWindowHost* guiWindow, uint64_t deadlineNanoseconds = 0) {
    if (guiWindow.contentSizeCallback && !guiWindow.closed && guiWindow.contentView) {
        return guiScheduleLatestSizeCallback(
            guiWindow,
            guiWindow.contentSizeCallback,
            guiWindow.contentSizeContext,
            deadlineNanoseconds
        );
    }
    return YuMainTaskDeadlineFailed;
}

static uint64_t
guiDeadlineAfter(uint64_t budgetNanoseconds) {
    uint64_t now = yumiMonotonicTimeNanoseconds();
    return now > UINT64_MAX - budgetNanoseconds
        ? UINT64_MAX
        : now + budgetNanoseconds;
}

static void
guiReceiveBackingScaleNotification(YuGuiWindowHost* guiWindow) {
    if (guiWindow.backingScaleCallback && !guiWindow.closed) {
        guiScheduleLatestActionCallback(
            guiWindow,
            guiWindow.backingScaleCallback,
            guiWindow.backingScaleContext,
            &guiWindow->backingScaleCallbackState
        );
    }
}

static void
guiReceiveMoveNotification(YuGuiWindowHost* guiWindow) {
    if (guiWindow.moveCallback && !guiWindow.closed) {
        guiScheduleLatestActionCallback(
            guiWindow,
            guiWindow.moveCallback,
            guiWindow.moveContext,
            &guiWindow->moveCallbackState
        );
    }
}

@interface YuGuiTextInputHost : NSObject <NSTextFieldDelegate>

@property(nonatomic, strong) NSTextField* control;
@property(nonatomic, weak) YuGuiWindowHost* guiWindow;
@property(nonatomic, assign) void* context;
@property(nonatomic, assign) void* contextRoot;
@property(nonatomic, assign) YuGuiTextInputEventCallback callback;
@property(nonatomic, assign) BOOL hasPendingSelection;
@property(nonatomic, assign) NSRange pendingSelection;
@property(nonatomic, assign) uint32_t passedKeys;
@property(nonatomic, assign) uint32_t maxLength;
@property(nonatomic, assign) double lineHeight;
@property(nonatomic, assign) double lineSpacing;
@property(nonatomic, assign) double textTopInset;
@property(nonatomic, assign) double textBottomInset;
@property(nonatomic, assign) NSRect hitRect;
@property(nonatomic, assign) BOOL hasHitRect;
@property(nonatomic, copy) NSString* placeholderText;
@property(nonatomic, strong) NSFont* baselineFont;
@property(nonatomic, strong) NSParagraphStyle* baselineStyle;
@property(nonatomic, assign) CGFloat baselineOffset;
@property(nonatomic, weak) NSTextField* cursorControl;
@property(nonatomic, assign) NSRect cursorRect;
@property(nonatomic, assign) BOOL cursorHidden;

- (void)textSubmitted:(id)sender;
- (void)applyPendingSelection;
- (void)applyLineMetrics;
- (void)applyCellLineMetrics;
- (void)applyTextArea;
- (void)applyEditorLayout;
- (NSParagraphStyle*)lineStyle;
- (void)sendEventKind:(uint32_t)eventKind;

@end

@protocol YuGuiTextControl <NSObject>

@property(nonatomic, weak) YuGuiTextInputHost* inputHost;
@property(nonatomic, assign) uint32_t cursorStyle;
@property(nonatomic, strong) NSCursor* customCursor;

@end

typedef NSTextField<YuGuiTextControl> YuGuiTextControlField;

@interface YuGuiTextField : NSTextField <YuGuiTextControl>

@property(nonatomic, weak) YuGuiTextInputHost* inputHost;
@property(nonatomic, assign) uint32_t cursorStyle;
@property(nonatomic, strong) NSCursor* customCursor;

@end

@interface YuGuiSecureTextField : NSSecureTextField <YuGuiTextControl>

@property(nonatomic, weak) YuGuiTextInputHost* inputHost;
@property(nonatomic, assign) uint32_t cursorStyle;
@property(nonatomic, strong) NSCursor* customCursor;

@end

@interface YuGuiFieldEditor : NSTextView

@property(nonatomic, assign) BOOL routesDrag;

@end

@interface YuGuiBoxHost : NSObject

@property(nonatomic, weak) YuGuiWindowHost* guiWindow;
@property(nonatomic, strong) YuGuiFlippedView* clipView;
@property(nonatomic, strong) YuGuiFlippedView* contentView;
@property(nonatomic, assign) NSEdgeInsets maskRadii;
@property(nonatomic, assign) NSRect maskBounds;

@end

@interface YuGuiPopupPanel : NSPanel

@property(nonatomic, weak) YuGuiWindowHost* ownerHost;
@property(nonatomic, strong) NSTextView* guiFieldEditor;
@property(nonatomic, assign) BOOL isUnconstrained;

@end

@interface YuGuiPopupHost : NSObject

@property(nonatomic, weak) YuGuiWindowHost* guiWindow;
@property(nonatomic, strong) YuGuiPopupPanel* panel;
@property(nonatomic, strong) YuGuiCanvasView* canvasView;
@property(nonatomic, strong) YuGuiMetalView* metalView;
@property(nonatomic, strong) YuGuiBoxHost* boxHost;
@property(nonatomic, assign) BOOL shown;
@property(nonatomic, assign) BOOL passesPointer;
@property(nonatomic, assign) NSRect contentFrame;
@property(nonatomic, assign) BOOL shadowStale;
@property(nonatomic, assign) BOOL presentationPending;

- (BOOL)containsWindowPoint:(NSPoint)windowPoint;
- (BOOL)nativeControlBlocksWindowPoint:(NSPoint)windowPoint;
- (void)markShadowStale;
- (void)scheduleShadowSettle;
- (void)hide;

@end

// **********************************************************************************************

@interface YuGuiAppDelegate : NSObject <NSApplicationDelegate>
@end

static void guiMainQueueRequestStop(void);
static void guiRequestAppQuit(void);
static void guiApplyKeepsDisplayAwake(void);
static YuGuiActionCallback guiAppQuitCallback = nullptr;
static void* guiAppQuitContext = nullptr;
static YuGuiActionCallback guiAppBackgroundCallback = nullptr;
static void* guiAppBackgroundContext = nullptr;
static YuGuiOpenCallback guiAppOpenCallback = nullptr;
static void* guiAppOpenContext = nullptr;
static YuGuiActionCallback guiAppSystemCallback = nullptr;
static void* guiAppSystemContext = nullptr;
static bool guiAppKeepsDisplayAwake = false;
static id<NSObject> guiAppDisplayAwakeActivity = nil;
static NSInteger guiStatusItemCount = 0;

@implementation YuGuiAppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    if (guiStatusItemCount == 0) {
        guiMainQueueRequestStop();
    }
    return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    guiRequestAppQuit();
    return NSTerminateCancel;
}

- (void)applicationDidBecomeActive:(NSNotification*)notification {
    guiApplyKeepsDisplayAwake();
}

- (void)applicationDidResignActive:(NSNotification*)notification {
    guiApplyKeepsDisplayAwake();
    if (guiAppBackgroundCallback) {
        guiScheduleActionCallback(guiAppBackgroundCallback, guiAppBackgroundContext, 0);
    }
}

- (void)application:(NSApplication*)application openURLs:(NSArray<NSURL*>*)urls {
    guiScheduleOpenCallback(guiAppOpenCallback, guiAppOpenContext, urls);
}

@end

static YuGuiAppDelegate* appDelegate = nil;

static const size_t kGuiMainQueueTaskBudget = 64;
static const uint64_t kGuiMainQueueNanosecondBudget = 2000000ull;
static const uint64_t kGuiIdleCollectNanoseconds = 1000000ull;
static const uint64_t kGuiIdleCollectQuietNanoseconds = 100000000ull;
static const uint64_t kGuiIdleCollectIntervalNanoseconds = 20000000ull;
static const uint64_t kGuiIdleReleaseQuietNanoseconds = 5000000000ull;
static std::mutex guiMainQueueSourceMutex;
static CFRunLoopSourceRef guiMainQueueSource = nullptr;
static CFRunLoopRef guiMainQueueRunLoop = nullptr;
static std::atomic_bool guiMainQueueWakePending(false);
static std::atomic_bool guiMainQueueStopRequested(false);

static void
guiMainQueueSignalSource(void) {
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
guiMainQueueWake(void*) {
    guiMainQueueWakePending.store(true, std::memory_order_release);
    guiMainQueueSignalSource();
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
    guiMainActivityNanoseconds = yumiMonotonicTimeNanoseconds();
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
                    yumiProcessTerminationRequested()) {
                return;
            }
            uint64_t quiet = yumiMonotonicTimeNanoseconds() - guiMainActivityNanoseconds;
            if (quiet < kGuiIdleReleaseQuietNanoseconds) {
                guiMainQueueScheduleIdleRelease(kGuiIdleReleaseQuietNanoseconds - quiet);
                return;
            }
            yumiGuiMetalReleaseIdle();
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
                    yumiProcessTerminationRequested()) {
                return;
            }
            if (guiMainActivityCount != guiIdleCollectArmedCount) {
                guiMainQueueScheduleIdleCollect(kGuiIdleCollectQuietNanoseconds);
                return;
            }
            if (yumiGCRuntimeIdleCollect(kGuiIdleCollectNanoseconds)) {
                guiMainQueueScheduleIdleCollect(kGuiIdleCollectIntervalNanoseconds);
            }
            guiMainQueueScheduleIdleRelease(kGuiIdleReleaseQuietNanoseconds);
        });
}

static void
guiMainQueueSourcePerform(void*) {
    guiMainQueueWakePending.store(false, std::memory_order_release);
    guiMainNoteActivity();

    uint64_t now = yumiMonotonicTimeNanoseconds();
    uint64_t deadline = now > UINT64_MAX - kGuiMainQueueNanosecondBudget
        ? UINT64_MAX
        : now + kGuiMainQueueNanosecondBudget;
    uint32_t result = yumiDrainMainTaskQueue(kGuiMainQueueTaskBudget, deadline);
    if (result == 0 &&
            !guiMainQueueStopRequested.load(std::memory_order_acquire) &&
            !yumiProcessTerminationRequested()) {
        guiMainQueueScheduleIdleCollect(kGuiIdleCollectQuietNanoseconds);
    }

    if (guiMainQueueStopRequested.load(std::memory_order_acquire) ||
        yumiProcessTerminationRequested()) {
        [NSApp stop:nil];
        NSEvent* wake = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                           location:NSZeroPoint
                                      modifierFlags:0
                                          timestamp:0
                                       windowNumber:0
                                            context:nil
                                            subtype:0
                                              data1:0
                                              data2:0];
        [NSApp postEvent:wake atStart:NO];
    }
    if (result == 1) {
        guiMainQueueWake(nullptr);
    }
}

static bool
guiMainQueueInstallSource(void) {
    CFRunLoopSourceContext context = {};
    context.perform = guiMainQueueSourcePerform;
    CFRunLoopSourceRef source = CFRunLoopSourceCreate(nullptr, 0, &context);
    if (!source) {
        return false;
    }
    CFRunLoopRef runLoop = CFRunLoopGetMain();

    {
        std::lock_guard<std::mutex> lock(guiMainQueueSourceMutex);
        guiMainQueueSource = source;
        guiMainQueueRunLoop = (CFRunLoopRef)CFRetain(runLoop);
    }

    CFRunLoopAddSource(runLoop, source, kCFRunLoopCommonModes);
    CFRunLoopAddSource(runLoop, source, (__bridge CFStringRef)NSEventTrackingRunLoopMode);
    CFRunLoopAddSource(runLoop, source, (__bridge CFStringRef)NSModalPanelRunLoopMode);
    guiMainQueueWake(nullptr);
    return true;
}

static void
guiMainQueueRemoveSource(void) {
    CFRunLoopSourceRef source = nullptr;
    CFRunLoopRef runLoop = nullptr;
    {
        std::lock_guard<std::mutex> lock(guiMainQueueSourceMutex);
        source = guiMainQueueSource;
        runLoop = guiMainQueueRunLoop;
        guiMainQueueSource = nullptr;
        guiMainQueueRunLoop = nullptr;
    }

    if (source) {
        CFRunLoopSourceInvalidate(source);
        CFRelease(source);
    }
    if (runLoop) {
        CFRelease(runLoop);
    }
    guiMainQueueWakePending.store(false, std::memory_order_release);
}

static void
guiMainQueueRunHost(void*) {
    guiMainQueueStopRequested.store(false, std::memory_order_release);
    if (!guiMainQueueInstallSource()) {
        std::fprintf(stderr, "Unable to install the AppKit main task queue source\n");
        std::abort();
    }
    if ([NSApp activationPolicy] == NSApplicationActivationPolicyRegular) {
        [NSApp activateIgnoringOtherApps:YES];
    }
    [NSApp run];
    guiMainQueueRemoveSource();
}

// **********************************************************************************************

@interface YuGuiFlippedView : NSView
@end

@implementation YuGuiFlippedView

- (BOOL)isFlipped {
    return YES;
}

- (void)mouseDown:(NSEvent*)event {
}

- (NSRect)_opaqueRectForWindowMoveWhenInTitlebar {
    return [self bounds];
}

@end

@interface YuGuiStoplightView : YuGuiFlippedView {
    NSButton* buttons[3];
    NSView* nativeParents[3];
    NSRect nativeFrames[3];
    NSRect overlayFrames[3];
    NSUInteger nativeMasks[3];
    BOOL restoringButtons;
}

@property(nonatomic, assign) NSPoint offset;
@property(nonatomic, assign) CGFloat zoom;
- (void)adoptButtonsFromWindow:(NSWindow*)window;
- (void)restoreButtons;
- (void)reclaimButtons;
- (CGFloat)nativeTrailingEdge;

@end

@implementation YuGuiStoplightView

- (CGFloat)nativeTrailingEdge {
    CGFloat edge = 0;
    for (size_t i = 0; i < 3; ++i) {
        if (buttons[i]) {
            edge = MAX(edge, NSMaxX([self convertRect:overlayFrames[i] toView:nil]));
        }
    }
    return edge;
}

- (void)adoptButtonsFromWindow:(NSWindow*)window {
    const NSWindowButton kinds[] = {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton};
    for (size_t i = 0; i < 3; ++i) {
        buttons[i] = [window standardWindowButton:kinds[i]];
        nativeParents[i] = [buttons[i] superview];
        nativeFrames[i] = [buttons[i] frame];
        nativeMasks[i] = [buttons[i] autoresizingMask];
        overlayFrames[i] = [self convertRect:[buttons[i] bounds] fromView:buttons[i]];
        [self addSubview:buttons[i]];
        [buttons[i] setAutoresizingMask:NSViewNotSizable];
    }
    [self setNeedsLayout:YES];
}

- (void)layout {
    [super layout];
    CGFloat top = NSMinY(overlayFrames[0]);
    CGFloat bottom = NSMaxY(overlayFrames[0]);
    for (size_t i = 1; i < 3; ++i) {
        top = MIN(top, NSMinY(overlayFrames[i]));
        bottom = MAX(bottom, NSMaxY(overlayFrames[i]));
    }
    CGFloat zoom = self.zoom > 0 ? self.zoom : 1;
    CGFloat center = (top + bottom) / 2;
    CGFloat scale = [[self window] backingScaleFactor] > 0 ? [[self window] backingScaleFactor] : 1;
    CGFloat dy = round((zoom * (center + self.offset.y) - center) * scale) / scale;
    if (zoom != 1) {
        dy = MAX(dy, -top);
    }
    for (size_t i = 0; i < 3; ++i) {
        [buttons[i] setFrame:NSOffsetRect(overlayFrames[i], self.offset.x, dy)];
    }
}

- (void)reclaimButtons {
    BOOL reclaimed = NO;
    for (size_t i = 0; i < 3; ++i) {
        NSView* parent = [buttons[i] superview];
        if (buttons[i] && parent != self) {
            if (parent) {
                nativeParents[i] = parent;
            }
            [self addSubview:buttons[i]];
            [buttons[i] setAutoresizingMask:NSViewNotSizable];
            reclaimed = YES;
        }
    }
    if (reclaimed) [self setNeedsLayout:YES];
}

- (void)willRemoveSubview:(NSView*)subview {
    [super willRemoveSubview:subview];
    if (restoringButtons) return;
    dispatch_async(dispatch_get_main_queue(), ^{
        id delegate = [[self window] delegate];
        if ([delegate respondsToSelector:@selector(updateStoplights)]) {
            [delegate updateStoplights];
        }
    });
}

- (NSView*)hitTest:(NSPoint)point {
    NSView* hit = [super hitTest:point];
    return hit == self ? nil : hit;
}

- (void)restoreButtons {
    restoringButtons = YES;
    for (size_t i = 0; i < 3; ++i) {
        if (buttons[i]) {
            if ([buttons[i] superview] == self) {
                [nativeParents[i] addSubview:buttons[i]];
                [buttons[i] setFrame:nativeFrames[i]];
                [buttons[i] setAutoresizingMask:nativeMasks[i]];
            }
            buttons[i] = nil;
            nativeParents[i] = nil;
        }
    }
}

- (void)dealloc {
    [self restoreButtons];
}

@end

// **********************************************************************************************

@interface YuGuiBoxHostView : YuGuiFlippedView
@end

@implementation YuGuiBoxHostView

// Box hosts only exist to parent native controls inside a clipping box. They must not absorb the
// pointer themselves, or the canvas beneath them never sees it.
- (NSView*)hitTest:(NSPoint)point {
    NSView* hit = [super hitTest:point];
    return hit == self ? nil : hit;
}

@end

// **********************************************************************************************

@interface YuGuiMaterialView : NSVisualEffectView

@property(nonatomic) int32_t depth;

@end

@implementation YuGuiMaterialView

- (NSView*)hitTest:(NSPoint)point {
    return nil;
}

@end

static bool
guiIsTextControl(id object) {
    return [object conformsToProtocol:@protocol(YuGuiTextControl)];
}

static NSRect
guiViewHitBounds(NSView* view) {
    NSRect bounds = [view bounds];
    if (!guiIsTextControl(view)) {
        return bounds;
    }
    YuGuiTextInputHost* host = static_cast<id<YuGuiTextControl>>(view).inputHost;
    if (!host.hasHitRect) {
        return bounds;
    }
    NSRect hit = host.hitRect;
    CGFloat y = [view isFlipped] ? NSMinY(bounds) + NSMinY(hit) : NSMaxY(bounds) - NSMaxY(hit);
    return NSIntersectionRect(bounds, NSMakeRect(NSMinX(bounds) + NSMinX(hit), y, NSWidth(hit), NSHeight(hit)));
}

static BOOL
guiTextControlContains(NSView* control, NSPoint superviewPoint) {
    NSPoint local = [control convertPoint:superviewPoint fromView:[control superview]];
    return NSPointInRect(local, guiViewHitBounds(control));
}

static BOOL
guiViewBlocksCanvas(NSView* view, NSPoint windowPoint) {
    if ([view isHidden] || [view isKindOfClass:[YuGuiMaterialView class]]) {
        return NO;
    }
    NSPoint local = [view convertPoint:windowPoint fromView:nil];
    if (!NSPointInRect(local, guiViewHitBounds(view))) {
        return NO;
    }
    // A box host is a clipping container, not a control, so only its real controls block the canvas.
    if ([view isKindOfClass:[YuGuiBoxHostView class]] ||
        [view isKindOfClass:[YuGuiStoplightView class]]) {
        for (NSView* subview in [view subviews]) {
            if (guiViewBlocksCanvas(subview, windowPoint)) {
                return YES;
            }
        }
        return NO;
    }
    return YES;
}

static char guiPeerOwnsKeysKey;
static char guiPeerOwnsCursorKey;
static char guiPeerPassesScrollKey;

static bool
guiViewHasPeerFlag(NSView* view, const void* key) {
    for (; view; view = [view superview]) {
        if (objc_getAssociatedObject(view, key)) {
            return true;
        }
    }
    return false;
}

static bool
guiResponderPeerOwnsKeys(NSResponder* responder) {
    if (![responder isKindOfClass:[NSView class]]) {
        return false;
    }
    return guiViewHasPeerFlag(static_cast<NSView*>(responder), &guiPeerOwnsKeysKey);
}

static bool
guiIsHelpMenuKey(NSEvent* event) {
    if ([event type] != NSEventTypeKeyDown || [NSApp helpMenu]) {
        return false;
    }
    unsigned short keyCode = 44;
    NSUInteger modifiers = NSEventModifierFlagCommand | NSEventModifierFlagShift;
    id hotKeys = CFBridgingRelease(
        CFPreferencesCopyAppValue(CFSTR("AppleSymbolicHotKeys"), CFSTR("com.apple.symbolichotkeys"))
    );
    id help = [hotKeys isKindOfClass:[NSDictionary class]] ? hotKeys[@"98"] : nil;
    if ([help isKindOfClass:[NSDictionary class]]) {
        if (![help[@"enabled"] boolValue]) {
            return false;
        }
        id value = help[@"value"];
        id parameters = [value isKindOfClass:[NSDictionary class]] ? value[@"parameters"] : nil;
        if ([parameters isKindOfClass:[NSArray class]] && [parameters count] >= 3) {
            keyCode = [parameters[1] unsignedShortValue];
            modifiers = [parameters[2] unsignedIntegerValue];
        }
    }
    NSEventModifierFlags mask = NSEventModifierFlagCommand | NSEventModifierFlagShift |
        NSEventModifierFlagOption | NSEventModifierFlagControl;
    return [event keyCode] == keyCode && ([event modifierFlags] & mask) == (modifiers & mask);
}

static bool
guiWindowPeerPassesScroll(NSWindow* window, NSPoint windowPoint) {
    NSView* content = [window contentView];
    NSView* frame = [content superview];
    if (!frame) {
        return false;
    }
    NSView* hit = [content hitTest:[frame convertPoint:windowPoint fromView:nil]];
    return guiViewHasPeerFlag(hit, &guiPeerPassesScrollKey);
}

static bool
guiViewContainsCursorPeer(NSView* view) {
    for (NSView* subview in [view subviews]) {
        if (objc_getAssociatedObject(subview, &guiPeerOwnsCursorKey) || guiViewContainsCursorPeer(subview)) {
            return true;
        }
    }
    return false;
}

static void
guiCollectCursorPeerRects(NSView* container, NSView* target, std::vector<NSRect>& holes) {
    for (NSView* subview in [container subviews]) {
        if (subview == target || [subview isHidden]) {
            continue;
        }
        if (objc_getAssociatedObject(subview, &guiPeerOwnsCursorKey)) {
            NSRect visible = [subview visibleRect];
            if (!NSIsEmptyRect(visible)) {
                holes.push_back([subview convertRect:visible toView:target]);
            }
            continue;
        }
        guiCollectCursorPeerRects(subview, target, holes);
    }
}

static void
guiAddCursorRectsAroundPeers(NSView* view, NSCursor* cursor) {
    std::vector<NSRect> holes;
    guiCollectCursorPeerRects([view superview], view, holes);
    std::vector<NSRect> rects(1, [view bounds]);
    for (const NSRect& hole : holes) {
        std::vector<NSRect> remaining;
        for (const NSRect& rect : rects) {
            NSRect cut = NSIntersectionRect(rect, hole);
            if (NSIsEmptyRect(cut)) {
                remaining.push_back(rect);
                continue;
            }
            if (NSMinY(cut) > NSMinY(rect)) {
                remaining.push_back(NSMakeRect(NSMinX(rect), NSMinY(rect), NSWidth(rect), NSMinY(cut) - NSMinY(rect)));
            }
            if (NSMaxY(cut) < NSMaxY(rect)) {
                remaining.push_back(NSMakeRect(NSMinX(rect), NSMaxY(cut), NSWidth(rect), NSMaxY(rect) - NSMaxY(cut)));
            }
            if (NSMinX(cut) > NSMinX(rect)) {
                remaining.push_back(NSMakeRect(NSMinX(rect), NSMinY(cut), NSMinX(cut) - NSMinX(rect), NSHeight(cut)));
            }
            if (NSMaxX(cut) < NSMaxX(rect)) {
                remaining.push_back(NSMakeRect(NSMaxX(cut), NSMinY(cut), NSMaxX(rect) - NSMaxX(cut), NSHeight(cut)));
            }
        }
        rects.swap(remaining);
    }
    for (const NSRect& rect : rects) {
        [view addCursorRect:rect cursor:cursor];
    }
}

// **********************************************************************************************

static bool
guiIsPointerEventType(NSEventType type) {
    return type == NSEventTypeMouseMoved ||
        type == NSEventTypeMouseEntered ||
        type == NSEventTypeMouseExited ||
        type == NSEventTypeLeftMouseDown ||
        type == NSEventTypeRightMouseDown ||
        type == NSEventTypeOtherMouseDown ||
        type == NSEventTypeLeftMouseUp ||
        type == NSEventTypeRightMouseUp ||
        type == NSEventTypeOtherMouseUp ||
        type == NSEventTypeLeftMouseDragged ||
        type == NSEventTypeRightMouseDragged ||
        type == NSEventTypeOtherMouseDragged ||
        type == NSEventTypeScrollWheel;
}

static bool
guiIsPointerButtonEventType(NSEventType type) {
    return type == NSEventTypeLeftMouseDown ||
        type == NSEventTypeRightMouseDown ||
        type == NSEventTypeOtherMouseDown ||
        type == NSEventTypeLeftMouseUp ||
        type == NSEventTypeRightMouseUp ||
        type == NSEventTypeOtherMouseUp ||
        type == NSEventTypeLeftMouseDragged ||
        type == NSEventTypeRightMouseDragged ||
        type == NSEventTypeOtherMouseDragged;
}

static bool
guiIsPointerMotionEventType(NSEventType type) {
    return type == NSEventTypeMouseMoved ||
        type == NSEventTypeLeftMouseDragged ||
        type == NSEventTypeRightMouseDragged ||
        type == NSEventTypeOtherMouseDragged;
}

static uint64_t
guiModifierBits(NSEventModifierFlags nativeModifiers) {
    uint64_t modifiers = 0;
    if (nativeModifiers & NSEventModifierFlagShift) modifiers |= 1;
    if (nativeModifiers & NSEventModifierFlagControl) modifiers |= 2;
    if (nativeModifiers & NSEventModifierFlagOption) modifiers |= 4;
    if (nativeModifiers & NSEventModifierFlagCommand) modifiers |= 8;
    if (nativeModifiers & NSEventModifierFlagCapsLock) modifiers |= 16;
    if (nativeModifiers & NSEventModifierFlagFunction) modifiers |= 32;
    return modifiers;
}

static NSArray<NSPasteboardType>*
guiDropTypes(void) {
    NSMutableArray<NSPasteboardType>* types = [NSMutableArray arrayWithArray:[NSImage imageTypes]];
    [types addObjectsFromArray:[NSFilePromiseReceiver readableDraggedTypes]];
    [types addObjectsFromArray:@[NSPasteboardTypeFileURL, NSPasteboardTypeURL, NSPasteboardTypeString]];
    return types;
}

static BOOL
guiPasteboardHoldsImages(NSPasteboard* pasteboard) {
    NSArray<NSString*>* imageTypes = @[UTTypeImage.identifier];
    if ([pasteboard canReadItemWithDataConformingToTypes:imageTypes]) {
        return YES;
    }
    NSDictionary* options = @{
        NSPasteboardURLReadingFileURLsOnlyKey: @YES,
        NSPasteboardURLReadingContentsConformToTypesKey: imageTypes,
    };
    if ([pasteboard canReadObjectForClasses:@[[NSURL class]] options:options]) {
        return YES;
    }
    for (NSFilePromiseReceiver* receiver in [pasteboard readObjectsForClasses:@[[NSFilePromiseReceiver class]] options:nil]) {
        for (NSString* type in [receiver fileTypes]) {
            if ([[UTType typeWithIdentifier:type] conformsToType:UTTypeImage]) {
                return YES;
            }
        }
    }
    return NO;
}

static bool
guiUsesFieldEditor(id object) {
    return [object isKindOfClass:[YuGuiTextField class]];
}

static NSTextView*
guiMakeFieldEditor(void) {
    YuGuiFieldEditor* editor = [[YuGuiFieldEditor alloc] initWithFrame:NSZeroRect];
    [editor setFieldEditor:YES];
    return editor;
}

static NSDragOperation
guiDropOperation(uint32_t operation, NSDragOperation allowed) {
    NSDragOperation wanted = operation == 1 ? NSDragOperationCopy
        : operation == 2 ? NSDragOperationLink
        : operation == 3 ? NSDragOperationMove
        : NSDragOperationNone;
    if (wanted == NSDragOperationNone || (allowed & wanted)) {
        return wanted;
    }
    for (NSDragOperation fallback : {NSDragOperationCopy, NSDragOperationGeneric, NSDragOperationMove, NSDragOperationLink}) {
        if (allowed & fallback) {
            return fallback;
        }
    }
    return NSDragOperationNone;
}

struct YuGuiPromiseBatch {
    std::mutex mutex;
    std::condition_variable settled;
    uint64_t pending = 0;
    std::vector<std::pair<uint64_t, std::string>> files;
    std::vector<std::string> paths;
};

static std::mutex guiPromiseBatchesMutex;
static std::unordered_map<uint64_t, std::shared_ptr<YuGuiPromiseBatch>> guiPromiseBatches;

static std::shared_ptr<YuGuiPromiseBatch>
guiPromiseBatch(uint64_t sequence) {
    std::lock_guard<std::mutex> lock(guiPromiseBatchesMutex);
    auto found = guiPromiseBatches.find(sequence);
    return found == guiPromiseBatches.end() ? nullptr : found->second;
}

static void
guiReceivePromisedFiles(id<NSDraggingInfo> info) {
    @autoreleasepool {
        NSArray<NSFilePromiseReceiver*>* receivers =
            [[info draggingPasteboard] readObjectsForClasses:@[[NSFilePromiseReceiver class]] options:nil];
        if (![receivers count]) {
            return;
        }
        NSString* name = [NSString stringWithFormat:@"yumi-drop-%@", [[NSUUID UUID] UUIDString]];
        NSURL* destination = [[NSURL fileURLWithPath:NSTemporaryDirectory()] URLByAppendingPathComponent:name
                                                                                           isDirectory:YES];
        if (![[NSFileManager defaultManager] createDirectoryAtURL:destination
                                      withIntermediateDirectories:YES
                                                       attributes:nil
                                                            error:nil]) {
            return;
        }
        std::shared_ptr<YuGuiPromiseBatch> batch = std::make_shared<YuGuiPromiseBatch>();
        for (NSFilePromiseReceiver* receiver in receivers) {
            batch->pending += std::max<uint64_t>([[receiver fileTypes] count], 1);
        }
        {
            std::lock_guard<std::mutex> lock(guiPromiseBatchesMutex);
            guiPromiseBatches[static_cast<uint64_t>([info draggingSequenceNumber])] = batch;
        }
        static NSOperationQueue* queue = [[NSOperationQueue alloc] init];
        uint64_t order = 0;
        for (NSFilePromiseReceiver* receiver in receivers) {
            uint64_t index = order++;
            [receiver receivePromisedFilesAtDestination:destination
                                                options:@{}
                                         operationQueue:queue
                                                 reader:^(NSURL* file, NSError* error) {
                std::lock_guard<std::mutex> lock(batch->mutex);
                if (!error && [file isFileURL]) {
                    batch->files.emplace_back(index, [[file path] UTF8String] ?: "");
                }
                if (batch->pending) {
                    --batch->pending;
                }
                batch->settled.notify_all();
            }];
        }
    }
}

static uint32_t
guiLogicalKeyKind(unsigned short keyCode) {
    switch (keyCode) {
        case 48: return 1;   // Tab
        case 36:
        case 76: return 2;   // Return and keypad Enter
        case 53: return 3;   // Escape
        case 49: return 4;   // Space
        case 126: return 5;  // Up
        case 125: return 6;  // Down
        case 123: return 7;  // Left
        case 124: return 8;  // Right
        case 115: return 9;  // Home
        case 119: return 10; // End
        case 116: return 11; // Page Up
        case 121: return 12; // Page Down
        case 51: return 13;  // Backspace
        case 117: return 14; // Forward Delete
        case 122: return 100;
        case 120: return 101;
        case 99: return 102;
        case 118: return 103;
        case 96: return 104;
        case 97: return 105;
        case 98: return 106;
        case 100: return 107;
        case 101: return 108;
        case 109: return 109;
        case 103: return 110;
        case 111: return 111;
        case 105: return 112;
        case 107: return 113;
        case 113: return 114;
        case 106: return 115;
        default: return 0;
    }
}

@interface YuGuiCanvasView : YuGuiFlippedView <NSTextInputClient>

@property(nonatomic, weak) YuGuiWindowHost* inputHost;
@property(nonatomic, strong) NSTrackingArea* pointerTrackingArea;
@property(nonatomic, assign) uint32_t cursorStyle;
@property(nonatomic, strong) NSCursor* customCursor;
@property(nonatomic, assign) BOOL takesFocus;

- (void)setCanvasImage:(CGImageRef)image scale:(CGFloat)scale;
- (void)setCanvasSurface:(IOSurfaceRef)surface scale:(CGFloat)scale;

@end

@interface YuGuiMetalView : NSView
@property(nonatomic, assign) uint32_t cursorStyle;
@property(nonatomic, strong) NSCursor* customCursor;
@end

@implementation YuGuiMetalView

- (CALayer*)makeBackingLayer {
    return [CAMetalLayer layer];
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
    return YES;
}

- (void)resetCursorRects {
    guiAddCursorRectsAroundPeers(self, guiCursorForStyle(self.cursorStyle, false, self.customCursor));
}

@end

static bool
guiIsUndoAction(SEL action) {
    return action == @selector(undo:) || action == @selector(redo:);
}

static bool
guiIsZoomAction(SEL action) {
    return action == @selector(yumiZoomIn:) || action == @selector(yumiZoomOut:) ||
        action == @selector(yumiActualSize:);
}

static BOOL
guiValidateRoutedUndo(YuGuiWindowHost* host, NSMenuItem* item) {
    bool redoes = [item action] == @selector(redo:);
    bool available = host && (redoes ? host->redoAvailable : host->undoAvailable);
    const std::string& title = redoes ? host->redoTitle : host->undoTitle;
    [item setTitle:available ? [NSString stringWithUTF8String:title.c_str()] : (redoes ? @"Redo" : @"Undo")];
    return available;
}

static void
guiPerformEditKey(YuGuiWindowHost* host, NSWindow* window, NSString* key, unsigned short keyCode, BOOL shift) {
    if (!host || !window) {
        return;
    }
    NSEventModifierFlags flags = NSEventModifierFlagCommand | (shift ? NSEventModifierFlagShift : 0);
    NSString* typed = shift ? [key uppercaseString] : key;
    NSTimeInterval now = [[NSProcessInfo processInfo] systemUptime];
    NSEvent* press = [NSEvent keyEventWithType:NSEventTypeKeyDown
                                      location:NSZeroPoint
                                 modifierFlags:flags
                                     timestamp:now
                                  windowNumber:[window windowNumber]
                                       context:nil
                                    characters:typed
                   charactersIgnoringModifiers:typed
                                     isARepeat:NO
                                       keyCode:keyCode];
    NSEvent* release = [NSEvent keyEventWithType:NSEventTypeKeyUp
                                        location:NSZeroPoint
                                   modifierFlags:flags
                                       timestamp:now
                                    windowNumber:[window windowNumber]
                                         context:nil
                                      characters:typed
                     charactersIgnoringModifiers:typed
                                       isARepeat:NO
                                         keyCode:keyCode];
    [host handleKeyEvent:press];
    [host handleKeyReleaseEvent:release];
}

static bool
guiResponderEditsTextInput(NSResponder* responder) {
    if (guiIsTextControl(responder)) {
        return true;
    }
    if (![responder isKindOfClass:[NSTextView class]]) {
        return false;
    }
    return guiIsTextControl([static_cast<NSTextView*>(responder) delegate]);
}

@implementation YuGuiCanvasView

- (BOOL)yieldsFocusToTextInput {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return self.takesFocus || ![self yieldsFocusToTextInput] ||
        !guiResponderEditsTextInput([[self window] firstResponder]);
}

- (void)resetCursorRects {
    guiAddCursorRectsAroundPeers(self, guiCursorForStyle(self.cursorStyle, false, self.customCursor));
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    if (self.pointerTrackingArea) {
        return;
    }
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc]
        initWithRect:NSZeroRect
             options:NSTrackingMouseMoved |
                     NSTrackingMouseEnteredAndExited |
                     NSTrackingActiveInKeyWindow |
                     NSTrackingInVisibleRect
               owner:self
            userInfo:nil
    ];
    self.pointerTrackingArea = trackingArea;
    [self addTrackingArea:trackingArea];
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
    return YES;
}

- (void)mouseDown:(NSEvent*)event {
    if ([self acceptsFirstResponder]) {
        [[self window] makeFirstResponder:self];
    }
    [super mouseDown:event];
}

- (void)setCanvasImage:(CGImageRef)image scale:(CGFloat)scale {
    CALayer* canvasLayer = [self layer];
    [canvasLayer setContents:(__bridge id)image];
    [canvasLayer setContentsScale:scale > 0 ? scale : 1.0];
}

- (void)setCanvasSurface:(IOSurfaceRef)surface scale:(CGFloat)scale {
    CALayer* canvasLayer = [self layer];
    [canvasLayer setContents:(__bridge id)surface];
    [canvasLayer setContentsScale:scale > 0 ? scale : 1.0];
}

- (NSTextInputContext*)inputContext {
    return self.inputHost.textInputActive ? [super inputContext] : nil;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    [self.inputHost textInputInsert:guiPlainString(string) replacementRange:replacementRange];
}

- (void)doCommandBySelector:(SEL)selector {
    [self.inputHost textInputCommand:selector];
}

- (void)performEditKey:(NSString*)key keyCode:(unsigned short)keyCode shift:(BOOL)shift {
    guiPerformEditKey(self.inputHost, [self window], key, keyCode, shift);
}

- (void)undo:(id)sender {
    [self performEditKey:@"z" keyCode:6 shift:NO];
}

- (void)redo:(id)sender {
    [self performEditKey:@"z" keyCode:6 shift:YES];
}

- (void)cut:(id)sender {
    [self performEditKey:@"x" keyCode:7 shift:NO];
}

- (void)copy:(id)sender {
    [self performEditKey:@"c" keyCode:8 shift:NO];
}

- (void)paste:(id)sender {
    [self performEditKey:@"v" keyCode:9 shift:NO];
}

- (void)selectAll:(id)sender {
    [self performEditKey:@"a" keyCode:0 shift:NO];
}

- (BOOL)validateMenuItem:(NSMenuItem*)item {
    if (self.inputHost == nil || [self window] == nil) {
        return NO;
    }
    if (guiIsUndoAction([item action])) {
        return guiValidateRoutedUndo(self.inputHost, item);
    }
    return YES;
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
    [self.inputHost textInputSetMarkedText:guiPlainString(string) selectedRange:selectedRange];
}

- (void)unmarkText {
    [self.inputHost textInputUnmark];
}

- (NSRange)selectedRange {
    return self.inputHost ? [self.inputHost textInputSelectedRange] : NSMakeRange(NSNotFound, 0);
}

- (NSRange)markedRange {
    return self.inputHost ? [self.inputHost textInputMarkedRange] : NSMakeRange(NSNotFound, 0);
}

- (BOOL)hasMarkedText {
    return [self.inputHost hasMarkedText];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
    return [self.inputHost textInputSubstring:range actualRange:actualRange];
}

- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText {
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    if (actualRange) {
        *actualRange = range;
    }
    return self.inputHost ? [self.inputHost textInputScreenRect] : NSZeroRect;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return NSNotFound;
}

@end

@implementation YuGuiEventWindow

- (NSText*)fieldEditor:(BOOL)createFlag forObject:(id)object {
    if (!guiUsesFieldEditor(object)) {
        return [super fieldEditor:createFlag forObject:object];
    }
    if (!self.guiFieldEditor && createFlag) {
        self.guiFieldEditor = guiMakeFieldEditor();
    }
    return self.guiFieldEditor;
}

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
    if (guiIsHelpMenuKey(event) && [self.guiWindow handleKeyEvent:event]) {
        return YES;
    }
    return [super performKeyEquivalent:event];
}

- (void)yumiZoomIn:(id)sender {
    [self.guiWindow dispatchWindowEventKind:12 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
}

- (void)yumiZoomOut:(id)sender {
    [self.guiWindow dispatchWindowEventKind:13 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
}

- (void)yumiActualSize:(id)sender {
    [self.guiWindow dispatchWindowEventKind:14 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
}

- (BOOL)validateMenuItem:(NSMenuItem*)item {
    if (guiIsZoomAction([item action])) {
        return self.guiWindow.allowsZoom;
    }
    return [super validateMenuItem:item];
}

- (void)sendEvent:(NSEvent*)event {
    if ([event type] == NSEventTypeFlagsChanged) {
        [self.guiWindow dispatchWindowEventKind:7 key:0 nativeKeyCode:0
                                     modifiers:guiModifierBits([event modifierFlags]) isRepeat:NO];
    }
    if ([event type] == NSEventTypeScrollWheel || [event type] == NSEventTypeMagnify) {
        NSPoint windowPoint = [self.guiWindow windowPointForEvent:event];
        if ([self.guiWindow isCanvasTargetAtWindowPoint:windowPoint] ||
            ([event type] == NSEventTypeScrollWheel && guiWindowPeerPassesScroll(self, windowPoint))) {
            [self.guiWindow dispatchPointerEvent:event kind:[event type] == NSEventTypeMagnify ? 7 : 4];
            return;
        }
        [super sendEvent:event];
        return;
    }
    if (guiIsPointerEventType([event type])) {
        [self.guiWindow handleWindowEvent:event];
    }
    if ([event type] == NSEventTypeKeyDown && [self.guiWindow handleKeyEvent:event]) {
        return;
    }
    if ([event type] == NSEventTypeKeyUp) {
        [self.guiWindow handleKeyReleaseEvent:event];
    }
    [super sendEvent:event];
}

- (void)selectKeyViewFollowingView:(NSView*)view {
    if (guiResponderPeerOwnsKeys(view)) {
        [self.guiWindow dispatchPeerFocusExit:NO];
        return;
    }
    [super selectKeyViewFollowingView:view];
}

- (void)selectKeyViewPrecedingView:(NSView*)view {
    if (guiResponderPeerOwnsKeys(view)) {
        [self.guiWindow dispatchPeerFocusExit:YES];
        return;
    }
    [super selectKeyViewPrecedingView:view];
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    return [self.guiWindow dispatchDrop:sender kind:9 windowPoint:[sender draggingLocation]];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    return [self.guiWindow dispatchDrop:sender kind:10 windowPoint:[sender draggingLocation]];
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
    [self.guiWindow dispatchDrop:sender kind:11 windowPoint:[sender draggingLocation]];
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    return [self.guiWindow performDrop:sender windowPoint:[sender draggingLocation]];
}

- (void)draggingEnded:(id<NSDraggingInfo>)sender {
    [self.guiWindow dispatchDrop:sender kind:11 windowPoint:[sender draggingLocation]];
}

@end

// **********************************************************************************************

struct YuGuiPendingFrame {
    YuGuiWindowHost* window;
    uint64_t notBeforeNanoseconds;
};

struct YuGuiDisplayLink {
    CGDirectDisplayID display = kCGNullDirectDisplay;
    CVDisplayLinkRef link = nullptr;
    bool running = false;
    bool drainHeld = false;
    std::atomic_bool drainPending{false};
    uint64_t eventLeadNanoseconds = 0;
    uint64_t lastFiredNanoseconds = 0;
    YuGuiFrameTiming latest = {};
    std::vector<YuGuiPendingFrame> pending;
};

static const uint64_t kGuiDisplayLinkIdleGraceNanoseconds = 250000000;

static std::mutex guiFrameSchedulerMutex;
static std::vector<YuGuiDisplayLink*> guiDisplayLinks;

// `CVDisplayLinkStop` waits for the in-flight display-link callback to unwind, and that callback
// takes `guiFrameSchedulerMutex`. Starting or stopping a link while holding that mutex therefore
// deadlocks the main thread against the display-link thread. Each link's handle and run state live
// under their own control mutex instead, which the display-link callback never touches.
static std::mutex guiDisplayLinkControlMutex;

static bool guiFrameLoopWaited = true;
static uint64_t guiFrameEndedNanoseconds = 0;
static CFRunLoopObserverRef guiFrameWaitObserver = nullptr;
static uint64_t guiFrameSequence = 0;
static mach_timebase_info_data_t guiMachTimebase = {0, 0};
static std::once_flag guiMachTimebaseOnce;

static void
guiFrameSchedulerDrain(YuGuiDisplayLink* entry);

static bool
guiFrameSchedulerHasDue(const YuGuiDisplayLink* entry, uint64_t targetNanoseconds) {
    for (const YuGuiPendingFrame& frame : entry->pending) {
        if (frame.notBeforeNanoseconds <= targetNanoseconds) {
            return true;
        }
    }
    return false;
}

static std::vector<YuGuiPendingFrame>::iterator
guiFrameSchedulerFindPending(YuGuiDisplayLink* entry, YuGuiWindowHost* guiWindow) {
    return std::find_if(entry->pending.begin(), entry->pending.end(), [guiWindow](const YuGuiPendingFrame& frame) {
        return frame.window == guiWindow;
    });
}

static uint64_t
guiMachTimeToNanoseconds(uint64_t machTime) {
    std::call_once(guiMachTimebaseOnce, [] {
        mach_timebase_info(&guiMachTimebase);
    });

    if (!guiMachTimebase.denom) {
        return machTime;
    }

    return static_cast<uint64_t>(
        (static_cast<__uint128_t>(machTime) * guiMachTimebase.numer) / guiMachTimebase.denom
    );
}

static uint64_t
guiTimestampToNanoseconds(const CVTimeStamp* timestamp) {
    if (!timestamp) {
        return 0;
    }

    if (timestamp->flags & kCVTimeStampHostTimeValid) {
        uint64_t absolute = mach_absolute_time();
        uint64_t continuous = mach_continuous_time();
        return guiMachTimeToNanoseconds(timestamp->hostTime + (continuous - absolute));
    }

    if ((timestamp->flags & kCVTimeStampVideoTimeValid) &&
        timestamp->videoTimeScale > 0 &&
        timestamp->videoTime >= 0) {
        return static_cast<uint64_t>(
            (static_cast<__uint128_t>(timestamp->videoTime) * 1000000000ull) /
            static_cast<uint64_t>(timestamp->videoTimeScale)
        );
    }

    return 0;
}

static uint64_t
guiRefreshIntervalNanoseconds(const CVTimeStamp* timestamp) {
    if (!timestamp ||
        !(timestamp->flags & kCVTimeStampVideoRefreshPeriodValid) ||
        timestamp->videoTimeScale <= 0 ||
        timestamp->videoRefreshPeriod <= 0) {
        return 0;
    }

    return static_cast<uint64_t>(
        (static_cast<__uint128_t>(timestamp->videoRefreshPeriod) * 1000000000ull) /
        static_cast<uint64_t>(timestamp->videoTimeScale)
    );
}

static double
guiDisplayLinkEventTimeSeconds(
    YuGuiDisplayLink* entry,
    const CVTimeStamp* now,
    const CVTimeStamp* outputTime,
    uint64_t refreshNanoseconds
) {
    if (!now || !(now->flags & kCVTimeStampHostTimeValid)) {
        return 0;
    }
    uint64_t nowNanoseconds = guiMachTimeToNanoseconds(now->hostTime);
    if (!outputTime || !(outputTime->flags & kCVTimeStampHostTimeValid) || !refreshNanoseconds) {
        return static_cast<double>(nowNanoseconds) / 1e9;
    }
    uint64_t outputNanoseconds = guiMachTimeToNanoseconds(outputTime->hostTime);
    if (outputNanoseconds <= nowNanoseconds) {
        entry->eventLeadNanoseconds = 0;
        return static_cast<double>(nowNanoseconds) / 1e9;
    }
    int64_t lead = static_cast<int64_t>(outputNanoseconds - nowNanoseconds);
    int64_t tracked = static_cast<int64_t>(entry->eventLeadNanoseconds);
    if (!tracked) {
        tracked = lead;
    } else {
        int64_t limit = static_cast<int64_t>(refreshNanoseconds / 16);
        tracked += std::max(-limit, std::min(limit, lead - tracked)) / 32;
    }
    entry->eventLeadNanoseconds = static_cast<uint64_t>(tracked);
    return static_cast<double>(outputNanoseconds - static_cast<uint64_t>(tracked)) / 1e9;
}

static void
guiFrameSchedulerScheduleDrain(YuGuiDisplayLink* entry) {
    if (entry->drainPending.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    CFRunLoopRef mainRunLoop = CFRunLoopGetMain();
    CFRunLoopPerformBlock(mainRunLoop, kCFRunLoopCommonModes, ^{
        guiFrameSchedulerDrain(entry);
    });
    CFRunLoopWakeUp(mainRunLoop);
}

static CVReturn
guiDisplayLinkCallback(
    CVDisplayLinkRef displayLink,
    const CVTimeStamp* now,
    const CVTimeStamp* outputTime,
    CVOptionFlags flagsIn,
    CVOptionFlags* flagsOut,
    void* displayLinkContext
) {
    YuGuiDisplayLink* entry = static_cast<YuGuiDisplayLink*>(displayLinkContext);
    YuGuiFrameTiming timing = {};
    timing.outputTimeNanoseconds = guiTimestampToNanoseconds(now);
    timing.targetTimeNanoseconds = guiTimestampToNanoseconds(outputTime);
    timing.refreshIntervalNanoseconds = guiRefreshIntervalNanoseconds(outputTime);
    timing.eventTimeSeconds =
        guiDisplayLinkEventTimeSeconds(entry, now, outputTime, timing.refreshIntervalNanoseconds);

    bool wanted = false;
    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        timing.sequence = ++guiFrameSequence;
        entry->latest = timing;
        if (entry->pending.empty()) {
            wanted = yumiMonotonicTimeNanoseconds() - entry->lastFiredNanoseconds >=
                kGuiDisplayLinkIdleGraceNanoseconds;
        } else {
            wanted = !timing.targetTimeNanoseconds ||
                guiFrameSchedulerHasDue(entry, timing.targetTimeNanoseconds);
        }
    }

    if (wanted) {
        guiFrameSchedulerScheduleDrain(entry);
    }
    return kCVReturnSuccess;
}

static YuGuiDisplayLink*
guiFrameSchedulerLinkFor(CGDirectDisplayID display) {
    for (YuGuiDisplayLink* entry : guiDisplayLinks) {
        if (entry->display == display) {
            return entry;
        }
    }

    YuGuiDisplayLink* entry = new YuGuiDisplayLink();
    entry->display = display;
    guiDisplayLinks.push_back(entry);
    return entry;
}

static YuGuiDisplayLink*
guiFrameSchedulerTakePending(YuGuiWindowHost* guiWindow) {
    for (YuGuiDisplayLink* entry : guiDisplayLinks) {
        auto found = guiFrameSchedulerFindPending(entry, guiWindow);
        if (found != entry->pending.end()) {
            entry->pending.erase(found);
            return entry;
        }
    }
    return nullptr;
}

static bool
guiFrameSchedulerEnsureDisplayLinkControlled(YuGuiDisplayLink* entry) {
    if (entry->link) {
        return true;
    }

    CVDisplayLinkRef link = nullptr;
    CVReturn result = CVDisplayLinkCreateWithCGDisplay(entry->display, &link);
    if (result != kCVReturnSuccess || !link) {
        return false;
    }

    result = CVDisplayLinkSetOutputCallback(link, guiDisplayLinkCallback, entry);
    if (result != kCVReturnSuccess) {
        CVDisplayLinkRelease(link);
        return false;
    }

    entry->link = link;
    return true;
}

// Reconciles a display's link with the scheduler's desired run state. Callers must not hold
// `guiFrameSchedulerMutex`: both CoreVideo entry points block on the display-link thread, which
// takes that mutex from its callback. Answers whether the link is running afterward.
static bool
guiFrameSchedulerApplyRunState(YuGuiDisplayLink* entry, bool shouldRun) {
    std::lock_guard<std::mutex> lock(guiDisplayLinkControlMutex);
    if (!shouldRun) {
        if (entry->running && entry->link) {
            CVDisplayLinkStop(entry->link);
        }
        entry->running = false;
        return false;
    }

    if (!guiFrameSchedulerEnsureDisplayLinkControlled(entry)) {
        return false;
    }

    if (!entry->running) {
        entry->eventLeadNanoseconds = 0;
        entry->running = CVDisplayLinkStart(entry->link) == kCVReturnSuccess;
    }
    return entry->running;
}

static CGDirectDisplayID
guiWindowDisplayID(YuGuiWindowHost* guiWindow) {
    NSScreen* screen = [guiWindow.window screen];
    NSNumber* number = screen ? [[screen deviceDescription] objectForKey:@"NSScreenNumber"] : nil;
    return number ? static_cast<CGDirectDisplayID>([number unsignedIntValue]) : CGMainDisplayID();
}

static void
guiFrameSchedulerStopIdleLinks(void) {
    std::vector<YuGuiDisplayLink*> idle;
    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        for (YuGuiDisplayLink* entry : guiDisplayLinks) {
            if (entry->pending.empty()) {
                idle.push_back(entry);
            }
        }
    }
    for (YuGuiDisplayLink* entry : idle) {
        guiFrameSchedulerApplyRunState(entry, false);
    }
}

static void
guiFrameSchedulerRehome(void) {
    std::vector<YuGuiWindowHost*> windows;
    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        for (YuGuiDisplayLink* entry : guiDisplayLinks) {
            for (const YuGuiPendingFrame& frame : entry->pending) {
                windows.push_back(frame.window);
            }
            entry->pending.clear();
            entry->drainHeld = false;
        }
    }
    for (YuGuiWindowHost* guiWindow : windows) {
        guiFrameSchedulerRequest(guiWindow);
    }
    guiFrameSchedulerStopIdleLinks();
}

static void
guiFrameSchedulerObserveWait(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void* info) {
    guiFrameLoopWaited = true;
    std::vector<YuGuiDisplayLink*> held;
    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        for (YuGuiDisplayLink* entry : guiDisplayLinks) {
            if (entry->drainHeld) {
                entry->drainHeld = false;
                held.push_back(entry);
            }
        }
    }
    for (YuGuiDisplayLink* entry : held) {
        guiFrameSchedulerDrain(entry);
    }
}

static void
guiFrameSchedulerEnsureWaitObserver(void) {
    if (guiFrameWaitObserver) {
        return;
    }
    CFRunLoopObserverContext context = {};
    guiFrameWaitObserver = CFRunLoopObserverCreate(
        nullptr, kCFRunLoopBeforeWaiting, true, 0, guiFrameSchedulerObserveWait, &context);
    CFRunLoopRef runLoop = CFRunLoopGetMain();
    CFRunLoopAddObserver(runLoop, guiFrameWaitObserver, kCFRunLoopCommonModes);
    CFRunLoopAddObserver(
        runLoop, guiFrameWaitObserver, (__bridge CFStringRef)NSEventTrackingRunLoopMode);
    CFRunLoopAddObserver(
        runLoop, guiFrameWaitObserver, (__bridge CFStringRef)NSModalPanelRunLoopMode);
    [[NSNotificationCenter defaultCenter]
        addObserverForName:NSApplicationDidChangeScreenParametersNotification
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification* notification) {
                    guiFrameSchedulerRehome();
                }];
}

static void
guiFrameSchedulerFrameEnded(void) {
    guiFrameLoopWaited = false;
    guiFrameEndedNanoseconds = yumiMonotonicTimeNanoseconds();
}

static bool
guiFrameSchedulerHoldsForWait(const YuGuiFrameTiming& timing) {
    if (guiFrameLoopWaited || !timing.refreshIntervalNanoseconds) {
        return false;
    }
    uint64_t margin = timing.refreshIntervalNanoseconds / 8;
    if (guiFrameEndedNanoseconds + margin < timing.outputTimeNanoseconds) {
        return false;
    }
    uint64_t now = yumiMonotonicTimeNanoseconds();
    return now - guiFrameEndedNanoseconds < timing.refreshIntervalNanoseconds * 2;
}

static void
guiFrameSchedulerRequest(YuGuiWindowHost* guiWindow) {
    if (!guiWindow) {
        return;
    }

    guiFrameSchedulerEnsureWaitObserver();
    CGDirectDisplayID display = guiWindowDisplayID(guiWindow);
    YuGuiDisplayLink* entry = nullptr;
    YuGuiDisplayLink* left = nullptr;
    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        entry = guiFrameSchedulerLinkFor(display);
        YuGuiDisplayLink* previous = guiFrameSchedulerTakePending(guiWindow);
        if (previous && previous != entry && previous->pending.empty()) {
            left = previous;
        }
        entry->pending.push_back({guiWindow, guiWindow.renderNotBeforeNanoseconds});
    }

    if (left) {
        guiFrameSchedulerApplyRunState(left, false);
    }
    if (!guiFrameSchedulerApplyRunState(entry, true)) {
        guiFrameSchedulerScheduleDrain(entry);
    }
}

static void
guiFrameSchedulerCancel(YuGuiWindowHost* guiWindow) {
    if (!guiWindow) {
        return;
    }

    YuGuiDisplayLink* entry = nullptr;
    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        YuGuiDisplayLink* previous = guiFrameSchedulerTakePending(guiWindow);
        if (previous && previous->pending.empty()) {
            entry = previous;
        }
    }

    if (entry) {
        guiFrameSchedulerApplyRunState(entry, false);
    }
}

static void
guiFrameSchedulerScheduleWindowDrain(YuGuiWindowHost* guiWindow) {
    YuGuiDisplayLink* entry = nullptr;
    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        for (YuGuiDisplayLink* candidate : guiDisplayLinks) {
            if (guiFrameSchedulerFindPending(candidate, guiWindow) != candidate->pending.end()) {
                entry = candidate;
                break;
            }
        }
    }
    if (entry) {
        guiFrameSchedulerScheduleDrain(entry);
    }
}

static void
guiFrameSchedulerDrain(YuGuiDisplayLink* entry) {
    guiMainNoteActivity();
    bool linked = false;
    {
        std::lock_guard<std::mutex> lock(guiDisplayLinkControlMutex);
        linked = entry->running;
    }

    std::vector<YuGuiWindowHost*> windows;
    YuGuiFrameTiming timing = {};

    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        timing = entry->latest;
        uint64_t due = linked && timing.targetTimeNanoseconds ? timing.targetTimeNanoseconds : UINT64_MAX;
        if (guiFrameSchedulerHoldsForWait(timing)) {
            entry->drainHeld = guiFrameSchedulerHasDue(entry, due);
        } else {
            entry->drainHeld = false;
            auto fired = std::stable_partition(entry->pending.begin(), entry->pending.end(), [due](const YuGuiPendingFrame& frame) {
                return frame.notBeforeNanoseconds > due;
            });
            for (auto frame = fired; frame != entry->pending.end(); ++frame) {
                windows.push_back(frame->window);
            }
            entry->pending.erase(fired, entry->pending.end());
        }
    }

    bool fired = !windows.empty();
    for (YuGuiWindowHost* guiWindow : windows) {
        [guiWindow fireRenderFrameWithTiming:timing];
    }

    bool shouldStop = false;
    bool hasPending = false;
    {
        std::lock_guard<std::mutex> lock(guiFrameSchedulerMutex);
        uint64_t now = yumiMonotonicTimeNanoseconds();
        if (fired) {
            entry->lastFiredNanoseconds = now;
        }
        hasPending = !entry->pending.empty();
        shouldStop = !fired && !hasPending &&
            now - entry->lastFiredNanoseconds >= kGuiDisplayLinkIdleGraceNanoseconds;
        entry->drainPending.store(false, std::memory_order_release);
    }

    bool isRunning = guiFrameSchedulerApplyRunState(entry, !shouldStop);
    if (hasPending && !isRunning) {
        guiFrameSchedulerScheduleDrain(entry);
    }
}

static const CFIndex kGuiNativeFlushObserverOrder = -1;
static CFRunLoopObserverRef guiNativeFlushObserver = nullptr;
static std::vector<YuGuiWindowHost*> guiNativeFlushWindows;

static void
guiNativeFlushObserve(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void* info) {
    if (guiNativeFlushWindows.empty()) {
        return;
    }
    guiMainNoteActivity();
    uint32_t drained =
        yumiDrainMainTaskQueue(kGuiMainQueueTaskBudget, guiDeadlineAfter(kGuiMainQueueNanosecondBudget));
    if (drained == 1) {
        guiMainQueueWake(nullptr);
    }
    std::vector<YuGuiWindowHost*> windows;
    windows.swap(guiNativeFlushWindows);
    for (YuGuiWindowHost* guiWindow : windows) {
        [guiWindow flushNativeChange];
    }
}

static void
guiMarkNativeChange(YuGuiWindowHost* guiWindow) {
    if (!guiWindow || guiWindow.closed) {
        return;
    }
    if (!guiNativeFlushObserver) {
        guiNativeFlushObserver = CFRunLoopObserverCreate(
            nullptr,
            kCFRunLoopAllActivities,
            true,
            kGuiNativeFlushObserverOrder,
            guiNativeFlushObserve,
            nullptr
        );
        CFRunLoopAddObserver(CFRunLoopGetMain(), guiNativeFlushObserver, kCFRunLoopCommonModes);
    }
    if (std::find(guiNativeFlushWindows.begin(), guiNativeFlushWindows.end(), guiWindow) ==
        guiNativeFlushWindows.end()) {
        guiNativeFlushWindows.push_back(guiWindow);
    }
}

// **********************************************************************************************

static void
guiReleasePresenter(YuGuiCanvasPresenter* presenter) {
    for (YuGuiPresentSurface& entry : presenter->surfaces) {
        if (entry.surface) {
            CFRelease(entry.surface);
        }
    }
    presenter->surfaces.clear();
    if (presenter->colorSpace) {
        CGColorSpaceRelease(presenter->colorSpace);
        presenter->colorSpace = nullptr;
    }
    presenter->width = 0;
    presenter->height = 0;
    presenter->front = -1;
}

static IOSurfaceRef
guiCreatePresentSurface(int32_t width, int32_t height, CGColorSpaceRef colorSpace) {
    const size_t bytesPerRow = IOSurfaceAlignProperty(kIOSurfaceBytesPerRow, static_cast<size_t>(width) * 4);
    NSDictionary* properties = @{
        (id)kIOSurfaceWidth: @(width),
        (id)kIOSurfaceHeight: @(height),
        (id)kIOSurfaceBytesPerElement: @4,
        (id)kIOSurfaceBytesPerRow: @(bytesPerRow),
        (id)kIOSurfacePixelFormat: @(kCVPixelFormatType_32BGRA),
    };
    IOSurfaceRef surface = IOSurfaceCreate((__bridge CFDictionaryRef)properties);
    if (surface && colorSpace) {
        CFPropertyListRef colorSpaceProperties = CGColorSpaceCopyPropertyList(colorSpace);
        if (colorSpaceProperties) {
            IOSurfaceSetValue(surface, kIOSurfaceColorSpace, colorSpaceProperties);
            CFRelease(colorSpaceProperties);
        }
    }
    return surface;
}

static void
guiIncludeStale(YuGuiPresentSurface& entry, int32_t left, int32_t top, int32_t right, int32_t bottom) {
    if (right <= left || bottom <= top) {
        return;
    }
    if (entry.staleRight <= entry.staleLeft || entry.staleBottom <= entry.staleTop) {
        entry.staleLeft = left;
        entry.staleTop = top;
        entry.staleRight = right;
        entry.staleBottom = bottom;
        return;
    }
    entry.staleLeft = std::min(entry.staleLeft, left);
    entry.staleTop = std::min(entry.staleTop, top);
    entry.staleRight = std::max(entry.staleRight, right);
    entry.staleBottom = std::max(entry.staleBottom, bottom);
}

static int32_t
guiChoosePresentSurface(YuGuiCanvasPresenter* presenter) {
    for (size_t index = 0; index < presenter->surfaces.size(); ++index) {
        const int32_t candidate = static_cast<int32_t>(index);
        if (candidate != presenter->front && !IOSurfaceIsInUse(presenter->surfaces[index].surface)) {
            return candidate;
        }
    }
    if (presenter->surfaces.size() < yuGuiPresentSurfaceLimit) {
        IOSurfaceRef surface = guiCreatePresentSurface(
            presenter->width,
            presenter->height,
            presenter->colorSpace
        );
        if (!surface) {
            return -1;
        }
        YuGuiPresentSurface entry;
        entry.surface = surface;
        entry.staleRight = presenter->width;
        entry.staleBottom = presenter->height;
        presenter->surfaces.push_back(entry);
        return static_cast<int32_t>(presenter->surfaces.size() - 1);
    }
    for (size_t index = 0; index < presenter->surfaces.size(); ++index) {
        if (static_cast<int32_t>(index) != presenter->front) {
            return static_cast<int32_t>(index);
        }
    }
    return -1;
}

static IOSurfaceRef
guiPresentCanvas(
    YuGuiCanvasPresenter* presenter,
    void* canvas,
    double x,
    double y,
    double width,
    double height) {
    const int32_t pixelWidth = yumiGraphicsCanvasGetPixelWidth(canvas);
    const int32_t pixelHeight = yumiGraphicsCanvasGetPixelHeight(canvas);
    const uint8_t* pixels = static_cast<const uint8_t*>(yumiGraphicsCanvasGetPixelData(canvas));
    const size_t sourceBytesPerRow = yumiGraphicsCanvasGetBytesPerRow(canvas);
    CGColorSpaceRef colorSpace = static_cast<CGColorSpaceRef>(yumiGraphicsCanvasGetColorSpace(canvas));
    if (pixelWidth <= 0 || pixelHeight <= 0 || !pixels) {
        return nullptr;
    }
    const bool sameSpace = presenter->colorSpace && colorSpace &&
        CFEqual(presenter->colorSpace, colorSpace);
    if (presenter->width != pixelWidth || presenter->height != pixelHeight || !sameSpace) {
        guiReleasePresenter(presenter);
        presenter->width = pixelWidth;
        presenter->height = pixelHeight;
        presenter->colorSpace = colorSpace ? CGColorSpaceRetain(colorSpace) : nullptr;
    }

    const double scale = yumiGraphicsCanvasGetScale(canvas);
    const int32_t left = std::max(0, static_cast<int32_t>(std::floor(x * scale)));
    const int32_t top = std::max(0, static_cast<int32_t>(std::floor(y * scale)));
    const int32_t right = std::min(pixelWidth, static_cast<int32_t>(std::ceil((x + width) * scale)));
    const int32_t bottom = std::min(pixelHeight, static_cast<int32_t>(std::ceil((y + height) * scale)));
    for (YuGuiPresentSurface& entry : presenter->surfaces) {
        guiIncludeStale(entry, left, top, right, bottom);
    }

    const int32_t chosen = guiChoosePresentSurface(presenter);
    if (chosen < 0) {
        return nullptr;
    }
    YuGuiPresentSurface& target = presenter->surfaces[static_cast<size_t>(chosen)];
    if (target.staleRight > target.staleLeft && target.staleBottom > target.staleTop) {
        IOSurfaceLock(target.surface, 0, nullptr);
        uint8_t* destination = static_cast<uint8_t*>(IOSurfaceGetBaseAddress(target.surface));
        const size_t destinationBytesPerRow = IOSurfaceGetBytesPerRow(target.surface);
        const size_t offset = static_cast<size_t>(target.staleLeft) * 4;
        const size_t length = static_cast<size_t>(target.staleRight - target.staleLeft) * 4;
        for (int32_t row = target.staleTop; row < target.staleBottom; ++row) {
            memcpy(
                destination + static_cast<size_t>(row) * destinationBytesPerRow + offset,
                pixels + static_cast<size_t>(row) * sourceBytesPerRow + offset,
                length
            );
        }
        IOSurfaceUnlock(target.surface, 0, nullptr);
        target.staleLeft = 0;
        target.staleTop = 0;
        target.staleRight = 0;
        target.staleBottom = 0;
    }
    presenter->front = chosen;
    return target.surface;
}

@implementation YuGuiAppHost
@end

@implementation YuGuiWindowHost

- (void)beginFrameUpdate {
    if (self.closed || self.frameUpdateOpen) {
        return;
    }

    // The managed frame may change both native subviews and the canvas. Hold their backing-layer
    // mutations in one transaction so AppKit never presents an intermediate layout.
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    self.frameUpdateOpen = YES;
    [self setPresentsWithTransaction:self.couplesPresentation || ![self.window isVisible]];
}

- (void)finishFrameUpdate {
    if (!self.frameUpdateOpen) {
        return;
    }

    self.frameUpdateOpen = NO;
    CALayer* layer = [self.metalView layer];
    BOOL flushes = !self.couplesPresentation && [layer isKindOfClass:[CAMetalLayer class]] &&
        static_cast<CAMetalLayer*>(layer).presentsWithTransaction;
    [CATransaction commit];
    if (flushes) {
        [CATransaction flush];
    }
    [self settleShadow];
}

- (void)markShadowStale {
    self.shadowStale = YES;
    [self scheduleShadowSettle];
}

- (void)scheduleShadowSettle {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self settleShadow];
    });
}

- (void)settleShadow {
    if (!self.shadowStale || self.closed || self.frameUpdateOpen || self.presentationPending ||
            ![self.window isVisible]) {
        return;
    }
    self.shadowStale = NO;
    if ([self.window hasShadow] && ![self.window isOpaque]) {
        [CATransaction flush];
        [self.window invalidateShadow];
    }
}

- (void)setPresentsWithTransaction:(BOOL)presentsWithTransaction {
    CALayer* layer = [self.metalView layer];
    if (![layer isKindOfClass:[CAMetalLayer class]]) {
        return;
    }
    CAMetalLayer* metalLayer = static_cast<CAMetalLayer*>(layer);
    if (metalLayer.presentsWithTransaction != presentsWithTransaction) {
        metalLayer.presentsWithTransaction = presentsWithTransaction;
    }
}

- (void)couplePresentation {
    if (self.frameUpdateOpen) {
        [self setPresentsWithTransaction:YES];
    }
}

- (void)flushNativeChange {
    YuGuiActionCallback callback = nullptr;
    void* context = nullptr;
    void* root = nullptr;

    @synchronized(self) {
        if (!self.renderScheduled || self.closed || self.fullScreenTransitioning ||
            self.frameUpdateOpen) {
            return;
        }
        if (self.presentationPending) {
            self.presentationHeldFrame = YES;
            return;
        }
        callback = self.renderCallback;
        context = self.renderContext;
        root = self.renderContextRoot;
        self.renderScheduled = NO;
        self.renderCallback = nullptr;
        self.renderContext = nullptr;
        self.renderContextRoot = nullptr;
    }

    guiFrameSchedulerCancel(self);
    if (callback) {
        uint64_t budgetNanoseconds = self.lastFrameRefreshIntervalNanoseconds
            ? self.lastFrameRefreshIntervalNanoseconds
            : kGuiResizeDefaultNanosecondBudget;
        YuGuiActionCallbackRecord* record = new YuGuiActionCallbackRecord(callback, context, 0);
        [self setPresentsWithTransaction:YES];
        self.couplesPresentation = YES;
        uint32_t result = yumiRunMainTaskFromNativeUntilDeadline(
            guiRunActionCallback, record, guiDeadlineAfter(budgetNanoseconds));
        self.couplesPresentation = NO;
        if (result == YuMainTaskDeadlineFailed) {
            delete record;
        }
    }
    if (root) {
        yumiRemoveRoot(root);
    }
}

- (void)dealloc {
    guiReleasePresenter(&canvasPresenter);
    if (inputStreamState) {
        inputStreamState->cancel();
    }
    self.textInputActive = NO;
    if (self.textActionContextRoot) {
        yumiRemoveRoot(self.textActionContextRoot);
        self.textActionContextRoot = nullptr;
    }
    [self closePopups];
    [self restoreStoplights];
    [self finishFrameUpdate];
    [self cancelRenderFrame];
    if (self.backingScaleContextRoot) {
        yumiRemoveRoot(self.backingScaleContextRoot);
        self.backingScaleContextRoot = nullptr;
    }
    if (self.moveContextRoot) {
        yumiRemoveRoot(self.moveContextRoot);
        self.moveContextRoot = nullptr;
    }
    if (self.contentSizeContextRoot) {
        yumiRemoveRoot(self.contentSizeContextRoot);
        self.contentSizeContextRoot = nullptr;
    }
    if (self.pointerContextRoot) {
        yumiRemoveRoot(self.pointerContextRoot);
        self.pointerContextRoot = nullptr;
    }
    if (self.eventContextRoot) {
        yumiRemoveRoot(self.eventContextRoot);
        self.eventContextRoot = nullptr;
    }
    [self.window setDelegate:nil];
    [self.window close];
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    if (self.closed || !self.eventCallback) {
        return YES;
    }
    if (!self.closeDecisionPending) {
        self.closeDecisionPending = YES;
        [self dispatchWindowEventKind:5 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
    }
    return NO;
}

- (void)windowWillClose:(NSNotification*)notification {
    // The managed close callback releases its retained host handle. Keep the delegate alive until
    // this callback unwinds so releasing that handle cannot deallocate `self` mid-method.
    NS_VALID_UNTIL_END_OF_SCOPE YuGuiWindowHost* delegate = self;
    self.closed = YES;
    self.textInputActive = NO;
    self.markedText = nil;
    [self closePopups];
    [self finishFrameUpdate];
    [self cancelRenderFrame];
    [self dispatchWindowEventKind:6 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
    inputStreamState->cancel();
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification {
    guiReceiveBackingScaleNotification(self);
}

- (void)windowDidMove:(NSNotification*)notification {
    guiReceiveMoveNotification(self);
}

- (void)windowDidResize:(NSNotification*)notification {
    [self updateStoplights];
    [self markShadowStale];
    guiReceiveMoveNotification(self);
    uint64_t budgetNanoseconds = self.lastFrameRefreshIntervalNanoseconds
        ? self.lastFrameRefreshIntervalNanoseconds
        : kGuiResizeDefaultNanosecondBudget;
    self.couplesPresentation = YES;
    guiReceiveContentSizeNotification(self, guiDeadlineAfter(budgetNanoseconds));
    self.couplesPresentation = NO;
}

- (void)restoreStoplights {
    [self.stoplightView restoreButtons];
    [self.stoplightView removeFromSuperview];
    self.stoplightView = nil;
}

- (CGFloat)stoplightLayoutZoom {
    return !self.titlebarVisible && self.stoplightZoom > 0 ? self.stoplightZoom : 1;
}

- (void)measureNativeStoplights {
    [[self.contentView superview] layoutSubtreeIfNeeded];
    const NSWindowButton kinds[] = {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton};
    CGFloat edge = 0;
    for (size_t i = 0; i < 3; ++i) {
        NSButton* button = [self.window standardWindowButton:kinds[i]];
        if (button) {
            edge = MAX(edge, NSMaxX([button convertRect:[button bounds] toView:nil]));
        }
    }
    self.stoplightEdge = edge;
}

- (void)updateStoplights {
    CGFloat zoom = [self stoplightLayoutZoom];
    BOOL fullScreen = (self.window.styleMask & NSWindowStyleMaskFullScreen) != 0;
    if ((!self.hasStoplightOffset && zoom == 1) || self.stoplightsSuspended || fullScreen) {
        [self restoreStoplights];
        if (!self.stoplightsSuspended && !fullScreen) {
            [self measureNativeStoplights];
        }
        return;
    }
    NSView* frameView = [self.contentView superview];
    if (!frameView) return;
    if (!self.stoplightView) {
        [frameView layoutSubtreeIfNeeded];
        NSView* host = (self.window.styleMask & NSWindowStyleMaskFullSizeContentView)
            ? self.contentView
            : [[self.window standardWindowButton:NSWindowCloseButton] superview];
        if (!host) return;
        YuGuiStoplightView* overlay = [[YuGuiStoplightView alloc] initWithFrame:[host bounds]];
        [overlay setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [host addSubview:overlay positioned:NSWindowAbove relativeTo:nil];
        [overlay adoptButtonsFromWindow:self.window];
        self.stoplightView = overlay;
    }
    [self.stoplightView reclaimButtons];
    self.stoplightEdge = [self.stoplightView nativeTrailingEdge];
    self.stoplightView.offset = self.hasStoplightOffset ? self.stoplightOffset : NSZeroPoint;
    self.stoplightView.zoom = zoom;
    [self.stoplightView setFrame:[[self.stoplightView superview] bounds]];
    [self.stoplightView setNeedsLayout:YES];
    [self.stoplightView layoutSubtreeIfNeeded];
}

- (void)finishFullScreenTransition {
    self.fullScreenTransitioning = NO;
    BOOL resume = NO;
    @synchronized(self) {
        resume = self.renderScheduled && !self.closed;
    }
    if (resume) {
        guiFrameSchedulerRequest(self);
    }
}

- (void)windowWillEnterFullScreen:(NSNotification*)notification {
    self.stoplightsSuspended = YES;
    [self restoreStoplights];
    self.fullScreenTransitioning = YES;
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
    [self dispatchWindowEventKind:10 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
    [self finishFullScreenTransition];
}

- (void)windowWillExitFullScreen:(NSNotification*)notification {
    self.fullScreenTransitioning = YES;
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
    self.stoplightsSuspended = NO;
    [self updateStoplights];
    [self dispatchWindowEventKind:11 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
    [self finishFullScreenTransition];
}

- (void)windowDidFailToEnterFullScreen:(NSWindow*)window {
    self.stoplightsSuspended = NO;
    [self updateStoplights];
    [self finishFullScreenTransition];
}

- (void)windowDidFailToExitFullScreen:(NSWindow*)window {
    [self finishFullScreenTransition];
}

- (void)windowDidResignKey:(NSNotification*)notification {
    self.canvasCapturedButtons = 0;
    inputStreamState->primaryDown = false;
    [self settleKeyResignation];
}

- (void)windowDidChangeOcclusionState:(NSNotification*)notification {
    if ([self.window occlusionState] & NSWindowOcclusionStateVisible) {
        [self dispatchWindowEventKind:9 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
    }
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    [self dispatchWindowEventKind:3 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
}

- (BOOL)ownsWindow:(NSWindow*)window {
    if (!window) {
        return NO;
    }
    if (window == self.window) {
        return YES;
    }
    for (YuGuiPopupHost* popup in self.popupHosts) {
        if (popup.panel == window) {
            return YES;
        }
    }
    return NO;
}

- (void)settleKeyResignation {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!self.closed && ![self ownsWindow:[NSApp keyWindow]]) {
            [self dispatchWindowEventKind:4 key:0 nativeKeyCode:0 modifiers:0 isRepeat:NO];
        }
    });
}

- (void)closePopups {
    NSArray* popups = [self.popupHosts copy];
    for (YuGuiPopupHost* popup in popups) {
        [popup hide];
    }
}

- (NSPoint)currentWindowPointerPoint {
    NSPoint screenPoint = [NSEvent mouseLocation];
    NSRect screenRect = NSMakeRect(screenPoint.x, screenPoint.y, 0, 0);
    return [self.window convertRectFromScreen:screenRect].origin;
}

- (NSPoint)windowPointForEvent:(NSEvent*)event {
    // Preserve the location attached to the event being dispatched. Looking the cursor up again
    // can observe a newer global position and either reject this event at the native-control seam
    // or deliver it to the wrong managed row.
    if (event && [event window] == self.window) {
        return [event locationInWindow];
    }
    if (event && [event window]) {
        NSPoint screenPoint = [[event window] convertPointToScreen:[event locationInWindow]];
        return [self.window convertPointFromScreen:screenPoint];
    }
    return [self currentWindowPointerPoint];
}

- (BOOL)isCanvasTargetAtWindowPoint:(NSPoint)windowPoint {
    return [self isCanvasAtWindowPoint:windowPoint controlsBlock:YES];
}

- (BOOL)isCanvasAreaAtWindowPoint:(NSPoint)windowPoint {
    return [self isCanvasAtWindowPoint:windowPoint controlsBlock:NO];
}

- (BOOL)isCanvasAtWindowPoint:(NSPoint)windowPoint controlsBlock:(BOOL)controlsBlock {
    for (YuGuiPopupHost* popup in [self.popupHosts reverseObjectEnumerator]) {
        if (popup.shown && !popup.passesPointer && [popup containsWindowPoint:windowPoint]) {
            return !controlsBlock || ![popup nativeControlBlocksWindowPoint:windowPoint];
        }
    }
    const NSWindowButton kinds[] = {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton};
    for (NSWindowButton kind : kinds) {
        NSButton* button = [self.window standardWindowButton:kind];
        if (button && ![button isHiddenOrHasHiddenAncestor] &&
            NSPointInRect([button convertPoint:windowPoint fromView:nil], button.bounds)) {
            return NO;
        }
    }
    NSPoint canvasPoint = [self.canvasView convertPoint:windowPoint fromView:nil];
    if (!NSPointInRect(canvasPoint, [self.canvasView bounds])) {
        return NO;
    }
    if (!controlsBlock) {
        return YES;
    }
    for (NSView* subview in [self.contentView subviews]) {
        if (subview != self.canvasView && subview != self.metalView &&
            guiViewBlocksCanvas(subview, windowPoint)) {
            return NO;
        }
    }
    return YES;
}

- (void)dispatchWindowEventKind:(uint32_t)kind
                            key:(uint32_t)key
                  nativeKeyCode:(uint32_t)nativeKeyCode
                      modifiers:(uint64_t)modifiers
                       isRepeat:(BOOL)isRepeat {
    if (!self.eventCallback) {
        return;
    }
    guiScheduleWindowEventCallback(
        inputStreamState,
        reinterpret_cast<uintptr_t>(self),
        self.eventCallback,
        self.eventContext,
        kind,
        key,
        nativeKeyCode,
        modifiers,
        static_cast<bool>(isRepeat),
        self.eventText
    );
}

- (BOOL)handleKeyEvent:(NSEvent*)event {
    if (!self.eventCallback || self.closed) {
        return NO;
    }
    NSResponder* responder = [([event window] ?: self.window) firstResponder];
    if (guiResponderPeerOwnsKeys(responder)) {
        return NO;
    }

    unsigned short nativeKeyCode = [event keyCode];
    uint32_t key = guiLogicalKeyKind(nativeKeyCode);
    NSEventModifierFlags nativeModifiers = [event modifierFlags];
    uint64_t modifiers = guiModifierBits(nativeModifiers);
    YuGuiCanvasView* canvas = responder == self.canvasView ? self.canvasView : nil;
    for (YuGuiPopupHost* popup in self.popupHosts) {
        if (responder == popup.canvasView) {
            canvas = popup.canvasView;
        }
    }
    BOOL canvasFocused = canvas != nil;
    NSString* charactersIgnoringModifiers = [event charactersIgnoringModifiers] ?: @"";
    BOOL commandDown = (nativeModifiers & NSEventModifierFlagCommand) != 0;
    BOOL interprets = canvasFocused && self.textInputActive;
    std::vector<YuGuiTextAction> actions;
    BOOL composing = NO;
    if (interprets) {
        composing = [self hasMarkedText];
        collectingTextActions = &actions;
        [canvas interpretKeyEvents:@[event]];
        collectingTextActions = nullptr;
        composing = composing || [self hasMarkedText];
        for (const YuGuiTextAction& action : actions) {
            composing = composing || action.kind == 2;
        }
        if (commandDown && !composing) {
            actions.erase(
                std::remove_if(actions.begin(), actions.end(), [](const YuGuiTextAction& action) {
                    return action.kind == 0;
                }),
                actions.end()
            );
        }
    }
    if (!composing) {
        self.eventText = charactersIgnoringModifiers;
        [self dispatchWindowEventKind:0
                                  key:key
                        nativeKeyCode:nativeKeyCode
                            modifiers:modifiers
                             isRepeat:[event isARepeat]];
    }
    if (interprets) {
        [self dispatchTextActions:actions keyed:!composing];
        return YES;
    }
    if (key == 1) {
        return YES;
    }

    if (!canvasFocused) {
        return NO;
    }

    BOOL hasCommandModifier = (nativeModifiers & NSEventModifierFlagCommand) ||
        (nativeModifiers & NSEventModifierFlagControl) ||
        (nativeModifiers & NSEventModifierFlagOption);
    if (key || hasCommandModifier) {
        return YES;
    }

    NSString* characters = [event characters] ?: @"";
    if ([characters length]) {
        std::vector<YuGuiTextAction> typed;
        typed.push_back({0, std::string([characters UTF8String]), 0, 0});
        [self dispatchTextActions:typed keyed:YES];
    }

    return YES;
}

- (BOOL)hasMarkedText {
    return self.markedText.length > 0;
}

- (void)emitTextActionKind:(uint32_t)kind
                      text:(NSString*)text
                     start:(uint32_t)start
                    length:(uint32_t)length {
    YuGuiTextAction action{kind, std::string([text ?: @"" UTF8String]), start, length};
    if (collectingTextActions) {
        collectingTextActions->push_back(std::move(action));
        return;
    }
    std::vector<YuGuiTextAction> actions;
    actions.push_back(std::move(action));
    [self dispatchTextActions:actions keyed:NO];
}

- (void)dispatchTextActions:(std::vector<YuGuiTextAction>&)actions keyed:(BOOL)keyed {
    if (!self.textActionCallback || self.closed) {
        return;
    }
    guiScheduleTextActionCallback(
        inputStreamState,
        reinterpret_cast<uintptr_t>(self),
        self.textActionCallback,
        self.textActionContext,
        std::move(actions),
        static_cast<bool>(keyed)
    );
}

- (void)textInputInsert:(NSString*)text replacementRange:(NSRange)replacementRange {
    NSString* before = self.textInputBefore ?: @"";
    uint32_t replaced = 0;
    if (!self.markedText && replacementRange.location != NSNotFound &&
        replacementRange.location < before.length) {
        NSRange start = [before rangeOfComposedCharacterSequenceAtIndex:replacementRange.location];
        replaced = guiCodePointCount([before substringFromIndex:start.location]);
        before = [before substringToIndex:start.location];
    }
    self.markedText = nil;
    self.textInputBefore = [before stringByAppendingString:text];
    self.textInputSelected = @"";
    [self emitTextActionKind:0 text:text start:replaced length:0];
}

- (void)textInputCommand:(SEL)selector {
    if (selector == @selector(noop:)) {
        return;
    }
    [self emitTextActionKind:1 text:NSStringFromSelector(selector) start:0 length:0];
}

- (void)textInputSetMarkedText:(NSString*)text selectedRange:(NSRange)selectedRange {
    if (!text.length) {
        BOOL wasComposing = [self hasMarkedText];
        self.markedText = nil;
        if (wasComposing) {
            [self emitTextActionKind:2 text:@"" start:0 length:0];
        }
        return;
    }
    NSUInteger location = selectedRange.location == NSNotFound
        ? text.length
        : MIN(selectedRange.location, text.length);
    NSUInteger length = MIN(selectedRange.length, text.length - location);
    self.markedText = text;
    self.markedSelection = NSMakeRange(location, length);
    [self emitTextActionKind:2
                        text:text
                       start:guiCodePointCount([text substringToIndex:location])
                      length:guiCodePointCount([text substringWithRange:NSMakeRange(location, length)])];
}

- (void)textInputUnmark {
    NSString* text = self.markedText;
    if (!text.length) {
        return;
    }
    self.markedText = nil;
    self.textInputBefore = [(self.textInputBefore ?: @"") stringByAppendingString:text];
    self.textInputSelected = @"";
    [self emitTextActionKind:0 text:text start:0 length:0];
}

- (NSRange)textInputSelectedRange {
    NSUInteger base = self.textInputBefore.length;
    if (self.markedText) {
        return NSMakeRange(base + self.markedSelection.location, self.markedSelection.length);
    }
    return NSMakeRange(base, self.textInputSelected.length);
}

- (NSRange)textInputMarkedRange {
    return self.markedText
        ? NSMakeRange(self.textInputBefore.length, self.markedText.length)
        : NSMakeRange(NSNotFound, 0);
}

- (NSAttributedString*)textInputSubstring:(NSRange)range actualRange:(NSRangePointer)actualRange {
    NSString* document = [(self.textInputBefore ?: @"")
        stringByAppendingString:(self.markedText ?: self.textInputSelected ?: @"")];
    if (range.location == NSNotFound || range.location > document.length) {
        return nil;
    }
    NSRange clamped = NSMakeRange(range.location, MIN(range.length, document.length - range.location));
    if (actualRange) {
        *actualRange = clamped;
    }
    return [[NSAttributedString alloc] initWithString:[document substringWithRange:clamped]];
}

- (NSRect)textInputScreenRect {
    if (!self.contentView || !self.window) {
        return NSZeroRect;
    }
    NSRect windowRect = [self.contentView convertRect:self.textInputCaret toView:nil];
    return [self.window convertRectToScreen:windowRect];
}

- (NSTextInputContext*)activeTextInputContext {
    NSTextInputContext* context = [NSTextInputContext currentInputContext];
    id client = context.client;
    if (client && client == self.canvasView) {
        return context;
    }
    for (YuGuiPopupHost* popup in self.popupHosts) {
        if (client && client == popup.canvasView) {
            return context;
        }
    }
    return nil;
}

- (void)finishTextComposition {
    if (![self hasMarkedText]) {
        return;
    }
    [self textInputUnmark];
    [[self activeTextInputContext] discardMarkedText];
}

- (void)discardTextComposition {
    if (!self.markedText) {
        return;
    }
    self.markedText = nil;
    [[self activeTextInputContext] discardMarkedText];
}

- (void)handleKeyReleaseEvent:(NSEvent*)event {
    if (!self.eventCallback || self.closed ||
        guiResponderPeerOwnsKeys([([event window] ?: self.window) firstResponder])) {
        return;
    }

    unsigned short nativeKeyCode = [event keyCode];
    self.eventText = [event charactersIgnoringModifiers] ?: @"";
    [self dispatchWindowEventKind:8
                              key:guiLogicalKeyKind(nativeKeyCode)
                    nativeKeyCode:nativeKeyCode
                        modifiers:guiModifierBits([event modifierFlags])
                         isRepeat:NO];
}

- (void)dispatchPeerKeyEvent:(NSEvent*)event {
    if (!self.eventCallback || self.closed || [event type] != NSEventTypeKeyDown) {
        return;
    }

    unsigned short nativeKeyCode = [event keyCode];
    self.eventText = [event charactersIgnoringModifiers] ?: @"";
    [self dispatchWindowEventKind:0
                              key:guiLogicalKeyKind(nativeKeyCode)
                    nativeKeyCode:nativeKeyCode
                        modifiers:guiModifierBits([event modifierFlags])
                         isRepeat:[event isARepeat]];
}

- (void)dispatchPeerFocusExit:(BOOL)backward {
    if (!self.eventCallback || self.closed) {
        return;
    }

    self.eventText = @"\t";
    [self dispatchWindowEventKind:0
                              key:guiLogicalKeyKind(48)
                    nativeKeyCode:48
                        modifiers:backward ? guiModifierBits(NSEventModifierFlagShift) : 0
                         isRepeat:NO];
}

- (void)handleWindowEvent:(NSEvent*)event {
    NSEventType type = [event type];
    if (!guiIsPointerEventType(type)) {
        return;
    }

    if (type == NSEventTypeLeftMouseDown ||
        type == NSEventTypeRightMouseDown ||
        type == NSEventTypeOtherMouseDown) {
        [self finishTextComposition];
    }
    if (type == NSEventTypeLeftMouseDown) {
        ++inputStreamState->primaryPress;
        inputStreamState->primaryDown = true;
        self.pressEvent = event;
    } else if (type == NSEventTypeLeftMouseUp) {
        inputStreamState->primaryDown = false;
    }

    NSPoint windowPoint = [self windowPointForEvent:event];
    BOOL isOverCanvas = [self isCanvasTargetAtWindowPoint:windowPoint];
    NSInteger buttonNumber = guiIsPointerButtonEventType(type) ? [event buttonNumber] : -1;
    uint64_t buttonMask = buttonNumber >= 0 && buttonNumber < 64
        ? uint64_t{1} << buttonNumber
        : uint64_t{0};

    if (type == NSEventTypeMouseMoved || type == NSEventTypeMouseEntered) {
        if ([self isCanvasAreaAtWindowPoint:windowPoint]) {
            self.pointerOverCanvas = YES;
            [self dispatchPointerEvent:event kind:0];
        } else if (self.pointerOverCanvas) {
            self.pointerOverCanvas = NO;
            [self dispatchPointerEvent:nil kind:5];
        }
        return;
    }

    if (type == NSEventTypeMouseExited) {
        if (![self isCanvasAreaAtWindowPoint:windowPoint] && self.pointerOverCanvas) {
            self.pointerOverCanvas = NO;
            [self dispatchPointerEvent:nil kind:5];
        }
        return;
    }

    if (type == NSEventTypeLeftMouseDown ||
        type == NSEventTypeRightMouseDown ||
        type == NSEventTypeOtherMouseDown) {
        if (isOverCanvas) {
            self.pointerOverCanvas = YES;
            self.canvasCapturedButtons |= buttonMask;
            [self dispatchPointerEvent:event kind:1];
        } else {
            [self dispatchPointerEvent:event kind:13];
        }
        return;
    }

    if (type == NSEventTypeLeftMouseDragged ||
        type == NSEventTypeRightMouseDragged ||
        type == NSEventTypeOtherMouseDragged) {
        if (self.canvasCapturedButtons & buttonMask) {
            [self dispatchPointerEvent:event kind:3];
        }
        return;
    }

    if (type == NSEventTypeLeftMouseUp ||
        type == NSEventTypeRightMouseUp ||
        type == NSEventTypeOtherMouseUp) {
        if (self.canvasCapturedButtons & buttonMask) {
            [self dispatchPointerEvent:event kind:2];
            self.canvasCapturedButtons &= ~buttonMask;
        }
        return;
    }

}

- (void)dispatchPointerEvent:(NSEvent*)event kind:(uint32_t)kind {
    if (!self.pointerCallback || self.closed) {
        return;
    }

    NSPoint point = NSZeroPoint;
    double deltaX = 0;
    double deltaY = 0;
    uint32_t deviceKind = 0;
    uint32_t button = 0;
    uint64_t buttons = static_cast<uint64_t>([NSEvent pressedMouseButtons]);
    uint64_t modifiers = 0;
    double orderingTime = 0;
    uint32_t clickCount = 0;
    bool isPrecise = false;
    uint64_t phase = 0;
    uint64_t momentumPhase = 0;

    if (event) {
        NSEventType type = [event type];
        point = [self.canvasView convertPoint:[self windowPointForEvent:event] fromView:nil];
        orderingTime = [event timestamp];

        if (guiIsPointerButtonEventType(type)) {
            button = static_cast<uint32_t>([event buttonNumber]);
            clickCount = static_cast<uint32_t>([event clickCount]);
        }
        if (guiIsPointerMotionEventType(type)) {
            deltaX = [event deltaX];
            deltaY = [event deltaY];
        }
        if (type == NSEventTypeScrollWheel) {
            deltaX = -[event scrollingDeltaX];
            deltaY = -[event scrollingDeltaY];
            isPrecise = [event hasPreciseScrollingDeltas];
            phase = static_cast<uint64_t>([event phase]);
            momentumPhase = static_cast<uint64_t>([event momentumPhase]);
        }
        if (type == NSEventTypeMagnify) {
            deltaX = [event magnification];
            isPrecise = true;
            phase = static_cast<uint64_t>([event phase]);
        }

        NSEventModifierFlags nativeModifiers = [event modifierFlags];
        if (nativeModifiers & NSEventModifierFlagShift) modifiers |= 1;
        if (nativeModifiers & NSEventModifierFlagControl) modifiers |= 2;
        if (nativeModifiers & NSEventModifierFlagOption) modifiers |= 4;
        if (nativeModifiers & NSEventModifierFlagCommand) modifiers |= 8;
        if (nativeModifiers & NSEventModifierFlagCapsLock) modifiers |= 16;
        if (nativeModifiers & NSEventModifierFlagFunction) modifiers |= 32;
    }

    guiSchedulePointerCallback(
        inputStreamState,
        reinterpret_cast<uintptr_t>(self),
        self.pointerCallback,
        self.pointerContext,
        kind,
        uint64_t{0},
        deviceKind,
        button,
        buttons,
        modifiers,
        static_cast<double>(point.x),
        static_cast<double>(point.y),
        deltaX,
        deltaY,
        orderingTime,
        clickCount,
        isPrecise,
        phase,
        momentumPhase,
        kind == 1 && [event type] == NSEventTypeLeftMouseDown ? event : nil
    );
}

- (NSDragOperation)dispatchDrop:(id<NSDraggingInfo>)info kind:(uint32_t)kind windowPoint:(NSPoint)windowPoint {
    if (!self.pointerCallback || self.closed) {
        return NSDragOperationNone;
    }
    if (kind == 9) {
        self.dropOperation = 0;
    }
    self.dropPasteboard = [info draggingPasteboard];
    NSPoint point = [self.canvasView convertPoint:windowPoint fromView:nil];
    guiSchedulePointerCallback(
        inputStreamState,
        reinterpret_cast<uintptr_t>(self),
        self.pointerCallback,
        self.pointerContext,
        kind,
        static_cast<uint64_t>([info draggingSequenceNumber]),
        uint32_t{0},
        uint32_t{0},
        static_cast<uint64_t>([NSEvent pressedMouseButtons]),
        guiModifierBits([NSEvent modifierFlags]),
        static_cast<double>(point.x),
        static_cast<double>(point.y),
        0,
        0,
        [[NSProcessInfo processInfo] systemUptime],
        uint32_t{0},
        false,
        uint64_t{0},
        uint64_t{0}
    );
    return guiDropOperation(self.dropOperation, [info draggingSourceOperationMask]);
}

- (BOOL)performDrop:(id<NSDraggingInfo>)info windowPoint:(NSPoint)windowPoint {
    BOOL accepted = guiDropOperation(self.dropOperation, [info draggingSourceOperationMask]) != NSDragOperationNone;
    if (accepted) {
        guiReceivePromisedFiles(info);
    }
    [self dispatchDrop:info kind:accepted ? 12 : 11 windowPoint:windowPoint];
    self.dropOperation = 0;
    return accepted;
}

- (void)requestRenderFrameWithCallback:(YuGuiActionCallback)callback
                               context:(void*)context
                             notBefore:(uint64_t)notBeforeNanoseconds {
    if (!callback) {
        return;
    }

    void* newRoot = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = nullptr;
    void* unusedRoot = nullptr;
    BOOL shouldRequest = NO;

    @synchronized(self) {
        oldRoot = self.renderContextRoot;
        if (self.closed) {
            self.renderScheduled = NO;
            self.renderCallback = nullptr;
            self.renderContext = nullptr;
            self.renderContextRoot = nullptr;
            unusedRoot = newRoot;
            newRoot = nullptr;
        } else {
            self.renderCallback = callback;
            self.renderContext = context;
            self.renderContextRoot = newRoot;
            self.renderScheduled = YES;
            self.renderNotBeforeNanoseconds = notBeforeNanoseconds;
            shouldRequest = YES;
        }
    }

    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
    if (unusedRoot) {
        yumiRemoveRoot(unusedRoot);
    }
    if (shouldRequest) {
        guiFrameSchedulerRequest(self);
    }
}

- (void)cancelRenderFrame {
    void* oldRoot = nullptr;

    @synchronized(self) {
        oldRoot = self.renderContextRoot;
        self.renderScheduled = NO;
        self.renderCallback = nullptr;
        self.renderContext = nullptr;
        self.renderContextRoot = nullptr;
    }

    guiFrameSchedulerCancel(self);
    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
}

- (void)fireRenderFrameWithTiming:(YuGuiFrameTiming)timing {
    YuGuiActionCallback scheduledCallback = nullptr;
    void* scheduledContext = nullptr;
    void* scheduledRoot = nullptr;
    BOOL shouldFire = NO;

    @synchronized(self) {
        self.lastFrameTargetTimeNanoseconds = timing.targetTimeNanoseconds;
        self.lastFrameRefreshIntervalNanoseconds = timing.refreshIntervalNanoseconds;
        self.lastFrameEventTimeSeconds = timing.eventTimeSeconds;

        if (self.fullScreenTransitioning && self.renderScheduled && !self.closed) {
            return;
        }
        if (self.presentationPending && self.renderScheduled && !self.closed) {
            self.presentationHeldFrame = YES;
            return;
        }
        if (!self.renderScheduled || self.closed) {
            scheduledRoot = self.renderContextRoot;
            self.renderContextRoot = nullptr;
        } else {
            scheduledCallback = self.renderCallback;
            scheduledContext = self.renderContext;
            scheduledRoot = self.renderContextRoot;
            shouldFire = YES;
        }

        self.renderScheduled = NO;
        self.renderCallback = nullptr;
        self.renderContext = nullptr;
        self.renderContextRoot = nullptr;
    }

    if (!shouldFire) {
        if (scheduledRoot) {
            yumiRemoveRoot(scheduledRoot);
        }
        return;
    }

    if (scheduledCallback) {
        uint64_t idleDeadlineNanoseconds = timing.targetTimeNanoseconds;
        if (timing.refreshIntervalNanoseconds) {
            idleDeadlineNanoseconds = timing.outputTimeNanoseconds >
                    UINT64_MAX - timing.refreshIntervalNanoseconds
                ? UINT64_MAX
                : timing.outputTimeNanoseconds + timing.refreshIntervalNanoseconds;
        }
        guiScheduleActionCallback(
            scheduledCallback, scheduledContext, idleDeadlineNanoseconds, true);
    }
    if (scheduledRoot) {
        yumiRemoveRoot(scheduledRoot);
    }
}

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
    return NSDragOperationCopy;
}

@end

@implementation YuGuiTextInputHost

- (void)dealloc {
    if (self.guiWindow && self.guiWindow->inputStreamState) {
        self.guiWindow->inputStreamState->seal();
    }
    [self.control setDelegate:nil];
    [self.control setTarget:nil];
    if (self.contextRoot) {
        yumiRemoveRoot(self.contextRoot);
        self.contextRoot = nullptr;
    }
    [self.control removeFromSuperview];
}

- (void)sendEventKind:(uint32_t)eventKind {
    if (!self.callback) {
        return;
    }
    NSText* editor = [self.control currentEditor];
    NSString* text = editor && (eventKind == 5 || eventKind == 6) ? [editor string] : [self.control stringValue];
    NSRange selection = editor
        ? [editor selectedRange]
        : NSMakeRange([text length], 0);
    uint64_t modifiers = eventKind == 2 || eventKind == 6
        ? guiModifierBits([[NSApp currentEvent] modifierFlags])
        : 0;
    guiMarkNativeChange(self.guiWindow);
    guiScheduleTextInputCallback(
        self.guiWindow->inputStreamState,
        reinterpret_cast<uintptr_t>(self),
        self.callback, self.context, eventKind, text, selection, modifiers);
}

- (void)controlTextDidBeginEditing:(NSNotification*)notification {
    [self sendEventKind:3];
}

- (void)controlTextDidChange:(NSNotification*)notification {
    [self sendEventKind:0];
}

- (void)controlTextDidEndEditing:(NSNotification*)notification {
    id editor = [[notification userInfo] objectForKey:@"NSFieldEditor"];
    if ([editor isKindOfClass:[NSTextView class]]) {
        NSTextView* view = static_cast<NSTextView*>(editor);
        if (self.lineHeight > 0 || self.lineSpacing > 0) {
            [view setDefaultParagraphStyle:nil];
        }
    }
    [self sendEventKind:1];
}

- (NSParagraphStyle*)lineStyle {
    if (self.lineHeight <= 0 && self.lineSpacing <= 0) {
        return nil;
    }
    NSMutableParagraphStyle* style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    [style setLineSpacing:self.lineSpacing];
    if (self.lineHeight > 0) {
        [style setMinimumLineHeight:self.lineHeight];
        [style setMaximumLineHeight:self.lineHeight];
    }
    [style setAlignment:[self.control alignment]];
    [style setLineBreakMode:[self.control lineBreakMode]];
    return style;
}

- (void)applyCellLineMetrics {
    NSTextField* control = self.control;
    NSParagraphStyle* style = [self lineStyle];
    if (!control || !style || [control currentEditor]) {
        return;
    }
    NSFont* font = [control font] ?: [NSFont systemFontOfSize:[NSFont systemFontSize]];
    NSColor* color = [control textColor] ?: [NSColor textColor];
    NSAttributedString* value = [[NSAttributedString alloc]
        initWithString:[control stringValue] ?: @""
            attributes:@{
                NSFontAttributeName: font,
                NSForegroundColorAttributeName: color,
                NSParagraphStyleAttributeName: style
            }];
    [control setAttributedStringValue:value];
    NSAttributedString* placeholder = [[NSAttributedString alloc]
        initWithString:self.placeholderText ?: @""
            attributes:@{
                NSFontAttributeName: font,
                NSForegroundColorAttributeName: [NSColor placeholderTextColor],
                NSParagraphStyleAttributeName: style
            }];
    [control setPlaceholderAttributedString:placeholder];
}

- (void)applyLineMetrics {
    NSParagraphStyle* style = [self lineStyle];
    if (!style) {
        return;
    }
    [self applyCellLineMetrics];
    NSText* editor = [self.control currentEditor];
    if (![editor isKindOfClass:[NSTextView class]]) {
        return;
    }
    NSTextView* view = static_cast<NSTextView*>(editor);
    [view setDefaultParagraphStyle:style];
    NSMutableDictionary* typing = [[view typingAttributes] mutableCopy];
    [typing setObject:style forKey:NSParagraphStyleAttributeName];
    [view setTypingAttributes:typing];
    NSTextStorage* storage = [view textStorage];
    [storage addAttribute:NSParagraphStyleAttributeName value:style range:NSMakeRange(0, [storage length])];
}

- (void)applyEditorLayout {
    [self applyLineMetrics];
    [self applyTextArea];
}

- (void)applyTextArea {
    NSTextField* control = self.control;
    NSText* editor = [control currentEditor];
    NSView* holder = [editor superview];
    if (![[control cell] wraps] || ![holder isKindOfClass:[NSClipView class]] || [holder superview] != control) {
        return;
    }
    NSClipView* clip = static_cast<NSClipView*>(holder);
    NSRect bounds = [control bounds];
    NSRect title = [[control cell] titleRectForBounds:bounds];
    BOOL flipped = [control isFlipped];
    CGFloat top = flipped ? NSMinY(title) - NSMinY(bounds) : NSMaxY(bounds) - NSMaxY(title);
    NSRect frame = [clip frame];
    frame.origin.y = flipped ? NSMinY(bounds) + top : NSMinY(bounds);
    frame.size.height = MAX(0.0, NSHeight(bounds) - top);
    NSEdgeInsets insets = [clip contentInsets];
    NSEdgeInsets wanted = NSEdgeInsetsMake(self.textTopInset, insets.left, self.textBottomInset, insets.right);
    if (NSEqualRects([clip frame], frame) && NSEdgeInsetsEqual(insets, wanted)) {
        return;
    }
    NSRect visible = [clip bounds];
    if (insets.top != wanted.top) {
        visible.origin.y = -wanted.top;
    }
    [clip setContentInsets:wanted];
    [clip setFrame:frame];
    visible.size = [clip bounds].size;
    [clip setBoundsOrigin:[clip constrainBoundsRect:visible].origin];
    [editor scrollRangeToVisible:[editor selectedRange]];
}

- (BOOL)control:(NSControl*)control textView:(NSTextView*)textView doCommandBySelector:(SEL)commandSelector {
    NSEvent* event = [NSApp currentEvent];
    if (!(self.passedKeys & 4) && [event type] == NSEventTypeKeyDown &&
        ([event modifierFlags] & NSEventModifierFlagCommand) &&
        guiLogicalKeyKind([event keyCode]) == 2) {
        [self sendEventKind:6];
        return YES;
    }
    if (!(self.passedKeys & 4) && [[control cell] wraps] &&
        (commandSelector == @selector(insertNewlineIgnoringFieldEditor:) ||
         (commandSelector == @selector(insertNewline:) &&
          ([event modifierFlags] & (NSEventModifierFlagShift | NSEventModifierFlagOption))))) {
        [textView insertNewlineIgnoringFieldEditor:self];
        return YES;
    }
    if (commandSelector == @selector(moveUp:) && (self.passedKeys & 1)) {
        return YES;
    }
    if (commandSelector == @selector(moveDown:) && (self.passedKeys & 2)) {
        return YES;
    }
    if ((commandSelector == @selector(insertNewline:) ||
         commandSelector == @selector(insertNewlineIgnoringFieldEditor:)) &&
        (self.passedKeys & 4)) {
        return YES;
    }
    if (commandSelector == @selector(moveLeft:) && (self.passedKeys & 8)) {
        return YES;
    }
    if (commandSelector == @selector(moveRight:) && (self.passedKeys & 16)) {
        return YES;
    }
    if ((commandSelector == @selector(scrollPageUp:) || commandSelector == @selector(pageUp:)) &&
        (self.passedKeys & 32)) {
        return YES;
    }
    if ((commandSelector == @selector(scrollPageDown:) || commandSelector == @selector(pageDown:)) &&
        (self.passedKeys & 64)) {
        return YES;
    }
    if ((commandSelector == @selector(scrollToBeginningOfDocument:) ||
         commandSelector == @selector(moveToBeginningOfDocument:) ||
         commandSelector == @selector(moveToBeginningOfLine:)) &&
        (self.passedKeys & 128)) {
        return YES;
    }
    if ((commandSelector == @selector(scrollToEndOfDocument:) ||
         commandSelector == @selector(moveToEndOfDocument:) ||
         commandSelector == @selector(moveToEndOfLine:)) &&
        (self.passedKeys & 256)) {
        return YES;
    }
    return commandSelector == @selector(cancelOperation:);
}

- (void)textSubmitted:(id)sender {
    [self sendEventKind:2];
}

- (void)applyPendingSelection {
    if (!self.hasPendingSelection) {
        return;
    }
    NSText* editor = [self.control currentEditor];
    if (!editor) {
        return;
    }
    NSUInteger length = [[editor string] length];
    NSRange range = self.pendingSelection;
    NSUInteger start = MIN(range.location, length);
    NSUInteger end = MIN(range.location + range.length, length);
    [editor setSelectedRange:NSMakeRange(start, end - start)];
    self.hasPendingSelection = NO;
}

@end

static void
guiTextControlBecameFirstResponder(YuGuiTextControlField* control, BOOL accepted) {
    if (accepted) {
        [control.inputHost applyPendingSelection];
    }
}

static bool
guiTextControlAddCursorRect(YuGuiTextControlField* control) {
    NSCursor* cursor = guiCursorForStyle(control.cursorStyle, true, control.customCursor);
    if (cursor) {
        [control addCursorRect:guiViewHitBounds(control) cursor:cursor];
    }
    return cursor != nil;
}

static bool
guiTextControlScrollEditor(YuGuiTextControlField* control, NSEvent* event) {
    NSText* editor = [control currentEditor];
    NSView* holder = [editor superview];
    if (![holder isKindOfClass:[NSClipView class]]) {
        return false;
    }
    NSClipView* clip = static_cast<NSClipView*>(holder);
    NSRect visible = [clip documentVisibleRect];
    CGFloat limit = MAX(0.0, NSHeight([editor frame]) - NSHeight(visible));
    if (limit <= 0) {
        return false;
    }
    CGFloat delta = [event hasPreciseScrollingDeltas] ? [event scrollingDeltaY] : [event scrollingDeltaY] * 10.0;
    CGFloat y = MIN(limit, MAX(0.0, visible.origin.y - delta));
    [clip scrollToPoint:NSMakePoint(visible.origin.x, y)];
    return true;
}

static BOOL
guiTextControlShouldChange(
    YuGuiTextControlField* control,
    Class base,
    SEL selector,
    NSTextView* textView,
    NSRange range,
    NSString* string
) {
    NSUInteger limit = control.inputHost.maxLength;
    if (limit > 0 && string) {
        NSUInteger kept = [[textView string] length] - range.length;
        if (kept + [string length] > limit) {
            NSUInteger room = limit > kept ? limit - kept : 0;
            NSUInteger cut = room < [string length] ? [string rangeOfComposedCharacterSequenceAtIndex:room].location : room;
            if (cut > 0) {
                [textView insertText:[string substringToIndex:cut] replacementRange:range];
            } else {
                NSBeep();
            }
            return NO;
        }
    }
    if (![base instancesRespondToSelector:selector]) {
        return YES;
    }
    IMP inherited = [base instanceMethodForSelector:selector];
    return ((BOOL (*)(id, SEL, NSTextView*, NSRange, NSString*))inherited)(control, selector, textView, range, string);
}

static NSDictionary*
guiTextControlTypingAttributes(
    YuGuiTextControlField* control,
    Class base,
    SEL selector,
    NSTextView* textView,
    NSDictionary* current,
    NSDictionary* proposed
) {
    NSDictionary* attributes = proposed;
    if ([base instancesRespondToSelector:selector]) {
        IMP inherited = [base instanceMethodForSelector:selector];
        attributes = ((NSDictionary* (*)(id, SEL, NSTextView*, NSDictionary*, NSDictionary*))inherited)(
            control, selector, textView, current, proposed
        );
    }
    NSParagraphStyle* style = [control.inputHost lineStyle];
    if (!style || [attributes[NSParagraphStyleAttributeName] isEqual:style]) {
        return attributes;
    }
    NSMutableDictionary* styled = [attributes mutableCopy];
    styled[NSParagraphStyleAttributeName] = style;
    return styled;
}

@interface YuGuiTextFieldCell : NSTextFieldCell
@end

@implementation YuGuiTextFieldCell

- (void)drawInteriorWithFrame:(NSRect)frame inView:(NSView*)view {
    CGFloat inset = guiIsTextControl(view) ? static_cast<id<YuGuiTextControl>>(view).inputHost.textTopInset : 0;
    if ([view isFlipped]) {
        frame.origin.y += inset;
    }
    frame.size.height = MAX(0.0, NSHeight(frame) - inset);
    [super drawInteriorWithFrame:frame inView:view];
}

- (void)selectWithFrame:(NSRect)frame
                 inView:(NSView*)view
                 editor:(NSText*)editor
               delegate:(id)delegate
                  start:(NSInteger)start
                 length:(NSInteger)length {
    [super selectWithFrame:frame inView:view editor:editor delegate:delegate start:start length:length];
    if (guiIsTextControl(view)) {
        [static_cast<id<YuGuiTextControl>>(view).inputHost applyEditorLayout];
    }
}

@end

@implementation YuGuiTextField

+ (Class)cellClass {
    return [YuGuiTextFieldCell class];
}

- (NSView*)hitTest:(NSPoint)point {
    return guiTextControlContains(self, point) ? [super hitTest:point] : nil;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
    return YES;
}

- (BOOL)becomeFirstResponder {
    BOOL accepted = [super becomeFirstResponder];
    guiTextControlBecameFirstResponder(self, accepted);
    return accepted;
}

- (void)resetCursorRects {
    if (!guiTextControlAddCursorRect(self)) {
        [super resetCursorRects];
    }
}

- (void)mouseDown:(NSEvent*)event {
    [self.inputHost sendEventKind:4];
    [super mouseDown:event];
}

- (void)textDidEndEditing:(NSNotification*)notification {
    [super textDidEndEditing:notification];
    [self.inputHost applyCellLineMetrics];
}

- (void)scrollWheel:(NSEvent*)event {
    if (!guiTextControlScrollEditor(self, event)) {
        [super scrollWheel:event];
    }
}

- (void)textViewDidChangeSelection:(NSNotification*)notification {
    [self.inputHost sendEventKind:5];
}

- (BOOL)textView:(NSTextView*)textView
    shouldChangeTextInRange:(NSRange)range
          replacementString:(NSString*)string {
    return guiTextControlShouldChange(self, [NSTextField class], _cmd, textView, range, string);
}

- (NSDictionary*)textView:(NSTextView*)textView
    shouldChangeTypingAttributes:(NSDictionary*)current
                    toAttributes:(NSDictionary*)proposed {
    return guiTextControlTypingAttributes(self, [NSTextField class], _cmd, textView, current, proposed);
}

@end

@implementation YuGuiSecureTextField

- (NSView*)hitTest:(NSPoint)point {
    return guiTextControlContains(self, point) ? [super hitTest:point] : nil;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
    return YES;
}

- (BOOL)becomeFirstResponder {
    BOOL accepted = [super becomeFirstResponder];
    guiTextControlBecameFirstResponder(self, accepted);
    return accepted;
}

- (void)resetCursorRects {
    if (!guiTextControlAddCursorRect(self)) {
        [super resetCursorRects];
    }
}

- (void)mouseDown:(NSEvent*)event {
    [self.inputHost sendEventKind:4];
    [super mouseDown:event];
}

- (void)textDidEndEditing:(NSNotification*)notification {
    [super textDidEndEditing:notification];
    [self.inputHost applyCellLineMetrics];
}

- (void)scrollWheel:(NSEvent*)event {
    if (!guiTextControlScrollEditor(self, event)) {
        [super scrollWheel:event];
    }
}

- (void)textViewDidChangeSelection:(NSNotification*)notification {
    [self.inputHost sendEventKind:5];
}

- (BOOL)textView:(NSTextView*)textView
    shouldChangeTextInRange:(NSRange)range
          replacementString:(NSString*)string {
    return guiTextControlShouldChange(self, [NSSecureTextField class], _cmd, textView, range, string);
}

- (NSDictionary*)textView:(NSTextView*)textView
    shouldChangeTypingAttributes:(NSDictionary*)current
                    toAttributes:(NSDictionary*)proposed {
    return guiTextControlTypingAttributes(self, [NSSecureTextField class], _cmd, textView, current, proposed);
}

@end

@implementation YuGuiFieldEditor

- (YuGuiTextInputHost*)inputHost {
    id client = [self delegate];
    return guiIsTextControl(client) ? static_cast<id<YuGuiTextControl>>(client).inputHost : nil;
}

- (NSView*)hitControl {
    NSView* control = [[self inputHost] control];
    return control && [self isDescendantOf:control] ? control : nil;
}

- (void)resetCursorRects {
    NSView* control = [self hitControl];
    if (!control) {
        [super resetCursorRects];
        return;
    }
    NSRect hit = [self convertRect:guiViewHitBounds(control) fromView:control];
    NSRect area = NSIntersectionRect([self visibleRect], hit);
    if (!NSIsEmptyRect(area)) {
        [self addCursorRect:area cursor:[NSCursor IBeamCursor]];
    }
}

- (void)mouseMoved:(NSEvent*)event {
    NSView* control = [self hitControl];
    NSPoint point = [[control superview] convertPoint:[event locationInWindow] fromView:nil];
    if (control && !guiTextControlContains(control, point)) {
        return;
    }
    [super mouseMoved:event];
}

- (void)mouseDown:(NSEvent*)event {
    [[self inputHost] applyEditorLayout];
    [super mouseDown:event];
}

- (id<NSDraggingDestination>)dropWindow {
    return static_cast<id<NSDraggingDestination>>([self window]);
}

- (void)paste:(id)sender {
    YuGuiTextInputHost* host = [self inputHost];
    if (host && guiPasteboardHoldsImages([NSPasteboard generalPasteboard])) {
        [host sendEventKind:7];
        return;
    }
    [super paste:sender];
}

- (BOOL)forwardsCopy {
    return [self inputHost] && [self selectedRange].length == 0;
}

- (void)copy:(id)sender {
    if ([self forwardsCopy]) {
        [[self inputHost] sendEventKind:8];
        return;
    }
    [super copy:sender];
}

- (void)undo:(id)sender {
    [self performUndo:NO];
}

- (void)redo:(id)sender {
    [self performUndo:YES];
}

- (void)performUndo:(BOOL)redoes {
    NSUndoManager* manager = [self undoManager];
    if (redoes ? [manager canRedo] : [manager canUndo]) {
        if (redoes) {
            [manager redo];
        } else {
            [manager undo];
        }
        return;
    }
    guiPerformEditKey([[self inputHost] guiWindow], [self window], @"z", 6, redoes);
}

- (BOOL)validateMenuItem:(NSMenuItem*)item {
    if ([item action] == @selector(copy:) && [self forwardsCopy]) {
        return YES;
    }
    if (guiIsUndoAction([item action])) {
        bool redoes = [item action] == @selector(redo:);
        NSUndoManager* manager = [self undoManager];
        if (redoes ? [manager canRedo] : [manager canUndo]) {
            [item setTitle:redoes ? [manager redoMenuItemTitle] : [manager undoMenuItemTitle]];
            return YES;
        }
        return guiValidateRoutedUndo([[self inputHost] guiWindow], item);
    }
    if ([NSTextView instancesRespondToSelector:@selector(validateMenuItem:)]) {
        return [super validateMenuItem:item];
    }
    return [super validateUserInterfaceItem:item];
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    self.routesDrag = guiPasteboardHoldsImages([sender draggingPasteboard]);
    return self.routesDrag ? [[self dropWindow] draggingEntered:sender] : [super draggingEntered:sender];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    return self.routesDrag ? [[self dropWindow] draggingUpdated:sender] : [super draggingUpdated:sender];
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
    if (self.routesDrag) {
        self.routesDrag = NO;
        [[self dropWindow] draggingExited:sender];
        return;
    }
    [super draggingExited:sender];
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender {
    return self.routesDrag ? YES : [super prepareForDragOperation:sender];
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    return self.routesDrag ? [[self dropWindow] performDragOperation:sender] : [super performDragOperation:sender];
}

- (void)concludeDragOperation:(id<NSDraggingInfo>)sender {
    if (!self.routesDrag) {
        [super concludeDragOperation:sender];
    }
}

- (void)draggingEnded:(id<NSDraggingInfo>)sender {
    if (self.routesDrag) {
        self.routesDrag = NO;
        [[self dropWindow] draggingEnded:sender];
        return;
    }
    if ([NSTextView instancesRespondToSelector:_cmd]) {
        [super draggingEnded:sender];
    }
}

@end

@implementation YuGuiBoxHost

- (void)dealloc {
    [self.clipView removeFromSuperview];
}

@end

@interface YuGuiMaterialHost : NSObject

@property(nonatomic, strong) YuGuiMaterialView* view;

@end

@implementation YuGuiMaterialHost

- (void)dealloc {
    [self.view removeFromSuperview];
}

@end

// **********************************************************************************************

@interface YuGuiPopupCanvasView : YuGuiCanvasView
@end

@implementation YuGuiPopupCanvasView

- (BOOL)needsPanelToBecomeKey {
    return NO;
}

- (BOOL)yieldsFocusToTextInput {
    return NO;
}

- (void)updateTrackingAreas {
    if (self.pointerTrackingArea) {
        [self removeTrackingArea:self.pointerTrackingArea];
        self.pointerTrackingArea = nil;
    }
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc]
        initWithRect:NSZeroRect
             options:NSTrackingMouseMoved |
                     NSTrackingMouseEnteredAndExited |
                     NSTrackingActiveAlways |
                     NSTrackingInVisibleRect
               owner:self
            userInfo:nil
    ];
    self.pointerTrackingArea = trackingArea;
    [self addTrackingArea:trackingArea];
    [super updateTrackingAreas];
}

@end

@implementation YuGuiPopupPanel

- (NSText*)fieldEditor:(BOOL)createFlag forObject:(id)object {
    if (!guiUsesFieldEditor(object)) {
        return [super fieldEditor:createFlag forObject:object];
    }
    if (!self.guiFieldEditor && createFlag) {
        self.guiFieldEditor = guiMakeFieldEditor();
    }
    return self.guiFieldEditor;
}

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return NO;
}

- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen*)screen {
    if (self.isUnconstrained) {
        return frameRect;
    }
    return [super constrainFrameRect:frameRect toScreen:screen];
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
    if (guiIsHelpMenuKey(event) && [self.ownerHost handleKeyEvent:event]) {
        return YES;
    }
    return [super performKeyEquivalent:event];
}

- (void)sendEvent:(NSEvent*)event {
    YuGuiWindowHost* owner = self.ownerHost;
    if (!owner || owner.closed) {
        [super sendEvent:event];
        return;
    }
    NSEventType type = [event type];
    if (type == NSEventTypeFlagsChanged) {
        [owner dispatchWindowEventKind:7 key:0 nativeKeyCode:0
                             modifiers:guiModifierBits([event modifierFlags]) isRepeat:NO];
    }
    if (type == NSEventTypeScrollWheel || type == NSEventTypeMagnify) {
        NSPoint windowPoint = [owner windowPointForEvent:event];
        if ([owner isCanvasTargetAtWindowPoint:windowPoint] ||
            (type == NSEventTypeScrollWheel && guiWindowPeerPassesScroll(self, [event locationInWindow]))) {
            [owner dispatchPointerEvent:event kind:type == NSEventTypeMagnify ? 7 : 4];
            return;
        }
        [super sendEvent:event];
        return;
    }
    if (guiIsPointerEventType(type)) {
        [owner handleWindowEvent:event];
    }
    if (type == NSEventTypeKeyDown && [owner handleKeyEvent:event]) {
        return;
    }
    if (type == NSEventTypeKeyUp) {
        [owner handleKeyReleaseEvent:event];
    }
    [super sendEvent:event];
}

- (void)selectKeyViewFollowingView:(NSView*)view {
    if (self.ownerHost && guiResponderPeerOwnsKeys(view)) {
        [self.ownerHost dispatchPeerFocusExit:NO];
        return;
    }
    [super selectKeyViewFollowingView:view];
}

- (void)selectKeyViewPrecedingView:(NSView*)view {
    if (self.ownerHost && guiResponderPeerOwnsKeys(view)) {
        [self.ownerHost dispatchPeerFocusExit:YES];
        return;
    }
    [super selectKeyViewPrecedingView:view];
}

- (void)resignKeyWindow {
    [super resignKeyWindow];
    [self.ownerHost settleKeyResignation];
}

- (NSPoint)ownerPointForDrag:(id<NSDraggingInfo>)sender {
    NSPoint screenPoint = [self convertPointToScreen:[sender draggingLocation]];
    return [self.ownerHost.window convertPointFromScreen:screenPoint];
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    return [self.ownerHost dispatchDrop:sender kind:9 windowPoint:[self ownerPointForDrag:sender]];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    return [self.ownerHost dispatchDrop:sender kind:10 windowPoint:[self ownerPointForDrag:sender]];
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
    [self.ownerHost dispatchDrop:sender kind:11 windowPoint:[self ownerPointForDrag:sender]];
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    return [self.ownerHost performDrop:sender windowPoint:[self ownerPointForDrag:sender]];
}

- (void)draggingEnded:(id<NSDraggingInfo>)sender {
    [self.ownerHost dispatchDrop:sender kind:11 windowPoint:[self ownerPointForDrag:sender]];
}

@end

@implementation YuGuiPopupHost

- (NSRect)windowFrame {
    return [self.guiWindow.contentView convertRect:self.contentFrame toView:nil];
}

- (BOOL)containsWindowPoint:(NSPoint)windowPoint {
    return NSPointInRect(windowPoint, [self windowFrame]);
}

- (void)markShadowStale {
    if (![self.panel hasShadow]) {
        return;
    }
    self.shadowStale = YES;
    [self scheduleShadowSettle];
}

- (void)scheduleShadowSettle {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self settleShadow];
    });
}

- (void)settleShadow {
    if (!self.shadowStale || self.presentationPending || !self.shown || ![self.panel isVisible]) {
        return;
    }
    self.shadowStale = NO;
    if ([self.panel hasShadow]) {
        [CATransaction flush];
        [self.panel invalidateShadow];
    }
}

- (BOOL)nativeControlBlocksWindowPoint:(NSPoint)windowPoint {
    NSView* root = self.boxHost.contentView;
    if (!root) {
        return NO;
    }
    NSPoint screenPoint = [self.guiWindow.window convertPointToScreen:windowPoint];
    NSPoint panelPoint = [self.panel convertPointFromScreen:screenPoint];
    for (NSView* subview in [root subviews]) {
        if (guiViewBlocksCanvas(subview, panelPoint)) {
            return YES;
        }
    }
    return NO;
}

- (void)hide {
    if (!self.shown) {
        return;
    }
    self.shown = NO;
    BOOL wasKey = [self.panel isKeyWindow];
    NSWindow* parent = [self.panel parentWindow];
    if (parent) {
        [parent removeChildWindow:self.panel];
    }
    [self.panel orderOut:nil];
    if (wasKey && self.guiWindow && !self.guiWindow.closed) {
        [self.guiWindow.window makeKeyWindow];
    }
}

- (void)dealloc {
    [self hide];
    self.panel.ownerHost = nil;
    [self.panel close];
}

@end

// **********************************************************************************************

static NSString*
stringFromUTF8(const char* value) {
    if (!value) {
        return @"";
    }

    NSString* string = [NSString stringWithUTF8String:value];
    return string ? string : @"";
}

static void
guiInstallDefaultMenuBar(void);

static void
guiWatchApplicationAppearance(NSApplication* application);

static NSApplication*
ensureApplication() {
    NSApplication* application = [NSApplication sharedApplication];
    BOOL isAgent = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"LSUIElement"] boolValue];
    [application setActivationPolicy:isAgent ? NSApplicationActivationPolicyAccessory
                                             : NSApplicationActivationPolicyRegular];

    if (![application delegate]) {
        [[NSUserDefaults standardUserDefaults]
            registerDefaults:@{@"NSTreatUnknownArgumentsAsOpen": @"NO"}];
        appDelegate = [[YuGuiAppDelegate alloc] init];
        [application setDelegate:appDelegate];
    }
    if (![application mainMenu]) {
        guiInstallDefaultMenuBar();
    }
    guiWatchApplicationAppearance(application);

    return application;
}

static YuGuiWindowHost*
getWindow(void* ptr) {
    return (__bridge YuGuiWindowHost*)ptr;
}

static YuGuiTextInputHost*
getTextInput(void* ptr) {
    return (__bridge YuGuiTextInputHost*)ptr;
}

static YuGuiBoxHost*
getBoxHost(void* ptr) {
    return (__bridge YuGuiBoxHost*)ptr;
}

static NSRect
rectFromTopLeft(NSView* view, double x, double y, double width, double height) {
    return NSMakeRect(x, y, width, height);
}

static bool
guiSetViewGeometry(NSView* view, NSRect frame, double a, double b, double c, double d) {
    double scaleX = std::hypot(a, b);
    double determinant = a * d - b * c;
    bool linear = std::isfinite(scaleX) && std::isfinite(determinant) && scaleX > 1e-9 && determinant > 1e-12;
    double scaleY = linear ? determinant / scaleX : 1;
    double degrees = linear ? std::atan2(b, a) * 180 / M_PI : 0;
    if (!linear) {
        scaleX = 1;
    }
    NSRect outer = NSMakeRect(
        frame.origin.x,
        frame.origin.y,
        std::max(0.0, frame.size.width) * scaleX,
        std::max(0.0, frame.size.height) * scaleY
    );
    NSSize inner = NSMakeSize(std::max(0.0, frame.size.width), std::max(0.0, frame.size.height));
    NSSize bounds = [view bounds].size;
    if (NSEqualRects([view frame], outer) && [view frameRotation] == degrees &&
        std::fabs(bounds.width - inner.width) < 1e-6 && std::fabs(bounds.height - inner.height) < 1e-6) {
        return false;
    }
    [view setFrame:outer];
    [view setFrameRotation:degrees];
    [view setBoundsSize:inner];
    return true;
}

// **********************************************************************************************

extern "C" void*
yumiGuiAppCreate(void) {
    YuGuiAppHost* app = [[YuGuiAppHost alloc] init];
    app->scheduledStopGeneration.store(0, std::memory_order_relaxed);
    return (__bridge_retained void*)app;
}

extern "C" void
yumiGuiAppRelease(void* ptr) {
    if (ptr) {
        YuGuiAppHost* app = (__bridge YuGuiAppHost*)ptr;
        app->scheduledStopGeneration.fetch_add(1, std::memory_order_release);
        guiAppQuitCallback = nullptr;
        guiAppQuitContext = nullptr;
        guiAppBackgroundCallback = nullptr;
        guiAppBackgroundContext = nullptr;
        guiAppOpenCallback = nullptr;
        guiAppOpenContext = nullptr;
        guiAppSystemCallback = nullptr;
        guiAppSystemContext = nullptr;
        guiAppKeepsDisplayAwake = false;
        guiApplyKeepsDisplayAwake();
        CFAutorelease(ptr);
    }
}

extern "C" void
yumiGuiAppRun(void* ptr) {
    ensureApplication();
    if (!yumiRunMainTaskQueueHost(guiMainQueueRunHost, guiMainQueueWake, ptr) &&
        !yumiProcessTerminationRequested()) {
        std::fprintf(stderr, "App.run must own the active main Yumi task queue\n");
        std::abort();
    }
}

extern "C" void
yumiGuiAppStop(void* ptr) {
    if (ptr) {
        YuGuiAppHost* app = (__bridge YuGuiAppHost*)ptr;
        app->scheduledStopGeneration.fetch_add(1, std::memory_order_release);
    }
    guiMainQueueRequestStop();
}

extern "C" void
yumiGuiAppScheduleStop(void* ptr, uint64_t delayNanoseconds) {
    if (!ptr) {
        return;
    }

    YuGuiAppHost* app = (__bridge YuGuiAppHost*)ptr;
    uint64_t generation =
        app->scheduledStopGeneration.fetch_add(1, std::memory_order_acq_rel) + 1;
    uint64_t boundedDelayNanoseconds = std::min<uint64_t>(
        delayNanoseconds,
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max())
    );
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(boundedDelayNanoseconds)),
        dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
        ^{
            if (app->scheduledStopGeneration.load(std::memory_order_acquire) == generation) {
                guiMainQueueRequestStop();
            }
        }
    );
}

// **********************************************************************************************

static const uint64_t kGuiMenuUpdateBudget = 100000000ull;

struct YuGuiMenuCallbackRecord : YuGuiCallbackRecord {
    YuGuiMenuCallbackRecord(
        YuGuiMenuCallback aCallback,
        void* aContext,
        uint32_t anEventKind,
        uint64_t anIndex):
        YuGuiCallbackRecord(aContext),
        callback(aCallback),
        eventKind(anEventKind),
        index(anIndex) {}
    YuGuiMenuCallback callback;
    uint32_t eventKind;
    uint64_t index;
};

static void
guiRunMenuCallback(void* closure) {
    YuGuiMenuCallbackRecord* record = static_cast<YuGuiMenuCallbackRecord*>(closure);
    if (!yumiProcessTerminationRequested()) {
        record->callback(record->context, record->eventKind, record->index);
    }
    guiFinishCallbackTask(record);
}

@interface YuGuiMenu : NSMenu <NSMenuDelegate>
@property(nonatomic, assign) YuGuiMenuCallback callback;
@property(nonatomic, assign) void* context;
@property(nonatomic, assign) BOOL populated;
@end

static SEL
guiMenuRoleAction(uint32_t role) {
    switch (role) {
        case 1: return @selector(undo:);
        case 2: return @selector(redo:);
        case 3: return @selector(cut:);
        case 4: return @selector(copy:);
        case 5: return @selector(paste:);
        case 6: return @selector(selectAll:);
        case 7: return @selector(performMiniaturize:);
        case 8: return @selector(performZoom:);
        case 9: return @selector(performClose:);
        case 10: return @selector(arrangeInFront:);
        case 11: return @selector(toggleFullScreen:);
        case 12: return @selector(delete:);
        case 13: return @selector(pasteAsPlainText:);
        case 14: return @selector(yumiZoomIn:);
        case 15: return @selector(yumiZoomOut:);
        case 16: return @selector(yumiActualSize:);
        default: return nullptr;
    }
}

static BOOL
guiMenuItemHasRole(NSMenuItem* item) {
    return [item action] != nullptr && [item action] != @selector(chooseItem:) && ![item hasSubmenu];
}

static BOOL
guiMenuRoleIsEnabled(NSMenuItem* item) {
    id target = [NSApp targetForAction:[item action] to:nil from:item];
    if (!target) {
        return NO;
    }
    if ([target respondsToSelector:@selector(validateMenuItem:)]) {
        return [target validateMenuItem:item];
    }
    if ([target conformsToProtocol:@protocol(NSUserInterfaceValidations)]) {
        return [target validateUserInterfaceItem:item];
    }
    return YES;
}

static BOOL
guiMenuItemMatchesKey(NSMenuItem* item, NSEvent* event) {
    NSString* key = [item keyEquivalent];
    if (![key length]) {
        return NO;
    }
    NSEventModifierFlags mask = NSEventModifierFlagCommand | NSEventModifierFlagShift |
        NSEventModifierFlagOption | NSEventModifierFlagControl;
    if (([event modifierFlags] & mask) != ([item keyEquivalentModifierMask] & mask)) {
        return NO;
    }
    NSString* typed = [[event charactersIgnoringModifiers] lowercaseString];
    return [typed isEqualToString:[key lowercaseString]];
}

@implementation YuGuiMenu

- (BOOL)performKeyEquivalent:(NSEvent*)event {
    if (!self.populated) {
        [self menuNeedsUpdate:self];
    }
    for (NSMenuItem* item in [self itemArray]) {
        if (!guiMenuItemHasRole(item) || guiIsZoomAction([item action]) ||
            !guiMenuItemMatchesKey(item, event)) {
            continue;
        }
        if (![item isEnabled] || !guiMenuRoleIsEnabled(item)) {
            return NO;
        }
        return [NSApp sendAction:[item action] to:nil from:item];
    }
    return NO;
}

- (void)menuNeedsUpdate:(NSMenu*)menu {
    if (!self.callback) {
        return;
    }
    YuGuiMenuCallbackRecord* record =
        new YuGuiMenuCallbackRecord(self.callback, self.context, 0, 0);
    uint32_t result = yumiRunMainTaskFromNativeUntilDeadline(
        guiRunMenuCallback, record, guiDeadlineAfter(kGuiMenuUpdateBudget));
    if (result == YuMainTaskDeadlineFailed) {
        delete record;
    }
    if (result == YuMainTaskDeadlineCompleted) {
        self.populated = YES;
    }
    for (NSMenuItem* item in [self itemArray]) {
        if (guiMenuItemHasRole(item) && [item isEnabled]) {
            [item setEnabled:guiMenuRoleIsEnabled(item)];
        }
    }
}

- (void)chooseItem:(NSMenuItem*)item {
    if (!self.callback) {
        return;
    }
    YuGuiMenuCallbackRecord* record = new YuGuiMenuCallbackRecord(
        self.callback, self.context, 1, static_cast<uint64_t>([item tag]));
    if (!yumiScheduleMainTaskFromNative(guiRunMenuCallback, record)) {
        delete record;
    }
}

@end

@interface YuGuiAppMenuTarget : NSObject
@end

@implementation YuGuiAppMenuTarget

- (void)quit:(id)sender {
    guiRequestAppQuit();
}

@end

static void
guiRequestAppQuit(void) {
    if (guiAppQuitCallback) {
        guiScheduleActionCallback(guiAppQuitCallback, guiAppQuitContext, 0);
    } else {
        guiMainQueueRequestStop();
    }
}

static YuGuiAppMenuTarget* guiAppMenuTarget = nil;
static NSMenu* guiDefaultMenuBar = nil;
static NSString* guiAppTitle = nil;

static const NSInteger kGuiAppMenuAboutTag = 1;
static const NSInteger kGuiAppMenuHideTag = 2;
static const NSInteger kGuiAppMenuQuitTag = 3;

static NSString*
guiCurrentAppTitle(void) {
    return guiAppTitle.length ? guiAppTitle : [[NSProcessInfo processInfo] processName];
}

static void
guiRefreshAppMenu(NSMenu* bar) {
    if (!bar || [bar numberOfItems] == 0) {
        return;
    }
    NSMenuItem* appItem = [bar itemAtIndex:0];
    NSMenu* appMenu = [appItem submenu];
    if (!appMenu) {
        return;
    }
    NSString* name = guiCurrentAppTitle();
    [appItem setTitle:name];
    [appMenu setTitle:name];
    [[appMenu itemWithTag:kGuiAppMenuAboutTag] setTitle:[@"About " stringByAppendingString:name]];
    [[appMenu itemWithTag:kGuiAppMenuHideTag] setTitle:[@"Hide " stringByAppendingString:name]];
    [[appMenu itemWithTag:kGuiAppMenuQuitTag] setTitle:[@"Quit " stringByAppendingString:name]];
}

static NSMenuItem*
guiAppMenuItem(NSString* title, SEL action, NSString* key, NSEventModifierFlags modifiers) {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:key];
    [item setKeyEquivalentModifierMask:modifiers];
    return item;
}

static NSMenu*
guiCreateMenuBar(void) {
    if (!guiAppMenuTarget) {
        guiAppMenuTarget = [[YuGuiAppMenuTarget alloc] init];
    }
    NSMenu* bar = [[NSMenu alloc] initWithTitle:@""];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* about = guiAppMenuItem(@"", @selector(orderFrontStandardAboutPanel:), @"", 0);
    [about setTag:kGuiAppMenuAboutTag];
    [appMenu addItem:about];
    [appMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* hide = guiAppMenuItem(@"", @selector(hide:), @"h", NSEventModifierFlagCommand);
    [hide setTag:kGuiAppMenuHideTag];
    [appMenu addItem:hide];
    [appMenu addItem:guiAppMenuItem(
        @"Hide Others",
        @selector(hideOtherApplications:),
        @"h",
        NSEventModifierFlagCommand | NSEventModifierFlagOption)];
    [appMenu addItem:guiAppMenuItem(@"Show All", @selector(unhideAllApplications:), @"", 0)];
    [appMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* quit = guiAppMenuItem(@"", @selector(quit:), @"q", NSEventModifierFlagCommand);
    [quit setTag:kGuiAppMenuQuitTag];
    [quit setTarget:guiAppMenuTarget];
    [appMenu addItem:quit];
    NSMenuItem* appItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [appItem setSubmenu:appMenu];
    [bar addItem:appItem];
    guiRefreshAppMenu(bar);
    return bar;
}

static void
guiInstallDefaultMenuBar(void) {
    if (!guiDefaultMenuBar) {
        guiDefaultMenuBar = guiCreateMenuBar();
    }
    guiRefreshAppMenu(guiDefaultMenuBar);
    [NSApp setMainMenu:guiDefaultMenuBar];
}

extern "C" void
yumiGuiAppSetTitle(void* app, const char* title) {
    guiAppTitle = [stringFromUTF8(title) copy];
    if (!NSApp) {
        return;
    }
    NSMenu* bar = [NSApp mainMenu];
    guiRefreshAppMenu(bar);
    if (guiDefaultMenuBar && guiDefaultMenuBar != bar) {
        guiRefreshAppMenu(guiDefaultMenuBar);
    }
}

extern "C" void
yumiGuiAppSetQuitCallback(void* app, YuGuiActionCallback callback, void* context) {
    guiAppQuitCallback = callback;
    guiAppQuitContext = context;
}

extern "C" void
yumiGuiAppSetBackgroundCallback(void* app, YuGuiActionCallback callback, void* context) {
    guiAppBackgroundCallback = callback;
    guiAppBackgroundContext = context;
}

extern "C" void
yumiGuiAppSetOpenCallback(void* app, YuGuiOpenCallback callback, void* context) {
    guiAppOpenCallback = callback;
    guiAppOpenContext = context;
}

static void
guiApplyKeepsDisplayAwake(void) {
    bool keeps = guiAppKeepsDisplayAwake && [NSApp isActive];
    if (keeps && !guiAppDisplayAwakeActivity) {
        guiAppDisplayAwakeActivity = [[NSProcessInfo processInfo]
            beginActivityWithOptions:NSActivityIdleDisplaySleepDisabled | NSActivityUserInitiated
                              reason:@"The app keeps the display awake while it is in use"];
    } else if (!keeps && guiAppDisplayAwakeActivity) {
        [[NSProcessInfo processInfo] endActivity:guiAppDisplayAwakeActivity];
        guiAppDisplayAwakeActivity = nil;
    }
}

extern "C" void
yumiGuiAppSetKeepsDisplayAwake(void* app, bool keeps) {
    guiAppKeepsDisplayAwake = keeps;
    guiApplyKeepsDisplayAwake();
}

extern "C" uint32_t
yumiGuiSystemAppearance(void) {
    NSAppearance* appearance = [NSApplication sharedApplication].effectiveAppearance;
    NSAppearanceName match = [appearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
    return [match isEqualToString:NSAppearanceNameDarkAqua] ? 2 : 1;
}

@interface YuGuiSystemObserver : NSObject
@end

@implementation YuGuiSystemObserver

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id>*)change
                       context:(void*)context {
    [self systemChanged:nil];
}

- (void)systemChanged:(NSNotification*)notification {
    guiScheduleActionCallback(guiAppSystemCallback, guiAppSystemContext, 0);
}

@end

static YuGuiSystemObserver* guiSystemObserver = nil;
static bool guiSystemObserverWatchesApplication = false;

static void
guiWatchApplicationAppearance(NSApplication* application) {
    if (!guiSystemObserver || guiSystemObserverWatchesApplication) {
        return;
    }
    guiSystemObserverWatchesApplication = true;
    [application addObserver:guiSystemObserver forKeyPath:@"effectiveAppearance" options:0 context:nullptr];
}

extern "C" void
yumiGuiAppSetSystemCallback(void* app, YuGuiActionCallback callback, void* context) {
    guiAppSystemCallback = callback;
    guiAppSystemContext = context;
    if (guiSystemObserver) {
        return;
    }
    guiSystemObserver = [[YuGuiSystemObserver alloc] init];
    if (NSApp) {
        guiWatchApplicationAppearance(NSApp);
    }
    [[NSNotificationCenter defaultCenter] addObserver:guiSystemObserver
                                             selector:@selector(systemChanged:)
                                                 name:NSSystemColorsDidChangeNotification
                                               object:nil];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:guiSystemObserver
                                                           selector:@selector(systemChanged:)
                                                               name:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification
                                                             object:nil];
}

static uint32_t
guiPackedColor(NSColor* color) {
    NSColor* converted = [color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
    if (!converted) {
        return 0;
    }
    CGFloat components[4] = {
        [converted alphaComponent],
        [converted blueComponent],
        [converted greenComponent],
        [converted redComponent],
    };
    uint32_t packed = 0;
    for (CGFloat component : components) {
        packed = (packed << 8) | static_cast<uint32_t>(lround(std::min(std::max(component, 0.0), 1.0) * 255.0));
    }
    return packed;
}

extern "C" uint32_t
yumiGuiSystemColor(uint32_t color, uint32_t appearance) {
    NSAppearance* drawn = [NSAppearance appearanceNamed:appearance == 2 ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
    __block uint32_t packed = 0;
    [drawn performAsCurrentDrawingAppearance:^{
        NSColor* named = nil;
        switch (color) {
            case 1: named = [NSColor controlAccentColor]; break;
            case 2: named = [NSColor selectedContentBackgroundColor]; break;
            case 3: named = [NSColor unemphasizedSelectedContentBackgroundColor]; break;
            default: break;
        }
        if (named) {
            packed = guiPackedColor(named);
        }
    }];
    return packed;
}

extern "C" uint32_t
yumiGuiSystemOptions(void) {
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    uint32_t options = 0;
    if ([workspace accessibilityDisplayShouldReduceMotion]) {
        options |= 1;
    }
    if ([workspace accessibilityDisplayShouldIncreaseContrast]) {
        options |= 2;
    }
    if ([workspace accessibilityDisplayShouldReduceTransparency]) {
        options |= 4;
    }
    return options;
}

static NSString*
guiMenuKeyEquivalent(const char* characters, uint32_t keyKind) {
    unichar code = 0;
    switch (keyKind) {
        case 1: code = '\t'; break;
        case 2: code = '\r'; break;
        case 3: code = 0x1b; break;
        case 4: code = ' '; break;
        case 5: code = NSUpArrowFunctionKey; break;
        case 6: code = NSDownArrowFunctionKey; break;
        case 7: code = NSLeftArrowFunctionKey; break;
        case 8: code = NSRightArrowFunctionKey; break;
        case 9: code = NSHomeFunctionKey; break;
        case 10: code = NSEndFunctionKey; break;
        case 11: code = NSPageUpFunctionKey; break;
        case 12: code = NSPageDownFunctionKey; break;
        case 13: code = 0x08; break;
        case 14: code = NSDeleteFunctionKey; break;
        default:
            if (keyKind >= 100 && keyKind <= 134) {
                code = static_cast<unichar>(NSF1FunctionKey + (keyKind - 100));
            }
            break;
    }
    if (code) {
        return [NSString stringWithCharacters:&code length:1];
    }
    NSString* text = characters ? [NSString stringWithUTF8String:characters] : nil;
    return [text lowercaseString] ?: @"";
}

static NSEventModifierFlags
guiMenuModifierFlags(uint64_t modifiers) {
    NSEventModifierFlags flags = 0;
    if (modifiers & 1) flags |= NSEventModifierFlagShift;
    if (modifiers & 2) flags |= NSEventModifierFlagControl;
    if (modifiers & 4) flags |= NSEventModifierFlagOption;
    if (modifiers & 8) flags |= NSEventModifierFlagCommand;
    return flags;
}

extern "C" void*
yumiGuiMenuBarCreate(void) {
    ensureApplication();
    return (__bridge_retained void*)guiCreateMenuBar();
}

extern "C" void
yumiGuiMenuBarInstall(void* ptr) {
    NSMenu* bar = (__bridge NSMenu*)ptr;
    if (bar) {
        guiRefreshAppMenu(bar);
        [NSApp setMainMenu:bar];
    } else {
        guiInstallDefaultMenuBar();
    }
}

extern "C" void
yumiGuiMenuBarUninstall(void* ptr) {
    if (ptr && [NSApp mainMenu] == (__bridge NSMenu*)ptr) {
        guiInstallDefaultMenuBar();
    }
}

extern "C" void*
yumiGuiMenuCreate(const char* title, YuGuiMenuCallback callback, void* context) {
    YuGuiMenu* menu = [[YuGuiMenu alloc] initWithTitle:stringFromUTF8(title)];
    [menu setAutoenablesItems:NO];
    [menu setDelegate:menu];
    menu.callback = callback;
    menu.context = context;
    return (__bridge_retained void*)menu;
}

extern "C" void
yumiGuiMenuRelease(void* ptr) {
    NSMenu* menu = (__bridge NSMenu*)ptr;
    if (!menu) {
        return;
    }
    if ([menu isKindOfClass:[YuGuiMenu class]]) {
        YuGuiMenu* owned = (YuGuiMenu*)menu;
        owned.callback = nullptr;
        owned.context = nullptr;
        [owned setDelegate:nil];
    }
    if ([NSApp mainMenu] == menu) {
        guiInstallDefaultMenuBar();
    }
    CFBridgingRelease(ptr);
}

extern "C" void
yumiGuiMenuRemoveAllItems(void* ptr) {
    NSMenu* menu = (__bridge NSMenu*)ptr;
    if (!menu) {
        return;
    }
    if ([menu isKindOfClass:[YuGuiMenu class]]) {
        [menu removeAllItems];
        return;
    }
    while ([menu numberOfItems] > 1) {
        [menu removeItemAtIndex:[menu numberOfItems] - 1];
    }
}

extern "C" void
yumiGuiMenuAddItem(
    void* ptr,
    const char* title,
    const char* keyCharacters,
    uint32_t keyKind,
    uint64_t modifiers,
    bool enabled,
    bool checked,
    uint32_t role,
    void* submenuPtr) {
    NSMenu* menu = (__bridge NSMenu*)ptr;
    if (!menu) {
        return;
    }
    NSMenu* submenu = (__bridge NSMenu*)submenuPtr;
    SEL roleAction = submenu ? nullptr : guiMenuRoleAction(role);
    NSMenuItem* item = [[NSMenuItem alloc]
        initWithTitle:stringFromUTF8(title)
               action:submenu ? nil : (roleAction ?: @selector(chooseItem:))
        keyEquivalent:submenu ? @"" : guiMenuKeyEquivalent(keyCharacters, keyKind)];
    [item setKeyEquivalentModifierMask:guiMenuModifierFlags(modifiers)];
    [item setTag:[menu numberOfItems]];
    [item setEnabled:enabled];
    [item setState:checked ? NSControlStateValueOn : NSControlStateValueOff];
    if (submenu) {
        NSMenu* previous = [submenu supermenu];
        if (previous) {
            NSInteger index = [previous indexOfItemWithSubmenu:submenu];
            if (index >= 0) {
                [[previous itemAtIndex:index] setSubmenu:nil];
            }
        }
        [submenu setTitle:stringFromUTF8(title)];
        [item setSubmenu:submenu];
    } else if (!roleAction && [menu isKindOfClass:[YuGuiMenu class]]) {
        [item setTarget:menu];
    }
    [menu addItem:item];
}

extern "C" void
yumiGuiMenuPerformRole(uint32_t role) {
    SEL action = guiMenuRoleAction(role);
    if (action) {
        [NSApp sendAction:action to:nil from:nil];
    }
}

extern "C" void
yumiGuiMenuAddSeparator(void* ptr) {
    NSMenu* menu = (__bridge NSMenu*)ptr;
    if (menu) {
        [menu addItem:[NSMenuItem separatorItem]];
    }
}

// **********************************************************************************************

extern "C" void*
yumiGuiStatusItemCreate(void) {
    ensureApplication();
    NSStatusItem* item =
        [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    guiStatusItemCount += 1;
    return (__bridge_retained void*)item;
}

extern "C" void
yumiGuiStatusItemRelease(void* ptr) {
    NSStatusItem* item = (__bridge NSStatusItem*)ptr;
    if (!item) {
        return;
    }
    yumiGuiStatusItemSetCallback(ptr, nullptr, nullptr);
    [item setMenu:nil];
    [[NSStatusBar systemStatusBar] removeStatusItem:item];
    CFBridgingRelease(ptr);
    guiStatusItemCount -= 1;
}

extern "C" void
yumiGuiStatusItemSetTitle(void* ptr, const char* title) {
    NSStatusItem* item = (__bridge NSStatusItem*)ptr;
    if (item) {
        [[item button] setTitle:stringFromUTF8(title)];
    }
}

extern "C" void
yumiGuiStatusItemSetMenu(void* ptr, void* menu) {
    NSStatusItem* item = (__bridge NSStatusItem*)ptr;
    if (item) {
        [item setMenu:(__bridge NSMenu*)menu];
    }
}

struct YuGuiStatusItemCallbackRecord : YuGuiCallbackRecord {
    YuGuiStatusItemCallbackRecord(YuGuiStatusItemCallback aCallback, void* aContext, uint32_t anEventKind):
        YuGuiCallbackRecord(aContext),
        callback(aCallback),
        eventKind(anEventKind) {}
    YuGuiStatusItemCallback callback;
    uint32_t eventKind;
};

static void
guiRunStatusItemCallback(void* closure) {
    YuGuiStatusItemCallbackRecord* record = static_cast<YuGuiStatusItemCallbackRecord*>(closure);
    if (!yumiProcessTerminationRequested()) {
        record->callback(record->context, record->eventKind);
    }
    guiFinishCallbackTask(record);
}

static const NSEventMask kGuiStatusItemPressMask =
    NSEventMaskLeftMouseDown | NSEventMaskRightMouseDown | NSEventMaskOtherMouseDown;

static const uint32_t kGuiStatusItemPressed = 0;
static const uint32_t kGuiStatusItemDismissed = 1;
static const uint32_t kGuiStatusItemMoved = 2;

@interface YuGuiStatusItemTarget : NSObject
@property(nonatomic, weak) NSStatusItem* item;
@property(nonatomic, assign) YuGuiStatusItemCallback callback;
@property(nonatomic, assign) void* context;
@property(nonatomic, strong) YuGuiWindowHost* panel;
@property(nonatomic, strong) id localMonitor;
@property(nonatomic, strong) id globalMonitor;
@end

@implementation YuGuiStatusItemTarget

- (void)dealloc {
    [self endPanel];
    if (self.localMonitor) {
        [NSEvent removeMonitor:self.localMonitor];
    }
}

- (void)send:(uint32_t)eventKind {
    if (!self.callback) {
        return;
    }
    YuGuiStatusItemCallbackRecord* record =
        new YuGuiStatusItemCallbackRecord(self.callback, self.context, eventKind);
    if (!yumiScheduleMainTaskFromNative(guiRunStatusItemCallback, record)) {
        delete record;
    }
}

- (void)activate:(id)sender {
    [self send:kGuiStatusItemPressed];
}

- (void)dismiss:(NSNotification*)notification {
    [self send:kGuiStatusItemDismissed];
}

- (void)anchorMoved:(NSNotification*)notification {
    [self send:kGuiStatusItemMoved];
}

- (void)panelClosed:(NSNotification*)notification {
    [self endPanel];
}

- (NSEvent*)filterPress:(NSEvent*)event {
    NSWindow* anchor = [[self.item button] window];
    NSWindow* window = [event window];
    if (anchor && window == anchor) {
        if ([event type] == NSEventTypeLeftMouseDown && ![self.item menu]) {
            [self send:kGuiStatusItemPressed];
            return nil;
        }
        return event;
    }
    if (self.panel && ![self.panel ownsWindow:window]) {
        [self send:kGuiStatusItemDismissed];
    }
    return event;
}

- (void)startMonitoring {
    __weak YuGuiStatusItemTarget* target = self;
    self.localMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:kGuiStatusItemPressMask
                                                              handler:^NSEvent*(NSEvent* event) {
        return [target filterPress:event];
    }];
}

- (void)beginPanel:(YuGuiWindowHost*)panel {
    [self endPanel];
    self.panel = panel;
    NSStatusBarButton* button = [self.item button];
    [button highlight:YES];
    __weak YuGuiStatusItemTarget* target = self;
    self.globalMonitor = [NSEvent addGlobalMonitorForEventsMatchingMask:kGuiStatusItemPressMask
                                                                handler:^(NSEvent* event) {
        [target dismiss:nil];
    }];
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(panelClosed:)
                   name:NSWindowWillCloseNotification
                 object:panel.window];
    [center addObserver:self
               selector:@selector(dismiss:)
                   name:NSApplicationDidResignActiveNotification
                 object:nil];
    [center addObserver:self
               selector:@selector(anchorMoved:)
                   name:NSApplicationDidChangeScreenParametersNotification
                 object:nil];
    NSWindow* anchor = [button window];
    if (anchor) {
        [center addObserver:self selector:@selector(anchorMoved:) name:NSWindowDidMoveNotification object:anchor];
        [center addObserver:self selector:@selector(anchorMoved:) name:NSWindowDidResizeNotification object:anchor];
    }
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                           selector:@selector(dismiss:)
                                                               name:NSWorkspaceActiveSpaceDidChangeNotification
                                                             object:nil];
}

- (void)endPanel {
    if (!self.panel) {
        return;
    }
    self.panel = nil;
    [[self.item button] highlight:NO];
    if (self.globalMonitor) {
        [NSEvent removeMonitor:self.globalMonitor];
        self.globalMonitor = nil;
    }
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
}

@end

static char guiStatusItemTargetKey;

extern "C" void
yumiGuiStatusItemSetCallback(void* ptr, YuGuiStatusItemCallback callback, void* context) {
    NSStatusItem* item = (__bridge NSStatusItem*)ptr;
    if (!item) {
        return;
    }
    YuGuiStatusItemTarget* target = nil;
    if (callback) {
        target = [[YuGuiStatusItemTarget alloc] init];
        target.item = item;
        target.callback = callback;
        target.context = context;
        [target startMonitoring];
    }
    NSStatusBarButton* button = [item button];
    [button setTarget:target];
    [button setAction:target ? @selector(activate:) : nil];
    objc_setAssociatedObject(item, &guiStatusItemTargetKey, target, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

extern "C" void
yumiGuiStatusItemSetPanel(void* ptr, void* window) {
    NSStatusItem* item = (__bridge NSStatusItem*)ptr;
    YuGuiStatusItemTarget* target = item ? objc_getAssociatedObject(item, &guiStatusItemTargetKey) : nil;
    YuGuiWindowHost* panel = window ? getWindow(window) : nil;
    if (panel && !panel.closed) {
        [target beginPanel:panel];
    } else {
        [target endPanel];
    }
}

static CGFloat guiPrimaryScreenTop(void);

static double
guiScreenRectComponent(NSRect rect, uint32_t component) {
    switch (component) {
        case 0: return NSMinX(rect);
        case 1: return guiPrimaryScreenTop() - NSMaxY(rect);
        case 2: return NSWidth(rect);
        default: return NSHeight(rect);
    }
}

extern "C" double
yumiGuiStatusItemGetBound(void* ptr, uint32_t component) {
    NSStatusItem* item = (__bridge NSStatusItem*)ptr;
    NSStatusBarButton* button = [item button];
    NSWindow* window = [button window];
    if (!window) {
        return 0;
    }
    return guiScreenRectComponent(
        [window convertRectToScreen:[button convertRect:[button bounds] toView:nil]], component);
}

extern "C" double
yumiGuiStatusItemGetScreenBound(void* ptr, uint32_t component) {
    NSStatusItem* item = (__bridge NSStatusItem*)ptr;
    NSScreen* screen = [[[item button] window] screen] ?: [NSScreen mainScreen];
    if (!screen) {
        return 0;
    }
    return guiScreenRectComponent([screen visibleFrame], component);
}

// **********************************************************************************************

extern "C" void*
yumiGuiWindowCreate(const char* title, double width, double height) {
    ensureApplication();
    yumiGuiMetalWarmUp();

    NSRect frame = NSMakeRect(100, 100, width, height);
    NSWindowStyleMask style =
        NSWindowStyleMaskTitled |
        NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable |
        NSWindowStyleMaskResizable;

    YuGuiEventWindow* window = [[YuGuiEventWindow alloc]
        initWithContentRect:frame
                  styleMask:style
                    backing:NSBackingStoreBuffered
                      defer:NO
    ];

    [window setTitle:stringFromUTF8(title)];
    [window setReleasedWhenClosed:NO];
    [window registerForDraggedTypes:guiDropTypes()];

    YuGuiFlippedView* contentView = [[YuGuiFlippedView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
    [contentView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    // A single layer tree gives the frame transaction one commit boundary for custom painting and
    // AppKit controls alike.
    [contentView setWantsLayer:YES];
    [window setContentView:contentView];

    YuGuiCanvasView* canvasView = [[YuGuiCanvasView alloc] initWithFrame:[contentView bounds]];
    [canvasView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [canvasView setWantsLayer:YES];
    CALayer* canvasLayer = [CALayer layer];
    [canvasView setLayer:canvasLayer];
    [canvasView setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawNever];
    [canvasLayer setContentsGravity:kCAGravityResize];
    [contentView addSubview:canvasView positioned:NSWindowBelow relativeTo:nil];

    // Keep window presentation out of the Core Graphics canvas layer tree. AppKit can continue
    // updating the canvas contents without ever touching the CAMetalLayer owned by this view.
    YuGuiMetalView* metalView = [[YuGuiMetalView alloc] initWithFrame:[contentView bounds]];
    [metalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [metalView setWantsLayer:YES];
    [metalView setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawNever];
    [metalView setHidden:YES];
    [contentView addSubview:metalView positioned:NSWindowAbove relativeTo:canvasView];

    YuGuiWindowHost* guiWindow = [[YuGuiWindowHost alloc] init];
    guiWindow->inputStreamState = std::make_shared<YuGuiInputStreamState>();
    guiWindow.window = window;
    guiWindow.titlebarVisible = YES;
    guiWindow.contentView = contentView;
    guiWindow.canvasView = canvasView;
    guiWindow.metalView = metalView;
    canvasView.inputHost = guiWindow;
    window.guiWindow = guiWindow;
    [window setAcceptsMouseMovedEvents:YES];
    [window setDelegate:guiWindow];

    return (__bridge_retained void*)guiWindow;
}

extern "C" void
yumiGuiWindowSetTransparent(void* ptr, bool windowTransparent, bool contentTransparent) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window) {
        return;
    }
    [guiWindow.window setOpaque:!windowTransparent];
    [guiWindow.window setBackgroundColor:windowTransparent ? [NSColor clearColor] : [NSColor windowBackgroundColor]];
    [[guiWindow.metalView layer] setOpaque:!contentTransparent];
    [guiWindow markShadowStale];
}

static NSAppearance*
guiAppearanceNamed(uint32_t appearance) {
    if (appearance == 1) {
        return [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    }
    if (appearance == 2) {
        return [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    }
    return nil;
}

extern "C" void
yumiGuiWindowSetAppearance(void* ptr, uint32_t appearance) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window) {
        return;
    }
    NSAppearance* chosen = guiAppearanceNamed(appearance);
    [guiWindow.window setAppearance:chosen];
    for (YuGuiPopupHost* popup in guiWindow.popupHosts) {
        [popup.panel setAppearance:chosen];
    }
}

extern "C" void*
yumiGuiWindowGetNativeWindow(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? (__bridge void*)guiWindow.window : nullptr;
}

extern "C" void
yumiGuiWindowRelease(void* ptr) {
    if (ptr) {
        YuGuiWindowHost* owner = getWindow(ptr);
        owner.canvasView.customCursor = nil;
        owner.metalView.customCursor = nil;
    }
    if (ptr) {
        YuGuiWindowHost* guiWindow = getWindow(ptr);
        [guiWindow cancelRenderFrame];
        CFAutorelease(ptr);
    }
}

extern "C" void
yumiGuiWindowShow(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window || guiWindow.closed) {
        return;
    }
    NSWindow* window = guiWindow.window;
    if (guiWindow.panel) {
        [NSApp activateIgnoringOtherApps:YES];
    }
    [window makeKeyAndOrderFront:nil];
    [guiWindow markShadowStale];
}

static BOOL
guiAppHasVisibleWindow(void) {
    for (NSWindow* window in [NSApp windows]) {
        if ([window isVisible] && [window canBecomeKeyWindow] && ![window isKindOfClass:[NSPanel class]]) {
            return YES;
        }
    }
    return NO;
}

extern "C" void
yumiGuiWindowClose(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window || guiWindow.closed) {
        return;
    }
    guiWindow.closed = YES;
    guiWindow->inputStreamState->cancel();
    [guiWindow cancelRenderFrame];
    [guiWindow.window close];
    if (guiWindow.panel && [NSApp isActive] && !guiAppHasVisibleWindow()) {
        [NSApp deactivate];
    }
}

extern "C" void
yumiGuiWindowSetAllowsZoom(void* ptr, bool allowsZoom) {
    YuGuiWindowHost* host = getWindow(ptr);
    if (host && !host.closed) {
        host.allowsZoom = allowsZoom;
    }
}

extern "C" void
yumiGuiWindowSetKind(void* ptr, uint32_t kind) {
    YuGuiWindowHost* host = getWindow(ptr);
    if (!host || host.closed) {
        return;
    }
    BOOL panel = kind == 1;
    NSWindow* window = host.window;
    NSWindowStyleMask framed = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
        NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    NSWindowStyleMask style = (window.styleMask & NSWindowStyleMaskFullSizeContentView) |
        (panel ? NSWindowStyleMaskBorderless : framed);
    host.panel = panel;
    if (window.styleMask != style) {
        NSSize contentSize = [host.contentView bounds].size;
        window.styleMask = style;
        [window setContentSize:contentSize];
    }
    window.level = panel ? NSStatusWindowLevel : NSNormalWindowLevel;
    window.collectionBehavior = panel
        ? NSWindowCollectionBehaviorMoveToActiveSpace | NSWindowCollectionBehaviorFullScreenAuxiliary |
            NSWindowCollectionBehaviorTransient | NSWindowCollectionBehaviorIgnoresCycle
        : NSWindowCollectionBehaviorDefault;
    window.movable = !panel;
    window.hasShadow = YES;
    if (panel) {
        window.ignoresMouseEvents = NO;
    }
    [host updateStoplights];
    [host markShadowStale];
}

extern "C" void
yumiGuiWindowRejectDeferredClose(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow && !guiWindow.closed) {
        guiWindow.closeDecisionPending = NO;
    }
}

extern "C" void
yumiGuiWindowSetChrome(
    void* ptr, bool titlebarVisible, bool hasStoplightOffset, double x, double y, double zoom) {
    YuGuiWindowHost* host = getWindow(ptr);
    if (!host || host.closed) return;
    host.hasStoplightOffset = hasStoplightOffset;
    host.stoplightOffset = NSMakePoint(std::isfinite(x) ? x : 0, std::isfinite(y) ? y : 0);
    host.stoplightZoom = std::isfinite(zoom) && zoom > 0 ? zoom : 1;
    if (host.titlebarVisible != titlebarVisible) {
        BOOL wasSuspended = host.stoplightsSuspended;
        host.stoplightsSuspended = YES;
        [host restoreStoplights];
        NSSize contentSize = [host.contentView bounds].size;
        host.titlebarVisible = titlebarVisible;
        NSWindowStyleMask style = host.window.styleMask;
        host.window.styleMask = titlebarVisible
            ? style & ~NSWindowStyleMaskFullSizeContentView
            : style | NSWindowStyleMaskFullSizeContentView;
        host.window.titleVisibility = titlebarVisible ? NSWindowTitleVisible : NSWindowTitleHidden;
        host.window.titlebarAppearsTransparent = !titlebarVisible;
        [host.window setContentSize:contentSize];
        host.stoplightsSuspended = wasSuspended;
    }
    [host updateStoplights];
}

extern "C" bool
yumiGuiWindowRequestDrag(void* ptr, uint64_t token) {
    YuGuiWindowHost* host = getWindow(ptr);
    if (!host || host.closed || !token) return false;
    auto& requests = host->inputStreamState->dragRequests;
    auto found = requests.find(token);
    if (found == requests.end()) return false;
    YuGuiPointerCallbackRecord* record = found->second;
    if (record->dragWindow != host.window || record->dragRequested || !record->canDrag()) return false;
    record->dragRequested = true;
    host.canvasCapturedButtons &= ~uint64_t{1};
    return true;
}

extern "C" void
yumiGuiWindowPerformTitlebarDoubleClick(void* ptr) {
    YuGuiWindowHost* host = getWindow(ptr);
    if (!host || host.closed) return;
    NSString* action = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleActionOnDoubleClick"];
    if ([action isEqualToString:@"None"]) return;
    if ([action isEqualToString:@"Minimize"]) {
        [host.window performMiniaturize:nil];
    } else {
        [host.window performZoom:nil];
    }
}

extern "C" void
yumiGuiWindowSetTitle(void* ptr, const char* title) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow && guiWindow.window && !guiWindow.closed) {
        [guiWindow.window setTitle:stringFromUTF8(title)];
        [guiWindow updateStoplights];
    }
}

extern "C" void
yumiGuiWindowSetContentSize(void* ptr, double width, double height) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow && guiWindow.window && !guiWindow.closed) {
        [guiWindow.window setContentSize:NSMakeSize(width, height)];
    }
}

extern "C" double
yumiGuiWindowGetContentWidth(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow.contentView
        ? static_cast<double>([guiWindow.contentView bounds].size.width)
        : 0.0;
}

extern "C" double
yumiGuiWindowGetContentHeight(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow.contentView
        ? static_cast<double>([guiWindow.contentView bounds].size.height)
        : 0.0;
}

extern "C" bool
yumiGuiWindowDisplayIsWideGamut(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    NSScreen* screen = guiWindow && guiWindow.window ? [guiWindow.window screen] : nil;
    if (!screen) {
        screen = [NSScreen mainScreen];
    }
    return screen && [screen canRepresentDisplayGamut:NSDisplayGamutP3];
}

extern "C" void*
yumiGuiWindowGetDisplayColorSpace(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window) {
        return nullptr;
    }

    NSColorSpace* colorSpace = [guiWindow.window colorSpace];
    if (!colorSpace) {
        colorSpace = [[guiWindow.window screen] colorSpace];
    }
    if (!colorSpace) {
        colorSpace = [[NSScreen mainScreen] colorSpace];
    }
    if (!colorSpace) {
        colorSpace = [NSColorSpace sRGBColorSpace];
    }
    return const_cast<void*>(static_cast<const void*>([colorSpace CGColorSpace]));
}

extern "C" void
yumiGuiWindowSetContentSizeChangedCallback(
    void* ptr,
    YuGuiSizeCallback callback,
    void* context
) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }

    void* newRoot = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = guiWindow.contentSizeContextRoot;
    guiWindow.contentSizeCallback = callback;
    guiWindow.contentSizeContext = context;
    guiWindow.contentSizeContextRoot = newRoot;
    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
}

static CGFloat
guiPrimaryScreenTop(void) {
    NSScreen* primary = [[NSScreen screens] firstObject];
    return primary ? NSMaxY([primary frame]) : 0;
}

static BOOL
guiFrameIsReachable(NSRect frame) {
    NSRect titlebar = NSMakeRect(NSMinX(frame), NSMaxY(frame) - 32, NSWidth(frame), 32);
    for (NSScreen* screen in [NSScreen screens]) {
        NSRect visible = NSIntersectionRect(titlebar, [screen visibleFrame]);
        if (NSWidth(visible) >= 80 && NSHeight(visible) >= 16) {
            return YES;
        }
    }
    return NO;
}

extern "C" double
yumiGuiWindowGetStoplightEdge(void* ptr) {
    YuGuiWindowHost* host = getWindow(ptr);
    return host && !host.closed ? host.stoplightEdge : 0;
}

extern "C" double
yumiGuiWindowGetPosition(void* ptr, uint32_t component) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window) {
        return 0;
    }
    NSRect content = [guiWindow.window contentRectForFrameRect:[guiWindow.window frame]];
    return component == 0
        ? NSMinX(content)
        : guiPrimaryScreenTop() - NSMaxY(content);
}

extern "C" void
yumiGuiWindowSetPosition(void* ptr, double x, double y) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window) {
        return;
    }
    NSWindow* window = guiWindow.window;
    NSRect content = [window contentRectForFrameRect:[window frame]];
    content.origin.x = x;
    content.origin.y = guiPrimaryScreenTop() - y - NSHeight(content);
    NSRect frame = [window frameRectForContentRect:content];
    if (guiFrameIsReachable(frame)) {
        [window setFrameOrigin:frame.origin];
    } else {
        [window center];
    }
}

extern "C" void
yumiGuiWindowSetMovedCallback(
    void* ptr,
    YuGuiActionCallback callback,
    void* context
) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }

    void* newRoot = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = guiWindow.moveContextRoot;
    guiWindow.moveCallback = callback;
    guiWindow.moveContext = context;
    guiWindow.moveContextRoot = newRoot;
    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
}

extern "C" double
yumiGuiWindowGetBackingScaleFactor(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window) {
        return 1.0;
    }
    CGFloat scale = [guiWindow.window backingScaleFactor];
    return scale > 0 ? scale : 1.0;
}

extern "C" void
yumiGuiWindowSetBackingScaleChangedCallback(
    void* ptr,
    YuGuiActionCallback callback,
    void* context
) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }

    void* newRoot = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = guiWindow.backingScaleContextRoot;
    guiWindow.backingScaleCallback = callback;
    guiWindow.backingScaleContext = context;
    guiWindow.backingScaleContextRoot = newRoot;
    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
}

extern "C" void
yumiGuiWindowSetPointerCallback(
    void* ptr,
    YuGuiPointerCallback callback,
    void* context
) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }

    void* newRoot = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = guiWindow.pointerContextRoot;
    guiWindow->inputStreamState->seal();
    guiWindow.pointerCallback = callback;
    guiWindow.pointerContext = context;
    guiWindow.pointerContextRoot = newRoot;
    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
}

extern "C" void
yumiGuiWindowSetTouchCallback(
    void* ptr,
    YuGuiTouchCallback callback,
    void* context
) {}

extern "C" void
yumiGuiWindowSetEventCallback(
    void* ptr,
    YuGuiWindowEventCallback callback,
    void* context
) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }

    void* newRoot = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = guiWindow.eventContextRoot;
    guiWindow->inputStreamState->seal();
    guiWindow.eventCallback = callback;
    guiWindow.eventContext = context;
    guiWindow.eventContextRoot = newRoot;
    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
}

extern "C" void
yumiGuiWindowSetTextActionCallback(
    void* ptr,
    YuGuiWindowTextActionCallback callback,
    void* context
) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }

    void* newRoot = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = guiWindow.textActionContextRoot;
    guiWindow->inputStreamState->seal();
    guiWindow.textActionCallback = callback;
    guiWindow.textActionContext = context;
    guiWindow.textActionContextRoot = newRoot;
    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
}

extern "C" void
yumiGuiWindowSetUndoState(void* ptr, bool canUndo, const char* undoTitle, bool canRedo, const char* redoTitle) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow->undoAvailable = canUndo;
    guiWindow->redoAvailable = canRedo;
    guiWindow->undoTitle = undoTitle ? undoTitle : "";
    guiWindow->redoTitle = redoTitle ? redoTitle : "";
}

extern "C" void
yumiGuiWindowSetTextInputActive(void* ptr, bool active) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || guiWindow.textInputActive == active) {
        return;
    }
    if (!active) {
        [guiWindow discardTextComposition];
    }
    guiWindow.textInputActive = active;
    guiWindow.textInputBefore = @"";
    guiWindow.textInputSelected = @"";
    guiWindow.textInputCaret = NSZeroRect;
}

extern "C" void
yumiGuiWindowSetTextInputContext(
    void* ptr,
    const char* before,
    const char* selected,
    double x,
    double y,
    double width,
    double height
) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    guiWindow.textInputBefore = stringFromUTF8(before);
    guiWindow.textInputSelected = stringFromUTF8(selected);
    guiWindow.textInputCaret = NSMakeRect(x, y, width, height);
    [[guiWindow activeTextInputContext] invalidateCharacterCoordinates];
}

extern "C" void
yumiGuiWindowDiscardTextComposition(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow discardTextComposition];
    }
}

extern "C" bool
yumiGuiWindowDebugTextInputActive(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow.textInputActive;
}

extern "C" void
yumiGuiWindowSetCursorStyle(void* ptr, uint32_t cursorStyle, void* cursorImage, bool captured) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window) {
        return;
    }
    NSCursor* customCursor = (__bridge NSCursor*)cursorImage;
    if (guiWindow.cursorStyle == cursorStyle &&
        guiWindow.canvasView.customCursor == customCursor &&
        guiWindow.cursorCaptured == captured) {
        return;
    }
    BOOL wasCaptured = guiWindow.cursorCaptured;
    guiWindow.canvasView.customCursor = customCursor;
    guiWindow.metalView.customCursor = customCursor;
    guiWindow.cursorCaptured = captured;
    guiWindow.cursorStyle = cursorStyle;
    guiWindow.canvasView.cursorStyle = cursorStyle;
    guiWindow.metalView.cursorStyle = cursorStyle;
    for (YuGuiPopupHost* popup in guiWindow.popupHosts) {
        popup.canvasView.customCursor = customCursor;
        popup.canvasView.cursorStyle = cursorStyle;
        popup.metalView.customCursor = customCursor;
        popup.metalView.cursorStyle = cursorStyle;
    }
    if (captured) {
        if (!wasCaptured) {
            [guiWindow.window disableCursorRects];
        }
        [guiCursorForStyle(cursorStyle, false, customCursor) set];
        return;
    }
    ++guiWindow.cursorRectRefreshCount;
    [guiWindow.window invalidateCursorRectsForView:guiWindow.canvasView];
    [guiWindow.window invalidateCursorRectsForView:guiWindow.metalView];
    for (YuGuiPopupHost* popup in guiWindow.popupHosts) {
        [popup.panel invalidateCursorRectsForView:popup.canvasView];
    }
    if (wasCaptured) {
        [guiWindow.window enableCursorRects];
    }
}

extern "C" uint32_t
yumiGuiWindowDebugCursorStyle(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow.cursorStyle : 0;
}

extern "C" void
yumiGuiWindowDebugSetPointerOverCanvas(void* ptr, bool value) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        guiWindow.pointerOverCanvas = value;
    }
}

extern "C" uint64_t
yumiGuiWindowDebugCursorRectRefreshCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow.cursorRectRefreshCount : 0;
}

extern "C" bool
yumiGuiWindowDebugCursorRectsEnabled(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow.window && [guiWindow.window areCursorRectsEnabled];
}

extern "C" const char*
yumiGuiWindowGetEventText(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow.eventText
        ? [guiWindow.eventText UTF8String]
        : "";
}

static bool
guiWindowControlHasFocus(NSWindow* window, id control) {
    if (!window || !control) {
        return false;
    }
    id firstResponder = [window firstResponder];
    if (firstResponder == control) {
        return true;
    }
    return [firstResponder isKindOfClass:[NSTextView class]] &&
        [(NSTextView*)firstResponder delegate] == control;
}

static NSResponder*
guiResponderWithin(NSView* subtree, NSRange* selection) {
    *selection = NSMakeRange(NSNotFound, 0);
    NSResponder* responder = [[subtree window] firstResponder];
    if ([responder isKindOfClass:[NSTextView class]]) {
        NSTextView* editor = static_cast<NSTextView*>(responder);
        id delegate = [editor delegate];
        if ([delegate isKindOfClass:[NSView class]] && [static_cast<NSView*>(delegate) isDescendantOf:subtree]) {
            *selection = [editor selectedRange];
            return static_cast<NSView*>(delegate);
        }
    }
    if ([responder isKindOfClass:[NSView class]] && [static_cast<NSView*>(responder) isDescendantOf:subtree]) {
        return responder;
    }
    return nil;
}

static void
guiRestoreResponder(NSView* subtree, NSResponder* responder, NSRange selection) {
    NSWindow* window = [subtree window];
    if (!responder || !window || ![window makeFirstResponder:responder]) {
        return;
    }
    if (selection.location == NSNotFound || ![responder isKindOfClass:[NSControl class]]) {
        return;
    }
    NSText* editor = [static_cast<NSControl*>(responder) currentEditor];
    if (editor && NSMaxRange(selection) <= [[editor string] length]) {
        [editor setSelectedRange:selection];
    }
}

extern "C" bool
yumiGuiWindowFocusCanvas(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window || guiWindow.closed) {
        return false;
    }
    NSWindow* keyWindow = [NSApp keyWindow];
    if (keyWindow && keyWindow != guiWindow.window && [guiWindow ownsWindow:keyWindow]) {
        [keyWindow makeFirstResponder:nil];
        [guiWindow.window makeKeyWindow];
    }
    if ([guiWindow.window firstResponder] == guiWindow.canvasView) {
        return true;
    }
    guiWindow.canvasView.takesFocus = YES;
    bool focused = [guiWindow.window makeFirstResponder:guiWindow.canvasView];
    guiWindow.canvasView.takesFocus = NO;
    return focused;
}

extern "C" bool
yumiGuiWindowRequestPeerFocus(void* ptr, void* peer) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window || guiWindow.closed || !peer) {
        return false;
    }

    NSObject* host = (__bridge NSObject*)peer;
    id control = [host valueForKey:@"control"];
    if (![control isKindOfClass:[NSView class]]) {
        return false;
    }
    NSWindow* controlWindow = [static_cast<NSView*>(control) window] ?: guiWindow.window;
    if (guiWindowControlHasFocus(controlWindow, control) && [controlWindow isKeyWindow]) {
        return true;
    }
    if (controlWindow != guiWindow.window && ![controlWindow isKeyWindow] &&
        [guiWindow.window isKeyWindow]) {
        [controlWindow makeKeyWindow];
    }
    return [controlWindow makeFirstResponder:static_cast<NSResponder*>(control)];
}

extern "C" bool
yumiGuiWindowPeerHasFocus(void* ptr, void* peer) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window || !peer) {
        return false;
    }
    NSObject* host = (__bridge NSObject*)peer;
    id control = [host valueForKey:@"control"];
    NSWindow* controlWindow = [control isKindOfClass:[NSView class]]
        ? ([static_cast<NSView*>(control) window] ?: guiWindow.window)
        : guiWindow.window;
    return guiWindowControlHasFocus(controlWindow, control);
}

extern "C" bool
yumiGuiWindowDebugPeerAt(void* ptr, void* peer, double x, double y) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window || !guiWindow.contentView || !peer) {
        return false;
    }
    NSObject* host = (__bridge NSObject*)peer;
    id control = [host valueForKey:@"control"];
    if (![control isKindOfClass:[NSView class]]) {
        return false;
    }
    NSPoint windowPoint = [guiWindow.contentView convertPoint:NSMakePoint(x, y) toView:nil];
    if ([guiWindow isCanvasTargetAtWindowPoint:windowPoint]) {
        return false;
    }
    NSView* frameView = guiWindow.window.contentView.superview ?: guiWindow.window.contentView;
    NSPoint hitPoint = frameView.superview
        ? [frameView.superview convertPoint:windowPoint fromView:nil]
        : windowPoint;
    NSView* hit = [frameView hitTest:hitPoint];
    return hit == control || [hit isDescendantOf:static_cast<NSView*>(control)];
}

static NSView*
guiPeerControl(void* peer) {
    if (!peer) {
        return nil;
    }
    id control = [(__bridge NSObject*)peer valueForKey:@"control"];
    return [control isKindOfClass:[NSView class]] ? static_cast<NSView*>(control) : nil;
}

static YuGuiWindowHost*
guiPeerWindowHost(NSView* control) {
    NSWindow* window = [control window];
    if ([window isKindOfClass:[YuGuiEventWindow class]]) {
        return static_cast<YuGuiEventWindow*>(window).guiWindow;
    }
    if ([window isKindOfClass:[YuGuiPopupPanel class]]) {
        return static_cast<YuGuiPopupPanel*>(window).ownerHost;
    }
    return nil;
}

static void
guiInvalidateCanvasCursorRects(YuGuiWindowHost* guiWindow) {
    if (!guiWindow || !guiWindow.window) {
        return;
    }
    [guiWindow.window invalidateCursorRectsForView:guiWindow.canvasView];
    [guiWindow.window invalidateCursorRectsForView:guiWindow.metalView];
    for (YuGuiPopupHost* popup in guiWindow.popupHosts) {
        [popup.panel invalidateCursorRectsForView:popup.canvasView];
        [popup.panel invalidateCursorRectsForView:popup.metalView];
    }
}

static void
guiInvalidatePeerCursorRects(NSView* control) {
    if (objc_getAssociatedObject(control, &guiPeerOwnsCursorKey)) {
        guiInvalidateCanvasCursorRects(guiPeerWindowHost(control));
    }
}

extern "C" void
yumiGuiWindowAddPeer(void* ptr, void* peer, bool ownsKeys, bool ownsCursor) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    NSView* control = guiPeerControl(peer);
    if (!guiWindow || !guiWindow.contentView || !control) {
        return;
    }
    objc_setAssociatedObject(
        control,
        &guiPeerOwnsKeysKey,
        ownsKeys ? @YES : nil,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
    objc_setAssociatedObject(
        control,
        &guiPeerOwnsCursorKey,
        ownsCursor ? @YES : nil,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC
    );
    if (![control superview]) {
        [guiWindow.contentView addSubview:control positioned:NSWindowAbove relativeTo:guiWindow.metalView];
    }
    guiInvalidatePeerCursorRects(control);
}

extern "C" void
yumiGuiPeerSetContainer(void* ptr, void* peer, void* boxHostPtr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    YuGuiBoxHost* boxHost = getBoxHost(boxHostPtr);
    NSView* control = guiPeerControl(peer);
    if (!control) {
        return;
    }
    NSView* parent = boxHost ? boxHost.contentView : guiWindow.contentView;
    if (!parent || [control superview] == parent) {
        return;
    }
    NSWindow* previousWindow = [control window];
    bool hadFocus = previousWindow && guiWindowControlHasFocus(previousWindow, control);
    [control removeFromSuperview];
    [parent addSubview:control positioned:NSWindowAbove relativeTo:nil];
    if (hadFocus && [control window]) {
        [[control window] makeFirstResponder:control];
    }
    guiInvalidatePeerCursorRects(control);
}

extern "C" void
yumiGuiPeerDetach(void* peer) {
    NSView* control = guiPeerControl(peer);
    if (!control || ![control superview]) {
        return;
    }
    YuGuiWindowHost* guiWindow =
        objc_getAssociatedObject(control, &guiPeerOwnsCursorKey) ? guiPeerWindowHost(control) : nil;
    [control removeFromSuperview];
    guiInvalidateCanvasCursorRects(guiWindow);
}

extern "C" void
yumiGuiPeerSetFrame(
    void* peer,
    double x,
    double y,
    double width,
    double height,
    double a,
    double b,
    double c,
    double d
) {
    NSView* control = guiPeerControl(peer);
    NSView* parent = [control superview];
    if (!control || !parent) {
        return;
    }
    NSRect frame = rectFromTopLeft(parent, x, y, width, height);
    if (guiSetViewGeometry(control, frame, a, b, c, d)) {
        guiInvalidatePeerCursorRects(control);
        [guiPeerWindowHost(control) couplePresentation];
    }
}

extern "C" void
yumiGuiPeerSetVisible(void* peer, bool visible) {
    NSView* control = guiPeerControl(peer);
    if (control && [control isHidden] == visible) {
        [control setHidden:!visible];
        guiInvalidatePeerCursorRects(control);
    }
}

extern "C" void
yumiGuiPeerSetOpacity(void* peer, double opacity) {
    NSView* control = guiPeerControl(peer);
    if (control && [control alphaValue] != opacity) {
        [control setAlphaValue:opacity];
    }
}

extern "C" void
yumiGuiPeerSetPassesScroll(void* peer, bool passesScroll) {
    NSView* control = guiPeerControl(peer);
    if (control) {
        objc_setAssociatedObject(
            control,
            &guiPeerPassesScrollKey,
            passesScroll ? @YES : nil,
            OBJC_ASSOCIATION_RETAIN_NONATOMIC
        );
    }
}

extern "C" void
yumiGuiPeerDispatchKeyEvent(void* peer, void* eventPtr) {
    NSView* control = guiPeerControl(peer);
    NSEvent* event = (__bridge NSEvent*)eventPtr;
    YuGuiWindowHost* guiWindow = control ? guiPeerWindowHost(control) : nil;
    if (guiWindow && event) {
        [guiWindow dispatchPeerKeyEvent:event];
    }
}

extern "C" void
yumiGuiWindowBeginFrame(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow) {
        [guiWindow beginFrameUpdate];
    }
}

static void
guiRecordResizeFrameCommit(YuGuiWindowHost* guiWindow) {
    YuGuiLatestCallbackState* state = &guiWindow->resizeCallbackState;
    if (!state->servicing) {
        return;
    }

    ++state->framesCommitted;
    ++state->framesCommittedThisService;
    state->framesCommittedPerServiceHighWaterMark = std::max(
        state->framesCommittedPerServiceHighWaterMark,
        state->framesCommittedThisService
    );

    NSSize currentSize = [guiWindow.contentView bounds].size;
    if (static_cast<double>(currentSize.width) != state->serviceWidth ||
        static_cast<double>(currentSize.height) != state->serviceHeight) {
        ++state->obsoleteSizesRendered;
    }
}

extern "C" void
yumiGuiWindowCommitFrame(
    void* ptr,
    void* canvas,
    double damageX,
    double damageY,
    double damageWidth,
    double damageHeight) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    if (guiWindow.closed || !guiWindow.canvasView || !canvas) {
        [guiWindow finishFrameUpdate];
        return;
    }

    [guiWindow beginFrameUpdate];
    IOSurfaceRef surface = guiPresentCanvas(
        &guiWindow->canvasPresenter,
        canvas,
        damageX,
        damageY,
        damageWidth,
        damageHeight
    );
    if (!surface) {
        [guiWindow finishFrameUpdate];
        return;
    }

    CGFloat scale = guiWindow.window ? [guiWindow.window backingScaleFactor] : 1.0;
    [guiWindow.canvasView setCanvasSurface:surface scale:scale];
    [guiWindow finishFrameUpdate];
    guiRecordResizeFrameCommit(guiWindow);
}

extern "C" bool
yumiGuiWindowCopyPresentedPixels(void* ptr, void* canvas) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !canvas) {
        return false;
    }
    YuGuiCanvasPresenter* presenter = &guiWindow->canvasPresenter;
    if (presenter->front < 0) {
        return false;
    }
    const int32_t pixelWidth = yumiGraphicsCanvasGetPixelWidth(canvas);
    const int32_t pixelHeight = yumiGraphicsCanvasGetPixelHeight(canvas);
    if (pixelWidth != presenter->width || pixelHeight != presenter->height) {
        return false;
    }
    uint8_t* pixels = static_cast<uint8_t*>(yumiGraphicsCanvasGetPixelData(canvas));
    const size_t destinationBytesPerRow = yumiGraphicsCanvasGetBytesPerRow(canvas);
    IOSurfaceRef surface = presenter->surfaces[static_cast<size_t>(presenter->front)].surface;
    IOSurfaceLock(surface, kIOSurfaceLockReadOnly, nullptr);
    const uint8_t* source = static_cast<const uint8_t*>(IOSurfaceGetBaseAddress(surface));
    const size_t sourceBytesPerRow = IOSurfaceGetBytesPerRow(surface);
    for (int32_t row = 0; row < pixelHeight; ++row) {
        memcpy(
            pixels + static_cast<size_t>(row) * destinationBytesPerRow,
            source + static_cast<size_t>(row) * sourceBytesPerRow,
            static_cast<size_t>(pixelWidth) * 4
        );
    }
    IOSurfaceUnlock(surface, kIOSurfaceLockReadOnly, nullptr);
    yumiGraphicsCanvasPixelsChanged(canvas);
    return true;
}

static bool
guiRenderLayerOffscreen(CALayer* layer, id<MTLDevice> device, int32_t width, int32_t height, CGColorSpaceRef space, std::vector<uint8_t>& pixels) {
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:static_cast<NSUInteger>(width)
                                    height:static_cast<NSUInteger>(height)
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModeShared;
    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    if (!texture) {
        return false;
    }
    id<MTLCommandQueue> queue = [device newCommandQueue];
    NSMutableDictionary* options = [NSMutableDictionary dictionary];
    options[kCARendererMetalCommandQueue] = queue;
    if (space) {
        options[kCARendererColorSpace] = (__bridge id)space;
    }
    CARenderer* renderer = [CARenderer rendererWithMTLTexture:texture options:options];
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    renderer.layer = layer;
    renderer.bounds = CGRectMake(0, 0, width, height);
    [CATransaction commit];
    [CATransaction flush];
    [renderer beginFrameAtTime:CACurrentMediaTime() timeStamp:nullptr];
    [renderer addUpdateRect:renderer.bounds];
    [renderer render];
    [renderer endFrame];
    id<MTLCommandBuffer> fence = [queue commandBuffer];
    [fence commit];
    [fence waitUntilCompleted];
    pixels.assign(static_cast<size_t>(width) * height * 4, 0);
    [texture getBytes:pixels.data()
          bytesPerRow:static_cast<NSUInteger>(width) * 4
           fromRegion:MTLRegionMake2D(0, 0, width, height)
          mipmapLevel:0];
    return true;
}

static int64_t
guiPresentationOrientationCheck(YuGuiWindowHost* guiWindow) {
    if (!guiWindow || !guiWindow.canvasView) {
        return -1;
    }
    YuGuiCanvasPresenter* presenter = &guiWindow->canvasPresenter;
    if (presenter->front < 0) {
        return -2;
    }
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        return -3;
    }
    IOSurfaceRef surface = presenter->surfaces[static_cast<size_t>(presenter->front)].surface;
    const int32_t width = presenter->width;
    const int32_t height = presenter->height;
    IOSurfaceLock(surface, kIOSurfaceLockReadOnly, nullptr);
    CGContextRef copy = CGBitmapContextCreate(
        nullptr,
        static_cast<size_t>(width),
        static_cast<size_t>(height),
        8,
        static_cast<size_t>(width) * 4,
        presenter->colorSpace,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
    );
    if (copy) {
        const uint8_t* source = static_cast<const uint8_t*>(IOSurfaceGetBaseAddress(surface));
        uint8_t* destination = static_cast<uint8_t*>(CGBitmapContextGetData(copy));
        const size_t sourceBytesPerRow = IOSurfaceGetBytesPerRow(surface);
        const size_t destinationBytesPerRow = CGBitmapContextGetBytesPerRow(copy);
        for (int32_t row = 0; row < height; ++row) {
            memcpy(
                destination + static_cast<size_t>(row) * destinationBytesPerRow,
                source + static_cast<size_t>(row) * sourceBytesPerRow,
                static_cast<size_t>(width) * 4
            );
        }
    }
    IOSurfaceUnlock(surface, kIOSurfaceLockReadOnly, nullptr);
    CGImageRef image = copy ? CGBitmapContextCreateImage(copy) : nullptr;
    if (copy) {
        CGContextRelease(copy);
    }
    if (!image) {
        return -4;
    }
    CALayer* canvasLayer = [guiWindow.canvasView layer];
    const CGRect bounds = CGRectMake(0, 0, width, height);
    std::vector<uint8_t> imagePixels;
    std::vector<uint8_t> surfacePixels;
    bool rendered = true;
    for (int pass = 0; pass < 2; ++pass) {
        CALayer* layer = [CALayer layer];
        layer.bounds = bounds;
        layer.anchorPoint = CGPointZero;
        layer.position = CGPointZero;
        layer.geometryFlipped = canvasLayer.geometryFlipped;
        layer.contentsGravity = canvasLayer.contentsGravity;
        layer.contents = pass == 0 ? (__bridge id)image : (__bridge id)surface;
        rendered = rendered && guiRenderLayerOffscreen(
            layer,
            device,
            width,
            height,
            presenter->colorSpace,
            pass == 0 ? imagePixels : surfacePixels
        );
    }
    CGImageRelease(image);
    if (!rendered) {
        return -5;
    }
    int64_t differing = 0;
    int64_t painted = 0;
    for (size_t offset = 0; offset < imagePixels.size(); offset += 4) {
        if (memcmp(imagePixels.data() + offset, surfacePixels.data() + offset, 4) != 0) {
            ++differing;
        }
        if (imagePixels[offset + 3] != 0) {
            ++painted;
        }
    }
    return painted == 0 ? -6 : differing;
}

extern "C" int64_t
yumiGuiWindowPresentationOrientationCheck(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    @autoreleasepool {
        return guiPresentationOrientationCheck(guiWindow);
    }
}

extern "C" int64_t
yumiGuiWindowPresentedSurfaceCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? static_cast<int64_t>(guiWindow->canvasPresenter.surfaces.size()) : 0;
}

extern "C" void
yumiGuiWindowCommitMetalFrame(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    [guiWindow finishFrameUpdate];
    guiRecordResizeFrameCommit(guiWindow);
}

extern "C" bool
yumiGuiWindowSetCanvasLayer(void* ptr, void* layerPointer) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    CALayer* layer = (__bridge CALayer*)layerPointer;
    if (!guiWindow || guiWindow.closed || !guiWindow.canvasView || !guiWindow.metalView || !layer) {
        return false;
    }
    [[guiWindow.canvasView layer] setContents:nil];
    [layer setFrame:[guiWindow.metalView bounds]];
    [layer setContentsGravity:kCAGravityResize];
    [layer setAutoresizingMask:kCALayerWidthSizable | kCALayerHeightSizable];
    if ([guiWindow.metalView layer] != layer) {
        return false;
    }
    [guiWindow.metalView setHidden:NO];
    return true;
}

extern "C" void*
yumiGuiWindowGetMetalLayer(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    CALayer* layer = guiWindow && !guiWindow.closed && guiWindow.metalView
        ? [guiWindow.metalView layer]
        : nil;
    return [layer isKindOfClass:[CAMetalLayer class]] ? (__bridge void*)layer : nullptr;
}

static int32_t
guiPresentationVisibility(NSWindow* window) {
    if (!window || ![window isVisible]) {
        return YuGuiPresentationHidden;
    }
    return ([window occlusionState] & NSWindowOcclusionStateVisible)
        ? YuGuiPresentationUnoccluded
        : YuGuiPresentationOccluded;
}

extern "C" int32_t
yumiGuiWindowPresentationVisibility(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiPresentationVisibility(guiWindow ? guiWindow.window : nil);
}

extern "C" void
yumiGuiWindowSetPresentationPending(void* ptr, bool pending) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow) {
        return;
    }
    BOOL resume = NO;
    @synchronized(guiWindow) {
        guiWindow.presentationPending = pending;
        if (!pending && guiWindow.shadowStale) {
            [guiWindow scheduleShadowSettle];
        }
        if (!pending && guiWindow.presentationHeldFrame) {
            guiWindow.presentationHeldFrame = NO;
            resume = guiWindow.renderScheduled && !guiWindow.closed &&
                !guiWindow.fullScreenTransitioning;
        }
    }
    if (resume) {
        guiFrameSchedulerRequest(guiWindow);
        guiFrameSchedulerScheduleWindowDrain(guiWindow);
    }
}

extern "C" void
yumiGuiWindowResetCanvasLayer(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.metalView) {
        return;
    }
    [guiWindow.metalView setHidden:YES];
}

extern "C" int32_t
yumiGuiWindowGetMaximumFramesPerSecond(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    NSScreen* screen = guiWindow && guiWindow.window
        ? [guiWindow.window screen]
        : [NSScreen mainScreen];
    if (!screen) {
        return 60;
    }
    if (@available(macOS 12.0, *)) {
        return static_cast<int32_t>([screen maximumFramesPerSecond]);
    }
    return 60;
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
    if (!guiWindow || !callback) {
        return;
    }

    [guiWindow requestRenderFrameWithCallback:callback context:context notBefore:notBeforeNanoseconds];
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
    return guiWindow ? guiWindow.lastFrameTargetTimeNanoseconds : 0;
}

extern "C" uint64_t
yumiGuiWindowGetLastFrameRefreshIntervalNanoseconds(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow.lastFrameRefreshIntervalNanoseconds : 0;
}

extern "C" double
yumiGuiWindowGetLastFrameEventTime(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow.lastFrameEventTimeSeconds : 0;
}

static uint64_t
debugCountViewSubtree(NSView* view) {
    uint64_t count = 1;
    for (NSView* subview in [view subviews]) {
        count += debugCountViewSubtree(subview);
    }
    return count;
}

static uint64_t
debugCountLayerSubtree(CALayer* layer) {
    uint64_t count = 1;
    for (CALayer* sublayer in [layer sublayers]) {
        count += debugCountLayerSubtree(sublayer);
    }
    return count;
}

extern "C" uint64_t
yumiGuiWindowDebugViewCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.contentView) {
        return 0;
    }
    return debugCountViewSubtree(guiWindow.contentView);
}

extern "C" uint64_t
yumiGuiWindowDebugLayerCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.contentView) {
        return 0;
    }
    CALayer* layer = [guiWindow.contentView layer];
    return layer ? debugCountLayerSubtree(layer) : 0;
}

extern "C" void
yumiGuiWindowDebugInjectContentSize(void* ptr, double width, double height) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || guiWindow.closed || !guiWindow.contentView) {
        return;
    }
    [guiWindow.contentView setFrameSize:NSMakeSize(width, height)];
    guiReceiveContentSizeNotification(guiWindow);
}

extern "C" bool
yumiGuiWindowDebugInjectTransactionalContentSize(
    void* ptr, double width, double height, uint64_t budgetNanoseconds) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || guiWindow.closed || !guiWindow.contentView) {
        return false;
    }
    [guiWindow.contentView setFrameSize:NSMakeSize(width, height)];
    uint32_t result = guiReceiveContentSizeNotification(
        guiWindow, guiDeadlineAfter(budgetNanoseconds));
    return result == YuMainTaskDeadlineCompleted;
}

extern "C" void
yumiGuiWindowDebugInjectBackingScaleChange(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (guiWindow && !guiWindow.closed) {
        guiReceiveBackingScaleNotification(guiWindow);
    }
}

extern "C" void
yumiGuiWindowDebugSendMouseClick(void* ptr, double x, double y) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || guiWindow.closed || !guiWindow.window) {
        return;
    }
    NSPoint location = [guiWindow.canvasView convertPoint:NSMakePoint(x, y) toView:nil];
    NSEventType types[2] = {NSEventTypeLeftMouseDown, NSEventTypeLeftMouseUp};
    for (NSEventType type : types) {
        NSEvent* event = [NSEvent mouseEventWithType:type
                                            location:location
                                       modifierFlags:0
                                           timestamp:[[NSProcessInfo processInfo] systemUptime]
                                        windowNumber:[guiWindow.window windowNumber]
                                             context:nil
                                         eventNumber:0
                                          clickCount:1
                                            pressure:type == NSEventTypeLeftMouseDown ? 1.0 : 0.0];
        [guiWindow.window sendEvent:event];
    }
}

extern "C" void
yumiGuiWindowDebugPostMouseClick(void* ptr, double x, double y) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || guiWindow.closed || !guiWindow.window) {
        return;
    }
    NSPoint location = [guiWindow.canvasView convertPoint:NSMakePoint(x, y) toView:nil];
    NSEventType types[2] = {NSEventTypeLeftMouseDown, NSEventTypeLeftMouseUp};
    for (NSEventType type : types) {
        NSEvent* event = [NSEvent mouseEventWithType:type
                                            location:location
                                       modifierFlags:0
                                           timestamp:[[NSProcessInfo processInfo] systemUptime]
                                        windowNumber:[guiWindow.window windowNumber]
                                             context:nil
                                         eventNumber:0
                                          clickCount:1
                                            pressure:type == NSEventTypeLeftMouseDown ? 1.0 : 0.0];
        [NSApp postEvent:event atStart:NO];
    }
}

static std::atomic<uint64_t> guiMainLoopProbeSentNanoseconds{0};
static uint64_t guiMainLoopProbeServiced = 0;
static uint64_t guiMainLoopProbeMaxNanoseconds = 0;
static uint64_t guiMainLoopProbeSleeps = 0;

static void
guiMainLoopProbeObserve(CFRunLoopObserverRef, CFRunLoopActivity, void*) {
    ++guiMainLoopProbeSleeps;
}

extern "C" void
yumiGuiAppDebugMainLoopProbeStart(uint64_t intervalNanoseconds) {
    CFRunLoopObserverRef observer = CFRunLoopObserverCreate(
        nullptr, kCFRunLoopBeforeWaiting, true, 0, guiMainLoopProbeObserve, nullptr);
    CFRunLoopAddObserver(CFRunLoopGetMain(), observer, kCFRunLoopCommonModes);
    CFRelease(observer);

    dispatch_source_t timer = dispatch_source_create(
        DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0));
    dispatch_source_set_timer(
        timer, dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(intervalNanoseconds)),
        intervalNanoseconds, 0);
    dispatch_source_set_event_handler(timer, ^{
        uint64_t idle = 0;
        if (!guiMainLoopProbeSentNanoseconds.compare_exchange_strong(
                idle, yumiMonotonicTimeNanoseconds(), std::memory_order_acq_rel)) {
            return;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            uint64_t sent =
                guiMainLoopProbeSentNanoseconds.exchange(0, std::memory_order_acq_rel);
            if (sent) {
                ++guiMainLoopProbeServiced;
                guiMainLoopProbeMaxNanoseconds = std::max(
                    guiMainLoopProbeMaxNanoseconds, yumiMonotonicTimeNanoseconds() - sent);
            }
        });
    });
    dispatch_resume(timer);
}

extern "C" int64_t
yumiGuiAppDebugMainLoopProbeValue(int32_t field) {
    switch (field) {
        case 0:
            return static_cast<int64_t>(guiMainLoopProbeServiced);
        case 1: {
            uint64_t sent = guiMainLoopProbeSentNanoseconds.load(std::memory_order_acquire);
            uint64_t pending = sent ? yumiMonotonicTimeNanoseconds() - sent : 0;
            return static_cast<int64_t>(std::max(guiMainLoopProbeMaxNanoseconds, pending));
        }
        case 2:
            return static_cast<int64_t>(guiMainLoopProbeSleeps);
        case 3:
            guiMainLoopProbeServiced = 0;
            guiMainLoopProbeMaxNanoseconds = 0;
            guiMainLoopProbeSleeps = 0;
            return 0;
        default:
            return -1;
    }
}

extern "C" void
yumiGuiWindowDebugInjectPointerEvent(
    void* ptr,
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
) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || guiWindow.closed) {
        return;
    }
    guiSchedulePointerCallback(
        guiWindow->inputStreamState,
        reinterpret_cast<uintptr_t>(guiWindow),
        guiWindow.pointerCallback,
        guiWindow.pointerContext,
        eventKind,
        pointerId,
        deviceKind,
        button,
        buttons,
        modifiers,
        x,
        y,
        deltaX,
        deltaY,
        orderingTime,
        clickCount,
        isPrecise,
        phase,
        momentumPhase
    );
}

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

extern "C" uint64_t
yumiGuiWindowDebugResizeNotificationCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->resizeCallbackState.notificationsReceived : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugResizeCallbacksServiced(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->resizeCallbackState.callbacksServiced : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugResizeCoalescedCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->resizeCallbackState.notificationsCoalesced : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugResizePendingCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow->resizeCallbackState.pending ? 1 : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugResizePendingHighWaterMark(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->resizeCallbackState.pendingHighWaterMark : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugResizeFrameCommitCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->resizeCallbackState.framesCommitted : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugResizeFramesPerCallbackHighWaterMark(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow
        ? guiWindow->resizeCallbackState.framesCommittedPerServiceHighWaterMark
        : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugResizeObsoleteSizeRenderCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->resizeCallbackState.obsoleteSizesRendered : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugBackingScaleNotificationCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->backingScaleCallbackState.notificationsReceived : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugBackingScaleCallbacksServiced(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->backingScaleCallbackState.callbacksServiced : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugBackingScaleCoalescedCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->backingScaleCallbackState.notificationsCoalesced : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugBackingScalePendingCount(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow && guiWindow->backingScaleCallbackState.pending ? 1 : 0;
}

extern "C" uint64_t
yumiGuiWindowDebugBackingScalePendingHighWaterMark(void* ptr) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    return guiWindow ? guiWindow->backingScaleCallbackState.pendingHighWaterMark : 0;
}

// **********************************************************************************************

extern "C" void*
yumiGuiBoxHostCreate(void* windowPtr) {
    YuGuiWindowHost* guiWindow = getWindow(windowPtr);
    if (!guiWindow || !guiWindow.contentView) {
        return nullptr;
    }

    YuGuiBoxHostView* clipView = [[YuGuiBoxHostView alloc] initWithFrame:NSZeroRect];
    [clipView setWantsLayer:YES];
    [clipView setAccessibilityElement:NO];
    [[clipView layer] setMasksToBounds:YES];
    YuGuiBoxHostView* contentView = [[YuGuiBoxHostView alloc] initWithFrame:NSZeroRect];
    [contentView setWantsLayer:YES];
    [contentView setAccessibilityElement:NO];
    [clipView addSubview:contentView];
    [guiWindow.contentView addSubview:clipView positioned:NSWindowAbove relativeTo:nil];

    YuGuiBoxHost* host = [[YuGuiBoxHost alloc] init];
    host.guiWindow = guiWindow;
    host.clipView = clipView;
    host.contentView = contentView;
    return (__bridge_retained void*)host;
}

extern "C" void
yumiGuiBoxHostRelease(void* ptr) {
    if (ptr) {
        CFAutorelease(ptr);
    }
}

extern "C" void
yumiGuiBoxHostSetParent(void* ptr, void* parentPtr) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    YuGuiBoxHost* parent = getBoxHost(parentPtr);
    if (!host || !host.clipView) {
        return;
    }

    NSView* nextParent = parent ? parent.contentView : host.guiWindow.contentView;
    if (!nextParent || [host.clipView superview] == nextParent) {
        return;
    }
    NSRange selection;
    NSResponder* responder = guiResponderWithin(host.clipView, &selection);
    [host.clipView removeFromSuperview];
    [nextParent addSubview:host.clipView positioned:NSWindowAbove relativeTo:nil];
    guiRestoreResponder(host.clipView, responder, selection);
}

extern "C" void
yumiGuiBoxHostSetFrame(
    void* ptr,
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
) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    if (!host || !host.clipView || !host.contentView) {
        return;
    }

    NSRect contentFrame = [host.contentView frame];
    BOOL wasHidden = [host.clipView isHidden];
    BOOL placed = guiSetViewGeometry(host.clipView, NSMakeRect(x, y, width, height), a, b, c, d);
    [host.contentView setFrame:NSMakeRect(
        contentX - scrollX,
        contentY - scrollY,
        std::max(0.0, contentWidth),
        std::max(0.0, contentHeight)
    )];
    [host.contentView setBoundsOrigin:NSMakePoint(contentX, contentY)];
    [host.clipView setHidden:!visible];
    BOOL moved = placed || !NSEqualRects(contentFrame, [host.contentView frame]) ||
        wasHidden != [host.clipView isHidden];
    if (moved) {
        [host.guiWindow couplePresentation];
    }
    if (moved && guiViewContainsCursorPeer(host.contentView)) {
        guiInvalidateCanvasCursorRects(host.guiWindow);
    }
}

extern "C" void
yumiGuiBoxHostSetCornerRadii(
    void* ptr,
    double left,
    double top,
    double right,
    double bottom
) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    if (!host || !host.clipView || !host.clipView.layer) {
        return;
    }
    if (left <= 0 && top <= 0 && right <= 0 && bottom <= 0) {
        if (host.clipView.layer.mask) {
            host.clipView.layer.mask = nil;
        }
        return;
    }

    NSEdgeInsets radii = NSEdgeInsetsMake(top, left, bottom, right);
    NSRect bounds = host.clipView.bounds;
    if (host.clipView.layer.mask && NSEdgeInsetsEqual(host.maskRadii, radii) &&
        NSEqualRects(host.maskBounds, bounds)) {
        return;
    }
    host.maskRadii = radii;
    host.maskBounds = bounds;

    double width = bounds.size.width;
    double height = bounds.size.height;
    double k = 0.5522847498307936;
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathMoveToPoint(path, nullptr, left, 0);
    CGPathAddLineToPoint(path, nullptr, width - right, 0);
    CGPathAddCurveToPoint(
        path,
        nullptr,
        width - right + right * k,
        0,
        width,
        top - top * k,
        width,
        top
    );
    CGPathAddLineToPoint(path, nullptr, width, height - bottom);
    CGPathAddCurveToPoint(
        path,
        nullptr,
        width,
        height - bottom + bottom * k,
        width - right + right * k,
        height,
        width - right,
        height
    );
    CGPathAddLineToPoint(path, nullptr, left, height);
    CGPathAddCurveToPoint(
        path,
        nullptr,
        left - left * k,
        height,
        0,
        height - bottom + bottom * k,
        0,
        height - bottom
    );
    CGPathAddLineToPoint(path, nullptr, 0, top);
    CGPathAddCurveToPoint(
        path,
        nullptr,
        0,
        top - top * k,
        left - left * k,
        0,
        left,
        0
    );
    CGPathCloseSubpath(path);

    CAShapeLayer* mask = [CAShapeLayer layer];
    mask.frame = bounds;
    mask.geometryFlipped = YES;
    mask.path = path;
    host.clipView.layer.mask = mask;
    CGPathRelease(path);
}

extern "C" bool
yumiGuiBoxHostHasParent(void* ptr, void* parentPtr) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    YuGuiBoxHost* parent = getBoxHost(parentPtr);
    return host && parent && [host.clipView superview] == parent.contentView;
}

extern "C" bool
yumiGuiBoxHostFrameEquals(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    double contentX,
    double contentY,
    double contentWidth,
    double contentHeight
) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    if (!host) {
        return false;
    }
    return NSEqualRects(host.clipView.frame, NSMakeRect(x, y, width, height)) &&
        NSEqualRects(
            host.contentView.frame,
            NSMakeRect(contentX, contentY, contentWidth, contentHeight)
        );
}

extern "C" bool
yumiGuiBoxHostContentOriginEquals(void* ptr, double x, double y) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    return host && host.contentView &&
        NSEqualPoints(host.contentView.bounds.origin, NSMakePoint(x, y));
}

extern "C" bool
yumiGuiBoxHostClips(void* ptr) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    return host && [[host.clipView layer] masksToBounds];
}

extern "C" bool
yumiGuiBoxHostHasRoundedClip(void* ptr) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    return host && host.clipView.layer.mask != nil;
}

extern "C" bool
yumiGuiBoxHostIsAccessibilityTransparent(void* ptr) {
    YuGuiBoxHost* host = getBoxHost(ptr);
    return host && ![host.clipView isAccessibilityElement] &&
        ![host.contentView isAccessibilityElement];
}

// **********************************************************************************************

static YuGuiMaterialHost*
getMaterial(void* ptr) {
    return (__bridge YuGuiMaterialHost*)ptr;
}

static NSVisualEffectMaterial
guiVisualEffectMaterial(uint32_t material) {
    static const NSVisualEffectMaterial materials[] = {
        NSVisualEffectMaterialTitlebar,
        NSVisualEffectMaterialSelection,
        NSVisualEffectMaterialMenu,
        NSVisualEffectMaterialPopover,
        NSVisualEffectMaterialSidebar,
        NSVisualEffectMaterialHeaderView,
        NSVisualEffectMaterialSheet,
        NSVisualEffectMaterialWindowBackground,
        NSVisualEffectMaterialHUDWindow,
        NSVisualEffectMaterialFullScreenUI,
        NSVisualEffectMaterialToolTip,
        NSVisualEffectMaterialContentBackground,
        NSVisualEffectMaterialUnderWindowBackground,
        NSVisualEffectMaterialUnderPageBackground,
    };
    return material < sizeof(materials) / sizeof(materials[0])
        ? materials[material]
        : NSVisualEffectMaterialWindowBackground;
}

extern "C" void*
yumiGuiMaterialCreate(void) {
    YuGuiMaterialView* view = [[YuGuiMaterialView alloc] initWithFrame:NSZeroRect];
    [view setBlendingMode:NSVisualEffectBlendingModeBehindWindow];
    [view setAccessibilityElement:NO];
    YuGuiMaterialHost* host = [[YuGuiMaterialHost alloc] init];
    host.view = view;
    return (__bridge_retained void*)host;
}

extern "C" void
yumiGuiMaterialRelease(void* ptr) {
    if (ptr) {
        CFAutorelease(ptr);
    }
}

extern "C" void
yumiGuiMaterialSetContainer(void* ptr, void* windowPtr, void* boxHostPtr, int32_t depth) {
    YuGuiMaterialHost* host = getMaterial(ptr);
    YuGuiWindowHost* guiWindow = getWindow(windowPtr);
    YuGuiBoxHost* boxHost = getBoxHost(boxHostPtr);
    if (!host || !host.view) {
        return;
    }
    NSView* anchor = boxHost ? static_cast<NSView*>(boxHost.clipView) : static_cast<NSView*>(guiWindow.contentView);
    NSWindow* window = [anchor window];
    NSView* container = [window contentView];
    if (!container) {
        return;
    }
    if ([host.view superview] != container || host.view.depth != depth) {
        [host.view removeFromSuperview];
        host.view.depth = depth;
        NSView* below = nil;
        for (NSView* subview in [container subviews]) {
            if (![subview isKindOfClass:[YuGuiMaterialView class]] ||
                    static_cast<YuGuiMaterialView*>(subview).depth > depth) {
                break;
            }
            below = subview;
        }
        [container addSubview:host.view positioned:below ? NSWindowAbove : NSWindowBelow relativeTo:below];
    }
    NSVisualEffectState state = [window isKindOfClass:[YuGuiPopupPanel class]]
        ? NSVisualEffectStateActive
        : NSVisualEffectStateFollowsWindowActiveState;
    if ([host.view state] != state) {
        [host.view setState:state];
    }
}

extern "C" void
yumiGuiMaterialSetFrame(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    uint32_t material,
    double opacity,
    bool visible
) {
    YuGuiMaterialHost* host = getMaterial(ptr);
    if (!host || !host.view) {
        return;
    }
    NSVisualEffectView* view = host.view;
    NSRect frame = NSMakeRect(x, y, std::max(0.0, width), std::max(0.0, height));
    if (!NSEqualRects([view frame], frame)) {
        [view setFrame:frame];
        [guiPeerWindowHost(view) couplePresentation];
    }
    const NSVisualEffectMaterial next = guiVisualEffectMaterial(material);
    if ([view material] != next) {
        [view setMaterial:next];
    }
    if ([view alphaValue] != static_cast<CGFloat>(opacity)) {
        [view setAlphaValue:static_cast<CGFloat>(opacity)];
    }
    if ([view isHidden] == visible) {
        [view setHidden:!visible];
    }
}

extern "C" void
yumiGuiMaterialSetMask(void* ptr, void* canvas, double width, double height, double cap) {
    YuGuiMaterialHost* host = getMaterial(ptr);
    if (!host || !host.view) {
        return;
    }
    CGImageRef image = canvas ? static_cast<CGImageRef>(yumiGraphicsCanvasCreateCGImage(canvas)) : nullptr;
    if (!image) {
        [host.view setMaskImage:nil];
        return;
    }
    NSImage* mask = [[NSImage alloc] initWithCGImage:image size:NSMakeSize(width, height)];
    if (cap > 0) {
        [mask setCapInsets:NSEdgeInsetsMake(cap, cap, cap, cap)];
        [mask setResizingMode:NSImageResizingModeStretch];
    }
    [host.view setMaskImage:mask];
    CGImageRelease(image);
}

// **********************************************************************************************

static YuGuiPopupHost*
getPopup(void* ptr) {
    return (__bridge YuGuiPopupHost*)ptr;
}

extern "C" void*
yumiGuiPopupCreate(void* windowPtr) {
    YuGuiWindowHost* guiWindow = getWindow(windowPtr);
    if (!guiWindow || !guiWindow.window || guiWindow.closed) {
        return nullptr;
    }

    YuGuiPopupPanel* panel = [[YuGuiPopupPanel alloc]
        initWithContentRect:NSMakeRect(0, 0, 1, 1)
                  styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                    backing:NSBackingStoreBuffered
                      defer:YES
    ];
    [panel setReleasedWhenClosed:NO];
    [panel setOpaque:NO];
    [panel setBackgroundColor:[NSColor clearColor]];
    [panel setHasShadow:NO];
    [panel setHidesOnDeactivate:NO];
    [panel setBecomesKeyOnlyIfNeeded:YES];
    [panel setFloatingPanel:NO];
    [panel setMovable:NO];
    [panel setAcceptsMouseMovedEvents:YES];
    [panel setAnimationBehavior:NSWindowAnimationBehaviorNone];
    [panel setAppearance:guiWindow.window.appearance];
    [panel registerForDraggedTypes:guiDropTypes()];
    panel.ownerHost = guiWindow;

    YuGuiFlippedView* contentView = [[YuGuiFlippedView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
    [contentView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [contentView setWantsLayer:YES];
    [panel setContentView:contentView];

    YuGuiPopupCanvasView* canvasView = [[YuGuiPopupCanvasView alloc] initWithFrame:[contentView bounds]];
    [canvasView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [canvasView setWantsLayer:YES];
    CALayer* canvasLayer = [CALayer layer];
    [canvasView setLayer:canvasLayer];
    [canvasView setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawNever];
    [canvasLayer setContentsGravity:kCAGravityResize];
    canvasView.cursorStyle = guiWindow.cursorStyle;
    canvasView.inputHost = guiWindow;
    [contentView addSubview:canvasView positioned:NSWindowBelow relativeTo:nil];

    YuGuiMetalView* metalView = [[YuGuiMetalView alloc] initWithFrame:[contentView bounds]];
    [metalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [metalView setWantsLayer:YES];
    [metalView setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawNever];
    [[metalView layer] setOpaque:NO];
    [metalView setHidden:YES];
    metalView.cursorStyle = guiWindow.cursorStyle;
    [contentView addSubview:metalView positioned:NSWindowAbove relativeTo:canvasView];

    YuGuiBoxHostView* clipView = [[YuGuiBoxHostView alloc] initWithFrame:[contentView bounds]];
    [clipView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [clipView setWantsLayer:YES];
    [clipView setAccessibilityElement:NO];
    YuGuiBoxHostView* hostContentView = [[YuGuiBoxHostView alloc] initWithFrame:[clipView bounds]];
    [hostContentView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [hostContentView setWantsLayer:YES];
    [hostContentView setAccessibilityElement:NO];
    [clipView addSubview:hostContentView];
    [contentView addSubview:clipView positioned:NSWindowAbove relativeTo:metalView];

    YuGuiBoxHost* boxHost = [[YuGuiBoxHost alloc] init];
    boxHost.guiWindow = guiWindow;
    boxHost.clipView = clipView;
    boxHost.contentView = hostContentView;

    YuGuiPopupHost* popup = [[YuGuiPopupHost alloc] init];
    popup.guiWindow = guiWindow;
    popup.panel = panel;
    popup.canvasView = canvasView;
    popup.metalView = metalView;
    popup.boxHost = boxHost;
    if (!guiWindow.popupHosts) {
        guiWindow.popupHosts = [NSMutableArray array];
    }
    [guiWindow.popupHosts addObject:popup];

    return (__bridge_retained void*)popup;
}

extern "C" void
yumiGuiPopupRelease(void* ptr) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup) {
        return;
    }
    [popup hide];
    [popup.guiWindow.popupHosts removeObject:popup];
    popup.guiWindow = nil;
    CFBridgingRelease(ptr);
}

extern "C" void*
yumiGuiPopupGetMetalLayer(void* ptr) {
    YuGuiPopupHost* popup = getPopup(ptr);
    CALayer* layer = popup && popup.guiWindow && !popup.guiWindow.closed && popup.metalView
        ? [popup.metalView layer]
        : nil;
    return [layer isKindOfClass:[CAMetalLayer class]] ? (__bridge void*)layer : nullptr;
}

extern "C" bool
yumiGuiPopupSetCanvasLayer(void* ptr, void* layerPointer) {
    YuGuiPopupHost* popup = getPopup(ptr);
    CALayer* layer = (__bridge CALayer*)layerPointer;
    if (!popup || !popup.guiWindow || popup.guiWindow.closed || !popup.metalView || !layer ||
        [popup.metalView layer] != layer) {
        return false;
    }
    [[popup.canvasView layer] setContents:nil];
    [layer setFrame:[popup.metalView bounds]];
    [layer setContentsGravity:kCAGravityResize];
    [layer setAutoresizingMask:kCALayerWidthSizable | kCALayerHeightSizable];
    [popup.metalView setHidden:NO];
    return true;
}

extern "C" int32_t
yumiGuiPopupPresentationVisibility(void* ptr) {
    YuGuiPopupHost* popup = getPopup(ptr);
    return guiPresentationVisibility(popup ? popup.panel : nil);
}

extern "C" void
yumiGuiPopupResetCanvasLayer(void* ptr) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (popup && popup.metalView) {
        [popup.metalView setHidden:YES];
    }
}

extern "C" void*
yumiGuiPopupGetWindow(void* ptr) {
    YuGuiPopupHost* popup = getPopup(ptr);
    return popup && popup.guiWindow ? (__bridge void*)popup.guiWindow : nullptr;
}

extern "C" void*
yumiGuiPopupGetHost(void* ptr) {
    YuGuiPopupHost* popup = getPopup(ptr);
    return popup ? (__bridge void*)popup.boxHost : nullptr;
}

extern "C" void
yumiGuiPopupSetFrame(void* ptr, double x, double y, double width, double height) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup || !popup.guiWindow || popup.guiWindow.closed) {
        return;
    }
    NSRect content = NSMakeRect(x, y, std::max(1.0, width), std::max(1.0, height));
    popup.contentFrame = content;
    NSRect windowRect = [popup.guiWindow.contentView convertRect:content toView:nil];
    NSRect screenRect = [popup.guiWindow.window convertRectToScreen:windowRect];
    NSRect current = [popup.panel frame];
    if (!NSEqualRects(current, screenRect)) {
        [popup.panel setFrame:screenRect display:NO];
        if (!NSEqualSizes(current.size, screenRect.size)) {
            [popup markShadowStale];
        }
    }
}

extern "C" void
yumiGuiPopupSetShadow(void* ptr, bool shadow) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup || [popup.panel hasShadow] == shadow) {
        return;
    }
    [popup.panel setHasShadow:shadow];
    [popup markShadowStale];
}

extern "C" void
yumiGuiPopupSetPresentationPending(void* ptr, bool pending) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup) {
        return;
    }
    @synchronized(popup) {
        popup.presentationPending = pending;
        if (!pending && popup.shadowStale) {
            [popup scheduleShadowSettle];
        }
    }
}

extern "C" void
yumiGuiPopupSetShown(void* ptr, bool shown) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup || !popup.guiWindow) {
        return;
    }
    if (!shown || popup.guiWindow.closed) {
        [popup hide];
        return;
    }
    if (popup.shown) {
        return;
    }
    popup.shown = YES;
    [popup.guiWindow.window addChildWindow:popup.panel ordered:NSWindowAbove];
    [popup.panel orderFront:nil];
    [popup markShadowStale];
}

extern "C" bool
yumiGuiPopupIsShown(void* ptr) {
    YuGuiPopupHost* popup = getPopup(ptr);
    return popup && popup.shown && [popup.panel isVisible];
}

extern "C" void
yumiGuiPopupOrderFront(void* ptr) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup || !popup.shown || !popup.guiWindow) {
        return;
    }
    NSWindow* parent = popup.guiWindow.window;
    [parent removeChildWindow:popup.panel];
    [parent addChildWindow:popup.panel ordered:NSWindowAbove];
    [popup.guiWindow.popupHosts removeObject:popup];
    [popup.guiWindow.popupHosts addObject:popup];
}

extern "C" void
yumiGuiPopupSetPassesPointer(void* ptr, bool passesPointer) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup) {
        return;
    }
    popup.passesPointer = passesPointer;
    [popup.panel setIgnoresMouseEvents:passesPointer];
}

extern "C" void
yumiGuiPopupSetConstrained(void* ptr, bool constrained) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup) {
        return;
    }
    popup.panel.isUnconstrained = !constrained;
}

extern "C" void
yumiGuiPopupCommitFrame(void* ptr, void* canvas) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup || !popup.guiWindow || popup.guiWindow.closed || !canvas) {
        return;
    }
    CGImageRef image = static_cast<CGImageRef>(yumiGraphicsCanvasCreateCGImage(canvas));
    if (!image) {
        return;
    }
    CGFloat scale = [popup.guiWindow.window backingScaleFactor];
    [popup.canvasView setCanvasImage:image scale:scale];
    CGImageRelease(image);
}

extern "C" double
yumiGuiPopupGetScreenBound(void* ptr, uint32_t component) {
    YuGuiPopupHost* popup = getPopup(ptr);
    if (!popup || !popup.panel) {
        return 0;
    }
    NSRect frame = [popup.panel frame];
    switch (component) {
        case 0: return frame.origin.x;
        case 1: return frame.origin.y;
        case 2: return frame.size.width;
        default: return frame.size.height;
    }
}

extern "C" double
yumiGuiWindowGetScreenBound(void* ptr, uint32_t component) {
    YuGuiWindowHost* guiWindow = getWindow(ptr);
    if (!guiWindow || !guiWindow.window || !guiWindow.contentView) {
        return 0;
    }
    NSScreen* screen = [guiWindow.window screen] ?: [NSScreen mainScreen];
    if (!screen) {
        return 0;
    }
    NSRect windowRect = [guiWindow.window convertRectFromScreen:[screen visibleFrame]];
    NSRect content = [guiWindow.contentView convertRect:windowRect fromView:nil];
    switch (component) {
        case 0: return content.origin.x;
        case 1: return content.origin.y;
        case 2: return content.size.width;
        default: return content.size.height;
    }
}

// **********************************************************************************************

static NSPasteboard*
guiClipboardPasteboard(const char* name) {
    NSString* pasteboardName = stringFromUTF8(name);
    return pasteboardName.length
        ? [NSPasteboard pasteboardWithName:pasteboardName]
        : [NSPasteboard generalPasteboard];
}

static std::string guiClipboardReadBuffer;

static NSPasteboardItem*
guiClipboardItem(const char* name, uint64_t index) {
    NSArray<NSPasteboardItem*>* items = [guiClipboardPasteboard(name) pasteboardItems];
    return index < [items count] ? items[index] : nil;
}

static const char*
guiClipboardBuffered(NSString* value) {
    if (!value) {
        return nullptr;
    }
    guiClipboardReadBuffer = [value UTF8String] ?: "";
    return guiClipboardReadBuffer.c_str();
}

extern "C" uint64_t
yumiGuiClipboardChangeCount(const char* name) {
    return static_cast<uint64_t>([guiClipboardPasteboard(name) changeCount]);
}

extern "C" uint64_t
yumiGuiClipboardItemCount(const char* name) {
    @autoreleasepool {
        return static_cast<uint64_t>([[guiClipboardPasteboard(name) pasteboardItems] count]);
    }
}

extern "C" const char*
yumiGuiClipboardItemTypes(const char* name, uint64_t index) {
    @autoreleasepool {
        NSPasteboardItem* item = guiClipboardItem(name, index);
        return item ? guiClipboardBuffered([[item types] componentsJoinedByString:@"\n"]) : nullptr;
    }
}

extern "C" const char*
yumiGuiClipboardItemString(const char* name, uint64_t index, const char* type) {
    @autoreleasepool {
        return guiClipboardBuffered([guiClipboardItem(name, index) stringForType:stringFromUTF8(type)]);
    }
}

extern "C" void*
yumiGuiClipboardItemData(const char* name, uint64_t index, const char* type) {
    @autoreleasepool {
        NSData* data = [guiClipboardItem(name, index) dataForType:stringFromUTF8(type)];
        return data ? (void*)CFBridgingRetain(data) : nullptr;
    }
}

extern "C" const char*
yumiGuiClipboardItemPath(const char* name, uint64_t index) {
    @autoreleasepool {
        NSString* value = [guiClipboardItem(name, index) stringForType:NSPasteboardTypeFileURL];
        NSURL* url = value ? [[NSURL URLWithString:value] filePathURL] : nil;
        return [url isFileURL] ? guiClipboardBuffered([url path]) : nullptr;
    }
}

extern "C" uint64_t
yumiGuiClipboardDataLength(void* data) {
    return static_cast<uint64_t>([(__bridge NSData*)data length]);
}

extern "C" void
yumiGuiClipboardDataCopy(void* data, uint8_t* bytes, uint64_t length) {
    NSData* source = (__bridge NSData*)data;
    [source getBytes:bytes length:std::min<uint64_t>(length, [source length])];
}

extern "C" void
yumiGuiClipboardDataRelease(void* data) {
    CFBridgingRelease(data);
}

struct YuGuiClipboardWriter {
    NSMutableArray<NSPasteboardItem*>* items;
};

static NSPasteboardItem*
guiClipboardWriterItem(void* writer) {
    NSMutableArray<NSPasteboardItem*>* items = static_cast<YuGuiClipboardWriter*>(writer)->items;
    if (![items count]) {
        yumiGuiClipboardWriterAddItem(writer);
    }
    return [items lastObject];
}

extern "C" void*
yumiGuiClipboardWriterCreate(void) {
    YuGuiClipboardWriter* writer = new YuGuiClipboardWriter();
    writer->items = [[NSMutableArray alloc] init];
    return writer;
}

extern "C" void
yumiGuiClipboardWriterAddItem(void* writer) {
    NSPasteboardItem* item = [[NSPasteboardItem alloc] init];
    [static_cast<YuGuiClipboardWriter*>(writer)->items addObject:item];
}

extern "C" bool
yumiGuiClipboardWriterSetString(void* writer, const char* type, const char* value) {
    @autoreleasepool {
        return [guiClipboardWriterItem(writer) setString:stringFromUTF8(value) forType:stringFromUTF8(type)];
    }
}

extern "C" bool
yumiGuiClipboardWriterSetData(void* writer, const char* type, const uint8_t* bytes, uint64_t length) {
    @autoreleasepool {
        NSData* data = [NSData dataWithBytes:bytes length:static_cast<NSUInteger>(length)];
        return [guiClipboardWriterItem(writer) setData:data forType:stringFromUTF8(type)];
    }
}

extern "C" bool
yumiGuiClipboardWriterSetPath(void* writer, const char* path) {
    @autoreleasepool {
        NSString* url = [[NSURL fileURLWithPath:stringFromUTF8(path)] absoluteString];
        return [guiClipboardWriterItem(writer) setString:url forType:NSPasteboardTypeFileURL];
    }
}

extern "C" bool
yumiGuiClipboardWriterCommit(void* writer, const char* name) {
    @autoreleasepool {
        NSPasteboard* pasteboard = guiClipboardPasteboard(name);
        [pasteboard clearContents];
        return [pasteboard writeObjects:static_cast<YuGuiClipboardWriter*>(writer)->items];
    }
}

extern "C" void
yumiGuiClipboardWriterRelease(void* writer) {
    YuGuiClipboardWriter* state = static_cast<YuGuiClipboardWriter*>(writer);
    delete state;
}

extern "C" bool
yumiGuiWindowBeginExportDrag(void* ptr, void* writer, void* canvas, double x, double y, double width, double height) {
    YuGuiWindowHost* host = getWindow(ptr);
    if (!host || host.closed || !host.pressEvent || !host.canvasView || !writer || !canvas) return false;
    if (!host->inputStreamState->primaryDown || !([NSEvent pressedMouseButtons] & 1)) return false;
    @autoreleasepool {
        CGImageRef picture = static_cast<CGImageRef>(yumiGraphicsCanvasCreateCGImage(canvas));
        if (!picture) return false;
        NSImage* image = [[NSImage alloc] initWithCGImage:picture size:NSMakeSize(width, height)];
        CGImageRelease(picture);
        NSRect frame = NSMakeRect(x, y, width, height);
        NSMutableArray<NSDraggingItem*>* dragged = [NSMutableArray array];
        for (NSPasteboardItem* item in static_cast<YuGuiClipboardWriter*>(writer)->items) {
            NSDraggingItem* entry = [[NSDraggingItem alloc] initWithPasteboardWriter:item];
            [entry setDraggingFrame:frame contents:[dragged count] ? nil : image];
            [dragged addObject:entry];
        }
        if (![dragged count]) return false;
        NSDraggingSession* session =
            [host.canvasView beginDraggingSessionWithItems:dragged event:host.pressEvent source:host];
        session.animatesToStartingPositionsOnCancelOrFail = YES;
        session.draggingFormation = NSDraggingFormationNone;
    }
    host->inputStreamState->draggedPress = host->inputStreamState->primaryPress;
    host->inputStreamState->primaryDown = false;
    host.canvasCapturedButtons &= ~uint64_t{1};
    return true;
}

extern "C" const char*
yumiGuiWindowDropPasteboardName(void* window) {
    YuGuiWindowHost* guiWindow = getWindow(window);
    return guiWindow ? guiClipboardBuffered([guiWindow.dropPasteboard name]) : nullptr;
}

extern "C" void
yumiGuiWindowSetDropOperation(void* window, uint32_t operation) {
    YuGuiWindowHost* guiWindow = getWindow(window);
    if (guiWindow) {
        guiWindow.dropOperation = operation;
    }
}

extern "C" uint64_t
yumiGuiDragWaitPromisedFiles(uint64_t sequence, uint64_t timeoutMilliseconds) {
    std::shared_ptr<YuGuiPromiseBatch> batch = guiPromiseBatch(sequence);
    if (!batch) {
        return 0;
    }
    std::unique_lock<std::mutex> lock(batch->mutex);
    batch->settled.wait_for(lock, std::chrono::milliseconds(timeoutMilliseconds), [&batch] {
        return batch->pending == 0;
    });
    typedef std::pair<uint64_t, std::string> PromisedFile;
    std::stable_sort(batch->files.begin(), batch->files.end(), [](const PromisedFile& left, const PromisedFile& right) {
        return left.first < right.first;
    });
    batch->paths.clear();
    for (const PromisedFile& file : batch->files) {
        batch->paths.push_back(file.second);
    }
    return static_cast<uint64_t>(batch->paths.size());
}

extern "C" const char*
yumiGuiDragPromisedFile(uint64_t sequence, uint64_t index) {
    std::shared_ptr<YuGuiPromiseBatch> batch = guiPromiseBatch(sequence);
    if (!batch) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(batch->mutex);
    return index < batch->paths.size() ? batch->paths[index].c_str() : nullptr;
}

extern "C" void
yumiGuiDragReleasePromisedFiles(uint64_t sequence) {
    std::lock_guard<std::mutex> lock(guiPromiseBatchesMutex);
    guiPromiseBatches.erase(sequence);
}

extern "C" bool
yumiGuiTypeConforms(const char* type, const char* parent) {
    @autoreleasepool {
        UTType* child = [UTType typeWithIdentifier:stringFromUTF8(type)];
        UTType* ancestor = [UTType typeWithIdentifier:stringFromUTF8(parent)];
        return child && ancestor && [child conformsToType:ancestor];
    }
}

extern "C" const char*
yumiGuiTypeOfPath(const char* path) {
    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:stringFromUTF8(path)];
        UTType* type = nil;
        [url getResourceValue:&type forKey:NSURLContentTypeKey error:nil];
        type = type ?: [UTType typeWithFilenameExtension:[url pathExtension]];
        return guiClipboardBuffered([type identifier]);
    }
}

extern "C" void
yumiGuiClipboardDiscard(const char* name) {
    NSString* pasteboardName = stringFromUTF8(name);
    if (pasteboardName.length) {
        [[NSPasteboard pasteboardWithName:pasteboardName] releaseGlobally];
    }
}

// **********************************************************************************************

extern "C" bool
yumiGuiTrashPath(const char* path) {
    if (!path || !path[0]) {
        return false;
    }

    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:stringFromUTF8(path)];
        NSError* error = nil;
        return [[NSFileManager defaultManager] trashItemAtURL:url resultingItemURL:nil error:&error];
    }
}

extern "C" bool
yumiGuiRevealPath(const char* path) {
    if (!path || !path[0]) {
        return false;
    }

    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:stringFromUTF8(path)];
        if (![url checkResourceIsReachableAndReturnError:nil]) {
            return false;
        }
        [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[url]];
        return true;
    }
}

extern "C" bool
yumiGuiOpenURL(const char* url) {
    if (!url || !url[0]) {
        return false;
    }

    @autoreleasepool {
        NSURL* value = [NSURL URLWithString:stringFromUTF8(url)];
        if (!value || ![[value scheme] length]) {
            return false;
        }
        return [[NSWorkspace sharedWorkspace] openURL:value];
    }
}

// **********************************************************************************************

static YuGuiTextControlField*
guiMakeTextControl(YuGuiTextInputHost* input, NSRect frame, bool secure) {
    YuGuiTextControlField* control = secure
        ? static_cast<YuGuiTextControlField*>([[YuGuiSecureTextField alloc] initWithFrame:frame])
        : static_cast<YuGuiTextControlField*>([[YuGuiTextField alloc] initWithFrame:frame]);
    [control setBezeled:NO];
    [control setBordered:NO];
    [control setDrawsBackground:NO];
    [control setFocusRingType:NSFocusRingTypeNone];
    control.inputHost = input;
    [control setDelegate:input];
    [control setTarget:input];
    [control setAction:@selector(textSubmitted:)];
    return control;
}

extern "C" void*
yumiGuiTextInputCreate(
    void* windowPtr,
    double x,
    double y,
    double width,
    double height,
    const char* text,
    const char* placeholder
) {
    YuGuiWindowHost* guiWindow = getWindow(windowPtr);
    if (!guiWindow || !guiWindow.contentView) {
        return nullptr;
    }

    NSRect frame = rectFromTopLeft(guiWindow.contentView, x, y, width, height);
    YuGuiTextInputHost* input = [[YuGuiTextInputHost alloc] init];
    input.guiWindow = guiWindow;
    YuGuiTextControlField* control = guiMakeTextControl(input, frame, false);
    [control setStringValue:stringFromUTF8(text)];
    [control setPlaceholderString:stringFromUTF8(placeholder)];
    [control setEditable:YES];
    [control setSelectable:YES];
    [control setUsesSingleLineMode:YES];
    [control setLineBreakMode:NSLineBreakByClipping];
    [[control cell] setWraps:NO];
    [[control cell] setScrollable:YES];
    input.control = control;

    [guiWindow.contentView addSubview:control positioned:NSWindowAbove relativeTo:guiWindow.metalView];

    return (__bridge_retained void*)input;
}

extern "C" void
yumiGuiTextInputRelease(void* ptr) {
    if (ptr) {
        YuGuiTextInputHost* owner = getTextInput(ptr);
        static_cast<YuGuiTextControlField*>(owner.control).customCursor = nil;
    }
    if (ptr) {
        CFAutorelease(ptr);
    }
}

extern "C" void
yumiGuiTextInputSetFrame(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    double a,
    double b,
    double c,
    double d
) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }

    NSView* parent = [input.control superview];
    if (!parent) {
        return;
    }

    NSRect frame = rectFromTopLeft(parent, x, y, width, height);
    if (guiSetViewGeometry(input.control, frame, a, b, c, d)) {
        [input applyTextArea];
        [input.guiWindow couplePresentation];
    }
}

extern "C" void
yumiGuiTextInputSetText(void* ptr, const char* text) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (input && input.control) {
        [input.control setStringValue:stringFromUTF8(text)];
        [input applyCellLineMetrics];
    }
}

extern "C" const char*
yumiGuiTextInputGetText(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    return input && input.control
        ? [[input.control stringValue] UTF8String]
        : "";
}

extern "C" void
yumiGuiTextInputSetEventCallback(
    void* ptr,
    YuGuiTextInputEventCallback callback,
    void* context
) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input) {
        return;
    }
    void* newRoot = context ? yumiAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = input.contextRoot;
    input.guiWindow->inputStreamState->seal();
    input.callback = callback;
    input.context = context;
    input.contextRoot = newRoot;
    if (oldRoot) {
        yumiRemoveRoot(oldRoot);
    }
}

extern "C" void
yumiGuiTextInputSetPlaceholder(void* ptr, const char* placeholder) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    NSString* value = stringFromUTF8(placeholder);
    if ([input.placeholderText isEqualToString:value]) {
        return;
    }
    input.placeholderText = value;
    if ([input lineStyle]) {
        [input applyCellLineMetrics];
    } else {
        [input.control setPlaceholderString:value];
    }
}

extern "C" void
yumiGuiTextInputSetTextColor(
    void* ptr,
    double red,
    double green,
    double blue,
    double alpha
) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }

    NSColor* color = [NSColor colorWithSRGBRed:red green:green blue:blue alpha:alpha];
    [input.control setTextColor:color];

    NSText* editor = [input.control currentEditor];
    if ([editor isKindOfClass:[NSTextView class]]) {
        [(NSTextView*)editor setTextColor:color];
    }
    [input applyCellLineMetrics];
}

extern "C" void
yumiGuiTextInputSetFont(void* ptr, const char* family, double size, double weight) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }

    CGFloat pointSize = size > 0 ? size : [NSFont systemFontSize];
    NSFontWeight fontWeight = std::isfinite(weight) ? std::max(-1.0, std::min(weight, 1.0)) : 0.0;
    NSFont* font = nil;
    if (family && family[0]) {
        font = [NSFont fontWithName:stringFromUTF8(family) size:pointSize];
    }
    if (!font) {
        font = [NSFont systemFontOfSize:pointSize weight:fontWeight];
    } else if (fontWeight != 0) {
        NSFontDescriptor* descriptor = [[font fontDescriptor]
            fontDescriptorByAddingAttributes:@{
                NSFontTraitsAttribute: @{NSFontWeightTrait: @(fontWeight)}
            }];
        NSFont* weighted = [NSFont fontWithDescriptor:descriptor size:pointSize];
        if (weighted) {
            font = weighted;
        }
    }
    [input.control setFont:font];
    [input applyCellLineMetrics];
}

extern "C" void
yumiGuiTextInputSetAlignment(void* ptr, int32_t alignment) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    NSTextAlignment value = NSTextAlignmentLeft;
    if (alignment == 1) {
        value = NSTextAlignmentCenter;
    } else if (alignment == 2) {
        value = NSTextAlignmentRight;
    }
    [input.control setAlignment:value];
    [input applyCellLineMetrics];
}

extern "C" void
yumiGuiTextInputSetEnabled(void* ptr, bool enabled) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (input && input.control && [input.control isEnabled] != enabled) {
        [input.control setEnabled:enabled];
    }
}

extern "C" void
yumiGuiTextInputSetEditable(void* ptr, bool editable) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (input && input.control && [input.control isEditable] != editable) {
        [input.control setEditable:editable];
        [input.control setSelectable:YES];
    }
}

extern "C" void
yumiGuiTextInputSetSecure(void* ptr, bool secure) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    YuGuiTextControlField* old = static_cast<YuGuiTextControlField*>(input.control);
    if ([old isKindOfClass:[NSSecureTextField class]] == secure) {
        return;
    }
    NSWindow* window = [old window];
    bool hadFocus = window && guiWindowControlHasFocus(window, old);
    NSString* text = [old stringValue];
    [old abortEditing];
    YuGuiTextControlField* control = guiMakeTextControl(input, [old frame], secure);
    [control setFrameRotation:[old frameRotation]];
    [control setBoundsSize:[old bounds].size];
    [control setStringValue:text];
    [control setPlaceholderString:[old placeholderString]];
    [control setFont:[old font]];
    [control setTextColor:[old textColor]];
    [control setAlignment:[old alignment]];
    [control setEnabled:[old isEnabled]];
    [control setEditable:[old isEditable]];
    [control setSelectable:[old isSelectable]];
    [control setUsesSingleLineMode:[old usesSingleLineMode]];
    [control setLineBreakMode:[old lineBreakMode]];
    [[control cell] setWraps:[[old cell] wraps]];
    [[control cell] setScrollable:[[old cell] isScrollable]];
    [control setHidden:[old isHidden]];
    [control setAlphaValue:[old alphaValue]];
    control.cursorStyle = old.cursorStyle;
    control.customCursor = old.customCursor;
    old.inputHost = nil;
    old.customCursor = nil;
    [old setDelegate:nil];
    [old setTarget:nil];
    if ([old superview]) {
        [[old superview] replaceSubview:old with:control];
    }
    input.control = control;
    [input applyCellLineMetrics];
    if (hadFocus && [control window]) {
        [[control window] makeFirstResponder:control];
    }
}

extern "C" void
yumiGuiTextInputSetMaxLength(void* ptr, uint32_t maxLength) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (input) {
        input.maxLength = maxLength;
    }
}

extern "C" void
yumiGuiTextInputSetVisible(void* ptr, bool visible) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (input && input.control) {
        [input.control setHidden:!visible];
    }
}

extern "C" void
yumiGuiTextInputSetOpacity(void* ptr, double opacity) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (input && input.control) {
        [input.control setAlphaValue:opacity];
    }
}

extern "C" double
yumiGuiTextInputGetOpacity(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    return input && input.control ? [input.control alphaValue] : 1;
}

extern "C" void
yumiGuiTextInputSetCursorStyle(void* ptr, uint32_t cursorStyle, void* cursorImage) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    YuGuiTextControlField* control = static_cast<YuGuiTextControlField*>(input.control);
    NSCursor* customCursor = (__bridge NSCursor*)cursorImage;
    NSRect placed = [control convertRect:[control bounds] toView:nil];
    BOOL hidden = [control isHiddenOrHasHiddenAncestor];
    if (input.cursorControl == control && control.cursorStyle == cursorStyle &&
        control.customCursor == customCursor && NSEqualRects(input.cursorRect, placed) &&
        input.cursorHidden == hidden) {
        return;
    }
    input.cursorControl = control;
    input.cursorRect = placed;
    input.cursorHidden = hidden;
    control.customCursor = customCursor;
    control.cursorStyle = cursorStyle;
    [input.guiWindow.window invalidateCursorRectsForView:control];
    NSPoint windowPoint = [input.guiWindow currentWindowPointerPoint];
    NSPoint contentPoint = [input.guiWindow.contentView convertPoint:windowPoint fromView:nil];
    NSView* hit = [input.guiWindow.contentView hitTest:contentPoint];
    BOOL isHit = hit == control || [hit isDescendantOf:control];
    if (![control isHidden] && isHit) {
        NSCursor* cursor = guiCursorForStyle(cursorStyle, true, control.customCursor);
        [(cursor ?: [NSCursor IBeamCursor]) set];
    }
}

extern "C" uint32_t
yumiGuiTextInputDebugCursorStyle(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return 0;
    }
    return static_cast<YuGuiTextControlField*>(input.control).cursorStyle;
}

extern "C" void
yumiGuiTextInputSetContainer(void* ptr, void* boxHostPtr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    YuGuiBoxHost* boxHost = getBoxHost(boxHostPtr);
    if (!input || !input.control) {
        return;
    }
    NSView* parent = boxHost ? boxHost.contentView : input.guiWindow.contentView;
    if (!parent || [input.control superview] == parent) {
        return;
    }
    NSRange selection;
    NSResponder* responder = guiResponderWithin(input.control, &selection);
    [input.control removeFromSuperview];
    [parent addSubview:input.control positioned:NSWindowAbove relativeTo:nil];
    guiRestoreResponder(input.control, responder, selection);
}

extern "C" bool
yumiGuiTextInputHasContainer(void* ptr, void* boxHostPtr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    YuGuiBoxHost* boxHost = getBoxHost(boxHostPtr);
    return input && boxHost && [input.control superview] == boxHost.contentView;
}

extern "C" double
yumiGuiTextInputGetFittingWidth(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    return input && input.control
        ? static_cast<double>([input.control fittingSize].width)
        : 0.0;
}

extern "C" double
yumiGuiTextInputGetFittingHeight(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    return input && input.control
        ? static_cast<double>([input.control fittingSize].height)
        : 0.0;
}

extern "C" double
yumiGuiTextInputGetFittingHeightForWidth(void* ptr, double width) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return 0.0;
    }
    NSTextField* control = input.control;
    NSText* editor = [control currentEditor];
    NSString* value = editor ? [editor string] : [control stringValue];
    NSTextFieldCell* cell = [[control cell] copy];
    [cell setStringValue:[value length] ? value : @" "];
    NSSize size = [cell cellSizeForBounds:NSMakeRect(0, 0, MAX(width, 1.0), CGFLOAT_MAX)];
    return static_cast<double>(ceil(size.height));
}

extern "C" void
yumiGuiTextInputSetWraps(void* ptr, bool wraps) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    NSTextField* control = input.control;
    [control setUsesSingleLineMode:!wraps];
    [control setLineBreakMode:wraps ? NSLineBreakByWordWrapping : NSLineBreakByClipping];
    [[control cell] setWraps:wraps];
    [[control cell] setScrollable:!wraps];
    [control setClipsToBounds:wraps];
}

extern "C" void
yumiGuiTextInputSetSelection(void* ptr, const char* before, const char* selected) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    input.pendingSelection = NSMakeRange(
        [stringFromUTF8(before) length],
        [stringFromUTF8(selected) length]
    );
    input.hasPendingSelection = YES;
    [input applyPendingSelection];
}

extern "C" void
yumiGuiTextInputSetLineMetrics(void* ptr, double lineHeight, double lineSpacing) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input) {
        return;
    }
    double height = std::isfinite(lineHeight) ? std::max(0.0, lineHeight) : 0.0;
    double spacing = std::isfinite(lineSpacing) ? std::max(0.0, lineSpacing) : 0.0;
    if (input.lineHeight == height && input.lineSpacing == spacing) {
        return;
    }
    input.lineHeight = height;
    input.lineSpacing = spacing;
    [input applyLineMetrics];
}

extern "C" void
yumiGuiTextInputSetTextInsets(void* ptr, double top, double bottom) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    double topValue = std::isfinite(top) ? std::max(0.0, top) : 0.0;
    double bottomValue = std::isfinite(bottom) ? std::max(0.0, bottom) : 0.0;
    if (input.textTopInset == topValue && input.textBottomInset == bottomValue) {
        return;
    }
    input.textTopInset = topValue;
    input.textBottomInset = bottomValue;
    [input applyTextArea];
    [input.control setNeedsDisplay:YES];
}

extern "C" void
yumiGuiTextInputSetHitRect(void* ptr, double x, double y, double width, double height) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    NSRect rect = NSMakeRect(x, y, std::max(0.0, width), std::max(0.0, height));
    if (input.hasHitRect && NSEqualRects(input.hitRect, rect)) {
        return;
    }
    input.hitRect = rect;
    input.hasHitRect = YES;
    [input.guiWindow.window invalidateCursorRectsForView:input.control];
}

extern "C" void
yumiGuiTextInputSetPassedKeys(void* ptr, uint32_t keys) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (input) {
        input.passedKeys = keys;
    }
}

extern "C" void
yumiGuiTextInputPasteText(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    NSText* editor = input ? [input.control currentEditor] : nil;
    if ([editor isKindOfClass:[NSTextView class]]) {
        [static_cast<NSTextView*>(editor) pasteAsPlainText:nil];
    }
}

extern "C" void
yumiGuiTextInputReplaceText(void* ptr, const char* text) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    NSString* value = stringFromUTF8(text);
    NSText* editor = [input.control currentEditor];
    if (editor) {
        [editor setString:value];
        [input applyLineMetrics];
        [editor setSelectedRange:NSMakeRange([value length], 0)];
    } else {
        [input.control setStringValue:value];
    }
}

extern "C" void
yumiGuiTextInputSyncText(void* ptr, const char* text) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    NSString* value = stringFromUTF8(text);
    NSText* editor = [input.control currentEditor];
    if (!editor) {
        [input.control setStringValue:value];
        [input applyCellLineMetrics];
        return;
    }
    NSRange selection = [editor selectedRange];
    [[editor undoManager] removeAllActions];
    [editor setString:value];
    [input applyLineMetrics];
    NSUInteger length = [value length];
    NSUInteger start = MIN(selection.location, length);
    [editor setSelectedRange:NSMakeRange(start, MIN(selection.length, length - start))];
}

extern "C" bool
yumiGuiTextInputCanUndo(void* ptr, bool redoes) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    NSText* editor = input && input.control ? [input.control currentEditor] : nil;
    NSUndoManager* manager = [editor undoManager];
    return manager && (redoes ? [manager canRedo] : [manager canUndo]);
}

extern "C" void
yumiGuiTextInputUndo(void* ptr, bool redoes) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    NSText* editor = input && input.control ? [input.control currentEditor] : nil;
    NSUndoManager* manager = [editor undoManager];
    if (redoes ? [manager canRedo] : [manager canUndo]) {
        if (redoes) {
            [manager redo];
        } else {
            [manager undo];
        }
    }
}

extern "C" void
yumiGuiTextInputMoveCaretToEnd(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control) {
        return;
    }
    NSText* editor = [input.control currentEditor];
    if (editor) {
        [editor setSelectedRange:NSMakeRange([[editor string] length], 0)];
    }
}

static CGFloat
guiFirstBaseline(NSFont* font, NSParagraphStyle* style) {
    NSDictionary* attributes = @{NSFontAttributeName: font, NSParagraphStyleAttributeName: style};
    NSTextStorage* storage = [[NSTextStorage alloc] initWithString:@" " attributes:attributes];
    NSLayoutManager* manager = [[NSLayoutManager alloc] init];
    NSTextContainer* container = [[NSTextContainer alloc] initWithSize:NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX)];
    [manager addTextContainer:container];
    [storage addLayoutManager:manager];
    [manager ensureLayoutForTextContainer:container];
    NSRect fragment = [manager lineFragmentRectForGlyphAtIndex:0 effectiveRange:nullptr];
    CGFloat baseline = fragment.origin.y + [manager locationForGlyphAtIndex:0].y;
    return baseline;
}

static NSPoint
guiTextInputTextOrigin(YuGuiTextInputHost* input) {
    NSTextField* control = input.control;
    NSRect bounds = [control bounds];
    NSFont* font = [control font];
    NSParagraphStyle* style = [input lineStyle];
    if (style && font) {
        NSRect title = [[control cell] titleRectForBounds:bounds];
        CGFloat top = [control isFlipped] ? NSMinY(title) : NSHeight(bounds) - NSMaxY(title);
        if (![input.baselineFont isEqual:font] || ![input.baselineStyle isEqual:style]) {
            input.baselineFont = font;
            input.baselineStyle = style;
            input.baselineOffset = guiFirstBaseline(font, style);
        }
        return NSMakePoint(NSMinX(title) + 2.0, top + input.textTopInset + input.baselineOffset);
    }
    NSText* editor = [control currentEditor];
    if ([editor isKindOfClass:[NSTextView class]]) {
        NSTextView* textView = static_cast<NSTextView*>(editor);
        NSLayoutManager* layoutManager = [textView layoutManager];
        NSTextContainer* container = [textView textContainer];
        [layoutManager ensureLayoutForTextContainer:container];
        NSPoint origin = [textView textContainerOrigin];
        NSPoint point;
        if ([layoutManager numberOfGlyphs] > 0) {
            NSRect fragment = [layoutManager lineFragmentRectForGlyphAtIndex:0 effectiveRange:nullptr];
            NSPoint location = [layoutManager locationForGlyphAtIndex:0];
            point = NSMakePoint(
                origin.x + fragment.origin.x + location.x,
                origin.y + fragment.origin.y + location.y
            );
        } else {
            NSRect fragment = [layoutManager extraLineFragmentRect];
            point = NSMakePoint(
                origin.x + fragment.origin.x + [container lineFragmentPadding],
                origin.y + fragment.origin.y + [layoutManager defaultBaselineOffsetForFont:font]
            );
        }
        NSView* holder = [textView superview];
        if ([holder isKindOfClass:[NSClipView class]]) {
            NSPoint scrolled = [holder bounds].origin;
            point.x += scrolled.x;
            point.y += scrolled.y;
        }
        NSPoint converted = [control convertPoint:point fromView:textView];
        if (![control isFlipped]) {
            converted.y = NSHeight(bounds) - converted.y;
        }
        return converted;
    }
    NSRect title = [[control cell] titleRectForBounds:bounds];
    CGFloat top = [control isFlipped] ? NSMinY(title) : NSHeight(bounds) - NSMaxY(title);
    return NSMakePoint(NSMinX(title) + 2.0, top + input.textTopInset + [font ascender]);
}

extern "C" double
yumiGuiTextInputGetTextOriginX(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    return input && input.control ? static_cast<double>(guiTextInputTextOrigin(input).x) : 0.0;
}

extern "C" double
yumiGuiTextInputGetTextBaseline(void* ptr) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    return input && input.control ? static_cast<double>(guiTextInputTextOrigin(input).y) : 0.0;
}

extern "C" double
yumiGuiTextInputDebugLineMetric(void* ptr, uint32_t metric) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.control || !input.guiWindow.contentView) {
        return -1;
    }
    NSText* editor = [input.control currentEditor];
    if (![editor isKindOfClass:[NSTextView class]]) {
        return -1;
    }
    NSTextView* view = static_cast<NSTextView*>(editor);
    NSLayoutManager* layoutManager = [view layoutManager];
    [layoutManager ensureLayoutForTextContainer:[view textContainer]];
    NSUInteger glyphs = [layoutManager numberOfGlyphs];
    NSUInteger lines = 0;
    NSUInteger secondLine = 0;
    for (NSUInteger index = 0; index < glyphs; lines += 1) {
        NSRange range;
        [layoutManager lineFragmentRectForGlyphAtIndex:index effectiveRange:&range];
        if (lines == 1) {
            secondLine = index;
        }
        index = NSMaxRange(range);
    }
    if (metric == 3) {
        return static_cast<double>(lines);
    }
    NSUInteger glyph = metric == 2 ? secondLine : 0;
    if (!glyphs || (metric == 2 && lines < 2)) {
        return -1;
    }
    NSRect fragment = [layoutManager lineFragmentRectForGlyphAtIndex:glyph effectiveRange:nullptr];
    NSPoint location = [layoutManager locationForGlyphAtIndex:glyph];
    NSPoint origin = [view textContainerOrigin];
    NSPoint point = NSMakePoint(
        origin.x + fragment.origin.x + location.x,
        origin.y + fragment.origin.y + location.y
    );
    NSView* content = input.guiWindow.contentView;
    NSPoint converted = [content convertPoint:point fromView:view];
    if (![content isFlipped]) {
        converted.y = NSHeight([content bounds]) - converted.y;
    }
    return metric == 0 ? converted.x : converted.y;
}



extern "C" void
yumiGuiTextInputDebugInjectEvent(void* ptr, uint32_t eventKind, const char* text) {
    YuGuiTextInputHost* input = getTextInput(ptr);
    if (!input || !input.guiWindow || input.guiWindow.closed) {
        return;
    }
    NSString* value = stringFromUTF8(text);
    guiScheduleTextInputCallback(
        input.guiWindow->inputStreamState,
        reinterpret_cast<uintptr_t>(input),
        input.callback,
        input.context,
        eventKind,
        value,
        NSMakeRange([value length], 0),
        0
    );
}

extern "C" void*
yumiGuiCursorCreate(void* canvas, double hotspotX, double hotspotY) {
    if (!canvas || !std::isfinite(hotspotX) || !std::isfinite(hotspotY)) return nullptr;
    double scale = yumiGraphicsCanvasGetScale(canvas);
    double width = yumiGraphicsCanvasGetPixelWidth(canvas) / scale;
    double height = yumiGraphicsCanvasGetPixelHeight(canvas) / scale;
    if (hotspotX < 0 || hotspotY < 0 || hotspotX >= width || hotspotY >= height) return nullptr;
    CGImageRef snapshot = (CGImageRef)yumiGraphicsCanvasCreateCGImage(canvas);
    if (!snapshot) return nullptr;
    NSImage* image = [[NSImage alloc] initWithCGImage:snapshot size:NSMakeSize(width, height)];
    CGImageRelease(snapshot);
    NSCursor* cursor = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(hotspotX, hotspotY)];
    return (__bridge_retained void*)cursor;
}

extern "C" void
yumiGuiCursorRelease(void* cursor) {
    if (cursor) CFBridgingRelease(cursor);
}

extern "C" double
yumiGuiCursorWidth(void* cursor) {
    return cursor ? [((__bridge NSCursor*)cursor) image].size.width : 0;
}

extern "C" double
yumiGuiCursorHotspotX(void* cursor) {
    return cursor ? [((__bridge NSCursor*)cursor) hotSpot].x : 0;
}

extern "C" double
yumiGuiCursorHotspotY(void* cursor) {
    return cursor ? [((__bridge NSCursor*)cursor) hotSpot].y : 0;
}
