#ifndef YU_GC_H
#define YU_GC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <YuGC/YuGCAnnotations.h>
#include <YuGC/YuGCTemporaryRoots.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct YumiGCAllocationBatch {
    char* cursor;
    char* limit;
    size_t slotSize;
} YumiGCAllocationBatch;

// The language wrapper contains and reports recoverable errors before crossing this C ABI.
// The return token is reserved; draining continues regardless of its value.
typedef size_t (*YumiGCCleanupCallback)(void* context);

#include <YuGC/YuGCAllocationBuffer.h>

#ifndef YU_GC_PAUSE_KIND_ENUM
#define YU_GC_PAUSE_KIND_ENUM
enum YumiGCPauseKind {
    YumiGCPauseKindMarkSetupTotal = 1,
    YumiGCPauseKindPerTaskSetupScan = 2,
    YumiGCPauseKindTermConfirm = 3,
    YumiGCPauseKindSweepTotal = 4,
    YumiGCPauseKindStallReport = 5,
    YumiGCPauseKindCount = 6
};
#endif

#ifndef YU_OBJECT_SIDE_PAYLOAD_LANE_ENUM
#define YU_OBJECT_SIDE_PAYLOAD_LANE_ENUM
enum YumiObjectSidePayloadLane {
    // Strong. Property subscriptions migrate to weak-backed subscriptions.
    YumiObjectSidePayloadPropertySubscribers = 0,
    // Strong: the payload is intentionally owned for the owner's lifetime.
    YumiObjectSidePayloadOwnedLiveActions = 1,
    // Strong and cleanup-participating; observer deactivation is the cleanup action.
    YumiObjectSidePayloadCollectionObservers = 2,
    // Strong. Cleanup retires the attachment when its owner becomes unreachable.
    YumiObjectSidePayloadMarkupAttachments = 3,
    // Strong. Cleanup retires the attachment when its owner becomes unreachable.
    YumiObjectSidePayloadStyleAttachments = 4,
    // Strong and explicitly clearable.
    YumiObjectSidePayloadStyleTemplates = 5,
    // Transitional strong lane; clients migrate to weak references plus cleanup.
    YumiObjectSidePayloadStyleObjects = 6,
    // Strong. Event endpoint entries live exactly as long as their owner.
    YumiObjectSidePayloadEvents = 7,
    YumiObjectSidePayloadLaneCount = 8
};
#endif

#ifndef YU_GC_BUFFER_RESIZE_ENUM
#define YU_GC_BUFFER_RESIZE_ENUM
enum YumiGCBufferResize {
    YumiGCBufferResizeRejected = 0,
    YumiGCBufferResizeInPlace = 1,
    YumiGCBufferResizeReplace = 2
};
#endif

YUMI_GC_POLLING void yumiGCInit(void);
YUMI_GC_POLLING void yumiGCScavenge(void);
YUMI_GC_LEAF unsigned char yumiGCCollectorPhase(void);

YUMI_GC_POLLING char* yumiGCAllocObject(size_t size);
YUMI_GC_POLLING char* yumiGCAllocObjectDescribed(size_t size, void* descriptor);
YUMI_GC_POLLING char* yumiGCAllocBuffer(size_t itemSize, size_t count);
YUMI_GC_POLLING char* yumiGCAllocBufferDescribed(size_t itemSize, size_t count, void* descriptor);
YUMI_GC_POLLING char* yumiGCResizeBuffer(char* buffer, size_t oldSize, size_t newSize);
YUMI_GC_POLLING char* yumiGCResizeBufferDescribed(char* buffer, size_t oldSize, size_t newSize, void* descriptor);
YUMI_GC_POLLING enum YumiGCBufferResize yumiGCResizeBufferPrepare(
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* descriptor,
    void** replacementDescriptor);
YUMI_GC_POLLING void yumiGCResizeBufferCopy(
    char* replacement,
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* replacementDescriptor);
YUMI_GC_POLLING char* yumiAllocObjectWithSiteKey(const YumiGCAllocationSiteKey* key);

YUMI_GC_POLLING void* yumiGCAddRoot(char* pointer);
YUMI_GC_POLLING void* yumiGCAddTemporaryRoot(char* pointer);
YUMI_GC_LEAF void* yumiGCAddIdleTemporaryRoot(char* pointer);
YUMI_GC_POLLING void* yumiGCAddDescribedRoot(
    char* pointer,
    size_t byteSize,
    void* descriptor);
YUMI_GC_LEAF void yumiGCUpdateRoot(void* root, char* pointer);
YUMI_GC_LEAF void yumiGCRemoveRoot(void* root);
YUMI_GC_LEAF void yumiGCRemoveTemporaryRoot(void* root);
YUMI_GC_LEAF bool yumiGCRemoveTaskTemporaryRoot(void* root);
YUMI_GC_LEAF void yumiGCDrainTemporaryRoots(YumiGCTemporaryRootStorage* storage);
YUMI_GC_POLLING void yumiGCCollect(void);
YUMI_GC_POLLING void yumiGCCollectForAllocation(void);
#ifndef YU_GC_COLLECT_RESULT_TYPE
#define YU_GC_COLLECT_RESULT_TYPE
typedef size_t YumiGCCollectResult;
enum {
    YumiGCCollectCompleted = 0
};
#endif
YUMI_GC_POLLING YumiGCCollectResult yumiCollect(void);
YUMI_GC_POLLING void yumiGCFrameBudget(uint64_t remainingNanos);
YUMI_GC_LEAF bool yumiGCVerifyObject(char* pointer);
YUMI_GC_LEAF void* yumiGCObjectDescriptor(char* pointer);
YUMI_GC_LEAF void yumiGCWriteBarrier(char* source, char* value);
YUMI_GC_LEAF void yumiGCBulkWriteBarrier(char* source, char* value, size_t byteSize, void* descriptor);
YUMI_GC_LEAF void yumiGCSetHeapGoal(size_t goal);
YUMI_GC_LEAF void yumiGCSetMinimumHeapGoal(size_t goal);
YUMI_GC_POLLING size_t yumiGCRegisterOwnerWeak(char* owner, char* target);
YUMI_GC_POLLING char* yumiGCOwnerWeakLoad(size_t handle);
YUMI_GC_POLLING bool yumiGCReleaseOwnerWeak(size_t handle);
YUMI_GC_POLLING size_t yumiGCRegisterCleanup(
    char* target,
    char* context,
    YumiGCCleanupCallback callback);
YUMI_GC_POLLING bool yumiGCCancelCleanup(size_t handle);
YUMI_GC_POLLING size_t yumiGCDrainCleanups(void);
YUMI_GC_LEAF void yumiGCQueueFileDescriptorCleanup(int fd);
YUMI_GC_LEAF void yumiGCReportCleanupException(void);
YUMI_GC_POLLING char* yumiGCObjectSidePayload(char* object, size_t lane);
YUMI_GC_POLLING char* yumiGCPublishObjectSidePayload(char* object, size_t lane, char* payload);
YUMI_GC_POLLING char* yumiGCPublishCleanupObjectSidePayload(
    char* object,
    size_t lane,
    char* payload,
    YumiGCCleanupCallback callback);
YUMI_GC_LEAF void yumiGCClearObjectSidePayload(char* object, size_t lane);
YUMI_GC_LEAF bool yumiGCClearCleanupObjectSidePayload(char* object, size_t lane);
YUMI_GC_LEAF bool yumiGCHasObjectSidePayload(char* object, size_t lane);
YUMI_GC_LEAF size_t yumiGCObjectSidePayloadCount(size_t lane);
YUMI_GC_LEAF size_t yumiGCCollectionObserversSidePayloadLane(void);
YUMI_GC_LEAF size_t yumiGCMarkupAttachmentsSidePayloadLane(void);
YUMI_GC_LEAF size_t yumiGCStyleAttachmentsSidePayloadLane(void);
YUMI_GC_LEAF size_t yumiGCStyleTemplatesSidePayloadLane(void);
YUMI_GC_LEAF size_t yumiGCStyleObjectsSidePayloadLane(void);
YUMI_GC_LEAF size_t yumiGCEventsSidePayloadLane(void);
YUMI_GC_POLLING size_t yumiCollectionObserverActivate(char* owner);
YUMI_GC_POLLING bool yumiCollectionObserverDeactivate(size_t cleanup);
YUMI_GC_LEAF bool yumiCollectionMayBeObserved(char* collection);
YUMI_GC_LEAF size_t yumiCollectionObserverActiveCount(void);

// The remaining public GC surface is metric/snapshot access. Keep the classification grouped so
// newly added observability imports cannot silently fall back to the blocking policy.
#if defined(__clang__)
#pragma clang attribute push(__attribute__((annotate("yumi:gc_leaf"))), apply_to = function)
#endif

size_t yumiGCBarrierCount(void);
size_t yumiGCAssistCount(void);
size_t yumiGCAssistTimeNanos(void);
size_t yumiGCAllocatedObjectCount(void);
size_t yumiGCRequestedByteCount(void);
size_t yumiGCAllocatedByteCount(void);
size_t yumiGCLiveByteCount(void);
size_t yumiGCAllocatedSpanCount(void);
size_t yumiGCAllocatedArenaCount(void);
size_t yumiGCHeapGoal(void);
size_t yumiGCPacerGrowthPercent(void);
size_t yumiGCBackgroundWorkerCount(void);
size_t yumiGCBackgroundWorkerThreadCapacity(void);
size_t yumiGCBackgroundWorkerDutyPermille(void);
size_t yumiGCBackgroundWorkerWork(void);
size_t yumiGCBackgroundCredit(void);
size_t yumiGCFrameBudgetOfferCount(void);
size_t yumiGCFrameBudgetWorkCount(void);
size_t yumiGCFrameBudgetWorkNanos(void);
size_t yumiGCFrameBudgetOverrunCount(void);
size_t yumiGCPacerHardCap(void);
size_t yumiGCPacerSoftGoal(void);
size_t yumiGCPacerHardGoal(void);
size_t yumiGCPacerLastTriggerReason(void);
size_t yumiGCCollectionCount(void);
size_t yumiGCWeakClearCount(void);
size_t yumiGCCleanupRegistrationCount(void);
size_t yumiGCCleanupPendingCount(void);
size_t yumiGCCleanupEnqueueCount(void);
size_t yumiGCCleanupFireCount(void);
size_t yumiGCCleanupCancelCount(void);
size_t yumiGCCleanupExceptionCount(void);
size_t yumiGCMarkTimeNanos(void);
size_t yumiGCSweepTimeNanos(void);
size_t yumiGCAllocationStallCount(void);

// Benchmark-only phase markers. Instrumented compiler builds emit these calls; clean benchmark
// builds elide them before object emission.
YUMI_GC_LEAF void yumiBenchMutatorPhaseStart(const char* name);
YUMI_GC_LEAF void yumiBenchMutatorPhaseEnd(const char* name);
size_t yumiGCAllocationStallTimeNanos(void);
size_t yumiGCHandshakeCount(void);
size_t yumiGCHandshakeLatencyNanos(void);
size_t yumiGCCurrentCycleId(void);
size_t yumiGCActiveCycleAgeNanos(void);
size_t yumiGCMarkQueueDepth(void);
size_t yumiGCAssistDebt(void);
size_t yumiGCMarkQueuePushCount(void);
size_t yumiGCMarkQueuePopCount(void);
size_t yumiGCMarkQueuePopBatchCount(void);
size_t yumiGCMarkQueueMaxPopBatch(void);
size_t yumiGCMarkParticipantCount(void);
size_t yumiGCActiveMarkerCount(void);
uint64_t yumiGCWorkFlushedEpoch(void);
size_t yumiGCTerminationConfirmCount(void);
size_t yumiGCTerminationConfirmResumeCount(void);
size_t yumiGCBatchReservedBytes(void);
size_t yumiGCBatchConsumedBytes(void);
size_t yumiGCBatchReleasedBytes(void);
size_t yumiGCBatchDiscardedBytes(void);
size_t yumiGCBatchCacheBytes(void);
size_t yumiGCBatchUnusedBytes(void);
size_t yumiGCThreadBatchCacheBytes(void);
size_t yumiGCThreadPendingAllocatedObjectCount(void);
size_t yumiGCThreadPendingBatchConsumedBytes(void);
size_t yumiGCStaleBatchDropCount(void);
size_t yumiGCFallbackAllocationCount(void);
size_t yumiGCAllocationCallCount(size_t kind);
size_t yumiGCAllocationPhaseCount(size_t phase);
size_t yumiGCAllocationFastHitCount(void);
size_t yumiGCAllocationFastRefillCount(void);
size_t yumiGCAllocationFastFallbackCount(void);
size_t yumiGCAllocationLargeCount(void);
size_t yumiGCAllocationBatchMissCount(size_t reason);
size_t yumiGCAllocationZeroedBytes(void);
size_t yumiGCAllocationStatPublicationCount(void);
size_t yumiGCAllocationAssistCheckCount(void);
size_t yumiGCAllocationAssistCallCount(void);
size_t yumiGCFastBarrierCallCount(void);
size_t yumiGCFastBarrierSlowEntryCount(void);
size_t yumiGCAllocationDescriptorMixCount(size_t index, size_t described);
size_t yumiGCAllocationSiteHitCount(void);
size_t yumiGCAllocationSiteMissCount(void);
size_t yumiGCAllocationSiteRefillCount(void);
size_t yumiGCAllocationSiteFallbackCount(void);
size_t yumiGCAutomaticFallbackCount(void);
size_t yumiGCAutomaticFallbackReason(void);
size_t yumiGCAutomaticDrainGiveUpCount(void);
size_t yumiGCRssBytes(void);
size_t yumiGCCommittedBytes(void);
size_t yumiGCCommittedPageCount(void);
size_t yumiGCDecommittedPageCount(void);
size_t yumiGCDirtyPageCount(void);
size_t yumiGCFreePageCount(void);
size_t yumiGCReservedArenaBytes(void);
size_t yumiGCLargeSpanFragmentationBytes(void);
size_t yumiGCFragmentationBytes(void);
size_t yumiGCSizeClassOccupancyBucketCount(size_t index, size_t bucket);
size_t yumiGCScavengeDebt(void);
size_t yumiGCScavengeDebtBytes(void);
size_t yumiGCFreeCommittedBytes(void);
size_t yumiGCFreeCommittedRunCount(void);
size_t yumiGCLargestFreeCommittedRunBytes(void);
size_t yumiGCAllocatedPageBytes(void);
size_t yumiGCPeakDemandBytes(void);
size_t yumiGCRetentionTargetBytes(void);
size_t yumiGCSizeClassSweepDebt(size_t index);
size_t yumiGCLargeSpanSweepDebt(void);
size_t yumiGCMarkSetupSampleCount(void);
size_t yumiGCMarkSetupP50Nanos(void);
size_t yumiGCMarkSetupMaxNanos(void);
size_t yumiGCRootScanSampleCount(void);
size_t yumiGCRootScanP95Nanos(void);
size_t yumiGCRootScanP99Nanos(void);
size_t yumiGCRootScanMaxNanos(void);
size_t yumiGCMarkWorkSampleCount(void);
size_t yumiGCMarkWorkP95Nanos(void);
size_t yumiGCMarkWorkP99Nanos(void);
size_t yumiGCMarkWorkMaxNanos(void);
size_t yumiGCMarkTerminationSampleCount(void);
size_t yumiGCMarkTerminationP99Nanos(void);
size_t yumiGCMarkTerminationMaxNanos(void);
size_t yumiGCSweepSampleCount(void);
size_t yumiGCSweepP99Nanos(void);
size_t yumiGCSweepMaxNanos(void);
size_t yumiGCScavengeSampleCount(void);
size_t yumiGCScavengeP99Nanos(void);
size_t yumiGCScavengeMaxNanos(void);
size_t yumiGCAllocationStallSampleCount(void);
size_t yumiGCFallbackAllocationSampleCount(void);
size_t yumiGCHandshakeSampleCount(void);
size_t yumiGCHandshakeP99Nanos(void);
size_t yumiGCHandshakeMaxNanos(void);
size_t yumiGCStackMapLookupSampleCount(void);
size_t yumiGCStackMapLookupP99Nanos(void);
size_t yumiGCPauseCount(void);
size_t yumiGCPauseEventCount(void);
size_t yumiGCPauseEventCycleId(size_t index);
size_t yumiGCPauseEventKind(size_t index);
size_t yumiGCPauseEventStartNanos(size_t index);
size_t yumiGCPauseEventDurationNanos(size_t index);
size_t yumiGCPauseEventTasksWaited(size_t index);
size_t yumiGCPauseEventSlowestTaskNanos(size_t index);
size_t yumiGCPauseP50Nanos(size_t kind);
size_t yumiGCPauseP95Nanos(size_t kind);
size_t yumiGCPauseP99Nanos(size_t kind);
size_t yumiGCPauseMaxNanos(size_t kind);
size_t yumiGCCycleSummaryCount(void);
size_t yumiGCCycleSummaryCycleId(size_t index);
size_t yumiGCCycleSummaryTriggerReason(size_t index);
size_t yumiGCCycleSummaryHeapAtTrigger(size_t index);
size_t yumiGCCycleSummaryHeapGoal(size_t index);
size_t yumiGCCycleSummaryHeapActual(size_t index);
size_t yumiGCCycleSummaryMarkDurationNanos(size_t index);
size_t yumiGCCycleSummaryScanWork(size_t index);
size_t yumiGCCycleSummaryAssistWork(size_t index);
size_t yumiGCCycleSummaryBackgroundWork(size_t index);
size_t yumiGCCycleSummarySweepPages(size_t index);
size_t yumiGCCycleSummaryScavengedPages(size_t index);

#if YU_ENABLE_BENCH_METRICS
// Raw sampled object counts by completed collection cycles survived. Sampling advances over
// GC-reserved slots; cleared reservations did not become observable allocations.
size_t yumiGCBenchLifetimeBinCount(void);
size_t yumiGCBenchLifetimeBinUpperBound(size_t index);
size_t yumiGCBenchLifetimeBinValue(size_t index);
size_t yumiGCBenchLifetimeCensoredBinValue(size_t index);
size_t yumiGCBenchLifetimeSampledReservationCount(void);
size_t yumiGCBenchLifetimeClearedReservationCount(void);
size_t yumiGCBenchLifetimeReclaimedCount(void);
size_t yumiGCBenchLifetimeOutstandingCount(void);
size_t yumiGCBenchLifetimeSampleEvery(void);
void yumiGCBenchLifetimeReset(void);
#endif

#if defined(__clang__)
#pragma clang attribute pop
#endif

#ifdef __cplusplus
}
#endif

#endif
