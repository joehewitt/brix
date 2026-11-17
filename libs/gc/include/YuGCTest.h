#ifndef YU_GC_TEST_H
#define YU_GC_TEST_H

#include "YuGC.h"

#ifdef __cplusplus
extern "C" {
#endif

// Manual phase controls are collector-internal test hooks. Applications use automatic pacing
// and yumiCollect() for an explicitly blocking collection.
YUMI_WITHOUT_BLOCKING bool yumiGCTestAllocBatch(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    YumiGCAllocationBatch* batch);
YUMI_WITHOUT_GC bool yumiGCTestDiscardBatch(
    char* cursor,
    char* limit,
    size_t slotSize,
    size_t generation);
YUMI_WITHOUT_BLOCKING void yumiGCStartConcurrentMark(void);
YUMI_WITHOUT_BLOCKING size_t yumiGCStepConcurrentMark(void);
YUMI_WITHOUT_BLOCKING void yumiGCFinishConcurrentMark(void);
YUMI_WITHOUT_BLOCKING void yumiGCFinishAutomaticCycle(void);
YUMI_WITHOUT_BLOCKING void yumiGCStepIncrementalSweep(size_t workLimit);
YUMI_WITHOUT_BLOCKING void yumiGCStepScavenge(size_t pageBudget);
YUMI_WITHOUT_BLOCKING size_t yumiGCMutatorAssist(size_t workLimit);

void* yumiGCTestSinglePointerDescriptorPointer(void);
void* yumiGCTestAlternateSinglePointerDescriptorPointer(void);

// Test-only views of the production two-level address index. The arena view returns its aligned
// base; the span view is an opaque identity, so tests do not depend on collector metadata layout.
void* yumiGCTestAddressIndexArena(void* pointer);
void* yumiGCTestAddressIndexSpan(void* pointer);
size_t yumiGCTestAddressIndexLookupLoads(void* pointer);

// Stage-3 WorkBuf shape, persistent high-water allocation, and distributed-quiescence state.
size_t yumiGCTestMarkWorkBufferSlots(void);
size_t yumiGCTestMarkWorkRetainedBytes(void);
size_t yumiGCTestMarkWorkParticipantCount(void);
size_t yumiGCTestMarkWorkActiveMarkers(void);
size_t yumiGCTestMarkWorkMaxActiveMarkers(void);
uint64_t yumiGCTestMarkWorkFlushedEpoch(void);
bool yumiGCTestDeepDescriptorTrace(size_t depth);
bool yumiGCTestConcurrentMarkParticipants(size_t count);

// Models a pointer acquired into a task register after its setup snapshot. Checkmark observes
// this test root, while normal marking deliberately does not, so omission fixtures can prove
// that either the deletion or insertion half was required.
void yumiGCTestSetCheckmarkExtraRoot(void* pointer);

// Exact span/mark state views for sweep-publication regression tests. These expose only
// booleans so fixtures do not depend on generation values or bitmap representation.
bool yumiGCTestObjectSpanSwept(void* pointer);
bool yumiGCTestObjectMarked(void* pointer);
bool yumiGCTestSweepUntilObjectSpanSwept(void* pointer);
size_t yumiGCTestSizeClassIndexForSize(size_t size);
size_t yumiGCTestSizeClassSweepCursor(size_t index);
size_t yumiGCTestSizeClassSweepLimit(size_t index);
size_t yumiGCTestSizeClassSweepRegistrySize(size_t index);
size_t yumiGCTestLargeSweepCursor(void);
size_t yumiGCTestLargeSweepLimit(void);
size_t yumiGCTestLargeSweepRegistrySize(void);

// Returns the count of slots that remain allocated in the span containing pointer.
// Used only to prove that an allocate-black unused batch tail is reclaimed at its flip.
size_t yumiGCTestObjectSpanAllocatedCount(void* pointer);

// Exercises the production deferred-span queue while the calling test retains a lookup result.
bool yumiGCTestDeferredSpanRetirement(void);
bool yumiGCTestThreadCacheOwnerFlushOnce(void);
size_t yumiGCTestAdaptiveBatchPreferredCount(size_t compilerBufferIndex);
size_t yumiGCTestParkedAllocationRangeCount(size_t sizeClassIndex);
size_t yumiGCTestCompilerRangePacerChargedBytes(void);
size_t yumiGCTestGenericRangePacerChargedBytes(void);
size_t yumiGCTestGenericRangePacerCollectionCheckCount(void);
void yumiGCTestCrossAllocationStatPublicationThreshold(void);

// Exact structural counters for the lifetime registry's cancellation path. A successful managed
// cleanup cancellation performs one indexed lookup and one intrusive root unlink regardless of
// registry size or cancellation order.
void yumiGCTestResetLifetimeLookupCounters(void);
size_t yumiGCTestCleanupCancelLookupCount(void);
size_t yumiGCTestRootUnlinkCount(void);
size_t yumiGCTestRootRegistrationCount(void);
size_t yumiGCTestOwnerWeakRegistrationCount(void);
YUMI_WITHOUT_BLOCKING bool yumiGCTestTemporaryRootStorage(char* pointer);

#ifdef __cplusplus
}
#endif

#endif
