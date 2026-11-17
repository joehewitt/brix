#ifndef YU_APPLE_GUI_H
#define YU_APPLE_GUI_H

#include <stdbool.h>
#include <stdint.h>

#if defined(__clang__)
#define YU_GUI_WITHOUT_BLOCKING __attribute__((annotate("yumi:gc_polling")))
#else
#define YU_GUI_WITHOUT_BLOCKING
#endif

// **********************************************************************************************

typedef void (*YuGuiActionCallback)(void* context);
typedef void (*YuGuiOpenCallback)(void* context, const char* const* paths, int32_t count);
typedef void (*YuGuiSizeCallback)(void* context, double width, double height);
typedef void (*YuGuiTextInputEventCallback)(
    void* context,
    uint32_t eventKind,
    const char* text,
    uint32_t selectionStart,
    uint32_t selectionEnd,
    uint64_t modifiers
);
typedef void (*YuGuiWindowEventCallback)(
    void* context,
    uint32_t eventKind,
    uint32_t key,
    uint32_t nativeKeyCode,
    uint64_t modifiers,
    bool isRepeat,
    const char* text
);
typedef void (*YuGuiWindowTextActionCallback)(
    void* context,
    uint32_t actionKind,
    const char* text,
    uint32_t start,
    uint32_t length,
    bool keyed
);
typedef void (*YuGuiPointerCallback)(
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
    uint64_t windowDragToken
);
typedef void (*YuGuiTouchCallback)(
    void* context,
    uint32_t phase,
    uint64_t touchId,
    double x,
    double y,
    double orderingTime,
    uint32_t tapCount,
    double radius
);

enum YuGuiPresentationVisibility {
    YuGuiPresentationHidden = 0,
    YuGuiPresentationOccluded = 1,
    YuGuiPresentationUnoccluded = 2,
};

// **********************************************************************************************

extern "C" void* yumiGuiAppCreate(void);
extern "C" void yumiGuiAppRelease(void* app);
extern "C" YU_GUI_WITHOUT_BLOCKING void yumiGuiAppRun(void* app);
extern "C" void yumiGuiAppStop(void* app);
extern "C" void yumiGuiAppScheduleStop(void* app, uint64_t delayNanoseconds);
extern "C" void yumiGuiAppSetTitle(void* app, const char* title);
extern "C" void yumiGuiAppSetQuitCallback(void* app, YuGuiActionCallback callback, void* context);
extern "C" void yumiGuiAppSetBackgroundCallback(void* app, YuGuiActionCallback callback, void* context);
extern "C" void yumiGuiAppSetOpenCallback(void* app, YuGuiOpenCallback callback, void* context);
extern "C" void yumiGuiAppSetKeepsDisplayAwake(void* app, bool keeps);
extern "C" uint32_t yumiGuiSystemAppearance(void);
extern "C" void yumiGuiAppSetSystemCallback(void* app, YuGuiActionCallback callback, void* context);
extern "C" uint32_t yumiGuiSystemColor(uint32_t color, uint32_t appearance);
extern "C" uint32_t yumiGuiSystemOptions(void);

// **********************************************************************************************

typedef void (*YuGuiMenuCallback)(void* context, uint32_t eventKind, uint64_t index);
typedef void (*YuGuiStatusItemCallback)(void* context, uint32_t eventKind);

extern "C" void* yumiGuiMenuBarCreate(void);
extern "C" void yumiGuiMenuBarInstall(void* menuBar);
extern "C" void yumiGuiMenuBarUninstall(void* menuBar);
extern "C" void* yumiGuiMenuCreate(const char* title, YuGuiMenuCallback callback, void* context);
extern "C" void yumiGuiMenuRelease(void* menu);
extern "C" void yumiGuiMenuRemoveAllItems(void* menu);
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
);
extern "C" void yumiGuiMenuAddSeparator(void* menu);
extern "C" void yumiGuiMenuPerformRole(uint32_t role);

// **********************************************************************************************

extern "C" void* yumiGuiStatusItemCreate(void);
extern "C" void yumiGuiStatusItemRelease(void* item);
extern "C" void yumiGuiStatusItemSetTitle(void* item, const char* title);
extern "C" void yumiGuiStatusItemSetMenu(void* item, void* menu);
extern "C" void yumiGuiStatusItemSetCallback(void* item, YuGuiStatusItemCallback callback, void* context);
extern "C" void yumiGuiStatusItemSetPanel(void* item, void* window);
extern "C" double yumiGuiStatusItemGetBound(void* item, uint32_t component);
extern "C" double yumiGuiStatusItemGetScreenBound(void* item, uint32_t component);

// **********************************************************************************************

extern "C" void* yumiGuiWindowCreate(const char* title, double width, double height);
extern "C" void yumiGuiWindowRelease(void* window);
extern "C" void* yumiGuiWindowGetNativeWindow(void* window);
extern "C" void yumiGuiWindowShow(void* window);
extern "C" void yumiGuiWindowClose(void* window);
extern "C" void yumiGuiWindowRejectDeferredClose(void* window);
extern "C" void yumiGuiWindowSetTitle(void* window, const char* title);
extern "C" void yumiGuiWindowSetTransparent(void* window, bool windowTransparent, bool contentTransparent);
extern "C" void yumiGuiWindowSetAppearance(void* window, uint32_t appearance);
extern "C" void yumiGuiWindowSetChrome(void* window, bool titlebarVisible, bool hasStoplightOffset, double x, double y, double zoom);
extern "C" double yumiGuiWindowGetStoplightEdge(void* window);
extern "C" void yumiGuiWindowSetAllowsZoom(void* window, bool allowsZoom);
extern "C" void yumiGuiWindowSetKind(void* window, uint32_t kind);
extern "C" bool yumiGuiWindowRequestDrag(void* window, uint64_t token);
extern "C" void yumiGuiWindowPerformTitlebarDoubleClick(void* window);
extern "C" void yumiGuiWindowSetContentSize(void* window, double width, double height);
extern "C" double yumiGuiWindowGetPosition(void* window, uint32_t component);
extern "C" void yumiGuiWindowSetPosition(void* window, double x, double y);
extern "C" void yumiGuiWindowSetMovedCallback(
    void* window,
    YuGuiActionCallback callback,
    void* context
);
extern "C" double yumiGuiWindowGetContentWidth(void* window);
extern "C" double yumiGuiWindowGetContentHeight(void* window);
extern "C" void* yumiGuiWindowGetDisplayColorSpace(void* window);
extern "C" bool yumiGuiWindowDisplayIsWideGamut(void* window);
extern "C" void yumiGuiWindowSetContentSizeChangedCallback(
    void* window,
    YuGuiSizeCallback callback,
    void* context
);
extern "C" double yumiGuiWindowGetBackingScaleFactor(void* window);
extern "C" void yumiGuiWindowSetBackingScaleChangedCallback(
    void* window,
    YuGuiActionCallback callback,
    void* context
);
extern "C" void yumiGuiWindowSetPointerCallback(
    void* window,
    YuGuiPointerCallback callback,
    void* context
);
extern "C" void yumiGuiWindowSetTouchCallback(
    void* window,
    YuGuiTouchCallback callback,
    void* context
);
extern "C" void yumiGuiWindowSetEventCallback(
    void* window,
    YuGuiWindowEventCallback callback,
    void* context
);
extern "C" void yumiGuiWindowSetTextActionCallback(
    void* window,
    YuGuiWindowTextActionCallback callback,
    void* context
);
extern "C" void yumiGuiWindowSetTextInputActive(void* window, bool active);
extern "C" void yumiGuiWindowSetUndoState(void* window, bool canUndo, const char* undoTitle, bool canRedo, const char* redoTitle);
extern "C" void yumiGuiWindowSetTextInputContext(
    void* window,
    const char* before,
    const char* selected,
    double x,
    double y,
    double width,
    double height
);
extern "C" void yumiGuiWindowDiscardTextComposition(void* window);
extern "C" bool yumiGuiWindowDebugTextInputActive(void* window);
extern "C" void yumiGuiWindowSetCursorStyle(void* window, uint32_t cursorStyle, void* cursorImage, bool captured);
extern "C" uint32_t yumiGuiWindowDebugCursorStyle(void* window);
extern "C" void yumiGuiWindowDebugSetPointerOverCanvas(void* window, bool value);
extern "C" uint64_t yumiGuiWindowDebugCursorRectRefreshCount(void* window);
extern "C" bool yumiGuiWindowDebugCursorRectsEnabled(void* window);
extern "C" const char* yumiGuiWindowGetEventText(void* window);
extern "C" bool yumiGuiWindowFocusCanvas(void* window);
extern "C" bool yumiGuiWindowRequestPeerFocus(void* window, void* peer);
extern "C" bool yumiGuiWindowPeerHasFocus(void* window, void* peer);
extern "C" bool yumiGuiWindowDebugPeerAt(void* window, void* peer, double x, double y);
extern "C" void yumiGuiWindowBeginFrame(void* window);
extern "C" void yumiGuiWindowCommitFrame(void* window, void* canvas, double damageX, double damageY, double damageWidth, double damageHeight);
extern "C" bool yumiGuiWindowCopyPresentedPixels(void* window, void* canvas);
extern "C" int64_t yumiGuiWindowPresentedSurfaceCount(void* window);
extern "C" int64_t yumiGuiWindowPresentationOrientationCheck(void* window);
extern "C" void yumiGuiWindowCommitMetalFrame(void* window);
extern "C" void* yumiGuiWindowGetMetalLayer(void* window);
extern "C" bool yumiGuiWindowSetCanvasLayer(void* window, void* layer);
extern "C" void yumiGuiWindowResetCanvasLayer(void* window);
extern "C" int32_t yumiGuiWindowPresentationVisibility(void* window);
extern "C" void yumiGuiWindowSetPresentationPending(void* window, bool pending);
extern "C" int32_t yumiGuiWindowGetMaximumFramesPerSecond(void* window);
extern "C" void yumiGuiWindowAbortFrame(void* window);
extern "C" void yumiGuiWindowRequestFrame(
    void* window,
    YuGuiActionCallback callback,
    void* context,
    uint64_t notBeforeNanoseconds
);
extern "C" void yumiGuiWindowCancelFrame(void* window);
extern "C" uint64_t yumiGuiWindowGetLastFrameTargetTimeNanoseconds(void* window);
extern "C" uint64_t yumiGuiWindowGetLastFrameRefreshIntervalNanoseconds(void* window);
extern "C" double yumiGuiWindowGetLastFrameEventTime(void* window);
extern "C" uint64_t yumiGuiWindowDebugViewCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugLayerCount(void* window);
extern "C" void yumiGuiWindowDebugInjectContentSize(
    void* window,
    double width,
    double height
);
extern "C" bool yumiGuiWindowDebugInjectTransactionalContentSize(
    void* window,
    double width,
    double height,
    uint64_t budgetNanoseconds
);
extern "C" void yumiGuiWindowDebugInjectBackingScaleChange(void* window);
extern "C" void yumiGuiWindowDebugSendMouseClick(void* window, double x, double y);
extern "C" void yumiGuiWindowDebugPostMouseClick(void* window, double x, double y);
extern "C" void yumiGuiAppDebugMainLoopProbeStart(uint64_t intervalNanoseconds);
extern "C" int64_t yumiGuiAppDebugMainLoopProbeValue(int32_t field);
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
);
extern "C" uint64_t yumiGuiWindowDebugInputEventCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugInputCallbacksScheduled(void* window);
extern "C" uint64_t yumiGuiWindowDebugInputCallbacksServiced(void* window);
extern "C" uint64_t yumiGuiWindowDebugInputCoalescedCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugInputBarrierCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugInputPendingCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugInputPendingHighWaterMark(void* window);
extern "C" uint64_t yumiGuiWindowDebugResizeNotificationCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugResizeCallbacksServiced(void* window);
extern "C" uint64_t yumiGuiWindowDebugResizeCoalescedCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugResizePendingCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugResizePendingHighWaterMark(void* window);
extern "C" uint64_t yumiGuiWindowDebugResizeFrameCommitCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugResizeFramesPerCallbackHighWaterMark(void* window);
extern "C" uint64_t yumiGuiWindowDebugResizeObsoleteSizeRenderCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugBackingScaleNotificationCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugBackingScaleCallbacksServiced(void* window);
extern "C" uint64_t yumiGuiWindowDebugBackingScaleCoalescedCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugBackingScalePendingCount(void* window);
extern "C" uint64_t yumiGuiWindowDebugBackingScalePendingHighWaterMark(void* window);

// **********************************************************************************************

extern "C" void* yumiGuiMaterialCreate(void);
extern "C" void yumiGuiMaterialRelease(void* material);
extern "C" void yumiGuiMaterialSetContainer(void* material, void* window, void* boxHost, int32_t depth);
extern "C" void yumiGuiMaterialSetFrame(
    void* material,
    double x,
    double y,
    double width,
    double height,
    uint32_t kind,
    double opacity,
    bool visible
);
extern "C" void yumiGuiMaterialSetMask(void* material, void* canvas, double width, double height, double cap);
extern "C" void* yumiGuiBoxHostCreate(void* window);
extern "C" void yumiGuiBoxHostRelease(void* host);
extern "C" void yumiGuiBoxHostSetParent(void* host, void* parentHost);
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
);
extern "C" void yumiGuiBoxHostSetCornerRadii(
    void* host,
    double left,
    double top,
    double right,
    double bottom
);
extern "C" bool yumiGuiBoxHostHasParent(void* host, void* parentHost);
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
);
extern "C" bool yumiGuiBoxHostContentOriginEquals(void* host, double x, double y);
extern "C" bool yumiGuiBoxHostClips(void* host);
extern "C" bool yumiGuiBoxHostHasRoundedClip(void* host);
extern "C" bool yumiGuiBoxHostIsAccessibilityTransparent(void* host);

// **********************************************************************************************

extern "C" void yumiGuiWindowAddPeer(void* window, void* peer, bool ownsKeys, bool ownsCursor);
extern "C" void yumiGuiPeerSetContainer(void* window, void* peer, void* boxHost);
extern "C" void yumiGuiPeerDetach(void* peer);
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
);
extern "C" void yumiGuiPeerSetVisible(void* peer, bool visible);
extern "C" void yumiGuiPeerSetOpacity(void* peer, double opacity);
extern "C" void yumiGuiPeerSetPassesScroll(void* peer, bool passesScroll);
extern "C" void yumiGuiPeerDispatchKeyEvent(void* peer, void* event);

// **********************************************************************************************

extern "C" void* yumiGuiPopupCreate(void* window);
extern "C" void yumiGuiPopupRelease(void* popup);
extern "C" void* yumiGuiPopupGetHost(void* popup);
extern "C" void* yumiGuiPopupGetMetalLayer(void* popup);
extern "C" bool yumiGuiPopupSetCanvasLayer(void* popup, void* layer);
extern "C" void yumiGuiPopupResetCanvasLayer(void* popup);
extern "C" int32_t yumiGuiPopupPresentationVisibility(void* popup);
extern "C" void* yumiGuiPopupGetWindow(void* popup);
extern "C" void yumiGuiPopupSetFrame(void* popup, double x, double y, double width, double height);
extern "C" void yumiGuiPopupSetShadow(void* popup, bool shadow);
extern "C" void yumiGuiPopupSetPresentationPending(void* popup, bool pending);
extern "C" void yumiGuiPopupSetShown(void* popup, bool shown);
extern "C" bool yumiGuiPopupIsShown(void* popup);
extern "C" void yumiGuiPopupOrderFront(void* popup);
extern "C" void yumiGuiPopupSetPassesPointer(void* popup, bool passesPointer);
extern "C" void yumiGuiPopupSetConstrained(void* popup, bool constrained);
extern "C" void yumiGuiPopupCommitFrame(void* popup, void* canvas);
extern "C" double yumiGuiPopupGetScreenBound(void* popup, uint32_t component);
extern "C" double yumiGuiWindowGetScreenBound(void* window, uint32_t component);

// **********************************************************************************************

extern "C" uint64_t yumiGuiClipboardChangeCount(const char* name);
extern "C" uint64_t yumiGuiClipboardItemCount(const char* name);
extern "C" const char* yumiGuiClipboardItemTypes(const char* name, uint64_t index);
extern "C" const char* yumiGuiClipboardItemString(const char* name, uint64_t index, const char* type);
extern "C" void* yumiGuiClipboardItemData(const char* name, uint64_t index, const char* type);
extern "C" const char* yumiGuiClipboardItemPath(const char* name, uint64_t index);
extern "C" uint64_t yumiGuiClipboardDataLength(void* data);
extern "C" void yumiGuiClipboardDataCopy(void* data, uint8_t* bytes, uint64_t length);
extern "C" void yumiGuiClipboardDataRelease(void* data);
extern "C" void* yumiGuiClipboardWriterCreate(void);
extern "C" void yumiGuiClipboardWriterAddItem(void* writer);
extern "C" bool yumiGuiClipboardWriterSetString(void* writer, const char* type, const char* value);
extern "C" bool yumiGuiClipboardWriterSetData(void* writer, const char* type, const uint8_t* bytes, uint64_t length);
extern "C" bool yumiGuiClipboardWriterSetPath(void* writer, const char* path);
extern "C" bool yumiGuiClipboardWriterCommit(void* writer, const char* name);
extern "C" void yumiGuiClipboardWriterRelease(void* writer);
extern "C" void yumiGuiClipboardDiscard(const char* name);
extern "C" const char* yumiGuiWindowDropPasteboardName(void* window);
extern "C" void yumiGuiWindowSetDropOperation(void* window, uint32_t operation);
extern "C" bool yumiGuiWindowBeginExportDrag(void* window, void* writer, void* canvas, double x, double y, double width, double height);
extern "C" uint64_t yumiGuiDragWaitPromisedFiles(uint64_t sequence, uint64_t timeoutMilliseconds);
extern "C" const char* yumiGuiDragPromisedFile(uint64_t sequence, uint64_t index);
extern "C" void yumiGuiDragReleasePromisedFiles(uint64_t sequence);
extern "C" bool yumiGuiTypeConforms(const char* type, const char* parent);
extern "C" const char* yumiGuiTypeOfPath(const char* path);

// **********************************************************************************************

extern "C" bool yumiGuiTrashPath(const char* path);
extern "C" bool yumiGuiRevealPath(const char* path);
extern "C" bool yumiGuiOpenURL(const char* url);

// **********************************************************************************************

extern "C" void* yumiGuiTextInputCreate(
    void* window,
    double x,
    double y,
    double width,
    double height,
    const char* text,
    const char* placeholder
);
extern "C" void yumiGuiTextInputRelease(void* input);
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
);
extern "C" void yumiGuiTextInputSetText(void* input, const char* text);
extern "C" const char* yumiGuiTextInputGetText(void* input);
extern "C" void yumiGuiTextInputSetEventCallback(
    void* input,
    YuGuiTextInputEventCallback callback,
    void* context
);
extern "C" void yumiGuiTextInputSetPlaceholder(void* input, const char* placeholder);
extern "C" void yumiGuiTextInputSetTextColor(
    void* input,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" void yumiGuiTextInputSetFont(
    void* input,
    const char* family,
    double size,
    double weight
);
extern "C" void yumiGuiTextInputSetAlignment(void* input, int32_t alignment);
extern "C" void yumiGuiTextInputSetEnabled(void* input, bool enabled);
extern "C" void yumiGuiTextInputSetEditable(void* input, bool editable);
extern "C" void yumiGuiTextInputSetSecure(void* input, bool secure);
extern "C" void yumiGuiTextInputSetMaxLength(void* input, uint32_t maxLength);
extern "C" void yumiGuiTextInputSetVisible(void* input, bool visible);
extern "C" void yumiGuiTextInputSetOpacity(void* input, double opacity);
extern "C" double yumiGuiTextInputGetOpacity(void* input);
extern "C" void yumiGuiTextInputSetCursorStyle(void* input, uint32_t cursorStyle, void* cursorImage);
extern "C" uint32_t yumiGuiTextInputDebugCursorStyle(void* input);
extern "C" void yumiGuiTextInputSetContainer(void* input, void* boxHost);
extern "C" bool yumiGuiTextInputHasContainer(void* input, void* boxHost);
extern "C" double yumiGuiTextInputGetFittingWidth(void* input);
extern "C" double yumiGuiTextInputGetFittingHeight(void* input);
extern "C" double yumiGuiTextInputGetFittingHeightForWidth(void* input, double width);
extern "C" void yumiGuiTextInputSetWraps(void* input, bool wraps);
extern "C" void yumiGuiTextInputSetSelection(void* input, const char* before, const char* selected);
extern "C" void yumiGuiTextInputSetPassedKeys(void* input, uint32_t keys);
extern "C" void yumiGuiTextInputSetLineMetrics(void* input, double lineHeight, double lineSpacing);
extern "C" void yumiGuiTextInputSetTextInsets(void* input, double top, double bottom);
extern "C" void yumiGuiTextInputSetHitRect(void* input, double x, double y, double width, double height);
extern "C" double yumiGuiTextInputDebugLineMetric(void* input, uint32_t metric);
extern "C" void yumiGuiTextInputReplaceText(void* input, const char* text);
extern "C" void yumiGuiTextInputPasteText(void* input);
extern "C" void yumiGuiTextInputMoveCaretToEnd(void* input);
extern "C" void yumiGuiTextInputSyncText(void* input, const char* text);
extern "C" bool yumiGuiTextInputCanUndo(void* input, bool redoes);
extern "C" void yumiGuiTextInputUndo(void* input, bool redoes);
extern "C" double yumiGuiTextInputGetTextOriginX(void* input);
extern "C" double yumiGuiTextInputGetTextBaseline(void* input);
extern "C" void yumiGuiTextInputDebugInjectEvent(
    void* input,
    uint32_t eventKind,
    const char* text
);

#undef YU_GUI_WITHOUT_BLOCKING

extern "C" void* yumiGuiCursorCreate(void* canvas, double hotspotX, double hotspotY);
extern "C" void yumiGuiCursorRelease(void* cursor);
extern "C" double yumiGuiCursorWidth(void* cursor);
extern "C" double yumiGuiCursorHotspotX(void* cursor);
extern "C" double yumiGuiCursorHotspotY(void* cursor);

#endif
