#ifndef YU_REGEX_H
#define YU_REGEX_H

#include <stdbool.h>
#include <stdint.h>

#include <YuGC/YuGCAnnotations.h>

#ifdef __cplusplus
#define YU_REGEX_API extern "C"
#else
#define YU_REGEX_API extern
#endif

#define YU_REGEX_IGNORES_CASE 1
#define YU_REGEX_MULTILINE 2
#define YU_REGEX_DOT_ALL 4

#define YU_REGEX_REPLACE_GLOBAL 1
#define YU_REGEX_REPLACE_ONLY 2

YU_REGEX_API int32_t yumiRegexCompile(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    int32_t* error,
    uint32_t* errorOffset
);

YU_REGEX_API int32_t yumiRegexFind(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* text,
    uint32_t textLength,
    uint32_t start,
    uint32_t limit,
    uint32_t* ranges,
    uint32_t capacity
);

YU_REGEX_API int32_t yumiRegexGroupNumber(
    const char* pattern,
    uint32_t patternLength,
    uint32_t flags,
    const char* name,
    uint32_t nameLength
);

YU_REGEX_API int32_t yumiRegexReplace(
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

YU_REGEX_API YUMI_WITHOUT_GC uint32_t yumiRegexErrorMessage(int32_t code, void* buffer, uint32_t capacity);
YU_REGEX_API YUMI_WITHOUT_GC bool yumiRegexIsLimitError(int32_t code);

#endif
