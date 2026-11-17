#ifndef BX_GC_ALLOCATION_BUFFER_H
#define BX_GC_ALLOCATION_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include "BxGCAnnotations.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BX_GC_ALLOCATION_BUFFER_STRUCT
#define BX_GC_ALLOCATION_BUFFER_STRUCT
typedef struct BrixGCAllocationBuffer {
    char* cursor;
    char* limit;
} BrixGCAllocationBuffer;
#endif

#ifndef BX_GC_RUNTIME_ALLOCATION_BUFFER_STRUCT
#define BX_GC_RUNTIME_ALLOCATION_BUFFER_STRUCT
typedef struct BrixGCRuntimeAllocationBuffer {
    BrixGCAllocationBuffer buffer;
    char* base;
    size_t slotSize;
    size_t requestedSize;
    void* descriptor;
    size_t generation;
    size_t sizeClassIndex;
    unsigned char pointerKind;
    unsigned char zeroingPolicy;
    unsigned char reserved[6];
    uint32_t abiVersion;
    uint32_t structSize;
} BrixGCRuntimeAllocationBuffer;
#endif

enum {
    BrixGCAllocationABIVersion = 5,
    BrixGCCompilerAllocationBufferCount = 512,
    BrixGCThreadCacheRangeCount = 193,
    BrixGCThreadCacheRangeWayCount = 4,
    BrixGCThreadCacheParkedRangeWayCount = BrixGCThreadCacheRangeWayCount - 1,
    BrixGCWriteBarrierBufferCapacity = 512,
    BrixGCThreadCacheCounterCount = 8,
    BrixGCThreadCacheMarkWorkHandleCount = 2
};

typedef struct BrixGCAllocationSiteKey {
    size_t sizeClassIndex;
    size_t requestedSize;
    size_t allocatedSize;
    void* descriptor;
    uint32_t compilerBufferIndex;
    unsigned char pointerKind;
    unsigned char zeroingPolicy;
    unsigned char reserved[2];
    uint32_t abiVersion;
    uint32_t structSize;
} BrixGCAllocationSiteKey;

#define BX_GC_ALLOCATION_SITE_KEY_STRUCT 1

typedef struct BrixGCThreadCache {
    uint32_t abiVersion;
    uint32_t structSize;
    uint64_t ownerEpoch;
    uint64_t flushEpoch;
    BrixGCRuntimeAllocationBuffer activeBuffer;
    BrixGCRuntimeAllocationBuffer compilerBuffers[BrixGCCompilerAllocationBufferCount];
    BrixGCRuntimeAllocationBuffer ranges[BrixGCThreadCacheRangeCount];
    char* writeBarrierEntries[BrixGCWriteBarrierBufferCapacity];
    uint32_t writeBarrierCount;
    uint32_t activeCompilerBufferCount;
    uint32_t activeCompilerBufferIndices[BrixGCCompilerAllocationBufferCount];
    uint64_t counters[BrixGCThreadCacheCounterCount];
    void* markWorkHandles[BrixGCThreadCacheMarkWorkHandleCount];
    BrixGCRuntimeAllocationBuffer
        parkedRanges[BrixGCThreadCacheRangeCount][BrixGCThreadCacheParkedRangeWayCount];
    unsigned char rangeNextVictimWay[BrixGCThreadCacheRangeCount];
} BrixGCThreadCache;

enum {
    BrixGCControlPhaseBits = 3,
    BrixGCControlPhaseMask = (1u << BrixGCControlPhaseBits) - 1u,
    BrixGCControlHandshakeRequestedBit = 1u << BrixGCControlPhaseBits,
    BrixGCControlTerminationRequestedBit = 1u << (BrixGCControlPhaseBits + 1),
    BrixGCControlHandshakeReasonShift = 5,
    BrixGCControlHandshakeReasonMask = 0xfu << BrixGCControlHandshakeReasonShift,
    BrixGCControlHandshakeEpochShift = 9,
    BrixGCControlHandshakeEpochMask = 0x7fffffull << BrixGCControlHandshakeEpochShift,
    BrixGCControlSweepGenerationShift = 32
};

enum {
    BrixGCAllocationBufferCursorOffset = offsetof(BrixGCAllocationBuffer, cursor),
    BrixGCAllocationBufferLimitOffset = offsetof(BrixGCAllocationBuffer, limit),
    BrixGCAllocationBufferSize = sizeof(BrixGCAllocationBuffer),
    BrixGCRuntimeAllocationBufferSize = sizeof(BrixGCRuntimeAllocationBuffer),
    BrixGCRuntimeAllocationBufferHotOffset =
        offsetof(BrixGCRuntimeAllocationBuffer, buffer),
    BrixGCRuntimeAllocationBufferSlotSizeOffset =
        offsetof(BrixGCRuntimeAllocationBuffer, slotSize),
    BrixGCRuntimeAllocationBufferDescriptorOffset =
        offsetof(BrixGCRuntimeAllocationBuffer, descriptor),
    BrixGCRuntimeAllocationBufferGenerationOffset =
        offsetof(BrixGCRuntimeAllocationBuffer, generation),
    BrixGCRuntimeAllocationBufferVersionOffset =
        offsetof(BrixGCRuntimeAllocationBuffer, abiVersion),
    BrixGCAllocationSiteKeySize = sizeof(BrixGCAllocationSiteKey),
    BrixGCAllocationSiteKeyVersionOffset = offsetof(BrixGCAllocationSiteKey, abiVersion),
    BrixGCThreadCacheSize = sizeof(BrixGCThreadCache),
    BrixGCThreadCacheActiveBufferOffset = offsetof(BrixGCThreadCache, activeBuffer),
    BrixGCThreadCacheCompilerBuffersOffset = offsetof(BrixGCThreadCache, compilerBuffers),
    BrixGCThreadCacheRangesOffset = offsetof(BrixGCThreadCache, ranges),
    BrixGCThreadCacheWriteBarrierEntriesOffset =
        offsetof(BrixGCThreadCache, writeBarrierEntries),
    BrixGCThreadCacheCountersOffset = offsetof(BrixGCThreadCache, counters),
    BrixGCThreadCacheMarkWorkHandlesOffset = offsetof(BrixGCThreadCache, markWorkHandles)
};

BRIX_WITHOUT_GC static inline uint64_t
brixGCPackControlWord(size_t phase, size_t sweepGeneration, int handshakeRequested) {
    return
        ((uint64_t)sweepGeneration << BrixGCControlSweepGenerationShift) |
        ((uint64_t)phase & BrixGCControlPhaseMask) |
        (handshakeRequested ? BrixGCControlHandshakeRequestedBit : 0);
}

BRIX_WITHOUT_GC static inline uint64_t
brixGCPackControlWordFull(
    size_t phase,
    size_t sweepGeneration,
    int handshakeRequested,
    size_t handshakeReason,
    size_t handshakeEpoch
) {
    return brixGCPackControlWord(phase, sweepGeneration, handshakeRequested) |
        (((uint64_t)handshakeReason << BrixGCControlHandshakeReasonShift) &
            BrixGCControlHandshakeReasonMask) |
        (((uint64_t)handshakeEpoch << BrixGCControlHandshakeEpochShift) &
            BrixGCControlHandshakeEpochMask);
}

BRIX_WITHOUT_GC static inline unsigned char
brixGCControlWordPhase(uint64_t word) {
    return (unsigned char)(word & BrixGCControlPhaseMask);
}

BRIX_WITHOUT_GC static inline size_t
brixGCControlWordSweepGeneration(uint64_t word) {
    return (size_t)(word >> BrixGCControlSweepGenerationShift);
}

BRIX_WITHOUT_GC static inline int
brixGCControlWordHandshakeRequested(uint64_t word) {
    return (word & BrixGCControlHandshakeRequestedBit) != 0;
}

BRIX_WITHOUT_GC static inline unsigned char
brixGCControlWordHandshakeReason(uint64_t word) {
    return (unsigned char)((word & BrixGCControlHandshakeReasonMask) >>
        BrixGCControlHandshakeReasonShift);
}

BRIX_WITHOUT_GC static inline size_t
brixGCControlWordHandshakeEpoch(uint64_t word) {
    return (size_t)((word & BrixGCControlHandshakeEpochMask) >>
        BrixGCControlHandshakeEpochShift);
}

BRIX_WITHOUT_GC static inline void
brixGCInitializeRuntimeAllocationBuffer(BrixGCRuntimeAllocationBuffer* buffer) {
    if (!buffer) {
        return;
    }
    buffer->abiVersion = BrixGCAllocationABIVersion;
    buffer->structSize = (uint32_t)sizeof(*buffer);
}

BRIX_WITHOUT_GC static inline void
brixGCInitializeThreadCache(BrixGCThreadCache* cache) {
    if (!cache) {
        return;
    }
    cache->abiVersion = BrixGCAllocationABIVersion;
    cache->structSize = (uint32_t)sizeof(*cache);
    brixGCInitializeRuntimeAllocationBuffer(&cache->activeBuffer);
    for (size_t index = 0; index < BrixGCCompilerAllocationBufferCount; ++index) {
        brixGCInitializeRuntimeAllocationBuffer(&cache->compilerBuffers[index]);
    }
    for (size_t index = 0; index < BrixGCThreadCacheRangeCount; ++index) {
        brixGCInitializeRuntimeAllocationBuffer(&cache->ranges[index]);
        for (size_t way = 0; way < BrixGCThreadCacheParkedRangeWayCount; ++way) {
            brixGCInitializeRuntimeAllocationBuffer(&cache->parkedRanges[index][way]);
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif
