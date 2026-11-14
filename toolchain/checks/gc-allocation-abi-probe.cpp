#include <YuGCAllocationBuffer.h>

#include <cstdio>

int main() {
    std::printf(
        "{\"version\":%u,\"cursorOffset\":%zu,\"limitOffset\":%zu,"
        "\"size\":%zu,\"runtimeSize\":%zu,\"runtimeHotOffset\":%zu,"
        "\"runtimeSlotSizeOffset\":%zu,\"runtimeDescriptorOffset\":%zu,"
        "\"runtimeGenerationOffset\":%zu,\"runtimeVersionOffset\":%zu,"
        "\"siteKeySize\":%zu,\"siteKeyVersionOffset\":%zu,"
        "\"threadCacheSize\":%zu,\"threadCacheActiveBufferOffset\":%zu,"
        "\"threadCacheCompilerBuffersOffset\":%zu,\"threadCacheRangesOffset\":%zu,"
        "\"threadCacheWriteBarrierEntriesOffset\":%zu,"
        "\"threadCacheCountersOffset\":%zu,\"threadCacheMarkWorkHandlesOffset\":%zu,"
        "\"threadCacheRangeCount\":%u,\"compilerBufferCount\":%u,"
        "\"controlPhaseMask\":%u,\"controlHandshakeRequestedBit\":%u,"
        "\"controlHandshakeReasonShift\":%u,\"controlHandshakeEpochShift\":%u,"
        "\"controlSweepGenerationShift\":%u}\n",
        (unsigned)YumiGCAllocationABIVersion,
        (size_t)YumiGCAllocationBufferCursorOffset,
        (size_t)YumiGCAllocationBufferLimitOffset,
        (size_t)YumiGCAllocationBufferSize,
        (size_t)YumiGCRuntimeAllocationBufferSize,
        (size_t)YumiGCRuntimeAllocationBufferHotOffset,
        (size_t)YumiGCRuntimeAllocationBufferSlotSizeOffset,
        (size_t)YumiGCRuntimeAllocationBufferDescriptorOffset,
        (size_t)YumiGCRuntimeAllocationBufferGenerationOffset,
        (size_t)YumiGCRuntimeAllocationBufferVersionOffset,
        (size_t)YumiGCAllocationSiteKeySize,
        (size_t)YumiGCAllocationSiteKeyVersionOffset,
        (size_t)YumiGCThreadCacheSize,
        (size_t)YumiGCThreadCacheActiveBufferOffset,
        (size_t)YumiGCThreadCacheCompilerBuffersOffset,
        (size_t)YumiGCThreadCacheRangesOffset,
        (size_t)YumiGCThreadCacheWriteBarrierEntriesOffset,
        (size_t)YumiGCThreadCacheCountersOffset,
        (size_t)YumiGCThreadCacheMarkWorkHandlesOffset,
        (unsigned)YumiGCThreadCacheRangeCount,
        (unsigned)YumiGCCompilerAllocationBufferCount,
        (unsigned)YumiGCControlPhaseMask,
        (unsigned)YumiGCControlHandshakeRequestedBit,
        (unsigned)YumiGCControlHandshakeReasonShift,
        (unsigned)YumiGCControlHandshakeEpochShift,
        (unsigned)YumiGCControlSweepGenerationShift);
    return 0;
}
