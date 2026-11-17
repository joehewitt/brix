#ifndef BX_STRING_H
#define BX_STRING_H

#include <cstdint>
#include <cstddef>
#include <BxGC/BxGCAnnotations.h>

// **********************************************************************************************

// A Brix String value points at the first UTF-8 byte. A fixed, naturally aligned header lives
// immediately before that data pointer. The data remains contiguous and NUL-terminated so native
// APIs continue to receive an ordinary `char*`. GC roots may contain the data pointer: the
// collector's existing interior-pointer lookup recovers the allocation base at the header.
//
// byteLength excludes the terminator. characterLength counts Unicode scalar/codepoint values,
// which is Brix's current Char unit. String construction rejects allocation-size overflow and
// character counts above UINT32_MAX.
typedef struct alignas(8) {
    uint64_t byteLength;
    uint32_t characterLength;
    uint32_t flags;
} BrixStringHeader;

enum : uint32_t {
    BRIX_STRING_FLAG_ASCII = 1u << 0,
    BRIX_STRING_FLAG_VALID_UTF8 = 1u << 1,
    BRIX_STRING_HEADER_SIZE = sizeof(BrixStringHeader)
};

static_assert(sizeof(BrixStringHeader) == 16, "Brix String header ABI changed");
static_assert(alignof(BrixStringHeader) == 8, "Brix String header alignment changed");

// Opaque only to the C-header importer: pointers to this type are compiler-level String values.
// The pointer value itself is still the first UTF-8 byte and is ABI-compatible with `char*`.
struct BrixStringData;

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

extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringCreate(
    const char* bytes, size_t byteLength);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringCreateValidated(
    const char* bytes, size_t byteLength);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringCreateASCII(
    const char* bytes, size_t byteLength);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringAdopt(const char* bytes);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringConcat(
    const char* left, const char* right);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringRepeat(
    const char* value, size_t count);
extern "C" BRIX_WITHOUT_GC bool brixStringFind(
    const char* self, const char* findStr, bool reverse, int start, int* outIndex);
extern "C" BRIX_WITHOUT_GC bool brixStringFindBytes(
    const char* self, const char* findStr, bool reverse, int start, int* outIndex);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringCopy(const char* value);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringSlice(
    char* self, size_t begin, size_t end);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringSliceUtf8(
    const char* bytes, size_t begin, size_t end);

extern "C" BRIX_WITHOUT_GC int brixStringGetCharAtIndex(
    const char* str, size_t index, ResultCharOffset* ret);
extern "C" BRIX_WITHOUT_GC int brixStringGetNextChar(
    const char* str, ResultCharOffset* ret);

extern "C" BRIX_WITHOUT_GC bool brixStringIsEmpty(const char* str);
extern "C" BRIX_WITHOUT_GC uint64_t brixStringGetByteLength(const char* str);
extern "C" BRIX_WITHOUT_GC uint32_t brixStringGetLength(const char* str);
extern "C" BRIX_WITHOUT_GC uint32_t brixStringGetFlags(const char* str);
extern "C" BRIX_WITHOUT_GC bool brixStringEquals(const char* left, const char* right);
extern "C" BRIX_WITHOUT_GC int brixStringCompare(const char* left, const char* right);
extern "C" BRIX_WITHOUT_GC uint32_t brixStringGetDisplayWidth(const char* str);
extern "C" BRIX_WITHOUT_BLOCKING uint8_t brixStringMeasureDisplayWidth(
    const char* str, ResultDisplayWidth* ret);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringToUppercase(const char* input);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringToLowercase(const char* input);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringDecompose(const char* input);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringNormalize(const char* str);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringFromUtf8Codepoint(uint32_t c);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixStringFromUtf8Codepoints(
    const uint32_t* self, size_t start, size_t end);
extern "C" BRIX_WITHOUT_GC uint8_t brixStringValidateUtf8(
    const char* input, size_t inputSize);

extern "C" BRIX_WITHOUT_GC uint8_t brixStringSliceToUtf8Codepoints(
    const char* input, size_t inputSize, uint32_t* output, size_t outputSize,
    ResultDecodeUTF8* ret);

extern "C" BRIX_WITHOUT_BLOCKING uint8_t brixStringToUtf8Codepoints(
    const char* str, ResultDecodeUTF8* ret);

extern "C" BRIX_WITHOUT_GC uint32_t brixCharGetCategory(uint32_t c);
extern "C" BRIX_WITHOUT_GC uint32_t brixCharGetDisplayWidth(uint32_t c);
extern "C" BRIX_WITHOUT_GC uint32_t brixCharToUppercase(uint32_t c);
extern "C" BRIX_WITHOUT_GC uint32_t brixCharToLowercase(uint32_t c);
extern "C" BRIX_WITHOUT_BLOCKING const BrixStringData* brixCharToString(char c);

#endif
