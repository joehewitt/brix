
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <cfloat>
#include <utf8proc/utf8proc.h>

#include "BxCore/BxCore.h"
#include "BxCore/BxString.h"
#include <BxGC/BxGCRuntime.h>

// *************************************************************************************************

static const uint8_t kDecodeOk = 0;
static const uint8_t kDecodeError = 1;
static const uint8_t kDecodeOutputFull = 2;
static const size_t kDisplayWidthHasWidths = 1;

static inline BrixStringHeader*
_stringHeader(char* data) {
    return (BrixStringHeader*)(data - sizeof(BrixStringHeader));
}

static inline const BrixStringHeader*
_stringHeader(const char* data) {
    return (const BrixStringHeader*)(data - sizeof(BrixStringHeader));
}

static inline const BrixStringData*
_stringValue(const char* data) {
    return (const BrixStringData*)data;
}

static void
_stringAllocationFailed(const char* reason) {
    fprintf(stderr, "%s\n", reason);
    exit(1);
}

class BrixStringTemporaryRoot {
public:
    explicit BrixStringTemporaryRoot(const char* pointer) {
        if (pointer) {
            root = brixAddTemporaryRoot((char*)pointer);
        }
    }

    ~BrixStringTemporaryRoot() {
        if (root) {
            brixRemoveTemporaryRoot(root);
        }
    }

    BrixStringTemporaryRoot(const BrixStringTemporaryRoot&) = delete;
    BrixStringTemporaryRoot& operator=(const BrixStringTemporaryRoot&) = delete;

private:
    void* root = nullptr;
};

static inline const char*
_managedSource(const char* bytes) {
#if BX_ENABLE_BRIX_GC
    return brixGCIsManagedAddress((char*)bytes) ? bytes : nullptr;
#else
    return nullptr;
#endif
}

static inline void
_recordAllocationSite(
    size_t siteId,
    const char* source,
    size_t line,
    size_t column,
    const char* function,
    const char* type,
    const char* operation,
    const char* pointerKind
) {
    if (brixBenchMetricsEnabledFlag) {
        brixBenchRecordAllocationSite(
            siteId, source, line, column, function, type, operation, pointerKind);
    }
}

static char*
_allocBytes(size_t size) {
    size_t dataCapacity = size ? size : 1;
    if (dataCapacity > SIZE_MAX - sizeof(BrixStringHeader)) {
        _stringAllocationFailed("String allocation size overflow");
    }
    char* allocation = (char*)brixAllocBuffer(1, sizeof(BrixStringHeader) + dataCapacity);
    if (!allocation) {
        _stringAllocationFailed("String allocation failed");
    }
    _recordAllocationSite(
        0xC0DE0001, "libs/core/BxString.cpp", 74, 0,
        "_allocBytes", "String", "string-data", "descriptorless");
    return allocation + sizeof(BrixStringHeader);
}

static char*
_allocBytesKeeping(size_t size, const char* first, const char* second = nullptr) {
    size_t dataCapacity = size ? size : 1;
    if (dataCapacity <= SIZE_MAX - sizeof(BrixStringHeader)) {
        char* allocation =
            brixTryAllocBufferWithoutSafepoint(sizeof(BrixStringHeader) + dataCapacity);
        if (allocation) {
            _recordAllocationSite(
                0xC0DE0001, "libs/core/BxString.cpp", 74, 0,
                "_allocBytes", "String", "string-data", "descriptorless");
            return allocation + sizeof(BrixStringHeader);
        }
    }
    BrixStringTemporaryRoot firstRoot(_managedSource(first));
    BrixStringTemporaryRoot secondRoot(_managedSource(second));
    return _allocBytes(size);
}

static char*
_resizeBytes(char* buf, size_t oldSize, size_t newSize) {
    size_t oldCapacity = oldSize ? oldSize : 1;
    size_t newCapacity = newSize ? newSize : 1;
    if (
        oldCapacity > SIZE_MAX - sizeof(BrixStringHeader) ||
        newCapacity > SIZE_MAX - sizeof(BrixStringHeader)
    ) {
        _stringAllocationFailed("String resize size overflow");
    }
    char* resized = (char*)brixResizeBuffer(
        (char*)_stringHeader(buf),
        sizeof(BrixStringHeader) + oldCapacity,
        sizeof(BrixStringHeader) + newCapacity);
    if (!resized) {
        _stringAllocationFailed("String resize failed");
    }
    _recordAllocationSite(
        0xC0DE0002, "libs/core/BxString.cpp", 91, 0,
        "_resizeBytes", "String", "string-resize", "descriptorless");
    return resized + sizeof(BrixStringHeader);
}

static void
_setKnownMetadata(
    char* data,
    size_t byteLength,
    size_t characterLength,
    bool isASCII,
    bool isValid = true
) {
    if (characterLength > UINT32_MAX) {
        _stringAllocationFailed("String character length exceeds UINT32_MAX");
    }

    BrixStringHeader* header = _stringHeader(data);
    header->byteLength = byteLength;
    header->characterLength = (uint32_t)characterLength;
    header->flags =
        (isASCII ? BRIX_STRING_FLAG_ASCII : 0) |
        (isValid ? BRIX_STRING_FLAG_VALID_UTF8 : 0);
}

struct BrixUtf8Scan {
    size_t characterLength;
    bool isASCII;
    bool isValid;
};

static inline size_t
_skipASCII(const utf8proc_uint8_t* bytes, size_t byteOffset, size_t byteLength) {
    uint64_t word;
    while (byteLength - byteOffset >= sizeof(word)) {
        memcpy(&word, bytes + byteOffset, sizeof(word));
        if (word & 0x8080808080808080ull) {
            break;
        }
        byteOffset += sizeof(word);
    }
    while (byteOffset < byteLength && bytes[byteOffset] < 0x80) {
        byteOffset += 1;
    }
    return byteOffset;
}

static BrixUtf8Scan
_scanUtf8(const char* data, size_t byteLength) {
    const utf8proc_uint8_t* bytes = (const utf8proc_uint8_t*)data;
    size_t byteOffset = _skipASCII(bytes, 0, byteLength);
    size_t characterLength = byteOffset;
    if (byteOffset == byteLength) {
        return {characterLength, true, true};
    }

    while (byteOffset < byteLength) {
        utf8proc_int32_t codepoint = 0;
        utf8proc_ssize_t consumed = utf8proc_iterate(
            bytes + byteOffset,
            byteLength - byteOffset,
            &codepoint);
        if (consumed <= 0) {
            return {0, false, false};
        }
        byteOffset += (size_t)consumed;
        size_t asciiStart = byteOffset;
        byteOffset = _skipASCII(bytes, byteOffset, byteLength);
        characterLength += 1 + (byteOffset - asciiStart);
    }
    return {characterLength, false, true};
}

static void
_setScannedMetadata(char* data, size_t byteLength) {
    BrixUtf8Scan scan = _scanUtf8(data, byteLength);
    data[byteLength] = 0;
    _setKnownMetadata(data, byteLength, scan.characterLength, scan.isASCII, scan.isValid);
}

static const BrixStringData*
_copyString(const char* nbuf, size_t len, const char* keep = nullptr) {
    char* buf = _allocBytesKeeping(len + 1, keep);
    _recordAllocationSite(
        0xC0DE0006, "libs/core/BxString.cpp", 169, 0,
        "_copyString", "String", "string-copy", "supplemental");
    memcpy(buf, nbuf, len);
    buf[len] = 0;
    _setScannedMetadata(buf, len);
    return _stringValue(buf);
}

static const BrixStringData*
_copyKnownString(
    const char* source,
    size_t byteLength,
    size_t characterLength,
    bool isASCII,
    const char* keep = nullptr
) {
    char* output = _allocBytesKeeping(byteLength + 1, keep);
    _recordAllocationSite(
        0xC0DE0007, "libs/core/BxString.cpp", 178, 0,
        "_copyKnownString", "String", "string-copy-known", "supplemental");
    memcpy(output, source, byteLength);
    output[byteLength] = 0;
    _setKnownMetadata(output, byteLength, characterLength, isASCII);
    return _stringValue(output);
}

static inline uint8_t
_codepointDisplayWidth(utf8proc_int32_t codepoint) {
    int width = utf8proc_charwidth(codepoint);
    return width > 0 ? (uint8_t)width : 0;
}

static uint8_t*
_ensureDisplayWidths(uint8_t* widths, size_t byteCapacity, size_t codepointCount) {
    if (widths) {
        return widths;
    }

    uint8_t* newWidths = (uint8_t*)brixAllocBuffer(1, byteCapacity ? byteCapacity : 1);
    if (!newWidths) {
        fprintf(stderr, "String display-width allocation failed\n");
        exit(1);
    }
    _recordAllocationSite(
        0xC0DE0003, "libs/core/BxString.cpp", 199, 0,
        "_ensureDisplayWidths", "String", "display-widths", "descriptorless");

    memset(newWidths, 1, codepointCount);
    return newWidths;
}

static size_t
_getCodepointByteOffset(const char* str, size_t offset) {
    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)str;
    size_t byteLength = _stringHeader(str)->byteLength;
    utf8proc_int32_t codepoint;
    size_t byteOffset = 0;
    size_t count = 0;

    while (byteOffset < byteLength && count < offset) {
        utf8proc_ssize_t bytes = utf8proc_iterate(
            ptr + byteOffset,
            byteLength - byteOffset,
            &codepoint);
        if (bytes < 0) {
            return 1;
        }
        byteOffset += bytes;
        count += 1;
    }

    return byteOffset;
}


static size_t
brixStringCodePointAtOffset(const char* str, size_t offset) {
    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)str;
    size_t byteLength = _stringHeader(str)->byteLength;
    utf8proc_int32_t codepoint;
    size_t byteOffset = 0;
    size_t count = 0;

    while (byteOffset < byteLength && byteOffset < offset) {
        utf8proc_ssize_t bytes = utf8proc_iterate(
            ptr + byteOffset,
            byteLength - byteOffset,
            &codepoint);
        if (bytes < 0) {
            return 1;
        }
        byteOffset += bytes;
        count += 1;
    }

    return count;
}

static const char*
_reverseStringFind(const char* s1, size_t s1len, const char* s2, size_t s2len) {
    if (s2len > s1len) {
        return NULL;
    }

    for (const char* s = s1 + s1len - s2len; s >= s1; --s) {
        if (memcmp(s, s2, s2len) == 0) {
            return s;
        }
    }
    return NULL;
}

// **********************************************************************************************

extern "C" const BrixStringData*
brixStringCreate(const char* bytes, size_t byteLength) {
    if (!bytes) {
        return _copyString("", 0);
    }
    return _copyString(bytes, byteLength, bytes);
}

extern "C" const BrixStringData*
brixStringCreateValidated(const char* bytes, size_t byteLength) {
    if (!bytes) {
        return _copyString("", 0);
    }
    const BrixStringData* value = _copyString(bytes, byteLength, bytes);
    if (!(_stringHeader((const char*)value)->flags & BRIX_STRING_FLAG_VALID_UTF8)) {
        return nullptr;
    }
    return value;
}

extern "C" const BrixStringData*
brixStringCreateASCII(const char* bytes, size_t byteLength) {
    if (!bytes) {
        return _copyKnownString("", 0, 0, true);
    }
    return _copyKnownString(bytes, byteLength, byteLength, true, bytes);
}

extern "C" const BrixStringData*
brixStringAdopt(const char* bytes) {
    if (!bytes) {
        return _copyString("", 0);
    }
    return _copyString(bytes, strlen(bytes), bytes);
}

extern "C" const BrixStringData*
brixStringConcat(const char* left, const char* right) {
    if (!left || _stringHeader(left)->byteLength == 0) {
        return _stringValue(right);
    }
    if (!right || _stringHeader(right)->byteLength == 0) {
        return _stringValue(left);
    }
    size_t l1 = left ? _stringHeader(left)->byteLength : 0;
    size_t l2 = right ? _stringHeader(right)->byteLength : 0;
    if (l1 > SIZE_MAX - l2 - 1) {
        _stringAllocationFailed("String concatenation size overflow");
    }
    char* buf = _allocBytesKeeping(l1 + l2 + 1, left, right);
    _recordAllocationSite(
        0xC0DE0008, "libs/core/BxString.cpp", 320, 0,
        "brixStringConcat", "String", "string-concat", "supplemental");
    if (left) {
        memcpy(buf, left, l1);
    }
    if (right) {
        memcpy(buf + l1, right, l2);
    }
    buf[l1 + l2] = 0;

    bool isValid =
        (!left || (_stringHeader(left)->flags & BRIX_STRING_FLAG_VALID_UTF8)) &&
        (!right || (_stringHeader(right)->flags & BRIX_STRING_FLAG_VALID_UTF8));
    if (isValid) {
        size_t characterLength =
            (left ? _stringHeader(left)->characterLength : 0) +
            (right ? _stringHeader(right)->characterLength : 0);
        bool isASCII =
            (!left || (_stringHeader(left)->flags & BRIX_STRING_FLAG_ASCII)) &&
            (!right || (_stringHeader(right)->flags & BRIX_STRING_FLAG_ASCII));
        _setKnownMetadata(buf, l1 + l2, characterLength, isASCII);
    } else {
        _setScannedMetadata(buf, l1 + l2);
    }
    return _stringValue(buf);
}

extern "C" const BrixStringData*
brixStringRepeat(const char* value, size_t count) {
    if (count == 1) {
        return _stringValue(value);
    }
    BrixStringTemporaryRoot valueRoot(value);
    const BrixStringHeader* sourceHeader = _stringHeader(value);
    if (count && sourceHeader->byteLength > (SIZE_MAX - 1) / count) {
        _stringAllocationFailed("Repeated String size overflow");
    }
    if (count && sourceHeader->characterLength > UINT32_MAX / count) {
        _stringAllocationFailed("Repeated String character length exceeds UINT32_MAX");
    }

    size_t byteLength = sourceHeader->byteLength * count;
    char* output = _allocBytes(byteLength + 1);
    for (size_t index = 0; index < count; ++index) {
        memcpy(
            output + index * sourceHeader->byteLength,
            value,
            sourceHeader->byteLength);
    }
    output[byteLength] = 0;
    _setKnownMetadata(
        output,
        byteLength,
        sourceHeader->characterLength * count,
        sourceHeader->flags & BRIX_STRING_FLAG_ASCII,
        sourceHeader->flags & BRIX_STRING_FLAG_VALID_UTF8);
    return _stringValue(output);
}

extern "C" bool
brixStringFind(const char* self, const char* findStr, bool reverse, int start, int* outIndex) {
    if (_stringHeader(self)->flags & BRIX_STRING_FLAG_ASCII) {
        return brixStringFindBytes(self, findStr, reverse, start, outIndex);
    }
    size_t startByteOffset = _getCodepointByteOffset(self, start);
    int byteOffset = 0;
    bool found = brixStringFindBytes(self, findStr, reverse, startByteOffset, &byteOffset);
    if (found) {
        ssize_t codepointIndex = brixStringCodePointAtOffset(self, byteOffset);
        *outIndex = codepointIndex;
        return true;
    } else {
        return false;
    }
}

extern "C" bool
brixStringFindBytes(const char* self, const char* findStr, bool reverse, int start, int* outIndex) {
    size_t startByteOffset = start;
    size_t selfByteLength = _stringHeader(self)->byteLength;
    size_t findByteLength = _stringHeader(findStr)->byteLength;
    if (startByteOffset > selfByteLength) {
        return false;
    }
    self = self + startByteOffset;
    size_t searchLength = selfByteLength - startByteOffset;
    const char* value;
    if (reverse) {
        value = _reverseStringFind(self, searchLength, findStr, findByteLength);
    } else if (findByteLength == 0) {
        value = self;
    } else {
        value = (const char*)memmem(self, searchLength, findStr, findByteLength);
    }
    if (value) {
        ssize_t byteOffset = value - self;
        *outIndex = start + byteOffset;
        return true;
    } else {
        return false;
    }
}

extern "C" const BrixStringData*
brixStringCopy(const char* value) {
    char* output = _allocBytesKeeping(_stringHeader(value)->byteLength + 1, value);
    _recordAllocationSite(
        0xC0DE000B, "libs/core/BxString.cpp", 500, 0,
        "brixStringCopy", "String", "string-copy-whole", "supplemental");
    const BrixStringHeader* header = _stringHeader(value);
    memcpy(output, value, header->byteLength);
    output[header->byteLength] = 0;
    _setKnownMetadata(
        output,
        header->byteLength,
        header->characterLength,
        header->flags & BRIX_STRING_FLAG_ASCII,
        header->flags & BRIX_STRING_FLAG_VALID_UTF8);
    return _stringValue(output);
}

extern "C" const BrixStringData*
brixStringSlice(char* self, size_t begin, size_t end) {
    const BrixStringHeader* header = _stringHeader(self);
    bool isASCII = header->flags & BRIX_STRING_FLAG_ASCII;
    begin = std::min(begin, (size_t)header->characterLength);
    end = std::min(end, (size_t)header->characterLength);
    if (
        begin == 0 &&
        end == header->characterLength &&
        (header->flags & BRIX_STRING_FLAG_VALID_UTF8)
    ) {
        return _stringValue(self);
    }
    size_t beginCodepoints = isASCII
        ? begin
        : _getCodepointByteOffset(self, begin);
    size_t endCodepoints = isASCII
        ? end
        : _getCodepointByteOffset(self, end);
    return _copyKnownString(
        self + beginCodepoints,
        endCodepoints - beginCodepoints,
        end - begin,
        isASCII,
        self);
}

extern "C" const BrixStringData*
brixStringSliceUtf8(const char* bytes, size_t begin, size_t end) {
    BrixUtf8Scan scan = _scanUtf8(bytes + begin, end - begin);
    if (!scan.isValid) {
        return nullptr;
    }
    return _copyKnownString(bytes + begin, end - begin, scan.characterLength, scan.isASCII, bytes);
}

extern "C" BRIX_WITHOUT_GC const BrixStringData*
brixArenaCopyByteSpan(
    void* arena,
    size_t incarnation,
    const uint8_t* source,
    size_t sourceLength,
    size_t start,
    size_t end) {
    if ((!source && sourceLength) || start > end || end > sourceLength ||
        end - start > SIZE_MAX - sizeof(BrixStringHeader) - 1) {
        return nullptr;
    }
    const size_t byteLength = end - start;
    char* allocation = brixArenaReserve(
        arena, incarnation, sizeof(BrixStringHeader) + byteLength + 1, alignof(BrixStringHeader), false);
    if (!allocation) {
        return nullptr;
    }
    char* data = allocation + sizeof(BrixStringHeader);
    if (byteLength) {
        memcpy(data, source + start, byteLength);
    }
    _setScannedMetadata(data, byteLength);
    if (!(_stringHeader(data)->flags & BRIX_STRING_FLAG_VALID_UTF8)) {
        return nullptr;
    }
    return _stringValue(data);
}

// **********************************************************************************************

extern "C" int
brixStringGetCharAtIndex(const char* str, size_t index, ResultCharOffset* ret) {
    const BrixStringHeader* header = _stringHeader(str);
    if (header->flags & BRIX_STRING_FLAG_ASCII) {
        if (index >= header->characterLength) {
            return 2;
        }
        ret->bytes = (int32_t)index;
        ret->codepoint = (uint8_t)str[index];
        return 0;
    }

    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)str;
    size_t byteLength = header->byteLength;
    utf8proc_int32_t codepoint;
    size_t byteOffset = 0;
    size_t count = 0;

    while (true) {
        if (byteOffset >= byteLength) {
            return 2;
        }
        utf8proc_ssize_t bytes = utf8proc_iterate(
            ptr + byteOffset,
            byteLength - byteOffset,
            &codepoint);
        if (bytes < 0) {
            return 1;
        }
        if (count == index) {
            break;
        } else {
            byteOffset += bytes;
            count += 1;
        }
    }

    ret->bytes = (int32_t)byteOffset;
    ret->codepoint = codepoint;
    return 0;
}

extern "C" int
brixStringGetNextChar(const char* str, ResultCharOffset* ret) {
    utf8proc_int32_t codepoint;
    utf8proc_ssize_t bytes = utf8proc_iterate((const utf8proc_uint8_t*)str, -1, &codepoint);
    if (bytes < 0) {
        return 1;
    }

    ret->bytes = bytes;
    ret->codepoint = codepoint;
    return 0;
}

extern "C" bool
brixStringIsEmpty(const char* str) {
    return _stringHeader(str)->byteLength == 0;
}

extern "C" uint64_t
brixStringGetByteLength(const char* str) {
    return _stringHeader(str)->byteLength;
}

extern "C" uint32_t
brixStringGetLength(const char* str) {
    return _stringHeader(str)->characterLength;
}

extern "C" uint32_t
brixStringGetFlags(const char* str) {
    return _stringHeader(str)->flags;
}

extern "C" bool
brixStringEquals(const char* left, const char* right) {
    size_t byteLength = _stringHeader(left)->byteLength;
    return byteLength == _stringHeader(right)->byteLength && memcmp(left, right, byteLength) == 0;
}

extern "C" int
brixStringCompare(const char* left, const char* right) {
    size_t leftLength = _stringHeader(left)->byteLength;
    size_t rightLength = _stringHeader(right)->byteLength;
    int order = memcmp(left, right, std::min(leftLength, rightLength));
    if (order != 0) {
        return order;
    }
    return leftLength < rightLength ? -1 : leftLength > rightLength ? 1 : 0;
}

extern "C" uint32_t
brixStringGetDisplayWidth(const char* str) {
    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)str;
    size_t byteLength = _stringHeader(str)->byteLength;
    utf8proc_int32_t codepoint;

    size_t byteOffset = 0;
    uint32_t width = 0;

    while (byteOffset < byteLength && ptr[byteOffset] >= 0x20 && ptr[byteOffset] < 0x7f) {
        byteOffset += 1;
        width += 1;
    }

    while (byteOffset < byteLength) {
        utf8proc_ssize_t bytes = utf8proc_iterate(
            ptr + byteOffset,
            byteLength - byteOffset,
            &codepoint);
        if (bytes < 0) {
            return 0;
        }

        width += _codepointDisplayWidth(codepoint);
        byteOffset += bytes;
    }

    return width;
}

extern "C" uint8_t
brixStringMeasureDisplayWidth(const char* str, ResultDisplayWidth* ret) {
    BrixStringTemporaryRoot inputRoot(str);
    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)str;
    size_t byteLength = _stringHeader(str)->byteLength;
    utf8proc_int32_t codepoint;

    size_t byteOffset = 0;
    size_t codepointCount = 0;
    size_t displayWidth = 0;
    uint8_t* widths = nullptr;

    while (byteOffset < byteLength && ptr[byteOffset] >= 0x20 && ptr[byteOffset] < 0x7f) {
        byteOffset += 1;
        codepointCount += 1;
        displayWidth += 1;
    }

    while (byteOffset < byteLength) {
        utf8proc_ssize_t bytes = utf8proc_iterate(
            ptr + byteOffset,
            byteLength - byteOffset,
            &codepoint);
        if (bytes < 0) {
            return kDecodeError;
        }

        uint8_t width = _codepointDisplayWidth(codepoint);
        if (width != 1 && !widths) {
            widths = _ensureDisplayWidths(widths, byteLength, codepointCount);
        }
        if (widths) {
            widths[codepointCount] = width;
        }

        byteOffset += bytes;
        codepointCount += 1;
        displayWidth += width;
    }

    ret->widths = widths;
    ret->displayWidth = displayWidth;
    ret->codepointsDecoded = codepointCount;
    ret->bytesDecoded = byteOffset;
    ret->flags = widths ? kDisplayWidthHasWidths : 0;
    return kDecodeOk;
}

extern "C" const BrixStringData*
brixStringToUppercase(const char* input) {
    BrixStringTemporaryRoot inputRoot(input);
    utf8proc_int32_t codepoint;
    utf8proc_ssize_t len;
    utf8proc_uint8_t *output;

    size_t input_len = _stringHeader(input)->byteLength;
    if (input_len > (SIZE_MAX - 1) / 4) {
        _stringAllocationFailed("Uppercase String size overflow");
    }
    size_t output_size = input_len * 4; // Worst case scenario
    output = (utf8proc_uint8_t *)_allocBytes(output_size + 1);

    utf8proc_uint8_t *ptr = output;
    size_t characterLength = 0;
    bool isASCII = true;
    const char* end = input + input_len;

    while (input < end) {
        // Decode a single UTF-8 codepoint
        len = utf8proc_iterate((const utf8proc_uint8_t *)input, end - input, &codepoint);
        if (len < 0) {
            return _copyString("", 0);
        }

        codepoint = utf8proc_toupper(codepoint);
        isASCII = isASCII && codepoint < 0x80;
        characterLength += 1;

        utf8proc_uint8_t encoded_char[4];
        utf8proc_ssize_t encoded_len = utf8proc_encode_char(codepoint, encoded_char);
        memcpy(ptr, encoded_char, encoded_len);
        ptr += encoded_len;

        input += len;
    }

    *ptr = '\0';
    _setKnownMetadata(
        (char*)output,
        (size_t)(ptr - output),
        characterLength,
        isASCII);
    return _stringValue((char*)output);
}

extern "C" const BrixStringData*
brixStringToLowercase(const char* input) {
    BrixStringTemporaryRoot inputRoot(input);
    utf8proc_int32_t codepoint;
    utf8proc_ssize_t len;
    utf8proc_uint8_t *output;

    size_t input_len = _stringHeader(input)->byteLength;
    if (input_len > (SIZE_MAX - 1) / 4) {
        _stringAllocationFailed("Lowercase String size overflow");
    }
    size_t output_size = input_len * 4; // Worst case scenario
    output = (utf8proc_uint8_t *)_allocBytes(output_size + 1);

    utf8proc_uint8_t *ptr = output;
    size_t characterLength = 0;
    bool isASCII = true;
    const char* end = input + input_len;

    while (input < end) {
        // Decode a single UTF-8 codepoint
        len = utf8proc_iterate((const utf8proc_uint8_t *)input, end - input, &codepoint);
        if (len < 0) {
            return _copyString("", 0);
        }

        codepoint = utf8proc_tolower(codepoint);
        isASCII = isASCII && codepoint < 0x80;
        characterLength += 1;

        utf8proc_uint8_t encoded_char[4];
        utf8proc_ssize_t encoded_len = utf8proc_encode_char(codepoint, encoded_char);
        memcpy(ptr, encoded_char, encoded_len);
        ptr += encoded_len;

        input += len;
    }

    *ptr = '\0';
    _setKnownMetadata(
        (char*)output,
        (size_t)(ptr - output),
        characterLength,
        isASCII);
    return _stringValue((char*)output);
}

extern "C" const BrixStringData*
brixStringDecompose(const char* input) {
    BrixStringTemporaryRoot inputRoot(input);
    utf8proc_int32_t codepoint;
    utf8proc_ssize_t input_len;
    utf8proc_int32_t stack_decomp_buffer[8];

    size_t inputSize = _stringHeader(input)->byteLength;
    if (inputSize > (SIZE_MAX - 1) / 4) {
        _stringAllocationFailed("Decomposed String size overflow");
    }
    size_t output_size = inputSize * 4 + 1;
    utf8proc_uint8_t* output = (utf8proc_uint8_t*)_allocBytes(output_size);
    size_t output_len = 0;
    size_t characterLength = 0;
    bool isASCII = true;
    const char* end = input + inputSize;

    while (input < end) {
        input_len = utf8proc_iterate((const utf8proc_uint8_t*)input, end - input, &codepoint);
        if (input_len < 0) {
            return _copyString("", 0);
        }

        utf8proc_int32_t* decomp_buffer = stack_decomp_buffer;
        int decomp_capacity = sizeof(stack_decomp_buffer) / sizeof(stack_decomp_buffer[0]);
        int decomp_len = utf8proc_decompose_char(codepoint, decomp_buffer, decomp_capacity,
            (utf8proc_option_t)(UTF8PROC_DECOMPOSE | UTF8PROC_STABLE), NULL);
        if (decomp_len < 0) {
            return _copyString("", 0);
        }

        if (decomp_len > decomp_capacity) {
            decomp_capacity = decomp_len;
            decomp_buffer = (utf8proc_int32_t*)malloc(sizeof(utf8proc_int32_t) * decomp_capacity);
            if (!decomp_buffer) {
                fprintf(stderr, "String decomposition allocation failed\n");
                exit(1);
            }
            decomp_len = utf8proc_decompose_char(codepoint, decomp_buffer, decomp_capacity,
                (utf8proc_option_t)(UTF8PROC_DECOMPOSE | UTF8PROC_STABLE), NULL);
            if (decomp_len < 0) {
                free(decomp_buffer);
                return _copyString("", 0);
            }
        }

        for (int i = 0; i < decomp_len; i++) {
            utf8proc_uint8_t encoded_char[4];
            utf8proc_ssize_t encoded_len = utf8proc_encode_char(decomp_buffer[i], encoded_char);
            if (encoded_len < 0) {
                if (decomp_buffer != stack_decomp_buffer) {
                    free(decomp_buffer);
                }
                return _copyString("", 0);
            }

            size_t needed = output_len + (size_t)encoded_len + 1;
            if (needed > output_size) {
                size_t old_size = output_size;
                while (needed > output_size) {
                    output_size *= 2;
                }
                output = (utf8proc_uint8_t*)_resizeBytes((char*)output, old_size, output_size);
            }

            memcpy(output + output_len, encoded_char, encoded_len);
            output_len += encoded_len;
            characterLength += 1;
            isASCII = isASCII && decomp_buffer[i] < 0x80;
        }

        if (decomp_buffer != stack_decomp_buffer) {
            free(decomp_buffer);
        }

        input += input_len;
    }

    output[output_len] = '\0';
    _setKnownMetadata((char*)output, output_len, characterLength, isASCII);
    return _stringValue((char*)output);
}

extern "C" const BrixStringData*
brixStringNormalize(const char* str) {
    BrixStringTemporaryRoot inputRoot(str);
    utf8proc_uint8_t* normalized = nullptr;
    utf8proc_ssize_t len = utf8proc_map(
        (const utf8proc_uint8_t*)str,
        (utf8proc_ssize_t)_stringHeader(str)->byteLength,
        &normalized,
        (utf8proc_option_t)(UTF8PROC_STABLE | UTF8PROC_COMPOSE));
    if (len < 0 || !normalized) {
        return _copyString("", 0);
    }

    char* result = _allocBytes((size_t)len + 1);
    memcpy(result, normalized, (size_t)len);
    result[(size_t)len] = '\0';
    _setScannedMetadata(result, (size_t)len);
    free(normalized);
    return _stringValue(result);
}

static inline bool
_isScalarValue(uint32_t codepoint) {
    return codepoint < 0x110000 && (codepoint < 0xd800 || codepoint >= 0xe000);
}

extern "C" const BrixStringData*
brixStringFromUtf8Codepoint(uint32_t c) {
    if (!_isScalarValue(c)) {
        return nullptr;
    }
    utf8proc_uint8_t str[5];
    utf8proc_ssize_t bytes = utf8proc_encode_char(c, (utf8proc_uint8_t *)str);
    if (bytes < 0) {
        return _copyString("", 0);
    }
    str[bytes] = '\0';
    char* result = _allocBytes(bytes + 1);
    _recordAllocationSite(
        0xC0DE0009, "libs/core/BxString.cpp", 805, 0,
        "brixStringFromUtf8Codepoint", "String", "char-to-string", "supplemental");
    memcpy(result, str, bytes + 1);
    _setKnownMetadata(result, (size_t)bytes, 1, c < 0x80);
    return _stringValue(result);
}

extern "C" const BrixStringData*
brixStringFromUtf8Codepoints(const uint32_t* self, size_t start, size_t end) {
    BrixStringTemporaryRoot selfRoot((const char*)self);
    size_t byteLength = 0;
    size_t characterLength = 0;
    bool isASCII = true;

    for (size_t i = start; i < end; i++) {
        uint32_t codepoint = self[i];
        if (!_isScalarValue(codepoint)) {
            return nullptr;
        }
        size_t bytes = 1 + (codepoint >= 0x80) + (codepoint >= 0x800) + (codepoint >= 0x10000);
        if (bytes > SIZE_MAX - byteLength) {
            _stringAllocationFailed("Encoded String size overflow");
        }
        byteLength += bytes;
        characterLength += 1;
        isASCII = isASCII && codepoint < 0x80;
    }

    if (byteLength == SIZE_MAX) {
        _stringAllocationFailed("Encoded String size overflow");
    }
    char* buf = _allocBytes(byteLength + 1);
    _recordAllocationSite(
        0xC0DE000A, "libs/core/BxString.cpp", 816, 0,
        "brixStringFromUtf8Codepoints", "String", "codepoints-to-string", "supplemental");
    size_t bufIndex = 0;

    for (size_t i = start; i < end; i++) {
        uint32_t codepoint = self[i];
        utf8proc_ssize_t bytes = utf8proc_encode_char(
            codepoint,
            (utf8proc_uint8_t *)(buf + bufIndex));
        if (bytes < 0) {
            fprintf(stderr, "Failed to encode codepoint\n");
            return NULL;
        }
        bufIndex += (size_t)bytes;
    }

    buf[bufIndex] = '\0';
    _setKnownMetadata(buf, bufIndex, characterLength, isASCII);
    return _stringValue(buf);
}

extern "C" uint8_t
brixStringValidateUtf8(const char* input, size_t inputSize) {
    return _scanUtf8(input, inputSize).isValid ? 0 : 1;
}

extern "C" uint8_t
brixStringSliceToUtf8Codepoints(const char* input, size_t inputSize, uint32_t* output,
                                size_t outputSize, ResultDecodeUTF8* ret) {
    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)input;
    utf8proc_int32_t codepoint;
    size_t byteCount = 0;
    size_t codepointCount = 0;
    uint8_t status = kDecodeOk;

    while (byteCount < inputSize) {
        if (codepointCount >= outputSize) {
            status = kDecodeOutputFull;
            break;
        }

        utf8proc_ssize_t bytes = utf8proc_iterate(ptr + byteCount, inputSize - byteCount, &codepoint);
        if (bytes < 0) {
            status = kDecodeError;
            break;
        }

        output[codepointCount] = codepoint;
        byteCount += bytes;
        codepointCount += 1;
    }

    ret->codepoints = (uint32_t*)output;
    ret->bytesDecoded = byteCount;
    ret->codepointsDecoded = codepointCount;
    return status;
}

extern "C" uint8_t
brixStringToUtf8Codepoints(const char* str, ResultDecodeUTF8* ret) {
    BrixStringTemporaryRoot inputRoot(str);
    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)str;
    size_t byteLength = _stringHeader(str)->byteLength;
    utf8proc_int32_t codepoint;
    size_t byteOffset = 0;
    int32_t count = 0;
    int32_t arraySize = 10;
    uint32_t* array = (uint32_t*)brixAllocBuffer(sizeof(uint32_t), arraySize);
    _recordAllocationSite(
        0xC0DE0004, "libs/core/BxString.cpp", 913, 0,
        "brixStringToUtf8Codepoints", "Char32", "codepoints", "descriptorless");

    while (byteOffset < byteLength) {
        utf8proc_ssize_t bytes = utf8proc_iterate(
            ptr + byteOffset,
            byteLength - byteOffset,
            &codepoint);
        if (bytes < 0) {
            return 1;
        }

        if (count >= arraySize) {
            size_t oldSize = arraySize * sizeof(uint32_t);
            arraySize *= 2;
            array = (uint32_t*)brixResizeBuffer((char*)array, oldSize, arraySize * sizeof(uint32_t));
            if (!array) {
                fprintf(stderr, "String codepoint allocation failed\n");
                exit(1);
            }
            _recordAllocationSite(
                0xC0DE0005, "libs/core/BxString.cpp", 926, 0,
                "brixStringToUtf8Codepoints", "Char32", "codepoints-resize", "descriptorless");
        }
        array[count] = codepoint;

        byteOffset += bytes;
        count += 1;
    }

    ret->codepoints = array;
    ret->codepointsDecoded = count;
    ret->bytesDecoded = byteOffset;
    return 0;
}

// **********************************************************************************************

extern "C" uint32_t
brixCharGetCategory(uint32_t c) {
    return utf8proc_category(c);
}

extern "C" uint32_t
brixCharGetDisplayWidth(uint32_t c) {
    return _codepointDisplayWidth((utf8proc_int32_t)c);
}

extern "C" uint32_t
brixCharToUppercase(uint32_t c) {
    return utf8proc_toupper(c);
}

extern "C" uint32_t
brixCharToLowercase(uint32_t c) {
    return utf8proc_tolower(c);
}

extern "C" const BrixStringData*
brixCharToString(char c) {
    char nbuf[2];
    snprintf(nbuf, 2, "%c", c);
    return _copyKnownString(nbuf, 1, 1, true);
}
