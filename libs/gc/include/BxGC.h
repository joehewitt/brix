#ifndef BX_GC_H
#define BX_GC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <BxGC/BxGCAnnotations.h>
#include <BxGC/BxGCTemporaryRoots.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BrixGCAllocationBatch {
    char* cursor;
    char* limit;
    size_t slotSize;
} BrixGCAllocationBatch;

// The language wrapper contains and reports recoverable errors before crossing this C ABI.
// The return token is reserved; draining continues regardless of its value.
typedef size_t (*BrixGCCleanupCallback)(void* context);

#include <BxGC/BxGCAllocationBuffer.h>

#ifndef BX_GC_PAUSE_KIND_ENUM
#define BX_GC_PAUSE_KIND_ENUM
enum BrixGCPauseKind {
    BrixGCPauseKindMarkSetupTotal = 1,
    BrixGCPauseKindPerTaskSetupScan = 2,
    BrixGCPauseKindTermConfirm = 3,
    BrixGCPauseKindSweepTotal = 4,
    BrixGCPauseKindStallReport = 5,
    BrixGCPauseKindCount = 6
};
#endif

#ifndef BX_OBJECT_SIDE_PAYLOAD_LANE_ENUM
#define BX_OBJECT_SIDE_PAYLOAD_LANE_ENUM
enum BrixObjectSidePayloadLane {
    // Strong. Property subscriptions migrate to weak-backed subscriptions.
    BrixObjectSidePayloadPropertySubscribers = 0,
    // Strong: the payload is intentionally owned for the owner's lifetime.
    BrixObjectSidePayloadOwnedLiveActions = 1,
    // Strong and cleanup-participating; observer deactivation is the cleanup action.
    BrixObjectSidePayloadCollectionObservers = 2,
    // Strong. Cleanup retires the attachment when its owner becomes unreachable.
    BrixObjectSidePayloadMarkupAttachments = 3,
    // Strong. Cleanup retires the attachment when its owner becomes unreachable.
    BrixObjectSidePayloadStyleAttachments = 4,
    // Strong and explicitly clearable.
    BrixObjectSidePayloadStyleTemplates = 5,
    // Transitional strong lane; clients migrate to weak references plus cleanup.
    BrixObjectSidePayloadStyleObjects = 6,
    // Strong. Event endpoint entries live exactly as long as their owner.
    BrixObjectSidePayloadEvents = 7,
    BrixObjectSidePayloadOwnedTasks = 8,
    BrixObjectSidePayloadLaneCount = 9
};
#endif

#ifndef BX_GC_BUFFER_RESIZE_ENUM
#define BX_GC_BUFFER_RESIZE_ENUM
enum BrixGCBufferResize {
    BrixGCBufferResizeRejected = 0,
    BrixGCBufferResizeInPlace = 1,
    BrixGCBufferResizeReplace = 2
};
#endif

BRIX_WITHOUT_BLOCKING void brixGCInit(void);
BRIX_WITHOUT_BLOCKING void brixGCScavenge(void);
BRIX_WITHOUT_GC unsigned char brixGCCollectorPhase(void);

BRIX_WITHOUT_BLOCKING char* brixGCAllocObject(size_t size);
BRIX_WITHOUT_BLOCKING char* brixGCAllocObjectDescribed(size_t size, void* descriptor);
BRIX_WITHOUT_BLOCKING char* brixGCAllocBuffer(size_t itemSize, size_t count);
BRIX_WITHOUT_BLOCKING char* brixGCAllocBufferDescribed(
    size_t itemSize,
    size_t count,
    void* descriptor
);
BRIX_WITHOUT_BLOCKING char* brixGCResizeBuffer(char* buffer, size_t oldSize, size_t newSize);
BRIX_WITHOUT_BLOCKING char* brixGCResizeBufferDescribed(char* buffer, size_t oldSize, size_t newSize, void* descriptor);
BRIX_WITHOUT_BLOCKING enum BrixGCBufferResize brixGCResizeBufferPrepare(
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* descriptor,
    void** replacementDescriptor);
BRIX_WITHOUT_BLOCKING void brixGCResizeBufferCopy(
    char* replacement,
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* replacementDescriptor);
BRIX_WITHOUT_BLOCKING char* brixAllocObjectWithSiteKey(const BrixGCAllocationSiteKey* key);

BRIX_WITHOUT_BLOCKING void* brixGCAddRoot(char* pointer);
BRIX_WITHOUT_BLOCKING void* brixGCAddTemporaryRoot(char* pointer);
BRIX_WITHOUT_GC void* brixGCAddIdleTemporaryRoot(char* pointer);
BRIX_WITHOUT_BLOCKING void* brixGCAddDescribedRoot(
    char* pointer,
    size_t byteSize,
    void* descriptor);
BRIX_WITHOUT_GC void brixGCUpdateRoot(void* root, char* pointer);
BRIX_WITHOUT_GC void brixGCRemoveRoot(void* root);
BRIX_WITHOUT_GC void brixGCRemoveTemporaryRoot(void* root);
BRIX_WITHOUT_GC bool brixGCRemoveTaskTemporaryRoot(void* root);
BRIX_WITHOUT_GC void brixGCDrainTemporaryRoots(BrixGCTemporaryRootStorage* storage);
BRIX_WITHOUT_BLOCKING void brixGCCollect(void);
BRIX_WITHOUT_BLOCKING void brixGCCollectForAllocation(void);
#ifndef BX_GC_COLLECT_RESULT_TYPE
#define BX_GC_COLLECT_RESULT_TYPE
typedef size_t BrixGCCollectResult;
enum {
    BrixGCCollectCompleted = 0
};
#endif
BRIX_WITHOUT_BLOCKING BrixGCCollectResult brixCollect(void);
BRIX_WITHOUT_BLOCKING void brixGCFrameBudget(uint64_t remainingNanos);
BRIX_WITHOUT_GC bool brixGCVerifyObject(char* pointer);
BRIX_WITHOUT_GC void* brixGCObjectDescriptor(char* pointer);
BRIX_WITHOUT_GC void brixGCWriteBarrier(char* source, char* value);
BRIX_WITHOUT_GC void brixGCBulkWriteBarrier(char* source, char* value, size_t byteSize, void* descriptor);
BRIX_WITHOUT_GC void brixGCSetHeapGoal(size_t goal);
BRIX_WITHOUT_GC void brixGCSetMinimumHeapGoal(size_t goal);
BRIX_WITHOUT_BLOCKING size_t brixGCRegisterOwnerWeak(char* owner, char* target);
BRIX_WITHOUT_BLOCKING char* brixGCOwnerWeakLoad(size_t handle);
BRIX_WITHOUT_BLOCKING bool brixGCReleaseOwnerWeak(size_t handle);
BRIX_WITHOUT_BLOCKING size_t brixGCRegisterCleanup(
    char* target,
    char* context,
    BrixGCCleanupCallback callback);
BRIX_WITHOUT_BLOCKING size_t brixGCRegisterNativeCleanup(
    char* target,
    void* context,
    BrixGCCleanupCallback callback);
BRIX_WITHOUT_BLOCKING bool brixGCCancelCleanup(size_t handle);
BRIX_WITHOUT_BLOCKING size_t brixGCDrainCleanups(void);
BRIX_WITHOUT_GC void brixGCQueueFileDescriptorCleanup(int fd);
BRIX_WITHOUT_GC void brixGCReportCleanupException(void);
BRIX_WITHOUT_BLOCKING char* brixGCObjectSidePayload(char* object, size_t lane);
BRIX_WITHOUT_BLOCKING char* brixGCPublishObjectSidePayload(
    char* object,
    size_t lane,
    char* payload
);
BRIX_WITHOUT_BLOCKING char* brixGCPublishCleanupObjectSidePayload(
    char* object,
    size_t lane,
    char* payload,
    BrixGCCleanupCallback callback);
BRIX_WITHOUT_GC void brixGCClearObjectSidePayload(char* object, size_t lane);
BRIX_WITHOUT_GC bool brixGCClearCleanupObjectSidePayload(char* object, size_t lane);
BRIX_WITHOUT_GC bool brixGCHasObjectSidePayload(char* object, size_t lane);
BRIX_WITHOUT_GC size_t brixGCObjectSidePayloadCount(size_t lane);
BRIX_WITHOUT_GC size_t brixGCCollectionObserversSidePayloadLane(void);
BRIX_WITHOUT_GC size_t brixGCMarkupAttachmentsSidePayloadLane(void);
BRIX_WITHOUT_GC size_t brixGCStyleAttachmentsSidePayloadLane(void);
BRIX_WITHOUT_GC size_t brixGCStyleTemplatesSidePayloadLane(void);
BRIX_WITHOUT_GC size_t brixGCStyleObjectsSidePayloadLane(void);
BRIX_WITHOUT_GC size_t brixGCEventsSidePayloadLane(void);
BRIX_WITHOUT_BLOCKING size_t brixCollectionObserverActivate(char* owner);
BRIX_WITHOUT_BLOCKING bool brixCollectionObserverDeactivate(size_t cleanup);
BRIX_WITHOUT_GC bool brixCollectionMayBeObserved(char* collection);
BRIX_WITHOUT_GC size_t brixCollectionObserverActiveCount(void);

// The remaining public GC surface is metric/snapshot access. Keep the classification grouped so
// newly added observability imports cannot silently fall back to the blocking policy.
#if defined(__clang__)
#pragma clang attribute push(__attribute__((annotate("brix:gc_leaf"))), apply_to = function)
#endif

size_t brixGCBarrierCount(void);
size_t brixGCAssistCount(void);
size_t brixGCAssistTimeNanos(void);
size_t brixGCAllocatedObjectCount(void);
size_t brixGCRequestedByteCount(void);
size_t brixGCAllocatedByteCount(void);
size_t brixGCLiveByteCount(void);
size_t brixGCAllocatedSpanCount(void);
size_t brixGCAllocatedArenaCount(void);
size_t brixGCHeapGoal(void);
size_t brixGCPacerGrowthPercent(void);
size_t brixGCBackgroundWorkerCount(void);
size_t brixGCBackgroundWorkerThreadCapacity(void);
size_t brixGCBackgroundWorkerDutyPermille(void);
size_t brixGCBackgroundWorkerWork(void);
size_t brixGCBackgroundCredit(void);
size_t brixGCFrameBudgetOfferCount(void);
size_t brixGCFrameBudgetWorkCount(void);
size_t brixGCFrameBudgetWorkNanos(void);
size_t brixGCFrameBudgetOverrunCount(void);
size_t brixGCPacerHardCap(void);
size_t brixGCPacerSoftGoal(void);
size_t brixGCPacerHardGoal(void);
size_t brixGCPacerLastTriggerReason(void);
size_t brixGCCollectionCount(void);
size_t brixGCWeakClearCount(void);
size_t brixGCCleanupRegistrationCount(void);
size_t brixGCCleanupPendingCount(void);
size_t brixGCCleanupEnqueueCount(void);
size_t brixGCCleanupFireCount(void);
size_t brixGCCleanupCancelCount(void);
size_t brixGCCleanupExceptionCount(void);
size_t brixGCMarkTimeNanos(void);
size_t brixGCSweepTimeNanos(void);
size_t brixGCAllocationStallCount(void);

// Benchmark-only phase markers. Instrumented compiler builds emit these calls; clean benchmark
// builds elide them before object emission.
BRIX_WITHOUT_GC void brixBenchMutatorPhaseStart(const char* name);
BRIX_WITHOUT_GC void brixBenchMutatorPhaseEnd(const char* name);
size_t brixGCAllocationStallTimeNanos(void);
size_t brixGCHandshakeCount(void);
size_t brixGCHandshakeLatencyNanos(void);
size_t brixGCCurrentCycleId(void);
size_t brixGCActiveCycleAgeNanos(void);
size_t brixGCMarkQueueDepth(void);
size_t brixGCAssistDebt(void);
size_t brixGCMarkQueuePushCount(void);
size_t brixGCMarkQueuePopCount(void);
size_t brixGCMarkQueuePopBatchCount(void);
size_t brixGCMarkQueueMaxPopBatch(void);
size_t brixGCMarkParticipantCount(void);
size_t brixGCActiveMarkerCount(void);
uint64_t brixGCWorkFlushedEpoch(void);
size_t brixGCTerminationConfirmCount(void);
size_t brixGCTerminationConfirmResumeCount(void);
size_t brixGCBatchReservedBytes(void);
size_t brixGCBatchConsumedBytes(void);
size_t brixGCBatchReleasedBytes(void);
size_t brixGCBatchDiscardedBytes(void);
size_t brixGCBatchCacheBytes(void);
size_t brixGCBatchUnusedBytes(void);
size_t brixGCThreadBatchCacheBytes(void);
size_t brixGCThreadPendingAllocatedObjectCount(void);
size_t brixGCThreadPendingBatchConsumedBytes(void);
size_t brixGCStaleBatchDropCount(void);
size_t brixGCFallbackAllocationCount(void);
size_t brixGCAllocationCallCount(size_t kind);
size_t brixGCAllocationPhaseCount(size_t phase);
size_t brixGCAllocationFastHitCount(void);
size_t brixGCAllocationFastRefillCount(void);
size_t brixGCAllocationFastFallbackCount(void);
size_t brixGCAllocationLargeCount(void);
size_t brixGCAllocationBatchMissCount(size_t reason);
size_t brixGCAllocationZeroedBytes(void);
size_t brixGCAllocationStatPublicationCount(void);
size_t brixGCAllocationAssistCheckCount(void);
size_t brixGCAllocationAssistCallCount(void);
size_t brixGCFastBarrierCallCount(void);
size_t brixGCFastBarrierSlowEntryCount(void);
size_t brixGCAllocationDescriptorMixCount(size_t index, size_t described);
size_t brixGCAllocationSiteHitCount(void);
size_t brixGCAllocationSiteMissCount(void);
size_t brixGCAllocationSiteRefillCount(void);
size_t brixGCAllocationSiteFallbackCount(void);
size_t brixGCAutomaticFallbackCount(void);
size_t brixGCAutomaticFallbackReason(void);
size_t brixGCAutomaticDrainGiveUpCount(void);
size_t brixGCRssBytes(void);
size_t brixGCCommittedBytes(void);
size_t brixGCCommittedPageCount(void);
size_t brixGCDecommittedPageCount(void);
size_t brixGCDirtyPageCount(void);
size_t brixGCFreePageCount(void);
size_t brixGCReservedArenaBytes(void);
size_t brixGCLargeSpanFragmentationBytes(void);
size_t brixGCFragmentationBytes(void);
size_t brixGCSizeClassOccupancyBucketCount(size_t index, size_t bucket);
size_t brixGCScavengeDebt(void);
size_t brixGCScavengeDebtBytes(void);
size_t brixGCFreeCommittedBytes(void);
size_t brixGCFreeCommittedRunCount(void);
size_t brixGCLargestFreeCommittedRunBytes(void);
size_t brixGCAllocatedPageBytes(void);
size_t brixGCPeakDemandBytes(void);
size_t brixGCRetentionTargetBytes(void);
size_t brixGCSizeClassSweepDebt(size_t index);
size_t brixGCLargeSpanSweepDebt(void);
size_t brixGCMarkSetupSampleCount(void);
size_t brixGCMarkSetupP50Nanos(void);
size_t brixGCMarkSetupMaxNanos(void);
size_t brixGCRootScanSampleCount(void);
size_t brixGCRootScanP95Nanos(void);
size_t brixGCRootScanP99Nanos(void);
size_t brixGCRootScanMaxNanos(void);
size_t brixGCMarkWorkSampleCount(void);
size_t brixGCMarkWorkP95Nanos(void);
size_t brixGCMarkWorkP99Nanos(void);
size_t brixGCMarkWorkMaxNanos(void);
size_t brixGCMarkTerminationSampleCount(void);
size_t brixGCMarkTerminationP99Nanos(void);
size_t brixGCMarkTerminationMaxNanos(void);
size_t brixGCSweepSampleCount(void);
size_t brixGCSweepP99Nanos(void);
size_t brixGCSweepMaxNanos(void);
size_t brixGCScavengeSampleCount(void);
size_t brixGCScavengeP99Nanos(void);
size_t brixGCScavengeMaxNanos(void);
size_t brixGCAllocationStallSampleCount(void);
size_t brixGCFallbackAllocationSampleCount(void);
size_t brixGCHandshakeSampleCount(void);
size_t brixGCHandshakeP99Nanos(void);
size_t brixGCHandshakeMaxNanos(void);
size_t brixGCStackMapLookupSampleCount(void);
size_t brixGCStackMapLookupP99Nanos(void);
size_t brixGCPauseCount(void);
size_t brixGCPauseEventCount(void);
size_t brixGCPauseEventCycleId(size_t index);
size_t brixGCPauseEventKind(size_t index);
size_t brixGCPauseEventStartNanos(size_t index);
size_t brixGCPauseEventDurationNanos(size_t index);
size_t brixGCPauseEventTasksWaited(size_t index);
size_t brixGCPauseEventSlowestTaskNanos(size_t index);
size_t brixGCPauseP50Nanos(size_t kind);
size_t brixGCPauseP95Nanos(size_t kind);
size_t brixGCPauseP99Nanos(size_t kind);
size_t brixGCPauseMaxNanos(size_t kind);
size_t brixGCCycleSummaryCount(void);
size_t brixGCCycleSummaryCycleId(size_t index);
size_t brixGCCycleSummaryTriggerReason(size_t index);
size_t brixGCCycleSummaryHeapAtTrigger(size_t index);
size_t brixGCCycleSummaryHeapGoal(size_t index);
size_t brixGCCycleSummaryHeapActual(size_t index);
size_t brixGCCycleSummaryMarkDurationNanos(size_t index);
size_t brixGCCycleSummaryScanWork(size_t index);
size_t brixGCCycleSummaryAssistWork(size_t index);
size_t brixGCCycleSummaryBackgroundWork(size_t index);
size_t brixGCCycleSummarySweepPages(size_t index);
size_t brixGCCycleSummaryScavengedPages(size_t index);

#if BX_ENABLE_BENCH_METRICS
// Raw sampled object counts by completed collection cycles survived. Sampling advances over
// GC-reserved slots; cleared reservations did not become observable allocations.
size_t brixGCBenchLifetimeBinCount(void);
size_t brixGCBenchLifetimeBinUpperBound(size_t index);
size_t brixGCBenchLifetimeBinValue(size_t index);
size_t brixGCBenchLifetimeCensoredBinValue(size_t index);
size_t brixGCBenchLifetimeSampledReservationCount(void);
size_t brixGCBenchLifetimeClearedReservationCount(void);
size_t brixGCBenchLifetimeReclaimedCount(void);
size_t brixGCBenchLifetimeOutstandingCount(void);
size_t brixGCBenchLifetimeSampleEvery(void);
void brixGCBenchLifetimeReset(void);
#endif

#if defined(__clang__)
#pragma clang attribute pop
#endif

#ifdef __cplusplus
}
#endif

#endif
