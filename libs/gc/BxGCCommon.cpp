#include "BxGCCommon.h"

namespace BxGC {

static constexpr size_t kSizeClassSizes[kSizeClassesCount] = {
    0, 8, 16, 24, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224,
    240, 256, 288, 320, 352, 384, 416, 448, 480, 512, 576, 640, 704, 768, 896,
    1024, 1152, 1280, 1408, 1536, 1792, 2048, 2304, 2688, 3072, 3200, 3456,
    4096, 4864, 5376, 6144, 6528, 6784, 6912, 8192, 9472, 9728, 10240, 10880,
    12288, 13568, 14336, 16384, 18432, 19072, 20480, 21760, 24576, 27264,
    28672, 32768
};

static constexpr size_t kSizeClassPageCounts[kSizeClassesCount] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 3, 2, 3, 1, 3,
    2, 3, 4, 5, 6, 1, 7, 6, 5, 4, 3, 5, 7, 2, 9, 7, 5, 8, 3, 10, 7, 4
};

static constexpr size_t kLargestSmallSize =
    kSizeClassSizes[kSizeClassesCount - 1];
static constexpr size_t kSizeClassLookupCount =
    kLargestSmallSize / kObjectAlignment + 1;

static std::array<uint8_t, kSizeClassLookupCount>
makeSizeClassLookup() {
    std::array<uint8_t, kSizeClassLookupCount> lookup {};
    size_t sizeClassIndex = 1;
    for (size_t alignedSizeIndex = 1;
            alignedSizeIndex < kSizeClassLookupCount;
            ++alignedSizeIndex) {
        size_t size = alignedSizeIndex * kObjectAlignment;
        while (size > kSizeClassSizes[sizeClassIndex]) {
            ++sizeClassIndex;
        }
        lookup[alignedSizeIndex] = static_cast<uint8_t>(sizeClassIndex);
    }
    return lookup;
}

static const std::array<uint8_t, kSizeClassLookupCount> kSizeClassLookup =
    makeSizeClassLookup();

static uint64_t
slotDivisionMagic(size_t size) {
    return size ? std::numeric_limits<uint64_t>::max() / size + 1 : 0;
}

static std::array<uint64_t, kSizeClassesCount>
makeSizeClassDivisionMagic() {
    std::array<uint64_t, kSizeClassesCount> magic {};
    for (size_t index = 1; index < kSizeClassesCount; ++index) {
        magic[index] = slotDivisionMagic(kSizeClassSizes[index]);
    }
    return magic;
}

static const std::array<uint64_t, kSizeClassesCount> kSizeClassDivisionMagic =
    makeSizeClassDivisionMagic();

bool
SizeClass::isLarge() const {
    return index == kLargeSizeClass;
}

extern "C" uint64_t brixGCPublicControlWord;

std::recursive_mutex gCollectorMutex;

void
abortGC(const char* message) {
    brixImmediateAbort(message, __FILE__, __LINE__, 0);
    std::abort();
}

size_t
nativePageSize() {
    static size_t cached = []() {
        long value = sysconf(_SC_PAGESIZE);
        return value > 0 ? static_cast<size_t>(value) : 4096ull;
    }();
    return cached;
}

size_t
scavengeUnitPages() {
    size_t nativeSize = nativePageSize();
    return nativeSize > kPageSize ? (nativeSize + kPageSize - 1) / kPageSize : 1;
}

size_t
alignDown(size_t value, size_t alignment) {
    return value - (value % alignment);
}

size_t
alignUp(size_t value, size_t alignment) {
    size_t remainder = value % alignment;
    return remainder == 0 ? value : value + (alignment - remainder);
}

size_t
objectAllocationSize(size_t value) {
    if (value == 0) {
        return kObjectAlignment;
    }
    if (value > std::numeric_limits<size_t>::max() - (kObjectAlignment - 1)) {
        abortGC("GC allocation size overflow");
    }
    return alignUp(value, kObjectAlignment);
}

SizeClass
sizeClassForIndex(size_t index) {
    if (index >= kSizeClassesCount) {
        return SizeClass {static_cast<uint8_t>(kLargeSizeClass), 0, 0};
    }
    return SizeClass {
        static_cast<uint8_t>(index),
        kSizeClassSizes[index],
        kSizeClassDivisionMagic[index]
    };
}

SizeClass
sizeClassForSize(size_t size) {
    size = objectAllocationSize(size);
    if (size <= kLargestSmallSize) {
        size_t alignedSizeIndex = size / kObjectAlignment;
        return sizeClassForIndex(kSizeClassLookup[alignedSizeIndex]);
    }
    return SizeClass {
        static_cast<uint8_t>(kLargeSizeClass),
        size,
        slotDivisionMagic(size)
    };
}

size_t
sizeClassPageCount(size_t index) {
    return index < kSizeClassesCount ? kSizeClassPageCounts[index] : 0;
}

size_t
largeSpanPageCount(size_t size) {
    size_t objectPages = (size + kPageSize - 1) / kPageSize;
    size_t targetObjects = objectPages ? kMaxCachedLargeSpanPages / objectPages : 0;
    if (targetObjects == 0) {
        return objectPages;
    }
    if (targetObjects > kLargeSpanTargetObjects) {
        targetObjects = kLargeSpanTargetObjects;
    }
    return objectPages * targetObjects;
}

bool
phaseNeedsMarkBarrier(unsigned char phase) {
    return phase >= kPhaseMarkSetup && phase <= kPhaseMarkComplete;
}

bool
phaseNeedsSweepValidation(unsigned char phase) {
    return phase == kPhaseSweep && brixGCRuntimeVerifyEachSweep();
}

unsigned char
publicCollectorPhase() {
    uint64_t control = __atomic_load_n(&brixGCPublicControlWord, __ATOMIC_ACQUIRE);
    return brixGCControlWordPhase(control);
}

void
publishPhase(unsigned char phase) {
    brixGCPublishPhase(phase);
}

void
publishSweepGeneration(size_t generation) {
    brixGCPublishSweepGeneration(generation);
}

} // namespace BxGC
