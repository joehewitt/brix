#ifndef YU_STRING_H
#define YU_STRING_H

#include <cstdint>
#include <cstddef>
#include <YuGC/YuGCAnnotations.h>

// **********************************************************************************************

// A Yumi String value points at the first UTF-8 byte. A fixed, naturally aligned header lives
// immediately before that data pointer. The data remains contiguous and NUL-terminated so native
// APIs continue to receive an ordinary `char*`. GC roots may contain the data pointer: the
// collector's existing interior-pointer lookup recovers the allocation base at the header.
//
// byteLength excludes the terminator. characterLength counts Unicode scalar/codepoint values,
// which is Yumi's current Char unit. String construction rejects allocation-size overflow and
// character counts above UINT32_MAX.
typedef struct alignas(8) {
    uint64_t byteLength;
    uint32_t characterLength;
    uint32_t flags;
} YumiStringHeader;

enum : uint32_t {
    YUMI_STRING_FLAG_ASCII = 1u << 0,
    YUMI_STRING_FLAG_VALID_UTF8 = 1u << 1,
    YUMI_STRING_HEADER_SIZE = sizeof(YumiStringHeader)
};

static_assert(sizeof(YumiStringHeader) == 16, "Yumi String header ABI changed");
static_assert(alignof(YumiStringHeader) == 8, "Yumi String header alignment changed");

// Opaque only to the C-header importer: pointers to this type are compiler-level String values.
// The pointer value itself is still the first UTF-8 byte and is ABI-compatible with `char*`.
struct YumiStringData;

// **********************************************************************************************

typedef struct {
    int32_t bytes;
    uint32_t codepoint;
} ResultCharOffset;

typedef struct {
    uint32_t* codepoints;
    size_t codepointsDecoded;
    size_t bytesDecoded;
} ResultDecodeUTF8;

typedef struct {
    uint8_t* widths;
    size_t displayWidth;
    size_t codepointsDecoded;
    size_t bytesDecoded;
    size_t flags;
} ResultDisplayWidth;

// **********************************************************************************************

extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringCreate(
    const char* bytes, size_t byteLength);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringCreateValidated(
    const char* bytes, size_t byteLength);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringCreateASCII(
    const char* bytes, size_t byteLength);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringAdopt(const char* bytes);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringConcat(
    const char* left, const char* right);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringRepeat(
    const char* value, size_t count);
extern "C" YUMI_WITHOUT_GC bool yumiStringFind(
    const char* self, const char* findStr, bool reverse, int start, int* outIndex);
extern "C" YUMI_WITHOUT_GC bool yumiStringFindBytes(
    const char* self, const char* findStr, bool reverse, int start, int* outIndex);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringCopy(const char* value);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringSlice(
    char* self, size_t begin, size_t end);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringSliceUtf8(
    const char* bytes, size_t begin, size_t end);

extern "C" YUMI_WITHOUT_GC int yumiStringGetCharAtIndex(
    const char* str, size_t index, ResultCharOffset* ret);
extern "C" YUMI_WITHOUT_GC int yumiStringGetNextChar(
    const char* str, ResultCharOffset* ret);

extern "C" YUMI_WITHOUT_GC bool yumiStringIsEmpty(const char* str);
extern "C" YUMI_WITHOUT_GC uint64_t yumiStringGetByteLength(const char* str);
extern "C" YUMI_WITHOUT_GC uint32_t yumiStringGetLength(const char* str);
extern "C" YUMI_WITHOUT_GC uint32_t yumiStringGetFlags(const char* str);
extern "C" YUMI_WITHOUT_GC bool yumiStringEquals(const char* left, const char* right);
extern "C" YUMI_WITHOUT_GC int yumiStringCompare(const char* left, const char* right);
extern "C" YUMI_WITHOUT_GC uint32_t yumiStringGetDisplayWidth(const char* str);
extern "C" YUMI_WITHOUT_BLOCKING uint8_t yumiStringMeasureDisplayWidth(
    const char* str, ResultDisplayWidth* ret);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringToUppercase(const char* input);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringToLowercase(const char* input);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringDecompose(const char* input);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringNormalize(const char* str);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringFromUtf8Codepoint(uint32_t c);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiStringFromUtf8Codepoints(
    const uint32_t* self, size_t start, size_t end);
extern "C" YUMI_WITHOUT_GC uint8_t yumiStringValidateUtf8(
    const char* input, size_t inputSize);

extern "C" YUMI_WITHOUT_GC uint8_t yumiStringSliceToUtf8Codepoints(
    const char* input, size_t inputSize, uint32_t* output, size_t outputSize,
    ResultDecodeUTF8* ret);

extern "C" YUMI_WITHOUT_BLOCKING uint8_t yumiStringToUtf8Codepoints(
    const char* str, ResultDecodeUTF8* ret);

extern "C" YUMI_WITHOUT_GC uint32_t yumiCharGetCategory(uint32_t c);
extern "C" YUMI_WITHOUT_GC uint32_t yumiCharGetDisplayWidth(uint32_t c);
extern "C" YUMI_WITHOUT_GC uint32_t yumiCharToUppercase(uint32_t c);
extern "C" YUMI_WITHOUT_GC uint32_t yumiCharToLowercase(uint32_t c);
extern "C" YUMI_WITHOUT_BLOCKING const YumiStringData* yumiCharToString(char c);

#endif
