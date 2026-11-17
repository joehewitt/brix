#ifndef YU_TERMINAL_H
#define YU_TERMINAL_H

#include <stdbool.h>
#include <stdint.h>

#include <YuGC/YuGCAnnotations.h>

#ifdef __cplusplus
#define YU_TERMINAL_API extern "C"
#else
#define YU_TERMINAL_API extern
#endif


YU_TERMINAL_API void* yumiTerminalCreate(uint32_t columns, uint32_t rows, uint32_t scrollbackLines);
YU_TERMINAL_API void yumiTerminalRelease(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalReset(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalResize(
    void* terminal,
    uint32_t columns,
    uint32_t rows,
    uint32_t cellWidth,
    uint32_t cellHeight
);

YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalWrite(void* terminal, const void* bytes, uint32_t length);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalReplyLength(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalTakeReplies(void* terminal, void* buffer, uint32_t capacity);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalTakeEvents(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalCopyTitle(void* terminal, void* buffer, uint32_t capacity);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalCopyPwd(void* terminal, void* buffer, uint32_t capacity);

YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalSetColors(
    void* terminal,
    uint32_t foreground,
    uint32_t background,
    uint32_t cursor,
    const void* palette,
    uint32_t paletteCount
);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalSetDark(void* terminal, bool dark);

YU_TERMINAL_API YUMI_WITHOUT_GC bool yumiTerminalModeEnabled(
    void* terminal,
    uint32_t mode,
    bool ansi
);
YU_TERMINAL_API YUMI_WITHOUT_GC bool yumiTerminalMouseTracking(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC bool yumiTerminalIsAlternateScreen(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC bool yumiTerminalViewportAtBottom(void* terminal);

YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalScrollDelta(void* terminal, int32_t rows);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalScrollTop(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalScrollBottom(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalScrollRow(void* terminal, uint32_t row);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalScrollbar(void* terminal, void* out);


YU_TERMINAL_API YUMI_WITHOUT_GC int32_t yumiTerminalRenderUpdate(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalRenderCursor(void* terminal, void* out);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalRenderColors(void* terminal, void* out);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalRenderBeginRows(void* terminal, bool dirtyOnly);
YU_TERMINAL_API YUMI_WITHOUT_GC int32_t yumiTerminalRenderNextRow(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalRowRunCount(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalRowTextLength(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalRowCopy(void* terminal, void* runs, void* text);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalRowInfo(void* terminal, void* out);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalRenderClean(void* terminal);


YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalEncodeKey(
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
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalEncodeMouse(
    void* terminal,
    uint32_t action,
    uint32_t button,
    uint32_t mods,
    float x,
    float y,
    void* buffer,
    uint32_t capacity
);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalEncodePaste(
    void* terminal,
    const void* text,
    uint32_t length,
    void* buffer,
    uint32_t capacity
);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalEncodeFocus(
    void* terminal,
    bool focused,
    void* buffer,
    uint32_t capacity
);


YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalSelectPress(
    void* terminal,
    uint32_t column,
    uint32_t row,
    double x,
    double y
);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalSelectDrag(
    void* terminal,
    uint32_t column,
    uint32_t row,
    double x,
    double y,
    bool rectangle
);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalSelectRelease(void* terminal, uint32_t column, uint32_t row);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalSelectAll(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC void yumiTerminalSelectClear(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC bool yumiTerminalHasSelection(void* terminal);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalCopySelection(void* terminal, void* buffer, uint32_t capacity);
YU_TERMINAL_API YUMI_WITHOUT_GC uint32_t yumiTerminalCopyAll(void* terminal, void* buffer, uint32_t capacity);

#endif
