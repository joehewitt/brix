#ifndef YU_GC_RUNTIME_H
#define YU_GC_RUNTIME_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <YuGC/YuGCAllocationBuffer.h>

struct YumiGCAllocationBatch;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef YU_GC_OBSERVATION_ENUM
#define YU_GC_OBSERVATION_ENUM
enum YumiGCObservation {
    YumiGCObservationMarkSetup = 1,
    YumiGCObservationRootScan = 2,
    YumiGCObservationMarkWork = 3,
    YumiGCObservationAssist = 4,
    YumiGCObservationMarkTermination = 5,
    YumiGCObservationSweep = 6,
    YumiGCObservationScavenge = 7,
    YumiGCObservationAllocationStall = 8,
    YumiGCObservationFallbackAllocation = 9,
    YumiGCObservationHandshake = 10,
    YumiGCObservationStackMapLookup = 11
};
#endif

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

void yumiGCPublishPhase(size_t phase);
size_t yumiGCPublicPhase(void);
void yumiGCPublishSweepGeneration(size_t generation);
size_t yumiGCPublicSweepGeneration(void);
void yumiGCPublishHandshakeRequest(size_t reason, size_t epoch, bool requested);
uint64_t yumiGCPublicControl(void);
bool yumiGCValidateAllocationABI(void);
bool yumiGCAllocBatchForRunningTask(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    struct YumiGCAllocationBatch* batch);
bool yumiGCAllocBatchForUnregisteredThread(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    struct YumiGCAllocationBatch* batch);
bool yumiGCDiscardUnusedBatch(
    char* cursor,
    char* limit,
    size_t slotSize,
    size_t generation);
bool yumiGCReleaseUnusedBatch(
    char* cursor,
    char* limit,
    size_t slotSize,
    size_t generation);
void yumiGCRuntimeCycleStart(size_t cycle);
void yumiGCRuntimeCycleEnd(void);
void yumiGCRuntimeSubphaseStart(size_t subphase, size_t cycle, size_t value);
void yumiGCRuntimeSubphaseEnd(size_t subphase, size_t value);
void yumiGCRuntimeSubphaseCancel(size_t subphase);
void yumiGCRuntimeRecordSubphaseDuration(
    size_t subphase,
    size_t cycle,
    size_t durationNanos,
    size_t value);
void yumiGCRuntimeSetMarkQueueDepth(size_t depth);
void yumiGCRuntimeSetAssistDebt(size_t debt);
void yumiGCRuntimeRecordBarrier(size_t slowCount, size_t shadeCount);
void yumiGCRuntimeRecordBarrierSlow(void);
void yumiGCRuntimeRecordBarrierShade(void);
void yumiGCRuntimeRecordAssist(void);
void yumiGCRuntimeRecordAllocatedDuringMark(void);
void yumiGCRuntimeRecordMarkQueuePushes(size_t count, size_t depth);
void yumiGCRuntimeRecordMarkQueuePopBatch(size_t count, size_t depth);
void yumiGCRuntimeRecordAllocation(size_t requestedSize, size_t allocatedSize);
void yumiGCRuntimeRecordZeroedBytes(size_t byteSize);
void yumiGCRuntimeRecordBatchReservation(size_t count, size_t allocatedSize);
void yumiGCRuntimeRecordBatchConsumption(size_t allocatedSize);
void yumiGCRuntimeCancelBatchCacheReservation(size_t bytes);
void yumiGCRuntimeRecordBatchRelease(size_t count, size_t allocatedSize);
void yumiGCRuntimeRecordBatchDiscard(size_t count, size_t allocatedSize);
void yumiGCRuntimeRecordSweep(size_t freedObjects, size_t freedBytes);
void yumiGCRuntimeRecordSweepPages(size_t pages);
void yumiGCRuntimeRecordScavengePages(size_t pages);
void yumiGCRuntimeRecordSpanAllocation(void);
void yumiGCRuntimeRecordArenaAllocation(void);
bool yumiGCRuntimeCanReserveBatchCache(size_t bytes);
extern thread_local YumiGCAllocationBuffer* yumiGCActiveAllocationBuffer;
extern thread_local YumiGCThreadCache* yumiGCActiveThreadCache;
extern uint64_t yumiGCPublicControlWord;
void yumiGCSetActiveThreadCache(YumiGCThreadCache* cache);
YumiGCThreadCache* yumiGCGetActiveThreadCache(void);
void yumiGCSetActiveAllocationBufferStorage(YumiGCRuntimeAllocationBuffer* storage);
YumiGCAllocationBuffer* yumiGCGetActiveAllocationBuffer(void);
void yumiGCRuntimeFlushThreadAllocationBatches(void);
void yumiGCRuntimeFlushWriteBarrierBuffer(void);
void yumiGCRuntimeFlushActiveAllocationBuffer(void);
void yumiGCRuntimeRevokeActiveAllocationBuffer(void);
void yumiWBBufRecordPointer(char* pointer);
void yumiWBBufFlush(void);
size_t yumiGCFlushWriteBarrierEntries(char* const* entries, size_t count);
void yumiGCRuntimeSetHeapGoal(size_t goal);
void yumiGCRuntimeFinishCollection(void);
void yumiGCRuntimeRecordAutomaticFallback(size_t reason);
void yumiGCRuntimeMarkStart(void);
void yumiGCRuntimeMarkEnd(void);
void yumiGCRuntimeSweepStart(void);
void yumiGCRuntimeSweepEnd(void);
size_t yumiGCIncrementalSweepDebtPages(void);
bool yumiGCIncrementalSweepHasDebt(void);
bool yumiGCRuntimeDecommitPages(void* pointer, size_t byteSize);
bool yumiGCRuntimeRecommitPages(void* pointer, size_t byteSize);
void yumiGCRuntimeHandshakeStart(void);
void yumiGCRuntimeHandshakeSetReason(size_t reason);
void yumiGCRuntimeHandshakeWaitEnd(void);
void yumiGCRuntimeHandshakeEnd(void);
void yumiGCRuntimeRecordPauseTaskWait(size_t tasksWaited, size_t slowestTaskNanos);
void yumiGCRuntimeRecordPerTaskSetupScan(size_t startNanos, size_t durationNanos);
void yumiGCRuntimeIdleCollect(uint64_t deadlineNanos);
bool yumiGCRuntimeVerifyEachSweep(void);
size_t yumiGCRuntimeAllocatedObjectCount(void);
size_t yumiGCRuntimeRequestedByteCount(void);
size_t yumiGCRuntimeAllocatedByteCount(void);
size_t yumiGCRuntimeLiveByteCount(void);
size_t yumiGCRuntimeAllocatedSpanCount(void);
size_t yumiGCRuntimeAllocatedArenaCount(void);
size_t yumiGCRuntimeHeapGoal(void);
size_t yumiGCRuntimePacerGrowthPercent(void);
size_t yumiGCRuntimeBackgroundWorkerCount(void);
size_t yumiGCRuntimeBackgroundWorkerThreadCapacity(void);
size_t yumiGCRuntimeBackgroundWorkerDutyPermille(void);
size_t yumiGCRuntimeBackgroundWorkerWork(void);
size_t yumiGCRuntimeBackgroundCredit(void);
size_t yumiGCRuntimeFrameBudgetOfferCount(void);
size_t yumiGCRuntimeFrameBudgetWorkCount(void);
size_t yumiGCRuntimeFrameBudgetWorkNanos(void);
size_t yumiGCRuntimeFrameBudgetOverrunCount(void);
size_t yumiGCRuntimeFrameBudgetMaxOverrunNanos(void);
size_t yumiGCRuntimePacerHardCap(void);
size_t yumiGCRuntimePacerSoftGoal(void);
size_t yumiGCRuntimePacerHardGoal(void);
size_t yumiGCRuntimePacerLastTriggerReason(void);
size_t yumiGCRuntimeCollectionCount(void);
size_t yumiGCRuntimeMarkTimeNanos(void);
size_t yumiGCRuntimeSweepTimeNanos(void);
size_t yumiGCRuntimeAllocationStallCount(void);
size_t yumiGCRuntimeAllocationStallTimeNanos(void);
size_t yumiGCRuntimeHandshakeCount(void);
size_t yumiGCRuntimeHandshakeLatencyNanos(void);
size_t yumiGCRuntimeCurrentCycleId(void);
size_t yumiGCRuntimeActiveCycleAgeNanos(void);
size_t yumiGCRuntimeMarkQueueDepth(void);
size_t yumiGCRuntimeAssistDebt(void);
size_t yumiGCRuntimeBarrierCount(void);
size_t yumiGCRuntimeAssistCount(void);
size_t yumiGCRuntimeAssistTimeNanos(void);
size_t yumiGCRuntimeMarkQueuePushCount(void);
size_t yumiGCRuntimeMarkQueuePopCount(void);
size_t yumiGCRuntimeMarkQueuePopBatchCount(void);
size_t yumiGCRuntimeMarkQueueMaxPopBatch(void);
size_t yumiGCRuntimeBatchReservedBytes(void);
size_t yumiGCRuntimeBatchConsumedBytes(void);
size_t yumiGCRuntimeBatchReleasedBytes(void);
size_t yumiGCRuntimeBatchDiscardedBytes(void);
size_t yumiGCRuntimeBatchCacheBytes(void);
size_t yumiGCRuntimeBatchUnusedBytes(void);
size_t yumiGCRuntimeThreadBatchCacheBytes(void);
size_t yumiGCRuntimeAdaptiveBatchGrowthCount(void);
size_t yumiGCRuntimeAdaptiveBatchShrinkCount(void);
size_t yumiGCRuntimeAdaptiveBatchReturnedUnusedSlotCount(void);
size_t yumiGCRuntimeCompilerIndexedRefillCount(void);
size_t yumiGCRuntimeActiveBufferRefillCount(void);
size_t yumiGCRuntimeGenericDescribedRefillCount(void);
size_t yumiGCRuntimeGenericDescriptorlessRefillCount(void);
size_t yumiGCRuntimeCompilerBufferSiteSwitchCount(void);
size_t yumiGCRuntimeFullAllocationRangeFlushCount(void);
size_t yumiGCRuntimeFullAllocationRangeFlushedBufferCount(void);
size_t yumiGCRuntimeBatchCachePressureFlushCount(void);
size_t yumiGCRuntimeThreadPendingAllocatedObjectCount(void);
size_t yumiGCRuntimeThreadPendingBatchConsumedBytes(void);
size_t yumiGCRuntimeStaleBatchDropCount(void);
size_t yumiGCRuntimeFallbackAllocationCount(void);
size_t yumiGCRuntimeAllocationCallCount(size_t kind);
size_t yumiGCRuntimeAllocationPhaseCount(size_t phase);
size_t yumiGCRuntimeAllocationFastHitCount(void);
size_t yumiGCRuntimeAllocationFastRefillCount(void);
size_t yumiGCRuntimeAllocationFastFallbackCount(void);
size_t yumiGCRuntimeAllocationLargeCount(void);
size_t yumiGCRuntimeAllocationBatchMissCount(size_t reason);
size_t yumiGCRuntimeAllocationZeroedBytes(void);
size_t yumiGCRuntimeAllocationStatPublicationCount(void);
size_t yumiGCRuntimeAllocationAssistCheckCount(void);
size_t yumiGCRuntimeAllocationAssistCallCount(void);
size_t yumiGCRuntimeFastBarrierCallCount(void);
size_t yumiGCRuntimeFastBarrierSlowEntryCount(void);
size_t yumiGCRuntimeBufferedBarrierSharedRMWCount(void);
size_t yumiGCRuntimeAllocationDescriptorMixCount(size_t index, size_t described);
size_t yumiGCRuntimeAllocationSiteHitCount(void);
size_t yumiGCRuntimeAllocationSiteMissCount(void);
size_t yumiGCRuntimeAllocationSiteRefillCount(void);
size_t yumiGCRuntimeAllocationSiteFallbackCount(void);
size_t yumiGCRuntimeAutomaticFallbackCount(void);
size_t yumiGCRuntimeAutomaticFallbackReason(void);
size_t yumiGCRuntimeAutomaticDrainGiveUpCount(void);
size_t yumiGCRuntimeMarkWorkRetainedBytes(void);
size_t yumiGCRuntimeSpanMetadataBytes(void);
size_t yumiGCRuntimeActiveSpanMetadataBytes(void);
size_t yumiGCRuntimeCachedSpanMetadataBytes(void);
size_t yumiGCRuntimeAddressIndexMetadataBytes(void);
size_t yumiGCRuntimeMarkSetupSampleCount(void);
size_t yumiGCRuntimeMarkSetupP50Nanos(void);
size_t yumiGCRuntimeMarkSetupMaxNanos(void);
size_t yumiGCRuntimeRootScanSampleCount(void);
size_t yumiGCRuntimeRootScanP95Nanos(void);
size_t yumiGCRuntimeRootScanP99Nanos(void);
size_t yumiGCRuntimeRootScanMaxNanos(void);
size_t yumiGCRuntimeMarkWorkSampleCount(void);
size_t yumiGCRuntimeMarkWorkP95Nanos(void);
size_t yumiGCRuntimeMarkWorkP99Nanos(void);
size_t yumiGCRuntimeMarkWorkMaxNanos(void);
size_t yumiGCRuntimeMarkTerminationSampleCount(void);
size_t yumiGCRuntimeMarkTerminationP99Nanos(void);
size_t yumiGCRuntimeMarkTerminationMaxNanos(void);
size_t yumiGCRuntimeSweepSampleCount(void);
size_t yumiGCRuntimeSweepP99Nanos(void);
size_t yumiGCRuntimeSweepMaxNanos(void);
size_t yumiGCRuntimeScavengeSampleCount(void);
size_t yumiGCRuntimeScavengeP99Nanos(void);
size_t yumiGCRuntimeScavengeMaxNanos(void);
size_t yumiGCRuntimeAllocationStallSampleCount(void);
size_t yumiGCRuntimeFallbackAllocationSampleCount(void);
size_t yumiGCRuntimeHandshakeSampleCount(void);
size_t yumiGCRuntimeHandshakeP99Nanos(void);
size_t yumiGCRuntimeHandshakeMaxNanos(void);
size_t yumiGCRuntimeStackMapLookupSampleCount(void);
size_t yumiGCRuntimeStackMapLookupP99Nanos(void);
size_t yumiGCRuntimePauseCount(void);
size_t yumiGCRuntimePauseEventCount(void);
size_t yumiGCRuntimePauseEventCycleId(size_t index);
size_t yumiGCRuntimePauseEventKind(size_t index);
size_t yumiGCRuntimePauseEventStartNanos(size_t index);
size_t yumiGCRuntimePauseEventDurationNanos(size_t index);
size_t yumiGCRuntimePauseEventTasksWaited(size_t index);
size_t yumiGCRuntimePauseEventSlowestTaskNanos(size_t index);
size_t yumiGCRuntimePauseP50Nanos(size_t kind);
size_t yumiGCRuntimePauseP95Nanos(size_t kind);
size_t yumiGCRuntimePauseP99Nanos(size_t kind);
size_t yumiGCRuntimePauseMaxNanos(size_t kind);
size_t yumiGCRuntimeCycleSummaryCount(void);
size_t yumiGCRuntimeCycleSummaryCycleId(size_t index);
size_t yumiGCRuntimeCycleSummaryTriggerReason(size_t index);
size_t yumiGCRuntimeCycleSummaryHeapAtTrigger(size_t index);
size_t yumiGCRuntimeCycleSummaryHeapGoal(size_t index);
size_t yumiGCRuntimeCycleSummaryHeapActual(size_t index);
size_t yumiGCRuntimeCycleSummaryMarkDurationNanos(size_t index);
size_t yumiGCRuntimeCycleSummaryScanWork(size_t index);
size_t yumiGCRuntimeCycleSummaryAssistWork(size_t index);
size_t yumiGCRuntimeCycleSummaryBackgroundWork(size_t index);
size_t yumiGCRuntimeCycleSummarySweepPages(size_t index);
size_t yumiGCRuntimeCycleSummaryScavengedPages(size_t index);
#if YU_ENABLE_BENCH_METRICS
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
void yumiWriteBarrierFast(char* source, char* value);
void yumiBulkWriteBarrierFast(char* source, char* value, size_t byteSize, void* descriptor);
typedef void (*YumiGCDescriptorPointerSlotVisitor)(char** slot, void* data);
void yumiGCVisitDescriptorPointerSlots(
    char* object,
    size_t byteSize,
    void* descriptor,
    YumiGCDescriptorPointerSlotVisitor visitor,
    void* data);
bool yumiGCIsManagedAddress(char* pointer);

#ifdef __cplusplus
}
#endif

#endif
