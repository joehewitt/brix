#include "include/BxTerminal.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ghostty/vt.h>

enum {
    BxTerminalEventBell = 1,
    BxTerminalEventTitle = 2,
    BxTerminalEventPwd = 4,
    BxTerminalEventReset = 8
};

enum {
    BxTerminalRunBold = 1 << 0,
    BxTerminalRunItalic = 1 << 1,
    BxTerminalRunFaint = 1 << 2,
    BxTerminalRunUnderline = 1 << 3,
    BxTerminalRunStrike = 1 << 4,
    BxTerminalRunOverline = 1 << 5,
    BxTerminalRunInvisible = 1 << 6,
    BxTerminalRunBlink = 1 << 7,
    BxTerminalRunWide = 1 << 8,
    BxTerminalRunIsolated = 1 << 9,
    BxTerminalRunSprite = 1 << 10,
    BxTerminalRunBackground = 1 << 15
};

enum {
    BxTerminalRunFields = 7
};

typedef struct BxTerminal {
    GhosttyTerminal terminal;
    GhosttyRenderState render;
    GhosttyRenderStateRowIterator rows;
    GhosttyRenderStateRowCells cells;
    GhosttyKeyEncoder keyEncoder;
    GhosttyKeyEvent keyEvent;
    GhosttyMouseEncoder mouseEncoder;
    GhosttyMouseEvent mouseEvent;
    GhosttySelectionGesture gesture;
    GhosttySelectionGestureEvent gesturePress;
    GhosttySelectionGestureEvent gestureDrag;
    GhosttySelectionGestureEvent gestureRelease;

    uint8_t* replies;
    size_t replyLength;
    size_t replyCapacity;
    uint32_t events;
    bool dark;

    uint16_t columns;
    uint16_t rowCount;
    uint32_t cellWidth;
    uint32_t cellHeight;

    bool dirtyOnly;
    GhosttyRenderStateColors colors;
    uint32_t* runs;
    size_t runCount;
    size_t runCapacity;
    uint8_t* text;
    size_t textLength;
    size_t textCapacity;
    bool hasRowSelection;
    uint16_t rowSelectionStart;
    uint16_t rowSelectionEnd;
    bool rowWraps;
} BxTerminal;

static const uint16_t kMacKeyMap[128] = {
    [0x00] = GHOSTTY_KEY_A,
    [0x01] = GHOSTTY_KEY_S,
    [0x02] = GHOSTTY_KEY_D,
    [0x03] = GHOSTTY_KEY_F,
    [0x04] = GHOSTTY_KEY_H,
    [0x05] = GHOSTTY_KEY_G,
    [0x06] = GHOSTTY_KEY_Z,
    [0x07] = GHOSTTY_KEY_X,
    [0x08] = GHOSTTY_KEY_C,
    [0x09] = GHOSTTY_KEY_V,
    [0x0A] = GHOSTTY_KEY_INTL_BACKSLASH,
    [0x0B] = GHOSTTY_KEY_B,
    [0x0C] = GHOSTTY_KEY_Q,
    [0x0D] = GHOSTTY_KEY_W,
    [0x0E] = GHOSTTY_KEY_E,
    [0x0F] = GHOSTTY_KEY_R,
    [0x10] = GHOSTTY_KEY_Y,
    [0x11] = GHOSTTY_KEY_T,
    [0x12] = GHOSTTY_KEY_DIGIT_1,
    [0x13] = GHOSTTY_KEY_DIGIT_2,
    [0x14] = GHOSTTY_KEY_DIGIT_3,
    [0x15] = GHOSTTY_KEY_DIGIT_4,
    [0x16] = GHOSTTY_KEY_DIGIT_6,
    [0x17] = GHOSTTY_KEY_DIGIT_5,
    [0x18] = GHOSTTY_KEY_EQUAL,
    [0x19] = GHOSTTY_KEY_DIGIT_9,
    [0x1A] = GHOSTTY_KEY_DIGIT_7,
    [0x1B] = GHOSTTY_KEY_MINUS,
    [0x1C] = GHOSTTY_KEY_DIGIT_8,
    [0x1D] = GHOSTTY_KEY_DIGIT_0,
    [0x1E] = GHOSTTY_KEY_BRACKET_RIGHT,
    [0x1F] = GHOSTTY_KEY_O,
    [0x20] = GHOSTTY_KEY_U,
    [0x21] = GHOSTTY_KEY_BRACKET_LEFT,
    [0x22] = GHOSTTY_KEY_I,
    [0x23] = GHOSTTY_KEY_P,
    [0x24] = GHOSTTY_KEY_ENTER,
    [0x25] = GHOSTTY_KEY_L,
    [0x26] = GHOSTTY_KEY_J,
    [0x27] = GHOSTTY_KEY_QUOTE,
    [0x28] = GHOSTTY_KEY_K,
    [0x29] = GHOSTTY_KEY_SEMICOLON,
    [0x2A] = GHOSTTY_KEY_BACKSLASH,
    [0x2B] = GHOSTTY_KEY_COMMA,
    [0x2C] = GHOSTTY_KEY_SLASH,
    [0x2D] = GHOSTTY_KEY_N,
    [0x2E] = GHOSTTY_KEY_M,
    [0x2F] = GHOSTTY_KEY_PERIOD,
    [0x30] = GHOSTTY_KEY_TAB,
    [0x31] = GHOSTTY_KEY_SPACE,
    [0x32] = GHOSTTY_KEY_BACKQUOTE,
    [0x33] = GHOSTTY_KEY_BACKSPACE,
    [0x35] = GHOSTTY_KEY_ESCAPE,
    [0x36] = GHOSTTY_KEY_META_RIGHT,
    [0x37] = GHOSTTY_KEY_META_LEFT,
    [0x38] = GHOSTTY_KEY_SHIFT_LEFT,
    [0x39] = GHOSTTY_KEY_CAPS_LOCK,
    [0x3A] = GHOSTTY_KEY_ALT_LEFT,
    [0x3B] = GHOSTTY_KEY_CONTROL_LEFT,
    [0x3C] = GHOSTTY_KEY_SHIFT_RIGHT,
    [0x3D] = GHOSTTY_KEY_ALT_RIGHT,
    [0x3E] = GHOSTTY_KEY_CONTROL_RIGHT,
    [0x3F] = GHOSTTY_KEY_FN,
    [0x40] = GHOSTTY_KEY_F17,
    [0x41] = GHOSTTY_KEY_NUMPAD_DECIMAL,
    [0x43] = GHOSTTY_KEY_NUMPAD_MULTIPLY,
    [0x45] = GHOSTTY_KEY_NUMPAD_ADD,
    [0x47] = GHOSTTY_KEY_NUM_LOCK,
    [0x48] = GHOSTTY_KEY_AUDIO_VOLUME_UP,
    [0x49] = GHOSTTY_KEY_AUDIO_VOLUME_DOWN,
    [0x4A] = GHOSTTY_KEY_AUDIO_VOLUME_MUTE,
    [0x4B] = GHOSTTY_KEY_NUMPAD_DIVIDE,
    [0x4C] = GHOSTTY_KEY_NUMPAD_ENTER,
    [0x4E] = GHOSTTY_KEY_NUMPAD_SUBTRACT,
    [0x4F] = GHOSTTY_KEY_F18,
    [0x50] = GHOSTTY_KEY_F19,
    [0x51] = GHOSTTY_KEY_NUMPAD_EQUAL,
    [0x52] = GHOSTTY_KEY_NUMPAD_0,
    [0x53] = GHOSTTY_KEY_NUMPAD_1,
    [0x54] = GHOSTTY_KEY_NUMPAD_2,
    [0x55] = GHOSTTY_KEY_NUMPAD_3,
    [0x56] = GHOSTTY_KEY_NUMPAD_4,
    [0x57] = GHOSTTY_KEY_NUMPAD_5,
    [0x58] = GHOSTTY_KEY_NUMPAD_6,
    [0x59] = GHOSTTY_KEY_NUMPAD_7,
    [0x5A] = GHOSTTY_KEY_F20,
    [0x5B] = GHOSTTY_KEY_NUMPAD_8,
    [0x5C] = GHOSTTY_KEY_NUMPAD_9,
    [0x5D] = GHOSTTY_KEY_INTL_YEN,
    [0x5E] = GHOSTTY_KEY_INTL_RO,
    [0x5F] = GHOSTTY_KEY_NUMPAD_COMMA,
    [0x60] = GHOSTTY_KEY_F5,
    [0x61] = GHOSTTY_KEY_F6,
    [0x62] = GHOSTTY_KEY_F7,
    [0x63] = GHOSTTY_KEY_F3,
    [0x64] = GHOSTTY_KEY_F8,
    [0x65] = GHOSTTY_KEY_F9,
    [0x66] = GHOSTTY_KEY_NON_CONVERT,
    [0x67] = GHOSTTY_KEY_F11,
    [0x68] = GHOSTTY_KEY_KANA_MODE,
    [0x69] = GHOSTTY_KEY_F13,
    [0x6A] = GHOSTTY_KEY_F16,
    [0x6B] = GHOSTTY_KEY_F14,
    [0x6D] = GHOSTTY_KEY_F10,
    [0x6E] = GHOSTTY_KEY_CONTEXT_MENU,
    [0x6F] = GHOSTTY_KEY_F12,
    [0x71] = GHOSTTY_KEY_F15,
    [0x72] = GHOSTTY_KEY_INSERT,
    [0x73] = GHOSTTY_KEY_HOME,
    [0x74] = GHOSTTY_KEY_PAGE_UP,
    [0x75] = GHOSTTY_KEY_DELETE,
    [0x76] = GHOSTTY_KEY_F4,
    [0x77] = GHOSTTY_KEY_END,
    [0x78] = GHOSTTY_KEY_F2,
    [0x79] = GHOSTTY_KEY_PAGE_DOWN,
    [0x7A] = GHOSTTY_KEY_F1,
    [0x7B] = GHOSTTY_KEY_ARROW_LEFT,
    [0x7C] = GHOSTTY_KEY_ARROW_RIGHT,
    [0x7D] = GHOSTTY_KEY_ARROW_DOWN,
    [0x7E] = GHOSTTY_KEY_ARROW_UP,
};

static const char kMacKeyUnshifted[128] = {
    [0x00] = 'a', [0x01] = 's', [0x02] = 'd', [0x03] = 'f', [0x04] = 'h', [0x05] = 'g',
    [0x06] = 'z', [0x07] = 'x', [0x08] = 'c', [0x09] = 'v', [0x0B] = 'b', [0x0C] = 'q',
    [0x0D] = 'w', [0x0E] = 'e', [0x0F] = 'r', [0x10] = 'y', [0x11] = 't', [0x12] = '1',
    [0x13] = '2', [0x14] = '3', [0x15] = '4', [0x16] = '6', [0x17] = '5', [0x18] = '=',
    [0x19] = '9', [0x1A] = '7', [0x1B] = '-', [0x1C] = '8', [0x1D] = '0', [0x1E] = ']',
    [0x1F] = 'o', [0x20] = 'u', [0x21] = '[', [0x22] = 'i', [0x23] = 'p', [0x25] = 'l',
    [0x26] = 'j', [0x27] = '\'', [0x28] = 'k', [0x29] = ';', [0x2A] = '\\', [0x2B] = ',',
    [0x2C] = '/', [0x2D] = 'n', [0x2E] = 'm', [0x2F] = '.', [0x31] = ' ', [0x32] = '`',
};


static uint32_t packColor(GhosttyColorRgb color) {
    return ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | (uint32_t)color.b;
}

static GhosttyColorRgb unpackColor(uint32_t value) {
    GhosttyColorRgb color;
    color.r = (uint8_t)((value >> 16) & 0xFF);
    color.g = (uint8_t)((value >> 8) & 0xFF);
    color.b = (uint8_t)(value & 0xFF);
    return color;
}

static bool reserve(void** buffer, size_t* capacity, size_t needed, size_t itemSize) {
    if (needed <= *capacity) {
        return true;
    }
    size_t next = *capacity ? *capacity : 64;
    while (next < needed) {
        next *= 2;
    }
    void* grown = realloc(*buffer, next * itemSize);
    if (!grown) {
        return false;
    }
    *buffer = grown;
    *capacity = next;
    return true;
}

static uint64_t monotonicNanoseconds(void) {
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
}

static GhosttyMode modeFor(uint32_t number, bool ansi) {
    return ghostty_mode_new((uint16_t)number, ansi);
}

static bool modeEnabled(BxTerminal* term, GhosttyMode mode) {
    GhosttyTerminalModeConfig config = {.mode = mode, .value = false};
    if (ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_MODE, &config) != GHOSTTY_SUCCESS) {
        return false;
    }
    return config.value;
}

static bool isValidUtf8(const uint8_t* bytes, size_t length) {
    size_t index = 0;
    while (index < length) {
        uint8_t lead = bytes[index];
        size_t extra = 0;
        uint32_t codepoint = 0;
        if (lead < 0x80) {
            index += 1;
            continue;
        } else if ((lead & 0xE0) == 0xC0) {
            extra = 1;
            codepoint = lead & 0x1F;
        } else if ((lead & 0xF0) == 0xE0) {
            extra = 2;
            codepoint = lead & 0x0F;
        } else if ((lead & 0xF8) == 0xF0) {
            extra = 3;
            codepoint = lead & 0x07;
        } else {
            return false;
        }
        if (index + extra >= length) {
            return false;
        }
        for (size_t offset = 1; offset <= extra; offset++) {
            uint8_t next = bytes[index + offset];
            if ((next & 0xC0) != 0x80) {
                return false;
            }
            codepoint = (codepoint << 6) | (next & 0x3F);
        }
        if ((extra == 1 && codepoint < 0x80) || (extra == 2 && codepoint < 0x800) ||
            (extra == 3 && (codepoint < 0x10000 || codepoint > 0x10FFFF)) ||
            (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            return false;
        }
        index += extra + 1;
    }
    return true;
}

static uint32_t copyString(GhosttyString string, void* buffer, uint32_t capacity) {
    if (!isValidUtf8(string.ptr, string.len)) {
        return 0;
    }
    if (buffer && string.len <= capacity && string.len > 0) {
        memcpy(buffer, string.ptr, string.len);
    }
    return (uint32_t)string.len;
}


static void onWritePty(GhosttyTerminal terminal, void* userdata, const uint8_t* data, size_t length) {
    (void)terminal;
    BxTerminal* term = (BxTerminal*)userdata;
    if (!reserve((void**)&term->replies, &term->replyCapacity, term->replyLength + length, 1)) {
        return;
    }
    memcpy(term->replies + term->replyLength, data, length);
    term->replyLength += length;
}

static void onBell(GhosttyTerminal terminal, void* userdata) {
    (void)terminal;
    ((BxTerminal*)userdata)->events |= BxTerminalEventBell;
}

static void onTitleChanged(GhosttyTerminal terminal, void* userdata) {
    (void)terminal;
    ((BxTerminal*)userdata)->events |= BxTerminalEventTitle;
}

static void onPwdChanged(GhosttyTerminal terminal, void* userdata) {
    (void)terminal;
    ((BxTerminal*)userdata)->events |= BxTerminalEventPwd;
}

static void onReset(GhosttyTerminal terminal, void* userdata) {
    (void)terminal;
    ((BxTerminal*)userdata)->events |= BxTerminalEventReset;
}

static bool onColorScheme(GhosttyTerminal terminal, void* userdata, GhosttyColorScheme* scheme) {
    (void)terminal;
    *scheme = ((BxTerminal*)userdata)->dark ? GHOSTTY_COLOR_SCHEME_DARK : GHOSTTY_COLOR_SCHEME_LIGHT;
    return true;
}

static bool onSize(GhosttyTerminal terminal, void* userdata, GhosttySizeReportSize* size) {
    (void)terminal;
    BxTerminal* term = (BxTerminal*)userdata;
    size->rows = term->rowCount;
    size->columns = term->columns;
    size->cell_width = term->cellWidth;
    size->cell_height = term->cellHeight;
    return true;
}

static GhosttyString onXtversion(GhosttyTerminal terminal, void* userdata) {
    (void)terminal;
    (void)userdata;
    static const char version[] = "Brix";
    GhosttyString string = {.ptr = (const uint8_t*)version, .len = sizeof(version) - 1};
    return string;
}


void* brixTerminalCreate(uint32_t columns, uint32_t rows, uint32_t scrollbackLines) {
    BxTerminal* term = (BxTerminal*)calloc(1, sizeof(BxTerminal));
    if (!term) {
        return NULL;
    }
    term->columns = (uint16_t)columns;
    term->rowCount = (uint16_t)rows;
    term->cellWidth = 8;
    term->cellHeight = 16;
    term->dark = true;

    if (ghostty_terminal_new(NULL, &term->terminal, (uint16_t)columns, (uint16_t)rows) != GHOSTTY_SUCCESS) {
        free(term);
        return NULL;
    }
    ghostty_render_state_new(NULL, &term->render);
    ghostty_render_state_row_iterator_new(NULL, &term->rows);
    ghostty_render_state_row_cells_new(NULL, &term->cells);
    ghostty_key_encoder_new(NULL, &term->keyEncoder);
    ghostty_key_event_new(NULL, &term->keyEvent);
    ghostty_mouse_encoder_new(NULL, &term->mouseEncoder);
    ghostty_mouse_event_new(NULL, &term->mouseEvent);
    ghostty_selection_gesture_new(NULL, &term->gesture);
    ghostty_selection_gesture_event_new(NULL, &term->gesturePress, GHOSTTY_SELECTION_GESTURE_EVENT_TYPE_PRESS);
    ghostty_selection_gesture_event_new(NULL, &term->gestureDrag, GHOSTTY_SELECTION_GESTURE_EVENT_TYPE_DRAG);
    ghostty_selection_gesture_event_new(NULL, &term->gestureRelease, GHOSTTY_SELECTION_GESTURE_EVENT_TYPE_RELEASE);

    GhosttyTerminal terminal = term->terminal;
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_USERDATA, term);
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_WRITE_PTY, (const void*)onWritePty);
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_BELL, (const void*)onBell);
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_TITLE_CHANGED, (const void*)onTitleChanged);
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_PWD_CHANGED, (const void*)onPwdChanged);
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_RESET, (const void*)onReset);
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_COLOR_SCHEME, (const void*)onColorScheme);
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_SIZE, (const void*)onSize);
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_XTVERSION, (const void*)onXtversion);

    size_t lines = scrollbackLines;
    ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_SCROLLBACK_MAX_LINES, &lines);

    GhosttyOptionAsAlt optionAsAlt = GHOSTTY_OPTION_AS_ALT_FALSE;
    ghostty_key_encoder_setopt(term->keyEncoder, GHOSTTY_KEY_ENCODER_OPT_MACOS_OPTION_AS_ALT, &optionAsAlt);
    return term;
}

void brixTerminalRelease(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    if (!term) {
        return;
    }
    ghostty_selection_gesture_event_free(term->gestureRelease);
    ghostty_selection_gesture_event_free(term->gestureDrag);
    ghostty_selection_gesture_event_free(term->gesturePress);
    ghostty_selection_gesture_free(term->gesture, term->terminal);
    ghostty_mouse_event_free(term->mouseEvent);
    ghostty_mouse_encoder_free(term->mouseEncoder);
    ghostty_key_event_free(term->keyEvent);
    ghostty_key_encoder_free(term->keyEncoder);
    ghostty_render_state_row_cells_free(term->cells);
    ghostty_render_state_row_iterator_free(term->rows);
    ghostty_render_state_free(term->render);
    ghostty_terminal_free(term->terminal);
    free(term->replies);
    free(term->runs);
    free(term->text);
    free(term);
}

void brixTerminalReset(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    ghostty_terminal_reset(term->terminal);
}

void brixTerminalResize(void* terminal, uint32_t columns, uint32_t rows, uint32_t cellWidth, uint32_t cellHeight) {
    BxTerminal* term = (BxTerminal*)terminal;
    if (columns < 1) {
        columns = 1;
    }
    if (rows < 1) {
        rows = 1;
    }
    term->columns = (uint16_t)columns;
    term->rowCount = (uint16_t)rows;
    term->cellWidth = cellWidth;
    term->cellHeight = cellHeight;
    ghostty_terminal_resize(term->terminal, (uint16_t)columns, (uint16_t)rows, cellWidth, cellHeight);
}


void brixTerminalWrite(void* terminal, const void* bytes, uint32_t length) {
    BxTerminal* term = (BxTerminal*)terminal;
    ghostty_terminal_vt_write(term->terminal, (const uint8_t*)bytes, length);
}

uint32_t brixTerminalReplyLength(void* terminal) {
    return (uint32_t)((BxTerminal*)terminal)->replyLength;
}

uint32_t brixTerminalTakeReplies(void* terminal, void* buffer, uint32_t capacity) {
    BxTerminal* term = (BxTerminal*)terminal;
    size_t count = term->replyLength < capacity ? term->replyLength : capacity;
    if (count > 0) {
        memcpy(buffer, term->replies, count);
        memmove(term->replies, term->replies + count, term->replyLength - count);
        term->replyLength -= count;
    }
    return (uint32_t)count;
}

uint32_t brixTerminalTakeEvents(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    uint32_t events = term->events;
    term->events = 0;
    return events;
}

uint32_t brixTerminalCopyTitle(void* terminal, void* buffer, uint32_t capacity) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyString title = {0};
    if (ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_TITLE, &title) != GHOSTTY_SUCCESS) {
        return 0;
    }
    return copyString(title, buffer, capacity);
}

uint32_t brixTerminalCopyPwd(void* terminal, void* buffer, uint32_t capacity) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyString pwd = {0};
    if (ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_PWD, &pwd) != GHOSTTY_SUCCESS) {
        return 0;
    }
    return copyString(pwd, buffer, capacity);
}

void brixTerminalSetColors(
    void* terminal,
    uint32_t foreground,
    uint32_t background,
    uint32_t cursor,
    const void* palette,
    uint32_t paletteCount
) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyColorRgb fg = unpackColor(foreground);
    GhosttyColorRgb bg = unpackColor(background);
    GhosttyColorRgb cursorColor = unpackColor(cursor);
    ghostty_terminal_set(term->terminal, GHOSTTY_TERMINAL_OPT_COLOR_FOREGROUND, &fg);
    ghostty_terminal_set(term->terminal, GHOSTTY_TERMINAL_OPT_COLOR_BACKGROUND, &bg);
    ghostty_terminal_set(term->terminal, GHOSTTY_TERMINAL_OPT_COLOR_CURSOR, &cursorColor);

    GhosttyColorRgb colors[256];
    if (ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_COLOR_PALETTE_DEFAULT, &colors) != GHOSTTY_SUCCESS) {
        return;
    }
    const uint32_t* entries = (const uint32_t*)palette;
    uint32_t count = paletteCount < 256 ? paletteCount : 256;
    for (uint32_t index = 0; index < count; index++) {
        colors[index] = unpackColor(entries[index]);
    }
    ghostty_terminal_set(term->terminal, GHOSTTY_TERMINAL_OPT_COLOR_PALETTE, &colors);
}

void brixTerminalSetDark(void* terminal, bool dark) {
    ((BxTerminal*)terminal)->dark = dark;
}

bool brixTerminalModeEnabled(void* terminal, uint32_t mode, bool ansi) {
    return modeEnabled((BxTerminal*)terminal, modeFor(mode, ansi));
}

bool brixTerminalMouseTracking(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    bool tracking = false;
    ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_MOUSE_TRACKING, &tracking);
    return tracking;
}

bool brixTerminalIsAlternateScreen(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyTerminalScreen screen = GHOSTTY_TERMINAL_SCREEN_PRIMARY;
    ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_ACTIVE_SCREEN, &screen);
    return screen == GHOSTTY_TERMINAL_SCREEN_ALTERNATE;
}

bool brixTerminalViewportAtBottom(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    bool active = true;
    ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_VIEWPORT_ACTIVE, &active);
    return active;
}

void brixTerminalScrollDelta(void* terminal, int32_t rows) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyTerminalScrollViewport scroll = {0};
    scroll.tag = GHOSTTY_SCROLL_VIEWPORT_DELTA;
    scroll.value.delta = rows;
    ghostty_terminal_scroll_viewport(term->terminal, scroll);
}

void brixTerminalScrollTop(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyTerminalScrollViewport scroll = {0};
    scroll.tag = GHOSTTY_SCROLL_VIEWPORT_TOP;
    ghostty_terminal_scroll_viewport(term->terminal, scroll);
}

void brixTerminalScrollBottom(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyTerminalScrollViewport scroll = {0};
    scroll.tag = GHOSTTY_SCROLL_VIEWPORT_BOTTOM;
    ghostty_terminal_scroll_viewport(term->terminal, scroll);
}

void brixTerminalScrollRow(void* terminal, uint32_t row) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyTerminalScrollViewport scroll = {0};
    scroll.tag = GHOSTTY_SCROLL_VIEWPORT_ROW;
    scroll.value.row = row;
    ghostty_terminal_scroll_viewport(term->terminal, scroll);
}

void brixTerminalScrollbar(void* terminal, void* out) {
    BxTerminal* term = (BxTerminal*)terminal;
    uint32_t* values = (uint32_t*)out;
    GhosttyTerminalScrollbar scrollbar = {0};
    ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_SCROLLBAR, &scrollbar);
    values[0] = (uint32_t)scrollbar.total;
    values[1] = (uint32_t)scrollbar.offset;
    values[2] = (uint32_t)scrollbar.len;
}


int32_t brixTerminalRenderUpdate(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    if (ghostty_render_state_update(term->render, term->terminal) != GHOSTTY_SUCCESS) {
        return 2;
    }
    term->colors = (GhosttyRenderStateColors)GHOSTTY_INIT_SIZED(GhosttyRenderStateColors);
    ghostty_render_state_get(term->render, GHOSTTY_RENDER_STATE_DATA_COLORS, &term->colors);
    GhosttyRenderStateDirty dirty = GHOSTTY_RENDER_STATE_DIRTY_FALSE;
    ghostty_render_state_get(term->render, GHOSTTY_RENDER_STATE_DATA_DIRTY, &dirty);
    return (int32_t)dirty;
}

void brixTerminalRenderCursor(void* terminal, void* out) {
    BxTerminal* term = (BxTerminal*)terminal;
    int32_t* values = (int32_t*)out;
    GhosttyRenderStateCursor cursor = GHOSTTY_INIT_SIZED(GhosttyRenderStateCursor);
    ghostty_render_state_get(term->render, GHOSTTY_RENDER_STATE_DATA_CURSOR, &cursor);
    values[0] = cursor.visible && cursor.viewport_has_value ? 1 : 0;
    values[1] = cursor.viewport_x;
    values[2] = cursor.viewport_y;
    values[3] = (int32_t)cursor.visual_style;
    values[4] = cursor.blinking ? 1 : 0;
    values[5] = cursor.wide_tail ? 1 : 0;
}

void brixTerminalRenderColors(void* terminal, void* out) {
    BxTerminal* term = (BxTerminal*)terminal;
    uint32_t* values = (uint32_t*)out;
    values[0] = packColor(term->colors.foreground);
    values[1] = packColor(term->colors.background);
    values[2] = packColor(term->colors.cursor_has_value ? term->colors.cursor : term->colors.foreground);
}

void brixTerminalRenderBeginRows(void* terminal, bool dirtyOnly) {
    BxTerminal* term = (BxTerminal*)terminal;
    term->dirtyOnly = dirtyOnly;
    ghostty_render_state_get(term->render, GHOSTTY_RENDER_STATE_DATA_ROW_ITERATOR, &term->rows);
}

static bool isSprite(uint32_t codepoint) {
    return codepoint >= 0x2500 && codepoint <= 0x259F;
}

static bool isGridSafe(uint32_t codepoint) {
    if (codepoint >= 0x20 && codepoint < 0x7F) {
        return true;
    }
    if (codepoint >= 0xA0 && codepoint < 0x300) {
        return true;
    }
    return isSprite(codepoint);
}

static size_t encodeUtf8(uint32_t codepoint, uint8_t* out) {
    if (codepoint < 0x80) {
        out[0] = (uint8_t)codepoint;
        return 1;
    }
    if (codepoint < 0x800) {
        out[0] = (uint8_t)(0xC0 | (codepoint >> 6));
        out[1] = (uint8_t)(0x80 | (codepoint & 0x3F));
        return 2;
    }
    if (codepoint < 0x10000) {
        out[0] = (uint8_t)(0xE0 | (codepoint >> 12));
        out[1] = (uint8_t)(0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = (uint8_t)(0x80 | (codepoint & 0x3F));
        return 3;
    }
    out[0] = (uint8_t)(0xF0 | (codepoint >> 18));
    out[1] = (uint8_t)(0x80 | ((codepoint >> 12) & 0x3F));
    out[2] = (uint8_t)(0x80 | ((codepoint >> 6) & 0x3F));
    out[3] = (uint8_t)(0x80 | (codepoint & 0x3F));
    return 4;
}

static GhosttyColorRgb resolveStyleColor(BxTerminal* term, GhosttyStyleColor color, GhosttyColorRgb fallback) {
    switch (color.tag) {
        case GHOSTTY_STYLE_COLOR_RGB:
            return color.value.rgb;
        case GHOSTTY_STYLE_COLOR_PALETTE:
            return term->colors.palette[color.value.palette];
        default:
            return fallback;
    }
}

static bool appendText(BxTerminal* term, const uint8_t* bytes, size_t length) {
    if (!reserve((void**)&term->text, &term->textCapacity, term->textLength + length, 1)) {
        return false;
    }
    memcpy(term->text + term->textLength, bytes, length);
    term->textLength += length;
    return true;
}

static void trimTrailingBlank(BxTerminal* term) {
    while (term->runCount > 0) {
        uint32_t* run = term->runs + (term->runCount - 1) * BxTerminalRunFields;
        uint32_t decorations = BxTerminalRunBackground | BxTerminalRunUnderline | BxTerminalRunStrike |
                               BxTerminalRunOverline;
        if (run[4] & decorations) {
            return;
        }
        while (run[6] > 0 && term->text[run[5] + run[6] - 1] == ' ') {
            run[6] -= 1;
            run[1] -= 1;
        }
        term->textLength = run[5] + run[6];
        if (run[6] > 0) {
            return;
        }
        term->runCount -= 1;
    }
}

static void buildRow(BxTerminal* term) {
    term->runCount = 0;
    term->textLength = 0;
    term->hasRowSelection = false;
    term->rowWraps = false;

    GhosttyRow row = 0;
    if (ghostty_render_state_row_get(term->rows, GHOSTTY_RENDER_STATE_ROW_DATA_RAW, &row) == GHOSTTY_SUCCESS) {
        ghostty_row_get(row, GHOSTTY_ROW_DATA_WRAP, &term->rowWraps);
    }

    GhosttyRenderStateRowSelection selection = GHOSTTY_INIT_SIZED(GhosttyRenderStateRowSelection);
    if (ghostty_render_state_row_get(term->rows, GHOSTTY_RENDER_STATE_ROW_DATA_SELECTION, &selection) ==
        GHOSTTY_SUCCESS) {
        term->hasRowSelection = true;
        term->rowSelectionStart = selection.start_x;
        term->rowSelectionEnd = selection.end_x;
    }

    if (ghostty_render_state_row_get(term->rows, GHOSTTY_RENDER_STATE_ROW_DATA_CELLS, &term->cells) !=
        GHOSTTY_SUCCESS) {
        return;
    }

    GhosttyColorRgb defaultForeground = term->colors.foreground;
    GhosttyColorRgb defaultBackground = term->colors.background;
    uint32_t column = 0;
    uint32_t codepoints[32];
    uint8_t utf8[4 * 32];

    while (ghostty_render_state_row_cells_next(term->cells)) {
        GhosttyCell raw = 0;
        ghostty_render_state_row_cells_get(term->cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_RAW, &raw);
        GhosttyCellWide wide = GHOSTTY_CELL_WIDE_NARROW;
        ghostty_cell_get(raw, GHOSTTY_CELL_DATA_WIDE, &wide);
        if (wide == GHOSTTY_CELL_WIDE_SPACER_TAIL) {
            column += 1;
            continue;
        }

        uint32_t length = 0;
        ghostty_render_state_row_cells_get(term->cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_GRAPHEMES_LEN, &length);

        bool styled = false;
        ghostty_render_state_row_cells_get(term->cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_HAS_STYLING, &styled);
        GhosttyStyle style = GHOSTTY_INIT_SIZED(GhosttyStyle);
        if (styled) {
            ghostty_render_state_row_cells_get(term->cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_STYLE, &style);
        }

        GhosttyColorRgb foreground = resolveStyleColor(term, style.fg_color, defaultForeground);
        GhosttyColorRgb background = defaultBackground;
        bool hasBackground =
            ghostty_render_state_row_cells_get(term->cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_BG_COLOR, &background) ==
            GHOSTTY_SUCCESS;
        if (!hasBackground) {
            background = defaultBackground;
        }

        uint32_t flags = 0;
        if (style.bold) flags |= BxTerminalRunBold;
        if (style.italic) flags |= BxTerminalRunItalic;
        if (style.faint) flags |= BxTerminalRunFaint;
        if (style.underline != 0) flags |= BxTerminalRunUnderline;
        if (style.strikethrough) flags |= BxTerminalRunStrike;
        if (style.overline) flags |= BxTerminalRunOverline;
        if (style.blink) flags |= BxTerminalRunBlink;
        if (style.inverse) {
            GhosttyColorRgb swap = foreground;
            foreground = background;
            background = swap;
            hasBackground = true;
        }
        if (style.invisible) {
            flags |= BxTerminalRunInvisible;
            foreground = background;
        }
        if (hasBackground) {
            flags |= BxTerminalRunBackground;
        }

        size_t byteCount = 0;
        bool isolated = wide == GHOSTTY_CELL_WIDE_WIDE;
        bool sprite = false;
        if (length == 0 || wide == GHOSTTY_CELL_WIDE_SPACER_HEAD) {
            utf8[0] = ' ';
            byteCount = 1;
        } else {
            uint32_t count = length < 32 ? length : 32;
            if (length > 32) {
                uint32_t scratch[256];
                if (length <= 256) {
                    ghostty_render_state_row_cells_get(
                        term->cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_GRAPHEMES_BUF, scratch);
                    memcpy(codepoints, scratch, sizeof(codepoints));
                } else {
                    codepoints[0] = 0xFFFD;
                    count = 1;
                }
            } else {
                ghostty_render_state_row_cells_get(
                    term->cells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_GRAPHEMES_BUF, codepoints);
            }
            for (uint32_t index = 0; index < count; index++) {
                byteCount += encodeUtf8(codepoints[index], utf8 + byteCount);
            }
            if (length > 1 || !isGridSafe(codepoints[0])) {
                isolated = true;
            } else {
                sprite = isSprite(codepoints[0]);
            }
        }
        if (isolated) {
            flags |= BxTerminalRunIsolated;
        } else if (sprite) {
            flags |= BxTerminalRunSprite;
        }
        if (wide == GHOSTTY_CELL_WIDE_WIDE) {
            flags |= BxTerminalRunWide;
        }

        uint32_t cellColumns = wide == GHOSTTY_CELL_WIDE_WIDE ? 2 : 1;
        uint32_t fg = packColor(foreground);
        uint32_t bg = packColor(background);
        uint32_t* last = term->runCount > 0 ? term->runs + (term->runCount - 1) * BxTerminalRunFields : NULL;
        bool extend = last && !isolated && !(last[4] & BxTerminalRunIsolated) && last[2] == fg && last[3] == bg &&
                      last[4] == flags && last[0] + last[1] == column;
        if (!appendText(term, utf8, byteCount)) {
            return;
        }
        if (extend) {
            last[1] += cellColumns;
            last[6] += (uint32_t)byteCount;
        } else {
            if (!reserve((void**)&term->runs, &term->runCapacity, (term->runCount + 1) * BxTerminalRunFields,
                         sizeof(uint32_t))) {
                return;
            }
            uint32_t* run = term->runs + term->runCount * BxTerminalRunFields;
            run[0] = column;
            run[1] = cellColumns;
            run[2] = fg;
            run[3] = bg;
            run[4] = flags;
            run[5] = (uint32_t)(term->textLength - byteCount);
            run[6] = (uint32_t)byteCount;
            term->runCount += 1;
        }
        column += 1;
    }
    trimTrailingBlank(term);
}

int32_t brixTerminalRenderNextRow(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    if (term->dirtyOnly) {
        uint16_t y = 0;
        if (!ghostty_render_state_row_iterator_next_dirty(term->rows, &y)) {
            return -1;
        }
        buildRow(term);
        return y;
    }
    if (!ghostty_render_state_row_iterator_next(term->rows)) {
        return -1;
    }
    int32_t y = 0;
    ghostty_render_state_row_get(term->rows, GHOSTTY_RENDER_STATE_ROW_DATA_VIEWPORT_Y, &y);
    buildRow(term);
    return y;
}

uint32_t brixTerminalRowRunCount(void* terminal) {
    return (uint32_t)((BxTerminal*)terminal)->runCount;
}

uint32_t brixTerminalRowTextLength(void* terminal) {
    return (uint32_t)((BxTerminal*)terminal)->textLength;
}

void brixTerminalRowCopy(void* terminal, void* runs, void* text) {
    BxTerminal* term = (BxTerminal*)terminal;
    if (runs && term->runCount > 0) {
        memcpy(runs, term->runs, term->runCount * BxTerminalRunFields * sizeof(uint32_t));
    }
    if (text && term->textLength > 0) {
        memcpy(text, term->text, term->textLength);
    }
}

void brixTerminalRowInfo(void* terminal, void* out) {
    BxTerminal* term = (BxTerminal*)terminal;
    int32_t* values = (int32_t*)out;
    values[0] = term->hasRowSelection ? 1 : 0;
    values[1] = term->rowSelectionStart;
    values[2] = term->rowSelectionEnd;
    values[3] = term->rowWraps ? 1 : 0;
}

void brixTerminalRenderClean(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    ghostty_render_state_clean(term->render);
}


uint32_t brixTerminalEncodeKey(
    void* terminal,
    uint32_t keyCode,
    uint32_t mods,
    uint32_t action,
    const void* text,
    uint32_t textLength,
    uint32_t unshiftedCodepoint,
    void* buffer,
    uint32_t capacity
) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyKey key = keyCode < 128 ? (GhosttyKey)kMacKeyMap[keyCode] : GHOSTTY_KEY_UNIDENTIFIED;
    if (unshiftedCodepoint == 0 && keyCode < 128) {
        unshiftedCodepoint = (uint32_t)(unsigned char)kMacKeyUnshifted[keyCode];
    }

    GhosttyMods consumed = 0;
    if (textLength > 0) {
        consumed = (GhosttyMods)(mods & (GHOSTTY_MODS_SHIFT | GHOSTTY_MODS_ALT));
    }

    ghostty_key_encoder_setopt_from_terminal(term->keyEncoder, term->terminal);
    GhosttyOptionAsAlt optionAsAlt = GHOSTTY_OPTION_AS_ALT_FALSE;
    ghostty_key_encoder_setopt(term->keyEncoder, GHOSTTY_KEY_ENCODER_OPT_MACOS_OPTION_AS_ALT, &optionAsAlt);

    ghostty_key_event_set_action(term->keyEvent, (GhosttyKeyAction)action);
    ghostty_key_event_set_key(term->keyEvent, key);
    ghostty_key_event_set_mods(term->keyEvent, (GhosttyMods)mods);
    ghostty_key_event_set_consumed_mods(term->keyEvent, consumed);
    ghostty_key_event_set_composing(term->keyEvent, false);
    ghostty_key_event_set_utf8(term->keyEvent, textLength > 0 ? (const char*)text : NULL, textLength);
    ghostty_key_event_set_unshifted_codepoint(term->keyEvent, unshiftedCodepoint);

    size_t written = 0;
    if (ghostty_key_encoder_encode(term->keyEncoder, term->keyEvent, (char*)buffer, capacity, &written) !=
        GHOSTTY_SUCCESS) {
        return 0;
    }
    return (uint32_t)written;
}

uint32_t brixTerminalEncodeMouse(
    void* terminal,
    uint32_t action,
    uint32_t button,
    uint32_t mods,
    float x,
    float y,
    void* buffer,
    uint32_t capacity
) {
    BxTerminal* term = (BxTerminal*)terminal;
    ghostty_mouse_encoder_setopt_from_terminal(term->mouseEncoder, term->terminal);
    GhosttyMouseEncoderSize size = GHOSTTY_INIT_SIZED(GhosttyMouseEncoderSize);
    size.screen_width = term->columns * term->cellWidth;
    size.screen_height = term->rowCount * term->cellHeight;
    size.cell_width = term->cellWidth;
    size.cell_height = term->cellHeight;
    ghostty_mouse_encoder_setopt(term->mouseEncoder, GHOSTTY_MOUSE_ENCODER_OPT_SIZE, &size);

    ghostty_mouse_event_set_action(term->mouseEvent, (GhosttyMouseAction)action);
    if (button == 0) {
        ghostty_mouse_event_clear_button(term->mouseEvent);
    } else {
        ghostty_mouse_event_set_button(term->mouseEvent, (GhosttyMouseButton)button);
    }
    ghostty_mouse_event_set_mods(term->mouseEvent, (GhosttyMods)mods);
    GhosttyMousePosition position = {.x = x, .y = y};
    ghostty_mouse_event_set_position(term->mouseEvent, position);

    size_t written = 0;
    if (ghostty_mouse_encoder_encode(term->mouseEncoder, term->mouseEvent, (char*)buffer, capacity, &written) !=
        GHOSTTY_SUCCESS) {
        return 0;
    }
    return (uint32_t)written;
}

uint32_t brixTerminalEncodePaste(void* terminal, const void* text, uint32_t length, void* buffer, uint32_t capacity) {
    BxTerminal* term = (BxTerminal*)terminal;
    if (length == 0) {
        return 0;
    }
    char* data = (char*)malloc(length);
    if (!data) {
        return 0;
    }
    memcpy(data, text, length);
    bool bracketed = modeEnabled(term, GHOSTTY_MODE_BRACKETED_PASTE);
    size_t written = 0;
    GhosttyResult result = ghostty_paste_encode(data, length, bracketed, (char*)buffer, capacity, &written);
    free(data);
    if (result != GHOSTTY_SUCCESS && result != GHOSTTY_OUT_OF_SPACE) {
        return 0;
    }
    return (uint32_t)written;
}

uint32_t brixTerminalEncodeFocus(void* terminal, bool focused, void* buffer, uint32_t capacity) {
    BxTerminal* term = (BxTerminal*)terminal;
    if (!modeEnabled(term, GHOSTTY_MODE_FOCUS_EVENT)) {
        return 0;
    }
    size_t written = 0;
    GhosttyFocusEvent event = focused ? GHOSTTY_FOCUS_GAINED : GHOSTTY_FOCUS_LOST;
    if (ghostty_focus_encode(event, (char*)buffer, capacity, &written) != GHOSTTY_SUCCESS) {
        return 0;
    }
    return (uint32_t)written;
}


static bool viewportRef(BxTerminal* term, uint32_t column, uint32_t row, GhosttyGridRef* out) {
    *out = (GhosttyGridRef)GHOSTTY_INIT_SIZED(GhosttyGridRef);
    GhosttyPoint point = {0};
    point.tag = GHOSTTY_POINT_TAG_VIEWPORT;
    point.value.coordinate.x = (uint16_t)(column < term->columns ? column : term->columns - 1);
    point.value.coordinate.y = row < term->rowCount ? row : term->rowCount - 1;
    return ghostty_terminal_grid_ref(term->terminal, point, out) == GHOSTTY_SUCCESS;
}

static void applySelection(BxTerminal* term, GhosttyResult result, GhosttySelection* selection) {
    if (result == GHOSTTY_SUCCESS) {
        ghostty_terminal_set(term->terminal, GHOSTTY_TERMINAL_OPT_SELECTION, selection);
    }
}

void brixTerminalSelectPress(void* terminal, uint32_t column, uint32_t row, double x, double y) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyGridRef ref;
    if (!viewportRef(term, column, row, &ref)) {
        return;
    }
    GhosttySurfacePosition position = {.x = x, .y = y};
    uint64_t now = monotonicNanoseconds();
    ghostty_selection_gesture_event_set(term->gesturePress, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REF, &ref);
    ghostty_selection_gesture_event_set(term->gesturePress, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_POSITION, &position);
    ghostty_selection_gesture_event_set(term->gesturePress, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_TIME_NS, &now);
    GhosttySelection selection = GHOSTTY_INIT_SIZED(GhosttySelection);
    GhosttyResult result = ghostty_selection_gesture_event(term->gesture, term->terminal, term->gesturePress, &selection);
    if (result == GHOSTTY_NO_VALUE) {
        ghostty_terminal_set(term->terminal, GHOSTTY_TERMINAL_OPT_SELECTION, NULL);
        return;
    }
    applySelection(term, result, &selection);
}

void brixTerminalSelectDrag(void* terminal, uint32_t column, uint32_t row, double x, double y, bool rectangle) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyGridRef ref;
    if (!viewportRef(term, column, row, &ref)) {
        return;
    }
    GhosttySurfacePosition position = {.x = x, .y = y};
    GhosttySelectionGestureGeometry geometry = {
        .columns = term->columns,
        .cell_width = term->cellWidth,
        .padding_left = 0,
        .screen_height = term->rowCount * term->cellHeight,
    };
    ghostty_selection_gesture_event_set(term->gestureDrag, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REF, &ref);
    ghostty_selection_gesture_event_set(term->gestureDrag, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_POSITION, &position);
    ghostty_selection_gesture_event_set(term->gestureDrag, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_GEOMETRY, &geometry);
    ghostty_selection_gesture_event_set(term->gestureDrag, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_RECTANGLE, &rectangle);
    GhosttySelection selection = GHOSTTY_INIT_SIZED(GhosttySelection);
    GhosttyResult result = ghostty_selection_gesture_event(term->gesture, term->terminal, term->gestureDrag, &selection);
    applySelection(term, result, &selection);
}

void brixTerminalSelectRelease(void* terminal, uint32_t column, uint32_t row) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttyGridRef ref;
    if (viewportRef(term, column, row, &ref)) {
        ghostty_selection_gesture_event_set(term->gestureRelease, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REF, &ref);
    } else {
        ghostty_selection_gesture_event_set(term->gestureRelease, GHOSTTY_SELECTION_GESTURE_EVENT_OPT_REF, NULL);
    }
    ghostty_selection_gesture_event(term->gesture, term->terminal, term->gestureRelease, NULL);
}

void brixTerminalSelectAll(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttySelection selection = GHOSTTY_INIT_SIZED(GhosttySelection);
    applySelection(term, ghostty_terminal_select_all(term->terminal, &selection), &selection);
}

void brixTerminalSelectClear(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    ghostty_terminal_set(term->terminal, GHOSTTY_TERMINAL_OPT_SELECTION, NULL);
}

bool brixTerminalHasSelection(void* terminal) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttySelection selection = GHOSTTY_INIT_SIZED(GhosttySelection);
    return ghostty_terminal_get(term->terminal, GHOSTTY_TERMINAL_DATA_SELECTION, &selection) == GHOSTTY_SUCCESS;
}

static uint32_t formatSelection(
    BxTerminal* term,
    const GhosttySelection* selection,
    void* buffer,
    uint32_t capacity
) {
    GhosttyTerminalSelectionFormatOptions options = GHOSTTY_INIT_SIZED(GhosttyTerminalSelectionFormatOptions);
    options.emit = GHOSTTY_FORMATTER_FORMAT_PLAIN;
    options.unwrap = true;
    options.trim = true;
    options.selection = selection;
    size_t written = 0;
    GhosttyResult result =
        ghostty_terminal_selection_format_buf(term->terminal, options, (uint8_t*)buffer, capacity, &written);
    if (result != GHOSTTY_SUCCESS && result != GHOSTTY_OUT_OF_SPACE) {
        return 0;
    }
    return (uint32_t)written;
}

uint32_t brixTerminalCopySelection(void* terminal, void* buffer, uint32_t capacity) {
    return formatSelection((BxTerminal*)terminal, NULL, buffer, capacity);
}

uint32_t brixTerminalCopyAll(void* terminal, void* buffer, uint32_t capacity) {
    BxTerminal* term = (BxTerminal*)terminal;
    GhosttySelection selection = GHOSTTY_INIT_SIZED(GhosttySelection);
    if (ghostty_terminal_select_all(term->terminal, &selection) != GHOSTTY_SUCCESS) {
        return 0;
    }
    return formatSelection(term, &selection, buffer, capacity);
}
