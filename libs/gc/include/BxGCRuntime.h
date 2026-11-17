#ifndef BX_GC_RUNTIME_H
#define BX_GC_RUNTIME_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <BxGC/BxGCAllocationBuffer.h>

struct BrixGCAllocationBatch;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BX_GC_OBSERVATION_ENUM
#define BX_GC_OBSERVATION_ENUM
enum BrixGCObservation {
    BrixGCObservationMarkSetup = 1,
    BrixGCObservationRootScan = 2,
    BrixGCObservationMarkWork = 3,
    BrixGCObservationAssist = 4,
    BrixGCObservationMarkTermination = 5,
    BrixGCObservationSweep = 6,
    BrixGCObservationScavenge = 7,
    BrixGCObservationAllocationStall = 8,
    BrixGCObservationFallbackAllocation = 9,
    BrixGCObservationHandshake = 10,
    BrixGCObservationStackMapLookup = 11
};
#endif

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

void brixGCPublishPhase(size_t phase);
size_t brixGCPublicPhase(void);
void brixGCPublishSweepGeneration(size_t generation);
size_t brixGCPublicSweepGeneration(void);
void brixGCPublishHandshakeRequest(size_t reason, size_t epoch, bool requested);
uint64_t brixGCPublicControl(void);
bool brixGCValidateAllocationABI(void);
bool brixGCAllocBatchForRunningTask(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    struct BrixGCAllocationBatch* batch);
bool brixGCAllocBatchForUnregisteredThread(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    struct BrixGCAllocationBatch* batch);
bool brixGCDiscardUnusedBatch(
    char* cursor,
    char* limit,
    size_t slotSize,
    size_t generation);
bool brixGCReleaseUnusedBatch(
    char* cursor,
    char* limit,
    size_t slotSize,
    size_t generation);
void brixGCRuntimeCycleStart(size_t cycle);
void brixGCRuntimeCycleEnd(void);
void brixGCRuntimeSubphaseStart(size_t subphase, size_t cycle, size_t value);
void brixGCRuntimeSubphaseEnd(size_t subphase, size_t value);
void brixGCRuntimeSubphaseCancel(size_t subphase);
void brixGCRuntimeRecordSubphaseDuration(
    size_t subphase,
    size_t cycle,
    size_t durationNanos,
    size_t value);
void brixGCRuntimeSetMarkQueueDepth(size_t depth);
void brixGCRuntimeSetAssistDebt(size_t debt);
void brixGCRuntimeRecordBarrier(size_t slowCount, size_t shadeCount);
void brixGCRuntimeRecordBarrierSlow(void);
void brixGCRuntimeRecordBarrierShade(void);
void brixGCRuntimeRecordAssist(void);
void brixGCRuntimeRecordAllocatedDuringMark(void);
void brixGCRuntimeRecordMarkQueuePushes(size_t count, size_t depth);
void brixGCRuntimeRecordMarkQueuePopBatch(size_t count, size_t depth);
void brixGCRuntimeRecordAllocation(size_t requestedSize, size_t allocatedSize);
void brixGCRuntimeRecordZeroedBytes(size_t byteSize);
void brixGCRuntimeRecordBatchReservation(size_t count, size_t allocatedSize);
void brixGCRuntimeRecordBatchConsumption(size_t allocatedSize);
void brixGCRuntimeCancelBatchCacheReservation(size_t bytes);
void brixGCRuntimeRecordBatchRelease(size_t count, size_t allocatedSize);
void brixGCRuntimeRecordBatchDiscard(size_t count, size_t allocatedSize);
void brixGCRuntimeRecordSweep(size_t freedObjects, size_t freedBytes);
void brixGCRuntimeRecordSweepPages(size_t pages);
void brixGCRuntimeRecordScavengePages(size_t pages);
void brixGCRuntimeRecordSpanAllocation(void);
void brixGCRuntimeRecordArenaAllocation(void);
bool brixGCRuntimeCanReserveBatchCache(size_t bytes);
extern thread_local BrixGCAllocationBuffer* brixGCActiveAllocationBuffer;
extern thread_local BrixGCThreadCache* brixGCActiveThreadCache;
extern uint64_t brixGCPublicControlWord;
void brixGCSetActiveThreadCache(BrixGCThreadCache* cache);
BrixGCThreadCache* brixGCGetActiveThreadCache(void);
void brixGCSetActiveAllocationBufferStorage(BrixGCRuntimeAllocationBuffer* storage);
BrixGCAllocationBuffer* brixGCGetActiveAllocationBuffer(void);
void brixGCRuntimeFlushThreadAllocationBatches(void);
void brixGCRuntimeFlushWriteBarrierBuffer(void);
void brixGCRuntimeFlushActiveAllocationBuffer(void);
void brixGCRuntimeRevokeActiveAllocationBuffer(void);
void brixWBBufRecordPointer(char* pointer);
void brixWBBufFlush(void);
size_t brixGCFlushWriteBarrierEntries(char* const* entries, size_t count);
void brixGCRuntimeSetHeapGoal(size_t goal);
void brixGCRuntimeSetMinimumHeapGoal(size_t goal);
void brixGCRuntimeFinishCollection(void);
void brixGCRuntimeRecordAutomaticFallback(size_t reason);
void brixGCRuntimeMarkStart(void);
void brixGCRuntimeMarkEnd(void);
void brixGCRuntimeSweepStart(void);
void brixGCRuntimeSweepEnd(void);
size_t brixGCIncrementalSweepDebtPages(void);
bool brixGCIncrementalSweepHasDebt(void);
bool brixGCRuntimeDecommitPages(void* pointer, size_t byteSize);
bool brixGCRuntimeRecommitPages(void* pointer, size_t byteSize);
void brixGCRuntimeHandshakeStart(void);
void brixGCRuntimeHandshakeSetReason(size_t reason);
void brixGCRuntimeHandshakeWaitEnd(void);
void brixGCRuntimeHandshakeEnd(void);
void brixGCRuntimeRecordPauseTaskWait(size_t tasksWaited, size_t slowestTaskNanos);
void brixGCRuntimeRecordPerTaskSetupScan(size_t startNanos, size_t durationNanos);
bool brixGCRuntimeIdleCollect(uint64_t budgetNanos);
bool brixGCRuntimeVerifyEachSweep(void);
size_t brixGCRuntimeAllocatedObjectCount(void);
size_t brixGCRuntimeRequestedByteCount(void);
size_t brixGCRuntimeAllocatedByteCount(void);
size_t brixGCRuntimeLiveByteCount(void);
size_t brixGCRuntimeAllocatedSpanCount(void);
size_t brixGCRuntimeAllocatedArenaCount(void);
size_t brixGCRuntimeHeapGoal(void);
size_t brixGCRuntimePacerGrowthPercent(void);
size_t brixGCRuntimeBackgroundWorkerCount(void);
size_t brixGCRuntimeBackgroundWorkerThreadCapacity(void);
size_t brixGCRuntimeBackgroundWorkerDutyPermille(void);
size_t brixGCRuntimeBackgroundWorkerWork(void);
size_t brixGCRuntimeBackgroundCredit(void);
size_t brixGCRuntimeFrameBudgetOfferCount(void);
size_t brixGCRuntimeFrameBudgetWorkCount(void);
size_t brixGCRuntimeFrameBudgetWorkNanos(void);
size_t brixGCRuntimeFrameBudgetOverrunCount(void);
size_t brixGCRuntimeFrameBudgetMaxOverrunNanos(void);
size_t brixGCRuntimePacerHardCap(void);
size_t brixGCRuntimePacerSoftGoal(void);
size_t brixGCRuntimePacerHardGoal(void);
size_t brixGCRuntimePacerLastTriggerReason(void);
size_t brixGCRuntimeCollectionCount(void);
size_t brixGCRuntimeMarkTimeNanos(void);
size_t brixGCRuntimeSweepTimeNanos(void);
size_t brixGCRuntimeAllocationStallCount(void);
size_t brixGCRuntimeAllocationStallTimeNanos(void);
size_t brixGCRuntimeHandshakeCount(void);
size_t brixGCRuntimeHandshakeLatencyNanos(void);
size_t brixGCRuntimeCurrentCycleId(void);
size_t brixGCRuntimeActiveCycleAgeNanos(void);
size_t brixGCRuntimeMarkQueueDepth(void);
size_t brixGCRuntimeAssistDebt(void);
size_t brixGCRuntimeBarrierCount(void);
size_t brixGCRuntimeAssistCount(void);
size_t brixGCRuntimeAssistTimeNanos(void);
size_t brixGCRuntimeMarkQueuePushCount(void);
size_t brixGCRuntimeMarkQueuePopCount(void);
size_t brixGCRuntimeMarkQueuePopBatchCount(void);
size_t brixGCRuntimeMarkQueueMaxPopBatch(void);
size_t brixGCRuntimeBatchReservedBytes(void);
size_t brixGCRuntimeBatchConsumedBytes(void);
size_t brixGCRuntimeBatchReleasedBytes(void);
size_t brixGCRuntimeBatchDiscardedBytes(void);
size_t brixGCRuntimeBatchCacheBytes(void);
size_t brixGCRuntimeBatchUnusedBytes(void);
size_t brixGCRuntimeThreadBatchCacheBytes(void);
size_t brixGCRuntimeAdaptiveBatchGrowthCount(void);
size_t brixGCRuntimeAdaptiveBatchShrinkCount(void);
size_t brixGCRuntimeAdaptiveBatchReturnedUnusedSlotCount(void);
size_t brixGCRuntimeCompilerIndexedRefillCount(void);
size_t brixGCRuntimeActiveBufferRefillCount(void);
size_t brixGCRuntimeGenericDescribedRefillCount(void);
size_t brixGCRuntimeGenericDescriptorlessRefillCount(void);
size_t brixGCRuntimeCompilerBufferSiteSwitchCount(void);
size_t brixGCRuntimeFullAllocationRangeFlushCount(void);
size_t brixGCRuntimeFullAllocationRangeFlushedBufferCount(void);
size_t brixGCRuntimeBatchCachePressureFlushCount(void);
size_t brixGCRuntimeThreadPendingAllocatedObjectCount(void);
size_t brixGCRuntimeThreadPendingBatchConsumedBytes(void);
size_t brixGCRuntimeStaleBatchDropCount(void);
size_t brixGCRuntimeFallbackAllocationCount(void);
size_t brixGCRuntimeAllocationCallCount(size_t kind);
size_t brixGCRuntimeAllocationPhaseCount(size_t phase);
size_t brixGCRuntimeAllocationFastHitCount(void);
size_t brixGCRuntimeAllocationFastRefillCount(void);
size_t brixGCRuntimeAllocationFastFallbackCount(void);
size_t brixGCRuntimeAllocationLargeCount(void);
size_t brixGCRuntimeAllocationBatchMissCount(size_t reason);
size_t brixGCRuntimeAllocationZeroedBytes(void);
size_t brixGCRuntimeAllocationStatPublicationCount(void);
size_t brixGCRuntimeAllocationAssistCheckCount(void);
size_t brixGCRuntimeAllocationAssistCallCount(void);
size_t brixGCRuntimeFastBarrierCallCount(void);
size_t brixGCRuntimeFastBarrierSlowEntryCount(void);
size_t brixGCRuntimeBufferedBarrierSharedRMWCount(void);
size_t brixGCRuntimeAllocationDescriptorMixCount(size_t index, size_t described);
size_t brixGCRuntimeAllocationSiteHitCount(void);
size_t brixGCRuntimeAllocationSiteMissCount(void);
size_t brixGCRuntimeAllocationSiteRefillCount(void);
size_t brixGCRuntimeAllocationSiteFallbackCount(void);
size_t brixGCRuntimeAutomaticFallbackCount(void);
size_t brixGCRuntimeAutomaticFallbackReason(void);
size_t brixGCRuntimeAutomaticDrainGiveUpCount(void);
size_t brixGCRuntimeMarkWorkRetainedBytes(void);
size_t brixGCRuntimeSpanMetadataBytes(void);
size_t brixGCRuntimeActiveSpanMetadataBytes(void);
size_t brixGCRuntimeCachedSpanMetadataBytes(void);
size_t brixGCRuntimeAddressIndexMetadataBytes(void);
size_t brixGCRuntimeMarkSetupSampleCount(void);
size_t brixGCRuntimeMarkSetupP50Nanos(void);
size_t brixGCRuntimeMarkSetupMaxNanos(void);
size_t brixGCRuntimeRootScanSampleCount(void);
size_t brixGCRuntimeRootScanP95Nanos(void);
size_t brixGCRuntimeRootScanP99Nanos(void);
size_t brixGCRuntimeRootScanMaxNanos(void);
size_t brixGCRuntimeMarkWorkSampleCount(void);
size_t brixGCRuntimeMarkWorkP95Nanos(void);
size_t brixGCRuntimeMarkWorkP99Nanos(void);
size_t brixGCRuntimeMarkWorkMaxNanos(void);
size_t brixGCRuntimeMarkTerminationSampleCount(void);
size_t brixGCRuntimeMarkTerminationP99Nanos(void);
size_t brixGCRuntimeMarkTerminationMaxNanos(void);
size_t brixGCRuntimeSweepSampleCount(void);
size_t brixGCRuntimeSweepP99Nanos(void);
size_t brixGCRuntimeSweepMaxNanos(void);
size_t brixGCRuntimeScavengeSampleCount(void);
size_t brixGCRuntimeScavengeP99Nanos(void);
size_t brixGCRuntimeScavengeMaxNanos(void);
size_t brixGCRuntimeAllocationStallSampleCount(void);
size_t brixGCRuntimeFallbackAllocationSampleCount(void);
size_t brixGCRuntimeHandshakeSampleCount(void);
size_t brixGCRuntimeHandshakeP99Nanos(void);
size_t brixGCRuntimeHandshakeMaxNanos(void);
size_t brixGCRuntimeStackMapLookupSampleCount(void);
size_t brixGCRuntimeStackMapLookupP99Nanos(void);
size_t brixGCRuntimePauseCount(void);
size_t brixGCRuntimePauseEventCount(void);
size_t brixGCRuntimePauseEventCycleId(size_t index);
size_t brixGCRuntimePauseEventKind(size_t index);
size_t brixGCRuntimePauseEventStartNanos(size_t index);
size_t brixGCRuntimePauseEventDurationNanos(size_t index);
size_t brixGCRuntimePauseEventTasksWaited(size_t index);
size_t brixGCRuntimePauseEventSlowestTaskNanos(size_t index);
size_t brixGCRuntimePauseP50Nanos(size_t kind);
size_t brixGCRuntimePauseP95Nanos(size_t kind);
size_t brixGCRuntimePauseP99Nanos(size_t kind);
size_t brixGCRuntimePauseMaxNanos(size_t kind);
size_t brixGCRuntimeCycleSummaryCount(void);
size_t brixGCRuntimeCycleSummaryCycleId(size_t index);
size_t brixGCRuntimeCycleSummaryTriggerReason(size_t index);
size_t brixGCRuntimeCycleSummaryHeapAtTrigger(size_t index);
size_t brixGCRuntimeCycleSummaryHeapGoal(size_t index);
size_t brixGCRuntimeCycleSummaryHeapActual(size_t index);
size_t brixGCRuntimeCycleSummaryMarkDurationNanos(size_t index);
size_t brixGCRuntimeCycleSummaryScanWork(size_t index);
size_t brixGCRuntimeCycleSummaryAssistWork(size_t index);
size_t brixGCRuntimeCycleSummaryBackgroundWork(size_t index);
size_t brixGCRuntimeCycleSummarySweepPages(size_t index);
size_t brixGCRuntimeCycleSummaryScavengedPages(size_t index);
#if BX_ENABLE_BENCH_METRICS
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
void brixWriteBarrierFast(char* source, char* value);
void brixBulkWriteBarrierFast(char* source, char* value, size_t byteSize, void* descriptor);
typedef void (*BrixGCDescriptorPointerSlotVisitor)(char** slot, void* data);
void brixGCVisitDescriptorPointerSlots(
    char* object,
    size_t byteSize,
    void* descriptor,
    BrixGCDescriptorPointerSlotVisitor visitor,
    void* data);
bool brixGCIsManagedAddress(char* pointer);

#ifdef __cplusplus
}
#endif

#endif
