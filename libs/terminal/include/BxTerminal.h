#ifndef BX_TERMINAL_H
#define BX_TERMINAL_H

#include <stdbool.h>
#include <stdint.h>

#include <BxGC/BxGCAnnotations.h>

#ifdef __cplusplus
#define BX_TERMINAL_API extern "C"
#else
#define BX_TERMINAL_API extern
#endif


BX_TERMINAL_API void* brixTerminalCreate(uint32_t columns, uint32_t rows, uint32_t scrollbackLines);
BX_TERMINAL_API void brixTerminalRelease(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalReset(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalResize(
    void* terminal,
    uint32_t columns,
    uint32_t rows,
    uint32_t cellWidth,
    uint32_t cellHeight
);

BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalWrite(void* terminal, const void* bytes, uint32_t length);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalReplyLength(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalTakeReplies(void* terminal, void* buffer, uint32_t capacity);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalTakeEvents(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalCopyTitle(void* terminal, void* buffer, uint32_t capacity);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalCopyPwd(void* terminal, void* buffer, uint32_t capacity);

BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalSetColors(
    void* terminal,
    uint32_t foreground,
    uint32_t background,
    uint32_t cursor,
    const void* palette,
    uint32_t paletteCount
);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalSetDark(void* terminal, bool dark);

BX_TERMINAL_API BRIX_WITHOUT_GC bool brixTerminalModeEnabled(
    void* terminal,
    uint32_t mode,
    bool ansi
);
BX_TERMINAL_API BRIX_WITHOUT_GC bool brixTerminalMouseTracking(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC bool brixTerminalIsAlternateScreen(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC bool brixTerminalViewportAtBottom(void* terminal);

BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalScrollDelta(void* terminal, int32_t rows);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalScrollTop(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalScrollBottom(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalScrollRow(void* terminal, uint32_t row);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalScrollbar(void* terminal, void* out);


BX_TERMINAL_API BRIX_WITHOUT_GC int32_t brixTerminalRenderUpdate(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalRenderCursor(void* terminal, void* out);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalRenderColors(void* terminal, void* out);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalRenderBeginRows(void* terminal, bool dirtyOnly);
BX_TERMINAL_API BRIX_WITHOUT_GC int32_t brixTerminalRenderNextRow(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalRowRunCount(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalRowTextLength(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalRowCopy(void* terminal, void* runs, void* text);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalRowInfo(void* terminal, void* out);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalRenderClean(void* terminal);


BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalEncodeKey(
    void* terminal,
    uint32_t keyCode,
    uint32_t mods,
    uint32_t action,
    const void* text,
    uint32_t textLength,
    uint32_t unshiftedCodepoint,
    void* buffer,
    uint32_t capacity
);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalEncodeMouse(
    void* terminal,
    uint32_t action,
    uint32_t button,
    uint32_t mods,
    float x,
    float y,
    void* buffer,
    uint32_t capacity
);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalEncodePaste(
    void* terminal,
    const void* text,
    uint32_t length,
    void* buffer,
    uint32_t capacity
);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalEncodeFocus(
    void* terminal,
    bool focused,
    void* buffer,
    uint32_t capacity
);


BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalSelectPress(
    void* terminal,
    uint32_t column,
    uint32_t row,
    double x,
    double y
);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalSelectDrag(
    void* terminal,
    uint32_t column,
    uint32_t row,
    double x,
    double y,
    bool rectangle
);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalSelectRelease(void* terminal, uint32_t column, uint32_t row);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalSelectAll(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC void brixTerminalSelectClear(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC bool brixTerminalHasSelection(void* terminal);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalCopySelection(void* terminal, void* buffer, uint32_t capacity);
BX_TERMINAL_API BRIX_WITHOUT_GC uint32_t brixTerminalCopyAll(void* terminal, void* buffer, uint32_t capacity);

#endif
