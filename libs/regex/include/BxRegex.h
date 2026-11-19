#ifndef BX_REGEX_H
#define BX_REGEX_H

#include <stdbool.h>
#include <stdint.h>

#include <BxGC/BxGCAnnotations.h>

#ifdef __cplusplus
#define BX_REGEX_API extern "C"
#else
#define BX_REGEX_API extern
#endif

#define BX_REGEX_IGNORES_CASE 1
#define BX_REGEX_MULTILINE 2
#define BX_REGEX_DOT_ALL 4

#define BX_REGEX_REPLACE_GLOBAL 1
#define BX_REGEX_REPLACE_ONLY 2

#define BX_REGEX_STATE_OFFSET 0
#define BX_REGEX_STATE_OPTIONS 1
#define BX_REGEX_STATE_CURSOR_OFFSET 2
#define BX_REGEX_STATE_CURSOR_INDEX 3
#define BX_REGEX_STATE_FINISHED 4
#define BX_REGEX_STATE_SIZE 5

BX_REGEX_API int32_t brixRegexCompile(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    int32_t* error,
    uint32_t* errorOffset
);

BX_REGEX_API int32_t brixRegexFind(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* text,
    uint32_t textLength,
    uint32_t start,
    uint32_t* ranges,
    uint32_t capacity
);

BX_REGEX_API int32_t brixRegexFindAll(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* text,
    uint32_t textLength,
    uint32_t* state,
    uint32_t* ranges,
    uint32_t capacity
);

BX_REGEX_API int32_t brixRegexGroupNumber(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* name,
    uint32_t nameLength
);

BX_REGEX_API int32_t brixRegexReplace(
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
);

BX_REGEX_API BRIX_WITHOUT_GC uint32_t brixRegexErrorMessage(int32_t code, void* buffer, uint32_t capacity);
BX_REGEX_API BRIX_WITHOUT_GC bool brixRegexIsLimitError(int32_t code);

#endif
