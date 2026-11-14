#include "include/YuRegex.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define YU_REGEX_CACHE_SIZE 128
#define YU_REGEX_MATCH_LIMIT 1000000
#define YU_REGEX_HEAP_LIMIT 65536

typedef struct {
    pcre2_code* code;
    uint32_t groupCount;
    atomic_uint references;
} YuRegexCode;

typedef struct {
    char* pattern;
    uint32_t length;
    uint32_t flags;
    uint64_t hash;
    uint64_t used;
    YuRegexCode* code;
} YuRegexEntry;

typedef struct {
    const char* text;
    uint32_t length;
    uint32_t offset;
    uint32_t index;
} YuRegexCursor;

static YuRegexEntry yumiRegexCache[YU_REGEX_CACHE_SIZE];
static uint64_t yumiRegexClock = 0;
static pthread_mutex_t yumiRegexLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t yumiRegexContextOnce = PTHREAD_ONCE_INIT;
static pcre2_match_context* yumiRegexMatchContext = NULL;

static bool yumiRegexIsLead(uint8_t byte) {
    return (byte & 0xC0) != 0x80;
}

static void yumiRegexAdvance(YuRegexCursor* cursor, uint32_t index) {
    while (cursor->offset < cursor->length) {
        if (yumiRegexIsLead((uint8_t)cursor->text[cursor->offset])) {
            if (cursor->index == index) {
                return;
            }
            cursor->index++;
        }
        cursor->offset++;
    }
}

static void yumiRegexSeek(YuRegexCursor* cursor, uint32_t offset) {
    while (cursor->offset < offset) {
        if (yumiRegexIsLead((uint8_t)cursor->text[cursor->offset])) {
            cursor->index++;
        }
        cursor->offset++;
    }
    while (cursor->offset > offset) {
        cursor->offset--;
        if (yumiRegexIsLead((uint8_t)cursor->text[cursor->offset])) {
            cursor->index--;
        }
    }
}

static uint32_t yumiRegexIndexAt(const char* text, uint32_t length, uint32_t offset) {
    YuRegexCursor cursor = {text, length, 0, 0};
    yumiRegexSeek(&cursor, offset < length ? offset : length);
    return cursor.index;
}

static void yumiRegexMakeContext(void) {
    yumiRegexMatchContext = pcre2_match_context_create(NULL);
    if (yumiRegexMatchContext) {
        pcre2_set_match_limit(yumiRegexMatchContext, YU_REGEX_MATCH_LIMIT);
        pcre2_set_heap_limit(yumiRegexMatchContext, YU_REGEX_HEAP_LIMIT);
    }
}

static pcre2_match_context* yumiRegexContext(void) {
    pthread_once(&yumiRegexContextOnce, yumiRegexMakeContext);
    return yumiRegexMatchContext;
}

static uint64_t yumiRegexHash(const char* pattern, uint32_t length, uint32_t flags) {
    uint64_t hash = 14695981039346656037ull ^ flags;
    for (uint32_t index = 0; index < length; index++) {
        hash ^= (uint8_t)pattern[index];
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint32_t yumiRegexOptions(uint32_t flags) {
    uint32_t options = PCRE2_UTF | PCRE2_UCP | PCRE2_MATCH_INVALID_UTF | PCRE2_NEVER_BACKSLASH_C;
    if (flags & YU_REGEX_IGNORES_CASE) {
        options |= PCRE2_CASELESS;
    }
    if (flags & YU_REGEX_MULTILINE) {
        options |= PCRE2_MULTILINE;
    }
    if (flags & YU_REGEX_DOT_ALL) {
        options |= PCRE2_DOTALL;
    }
    return options;
}

static void yumiRegexRelease(YuRegexCode* code) {
    if (atomic_fetch_sub(&code->references, 1) == 1) {
        pcre2_code_free(code->code);
        free(code);
    }
}

static YuRegexCode* yumiRegexLookup(const char* pattern, uint32_t length, uint32_t flags, uint64_t hash) {
    for (uint32_t slot = 0; slot < YU_REGEX_CACHE_SIZE; slot++) {
        YuRegexEntry* entry = &yumiRegexCache[slot];
        if (entry->code && entry->hash == hash && entry->flags == flags && entry->length == length &&
            memcmp(entry->pattern, pattern, length) == 0) {
            entry->used = ++yumiRegexClock;
            atomic_fetch_add(&entry->code->references, 1);
            return entry->code;
        }
    }
    return NULL;
}

static void yumiRegexStore(YuRegexCode* code, char* pattern, uint32_t length, uint32_t flags, uint64_t hash) {
    YuRegexEntry* chosen = &yumiRegexCache[0];
    for (uint32_t slot = 0; slot < YU_REGEX_CACHE_SIZE; slot++) {
        YuRegexEntry* entry = &yumiRegexCache[slot];
        if (!entry->code) {
            chosen = entry;
            break;
        }
        if (entry->used < chosen->used) {
            chosen = entry;
        }
    }
    if (chosen->code) {
        yumiRegexRelease(chosen->code);
        free(chosen->pattern);
    }
    chosen->pattern = pattern;
    chosen->length = length;
    chosen->flags = flags;
    chosen->hash = hash;
    chosen->used = ++yumiRegexClock;
    chosen->code = code;
}

static YuRegexCode* yumiRegexAcquire(
    const char* pattern,
    uint32_t length,
    uint32_t flags,
    int32_t* error,
    uint32_t* errorOffset
) {
    uint64_t hash = yumiRegexHash(pattern, length, flags);
    pthread_mutex_lock(&yumiRegexLock);
    YuRegexCode* found = yumiRegexLookup(pattern, length, flags, hash);
    pthread_mutex_unlock(&yumiRegexLock);
    if (found) {
        return found;
    }

    int status = 0;
    PCRE2_SIZE offset = 0;
    pcre2_code* compiled = pcre2_compile(
        (PCRE2_SPTR)pattern, length, yumiRegexOptions(flags), &status, &offset, NULL);
    if (!compiled) {
        *error = status;
        *errorOffset = yumiRegexIndexAt(pattern, length, (uint32_t)offset);
        return NULL;
    }
    YuRegexCode* made = malloc(sizeof(YuRegexCode));
    char* copy = malloc(length ? length : 1);
    if (!made || !copy) {
        free(made);
        free(copy);
        pcre2_code_free(compiled);
        *error = PCRE2_ERROR_NOMEMORY;
        *errorOffset = 0;
        return NULL;
    }
    made->code = compiled;
    made->groupCount = 0;
    pcre2_pattern_info(compiled, PCRE2_INFO_CAPTURECOUNT, &made->groupCount);
    atomic_init(&made->references, 2);
    memcpy(copy, pattern, length);

    pthread_mutex_lock(&yumiRegexLock);
    found = yumiRegexLookup(pattern, length, flags, hash);
    if (!found) {
        yumiRegexStore(made, copy, length, flags, hash);
    }
    pthread_mutex_unlock(&yumiRegexLock);
    if (found) {
        pcre2_code_free(compiled);
        free(made);
        free(copy);
        return found;
    }
    return made;
}

int32_t yumiRegexCompile(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    int32_t* error,
    uint32_t* errorOffset
) {
    YuRegexCode* code = yumiRegexAcquire(pattern, patternLength, flags, error, errorOffset);
    if (!code) {
        return -1;
    }
    int32_t groupCount = (int32_t)code->groupCount;
    yumiRegexRelease(code);
    return groupCount;
}

int32_t yumiRegexFind(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* text,
    uint32_t textLength,
    uint32_t start,
    uint32_t limit,
    uint32_t* ranges,
    uint32_t capacity
) {
    int32_t error = 0;
    uint32_t errorOffset = 0;
    YuRegexCode* code = yumiRegexAcquire(pattern, patternLength, flags, &error, &errorOffset);
    if (!code) {
        return PCRE2_ERROR_NOMEMORY;
    }
    pcre2_match_data* data = pcre2_match_data_create_from_pattern(code->code, NULL);
    if (!data) {
        yumiRegexRelease(code);
        return PCRE2_ERROR_NOMEMORY;
    }

    YuRegexCursor cursor = {text, textLength, 0, 0};
    yumiRegexAdvance(&cursor, start);
    PCRE2_SIZE offset = cursor.offset;
    uint32_t options = 0;
    uint32_t stride = (code->groupCount + 1) * 2;
    uint32_t count = 0;
    int32_t result = 0;
    while (limit == 0 || count < limit) {
        int status = pcre2_match(
            code->code, (PCRE2_SPTR)text, textLength, offset, options, data, yumiRegexContext());
        if (status == PCRE2_ERROR_NOMATCH) {
            break;
        }
        if (status < 0) {
            result = status;
            break;
        }
        if ((uint64_t)(count + 1) * stride <= capacity) {
            PCRE2_SIZE* vector = pcre2_get_ovector_pointer(data);
            uint32_t* found = ranges + count * stride;
            for (uint32_t group = 0; group <= code->groupCount; group++) {
                PCRE2_SIZE first = vector[group * 2];
                PCRE2_SIZE last = vector[group * 2 + 1];
                if (first == PCRE2_UNSET) {
                    found[group * 2] = UINT32_MAX;
                    found[group * 2 + 1] = UINT32_MAX;
                    continue;
                }
                yumiRegexSeek(&cursor, (uint32_t)first);
                found[group * 2] = cursor.index;
                yumiRegexSeek(&cursor, (uint32_t)last);
                found[group * 2 + 1] = cursor.index;
            }
        }
        count++;
        if (!pcre2_next_match(data, &offset, &options)) {
            break;
        }
    }
    pcre2_match_data_free(data);
    yumiRegexRelease(code);
    return result < 0 ? result : (int32_t)count;
}

int32_t yumiRegexGroupNumber(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* name,
    uint32_t nameLength
) {
    int32_t error = 0;
    uint32_t errorOffset = 0;
    YuRegexCode* code = yumiRegexAcquire(pattern, patternLength, flags, &error, &errorOffset);
    if (!code) {
        return -1;
    }
    char* copy = malloc(nameLength + 1);
    int number = -1;
    if (copy) {
        memcpy(copy, name, nameLength);
        copy[nameLength] = 0;
        number = pcre2_substring_number_from_name(code->code, (PCRE2_SPTR)copy);
        free(copy);
    }
    yumiRegexRelease(code);
    return number < 0 ? -1 : number;
}

int32_t yumiRegexReplace(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* text,
    uint32_t textLength,
    uint32_t start,
    const char* replacement,
    uint32_t replacementLength,
    uint32_t mode,
    void* output,
    uint32_t capacity,
    uint32_t* outputLength,
    int32_t* errorOffset
) {
    int32_t error = 0;
    uint32_t compileOffset = 0;
    *outputLength = 0;
    *errorOffset = -1;
    YuRegexCode* code = yumiRegexAcquire(pattern, patternLength, flags, &error, &compileOffset);
    if (!code) {
        return PCRE2_ERROR_NOMEMORY;
    }
    uint32_t options = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | PCRE2_SUBSTITUTE_UNSET_EMPTY;
    if (mode & YU_REGEX_REPLACE_GLOBAL) {
        options |= PCRE2_SUBSTITUTE_GLOBAL;
    }
    if (mode & YU_REGEX_REPLACE_ONLY) {
        options |= PCRE2_SUBSTITUTE_REPLACEMENT_ONLY;
    }
    YuRegexCursor cursor = {text, textLength, 0, 0};
    yumiRegexAdvance(&cursor, start);
    PCRE2_SIZE length = capacity;
    int status = pcre2_substitute(
        code->code,
        (PCRE2_SPTR)text,
        textLength,
        cursor.offset,
        options,
        NULL,
        yumiRegexContext(),
        (PCRE2_SPTR)replacement,
        replacementLength,
        (PCRE2_UCHAR*)output,
        &length);
    yumiRegexRelease(code);
    if (status == PCRE2_ERROR_NOMEMORY && length > capacity) {
        *outputLength = (uint32_t)length;
        return 0;
    }
    if (status < 0) {
        if (length != PCRE2_UNSET && length <= replacementLength) {
            *errorOffset = (int32_t)yumiRegexIndexAt(replacement, replacementLength, (uint32_t)length);
        }
        return status;
    }
    *outputLength = (uint32_t)length;
    return status;
}

uint32_t yumiRegexErrorMessage(int32_t code, void* buffer, uint32_t capacity) {
    if (!capacity) {
        return 0;
    }
    pcre2_get_error_message(code, (PCRE2_UCHAR*)buffer, capacity);
    return (uint32_t)strnlen((const char*)buffer, capacity);
}

bool yumiRegexIsLimitError(int32_t code) {
    return code == PCRE2_ERROR_MATCHLIMIT || code == PCRE2_ERROR_DEPTHLIMIT ||
        code == PCRE2_ERROR_HEAPLIMIT;
}
