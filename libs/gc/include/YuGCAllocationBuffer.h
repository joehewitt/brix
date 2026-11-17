#ifndef YU_GC_ALLOCATION_BUFFER_H
#define YU_GC_ALLOCATION_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include "YuGCAnnotations.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef YU_GC_ALLOCATION_BUFFER_STRUCT
#define YU_GC_ALLOCATION_BUFFER_STRUCT
typedef struct YumiGCAllocationBuffer {
    char* cursor;
    char* limit;
} YumiGCAllocationBuffer;
#endif

#ifndef YU_GC_RUNTIME_ALLOCATION_BUFFER_STRUCT
#define YU_GC_RUNTIME_ALLOCATION_BUFFER_STRUCT
typedef struct YumiGCRuntimeAllocationBuffer {
    YumiGCAllocationBuffer buffer;
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
} YumiGCRuntimeAllocationBuffer;
#endif

enum {
    YumiGCAllocationABIVersion = 5,
    YumiGCCompilerAllocationBufferCount = 512,
    YumiGCThreadCacheRangeCount = 193,
    YumiGCThreadCacheRangeWayCount = 4,
    YumiGCThreadCacheParkedRangeWayCount = YumiGCThreadCacheRangeWayCount - 1,
    YumiGCWriteBarrierBufferCapacity = 512,
    YumiGCThreadCacheCounterCount = 8,
    YumiGCThreadCacheMarkWorkHandleCount = 2
};

typedef struct YumiGCAllocationSiteKey {
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
} YumiGCAllocationSiteKey;

#define YU_GC_ALLOCATION_SITE_KEY_STRUCT 1

typedef struct YumiGCThreadCache {
    uint32_t abiVersion;
    uint32_t structSize;
    uint64_t ownerEpoch;
    uint64_t flushEpoch;
    YumiGCRuntimeAllocationBuffer activeBuffer;
    YumiGCRuntimeAllocationBuffer compilerBuffers[YumiGCCompilerAllocationBufferCount];
    YumiGCRuntimeAllocationBuffer ranges[YumiGCThreadCacheRangeCount];
    char* writeBarrierEntries[YumiGCWriteBarrierBufferCapacity];
    uint32_t writeBarrierCount;
    uint32_t activeCompilerBufferCount;
    uint32_t activeCompilerBufferIndices[YumiGCCompilerAllocationBufferCount];
    uint64_t counters[YumiGCThreadCacheCounterCount];
    void* markWorkHandles[YumiGCThreadCacheMarkWorkHandleCount];
    YumiGCRuntimeAllocationBuffer
        parkedRanges[YumiGCThreadCacheRangeCount][YumiGCThreadCacheParkedRangeWayCount];
    unsigned char rangeNextVictimWay[YumiGCThreadCacheRangeCount];
} YumiGCThreadCache;

enum {
    YumiGCControlPhaseBits = 3,
    YumiGCControlPhaseMask = (1u << YumiGCControlPhaseBits) - 1u,
    YumiGCControlHandshakeRequestedBit = 1u << YumiGCControlPhaseBits,
    YumiGCControlTerminationRequestedBit = 1u << (YumiGCControlPhaseBits + 1),
    YumiGCControlHandshakeReasonShift = 5,
    YumiGCControlHandshakeReasonMask = 0xfu << YumiGCControlHandshakeReasonShift,
    YumiGCControlHandshakeEpochShift = 9,
    YumiGCControlHandshakeEpochMask = 0x7fffffull << YumiGCControlHandshakeEpochShift,
    YumiGCControlSweepGenerationShift = 32
};

enum {
    YumiGCAllocationBufferCursorOffset = offsetof(YumiGCAllocationBuffer, cursor),
    YumiGCAllocationBufferLimitOffset = offsetof(YumiGCAllocationBuffer, limit),
    YumiGCAllocationBufferSize = sizeof(YumiGCAllocationBuffer),
    YumiGCRuntimeAllocationBufferSize = sizeof(YumiGCRuntimeAllocationBuffer),
    YumiGCRuntimeAllocationBufferHotOffset =
        offsetof(YumiGCRuntimeAllocationBuffer, buffer),
    YumiGCRuntimeAllocationBufferSlotSizeOffset =
        offsetof(YumiGCRuntimeAllocationBuffer, slotSize),
    YumiGCRuntimeAllocationBufferDescriptorOffset =
        offsetof(YumiGCRuntimeAllocationBuffer, descriptor),
    YumiGCRuntimeAllocationBufferGenerationOffset =
        offsetof(YumiGCRuntimeAllocationBuffer, generation),
    YumiGCRuntimeAllocationBufferVersionOffset =
        offsetof(YumiGCRuntimeAllocationBuffer, abiVersion),
    YumiGCAllocationSiteKeySize = sizeof(YumiGCAllocationSiteKey),
    YumiGCAllocationSiteKeyVersionOffset = offsetof(YumiGCAllocationSiteKey, abiVersion),
    YumiGCThreadCacheSize = sizeof(YumiGCThreadCache),
    YumiGCThreadCacheActiveBufferOffset = offsetof(YumiGCThreadCache, activeBuffer),
    YumiGCThreadCacheCompilerBuffersOffset = offsetof(YumiGCThreadCache, compilerBuffers),
    YumiGCThreadCacheRangesOffset = offsetof(YumiGCThreadCache, ranges),
    YumiGCThreadCacheWriteBarrierEntriesOffset =
        offsetof(YumiGCThreadCache, writeBarrierEntries),
    YumiGCThreadCacheCountersOffset = offsetof(YumiGCThreadCache, counters),
    YumiGCThreadCacheMarkWorkHandlesOffset = offsetof(YumiGCThreadCache, markWorkHandles)
};

YUMI_WITHOUT_GC static inline uint64_t
yumiGCPackControlWord(size_t phase, size_t sweepGeneration, int handshakeRequested) {
    return
        ((uint64_t)sweepGeneration << YumiGCControlSweepGenerationShift) |
        ((uint64_t)phase & YumiGCControlPhaseMask) |
        (handshakeRequested ? YumiGCControlHandshakeRequestedBit : 0);
}

YUMI_WITHOUT_GC static inline uint64_t
yumiGCPackControlWordFull(
    size_t phase,
    size_t sweepGeneration,
    int handshakeRequested,
    size_t handshakeReason,
    size_t handshakeEpoch
) {
    return yumiGCPackControlWord(phase, sweepGeneration, handshakeRequested) |
        (((uint64_t)handshakeReason << YumiGCControlHandshakeReasonShift) &
            YumiGCControlHandshakeReasonMask) |
        (((uint64_t)handshakeEpoch << YumiGCControlHandshakeEpochShift) &
            YumiGCControlHandshakeEpochMask);
}

YUMI_WITHOUT_GC static inline unsigned char
yumiGCControlWordPhase(uint64_t word) {
    return (unsigned char)(word & YumiGCControlPhaseMask);
}

YUMI_WITHOUT_GC static inline size_t
yumiGCControlWordSweepGeneration(uint64_t word) {
    return (size_t)(word >> YumiGCControlSweepGenerationShift);
}

YUMI_WITHOUT_GC static inline int
yumiGCControlWordHandshakeRequested(uint64_t word) {
    return (word & YumiGCControlHandshakeRequestedBit) != 0;
}

YUMI_WITHOUT_GC static inline unsigned char
yumiGCControlWordHandshakeReason(uint64_t word) {
    return (unsigned char)((word & YumiGCControlHandshakeReasonMask) >>
        YumiGCControlHandshakeReasonShift);
}

YUMI_WITHOUT_GC static inline size_t
yumiGCControlWordHandshakeEpoch(uint64_t word) {
    return (size_t)((word & YumiGCControlHandshakeEpochMask) >>
        YumiGCControlHandshakeEpochShift);
}

YUMI_WITHOUT_GC static inline void
yumiGCInitializeRuntimeAllocationBuffer(YumiGCRuntimeAllocationBuffer* buffer) {
    if (!buffer) {
        return;
    }
    buffer->abiVersion = YumiGCAllocationABIVersion;
    buffer->structSize = (uint32_t)sizeof(*buffer);
}

YUMI_WITHOUT_GC static inline void
yumiGCInitializeThreadCache(YumiGCThreadCache* cache) {
    if (!cache) {
        return;
    }
    cache->abiVersion = YumiGCAllocationABIVersion;
    cache->structSize = (uint32_t)sizeof(*cache);
    yumiGCInitializeRuntimeAllocationBuffer(&cache->activeBuffer);
    for (size_t index = 0; index < YumiGCCompilerAllocationBufferCount; ++index) {
        yumiGCInitializeRuntimeAllocationBuffer(&cache->compilerBuffers[index]);
    }
    for (size_t index = 0; index < YumiGCThreadCacheRangeCount; ++index) {
        yumiGCInitializeRuntimeAllocationBuffer(&cache->ranges[index]);
        for (size_t way = 0; way < YumiGCThreadCacheParkedRangeWayCount; ++way) {
            yumiGCInitializeRuntimeAllocationBuffer(&cache->parkedRanges[index][way]);
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif
