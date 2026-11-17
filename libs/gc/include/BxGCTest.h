#ifndef BX_GC_TEST_H
#define BX_GC_TEST_H

#include "BxGC.h"

#ifdef __cplusplus
extern "C" {
#endif

// Manual phase controls are collector-internal test hooks. Applications use automatic pacing
// and brixCollect() for an explicitly blocking collection.
BRIX_WITHOUT_BLOCKING bool brixGCTestAllocBatch(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    BrixGCAllocationBatch* batch);
BRIX_WITHOUT_GC bool brixGCTestDiscardBatch(
    char* cursor,
    char* limit,
    size_t slotSize,
    size_t generation);
BRIX_WITHOUT_BLOCKING void brixGCStartConcurrentMark(void);
BRIX_WITHOUT_BLOCKING size_t brixGCStepConcurrentMark(void);
BRIX_WITHOUT_BLOCKING void brixGCFinishConcurrentMark(void);
BRIX_WITHOUT_BLOCKING void brixGCFinishAutomaticCycle(void);
BRIX_WITHOUT_BLOCKING void brixGCStepIncrementalSweep(size_t workLimit);
BRIX_WITHOUT_BLOCKING void brixGCStepScavenge(size_t pageBudget);
BRIX_WITHOUT_BLOCKING size_t brixGCMutatorAssist(size_t workLimit);

void* brixGCTestSinglePointerDescriptorPointer(void);
void* brixGCTestAlternateSinglePointerDescriptorPointer(void);

// Test-only views of the production two-level address index. The arena view returns its aligned
// base; the span view is an opaque identity, so tests do not depend on collector metadata layout.
void* brixGCTestAddressIndexArena(void* pointer);
void* brixGCTestAddressIndexSpan(void* pointer);
size_t brixGCTestAddressIndexLookupLoads(void* pointer);

// Stage-3 WorkBuf shape, persistent high-water allocation, and distributed-quiescence state.
size_t brixGCTestMarkWorkBufferSlots(void);
size_t brixGCTestMarkWorkRetainedBytes(void);
size_t brixGCTestMarkWorkParticipantCount(void);
size_t brixGCTestMarkWorkActiveMarkers(void);
size_t brixGCTestMarkWorkMaxActiveMarkers(void);
uint64_t brixGCTestMarkWorkFlushedEpoch(void);
bool brixGCTestDeepDescriptorTrace(size_t depth);
bool brixGCTestConcurrentMarkParticipants(size_t count);

// Models a pointer acquired into a task register after its setup snapshot. Checkmark observes
// this test root, while normal marking deliberately does not, so omission fixtures can prove
// that either the deletion or insertion half was required.
void brixGCTestSetCheckmarkExtraRoot(void* pointer);

// Exact span/mark state views for sweep-publication regression tests. These expose only
// booleans so fixtures do not depend on generation values or bitmap representation.
bool brixGCTestObjectSpanSwept(void* pointer);
bool brixGCTestObjectMarked(void* pointer);
bool brixGCTestSweepUntilObjectSpanSwept(void* pointer);
size_t brixGCTestSizeClassIndexForSize(size_t size);
size_t brixGCTestSizeClassSweepCursor(size_t index);
size_t brixGCTestSizeClassSweepLimit(size_t index);
size_t brixGCTestSizeClassSweepRegistrySize(size_t index);
size_t brixGCTestLargeSweepCursor(void);
size_t brixGCTestLargeSweepLimit(void);
size_t brixGCTestLargeSweepRegistrySize(void);

// Returns the count of slots that remain allocated in the span containing pointer.
// Used only to prove that an allocate-black unused batch tail is reclaimed at its flip.
size_t brixGCTestObjectSpanAllocatedCount(void* pointer);

// Exercises the production deferred-span queue while the calling test retains a lookup result.
bool brixGCTestDeferredSpanRetirement(void);
bool brixGCTestThreadCacheOwnerFlushOnce(void);
size_t brixGCTestAdaptiveBatchPreferredCount(size_t compilerBufferIndex);
size_t brixGCTestParkedAllocationRangeCount(size_t sizeClassIndex);
size_t brixGCTestCompilerRangePacerChargedBytes(void);
size_t brixGCTestGenericRangePacerChargedBytes(void);
size_t brixGCTestGenericRangePacerCollectionCheckCount(void);
void brixGCTestCrossAllocationStatPublicationThreshold(void);

// Exact structural counters for the lifetime registry's cancellation path. A successful managed
// cleanup cancellation performs one indexed lookup and one intrusive root unlink regardless of
// registry size or cancellation order.
void brixGCTestResetLifetimeLookupCounters(void);
size_t brixGCTestCleanupCancelLookupCount(void);
size_t brixGCTestRootUnlinkCount(void);
size_t brixGCTestRootRegistrationCount(void);
size_t brixGCTestOwnerWeakRegistrationCount(void);
BRIX_WITHOUT_BLOCKING bool brixGCTestTemporaryRootStorage(char* pointer);

#ifdef __cplusplus
}
#endif

#endif
