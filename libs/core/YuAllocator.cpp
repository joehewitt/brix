#include "YuCore/YuCore.h"
#include "YuCore/YuParseRegion.h"

#include <cstdlib>
#include <cstring>
#include <limits>
#include <atomic>
#include <mutex>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ctime>
#include <algorithm>
#include <csignal>
#include <dlfcn.h>
#include <execinfo.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <dispatch/dispatch.h>
#if TARGET_OS_IPHONE
#include <os/proc.h>
#endif
#endif

// **********************************************************************************************

#if YU_ENABLE_BENCH_METRICS
#define YUMI_BENCH_RECORD_ALLOCATION(size) \
    do { \
        if (__builtin_expect(yumiBenchMetricsEnabledFlag, 0)) { \
            yumiBenchRecordAllocation(size); \
        } \
    } while (0)
#else
#define YUMI_BENCH_RECORD_ALLOCATION(size) do { } while (0)
#endif

// **********************************************************************************************

extern "C" thread_local YumiGCAllocationBuffer* yumiGCActiveAllocationBuffer = nullptr;
static thread_local YumiGCThreadCache gcFallbackThreadCache = {};
extern "C" thread_local YumiGCThreadCache* yumiGCActiveThreadCache = &gcFallbackThreadCache;
extern "C" uint64_t yumiGCPublicControlWord = 0;

#include "YuGC/YuGC.h"
#include "YuGC/YuGCCollectionObserverFilter.h"
#include "YuGC/YuGCRuntime.h"
#include "YuGC/YuGCTest.h"
#include "YuCore/YuPanic.h"

extern "C" void yumiGCSafepoint(void);
extern "C" void yumiGCEnterRuntimeCritical(void);
extern "C" void yumiGCLeaveRuntimeCritical(void);
extern "C" bool yumiGCIsRuntimeCritical(void);
extern "C" void yumiGCEnterRuntimeBlock(void);
extern "C" void yumiGCLeaveRuntimeBlock(void);
extern "C" void yumiTerminateActiveTask(void);
extern "C" void yumiGCStartConcurrentMark(void);
extern "C" size_t yumiGCStepConcurrentMark(void);
extern "C" void yumiGCFinishAutomaticCycle(void);
extern "C" void yumiGCStepIncrementalSweep(size_t workLimit);
extern "C" void yumiGCStepScavenge(size_t pageBudget);
extern "C" size_t yumiGCTrimMetadataCaches(void);
extern "C" size_t yumiGCScavengeMinimumStepPages(void);
extern "C" size_t yumiGCMutatorAssist(size_t workLimit);
extern "C" bool yumiGCActiveTaskCanUseFastAllocation(void) __attribute__((weak));
extern "C" bool yumiGCActiveTaskCanUseFastAllocation(void) {
    return false;
}
extern "C" bool yumiGCActiveTaskAcknowledgedHandshake(uint64_t control) __attribute__((weak));
extern "C" bool yumiGCActiveTaskAcknowledgedHandshake(uint64_t) {
    return false;
}
extern "C" bool yumiGCThreadHasLocalCallbackRegistration(void) __attribute__((weak));
extern "C" bool yumiGCThreadHasLocalCallbackRegistration(void) {
    return false;
}
class YuTaskQueue;
typedef void (*YumiGCBackgroundTaskFunc)(void* closure);
extern "C" int yumiGCBackgroundTaskQueueThreadCount(void) __attribute__((weak));
extern "C" YuTaskQueue* yumiGCBackgroundTaskQueue(void) __attribute__((weak));
extern "C" bool yumiForkGCMarkTaskInWorker(
    YuTaskQueue* queue,
    YumiGCBackgroundTaskFunc func,
    void* closure) __attribute__((weak));
extern "C" size_t yumiGCActiveTaskAssistDebt(void) __attribute__((weak));
extern "C" size_t yumiGCActiveTaskAssistDebtCycle(void) __attribute__((weak));
extern "C" void yumiGCSetActiveTaskAssistDebt(size_t debt) __attribute__((weak));
extern "C" void yumiGCSetActiveTaskAssistDebtCycle(size_t cycle) __attribute__((weak));
extern "C" void yumiGCDebugUnstoppableFastAllocationRejected(void) __attribute__((weak));
extern "C" void yumiGCDebugUnstoppableFastAllocationRejected(void) {}
extern "C" unsigned char yumiInterruptRequestedFlags = 0;
static_assert(sizeof(yumiInterruptRequestedFlags) == 1, "interrupt flags must be one byte");
static_assert(sizeof(yumiGCPublicControlWord) == sizeof(uint64_t), "GC control word drifted");
static_assert(
    offsetof(YumiGCAllocationBuffer, cursor) == YumiGCAllocationBufferCursorOffset,
    "GC allocation buffer cursor offset drifted");
static_assert(
    offsetof(YumiGCAllocationBuffer, limit) == YumiGCAllocationBufferLimitOffset,
    "GC allocation buffer limit offset drifted");
static_assert(
    sizeof(YumiGCAllocationBuffer) == YumiGCAllocationBufferSize,
    "GC allocation buffer size drifted");
static_assert(
    offsetof(YumiGCRuntimeAllocationBuffer, buffer) ==
        YumiGCRuntimeAllocationBufferHotOffset,
    "GC runtime allocation buffer hot offset drifted");
static_assert(
    offsetof(YumiGCRuntimeAllocationBuffer, generation) ==
        YumiGCRuntimeAllocationBufferGenerationOffset,
    "GC runtime allocation buffer generation offset drifted");
static_assert(
    sizeof(YumiGCRuntimeAllocationBuffer) == YumiGCRuntimeAllocationBufferSize,
    "GC runtime allocation buffer size drifted");
static_assert(
    sizeof(YumiGCAllocationSiteKey) == YumiGCAllocationSiteKeySize,
    "GC allocation site key size drifted");
static_assert(
    sizeof(YumiGCThreadCache) == YumiGCThreadCacheSize,
    "GC thread cache size drifted");

#if YU_ENABLE_YUMI_GC

static constexpr bool
yumiGCCollectionDisabled() {
    return YU_GC_NEVER_COLLECT != 0;
}

static uint64_t
yumiGCLoadControlWord() {
    return __atomic_load_n(&yumiGCPublicControlWord, __ATOMIC_ACQUIRE);
}

static unsigned char
yumiGCLoadPublicPhase() {
    return yumiGCControlWordPhase(yumiGCLoadControlWord());
}

static size_t
yumiGCLoadPublicSweepGeneration() {
    return yumiGCControlWordSweepGeneration(yumiGCLoadControlWord());
}

static void
yumiGCUpdateControlWord(uint64_t mask, uint64_t value) {
    uint64_t current = __atomic_load_n(&yumiGCPublicControlWord, __ATOMIC_RELAXED);
    while (true) {
        uint64_t next = (current & ~mask) | (value & mask);
        if (__atomic_compare_exchange_n(
                &yumiGCPublicControlWord,
                &current,
                next,
                false,
                __ATOMIC_RELEASE,
                __ATOMIC_RELAXED)) {
            return;
        }
    }
}

static YumiGCThreadCache*
yumiGCEnsureActiveThreadCache() {
    if (!yumiGCActiveThreadCache) {
        yumiGCActiveThreadCache = &gcFallbackThreadCache;
    }
    if (
        yumiGCActiveThreadCache->abiVersion != YumiGCAllocationABIVersion ||
        yumiGCActiveThreadCache->structSize != sizeof(YumiGCThreadCache)
    ) {
        *yumiGCActiveThreadCache = YumiGCThreadCache {};
        yumiGCInitializeThreadCache(yumiGCActiveThreadCache);
    }
    return yumiGCActiveThreadCache;
}

static std::once_flag gcInitOnce;
static std::atomic<bool> gcInitialized {false};
static std::atomic<bool> gcMemoryPressurePending {false};
static std::atomic<size_t> gcAllocatedObjects {0};
static std::atomic<size_t> gcRequestedBytes {0};
static std::atomic<size_t> gcAllocatedBytes {0};
static std::atomic<size_t> gcAllocatedSpans {0};
static std::atomic<size_t> gcAllocatedArenas {0};
static std::atomic<size_t> gcLiveObjects {0};
static std::atomic<size_t> gcLiveBytes {0};
static std::atomic<size_t> gcIdleSettledLiveBytes {0};
static std::atomic<size_t> gcIdleSettleSeenLiveBytes {0};
static std::atomic<uint64_t> gcIdleSettleSeenNanos {0};
static std::atomic<size_t> gcHeapGoal {0};
static std::atomic<size_t> gcCollectionCount {0};
static std::atomic<size_t> gcMarkTimeNanos {0};
static std::atomic<size_t> gcSweepTimeNanos {0};
static std::atomic<size_t> gcAllocationStallCount {0};
static std::atomic<size_t> gcAllocationStallTimeNanos {0};
static std::atomic<size_t> gcHandshakeCount {0};
static std::atomic<size_t> gcHandshakeLatencyNanos {0};
static std::atomic<size_t> gcCurrentCycleId {0};
static std::atomic<uint64_t> gcActiveCycleStartNanos {0};
static std::atomic<size_t> gcCycleAllocatedBytes {0};
static std::atomic<size_t> gcLastCycleAllocatedBytes {0};
static std::atomic<size_t> gcMarkQueueDepth {0};
static std::atomic<size_t> gcAssistDebt {0};
static std::atomic<size_t> gcBackgroundCredit {0};
static std::atomic<size_t> gcBackgroundWorkerCount {0};
static std::atomic<size_t> gcBackgroundWorkerThreadCapacity {0};
static std::atomic<size_t> gcBackgroundWorkerActiveCount {0};
static std::atomic<size_t> gcBackgroundWorkerWork {0};
static std::atomic<size_t> gcBackgroundWorkerCpuNanos {0};
static std::atomic<size_t> gcBackgroundWorkerDutyPermille {0};
static std::atomic<size_t> gcBackgroundWorkerGeneration {0};
static std::atomic<size_t> gcFrameBudgetOfferCount {0};
static std::atomic<size_t> gcFrameBudgetWorkCount {0};
static std::atomic<size_t> gcFrameBudgetWorkNanos {0};
static std::atomic<size_t> gcFrameBudgetOverrunCount {0};
static std::atomic<size_t> gcFrameBudgetMaxOverrunNanos {0};
static std::atomic<size_t> gcAssistCount {0};
static std::atomic<size_t> gcAllocatedDuringMarkCount {0};
static std::atomic<size_t> gcMarkQueuePushCount {0};
static std::atomic<size_t> gcMarkQueuePopCount {0};
static std::atomic<size_t> gcMarkQueuePopBatchCount {0};
static std::atomic<size_t> gcMarkQueueMaxPopBatch {0};
static std::atomic<size_t> gcBatchReservedBytes {0};
static std::atomic<size_t> gcBatchConsumedBytes {0};
static std::atomic<size_t> gcBatchReleasedBytes {0};
static std::atomic<size_t> gcBatchDiscardedBytes {0};
static std::atomic<size_t> gcBatchCacheBytes {0};
static std::atomic<size_t> gcStaleBatchDropCount {0};
static std::atomic<size_t> gcThreadCacheOwnerFlushCount {0};
static std::atomic<size_t> gcFallbackAllocationCount {0};
static std::atomic<size_t> gcFallbackAllocationTimeNanos {0};
static std::atomic<size_t> gcAutomaticFallbackCount {0};
static std::atomic<size_t> gcAutomaticFallbackReason {0};
static std::atomic<size_t> gcAutomaticDrainGiveUpCount {0};
static std::atomic<bool> gcCollectionActive {false};
static std::atomic<bool> gcAutomaticCycleActive {false};
static std::atomic<size_t> gcRequestedHeapGoal {0};
static std::atomic<bool> gcCollectionMayGrowRequestedGoal {false};
static std::atomic<size_t> gcPacerCurrentRootScanNanos {0};
static std::atomic<size_t> gcPacerCurrentMarkWorkItems {0};
static std::atomic<size_t> gcPacerCurrentMarkWorkNanos {0};
static std::atomic<size_t> gcPacerCurrentAssistWorkItems {0};
static std::atomic<size_t> gcPacerCurrentAssistWorkNanos {0};
static std::atomic<size_t> gcAssistTimeNanos {0};
static std::atomic<size_t> gcPendingCycleTriggerReason {0};
static std::atomic<size_t> gcCurrentCycleTriggerReason {0};
static std::atomic<size_t> gcCurrentCycleHeapAtTrigger {0};
static std::atomic<size_t> gcCurrentCycleHeapGoal {0};
static std::atomic<uint64_t> gcMarkStartNanos {0};
static std::atomic<size_t> gcCurrentCycleMarkDurationNanos {0};
static std::atomic<size_t> gcCurrentCycleScanWork {0};
static std::atomic<size_t> gcCurrentCycleSweepPages {0};
static std::atomic<size_t> gcPacerGoalCacheGoal {0};
static std::atomic<size_t> gcPacerGoalCacheSoftGoal {0};
static std::atomic<size_t> gcPacerGoalCacheHardGoal {0};
static std::atomic<size_t> gcPacerLastMarkedLiveBytes {0};
static std::atomic<size_t> gcPacerLastHeapGoal {0};
static std::atomic<size_t> gcPacerLastSoftGoal {0};
static std::atomic<size_t> gcPacerLastHardGoal {0};
static std::atomic<size_t> gcPacerLastTriggerReserveBytes {0};
static std::atomic<size_t> gcPacerLastCycleNanos {0};
static std::atomic<size_t> gcPacerLastAllocationRateBytesPerSecond {0};
static std::atomic<size_t> gcPacerLastRootScanNanos {0};
static std::atomic<size_t> gcPacerLastMarkThroughputItemsPerSecond {0};
static std::atomic<size_t> gcPacerLastAssistThroughputItemsPerSecond {0};
static std::atomic<size_t> gcPacerLastSweepDebt {0};
static std::atomic<size_t> gcPacerLastScavengeDebt {0};
static std::atomic<size_t> gcPacerLastTriggerReason {0};
static std::atomic<size_t> gcPacerAllocationRateEWMA {0};
static std::atomic<size_t> gcPacerMarkDurationEWMA {0};
static std::atomic<size_t> gcPacerLastGoalOvershootBytes {0};
static std::atomic<size_t> gcAllocationFastHitCount {0};
static std::atomic<size_t> gcAllocationFastRefillCount {0};
static std::atomic<size_t> gcAllocationFastFallbackCount {0};
static std::atomic<size_t> gcAllocationLargeCount {0};
static std::atomic<size_t> gcAllocationZeroedBytes {0};
static std::atomic<size_t> gcAllocationStatPublicationCount {0};
static std::atomic<size_t> gcAllocationAssistCheckCount {0};
static std::atomic<size_t> gcAllocationAssistCallCount {0};
static std::atomic<size_t> gcAllocationSiteHitCount {0};
static std::atomic<size_t> gcAllocationSiteMissCount {0};
static std::atomic<size_t> gcAllocationSiteRefillCount {0};
static std::atomic<size_t> gcAllocationSiteFallbackCount {0};

struct YumiGCBarrierStatsRecord {
    std::atomic<size_t> barrierCalls;
    std::atomic<size_t> barrierSlowEntries;
    std::atomic<size_t> barrierShades;
    std::atomic<size_t> fastCalls;
    std::atomic<size_t> fastSlowEntries;

    YumiGCBarrierStatsRecord()
        : barrierCalls(0),
          barrierSlowEntries(0),
          barrierShades(0),
          fastCalls(0),
          fastSlowEntries(0) {}
};

enum YumiGCBarrierStatsKind {
    YumiGCBarrierStatsCalls,
    YumiGCBarrierStatsSlowEntries,
    YumiGCBarrierStatsShades,
    YumiGCBarrierStatsFastCalls,
    YumiGCBarrierStatsFastSlowEntries
};

static std::mutex gcBarrierStatsMutex;
static std::vector<YumiGCBarrierStatsRecord*> gcBarrierStatsRecords;
static thread_local YumiGCBarrierStatsRecord* gcThreadBarrierStats = nullptr;
#if YU_ENABLE_BENCH_METRICS
static constexpr size_t kYumiGCBenchAllocationSmallBinCount = 128;
static constexpr size_t kYumiGCBenchAllocationLargeBinCount = 64;
static constexpr size_t kYumiGCBenchAllocationBinCount =
    kYumiGCBenchAllocationSmallBinCount + kYumiGCBenchAllocationLargeBinCount + 1;
static constexpr size_t kYumiGCBenchAllocationOverflowBin =
    kYumiGCBenchAllocationBinCount - 1;
static std::atomic<size_t> gcBenchCpuNanos {0};
static std::array<std::atomic<size_t>, YumiGCCompilerAllocationBufferCount + 1>
    gcBenchAllocationSites {};
static std::array<std::atomic<size_t>, kYumiGCBenchAllocationBinCount>
    gcBenchAllocationSizeBins {};
static constexpr size_t kYumiGCBenchSizeClassCount = 69;
static constexpr size_t kYumiGCBenchLargeSizeClass = 68;
static constexpr size_t kYumiGCBenchSizeClassUpperBounds[kYumiGCBenchSizeClassCount] = {
    0, 8, 16, 24, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224,
    240, 256, 288, 320, 352, 384, 416, 448, 480, 512, 576, 640, 704, 768, 896,
    1024, 1152, 1280, 1408, 1536, 1792, 2048, 2304, 2688, 3072, 3200, 3456,
    4096, 4864, 5376, 6144, 6528, 6784, 6912, 8192, 9472, 9728, 10240, 10880,
    12288, 13568, 14336, 16384, 18432, 19072, 20480, 21760, 24576, 27264,
    28672, 32768, std::numeric_limits<size_t>::max()
};
static std::array<std::atomic<size_t>, kYumiGCBenchSizeClassCount>
    gcBenchAllocatedSizeClasses {};

struct YumiGCBenchThreadRefillRecord {
    size_t ordinal;
    std::atomic<size_t> refills {0};

    explicit YumiGCBenchThreadRefillRecord(size_t ordinal)
        : ordinal(ordinal) {}
};

static std::mutex gcBenchThreadRefillMutex;
static std::vector<YumiGCBenchThreadRefillRecord*> gcBenchThreadRefillRecords;
static thread_local YumiGCBenchThreadRefillRecord* gcBenchThreadRefillRecord = nullptr;
#endif
static constexpr size_t kYumiGCSubphaseCount = YumiGCObservationStackMapLookup + 1;
static constexpr size_t kYumiGCPauseEventCapacity = 8192;
static constexpr size_t kYumiGCCycleSummaryCapacity = 1024;
static constexpr size_t kYumiGCSubphaseSampleCapacity = 8192;
static constexpr size_t kYumiGCDefaultHeapGoal = 32ull * 1024ull * 1024ull;
static constexpr size_t kYumiGCDefaultHeapMinGrowth = 16ull * 1024ull * 1024ull;
static constexpr size_t kYumiGCHeapTaperStart = 128ull * 1024ull * 1024ull;
static constexpr size_t kYumiGCIdleSettleMinimumBytes = 4ull * 1024ull * 1024ull;
static constexpr uint64_t kYumiGCIdleSettleQuietNanos = 250000000ull;
static constexpr size_t kYumiGCDefaultHeapGrowthPercent = 100;
static constexpr size_t kYumiGCDefaultHeapHardCap = 0;
static constexpr size_t kYumiGCDefaultThreadBatchCacheBytes = 8ull * 1024ull * 1024ull;
static constexpr size_t kYumiGCDefaultGlobalBatchCacheBytes = 128ull * 1024ull * 1024ull;
static constexpr size_t kYumiGCThreadStatFlushObjectCount = 65536;
static constexpr unsigned char kYumiGCPhaseIdle = 0;
static constexpr unsigned char kYumiGCPhaseMarkSetup = 1;
static constexpr unsigned char kYumiGCPhaseConcurrentMark = 2;
static constexpr unsigned char kYumiGCPhaseMarkTermination = 3;
static constexpr unsigned char kYumiGCPhaseMarkComplete = 4;
static constexpr unsigned char kYumiGCPhaseSweep = 5;
static constexpr size_t kYumiGCAutomaticAssistWork = 16;
static constexpr size_t kYumiGCAutomaticHardAssistWork = 128;
static constexpr size_t kYumiGCAutomaticStepWork = 64;
static constexpr size_t kYumiGCAutomaticIdleWork = 256;
static constexpr size_t kYumiGCAutomaticIdleScavengePages = 64;
static constexpr size_t kYumiGCIdleScavengeMaximumPages = 16384;
static constexpr size_t kYumiGCAutomaticTerminationQueueBudget = 64;
static constexpr size_t kYumiGCAutomaticMaxActiveCycleNanos = 250ull * 1000ull * 1000ull;
static constexpr size_t kYumiGCMaxAssistSliceNanos = 50ull * 1000ull;
static constexpr size_t kYumiGCBackgroundWorkerStepWork = 64;
static constexpr size_t kYumiGCBackgroundDutyDenominator = 1000;
static constexpr size_t kYumiGCBudgetGuardNanos = 50ull * 1000ull;

enum YumiGCAutomaticFallbackReason {
    YumiGCAutomaticFallbackNone = 0,
    YumiGCAutomaticFallbackNoProgressDriver = 1,
    YumiGCAutomaticFallbackMaxActiveAge = 2,
    YumiGCAutomaticFallbackHardCap = 3
};

enum YumiGCPacerTriggerReason {
    YumiGCPacerTriggerNone = 0,
    YumiGCPacerTriggerSoftGoal = 1,
    YumiGCPacerTriggerHardGoal = 2,
    YumiGCPacerTriggerHardCap = 3,
    YumiGCPacerTriggerIdle = 4,
    YumiGCPacerTriggerExplicit = 5,
    YumiGCPacerTriggerFrame = 6
};

static thread_local size_t gcFallbackActiveTaskAssistDebt = 0;
static thread_local size_t gcFallbackActiveTaskAssistDebtCycle = 0;

static constexpr size_t kYumiGCAllocationBatchSmallCount = 129;
static constexpr size_t kYumiGCAllocationBatchLargeBucketSize = 1024;
static constexpr size_t kYumiGCAllocationBatchLargeBucketCount = 64;
static constexpr size_t kYumiGCAllocationBatchCount =
    kYumiGCAllocationBatchSmallCount + kYumiGCAllocationBatchLargeBucketCount;
static constexpr size_t kYumiGCAllocationBatchMaxSize =
    kYumiGCAllocationBatchLargeBucketSize * kYumiGCAllocationBatchLargeBucketCount;
static constexpr size_t kYumiGCDescriptorlessAllocationBatchBytes = 8 * 1024;
static constexpr size_t kYumiGCDescriptorlessAllocationBatchMinCount = 64;
static constexpr size_t kYumiGCDescriptorlessAllocationBatchMaxCount = 256;
static constexpr size_t kYumiGCDescribedSmallAllocationBatchPreferredCount = 8;
static constexpr size_t kYumiGCDescribedMediumAllocationBatchPreferredCount = 4;
static constexpr size_t kYumiGCDescribedLargeAllocationBatchPreferredCount = 2;
static constexpr size_t kYumiGCAdaptiveAllocationBatchMaxCount = 64;
static constexpr unsigned char kYumiGCAdaptiveFullConsumptionStreak = 2;
static constexpr size_t kYumiGCAdaptiveLowUseDenominator = 4;
static constexpr size_t kYumiGCAdaptiveLowUseUnusedParts = 3;
static constexpr size_t kYumiGCAdaptiveCountByte = 0;
static constexpr size_t kYumiGCAdaptiveFullStreakByte = 1;
static constexpr size_t kYumiGCAdaptiveEnabledByte = 2;
static constexpr size_t kYumiGCAdaptiveReservedSlotCountByte = 3;
static constexpr size_t kYumiGCCompilerRangePacerCreditBytes = 64 * 1024;
static constexpr size_t kYumiGCGenericRangePacerCreditBytes = 64 * 1024;
static constexpr size_t kYumiGCObjectAlignment = 8;
static constexpr size_t kYumiGCThreadCounterCompilerRefills = 0;
static constexpr size_t kYumiGCThreadCounterBarrierCalls = 1;
static constexpr size_t kYumiGCThreadCounterBarrierSlowEntries = 2;
static constexpr size_t kYumiGCThreadCounterBarrierShades = 3;
static constexpr unsigned char kYumiGCAllocationPointerFree = 0;
static constexpr unsigned char kYumiGCAllocationPointerful = 1;
static constexpr unsigned char kYumiGCAllocationZeroFull = 0;

enum YumiGCAllocationCallKind {
    YumiGCAllocationCallObject = 0,
    YumiGCAllocationCallDescribedObject = 1,
    YumiGCAllocationCallBuffer = 2,
    YumiGCAllocationCallDescribedBuffer = 3,
    YumiGCAllocationCallResizeBuffer = 4,
    YumiGCAllocationCallDescribedResizeBuffer = 5,
    YumiGCAllocationCallKindCount = 6
};

enum YumiGCAllocationPhaseBucket {
    YumiGCAllocationPhaseIdle = 0,
    YumiGCAllocationPhaseMarkSetup = 1,
    YumiGCAllocationPhaseConcurrentMark = 2,
    YumiGCAllocationPhaseMarkComplete = 3,
    YumiGCAllocationPhaseSweep = 4,
    YumiGCAllocationPhaseOther = 5,
    YumiGCAllocationPhaseBucketCount = 6
};

enum YumiGCAllocationBatchMissReason {
    YumiGCAllocationBatchMissGeneration = 0,
    YumiGCAllocationBatchMissDescriptor = 1,
    YumiGCAllocationBatchMissRequestedSize = 2,
    YumiGCAllocationBatchMissEmpty = 3,
    YumiGCAllocationBatchMissCap = 4,
    YumiGCAllocationBatchMissReasonCount = 5
};

static std::array<std::atomic<size_t>, YumiGCAllocationCallKindCount> gcAllocationCallsByKind = {};
static std::array<std::atomic<size_t>, YumiGCAllocationPhaseBucketCount> gcAllocationCallsByPhase = {};
static std::array<std::atomic<size_t>, YumiGCAllocationBatchMissReasonCount> gcAllocationBatchMisses = {};
static std::array<std::atomic<size_t>, kYumiGCAllocationBatchCount> gcAllocationDescriptorlessBySizeClass = {};
static std::array<std::atomic<size_t>, kYumiGCAllocationBatchCount> gcAllocationDescribedBySizeClass = {};
static std::atomic<size_t> gcAdaptiveBatchGrowthCount {0};
static std::atomic<size_t> gcAdaptiveBatchShrinkCount {0};
static std::atomic<size_t> gcAdaptiveBatchReturnedUnusedSlotCount {0};
static std::atomic<size_t> gcCompilerIndexedRefillCount {0};
static std::atomic<size_t> gcActiveBufferRefillCount {0};
static std::atomic<size_t> gcGenericDescribedRefillCount {0};
static std::atomic<size_t> gcGenericDescriptorlessRefillCount {0};
static std::atomic<size_t> gcCompilerBufferSiteSwitchCount {0};
static std::atomic<size_t> gcFullAllocationRangeFlushCount {0};
static std::atomic<size_t> gcFullAllocationRangeFlushedBufferCount {0};
static std::atomic<size_t> gcBatchCachePressureFlushCount {0};

struct YumiGCThreadAllocationBatch {
    char* cursor = nullptr;
    char* limit = nullptr;
    size_t slotSize = 0;
    size_t requestedSize = 0;
    void* descriptor = nullptr;
    size_t generation = 0;
};

struct YumiGCSubphaseSamples {
    std::vector<size_t> durations;
    uint64_t sampleSequence = 0;
    uint64_t randomState = 0;
    size_t maxDurationNanos = 0;
};

struct YumiGCPauseEvent {
    size_t cycleId = 0;
    size_t kind = 0;
    uint64_t startNanos = 0;
    size_t durationNanos = 0;
    size_t tasksWaited = 0;
    size_t slowestTaskNanos = 0;

    YumiGCPauseEvent() = default;
    YumiGCPauseEvent(
        size_t cycleId,
        size_t kind,
        uint64_t startNanos,
        size_t durationNanos,
        size_t tasksWaited,
        size_t slowestTaskNanos
    )
        : cycleId(cycleId),
          kind(kind),
          startNanos(startNanos),
          durationNanos(durationNanos),
          tasksWaited(tasksWaited),
          slowestTaskNanos(slowestTaskNanos) {}
};

struct YumiGCCycleSummary {
    size_t cycleId = 0;
    size_t triggerReason = 0;
    size_t heapAtTrigger = 0;
    size_t heapGoal = 0;
    size_t heapActual = 0;
    size_t markDurationNanos = 0;
    size_t scanWork = 0;
    size_t assistWork = 0;
    size_t backgroundWork = 0;
    size_t sweepPages = 0;
    size_t scavengedPages = 0;

    YumiGCCycleSummary() = default;
    YumiGCCycleSummary(
        size_t cycleId,
        size_t triggerReason,
        size_t heapAtTrigger,
        size_t heapGoal,
        size_t heapActual,
        size_t markDurationNanos,
        size_t scanWork,
        size_t assistWork,
        size_t backgroundWork,
        size_t sweepPages,
        size_t scavengedPages
    )
        : cycleId(cycleId),
          triggerReason(triggerReason),
          heapAtTrigger(heapAtTrigger),
          heapGoal(heapGoal),
          heapActual(heapActual),
          markDurationNanos(markDurationNanos),
          scanWork(scanWork),
          assistWork(assistWork),
          backgroundWork(backgroundWork),
          sweepPages(sweepPages),
          scavengedPages(scavengedPages) {}
};

static std::mutex gcObservabilityMutex;
static std::array<YumiGCSubphaseSamples, kYumiGCSubphaseCount> gcSubphaseSamples;
static std::array<YumiGCPauseEvent, kYumiGCPauseEventCapacity> gcPauseEvents;
static size_t gcPauseEventSequence = 0;
static size_t gcPauseEventStored = 0;
static std::atomic<size_t> gcPauseCount {0};
static std::array<YumiGCCycleSummary, kYumiGCCycleSummaryCapacity> gcCycleSummaries;
static size_t gcCycleSummarySequence = 0;
static size_t gcCycleSummaryStored = 0;

static thread_local YumiGCRuntimeAllocationBuffer* gcActiveAllocationBufferStorage = nullptr;
static thread_local bool gcAllocationBufferFlushInProgress = false;
static thread_local bool gcDeferredCompilerAllocationBufferFlush = false;
static thread_local size_t gcRuntimeCallDepth = 0;
static thread_local size_t gcCompilerRangePacerPendingBytes = 0;
static thread_local size_t gcCompilerRangePacerChargedBytes = 0;
static thread_local uint64_t gcSweepStartNanos = 0;
static thread_local uint64_t gcHandshakeStartNanos = 0;
static thread_local size_t gcHandshakePauseKind = 0;
static thread_local bool gcHandshakeWaitRecorded = false;
static thread_local size_t gcHandshakeTasksWaited = 0;
static thread_local size_t gcHandshakeSlowestTaskNanos = 0;
static thread_local std::array<uint64_t, kYumiGCSubphaseCount> gcSubphaseStartNanos = {};
static thread_local std::array<size_t, kYumiGCSubphaseCount> gcSubphaseStartCycles = {};
#if YU_ENABLE_BENCH_METRICS
static thread_local size_t gcBenchCpuDepth = 0;
static thread_local uint64_t gcBenchCpuStartNanos = 0;
#endif

static uint64_t yumiGCMonotonicNanos();
#if YU_ENABLE_BENCH_METRICS
static uint64_t
yumiGCBenchThreadCpuNanos() {
    struct timespec now {};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0) {
        return 0;
    }
    return static_cast<uint64_t>(now.tv_sec) * 1000000000ull +
        static_cast<uint64_t>(now.tv_nsec);
}

static void
yumiGCBenchCpuStart() {
    if (!yumiBenchMetricsEnabledFlag) {
        return;
    }
    if (gcBenchCpuDepth++ == 0) {
        gcBenchCpuStartNanos = yumiGCBenchThreadCpuNanos();
    }
}

static void
yumiGCBenchCpuEnd() {
    if (!yumiBenchMetricsEnabledFlag || !gcBenchCpuDepth) {
        return;
    }
    if (--gcBenchCpuDepth == 0 && gcBenchCpuStartNanos) {
        uint64_t end = yumiGCBenchThreadCpuNanos();
        if (end > gcBenchCpuStartNanos) {
            gcBenchCpuNanos.fetch_add(
                static_cast<size_t>(end - gcBenchCpuStartNanos),
                std::memory_order_relaxed);
        }
        gcBenchCpuStartNanos = 0;
    }
}
#else
static void yumiGCBenchCpuStart() {}
static void yumiGCBenchCpuEnd() {}
#endif
static void yumiGCRecordSubphaseSample(
    size_t subphase,
    size_t cycle,
    uint64_t startNanos,
    size_t durationNanos,
    size_t value);
static void yumiGCRecordPauseEvent(
    size_t kind,
    size_t cycleId,
    uint64_t startNanos,
    size_t durationNanos,
    size_t tasksWaited,
    size_t slowestTaskNanos);
static void yumiDiscardCachedGCAllocationBatch(YumiGCThreadAllocationBatch& batch);
static void yumiFlushGCAllocationBatches();
static void yumiPublishGCWriteBarrierCounters(YumiGCThreadCache* cache);
static YumiGCBarrierStatsRecord* yumiGCEnsureThreadBarrierStats();
static void yumiGCAddOwnedBarrierCounter(std::atomic<size_t>& counter, size_t amount);
static size_t yumiGCSumBarrierCounter(YumiGCBarrierStatsKind kind);
static void yumiFlushGCCompilerAllocationBuffer();
static void yumiFlushGCCompilerAllocationBuffer(YumiGCRuntimeAllocationBuffer& storage);
static bool yumiGCCompilerAllocationBufferIndexIsValid(size_t index);
static void yumiMarkGCCompilerAllocationBufferActive(size_t index);
static void yumiGCPrepareCompilerRangeVerificationAllocation(
    const YumiGCAllocationSiteKey& key,
    size_t selectedCount);
static void yumiGCChargeCompilerRangePacerBytes(size_t deliveredBytes);
struct YumiGCHotAllocationState;
static void yumiGCChargeGenericRangePacerBytes(
    YumiGCHotAllocationState& hot,
    size_t allocatedBytes);
static void yumiRecordGCPublicAllocation(
    YumiGCHotAllocationState& hot,
    size_t requestedSize,
    size_t allocatedSize);
static void yumiRecordGCPublicAllocations(
    size_t count,
    size_t requestedSize,
    size_t allocatedSize);
static void yumiGCSubtractAtomicSaturating(std::atomic<size_t>& value, size_t amount);
static void yumiPublishGCThreadBatchConsumption();
static void yumiPublishGCThreadAllocationStats();
static bool yumiGCAllocationAssistDisabled();
static bool yumiGCIdleProgressDisabled();
static bool yumiGCVerificationSlowPathsEnabled();
static size_t yumiGCDrainAutomaticCycleBlocking();
static void yumiGCStartBackgroundWorkers();
static size_t yumiGCAutomaticHardGoal(size_t goal);
static size_t yumiGCPacerSoftGoal(size_t goal);
static size_t yumiGCPacerHardGoal(size_t goal);
static size_t yumiGCPacerTriggerReserveBytes(size_t goal);

static bool
yumiGCPublishThreadCounter(std::atomic<size_t>& target, size_t& value) {
    if (!value) {
        return false;
    }
    target.fetch_add(value, std::memory_order_relaxed);
    value = 0;
    return true;
}

template <size_t Count>
static bool
yumiGCPublishThreadCounterArray(
    std::array<std::atomic<size_t>, Count>& target,
    std::array<size_t, Count>& local
) {
    bool published = false;
    for (size_t index = 0; index < Count; index++) {
        published = yumiGCPublishThreadCounter(target[index], local[index]) || published;
    }
    return published;
}

#if YU_ENABLE_BENCH_METRICS
static YumiGCBenchThreadRefillRecord*
yumiGCEnsureBenchThreadRefillRecord() {
    if (gcBenchThreadRefillRecord) {
        return gcBenchThreadRefillRecord;
    }
    std::lock_guard<std::mutex> lock(gcBenchThreadRefillMutex);
    gcBenchThreadRefillRecord =
        new YumiGCBenchThreadRefillRecord(gcBenchThreadRefillRecords.size());
    gcBenchThreadRefillRecords.push_back(gcBenchThreadRefillRecord);
    return gcBenchThreadRefillRecord;
}

static void
yumiGCPublishBenchThreadRefills(size_t count) {
    if (!count) {
        return;
    }
    YumiGCBenchThreadRefillRecord* record = yumiGCEnsureBenchThreadRefillRecord();
    record->refills.store(
        record->refills.load(std::memory_order_relaxed) + count,
        std::memory_order_relaxed);
}
#endif

struct YumiGCThreadStats {
    size_t allocatedObjects = 0;
    size_t requestedBytes = 0;
    size_t allocatedBytes = 0;
    size_t cycleAllocatedBytes = 0;
    size_t batchConsumedBytes = 0;
    size_t zeroedBytes = 0;
    size_t allocationFastHitCount = 0;
    size_t allocationFastRefillCount = 0;
    size_t allocationFastFallbackCount = 0;
    size_t allocationLargeCount = 0;
    size_t allocationAssistCheckCount = 0;
    size_t allocationAssistCallCount = 0;
    size_t assistCount = 0;
    size_t assistTimeNanos = 0;
    size_t allocationSiteHitCount = 0;
    size_t allocationSiteMissCount = 0;
    size_t allocationSiteRefillCount = 0;
    size_t allocationSiteFallbackCount = 0;
    size_t adaptiveBatchGrowthCount = 0;
    size_t adaptiveBatchShrinkCount = 0;
    size_t adaptiveBatchReturnedUnusedSlotCount = 0;
    size_t compilerIndexedRefillCount = 0;
    size_t activeBufferRefillCount = 0;
    size_t genericDescribedRefillCount = 0;
    size_t genericDescriptorlessRefillCount = 0;
    std::array<size_t, YumiGCAllocationCallKindCount> allocationCallsByKind = {};
    std::array<size_t, YumiGCAllocationPhaseBucketCount> allocationCallsByPhase = {};
    std::array<size_t, YumiGCAllocationBatchMissReasonCount> allocationBatchMisses = {};
    std::array<size_t, kYumiGCAllocationBatchCount> allocationDescriptorlessBySizeClass = {};
    std::array<size_t, kYumiGCAllocationBatchCount> allocationDescribedBySizeClass = {};

    ~YumiGCThreadStats() {
        yumiFlushGCAllocationBatches();
        flush();
    }

    bool flushBatchConsumption() {
        if (!batchConsumedBytes) {
            return false;
        }

        gcBatchConsumedBytes.fetch_add(batchConsumedBytes, std::memory_order_relaxed);
        yumiGCSubtractAtomicSaturating(gcBatchCacheBytes, batchConsumedBytes);
        batchConsumedBytes = 0;
        return true;
    }

    void flush() {
        bool published = flushBatchConsumption();
#if YU_ENABLE_BENCH_METRICS
        yumiGCPublishBenchThreadRefills(allocationFastRefillCount);
#endif
        published = yumiGCPublishThreadCounter(gcAllocatedObjects, allocatedObjects) || published;
        published = yumiGCPublishThreadCounter(gcRequestedBytes, requestedBytes) || published;
        published = yumiGCPublishThreadCounter(gcAllocatedBytes, allocatedBytes) || published;
        published = yumiGCPublishThreadCounter(gcCycleAllocatedBytes, cycleAllocatedBytes) ||
            published;
        published = yumiGCPublishThreadCounter(gcAllocationZeroedBytes, zeroedBytes) || published;
        published = yumiGCPublishThreadCounter(gcAllocationFastHitCount, allocationFastHitCount) ||
            published;
        published = yumiGCPublishThreadCounter(
            gcAllocationFastRefillCount,
            allocationFastRefillCount) || published;
        published = yumiGCPublishThreadCounter(
            gcAllocationFastFallbackCount,
            allocationFastFallbackCount) || published;
        published = yumiGCPublishThreadCounter(gcAllocationLargeCount, allocationLargeCount) ||
            published;
        published = yumiGCPublishThreadCounter(
            gcAllocationAssistCheckCount,
            allocationAssistCheckCount) || published;
        published = yumiGCPublishThreadCounter(
            gcAllocationAssistCallCount,
            allocationAssistCallCount) || published;
        published = yumiGCPublishThreadCounter(gcAssistCount, assistCount) || published;
        published = yumiGCPublishThreadCounter(gcAssistTimeNanos, assistTimeNanos) || published;
        published = yumiGCPublishThreadCounter(gcAllocationSiteHitCount, allocationSiteHitCount) ||
            published;
        published = yumiGCPublishThreadCounter(
            gcAllocationSiteMissCount,
            allocationSiteMissCount) || published;
        published = yumiGCPublishThreadCounter(
            gcAllocationSiteRefillCount,
            allocationSiteRefillCount) || published;
        published = yumiGCPublishThreadCounter(
            gcAllocationSiteFallbackCount,
            allocationSiteFallbackCount) || published;
        published = yumiGCPublishThreadCounter(
            gcAdaptiveBatchGrowthCount,
            adaptiveBatchGrowthCount) || published;
        published = yumiGCPublishThreadCounter(
            gcAdaptiveBatchShrinkCount,
            adaptiveBatchShrinkCount) || published;
        published = yumiGCPublishThreadCounter(
            gcAdaptiveBatchReturnedUnusedSlotCount,
            adaptiveBatchReturnedUnusedSlotCount) || published;
        published = yumiGCPublishThreadCounter(
            gcCompilerIndexedRefillCount,
            compilerIndexedRefillCount) || published;
        published = yumiGCPublishThreadCounter(
            gcActiveBufferRefillCount,
            activeBufferRefillCount) || published;
        published = yumiGCPublishThreadCounter(
            gcGenericDescribedRefillCount,
            genericDescribedRefillCount) || published;
        published = yumiGCPublishThreadCounter(
            gcGenericDescriptorlessRefillCount,
            genericDescriptorlessRefillCount) || published;
        published = yumiGCPublishThreadCounterArray(
            gcAllocationCallsByKind,
            allocationCallsByKind) || published;
        published = yumiGCPublishThreadCounterArray(
            gcAllocationCallsByPhase,
            allocationCallsByPhase) || published;
        published = yumiGCPublishThreadCounterArray(
            gcAllocationBatchMisses,
            allocationBatchMisses) || published;
        published = yumiGCPublishThreadCounterArray(
            gcAllocationDescriptorlessBySizeClass,
            allocationDescriptorlessBySizeClass) || published;
        published = yumiGCPublishThreadCounterArray(
            gcAllocationDescribedBySizeClass,
            allocationDescribedBySizeClass) || published;

        if (published) {
            gcAllocationStatPublicationCount.fetch_add(1, std::memory_order_relaxed);
        }
    }
};

struct YumiGCHotAllocationState {
    size_t batchCacheBytes = 0;
    size_t batchPendingReservationBytes = 0;
    size_t genericPacerPendingBytes = 0;
    size_t genericPacerChargedBytes = 0;
    size_t genericPacerCollectionChecks = 0;
    // Keep stats last so its thread-exit destructor can publish through the still-live scalars.
    YumiGCThreadStats stats;
};

static thread_local YumiGCHotAllocationState gcHotAllocationState;

static void
yumiPublishGCThreadBatchConsumption() {
    gcHotAllocationState.stats.flushBatchConsumption();
}

static void
yumiPublishGCThreadAllocationStats() {
    if (!gcAllocationBufferFlushInProgress) {
        yumiFlushGCCompilerAllocationBuffer();
    }
    gcHotAllocationState.stats.flush();
}

static size_t
yumiGCAllocationBatchPreferredCount(size_t allocSize, void* descriptor) {
    if (yumiGCVerificationSlowPathsEnabled()) {
        return 1;
    }
    if (!descriptor) {
        size_t byteCount = allocSize
            ? kYumiGCDescriptorlessAllocationBatchBytes / allocSize
            : kYumiGCDescriptorlessAllocationBatchMaxCount;
        return std::min(
            kYumiGCDescriptorlessAllocationBatchMaxCount,
            std::max(kYumiGCDescriptorlessAllocationBatchMinCount, byteCount));
    }
    if (allocSize <= 256) {
        return kYumiGCDescribedSmallAllocationBatchPreferredCount;
    }
    if (allocSize <= 1024) {
        return kYumiGCDescribedMediumAllocationBatchPreferredCount;
    }
    return kYumiGCDescribedLargeAllocationBatchPreferredCount;
}

static size_t
yumiGCAdaptiveAllocationBatchCount(
    const YumiGCRuntimeAllocationBuffer& storage,
    size_t allocatedSize,
    void* descriptor
) {
    size_t baseCount = yumiGCAllocationBatchPreferredCount(
        allocatedSize,
        descriptor);
    if (!descriptor || yumiGCVerificationSlowPathsEnabled()) {
        return baseCount;
    }
    size_t count = storage.reserved[kYumiGCAdaptiveCountByte];
    return std::max(
        baseCount,
        std::min(count, kYumiGCAdaptiveAllocationBatchMaxCount));
}

static size_t
yumiGCAdaptiveAllocationBatchCount(
    const YumiGCRuntimeAllocationBuffer& storage,
    const YumiGCAllocationSiteKey& key
) {
    return yumiGCAdaptiveAllocationBatchCount(
        storage,
        key.allocatedSize,
        key.descriptor);
}

static void
yumiGCResetAdaptiveAllocationBufferPolicy(YumiGCRuntimeAllocationBuffer& storage) {
    storage.reserved[kYumiGCAdaptiveCountByte] = 0;
    storage.reserved[kYumiGCAdaptiveFullStreakByte] = 0;
    storage.reserved[kYumiGCAdaptiveReservedSlotCountByte] = 0;
}

static void
yumiGCInitializeEmptyRuntimeAllocationBuffer(YumiGCRuntimeAllocationBuffer& storage) {
    storage = YumiGCRuntimeAllocationBuffer {};
    yumiGCInitializeRuntimeAllocationBuffer(&storage);
}

static YumiGCRuntimeAllocationBuffer&
yumiGCThreadRangeWay(YumiGCThreadCache* cache, size_t index, size_t way) {
    if (way == 0) {
        return cache->ranges[index];
    }
    return cache->parkedRanges[index][way - 1];
}

static bool
yumiGCAllocationRangeHasOwnership(const YumiGCRuntimeAllocationBuffer& storage) {
    return storage.base || storage.buffer.cursor || storage.buffer.limit;
}

static bool
yumiGCAllocationRangeHasPolicySite(const YumiGCRuntimeAllocationBuffer& storage) {
    return storage.descriptor && storage.requestedSize && storage.slotSize;
}

static bool
yumiGCAllocationRangeHasUnconsumedSlots(const YumiGCRuntimeAllocationBuffer& storage) {
    return storage.buffer.cursor && storage.buffer.limit &&
        storage.buffer.cursor < storage.buffer.limit;
}

static bool
yumiGCAllocationRangeIsParkable(const YumiGCRuntimeAllocationBuffer& storage) {
    if (!storage.descriptor) {
        return yumiGCAllocationRangeHasUnconsumedSlots(storage);
    }
    return yumiGCAllocationRangeHasOwnership(storage) ||
        yumiGCAllocationRangeHasPolicySite(storage);
}

static bool
yumiGCAllocationRangeMatchesSite(
    const YumiGCRuntimeAllocationBuffer& storage,
    size_t requestedSize,
    size_t allocatedSize,
    void* descriptor,
    size_t generation
) {
    return
        storage.descriptor == descriptor &&
        storage.requestedSize == requestedSize &&
        storage.slotSize >= allocatedSize &&
        storage.generation == generation;
}

static void
yumiGCMoveRuntimeAllocationBuffer(
    YumiGCRuntimeAllocationBuffer& destination,
    YumiGCRuntimeAllocationBuffer& source
) {
    destination = source;
    yumiGCInitializeEmptyRuntimeAllocationBuffer(source);
}

static void
yumiGCResetAdaptiveAllocationBufferPolicies(YumiGCThreadCache* cache) {
    if (!cache) {
        return;
    }
    for (size_t index = 0; index < YumiGCCompilerAllocationBufferCount; ++index) {
        yumiGCResetAdaptiveAllocationBufferPolicy(cache->compilerBuffers[index]);
    }
    for (size_t index = 0; index < YumiGCThreadCacheRangeCount; ++index) {
        yumiGCResetAdaptiveAllocationBufferPolicy(cache->ranges[index]);
        for (size_t way = 0; way < YumiGCThreadCacheParkedRangeWayCount; ++way) {
            yumiGCResetAdaptiveAllocationBufferPolicy(cache->parkedRanges[index][way]);
        }
        cache->rangeNextVictimWay[index] = 0;
    }
}

static size_t
yumiGCAllocationPhaseBucket(unsigned char phase) {
    switch (phase) {
    case kYumiGCPhaseIdle:
        return YumiGCAllocationPhaseIdle;
    case kYumiGCPhaseMarkSetup:
        return YumiGCAllocationPhaseMarkSetup;
    case kYumiGCPhaseConcurrentMark:
    case kYumiGCPhaseMarkTermination:
        return YumiGCAllocationPhaseConcurrentMark;
    case kYumiGCPhaseMarkComplete:
        return YumiGCAllocationPhaseMarkComplete;
    case kYumiGCPhaseSweep:
        return YumiGCAllocationPhaseSweep;
    default:
        return YumiGCAllocationPhaseOther;
    }
}

static bool
yumiGCPhaseAllowsFastBatchAllocation(unsigned char phase) {
    return phase <= kYumiGCPhaseSweep;
}

static void
yumiGCRecordAllocationPhaseCount(YumiGCHotAllocationState& hot, size_t count) {
    if (!count) {
        return;
    }
    size_t phase = yumiGCAllocationPhaseBucket(yumiGCLoadPublicPhase());
    hot.stats.allocationCallsByPhase[phase] += count;
}

static void
yumiGCRecordPublicAllocationCall(YumiGCHotAllocationState& hot, size_t kind) {
    if (kind < gcAllocationCallsByKind.size()) {
        hot.stats.allocationCallsByKind[kind] += 1;
    }
}

#if YU_ENABLE_BENCH_METRICS
static size_t
yumiGCBenchAllocationSizeBin(size_t requestedSize) {
    if (requestedSize > 0 && requestedSize <= 1024) {
        return (requestedSize - 1) / 8;
    }
    if (requestedSize > 1024 && requestedSize <= 64 * 1024) {
        return kYumiGCBenchAllocationSmallBinCount + (requestedSize - 1025) / 1024;
    }
    return kYumiGCBenchAllocationOverflowBin;
}

static void
yumiGCBenchRecordAllocationSizes(size_t count, size_t requestedSize) {
    if (yumiBenchMetricsEnabledFlag && count) {
        gcBenchAllocationSizeBins[yumiGCBenchAllocationSizeBin(requestedSize)].fetch_add(
            count,
            std::memory_order_relaxed);
    }
}

static void
yumiGCBenchRecordAllocatedSizeClasses(size_t count, size_t allocatedSize) {
    if (!yumiBenchMetricsEnabledFlag || !count) {
        return;
    }
    size_t sizeClass = kYumiGCBenchLargeSizeClass;
    for (size_t index = 1; index < kYumiGCBenchLargeSizeClass; ++index) {
        if (allocatedSize <= kYumiGCBenchSizeClassUpperBounds[index]) {
            sizeClass = index;
            break;
        }
    }
    gcBenchAllocatedSizeClasses[sizeClass].fetch_add(count, std::memory_order_relaxed);
}
#else
static void yumiGCBenchRecordAllocationSizes(size_t, size_t) {}
static void yumiGCBenchRecordAllocatedSizeClasses(size_t, size_t) {}
#endif

static void
yumiGCRecordAllocationBatchMiss(size_t reason) {
    if (reason < gcAllocationBatchMisses.size()) {
        gcHotAllocationState.stats.allocationBatchMisses[reason] += 1;
    }
}

static void
yumiGCRecordAllocationDescriptorMix(
    YumiGCHotAllocationState& hot,
    size_t index,
    void* descriptor
) {
    if (index >= kYumiGCAllocationBatchCount) {
        return;
    }
    auto& counters = descriptor
        ? hot.stats.allocationDescribedBySizeClass
        : hot.stats.allocationDescriptorlessBySizeClass;
    counters[index] += 1;
}

static void
yumiGCRecordAllocationZeroedBytes(size_t bytes) {
    if (bytes) {
        gcHotAllocationState.stats.zeroedBytes += bytes;
    }
}

class YumiGCPanicSuspension {
public:
    YumiGCPanicSuspension()
        : panic(yumiPanicSuspend()) {}

    ~YumiGCPanicSuspension() {
        yumiPanicResume(panic);
    }

private:
    void* panic;
};

class YumiGCRuntimeCritical {
public:
    YumiGCRuntimeCritical() {
        yumiGCEnterRuntimeCritical();
    }

    ~YumiGCRuntimeCritical() {
        yumiGCLeaveRuntimeCritical();
    }

    YumiGCRuntimeCritical(const YumiGCRuntimeCritical&) = delete;
    YumiGCRuntimeCritical& operator=(const YumiGCRuntimeCritical&) = delete;
};

class YumiGCCompilerAllocationBufferFlush {
public:
    YumiGCCompilerAllocationBufferFlush() {
        gcAllocationBufferFlushInProgress = true;
    }

    ~YumiGCCompilerAllocationBufferFlush() {
        gcAllocationBufferFlushInProgress = false;
    }

    YumiGCCompilerAllocationBufferFlush(const YumiGCCompilerAllocationBufferFlush&) = delete;
    YumiGCCompilerAllocationBufferFlush& operator=(
        const YumiGCCompilerAllocationBufferFlush&) = delete;
};

class YumiGCRuntimeCall {
public:
    YumiGCRuntimeCall() {
        gcRuntimeCallDepth += 1;
    }

    ~YumiGCRuntimeCall() {
        if (gcRuntimeCallDepth > 0) {
            gcRuntimeCallDepth -= 1;
        }
        if (gcRuntimeCallDepth == 0 && gcDeferredCompilerAllocationBufferFlush) {
            gcDeferredCompilerAllocationBufferFlush = false;
            yumiFlushGCCompilerAllocationBuffer();
        }
    }

    YumiGCRuntimeCall(const YumiGCRuntimeCall&) = delete;
    YumiGCRuntimeCall& operator=(const YumiGCRuntimeCall&) = delete;
};

template <typename F>
static auto
yumiCallIgnoringPanic(F f) -> decltype(f()) {
    YumiGCPanicSuspension suspension;
    return f();
}

template <typename F>
static auto
yumiCallGCIgnoringPanic(F f) -> decltype(f()) {
    YumiGCPanicSuspension suspension;
    YumiGCRuntimeCall runtimeCall;
    if (yumiGCIsRuntimeCritical()) {
        return f();
    }
    YumiGCRuntimeCritical critical;
    return f();
}

template <typename F>
static auto
yumiCallGCFallbackAllocation(F f) -> decltype(f()) {
    uint64_t start = yumiGCMonotonicNanos();
    auto result = yumiCallGCIgnoringPanic(f);
    uint64_t end = yumiGCMonotonicNanos();
    size_t duration = static_cast<size_t>(end - start);
    gcFallbackAllocationCount.fetch_add(1, std::memory_order_relaxed);
    gcFallbackAllocationTimeNanos.fetch_add(duration, std::memory_order_relaxed);
    yumiGCRecordSubphaseSample(
        YumiGCObservationFallbackAllocation,
        gcCurrentCycleId.load(std::memory_order_acquire),
        start,
        duration,
        result ? 1 : 0);
    return result;
}

static void*
yumiGCPushTemporaryRoot(char* pointer) {
    if (void* root = yumiGCAddIdleTemporaryRoot(pointer)) {
        return root;
    }
    return yumiCallGCIgnoringPanic([&]() {
        return yumiGCAddTemporaryRoot(pointer);
    });
}

static void
yumiGCPopTemporaryRoot(void* root) {
    if (yumiGCRemoveTaskTemporaryRoot(root)) {
        return;
    }
    yumiCallGCIgnoringPanic([&]() {
        yumiGCRemoveTemporaryRoot(root);
    });
}

class YumiGCTemporaryRoot {
public:
    explicit YumiGCTemporaryRoot(char* pointer) {
        if (pointer) {
            root = yumiGCPushTemporaryRoot(pointer);
        }
    }

    ~YumiGCTemporaryRoot() {
        if (root) {
            yumiGCPopTemporaryRoot(root);
        }
    }

    YumiGCTemporaryRoot(const YumiGCTemporaryRoot&) = delete;
    YumiGCTemporaryRoot& operator=(const YumiGCTemporaryRoot&) = delete;

private:
    void* root = nullptr;
};

static bool
yumiReleaseCachedGCAllocationBatch(YumiGCThreadAllocationBatch& batch) {
    yumiPublishGCThreadBatchConsumption();
    if (!batch.cursor || !batch.limit || batch.cursor >= batch.limit) {
        batch = YumiGCThreadAllocationBatch {};
        return true;
    }

    size_t generation = yumiGCLoadPublicSweepGeneration();
    if (batch.generation != generation) {
        yumiDiscardCachedGCAllocationBatch(batch);
        return true;
    }

    YumiGCThreadAllocationBatch releasedBatch = batch;
    bool released = yumiCallGCIgnoringPanic([&]() {
        return yumiGCReleaseUnusedBatch(
            releasedBatch.cursor,
            releasedBatch.limit,
            releasedBatch.slotSize,
            releasedBatch.generation);
    });
    if (released) {
        batch = YumiGCThreadAllocationBatch {};
    }
    return released;
}

static void
yumiClearGCCompilerAllocationBuffer(YumiGCRuntimeAllocationBuffer& storage) {
    unsigned char adaptivePolicy[sizeof(storage.reserved)] {};
    std::memcpy(adaptivePolicy, storage.reserved, sizeof(adaptivePolicy));
    storage = YumiGCRuntimeAllocationBuffer {};
    yumiGCInitializeRuntimeAllocationBuffer(&storage);
    std::memcpy(storage.reserved, adaptivePolicy, sizeof(adaptivePolicy));
}

static size_t
yumiGCAdaptCompilerAllocationBufferAfterUse(YumiGCRuntimeAllocationBuffer& storage) {
    if (!storage.reserved[kYumiGCAdaptiveEnabledByte] ||
            !storage.descriptor || !storage.buffer.limit || !storage.slotSize) {
        return 0;
    }

    char* cursor = storage.buffer.cursor;
    if (!cursor) {
        cursor = storage.base;
    }
    if (!cursor) {
        return 0;
    }
    if (storage.base && cursor < storage.base) {
        cursor = storage.base;
    }
    if (cursor > storage.buffer.limit) {
        cursor = storage.buffer.limit;
    }
    size_t totalSlots = storage.reserved[kYumiGCAdaptiveReservedSlotCountByte];
    if (!totalSlots && storage.base && storage.buffer.limit > storage.base) {
        totalSlots = static_cast<size_t>(storage.buffer.limit - storage.base) /
            storage.slotSize;
    }
    size_t unusedSlots = cursor < storage.buffer.limit
        ? static_cast<size_t>(storage.buffer.limit - cursor) / storage.slotSize
        : 0;
    size_t baseCount = yumiGCAllocationBatchPreferredCount(
        storage.slotSize,
        storage.descriptor);
    size_t count = std::max(
        baseCount,
        std::min(
            static_cast<size_t>(storage.reserved[kYumiGCAdaptiveCountByte]),
            kYumiGCAdaptiveAllocationBatchMaxCount));

    if (totalSlots && !unusedSlots) {
        size_t streak = storage.reserved[kYumiGCAdaptiveFullStreakByte] + 1;
        if (streak >= kYumiGCAdaptiveFullConsumptionStreak &&
                count < kYumiGCAdaptiveAllocationBatchMaxCount) {
            count = std::min(count * 2, kYumiGCAdaptiveAllocationBatchMaxCount);
            streak = 0;
            gcHotAllocationState.stats.adaptiveBatchGrowthCount += 1;
        }
        storage.reserved[kYumiGCAdaptiveCountByte] = static_cast<unsigned char>(count);
        storage.reserved[kYumiGCAdaptiveFullStreakByte] =
            static_cast<unsigned char>(streak);
        return 0;
    }

    storage.reserved[kYumiGCAdaptiveFullStreakByte] = 0;
    if (totalSlots &&
            unusedSlots * kYumiGCAdaptiveLowUseDenominator >=
                totalSlots * kYumiGCAdaptiveLowUseUnusedParts &&
            count > baseCount) {
        count = std::max(baseCount, count / 2);
        storage.reserved[kYumiGCAdaptiveCountByte] = static_cast<unsigned char>(count);
        gcHotAllocationState.stats.adaptiveBatchShrinkCount += 1;
    }
    return unusedSlots;
}

static void
yumiRecordGCCompilerAllocationBufferConsumption(
    YumiGCRuntimeAllocationBuffer& storage,
    char*& remainingCursor
) {
    remainingCursor = storage.buffer.cursor;
    if (!storage.base || !storage.buffer.limit || !storage.slotSize) {
        remainingCursor = nullptr;
        return;
    }

    char* cursor = storage.buffer.cursor;
    if (!cursor || cursor < storage.base) {
        cursor = storage.base;
    }
    if (cursor > storage.buffer.limit) {
        cursor = storage.buffer.limit;
    }

    size_t consumedBytes = static_cast<size_t>(cursor - storage.base);
    consumedBytes -= consumedBytes % storage.slotSize;
    size_t count = consumedBytes / storage.slotSize;
    remainingCursor = storage.base + consumedBytes;
    if (!count) {
        return;
    }

    yumiRecordGCPublicAllocations(count, storage.requestedSize, storage.slotSize);
#if YU_ENABLE_BENCH_METRICS
    if (yumiBenchMetricsEnabledFlag) {
        size_t site = 0;
        // Compiler buffers use a stable 1..16 identity. The generic buffer has no compiler
        // identity and is therefore grouped into site 0.
        for (size_t index = 0; index < YumiGCCompilerAllocationBufferCount; ++index) {
            if (&storage == &yumiGCEnsureActiveThreadCache()->compilerBuffers[index]) {
                site = index + 1;
                break;
            }
        }
        if (site > YumiGCCompilerAllocationBufferCount) {
            site = 0;
        }
        gcBenchAllocationSites[site].fetch_add(count, std::memory_order_relaxed);
    }
#endif
    yumiGCRecordAllocationZeroedBytes(consumedBytes);
    gcHotAllocationState.stats.batchConsumedBytes += consumedBytes;
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > consumedBytes
        ? gcHotAllocationState.batchCacheBytes - consumedBytes
        : 0;

    size_t kind = storage.descriptor
        ? YumiGCAllocationCallDescribedObject
        : YumiGCAllocationCallObject;
    gcHotAllocationState.stats.allocationCallsByKind[kind] += count;
    if (storage.sizeClassIndex < kYumiGCAllocationBatchCount) {
        auto& mix = storage.descriptor
            ? gcHotAllocationState.stats.allocationDescribedBySizeClass
            : gcHotAllocationState.stats.allocationDescriptorlessBySizeClass;
        mix[storage.sizeClassIndex] += count;
    }
    if (count > 1) {
        size_t hits = count - 1;
        gcHotAllocationState.stats.allocationFastHitCount += hits;
        gcHotAllocationState.stats.allocationSiteHitCount += hits;
    }
}

static void
yumiPublishGCCompilerAllocationBufferConsumption(
    YumiGCRuntimeAllocationBuffer& storage
) {
    char* remainingCursor = nullptr;
    yumiRecordGCCompilerAllocationBufferConsumption(storage, remainingCursor);
    if (remainingCursor) {
        // Consumption before this point is now globally accounted. Keep ownership of the
        // unconsumed tail and advance the accounting base so a later publication is delta-only.
        storage.base = remainingCursor;
    }
}

static void
yumiPublishGCCompilerAllocationBufferConsumptions() {
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    if (gcActiveAllocationBufferStorage) {
        yumiPublishGCCompilerAllocationBufferConsumption(
            *gcActiveAllocationBufferStorage);
    }
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        size_t slot = cache->activeCompilerBufferIndices[active];
        if (slot >= YumiGCCompilerAllocationBufferCount) {
            continue;
        }
        YumiGCRuntimeAllocationBuffer& storage = cache->compilerBuffers[slot];
        if (gcActiveAllocationBufferStorage != &storage) {
            yumiPublishGCCompilerAllocationBufferConsumption(storage);
        }
    }
    for (size_t index = 0; index < YumiGCThreadCacheRangeCount; ++index) {
        YumiGCRuntimeAllocationBuffer& storage = cache->ranges[index];
        if (gcActiveAllocationBufferStorage != &storage) {
            yumiPublishGCCompilerAllocationBufferConsumption(storage);
        }
        for (size_t way = 0; way < YumiGCThreadCacheParkedRangeWayCount; ++way) {
            yumiPublishGCCompilerAllocationBufferConsumption(
                cache->parkedRanges[index][way]);
        }
    }
    gcHotAllocationState.stats.flush();
}

static size_t
yumiGCCompilerAllocationBufferConsumedBytes(const YumiGCRuntimeAllocationBuffer& storage) {
    if (!storage.base || !storage.buffer.limit || !storage.slotSize) {
        return 0;
    }

    char* cursor = storage.buffer.cursor;
    if (!cursor || cursor < storage.base) {
        cursor = storage.base;
    }
    if (cursor > storage.buffer.limit) {
        cursor = storage.buffer.limit;
    }

    size_t consumedBytes = static_cast<size_t>(cursor - storage.base);
    return consumedBytes - (consumedBytes % storage.slotSize);
}

static void
yumiGCAddPendingCompilerAllocationBufferStats(
    const YumiGCRuntimeAllocationBuffer& storage,
    size_t& objectCount,
    size_t& consumedBytes
) {
    size_t bytes = yumiGCCompilerAllocationBufferConsumedBytes(storage);
    if (!bytes || !storage.slotSize) {
        return;
    }
    objectCount += bytes / storage.slotSize;
    consumedBytes += bytes;
}

static void
yumiFlushGCCompilerAllocationBuffer(YumiGCRuntimeAllocationBuffer& storage) {
    if (gcAllocationBufferFlushInProgress) {
        return;
    }
    if (!storage.base && !storage.buffer.cursor && !storage.buffer.limit) {
        return;
    }

    YumiGCCompilerAllocationBufferFlush flush;
    size_t adaptiveUnusedSlots = yumiGCAdaptCompilerAllocationBufferAfterUse(storage);
    char* remainingCursor = nullptr;
    yumiRecordGCCompilerAllocationBufferConsumption(storage, remainingCursor);

    if (remainingCursor && storage.buffer.limit && remainingCursor < storage.buffer.limit) {
        YumiGCThreadAllocationBatch remaining {};
        remaining.cursor = remainingCursor;
        remaining.limit = storage.buffer.limit;
        remaining.slotSize = storage.slotSize;
        remaining.requestedSize = storage.requestedSize;
        remaining.descriptor = storage.descriptor;
        remaining.generation = storage.generation;
        if (yumiReleaseCachedGCAllocationBatch(remaining)) {
            gcHotAllocationState.stats.adaptiveBatchReturnedUnusedSlotCount += adaptiveUnusedSlots;
        } else {
            yumiDiscardCachedGCAllocationBatch(remaining);
        }
    }

    yumiClearGCCompilerAllocationBuffer(storage);
}

static void
yumiGCClearStagedGCAllocationRange(YumiGCRuntimeAllocationBuffer& storage) {
    if (yumiGCAllocationRangeHasOwnership(storage)) {
        yumiFlushGCCompilerAllocationBuffer(storage);
    }
    yumiGCResetAdaptiveAllocationBufferPolicy(storage);
    yumiGCInitializeEmptyRuntimeAllocationBuffer(storage);
}

static void
yumiGCInvalidateStaleThreadRangeWays(
    YumiGCThreadCache* cache,
    size_t index,
    size_t generation
) {
    for (size_t way = 0; way < YumiGCThreadCacheRangeWayCount; ++way) {
        YumiGCRuntimeAllocationBuffer& range = yumiGCThreadRangeWay(cache, index, way);
        if ((yumiGCAllocationRangeHasOwnership(range) ||
                yumiGCAllocationRangeHasPolicySite(range)) &&
                range.generation != generation) {
            yumiGCClearStagedGCAllocationRange(range);
        }
    }
}

static size_t
yumiGCFindThreadRangeWay(
    YumiGCThreadCache* cache,
    size_t index,
    size_t requestedSize,
    size_t allocatedSize,
    void* descriptor,
    size_t generation
) {
    size_t policyOnlyWay = YumiGCThreadCacheRangeWayCount;
    for (size_t way = 0; way < YumiGCThreadCacheRangeWayCount; ++way) {
        YumiGCRuntimeAllocationBuffer& range = yumiGCThreadRangeWay(cache, index, way);
        if (!yumiGCAllocationRangeMatchesSite(
                range,
                requestedSize,
                allocatedSize,
                descriptor,
                generation)) {
            continue;
        }
        if (range.buffer.cursor && range.buffer.limit &&
                range.buffer.cursor < range.buffer.limit) {
            return way;
        }
        if (policyOnlyWay == YumiGCThreadCacheRangeWayCount) {
            policyOnlyWay = way;
        }
    }
    return policyOnlyWay;
}

static size_t
yumiGCFindDescriptorlessThreadRangeWay(
    YumiGCThreadCache* cache,
    size_t index,
    size_t allocatedSize,
    size_t generation
) {
    for (size_t way = 1; way < YumiGCThreadCacheRangeWayCount; ++way) {
        YumiGCRuntimeAllocationBuffer& range = yumiGCThreadRangeWay(cache, index, way);
        if (!range.descriptor &&
                range.generation == generation &&
                range.slotSize >= allocatedSize &&
                yumiGCAllocationRangeHasUnconsumedSlots(range)) {
            return way;
        }
    }
    return YumiGCThreadCacheRangeWayCount;
}

static size_t
yumiGCChooseThreadRangeVictimWay(
    YumiGCThreadCache* cache,
    size_t index,
    size_t firstWay
) {
    for (size_t way = firstWay; way < YumiGCThreadCacheRangeWayCount; ++way) {
        YumiGCRuntimeAllocationBuffer& range = yumiGCThreadRangeWay(cache, index, way);
        if (!yumiGCAllocationRangeHasOwnership(range) &&
                !yumiGCAllocationRangeHasPolicySite(range)) {
            return way;
        }
    }

    size_t wayCount = YumiGCThreadCacheRangeWayCount - firstWay;
    size_t victim = firstWay + cache->rangeNextVictimWay[index] % wayCount;
    cache->rangeNextVictimWay[index] = static_cast<unsigned char>(
        (cache->rangeNextVictimWay[index] + 1) % wayCount);
    return victim;
}

static void
yumiGCParkGCAllocationRange(
    YumiGCThreadCache* cache,
    size_t index,
    YumiGCRuntimeAllocationBuffer& source,
    size_t firstWay
) {
    if (!yumiGCAllocationRangeIsParkable(source)) {
        yumiFlushGCCompilerAllocationBuffer(source);
        return;
    }

    size_t victimWay = yumiGCChooseThreadRangeVictimWay(cache, index, firstWay);
    YumiGCRuntimeAllocationBuffer& victim =
        yumiGCThreadRangeWay(cache, index, victimWay);
    yumiGCClearStagedGCAllocationRange(victim);
    yumiGCMoveRuntimeAllocationBuffer(victim, source);
}

static bool
yumiGCTakeParkedGCAllocationRange(
    YumiGCThreadCache* cache,
    const YumiGCAllocationSiteKey& key,
    size_t generation,
    YumiGCRuntimeAllocationBuffer& destination
) {
    yumiGCInvalidateStaleThreadRangeWays(cache, key.sizeClassIndex, generation);
    size_t way = yumiGCFindThreadRangeWay(
        cache,
        key.sizeClassIndex,
        key.requestedSize,
        key.allocatedSize,
        key.descriptor,
        generation);
    if (way == YumiGCThreadCacheRangeWayCount) {
        return false;
    }
    YumiGCRuntimeAllocationBuffer& range =
        yumiGCThreadRangeWay(cache, key.sizeClassIndex, way);
    yumiGCMoveRuntimeAllocationBuffer(destination, range);
    return true;
}

static void
yumiGCParkCompilerGCAllocationRange(
    YumiGCThreadCache* cache,
    YumiGCRuntimeAllocationBuffer& storage,
    size_t generation
) {
    if (!storage.descriptor || storage.sizeClassIndex >= YumiGCThreadCacheRangeCount) {
        yumiFlushGCCompilerAllocationBuffer(storage);
        return;
    }
    if (storage.generation != generation) {
        yumiGCClearStagedGCAllocationRange(storage);
        return;
    }

    yumiPublishGCCompilerAllocationBufferConsumption(storage);
    if (!storage.buffer.cursor || !storage.buffer.limit ||
            storage.buffer.cursor >= storage.buffer.limit) {
        yumiGCAdaptCompilerAllocationBufferAfterUse(storage);
    }
    yumiGCParkGCAllocationRange(cache, storage.sizeClassIndex, storage, 0);
}

static bool
yumiGCCompilerAllocationBufferIndexIsValid(size_t index) {
    return index > 0 && index <= YumiGCCompilerAllocationBufferCount;
}

static void
yumiMarkGCCompilerAllocationBufferActive(size_t index) {
    if (!yumiGCCompilerAllocationBufferIndexIsValid(index)) {
        return;
    }

    size_t slot = index - 1;
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        if (cache->activeCompilerBufferIndices[active] == slot) {
            return;
        }
    }
    if (cache->activeCompilerBufferCount >= YumiGCCompilerAllocationBufferCount) {
        return;
    }

    cache->activeCompilerBufferIndices[cache->activeCompilerBufferCount++] =
        static_cast<uint32_t>(slot);
}

static void
yumiFlushGCCompilerAllocationBuffer() {
    if (gcRuntimeCallDepth > 0 && !gcAllocationBufferFlushInProgress) {
        gcDeferredCompilerAllocationBufferFlush = true;
        return;
    }
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    bool flushedOwnerState = false;
    size_t flushedBufferCount = 0;
    if (gcActiveAllocationBufferStorage) {
        bool activeBufferLive =
            gcActiveAllocationBufferStorage == &cache->activeBuffer &&
            (gcActiveAllocationBufferStorage->base ||
             gcActiveAllocationBufferStorage->buffer.cursor ||
             gcActiveAllocationBufferStorage->buffer.limit);
        flushedOwnerState = activeBufferLive;
        flushedBufferCount += activeBufferLive ? 1 : 0;
        yumiFlushGCCompilerAllocationBuffer(*gcActiveAllocationBufferStorage);
    }
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        size_t slot = cache->activeCompilerBufferIndices[active];
        if (slot >= YumiGCCompilerAllocationBufferCount) {
            continue;
        }
        bool compilerBufferLive =
            cache->compilerBuffers[slot].base ||
            cache->compilerBuffers[slot].buffer.cursor ||
            cache->compilerBuffers[slot].buffer.limit;
        flushedOwnerState = flushedOwnerState || compilerBufferLive;
        flushedBufferCount += compilerBufferLive ? 1 : 0;
        yumiFlushGCCompilerAllocationBuffer(cache->compilerBuffers[slot]);
    }
    cache->activeCompilerBufferCount = 0;
    for (size_t index = 0; index < YumiGCThreadCacheRangeCount; ++index) {
        bool rangeLive =
            cache->ranges[index].base ||
            cache->ranges[index].buffer.cursor ||
            cache->ranges[index].buffer.limit;
        flushedOwnerState = flushedOwnerState || rangeLive;
        flushedBufferCount += rangeLive ? 1 : 0;
        yumiFlushGCCompilerAllocationBuffer(cache->ranges[index]);
        for (size_t way = 0; way < YumiGCThreadCacheParkedRangeWayCount; ++way) {
            YumiGCRuntimeAllocationBuffer& parked = cache->parkedRanges[index][way];
            bool parkedLive =
                parked.base || parked.buffer.cursor || parked.buffer.limit;
            flushedOwnerState = flushedOwnerState || parkedLive;
            flushedBufferCount += parkedLive ? 1 : 0;
            yumiFlushGCCompilerAllocationBuffer(parked);
        }
    }
    if (flushedOwnerState) {
        gcFullAllocationRangeFlushCount.fetch_add(1, std::memory_order_relaxed);
        gcFullAllocationRangeFlushedBufferCount.fetch_add(
            flushedBufferCount,
            std::memory_order_relaxed);
        cache->flushEpoch += 1;
        gcThreadCacheOwnerFlushCount.fetch_add(1, std::memory_order_relaxed);
    }
}

static void
yumiFlushGCAllocationBatches() {
    yumiFlushGCCompilerAllocationBuffer();
}

static bool
yumiGCPhaseNeedsMarkBarrier(unsigned char phase) {
    return phase >= 1 && phase <= 4;
}

static bool
yumiGCPhaseNeedsSweepPublicationValidation(unsigned char phase) {
    return phase == kYumiGCPhaseSweep;
}

static YumiGCBarrierStatsRecord*
yumiGCEnsureThreadBarrierStats() {
    if (gcThreadBarrierStats) {
        return gcThreadBarrierStats;
    }
    YumiGCBarrierStatsRecord* record = new YumiGCBarrierStatsRecord();
    {
        std::lock_guard<std::mutex> lock(gcBarrierStatsMutex);
        gcBarrierStatsRecords.push_back(record);
    }
    gcThreadBarrierStats = record;
    return record;
}

static void
yumiGCAddOwnedBarrierCounter(std::atomic<size_t>& counter, size_t amount) {
    if (!amount) {
        return;
    }
    // This record has exactly one OS-thread writer. Atomic load/store lets cold metric readers
    // sample it without putting a shared read-modify-write on barrier or buffer-flush paths.
    counter.store(counter.load(std::memory_order_relaxed) + amount,
        std::memory_order_relaxed);
}

static size_t
yumiGCSumBarrierCounter(YumiGCBarrierStatsKind kind) {
    std::lock_guard<std::mutex> lock(gcBarrierStatsMutex);
    size_t total = 0;
    for (YumiGCBarrierStatsRecord* record : gcBarrierStatsRecords) {
        const std::atomic<size_t>* counter = nullptr;
        switch (kind) {
            case YumiGCBarrierStatsCalls:
                counter = &record->barrierCalls;
                break;
            case YumiGCBarrierStatsSlowEntries:
                counter = &record->barrierSlowEntries;
                break;
            case YumiGCBarrierStatsShades:
                counter = &record->barrierShades;
                break;
            case YumiGCBarrierStatsFastCalls:
                counter = &record->fastCalls;
                break;
            case YumiGCBarrierStatsFastSlowEntries:
                counter = &record->fastSlowEntries;
                break;
        }
        if (counter) {
            total += counter->load(std::memory_order_relaxed);
        }
    }
    return total;
}

static void
yumiPublishGCWriteBarrierCounters(YumiGCThreadCache* cache) {
    if (!cache) {
        return;
    }
    uint64_t calls = cache->counters[kYumiGCThreadCounterBarrierCalls];
    uint64_t slowEntries = cache->counters[kYumiGCThreadCounterBarrierSlowEntries];
    uint64_t shades = cache->counters[kYumiGCThreadCounterBarrierShades];
    cache->counters[kYumiGCThreadCounterBarrierCalls] = 0;
    cache->counters[kYumiGCThreadCounterBarrierSlowEntries] = 0;
    cache->counters[kYumiGCThreadCounterBarrierShades] = 0;
    if (!calls && !slowEntries && !shades) {
        return;
    }
    YumiGCBarrierStatsRecord* record = yumiGCEnsureThreadBarrierStats();
    yumiGCAddOwnedBarrierCounter(record->fastCalls, calls);
    yumiGCAddOwnedBarrierCounter(record->barrierCalls, calls);
    yumiGCAddOwnedBarrierCounter(record->fastSlowEntries, slowEntries);
    yumiGCAddOwnedBarrierCounter(record->barrierSlowEntries, slowEntries);
    yumiGCAddOwnedBarrierCounter(record->barrierShades, shades);
}

extern "C" void
yumiBenchRecordBarrierCall() {
    // Keep the compiler hot path to one thread-owned increment. Task suspension, termination,
    // handshake acknowledgement, thread-cache detachment, and metric reads all publish this
    // cache into its OS-thread-owned record without a shared RMW.
    yumiGCEnsureActiveThreadCache()->counters[kYumiGCThreadCounterBarrierCalls] += 1;
}

extern "C" void
yumiWBBufFlush() {
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    size_t count = std::min<size_t>(
        cache->writeBarrierCount,
        YumiGCWriteBarrierBufferCapacity);
    if (count) {
        size_t shades = yumiGCFlushWriteBarrierEntries(cache->writeBarrierEntries, count);
        if (shades) {
            YumiGCBarrierStatsRecord* record = yumiGCEnsureThreadBarrierStats();
            yumiGCAddOwnedBarrierCounter(record->barrierShades, shades);
        }
        for (size_t index = 0; index < count; ++index) {
            cache->writeBarrierEntries[index] = nullptr;
        }
        cache->writeBarrierCount = 0;
    }
    yumiPublishGCWriteBarrierCounters(cache);
}

static void
yumiWBBufRecordActiveTaskPointer(char* pointer) {
    if (!pointer) {
        return;
    }
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    if (cache->writeBarrierCount >= YumiGCWriteBarrierBufferCapacity) {
        yumiWBBufFlush();
    }
    cache->writeBarrierEntries[cache->writeBarrierCount++] = pointer;
    if (cache->writeBarrierCount == YumiGCWriteBarrierBufferCapacity) {
        yumiWBBufFlush();
    }
}

extern "C" void
yumiWBBufRecordPointer(char* pointer) {
    if (!pointer) {
        return;
    }
    if (!yumiGCActiveTaskCanUseFastAllocation()) {
        yumiGCWriteBarrier(nullptr, pointer);
        return;
    }
    yumiWBBufRecordActiveTaskPointer(pointer);
}

static uint64_t
yumiGCMonotonicNanos() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static bool
yumiGCValidSubphase(size_t subphase) {
    return subphase > 0 && subphase < kYumiGCSubphaseCount;
}

static void
yumiGCSubtractAtomicSaturating(std::atomic<size_t>& value, size_t amount) {
    size_t current = value.load(std::memory_order_relaxed);
    while (
        current &&
        !value.compare_exchange_weak(
            current,
            current > amount ? current - amount : 0,
            std::memory_order_relaxed,
            std::memory_order_relaxed
        )
    ) {}
}

static void
yumiGCMaxAtomic(std::atomic<size_t>& value, size_t amount) {
    size_t current = value.load(std::memory_order_relaxed);
    while (
        current < amount &&
        !value.compare_exchange_weak(
            current,
            amount,
            std::memory_order_relaxed,
            std::memory_order_relaxed
        )
    ) {}
}

static void
yumiGCRecordSubphaseSample(
    size_t subphase,
    size_t,
    uint64_t,
    size_t durationNanos,
    size_t value
) {
    if (!yumiGCValidSubphase(subphase)) {
        return;
    }

    if (subphase == YumiGCObservationRootScan) {
        if (durationNanos) {
            gcPacerCurrentRootScanNanos.store(durationNanos, std::memory_order_release);
        }
    } else if (subphase == YumiGCObservationMarkWork) {
        gcPacerCurrentMarkWorkItems.fetch_add(value, std::memory_order_relaxed);
        if (durationNanos) {
            gcPacerCurrentMarkWorkNanos.fetch_add(durationNanos, std::memory_order_relaxed);
        }
    } else if (subphase == YumiGCObservationAssist) {
        gcPacerCurrentAssistWorkItems.fetch_add(value, std::memory_order_relaxed);
        if (durationNanos) {
            gcPacerCurrentAssistWorkNanos.fetch_add(durationNanos, std::memory_order_relaxed);
            gcHotAllocationState.stats.assistTimeNanos += durationNanos;
        }
    }

    if (!durationNanos) {
        return;
    }

    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    YumiGCSubphaseSamples& samples = gcSubphaseSamples[subphase];
    ++samples.sampleSequence;
    if (samples.durations.size() < kYumiGCSubphaseSampleCapacity) {
        samples.durations.push_back(durationNanos);
    } else {
        if (!samples.randomState) {
            samples.randomState = 0x9e3779b97f4a7c15ull ^
                (static_cast<uint64_t>(subphase) * 0xbf58476d1ce4e5b9ull);
        }
        samples.randomState ^= samples.randomState >> 12;
        samples.randomState ^= samples.randomState << 25;
        samples.randomState ^= samples.randomState >> 27;
        uint64_t random = samples.randomState * 0x2545f4914f6cdd1dull;
        uint64_t replacement = random % samples.sampleSequence;
        if (replacement < kYumiGCSubphaseSampleCapacity) {
            samples.durations[static_cast<size_t>(replacement)] = durationNanos;
        }
    }
    samples.maxDurationNanos = std::max(samples.maxDurationNanos, durationNanos);

}

static size_t
yumiGCSubphasePercentile(size_t subphase, size_t percentile) {
    if (!yumiGCValidSubphase(subphase)) {
        return 0;
    }

    std::vector<size_t> durations;
    {
        std::lock_guard<std::mutex> lock(gcObservabilityMutex);
        durations = gcSubphaseSamples[subphase].durations;
    }

    if (durations.empty()) {
        return 0;
    }

    std::sort(durations.begin(), durations.end());
    size_t index = ((durations.size() - 1) * percentile) / 100;
    return durations[index];
}

static bool
yumiGCValidPauseKind(size_t kind) {
    return kind > 0 && kind < YumiGCPauseKindCount;
}

static size_t
yumiGCPauseKindForHandshakeReason(size_t reason) {
    switch (reason) {
        case 1:
            return YumiGCPauseKindMarkSetupTotal;
        case 2:
            return YumiGCPauseKindTermConfirm;
        case 3:
            return YumiGCPauseKindSweepTotal;
        default:
            return 0;
    }
}

static void
yumiGCRecordPauseEvent(
    size_t kind,
    size_t cycleId,
    uint64_t startNanos,
    size_t durationNanos,
    size_t tasksWaited,
    size_t slowestTaskNanos
) {
    if (!yumiGCValidPauseKind(kind) || !durationNanos) {
        return;
    }

    gcPauseCount.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    size_t eventIndex = gcPauseEventSequence % kYumiGCPauseEventCapacity;
    gcPauseEvents[eventIndex] = YumiGCPauseEvent {
        cycleId,
        kind,
        startNanos,
        durationNanos,
        tasksWaited,
        slowestTaskNanos
    };
    ++gcPauseEventSequence;
    if (gcPauseEventStored < kYumiGCPauseEventCapacity) {
        ++gcPauseEventStored;
    }
}

static size_t
yumiGCPausePercentile(size_t kind, size_t percentile) {
    if (!yumiGCValidPauseKind(kind)) {
        return 0;
    }

    std::vector<size_t> durations;
    {
        std::lock_guard<std::mutex> lock(gcObservabilityMutex);
        durations.reserve(gcPauseEventStored);
        size_t firstSequence = gcPauseEventSequence - gcPauseEventStored;
        for (size_t index = 0; index < gcPauseEventStored; ++index) {
            const YumiGCPauseEvent& event =
                gcPauseEvents[(firstSequence + index) % kYumiGCPauseEventCapacity];
            if (event.kind == kind) {
                durations.push_back(event.durationNanos);
            }
        }
    }
    if (durations.empty()) {
        return 0;
    }

    std::sort(durations.begin(), durations.end());
    size_t index = ((durations.size() - 1) * percentile) / 100;
    return durations[index];
}

static size_t
yumiGCRemainingBatchBytes(const YumiGCThreadAllocationBatch& batch) {
    if (!batch.cursor || !batch.limit || batch.cursor >= batch.limit) {
        return 0;
    }
    return static_cast<size_t>(batch.limit - batch.cursor);
}

static void
yumiGCRecordBatchDiscard(size_t bytes) {
    yumiPublishGCThreadBatchConsumption();
    if (!bytes) {
        return;
    }
    gcBatchDiscardedBytes.fetch_add(bytes, std::memory_order_relaxed);
    gcStaleBatchDropCount.fetch_add(1, std::memory_order_relaxed);
    yumiGCSubtractAtomicSaturating(gcBatchCacheBytes, bytes);
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > bytes ? gcHotAllocationState.batchCacheBytes - bytes : 0;
}

static void
yumiDiscardCachedGCAllocationBatch(YumiGCThreadAllocationBatch& batch) {
    yumiPublishGCThreadBatchConsumption();
    size_t bytes = yumiGCRemainingBatchBytes(batch);
    if (bytes) {
        YumiGCThreadAllocationBatch discardedBatch = batch;
        bool discarded = yumiCallGCIgnoringPanic([&]() {
            return yumiGCDiscardUnusedBatch(
                discardedBatch.cursor,
                discardedBatch.limit,
                discardedBatch.slotSize,
                discardedBatch.generation);
        });
        if (!discarded) {
            yumiGCRecordBatchDiscard(bytes);
        }
    }
    batch = YumiGCThreadAllocationBatch {};
}

static void
yumiGCRecordAllocationStall(uint64_t startNanos, uint64_t endNanos) {
    size_t duration = static_cast<size_t>(endNanos - startNanos);
    gcAllocationStallTimeNanos.fetch_add(duration, std::memory_order_relaxed);
    yumiGCRecordSubphaseSample(
        YumiGCObservationAllocationStall,
        gcCurrentCycleId.load(std::memory_order_acquire),
        startNanos,
        duration,
        gcCycleAllocatedBytes.load(std::memory_order_relaxed));
    yumiGCRecordPauseEvent(
        YumiGCPauseKindStallReport,
        gcCurrentCycleId.load(std::memory_order_acquire),
        startNanos,
        duration,
        0,
        0);
}

static size_t
yumiGCEnvSize(const char* name, size_t fallback) {
    const char* value = std::getenv(name);
    if (!value || !*value) {
        return fallback;
    }

    char* end = nullptr;
    unsigned long long parsed = std::strtoull(value, &end, 10);
    if (!parsed || (end && *end)) {
        return fallback;
    }
    return (size_t)parsed;
}

static size_t
yumiGCEnvSizeAllowZero(const char* name, size_t fallback) {
    const char* value = std::getenv(name);
    if (!value || !*value) {
        return fallback;
    }

    char* end = nullptr;
    unsigned long long parsed = std::strtoull(value, &end, 10);
    if (end && *end) {
        return fallback;
    }
    return (size_t)parsed;
}

static size_t
yumiGCMultiplyDivideSaturating(size_t value, size_t multiplier, size_t divisor) {
    if (divisor == 0 || value == 0 || multiplier == 0) {
        return 0;
    }
    if (value > std::numeric_limits<size_t>::max() / multiplier) {
        return std::numeric_limits<size_t>::max();
    }
    return (value * multiplier) / divisor;
}

static size_t
yumiGCHeapHardCap() {
    if (yumiGCCollectionDisabled()) {
        return 0;
    }
    static const size_t hardCap = yumiGCEnvSizeAllowZero(
        "YUMI_GC_MEM_LIMIT",
        kYumiGCDefaultHeapHardCap);
    return hardCap;
}

static size_t
yumiGCApplyHeapHardCap(size_t goal) {
    size_t hardCap = yumiGCHeapHardCap();
    return hardCap ? std::min(goal, hardCap) : goal;
}

static std::atomic<size_t> gcProgramMinimumHeapGoal {0};

static size_t
yumiGCMinimumHeapGoal() {
    if (yumiGCCollectionDisabled()) {
        return std::numeric_limits<size_t>::max();
    }
    static const size_t configuredGoal = yumiGCEnvSize("YUMI_GC_MIN_HEAP_GOAL", 0);
    size_t goal = configuredGoal ? configuredGoal : gcProgramMinimumHeapGoal.load(std::memory_order_acquire);
    return yumiGCApplyHeapHardCap(goal ? goal : kYumiGCDefaultHeapGoal);
}

static size_t
yumiGCHeapMinGrowth() {
    static const size_t minGrowth =
        yumiGCEnvSize("YUMI_GC_HEAP_MIN_GROWTH", kYumiGCDefaultHeapMinGrowth);
    return minGrowth;
}

static size_t
yumiGCHeapGrowthPercent() {
    static const size_t growthPercent = yumiGCEnvSize(
        "YUMI_GC_GROWTH",
        kYumiGCDefaultHeapGrowthPercent);
    return growthPercent;
}

static size_t
yumiGCThreadBatchCacheByteCap() {
    static const size_t cap = yumiGCEnvSize(
        "YUMI_GC_THREAD_BATCH_CACHE_BYTES",
        kYumiGCDefaultThreadBatchCacheBytes);
    return cap;
}

static size_t
yumiGCGlobalBatchCacheByteCap() {
    static const size_t cap = yumiGCEnvSize(
        "YUMI_GC_GLOBAL_BATCH_CACHE_BYTES",
        kYumiGCDefaultGlobalBatchCacheBytes);
    return cap;
}

static bool
yumiGCBatchCacheCapAllows(size_t bytes) {
    yumiPublishGCThreadBatchConsumption();
    if (!bytes) {
        return true;
    }

    size_t threadCap = yumiGCThreadBatchCacheByteCap();
    if (threadCap == 0 || bytes > threadCap || gcHotAllocationState.batchCacheBytes > threadCap - bytes) {
        return false;
    }

    size_t globalCap = yumiGCGlobalBatchCacheByteCap();
    size_t globalBytes = gcBatchCacheBytes.load(std::memory_order_relaxed);
    if (globalCap == 0 || bytes > globalCap || globalBytes > globalCap - bytes) {
        return false;
    }

    return true;
}

static bool
yumiGCBatchCacheReserve(size_t bytes) {
    yumiPublishGCThreadBatchConsumption();
    if (!bytes) {
        return true;
    }

    size_t threadCap = yumiGCThreadBatchCacheByteCap();
    if (threadCap == 0 || bytes > threadCap || gcHotAllocationState.batchCacheBytes > threadCap - bytes) {
        return false;
    }

    size_t globalCap = yumiGCGlobalBatchCacheByteCap();
    if (globalCap == 0 || bytes > globalCap) {
        return false;
    }

    size_t globalBytes = gcBatchCacheBytes.load(std::memory_order_relaxed);
    while (true) {
        if (globalBytes > globalCap - bytes) {
            return false;
        }
        if (gcBatchCacheBytes.compare_exchange_weak(
                globalBytes,
                globalBytes + bytes,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            gcHotAllocationState.batchCacheBytes += bytes;
            gcHotAllocationState.batchPendingReservationBytes += bytes;
            return true;
        }
    }
}

static void
yumiGCBatchCacheCancelReservation(size_t bytes) {
    if (!bytes) {
        return;
    }
    size_t canceled = std::min(bytes, gcHotAllocationState.batchPendingReservationBytes);
    gcHotAllocationState.batchPendingReservationBytes -= canceled;
    yumiGCSubtractAtomicSaturating(gcBatchCacheBytes, canceled);
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > canceled
        ? gcHotAllocationState.batchCacheBytes - canceled
        : 0;
}

static bool
yumiGCEnvFlagEnabled(const char* name) {
    const char* value = std::getenv(name);
    return value && *value && std::strcmp(value, "0") != 0;
}

static size_t
yumiGCStressAllocationInterval() {
    static const size_t interval = yumiGCEnvFlagEnabled("YUMI_GC_STRESS")
        ? yumiGCEnvSize("YUMI_GC_STRESS", 1)
        : 0;
    return interval;
}

static size_t
yumiGCStressByteInterval() {
    static const size_t interval = yumiGCEnvSizeAllowZero("YUMI_GC_STRESS_BYTES", 0);
    return interval;
}

static uint64_t
yumiGCReplaySeed() {
    static const uint64_t seed = []() -> uint64_t {
        if (yumiGCEnvFlagEnabled("YUMI_GC_REPLAY_SEED")) {
            return static_cast<uint64_t>(yumiGCEnvSize("YUMI_GC_REPLAY_SEED", 1));
        }
        if (yumiGCEnvFlagEnabled("YUMI_GC_REPLAY")) {
            return static_cast<uint64_t>(yumiGCEnvSize("YUMI_GC_REPLAY", 1));
        }
        return 0;
    }();
    return seed;
}

static size_t
yumiGCReplayAllocationInterval() {
    static const size_t interval = yumiGCEnvSize("YUMI_GC_REPLAY_INTERVAL", 4);
    return interval;
}

static bool
yumiGCStressModeEnabled() {
    return yumiGCStressAllocationInterval() != 0 || yumiGCStressByteInterval() != 0;
}

static bool
yumiGCReplayModeEnabled() {
    return yumiGCReplaySeed() != 0;
}

static bool
yumiGCVerificationSlowPathsEnabled() {
    static const bool enabled = yumiGCStressModeEnabled() || yumiGCReplayModeEnabled();
    return enabled;
}

static size_t
yumiGCAutomaticSoftGoal(size_t goal) {
    size_t cachedGoal = gcPacerGoalCacheGoal.load(std::memory_order_acquire);
    size_t cachedSoftGoal = gcPacerGoalCacheSoftGoal.load(std::memory_order_acquire);
    if (cachedGoal == goal && cachedSoftGoal != 0) {
        return cachedSoftGoal;
    }

    size_t softGoal = yumiGCPacerSoftGoal(goal);
    gcPacerGoalCacheSoftGoal.store(softGoal, std::memory_order_release);
    gcPacerGoalCacheHardGoal.store(yumiGCAutomaticHardGoal(goal), std::memory_order_release);
    gcPacerGoalCacheGoal.store(goal, std::memory_order_release);
    return softGoal;
}

static size_t
yumiGCAutomaticHardGoal(size_t goal) {
    size_t cachedGoal = gcPacerGoalCacheGoal.load(std::memory_order_acquire);
    size_t cachedHardGoal = gcPacerGoalCacheHardGoal.load(std::memory_order_acquire);
    if (cachedGoal == goal && cachedHardGoal != 0) {
        return cachedHardGoal;
    }

    size_t hardGoal = yumiGCPacerHardGoal(goal);
    gcPacerGoalCacheHardGoal.store(hardGoal, std::memory_order_release);
    return hardGoal;
}

static size_t
yumiGCAutomaticAssistWork() {
    size_t throughput = gcPacerLastMarkThroughputItemsPerSecond.load(std::memory_order_relaxed);
    if (!throughput) {
        return kYumiGCAutomaticAssistWork;
    }
    size_t slice = yumiGCMultiplyDivideSaturating(
        throughput,
        kYumiGCMaxAssistSliceNanos,
        1000000000ull);
    return std::max<size_t>(1, std::min<size_t>(slice, kYumiGCAutomaticAssistWork));
}

static size_t
yumiGCAutomaticHardAssistWork() {
    return std::min<size_t>(
        kYumiGCAutomaticHardAssistWork,
        std::max<size_t>(1, yumiGCAutomaticAssistWork() * 2));
}

static size_t
yumiGCAutomaticIdleWork() {
    return kYumiGCAutomaticIdleWork;
}

static std::atomic<size_t> gcScavengedPageTotal {0};
static std::atomic<bool> gcMetadataTrimPending {false};
static std::atomic<uint64_t> gcIdleScavengePicosPerPage {0};

static size_t
yumiGCIdleScavengePageLimit(uint64_t budgetNanos, size_t debt, size_t minimum) {
    size_t limit = kYumiGCAutomaticIdleScavengePages;
    uint64_t picosPerPage = gcIdleScavengePicosPerPage.load(std::memory_order_relaxed);
    if (picosPerPage) {
        limit = static_cast<size_t>(std::min<uint64_t>(
            budgetNanos * 1000ull / picosPerPage,
            kYumiGCIdleScavengeMaximumPages));
    }
    return std::min(debt, std::max(limit, minimum));
}

static void
yumiGCRecordIdleScavengeCost(uint64_t elapsedNanos, size_t released) {
    uint64_t sample = std::max<uint64_t>(elapsedNanos * 1000ull / released, 1);
    uint64_t previous = gcIdleScavengePicosPerPage.load(std::memory_order_relaxed);
    gcIdleScavengePicosPerPage.store(
        previous ? previous - previous / 4 + sample / 4 : sample,
        std::memory_order_relaxed);
}

static size_t
yumiGCAutomaticTerminationQueueBudget() {
    return kYumiGCAutomaticTerminationQueueBudget;
}

static size_t
yumiGCAutomaticMaxActiveCycleNanos() {
    static const size_t maxActiveCycleNanos = yumiGCEnvSize(
        "YUMI_GC_MAX_ACTIVE_CYCLE_NANOS",
        kYumiGCAutomaticMaxActiveCycleNanos);
    return maxActiveCycleNanos;
}

static bool
yumiGCAllocationAssistDisabled() {
    static const bool disabled =
        yumiGCEnvFlagEnabled("YUMI_GC_DISABLE_AUTOMATIC_PROGRESS") ||
        yumiGCEnvFlagEnabled("YUMI_GC_DISABLE_ALLOCATION_ASSIST");
    return disabled;
}

static bool
yumiGCIdleProgressDisabled() {
    static const bool disabled =
        yumiGCEnvFlagEnabled("YUMI_GC_DISABLE_AUTOMATIC_PROGRESS") ||
        yumiGCEnvFlagEnabled("YUMI_GC_DISABLE_IDLE_PROGRESS");
    return disabled;
}

static bool
yumiGCBytesReachThreshold(size_t live, size_t upcomingSize, size_t threshold) {
    if (live >= threshold) {
        return true;
    }
    return upcomingSize >= threshold - live;
}

static size_t
yumiGCNativePageSize() {
    static const size_t pageSize = []() {
        long value = sysconf(_SC_PAGESIZE);
        return value > 0 ? static_cast<size_t>(value) : 4096ull;
    }();
    return pageSize;
}

static size_t
yumiGCPacerThroughput(size_t workItems, size_t durationNanos) {
    return yumiGCMultiplyDivideSaturating(workItems, 1000000000ull, durationNanos);
}

static size_t
yumiGCPacerUpdateEWMA(std::atomic<size_t>& target, size_t sample) {
    size_t previous = target.load(std::memory_order_relaxed);
    size_t next = previous
        ? yumiGCMultiplyDivideSaturating(previous, 7, 8) + sample / 8
        : sample;
    target.store(next, std::memory_order_release);
    return next;
}

static size_t
yumiGCPacerDebtBytes() {
    size_t sweepDebt = gcPacerLastSweepDebt.load(std::memory_order_relaxed);
    size_t scavengeDebt = gcPacerLastScavengeDebt.load(std::memory_order_relaxed);
    size_t debt = sweepDebt;
    if (debt > std::numeric_limits<size_t>::max() - scavengeDebt) {
        debt = std::numeric_limits<size_t>::max();
    } else {
        debt += scavengeDebt;
    }
    return yumiGCMultiplyDivideSaturating(debt, yumiGCNativePageSize(), 1);
}

static size_t
yumiGCPacerTriggerReserveBytes(size_t goal) {
    if (goal == 0) {
        return 0;
    }

    size_t allocationRate = gcPacerAllocationRateEWMA.load(std::memory_order_relaxed);
    size_t markNanos = gcPacerMarkDurationEWMA.load(std::memory_order_relaxed);
    size_t reserve = yumiGCMultiplyDivideSaturating(
        allocationRate,
        markNanos,
        1000000000ull);
    size_t fixedReserve = std::max<size_t>(yumiGCNativePageSize() * 16, goal / 64);
    reserve = std::min(std::numeric_limits<size_t>::max() - reserve, fixedReserve) + reserve;
    size_t correction = gcPacerLastGoalOvershootBytes.load(std::memory_order_relaxed) / 2;
    reserve = std::min(std::numeric_limits<size_t>::max() - reserve, correction) + reserve;
    size_t debtReserve = std::min(goal / 8, yumiGCPacerDebtBytes());
    reserve = std::min(std::numeric_limits<size_t>::max() - reserve, debtReserve) + reserve;
    size_t markedLive = gcPacerLastMarkedLiveBytes.load(std::memory_order_relaxed);
    size_t growthWindow = goal > markedLive ? goal - markedLive : 0;
    // Feedback may move collection earlier inside the newly granted growth window, but it
    // must not place the next trigger on the heap we just marked. Preserve half the window
    // for mutator progress; if marking needs more runway, bounded assists cover the deficit.
    size_t reserveLimit = markedLive ? growthWindow / 2 : goal / 2;
    return std::min(reserve, reserveLimit);
}

static size_t
yumiGCPacerSoftGoal(size_t goal) {
    size_t reserve = yumiGCPacerTriggerReserveBytes(goal);
    gcPacerLastTriggerReserveBytes.store(reserve, std::memory_order_release);
    if (reserve >= goal) {
        return 1;
    }
    return std::max<size_t>(1, goal - reserve);
}

static size_t
yumiGCPacerHardGoal(size_t goal) {
    return yumiGCApplyHeapHardCap(goal);
}

static void
yumiGCPacerInvalidateGoalCache() {
    gcPacerGoalCacheGoal.store(0, std::memory_order_release);
    gcPacerGoalCacheSoftGoal.store(0, std::memory_order_release);
    gcPacerGoalCacheHardGoal.store(0, std::memory_order_release);
}

static bool
yumiGCHardCapReached(size_t upcomingSize) {
    size_t hardCap = yumiGCHeapHardCap();
    if (hardCap == 0) {
        return false;
    }
    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    return yumiGCBytesReachThreshold(live, upcomingSize, hardCap);
}

static void
yumiGCPacerPublishCycleStats(size_t live, size_t goal) {
    yumiPublishGCThreadAllocationStats();
    uint64_t start = gcActiveCycleStartNanos.load(std::memory_order_acquire);
    size_t cycleNanos = start
        ? static_cast<size_t>(yumiGCMonotonicNanos() - start)
        : 0;
    size_t allocated = gcCycleAllocatedBytes.load(std::memory_order_acquire);
    size_t allocationRateSample = yumiGCMultiplyDivideSaturating(
        allocated,
        1000000000ull,
        cycleNanos);
    size_t allocationRate = yumiGCPacerUpdateEWMA(
        gcPacerAllocationRateEWMA,
        allocationRateSample);
    size_t markDuration = yumiGCPacerUpdateEWMA(
        gcPacerMarkDurationEWMA,
        gcCurrentCycleMarkDurationNanos.load(std::memory_order_acquire));
    (void)markDuration;
    size_t overshoot = live > goal ? live - goal : 0;
    gcPacerLastGoalOvershootBytes.store(overshoot, std::memory_order_release);

    gcPacerLastMarkedLiveBytes.store(live, std::memory_order_release);
    gcPacerLastHeapGoal.store(goal, std::memory_order_release);
    gcPacerLastCycleNanos.store(cycleNanos, std::memory_order_release);
    gcPacerLastAllocationRateBytesPerSecond.store(allocationRate, std::memory_order_release);
    gcPacerLastRootScanNanos.store(
        gcPacerCurrentRootScanNanos.load(std::memory_order_acquire),
        std::memory_order_release);
    gcPacerLastMarkThroughputItemsPerSecond.store(
        yumiGCPacerThroughput(
            gcPacerCurrentMarkWorkItems.load(std::memory_order_acquire),
            gcPacerCurrentMarkWorkNanos.load(std::memory_order_acquire)),
        std::memory_order_release);
    gcPacerLastAssistThroughputItemsPerSecond.store(
        yumiGCPacerThroughput(
            gcPacerCurrentAssistWorkItems.load(std::memory_order_acquire),
            gcPacerCurrentAssistWorkNanos.load(std::memory_order_acquire)),
        std::memory_order_release);
    gcPacerLastSweepDebt.store(
        yumiGCIncrementalSweepDebtPages(),
        std::memory_order_release);
    gcPacerLastScavengeDebt.store(yumiGCScavengeDebt(), std::memory_order_release);
    yumiGCPacerInvalidateGoalCache();
    gcPacerLastSoftGoal.store(yumiGCAutomaticSoftGoal(goal), std::memory_order_release);
    gcPacerLastHardGoal.store(yumiGCAutomaticHardGoal(goal), std::memory_order_release);
}

static size_t
yumiGCObjectAllocationSize(size_t size) {
    size_t minimum = std::max(size, kYumiGCObjectAlignment);
    if (minimum > std::numeric_limits<size_t>::max() - (kYumiGCObjectAlignment - 1)) {
        std::fprintf(stderr, "Yumi GC allocation size overflow\n");
        std::abort();
    }
    return (minimum + kYumiGCObjectAlignment - 1) & ~(kYumiGCObjectAlignment - 1);
}

static size_t
yumiGCAllocationBatchIndex(size_t allocSize) {
    if (allocSize <= 1024) {
        return (allocSize + 7) / 8;
    }

    return kYumiGCAllocationBatchSmallCount +
        (allocSize - 1025) / kYumiGCAllocationBatchLargeBucketSize;
}

static bool
yumiGCAllocationSiteKeyIsValid(const YumiGCAllocationSiteKey* key) {
    if (!key) {
        return false;
    }
    if (
        key->abiVersion != YumiGCAllocationABIVersion ||
        key->structSize != sizeof(YumiGCAllocationSiteKey)
    ) {
        return false;
    }
    if (key->pointerKind > kYumiGCAllocationPointerful ||
            key->zeroingPolicy != kYumiGCAllocationZeroFull) {
        return false;
    }

    size_t allocSize = yumiGCObjectAllocationSize(key->requestedSize);
    if (allocSize != key->allocatedSize || allocSize > kYumiGCAllocationBatchMaxSize) {
        return false;
    }

    size_t index = yumiGCAllocationBatchIndex(allocSize);
    return index == key->sizeClassIndex && index < kYumiGCAllocationBatchCount;
}

static bool
yumiGCInitAllocationSiteKey(
    YumiGCAllocationSiteKey* key,
    size_t requestedSize,
    void* descriptor,
    size_t pointerKind,
    size_t zeroingPolicy
) {
    if (!key || pointerKind > kYumiGCAllocationPointerful ||
            zeroingPolicy != kYumiGCAllocationZeroFull) {
        return false;
    }

    size_t allocSize = yumiGCObjectAllocationSize(requestedSize);
    if (allocSize > kYumiGCAllocationBatchMaxSize) {
        *key = YumiGCAllocationSiteKey {};
        key->abiVersion = YumiGCAllocationABIVersion;
        key->structSize = sizeof(YumiGCAllocationSiteKey);
        key->requestedSize = requestedSize;
        key->allocatedSize = allocSize;
        key->descriptor = descriptor;
        key->pointerKind = static_cast<unsigned char>(pointerKind);
        key->zeroingPolicy = static_cast<unsigned char>(zeroingPolicy);
        return false;
    }

    size_t index = yumiGCAllocationBatchIndex(allocSize);
    if (index >= kYumiGCAllocationBatchCount) {
        return false;
    }

    *key = YumiGCAllocationSiteKey {};
    key->abiVersion = YumiGCAllocationABIVersion;
    key->structSize = sizeof(YumiGCAllocationSiteKey);
    key->sizeClassIndex = index;
    key->requestedSize = requestedSize;
    key->allocatedSize = allocSize;
    key->descriptor = descriptor;
    key->pointerKind = static_cast<unsigned char>(pointerKind);
    key->zeroingPolicy = static_cast<unsigned char>(zeroingPolicy);
    return true;
}

static bool
yumiRefillCachedGCAllocationBatch(
    YumiGCThreadAllocationBatch& cached,
    size_t requestedSize,
    size_t allocSize,
    void* descriptor,
    size_t generation,
    size_t selectedCount = 0
) {
    if (cached.generation != generation) {
        yumiDiscardCachedGCAllocationBatch(cached);
    }

    YumiGCAllocationBatch batch {};
    size_t preferredCount = selectedCount
        ? selectedCount
        : yumiGCAllocationBatchPreferredCount(allocSize, descriptor);
    auto reserveBytesForCount = [&](size_t count) {
        return count > std::numeric_limits<size_t>::max() / allocSize
            ? std::numeric_limits<size_t>::max()
            : count * allocSize;
    };
    size_t reserveBytes = reserveBytesForCount(preferredCount);
    if (!yumiGCBatchCacheCapAllows(reserveBytes)) {
        gcBatchCachePressureFlushCount.fetch_add(1, std::memory_order_relaxed);
        yumiFlushGCAllocationBatches();
        while (preferredCount > 1 &&
                !yumiGCBatchCacheCapAllows(reserveBytes)) {
            preferredCount /= 2;
            reserveBytes = reserveBytesForCount(preferredCount);
        }
        if (!yumiGCBatchCacheCapAllows(reserveBytes)) {
            cached = YumiGCThreadAllocationBatch {};
            yumiGCRecordAllocationBatchMiss(YumiGCAllocationBatchMissCap);
            return false;
        }
    }

    bool allocated = yumiCallGCIgnoringPanic([&]() {
        return yumiGCActiveTaskCanUseFastAllocation()
            ? yumiGCAllocBatchForRunningTask(
                requestedSize, descriptor, preferredCount, &batch)
            : yumiGCAllocBatchForUnregisteredThread(
                requestedSize,
                descriptor,
                preferredCount,
                &batch);
    });
    if (!allocated) {
        cached = YumiGCThreadAllocationBatch {};
        return false;
    }

    cached.cursor = batch.cursor;
    cached.limit = batch.limit;
    cached.slotSize = batch.slotSize;
    cached.requestedSize = requestedSize;
    cached.descriptor = descriptor;
    cached.generation = generation;
    return true;
}

static void
yumiGCTraceSignalHandler(int signal, siginfo_t* info, void* context) {
    void* frames[96];
    int frameCount = backtrace(frames, 96);
    void* pc = nullptr;
    void* sp = nullptr;
    void* fp = nullptr;
    uint64_t x19 = 0;
    uint64_t x20 = 0;
    uint64_t x21 = 0;
    uint64_t x22 = 0;
    uint64_t x10 = 0;
    uint64_t x11 = 0;
    uint64_t x12 = 0;
#if defined(__aarch64__)
    if (context) {
        ucontext_t* ucontext = static_cast<ucontext_t*>(context);
#if defined(__APPLE__)
        const uint64_t* x = ucontext->uc_mcontext->__ss.__x;
        pc = reinterpret_cast<void*>(ucontext->uc_mcontext->__ss.__pc);
        sp = reinterpret_cast<void*>(ucontext->uc_mcontext->__ss.__sp);
        fp = reinterpret_cast<void*>(ucontext->uc_mcontext->__ss.__fp);
#else
        const uint64_t* x = reinterpret_cast<const uint64_t*>(ucontext->uc_mcontext.regs);
        pc = reinterpret_cast<void*>(ucontext->uc_mcontext.pc);
        sp = reinterpret_cast<void*>(ucontext->uc_mcontext.sp);
        fp = reinterpret_cast<void*>(ucontext->uc_mcontext.regs[29]);
#endif
        x19 = x[19];
        x20 = x[20];
        x21 = x[21];
        x22 = x[22];
        x10 = x[10];
        x11 = x[11];
        x12 = x[12];
    }
#endif
    Dl_info symbolInfo;
    std::memset(&symbolInfo, 0, sizeof(symbolInfo));
    if (pc) {
        dladdr(pc, &symbolInfo);
    }
    char header[512];
    int headerLength = std::snprintf(
        header,
        sizeof(header),
        "Yumi GC signal trace: signal=%d fault=%p pc=%p image=%p symbol=%s symbolAddr=%p sp=%p fp=%p x10=%#llx x11=%#llx x12=%#llx x19=%#llx x20=%#llx x21=%#llx x22=%#llx frames=%d\n",
        signal,
        info ? info->si_addr : nullptr,
        pc,
        symbolInfo.dli_fbase,
        symbolInfo.dli_sname ? symbolInfo.dli_sname : "",
        symbolInfo.dli_saddr,
        sp,
        fp,
        static_cast<unsigned long long>(x10),
        static_cast<unsigned long long>(x11),
        static_cast<unsigned long long>(x12),
        static_cast<unsigned long long>(x19),
        static_cast<unsigned long long>(x20),
        static_cast<unsigned long long>(x21),
        static_cast<unsigned long long>(x22),
        frameCount);
    if (headerLength > 0) {
        write(STDERR_FILENO, header, static_cast<size_t>(headerLength));
    }

#if defined(__aarch64__)
    uintptr_t framePointer = reinterpret_cast<uintptr_t>(fp);
    for (int index = 0; index < 24 && framePointer; ++index) {
        uintptr_t* frame = reinterpret_cast<uintptr_t*>(framePointer);
        uintptr_t nextFrame = frame[0];
        void* returnAddress = reinterpret_cast<void*>(frame[1]);
        uintptr_t word2 = frame[2];
        uintptr_t word3 = frame[3];
        uintptr_t word4 = frame[4];
        uintptr_t word5 = frame[5];
        uintptr_t word6 = frame[6];
        uintptr_t word7 = frame[7];
        Dl_info frameInfo;
        std::memset(&frameInfo, 0, sizeof(frameInfo));
        dladdr(returnAddress, &frameInfo);
        char line[768];
        int lineLength = std::snprintf(
            line,
            sizeof(line),
            "Yumi GC signal frame %d: fp=%p ra=%p symbol=%s symbolAddr=%p image=%p w2=%#llx w3=%#llx w4=%#llx w5=%#llx w6=%#llx w7=%#llx\n",
            index,
            reinterpret_cast<void*>(framePointer),
            returnAddress,
            frameInfo.dli_sname ? frameInfo.dli_sname : "",
            frameInfo.dli_saddr,
            frameInfo.dli_fbase,
            static_cast<unsigned long long>(word2),
            static_cast<unsigned long long>(word3),
            static_cast<unsigned long long>(word4),
            static_cast<unsigned long long>(word5),
            static_cast<unsigned long long>(word6),
            static_cast<unsigned long long>(word7));
        if (lineLength > 0) {
            write(STDERR_FILENO, line, static_cast<size_t>(lineLength));
        }
        if (nextFrame <= framePointer) {
            break;
        }
        framePointer = nextFrame;
    }
#endif

    backtrace_symbols_fd(frames, frameCount, STDERR_FILENO);
    _exit(128 + signal);
}

static void
yumiGCInstallSignalTraceIfRequested() {
    if (!std::getenv("YUMI_GC_TRACE_SIGNALS")) {
        return;
    }

    struct sigaction action;
    std::memset(&action, 0, sizeof(action));
    action.sa_sigaction = yumiGCTraceSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
}

#if defined(__APPLE__)
static void
yumiGCNoteMemoryPressure(void*) {
    gcMemoryPressurePending.store(true, std::memory_order_release);
}
#endif

static void
yumiGCWatchMemoryPressure() {
#if defined(__APPLE__)
    dispatch_source_t source = dispatch_source_create(
        DISPATCH_SOURCE_TYPE_MEMORYPRESSURE,
        0,
        DISPATCH_MEMORYPRESSURE_WARN | DISPATCH_MEMORYPRESSURE_CRITICAL,
        dispatch_get_global_queue(QOS_CLASS_UTILITY, 0));
    if (!source) {
        return;
    }
    dispatch_source_set_event_handler_f(source, yumiGCNoteMemoryPressure);
    dispatch_resume(source);
#endif
}

static void
yumiEnsureGCInitializedSlow() {
    std::call_once(gcInitOnce, []() {
        yumiGCInstallSignalTraceIfRequested();
        size_t expected = 0;
        gcHeapGoal.compare_exchange_strong(
            expected,
            yumiGCMinimumHeapGoal(),
            std::memory_order_release,
            std::memory_order_relaxed);
        yumiCallGCIgnoringPanic([]() {
            yumiGCInit();
        });
        gcInitialized.store(true, std::memory_order_release);
        yumiGCWatchMemoryPressure();
    });
}

static void
yumiEnsureGCInitialized() {
    if (!gcInitialized.load(std::memory_order_acquire)) {
        yumiEnsureGCInitializedSlow();
    }
}

static void
yumiRecordGCPublicAllocation(
    YumiGCHotAllocationState& hot,
    size_t requestedSize,
    size_t allocatedSize
) {
    hot.stats.allocatedObjects += 1;
    hot.stats.requestedBytes += requestedSize;
    hot.stats.allocatedBytes += allocatedSize;
    yumiGCRecordAllocationPhaseCount(hot, 1);
    if (yumiGCLoadPublicPhase() != 0) {
        hot.stats.cycleAllocatedBytes += allocatedSize;
    }
    yumiGCBenchRecordAllocationSizes(1, requestedSize);
    yumiGCBenchRecordAllocatedSizeClasses(1, allocatedSize);
#if YU_ENABLE_BENCH_METRICS
    if (yumiBenchMetricsEnabledFlag) {
        gcBenchAllocationSites[0].fetch_add(1, std::memory_order_relaxed);
    }
#endif
    if (hot.stats.allocatedObjects >= kYumiGCThreadStatFlushObjectCount) {
        // Periodic accounting must not revoke ranges: ownership changes only at collector,
        // cache, or task boundaries. Publish consumed prefixes and retain every unused tail.
        yumiPublishGCCompilerAllocationBufferConsumptions();
    }
}

static void
yumiRecordGCPublicAllocations(size_t count, size_t requestedSize, size_t allocatedSize) {
    if (!count) {
        return;
    }
    gcHotAllocationState.stats.allocatedObjects += count;
    gcHotAllocationState.stats.requestedBytes += requestedSize * count;
    gcHotAllocationState.stats.allocatedBytes += allocatedSize * count;
    yumiGCRecordAllocationPhaseCount(gcHotAllocationState, count);
    if (yumiGCLoadPublicPhase() != 0) {
        gcHotAllocationState.stats.cycleAllocatedBytes += allocatedSize * count;
    }
    yumiGCBenchRecordAllocationSizes(count, requestedSize);
    yumiGCBenchRecordAllocatedSizeClasses(count, allocatedSize);
}

static void
yumiRecordGCHeapAllocation(size_t count, size_t allocatedSize) {
    gcLiveObjects.fetch_add(count, std::memory_order_relaxed);
    gcLiveBytes.fetch_add(count * allocatedSize, std::memory_order_relaxed);
}

static void
yumiRecordGCAllocation(size_t requestedSize, size_t allocatedSize) {
    yumiRecordGCPublicAllocation(gcHotAllocationState, requestedSize, allocatedSize);
    yumiRecordGCHeapAllocation(1, allocatedSize);
}

static void
yumiGCSafepointIfRequested() {
    uint64_t control = yumiGCLoadControlWord();
    if (!yumiGCControlWordHandshakeRequested(control) ||
            yumiGCActiveTaskAcknowledgedHandshake(control)) {
        return;
    }

    yumiPublishGCThreadAllocationStats();
    yumiCallIgnoringPanic([]() {
        yumiGCSafepoint();
    });
}

static bool
yumiGCShouldStartAutomaticCycle(size_t upcomingSize) {
    if (yumiGCCollectionDisabled()) {
        return false;
    }
    if (!gcInitialized.load(std::memory_order_acquire)) {
        return false;
    }
    if (yumiGCLoadPublicPhase() != kYumiGCPhaseIdle) {
        return false;
    }
    if (gcMemoryPressurePending.load(std::memory_order_acquire)) {
        return true;
    }

    size_t goal = gcHeapGoal.load(std::memory_order_acquire);
    if (goal == 0) {
        goal = yumiGCMinimumHeapGoal();
        gcHeapGoal.store(goal, std::memory_order_release);
    }

    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    return yumiGCBytesReachThreshold(live, upcomingSize, yumiGCAutomaticSoftGoal(goal));
}

static bool
yumiGCHardTriggerReached(size_t upcomingSize) {
    if (yumiGCCollectionDisabled()) {
        return false;
    }
    if (!gcInitialized.load(std::memory_order_acquire)) {
        return false;
    }

    size_t goal = gcHeapGoal.load(std::memory_order_acquire);
    if (goal == 0) {
        goal = yumiGCMinimumHeapGoal();
        gcHeapGoal.store(goal, std::memory_order_release);
    }

    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    return yumiGCBytesReachThreshold(live, upcomingSize, yumiGCAutomaticHardGoal(goal));
}

static void
yumiGCEndCollection() {
    gcCollectionActive.store(false, std::memory_order_release);
}

static bool
yumiGCTryBeginCollectionWork() {
    bool expected = false;
    return gcCollectionActive.compare_exchange_strong(
        expected,
        true,
        std::memory_order_acq_rel,
        std::memory_order_acquire
    );
}

static bool
yumiGCTryBeginCollection() {
    if (!yumiGCTryBeginCollectionWork()) {
        return false;
    }

    if (yumiGCLoadPublicPhase() != kYumiGCPhaseIdle) {
        yumiGCEndCollection();
        return false;
    }

    return true;
}

static void
yumiGCFinishAutomaticGrowthWindowIfIdle() {
    if (yumiGCLoadPublicPhase() == kYumiGCPhaseIdle) {
        gcAutomaticCycleActive.store(false, std::memory_order_release);
        gcCollectionMayGrowRequestedGoal.store(false, std::memory_order_release);
    }
}

static void
yumiGCRecordAutomaticFallback(size_t reason) {
    size_t triggerReason = reason == YumiGCAutomaticFallbackHardCap
        ? YumiGCPacerTriggerHardCap
        : YumiGCPacerTriggerHardGoal;
    gcPacerLastTriggerReason.store(triggerReason, std::memory_order_release);
    gcPendingCycleTriggerReason.store(triggerReason, std::memory_order_release);
    yumiGCRuntimeRecordAutomaticFallback(reason);
}

static void
yumiGCRunAutomaticFallbackCollection(size_t reason, bool recordAllocationStall) {
    if (yumiGCThreadHasLocalCallbackRegistration()) {
        return;
    }
    yumiFlushGCAllocationBatches();
    if (!yumiGCTryBeginCollection()) {
        return;
    }

    yumiPublishGCThreadAllocationStats();
    gcCollectionMayGrowRequestedGoal.store(true, std::memory_order_release);
    yumiGCRecordAutomaticFallback(reason);

    uint64_t start = 0;
    if (recordAllocationStall) {
        start = yumiGCMonotonicNanos();
        gcAllocationStallCount.fetch_add(1, std::memory_order_relaxed);
    }

    yumiCallGCIgnoringPanic([]() {
        yumiGCCollectForAllocation();
        yumiGCDrainAutomaticCycleBlocking();
    });

    if (recordAllocationStall) {
        yumiGCRecordAllocationStall(start, yumiGCMonotonicNanos());
    }
    yumiGCFinishAutomaticGrowthWindowIfIdle();
    yumiGCEndCollection();
}

static bool
yumiGCStartAutomaticCycle(bool recordAllocationStall, size_t triggerReason) {
    if (!yumiGCTryBeginCollection()) {
        return false;
    }
    gcMemoryPressurePending.store(false, std::memory_order_release);

    yumiPublishGCThreadAllocationStats();
    gcCollectionMayGrowRequestedGoal.store(true, std::memory_order_release);
    gcPacerLastTriggerReason.store(triggerReason, std::memory_order_release);
    gcPendingCycleTriggerReason.store(triggerReason, std::memory_order_release);

    uint64_t start = 0;
    if (recordAllocationStall) {
        start = yumiGCMonotonicNanos();
        gcAllocationStallCount.fetch_add(1, std::memory_order_relaxed);
    }

    yumiCallGCIgnoringPanic([]() {
        yumiGCStartConcurrentMark();
    });

    if (recordAllocationStall) {
        yumiGCRecordAllocationStall(start, yumiGCMonotonicNanos());
    }

    bool active = yumiGCLoadPublicPhase() != kYumiGCPhaseIdle;
    if (!active) {
        gcPendingCycleTriggerReason.store(0, std::memory_order_release);
    }
    gcAutomaticCycleActive.store(active, std::memory_order_release);
    yumiGCFinishAutomaticGrowthWindowIfIdle();
    yumiGCEndCollection();
    if (active) {
        yumiGCStartBackgroundWorkers();
    }
    return active;
}

static bool
yumiGCCompleteAutomaticCycle(size_t fallbackReason) {
    if (!yumiGCTryBeginCollectionWork()) {
        return false;
    }

    if (fallbackReason != YumiGCAutomaticFallbackNone) {
        yumiGCRecordAutomaticFallback(fallbackReason);
    }

    yumiCallGCIgnoringPanic([&]() {
        if (fallbackReason == YumiGCAutomaticFallbackNoProgressDriver) {
            yumiGCDrainAutomaticCycleBlocking();
        } else {
            yumiGCFinishAutomaticCycle();
        }
    });

    yumiGCFinishAutomaticGrowthWindowIfIdle();
    yumiGCEndCollection();
    return yumiGCLoadPublicPhase() == kYumiGCPhaseIdle;
}

static bool
yumiGCAutomaticCycleExpired() {
    size_t maxAge = yumiGCAutomaticMaxActiveCycleNanos();
    return maxAge != 0 && yumiGCRuntimeActiveCycleAgeNanos() > maxAge;
}

static size_t
yumiGCDrainAutomaticCycleBlocking() {
    if (yumiGCThreadHasLocalCallbackRegistration()) {
        return 0;
    }
    while (yumiGCLoadPublicPhase() != kYumiGCPhaseIdle) {
        unsigned char phase = yumiGCLoadPublicPhase();
        if (phase == kYumiGCPhaseSweep) {
            yumiGCStepIncrementalSweep(yumiGCAutomaticHardAssistWork());
        } else {
            yumiGCFinishAutomaticCycle();
        }
    }
    return 0;
}

static bool
yumiGCBudgetHasTime(uint64_t deadlineNanos) {
    uint64_t now = yumiGCMonotonicNanos();
    return now < deadlineNanos && deadlineNanos - now > kYumiGCBudgetGuardNanos;
}

enum YumiGCIdleSettle {
    YumiGCIdleSettleNone,
    YumiGCIdleSettleWaiting,
    YumiGCIdleSettleDue
};

static YumiGCIdleSettle
yumiGCIdleSettleState(uint64_t nowNanos) {
    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    size_t settled = gcIdleSettledLiveBytes.load(std::memory_order_acquire);
    size_t slack = std::max(settled / 4, kYumiGCIdleSettleMinimumBytes);
    if (live <= settled || live - settled < slack) {
        return YumiGCIdleSettleNone;
    }
    if (gcIdleSettleSeenLiveBytes.exchange(live, std::memory_order_acq_rel) != live) {
        gcIdleSettleSeenNanos.store(nowNanos, std::memory_order_release);
        return YumiGCIdleSettleWaiting;
    }
    uint64_t seen = gcIdleSettleSeenNanos.load(std::memory_order_acquire);
    return nowNanos - seen < kYumiGCIdleSettleQuietNanos
        ? YumiGCIdleSettleWaiting
        : YumiGCIdleSettleDue;
}

static bool
yumiGCIdleScavengeIfNeeded(uint64_t deadlineNanos, bool trimsMetadata, bool* pending = nullptr) {
    if (!gcInitialized.load(std::memory_order_acquire) ||
            yumiGCLoadPublicPhase() != kYumiGCPhaseIdle ||
            !yumiGCBudgetHasTime(deadlineNanos)) {
        return false;
    }
    bool wantsTrim = trimsMetadata && gcMetadataTrimPending.load(std::memory_order_acquire);
    if (yumiGCScavengeDebt() == 0 && !wantsTrim) {
        return false;
    }
    if (!yumiGCTryBeginCollectionWork()) {
        return false;
    }

    yumiFlushGCAllocationBatches();
    bool remains = false;
    yumiCallGCIgnoringPanic([&]() {
        size_t debt = yumiGCScavengeDebt();
        if (!debt) {
            if (trimsMetadata && gcMetadataTrimPending.exchange(false, std::memory_order_acq_rel)) {
                yumiGCTrimMetadataCaches();
            }
            return;
        }
        uint64_t startNanos = yumiGCMonotonicNanos();
        uint64_t remainingNanos = deadlineNanos > startNanos ? deadlineNanos - startNanos : 0;
        size_t minimum = yumiGCScavengeMinimumStepPages();
        size_t pageLimit = remainingNanos <= kYumiGCBudgetGuardNanos * 4
            ? minimum
            : yumiGCIdleScavengePageLimit(
                remainingNanos - kYumiGCBudgetGuardNanos,
                debt,
                minimum);
        size_t releasedBefore = gcScavengedPageTotal.load(std::memory_order_relaxed);
        yumiGCStepScavenge(pageLimit);
        size_t released = gcScavengedPageTotal.load(std::memory_order_relaxed) - releasedBefore;
        if (released) {
            yumiGCRecordIdleScavengeCost(yumiGCMonotonicNanos() - startNanos, released);
            remains = yumiGCScavengeDebt() != 0;
        }
        if (!remains && trimsMetadata &&
                gcMetadataTrimPending.exchange(false, std::memory_order_acq_rel)) {
            yumiGCTrimMetadataCaches();
        }
    });
    yumiGCEndCollection();
    if (pending) {
        *pending = remains;
    }
    return true;
}

static size_t
yumiGCProgressAutomaticCycle(size_t workLimit, bool assist, bool bounded = false) {
    unsigned char phase = yumiGCLoadPublicPhase();
    if (phase == kYumiGCPhaseIdle) {
        return 0;
    }

    if (phase == kYumiGCPhaseMarkComplete) {
        yumiGCCompleteAutomaticCycle(YumiGCAutomaticFallbackNone);
        return 0;
    }

    if (phase == kYumiGCPhaseSweep) {
        if (workLimit == 0) {
            return 0;
        }
        if (!yumiGCTryBeginCollectionWork()) {
            return 0;
        }
        yumiCallGCIgnoringPanic([&]() {
            size_t sweepWorkLimit =
                (!assist && !bounded && !yumiGCIncrementalSweepHasDebt())
                ? std::numeric_limits<size_t>::max()
                : workLimit;
            yumiGCStepIncrementalSweep(sweepWorkLimit);
        });
        yumiGCFinishAutomaticGrowthWindowIfIdle();
        yumiGCEndCollection();
        return 0;
    }

    if (yumiGCAutomaticCycleExpired()) {
        yumiGCCompleteAutomaticCycle(YumiGCAutomaticFallbackMaxActiveAge);
        return 0;
    }

    if (phase != kYumiGCPhaseConcurrentMark || workLimit == 0) {
        return 0;
    }

    if (!yumiGCTryBeginCollectionWork()) {
        return 0;
    }

    size_t drained = 0;
    yumiCallGCIgnoringPanic([&]() {
        if (assist) {
            drained = yumiGCMutatorAssist(workLimit);
        } else {
            size_t remaining = workLimit;
            do {
                drained += yumiGCStepConcurrentMark();
                if (remaining <= kYumiGCAutomaticStepWork) {
                    break;
                }
                remaining -= kYumiGCAutomaticStepWork;
            } while (
                yumiGCLoadPublicPhase() == kYumiGCPhaseConcurrentMark &&
                yumiGCRuntimeMarkQueueDepth() != 0);
        }
    });

    bool leavesTermination =
        assist && gcBackgroundWorkerActiveCount.load(std::memory_order_acquire) != 0;
    bool shouldFinish = !leavesTermination && (
        yumiGCLoadPublicPhase() == kYumiGCPhaseMarkComplete ||
        (
            yumiGCLoadPublicPhase() == kYumiGCPhaseConcurrentMark &&
            yumiGCRuntimeMarkQueueDepth() <= yumiGCAutomaticTerminationQueueBudget()
        ));
    if (shouldFinish) {
        yumiCallGCIgnoringPanic([]() {
            yumiGCFinishAutomaticCycle();
        });
        yumiGCFinishAutomaticGrowthWindowIfIdle();
    }

    yumiGCEndCollection();
    return drained;
}

static bool
yumiGCProgressWithinDeadline(uint64_t deadlineNanos, size_t preferredWork) {
    bool attempted = false;
    while (yumiGCLoadPublicPhase() != kYumiGCPhaseIdle &&
            yumiGCBudgetHasTime(deadlineNanos)) {
        if (gcCollectionActive.load(std::memory_order_acquire)) {
            break;
        }
        uint64_t remainingNanos = deadlineNanos - yumiGCMonotonicNanos();
        size_t workLimit = remainingNanos <= kYumiGCBudgetGuardNanos * 4
            ? 1
            : preferredWork;
        yumiGCProgressAutomaticCycle(workLimit, false, true);
        attempted = true;
    }
    return attempted;
}

static size_t
yumiGCGetActiveTaskAssistDebt() {
    size_t cycle = gcCurrentCycleId.load(std::memory_order_acquire);
    if (yumiGCActiveTaskAssistDebt && yumiGCActiveTaskAssistDebtCycle) {
        if (yumiGCActiveTaskAssistDebtCycle() != cycle) {
            yumiGCSetActiveTaskAssistDebt(0);
            yumiGCSetActiveTaskAssistDebtCycle(cycle);
            return 0;
        }
        return yumiGCActiveTaskAssistDebt();
    }
    if (gcFallbackActiveTaskAssistDebtCycle != cycle) {
        gcFallbackActiveTaskAssistDebt = 0;
        gcFallbackActiveTaskAssistDebtCycle = cycle;
    }
    return gcFallbackActiveTaskAssistDebt;
}

static void
yumiGCSetTaskAssistDebt(size_t debt) {
    if (yumiGCSetActiveTaskAssistDebt) {
        yumiGCSetActiveTaskAssistDebt(debt);
        if (yumiGCSetActiveTaskAssistDebtCycle) {
            yumiGCSetActiveTaskAssistDebtCycle(
                gcCurrentCycleId.load(std::memory_order_acquire));
        }
    } else {
        gcFallbackActiveTaskAssistDebt = debt;
        gcFallbackActiveTaskAssistDebtCycle =
            gcCurrentCycleId.load(std::memory_order_acquire);
    }
}

static size_t
yumiGCTakeBackgroundCredit(size_t requested) {
    size_t available = gcBackgroundCredit.load(std::memory_order_relaxed);
    while (available) {
        size_t taken = std::min(available, requested);
        if (gcBackgroundCredit.compare_exchange_weak(
                available,
                available - taken,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return taken;
        }
    }
    return 0;
}

static size_t
yumiGCAccrueAssistDebt(size_t allocatedBytes) {
    size_t queueDepth = yumiGCRuntimeMarkQueueDepth();
    if (!queueDepth || !allocatedBytes) {
        return yumiGCGetActiveTaskAssistDebt();
    }
    size_t heapBytes = gcLiveBytes.load(std::memory_order_relaxed);
    size_t heapGoal = gcHeapGoal.load(std::memory_order_acquire);
    size_t allocationRoom = heapGoal > heapBytes ? heapGoal - heapBytes : 1;
    size_t accrued = yumiGCMultiplyDivideSaturating(
        allocatedBytes,
        queueDepth,
        allocationRoom);
    if (!accrued) {
        accrued = 1;
    }
    size_t debt = yumiGCGetActiveTaskAssistDebt();
    size_t next = debt > std::numeric_limits<size_t>::max() - accrued
        ? std::numeric_limits<size_t>::max()
        : debt + accrued;
    gcAssistDebt.fetch_add(next - debt, std::memory_order_relaxed);

    size_t credited = yumiGCTakeBackgroundCredit(next);
    next -= credited;
    yumiGCSubtractAtomicSaturating(gcAssistDebt, credited);
    yumiGCSetTaskAssistDebt(next);
    return next;
}

static void
yumiGCBackgroundWorkerSleep(size_t delayNanos) {
    if (!delayNanos) {
        return;
    }
    struct timespec delay = {
        static_cast<time_t>(delayNanos / 1000000000ull),
        static_cast<long>(delayNanos % 1000000000ull)
    };
    yumiGCEnterRuntimeBlock();
    nanosleep(&delay, nullptr);
    yumiGCLeaveRuntimeBlock();
}

static void
yumiGCBackgroundMarkWorker(void* closure) {
    size_t token = reinterpret_cast<size_t>(closure);
    size_t generation = token >> 8;
    size_t workerIndex = (token & 0xff) - 1;
    gcBackgroundWorkerActiveCount.fetch_add(1, std::memory_order_relaxed);

    while (
        generation == gcBackgroundWorkerGeneration.load(std::memory_order_acquire) &&
        gcAutomaticCycleActive.load(std::memory_order_acquire)
    ) {
        yumiGCSafepointIfRequested();
        unsigned char phase = yumiGCLoadPublicPhase();
        if (phase == kYumiGCPhaseIdle) {
            break;
        }
        if (phase != kYumiGCPhaseConcurrentMark) {
            if (workerIndex == 0) {
                yumiGCProgressAutomaticCycle(kYumiGCBackgroundWorkerStepWork, false);
            } else {
                break;
            }
            continue;
        }

        uint64_t start = yumiGCMonotonicNanos();
        size_t drained = 0;
        yumiCallGCIgnoringPanic([&]() {
            drained = yumiGCStepConcurrentMark();
        });
        size_t elapsed = static_cast<size_t>(yumiGCMonotonicNanos() - start);
        gcBackgroundWorkerCpuNanos.fetch_add(elapsed, std::memory_order_relaxed);
        if (drained) {
            gcBackgroundWorkerWork.fetch_add(drained, std::memory_order_relaxed);
            gcBackgroundCredit.fetch_add(drained, std::memory_order_relaxed);
        }

        if (
            workerIndex == 0 &&
            (yumiGCLoadPublicPhase() == kYumiGCPhaseMarkComplete ||
                yumiGCRuntimeMarkQueueDepth() <= yumiGCAutomaticTerminationQueueBudget())
        ) {
            yumiGCProgressAutomaticCycle(1, false);
        }

        size_t duty = gcBackgroundWorkerDutyPermille.load(std::memory_order_relaxed);
        size_t delayNanos = duty && duty < kYumiGCBackgroundDutyDenominator
            ? yumiGCMultiplyDivideSaturating(
                elapsed,
                kYumiGCBackgroundDutyDenominator - duty,
                duty)
            : 0;
        if (!drained && !delayNanos) {
            delayNanos = kYumiGCMaxAssistSliceNanos;
        }
        yumiGCBackgroundWorkerSleep(delayNanos);
    }

    gcBackgroundWorkerActiveCount.fetch_sub(1, std::memory_order_relaxed);
    // YuTask entrypoints are stackful scheduler contexts, not ordinary pthread routines.
    // They must hand control back through the task trampoline instead of returning.
    yumiTerminateActiveTask();
}

static void
yumiGCStartBackgroundWorkers() {
    if (yumiGCReplayModeEnabled() ||
            yumiGCEnvFlagEnabled("YUMI_GC_DISABLE_AUTOMATIC_PROGRESS")) {
        return;
    }
    if (!yumiGCBackgroundTaskQueue || !yumiGCBackgroundTaskQueueThreadCount ||
            !yumiForkGCMarkTaskInWorker) {
        return;
    }
    int schedulerThreads = yumiGCBackgroundTaskQueueThreadCount();
    YuTaskQueue* queue = yumiGCBackgroundTaskQueue();
    if (schedulerThreads <= 0 || !queue) {
        return;
    }
    size_t workerCount = std::max<size_t>(1, static_cast<size_t>(schedulerThreads) / 4);
    size_t duty = std::min<size_t>(
        kYumiGCBackgroundDutyDenominator,
        (static_cast<size_t>(schedulerThreads) * 250) / workerCount);

    gcBackgroundWorkerCount.store(workerCount, std::memory_order_release);
    gcBackgroundWorkerThreadCapacity.store(
        static_cast<size_t>(schedulerThreads),
        std::memory_order_release);
    gcBackgroundWorkerDutyPermille.store(duty, std::memory_order_release);
    size_t generation = gcBackgroundWorkerGeneration.fetch_add(1, std::memory_order_acq_rel) + 1;
    for (size_t index = 0; index < workerCount; ++index) {
        size_t token = (generation << 8) | (index + 1);
        yumiForkGCMarkTaskInWorker(
            queue,
            yumiGCBackgroundMarkWorker,
            reinterpret_cast<void*>(token));
    }
}

static void
yumiGCAllocationAssistIfNeeded(size_t upcomingSize) {
    unsigned char phase = yumiGCLoadPublicPhase();
    if (phase == kYumiGCPhaseIdle) {
        return;
    }
    if (!gcAutomaticCycleActive.load(std::memory_order_acquire)) {
        return;
    }

    if (yumiGCHardCapReached(upcomingSize)) {
        yumiGCCompleteAutomaticCycle(YumiGCAutomaticFallbackHardCap);
        return;
    }

    if (phase != kYumiGCPhaseConcurrentMark) {
        return;
    }

    gcHotAllocationState.stats.allocationAssistCheckCount += 1;
    bool hardTrigger = yumiGCHardTriggerReached(upcomingSize);
    size_t debt = yumiGCAccrueAssistDebt(upcomingSize);
    if (!debt) {
        return;
    }

    if (hardTrigger && yumiGCAllocationAssistDisabled()) {
        yumiGCCompleteAutomaticCycle(YumiGCAutomaticFallbackNoProgressDriver);
        return;
    }

    if (yumiGCAllocationAssistDisabled()) {
        return;
    }

    size_t workLimit = hardTrigger
        ? yumiGCAutomaticHardAssistWork()
        : yumiGCAutomaticAssistWork();
    workLimit = std::min(workLimit, debt);
    gcHotAllocationState.stats.allocationAssistCallCount += 1;
    size_t drained = yumiGCProgressAutomaticCycle(workLimit, true);
    size_t remaining = debt > drained ? debt - drained : 0;
    yumiGCSubtractAtomicSaturating(gcAssistDebt, debt - remaining);
    yumiGCSetTaskAssistDebt(remaining);
}

struct YumiGCVerificationDriverState {
    size_t allocationsSinceCycle = 0;
    size_t bytesSinceCycle = 0;
    size_t replayAllocations = 0;
    uint64_t replayState = 0;
};

static YumiGCVerificationDriverState&
yumiGCVerificationDriverState() {
    static thread_local YumiGCVerificationDriverState state;
    return state;
}

static bool
yumiGCRunForcedFullCycle() {
    if (yumiGCThreadHasLocalCallbackRegistration()) {
        return false;
    }
    yumiFlushGCAllocationBatches();
    if (!yumiGCTryBeginCollectionWork()) {
        return false;
    }

    yumiPublishGCThreadAllocationStats();
    yumiCallGCIgnoringPanic([]() {
        if (yumiGCLoadPublicPhase() == kYumiGCPhaseIdle) {
            yumiGCCollectForAllocation();
        } else {
            yumiGCDrainAutomaticCycleBlocking();
        }
    });
    yumiGCFinishAutomaticGrowthWindowIfIdle();
    bool complete = yumiGCLoadPublicPhase() == kYumiGCPhaseIdle;
    yumiGCEndCollection();
    return complete;
}

static void
yumiGCStressBeforeAllocation(size_t upcomingSize) {
    YumiGCVerificationDriverState& state = yumiGCVerificationDriverState();
    if (state.allocationsSinceCycle != std::numeric_limits<size_t>::max()) {
        ++state.allocationsSinceCycle;
    }
    state.bytesSinceCycle = upcomingSize > std::numeric_limits<size_t>::max() - state.bytesSinceCycle
        ? std::numeric_limits<size_t>::max()
        : state.bytesSinceCycle + upcomingSize;

    size_t allocationInterval = yumiGCStressAllocationInterval();
    size_t byteInterval = yumiGCStressByteInterval();
    bool allocationDue = allocationInterval != 0 &&
        state.allocationsSinceCycle >= allocationInterval;
    bool bytesDue = byteInterval != 0 && state.bytesSinceCycle >= byteInterval;
    if ((!allocationDue && !bytesDue) || !yumiGCRunForcedFullCycle()) {
        return;
    }

    state.allocationsSinceCycle = allocationInterval
        ? state.allocationsSinceCycle % allocationInterval
        : 0;
    state.bytesSinceCycle = byteInterval ? state.bytesSinceCycle % byteInterval : 0;
}

static uint64_t
yumiGCReplayNext(YumiGCVerificationDriverState& state) {
    if (state.replayState == 0) {
        state.replayState = yumiGCReplaySeed();
    }
    state.replayState ^= state.replayState >> 12;
    state.replayState ^= state.replayState << 25;
    state.replayState ^= state.replayState >> 27;
    return state.replayState * 0x2545f4914f6cdd1dull;
}

static void
yumiGCReplayBeforeAllocation() {
    YumiGCVerificationDriverState& state = yumiGCVerificationDriverState();
    ++state.replayAllocations;
    uint64_t choice = yumiGCReplayNext(state);
    unsigned char phase = yumiGCLoadPublicPhase();

    if (phase == kYumiGCPhaseIdle) {
        size_t interval = yumiGCReplayAllocationInterval();
        if (interval != 0 && state.replayAllocations % interval == 0) {
            yumiFlushGCAllocationBatches();
            yumiGCStartAutomaticCycle(false, YumiGCPacerTriggerIdle);
        }
        return;
    }

    size_t workLimit = 1 + static_cast<size_t>(choice % kYumiGCAutomaticStepWork);
    yumiGCProgressAutomaticCycle(workLimit, false);
}

static void
yumiGCCollectForAllocationIfNeeded(size_t upcomingSize) {
    if (yumiGCCollectionDisabled() || yumiGCThreadHasLocalCallbackRegistration()) {
        return;
    }
    if (yumiGCStressModeEnabled()) {
        yumiGCStressBeforeAllocation(upcomingSize);
        return;
    }
    if (yumiGCReplayModeEnabled()) {
        yumiGCReplayBeforeAllocation();
        return;
    }

    yumiGCAllocationAssistIfNeeded(upcomingSize);

    if (yumiGCLoadPublicPhase() != kYumiGCPhaseIdle) {
        return;
    }

    if (yumiGCHardCapReached(upcomingSize)) {
        yumiGCRunAutomaticFallbackCollection(YumiGCAutomaticFallbackHardCap, true);
        return;
    }

    if (!yumiGCShouldStartAutomaticCycle(upcomingSize)) {
        return;
    }

    yumiFlushGCAllocationBatches();
    if (yumiGCHardCapReached(upcomingSize)) {
        yumiGCRunAutomaticFallbackCollection(YumiGCAutomaticFallbackHardCap, true);
        return;
    }
    if (!yumiGCShouldStartAutomaticCycle(upcomingSize)) {
        return;
    }

    if (yumiGCAllocationAssistDisabled()) {
        if (yumiGCHardTriggerReached(upcomingSize)) {
            yumiGCRunAutomaticFallbackCollection(
                YumiGCAutomaticFallbackNoProgressDriver,
                true);
        }
        return;
    }

    size_t triggerReason = yumiGCHardTriggerReached(upcomingSize)
        ? YumiGCPacerTriggerHardGoal
        : YumiGCPacerTriggerSoftGoal;
    if (yumiGCStartAutomaticCycle(true, triggerReason)) {
        yumiGCAllocationAssistIfNeeded(upcomingSize);
    }
}

extern "C" bool
yumiGCRuntimeIdleCollect(uint64_t budgetNanos) {
    if (yumiGCCollectionDisabled() || !budgetNanos) {
        return false;
    }
    uint64_t startNanos = yumiGCMonotonicNanos();
    uint64_t deadlineNanos = budgetNanos > std::numeric_limits<uint64_t>::max() - startNanos
        ? std::numeric_limits<uint64_t>::max()
        : startNanos + budgetNanos;
    if (yumiGCIdleProgressDisabled()) {
        return false;
    }
    if (!yumiGCBudgetHasTime(deadlineNanos)) {
        return false;
    }

    unsigned char phase = yumiGCLoadPublicPhase();
    if (phase != kYumiGCPhaseIdle) {
        if (!gcAutomaticCycleActive.load(std::memory_order_acquire)) {
            return false;
        }
        yumiGCProgressWithinDeadline(deadlineNanos, yumiGCAutomaticIdleWork());
        return gcAutomaticCycleActive.load(std::memory_order_acquire);
    }

    bool pending = false;
    YumiGCIdleSettle settle = YumiGCIdleSettleNone;
    if (!yumiGCShouldStartAutomaticCycle(0)) {
        settle = yumiGCIdleSettleState(startNanos);
        if (settle != YumiGCIdleSettleDue) {
            yumiGCIdleScavengeIfNeeded(deadlineNanos, true, &pending);
            return pending || settle == YumiGCIdleSettleWaiting;
        }
    }

    yumiFlushGCAllocationBatches();
    if (!yumiGCBudgetHasTime(deadlineNanos)) {
        return false;
    }
    if (!yumiGCShouldStartAutomaticCycle(0) && settle != YumiGCIdleSettleDue) {
        yumiGCIdleScavengeIfNeeded(deadlineNanos, true, &pending);
        return pending;
    }

    if (yumiGCStartAutomaticCycle(false, YumiGCPacerTriggerIdle)) {
        yumiGCProgressWithinDeadline(deadlineNanos, yumiGCAutomaticIdleWork());
        return gcAutomaticCycleActive.load(std::memory_order_acquire);
    }
    yumiGCIdleScavengeIfNeeded(deadlineNanos, true, &pending);
    return pending;
}

extern "C" void
yumiGCFrameBudget(uint64_t remainingNanos) {
    if (yumiGCCollectionDisabled() || !remainingNanos) {
        return;
    }
    yumiEnsureGCInitialized();
    gcFrameBudgetOfferCount.fetch_add(1, std::memory_order_relaxed);
    uint64_t startNanos = yumiGCMonotonicNanos();
    uint64_t deadline = remainingNanos > std::numeric_limits<uint64_t>::max() - startNanos
        ? std::numeric_limits<uint64_t>::max()
        : startNanos + remainingNanos;
    if (!yumiGCBudgetHasTime(deadline)) {
        return;
    }

    bool worked = false;
    if (yumiGCLoadPublicPhase() == kYumiGCPhaseIdle && yumiGCShouldStartAutomaticCycle(0)) {
        yumiFlushGCAllocationBatches();
        if (yumiGCBudgetHasTime(deadline) && yumiGCShouldStartAutomaticCycle(0)) {
            worked = yumiGCStartAutomaticCycle(false, YumiGCPacerTriggerFrame);
        }
    }
    if (yumiGCLoadPublicPhase() != kYumiGCPhaseIdle) {
        worked = yumiGCProgressWithinDeadline(deadline, kYumiGCAutomaticStepWork) || worked;
    } else {
        worked = yumiGCIdleScavengeIfNeeded(deadline, false) || worked;
    }

    if (!worked) {
        return;
    }
    uint64_t endNanos = yumiGCMonotonicNanos();
    size_t elapsedNanos = static_cast<size_t>(endNanos - startNanos);
    gcFrameBudgetWorkCount.fetch_add(1, std::memory_order_relaxed);
    gcFrameBudgetWorkNanos.fetch_add(elapsedNanos, std::memory_order_relaxed);
    if (endNanos > deadline) {
        gcFrameBudgetOverrunCount.fetch_add(1, std::memory_order_relaxed);
        yumiGCMaxAtomic(
            gcFrameBudgetMaxOverrunNanos,
            static_cast<size_t>(endNanos - deadline));
    }
}

static char*
yumiTryFastGCAllocationInThreadRange(
    YumiGCHotAllocationState& hot,
    size_t size,
    size_t allocSize,
    size_t index,
    void* descriptor,
    bool siteAllocation
) {
    // Scheduler handshakes flush and stop Running tasks before phase/generation changes.
    // Callers that cannot participate use the collector-admitted scalar path.
    if (!yumiGCActiveTaskCanUseFastAllocation()) {
        yumiGCDebugUnstoppableFastAllocationRejected();
        if (siteAllocation) {
            hot.stats.allocationSiteFallbackCount += 1;
        }
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    if (yumiGCVerificationSlowPathsEnabled()) {
        if (siteAllocation) {
            hot.stats.allocationSiteFallbackCount += 1;
        }
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }

    uint64_t control = yumiGCLoadControlWord();
    if (yumiGCControlWordHandshakeRequested(control)) {
        if (siteAllocation) {
            hot.stats.allocationSiteFallbackCount += 1;
        }
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }

    size_t generation = yumiGCControlWordSweepGeneration(control);
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    YumiGCRuntimeAllocationBuffer& range = cache->ranges[index];
    bool matches =
        range.generation == generation &&
        range.slotSize >= allocSize &&
        range.descriptor == descriptor &&
        (!descriptor || range.requestedSize == size) &&
        range.buffer.cursor &&
        range.buffer.limit &&
        range.buffer.cursor < range.buffer.limit;
    // The active way is overwhelmingly the hit. Consult parked ways only after that
    // single-range check fails, keeping their generation scan off the ordinary allocation path.
    if (!matches) {
        size_t matchingWay = YumiGCThreadCacheRangeWayCount;
        if (descriptor) {
            yumiGCInvalidateStaleThreadRangeWays(cache, index, generation);
            matchingWay = yumiGCFindThreadRangeWay(
                cache,
                index,
                size,
                allocSize,
                descriptor,
                generation);
        } else {
            matchingWay = yumiGCFindDescriptorlessThreadRangeWay(
                cache,
                index,
                allocSize,
                generation);
        }
        if (matchingWay != 0 && matchingWay < YumiGCThreadCacheRangeWayCount) {
            YumiGCRuntimeAllocationBuffer& parked =
                yumiGCThreadRangeWay(cache, index, matchingWay);
            std::swap(range, parked);
            if (!yumiGCAllocationRangeIsParkable(parked) ||
                    parked.generation != generation) {
                yumiGCClearStagedGCAllocationRange(parked);
            }
            matches =
                range.generation == generation &&
                range.slotSize >= allocSize &&
                range.descriptor == descriptor &&
                (!descriptor || range.requestedSize == size) &&
                yumiGCAllocationRangeHasUnconsumedSlots(range);
        }
    }
    yumiGCRecordAllocationDescriptorMix(hot, index, descriptor);
    bool refilled = false;

    if (!matches) {
        size_t miss = YumiGCAllocationBatchMissEmpty;
        if (range.buffer.cursor && range.buffer.cursor < range.buffer.limit) {
            if (range.generation != generation) {
                miss = YumiGCAllocationBatchMissGeneration;
            } else if (range.descriptor != descriptor) {
                miss = YumiGCAllocationBatchMissDescriptor;
            } else {
                miss = YumiGCAllocationBatchMissRequestedSize;
            }
        }
        yumiGCRecordAllocationBatchMiss(miss);
        if (siteAllocation) {
            hot.stats.allocationSiteMissCount += 1;
        }
        bool samePolicySite =
            range.descriptor == descriptor &&
            range.requestedSize == size &&
            (!range.slotSize || range.slotSize >= allocSize) &&
            (!range.generation || range.generation == generation);
        if (!samePolicySite && (descriptor || range.descriptor) &&
                yumiGCAllocationRangeIsParkable(range)) {
            yumiGCParkGCAllocationRange(cache, index, range, 1);
        } else {
            if (!samePolicySite) {
                yumiGCResetAdaptiveAllocationBufferPolicy(range);
            }
            yumiFlushGCCompilerAllocationBuffer(range);
        }

        YumiGCThreadAllocationBatch batch {};
        size_t selectedCount = yumiGCAdaptiveAllocationBatchCount(
            range,
            allocSize,
            descriptor);
        if (!yumiRefillCachedGCAllocationBatch(
                batch,
                size,
                allocSize,
                descriptor,
                generation,
                selectedCount)) {
            if (siteAllocation) {
                hot.stats.allocationSiteFallbackCount += 1;
            }
            hot.stats.allocationFastFallbackCount += 1;
            return nullptr;
        }
        range.buffer.cursor = batch.cursor;
        range.buffer.limit = batch.limit;
        range.base = batch.cursor;
        range.slotSize = batch.slotSize;
        range.requestedSize = size;
        range.descriptor = descriptor;
        range.generation = generation;
        range.sizeClassIndex = index;
        range.pointerKind = descriptor
            ? kYumiGCAllocationPointerful
            : kYumiGCAllocationPointerFree;
        range.zeroingPolicy = kYumiGCAllocationZeroFull;
        range.reserved[kYumiGCAdaptiveEnabledByte] = descriptor ? 1 : 0;
        range.reserved[kYumiGCAdaptiveCountByte] = static_cast<unsigned char>(
            std::min(selectedCount, kYumiGCAdaptiveAllocationBatchMaxCount));
        range.reserved[kYumiGCAdaptiveReservedSlotCountByte] =
            static_cast<unsigned char>(std::min(
                static_cast<size_t>((batch.limit - batch.cursor) / batch.slotSize),
                kYumiGCAdaptiveAllocationBatchMaxCount));
        range.abiVersion = YumiGCAllocationABIVersion;
        range.structSize = sizeof(YumiGCRuntimeAllocationBuffer);
        refilled = true;
    }

    uint64_t consumeControl = yumiGCLoadControlWord();
    if (yumiGCControlWordHandshakeRequested(consumeControl) ||
            range.generation != yumiGCControlWordSweepGeneration(consumeControl)) {
        yumiFlushGCCompilerAllocationBuffer(range);
        if (siteAllocation) {
            hot.stats.allocationSiteFallbackCount += 1;
        }
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }

    char* result = range.buffer.cursor;
    if (!result || result >= range.buffer.limit) {
        if (siteAllocation) {
            hot.stats.allocationSiteFallbackCount += 1;
        }
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    range.buffer.cursor += range.slotSize;
    // Generic ycache consumers account immediately. Advancing base prevents the owner flush
    // from counting the same slots again while it still returns the unused tail symmetrically.
    range.base = range.buffer.cursor;
    hot.stats.zeroedBytes += range.slotSize;
    hot.stats.batchConsumedBytes += range.slotSize;
    hot.batchCacheBytes = hot.batchCacheBytes > range.slotSize
        ? hot.batchCacheBytes - range.slotSize
        : 0;
    yumiRecordGCPublicAllocation(hot, size, range.slotSize);

    if (refilled) {
        hot.stats.allocationFastRefillCount += 1;
        if (descriptor) {
            hot.stats.genericDescribedRefillCount += 1;
        } else {
            hot.stats.genericDescriptorlessRefillCount += 1;
        }
        if (siteAllocation) {
            hot.stats.allocationSiteRefillCount += 1;
        }
    } else {
        hot.stats.allocationFastHitCount += 1;
        if (siteAllocation) {
            hot.stats.allocationSiteHitCount += 1;
        }
    }
    return result;
}

static char*
yumiTryFastGCAllocation(YumiGCHotAllocationState& hot, size_t size, void* descriptor) {
    size_t allocSize = yumiGCObjectAllocationSize(size);
    if (allocSize > kYumiGCAllocationBatchMaxSize) {
        hot.stats.allocationLargeCount += 1;
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    size_t index = yumiGCAllocationBatchIndex(allocSize);
    if (index >= kYumiGCAllocationBatchCount) {
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    return yumiTryFastGCAllocationInThreadRange(
        hot, size, allocSize, index, descriptor, false);
}

static char*
yumiTryFastGCAllocationWithSiteKey(
    YumiGCHotAllocationState& hot,
    const YumiGCAllocationSiteKey& key
) {
    if (key.allocatedSize > kYumiGCAllocationBatchMaxSize ||
            key.sizeClassIndex >= kYumiGCAllocationBatchCount) {
        hot.stats.allocationLargeCount += 1;
        hot.stats.allocationSiteFallbackCount += 1;
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    return yumiTryFastGCAllocationInThreadRange(
        hot,
        key.requestedSize,
        key.allocatedSize,
        key.sizeClassIndex,
        key.descriptor,
        true);
}

extern "C" void
yumiGCPublishPhase(size_t phase) {
    yumiFlushGCCompilerAllocationBuffer();
    yumiGCUpdateControlWord(YumiGCControlPhaseMask, phase);
}

extern "C" size_t
yumiGCPublicPhase() {
    return (size_t)yumiGCLoadPublicPhase();
}

extern "C" void
yumiGCPublishSweepGeneration(size_t generation) {
    yumiFlushGCCompilerAllocationBuffer();
    const uint64_t generationMask = ~((1ull << YumiGCControlSweepGenerationShift) - 1ull);
    yumiGCUpdateControlWord(
        generationMask,
        (uint64_t)generation << YumiGCControlSweepGenerationShift);
    yumiGCResetAdaptiveAllocationBufferPolicies(yumiGCEnsureActiveThreadCache());
}

extern "C" size_t
yumiGCPublicSweepGeneration() {
    return yumiGCLoadPublicSweepGeneration();
}

extern "C" void
yumiGCPublishHandshakeRequest(size_t reason, size_t epoch, bool requested) {
    const uint64_t mask = YumiGCControlHandshakeRequestedBit |
        YumiGCControlHandshakeReasonMask |
        YumiGCControlHandshakeEpochMask;
    uint64_t value = requested
        ? YumiGCControlHandshakeRequestedBit |
            (((uint64_t)reason << YumiGCControlHandshakeReasonShift) &
                YumiGCControlHandshakeReasonMask) |
            (((uint64_t)epoch << YumiGCControlHandshakeEpochShift) &
                YumiGCControlHandshakeEpochMask)
        : 0;
    yumiGCUpdateControlWord(
        mask,
        value);
}

extern "C" uint64_t
yumiGCPublicControl() {
    return yumiGCLoadControlWord();
}

extern "C" void
yumiGCRuntimeCycleStart(size_t cycle) {
    yumiPublishGCThreadAllocationStats();
    gcCurrentCycleId.store(cycle, std::memory_order_release);
    gcCurrentCycleTriggerReason.store(
        gcPendingCycleTriggerReason.exchange(0, std::memory_order_acq_rel),
        std::memory_order_release);
    gcCurrentCycleHeapAtTrigger.store(
        gcLiveBytes.load(std::memory_order_relaxed),
        std::memory_order_release);
    gcCurrentCycleHeapGoal.store(
        gcHeapGoal.load(std::memory_order_acquire),
        std::memory_order_release);
    gcCurrentCycleMarkDurationNanos.store(0, std::memory_order_release);
    gcCurrentCycleScanWork.store(0, std::memory_order_release);
    gcCurrentCycleSweepPages.store(0, std::memory_order_release);
    gcCycleAllocatedBytes.store(0, std::memory_order_release);
    gcActiveCycleStartNanos.store(yumiGCMonotonicNanos(), std::memory_order_release);
    gcPacerCurrentRootScanNanos.store(0, std::memory_order_release);
    gcPacerCurrentMarkWorkItems.store(0, std::memory_order_release);
    gcPacerCurrentMarkWorkNanos.store(0, std::memory_order_release);
    gcPacerCurrentAssistWorkItems.store(0, std::memory_order_release);
    gcPacerCurrentAssistWorkNanos.store(0, std::memory_order_release);
    gcBackgroundCredit.store(0, std::memory_order_release);
    gcBackgroundWorkerWork.store(0, std::memory_order_release);
    gcBackgroundWorkerCpuNanos.store(0, std::memory_order_release);
}

extern "C" void
yumiGCRuntimeCycleEnd() {
    yumiPublishGCThreadAllocationStats();
    size_t markWork = gcPacerCurrentMarkWorkItems.load(std::memory_order_acquire);
    size_t assistWork = gcPacerCurrentAssistWorkItems.load(std::memory_order_acquire);
    YumiGCCycleSummary summary {
        gcCurrentCycleId.load(std::memory_order_acquire),
        gcCurrentCycleTriggerReason.load(std::memory_order_acquire),
        gcCurrentCycleHeapAtTrigger.load(std::memory_order_acquire),
        gcCurrentCycleHeapGoal.load(std::memory_order_acquire),
        gcLiveBytes.load(std::memory_order_relaxed),
        gcCurrentCycleMarkDurationNanos.load(std::memory_order_acquire),
        markWork,
        assistWork,
        markWork > assistWork ? markWork - assistWork : 0,
        gcCurrentCycleSweepPages.load(std::memory_order_acquire),
        0
    };
    {
        std::lock_guard<std::mutex> lock(gcObservabilityMutex);
        size_t summaryIndex = gcCycleSummarySequence % kYumiGCCycleSummaryCapacity;
        gcCycleSummaries[summaryIndex] = summary;
        ++gcCycleSummarySequence;
        if (gcCycleSummaryStored < kYumiGCCycleSummaryCapacity) {
            ++gcCycleSummaryStored;
        }
    }
    gcLastCycleAllocatedBytes.store(
        gcCycleAllocatedBytes.load(std::memory_order_acquire),
        std::memory_order_release);
    gcActiveCycleStartNanos.store(0, std::memory_order_release);
    gcMarkQueueDepth.store(0, std::memory_order_release);
    gcAssistDebt.store(0, std::memory_order_release);
    gcAutomaticCycleActive.store(false, std::memory_order_release);
    gcBackgroundWorkerGeneration.fetch_add(1, std::memory_order_acq_rel);
    gcCollectionMayGrowRequestedGoal.store(false, std::memory_order_release);
    gcPendingCycleTriggerReason.store(0, std::memory_order_release);
}

extern "C" void
yumiGCRuntimeSubphaseStart(size_t subphase, size_t cycle, size_t) {
    if (!yumiGCValidSubphase(subphase)) {
        return;
    }
    gcSubphaseStartNanos[subphase] = yumiGCMonotonicNanos();
    gcSubphaseStartCycles[subphase] = cycle;
    yumiGCBenchCpuStart();
}

extern "C" void
yumiGCRuntimeSubphaseEnd(size_t subphase, size_t value) {
    if (!yumiGCValidSubphase(subphase) || !gcSubphaseStartNanos[subphase]) {
        return;
    }
    uint64_t start = gcSubphaseStartNanos[subphase];
    uint64_t end = yumiGCMonotonicNanos();
    yumiGCRecordSubphaseSample(
        subphase,
        gcSubphaseStartCycles[subphase],
        start,
        static_cast<size_t>(end - start),
        value);
    gcSubphaseStartNanos[subphase] = 0;
    gcSubphaseStartCycles[subphase] = 0;
    yumiGCBenchCpuEnd();
}

extern "C" void
yumiGCRuntimeSubphaseCancel(size_t subphase) {
    if (!yumiGCValidSubphase(subphase)) {
        return;
    }
    bool hadStart = gcSubphaseStartNanos[subphase] != 0;
    gcSubphaseStartNanos[subphase] = 0;
    gcSubphaseStartCycles[subphase] = 0;
    if (hadStart) {
        yumiGCBenchCpuEnd();
    }
}

extern "C" size_t
yumiGCRuntimeBenchGCCpuNanos() {
#if YU_ENABLE_BENCH_METRICS
    return gcBenchCpuNanos.load(std::memory_order_relaxed);
#else
    return 0;
#endif
}

extern "C" size_t
yumiGCRuntimeBenchAllocationSiteCount() {
    return YumiGCCompilerAllocationBufferCount + 1;
}

extern "C" size_t
yumiGCRuntimeBenchAllocationSiteValue(size_t index) {
#if YU_ENABLE_BENCH_METRICS
    return index < gcBenchAllocationSites.size()
        ? gcBenchAllocationSites[index].load(std::memory_order_relaxed)
        : 0;
#else
    (void)index;
    return 0;
#endif
}

extern "C" size_t
yumiGCRuntimeBenchThreadRefillCount() {
#if YU_ENABLE_BENCH_METRICS
    yumiPublishGCThreadAllocationStats();
    std::lock_guard<std::mutex> lock(gcBenchThreadRefillMutex);
    return gcBenchThreadRefillRecords.size();
#else
    return 0;
#endif
}

extern "C" size_t
yumiGCRuntimeBenchThreadRefillValue(size_t index) {
#if YU_ENABLE_BENCH_METRICS
    std::lock_guard<std::mutex> lock(gcBenchThreadRefillMutex);
    return index < gcBenchThreadRefillRecords.size()
        ? gcBenchThreadRefillRecords[index]->refills.load(std::memory_order_relaxed)
        : 0;
#else
    (void)index;
    return 0;
#endif
}

extern "C" size_t
yumiGCRuntimeBenchAllocationSizeBinCount() {
#if YU_ENABLE_BENCH_METRICS
    return kYumiGCBenchAllocationBinCount;
#else
    return 0;
#endif
}

extern "C" size_t
yumiGCRuntimeBenchAllocationSizeBinUpperBound(size_t index) {
#if YU_ENABLE_BENCH_METRICS
    if (index < kYumiGCBenchAllocationSmallBinCount) {
        return (index + 1) * 8;
    }
    if (index < kYumiGCBenchAllocationOverflowBin) {
        return 1024 +
            (index - kYumiGCBenchAllocationSmallBinCount + 1) * 1024;
    }
#else
    (void)index;
#endif
    return std::numeric_limits<size_t>::max();
}

extern "C" size_t
yumiGCRuntimeBenchAllocationSizeBinValue(size_t index) {
#if YU_ENABLE_BENCH_METRICS
    return index < gcBenchAllocationSizeBins.size()
        ? gcBenchAllocationSizeBins[index].load(std::memory_order_relaxed)
        : 0;
#else
    (void)index;
    return 0;
#endif
}

extern "C" size_t
yumiGCRuntimeBenchAllocatedSizeClassCount() {
#if YU_ENABLE_BENCH_METRICS
    return kYumiGCBenchSizeClassCount;
#else
    return 0;
#endif
}

extern "C" size_t
yumiGCRuntimeBenchAllocatedSizeClassUpperBound(size_t index) {
#if YU_ENABLE_BENCH_METRICS
    return index < kYumiGCBenchSizeClassCount
        ? kYumiGCBenchSizeClassUpperBounds[index]
        : std::numeric_limits<size_t>::max();
#else
    (void)index;
    return std::numeric_limits<size_t>::max();
#endif
}

extern "C" size_t
yumiGCRuntimeBenchAllocatedSizeClassValue(size_t index) {
#if YU_ENABLE_BENCH_METRICS
    return index < gcBenchAllocatedSizeClasses.size()
        ? gcBenchAllocatedSizeClasses[index].load(std::memory_order_relaxed)
        : 0;
#else
    (void)index;
    return 0;
#endif
}

extern "C" void
yumiGCRuntimeBenchResetAllocationHistograms() {
#if YU_ENABLE_BENCH_METRICS
    for (auto& count : gcBenchAllocationSizeBins) {
        count.store(0, std::memory_order_relaxed);
    }
    for (auto& count : gcBenchAllocatedSizeClasses) {
        count.store(0, std::memory_order_relaxed);
    }
#endif
}

extern "C" size_t
yumiGCRuntimePauseEventCapacity() {
    return kYumiGCPauseEventCapacity;
}

extern "C" void
yumiGCRuntimeRecordSubphaseDuration(
    size_t subphase,
    size_t cycle,
    size_t durationNanos,
    size_t value
) {
    if (!yumiGCValidSubphase(subphase) || !durationNanos) {
        return;
    }
    yumiGCRecordSubphaseSample(
        subphase,
        cycle,
        yumiGCMonotonicNanos() - durationNanos,
        durationNanos,
        value);
}

extern "C" void
yumiGCRuntimeSetMarkQueueDepth(size_t depth) {
    gcMarkQueueDepth.store(depth, std::memory_order_release);
}

extern "C" void
yumiGCRuntimeSetAssistDebt(size_t debt) {
    gcAssistDebt.store(debt, std::memory_order_release);
}

extern "C" void
yumiGCRuntimeRecordBarrier(size_t slowCount, size_t shadeCount) {
    YumiGCBarrierStatsRecord* record = yumiGCEnsureThreadBarrierStats();
    yumiGCAddOwnedBarrierCounter(record->barrierCalls, 1);
    yumiGCAddOwnedBarrierCounter(record->barrierSlowEntries, slowCount);
    yumiGCAddOwnedBarrierCounter(record->barrierShades, shadeCount);
}

extern "C" void
yumiGCRuntimeRecordBarrierSlow() {
    YumiGCBarrierStatsRecord* record = yumiGCEnsureThreadBarrierStats();
    yumiGCAddOwnedBarrierCounter(record->barrierSlowEntries, 1);
}

extern "C" void
yumiGCRuntimeRecordBarrierShade() {
    YumiGCBarrierStatsRecord* record = yumiGCEnsureThreadBarrierStats();
    yumiGCAddOwnedBarrierCounter(record->barrierShades, 1);
}

extern "C" void
yumiGCRuntimeRecordAssist() {
    ++gcHotAllocationState.stats.assistCount;
}

extern "C" void
yumiGCRuntimeRecordAllocatedDuringMark() {
    gcAllocatedDuringMarkCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
yumiGCRuntimeRecordMarkQueuePushes(size_t count, size_t depth) {
    gcMarkQueuePushCount.fetch_add(count, std::memory_order_relaxed);
    gcMarkQueueDepth.store(depth, std::memory_order_release);
}

extern "C" void
yumiGCRuntimeRecordMarkQueuePopBatch(size_t count, size_t depth) {
    if (!count) {
        return;
    }
    gcMarkQueuePopCount.fetch_add(count, std::memory_order_relaxed);
    gcMarkQueuePopBatchCount.fetch_add(1, std::memory_order_relaxed);
    yumiGCMaxAtomic(gcMarkQueueMaxPopBatch, count);
    gcMarkQueueDepth.store(depth, std::memory_order_release);
}

extern "C" void
yumiGCRuntimeRecordAllocation(size_t requestedSize, size_t allocatedSize) {
    yumiRecordGCAllocation(requestedSize, allocatedSize);
}

extern "C" void
yumiGCRuntimeRecordZeroedBytes(size_t byteSize) {
    yumiGCRecordAllocationZeroedBytes(byteSize);
}

extern "C" void
yumiGCRuntimeRecordBatchReservation(size_t count, size_t allocatedSize) {
    size_t bytes = count * allocatedSize;
    gcBatchReservedBytes.fetch_add(bytes, std::memory_order_relaxed);
    size_t reservedBytes = gcHotAllocationState.batchPendingReservationBytes;
    if (reservedBytes >= bytes) {
        size_t surplusBytes = reservedBytes - bytes;
        gcHotAllocationState.batchPendingReservationBytes = 0;
        if (surplusBytes) {
            yumiGCSubtractAtomicSaturating(gcBatchCacheBytes, surplusBytes);
            gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > surplusBytes
                ? gcHotAllocationState.batchCacheBytes - surplusBytes
                : 0;
        }
    } else {
        size_t extraBytes = bytes - reservedBytes;
        gcHotAllocationState.batchPendingReservationBytes = 0;
        if (extraBytes) {
            if (yumiGCBatchCacheReserve(extraBytes)) {
                gcHotAllocationState.batchPendingReservationBytes = gcHotAllocationState.batchPendingReservationBytes >
                        extraBytes
                    ? gcHotAllocationState.batchPendingReservationBytes - extraBytes
                    : 0;
            } else {
                gcBatchCacheBytes.fetch_add(extraBytes, std::memory_order_relaxed);
                gcHotAllocationState.batchCacheBytes += extraBytes;
            }
        }
    }
    yumiRecordGCHeapAllocation(count, allocatedSize);
}

extern "C" void
yumiGCRuntimeRecordBatchConsumption(size_t allocatedSize) {
    gcHotAllocationState.stats.batchConsumedBytes += allocatedSize;
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > allocatedSize
        ? gcHotAllocationState.batchCacheBytes - allocatedSize
        : 0;
}

extern "C" void
yumiGCRuntimeCancelBatchCacheReservation(size_t bytes) {
    yumiGCBatchCacheCancelReservation(bytes);
}

extern "C" void
yumiGCRuntimeRecordBatchRelease(size_t count, size_t allocatedSize) {
    yumiPublishGCThreadBatchConsumption();
    size_t bytes = count * allocatedSize;
    gcBatchReleasedBytes.fetch_add(bytes, std::memory_order_relaxed);
    yumiGCSubtractAtomicSaturating(gcBatchCacheBytes, bytes);
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > bytes ? gcHotAllocationState.batchCacheBytes - bytes : 0;
}

extern "C" void
yumiGCRuntimeRecordBatchDiscard(size_t count, size_t allocatedSize) {
    yumiPublishGCThreadBatchConsumption();
    size_t bytes = count * allocatedSize;
    yumiGCRecordBatchDiscard(bytes);
}

extern "C" void
yumiGCRuntimeRecordSweep(size_t freedObjects, size_t freedBytes) {
    size_t liveObjects = gcLiveObjects.load(std::memory_order_relaxed);
    while (
        liveObjects &&
        !gcLiveObjects.compare_exchange_weak(
            liveObjects,
            liveObjects > freedObjects ? liveObjects - freedObjects : 0,
            std::memory_order_relaxed,
            std::memory_order_relaxed
        )
    ) {}

    size_t liveBytes = gcLiveBytes.load(std::memory_order_relaxed);
    while (
        liveBytes &&
        !gcLiveBytes.compare_exchange_weak(
            liveBytes,
            liveBytes > freedBytes ? liveBytes - freedBytes : 0,
            std::memory_order_relaxed,
            std::memory_order_relaxed
        )
    ) {}
}

extern "C" void
yumiGCRuntimeRecordSweepPages(size_t pages) {
    if (pages) {
        gcCurrentCycleSweepPages.fetch_add(pages, std::memory_order_relaxed);
    }
}

extern "C" void
yumiGCRuntimeRecordScavengePages(size_t pages) {
    if (!pages) {
        return;
    }
    gcScavengedPageTotal.fetch_add(pages, std::memory_order_relaxed);
    size_t cycleId = gcCurrentCycleId.load(std::memory_order_acquire);
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    for (size_t offset = 0; offset < gcCycleSummaryStored; ++offset) {
        size_t sequence = gcCycleSummarySequence - 1 - offset;
        YumiGCCycleSummary& summary =
            gcCycleSummaries[sequence % kYumiGCCycleSummaryCapacity];
        if (summary.cycleId == cycleId) {
            summary.scavengedPages += pages;
            return;
        }
    }
}

extern "C" void
yumiGCRuntimeRecordSpanAllocation() {
    gcAllocatedSpans.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
yumiGCRuntimeRecordArenaAllocation() {
    gcAllocatedArenas.fetch_add(1, std::memory_order_relaxed);
}

extern "C" bool
yumiGCRuntimeCanReserveBatchCache(size_t bytes) {
    return yumiGCBatchCacheReserve(bytes);
}

extern "C" void
yumiGCRuntimeFlushThreadAllocationBatches() {
    yumiWBBufFlush();
    yumiFlushGCAllocationBatches();
    yumiPublishGCThreadAllocationStats();
}

extern "C" void
yumiGCRuntimeFlushWriteBarrierBuffer() {
    // Idle tasks cannot have acquired new buffered edges, and the termination handshake drains
    // every task before Sweep. Avoid resolving the thread-local cache on the very frequent
    // coroutine blocked-state boundary when no buffered publication can exist.
    if (!yumiGCPhaseNeedsMarkBarrier(yumiGCLoadPublicPhase())) {
        return;
    }
    yumiWBBufFlush();
}

extern "C" void
yumiGCRuntimeFlushActiveAllocationBuffer() {
    // Ordinary task suspension detaches task-local roots, but compiler and generic ranges are
    // owned by the YuThread cache. Keep those generation-checked ranges hot across coroutine
    // switches; collector handshakes and explicit full-thread flushes still revoke them.
    if (gcActiveAllocationBufferStorage) {
        yumiFlushGCCompilerAllocationBuffer(*gcActiveAllocationBufferStorage);
    }
    gcHotAllocationState.stats.flush();
}

extern "C" void
yumiGCRuntimeRevokeActiveAllocationBuffer() {
    yumiFlushGCCompilerAllocationBuffer();
}

extern "C" void
yumiGCRuntimeSetHeapGoal(size_t goal) {
    if (yumiGCCollectionDisabled()) {
        gcRequestedHeapGoal.store(
            std::numeric_limits<size_t>::max(),
            std::memory_order_release);
        gcHeapGoal.store(std::numeric_limits<size_t>::max(), std::memory_order_release);
        yumiGCPacerInvalidateGoalCache();
        return;
    }
    size_t cappedGoal = goal ? yumiGCApplyHeapHardCap(goal) : yumiGCMinimumHeapGoal();
    gcRequestedHeapGoal.store(cappedGoal, std::memory_order_release);
    gcHeapGoal.store(cappedGoal, std::memory_order_release);
    yumiGCPacerInvalidateGoalCache();
}

extern "C" void
yumiGCRuntimeSetMinimumHeapGoal(size_t goal) {
    gcProgramMinimumHeapGoal.store(goal, std::memory_order_release);
    if (yumiGCCollectionDisabled() || gcRequestedHeapGoal.load(std::memory_order_acquire)) {
        return;
    }
    size_t minimum = yumiGCMinimumHeapGoal();
    size_t current = gcHeapGoal.load(std::memory_order_acquire);
    while (current < minimum &&
            !gcHeapGoal.compare_exchange_weak(
                current,
                minimum,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
    }
    yumiGCPacerInvalidateGoalCache();
}

static size_t
yumiGCTaperedGrowth(size_t live) {
    size_t percent = yumiGCHeapGrowthPercent();
    if (live <= kYumiGCHeapTaperStart) {
        return yumiGCMultiplyDivideSaturating(live, percent, 100);
    }
    size_t full = yumiGCMultiplyDivideSaturating(kYumiGCHeapTaperStart, percent, 100);
    size_t half = yumiGCMultiplyDivideSaturating(live - kYumiGCHeapTaperStart, percent, 200);
    return full > std::numeric_limits<size_t>::max() - half
        ? std::numeric_limits<size_t>::max()
        : full + half;
}

static size_t
yumiGCLimitGrowthToAvailableMemory(size_t growth) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    static constexpr size_t kLeastGrowth = 16ull * 1024ull * 1024ull;
    size_t available = os_proc_available_memory();
    if (available) {
        return std::min(growth, std::max(available / 2, kLeastGrowth));
    }
#endif
    return growth;
}

extern "C" void
yumiGCRuntimeFinishCollection() {
    gcMetadataTrimPending.store(true, std::memory_order_release);
    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    if (gcCurrentCycleTriggerReason.load(std::memory_order_acquire) == YumiGCPacerTriggerIdle) {
        gcIdleSettledLiveBytes.store(live, std::memory_order_release);
    }
    size_t requestedGoal = gcRequestedHeapGoal.load(std::memory_order_acquire);
    if (requestedGoal && !gcCollectionMayGrowRequestedGoal.load(std::memory_order_acquire)) {
        yumiGCPacerPublishCycleStats(live, gcHeapGoal.load(std::memory_order_acquire));
        gcCollectionCount.fetch_add(1, std::memory_order_relaxed);
        yumiGCRuntimeCycleEnd();
        return;
    }

    size_t minimumGoal = requestedGoal ? requestedGoal : yumiGCMinimumHeapGoal();
    size_t minimumGrowth = requestedGoal
        ? std::max<size_t>(requestedGoal / 2, 64ull * 1024ull)
        : yumiGCHeapMinGrowth();
    size_t growth = yumiGCLimitGrowthToAvailableMemory(std::max(
        minimumGrowth,
        yumiGCTaperedGrowth(live)));
    size_t goal = live > std::numeric_limits<size_t>::max() - growth
        ? std::numeric_limits<size_t>::max()
        : live + growth;
    goal = std::max(goal, minimumGoal);
    size_t overshootCorrection =
        gcPacerLastGoalOvershootBytes.load(std::memory_order_acquire) / 2;
    if (goal > live && overshootCorrection) {
        goal -= std::min(goal - live, overshootCorrection);
    }
    goal = yumiGCApplyHeapHardCap(goal);
    gcHeapGoal.store(goal, std::memory_order_release);
    yumiGCPacerPublishCycleStats(live, goal);
    gcCollectionCount.fetch_add(1, std::memory_order_relaxed);
    yumiGCRuntimeCycleEnd();
}

extern "C" void
yumiGCRuntimeRecordAutomaticFallback(size_t reason) {
    gcAutomaticFallbackReason.store(reason, std::memory_order_release);
    gcAutomaticFallbackCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
yumiGCRuntimeMarkStart() {
    gcMarkStartNanos.store(yumiGCMonotonicNanos(), std::memory_order_release);
}

extern "C" void
yumiGCRuntimeMarkEnd() {
    uint64_t startNanos = gcMarkStartNanos.exchange(0, std::memory_order_acq_rel);
    if (!startNanos) {
        return;
    }
    size_t duration = static_cast<size_t>(yumiGCMonotonicNanos() - startNanos);
    gcMarkTimeNanos.fetch_add(duration, std::memory_order_relaxed);
    gcCurrentCycleMarkDurationNanos.store(duration, std::memory_order_release);
}

extern "C" void
yumiGCRuntimeSweepStart() {
    gcSweepStartNanos = yumiGCMonotonicNanos();
    yumiGCRuntimeSubphaseStart(
        YumiGCObservationSweep,
        gcCurrentCycleId.load(std::memory_order_acquire),
        0);
}

extern "C" void
yumiGCRuntimeSweepEnd() {
    if (!gcSweepStartNanos) {
        return;
    }
    gcSweepTimeNanos.fetch_add(
        (size_t)(yumiGCMonotonicNanos() - gcSweepStartNanos),
        std::memory_order_relaxed);
    yumiGCRuntimeSubphaseEnd(
        YumiGCObservationSweep,
        gcCycleAllocatedBytes.load(std::memory_order_relaxed));
    gcSweepStartNanos = 0;
}

extern "C" bool
yumiGCRuntimeDecommitPages(void* pointer, size_t byteSize) {
    if (!pointer || byteSize == 0) {
        return true;
    }

    bool advised = false;
#if defined(MADV_FREE)
    advised = madvise(pointer, byteSize, MADV_FREE) == 0;
#endif
#if defined(MADV_DONTNEED)
    if (!advised) {
        advised = madvise(pointer, byteSize, MADV_DONTNEED) == 0;
    }
#endif
    return advised && mprotect(pointer, byteSize, PROT_NONE) == 0;
}

extern "C" bool
yumiGCRuntimeRecommitPages(void* pointer, size_t byteSize) {
    if (!pointer || byteSize == 0) {
        return true;
    }
    return mprotect(pointer, byteSize, PROT_READ | PROT_WRITE) == 0;
}

extern "C" void
yumiGCRuntimeHandshakeStart() {
    yumiPublishGCThreadAllocationStats();
    gcHandshakeStartNanos = yumiGCMonotonicNanos();
    gcHandshakePauseKind = 0;
    gcHandshakeWaitRecorded = false;
    gcHandshakeTasksWaited = 0;
    gcHandshakeSlowestTaskNanos = 0;
    yumiGCRuntimeSubphaseStart(
        YumiGCObservationHandshake,
        gcCurrentCycleId.load(std::memory_order_acquire),
        0);
}

extern "C" void
yumiGCRuntimeHandshakeSetReason(size_t reason) {
    gcHandshakePauseKind = yumiGCPauseKindForHandshakeReason(reason);
}

extern "C" void
yumiGCRuntimeHandshakeWaitEnd() {
    yumiPublishGCThreadAllocationStats();
    if (gcHandshakeStartNanos && !gcHandshakeWaitRecorded) {
        uint64_t now = yumiGCMonotonicNanos();
        gcHandshakeLatencyNanos.fetch_add(
            (size_t)(now - gcHandshakeStartNanos),
            std::memory_order_relaxed);
        gcHandshakeCount.fetch_add(1, std::memory_order_relaxed);
        gcHandshakeWaitRecorded = true;
    }
}

extern "C" void
yumiGCRuntimeRecordPauseTaskWait(size_t tasksWaited, size_t slowestTaskNanos) {
    gcHandshakeTasksWaited = tasksWaited;
    gcHandshakeSlowestTaskNanos = slowestTaskNanos;
}

extern "C" void
yumiGCRuntimeRecordPerTaskSetupScan(size_t startNanos, size_t durationNanos) {
    if (gcHandshakePauseKind != YumiGCPauseKindMarkSetupTotal) {
        return;
    }
    yumiGCRecordPauseEvent(
        YumiGCPauseKindPerTaskSetupScan,
        gcCurrentCycleId.load(std::memory_order_acquire),
        startNanos,
        durationNanos,
        1,
        durationNanos);
}

extern "C" void
yumiGCRuntimeHandshakeEnd() {
    yumiPublishGCThreadAllocationStats();
    if (!gcHandshakeStartNanos) {
        return;
    }
    yumiGCRuntimeHandshakeWaitEnd();

    uint64_t endNanos = yumiGCMonotonicNanos();
    size_t durationNanos = static_cast<size_t>(endNanos - gcHandshakeStartNanos);
    size_t cycleId = gcCurrentCycleId.load(std::memory_order_acquire);
    yumiGCRecordPauseEvent(
        gcHandshakePauseKind,
        cycleId,
        gcHandshakeStartNanos,
        durationNanos,
        gcHandshakeTasksWaited,
        gcHandshakeSlowestTaskNanos);
    yumiGCRuntimeSubphaseEnd(
        YumiGCObservationHandshake,
        gcMarkQueueDepth.load(std::memory_order_relaxed));
    gcHandshakeStartNanos = 0;
    gcHandshakePauseKind = 0;
    gcHandshakeWaitRecorded = false;
    gcHandshakeTasksWaited = 0;
    gcHandshakeSlowestTaskNanos = 0;
}

extern "C" bool
yumiGCRuntimeVerifyEachSweep() {
    static const bool verifyEachSweep = []() {
        const char* value = std::getenv("YUMI_GC_VERIFY_EACH_SWEEP");
        return value && *value && std::strcmp(value, "0") != 0;
    }();
    return verifyEachSweep;
}

extern "C" size_t
yumiGCRuntimeAllocatedObjectCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocatedObjects.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeRequestedByteCount() {
    yumiPublishGCThreadAllocationStats();
    return gcRequestedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocatedByteCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocatedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeLiveByteCount() {
    return gcLiveBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocatedSpanCount() {
    return gcAllocatedSpans.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocatedArenaCount() {
    return gcAllocatedArenas.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeHeapGoal() {
    size_t goal = gcHeapGoal.load(std::memory_order_acquire);
    return goal ? goal : yumiGCMinimumHeapGoal();
}

extern "C" size_t
yumiGCRuntimePacerGrowthPercent() {
    return yumiGCHeapGrowthPercent();
}

extern "C" size_t
yumiGCRuntimeBackgroundWorkerCount() {
    return gcBackgroundWorkerCount.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeBackgroundWorkerThreadCapacity() {
    return gcBackgroundWorkerThreadCapacity.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeBackgroundWorkerDutyPermille() {
    return gcBackgroundWorkerDutyPermille.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeBackgroundWorkerWork() {
    return gcBackgroundWorkerWork.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeBackgroundCredit() {
    return gcBackgroundCredit.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeFrameBudgetOfferCount() {
    return gcFrameBudgetOfferCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeFrameBudgetWorkCount() {
    return gcFrameBudgetWorkCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeFrameBudgetWorkNanos() {
    return gcFrameBudgetWorkNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeFrameBudgetOverrunCount() {
    return gcFrameBudgetOverrunCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeFrameBudgetMaxOverrunNanos() {
    return gcFrameBudgetMaxOverrunNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimePacerHardCap() {
    return yumiGCHeapHardCap();
}

extern "C" size_t
yumiGCRuntimePacerSoftGoal() {
    size_t goal = yumiGCRuntimeHeapGoal();
    size_t softGoal = yumiGCAutomaticSoftGoal(goal);
    gcPacerLastSoftGoal.store(softGoal, std::memory_order_release);
    return softGoal;
}

extern "C" size_t
yumiGCRuntimePacerHardGoal() {
    size_t goal = yumiGCRuntimeHeapGoal();
    size_t hardGoal = yumiGCAutomaticHardGoal(goal);
    gcPacerLastHardGoal.store(hardGoal, std::memory_order_release);
    return hardGoal;
}

extern "C" size_t
yumiGCRuntimePacerLastTriggerReason() {
    return gcPacerLastTriggerReason.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeCollectionCount() {
    return gcCollectionCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeMarkTimeNanos() {
    return gcMarkTimeNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeSweepTimeNanos() {
    return gcSweepTimeNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationStallCount() {
    return gcAllocationStallCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationStallTimeNanos() {
    return gcAllocationStallTimeNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeHandshakeCount() {
    return gcHandshakeCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeHandshakeLatencyNanos() {
    return gcHandshakeLatencyNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeCurrentCycleId() {
    return gcCurrentCycleId.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeActiveCycleAgeNanos() {
    uint64_t start = gcActiveCycleStartNanos.load(std::memory_order_acquire);
    if (!start) {
        return 0;
    }
    return static_cast<size_t>(yumiGCMonotonicNanos() - start);
}

extern "C" size_t
yumiGCRuntimeMarkQueueDepth() {
    return gcMarkQueueDepth.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeAssistDebt() {
    return gcAssistDebt.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeBarrierCount() {
    yumiPublishGCWriteBarrierCounters(yumiGCEnsureActiveThreadCache());
    return yumiGCSumBarrierCounter(YumiGCBarrierStatsCalls);
}

extern "C" size_t
yumiGCRuntimeAssistCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAssistCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAssistTimeNanos() {
    yumiPublishGCThreadAllocationStats();
    return gcAssistTimeNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeMarkQueuePushCount() {
    return gcMarkQueuePushCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeMarkQueuePopCount() {
    return gcMarkQueuePopCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeMarkQueuePopBatchCount() {
    return gcMarkQueuePopBatchCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeMarkQueueMaxPopBatch() {
    return gcMarkQueueMaxPopBatch.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeBatchReservedBytes() {
    yumiPublishGCThreadBatchConsumption();
    return gcBatchReservedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeBatchConsumedBytes() {
    yumiPublishGCThreadBatchConsumption();
    return gcBatchConsumedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeBatchReleasedBytes() {
    yumiPublishGCThreadBatchConsumption();
    return gcBatchReleasedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeBatchDiscardedBytes() {
    yumiPublishGCThreadBatchConsumption();
    return gcBatchDiscardedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeBatchCacheBytes() {
    yumiPublishGCThreadBatchConsumption();
    return gcBatchCacheBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeBatchUnusedBytes() {
    yumiPublishGCThreadBatchConsumption();
    return gcBatchCacheBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeThreadBatchCacheBytes() {
    return gcHotAllocationState.batchCacheBytes;
}

extern "C" size_t
yumiGCRuntimeAdaptiveBatchGrowthCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAdaptiveBatchGrowthCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAdaptiveBatchShrinkCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAdaptiveBatchShrinkCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAdaptiveBatchReturnedUnusedSlotCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAdaptiveBatchReturnedUnusedSlotCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeCompilerIndexedRefillCount() {
    yumiPublishGCThreadAllocationStats();
    return gcCompilerIndexedRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeActiveBufferRefillCount() {
    yumiPublishGCThreadAllocationStats();
    return gcActiveBufferRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeGenericDescribedRefillCount() {
    yumiPublishGCThreadAllocationStats();
    return gcGenericDescribedRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeGenericDescriptorlessRefillCount() {
    yumiPublishGCThreadAllocationStats();
    return gcGenericDescriptorlessRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeCompilerBufferSiteSwitchCount() {
    return gcCompilerBufferSiteSwitchCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeFullAllocationRangeFlushCount() {
    return gcFullAllocationRangeFlushCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeFullAllocationRangeFlushedBufferCount() {
    return gcFullAllocationRangeFlushedBufferCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeBatchCachePressureFlushCount() {
    return gcBatchCachePressureFlushCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeThreadPendingAllocatedObjectCount() {
    size_t objectCount = gcHotAllocationState.stats.allocatedObjects;
    size_t consumedBytes = 0;
    if (gcActiveAllocationBufferStorage) {
        yumiGCAddPendingCompilerAllocationBufferStats(
            *gcActiveAllocationBufferStorage,
            objectCount,
            consumedBytes);
    }
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        size_t slot = cache->activeCompilerBufferIndices[active];
        if (slot >= YumiGCCompilerAllocationBufferCount) {
            continue;
        }
        const YumiGCRuntimeAllocationBuffer& storage = cache->compilerBuffers[slot];
        if (gcActiveAllocationBufferStorage == &storage) {
            continue;
        }
        yumiGCAddPendingCompilerAllocationBufferStats(storage, objectCount, consumedBytes);
    }
    return objectCount;
}

extern "C" size_t
yumiGCRuntimeThreadPendingBatchConsumedBytes() {
    size_t objectCount = 0;
    size_t consumedBytes = gcHotAllocationState.stats.batchConsumedBytes;
    if (gcActiveAllocationBufferStorage) {
        yumiGCAddPendingCompilerAllocationBufferStats(
            *gcActiveAllocationBufferStorage,
            objectCount,
            consumedBytes);
    }
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        size_t slot = cache->activeCompilerBufferIndices[active];
        if (slot >= YumiGCCompilerAllocationBufferCount) {
            continue;
        }
        const YumiGCRuntimeAllocationBuffer& storage = cache->compilerBuffers[slot];
        if (gcActiveAllocationBufferStorage == &storage) {
            continue;
        }
        yumiGCAddPendingCompilerAllocationBufferStats(storage, objectCount, consumedBytes);
    }
    return consumedBytes;
}

extern "C" size_t
yumiGCRuntimeStaleBatchDropCount() {
    return gcStaleBatchDropCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeFallbackAllocationCount() {
    return gcFallbackAllocationCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationCallCount(size_t kind) {
    yumiPublishGCThreadAllocationStats();
    if (kind >= gcAllocationCallsByKind.size()) {
        return 0;
    }
    return gcAllocationCallsByKind[kind].load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationPhaseCount(size_t phase) {
    yumiPublishGCThreadAllocationStats();
    if (phase >= gcAllocationCallsByPhase.size()) {
        return 0;
    }
    return gcAllocationCallsByPhase[phase].load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationFastHitCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationFastHitCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationFastRefillCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationFastRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationFastFallbackCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationFastFallbackCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationLargeCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationLargeCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationBatchMissCount(size_t reason) {
    yumiPublishGCThreadAllocationStats();
    if (reason >= gcAllocationBatchMisses.size()) {
        return 0;
    }
    return gcAllocationBatchMisses[reason].load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationZeroedBytes() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationZeroedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationStatPublicationCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationStatPublicationCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationAssistCheckCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationAssistCheckCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationAssistCallCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationAssistCallCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeFastBarrierCallCount() {
    yumiPublishGCWriteBarrierCounters(yumiGCEnsureActiveThreadCache());
    return yumiGCSumBarrierCounter(YumiGCBarrierStatsFastCalls);
}

extern "C" size_t
yumiGCRuntimeFastBarrierSlowEntryCount() {
    yumiPublishGCWriteBarrierCounters(yumiGCEnsureActiveThreadCache());
    return yumiGCSumBarrierCounter(YumiGCBarrierStatsFastSlowEntries);
}

extern "C" size_t
yumiGCRuntimeBufferedBarrierSharedRMWCount() {
    yumiPublishGCWriteBarrierCounters(yumiGCEnsureActiveThreadCache());
    // Compiler barrier counters are transferred only into their OS-thread-owned record.
    return 0;
}

extern "C" size_t
yumiGCRuntimeAllocationDescriptorMixCount(size_t index, size_t described) {
    yumiPublishGCThreadAllocationStats();
    if (index >= kYumiGCAllocationBatchCount) {
        return 0;
    }
    const auto& counters = described
        ? gcAllocationDescribedBySizeClass
        : gcAllocationDescriptorlessBySizeClass;
    return counters[index].load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationSiteHitCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationSiteHitCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationSiteMissCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationSiteMissCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationSiteRefillCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationSiteRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAllocationSiteFallbackCount() {
    yumiPublishGCThreadAllocationStats();
    return gcAllocationSiteFallbackCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAutomaticFallbackCount() {
    return gcAutomaticFallbackCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimeAutomaticFallbackReason() {
    return gcAutomaticFallbackReason.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCRuntimeAutomaticDrainGiveUpCount() {
    return gcAutomaticDrainGiveUpCount.load(std::memory_order_relaxed);
}

static size_t
yumiGCObservationSampleCountForKind(size_t observation) {
    if (!yumiGCValidSubphase(observation)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    return gcSubphaseSamples[observation].durations.size();
}

static size_t
yumiGCObservationMaxNanosForKind(size_t observation) {
    if (!yumiGCValidSubphase(observation)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    return gcSubphaseSamples[observation].maxDurationNanos;
}

#define YU_GC_NAMED_OBSERVATION_COUNT(name, observation) \
    extern "C" size_t yumiGCRuntime##name(void) { \
        return yumiGCObservationSampleCountForKind(observation); \
    }
#define YU_GC_NAMED_OBSERVATION_PERCENTILE(name, observation, percentile) \
    extern "C" size_t yumiGCRuntime##name(void) { \
        return yumiGCSubphasePercentile(observation, percentile); \
    }
#define YU_GC_NAMED_OBSERVATION_MAX(name, observation) \
    extern "C" size_t yumiGCRuntime##name(void) { \
        return yumiGCObservationMaxNanosForKind(observation); \
    }

YU_GC_NAMED_OBSERVATION_COUNT(MarkSetupSampleCount, YumiGCObservationMarkSetup)
YU_GC_NAMED_OBSERVATION_PERCENTILE(MarkSetupP50Nanos, YumiGCObservationMarkSetup, 50)
YU_GC_NAMED_OBSERVATION_MAX(MarkSetupMaxNanos, YumiGCObservationMarkSetup)
YU_GC_NAMED_OBSERVATION_COUNT(RootScanSampleCount, YumiGCObservationRootScan)
YU_GC_NAMED_OBSERVATION_PERCENTILE(RootScanP95Nanos, YumiGCObservationRootScan, 95)
YU_GC_NAMED_OBSERVATION_PERCENTILE(RootScanP99Nanos, YumiGCObservationRootScan, 99)
YU_GC_NAMED_OBSERVATION_MAX(RootScanMaxNanos, YumiGCObservationRootScan)
YU_GC_NAMED_OBSERVATION_COUNT(MarkWorkSampleCount, YumiGCObservationMarkWork)
YU_GC_NAMED_OBSERVATION_PERCENTILE(MarkWorkP95Nanos, YumiGCObservationMarkWork, 95)
YU_GC_NAMED_OBSERVATION_PERCENTILE(MarkWorkP99Nanos, YumiGCObservationMarkWork, 99)
YU_GC_NAMED_OBSERVATION_MAX(MarkWorkMaxNanos, YumiGCObservationMarkWork)
YU_GC_NAMED_OBSERVATION_COUNT(
    MarkTerminationSampleCount,
    YumiGCObservationMarkTermination)
YU_GC_NAMED_OBSERVATION_PERCENTILE(
    MarkTerminationP99Nanos,
    YumiGCObservationMarkTermination,
    99)
YU_GC_NAMED_OBSERVATION_MAX(
    MarkTerminationMaxNanos,
    YumiGCObservationMarkTermination)
YU_GC_NAMED_OBSERVATION_COUNT(SweepSampleCount, YumiGCObservationSweep)
YU_GC_NAMED_OBSERVATION_PERCENTILE(SweepP99Nanos, YumiGCObservationSweep, 99)
YU_GC_NAMED_OBSERVATION_MAX(SweepMaxNanos, YumiGCObservationSweep)
YU_GC_NAMED_OBSERVATION_COUNT(ScavengeSampleCount, YumiGCObservationScavenge)
YU_GC_NAMED_OBSERVATION_PERCENTILE(ScavengeP99Nanos, YumiGCObservationScavenge, 99)
YU_GC_NAMED_OBSERVATION_MAX(ScavengeMaxNanos, YumiGCObservationScavenge)
YU_GC_NAMED_OBSERVATION_COUNT(
    AllocationStallSampleCount,
    YumiGCObservationAllocationStall)
YU_GC_NAMED_OBSERVATION_COUNT(
    FallbackAllocationSampleCount,
    YumiGCObservationFallbackAllocation)
YU_GC_NAMED_OBSERVATION_COUNT(HandshakeSampleCount, YumiGCObservationHandshake)
YU_GC_NAMED_OBSERVATION_PERCENTILE(HandshakeP99Nanos, YumiGCObservationHandshake, 99)
YU_GC_NAMED_OBSERVATION_MAX(HandshakeMaxNanos, YumiGCObservationHandshake)
YU_GC_NAMED_OBSERVATION_COUNT(
    StackMapLookupSampleCount,
    YumiGCObservationStackMapLookup)
YU_GC_NAMED_OBSERVATION_PERCENTILE(
    StackMapLookupP99Nanos,
    YumiGCObservationStackMapLookup,
    99)

#undef YU_GC_NAMED_OBSERVATION_COUNT
#undef YU_GC_NAMED_OBSERVATION_PERCENTILE
#undef YU_GC_NAMED_OBSERVATION_MAX

extern "C" size_t
yumiGCRuntimePauseCount() {
    return gcPauseCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRuntimePauseEventCount() {
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    return gcPauseEventStored;
}

static YumiGCPauseEvent
yumiGCRuntimePauseEventAt(size_t index) {
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    if (index >= gcPauseEventStored) {
        return YumiGCPauseEvent {};
    }
    size_t firstSequence = gcPauseEventSequence - gcPauseEventStored;
    size_t eventIndex = (firstSequence + index) % kYumiGCPauseEventCapacity;
    return gcPauseEvents[eventIndex];
}

extern "C" size_t
yumiGCRuntimePauseEventCycleId(size_t index) {
    return yumiGCRuntimePauseEventAt(index).cycleId;
}

extern "C" size_t
yumiGCRuntimePauseEventKind(size_t index) {
    return yumiGCRuntimePauseEventAt(index).kind;
}

extern "C" size_t
yumiGCRuntimePauseEventStartNanos(size_t index) {
    return static_cast<size_t>(yumiGCRuntimePauseEventAt(index).startNanos);
}

extern "C" size_t
yumiGCRuntimePauseEventDurationNanos(size_t index) {
    return yumiGCRuntimePauseEventAt(index).durationNanos;
}

extern "C" size_t
yumiGCRuntimePauseEventTasksWaited(size_t index) {
    return yumiGCRuntimePauseEventAt(index).tasksWaited;
}

extern "C" size_t
yumiGCRuntimePauseEventSlowestTaskNanos(size_t index) {
    return yumiGCRuntimePauseEventAt(index).slowestTaskNanos;
}

extern "C" size_t
yumiGCRuntimePauseP50Nanos(size_t kind) {
    return yumiGCPausePercentile(kind, 50);
}

extern "C" size_t
yumiGCRuntimePauseP95Nanos(size_t kind) {
    return yumiGCPausePercentile(kind, 95);
}

extern "C" size_t
yumiGCRuntimePauseP99Nanos(size_t kind) {
    return yumiGCPausePercentile(kind, 99);
}

extern "C" size_t
yumiGCRuntimePauseMaxNanos(size_t kind) {
    if (!yumiGCValidPauseKind(kind)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    size_t maximum = 0;
    size_t firstSequence = gcPauseEventSequence - gcPauseEventStored;
    for (size_t index = 0; index < gcPauseEventStored; ++index) {
        const YumiGCPauseEvent& event =
            gcPauseEvents[(firstSequence + index) % kYumiGCPauseEventCapacity];
        if (event.kind == kind) {
            maximum = std::max(maximum, event.durationNanos);
        }
    }
    return maximum;
}

extern "C" size_t
yumiGCRuntimeCycleSummaryCount() {
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    return gcCycleSummaryStored;
}

static YumiGCCycleSummary
yumiGCRuntimeCycleSummaryAt(size_t index) {
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    if (index >= gcCycleSummaryStored) {
        return YumiGCCycleSummary {};
    }
    size_t firstSequence = gcCycleSummarySequence - gcCycleSummaryStored;
    return gcCycleSummaries[(firstSequence + index) % kYumiGCCycleSummaryCapacity];
}

#define YU_GC_CYCLE_SUMMARY_ACCESSOR(name, field) \
    extern "C" size_t name(size_t index) { \
        return yumiGCRuntimeCycleSummaryAt(index).field; \
    }

YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryCycleId, cycleId)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryTriggerReason, triggerReason)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryHeapAtTrigger, heapAtTrigger)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryHeapGoal, heapGoal)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryHeapActual, heapActual)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryMarkDurationNanos, markDurationNanos)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryScanWork, scanWork)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryAssistWork, assistWork)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryBackgroundWork, backgroundWork)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummarySweepPages, sweepPages)
YU_GC_CYCLE_SUMMARY_ACCESSOR(yumiGCRuntimeCycleSummaryScavengedPages, scavengedPages)

#undef YU_GC_CYCLE_SUMMARY_ACCESSOR

static bool
yumiAllocSizeOverflow(size_t itemSize, size_t count) {
    return count != 0 && itemSize > std::numeric_limits<size_t>::max() / count;
}

// **********************************************************************************************

extern "C" bool
yumiGCAllocationSiteKeyInit(
    YumiGCAllocationSiteKey* key,
    size_t requestedSize,
    void* descriptor,
    size_t pointerKind,
    size_t zeroingPolicy
) {
    return yumiGCInitAllocationSiteKey(
        key,
        requestedSize,
        descriptor,
        pointerKind,
        zeroingPolicy);
}

extern "C" void
yumiGCSetActiveThreadCache(YumiGCThreadCache* cache) {
    bool hasOwner = cache != nullptr;
    YumiGCThreadCache* previous = yumiGCEnsureActiveThreadCache();
    if (previous != cache) {
        yumiWBBufFlush();
        yumiFlushGCCompilerAllocationBuffer();
    }
    if (!cache) {
        cache = &gcFallbackThreadCache;
        if (
            cache->abiVersion != YumiGCAllocationABIVersion ||
            cache->structSize != sizeof(YumiGCThreadCache)
        ) {
            *cache = YumiGCThreadCache {};
            yumiGCInitializeThreadCache(cache);
        }
    }
    if (
        cache->abiVersion != YumiGCAllocationABIVersion ||
        cache->structSize != sizeof(YumiGCThreadCache)
    ) {
        std::fprintf(stderr, "Yumi GC thread-cache ABI mismatch\n");
        std::abort();
    }
    yumiGCActiveThreadCache = cache;
    // Register observability storage at thread/task attachment, not on the first hot barrier.
    yumiGCEnsureThreadBarrierStats();
    cache->ownerEpoch += 1;
    gcActiveAllocationBufferStorage = hasOwner ? &cache->activeBuffer : nullptr;
    yumiGCActiveAllocationBuffer = hasOwner ? &cache->activeBuffer.buffer : nullptr;
}

extern "C" YumiGCThreadCache*
yumiGCGetActiveThreadCache() {
    return yumiGCEnsureActiveThreadCache();
}

extern "C" bool
yumiGCTestThreadCacheOwnerFlushOnce() {
    YumiGCThreadCache* previous = yumiGCEnsureActiveThreadCache();
    yumiFlushGCCompilerAllocationBuffer();

    YumiGCAllocationBatch batch {};
    if (!yumiGCTestAllocBatch(16, nullptr, 4, &batch) || !batch.cursor || !batch.slotSize) {
        return false;
    }

    YumiGCThreadCache cache {};
    yumiGCInitializeThreadCache(&cache);
    yumiGCSetActiveThreadCache(&cache);
    YumiGCRuntimeAllocationBuffer& storage = cache.compilerBuffers[0];
    storage.base = batch.cursor;
    storage.buffer.cursor = batch.cursor + batch.slotSize;
    storage.buffer.limit = batch.limit;
    storage.slotSize = batch.slotSize;
    storage.requestedSize = 16;
    storage.generation = yumiGCLoadPublicSweepGeneration();
    storage.sizeClassIndex = 2;
    cache.activeCompilerBufferCount = 1;
    cache.activeCompilerBufferIndices[0] = 0;

    size_t before = gcThreadCacheOwnerFlushCount.load(std::memory_order_relaxed);
    yumiGCSetActiveThreadCache(nullptr);
    size_t after = gcThreadCacheOwnerFlushCount.load(std::memory_order_relaxed);
    size_t flushEpoch = cache.flushEpoch;
    yumiGCSetActiveThreadCache(nullptr);
    bool unchanged =
        gcThreadCacheOwnerFlushCount.load(std::memory_order_relaxed) == after;
    yumiGCSetActiveThreadCache(previous);
    return after == before + 1 && flushEpoch == 1 && unchanged;
}

extern "C" size_t
yumiGCTestAdaptiveBatchPreferredCount(size_t compilerBufferIndex) {
    if (!yumiGCCompilerAllocationBufferIndexIsValid(compilerBufferIndex)) {
        return 0;
    }
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    return cache->compilerBuffers[compilerBufferIndex - 1]
        .reserved[kYumiGCAdaptiveCountByte];
}

extern "C" size_t
yumiGCTestParkedAllocationRangeCount(size_t sizeClassIndex) {
    if (sizeClassIndex >= YumiGCThreadCacheRangeCount) {
        return 0;
    }
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    size_t count = 0;
    for (size_t way = 0; way < YumiGCThreadCacheRangeWayCount; ++way) {
        const YumiGCRuntimeAllocationBuffer& storage =
            yumiGCThreadRangeWay(cache, sizeClassIndex, way);
        if (yumiGCAllocationRangeHasOwnership(storage) ||
                yumiGCAllocationRangeHasPolicySite(storage)) {
            count += 1;
        }
    }
    return count;
}

extern "C" size_t
yumiGCTestCompilerRangePacerChargedBytes() {
    return gcCompilerRangePacerChargedBytes;
}

extern "C" size_t
yumiGCTestGenericRangePacerChargedBytes() {
    return gcHotAllocationState.genericPacerChargedBytes;
}

extern "C" size_t
yumiGCTestGenericRangePacerCollectionCheckCount() {
    return gcHotAllocationState.genericPacerCollectionChecks;
}

extern "C" void
yumiGCTestCrossAllocationStatPublicationThreshold() {
    gcHotAllocationState.stats.allocatedObjects = kYumiGCThreadStatFlushObjectCount - 1;
    yumiRecordGCPublicAllocation(gcHotAllocationState, 1, yumiGCObjectAllocationSize(1));
}

extern "C" void
yumiGCSetActiveAllocationBufferStorage(YumiGCRuntimeAllocationBuffer* storage) {
    if (gcActiveAllocationBufferStorage && gcActiveAllocationBufferStorage != storage) {
        yumiFlushGCCompilerAllocationBuffer(*gcActiveAllocationBufferStorage);
    }
    if (storage && (
            storage->abiVersion != YumiGCAllocationABIVersion ||
            storage->structSize != sizeof(YumiGCRuntimeAllocationBuffer))) {
        storage = nullptr;
    }
    gcActiveAllocationBufferStorage = storage;
    yumiGCActiveAllocationBuffer = storage ? &storage->buffer : nullptr;
}

extern "C" YumiGCAllocationBuffer*
yumiGCGetActiveAllocationBuffer() {
    return yumiGCActiveAllocationBuffer;
}

extern "C" size_t
yumiGCAllocationBufferCursorOffset() {
    return YumiGCAllocationBufferCursorOffset;
}

extern "C" size_t
yumiGCAllocationBufferLimitOffset() {
    return YumiGCAllocationBufferLimitOffset;
}

extern "C" size_t
yumiGCAllocationBufferSize() {
    return YumiGCAllocationBufferSize;
}

extern "C" size_t
yumiGCRuntimeAllocationBufferHotOffset() {
    return YumiGCRuntimeAllocationBufferHotOffset;
}

extern "C" void
yumiGCAllocationBufferFlush() {
    yumiFlushGCCompilerAllocationBuffer();
    yumiPublishGCThreadAllocationStats();
}

extern "C" void
yumiGCAllocationBufferPublish() {
    yumiPublishGCCompilerAllocationBufferConsumptions();
}

extern "C" void
yumiGCAllocationBufferRevoke() {
    yumiFlushGCCompilerAllocationBuffer();
}

static bool
yumiGCHandoffBatchToThreadRange(
    const YumiGCAllocationSiteKey& key,
    size_t generation,
    YumiGCRuntimeAllocationBuffer& destination,
    bool& reservedBatch
) {
    if (key.sizeClassIndex >= YumiGCThreadCacheRangeCount) {
        return false;
    }
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    bool destinationMatches = yumiGCAllocationRangeMatchesSite(
        destination,
        key.requestedSize,
        key.allocatedSize,
        key.descriptor,
        generation);
    if (!destinationMatches) {
        yumiGCTakeParkedGCAllocationRange(cache, key, generation, destination);
    }

    if (destination.buffer.cursor && destination.buffer.limit &&
            destination.buffer.cursor < destination.buffer.limit) {
        reservedBatch = false;
        return true;
    }

    size_t selectedCount = yumiGCAdaptiveAllocationBatchCount(destination, key);
    yumiGCPrepareCompilerRangeVerificationAllocation(key, selectedCount);
    uint64_t reservedControl = yumiGCLoadControlWord();
    YumiGCThreadAllocationBatch batch {};
    if (!yumiRefillCachedGCAllocationBatch(
            batch,
            key.requestedSize,
            key.allocatedSize,
            key.descriptor,
            generation,
            selectedCount)) {
        return false;
    }
    yumiGCChargeCompilerRangePacerBytes(
        static_cast<size_t>(batch.limit - batch.cursor));
    uint64_t chargedControl = yumiGCLoadControlWord();
    if (yumiGCControlWordPhase(chargedControl) != yumiGCControlWordPhase(reservedControl) ||
            yumiGCControlWordSweepGeneration(chargedControl) != generation) {
        if (!yumiReleaseCachedGCAllocationBatch(batch)) {
            yumiDiscardCachedGCAllocationBatch(batch);
        }
        return false;
    }

    unsigned char adaptivePolicy[sizeof(destination.reserved)] {};
    std::memcpy(adaptivePolicy, destination.reserved, sizeof(adaptivePolicy));
    yumiGCInitializeEmptyRuntimeAllocationBuffer(destination);
    std::memcpy(destination.reserved, adaptivePolicy, sizeof(adaptivePolicy));
    destination.buffer.cursor = batch.cursor;
    destination.buffer.limit = batch.limit;
    destination.base = batch.cursor;
    destination.slotSize = batch.slotSize;
    destination.requestedSize = key.requestedSize;
    destination.descriptor = key.descriptor;
    destination.generation = generation;
    destination.sizeClassIndex = key.sizeClassIndex;
    destination.pointerKind = key.pointerKind;
    destination.zeroingPolicy = key.zeroingPolicy;
    destination.reserved[kYumiGCAdaptiveEnabledByte] = key.descriptor ? 1 : 0;
    destination.reserved[kYumiGCAdaptiveCountByte] = static_cast<unsigned char>(
        std::min(selectedCount, kYumiGCAdaptiveAllocationBatchMaxCount));
    destination.reserved[kYumiGCAdaptiveReservedSlotCountByte] =
        static_cast<unsigned char>(std::min(
            static_cast<size_t>((batch.limit - batch.cursor) / batch.slotSize),
            kYumiGCAdaptiveAllocationBatchMaxCount));
    cache->counters[kYumiGCThreadCounterCompilerRefills] += 1;
    reservedBatch = true;
    return true;
}

static void
yumiGCPrepareCompilerRangeVerificationAllocation(
    const YumiGCAllocationSiteKey& key,
    size_t selectedCount
) {
    // Verification modes retain one callback per logical allocation before the refill because
    // their allocation intervals and replay choices are part of the requested deterministic trace.
    if (yumiGCVerificationSlowPathsEnabled()) {
        for (size_t index = 0; index < selectedCount; ++index) {
            yumiGCCollectForAllocationIfNeeded(key.allocatedSize);
        }
    }
}

static void
yumiGCChargeCompilerRangePacerBytes(size_t deliveredBytes) {
    // Inline bump allocations intentionally perform no collector call. Bill the bytes actually
    // handed to the range at refill frequency without putting work back on the object fast path.
    if (yumiGCVerificationSlowPathsEnabled()) {
        return;
    }
    gcCompilerRangePacerChargedBytes =
        deliveredBytes > std::numeric_limits<size_t>::max() -
                gcCompilerRangePacerChargedBytes
            ? std::numeric_limits<size_t>::max()
            : gcCompilerRangePacerChargedBytes + deliveredBytes;
    gcCompilerRangePacerPendingBytes =
        deliveredBytes > std::numeric_limits<size_t>::max() - gcCompilerRangePacerPendingBytes
            ? std::numeric_limits<size_t>::max()
            : gcCompilerRangePacerPendingBytes + deliveredBytes;
    if (gcCompilerRangePacerPendingBytes >= kYumiGCCompilerRangePacerCreditBytes) {
        size_t chargedBytes = gcCompilerRangePacerPendingBytes;
        gcCompilerRangePacerPendingBytes = 0;
        yumiGCCollectForAllocationIfNeeded(chargedBytes);
    }
}

static void
yumiGCAddGenericRangePacerBytes(YumiGCHotAllocationState& hot, size_t allocatedBytes) {
    hot.genericPacerChargedBytes =
        allocatedBytes > std::numeric_limits<size_t>::max() - hot.genericPacerChargedBytes
            ? std::numeric_limits<size_t>::max()
            : hot.genericPacerChargedBytes + allocatedBytes;
    hot.genericPacerPendingBytes =
        allocatedBytes > std::numeric_limits<size_t>::max() - hot.genericPacerPendingBytes
            ? std::numeric_limits<size_t>::max()
            : hot.genericPacerPendingBytes + allocatedBytes;
}

static void
yumiGCChargeGenericRangePacerBytes(YumiGCHotAllocationState& hot, size_t allocatedBytes) {
    // Verification modes define behavior at every logical allocation. Ordinary pacing only
    // needs byte-granular progress no coarser than the existing 64 KiB compiler-range credit.
    if (yumiGCVerificationSlowPathsEnabled()) {
        yumiGCCollectForAllocationIfNeeded(allocatedBytes);
        return;
    }
    yumiGCAddGenericRangePacerBytes(hot, allocatedBytes);
    if (hot.genericPacerPendingBytes >= kYumiGCGenericRangePacerCreditBytes) {
        size_t chargedBytes = hot.genericPacerPendingBytes;
        hot.genericPacerPendingBytes = 0;
        ++hot.genericPacerCollectionChecks;
        yumiGCCollectForAllocationIfNeeded(chargedBytes);
    }
}

extern "C" char*
yumiGCAllocationBufferRefill(const YumiGCAllocationSiteKey* key) {
    if (!key) {
        return nullptr;
    }

    if (!yumiGCActiveTaskCanUseFastAllocation()) {
        yumiGCDebugUnstoppableFastAllocationRejected();
        yumiFlushGCCompilerAllocationBuffer();
        return yumiAllocObjectWithSiteKey(key);
    }

    if (!gcActiveAllocationBufferStorage || !yumiGCAllocationSiteKeyIsValid(key)) {
        yumiFlushGCCompilerAllocationBuffer();
        return yumiAllocObjectWithSiteKey(key);
    }

    yumiGCSafepointIfRequested();
    yumiEnsureGCInitialized();
    YumiGCRuntimeAllocationBuffer& storage = *gcActiveAllocationBufferStorage;
    yumiFlushGCCompilerAllocationBuffer(storage);

    unsigned char phase = yumiGCLoadPublicPhase();
    if (!yumiGCPhaseAllowsFastBatchAllocation(phase)) {
        yumiFlushGCCompilerAllocationBuffer();
        return yumiAllocObjectWithSiteKey(key);
    }

    size_t generation = yumiGCLoadPublicSweepGeneration();
    bool reservedBatch = false;
    if (!yumiGCHandoffBatchToThreadRange(
            *key,
            generation,
            storage,
            reservedBatch)) {
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return yumiAllocObjectWithSiteKey(key);
    }

    if (
        generation != yumiGCLoadPublicSweepGeneration() ||
        !yumiGCPhaseAllowsFastBatchAllocation(yumiGCLoadPublicPhase())
    ) {
        if (generation != yumiGCLoadPublicSweepGeneration()) {
            yumiGCResetAdaptiveAllocationBufferPolicy(storage);
        }
        yumiFlushGCCompilerAllocationBuffer(storage);
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return yumiAllocObjectWithSiteKey(key);
    }

    if (!storage.buffer.cursor || storage.buffer.cursor >= storage.buffer.limit) {
        yumiClearGCCompilerAllocationBuffer(storage);
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return yumiAllocObjectWithSiteKey(key);
    }

    char* pointer = storage.buffer.cursor;
    storage.buffer.cursor += storage.slotSize;
    if (reservedBatch) {
        gcHotAllocationState.stats.allocationFastRefillCount += 1;
        gcHotAllocationState.stats.activeBufferRefillCount += 1;
        gcHotAllocationState.stats.allocationSiteRefillCount += 1;
    } else {
        gcHotAllocationState.stats.allocationFastHitCount += 1;
        gcHotAllocationState.stats.allocationSiteHitCount += 1;
    }
    return pointer;
}

extern "C" char*
yumiGCCompilerAllocationBufferRefill(const YumiGCAllocationSiteKey* key) {
    if (!key) {
        return nullptr;
    }

    if (!yumiGCActiveTaskCanUseFastAllocation()) {
        yumiGCDebugUnstoppableFastAllocationRejected();
        yumiFlushGCCompilerAllocationBuffer();
        return yumiAllocObjectWithSiteKey(key);
    }

    if (!yumiGCAllocationSiteKeyIsValid(key)) {
        yumiFlushGCCompilerAllocationBuffer();
        return yumiAllocObjectWithSiteKey(key);
    }

    if (!yumiGCCompilerAllocationBufferIndexIsValid(key->compilerBufferIndex)) {
        return yumiAllocObjectWithSiteKey(key);
    }

    yumiGCSafepointIfRequested();
    yumiEnsureGCInitialized();
    size_t slot = key->compilerBufferIndex - 1;
    YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
    YumiGCRuntimeAllocationBuffer& storage = cache->compilerBuffers[slot];
    size_t generation = yumiGCLoadPublicSweepGeneration();
    bool sameSite =
        yumiGCAllocationRangeMatchesSite(
            storage,
            key->requestedSize,
            key->allocatedSize,
            key->descriptor,
            generation);
    if (!sameSite &&
            (yumiGCAllocationRangeHasOwnership(storage) ||
             yumiGCAllocationRangeHasPolicySite(storage))) {
        gcCompilerBufferSiteSwitchCount.fetch_add(1, std::memory_order_relaxed);
    }
    if (sameSite) {
        yumiFlushGCCompilerAllocationBuffer(storage);
    } else {
        yumiGCParkCompilerGCAllocationRange(cache, storage, generation);
    }

    unsigned char phase = yumiGCLoadPublicPhase();
    if (!yumiGCPhaseAllowsFastBatchAllocation(phase)) {
        yumiFlushGCCompilerAllocationBuffer();
        return yumiAllocObjectWithSiteKey(key);
    }

    generation = yumiGCLoadPublicSweepGeneration();
    if (yumiGCAllocationRangeHasPolicySite(storage) &&
            storage.generation != generation) {
        yumiGCClearStagedGCAllocationRange(storage);
    }
    bool reservedBatch = false;
    if (!yumiGCHandoffBatchToThreadRange(
            *key,
            generation,
            storage,
            reservedBatch)) {
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return yumiAllocObjectWithSiteKey(key);
    }
    yumiMarkGCCompilerAllocationBufferActive(key->compilerBufferIndex);

    if (
        generation != yumiGCLoadPublicSweepGeneration() ||
        !yumiGCPhaseAllowsFastBatchAllocation(yumiGCLoadPublicPhase())
    ) {
        if (generation != yumiGCLoadPublicSweepGeneration()) {
            yumiGCResetAdaptiveAllocationBufferPolicy(storage);
        }
        yumiFlushGCCompilerAllocationBuffer(storage);
        for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
            if (cache->activeCompilerBufferIndices[active] == slot) {
                cache->activeCompilerBufferIndices[active] =
                    cache->activeCompilerBufferIndices[--cache->activeCompilerBufferCount];
                break;
            }
        }
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return yumiAllocObjectWithSiteKey(key);
    }

    if (!storage.buffer.cursor || storage.buffer.cursor >= storage.buffer.limit) {
        yumiClearGCCompilerAllocationBuffer(storage);
        for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
            if (cache->activeCompilerBufferIndices[active] == slot) {
                cache->activeCompilerBufferIndices[active] =
                    cache->activeCompilerBufferIndices[--cache->activeCompilerBufferCount];
                break;
            }
        }
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return yumiAllocObjectWithSiteKey(key);
    }

    char* pointer = storage.buffer.cursor;
    storage.buffer.cursor += storage.slotSize;
    if (reservedBatch) {
        gcHotAllocationState.stats.allocationFastRefillCount += 1;
        gcHotAllocationState.stats.compilerIndexedRefillCount += 1;
        gcHotAllocationState.stats.allocationSiteRefillCount += 1;
    } else {
        gcHotAllocationState.stats.allocationFastHitCount += 1;
        gcHotAllocationState.stats.allocationSiteHitCount += 1;
    }
    return pointer;
}

// **********************************************************************************************

extern "C" char*
yumiAllocObject(size_t size) {
    YumiGCHotAllocationState& hot = gcHotAllocationState;
    yumiGCRecordPublicAllocationCall(hot, YumiGCAllocationCallObject);
    yumiGCSafepointIfRequested();
    yumiEnsureGCInitialized();
    yumiGCChargeGenericRangePacerBytes(hot, yumiGCObjectAllocationSize(size));
    if (char* fast = yumiTryFastGCAllocation(hot, size, nullptr)) {
        return fast;
    }
    return yumiCallGCFallbackAllocation([&]() {
        return yumiGCAllocObject(size);
    });
}

extern "C" char*
yumiAllocObjectWithSiteKey(const YumiGCAllocationSiteKey* key) {
    if (!key) {
        return nullptr;
    }

    bool validKey = yumiGCAllocationSiteKeyIsValid(key);
    YumiGCHotAllocationState& hot = gcHotAllocationState;
    yumiGCRecordPublicAllocationCall(hot, key->descriptor
        ? YumiGCAllocationCallDescribedObject
        : YumiGCAllocationCallObject);
    yumiGCSafepointIfRequested();
    yumiEnsureGCInitialized();
    yumiGCChargeGenericRangePacerBytes(hot, yumiGCObjectAllocationSize(key->requestedSize));
    if (validKey) {
        if (char* fast = yumiTryFastGCAllocationWithSiteKey(hot, *key)) {
            return fast;
        }
    } else {
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        if (char* fast = yumiTryFastGCAllocation(hot, key->requestedSize, key->descriptor)) {
            return fast;
        }
    }

    if (!key->descriptor) {
        return yumiCallGCFallbackAllocation([&]() {
            return yumiGCAllocObject(key->requestedSize);
        });
    }
    return yumiCallGCFallbackAllocation([&]() {
        return yumiGCAllocObjectDescribed(key->requestedSize, key->descriptor);
    });
}

extern "C" char*
yumiAllocObjectDescribed(size_t size, void* descriptor) {
    YumiGCHotAllocationState& hot = gcHotAllocationState;
    yumiGCRecordPublicAllocationCall(hot, YumiGCAllocationCallDescribedObject);
    yumiGCSafepointIfRequested();
    yumiEnsureGCInitialized();
    yumiGCChargeGenericRangePacerBytes(hot, yumiGCObjectAllocationSize(size));
    if (!descriptor) {
        if (char* fast = yumiTryFastGCAllocation(hot, size, nullptr)) {
            return fast;
        }
        return yumiCallGCFallbackAllocation([&]() {
            return yumiGCAllocObject(size);
        });
    }
    if (char* fast = yumiTryFastGCAllocation(hot, size, descriptor)) {
        return fast;
    }
    return yumiCallGCFallbackAllocation([&]() {
        return yumiGCAllocObjectDescribed(size, descriptor);
    });
}

extern "C" char*
yumiAllocBuffer(size_t itemSize, size_t count) {
    if (yumiAllocSizeOverflow(itemSize, count)) {
        return NULL;
    }

    size_t size = count * itemSize;
    if (size == 0) {
        return NULL;
    }

    YumiGCHotAllocationState& hot = gcHotAllocationState;
    yumiGCRecordPublicAllocationCall(hot, YumiGCAllocationCallBuffer);
    yumiGCSafepointIfRequested();
    yumiEnsureGCInitialized();
    yumiGCChargeGenericRangePacerBytes(hot, yumiGCObjectAllocationSize(size));
    if (char* fast = yumiTryFastGCAllocation(hot, size, nullptr)) {
        return fast;
    }
    return yumiCallGCFallbackAllocation([&]() {
        return yumiGCAllocBuffer(itemSize, count);
    });
}

extern "C" char*
yumiTryAllocBufferWithoutSafepoint(size_t size) {
    if (size == 0 || !gcInitialized.load(std::memory_order_acquire) ||
            !yumiGCActiveTaskCanUseFastAllocation() ||
            yumiGCVerificationSlowPathsEnabled()) {
        return nullptr;
    }
    size_t allocSize = yumiGCObjectAllocationSize(size);
    if (allocSize > kYumiGCAllocationBatchMaxSize) {
        return nullptr;
    }
    size_t index = yumiGCAllocationBatchIndex(allocSize);
    if (index >= kYumiGCAllocationBatchCount) {
        return nullptr;
    }
    YumiGCHotAllocationState& hot = gcHotAllocationState;
    if (hot.genericPacerPendingBytes + allocSize >= kYumiGCGenericRangePacerCreditBytes) {
        return nullptr;
    }
    uint64_t control = yumiGCLoadControlWord();
    if (yumiGCControlWordHandshakeRequested(control)) {
        return nullptr;
    }
    YumiGCRuntimeAllocationBuffer& range = yumiGCEnsureActiveThreadCache()->ranges[index];
    char* result = range.buffer.cursor;
    if (range.generation != yumiGCControlWordSweepGeneration(control) ||
            range.descriptor ||
            range.slotSize < allocSize ||
            !result ||
            !range.buffer.limit ||
            result >= range.buffer.limit) {
        return nullptr;
    }
    yumiGCRecordPublicAllocationCall(hot, YumiGCAllocationCallBuffer);
    yumiGCRecordAllocationDescriptorMix(hot, index, nullptr);
    yumiGCAddGenericRangePacerBytes(hot, allocSize);
    range.buffer.cursor += range.slotSize;
    range.base = range.buffer.cursor;
    hot.stats.zeroedBytes += range.slotSize;
    hot.stats.batchConsumedBytes += range.slotSize;
    hot.batchCacheBytes = hot.batchCacheBytes > range.slotSize
        ? hot.batchCacheBytes - range.slotSize
        : 0;
    yumiRecordGCPublicAllocation(hot, size, range.slotSize);
    hot.stats.allocationFastHitCount += 1;
    return result;
}

extern "C" char*
yumiAllocBufferDescribed(size_t itemSize, size_t count, void* descriptor) {
    if (yumiAllocSizeOverflow(itemSize, count)) {
        return NULL;
    }

    size_t size = count * itemSize;
    if (size == 0) {
        return NULL;
    }

    YumiGCHotAllocationState& hot = gcHotAllocationState;
    yumiGCRecordPublicAllocationCall(hot, YumiGCAllocationCallDescribedBuffer);
    yumiGCSafepointIfRequested();
    yumiEnsureGCInitialized();
    yumiGCChargeGenericRangePacerBytes(hot, yumiGCObjectAllocationSize(size));
    if (!descriptor) {
        if (char* fast = yumiTryFastGCAllocation(hot, size, nullptr)) {
            return fast;
        }
        return yumiCallGCFallbackAllocation([&]() {
            return yumiGCAllocBuffer(itemSize, count);
        });
    }
    if (char* fast = yumiTryFastGCAllocation(hot, size, descriptor)) {
        return fast;
    }
    return yumiCallGCFallbackAllocation([&]() {
        return yumiGCAllocBufferDescribed(itemSize, count, descriptor);
    });
}

extern "C" char*
yumiResizeBuffer(char* buf, size_t oldSize, size_t newSize) {
    return yumiResizeBufferDescribed(buf, oldSize, newSize, nullptr);
}

extern "C" char*
yumiResizeBufferDescribed(char* buf, size_t oldSize, size_t newSize, void* descriptor) {
    if (buf && yumiParseRegionOwns(buf)) {
        return yumiParseRegionResizeBuffer(buf, oldSize, newSize, descriptor);
    }
    YumiGCHotAllocationState& hot = gcHotAllocationState;
    yumiGCRecordPublicAllocationCall(hot, descriptor
        ? YumiGCAllocationCallDescribedResizeBuffer
        : YumiGCAllocationCallResizeBuffer);
    if (newSize == 0) {
        return NULL;
    }

    yumiEnsureGCInitialized();
    YumiGCTemporaryRoot oldBufferRoot((buf && oldSize > 0) ? buf : nullptr);
    yumiGCSafepointIfRequested();
    if (buf && oldSize > 0 && newSize <= oldSize && !descriptor) {
        return buf;
    }

    if (!buf || oldSize == 0 || newSize > oldSize) {
        yumiGCChargeGenericRangePacerBytes(hot, yumiGCObjectAllocationSize(newSize));
    }
    bool copiesOldBuffer = buf && oldSize > 0;
    void* replacementDescriptor = descriptor;
    if (copiesOldBuffer) {
        YumiGCBufferResize resize = yumiCallGCIgnoringPanic([&]() {
            return yumiGCResizeBufferPrepare(
                buf,
                oldSize,
                newSize,
                descriptor,
                &replacementDescriptor);
        });
        if (resize == YumiGCBufferResizeRejected) {
            return NULL;
        }
        if (resize == YumiGCBufferResizeInPlace) {
            return buf;
        }
    }

    char* replacement = yumiTryFastGCAllocation(hot, newSize, replacementDescriptor);
    if (!replacement) {
        replacement = yumiCallGCFallbackAllocation([&]() {
            return yumiGCAllocBufferDescribed(1, newSize, replacementDescriptor);
        });
    }
    if (replacement && copiesOldBuffer) {
        yumiCallGCIgnoringPanic([&]() {
            yumiGCResizeBufferCopy(replacement, buf, oldSize, newSize, replacementDescriptor);
        });
    }
    return replacement;
}

extern "C" void*
yumiAddRoot(char* pointer) {
    yumiEnsureGCInitialized();
    // Root publication is not a collection boundary; collection entry points flush batches.
    return yumiCallGCIgnoringPanic([&]() {
        return yumiGCAddRoot(pointer);
    });
}

extern "C" void*
yumiAddTemporaryRoot(char* pointer) {
    yumiEnsureGCInitialized();
    return yumiGCPushTemporaryRoot(pointer);
}

extern "C" void*
yumiAddDescribedRoot(char* pointer, size_t byteSize, void* descriptor) {
    if (yumiGCCollectionDisabled()) {
        return nullptr;
    }
    yumiEnsureGCInitialized();
    return yumiCallGCIgnoringPanic([&]() {
        return yumiGCAddDescribedRoot(pointer, byteSize, descriptor);
    });
}

extern "C" void
yumiUpdateRoot(void* root, char* pointer) {
    yumiEnsureGCInitialized();
    yumiCallGCIgnoringPanic([&]() {
        yumiGCUpdateRoot(root, pointer);
    });
}

extern "C" void
yumiRemoveRoot(void* root) {
    yumiEnsureGCInitialized();
    yumiCallGCIgnoringPanic([&]() {
        yumiGCRemoveRoot(root);
    });
}

extern "C" void
yumiRemoveTemporaryRoot(void* root) {
    yumiEnsureGCInitialized();
    yumiGCPopTemporaryRoot(root);
}

extern "C" void
yumiDrainTemporaryRoots(YumiGCTemporaryRootStorage* storage) {
    yumiGCDrainTemporaryRoots(storage);
}

extern "C" YumiGCCollectResult
yumiCollect() {
    yumiGCSafepointIfRequested();
    if (yumiGCCollectionDisabled() || yumiGCThreadHasLocalCallbackRegistration()) {
        return YumiGCCollectCompleted;
    }
    yumiEnsureGCInitialized();
    yumiFlushGCAllocationBatches();
    while (!yumiGCTryBeginCollectionWork()) {
        // An automatic worker owns a bounded collector operation. Participate at safepoints
        // and yield until that operation publishes the collector admission token.
        yumiGCSafepointIfRequested();
        sched_yield();
    }
    gcCollectionMayGrowRequestedGoal.store(false, std::memory_order_release);
    gcPendingCycleTriggerReason.store(YumiGCPacerTriggerExplicit, std::memory_order_release);
    yumiCallGCIgnoringPanic([]() {
        if (yumiGCLoadPublicPhase() != kYumiGCPhaseIdle) {
            yumiGCDrainAutomaticCycleBlocking();
        } else {
            yumiGCCollect();
        }
    });
    yumiGCFinishAutomaticGrowthWindowIfIdle();
    yumiGCEndCollection();
    return YumiGCCollectCompleted;
}

extern "C" void
yumiWriteBarrier(char* source, char* value) {
    if (yumiGCIsRuntimeCritical()) {
        return;
    }

    yumiEnsureGCInitialized();
    yumiCallGCIgnoringPanic([&]() {
        yumiGCWriteBarrier(source, value);
    });
}

extern "C" void
yumiWriteBarrierRecord(char* source, char* value) {
    if (!source && !value) {
        return;
    }
    if (yumiGCIsRuntimeCritical()) {
        return;
    }

    unsigned char phase = yumiGCLoadPublicPhase();
    if (yumiGCPhaseNeedsMarkBarrier(phase) &&
            yumiGCActiveTaskCanUseFastAllocation()) {
        YumiGCThreadCache* cache = yumiGCEnsureActiveThreadCache();
#if !YU_ENABLE_BENCH_METRICS
        // Instrumented compiler builds count every attempted barrier before the inline phase
        // check. Ordinary builds have no such call, so count the non-Idle record entry here.
        cache->counters[kYumiGCThreadCounterBarrierCalls] += 1;
#endif
        cache->counters[kYumiGCThreadCounterBarrierSlowEntries] += 1;
        char* current = source ? *reinterpret_cast<char**>(source) : nullptr;
        yumiWBBufRecordActiveTaskPointer(current);
        yumiWBBufRecordActiveTaskPointer(value);
        return;
    }

    if (!yumiGCPhaseNeedsSweepPublicationValidation(phase) &&
            !yumiGCPhaseNeedsMarkBarrier(phase)) {
        return;
    }

    // Native, FFI-blocked, and sweep-transition callers cannot leave state in a thread-owned
    // buffer: use synchronous publisher admission and Sweep publication validation.
    yumiWriteBarrier(source, value);
}

extern "C" void
yumiWriteBarrierFast(char* source, char* value) {
    // This compatibility entry is no longer emitted for ordinary scalar stores; native/direct
    // callers retain exact call accounting without putting a shared RMW on any barrier path.
    YumiGCBarrierStatsRecord* stats = yumiGCEnsureThreadBarrierStats();
    yumiGCAddOwnedBarrierCounter(stats->fastCalls, 1);
    if (!source && !value) {
        return;
    }

    if (yumiGCIsRuntimeCritical()) {
        return;
    }

    unsigned char phase = yumiGCLoadPublicPhase();
    if (!yumiGCPhaseNeedsMarkBarrier(phase) &&
            !yumiGCPhaseNeedsSweepPublicationValidation(phase)) {
        return;
    }

    yumiEnsureGCInitialized();
    if (yumiGCPhaseNeedsMarkBarrier(phase) &&
            yumiGCActiveTaskCanUseFastAllocation()) {
        yumiWriteBarrierRecord(source, value);
    } else {
        yumiGCAddOwnedBarrierCounter(stats->fastSlowEntries, 1);
        yumiCallGCIgnoringPanic([&]() {
            yumiGCWriteBarrier(source, value);
        });
    }
}

extern "C" void
yumiBulkWriteBarrierFast(char* source, char* value, size_t byteSize, void* descriptor) {
    bool countsEveryCall = false;
#if YU_ENABLE_BENCH_METRICS
    countsEveryCall = yumiBenchMetricsEnabledFlag != 0;
    if (__builtin_expect(countsEveryCall, 0)) {
        yumiGCAddOwnedBarrierCounter(yumiGCEnsureThreadBarrierStats()->fastCalls, 1);
    }
#endif
    unsigned char phase = yumiGCLoadPublicPhase();
    if (!yumiGCPhaseNeedsMarkBarrier(phase) &&
            !yumiGCPhaseNeedsSweepPublicationValidation(phase)) {
        return;
    }

    YumiGCBarrierStatsRecord* stats = yumiGCEnsureThreadBarrierStats();
    if (!countsEveryCall) {
        yumiGCAddOwnedBarrierCounter(stats->fastCalls, 1);
    }
    if ((!source && !value) || byteSize == 0 || !descriptor) {
        return;
    }

    if (yumiGCIsRuntimeCritical()) {
        return;
    }

    yumiEnsureGCInitialized();
    yumiGCAddOwnedBarrierCounter(stats->fastSlowEntries, 1);
    yumiCallGCIgnoringPanic([&]() {
        yumiGCBulkWriteBarrier(source, value, byteSize, descriptor);
    });
}

extern "C" size_t
yumiGCRuntimeMode() {
    return YU_GC_NEVER_COLLECT ? 1 : 2;
}

#else

namespace {

static std::atomic<size_t> noGCAllocatedObjects {0};
static std::atomic<size_t> noGCAllocatedBytes {0};
static std::atomic<size_t> noGCCollectionObserverCount {0};
static YumiCollectionObserverFilter noGCCollectionObserverFilter;
static std::unordered_map<size_t, std::atomic<uintptr_t>*> noGCCollectionObserverSlots;
static std::mutex noGCObjectSidePayloadMutex;
using YumiNoGCObjectSidePayloads = std::array<char*, YumiObjectSidePayloadLaneCount>;
static std::unordered_map<char*, YumiNoGCObjectSidePayloads> noGCObjectSidePayloads;
static std::array<size_t, YumiObjectSidePayloadLaneCount> noGCObjectSidePayloadCounts {};
static std::mutex noGCWeakMutex;
static std::unordered_map<size_t, char*> noGCOwnerWeakTargets;
static std::atomic<size_t> noGCNextOwnerWeakHandle {1};
static std::mutex noGCCleanupMutex;
static std::unordered_set<size_t> noGCCleanupHandles;
static std::atomic<size_t> noGCNextCleanupHandle {1};
static std::atomic<size_t> noGCCleanupCancelCount {0};
static std::atomic<size_t> noGCCleanupExceptionCount {0};
static thread_local char* noGCBumpCursor = nullptr;
static thread_local char* noGCBumpLimit = nullptr;
static constexpr size_t kYumiNoGCBumpChunkSize = 1024ull * 1024ull;
static constexpr size_t kYumiNoGCBumpAlignment = 16;

static inline void
yumiNoGCRecordAllocation(size_t size) {
#if YU_ENABLE_BENCH_METRICS
    if (__builtin_expect(yumiBenchMetricsEnabledFlag, 0)) {
        yumiBenchRecordAllocation(size);
        noGCAllocatedObjects.fetch_add(1, std::memory_order_relaxed);
        noGCAllocatedBytes.fetch_add(size, std::memory_order_relaxed);
    }
#else
    (void)size;
#endif
}

static char*
yumiNoGCAllocate(size_t size) {
    yumiNoGCRecordAllocation(size);

    size_t storageSize = std::max<size_t>(size, 1);
    if (storageSize > std::numeric_limits<size_t>::max() - (kYumiNoGCBumpAlignment - 1)) {
        std::fprintf(stderr, "Yumi no-GC bump allocation size overflow\n");
        std::abort();
    }
    storageSize = (storageSize + kYumiNoGCBumpAlignment - 1) &
        ~(kYumiNoGCBumpAlignment - 1);
    size_t remaining = noGCBumpCursor && noGCBumpLimit
        ? static_cast<size_t>(noGCBumpLimit - noGCBumpCursor)
        : 0;
    if (remaining < storageSize) {
        size_t chunkSize = std::max(kYumiNoGCBumpChunkSize, storageSize);
        noGCBumpCursor = static_cast<char*>(YU_MALLOC(chunkSize));
        noGCBumpLimit = noGCBumpCursor + chunkSize;
    }

    char* buffer = noGCBumpCursor;
    noGCBumpCursor += storageSize;
    if (size) {
        std::memset(buffer, 0, size);
    }
    return buffer;
}

static bool
yumiNoGCAllocationSize(size_t itemSize, size_t count, size_t& size) {
    if (count != 0 && itemSize > std::numeric_limits<size_t>::max() / count) {
        return false;
    }
    size = itemSize * count;
    return true;
}

} // namespace

extern "C" bool
yumiGCAllocationSiteKeyInit(
    YumiGCAllocationSiteKey* key,
    size_t requestedSize,
    void* descriptor,
    size_t pointerKind,
    size_t zeroingPolicy
) {
    if (!key || pointerKind > 1 || zeroingPolicy != 0) {
        return false;
    }
    *key = YumiGCAllocationSiteKey {};
    key->abiVersion = YumiGCAllocationABIVersion;
    key->structSize = sizeof(YumiGCAllocationSiteKey);
    key->requestedSize = requestedSize;
    key->allocatedSize = requestedSize;
    key->descriptor = descriptor;
    key->pointerKind = static_cast<unsigned char>(pointerKind);
    key->zeroingPolicy = static_cast<unsigned char>(zeroingPolicy);
    return true;
}

extern "C" void
yumiGCSetActiveThreadCache(YumiGCThreadCache* cache) {
    bool hasOwner = cache != nullptr;
    if (!cache) {
        cache = &gcFallbackThreadCache;
    }
    if (
        cache->abiVersion != YumiGCAllocationABIVersion ||
        cache->structSize != sizeof(YumiGCThreadCache)
    ) {
        *cache = YumiGCThreadCache {};
        yumiGCInitializeThreadCache(cache);
    }
    yumiGCActiveThreadCache = cache;
    yumiGCActiveAllocationBuffer = hasOwner ? &cache->activeBuffer.buffer : nullptr;
}

extern "C" YumiGCThreadCache*
yumiGCGetActiveThreadCache() {
    if (!yumiGCActiveThreadCache) {
        yumiGCSetActiveThreadCache(nullptr);
    }
    return yumiGCActiveThreadCache;
}

extern "C" bool
yumiGCValidateAllocationABI() {
    return true;
}

extern "C" bool
yumiGCTestThreadCacheOwnerFlushOnce() {
    return true;
}

extern "C" void
yumiGCSetActiveAllocationBufferStorage(YumiGCRuntimeAllocationBuffer* storage) {
    yumiGCActiveAllocationBuffer = storage ? &storage->buffer : nullptr;
}

extern "C" YumiGCAllocationBuffer*
yumiGCGetActiveAllocationBuffer() {
    return yumiGCActiveAllocationBuffer;
}

extern "C" size_t
yumiGCAllocationBufferCursorOffset() {
    return YumiGCAllocationBufferCursorOffset;
}

extern "C" size_t
yumiGCAllocationBufferLimitOffset() {
    return YumiGCAllocationBufferLimitOffset;
}

extern "C" size_t
yumiGCAllocationBufferSize() {
    return YumiGCAllocationBufferSize;
}

extern "C" size_t
yumiGCRuntimeAllocationBufferHotOffset() {
    return YumiGCRuntimeAllocationBufferHotOffset;
}

extern "C" void
yumiGCAllocationBufferFlush() {
}

extern "C" void
yumiGCAllocationBufferPublish() {
}

extern "C" void
yumiGCAllocationBufferRevoke() {
}

extern "C" void
yumiGCRuntimeFlushThreadAllocationBatches() {
}

extern "C" void
yumiGCRuntimeFlushWriteBarrierBuffer() {
}

extern "C" void
yumiGCRuntimeFlushActiveAllocationBuffer() {
}

extern "C" void
yumiGCRuntimeRevokeActiveAllocationBuffer() {
}

extern "C" bool
yumiGCRuntimeIdleCollect(uint64_t) {
    return false;
}

extern "C" void
yumiGCFrameBudget(uint64_t) {
}

extern "C" char*
yumiGCAllocationBufferRefill(const YumiGCAllocationSiteKey* key) {
    return key ? yumiNoGCAllocate(key->requestedSize) : nullptr;
}

extern "C" char*
yumiGCCompilerAllocationBufferRefill(const YumiGCAllocationSiteKey* key) {
    return key ? yumiNoGCAllocate(key->requestedSize) : nullptr;
}

extern "C" char*
yumiAllocObject(size_t size) {
    return yumiNoGCAllocate(size);
}

extern "C" char*
yumiAllocObjectWithSiteKey(const YumiGCAllocationSiteKey* key) {
    return key ? yumiNoGCAllocate(key->requestedSize) : nullptr;
}

extern "C" char*
yumiAllocObjectDescribed(size_t size, void*) {
    return yumiNoGCAllocate(size);
}

extern "C" char*
yumiAllocBuffer(size_t itemSize, size_t count) {
    size_t size = 0;
    if (!yumiNoGCAllocationSize(itemSize, count, size) || size == 0) {
        return nullptr;
    }
    return yumiNoGCAllocate(size);
}

extern "C" char*
yumiTryAllocBufferWithoutSafepoint(size_t size) {
    return yumiAllocBuffer(1, size);
}

extern "C" char*
yumiAllocBufferDescribed(size_t itemSize, size_t count, void*) {
    return yumiAllocBuffer(itemSize, count);
}

extern "C" char*
yumiResizeBuffer(char* buffer, size_t oldSize, size_t newSize) {
    return yumiResizeBufferDescribed(buffer, oldSize, newSize, nullptr);
}

extern "C" char*
yumiResizeBufferDescribed(char* buffer, size_t oldSize, size_t newSize, void* descriptor) {
    if (buffer && yumiParseRegionOwns(buffer)) {
        return yumiParseRegionResizeBuffer(buffer, oldSize, newSize, descriptor);
    }
    if (newSize == 0) {
        return nullptr;
    }
    char* resized = yumiNoGCAllocate(newSize);
    if (buffer && oldSize) {
        std::memcpy(resized, buffer, std::min(oldSize, newSize));
    }
    return resized;
}

extern "C" void*
yumiAddRoot(char*) {
    return nullptr;
}

extern "C" void*
yumiAddTemporaryRoot(char*) {
    return nullptr;
}

extern "C" void
yumiUpdateRoot(void*, char*) {
}

extern "C" void
yumiRemoveRoot(void*) {
}

extern "C" void
yumiRemoveTemporaryRoot(void*) {
}

extern "C" void
yumiDrainTemporaryRoots(YumiGCTemporaryRootStorage*) {
}

extern "C" YumiGCCollectResult
yumiCollect() {
    return YumiGCCollectCompleted;
}

extern "C" void
yumiWriteBarrier(char*, char*) {
}

extern "C" void
yumiWriteBarrierRecord(char*, char*) {
}

extern "C" void
yumiBenchRecordBarrierCall() {
}

extern "C" void
yumiWriteBarrierFast(char*, char*) {
}

extern "C" void
yumiBulkWriteBarrierFast(char*, char*, size_t, void*) {
}

extern "C" void
yumiWBBufRecordPointer(char*) {
}

extern "C" void
yumiWBBufFlush() {
}

extern "C" size_t
yumiGCFlushWriteBarrierEntries(char* const*, size_t) {
    return 0;
}

extern "C" void
yumiGCInit() {
}

extern "C" void
yumiGCStartConcurrentMark() {
}

extern "C" size_t
yumiGCStepConcurrentMark() {
    return 0;
}

extern "C" void
yumiGCFinishConcurrentMark() {
}

extern "C" void
yumiGCFinishAutomaticCycle() {
}

extern "C" void
yumiGCStepIncrementalSweep(size_t) {
}

extern "C" void
yumiGCScavenge() {
}

extern "C" void
yumiGCStepScavenge(size_t) {
}

extern "C" size_t
yumiGCMutatorAssist(size_t) {
    return 0;
}

extern "C" unsigned char
yumiGCCollectorPhase() {
    return 0;
}

extern "C" char*
yumiGCAllocObject(size_t size) {
    return yumiAllocObject(size);
}

extern "C" char*
yumiGCAllocObjectDescribed(size_t size, void* descriptor) {
    return yumiAllocObjectDescribed(size, descriptor);
}

extern "C" char*
yumiGCAllocBuffer(size_t itemSize, size_t count) {
    return yumiAllocBuffer(itemSize, count);
}

extern "C" char*
yumiGCAllocBufferDescribed(size_t itemSize, size_t count, void* descriptor) {
    return yumiAllocBufferDescribed(itemSize, count, descriptor);
}

extern "C" char*
yumiGCResizeBuffer(char* buffer, size_t oldSize, size_t newSize) {
    return yumiResizeBuffer(buffer, oldSize, newSize);
}

extern "C" char*
yumiGCResizeBufferDescribed(char* buffer, size_t oldSize, size_t newSize, void* descriptor) {
    return yumiResizeBufferDescribed(buffer, oldSize, newSize, descriptor);
}

extern "C" bool
yumiGCAllocBatchForUnregisteredThread(size_t, void*, size_t, YumiGCAllocationBatch*) {
    return false;
}

extern "C" bool
yumiGCTestAllocBatch(size_t size, void* descriptor, size_t count, YumiGCAllocationBatch* batch) {
    return yumiGCAllocBatchForUnregisteredThread(size, descriptor, count, batch);
}

extern "C" bool
yumiGCAllocBatchForRunningTask(size_t, void*, size_t, YumiGCAllocationBatch*) {
    return false;
}

extern "C" bool
yumiGCReleaseUnusedBatch(char*, char*, size_t, size_t) {
    return false;
}

extern "C" bool
yumiGCDiscardUnusedBatch(char*, char*, size_t, size_t) {
    return false;
}

extern "C" bool
yumiGCTestDiscardBatch(char* cursor, char* limit, size_t slotSize, size_t generation) {
    return yumiGCDiscardUnusedBatch(cursor, limit, slotSize, generation);
}

extern "C" void*
yumiGCAddRoot(char*) {
    return nullptr;
}

extern "C" void*
yumiGCAddTemporaryRoot(char*) {
    return nullptr;
}

extern "C" void*
yumiGCAddDescribedRoot(char*, size_t, void*) {
    return nullptr;
}

extern "C" void*
yumiAddDescribedRoot(char*, size_t, void*) {
    return nullptr;
}

extern "C" void
yumiGCUpdateRoot(void*, char*) {
}

extern "C" void
yumiGCRemoveRoot(void*) {
}

extern "C" void
yumiGCRemoveTemporaryRoot(void*) {
}

extern "C" void
yumiGCDrainTemporaryRoots(YumiGCTemporaryRootStorage*) {
}

extern "C" void
yumiGCCollect() {
}

extern "C" void
yumiGCCollectForAllocation() {
}

extern "C" bool
yumiGCVerifyObject(char* pointer) {
    return pointer != nullptr;
}

extern "C" void*
yumiGCObjectDescriptor(char*) {
    return nullptr;
}

extern "C" void
yumiGCWriteBarrier(char*, char*) {
}

extern "C" void
yumiGCBulkWriteBarrier(char*, char*, size_t, void*) {
}

extern "C" void
yumiGCSetHeapGoal(size_t) {
}

extern "C" void
yumiGCSetMinimumHeapGoal(size_t) {
}

extern "C" size_t
yumiGCRegisterOwnerWeak(char* owner, char* target) {
    if (!owner || !target) {
        return 0;
    }
    size_t handle = noGCNextOwnerWeakHandle.fetch_add(1, std::memory_order_relaxed);
    if (!handle) {
        handle = noGCNextOwnerWeakHandle.fetch_add(1, std::memory_order_relaxed);
    }
    std::lock_guard<std::mutex> lock(noGCWeakMutex);
    noGCOwnerWeakTargets[handle] = target;
    return handle;
}

extern "C" char*
yumiGCOwnerWeakLoad(size_t handle) {
    std::lock_guard<std::mutex> lock(noGCWeakMutex);
    auto found = noGCOwnerWeakTargets.find(handle);
    return found == noGCOwnerWeakTargets.end() ? nullptr : found->second;
}

extern "C" bool
yumiGCReleaseOwnerWeak(size_t handle) {
    std::lock_guard<std::mutex> lock(noGCWeakMutex);
    return noGCOwnerWeakTargets.erase(handle) != 0;
}

extern "C" size_t
yumiGCRegisterCleanup(char* target, char* context, YumiGCCleanupCallback callback) {
    if (!target || !context || !callback) {
        return 0;
    }
    size_t handle = noGCNextCleanupHandle.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(noGCCleanupMutex);
    noGCCleanupHandles.insert(handle);
    return handle;
}

extern "C" size_t
yumiGCRegisterNativeCleanup(char* target, void* context, YumiGCCleanupCallback callback) {
    return yumiGCRegisterCleanup(target, static_cast<char*>(context), callback);
}

extern "C" bool
yumiGCCancelCleanup(size_t handle) {
    std::lock_guard<std::mutex> lock(noGCCleanupMutex);
    if (!noGCCleanupHandles.erase(handle)) {
        return false;
    }
    noGCCleanupCancelCount.fetch_add(1, std::memory_order_relaxed);
    return true;
}

extern "C" size_t
yumiGCDrainCleanups() {
    return 0;
}

extern "C" void
yumiGCQueueFileDescriptorCleanup(int) {
}

extern "C" void
yumiGCReportCleanupException() {
    noGCCleanupExceptionCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" char*
yumiGCObjectSidePayload(char* object, size_t lane) {
    if (!object || lane >= YumiObjectSidePayloadLaneCount) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(noGCObjectSidePayloadMutex);
    auto found = noGCObjectSidePayloads.find(object);
    return found == noGCObjectSidePayloads.end() ? nullptr : found->second[lane];
}

extern "C" char*
yumiGCPublishObjectSidePayload(char* object, size_t lane, char* payload) {
    if (!object || !payload || lane >= YumiObjectSidePayloadLaneCount) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(noGCObjectSidePayloadMutex);
    auto result = noGCObjectSidePayloads.emplace(object, YumiNoGCObjectSidePayloads {});
    char*& published = result.first->second[lane];
    if (!published) {
        published = payload;
        ++noGCObjectSidePayloadCounts[lane];
    }
    return published;
}

extern "C" char*
yumiGCPublishCleanupObjectSidePayload(
    char* object,
    size_t lane,
    char* payload,
    YumiGCCleanupCallback callback
) {
    if (!callback) {
        return nullptr;
    }
    // No-GC builds never discover dead owners; explicit detach still clears the same side slot.
    return yumiGCPublishObjectSidePayload(object, lane, payload);
}

extern "C" void
yumiGCClearObjectSidePayload(char* object, size_t lane) {
    if (!object || lane >= YumiObjectSidePayloadLaneCount) {
        return;
    }
    std::lock_guard<std::mutex> lock(noGCObjectSidePayloadMutex);
    auto found = noGCObjectSidePayloads.find(object);
    if (found != noGCObjectSidePayloads.end() && found->second[lane]) {
        found->second[lane] = nullptr;
        --noGCObjectSidePayloadCounts[lane];
    }
}

extern "C" bool
yumiGCClearCleanupObjectSidePayload(char* object, size_t lane) {
    if (!object || lane >= YumiObjectSidePayloadLaneCount) {
        return false;
    }
    std::lock_guard<std::mutex> lock(noGCObjectSidePayloadMutex);
    auto found = noGCObjectSidePayloads.find(object);
    if (found == noGCObjectSidePayloads.end() || !found->second[lane]) {
        return false;
    }
    found->second[lane] = nullptr;
    --noGCObjectSidePayloadCounts[lane];
    return true;
}

extern "C" bool
yumiGCHasObjectSidePayload(char* object, size_t lane) {
    return yumiGCObjectSidePayload(object, lane) != nullptr;
}

extern "C" size_t
yumiGCObjectSidePayloadCount(size_t lane) {
    if (lane >= YumiObjectSidePayloadLaneCount) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(noGCObjectSidePayloadMutex);
    return noGCObjectSidePayloadCounts[lane];
}

extern "C" size_t
yumiGCCollectionObserversSidePayloadLane() {
    return YumiObjectSidePayloadCollectionObservers;
}

extern "C" size_t
yumiGCMarkupAttachmentsSidePayloadLane() {
    return YumiObjectSidePayloadMarkupAttachments;
}

extern "C" size_t
yumiGCStyleAttachmentsSidePayloadLane() {
    return YumiObjectSidePayloadStyleAttachments;
}

extern "C" size_t
yumiGCStyleTemplatesSidePayloadLane() {
    return YumiObjectSidePayloadStyleTemplates;
}

extern "C" size_t
yumiGCStyleObjectsSidePayloadLane() {
    return YumiObjectSidePayloadStyleObjects;
}

extern "C" size_t
yumiGCEventsSidePayloadLane() {
    return YumiObjectSidePayloadEvents;
}

extern "C" size_t
yumiCollectionObserverActivate(char* owner) {
    if (!owner) {
        return 0;
    }
    std::atomic<uintptr_t>& slot = noGCCollectionObserverFilter.slot(owner);
    if (!YumiCollectionObserverFilter::retain(slot, owner)) {
        std::abort();
    }
    size_t cleanup = noGCNextCleanupHandle.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(noGCCleanupMutex);
        noGCCleanupHandles.insert(cleanup);
        noGCCollectionObserverSlots[cleanup] = &slot;
    }
    noGCCollectionObserverCount.fetch_add(1, std::memory_order_relaxed);
    return cleanup;
}

extern "C" bool
yumiCollectionObserverDeactivate(size_t cleanup) {
    std::atomic<uintptr_t>* slot = nullptr;
    {
        std::lock_guard<std::mutex> lock(noGCCleanupMutex);
        auto found = noGCCollectionObserverSlots.find(cleanup);
        if (found == noGCCollectionObserverSlots.end()) {
            return false;
        }
        slot = found->second;
        noGCCollectionObserverSlots.erase(found);
    }
    if (!yumiGCCancelCleanup(cleanup)) {
        return false;
    }
    if (!YumiCollectionObserverFilter::release(*slot)) {
        std::abort();
    }
    size_t count = noGCCollectionObserverCount.load(std::memory_order_relaxed);
    while (count && !noGCCollectionObserverCount.compare_exchange_weak(
                        count,
                        count - 1,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed)) {
    }
    return count != 0;
}

extern "C" bool
yumiCollectionMayBeObserved(char* collection) {
    return noGCCollectionObserverFilter.mayBeObserved(collection);
}

extern "C" size_t
yumiCollectionObserverActiveCount() {
    return noGCCollectionObserverCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCAllocatedObjectCount() {
    return noGCAllocatedObjects.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCRequestedByteCount() {
    return noGCAllocatedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCAllocatedByteCount() {
    return noGCAllocatedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCLiveByteCount() {
    return noGCAllocatedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCHeapGoal() {
    return std::numeric_limits<size_t>::max();
}

extern "C" size_t
yumiGCBackgroundWorkerCount() {
    return 0;
}

extern "C" size_t
yumiGCBackgroundWorkerThreadCapacity() {
    return 0;
}

extern "C" size_t
yumiGCBackgroundWorkerDutyPermille() {
    return 0;
}

extern "C" size_t
yumiGCBackgroundWorkerWork() {
    return 0;
}

extern "C" size_t
yumiGCBackgroundCredit() {
    return 0;
}

extern "C" size_t
yumiGCFrameBudgetOfferCount() {
    return 0;
}

extern "C" size_t
yumiGCFrameBudgetWorkCount() {
    return 0;
}

extern "C" size_t
yumiGCFrameBudgetWorkNanos() {
    return 0;
}

extern "C" size_t
yumiGCFrameBudgetOverrunCount() {
    return 0;
}

extern "C" size_t
yumiGCCollectionCount() {
    return 0;
}

extern "C" size_t
yumiGCWeakClearCount() {
    return 0;
}

extern "C" size_t
yumiGCCleanupRegistrationCount() {
    std::lock_guard<std::mutex> lock(noGCCleanupMutex);
    return noGCCleanupHandles.size();
}

extern "C" size_t
yumiGCCleanupPendingCount() {
    return 0;
}

extern "C" size_t
yumiGCCleanupEnqueueCount() {
    return 0;
}

extern "C" size_t
yumiGCCleanupFireCount() {
    return 0;
}

extern "C" size_t
yumiGCCleanupCancelCount() {
    return noGCCleanupCancelCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupExceptionCount() {
    return noGCCleanupExceptionCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCBarrierCount() {
    return 0;
}

extern "C" size_t
yumiGCMarkParticipantCount() {
    return 0;
}

extern "C" size_t
yumiGCActiveMarkerCount() {
    return 0;
}

extern "C" uint64_t
yumiGCWorkFlushedEpoch() {
    return 0;
}

extern "C" size_t
yumiGCTerminationConfirmCount() {
    return 0;
}

extern "C" size_t
yumiGCTerminationConfirmResumeCount() {
    return 0;
}

extern "C" size_t
yumiGCAssistCount() {
    return 0;
}

extern "C" size_t
yumiGCAssistTimeNanos() {
    return 0;
}

extern "C" size_t
yumiGCMarkTimeNanos() {
    return 0;
}

extern "C" size_t
yumiGCSweepTimeNanos() {
    return 0;
}

extern "C" size_t
yumiGCPauseCount() {
    return 0;
}

extern "C" size_t
yumiGCRuntimeMode() {
    return 0;
}

#endif
