#include "include/BxRegex.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define BX_REGEX_CACHE_SIZE 128
#define BX_REGEX_MATCH_LIMIT 1000000
#define BX_REGEX_HEAP_LIMIT 65536

typedef struct {
    pcre2_code* code;
    uint32_t groupCount;
    atomic_uint references;
} BxRegexCode;

typedef struct {
    char* pattern;
    uint32_t length;
    uint32_t flags;
    uint64_t hash;
    uint64_t used;
    BxRegexCode* code;
} BxRegexEntry;

typedef struct {
    const char* text;
    uint32_t length;
    uint32_t offset;
    uint32_t index;
} BxRegexCursor;

static BxRegexEntry brixRegexCache[BX_REGEX_CACHE_SIZE];
static uint64_t brixRegexClock = 0;
static pthread_mutex_t brixRegexLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t brixRegexContextOnce = PTHREAD_ONCE_INIT;
static pcre2_match_context* brixRegexMatchContext = NULL;

static bool brixRegexIsLead(uint8_t byte) {
    return (byte & 0xC0) != 0x80;
}

static void brixRegexAdvance(BxRegexCursor* cursor, uint32_t index) {
    while (cursor->offset < cursor->length) {
        if (brixRegexIsLead((uint8_t)cursor->text[cursor->offset])) {
            if (cursor->index == index) {
                return;
            }
            cursor->index++;
        }
        cursor->offset++;
    }
}

static void brixRegexSeek(BxRegexCursor* cursor, uint32_t offset) {
    while (cursor->offset < offset) {
        if (brixRegexIsLead((uint8_t)cursor->text[cursor->offset])) {
            cursor->index++;
        }
        cursor->offset++;
    }
    while (cursor->offset > offset) {
        cursor->offset--;
        if (brixRegexIsLead((uint8_t)cursor->text[cursor->offset])) {
            cursor->index--;
        }
    }
}

static uint32_t brixRegexIndexAt(const char* text, uint32_t length, uint32_t offset) {
    BxRegexCursor cursor = {text, length, 0, 0};
    brixRegexSeek(&cursor, offset < length ? offset : length);
    return cursor.index;
}

static void brixRegexMakeContext(void) {
    brixRegexMatchContext = pcre2_match_context_create(NULL);
    if (brixRegexMatchContext) {
        pcre2_set_match_limit(brixRegexMatchContext, BX_REGEX_MATCH_LIMIT);
        pcre2_set_heap_limit(brixRegexMatchContext, BX_REGEX_HEAP_LIMIT);
    }
}

static pcre2_match_context* brixRegexContext(void) {
    pthread_once(&brixRegexContextOnce, brixRegexMakeContext);
    return brixRegexMatchContext;
}

static uint64_t brixRegexHash(const char* pattern, uint32_t length, uint32_t flags) {
    uint64_t hash = 14695981039346656037ull ^ flags;
    for (uint32_t index = 0; index < length; index++) {
        hash ^= (uint8_t)pattern[index];
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint32_t brixRegexOptions(uint32_t flags) {
    uint32_t options = PCRE2_UTF | PCRE2_UCP | PCRE2_MATCH_INVALID_UTF | PCRE2_NEVER_BACKSLASH_C;
    if (flags & BX_REGEX_IGNORES_CASE) {
        options |= PCRE2_CASELESS;
    }
    if (flags & BX_REGEX_MULTILINE) {
        options |= PCRE2_MULTILINE;
    }
    if (flags & BX_REGEX_DOT_ALL) {
        options |= PCRE2_DOTALL;
    }
    return options;
}

static void brixRegexRelease(BxRegexCode* code) {
    if (atomic_fetch_sub(&code->references, 1) == 1) {
        pcre2_code_free(code->code);
        free(code);
    }
}

static BxRegexCode* brixRegexLookup(const char* pattern, uint32_t length, uint32_t flags, uint64_t hash) {
    for (uint32_t slot = 0; slot < BX_REGEX_CACHE_SIZE; slot++) {
        BxRegexEntry* entry = &brixRegexCache[slot];
        if (entry->code && entry->hash == hash && entry->flags == flags && entry->length == length &&
            memcmp(entry->pattern, pattern, length) == 0) {
            entry->used = ++brixRegexClock;
            atomic_fetch_add(&entry->code->references, 1);
            return entry->code;
        }
    }
    return NULL;
}

static void brixRegexStore(BxRegexCode* code, char* pattern, uint32_t length, uint32_t flags, uint64_t hash) {
    BxRegexEntry* chosen = &brixRegexCache[0];
    for (uint32_t slot = 0; slot < BX_REGEX_CACHE_SIZE; slot++) {
        BxRegexEntry* entry = &brixRegexCache[slot];
        if (!entry->code) {
            chosen = entry;
            break;
        }
        if (entry->used < chosen->used) {
            chosen = entry;
        }
    }
    if (chosen->code) {
        brixRegexRelease(chosen->code);
        free(chosen->pattern);
    }
    chosen->pattern = pattern;
    chosen->length = length;
    chosen->flags = flags;
    chosen->hash = hash;
    chosen->used = ++brixRegexClock;
    chosen->code = code;
}

static BxRegexCode* brixRegexAcquire(
    const char* pattern,
    uint32_t length,
    uint32_t flags,
    int32_t* error,
    uint32_t* errorOffset
) {
    uint64_t hash = brixRegexHash(pattern, length, flags);
    pthread_mutex_lock(&brixRegexLock);
    BxRegexCode* found = brixRegexLookup(pattern, length, flags, hash);
    pthread_mutex_unlock(&brixRegexLock);
    if (found) {
        return found;
    }

    int status = 0;
    PCRE2_SIZE offset = 0;
    pcre2_code* compiled = pcre2_compile(
        (PCRE2_SPTR)pattern, length, brixRegexOptions(flags), &status, &offset, NULL);
    if (!compiled) {
        *error = status;
        *errorOffset = brixRegexIndexAt(pattern, length, (uint32_t)offset);
        return NULL;
    }
    BxRegexCode* made = malloc(sizeof(BxRegexCode));
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

    pthread_mutex_lock(&brixRegexLock);
    found = brixRegexLookup(pattern, length, flags, hash);
    if (!found) {
        brixRegexStore(made, copy, length, flags, hash);
    }
    pthread_mutex_unlock(&brixRegexLock);
    if (found) {
        pcre2_code_free(compiled);
        free(made);
        free(copy);
        return found;
    }
    return made;
}

int32_t brixRegexCompile(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    int32_t* error,
    uint32_t* errorOffset
) {
    BxRegexCode* code = brixRegexAcquire(pattern, patternLength, flags, error, errorOffset);
    if (!code) {
        return -1;
    }
    int32_t groupCount = (int32_t)code->groupCount;
    brixRegexRelease(code);
    return groupCount;
}

int32_t brixRegexFind(
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
    BxRegexCode* code = brixRegexAcquire(pattern, patternLength, flags, &error, &errorOffset);
    if (!code) {
        return PCRE2_ERROR_NOMEMORY;
    }
    pcre2_match_data* data = pcre2_match_data_create_from_pattern(code->code, NULL);
    if (!data) {
        brixRegexRelease(code);
        return PCRE2_ERROR_NOMEMORY;
    }

    BxRegexCursor cursor = {text, textLength, 0, 0};
    brixRegexAdvance(&cursor, start);
    PCRE2_SIZE offset = cursor.offset;
    uint32_t options = 0;
    uint32_t stride = (code->groupCount + 1) * 2;
    uint32_t count = 0;
    int32_t result = 0;
    while (limit == 0 || count < limit) {
        int status = pcre2_match(
            code->code, (PCRE2_SPTR)text, textLength, offset, options, data, brixRegexContext());
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
                brixRegexSeek(&cursor, (uint32_t)first);
                found[group * 2] = cursor.index;
                brixRegexSeek(&cursor, (uint32_t)last);
                found[group * 2 + 1] = cursor.index;
            }
        }
        count++;
        if (!pcre2_next_match(data, &offset, &options)) {
            break;
        }
    }
    pcre2_match_data_free(data);
    brixRegexRelease(code);
    return result < 0 ? result : (int32_t)count;
}

int32_t brixRegexGroupNumber(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* name,
    uint32_t nameLength
) {
    int32_t error = 0;
    uint32_t errorOffset = 0;
    BxRegexCode* code = brixRegexAcquire(pattern, patternLength, flags, &error, &errorOffset);
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
    brixRegexRelease(code);
    return number < 0 ? -1 : number;
}

int32_t brixRegexReplace(
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
    BxRegexCode* code = brixRegexAcquire(pattern, patternLength, flags, &error, &compileOffset);
    if (!code) {
        return PCRE2_ERROR_NOMEMORY;
    }
    uint32_t options = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | PCRE2_SUBSTITUTE_UNSET_EMPTY;
    if (mode & BX_REGEX_REPLACE_GLOBAL) {
        options |= PCRE2_SUBSTITUTE_GLOBAL;
    }
    if (mode & BX_REGEX_REPLACE_ONLY) {
        options |= PCRE2_SUBSTITUTE_REPLACEMENT_ONLY;
    }
    BxRegexCursor cursor = {text, textLength, 0, 0};
    brixRegexAdvance(&cursor, start);
    PCRE2_SIZE length = capacity;
    int status = pcre2_substitute(
        code->code,
        (PCRE2_SPTR)text,
        textLength,
        cursor.offset,
        options,
        NULL,
        brixRegexContext(),
        (PCRE2_SPTR)replacement,
        replacementLength,
        (PCRE2_UCHAR*)output,
        &length);
    brixRegexRelease(code);
    if (status == PCRE2_ERROR_NOMEMORY && length > capacity) {
        *outputLength = (uint32_t)length;
        return 0;
    }
    if (status < 0) {
        if (length != PCRE2_UNSET && length <= replacementLength) {
            *errorOffset = (int32_t)brixRegexIndexAt(replacement, replacementLength, (uint32_t)length);
        }
        return status;
    }
    *outputLength = (uint32_t)length;
    return status;
}

uint32_t brixRegexErrorMessage(int32_t code, void* buffer, uint32_t capacity) {
    if (!capacity) {
        return 0;
    }
    pcre2_get_error_message(code, (PCRE2_UCHAR*)buffer, capacity);
    return (uint32_t)strnlen((const char*)buffer, capacity);
}

bool brixRegexIsLimitError(int32_t code) {
    return code == PCRE2_ERROR_MATCHLIMIT || code == PCRE2_ERROR_DEPTHLIMIT ||
        code == PCRE2_ERROR_HEAPLIMIT;
}
