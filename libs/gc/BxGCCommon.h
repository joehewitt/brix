#ifndef BX_GC_COMMON_H
#define BX_GC_COMMON_H

#include "BxGC/BxGC.h"
#include "BxGC/BxGCRuntime.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <unordered_map>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

extern "C" void brixImmediateAbort(
    const char* message,
    const char* file,
    unsigned int line,
    unsigned int column);

namespace BxGC {

static const unsigned char kPhaseIdle = 0;
static const unsigned char kPhaseMarkSetup = 1;
static const unsigned char kPhaseConcurrentMark = 2;
static const unsigned char kPhaseMarkTermination = 3;
static const unsigned char kPhaseMarkComplete = 4;
static const unsigned char kPhaseSweep = 5;

static const uint32_t kHandshakeReasonMarkSetup = 1;
static const uint32_t kHandshakeReasonMarkTermination = 2;
static const uint32_t kHandshakeReasonSweep = 3;

static const size_t kObjectAlignment = 8;
static const size_t kPageSize = 8192;
static const size_t kArenaSize = 64ull * 1024ull * 1024ull;
static const size_t kHeapArenaShift = 26;
static const size_t kAddressIndexBits = 13;
static const size_t kAddressIndexEntries = 1ull << kAddressIndexBits;
static const size_t kAddressIndexLimit = 1ull << 52;
static const size_t kArenaAddressPageCount = kArenaSize / kPageSize;
static const size_t kMarkerSize = ((kArenaSize / 8) * 2) / 8;
static const size_t kArenaCapacity = kArenaSize - kMarkerSize;
static const size_t kArenaPageCapacity = kArenaCapacity / kPageSize;
static const size_t kLargeSizeClass = 68;
static const size_t kSizeClassesCount = 68;
static const size_t kLargeSpanTargetObjects = 8;
static const size_t kMaxCachedLargeSpanPages = 256;
static const size_t kCollectorStepWork = 64;
static const size_t kLazySweepStepBudget = 32;
static const size_t kLazySweepLargeSpanBudget = 16;
static const size_t kPartialSpanScanBudget = 8;
static const size_t kFullSpanReclaimBudget = 1;
static const size_t kScavengeEmptyArenaReleaseBudget = 1;
static const size_t kScavengeArenaHoldDownCycles = 2;
static const size_t kScavengeRetentionSlackPercent = 10;
static const size_t kSpanOccupancyBucketCount = 11;
static const unsigned char kSweptPoisonByte = 221;

static const unsigned char kSpanReuseNew = 0;
static const unsigned char kSpanReuseActive = 1;
static const unsigned char kSpanReuseNeedsSweep = 2;
static const unsigned char kSpanReuseSwept = 3;
static const unsigned char kSpanReuseReset = 4;
static const unsigned char kSpanReuseReleased = 5;

static const unsigned char kSpanSweepAvailable = 0;
static const unsigned char kSpanSweepClaimed = 1;

static const unsigned char kSweepListPartial = 0;
static const unsigned char kSweepListFull = 1;
static const unsigned char kSweepListDone = 2;

// Refill packs spans that are at least half occupied before considering sparse spans.
static const size_t kSpanPackingOccupancyPercent = 50;

extern std::recursive_mutex gCollectorMutex;

struct GCVecState {
    void* items;
    size_t length;
    size_t capacity;
};

struct GCVec {
    GCVecState** stateSlot;
};

struct GCDescriptor {
    size_t objectSize;
    GCVec pointerOffsets;
    GCVec scalarRanges;
    size_t elementSize;
    GCVec elementPointerOffsets;
    void* elementDescriptor;
    GCVec unionRanges;
    bool isBuffer;
    bool hasCleanup;
};

struct GCUnionRange {
    size_t tagOffset;
    size_t payloadOffset;
    GCVec caseDescriptors;
};

void abortGC(const char* message);

struct SizeClass {
    uint8_t index;
    size_t size;
    uint64_t divisionMagic;

    bool isLarge() const;

    // Exact unsigned division by the immutable slot size without a hardware divide.
    // ceil(2^64 / size) can overestimate the quotient by at most one; the multiply-only
    // correction makes both base and interior-pointer lookup exact.
    size_t divideSlotOffset(size_t offset) const {
        if (!size || !divisionMagic) {
            abortGC("GC invalid slot-division metadata");
        }
        __uint128_t product = static_cast<__uint128_t>(offset) * divisionMagic;
        size_t quotient = static_cast<size_t>(product >> 64);
        if (static_cast<__uint128_t>(quotient) * size > offset) {
            --quotient;
        }
        return quotient;
    }
};

struct GCBitmap;
struct GCSpan;
struct GCArena;
struct ArenaMeta;
struct GCSpanStore;
struct GCHeap;

GCHeap& heap();
size_t nativePageSize();
size_t scavengeUnitPages();
size_t alignDown(size_t value, size_t alignment);
size_t alignUp(size_t value, size_t alignment);
size_t objectAllocationSize(size_t value);
SizeClass sizeClassForIndex(size_t index);
SizeClass sizeClassForSize(size_t size);
size_t sizeClassPageCount(size_t index);
size_t largeSpanPageCount(size_t size);
bool phaseNeedsMarkBarrier(unsigned char phase);
bool phaseNeedsSweepValidation(unsigned char phase);
unsigned char publicCollectorPhase();
void publishPhase(unsigned char phase);
void publishSweepGeneration(size_t generation);
void recordObjectSidePayloadPublish(size_t lane);
void recordObjectSidePayloadClear(size_t lane);
void recordObjectSideCleanupPayloadPublish();
void recordObjectSideCleanupPayloadClear();
size_t objectSidePayloadCount(size_t lane);
size_t objectSideCleanupPayloadCount();

} // namespace BxGC

#endif
