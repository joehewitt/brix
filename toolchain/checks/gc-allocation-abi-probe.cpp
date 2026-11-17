#include <BxGCAllocationBuffer.h>

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
        (unsigned)BrixGCAllocationABIVersion,
        (size_t)BrixGCAllocationBufferCursorOffset,
        (size_t)BrixGCAllocationBufferLimitOffset,
        (size_t)BrixGCAllocationBufferSize,
        (size_t)BrixGCRuntimeAllocationBufferSize,
        (size_t)BrixGCRuntimeAllocationBufferHotOffset,
        (size_t)BrixGCRuntimeAllocationBufferSlotSizeOffset,
        (size_t)BrixGCRuntimeAllocationBufferDescriptorOffset,
        (size_t)BrixGCRuntimeAllocationBufferGenerationOffset,
        (size_t)BrixGCRuntimeAllocationBufferVersionOffset,
        (size_t)BrixGCAllocationSiteKeySize,
        (size_t)BrixGCAllocationSiteKeyVersionOffset,
        (size_t)BrixGCThreadCacheSize,
        (size_t)BrixGCThreadCacheActiveBufferOffset,
        (size_t)BrixGCThreadCacheCompilerBuffersOffset,
        (size_t)BrixGCThreadCacheRangesOffset,
        (size_t)BrixGCThreadCacheWriteBarrierEntriesOffset,
        (size_t)BrixGCThreadCacheCountersOffset,
        (size_t)BrixGCThreadCacheMarkWorkHandlesOffset,
        (unsigned)BrixGCThreadCacheRangeCount,
        (unsigned)BrixGCCompilerAllocationBufferCount,
        (unsigned)BrixGCControlPhaseMask,
        (unsigned)BrixGCControlHandshakeRequestedBit,
        (unsigned)BrixGCControlHandshakeReasonShift,
        (unsigned)BrixGCControlHandshakeEpochShift,
        (unsigned)BrixGCControlSweepGenerationShift);
    return 0;
}
