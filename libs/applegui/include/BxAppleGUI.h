#ifndef BX_APPLE_GUI_H
#define BX_APPLE_GUI_H

#include <stdbool.h>
#include <stdint.h>

#if defined(__clang__)
#define BX_GUI_WITHOUT_BLOCKING __attribute__((annotate("brix:gc_polling")))
#else
#define BX_GUI_WITHOUT_BLOCKING
#endif

// **********************************************************************************************

typedef void (*BxGuiActionCallback)(void* context);
typedef void (*BxGuiOpenCallback)(void* context, const char* const* paths, int32_t count);
typedef void (*BxGuiSizeCallback)(void* context, double width, double height);
typedef void (*BxGuiTextInputEventCallback)(
    void* context,
    uint32_t eventKind,
    const char* text,
    uint32_t selectionStart,
    uint32_t selectionEnd,
    uint64_t modifiers
);
typedef void (*BxGuiWindowEventCallback)(
    void* context,
    uint32_t eventKind,
    uint32_t key,
    uint32_t nativeKeyCode,
    uint64_t modifiers,
    bool isRepeat,
    const char* text
);
typedef void (*BxGuiWindowTextActionCallback)(
    void* context,
    uint32_t actionKind,
    const char* text,
    uint32_t start,
    uint32_t length,
    bool keyed
);
typedef void (*BxGuiPointerCallback)(
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
typedef void (*BxGuiTouchCallback)(
    void* context,
    uint32_t phase,
    uint64_t touchId,
    double x,
    double y,
    double orderingTime,
    uint32_t tapCount,
    double radius
);

enum BxGuiPresentationVisibility {
    BxGuiPresentationHidden = 0,
    BxGuiPresentationOccluded = 1,
    BxGuiPresentationUnoccluded = 2,
};

// **********************************************************************************************

extern "C" void* brixGuiAppCreate(void);
extern "C" void brixGuiAppRelease(void* app);
extern "C" BX_GUI_WITHOUT_BLOCKING void brixGuiAppRun(void* app);
extern "C" void brixGuiAppStop(void* app);
extern "C" void brixGuiAppScheduleStop(void* app, uint64_t delayNanoseconds);
extern "C" void brixGuiAppSetTitle(void* app, const char* title);
extern "C" void brixGuiAppSetQuitCallback(void* app, BxGuiActionCallback callback, void* context);
extern "C" void brixGuiAppSetBackgroundCallback(void* app, BxGuiActionCallback callback, void* context);
extern "C" void brixGuiAppSetOpenCallback(void* app, BxGuiOpenCallback callback, void* context);
extern "C" void brixGuiAppSetKeepsDisplayAwake(void* app, bool keeps);
extern "C" uint32_t brixGuiSystemAppearance(void);
extern "C" void brixGuiAppSetSystemCallback(void* app, BxGuiActionCallback callback, void* context);
extern "C" uint32_t brixGuiSystemColor(uint32_t color, uint32_t appearance);
extern "C" uint32_t brixGuiSystemOptions(void);

// **********************************************************************************************

typedef void (*BxGuiMenuCallback)(void* context, uint32_t eventKind, uint64_t index);
typedef void (*BxGuiStatusItemCallback)(void* context, uint32_t eventKind);

extern "C" void* brixGuiMenuBarCreate(void);
extern "C" void brixGuiMenuBarInstall(void* menuBar);
extern "C" void brixGuiMenuBarUninstall(void* menuBar);
extern "C" void* brixGuiMenuCreate(const char* title, BxGuiMenuCallback callback, void* context);
extern "C" void brixGuiMenuRelease(void* menu);
extern "C" void brixGuiMenuRemoveAllItems(void* menu);
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
);
extern "C" void brixGuiMenuAddSeparator(void* menu);
extern "C" void brixGuiMenuPerformRole(uint32_t role);

// **********************************************************************************************

extern "C" void* brixGuiStatusItemCreate(void);
extern "C" void brixGuiStatusItemRelease(void* item);
extern "C" void brixGuiStatusItemSetTitle(void* item, const char* title);
extern "C" void brixGuiStatusItemSetAutosaveName(void* item, const char* name);
extern "C" void brixGuiStatusItemSetImage(void* item, void* canvas, double width, double height, bool isTemplate);
extern "C" void brixGuiStatusItemSetMenu(void* item, void* menu);
extern "C" void brixGuiStatusItemSetContextMenu(void* item, void* menu);
extern "C" void brixGuiStatusItemSetCallback(void* item, BxGuiStatusItemCallback callback, void* context);
extern "C" void brixGuiStatusItemSetPanel(void* item, void* window);
extern "C" void* brixGuiStatusItemCreatePopover(void* item, double width, double height);
extern "C" void brixGuiStatusItemShowPopover(void* item);
extern "C" void brixGuiStatusItemClosePopover(void* item);
extern "C" double brixGuiStatusItemGetBound(void* item, uint32_t component);
extern "C" double brixGuiStatusItemGetScreenBound(void* item, uint32_t component);

// **********************************************************************************************

extern "C" void* brixGuiWindowCreate(const char* title, double width, double height);
extern "C" void* brixGuiWindowCreateWithKind(const char* title, double width, double height, uint32_t kind);
extern "C" void brixGuiWindowRelease(void* window);
extern "C" void* brixGuiWindowGetNativeWindow(void* window);
extern "C" void brixGuiWindowShow(void* window);
extern "C" void brixGuiWindowHide(void* window);
extern "C" bool brixGuiWindowIsVisible(void* window);
extern "C" void brixGuiWindowClose(void* window);
extern "C" void brixGuiWindowRejectDeferredClose(void* window);
extern "C" void brixGuiWindowSetTitle(void* window, const char* title);
extern "C" void brixGuiWindowSetTransparent(void* window, bool windowTransparent, bool contentTransparent);
extern "C" void brixGuiWindowSetAppearance(void* window, uint32_t appearance);
extern "C" void brixGuiWindowSetChrome(void* window, bool titlebarVisible, bool hasStoplightOffset, double x, double y, double zoom);
extern "C" double brixGuiWindowGetStoplightEdge(void* window);
extern "C" void brixGuiWindowSetAllowsZoom(void* window, bool allowsZoom);
extern "C" void brixGuiWindowSetKind(void* window, uint32_t kind);
extern "C" void brixGuiWindowSetHasShadow(void* window, bool hasShadow);
extern "C" bool brixGuiWindowRequestDrag(void* window, uint64_t token);
extern "C" void brixGuiWindowPerformTitlebarDoubleClick(void* window);
extern "C" void brixGuiWindowSetContentSize(void* window, double width, double height);
extern "C" double brixGuiWindowGetPosition(void* window, uint32_t component);
extern "C" void brixGuiWindowSetPosition(void* window, double x, double y);
extern "C" void brixGuiWindowSetMovedCallback(
    void* window,
    BxGuiActionCallback callback,
    void* context
);
extern "C" double brixGuiWindowGetContentWidth(void* window);
extern "C" double brixGuiWindowGetContentHeight(void* window);
extern "C" void* brixGuiWindowGetDisplayColorSpace(void* window);
extern "C" bool brixGuiWindowDisplayIsWideGamut(void* window);
extern "C" void brixGuiWindowSetContentSizeChangedCallback(
    void* window,
    BxGuiSizeCallback callback,
    void* context
);
extern "C" double brixGuiWindowGetBackingScaleFactor(void* window);
extern "C" void brixGuiWindowSetBackingScaleChangedCallback(
    void* window,
    BxGuiActionCallback callback,
    void* context
);
extern "C" void brixGuiWindowSetPointerCallback(
    void* window,
    BxGuiPointerCallback callback,
    void* context
);
extern "C" void brixGuiWindowSetTouchCallback(
    void* window,
    BxGuiTouchCallback callback,
    void* context
);
extern "C" void brixGuiWindowSetEventCallback(
    void* window,
    BxGuiWindowEventCallback callback,
    void* context
);
extern "C" void brixGuiWindowSetTextActionCallback(
    void* window,
    BxGuiWindowTextActionCallback callback,
    void* context
);
extern "C" void brixGuiWindowSetTextInputActive(void* window, bool active);
extern "C" void brixGuiWindowSetUndoState(void* window, bool canUndo, const char* undoTitle, bool canRedo, const char* redoTitle);
extern "C" void brixGuiWindowSetTextInputContext(
    void* window,
    const char* before,
    const char* selected,
    double x,
    double y,
    double width,
    double height
);
extern "C" void brixGuiWindowDiscardTextComposition(void* window);
extern "C" bool brixGuiWindowDebugTextInputActive(void* window);
extern "C" void brixGuiWindowSetCursorStyle(void* window, uint32_t cursorStyle, void* cursorImage, bool captured);
extern "C" uint32_t brixGuiWindowDebugCursorStyle(void* window);
extern "C" void brixGuiWindowDebugSetPointerOverCanvas(void* window, bool value);
extern "C" uint64_t brixGuiWindowDebugCursorRectRefreshCount(void* window);
extern "C" bool brixGuiWindowDebugCursorRectsEnabled(void* window);
extern "C" const char* brixGuiWindowGetEventText(void* window);
extern "C" bool brixGuiWindowFocusCanvas(void* window);
extern "C" bool brixGuiWindowRequestPeerFocus(void* window, void* peer);
extern "C" bool brixGuiWindowPeerHasFocus(void* window, void* peer);
extern "C" bool brixGuiWindowDebugPeerAt(void* window, void* peer, double x, double y);
extern "C" void brixGuiWindowBeginFrame(void* window);
extern "C" void brixGuiWindowCommitFrame(void* window, void* canvas, double damageX, double damageY, double damageWidth, double damageHeight);
extern "C" bool brixGuiWindowCopyPresentedPixels(void* window, void* canvas);
extern "C" int64_t brixGuiWindowPresentedSurfaceCount(void* window);
extern "C" int64_t brixGuiWindowPresentationOrientationCheck(void* window);
extern "C" void brixGuiWindowCommitMetalFrame(void* window);
extern "C" void* brixGuiWindowGetMetalLayer(void* window);
extern "C" bool brixGuiWindowSetCanvasLayer(void* window, void* layer);
extern "C" void brixGuiWindowResetCanvasLayer(void* window);
extern "C" int32_t brixGuiWindowPresentationVisibility(void* window);
extern "C" void brixGuiWindowSetPresentationPending(void* window, bool pending);
extern "C" int32_t brixGuiWindowGetMaximumFramesPerSecond(void* window);
extern "C" void brixGuiWindowAbortFrame(void* window);
extern "C" void brixGuiWindowRequestFrame(
    void* window,
    BxGuiActionCallback callback,
    void* context,
    uint64_t notBeforeNanoseconds
);
extern "C" void brixGuiWindowCancelFrame(void* window);
extern "C" uint64_t brixGuiWindowGetLastFrameTargetTimeNanoseconds(void* window);
extern "C" uint64_t brixGuiWindowGetLastFrameRefreshIntervalNanoseconds(void* window);
extern "C" double brixGuiWindowGetLastFrameEventTime(void* window);
extern "C" double brixGuiEventClockSeconds(void);
extern "C" uint64_t brixGuiWindowDebugViewCount(void* window);
extern "C" uint64_t brixGuiWindowDebugLayerCount(void* window);
extern "C" void brixGuiWindowDebugInjectContentSize(
    void* window,
    double width,
    double height
);
extern "C" bool brixGuiWindowDebugInjectTransactionalContentSize(
    void* window,
    double width,
    double height,
    uint64_t budgetNanoseconds
);
extern "C" void brixGuiWindowDebugInjectBackingScaleChange(void* window);
extern "C" void brixGuiWindowDebugSendMouseClick(void* window, double x, double y);
extern "C" void brixGuiWindowDebugPostMouseClick(void* window, double x, double y);
extern "C" void brixGuiAppDebugMainLoopProbeStart(uint64_t intervalNanoseconds);
extern "C" int64_t brixGuiAppDebugMainLoopProbeValue(int32_t field);
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
);
extern "C" uint64_t brixGuiWindowDebugInputEventCount(void* window);
extern "C" uint64_t brixGuiWindowDebugInputCallbacksScheduled(void* window);
extern "C" uint64_t brixGuiWindowDebugInputCallbacksServiced(void* window);
extern "C" uint64_t brixGuiWindowDebugInputCoalescedCount(void* window);
extern "C" uint64_t brixGuiWindowDebugInputBarrierCount(void* window);
extern "C" uint64_t brixGuiWindowDebugInputPendingCount(void* window);
extern "C" uint64_t brixGuiWindowDebugInputPendingHighWaterMark(void* window);
extern "C" uint64_t brixGuiWindowDebugResizeNotificationCount(void* window);
extern "C" uint64_t brixGuiWindowDebugResizeCallbacksServiced(void* window);
extern "C" uint64_t brixGuiWindowDebugResizeCoalescedCount(void* window);
extern "C" uint64_t brixGuiWindowDebugResizePendingCount(void* window);
extern "C" uint64_t brixGuiWindowDebugResizePendingHighWaterMark(void* window);
extern "C" uint64_t brixGuiWindowDebugResizeFrameCommitCount(void* window);
extern "C" uint64_t brixGuiWindowDebugResizeFramesPerCallbackHighWaterMark(void* window);
extern "C" uint64_t brixGuiWindowDebugResizeObsoleteSizeRenderCount(void* window);
extern "C" uint64_t brixGuiWindowDebugBackingScaleNotificationCount(void* window);
extern "C" uint64_t brixGuiWindowDebugBackingScaleCallbacksServiced(void* window);
extern "C" uint64_t brixGuiWindowDebugBackingScaleCoalescedCount(void* window);
extern "C" uint64_t brixGuiWindowDebugBackingScalePendingCount(void* window);
extern "C" uint64_t brixGuiWindowDebugBackingScalePendingHighWaterMark(void* window);

// **********************************************************************************************

extern "C" void* brixGuiMaterialCreate(void);
extern "C" void brixGuiMaterialRelease(void* material);
extern "C" void brixGuiMaterialSetContainer(void* material, void* window, void* boxHost, int32_t depth);
extern "C" void brixGuiMaterialSetFrame(
    void* material,
    double x,
    double y,
    double width,
    double height,
    uint32_t kind,
    double opacity,
    bool visible
);
extern "C" void brixGuiMaterialSetMask(void* material, void* canvas, double width, double height, double cap);
extern "C" void brixGuiMaterialSetCornerRadius(void* material, double radius);
extern "C" void* brixGuiBoxHostCreate(void* window);
extern "C" void brixGuiBoxHostRelease(void* host);
extern "C" void brixGuiBoxHostSetParent(void* host, void* parentHost);
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
);
extern "C" void brixGuiBoxHostSetCornerRadii(
    void* host,
    double left,
    double top,
    double right,
    double bottom
);
extern "C" bool brixGuiBoxHostHasParent(void* host, void* parentHost);
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
);
extern "C" bool brixGuiBoxHostContentOriginEquals(void* host, double x, double y);
extern "C" bool brixGuiBoxHostClips(void* host);
extern "C" bool brixGuiBoxHostHasRoundedClip(void* host);
extern "C" bool brixGuiBoxHostIsAccessibilityTransparent(void* host);

// **********************************************************************************************

extern "C" void brixGuiWindowAddPeer(void* window, void* peer, bool ownsKeys, bool ownsCursor);
extern "C" void brixGuiPeerSetContainer(void* window, void* peer, void* boxHost);
extern "C" void brixGuiPeerDetach(void* peer);
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
);
extern "C" void brixGuiPeerSetVisible(void* peer, bool visible);
extern "C" void brixGuiPeerSetOpacity(void* peer, double opacity);
extern "C" void brixGuiPeerSetPassesScroll(void* peer, bool passesScroll);
extern "C" void brixGuiPeerDispatchKeyEvent(void* peer, void* event);

// **********************************************************************************************

extern "C" void* brixGuiPopupCreate(void* window);
extern "C" void brixGuiPopupRelease(void* popup);
extern "C" void* brixGuiPopupGetHost(void* popup);
extern "C" void* brixGuiPopupGetMetalLayer(void* popup);
extern "C" bool brixGuiPopupSetCanvasLayer(void* popup, void* layer);
extern "C" void brixGuiPopupResetCanvasLayer(void* popup);
extern "C" int32_t brixGuiPopupPresentationVisibility(void* popup);
extern "C" void* brixGuiPopupGetWindow(void* popup);
extern "C" void brixGuiPopupSetFrame(void* popup, double x, double y, double width, double height);
extern "C" void brixGuiPopupSetShadow(void* popup, bool shadow);
extern "C" void brixGuiPopupSetPresentationPending(void* popup, bool pending);
extern "C" void brixGuiPopupSetShown(void* popup, bool shown);
extern "C" bool brixGuiPopupIsShown(void* popup);
extern "C" void brixGuiPopupOrderFront(void* popup);
extern "C" void brixGuiPopupSetPassesPointer(void* popup, bool passesPointer);
extern "C" void brixGuiPopupSetConstrained(void* popup, bool constrained);
extern "C" void brixGuiPopupCommitFrame(void* popup, void* canvas);
extern "C" double brixGuiPopupGetScreenBound(void* popup, uint32_t component);
extern "C" double brixGuiWindowGetScreenBound(void* window, uint32_t component);
extern "C" double brixGuiMouseScreenBound(uint32_t component);

// **********************************************************************************************

extern "C" uint64_t brixGuiClipboardChangeCount(const char* name);
extern "C" uint64_t brixGuiClipboardItemCount(const char* name);
extern "C" const char* brixGuiClipboardItemTypes(const char* name, uint64_t index);
extern "C" const char* brixGuiClipboardItemString(const char* name, uint64_t index, const char* type);
extern "C" void* brixGuiClipboardItemData(const char* name, uint64_t index, const char* type);
extern "C" const char* brixGuiClipboardItemPath(const char* name, uint64_t index);
extern "C" uint64_t brixGuiClipboardDataLength(void* data);
extern "C" void brixGuiClipboardDataCopy(void* data, uint8_t* bytes, uint64_t length);
extern "C" void brixGuiClipboardDataRelease(void* data);
extern "C" void* brixGuiClipboardWriterCreate(void);
extern "C" void brixGuiClipboardWriterAddItem(void* writer);
extern "C" bool brixGuiClipboardWriterSetString(void* writer, const char* type, const char* value);
extern "C" bool brixGuiClipboardWriterSetData(void* writer, const char* type, const uint8_t* bytes, uint64_t length);
extern "C" bool brixGuiClipboardWriterSetPath(void* writer, const char* path);
extern "C" bool brixGuiClipboardWriterCommit(void* writer, const char* name);
extern "C" void brixGuiClipboardWriterRelease(void* writer);
extern "C" void brixGuiClipboardDiscard(const char* name);
extern "C" const char* brixGuiWindowDropPasteboardName(void* window);
extern "C" void brixGuiWindowSetDropOperation(void* window, uint32_t operation);
extern "C" bool brixGuiWindowBeginExportDrag(void* window, void* writer, void* canvas, double x, double y, double width, double height);
extern "C" uint64_t brixGuiDragWaitPromisedFiles(uint64_t sequence, uint64_t timeoutMilliseconds);
extern "C" const char* brixGuiDragPromisedFile(uint64_t sequence, uint64_t index);
extern "C" void brixGuiDragReleasePromisedFiles(uint64_t sequence);
extern "C" bool brixGuiTypeConforms(const char* type, const char* parent);
extern "C" const char* brixGuiTypeOfPath(const char* path);

// **********************************************************************************************

extern "C" bool brixGuiTrashPath(const char* path);
extern "C" bool brixGuiRevealPath(const char* path);
extern "C" bool brixGuiShowPathInfo(const char* path);
extern "C" bool brixGuiOpenURL(const char* url);
extern "C" bool brixGuiOpenPath(const char* path);
extern "C" bool brixGuiOpenPathWithApp(const char* path, const char* appPath);
extern "C" const char* brixGuiPathDisplayName(const char* path);
extern "C" void* brixGuiPathIconCanvas(const char* path, double size, double scale);

extern "C" void* brixGuiHotKeyRegister(
    uint32_t nativeKeyCode,
    uint64_t modifiers,
    BxGuiActionCallback callback,
    void* context
);
extern "C" void brixGuiHotKeyUnregister(void* hotKey);
extern "C" void* brixGuiModifierDoubleTapRegister(
    uint64_t modifiers,
    BxGuiActionCallback callback,
    void* context
);
extern "C" void brixGuiModifierDoubleTapUnregister(void* monitor);

extern "C" void* brixGuiQuickLookCreate(void* window);
extern "C" void brixGuiQuickLookRelease(void* quickLook);
extern "C" void brixGuiQuickLookSetPath(void* quickLook, const char* path);
extern "C" void brixGuiQuickLookPanelShow(void* window, const char* path);
extern "C" void brixGuiQuickLookPanelUpdate(void* window, const char* path);
extern "C" void brixGuiQuickLookPanelClose(void* window);
extern "C" bool brixGuiQuickLookPanelIsVisible(void);

typedef void (*BxGuiQuickLookPreviewCallback)(
    void* context,
    void* hostWindow,
    const char* path,
    void* completion
);
typedef void (*BxGuiQuickLookCloseCallback)(
    void* context,
    void* hostWindow
);

extern "C" BX_GUI_WITHOUT_BLOCKING void brixGuiQuickLookExtensionRun(
    BxGuiQuickLookPreviewCallback previewCallback,
    BxGuiQuickLookCloseCallback closeCallback,
    void* context
);
extern "C" void brixGuiQuickLookExtensionFinish(
    void* completion,
    const char* errorMessage
);

// **********************************************************************************************

extern "C" void* brixGuiTextInputCreate(
    void* window,
    double x,
    double y,
    double width,
    double height,
    const char* text,
    const char* placeholder
);
extern "C" void brixGuiTextInputRelease(void* input);
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
);
extern "C" void brixGuiTextInputSetText(void* input, const char* text);
extern "C" const char* brixGuiTextInputGetText(void* input);
extern "C" void brixGuiTextInputSetEventCallback(
    void* input,
    BxGuiTextInputEventCallback callback,
    void* context
);
extern "C" void brixGuiTextInputSetPlaceholder(void* input, const char* placeholder);
extern "C" void brixGuiTextInputSetTextColor(
    void* input,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" void brixGuiTextInputSetFont(
    void* input,
    const char* family,
    double size,
    double weight
);
extern "C" void brixGuiTextInputSetAlignment(void* input, int32_t alignment);
extern "C" void brixGuiTextInputSetEnabled(void* input, bool enabled);
extern "C" void brixGuiTextInputSetEditable(void* input, bool editable);
extern "C" void brixGuiTextInputSetSecure(void* input, bool secure);
extern "C" void brixGuiTextInputSetMaxLength(void* input, uint32_t maxLength);
extern "C" void brixGuiTextInputSetVisible(void* input, bool visible);
extern "C" void brixGuiTextInputSetOpacity(void* input, double opacity);
extern "C" double brixGuiTextInputGetOpacity(void* input);
extern "C" void brixGuiTextInputSetCursorStyle(void* input, uint32_t cursorStyle, void* cursorImage);
extern "C" uint32_t brixGuiTextInputDebugCursorStyle(void* input);
extern "C" void brixGuiTextInputSetContainer(void* input, void* boxHost);
extern "C" bool brixGuiTextInputHasContainer(void* input, void* boxHost);
extern "C" double brixGuiTextInputGetFittingWidth(void* input);
extern "C" double brixGuiTextInputGetFittingHeight(void* input);
extern "C" double brixGuiTextInputGetFittingHeightForWidth(void* input, double width);
extern "C" void brixGuiTextInputSetWraps(void* input, bool wraps);
extern "C" void brixGuiTextInputSetSelection(void* input, const char* before, const char* selected);
extern "C" void brixGuiTextInputSetPassedKeys(void* input, uint32_t keys);
extern "C" void brixGuiTextInputSetLineMetrics(void* input, double lineHeight, double lineSpacing);
extern "C" void brixGuiTextInputSetTextInsets(void* input, double top, double bottom);
extern "C" void brixGuiTextInputSetHitRect(void* input, double x, double y, double width, double height);
extern "C" double brixGuiTextInputDebugLineMetric(void* input, uint32_t metric);
extern "C" void brixGuiTextInputReplaceText(void* input, const char* text);
extern "C" void brixGuiTextInputPasteText(void* input);
extern "C" void brixGuiTextInputMoveCaretToEnd(void* input);
extern "C" void brixGuiTextInputSyncText(void* input, const char* text);
extern "C" bool brixGuiTextInputCanUndo(void* input, bool redoes);
extern "C" void brixGuiTextInputUndo(void* input, bool redoes);
extern "C" double brixGuiTextInputGetTextOriginX(void* input);
extern "C" double brixGuiTextInputGetTextBaseline(void* input);
extern "C" void brixGuiTextInputDebugInjectEvent(
    void* input,
    uint32_t eventKind,
    const char* text
);

#undef BX_GUI_WITHOUT_BLOCKING

extern "C" void* brixGuiCursorCreate(void* canvas, double hotspotX, double hotspotY);
extern "C" void brixGuiCursorRelease(void* cursor);
extern "C" double brixGuiCursorWidth(void* cursor);
extern "C" double brixGuiCursorHotspotX(void* cursor);
extern "C" double brixGuiCursorHotspotY(void* cursor);

#endif
