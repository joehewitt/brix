#include "BxCore/BxCore.h"
#include "BxCore/BxParseRegion.h"

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

#if BX_ENABLE_BENCH_METRICS
#define BRIX_BENCH_RECORD_ALLOCATION(size) \
    do { \
        if (__builtin_expect(brixBenchMetricsEnabledFlag, 0)) { \
            brixBenchRecordAllocation(size); \
        } \
    } while (0)
#else
#define BRIX_BENCH_RECORD_ALLOCATION(size) do { } while (0)
#endif

// **********************************************************************************************

extern "C" thread_local BrixGCAllocationBuffer* brixGCActiveAllocationBuffer = nullptr;
static thread_local BrixGCThreadCache gcFallbackThreadCache = {};
extern "C" thread_local BrixGCThreadCache* brixGCActiveThreadCache = &gcFallbackThreadCache;
extern "C" uint64_t brixGCPublicControlWord = 0;

#include "BxGC/BxGC.h"
#include "BxGC/BxGCCollectionObserverFilter.h"
#include "BxGC/BxGCRuntime.h"
#include "BxGC/BxGCTest.h"
#include "BxCore/BxPanic.h"

extern "C" void brixGCSafepoint(void);
extern "C" void brixGCEnterRuntimeCritical(void);
extern "C" void brixGCLeaveRuntimeCritical(void);
extern "C" bool brixGCIsRuntimeCritical(void);
extern "C" void brixGCEnterRuntimeBlock(void);
extern "C" void brixGCLeaveRuntimeBlock(void);
extern "C" void brixTerminateActiveTask(void);
extern "C" void brixGCStartConcurrentMark(void);
extern "C" size_t brixGCStepConcurrentMark(void);
extern "C" void brixGCFinishAutomaticCycle(void);
extern "C" void brixGCStepIncrementalSweep(size_t workLimit);
extern "C" void brixGCStepScavenge(size_t pageBudget);
extern "C" size_t brixGCTrimMetadataCaches(void);
extern "C" size_t brixGCScavengeMinimumStepPages(void);
extern "C" size_t brixGCMutatorAssist(size_t workLimit);
extern "C" bool brixGCActiveTaskCanUseFastAllocation(void) __attribute__((weak));
extern "C" bool brixGCActiveTaskCanUseFastAllocation(void) {
    return false;
}
extern "C" bool brixGCActiveTaskAcknowledgedHandshake(uint64_t control) __attribute__((weak));
extern "C" bool brixGCActiveTaskAcknowledgedHandshake(uint64_t) {
    return false;
}
extern "C" bool brixGCThreadHasLocalCallbackRegistration(void) __attribute__((weak));
extern "C" bool brixGCThreadHasLocalCallbackRegistration(void) {
    return false;
}
class BxTaskQueue;
typedef void (*BrixGCBackgroundTaskFunc)(void* closure);
extern "C" int brixGCBackgroundTaskQueueThreadCount(void) __attribute__((weak));
extern "C" BxTaskQueue* brixGCBackgroundTaskQueue(void) __attribute__((weak));
extern "C" bool brixForkGCMarkTaskInWorker(
    BxTaskQueue* queue,
    BrixGCBackgroundTaskFunc func,
    void* closure) __attribute__((weak));
extern "C" size_t brixGCActiveTaskAssistDebt(void) __attribute__((weak));
extern "C" size_t brixGCActiveTaskAssistDebtCycle(void) __attribute__((weak));
extern "C" void brixGCSetActiveTaskAssistDebt(size_t debt) __attribute__((weak));
extern "C" void brixGCSetActiveTaskAssistDebtCycle(size_t cycle) __attribute__((weak));
extern "C" void brixGCDebugUnstoppableFastAllocationRejected(void) __attribute__((weak));
extern "C" void brixGCDebugUnstoppableFastAllocationRejected(void) {}
extern "C" unsigned char brixInterruptRequestedFlags = 0;
static_assert(sizeof(brixInterruptRequestedFlags) == 1, "interrupt flags must be one byte");
static_assert(sizeof(brixGCPublicControlWord) == sizeof(uint64_t), "GC control word drifted");
static_assert(
    offsetof(BrixGCAllocationBuffer, cursor) == BrixGCAllocationBufferCursorOffset,
    "GC allocation buffer cursor offset drifted");
static_assert(
    offsetof(BrixGCAllocationBuffer, limit) == BrixGCAllocationBufferLimitOffset,
    "GC allocation buffer limit offset drifted");
static_assert(
    sizeof(BrixGCAllocationBuffer) == BrixGCAllocationBufferSize,
    "GC allocation buffer size drifted");
static_assert(
    offsetof(BrixGCRuntimeAllocationBuffer, buffer) ==
        BrixGCRuntimeAllocationBufferHotOffset,
    "GC runtime allocation buffer hot offset drifted");
static_assert(
    offsetof(BrixGCRuntimeAllocationBuffer, generation) ==
        BrixGCRuntimeAllocationBufferGenerationOffset,
    "GC runtime allocation buffer generation offset drifted");
static_assert(
    sizeof(BrixGCRuntimeAllocationBuffer) == BrixGCRuntimeAllocationBufferSize,
    "GC runtime allocation buffer size drifted");
static_assert(
    sizeof(BrixGCAllocationSiteKey) == BrixGCAllocationSiteKeySize,
    "GC allocation site key size drifted");
static_assert(
    sizeof(BrixGCThreadCache) == BrixGCThreadCacheSize,
    "GC thread cache size drifted");

#if BX_ENABLE_BRIX_GC

static constexpr bool
brixGCCollectionDisabled() {
    return BX_GC_NEVER_COLLECT != 0;
}

static uint64_t
brixGCLoadControlWord() {
    return __atomic_load_n(&brixGCPublicControlWord, __ATOMIC_ACQUIRE);
}

static unsigned char
brixGCLoadPublicPhase() {
    return brixGCControlWordPhase(brixGCLoadControlWord());
}

static size_t
brixGCLoadPublicSweepGeneration() {
    return brixGCControlWordSweepGeneration(brixGCLoadControlWord());
}

static void
brixGCUpdateControlWord(uint64_t mask, uint64_t value) {
    uint64_t current = __atomic_load_n(&brixGCPublicControlWord, __ATOMIC_RELAXED);
    while (true) {
        uint64_t next = (current & ~mask) | (value & mask);
        if (__atomic_compare_exchange_n(
                &brixGCPublicControlWord,
                &current,
                next,
                false,
                __ATOMIC_RELEASE,
                __ATOMIC_RELAXED)) {
            return;
        }
    }
}

static BrixGCThreadCache*
brixGCEnsureActiveThreadCache() {
    if (!brixGCActiveThreadCache) {
        brixGCActiveThreadCache = &gcFallbackThreadCache;
    }
    if (
        brixGCActiveThreadCache->abiVersion != BrixGCAllocationABIVersion ||
        brixGCActiveThreadCache->structSize != sizeof(BrixGCThreadCache)
    ) {
        *brixGCActiveThreadCache = BrixGCThreadCache {};
        brixGCInitializeThreadCache(brixGCActiveThreadCache);
    }
    return brixGCActiveThreadCache;
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

struct BrixGCBarrierStatsRecord {
    std::atomic<size_t> barrierCalls;
    std::atomic<size_t> barrierSlowEntries;
    std::atomic<size_t> barrierShades;
    std::atomic<size_t> fastCalls;
    std::atomic<size_t> fastSlowEntries;

    BrixGCBarrierStatsRecord()
        : barrierCalls(0),
          barrierSlowEntries(0),
          barrierShades(0),
          fastCalls(0),
          fastSlowEntries(0) {}
};

enum BrixGCBarrierStatsKind {
    BrixGCBarrierStatsCalls,
    BrixGCBarrierStatsSlowEntries,
    BrixGCBarrierStatsShades,
    BrixGCBarrierStatsFastCalls,
    BrixGCBarrierStatsFastSlowEntries
};

static std::mutex gcBarrierStatsMutex;
static std::vector<BrixGCBarrierStatsRecord*> gcBarrierStatsRecords;
static thread_local BrixGCBarrierStatsRecord* gcThreadBarrierStats = nullptr;
#if BX_ENABLE_BENCH_METRICS
static constexpr size_t kBrixGCBenchAllocationSmallBinCount = 128;
static constexpr size_t kBrixGCBenchAllocationLargeBinCount = 64;
static constexpr size_t kBrixGCBenchAllocationBinCount =
    kBrixGCBenchAllocationSmallBinCount + kBrixGCBenchAllocationLargeBinCount + 1;
static constexpr size_t kBrixGCBenchAllocationOverflowBin =
    kBrixGCBenchAllocationBinCount - 1;
static std::atomic<size_t> gcBenchCpuNanos {0};
static std::array<std::atomic<size_t>, BrixGCCompilerAllocationBufferCount + 1>
    gcBenchAllocationSites {};
static std::array<std::atomic<size_t>, kBrixGCBenchAllocationBinCount>
    gcBenchAllocationSizeBins {};
static constexpr size_t kBrixGCBenchSizeClassCount = 69;
static constexpr size_t kBrixGCBenchLargeSizeClass = 68;
static constexpr size_t kBrixGCBenchSizeClassUpperBounds[kBrixGCBenchSizeClassCount] = {
    0, 8, 16, 24, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224,
    240, 256, 288, 320, 352, 384, 416, 448, 480, 512, 576, 640, 704, 768, 896,
    1024, 1152, 1280, 1408, 1536, 1792, 2048, 2304, 2688, 3072, 3200, 3456,
    4096, 4864, 5376, 6144, 6528, 6784, 6912, 8192, 9472, 9728, 10240, 10880,
    12288, 13568, 14336, 16384, 18432, 19072, 20480, 21760, 24576, 27264,
    28672, 32768, std::numeric_limits<size_t>::max()
};
static std::array<std::atomic<size_t>, kBrixGCBenchSizeClassCount>
    gcBenchAllocatedSizeClasses {};

struct BrixGCBenchThreadRefillRecord {
    size_t ordinal;
    std::atomic<size_t> refills {0};

    explicit BrixGCBenchThreadRefillRecord(size_t ordinal)
        : ordinal(ordinal) {}
};

static std::mutex gcBenchThreadRefillMutex;
static std::vector<BrixGCBenchThreadRefillRecord*> gcBenchThreadRefillRecords;
static thread_local BrixGCBenchThreadRefillRecord* gcBenchThreadRefillRecord = nullptr;
#endif
static constexpr size_t kBrixGCSubphaseCount = BrixGCObservationStackMapLookup + 1;
static constexpr size_t kBrixGCPauseEventCapacity = 8192;
static constexpr size_t kBrixGCCycleSummaryCapacity = 1024;
static constexpr size_t kBrixGCSubphaseSampleCapacity = 8192;
static constexpr size_t kBrixGCDefaultHeapGoal = 32ull * 1024ull * 1024ull;
static constexpr size_t kBrixGCDefaultHeapMinGrowth = 16ull * 1024ull * 1024ull;
static constexpr size_t kBrixGCHeapTaperStart = 128ull * 1024ull * 1024ull;
static constexpr size_t kBrixGCIdleSettleMinimumBytes = 4ull * 1024ull * 1024ull;
static constexpr uint64_t kBrixGCIdleSettleQuietNanos = 250000000ull;
static constexpr size_t kBrixGCDefaultHeapGrowthPercent = 100;
static constexpr size_t kBrixGCDefaultHeapHardCap = 0;
static constexpr size_t kBrixGCDefaultThreadBatchCacheBytes = 8ull * 1024ull * 1024ull;
static constexpr size_t kBrixGCDefaultGlobalBatchCacheBytes = 128ull * 1024ull * 1024ull;
static constexpr size_t kBrixGCThreadStatFlushObjectCount = 65536;
static constexpr unsigned char kBrixGCPhaseIdle = 0;
static constexpr unsigned char kBrixGCPhaseMarkSetup = 1;
static constexpr unsigned char kBrixGCPhaseConcurrentMark = 2;
static constexpr unsigned char kBrixGCPhaseMarkTermination = 3;
static constexpr unsigned char kBrixGCPhaseMarkComplete = 4;
static constexpr unsigned char kBrixGCPhaseSweep = 5;
static constexpr size_t kBrixGCAutomaticAssistWork = 16;
static constexpr size_t kBrixGCAutomaticHardAssistWork = 128;
static constexpr size_t kBrixGCAutomaticStepWork = 64;
static constexpr size_t kBrixGCAutomaticIdleWork = 256;
static constexpr size_t kBrixGCAutomaticIdleScavengePages = 64;
static constexpr size_t kBrixGCIdleScavengeMaximumPages = 16384;
static constexpr size_t kBrixGCAutomaticTerminationQueueBudget = 64;
static constexpr size_t kBrixGCAutomaticMaxActiveCycleNanos = 250ull * 1000ull * 1000ull;
static constexpr size_t kBrixGCMaxAssistSliceNanos = 50ull * 1000ull;
static constexpr size_t kBrixGCBackgroundWorkerStepWork = 64;
static constexpr size_t kBrixGCBackgroundDutyDenominator = 1000;
static constexpr size_t kBrixGCBudgetGuardNanos = 50ull * 1000ull;

enum BrixGCAutomaticFallbackReason {
    BrixGCAutomaticFallbackNone = 0,
    BrixGCAutomaticFallbackNoProgressDriver = 1,
    BrixGCAutomaticFallbackMaxActiveAge = 2,
    BrixGCAutomaticFallbackHardCap = 3
};

enum BrixGCPacerTriggerReason {
    BrixGCPacerTriggerNone = 0,
    BrixGCPacerTriggerSoftGoal = 1,
    BrixGCPacerTriggerHardGoal = 2,
    BrixGCPacerTriggerHardCap = 3,
    BrixGCPacerTriggerIdle = 4,
    BrixGCPacerTriggerExplicit = 5,
    BrixGCPacerTriggerFrame = 6
};

static thread_local size_t gcFallbackActiveTaskAssistDebt = 0;
static thread_local size_t gcFallbackActiveTaskAssistDebtCycle = 0;

static constexpr size_t kBrixGCAllocationBatchSmallCount = 129;
static constexpr size_t kBrixGCAllocationBatchLargeBucketSize = 1024;
static constexpr size_t kBrixGCAllocationBatchLargeBucketCount = 64;
static constexpr size_t kBrixGCAllocationBatchCount =
    kBrixGCAllocationBatchSmallCount + kBrixGCAllocationBatchLargeBucketCount;
static constexpr size_t kBrixGCAllocationBatchMaxSize =
    kBrixGCAllocationBatchLargeBucketSize * kBrixGCAllocationBatchLargeBucketCount;
static constexpr size_t kBrixGCDescriptorlessAllocationBatchBytes = 8 * 1024;
static constexpr size_t kBrixGCDescriptorlessAllocationBatchMinCount = 64;
static constexpr size_t kBrixGCDescriptorlessAllocationBatchMaxCount = 256;
static constexpr size_t kBrixGCDescribedSmallAllocationBatchPreferredCount = 8;
static constexpr size_t kBrixGCDescribedMediumAllocationBatchPreferredCount = 4;
static constexpr size_t kBrixGCDescribedLargeAllocationBatchPreferredCount = 2;
static constexpr size_t kBrixGCAdaptiveAllocationBatchMaxCount = 64;
static constexpr unsigned char kBrixGCAdaptiveFullConsumptionStreak = 2;
static constexpr size_t kBrixGCAdaptiveLowUseDenominator = 4;
static constexpr size_t kBrixGCAdaptiveLowUseUnusedParts = 3;
static constexpr size_t kBrixGCAdaptiveCountByte = 0;
static constexpr size_t kBrixGCAdaptiveFullStreakByte = 1;
static constexpr size_t kBrixGCAdaptiveEnabledByte = 2;
static constexpr size_t kBrixGCAdaptiveReservedSlotCountByte = 3;
static constexpr size_t kBrixGCCompilerRangePacerCreditBytes = 64 * 1024;
static constexpr size_t kBrixGCGenericRangePacerCreditBytes = 64 * 1024;
static constexpr size_t kBrixGCObjectAlignment = 8;
static constexpr size_t kBrixGCThreadCounterCompilerRefills = 0;
static constexpr size_t kBrixGCThreadCounterBarrierCalls = 1;
static constexpr size_t kBrixGCThreadCounterBarrierSlowEntries = 2;
static constexpr size_t kBrixGCThreadCounterBarrierShades = 3;
static constexpr unsigned char kBrixGCAllocationPointerFree = 0;
static constexpr unsigned char kBrixGCAllocationPointerful = 1;
static constexpr unsigned char kBrixGCAllocationZeroFull = 0;

enum BrixGCAllocationCallKind {
    BrixGCAllocationCallObject = 0,
    BrixGCAllocationCallDescribedObject = 1,
    BrixGCAllocationCallBuffer = 2,
    BrixGCAllocationCallDescribedBuffer = 3,
    BrixGCAllocationCallResizeBuffer = 4,
    BrixGCAllocationCallDescribedResizeBuffer = 5,
    BrixGCAllocationCallKindCount = 6
};

enum BrixGCAllocationPhaseBucket {
    BrixGCAllocationPhaseIdle = 0,
    BrixGCAllocationPhaseMarkSetup = 1,
    BrixGCAllocationPhaseConcurrentMark = 2,
    BrixGCAllocationPhaseMarkComplete = 3,
    BrixGCAllocationPhaseSweep = 4,
    BrixGCAllocationPhaseOther = 5,
    BrixGCAllocationPhaseBucketCount = 6
};

enum BrixGCAllocationBatchMissReason {
    BrixGCAllocationBatchMissGeneration = 0,
    BrixGCAllocationBatchMissDescriptor = 1,
    BrixGCAllocationBatchMissRequestedSize = 2,
    BrixGCAllocationBatchMissEmpty = 3,
    BrixGCAllocationBatchMissCap = 4,
    BrixGCAllocationBatchMissReasonCount = 5
};

static std::array<std::atomic<size_t>, BrixGCAllocationCallKindCount> gcAllocationCallsByKind = {};
static std::array<std::atomic<size_t>, BrixGCAllocationPhaseBucketCount> gcAllocationCallsByPhase = {};
static std::array<std::atomic<size_t>, BrixGCAllocationBatchMissReasonCount> gcAllocationBatchMisses = {};
static std::array<std::atomic<size_t>, kBrixGCAllocationBatchCount> gcAllocationDescriptorlessBySizeClass = {};
static std::array<std::atomic<size_t>, kBrixGCAllocationBatchCount> gcAllocationDescribedBySizeClass = {};
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

struct BrixGCThreadAllocationBatch {
    char* cursor = nullptr;
    char* limit = nullptr;
    size_t slotSize = 0;
    size_t requestedSize = 0;
    void* descriptor = nullptr;
    size_t generation = 0;
};

struct BrixGCSubphaseSamples {
    std::vector<size_t> durations;
    uint64_t sampleSequence = 0;
    uint64_t randomState = 0;
    size_t maxDurationNanos = 0;
};

struct BrixGCPauseEvent {
    size_t cycleId = 0;
    size_t kind = 0;
    uint64_t startNanos = 0;
    size_t durationNanos = 0;
    size_t tasksWaited = 0;
    size_t slowestTaskNanos = 0;

    BrixGCPauseEvent() = default;
    BrixGCPauseEvent(
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

struct BrixGCCycleSummary {
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

    BrixGCCycleSummary() = default;
    BrixGCCycleSummary(
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
static std::array<BrixGCSubphaseSamples, kBrixGCSubphaseCount> gcSubphaseSamples;
static std::array<BrixGCPauseEvent, kBrixGCPauseEventCapacity> gcPauseEvents;
static size_t gcPauseEventSequence = 0;
static size_t gcPauseEventStored = 0;
static std::atomic<size_t> gcPauseCount {0};
static std::array<BrixGCCycleSummary, kBrixGCCycleSummaryCapacity> gcCycleSummaries;
static size_t gcCycleSummarySequence = 0;
static size_t gcCycleSummaryStored = 0;

static thread_local BrixGCRuntimeAllocationBuffer* gcActiveAllocationBufferStorage = nullptr;
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
static thread_local std::array<uint64_t, kBrixGCSubphaseCount> gcSubphaseStartNanos = {};
static thread_local std::array<size_t, kBrixGCSubphaseCount> gcSubphaseStartCycles = {};
#if BX_ENABLE_BENCH_METRICS
static thread_local size_t gcBenchCpuDepth = 0;
static thread_local uint64_t gcBenchCpuStartNanos = 0;
#endif

static uint64_t brixGCMonotonicNanos();
#if BX_ENABLE_BENCH_METRICS
static uint64_t
brixGCBenchThreadCpuNanos() {
    struct timespec now {};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0) {
        return 0;
    }
    return static_cast<uint64_t>(now.tv_sec) * 1000000000ull +
        static_cast<uint64_t>(now.tv_nsec);
}

static void
brixGCBenchCpuStart() {
    if (!brixBenchMetricsEnabledFlag) {
        return;
    }
    if (gcBenchCpuDepth++ == 0) {
        gcBenchCpuStartNanos = brixGCBenchThreadCpuNanos();
    }
}

static void
brixGCBenchCpuEnd() {
    if (!brixBenchMetricsEnabledFlag || !gcBenchCpuDepth) {
        return;
    }
    if (--gcBenchCpuDepth == 0 && gcBenchCpuStartNanos) {
        uint64_t end = brixGCBenchThreadCpuNanos();
        if (end > gcBenchCpuStartNanos) {
            gcBenchCpuNanos.fetch_add(
                static_cast<size_t>(end - gcBenchCpuStartNanos),
                std::memory_order_relaxed);
        }
        gcBenchCpuStartNanos = 0;
    }
}
#else
static void brixGCBenchCpuStart() {}
static void brixGCBenchCpuEnd() {}
#endif
static void brixGCRecordSubphaseSample(
    size_t subphase,
    size_t cycle,
    uint64_t startNanos,
    size_t durationNanos,
    size_t value);
static void brixGCRecordPauseEvent(
    size_t kind,
    size_t cycleId,
    uint64_t startNanos,
    size_t durationNanos,
    size_t tasksWaited,
    size_t slowestTaskNanos);
static void brixDiscardCachedGCAllocationBatch(BrixGCThreadAllocationBatch& batch);
static void brixFlushGCAllocationBatches();
static void brixPublishGCWriteBarrierCounters(BrixGCThreadCache* cache);
static BrixGCBarrierStatsRecord* brixGCEnsureThreadBarrierStats();
static void brixGCAddOwnedBarrierCounter(std::atomic<size_t>& counter, size_t amount);
static size_t brixGCSumBarrierCounter(BrixGCBarrierStatsKind kind);
static void brixFlushGCCompilerAllocationBuffer();
static void brixFlushGCCompilerAllocationBuffer(BrixGCRuntimeAllocationBuffer& storage);
static bool brixGCCompilerAllocationBufferIndexIsValid(size_t index);
static void brixMarkGCCompilerAllocationBufferActive(size_t index);
static void brixGCPrepareCompilerRangeVerificationAllocation(
    const BrixGCAllocationSiteKey& key,
    size_t selectedCount);
static void brixGCChargeCompilerRangePacerBytes(size_t deliveredBytes);
struct BrixGCHotAllocationState;
static void brixGCChargeGenericRangePacerBytes(
    BrixGCHotAllocationState& hot,
    size_t allocatedBytes);
static void brixRecordGCPublicAllocation(
    BrixGCHotAllocationState& hot,
    size_t requestedSize,
    size_t allocatedSize);
static void brixRecordGCPublicAllocations(
    size_t count,
    size_t requestedSize,
    size_t allocatedSize);
static void brixGCSubtractAtomicSaturating(std::atomic<size_t>& value, size_t amount);
static void brixPublishGCThreadBatchConsumption();
static void brixPublishGCThreadAllocationStats();
static bool brixGCAllocationAssistDisabled();
static bool brixGCIdleProgressDisabled();
static bool brixGCVerificationSlowPathsEnabled();
static size_t brixGCDrainAutomaticCycleBlocking();
static void brixGCStartBackgroundWorkers();
static size_t brixGCAutomaticHardGoal(size_t goal);
static size_t brixGCPacerSoftGoal(size_t goal);
static size_t brixGCPacerHardGoal(size_t goal);
static size_t brixGCPacerTriggerReserveBytes(size_t goal);

static bool
brixGCPublishThreadCounter(std::atomic<size_t>& target, size_t& value) {
    if (!value) {
        return false;
    }
    target.fetch_add(value, std::memory_order_relaxed);
    value = 0;
    return true;
}

template <size_t Count>
static bool
brixGCPublishThreadCounterArray(
    std::array<std::atomic<size_t>, Count>& target,
    std::array<size_t, Count>& local
) {
    bool published = false;
    for (size_t index = 0; index < Count; index++) {
        published = brixGCPublishThreadCounter(target[index], local[index]) || published;
    }
    return published;
}

#if BX_ENABLE_BENCH_METRICS
static BrixGCBenchThreadRefillRecord*
brixGCEnsureBenchThreadRefillRecord() {
    if (gcBenchThreadRefillRecord) {
        return gcBenchThreadRefillRecord;
    }
    std::lock_guard<std::mutex> lock(gcBenchThreadRefillMutex);
    gcBenchThreadRefillRecord =
        new BrixGCBenchThreadRefillRecord(gcBenchThreadRefillRecords.size());
    gcBenchThreadRefillRecords.push_back(gcBenchThreadRefillRecord);
    return gcBenchThreadRefillRecord;
}

static void
brixGCPublishBenchThreadRefills(size_t count) {
    if (!count) {
        return;
    }
    BrixGCBenchThreadRefillRecord* record = brixGCEnsureBenchThreadRefillRecord();
    record->refills.store(
        record->refills.load(std::memory_order_relaxed) + count,
        std::memory_order_relaxed);
}
#endif

struct BrixGCThreadStats {
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
    std::array<size_t, BrixGCAllocationCallKindCount> allocationCallsByKind = {};
    std::array<size_t, BrixGCAllocationPhaseBucketCount> allocationCallsByPhase = {};
    std::array<size_t, BrixGCAllocationBatchMissReasonCount> allocationBatchMisses = {};
    std::array<size_t, kBrixGCAllocationBatchCount> allocationDescriptorlessBySizeClass = {};
    std::array<size_t, kBrixGCAllocationBatchCount> allocationDescribedBySizeClass = {};

    ~BrixGCThreadStats() {
        brixFlushGCAllocationBatches();
        flush();
    }

    bool flushBatchConsumption() {
        if (!batchConsumedBytes) {
            return false;
        }

        gcBatchConsumedBytes.fetch_add(batchConsumedBytes, std::memory_order_relaxed);
        brixGCSubtractAtomicSaturating(gcBatchCacheBytes, batchConsumedBytes);
        batchConsumedBytes = 0;
        return true;
    }

    void flush() {
        bool published = flushBatchConsumption();
#if BX_ENABLE_BENCH_METRICS
        brixGCPublishBenchThreadRefills(allocationFastRefillCount);
#endif
        published = brixGCPublishThreadCounter(gcAllocatedObjects, allocatedObjects) || published;
        published = brixGCPublishThreadCounter(gcRequestedBytes, requestedBytes) || published;
        published = brixGCPublishThreadCounter(gcAllocatedBytes, allocatedBytes) || published;
        published = brixGCPublishThreadCounter(gcCycleAllocatedBytes, cycleAllocatedBytes) ||
            published;
        published = brixGCPublishThreadCounter(gcAllocationZeroedBytes, zeroedBytes) || published;
        published = brixGCPublishThreadCounter(gcAllocationFastHitCount, allocationFastHitCount) ||
            published;
        published = brixGCPublishThreadCounter(
            gcAllocationFastRefillCount,
            allocationFastRefillCount) || published;
        published = brixGCPublishThreadCounter(
            gcAllocationFastFallbackCount,
            allocationFastFallbackCount) || published;
        published = brixGCPublishThreadCounter(gcAllocationLargeCount, allocationLargeCount) ||
            published;
        published = brixGCPublishThreadCounter(
            gcAllocationAssistCheckCount,
            allocationAssistCheckCount) || published;
        published = brixGCPublishThreadCounter(
            gcAllocationAssistCallCount,
            allocationAssistCallCount) || published;
        published = brixGCPublishThreadCounter(gcAssistCount, assistCount) || published;
        published = brixGCPublishThreadCounter(gcAssistTimeNanos, assistTimeNanos) || published;
        published = brixGCPublishThreadCounter(gcAllocationSiteHitCount, allocationSiteHitCount) ||
            published;
        published = brixGCPublishThreadCounter(
            gcAllocationSiteMissCount,
            allocationSiteMissCount) || published;
        published = brixGCPublishThreadCounter(
            gcAllocationSiteRefillCount,
            allocationSiteRefillCount) || published;
        published = brixGCPublishThreadCounter(
            gcAllocationSiteFallbackCount,
            allocationSiteFallbackCount) || published;
        published = brixGCPublishThreadCounter(
            gcAdaptiveBatchGrowthCount,
            adaptiveBatchGrowthCount) || published;
        published = brixGCPublishThreadCounter(
            gcAdaptiveBatchShrinkCount,
            adaptiveBatchShrinkCount) || published;
        published = brixGCPublishThreadCounter(
            gcAdaptiveBatchReturnedUnusedSlotCount,
            adaptiveBatchReturnedUnusedSlotCount) || published;
        published = brixGCPublishThreadCounter(
            gcCompilerIndexedRefillCount,
            compilerIndexedRefillCount) || published;
        published = brixGCPublishThreadCounter(
            gcActiveBufferRefillCount,
            activeBufferRefillCount) || published;
        published = brixGCPublishThreadCounter(
            gcGenericDescribedRefillCount,
            genericDescribedRefillCount) || published;
        published = brixGCPublishThreadCounter(
            gcGenericDescriptorlessRefillCount,
            genericDescriptorlessRefillCount) || published;
        published = brixGCPublishThreadCounterArray(
            gcAllocationCallsByKind,
            allocationCallsByKind) || published;
        published = brixGCPublishThreadCounterArray(
            gcAllocationCallsByPhase,
            allocationCallsByPhase) || published;
        published = brixGCPublishThreadCounterArray(
            gcAllocationBatchMisses,
            allocationBatchMisses) || published;
        published = brixGCPublishThreadCounterArray(
            gcAllocationDescriptorlessBySizeClass,
            allocationDescriptorlessBySizeClass) || published;
        published = brixGCPublishThreadCounterArray(
            gcAllocationDescribedBySizeClass,
            allocationDescribedBySizeClass) || published;

        if (published) {
            gcAllocationStatPublicationCount.fetch_add(1, std::memory_order_relaxed);
        }
    }
};

struct BrixGCHotAllocationState {
    size_t batchCacheBytes = 0;
    size_t batchPendingReservationBytes = 0;
    size_t genericPacerPendingBytes = 0;
    size_t genericPacerChargedBytes = 0;
    size_t genericPacerCollectionChecks = 0;
    // Keep stats last so its thread-exit destructor can publish through the still-live scalars.
    BrixGCThreadStats stats;
};

static thread_local BrixGCHotAllocationState gcHotAllocationState;

static void
brixPublishGCThreadBatchConsumption() {
    gcHotAllocationState.stats.flushBatchConsumption();
}

static void
brixPublishGCThreadAllocationStats() {
    if (!gcAllocationBufferFlushInProgress) {
        brixFlushGCCompilerAllocationBuffer();
    }
    gcHotAllocationState.stats.flush();
}

static size_t
brixGCAllocationBatchPreferredCount(size_t allocSize, void* descriptor) {
    if (brixGCVerificationSlowPathsEnabled()) {
        return 1;
    }
    if (!descriptor) {
        size_t byteCount = allocSize
            ? kBrixGCDescriptorlessAllocationBatchBytes / allocSize
            : kBrixGCDescriptorlessAllocationBatchMaxCount;
        return std::min(
            kBrixGCDescriptorlessAllocationBatchMaxCount,
            std::max(kBrixGCDescriptorlessAllocationBatchMinCount, byteCount));
    }
    if (allocSize <= 256) {
        return kBrixGCDescribedSmallAllocationBatchPreferredCount;
    }
    if (allocSize <= 1024) {
        return kBrixGCDescribedMediumAllocationBatchPreferredCount;
    }
    return kBrixGCDescribedLargeAllocationBatchPreferredCount;
}

static size_t
brixGCAdaptiveAllocationBatchCount(
    const BrixGCRuntimeAllocationBuffer& storage,
    size_t allocatedSize,
    void* descriptor
) {
    size_t baseCount = brixGCAllocationBatchPreferredCount(
        allocatedSize,
        descriptor);
    if (!descriptor || brixGCVerificationSlowPathsEnabled()) {
        return baseCount;
    }
    size_t count = storage.reserved[kBrixGCAdaptiveCountByte];
    return std::max(
        baseCount,
        std::min(count, kBrixGCAdaptiveAllocationBatchMaxCount));
}

static size_t
brixGCAdaptiveAllocationBatchCount(
    const BrixGCRuntimeAllocationBuffer& storage,
    const BrixGCAllocationSiteKey& key
) {
    return brixGCAdaptiveAllocationBatchCount(
        storage,
        key.allocatedSize,
        key.descriptor);
}

static void
brixGCResetAdaptiveAllocationBufferPolicy(BrixGCRuntimeAllocationBuffer& storage) {
    storage.reserved[kBrixGCAdaptiveCountByte] = 0;
    storage.reserved[kBrixGCAdaptiveFullStreakByte] = 0;
    storage.reserved[kBrixGCAdaptiveReservedSlotCountByte] = 0;
}

static void
brixGCInitializeEmptyRuntimeAllocationBuffer(BrixGCRuntimeAllocationBuffer& storage) {
    storage = BrixGCRuntimeAllocationBuffer {};
    brixGCInitializeRuntimeAllocationBuffer(&storage);
}

static BrixGCRuntimeAllocationBuffer&
brixGCThreadRangeWay(BrixGCThreadCache* cache, size_t index, size_t way) {
    if (way == 0) {
        return cache->ranges[index];
    }
    return cache->parkedRanges[index][way - 1];
}

static bool
brixGCAllocationRangeHasOwnership(const BrixGCRuntimeAllocationBuffer& storage) {
    return storage.base || storage.buffer.cursor || storage.buffer.limit;
}

static bool
brixGCAllocationRangeHasPolicySite(const BrixGCRuntimeAllocationBuffer& storage) {
    return storage.descriptor && storage.requestedSize && storage.slotSize;
}

static bool
brixGCAllocationRangeHasUnconsumedSlots(const BrixGCRuntimeAllocationBuffer& storage) {
    return storage.buffer.cursor && storage.buffer.limit &&
        storage.buffer.cursor < storage.buffer.limit;
}

static bool
brixGCAllocationRangeIsParkable(const BrixGCRuntimeAllocationBuffer& storage) {
    if (!storage.descriptor) {
        return brixGCAllocationRangeHasUnconsumedSlots(storage);
    }
    return brixGCAllocationRangeHasOwnership(storage) ||
        brixGCAllocationRangeHasPolicySite(storage);
}

static bool
brixGCAllocationRangeMatchesSite(
    const BrixGCRuntimeAllocationBuffer& storage,
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
brixGCMoveRuntimeAllocationBuffer(
    BrixGCRuntimeAllocationBuffer& destination,
    BrixGCRuntimeAllocationBuffer& source
) {
    destination = source;
    brixGCInitializeEmptyRuntimeAllocationBuffer(source);
}

static void
brixGCResetAdaptiveAllocationBufferPolicies(BrixGCThreadCache* cache) {
    if (!cache) {
        return;
    }
    for (size_t index = 0; index < BrixGCCompilerAllocationBufferCount; ++index) {
        brixGCResetAdaptiveAllocationBufferPolicy(cache->compilerBuffers[index]);
    }
    for (size_t index = 0; index < BrixGCThreadCacheRangeCount; ++index) {
        brixGCResetAdaptiveAllocationBufferPolicy(cache->ranges[index]);
        for (size_t way = 0; way < BrixGCThreadCacheParkedRangeWayCount; ++way) {
            brixGCResetAdaptiveAllocationBufferPolicy(cache->parkedRanges[index][way]);
        }
        cache->rangeNextVictimWay[index] = 0;
    }
}

static size_t
brixGCAllocationPhaseBucket(unsigned char phase) {
    switch (phase) {
    case kBrixGCPhaseIdle:
        return BrixGCAllocationPhaseIdle;
    case kBrixGCPhaseMarkSetup:
        return BrixGCAllocationPhaseMarkSetup;
    case kBrixGCPhaseConcurrentMark:
    case kBrixGCPhaseMarkTermination:
        return BrixGCAllocationPhaseConcurrentMark;
    case kBrixGCPhaseMarkComplete:
        return BrixGCAllocationPhaseMarkComplete;
    case kBrixGCPhaseSweep:
        return BrixGCAllocationPhaseSweep;
    default:
        return BrixGCAllocationPhaseOther;
    }
}

static bool
brixGCPhaseAllowsFastBatchAllocation(unsigned char phase) {
    return phase <= kBrixGCPhaseSweep;
}

static void
brixGCRecordAllocationPhaseCount(BrixGCHotAllocationState& hot, size_t count) {
    if (!count) {
        return;
    }
    size_t phase = brixGCAllocationPhaseBucket(brixGCLoadPublicPhase());
    hot.stats.allocationCallsByPhase[phase] += count;
}

static void
brixGCRecordPublicAllocationCall(BrixGCHotAllocationState& hot, size_t kind) {
    if (kind < gcAllocationCallsByKind.size()) {
        hot.stats.allocationCallsByKind[kind] += 1;
    }
}

#if BX_ENABLE_BENCH_METRICS
static size_t
brixGCBenchAllocationSizeBin(size_t requestedSize) {
    if (requestedSize > 0 && requestedSize <= 1024) {
        return (requestedSize - 1) / 8;
    }
    if (requestedSize > 1024 && requestedSize <= 64 * 1024) {
        return kBrixGCBenchAllocationSmallBinCount + (requestedSize - 1025) / 1024;
    }
    return kBrixGCBenchAllocationOverflowBin;
}

static void
brixGCBenchRecordAllocationSizes(size_t count, size_t requestedSize) {
    if (brixBenchMetricsEnabledFlag && count) {
        gcBenchAllocationSizeBins[brixGCBenchAllocationSizeBin(requestedSize)].fetch_add(
            count,
            std::memory_order_relaxed);
    }
}

static void
brixGCBenchRecordAllocatedSizeClasses(size_t count, size_t allocatedSize) {
    if (!brixBenchMetricsEnabledFlag || !count) {
        return;
    }
    size_t sizeClass = kBrixGCBenchLargeSizeClass;
    for (size_t index = 1; index < kBrixGCBenchLargeSizeClass; ++index) {
        if (allocatedSize <= kBrixGCBenchSizeClassUpperBounds[index]) {
            sizeClass = index;
            break;
        }
    }
    gcBenchAllocatedSizeClasses[sizeClass].fetch_add(count, std::memory_order_relaxed);
}
#else
static void brixGCBenchRecordAllocationSizes(size_t, size_t) {}
static void brixGCBenchRecordAllocatedSizeClasses(size_t, size_t) {}
#endif

static void
brixGCRecordAllocationBatchMiss(size_t reason) {
    if (reason < gcAllocationBatchMisses.size()) {
        gcHotAllocationState.stats.allocationBatchMisses[reason] += 1;
    }
}

static void
brixGCRecordAllocationDescriptorMix(
    BrixGCHotAllocationState& hot,
    size_t index,
    void* descriptor
) {
    if (index >= kBrixGCAllocationBatchCount) {
        return;
    }
    auto& counters = descriptor
        ? hot.stats.allocationDescribedBySizeClass
        : hot.stats.allocationDescriptorlessBySizeClass;
    counters[index] += 1;
}

static void
brixGCRecordAllocationZeroedBytes(size_t bytes) {
    if (bytes) {
        gcHotAllocationState.stats.zeroedBytes += bytes;
    }
}

class BrixGCPanicSuspension {
public:
    BrixGCPanicSuspension()
        : panic(brixPanicSuspend()) {}

    ~BrixGCPanicSuspension() {
        brixPanicResume(panic);
    }

private:
    void* panic;
};

class BrixGCRuntimeCritical {
public:
    BrixGCRuntimeCritical() {
        brixGCEnterRuntimeCritical();
    }

    ~BrixGCRuntimeCritical() {
        brixGCLeaveRuntimeCritical();
    }

    BrixGCRuntimeCritical(const BrixGCRuntimeCritical&) = delete;
    BrixGCRuntimeCritical& operator=(const BrixGCRuntimeCritical&) = delete;
};

class BrixGCCompilerAllocationBufferFlush {
public:
    BrixGCCompilerAllocationBufferFlush() {
        gcAllocationBufferFlushInProgress = true;
    }

    ~BrixGCCompilerAllocationBufferFlush() {
        gcAllocationBufferFlushInProgress = false;
    }

    BrixGCCompilerAllocationBufferFlush(const BrixGCCompilerAllocationBufferFlush&) = delete;
    BrixGCCompilerAllocationBufferFlush& operator=(
        const BrixGCCompilerAllocationBufferFlush&) = delete;
};

class BrixGCRuntimeCall {
public:
    BrixGCRuntimeCall() {
        gcRuntimeCallDepth += 1;
    }

    ~BrixGCRuntimeCall() {
        if (gcRuntimeCallDepth > 0) {
            gcRuntimeCallDepth -= 1;
        }
        if (gcRuntimeCallDepth == 0 && gcDeferredCompilerAllocationBufferFlush) {
            gcDeferredCompilerAllocationBufferFlush = false;
            brixFlushGCCompilerAllocationBuffer();
        }
    }

    BrixGCRuntimeCall(const BrixGCRuntimeCall&) = delete;
    BrixGCRuntimeCall& operator=(const BrixGCRuntimeCall&) = delete;
};

template <typename F>
static auto
brixCallIgnoringPanic(F f) -> decltype(f()) {
    BrixGCPanicSuspension suspension;
    return f();
}

template <typename F>
static auto
brixCallGCIgnoringPanic(F f) -> decltype(f()) {
    BrixGCPanicSuspension suspension;
    BrixGCRuntimeCall runtimeCall;
    if (brixGCIsRuntimeCritical()) {
        return f();
    }
    BrixGCRuntimeCritical critical;
    return f();
}

template <typename F>
static auto
brixCallGCFallbackAllocation(F f) -> decltype(f()) {
    uint64_t start = brixGCMonotonicNanos();
    auto result = brixCallGCIgnoringPanic(f);
    uint64_t end = brixGCMonotonicNanos();
    size_t duration = static_cast<size_t>(end - start);
    gcFallbackAllocationCount.fetch_add(1, std::memory_order_relaxed);
    gcFallbackAllocationTimeNanos.fetch_add(duration, std::memory_order_relaxed);
    brixGCRecordSubphaseSample(
        BrixGCObservationFallbackAllocation,
        gcCurrentCycleId.load(std::memory_order_acquire),
        start,
        duration,
        result ? 1 : 0);
    return result;
}

static void*
brixGCPushTemporaryRoot(char* pointer) {
    if (void* root = brixGCAddIdleTemporaryRoot(pointer)) {
        return root;
    }
    return brixCallGCIgnoringPanic([&]() {
        return brixGCAddTemporaryRoot(pointer);
    });
}

static void
brixGCPopTemporaryRoot(void* root) {
    if (brixGCRemoveTaskTemporaryRoot(root)) {
        return;
    }
    brixCallGCIgnoringPanic([&]() {
        brixGCRemoveTemporaryRoot(root);
    });
}

class BrixGCTemporaryRoot {
public:
    explicit BrixGCTemporaryRoot(char* pointer) {
        if (pointer) {
            root = brixGCPushTemporaryRoot(pointer);
        }
    }

    ~BrixGCTemporaryRoot() {
        if (root) {
            brixGCPopTemporaryRoot(root);
        }
    }

    BrixGCTemporaryRoot(const BrixGCTemporaryRoot&) = delete;
    BrixGCTemporaryRoot& operator=(const BrixGCTemporaryRoot&) = delete;

private:
    void* root = nullptr;
};

static bool
brixReleaseCachedGCAllocationBatch(BrixGCThreadAllocationBatch& batch) {
    brixPublishGCThreadBatchConsumption();
    if (!batch.cursor || !batch.limit || batch.cursor >= batch.limit) {
        batch = BrixGCThreadAllocationBatch {};
        return true;
    }

    size_t generation = brixGCLoadPublicSweepGeneration();
    if (batch.generation != generation) {
        brixDiscardCachedGCAllocationBatch(batch);
        return true;
    }

    BrixGCThreadAllocationBatch releasedBatch = batch;
    bool released = brixCallGCIgnoringPanic([&]() {
        return brixGCReleaseUnusedBatch(
            releasedBatch.cursor,
            releasedBatch.limit,
            releasedBatch.slotSize,
            releasedBatch.generation);
    });
    if (released) {
        batch = BrixGCThreadAllocationBatch {};
    }
    return released;
}

static void
brixClearGCCompilerAllocationBuffer(BrixGCRuntimeAllocationBuffer& storage) {
    unsigned char adaptivePolicy[sizeof(storage.reserved)] {};
    std::memcpy(adaptivePolicy, storage.reserved, sizeof(adaptivePolicy));
    storage = BrixGCRuntimeAllocationBuffer {};
    brixGCInitializeRuntimeAllocationBuffer(&storage);
    std::memcpy(storage.reserved, adaptivePolicy, sizeof(adaptivePolicy));
}

static size_t
brixGCAdaptCompilerAllocationBufferAfterUse(BrixGCRuntimeAllocationBuffer& storage) {
    if (!storage.reserved[kBrixGCAdaptiveEnabledByte] ||
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
    size_t totalSlots = storage.reserved[kBrixGCAdaptiveReservedSlotCountByte];
    if (!totalSlots && storage.base && storage.buffer.limit > storage.base) {
        totalSlots = static_cast<size_t>(storage.buffer.limit - storage.base) /
            storage.slotSize;
    }
    size_t unusedSlots = cursor < storage.buffer.limit
        ? static_cast<size_t>(storage.buffer.limit - cursor) / storage.slotSize
        : 0;
    size_t baseCount = brixGCAllocationBatchPreferredCount(
        storage.slotSize,
        storage.descriptor);
    size_t count = std::max(
        baseCount,
        std::min(
            static_cast<size_t>(storage.reserved[kBrixGCAdaptiveCountByte]),
            kBrixGCAdaptiveAllocationBatchMaxCount));

    if (totalSlots && !unusedSlots) {
        size_t streak = storage.reserved[kBrixGCAdaptiveFullStreakByte] + 1;
        if (streak >= kBrixGCAdaptiveFullConsumptionStreak &&
                count < kBrixGCAdaptiveAllocationBatchMaxCount) {
            count = std::min(count * 2, kBrixGCAdaptiveAllocationBatchMaxCount);
            streak = 0;
            gcHotAllocationState.stats.adaptiveBatchGrowthCount += 1;
        }
        storage.reserved[kBrixGCAdaptiveCountByte] = static_cast<unsigned char>(count);
        storage.reserved[kBrixGCAdaptiveFullStreakByte] =
            static_cast<unsigned char>(streak);
        return 0;
    }

    storage.reserved[kBrixGCAdaptiveFullStreakByte] = 0;
    if (totalSlots &&
            unusedSlots * kBrixGCAdaptiveLowUseDenominator >=
                totalSlots * kBrixGCAdaptiveLowUseUnusedParts &&
            count > baseCount) {
        count = std::max(baseCount, count / 2);
        storage.reserved[kBrixGCAdaptiveCountByte] = static_cast<unsigned char>(count);
        gcHotAllocationState.stats.adaptiveBatchShrinkCount += 1;
    }
    return unusedSlots;
}

static void
brixRecordGCCompilerAllocationBufferConsumption(
    BrixGCRuntimeAllocationBuffer& storage,
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

    brixRecordGCPublicAllocations(count, storage.requestedSize, storage.slotSize);
#if BX_ENABLE_BENCH_METRICS
    if (brixBenchMetricsEnabledFlag) {
        size_t site = 0;
        // Compiler buffers use a stable 1..16 identity. The generic buffer has no compiler
        // identity and is therefore grouped into site 0.
        for (size_t index = 0; index < BrixGCCompilerAllocationBufferCount; ++index) {
            if (&storage == &brixGCEnsureActiveThreadCache()->compilerBuffers[index]) {
                site = index + 1;
                break;
            }
        }
        if (site > BrixGCCompilerAllocationBufferCount) {
            site = 0;
        }
        gcBenchAllocationSites[site].fetch_add(count, std::memory_order_relaxed);
    }
#endif
    brixGCRecordAllocationZeroedBytes(consumedBytes);
    gcHotAllocationState.stats.batchConsumedBytes += consumedBytes;
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > consumedBytes
        ? gcHotAllocationState.batchCacheBytes - consumedBytes
        : 0;

    size_t kind = storage.descriptor
        ? BrixGCAllocationCallDescribedObject
        : BrixGCAllocationCallObject;
    gcHotAllocationState.stats.allocationCallsByKind[kind] += count;
    if (storage.sizeClassIndex < kBrixGCAllocationBatchCount) {
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
brixPublishGCCompilerAllocationBufferConsumption(
    BrixGCRuntimeAllocationBuffer& storage
) {
    char* remainingCursor = nullptr;
    brixRecordGCCompilerAllocationBufferConsumption(storage, remainingCursor);
    if (remainingCursor) {
        // Consumption before this point is now globally accounted. Keep ownership of the
        // unconsumed tail and advance the accounting base so a later publication is delta-only.
        storage.base = remainingCursor;
    }
}

static void
brixPublishGCCompilerAllocationBufferConsumptions() {
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    if (gcActiveAllocationBufferStorage) {
        brixPublishGCCompilerAllocationBufferConsumption(
            *gcActiveAllocationBufferStorage);
    }
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        size_t slot = cache->activeCompilerBufferIndices[active];
        if (slot >= BrixGCCompilerAllocationBufferCount) {
            continue;
        }
        BrixGCRuntimeAllocationBuffer& storage = cache->compilerBuffers[slot];
        if (gcActiveAllocationBufferStorage != &storage) {
            brixPublishGCCompilerAllocationBufferConsumption(storage);
        }
    }
    for (size_t index = 0; index < BrixGCThreadCacheRangeCount; ++index) {
        BrixGCRuntimeAllocationBuffer& storage = cache->ranges[index];
        if (gcActiveAllocationBufferStorage != &storage) {
            brixPublishGCCompilerAllocationBufferConsumption(storage);
        }
        for (size_t way = 0; way < BrixGCThreadCacheParkedRangeWayCount; ++way) {
            brixPublishGCCompilerAllocationBufferConsumption(
                cache->parkedRanges[index][way]);
        }
    }
    gcHotAllocationState.stats.flush();
}

static size_t
brixGCCompilerAllocationBufferConsumedBytes(const BrixGCRuntimeAllocationBuffer& storage) {
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
brixGCAddPendingCompilerAllocationBufferStats(
    const BrixGCRuntimeAllocationBuffer& storage,
    size_t& objectCount,
    size_t& consumedBytes
) {
    size_t bytes = brixGCCompilerAllocationBufferConsumedBytes(storage);
    if (!bytes || !storage.slotSize) {
        return;
    }
    objectCount += bytes / storage.slotSize;
    consumedBytes += bytes;
}

static void
brixFlushGCCompilerAllocationBuffer(BrixGCRuntimeAllocationBuffer& storage) {
    if (gcAllocationBufferFlushInProgress) {
        return;
    }
    if (!storage.base && !storage.buffer.cursor && !storage.buffer.limit) {
        return;
    }

    BrixGCCompilerAllocationBufferFlush flush;
    size_t adaptiveUnusedSlots = brixGCAdaptCompilerAllocationBufferAfterUse(storage);
    char* remainingCursor = nullptr;
    brixRecordGCCompilerAllocationBufferConsumption(storage, remainingCursor);

    if (remainingCursor && storage.buffer.limit && remainingCursor < storage.buffer.limit) {
        BrixGCThreadAllocationBatch remaining {};
        remaining.cursor = remainingCursor;
        remaining.limit = storage.buffer.limit;
        remaining.slotSize = storage.slotSize;
        remaining.requestedSize = storage.requestedSize;
        remaining.descriptor = storage.descriptor;
        remaining.generation = storage.generation;
        if (brixReleaseCachedGCAllocationBatch(remaining)) {
            gcHotAllocationState.stats.adaptiveBatchReturnedUnusedSlotCount += adaptiveUnusedSlots;
        } else {
            brixDiscardCachedGCAllocationBatch(remaining);
        }
    }

    brixClearGCCompilerAllocationBuffer(storage);
}

static void
brixGCClearStagedGCAllocationRange(BrixGCRuntimeAllocationBuffer& storage) {
    if (brixGCAllocationRangeHasOwnership(storage)) {
        brixFlushGCCompilerAllocationBuffer(storage);
    }
    brixGCResetAdaptiveAllocationBufferPolicy(storage);
    brixGCInitializeEmptyRuntimeAllocationBuffer(storage);
}

static void
brixGCInvalidateStaleThreadRangeWays(
    BrixGCThreadCache* cache,
    size_t index,
    size_t generation
) {
    for (size_t way = 0; way < BrixGCThreadCacheRangeWayCount; ++way) {
        BrixGCRuntimeAllocationBuffer& range = brixGCThreadRangeWay(cache, index, way);
        if ((brixGCAllocationRangeHasOwnership(range) ||
                brixGCAllocationRangeHasPolicySite(range)) &&
                range.generation != generation) {
            brixGCClearStagedGCAllocationRange(range);
        }
    }
}

static size_t
brixGCFindThreadRangeWay(
    BrixGCThreadCache* cache,
    size_t index,
    size_t requestedSize,
    size_t allocatedSize,
    void* descriptor,
    size_t generation
) {
    size_t policyOnlyWay = BrixGCThreadCacheRangeWayCount;
    for (size_t way = 0; way < BrixGCThreadCacheRangeWayCount; ++way) {
        BrixGCRuntimeAllocationBuffer& range = brixGCThreadRangeWay(cache, index, way);
        if (!brixGCAllocationRangeMatchesSite(
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
        if (policyOnlyWay == BrixGCThreadCacheRangeWayCount) {
            policyOnlyWay = way;
        }
    }
    return policyOnlyWay;
}

static size_t
brixGCFindDescriptorlessThreadRangeWay(
    BrixGCThreadCache* cache,
    size_t index,
    size_t allocatedSize,
    size_t generation
) {
    for (size_t way = 1; way < BrixGCThreadCacheRangeWayCount; ++way) {
        BrixGCRuntimeAllocationBuffer& range = brixGCThreadRangeWay(cache, index, way);
        if (!range.descriptor &&
                range.generation == generation &&
                range.slotSize >= allocatedSize &&
                brixGCAllocationRangeHasUnconsumedSlots(range)) {
            return way;
        }
    }
    return BrixGCThreadCacheRangeWayCount;
}

static size_t
brixGCChooseThreadRangeVictimWay(
    BrixGCThreadCache* cache,
    size_t index,
    size_t firstWay
) {
    for (size_t way = firstWay; way < BrixGCThreadCacheRangeWayCount; ++way) {
        BrixGCRuntimeAllocationBuffer& range = brixGCThreadRangeWay(cache, index, way);
        if (!brixGCAllocationRangeHasOwnership(range) &&
                !brixGCAllocationRangeHasPolicySite(range)) {
            return way;
        }
    }

    size_t wayCount = BrixGCThreadCacheRangeWayCount - firstWay;
    size_t victim = firstWay + cache->rangeNextVictimWay[index] % wayCount;
    cache->rangeNextVictimWay[index] = static_cast<unsigned char>(
        (cache->rangeNextVictimWay[index] + 1) % wayCount);
    return victim;
}

static void
brixGCParkGCAllocationRange(
    BrixGCThreadCache* cache,
    size_t index,
    BrixGCRuntimeAllocationBuffer& source,
    size_t firstWay
) {
    if (!brixGCAllocationRangeIsParkable(source)) {
        brixFlushGCCompilerAllocationBuffer(source);
        return;
    }

    size_t victimWay = brixGCChooseThreadRangeVictimWay(cache, index, firstWay);
    BrixGCRuntimeAllocationBuffer& victim =
        brixGCThreadRangeWay(cache, index, victimWay);
    brixGCClearStagedGCAllocationRange(victim);
    brixGCMoveRuntimeAllocationBuffer(victim, source);
}

static bool
brixGCTakeParkedGCAllocationRange(
    BrixGCThreadCache* cache,
    const BrixGCAllocationSiteKey& key,
    size_t generation,
    BrixGCRuntimeAllocationBuffer& destination
) {
    brixGCInvalidateStaleThreadRangeWays(cache, key.sizeClassIndex, generation);
    size_t way = brixGCFindThreadRangeWay(
        cache,
        key.sizeClassIndex,
        key.requestedSize,
        key.allocatedSize,
        key.descriptor,
        generation);
    if (way == BrixGCThreadCacheRangeWayCount) {
        return false;
    }
    BrixGCRuntimeAllocationBuffer& range =
        brixGCThreadRangeWay(cache, key.sizeClassIndex, way);
    brixGCMoveRuntimeAllocationBuffer(destination, range);
    return true;
}

static void
brixGCParkCompilerGCAllocationRange(
    BrixGCThreadCache* cache,
    BrixGCRuntimeAllocationBuffer& storage,
    size_t generation
) {
    if (!storage.descriptor || storage.sizeClassIndex >= BrixGCThreadCacheRangeCount) {
        brixFlushGCCompilerAllocationBuffer(storage);
        return;
    }
    if (storage.generation != generation) {
        brixGCClearStagedGCAllocationRange(storage);
        return;
    }

    brixPublishGCCompilerAllocationBufferConsumption(storage);
    if (!storage.buffer.cursor || !storage.buffer.limit ||
            storage.buffer.cursor >= storage.buffer.limit) {
        brixGCAdaptCompilerAllocationBufferAfterUse(storage);
    }
    brixGCParkGCAllocationRange(cache, storage.sizeClassIndex, storage, 0);
}

static bool
brixGCCompilerAllocationBufferIndexIsValid(size_t index) {
    return index > 0 && index <= BrixGCCompilerAllocationBufferCount;
}

static void
brixMarkGCCompilerAllocationBufferActive(size_t index) {
    if (!brixGCCompilerAllocationBufferIndexIsValid(index)) {
        return;
    }

    size_t slot = index - 1;
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        if (cache->activeCompilerBufferIndices[active] == slot) {
            return;
        }
    }
    if (cache->activeCompilerBufferCount >= BrixGCCompilerAllocationBufferCount) {
        return;
    }

    cache->activeCompilerBufferIndices[cache->activeCompilerBufferCount++] =
        static_cast<uint32_t>(slot);
}

static void
brixFlushGCCompilerAllocationBuffer() {
    if (gcRuntimeCallDepth > 0 && !gcAllocationBufferFlushInProgress) {
        gcDeferredCompilerAllocationBufferFlush = true;
        return;
    }
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
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
        brixFlushGCCompilerAllocationBuffer(*gcActiveAllocationBufferStorage);
    }
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        size_t slot = cache->activeCompilerBufferIndices[active];
        if (slot >= BrixGCCompilerAllocationBufferCount) {
            continue;
        }
        bool compilerBufferLive =
            cache->compilerBuffers[slot].base ||
            cache->compilerBuffers[slot].buffer.cursor ||
            cache->compilerBuffers[slot].buffer.limit;
        flushedOwnerState = flushedOwnerState || compilerBufferLive;
        flushedBufferCount += compilerBufferLive ? 1 : 0;
        brixFlushGCCompilerAllocationBuffer(cache->compilerBuffers[slot]);
    }
    cache->activeCompilerBufferCount = 0;
    for (size_t index = 0; index < BrixGCThreadCacheRangeCount; ++index) {
        bool rangeLive =
            cache->ranges[index].base ||
            cache->ranges[index].buffer.cursor ||
            cache->ranges[index].buffer.limit;
        flushedOwnerState = flushedOwnerState || rangeLive;
        flushedBufferCount += rangeLive ? 1 : 0;
        brixFlushGCCompilerAllocationBuffer(cache->ranges[index]);
        for (size_t way = 0; way < BrixGCThreadCacheParkedRangeWayCount; ++way) {
            BrixGCRuntimeAllocationBuffer& parked = cache->parkedRanges[index][way];
            bool parkedLive =
                parked.base || parked.buffer.cursor || parked.buffer.limit;
            flushedOwnerState = flushedOwnerState || parkedLive;
            flushedBufferCount += parkedLive ? 1 : 0;
            brixFlushGCCompilerAllocationBuffer(parked);
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
brixFlushGCAllocationBatches() {
    brixFlushGCCompilerAllocationBuffer();
}

static bool
brixGCPhaseNeedsMarkBarrier(unsigned char phase) {
    return phase >= 1 && phase <= 4;
}

static bool
brixGCPhaseNeedsSweepPublicationValidation(unsigned char phase) {
    return phase == kBrixGCPhaseSweep;
}

static BrixGCBarrierStatsRecord*
brixGCEnsureThreadBarrierStats() {
    if (gcThreadBarrierStats) {
        return gcThreadBarrierStats;
    }
    BrixGCBarrierStatsRecord* record = new BrixGCBarrierStatsRecord();
    {
        std::lock_guard<std::mutex> lock(gcBarrierStatsMutex);
        gcBarrierStatsRecords.push_back(record);
    }
    gcThreadBarrierStats = record;
    return record;
}

static void
brixGCAddOwnedBarrierCounter(std::atomic<size_t>& counter, size_t amount) {
    if (!amount) {
        return;
    }
    // This record has exactly one OS-thread writer. Atomic load/store lets cold metric readers
    // sample it without putting a shared read-modify-write on barrier or buffer-flush paths.
    counter.store(counter.load(std::memory_order_relaxed) + amount,
        std::memory_order_relaxed);
}

static size_t
brixGCSumBarrierCounter(BrixGCBarrierStatsKind kind) {
    std::lock_guard<std::mutex> lock(gcBarrierStatsMutex);
    size_t total = 0;
    for (BrixGCBarrierStatsRecord* record : gcBarrierStatsRecords) {
        const std::atomic<size_t>* counter = nullptr;
        switch (kind) {
            case BrixGCBarrierStatsCalls:
                counter = &record->barrierCalls;
                break;
            case BrixGCBarrierStatsSlowEntries:
                counter = &record->barrierSlowEntries;
                break;
            case BrixGCBarrierStatsShades:
                counter = &record->barrierShades;
                break;
            case BrixGCBarrierStatsFastCalls:
                counter = &record->fastCalls;
                break;
            case BrixGCBarrierStatsFastSlowEntries:
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
brixPublishGCWriteBarrierCounters(BrixGCThreadCache* cache) {
    if (!cache) {
        return;
    }
    uint64_t calls = cache->counters[kBrixGCThreadCounterBarrierCalls];
    uint64_t slowEntries = cache->counters[kBrixGCThreadCounterBarrierSlowEntries];
    uint64_t shades = cache->counters[kBrixGCThreadCounterBarrierShades];
    cache->counters[kBrixGCThreadCounterBarrierCalls] = 0;
    cache->counters[kBrixGCThreadCounterBarrierSlowEntries] = 0;
    cache->counters[kBrixGCThreadCounterBarrierShades] = 0;
    if (!calls && !slowEntries && !shades) {
        return;
    }
    BrixGCBarrierStatsRecord* record = brixGCEnsureThreadBarrierStats();
    brixGCAddOwnedBarrierCounter(record->fastCalls, calls);
    brixGCAddOwnedBarrierCounter(record->barrierCalls, calls);
    brixGCAddOwnedBarrierCounter(record->fastSlowEntries, slowEntries);
    brixGCAddOwnedBarrierCounter(record->barrierSlowEntries, slowEntries);
    brixGCAddOwnedBarrierCounter(record->barrierShades, shades);
}

extern "C" void
brixBenchRecordBarrierCall() {
    // Keep the compiler hot path to one thread-owned increment. Task suspension, termination,
    // handshake acknowledgement, thread-cache detachment, and metric reads all publish this
    // cache into its OS-thread-owned record without a shared RMW.
    brixGCEnsureActiveThreadCache()->counters[kBrixGCThreadCounterBarrierCalls] += 1;
}

extern "C" void
brixWBBufFlush() {
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    size_t count = std::min<size_t>(
        cache->writeBarrierCount,
        BrixGCWriteBarrierBufferCapacity);
    if (count) {
        size_t shades = brixGCFlushWriteBarrierEntries(cache->writeBarrierEntries, count);
        if (shades) {
            BrixGCBarrierStatsRecord* record = brixGCEnsureThreadBarrierStats();
            brixGCAddOwnedBarrierCounter(record->barrierShades, shades);
        }
        for (size_t index = 0; index < count; ++index) {
            cache->writeBarrierEntries[index] = nullptr;
        }
        cache->writeBarrierCount = 0;
    }
    brixPublishGCWriteBarrierCounters(cache);
}

static void
brixWBBufRecordActiveTaskPointer(char* pointer) {
    if (!pointer) {
        return;
    }
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    if (cache->writeBarrierCount >= BrixGCWriteBarrierBufferCapacity) {
        brixWBBufFlush();
    }
    cache->writeBarrierEntries[cache->writeBarrierCount++] = pointer;
    if (cache->writeBarrierCount == BrixGCWriteBarrierBufferCapacity) {
        brixWBBufFlush();
    }
}

extern "C" void
brixWBBufRecordPointer(char* pointer) {
    if (!pointer) {
        return;
    }
    if (!brixGCActiveTaskCanUseFastAllocation()) {
        brixGCWriteBarrier(nullptr, pointer);
        return;
    }
    brixWBBufRecordActiveTaskPointer(pointer);
}

static uint64_t
brixGCMonotonicNanos() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static bool
brixGCValidSubphase(size_t subphase) {
    return subphase > 0 && subphase < kBrixGCSubphaseCount;
}

static void
brixGCSubtractAtomicSaturating(std::atomic<size_t>& value, size_t amount) {
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
brixGCMaxAtomic(std::atomic<size_t>& value, size_t amount) {
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
brixGCRecordSubphaseSample(
    size_t subphase,
    size_t,
    uint64_t,
    size_t durationNanos,
    size_t value
) {
    if (!brixGCValidSubphase(subphase)) {
        return;
    }

    if (subphase == BrixGCObservationRootScan) {
        if (durationNanos) {
            gcPacerCurrentRootScanNanos.store(durationNanos, std::memory_order_release);
        }
    } else if (subphase == BrixGCObservationMarkWork) {
        gcPacerCurrentMarkWorkItems.fetch_add(value, std::memory_order_relaxed);
        if (durationNanos) {
            gcPacerCurrentMarkWorkNanos.fetch_add(durationNanos, std::memory_order_relaxed);
        }
    } else if (subphase == BrixGCObservationAssist) {
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
    BrixGCSubphaseSamples& samples = gcSubphaseSamples[subphase];
    ++samples.sampleSequence;
    if (samples.durations.size() < kBrixGCSubphaseSampleCapacity) {
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
        if (replacement < kBrixGCSubphaseSampleCapacity) {
            samples.durations[static_cast<size_t>(replacement)] = durationNanos;
        }
    }
    samples.maxDurationNanos = std::max(samples.maxDurationNanos, durationNanos);

}

static size_t
brixGCSubphasePercentile(size_t subphase, size_t percentile) {
    if (!brixGCValidSubphase(subphase)) {
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
brixGCValidPauseKind(size_t kind) {
    return kind > 0 && kind < BrixGCPauseKindCount;
}

static size_t
brixGCPauseKindForHandshakeReason(size_t reason) {
    switch (reason) {
        case 1:
            return BrixGCPauseKindMarkSetupTotal;
        case 2:
            return BrixGCPauseKindTermConfirm;
        case 3:
            return BrixGCPauseKindSweepTotal;
        default:
            return 0;
    }
}

static void
brixGCRecordPauseEvent(
    size_t kind,
    size_t cycleId,
    uint64_t startNanos,
    size_t durationNanos,
    size_t tasksWaited,
    size_t slowestTaskNanos
) {
    if (!brixGCValidPauseKind(kind) || !durationNanos) {
        return;
    }

    gcPauseCount.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    size_t eventIndex = gcPauseEventSequence % kBrixGCPauseEventCapacity;
    gcPauseEvents[eventIndex] = BrixGCPauseEvent {
        cycleId,
        kind,
        startNanos,
        durationNanos,
        tasksWaited,
        slowestTaskNanos
    };
    ++gcPauseEventSequence;
    if (gcPauseEventStored < kBrixGCPauseEventCapacity) {
        ++gcPauseEventStored;
    }
}

static size_t
brixGCPausePercentile(size_t kind, size_t percentile) {
    if (!brixGCValidPauseKind(kind)) {
        return 0;
    }

    std::vector<size_t> durations;
    {
        std::lock_guard<std::mutex> lock(gcObservabilityMutex);
        durations.reserve(gcPauseEventStored);
        size_t firstSequence = gcPauseEventSequence - gcPauseEventStored;
        for (size_t index = 0; index < gcPauseEventStored; ++index) {
            const BrixGCPauseEvent& event =
                gcPauseEvents[(firstSequence + index) % kBrixGCPauseEventCapacity];
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
brixGCRemainingBatchBytes(const BrixGCThreadAllocationBatch& batch) {
    if (!batch.cursor || !batch.limit || batch.cursor >= batch.limit) {
        return 0;
    }
    return static_cast<size_t>(batch.limit - batch.cursor);
}

static void
brixGCRecordBatchDiscard(size_t bytes) {
    brixPublishGCThreadBatchConsumption();
    if (!bytes) {
        return;
    }
    gcBatchDiscardedBytes.fetch_add(bytes, std::memory_order_relaxed);
    gcStaleBatchDropCount.fetch_add(1, std::memory_order_relaxed);
    brixGCSubtractAtomicSaturating(gcBatchCacheBytes, bytes);
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > bytes ? gcHotAllocationState.batchCacheBytes - bytes : 0;
}

static void
brixDiscardCachedGCAllocationBatch(BrixGCThreadAllocationBatch& batch) {
    brixPublishGCThreadBatchConsumption();
    size_t bytes = brixGCRemainingBatchBytes(batch);
    if (bytes) {
        BrixGCThreadAllocationBatch discardedBatch = batch;
        bool discarded = brixCallGCIgnoringPanic([&]() {
            return brixGCDiscardUnusedBatch(
                discardedBatch.cursor,
                discardedBatch.limit,
                discardedBatch.slotSize,
                discardedBatch.generation);
        });
        if (!discarded) {
            brixGCRecordBatchDiscard(bytes);
        }
    }
    batch = BrixGCThreadAllocationBatch {};
}

static void
brixGCRecordAllocationStall(uint64_t startNanos, uint64_t endNanos) {
    size_t duration = static_cast<size_t>(endNanos - startNanos);
    gcAllocationStallTimeNanos.fetch_add(duration, std::memory_order_relaxed);
    brixGCRecordSubphaseSample(
        BrixGCObservationAllocationStall,
        gcCurrentCycleId.load(std::memory_order_acquire),
        startNanos,
        duration,
        gcCycleAllocatedBytes.load(std::memory_order_relaxed));
    brixGCRecordPauseEvent(
        BrixGCPauseKindStallReport,
        gcCurrentCycleId.load(std::memory_order_acquire),
        startNanos,
        duration,
        0,
        0);
}

static size_t
brixGCEnvSize(const char* name, size_t fallback) {
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
brixGCEnvSizeAllowZero(const char* name, size_t fallback) {
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
brixGCMultiplyDivideSaturating(size_t value, size_t multiplier, size_t divisor) {
    if (divisor == 0 || value == 0 || multiplier == 0) {
        return 0;
    }
    if (value > std::numeric_limits<size_t>::max() / multiplier) {
        return std::numeric_limits<size_t>::max();
    }
    return (value * multiplier) / divisor;
}

static size_t
brixGCHeapHardCap() {
    if (brixGCCollectionDisabled()) {
        return 0;
    }
    static const size_t hardCap = brixGCEnvSizeAllowZero(
        "BRIX_GC_MEM_LIMIT",
        kBrixGCDefaultHeapHardCap);
    return hardCap;
}

static size_t
brixGCApplyHeapHardCap(size_t goal) {
    size_t hardCap = brixGCHeapHardCap();
    return hardCap ? std::min(goal, hardCap) : goal;
}

static std::atomic<size_t> gcProgramMinimumHeapGoal {0};

static size_t
brixGCMinimumHeapGoal() {
    if (brixGCCollectionDisabled()) {
        return std::numeric_limits<size_t>::max();
    }
    static const size_t configuredGoal = brixGCEnvSize("BRIX_GC_MIN_HEAP_GOAL", 0);
    size_t goal = configuredGoal ? configuredGoal : gcProgramMinimumHeapGoal.load(std::memory_order_acquire);
    return brixGCApplyHeapHardCap(goal ? goal : kBrixGCDefaultHeapGoal);
}

static size_t
brixGCHeapMinGrowth() {
    static const size_t minGrowth =
        brixGCEnvSize("BRIX_GC_HEAP_MIN_GROWTH", kBrixGCDefaultHeapMinGrowth);
    return minGrowth;
}

static size_t
brixGCHeapGrowthPercent() {
    static const size_t growthPercent = brixGCEnvSize(
        "BRIX_GC_GROWTH",
        kBrixGCDefaultHeapGrowthPercent);
    return growthPercent;
}

static size_t
brixGCThreadBatchCacheByteCap() {
    static const size_t cap = brixGCEnvSize(
        "BRIX_GC_THREAD_BATCH_CACHE_BYTES",
        kBrixGCDefaultThreadBatchCacheBytes);
    return cap;
}

static size_t
brixGCGlobalBatchCacheByteCap() {
    static const size_t cap = brixGCEnvSize(
        "BRIX_GC_GLOBAL_BATCH_CACHE_BYTES",
        kBrixGCDefaultGlobalBatchCacheBytes);
    return cap;
}

static bool
brixGCBatchCacheCapAllows(size_t bytes) {
    brixPublishGCThreadBatchConsumption();
    if (!bytes) {
        return true;
    }

    size_t threadCap = brixGCThreadBatchCacheByteCap();
    if (threadCap == 0 || bytes > threadCap || gcHotAllocationState.batchCacheBytes > threadCap - bytes) {
        return false;
    }

    size_t globalCap = brixGCGlobalBatchCacheByteCap();
    size_t globalBytes = gcBatchCacheBytes.load(std::memory_order_relaxed);
    if (globalCap == 0 || bytes > globalCap || globalBytes > globalCap - bytes) {
        return false;
    }

    return true;
}

static bool
brixGCBatchCacheReserve(size_t bytes) {
    brixPublishGCThreadBatchConsumption();
    if (!bytes) {
        return true;
    }

    size_t threadCap = brixGCThreadBatchCacheByteCap();
    if (threadCap == 0 || bytes > threadCap || gcHotAllocationState.batchCacheBytes > threadCap - bytes) {
        return false;
    }

    size_t globalCap = brixGCGlobalBatchCacheByteCap();
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
brixGCBatchCacheCancelReservation(size_t bytes) {
    if (!bytes) {
        return;
    }
    size_t canceled = std::min(bytes, gcHotAllocationState.batchPendingReservationBytes);
    gcHotAllocationState.batchPendingReservationBytes -= canceled;
    brixGCSubtractAtomicSaturating(gcBatchCacheBytes, canceled);
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > canceled
        ? gcHotAllocationState.batchCacheBytes - canceled
        : 0;
}

static bool
brixGCEnvFlagEnabled(const char* name) {
    const char* value = std::getenv(name);
    return value && *value && std::strcmp(value, "0") != 0;
}

static size_t
brixGCStressAllocationInterval() {
    static const size_t interval = brixGCEnvFlagEnabled("BRIX_GC_STRESS")
        ? brixGCEnvSize("BRIX_GC_STRESS", 1)
        : 0;
    return interval;
}

static size_t
brixGCStressByteInterval() {
    static const size_t interval = brixGCEnvSizeAllowZero("BRIX_GC_STRESS_BYTES", 0);
    return interval;
}

static uint64_t
brixGCReplaySeed() {
    static const uint64_t seed = []() -> uint64_t {
        if (brixGCEnvFlagEnabled("BRIX_GC_REPLAY_SEED")) {
            return static_cast<uint64_t>(brixGCEnvSize("BRIX_GC_REPLAY_SEED", 1));
        }
        if (brixGCEnvFlagEnabled("BRIX_GC_REPLAY")) {
            return static_cast<uint64_t>(brixGCEnvSize("BRIX_GC_REPLAY", 1));
        }
        return 0;
    }();
    return seed;
}

static size_t
brixGCReplayAllocationInterval() {
    static const size_t interval = brixGCEnvSize("BRIX_GC_REPLAY_INTERVAL", 4);
    return interval;
}

static bool
brixGCStressModeEnabled() {
    return brixGCStressAllocationInterval() != 0 || brixGCStressByteInterval() != 0;
}

static bool
brixGCReplayModeEnabled() {
    return brixGCReplaySeed() != 0;
}

static bool
brixGCVerificationSlowPathsEnabled() {
    static const bool enabled = brixGCStressModeEnabled() || brixGCReplayModeEnabled();
    return enabled;
}

static size_t
brixGCAutomaticSoftGoal(size_t goal) {
    size_t cachedGoal = gcPacerGoalCacheGoal.load(std::memory_order_acquire);
    size_t cachedSoftGoal = gcPacerGoalCacheSoftGoal.load(std::memory_order_acquire);
    if (cachedGoal == goal && cachedSoftGoal != 0) {
        return cachedSoftGoal;
    }

    size_t softGoal = brixGCPacerSoftGoal(goal);
    gcPacerGoalCacheSoftGoal.store(softGoal, std::memory_order_release);
    gcPacerGoalCacheHardGoal.store(brixGCAutomaticHardGoal(goal), std::memory_order_release);
    gcPacerGoalCacheGoal.store(goal, std::memory_order_release);
    return softGoal;
}

static size_t
brixGCAutomaticHardGoal(size_t goal) {
    size_t cachedGoal = gcPacerGoalCacheGoal.load(std::memory_order_acquire);
    size_t cachedHardGoal = gcPacerGoalCacheHardGoal.load(std::memory_order_acquire);
    if (cachedGoal == goal && cachedHardGoal != 0) {
        return cachedHardGoal;
    }

    size_t hardGoal = brixGCPacerHardGoal(goal);
    gcPacerGoalCacheHardGoal.store(hardGoal, std::memory_order_release);
    return hardGoal;
}

static size_t
brixGCAutomaticAssistWork() {
    size_t throughput = gcPacerLastMarkThroughputItemsPerSecond.load(std::memory_order_relaxed);
    if (!throughput) {
        return kBrixGCAutomaticAssistWork;
    }
    size_t slice = brixGCMultiplyDivideSaturating(
        throughput,
        kBrixGCMaxAssistSliceNanos,
        1000000000ull);
    return std::max<size_t>(1, std::min<size_t>(slice, kBrixGCAutomaticAssistWork));
}

static size_t
brixGCAutomaticHardAssistWork() {
    return std::min<size_t>(
        kBrixGCAutomaticHardAssistWork,
        std::max<size_t>(1, brixGCAutomaticAssistWork() * 2));
}

static size_t
brixGCAutomaticIdleWork() {
    return kBrixGCAutomaticIdleWork;
}

static std::atomic<size_t> gcScavengedPageTotal {0};
static std::atomic<bool> gcMetadataTrimPending {false};
static std::atomic<uint64_t> gcIdleScavengePicosPerPage {0};

static size_t
brixGCIdleScavengePageLimit(uint64_t budgetNanos, size_t debt, size_t minimum) {
    size_t limit = kBrixGCAutomaticIdleScavengePages;
    uint64_t picosPerPage = gcIdleScavengePicosPerPage.load(std::memory_order_relaxed);
    if (picosPerPage) {
        limit = static_cast<size_t>(std::min<uint64_t>(
            budgetNanos * 1000ull / picosPerPage,
            kBrixGCIdleScavengeMaximumPages));
    }
    return std::min(debt, std::max(limit, minimum));
}

static void
brixGCRecordIdleScavengeCost(uint64_t elapsedNanos, size_t released) {
    uint64_t sample = std::max<uint64_t>(elapsedNanos * 1000ull / released, 1);
    uint64_t previous = gcIdleScavengePicosPerPage.load(std::memory_order_relaxed);
    gcIdleScavengePicosPerPage.store(
        previous ? previous - previous / 4 + sample / 4 : sample,
        std::memory_order_relaxed);
}

static size_t
brixGCAutomaticTerminationQueueBudget() {
    return kBrixGCAutomaticTerminationQueueBudget;
}

static size_t
brixGCAutomaticMaxActiveCycleNanos() {
    static const size_t maxActiveCycleNanos = brixGCEnvSize(
        "BRIX_GC_MAX_ACTIVE_CYCLE_NANOS",
        kBrixGCAutomaticMaxActiveCycleNanos);
    return maxActiveCycleNanos;
}

static bool
brixGCAllocationAssistDisabled() {
    static const bool disabled =
        brixGCEnvFlagEnabled("BRIX_GC_DISABLE_AUTOMATIC_PROGRESS") ||
        brixGCEnvFlagEnabled("BRIX_GC_DISABLE_ALLOCATION_ASSIST");
    return disabled;
}

static bool
brixGCIdleProgressDisabled() {
    static const bool disabled =
        brixGCEnvFlagEnabled("BRIX_GC_DISABLE_AUTOMATIC_PROGRESS") ||
        brixGCEnvFlagEnabled("BRIX_GC_DISABLE_IDLE_PROGRESS");
    return disabled;
}

static bool
brixGCBytesReachThreshold(size_t live, size_t upcomingSize, size_t threshold) {
    if (live >= threshold) {
        return true;
    }
    return upcomingSize >= threshold - live;
}

static size_t
brixGCNativePageSize() {
    static const size_t pageSize = []() {
        long value = sysconf(_SC_PAGESIZE);
        return value > 0 ? static_cast<size_t>(value) : 4096ull;
    }();
    return pageSize;
}

static size_t
brixGCPacerThroughput(size_t workItems, size_t durationNanos) {
    return brixGCMultiplyDivideSaturating(workItems, 1000000000ull, durationNanos);
}

static size_t
brixGCPacerUpdateEWMA(std::atomic<size_t>& target, size_t sample) {
    size_t previous = target.load(std::memory_order_relaxed);
    size_t next = previous
        ? brixGCMultiplyDivideSaturating(previous, 7, 8) + sample / 8
        : sample;
    target.store(next, std::memory_order_release);
    return next;
}

static size_t
brixGCPacerDebtBytes() {
    size_t sweepDebt = gcPacerLastSweepDebt.load(std::memory_order_relaxed);
    size_t scavengeDebt = gcPacerLastScavengeDebt.load(std::memory_order_relaxed);
    size_t debt = sweepDebt;
    if (debt > std::numeric_limits<size_t>::max() - scavengeDebt) {
        debt = std::numeric_limits<size_t>::max();
    } else {
        debt += scavengeDebt;
    }
    return brixGCMultiplyDivideSaturating(debt, brixGCNativePageSize(), 1);
}

static size_t
brixGCPacerTriggerReserveBytes(size_t goal) {
    if (goal == 0) {
        return 0;
    }

    size_t allocationRate = gcPacerAllocationRateEWMA.load(std::memory_order_relaxed);
    size_t markNanos = gcPacerMarkDurationEWMA.load(std::memory_order_relaxed);
    size_t reserve = brixGCMultiplyDivideSaturating(
        allocationRate,
        markNanos,
        1000000000ull);
    size_t fixedReserve = std::max<size_t>(brixGCNativePageSize() * 16, goal / 64);
    reserve = std::min(std::numeric_limits<size_t>::max() - reserve, fixedReserve) + reserve;
    size_t correction = gcPacerLastGoalOvershootBytes.load(std::memory_order_relaxed) / 2;
    reserve = std::min(std::numeric_limits<size_t>::max() - reserve, correction) + reserve;
    size_t debtReserve = std::min(goal / 8, brixGCPacerDebtBytes());
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
brixGCPacerSoftGoal(size_t goal) {
    size_t reserve = brixGCPacerTriggerReserveBytes(goal);
    gcPacerLastTriggerReserveBytes.store(reserve, std::memory_order_release);
    if (reserve >= goal) {
        return 1;
    }
    return std::max<size_t>(1, goal - reserve);
}

static size_t
brixGCPacerHardGoal(size_t goal) {
    return brixGCApplyHeapHardCap(goal);
}

static void
brixGCPacerInvalidateGoalCache() {
    gcPacerGoalCacheGoal.store(0, std::memory_order_release);
    gcPacerGoalCacheSoftGoal.store(0, std::memory_order_release);
    gcPacerGoalCacheHardGoal.store(0, std::memory_order_release);
}

static bool
brixGCHardCapReached(size_t upcomingSize) {
    size_t hardCap = brixGCHeapHardCap();
    if (hardCap == 0) {
        return false;
    }
    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    return brixGCBytesReachThreshold(live, upcomingSize, hardCap);
}

static void
brixGCPacerPublishCycleStats(size_t live, size_t goal) {
    brixPublishGCThreadAllocationStats();
    uint64_t start = gcActiveCycleStartNanos.load(std::memory_order_acquire);
    size_t cycleNanos = start
        ? static_cast<size_t>(brixGCMonotonicNanos() - start)
        : 0;
    size_t allocated = gcCycleAllocatedBytes.load(std::memory_order_acquire);
    size_t allocationRateSample = brixGCMultiplyDivideSaturating(
        allocated,
        1000000000ull,
        cycleNanos);
    size_t allocationRate = brixGCPacerUpdateEWMA(
        gcPacerAllocationRateEWMA,
        allocationRateSample);
    size_t markDuration = brixGCPacerUpdateEWMA(
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
        brixGCPacerThroughput(
            gcPacerCurrentMarkWorkItems.load(std::memory_order_acquire),
            gcPacerCurrentMarkWorkNanos.load(std::memory_order_acquire)),
        std::memory_order_release);
    gcPacerLastAssistThroughputItemsPerSecond.store(
        brixGCPacerThroughput(
            gcPacerCurrentAssistWorkItems.load(std::memory_order_acquire),
            gcPacerCurrentAssistWorkNanos.load(std::memory_order_acquire)),
        std::memory_order_release);
    gcPacerLastSweepDebt.store(
        brixGCIncrementalSweepDebtPages(),
        std::memory_order_release);
    gcPacerLastScavengeDebt.store(brixGCScavengeDebt(), std::memory_order_release);
    brixGCPacerInvalidateGoalCache();
    gcPacerLastSoftGoal.store(brixGCAutomaticSoftGoal(goal), std::memory_order_release);
    gcPacerLastHardGoal.store(brixGCAutomaticHardGoal(goal), std::memory_order_release);
}

static size_t
brixGCObjectAllocationSize(size_t size) {
    size_t minimum = std::max(size, kBrixGCObjectAlignment);
    if (minimum > std::numeric_limits<size_t>::max() - (kBrixGCObjectAlignment - 1)) {
        std::fprintf(stderr, "Brix GC allocation size overflow\n");
        std::abort();
    }
    return (minimum + kBrixGCObjectAlignment - 1) & ~(kBrixGCObjectAlignment - 1);
}

static size_t
brixGCAllocationBatchIndex(size_t allocSize) {
    if (allocSize <= 1024) {
        return (allocSize + 7) / 8;
    }

    return kBrixGCAllocationBatchSmallCount +
        (allocSize - 1025) / kBrixGCAllocationBatchLargeBucketSize;
}

static bool
brixGCAllocationSiteKeyIsValid(const BrixGCAllocationSiteKey* key) {
    if (!key) {
        return false;
    }
    if (
        key->abiVersion != BrixGCAllocationABIVersion ||
        key->structSize != sizeof(BrixGCAllocationSiteKey)
    ) {
        return false;
    }
    if (key->pointerKind > kBrixGCAllocationPointerful ||
            key->zeroingPolicy != kBrixGCAllocationZeroFull) {
        return false;
    }

    size_t allocSize = brixGCObjectAllocationSize(key->requestedSize);
    if (allocSize != key->allocatedSize || allocSize > kBrixGCAllocationBatchMaxSize) {
        return false;
    }

    size_t index = brixGCAllocationBatchIndex(allocSize);
    return index == key->sizeClassIndex && index < kBrixGCAllocationBatchCount;
}

static bool
brixGCInitAllocationSiteKey(
    BrixGCAllocationSiteKey* key,
    size_t requestedSize,
    void* descriptor,
    size_t pointerKind,
    size_t zeroingPolicy
) {
    if (!key || pointerKind > kBrixGCAllocationPointerful ||
            zeroingPolicy != kBrixGCAllocationZeroFull) {
        return false;
    }

    size_t allocSize = brixGCObjectAllocationSize(requestedSize);
    if (allocSize > kBrixGCAllocationBatchMaxSize) {
        *key = BrixGCAllocationSiteKey {};
        key->abiVersion = BrixGCAllocationABIVersion;
        key->structSize = sizeof(BrixGCAllocationSiteKey);
        key->requestedSize = requestedSize;
        key->allocatedSize = allocSize;
        key->descriptor = descriptor;
        key->pointerKind = static_cast<unsigned char>(pointerKind);
        key->zeroingPolicy = static_cast<unsigned char>(zeroingPolicy);
        return false;
    }

    size_t index = brixGCAllocationBatchIndex(allocSize);
    if (index >= kBrixGCAllocationBatchCount) {
        return false;
    }

    *key = BrixGCAllocationSiteKey {};
    key->abiVersion = BrixGCAllocationABIVersion;
    key->structSize = sizeof(BrixGCAllocationSiteKey);
    key->sizeClassIndex = index;
    key->requestedSize = requestedSize;
    key->allocatedSize = allocSize;
    key->descriptor = descriptor;
    key->pointerKind = static_cast<unsigned char>(pointerKind);
    key->zeroingPolicy = static_cast<unsigned char>(zeroingPolicy);
    return true;
}

static bool
brixRefillCachedGCAllocationBatch(
    BrixGCThreadAllocationBatch& cached,
    size_t requestedSize,
    size_t allocSize,
    void* descriptor,
    size_t generation,
    size_t selectedCount = 0
) {
    if (cached.generation != generation) {
        brixDiscardCachedGCAllocationBatch(cached);
    }

    BrixGCAllocationBatch batch {};
    size_t preferredCount = selectedCount
        ? selectedCount
        : brixGCAllocationBatchPreferredCount(allocSize, descriptor);
    auto reserveBytesForCount = [&](size_t count) {
        return count > std::numeric_limits<size_t>::max() / allocSize
            ? std::numeric_limits<size_t>::max()
            : count * allocSize;
    };
    size_t reserveBytes = reserveBytesForCount(preferredCount);
    if (!brixGCBatchCacheCapAllows(reserveBytes)) {
        gcBatchCachePressureFlushCount.fetch_add(1, std::memory_order_relaxed);
        brixFlushGCAllocationBatches();
        while (preferredCount > 1 &&
                !brixGCBatchCacheCapAllows(reserveBytes)) {
            preferredCount /= 2;
            reserveBytes = reserveBytesForCount(preferredCount);
        }
        if (!brixGCBatchCacheCapAllows(reserveBytes)) {
            cached = BrixGCThreadAllocationBatch {};
            brixGCRecordAllocationBatchMiss(BrixGCAllocationBatchMissCap);
            return false;
        }
    }

    bool allocated = brixCallGCIgnoringPanic([&]() {
        return brixGCActiveTaskCanUseFastAllocation()
            ? brixGCAllocBatchForRunningTask(
                requestedSize, descriptor, preferredCount, &batch)
            : brixGCAllocBatchForUnregisteredThread(
                requestedSize,
                descriptor,
                preferredCount,
                &batch);
    });
    if (!allocated) {
        cached = BrixGCThreadAllocationBatch {};
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
brixGCTraceSignalHandler(int signal, siginfo_t* info, void* context) {
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
        "Brix GC signal trace: signal=%d fault=%p pc=%p image=%p symbol=%s symbolAddr=%p sp=%p fp=%p x10=%#llx x11=%#llx x12=%#llx x19=%#llx x20=%#llx x21=%#llx x22=%#llx frames=%d\n",
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
            "Brix GC signal frame %d: fp=%p ra=%p symbol=%s symbolAddr=%p image=%p w2=%#llx w3=%#llx w4=%#llx w5=%#llx w6=%#llx w7=%#llx\n",
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
brixGCInstallSignalTraceIfRequested() {
    if (!std::getenv("BRIX_GC_TRACE_SIGNALS")) {
        return;
    }

    struct sigaction action;
    std::memset(&action, 0, sizeof(action));
    action.sa_sigaction = brixGCTraceSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
}

#if defined(__APPLE__)
static void
brixGCNoteMemoryPressure(void*) {
    gcMemoryPressurePending.store(true, std::memory_order_release);
}
#endif

static void
brixGCWatchMemoryPressure() {
#if defined(__APPLE__)
    dispatch_source_t source = dispatch_source_create(
        DISPATCH_SOURCE_TYPE_MEMORYPRESSURE,
        0,
        DISPATCH_MEMORYPRESSURE_WARN | DISPATCH_MEMORYPRESSURE_CRITICAL,
        dispatch_get_global_queue(QOS_CLASS_UTILITY, 0));
    if (!source) {
        return;
    }
    dispatch_source_set_event_handler_f(source, brixGCNoteMemoryPressure);
    dispatch_resume(source);
#endif
}

static void
brixEnsureGCInitializedSlow() {
    std::call_once(gcInitOnce, []() {
        brixGCInstallSignalTraceIfRequested();
        size_t expected = 0;
        gcHeapGoal.compare_exchange_strong(
            expected,
            brixGCMinimumHeapGoal(),
            std::memory_order_release,
            std::memory_order_relaxed);
        brixCallGCIgnoringPanic([]() {
            brixGCInit();
        });
        gcInitialized.store(true, std::memory_order_release);
        brixGCWatchMemoryPressure();
    });
}

static void
brixEnsureGCInitialized() {
    if (!gcInitialized.load(std::memory_order_acquire)) {
        brixEnsureGCInitializedSlow();
    }
}

static void
brixRecordGCPublicAllocation(
    BrixGCHotAllocationState& hot,
    size_t requestedSize,
    size_t allocatedSize
) {
    hot.stats.allocatedObjects += 1;
    hot.stats.requestedBytes += requestedSize;
    hot.stats.allocatedBytes += allocatedSize;
    brixGCRecordAllocationPhaseCount(hot, 1);
    if (brixGCLoadPublicPhase() != 0) {
        hot.stats.cycleAllocatedBytes += allocatedSize;
    }
    brixGCBenchRecordAllocationSizes(1, requestedSize);
    brixGCBenchRecordAllocatedSizeClasses(1, allocatedSize);
#if BX_ENABLE_BENCH_METRICS
    if (brixBenchMetricsEnabledFlag) {
        gcBenchAllocationSites[0].fetch_add(1, std::memory_order_relaxed);
    }
#endif
    if (hot.stats.allocatedObjects >= kBrixGCThreadStatFlushObjectCount) {
        // Periodic accounting must not revoke ranges: ownership changes only at collector,
        // cache, or task boundaries. Publish consumed prefixes and retain every unused tail.
        brixPublishGCCompilerAllocationBufferConsumptions();
    }
}

static void
brixRecordGCPublicAllocations(size_t count, size_t requestedSize, size_t allocatedSize) {
    if (!count) {
        return;
    }
    gcHotAllocationState.stats.allocatedObjects += count;
    gcHotAllocationState.stats.requestedBytes += requestedSize * count;
    gcHotAllocationState.stats.allocatedBytes += allocatedSize * count;
    brixGCRecordAllocationPhaseCount(gcHotAllocationState, count);
    if (brixGCLoadPublicPhase() != 0) {
        gcHotAllocationState.stats.cycleAllocatedBytes += allocatedSize * count;
    }
    brixGCBenchRecordAllocationSizes(count, requestedSize);
    brixGCBenchRecordAllocatedSizeClasses(count, allocatedSize);
}

static void
brixRecordGCHeapAllocation(size_t count, size_t allocatedSize) {
    gcLiveObjects.fetch_add(count, std::memory_order_relaxed);
    gcLiveBytes.fetch_add(count * allocatedSize, std::memory_order_relaxed);
}

static void
brixRecordGCAllocation(size_t requestedSize, size_t allocatedSize) {
    brixRecordGCPublicAllocation(gcHotAllocationState, requestedSize, allocatedSize);
    brixRecordGCHeapAllocation(1, allocatedSize);
}

static void
brixGCSafepointIfRequested() {
    uint64_t control = brixGCLoadControlWord();
    if (!brixGCControlWordHandshakeRequested(control) ||
            brixGCActiveTaskAcknowledgedHandshake(control)) {
        return;
    }

    brixPublishGCThreadAllocationStats();
    brixCallIgnoringPanic([]() {
        brixGCSafepoint();
    });
}

static bool
brixGCShouldStartAutomaticCycle(size_t upcomingSize) {
    if (brixGCCollectionDisabled()) {
        return false;
    }
    if (!gcInitialized.load(std::memory_order_acquire)) {
        return false;
    }
    if (brixGCLoadPublicPhase() != kBrixGCPhaseIdle) {
        return false;
    }
    if (gcMemoryPressurePending.load(std::memory_order_acquire)) {
        return true;
    }

    size_t goal = gcHeapGoal.load(std::memory_order_acquire);
    if (goal == 0) {
        goal = brixGCMinimumHeapGoal();
        gcHeapGoal.store(goal, std::memory_order_release);
    }

    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    return brixGCBytesReachThreshold(live, upcomingSize, brixGCAutomaticSoftGoal(goal));
}

static bool
brixGCHardTriggerReached(size_t upcomingSize) {
    if (brixGCCollectionDisabled()) {
        return false;
    }
    if (!gcInitialized.load(std::memory_order_acquire)) {
        return false;
    }

    size_t goal = gcHeapGoal.load(std::memory_order_acquire);
    if (goal == 0) {
        goal = brixGCMinimumHeapGoal();
        gcHeapGoal.store(goal, std::memory_order_release);
    }

    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    return brixGCBytesReachThreshold(live, upcomingSize, brixGCAutomaticHardGoal(goal));
}

static void
brixGCEndCollection() {
    gcCollectionActive.store(false, std::memory_order_release);
}

static bool
brixGCTryBeginCollectionWork() {
    bool expected = false;
    return gcCollectionActive.compare_exchange_strong(
        expected,
        true,
        std::memory_order_acq_rel,
        std::memory_order_acquire
    );
}

static bool
brixGCTryBeginCollection() {
    if (!brixGCTryBeginCollectionWork()) {
        return false;
    }

    if (brixGCLoadPublicPhase() != kBrixGCPhaseIdle) {
        brixGCEndCollection();
        return false;
    }

    return true;
}

static void
brixGCFinishAutomaticGrowthWindowIfIdle() {
    if (brixGCLoadPublicPhase() == kBrixGCPhaseIdle) {
        gcAutomaticCycleActive.store(false, std::memory_order_release);
        gcCollectionMayGrowRequestedGoal.store(false, std::memory_order_release);
    }
}

static void
brixGCRecordAutomaticFallback(size_t reason) {
    size_t triggerReason = reason == BrixGCAutomaticFallbackHardCap
        ? BrixGCPacerTriggerHardCap
        : BrixGCPacerTriggerHardGoal;
    gcPacerLastTriggerReason.store(triggerReason, std::memory_order_release);
    gcPendingCycleTriggerReason.store(triggerReason, std::memory_order_release);
    brixGCRuntimeRecordAutomaticFallback(reason);
}

static void
brixGCRunAutomaticFallbackCollection(size_t reason, bool recordAllocationStall) {
    if (brixGCThreadHasLocalCallbackRegistration()) {
        return;
    }
    brixFlushGCAllocationBatches();
    if (!brixGCTryBeginCollection()) {
        return;
    }

    brixPublishGCThreadAllocationStats();
    gcCollectionMayGrowRequestedGoal.store(true, std::memory_order_release);
    brixGCRecordAutomaticFallback(reason);

    uint64_t start = 0;
    if (recordAllocationStall) {
        start = brixGCMonotonicNanos();
        gcAllocationStallCount.fetch_add(1, std::memory_order_relaxed);
    }

    brixCallGCIgnoringPanic([]() {
        brixGCCollectForAllocation();
        brixGCDrainAutomaticCycleBlocking();
    });

    if (recordAllocationStall) {
        brixGCRecordAllocationStall(start, brixGCMonotonicNanos());
    }
    brixGCFinishAutomaticGrowthWindowIfIdle();
    brixGCEndCollection();
}

static bool
brixGCStartAutomaticCycle(bool recordAllocationStall, size_t triggerReason) {
    if (!brixGCTryBeginCollection()) {
        return false;
    }
    gcMemoryPressurePending.store(false, std::memory_order_release);

    brixPublishGCThreadAllocationStats();
    gcCollectionMayGrowRequestedGoal.store(true, std::memory_order_release);
    gcPacerLastTriggerReason.store(triggerReason, std::memory_order_release);
    gcPendingCycleTriggerReason.store(triggerReason, std::memory_order_release);

    uint64_t start = 0;
    if (recordAllocationStall) {
        start = brixGCMonotonicNanos();
        gcAllocationStallCount.fetch_add(1, std::memory_order_relaxed);
    }

    brixCallGCIgnoringPanic([]() {
        brixGCStartConcurrentMark();
    });

    if (recordAllocationStall) {
        brixGCRecordAllocationStall(start, brixGCMonotonicNanos());
    }

    bool active = brixGCLoadPublicPhase() != kBrixGCPhaseIdle;
    if (!active) {
        gcPendingCycleTriggerReason.store(0, std::memory_order_release);
    }
    gcAutomaticCycleActive.store(active, std::memory_order_release);
    brixGCFinishAutomaticGrowthWindowIfIdle();
    brixGCEndCollection();
    if (active) {
        brixGCStartBackgroundWorkers();
    }
    return active;
}

static bool
brixGCCompleteAutomaticCycle(size_t fallbackReason) {
    if (!brixGCTryBeginCollectionWork()) {
        return false;
    }

    if (fallbackReason != BrixGCAutomaticFallbackNone) {
        brixGCRecordAutomaticFallback(fallbackReason);
    }

    brixCallGCIgnoringPanic([&]() {
        if (fallbackReason == BrixGCAutomaticFallbackNoProgressDriver) {
            brixGCDrainAutomaticCycleBlocking();
        } else {
            brixGCFinishAutomaticCycle();
        }
    });

    brixGCFinishAutomaticGrowthWindowIfIdle();
    brixGCEndCollection();
    return brixGCLoadPublicPhase() == kBrixGCPhaseIdle;
}

static bool
brixGCAutomaticCycleExpired() {
    size_t maxAge = brixGCAutomaticMaxActiveCycleNanos();
    return maxAge != 0 && brixGCRuntimeActiveCycleAgeNanos() > maxAge;
}

static size_t
brixGCDrainAutomaticCycleBlocking() {
    if (brixGCThreadHasLocalCallbackRegistration()) {
        return 0;
    }
    while (brixGCLoadPublicPhase() != kBrixGCPhaseIdle) {
        unsigned char phase = brixGCLoadPublicPhase();
        if (phase == kBrixGCPhaseSweep) {
            brixGCStepIncrementalSweep(brixGCAutomaticHardAssistWork());
        } else {
            brixGCFinishAutomaticCycle();
        }
    }
    return 0;
}

static bool
brixGCBudgetHasTime(uint64_t deadlineNanos) {
    uint64_t now = brixGCMonotonicNanos();
    return now < deadlineNanos && deadlineNanos - now > kBrixGCBudgetGuardNanos;
}

enum BrixGCIdleSettle {
    BrixGCIdleSettleNone,
    BrixGCIdleSettleWaiting,
    BrixGCIdleSettleDue
};

static BrixGCIdleSettle
brixGCIdleSettleState(uint64_t nowNanos) {
    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    size_t settled = gcIdleSettledLiveBytes.load(std::memory_order_acquire);
    size_t slack = std::max(settled / 4, kBrixGCIdleSettleMinimumBytes);
    if (live <= settled || live - settled < slack) {
        return BrixGCIdleSettleNone;
    }
    if (gcIdleSettleSeenLiveBytes.exchange(live, std::memory_order_acq_rel) != live) {
        gcIdleSettleSeenNanos.store(nowNanos, std::memory_order_release);
        return BrixGCIdleSettleWaiting;
    }
    uint64_t seen = gcIdleSettleSeenNanos.load(std::memory_order_acquire);
    return nowNanos - seen < kBrixGCIdleSettleQuietNanos
        ? BrixGCIdleSettleWaiting
        : BrixGCIdleSettleDue;
}

static bool
brixGCIdleScavengeIfNeeded(uint64_t deadlineNanos, bool trimsMetadata, bool* pending = nullptr) {
    if (!gcInitialized.load(std::memory_order_acquire) ||
            brixGCLoadPublicPhase() != kBrixGCPhaseIdle ||
            !brixGCBudgetHasTime(deadlineNanos)) {
        return false;
    }
    bool wantsTrim = trimsMetadata && gcMetadataTrimPending.load(std::memory_order_acquire);
    if (brixGCScavengeDebt() == 0 && !wantsTrim) {
        return false;
    }
    if (!brixGCTryBeginCollectionWork()) {
        return false;
    }

    brixFlushGCAllocationBatches();
    bool remains = false;
    brixCallGCIgnoringPanic([&]() {
        size_t debt = brixGCScavengeDebt();
        if (!debt) {
            if (trimsMetadata && gcMetadataTrimPending.exchange(false, std::memory_order_acq_rel)) {
                brixGCTrimMetadataCaches();
            }
            return;
        }
        uint64_t startNanos = brixGCMonotonicNanos();
        uint64_t remainingNanos = deadlineNanos > startNanos ? deadlineNanos - startNanos : 0;
        size_t minimum = brixGCScavengeMinimumStepPages();
        size_t pageLimit = remainingNanos <= kBrixGCBudgetGuardNanos * 4
            ? minimum
            : brixGCIdleScavengePageLimit(
                remainingNanos - kBrixGCBudgetGuardNanos,
                debt,
                minimum);
        size_t releasedBefore = gcScavengedPageTotal.load(std::memory_order_relaxed);
        brixGCStepScavenge(pageLimit);
        size_t released = gcScavengedPageTotal.load(std::memory_order_relaxed) - releasedBefore;
        if (released) {
            brixGCRecordIdleScavengeCost(brixGCMonotonicNanos() - startNanos, released);
            remains = brixGCScavengeDebt() != 0;
        }
        if (!remains && trimsMetadata &&
                gcMetadataTrimPending.exchange(false, std::memory_order_acq_rel)) {
            brixGCTrimMetadataCaches();
        }
    });
    brixGCEndCollection();
    if (pending) {
        *pending = remains;
    }
    return true;
}

static size_t
brixGCProgressAutomaticCycle(size_t workLimit, bool assist, bool bounded = false) {
    unsigned char phase = brixGCLoadPublicPhase();
    if (phase == kBrixGCPhaseIdle) {
        return 0;
    }

    if (phase == kBrixGCPhaseMarkComplete) {
        brixGCCompleteAutomaticCycle(BrixGCAutomaticFallbackNone);
        return 0;
    }

    if (phase == kBrixGCPhaseSweep) {
        if (workLimit == 0) {
            return 0;
        }
        if (!brixGCTryBeginCollectionWork()) {
            return 0;
        }
        brixCallGCIgnoringPanic([&]() {
            size_t sweepWorkLimit =
                (!assist && !bounded && !brixGCIncrementalSweepHasDebt())
                ? std::numeric_limits<size_t>::max()
                : workLimit;
            brixGCStepIncrementalSweep(sweepWorkLimit);
        });
        brixGCFinishAutomaticGrowthWindowIfIdle();
        brixGCEndCollection();
        return 0;
    }

    if (brixGCAutomaticCycleExpired()) {
        brixGCCompleteAutomaticCycle(BrixGCAutomaticFallbackMaxActiveAge);
        return 0;
    }

    if (phase != kBrixGCPhaseConcurrentMark || workLimit == 0) {
        return 0;
    }

    if (!brixGCTryBeginCollectionWork()) {
        return 0;
    }

    size_t drained = 0;
    brixCallGCIgnoringPanic([&]() {
        if (assist) {
            drained = brixGCMutatorAssist(workLimit);
        } else {
            size_t remaining = workLimit;
            do {
                drained += brixGCStepConcurrentMark();
                if (remaining <= kBrixGCAutomaticStepWork) {
                    break;
                }
                remaining -= kBrixGCAutomaticStepWork;
            } while (
                brixGCLoadPublicPhase() == kBrixGCPhaseConcurrentMark &&
                brixGCRuntimeMarkQueueDepth() != 0);
        }
    });

    bool leavesTermination =
        assist && gcBackgroundWorkerActiveCount.load(std::memory_order_acquire) != 0;
    bool shouldFinish = !leavesTermination && (
        brixGCLoadPublicPhase() == kBrixGCPhaseMarkComplete ||
        (
            brixGCLoadPublicPhase() == kBrixGCPhaseConcurrentMark &&
            brixGCRuntimeMarkQueueDepth() <= brixGCAutomaticTerminationQueueBudget()
        ));
    if (shouldFinish) {
        brixCallGCIgnoringPanic([]() {
            brixGCFinishAutomaticCycle();
        });
        brixGCFinishAutomaticGrowthWindowIfIdle();
    }

    brixGCEndCollection();
    return drained;
}

static bool
brixGCProgressWithinDeadline(uint64_t deadlineNanos, size_t preferredWork) {
    bool attempted = false;
    while (brixGCLoadPublicPhase() != kBrixGCPhaseIdle &&
            brixGCBudgetHasTime(deadlineNanos)) {
        if (gcCollectionActive.load(std::memory_order_acquire)) {
            break;
        }
        uint64_t remainingNanos = deadlineNanos - brixGCMonotonicNanos();
        size_t workLimit = remainingNanos <= kBrixGCBudgetGuardNanos * 4
            ? 1
            : preferredWork;
        brixGCProgressAutomaticCycle(workLimit, false, true);
        attempted = true;
    }
    return attempted;
}

static size_t
brixGCGetActiveTaskAssistDebt() {
    size_t cycle = gcCurrentCycleId.load(std::memory_order_acquire);
    if (brixGCActiveTaskAssistDebt && brixGCActiveTaskAssistDebtCycle) {
        if (brixGCActiveTaskAssistDebtCycle() != cycle) {
            brixGCSetActiveTaskAssistDebt(0);
            brixGCSetActiveTaskAssistDebtCycle(cycle);
            return 0;
        }
        return brixGCActiveTaskAssistDebt();
    }
    if (gcFallbackActiveTaskAssistDebtCycle != cycle) {
        gcFallbackActiveTaskAssistDebt = 0;
        gcFallbackActiveTaskAssistDebtCycle = cycle;
    }
    return gcFallbackActiveTaskAssistDebt;
}

static void
brixGCSetTaskAssistDebt(size_t debt) {
    if (brixGCSetActiveTaskAssistDebt) {
        brixGCSetActiveTaskAssistDebt(debt);
        if (brixGCSetActiveTaskAssistDebtCycle) {
            brixGCSetActiveTaskAssistDebtCycle(
                gcCurrentCycleId.load(std::memory_order_acquire));
        }
    } else {
        gcFallbackActiveTaskAssistDebt = debt;
        gcFallbackActiveTaskAssistDebtCycle =
            gcCurrentCycleId.load(std::memory_order_acquire);
    }
}

static size_t
brixGCTakeBackgroundCredit(size_t requested) {
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
brixGCAccrueAssistDebt(size_t allocatedBytes) {
    size_t queueDepth = brixGCRuntimeMarkQueueDepth();
    if (!queueDepth || !allocatedBytes) {
        return brixGCGetActiveTaskAssistDebt();
    }
    size_t heapBytes = gcLiveBytes.load(std::memory_order_relaxed);
    size_t heapGoal = gcHeapGoal.load(std::memory_order_acquire);
    size_t allocationRoom = heapGoal > heapBytes ? heapGoal - heapBytes : 1;
    size_t accrued = brixGCMultiplyDivideSaturating(
        allocatedBytes,
        queueDepth,
        allocationRoom);
    if (!accrued) {
        accrued = 1;
    }
    size_t debt = brixGCGetActiveTaskAssistDebt();
    size_t next = debt > std::numeric_limits<size_t>::max() - accrued
        ? std::numeric_limits<size_t>::max()
        : debt + accrued;
    gcAssistDebt.fetch_add(next - debt, std::memory_order_relaxed);

    size_t credited = brixGCTakeBackgroundCredit(next);
    next -= credited;
    brixGCSubtractAtomicSaturating(gcAssistDebt, credited);
    brixGCSetTaskAssistDebt(next);
    return next;
}

static void
brixGCBackgroundWorkerSleep(size_t delayNanos) {
    if (!delayNanos) {
        return;
    }
    struct timespec delay = {
        static_cast<time_t>(delayNanos / 1000000000ull),
        static_cast<long>(delayNanos % 1000000000ull)
    };
    brixGCEnterRuntimeBlock();
    nanosleep(&delay, nullptr);
    brixGCLeaveRuntimeBlock();
}

static void
brixGCBackgroundMarkWorker(void* closure) {
    size_t token = reinterpret_cast<size_t>(closure);
    size_t generation = token >> 8;
    size_t workerIndex = (token & 0xff) - 1;
    gcBackgroundWorkerActiveCount.fetch_add(1, std::memory_order_relaxed);

    while (
        generation == gcBackgroundWorkerGeneration.load(std::memory_order_acquire) &&
        gcAutomaticCycleActive.load(std::memory_order_acquire)
    ) {
        brixGCSafepointIfRequested();
        unsigned char phase = brixGCLoadPublicPhase();
        if (phase == kBrixGCPhaseIdle) {
            break;
        }
        if (phase != kBrixGCPhaseConcurrentMark) {
            if (workerIndex == 0) {
                brixGCProgressAutomaticCycle(kBrixGCBackgroundWorkerStepWork, false);
            } else {
                break;
            }
            continue;
        }

        uint64_t start = brixGCMonotonicNanos();
        size_t drained = 0;
        brixCallGCIgnoringPanic([&]() {
            drained = brixGCStepConcurrentMark();
        });
        size_t elapsed = static_cast<size_t>(brixGCMonotonicNanos() - start);
        gcBackgroundWorkerCpuNanos.fetch_add(elapsed, std::memory_order_relaxed);
        if (drained) {
            gcBackgroundWorkerWork.fetch_add(drained, std::memory_order_relaxed);
            gcBackgroundCredit.fetch_add(drained, std::memory_order_relaxed);
        }

        if (
            workerIndex == 0 &&
            (brixGCLoadPublicPhase() == kBrixGCPhaseMarkComplete ||
                brixGCRuntimeMarkQueueDepth() <= brixGCAutomaticTerminationQueueBudget())
        ) {
            brixGCProgressAutomaticCycle(1, false);
        }

        size_t duty = gcBackgroundWorkerDutyPermille.load(std::memory_order_relaxed);
        size_t delayNanos = duty && duty < kBrixGCBackgroundDutyDenominator
            ? brixGCMultiplyDivideSaturating(
                elapsed,
                kBrixGCBackgroundDutyDenominator - duty,
                duty)
            : 0;
        if (!drained && !delayNanos) {
            delayNanos = kBrixGCMaxAssistSliceNanos;
        }
        brixGCBackgroundWorkerSleep(delayNanos);
    }

    gcBackgroundWorkerActiveCount.fetch_sub(1, std::memory_order_relaxed);
    // BxTask entrypoints are stackful scheduler contexts, not ordinary pthread routines.
    // They must hand control back through the task trampoline instead of returning.
    brixTerminateActiveTask();
}

static void
brixGCStartBackgroundWorkers() {
    if (brixGCReplayModeEnabled() ||
            brixGCEnvFlagEnabled("BRIX_GC_DISABLE_AUTOMATIC_PROGRESS")) {
        return;
    }
    if (!brixGCBackgroundTaskQueue || !brixGCBackgroundTaskQueueThreadCount ||
            !brixForkGCMarkTaskInWorker) {
        return;
    }
    int schedulerThreads = brixGCBackgroundTaskQueueThreadCount();
    BxTaskQueue* queue = brixGCBackgroundTaskQueue();
    if (schedulerThreads <= 0 || !queue) {
        return;
    }
    size_t workerCount = std::max<size_t>(1, static_cast<size_t>(schedulerThreads) / 4);
    size_t duty = std::min<size_t>(
        kBrixGCBackgroundDutyDenominator,
        (static_cast<size_t>(schedulerThreads) * 250) / workerCount);

    gcBackgroundWorkerCount.store(workerCount, std::memory_order_release);
    gcBackgroundWorkerThreadCapacity.store(
        static_cast<size_t>(schedulerThreads),
        std::memory_order_release);
    gcBackgroundWorkerDutyPermille.store(duty, std::memory_order_release);
    size_t generation = gcBackgroundWorkerGeneration.fetch_add(1, std::memory_order_acq_rel) + 1;
    for (size_t index = 0; index < workerCount; ++index) {
        size_t token = (generation << 8) | (index + 1);
        brixForkGCMarkTaskInWorker(
            queue,
            brixGCBackgroundMarkWorker,
            reinterpret_cast<void*>(token));
    }
}

static void
brixGCAllocationAssistIfNeeded(size_t upcomingSize) {
    unsigned char phase = brixGCLoadPublicPhase();
    if (phase == kBrixGCPhaseIdle) {
        return;
    }
    if (!gcAutomaticCycleActive.load(std::memory_order_acquire)) {
        return;
    }

    if (brixGCHardCapReached(upcomingSize)) {
        brixGCCompleteAutomaticCycle(BrixGCAutomaticFallbackHardCap);
        return;
    }

    if (phase != kBrixGCPhaseConcurrentMark) {
        return;
    }

    gcHotAllocationState.stats.allocationAssistCheckCount += 1;
    bool hardTrigger = brixGCHardTriggerReached(upcomingSize);
    size_t debt = brixGCAccrueAssistDebt(upcomingSize);
    if (!debt) {
        return;
    }

    if (hardTrigger && brixGCAllocationAssistDisabled()) {
        brixGCCompleteAutomaticCycle(BrixGCAutomaticFallbackNoProgressDriver);
        return;
    }

    if (brixGCAllocationAssistDisabled()) {
        return;
    }

    size_t workLimit = hardTrigger
        ? brixGCAutomaticHardAssistWork()
        : brixGCAutomaticAssistWork();
    workLimit = std::min(workLimit, debt);
    gcHotAllocationState.stats.allocationAssistCallCount += 1;
    size_t drained = brixGCProgressAutomaticCycle(workLimit, true);
    size_t remaining = debt > drained ? debt - drained : 0;
    brixGCSubtractAtomicSaturating(gcAssistDebt, debt - remaining);
    brixGCSetTaskAssistDebt(remaining);
}

struct BrixGCVerificationDriverState {
    size_t allocationsSinceCycle = 0;
    size_t bytesSinceCycle = 0;
    size_t replayAllocations = 0;
    uint64_t replayState = 0;
};

static BrixGCVerificationDriverState&
brixGCVerificationDriverState() {
    static thread_local BrixGCVerificationDriverState state;
    return state;
}

static bool
brixGCRunForcedFullCycle() {
    if (brixGCThreadHasLocalCallbackRegistration()) {
        return false;
    }
    brixFlushGCAllocationBatches();
    if (!brixGCTryBeginCollectionWork()) {
        return false;
    }

    brixPublishGCThreadAllocationStats();
    brixCallGCIgnoringPanic([]() {
        if (brixGCLoadPublicPhase() == kBrixGCPhaseIdle) {
            brixGCCollectForAllocation();
        } else {
            brixGCDrainAutomaticCycleBlocking();
        }
    });
    brixGCFinishAutomaticGrowthWindowIfIdle();
    bool complete = brixGCLoadPublicPhase() == kBrixGCPhaseIdle;
    brixGCEndCollection();
    return complete;
}

static void
brixGCStressBeforeAllocation(size_t upcomingSize) {
    BrixGCVerificationDriverState& state = brixGCVerificationDriverState();
    if (state.allocationsSinceCycle != std::numeric_limits<size_t>::max()) {
        ++state.allocationsSinceCycle;
    }
    state.bytesSinceCycle = upcomingSize > std::numeric_limits<size_t>::max() - state.bytesSinceCycle
        ? std::numeric_limits<size_t>::max()
        : state.bytesSinceCycle + upcomingSize;

    size_t allocationInterval = brixGCStressAllocationInterval();
    size_t byteInterval = brixGCStressByteInterval();
    bool allocationDue = allocationInterval != 0 &&
        state.allocationsSinceCycle >= allocationInterval;
    bool bytesDue = byteInterval != 0 && state.bytesSinceCycle >= byteInterval;
    if ((!allocationDue && !bytesDue) || !brixGCRunForcedFullCycle()) {
        return;
    }

    state.allocationsSinceCycle = allocationInterval
        ? state.allocationsSinceCycle % allocationInterval
        : 0;
    state.bytesSinceCycle = byteInterval ? state.bytesSinceCycle % byteInterval : 0;
}

static uint64_t
brixGCReplayNext(BrixGCVerificationDriverState& state) {
    if (state.replayState == 0) {
        state.replayState = brixGCReplaySeed();
    }
    state.replayState ^= state.replayState >> 12;
    state.replayState ^= state.replayState << 25;
    state.replayState ^= state.replayState >> 27;
    return state.replayState * 0x2545f4914f6cdd1dull;
}

static void
brixGCReplayBeforeAllocation() {
    BrixGCVerificationDriverState& state = brixGCVerificationDriverState();
    ++state.replayAllocations;
    uint64_t choice = brixGCReplayNext(state);
    unsigned char phase = brixGCLoadPublicPhase();

    if (phase == kBrixGCPhaseIdle) {
        size_t interval = brixGCReplayAllocationInterval();
        if (interval != 0 && state.replayAllocations % interval == 0) {
            brixFlushGCAllocationBatches();
            brixGCStartAutomaticCycle(false, BrixGCPacerTriggerIdle);
        }
        return;
    }

    size_t workLimit = 1 + static_cast<size_t>(choice % kBrixGCAutomaticStepWork);
    brixGCProgressAutomaticCycle(workLimit, false);
}

static void
brixGCCollectForAllocationIfNeeded(size_t upcomingSize) {
    if (brixGCCollectionDisabled() || brixGCThreadHasLocalCallbackRegistration()) {
        return;
    }
    if (brixGCStressModeEnabled()) {
        brixGCStressBeforeAllocation(upcomingSize);
        return;
    }
    if (brixGCReplayModeEnabled()) {
        brixGCReplayBeforeAllocation();
        return;
    }

    brixGCAllocationAssistIfNeeded(upcomingSize);

    if (brixGCLoadPublicPhase() != kBrixGCPhaseIdle) {
        return;
    }

    if (brixGCHardCapReached(upcomingSize)) {
        brixGCRunAutomaticFallbackCollection(BrixGCAutomaticFallbackHardCap, true);
        return;
    }

    if (!brixGCShouldStartAutomaticCycle(upcomingSize)) {
        return;
    }

    brixFlushGCAllocationBatches();
    if (brixGCHardCapReached(upcomingSize)) {
        brixGCRunAutomaticFallbackCollection(BrixGCAutomaticFallbackHardCap, true);
        return;
    }
    if (!brixGCShouldStartAutomaticCycle(upcomingSize)) {
        return;
    }

    if (brixGCAllocationAssistDisabled()) {
        if (brixGCHardTriggerReached(upcomingSize)) {
            brixGCRunAutomaticFallbackCollection(
                BrixGCAutomaticFallbackNoProgressDriver,
                true);
        }
        return;
    }

    size_t triggerReason = brixGCHardTriggerReached(upcomingSize)
        ? BrixGCPacerTriggerHardGoal
        : BrixGCPacerTriggerSoftGoal;
    if (brixGCStartAutomaticCycle(true, triggerReason)) {
        brixGCAllocationAssistIfNeeded(upcomingSize);
    }
}

extern "C" bool
brixGCRuntimeIdleCollect(uint64_t budgetNanos) {
    if (brixGCCollectionDisabled() || !budgetNanos) {
        return false;
    }
    uint64_t startNanos = brixGCMonotonicNanos();
    uint64_t deadlineNanos = budgetNanos > std::numeric_limits<uint64_t>::max() - startNanos
        ? std::numeric_limits<uint64_t>::max()
        : startNanos + budgetNanos;
    if (brixGCIdleProgressDisabled()) {
        return false;
    }
    if (!brixGCBudgetHasTime(deadlineNanos)) {
        return false;
    }

    unsigned char phase = brixGCLoadPublicPhase();
    if (phase != kBrixGCPhaseIdle) {
        if (!gcAutomaticCycleActive.load(std::memory_order_acquire)) {
            return false;
        }
        brixGCProgressWithinDeadline(deadlineNanos, brixGCAutomaticIdleWork());
        return gcAutomaticCycleActive.load(std::memory_order_acquire);
    }

    bool pending = false;
    BrixGCIdleSettle settle = BrixGCIdleSettleNone;
    if (!brixGCShouldStartAutomaticCycle(0)) {
        settle = brixGCIdleSettleState(startNanos);
        if (settle != BrixGCIdleSettleDue) {
            brixGCIdleScavengeIfNeeded(deadlineNanos, true, &pending);
            return pending || settle == BrixGCIdleSettleWaiting;
        }
    }

    brixFlushGCAllocationBatches();
    if (!brixGCBudgetHasTime(deadlineNanos)) {
        return false;
    }
    if (!brixGCShouldStartAutomaticCycle(0) && settle != BrixGCIdleSettleDue) {
        brixGCIdleScavengeIfNeeded(deadlineNanos, true, &pending);
        return pending;
    }

    if (brixGCStartAutomaticCycle(false, BrixGCPacerTriggerIdle)) {
        brixGCProgressWithinDeadline(deadlineNanos, brixGCAutomaticIdleWork());
        return gcAutomaticCycleActive.load(std::memory_order_acquire);
    }
    brixGCIdleScavengeIfNeeded(deadlineNanos, true, &pending);
    return pending;
}

extern "C" void
brixGCFrameBudget(uint64_t remainingNanos) {
    if (brixGCCollectionDisabled() || !remainingNanos) {
        return;
    }
    brixEnsureGCInitialized();
    gcFrameBudgetOfferCount.fetch_add(1, std::memory_order_relaxed);
    uint64_t startNanos = brixGCMonotonicNanos();
    uint64_t deadline = remainingNanos > std::numeric_limits<uint64_t>::max() - startNanos
        ? std::numeric_limits<uint64_t>::max()
        : startNanos + remainingNanos;
    if (!brixGCBudgetHasTime(deadline)) {
        return;
    }

    bool worked = false;
    if (brixGCLoadPublicPhase() == kBrixGCPhaseIdle && brixGCShouldStartAutomaticCycle(0)) {
        brixFlushGCAllocationBatches();
        if (brixGCBudgetHasTime(deadline) && brixGCShouldStartAutomaticCycle(0)) {
            worked = brixGCStartAutomaticCycle(false, BrixGCPacerTriggerFrame);
        }
    }
    if (brixGCLoadPublicPhase() != kBrixGCPhaseIdle) {
        worked = brixGCProgressWithinDeadline(deadline, kBrixGCAutomaticStepWork) || worked;
    } else {
        worked = brixGCIdleScavengeIfNeeded(deadline, false) || worked;
    }

    if (!worked) {
        return;
    }
    uint64_t endNanos = brixGCMonotonicNanos();
    size_t elapsedNanos = static_cast<size_t>(endNanos - startNanos);
    gcFrameBudgetWorkCount.fetch_add(1, std::memory_order_relaxed);
    gcFrameBudgetWorkNanos.fetch_add(elapsedNanos, std::memory_order_relaxed);
    if (endNanos > deadline) {
        gcFrameBudgetOverrunCount.fetch_add(1, std::memory_order_relaxed);
        brixGCMaxAtomic(
            gcFrameBudgetMaxOverrunNanos,
            static_cast<size_t>(endNanos - deadline));
    }
}

static char*
brixTryFastGCAllocationInThreadRange(
    BrixGCHotAllocationState& hot,
    size_t size,
    size_t allocSize,
    size_t index,
    void* descriptor,
    bool siteAllocation
) {
    // Scheduler handshakes flush and stop Running tasks before phase/generation changes.
    // Callers that cannot participate use the collector-admitted scalar path.
    if (!brixGCActiveTaskCanUseFastAllocation()) {
        brixGCDebugUnstoppableFastAllocationRejected();
        if (siteAllocation) {
            hot.stats.allocationSiteFallbackCount += 1;
        }
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    if (brixGCVerificationSlowPathsEnabled()) {
        if (siteAllocation) {
            hot.stats.allocationSiteFallbackCount += 1;
        }
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }

    uint64_t control = brixGCLoadControlWord();
    if (brixGCControlWordHandshakeRequested(control)) {
        if (siteAllocation) {
            hot.stats.allocationSiteFallbackCount += 1;
        }
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }

    size_t generation = brixGCControlWordSweepGeneration(control);
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    BrixGCRuntimeAllocationBuffer& range = cache->ranges[index];
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
        size_t matchingWay = BrixGCThreadCacheRangeWayCount;
        if (descriptor) {
            brixGCInvalidateStaleThreadRangeWays(cache, index, generation);
            matchingWay = brixGCFindThreadRangeWay(
                cache,
                index,
                size,
                allocSize,
                descriptor,
                generation);
        } else {
            matchingWay = brixGCFindDescriptorlessThreadRangeWay(
                cache,
                index,
                allocSize,
                generation);
        }
        if (matchingWay != 0 && matchingWay < BrixGCThreadCacheRangeWayCount) {
            BrixGCRuntimeAllocationBuffer& parked =
                brixGCThreadRangeWay(cache, index, matchingWay);
            std::swap(range, parked);
            if (!brixGCAllocationRangeIsParkable(parked) ||
                    parked.generation != generation) {
                brixGCClearStagedGCAllocationRange(parked);
            }
            matches =
                range.generation == generation &&
                range.slotSize >= allocSize &&
                range.descriptor == descriptor &&
                (!descriptor || range.requestedSize == size) &&
                brixGCAllocationRangeHasUnconsumedSlots(range);
        }
    }
    brixGCRecordAllocationDescriptorMix(hot, index, descriptor);
    bool refilled = false;

    if (!matches) {
        size_t miss = BrixGCAllocationBatchMissEmpty;
        if (range.buffer.cursor && range.buffer.cursor < range.buffer.limit) {
            if (range.generation != generation) {
                miss = BrixGCAllocationBatchMissGeneration;
            } else if (range.descriptor != descriptor) {
                miss = BrixGCAllocationBatchMissDescriptor;
            } else {
                miss = BrixGCAllocationBatchMissRequestedSize;
            }
        }
        brixGCRecordAllocationBatchMiss(miss);
        if (siteAllocation) {
            hot.stats.allocationSiteMissCount += 1;
        }
        bool samePolicySite =
            range.descriptor == descriptor &&
            range.requestedSize == size &&
            (!range.slotSize || range.slotSize >= allocSize) &&
            (!range.generation || range.generation == generation);
        if (!samePolicySite && (descriptor || range.descriptor) &&
                brixGCAllocationRangeIsParkable(range)) {
            brixGCParkGCAllocationRange(cache, index, range, 1);
        } else {
            if (!samePolicySite) {
                brixGCResetAdaptiveAllocationBufferPolicy(range);
            }
            brixFlushGCCompilerAllocationBuffer(range);
        }

        BrixGCThreadAllocationBatch batch {};
        size_t selectedCount = brixGCAdaptiveAllocationBatchCount(
            range,
            allocSize,
            descriptor);
        if (!brixRefillCachedGCAllocationBatch(
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
            ? kBrixGCAllocationPointerful
            : kBrixGCAllocationPointerFree;
        range.zeroingPolicy = kBrixGCAllocationZeroFull;
        range.reserved[kBrixGCAdaptiveEnabledByte] = descriptor ? 1 : 0;
        range.reserved[kBrixGCAdaptiveCountByte] = static_cast<unsigned char>(
            std::min(selectedCount, kBrixGCAdaptiveAllocationBatchMaxCount));
        range.reserved[kBrixGCAdaptiveReservedSlotCountByte] =
            static_cast<unsigned char>(std::min(
                static_cast<size_t>((batch.limit - batch.cursor) / batch.slotSize),
                kBrixGCAdaptiveAllocationBatchMaxCount));
        range.abiVersion = BrixGCAllocationABIVersion;
        range.structSize = sizeof(BrixGCRuntimeAllocationBuffer);
        refilled = true;
    }

    uint64_t consumeControl = brixGCLoadControlWord();
    if (brixGCControlWordHandshakeRequested(consumeControl) ||
            range.generation != brixGCControlWordSweepGeneration(consumeControl)) {
        brixFlushGCCompilerAllocationBuffer(range);
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
    brixRecordGCPublicAllocation(hot, size, range.slotSize);

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
brixTryFastGCAllocation(BrixGCHotAllocationState& hot, size_t size, void* descriptor) {
    size_t allocSize = brixGCObjectAllocationSize(size);
    if (allocSize > kBrixGCAllocationBatchMaxSize) {
        hot.stats.allocationLargeCount += 1;
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    size_t index = brixGCAllocationBatchIndex(allocSize);
    if (index >= kBrixGCAllocationBatchCount) {
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    return brixTryFastGCAllocationInThreadRange(
        hot, size, allocSize, index, descriptor, false);
}

static char*
brixTryFastGCAllocationWithSiteKey(
    BrixGCHotAllocationState& hot,
    const BrixGCAllocationSiteKey& key
) {
    if (key.allocatedSize > kBrixGCAllocationBatchMaxSize ||
            key.sizeClassIndex >= kBrixGCAllocationBatchCount) {
        hot.stats.allocationLargeCount += 1;
        hot.stats.allocationSiteFallbackCount += 1;
        hot.stats.allocationFastFallbackCount += 1;
        return nullptr;
    }
    return brixTryFastGCAllocationInThreadRange(
        hot,
        key.requestedSize,
        key.allocatedSize,
        key.sizeClassIndex,
        key.descriptor,
        true);
}

extern "C" void
brixGCPublishPhase(size_t phase) {
    brixFlushGCCompilerAllocationBuffer();
    brixGCUpdateControlWord(BrixGCControlPhaseMask, phase);
}

extern "C" size_t
brixGCPublicPhase() {
    return (size_t)brixGCLoadPublicPhase();
}

extern "C" void
brixGCPublishSweepGeneration(size_t generation) {
    brixFlushGCCompilerAllocationBuffer();
    const uint64_t generationMask = ~((1ull << BrixGCControlSweepGenerationShift) - 1ull);
    brixGCUpdateControlWord(
        generationMask,
        (uint64_t)generation << BrixGCControlSweepGenerationShift);
    brixGCResetAdaptiveAllocationBufferPolicies(brixGCEnsureActiveThreadCache());
}

extern "C" size_t
brixGCPublicSweepGeneration() {
    return brixGCLoadPublicSweepGeneration();
}

extern "C" void
brixGCPublishHandshakeRequest(size_t reason, size_t epoch, bool requested) {
    const uint64_t mask = BrixGCControlHandshakeRequestedBit |
        BrixGCControlHandshakeReasonMask |
        BrixGCControlHandshakeEpochMask;
    uint64_t value = requested
        ? BrixGCControlHandshakeRequestedBit |
            (((uint64_t)reason << BrixGCControlHandshakeReasonShift) &
                BrixGCControlHandshakeReasonMask) |
            (((uint64_t)epoch << BrixGCControlHandshakeEpochShift) &
                BrixGCControlHandshakeEpochMask)
        : 0;
    brixGCUpdateControlWord(
        mask,
        value);
}

extern "C" uint64_t
brixGCPublicControl() {
    return brixGCLoadControlWord();
}

extern "C" void
brixGCRuntimeCycleStart(size_t cycle) {
    brixPublishGCThreadAllocationStats();
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
    gcActiveCycleStartNanos.store(brixGCMonotonicNanos(), std::memory_order_release);
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
brixGCRuntimeCycleEnd() {
    brixPublishGCThreadAllocationStats();
    size_t markWork = gcPacerCurrentMarkWorkItems.load(std::memory_order_acquire);
    size_t assistWork = gcPacerCurrentAssistWorkItems.load(std::memory_order_acquire);
    BrixGCCycleSummary summary {
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
        size_t summaryIndex = gcCycleSummarySequence % kBrixGCCycleSummaryCapacity;
        gcCycleSummaries[summaryIndex] = summary;
        ++gcCycleSummarySequence;
        if (gcCycleSummaryStored < kBrixGCCycleSummaryCapacity) {
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
brixGCRuntimeSubphaseStart(size_t subphase, size_t cycle, size_t) {
    if (!brixGCValidSubphase(subphase)) {
        return;
    }
    gcSubphaseStartNanos[subphase] = brixGCMonotonicNanos();
    gcSubphaseStartCycles[subphase] = cycle;
    brixGCBenchCpuStart();
}

extern "C" void
brixGCRuntimeSubphaseEnd(size_t subphase, size_t value) {
    if (!brixGCValidSubphase(subphase) || !gcSubphaseStartNanos[subphase]) {
        return;
    }
    uint64_t start = gcSubphaseStartNanos[subphase];
    uint64_t end = brixGCMonotonicNanos();
    brixGCRecordSubphaseSample(
        subphase,
        gcSubphaseStartCycles[subphase],
        start,
        static_cast<size_t>(end - start),
        value);
    gcSubphaseStartNanos[subphase] = 0;
    gcSubphaseStartCycles[subphase] = 0;
    brixGCBenchCpuEnd();
}

extern "C" void
brixGCRuntimeSubphaseCancel(size_t subphase) {
    if (!brixGCValidSubphase(subphase)) {
        return;
    }
    bool hadStart = gcSubphaseStartNanos[subphase] != 0;
    gcSubphaseStartNanos[subphase] = 0;
    gcSubphaseStartCycles[subphase] = 0;
    if (hadStart) {
        brixGCBenchCpuEnd();
    }
}

extern "C" size_t
brixGCRuntimeBenchGCCpuNanos() {
#if BX_ENABLE_BENCH_METRICS
    return gcBenchCpuNanos.load(std::memory_order_relaxed);
#else
    return 0;
#endif
}

extern "C" size_t
brixGCRuntimeBenchAllocationSiteCount() {
    return BrixGCCompilerAllocationBufferCount + 1;
}

extern "C" size_t
brixGCRuntimeBenchAllocationSiteValue(size_t index) {
#if BX_ENABLE_BENCH_METRICS
    return index < gcBenchAllocationSites.size()
        ? gcBenchAllocationSites[index].load(std::memory_order_relaxed)
        : 0;
#else
    (void)index;
    return 0;
#endif
}

extern "C" size_t
brixGCRuntimeBenchThreadRefillCount() {
#if BX_ENABLE_BENCH_METRICS
    brixPublishGCThreadAllocationStats();
    std::lock_guard<std::mutex> lock(gcBenchThreadRefillMutex);
    return gcBenchThreadRefillRecords.size();
#else
    return 0;
#endif
}

extern "C" size_t
brixGCRuntimeBenchThreadRefillValue(size_t index) {
#if BX_ENABLE_BENCH_METRICS
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
brixGCRuntimeBenchAllocationSizeBinCount() {
#if BX_ENABLE_BENCH_METRICS
    return kBrixGCBenchAllocationBinCount;
#else
    return 0;
#endif
}

extern "C" size_t
brixGCRuntimeBenchAllocationSizeBinUpperBound(size_t index) {
#if BX_ENABLE_BENCH_METRICS
    if (index < kBrixGCBenchAllocationSmallBinCount) {
        return (index + 1) * 8;
    }
    if (index < kBrixGCBenchAllocationOverflowBin) {
        return 1024 +
            (index - kBrixGCBenchAllocationSmallBinCount + 1) * 1024;
    }
#else
    (void)index;
#endif
    return std::numeric_limits<size_t>::max();
}

extern "C" size_t
brixGCRuntimeBenchAllocationSizeBinValue(size_t index) {
#if BX_ENABLE_BENCH_METRICS
    return index < gcBenchAllocationSizeBins.size()
        ? gcBenchAllocationSizeBins[index].load(std::memory_order_relaxed)
        : 0;
#else
    (void)index;
    return 0;
#endif
}

extern "C" size_t
brixGCRuntimeBenchAllocatedSizeClassCount() {
#if BX_ENABLE_BENCH_METRICS
    return kBrixGCBenchSizeClassCount;
#else
    return 0;
#endif
}

extern "C" size_t
brixGCRuntimeBenchAllocatedSizeClassUpperBound(size_t index) {
#if BX_ENABLE_BENCH_METRICS
    return index < kBrixGCBenchSizeClassCount
        ? kBrixGCBenchSizeClassUpperBounds[index]
        : std::numeric_limits<size_t>::max();
#else
    (void)index;
    return std::numeric_limits<size_t>::max();
#endif
}

extern "C" size_t
brixGCRuntimeBenchAllocatedSizeClassValue(size_t index) {
#if BX_ENABLE_BENCH_METRICS
    return index < gcBenchAllocatedSizeClasses.size()
        ? gcBenchAllocatedSizeClasses[index].load(std::memory_order_relaxed)
        : 0;
#else
    (void)index;
    return 0;
#endif
}

extern "C" void
brixGCRuntimeBenchResetAllocationHistograms() {
#if BX_ENABLE_BENCH_METRICS
    for (auto& count : gcBenchAllocationSizeBins) {
        count.store(0, std::memory_order_relaxed);
    }
    for (auto& count : gcBenchAllocatedSizeClasses) {
        count.store(0, std::memory_order_relaxed);
    }
#endif
}

extern "C" size_t
brixGCRuntimePauseEventCapacity() {
    return kBrixGCPauseEventCapacity;
}

extern "C" void
brixGCRuntimeRecordSubphaseDuration(
    size_t subphase,
    size_t cycle,
    size_t durationNanos,
    size_t value
) {
    if (!brixGCValidSubphase(subphase) || !durationNanos) {
        return;
    }
    brixGCRecordSubphaseSample(
        subphase,
        cycle,
        brixGCMonotonicNanos() - durationNanos,
        durationNanos,
        value);
}

extern "C" void
brixGCRuntimeSetMarkQueueDepth(size_t depth) {
    gcMarkQueueDepth.store(depth, std::memory_order_release);
}

extern "C" void
brixGCRuntimeSetAssistDebt(size_t debt) {
    gcAssistDebt.store(debt, std::memory_order_release);
}

extern "C" void
brixGCRuntimeRecordBarrier(size_t slowCount, size_t shadeCount) {
    BrixGCBarrierStatsRecord* record = brixGCEnsureThreadBarrierStats();
    brixGCAddOwnedBarrierCounter(record->barrierCalls, 1);
    brixGCAddOwnedBarrierCounter(record->barrierSlowEntries, slowCount);
    brixGCAddOwnedBarrierCounter(record->barrierShades, shadeCount);
}

extern "C" void
brixGCRuntimeRecordBarrierSlow() {
    BrixGCBarrierStatsRecord* record = brixGCEnsureThreadBarrierStats();
    brixGCAddOwnedBarrierCounter(record->barrierSlowEntries, 1);
}

extern "C" void
brixGCRuntimeRecordBarrierShade() {
    BrixGCBarrierStatsRecord* record = brixGCEnsureThreadBarrierStats();
    brixGCAddOwnedBarrierCounter(record->barrierShades, 1);
}

extern "C" void
brixGCRuntimeRecordAssist() {
    ++gcHotAllocationState.stats.assistCount;
}

extern "C" void
brixGCRuntimeRecordAllocatedDuringMark() {
    gcAllocatedDuringMarkCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
brixGCRuntimeRecordMarkQueuePushes(size_t count, size_t depth) {
    gcMarkQueuePushCount.fetch_add(count, std::memory_order_relaxed);
    gcMarkQueueDepth.store(depth, std::memory_order_release);
}

extern "C" void
brixGCRuntimeRecordMarkQueuePopBatch(size_t count, size_t depth) {
    if (!count) {
        return;
    }
    gcMarkQueuePopCount.fetch_add(count, std::memory_order_relaxed);
    gcMarkQueuePopBatchCount.fetch_add(1, std::memory_order_relaxed);
    brixGCMaxAtomic(gcMarkQueueMaxPopBatch, count);
    gcMarkQueueDepth.store(depth, std::memory_order_release);
}

extern "C" void
brixGCRuntimeRecordAllocation(size_t requestedSize, size_t allocatedSize) {
    brixRecordGCAllocation(requestedSize, allocatedSize);
}

extern "C" void
brixGCRuntimeRecordZeroedBytes(size_t byteSize) {
    brixGCRecordAllocationZeroedBytes(byteSize);
}

extern "C" void
brixGCRuntimeRecordBatchReservation(size_t count, size_t allocatedSize) {
    size_t bytes = count * allocatedSize;
    gcBatchReservedBytes.fetch_add(bytes, std::memory_order_relaxed);
    size_t reservedBytes = gcHotAllocationState.batchPendingReservationBytes;
    if (reservedBytes >= bytes) {
        size_t surplusBytes = reservedBytes - bytes;
        gcHotAllocationState.batchPendingReservationBytes = 0;
        if (surplusBytes) {
            brixGCSubtractAtomicSaturating(gcBatchCacheBytes, surplusBytes);
            gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > surplusBytes
                ? gcHotAllocationState.batchCacheBytes - surplusBytes
                : 0;
        }
    } else {
        size_t extraBytes = bytes - reservedBytes;
        gcHotAllocationState.batchPendingReservationBytes = 0;
        if (extraBytes) {
            if (brixGCBatchCacheReserve(extraBytes)) {
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
    brixRecordGCHeapAllocation(count, allocatedSize);
}

extern "C" void
brixGCRuntimeRecordBatchConsumption(size_t allocatedSize) {
    gcHotAllocationState.stats.batchConsumedBytes += allocatedSize;
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > allocatedSize
        ? gcHotAllocationState.batchCacheBytes - allocatedSize
        : 0;
}

extern "C" void
brixGCRuntimeCancelBatchCacheReservation(size_t bytes) {
    brixGCBatchCacheCancelReservation(bytes);
}

extern "C" void
brixGCRuntimeRecordBatchRelease(size_t count, size_t allocatedSize) {
    brixPublishGCThreadBatchConsumption();
    size_t bytes = count * allocatedSize;
    gcBatchReleasedBytes.fetch_add(bytes, std::memory_order_relaxed);
    brixGCSubtractAtomicSaturating(gcBatchCacheBytes, bytes);
    gcHotAllocationState.batchCacheBytes = gcHotAllocationState.batchCacheBytes > bytes ? gcHotAllocationState.batchCacheBytes - bytes : 0;
}

extern "C" void
brixGCRuntimeRecordBatchDiscard(size_t count, size_t allocatedSize) {
    brixPublishGCThreadBatchConsumption();
    size_t bytes = count * allocatedSize;
    brixGCRecordBatchDiscard(bytes);
}

extern "C" void
brixGCRuntimeRecordSweep(size_t freedObjects, size_t freedBytes) {
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
brixGCRuntimeRecordSweepPages(size_t pages) {
    if (pages) {
        gcCurrentCycleSweepPages.fetch_add(pages, std::memory_order_relaxed);
    }
}

extern "C" void
brixGCRuntimeRecordScavengePages(size_t pages) {
    if (!pages) {
        return;
    }
    gcScavengedPageTotal.fetch_add(pages, std::memory_order_relaxed);
    size_t cycleId = gcCurrentCycleId.load(std::memory_order_acquire);
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    for (size_t offset = 0; offset < gcCycleSummaryStored; ++offset) {
        size_t sequence = gcCycleSummarySequence - 1 - offset;
        BrixGCCycleSummary& summary =
            gcCycleSummaries[sequence % kBrixGCCycleSummaryCapacity];
        if (summary.cycleId == cycleId) {
            summary.scavengedPages += pages;
            return;
        }
    }
}

extern "C" void
brixGCRuntimeRecordSpanAllocation() {
    gcAllocatedSpans.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
brixGCRuntimeRecordArenaAllocation() {
    gcAllocatedArenas.fetch_add(1, std::memory_order_relaxed);
}

extern "C" bool
brixGCRuntimeCanReserveBatchCache(size_t bytes) {
    return brixGCBatchCacheReserve(bytes);
}

extern "C" void
brixGCRuntimeFlushThreadAllocationBatches() {
    brixWBBufFlush();
    brixFlushGCAllocationBatches();
    brixPublishGCThreadAllocationStats();
}

extern "C" void
brixGCRuntimeFlushWriteBarrierBuffer() {
    // Idle tasks cannot have acquired new buffered edges, and the termination handshake drains
    // every task before Sweep. Avoid resolving the thread-local cache on the very frequent
    // coroutine blocked-state boundary when no buffered publication can exist.
    if (!brixGCPhaseNeedsMarkBarrier(brixGCLoadPublicPhase())) {
        return;
    }
    brixWBBufFlush();
}

extern "C" void
brixGCRuntimeFlushActiveAllocationBuffer() {
    // Ordinary task suspension detaches task-local roots, but compiler and generic ranges are
    // owned by the BxThread cache. Keep those generation-checked ranges hot across coroutine
    // switches; collector handshakes and explicit full-thread flushes still revoke them.
    if (gcActiveAllocationBufferStorage) {
        brixFlushGCCompilerAllocationBuffer(*gcActiveAllocationBufferStorage);
    }
    gcHotAllocationState.stats.flush();
}

extern "C" void
brixGCRuntimeRevokeActiveAllocationBuffer() {
    brixFlushGCCompilerAllocationBuffer();
}

extern "C" void
brixGCRuntimeSetHeapGoal(size_t goal) {
    if (brixGCCollectionDisabled()) {
        gcRequestedHeapGoal.store(
            std::numeric_limits<size_t>::max(),
            std::memory_order_release);
        gcHeapGoal.store(std::numeric_limits<size_t>::max(), std::memory_order_release);
        brixGCPacerInvalidateGoalCache();
        return;
    }
    size_t cappedGoal = goal ? brixGCApplyHeapHardCap(goal) : brixGCMinimumHeapGoal();
    gcRequestedHeapGoal.store(cappedGoal, std::memory_order_release);
    gcHeapGoal.store(cappedGoal, std::memory_order_release);
    brixGCPacerInvalidateGoalCache();
}

extern "C" void
brixGCRuntimeSetMinimumHeapGoal(size_t goal) {
    gcProgramMinimumHeapGoal.store(goal, std::memory_order_release);
    if (brixGCCollectionDisabled() || gcRequestedHeapGoal.load(std::memory_order_acquire)) {
        return;
    }
    size_t minimum = brixGCMinimumHeapGoal();
    size_t current = gcHeapGoal.load(std::memory_order_acquire);
    while (current < minimum &&
            !gcHeapGoal.compare_exchange_weak(
                current,
                minimum,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
    }
    brixGCPacerInvalidateGoalCache();
}

static size_t
brixGCTaperedGrowth(size_t live) {
    size_t percent = brixGCHeapGrowthPercent();
    if (live <= kBrixGCHeapTaperStart) {
        return brixGCMultiplyDivideSaturating(live, percent, 100);
    }
    size_t full = brixGCMultiplyDivideSaturating(kBrixGCHeapTaperStart, percent, 100);
    size_t half = brixGCMultiplyDivideSaturating(live - kBrixGCHeapTaperStart, percent, 200);
    return full > std::numeric_limits<size_t>::max() - half
        ? std::numeric_limits<size_t>::max()
        : full + half;
}

static size_t
brixGCLimitGrowthToAvailableMemory(size_t growth) {
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
brixGCRuntimeFinishCollection() {
    gcMetadataTrimPending.store(true, std::memory_order_release);
    size_t live = gcLiveBytes.load(std::memory_order_relaxed);
    if (gcCurrentCycleTriggerReason.load(std::memory_order_acquire) == BrixGCPacerTriggerIdle) {
        gcIdleSettledLiveBytes.store(live, std::memory_order_release);
    }
    size_t requestedGoal = gcRequestedHeapGoal.load(std::memory_order_acquire);
    if (requestedGoal && !gcCollectionMayGrowRequestedGoal.load(std::memory_order_acquire)) {
        brixGCPacerPublishCycleStats(live, gcHeapGoal.load(std::memory_order_acquire));
        gcCollectionCount.fetch_add(1, std::memory_order_relaxed);
        brixGCRuntimeCycleEnd();
        return;
    }

    size_t minimumGoal = requestedGoal ? requestedGoal : brixGCMinimumHeapGoal();
    size_t minimumGrowth = requestedGoal
        ? std::max<size_t>(requestedGoal / 2, 64ull * 1024ull)
        : brixGCHeapMinGrowth();
    size_t growth = brixGCLimitGrowthToAvailableMemory(std::max(
        minimumGrowth,
        brixGCTaperedGrowth(live)));
    size_t goal = live > std::numeric_limits<size_t>::max() - growth
        ? std::numeric_limits<size_t>::max()
        : live + growth;
    goal = std::max(goal, minimumGoal);
    size_t overshootCorrection =
        gcPacerLastGoalOvershootBytes.load(std::memory_order_acquire) / 2;
    if (goal > live && overshootCorrection) {
        goal -= std::min(goal - live, overshootCorrection);
    }
    goal = brixGCApplyHeapHardCap(goal);
    gcHeapGoal.store(goal, std::memory_order_release);
    brixGCPacerPublishCycleStats(live, goal);
    gcCollectionCount.fetch_add(1, std::memory_order_relaxed);
    brixGCRuntimeCycleEnd();
}

extern "C" void
brixGCRuntimeRecordAutomaticFallback(size_t reason) {
    gcAutomaticFallbackReason.store(reason, std::memory_order_release);
    gcAutomaticFallbackCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
brixGCRuntimeMarkStart() {
    gcMarkStartNanos.store(brixGCMonotonicNanos(), std::memory_order_release);
}

extern "C" void
brixGCRuntimeMarkEnd() {
    uint64_t startNanos = gcMarkStartNanos.exchange(0, std::memory_order_acq_rel);
    if (!startNanos) {
        return;
    }
    size_t duration = static_cast<size_t>(brixGCMonotonicNanos() - startNanos);
    gcMarkTimeNanos.fetch_add(duration, std::memory_order_relaxed);
    gcCurrentCycleMarkDurationNanos.store(duration, std::memory_order_release);
}

extern "C" void
brixGCRuntimeSweepStart() {
    gcSweepStartNanos = brixGCMonotonicNanos();
    brixGCRuntimeSubphaseStart(
        BrixGCObservationSweep,
        gcCurrentCycleId.load(std::memory_order_acquire),
        0);
}

extern "C" void
brixGCRuntimeSweepEnd() {
    if (!gcSweepStartNanos) {
        return;
    }
    gcSweepTimeNanos.fetch_add(
        (size_t)(brixGCMonotonicNanos() - gcSweepStartNanos),
        std::memory_order_relaxed);
    brixGCRuntimeSubphaseEnd(
        BrixGCObservationSweep,
        gcCycleAllocatedBytes.load(std::memory_order_relaxed));
    gcSweepStartNanos = 0;
}

extern "C" bool
brixGCRuntimeDecommitPages(void* pointer, size_t byteSize) {
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
brixGCRuntimeRecommitPages(void* pointer, size_t byteSize) {
    if (!pointer || byteSize == 0) {
        return true;
    }
    return mprotect(pointer, byteSize, PROT_READ | PROT_WRITE) == 0;
}

extern "C" void
brixGCRuntimeHandshakeStart() {
    brixPublishGCThreadAllocationStats();
    gcHandshakeStartNanos = brixGCMonotonicNanos();
    gcHandshakePauseKind = 0;
    gcHandshakeWaitRecorded = false;
    gcHandshakeTasksWaited = 0;
    gcHandshakeSlowestTaskNanos = 0;
    brixGCRuntimeSubphaseStart(
        BrixGCObservationHandshake,
        gcCurrentCycleId.load(std::memory_order_acquire),
        0);
}

extern "C" void
brixGCRuntimeHandshakeSetReason(size_t reason) {
    gcHandshakePauseKind = brixGCPauseKindForHandshakeReason(reason);
}

extern "C" void
brixGCRuntimeHandshakeWaitEnd() {
    brixPublishGCThreadAllocationStats();
    if (gcHandshakeStartNanos && !gcHandshakeWaitRecorded) {
        uint64_t now = brixGCMonotonicNanos();
        gcHandshakeLatencyNanos.fetch_add(
            (size_t)(now - gcHandshakeStartNanos),
            std::memory_order_relaxed);
        gcHandshakeCount.fetch_add(1, std::memory_order_relaxed);
        gcHandshakeWaitRecorded = true;
    }
}

extern "C" void
brixGCRuntimeRecordPauseTaskWait(size_t tasksWaited, size_t slowestTaskNanos) {
    gcHandshakeTasksWaited = tasksWaited;
    gcHandshakeSlowestTaskNanos = slowestTaskNanos;
}

extern "C" void
brixGCRuntimeRecordPerTaskSetupScan(size_t startNanos, size_t durationNanos) {
    if (gcHandshakePauseKind != BrixGCPauseKindMarkSetupTotal) {
        return;
    }
    brixGCRecordPauseEvent(
        BrixGCPauseKindPerTaskSetupScan,
        gcCurrentCycleId.load(std::memory_order_acquire),
        startNanos,
        durationNanos,
        1,
        durationNanos);
}

extern "C" void
brixGCRuntimeHandshakeEnd() {
    brixPublishGCThreadAllocationStats();
    if (!gcHandshakeStartNanos) {
        return;
    }
    brixGCRuntimeHandshakeWaitEnd();

    uint64_t endNanos = brixGCMonotonicNanos();
    size_t durationNanos = static_cast<size_t>(endNanos - gcHandshakeStartNanos);
    size_t cycleId = gcCurrentCycleId.load(std::memory_order_acquire);
    brixGCRecordPauseEvent(
        gcHandshakePauseKind,
        cycleId,
        gcHandshakeStartNanos,
        durationNanos,
        gcHandshakeTasksWaited,
        gcHandshakeSlowestTaskNanos);
    brixGCRuntimeSubphaseEnd(
        BrixGCObservationHandshake,
        gcMarkQueueDepth.load(std::memory_order_relaxed));
    gcHandshakeStartNanos = 0;
    gcHandshakePauseKind = 0;
    gcHandshakeWaitRecorded = false;
    gcHandshakeTasksWaited = 0;
    gcHandshakeSlowestTaskNanos = 0;
}

extern "C" bool
brixGCRuntimeVerifyEachSweep() {
    static const bool verifyEachSweep = []() {
        const char* value = std::getenv("BRIX_GC_VERIFY_EACH_SWEEP");
        return value && *value && std::strcmp(value, "0") != 0;
    }();
    return verifyEachSweep;
}

extern "C" size_t
brixGCRuntimeAllocatedObjectCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocatedObjects.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeRequestedByteCount() {
    brixPublishGCThreadAllocationStats();
    return gcRequestedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocatedByteCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocatedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeLiveByteCount() {
    return gcLiveBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocatedSpanCount() {
    return gcAllocatedSpans.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocatedArenaCount() {
    return gcAllocatedArenas.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeHeapGoal() {
    size_t goal = gcHeapGoal.load(std::memory_order_acquire);
    return goal ? goal : brixGCMinimumHeapGoal();
}

extern "C" size_t
brixGCRuntimePacerGrowthPercent() {
    return brixGCHeapGrowthPercent();
}

extern "C" size_t
brixGCRuntimeBackgroundWorkerCount() {
    return gcBackgroundWorkerCount.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeBackgroundWorkerThreadCapacity() {
    return gcBackgroundWorkerThreadCapacity.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeBackgroundWorkerDutyPermille() {
    return gcBackgroundWorkerDutyPermille.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeBackgroundWorkerWork() {
    return gcBackgroundWorkerWork.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeBackgroundCredit() {
    return gcBackgroundCredit.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeFrameBudgetOfferCount() {
    return gcFrameBudgetOfferCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeFrameBudgetWorkCount() {
    return gcFrameBudgetWorkCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeFrameBudgetWorkNanos() {
    return gcFrameBudgetWorkNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeFrameBudgetOverrunCount() {
    return gcFrameBudgetOverrunCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeFrameBudgetMaxOverrunNanos() {
    return gcFrameBudgetMaxOverrunNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimePacerHardCap() {
    return brixGCHeapHardCap();
}

extern "C" size_t
brixGCRuntimePacerSoftGoal() {
    size_t goal = brixGCRuntimeHeapGoal();
    size_t softGoal = brixGCAutomaticSoftGoal(goal);
    gcPacerLastSoftGoal.store(softGoal, std::memory_order_release);
    return softGoal;
}

extern "C" size_t
brixGCRuntimePacerHardGoal() {
    size_t goal = brixGCRuntimeHeapGoal();
    size_t hardGoal = brixGCAutomaticHardGoal(goal);
    gcPacerLastHardGoal.store(hardGoal, std::memory_order_release);
    return hardGoal;
}

extern "C" size_t
brixGCRuntimePacerLastTriggerReason() {
    return gcPacerLastTriggerReason.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeCollectionCount() {
    return gcCollectionCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeMarkTimeNanos() {
    return gcMarkTimeNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeSweepTimeNanos() {
    return gcSweepTimeNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationStallCount() {
    return gcAllocationStallCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationStallTimeNanos() {
    return gcAllocationStallTimeNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeHandshakeCount() {
    return gcHandshakeCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeHandshakeLatencyNanos() {
    return gcHandshakeLatencyNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeCurrentCycleId() {
    return gcCurrentCycleId.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeActiveCycleAgeNanos() {
    uint64_t start = gcActiveCycleStartNanos.load(std::memory_order_acquire);
    if (!start) {
        return 0;
    }
    return static_cast<size_t>(brixGCMonotonicNanos() - start);
}

extern "C" size_t
brixGCRuntimeMarkQueueDepth() {
    return gcMarkQueueDepth.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeAssistDebt() {
    return gcAssistDebt.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeBarrierCount() {
    brixPublishGCWriteBarrierCounters(brixGCEnsureActiveThreadCache());
    return brixGCSumBarrierCounter(BrixGCBarrierStatsCalls);
}

extern "C" size_t
brixGCRuntimeAssistCount() {
    brixPublishGCThreadAllocationStats();
    return gcAssistCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAssistTimeNanos() {
    brixPublishGCThreadAllocationStats();
    return gcAssistTimeNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeMarkQueuePushCount() {
    return gcMarkQueuePushCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeMarkQueuePopCount() {
    return gcMarkQueuePopCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeMarkQueuePopBatchCount() {
    return gcMarkQueuePopBatchCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeMarkQueueMaxPopBatch() {
    return gcMarkQueueMaxPopBatch.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeBatchReservedBytes() {
    brixPublishGCThreadBatchConsumption();
    return gcBatchReservedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeBatchConsumedBytes() {
    brixPublishGCThreadBatchConsumption();
    return gcBatchConsumedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeBatchReleasedBytes() {
    brixPublishGCThreadBatchConsumption();
    return gcBatchReleasedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeBatchDiscardedBytes() {
    brixPublishGCThreadBatchConsumption();
    return gcBatchDiscardedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeBatchCacheBytes() {
    brixPublishGCThreadBatchConsumption();
    return gcBatchCacheBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeBatchUnusedBytes() {
    brixPublishGCThreadBatchConsumption();
    return gcBatchCacheBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeThreadBatchCacheBytes() {
    return gcHotAllocationState.batchCacheBytes;
}

extern "C" size_t
brixGCRuntimeAdaptiveBatchGrowthCount() {
    brixPublishGCThreadAllocationStats();
    return gcAdaptiveBatchGrowthCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAdaptiveBatchShrinkCount() {
    brixPublishGCThreadAllocationStats();
    return gcAdaptiveBatchShrinkCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAdaptiveBatchReturnedUnusedSlotCount() {
    brixPublishGCThreadAllocationStats();
    return gcAdaptiveBatchReturnedUnusedSlotCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeCompilerIndexedRefillCount() {
    brixPublishGCThreadAllocationStats();
    return gcCompilerIndexedRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeActiveBufferRefillCount() {
    brixPublishGCThreadAllocationStats();
    return gcActiveBufferRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeGenericDescribedRefillCount() {
    brixPublishGCThreadAllocationStats();
    return gcGenericDescribedRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeGenericDescriptorlessRefillCount() {
    brixPublishGCThreadAllocationStats();
    return gcGenericDescriptorlessRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeCompilerBufferSiteSwitchCount() {
    return gcCompilerBufferSiteSwitchCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeFullAllocationRangeFlushCount() {
    return gcFullAllocationRangeFlushCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeFullAllocationRangeFlushedBufferCount() {
    return gcFullAllocationRangeFlushedBufferCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeBatchCachePressureFlushCount() {
    return gcBatchCachePressureFlushCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeThreadPendingAllocatedObjectCount() {
    size_t objectCount = gcHotAllocationState.stats.allocatedObjects;
    size_t consumedBytes = 0;
    if (gcActiveAllocationBufferStorage) {
        brixGCAddPendingCompilerAllocationBufferStats(
            *gcActiveAllocationBufferStorage,
            objectCount,
            consumedBytes);
    }
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        size_t slot = cache->activeCompilerBufferIndices[active];
        if (slot >= BrixGCCompilerAllocationBufferCount) {
            continue;
        }
        const BrixGCRuntimeAllocationBuffer& storage = cache->compilerBuffers[slot];
        if (gcActiveAllocationBufferStorage == &storage) {
            continue;
        }
        brixGCAddPendingCompilerAllocationBufferStats(storage, objectCount, consumedBytes);
    }
    return objectCount;
}

extern "C" size_t
brixGCRuntimeThreadPendingBatchConsumedBytes() {
    size_t objectCount = 0;
    size_t consumedBytes = gcHotAllocationState.stats.batchConsumedBytes;
    if (gcActiveAllocationBufferStorage) {
        brixGCAddPendingCompilerAllocationBufferStats(
            *gcActiveAllocationBufferStorage,
            objectCount,
            consumedBytes);
    }
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
        size_t slot = cache->activeCompilerBufferIndices[active];
        if (slot >= BrixGCCompilerAllocationBufferCount) {
            continue;
        }
        const BrixGCRuntimeAllocationBuffer& storage = cache->compilerBuffers[slot];
        if (gcActiveAllocationBufferStorage == &storage) {
            continue;
        }
        brixGCAddPendingCompilerAllocationBufferStats(storage, objectCount, consumedBytes);
    }
    return consumedBytes;
}

extern "C" size_t
brixGCRuntimeStaleBatchDropCount() {
    return gcStaleBatchDropCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeFallbackAllocationCount() {
    return gcFallbackAllocationCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationCallCount(size_t kind) {
    brixPublishGCThreadAllocationStats();
    if (kind >= gcAllocationCallsByKind.size()) {
        return 0;
    }
    return gcAllocationCallsByKind[kind].load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationPhaseCount(size_t phase) {
    brixPublishGCThreadAllocationStats();
    if (phase >= gcAllocationCallsByPhase.size()) {
        return 0;
    }
    return gcAllocationCallsByPhase[phase].load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationFastHitCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationFastHitCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationFastRefillCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationFastRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationFastFallbackCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationFastFallbackCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationLargeCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationLargeCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationBatchMissCount(size_t reason) {
    brixPublishGCThreadAllocationStats();
    if (reason >= gcAllocationBatchMisses.size()) {
        return 0;
    }
    return gcAllocationBatchMisses[reason].load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationZeroedBytes() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationZeroedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationStatPublicationCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationStatPublicationCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationAssistCheckCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationAssistCheckCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationAssistCallCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationAssistCallCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeFastBarrierCallCount() {
    brixPublishGCWriteBarrierCounters(brixGCEnsureActiveThreadCache());
    return brixGCSumBarrierCounter(BrixGCBarrierStatsFastCalls);
}

extern "C" size_t
brixGCRuntimeFastBarrierSlowEntryCount() {
    brixPublishGCWriteBarrierCounters(brixGCEnsureActiveThreadCache());
    return brixGCSumBarrierCounter(BrixGCBarrierStatsFastSlowEntries);
}

extern "C" size_t
brixGCRuntimeBufferedBarrierSharedRMWCount() {
    brixPublishGCWriteBarrierCounters(brixGCEnsureActiveThreadCache());
    // Compiler barrier counters are transferred only into their OS-thread-owned record.
    return 0;
}

extern "C" size_t
brixGCRuntimeAllocationDescriptorMixCount(size_t index, size_t described) {
    brixPublishGCThreadAllocationStats();
    if (index >= kBrixGCAllocationBatchCount) {
        return 0;
    }
    const auto& counters = described
        ? gcAllocationDescribedBySizeClass
        : gcAllocationDescriptorlessBySizeClass;
    return counters[index].load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationSiteHitCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationSiteHitCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationSiteMissCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationSiteMissCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationSiteRefillCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationSiteRefillCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAllocationSiteFallbackCount() {
    brixPublishGCThreadAllocationStats();
    return gcAllocationSiteFallbackCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAutomaticFallbackCount() {
    return gcAutomaticFallbackCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimeAutomaticFallbackReason() {
    return gcAutomaticFallbackReason.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeAutomaticDrainGiveUpCount() {
    return gcAutomaticDrainGiveUpCount.load(std::memory_order_relaxed);
}

static size_t
brixGCObservationSampleCountForKind(size_t observation) {
    if (!brixGCValidSubphase(observation)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    return gcSubphaseSamples[observation].durations.size();
}

static size_t
brixGCObservationMaxNanosForKind(size_t observation) {
    if (!brixGCValidSubphase(observation)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    return gcSubphaseSamples[observation].maxDurationNanos;
}

#define BX_GC_NAMED_OBSERVATION_COUNT(name, observation) \
    extern "C" size_t brixGCRuntime##name(void) { \
        return brixGCObservationSampleCountForKind(observation); \
    }
#define BX_GC_NAMED_OBSERVATION_PERCENTILE(name, observation, percentile) \
    extern "C" size_t brixGCRuntime##name(void) { \
        return brixGCSubphasePercentile(observation, percentile); \
    }
#define BX_GC_NAMED_OBSERVATION_MAX(name, observation) \
    extern "C" size_t brixGCRuntime##name(void) { \
        return brixGCObservationMaxNanosForKind(observation); \
    }

BX_GC_NAMED_OBSERVATION_COUNT(MarkSetupSampleCount, BrixGCObservationMarkSetup)
BX_GC_NAMED_OBSERVATION_PERCENTILE(MarkSetupP50Nanos, BrixGCObservationMarkSetup, 50)
BX_GC_NAMED_OBSERVATION_MAX(MarkSetupMaxNanos, BrixGCObservationMarkSetup)
BX_GC_NAMED_OBSERVATION_COUNT(RootScanSampleCount, BrixGCObservationRootScan)
BX_GC_NAMED_OBSERVATION_PERCENTILE(RootScanP95Nanos, BrixGCObservationRootScan, 95)
BX_GC_NAMED_OBSERVATION_PERCENTILE(RootScanP99Nanos, BrixGCObservationRootScan, 99)
BX_GC_NAMED_OBSERVATION_MAX(RootScanMaxNanos, BrixGCObservationRootScan)
BX_GC_NAMED_OBSERVATION_COUNT(MarkWorkSampleCount, BrixGCObservationMarkWork)
BX_GC_NAMED_OBSERVATION_PERCENTILE(MarkWorkP95Nanos, BrixGCObservationMarkWork, 95)
BX_GC_NAMED_OBSERVATION_PERCENTILE(MarkWorkP99Nanos, BrixGCObservationMarkWork, 99)
BX_GC_NAMED_OBSERVATION_MAX(MarkWorkMaxNanos, BrixGCObservationMarkWork)
BX_GC_NAMED_OBSERVATION_COUNT(
    MarkTerminationSampleCount,
    BrixGCObservationMarkTermination)
BX_GC_NAMED_OBSERVATION_PERCENTILE(
    MarkTerminationP99Nanos,
    BrixGCObservationMarkTermination,
    99)
BX_GC_NAMED_OBSERVATION_MAX(
    MarkTerminationMaxNanos,
    BrixGCObservationMarkTermination)
BX_GC_NAMED_OBSERVATION_COUNT(SweepSampleCount, BrixGCObservationSweep)
BX_GC_NAMED_OBSERVATION_PERCENTILE(SweepP99Nanos, BrixGCObservationSweep, 99)
BX_GC_NAMED_OBSERVATION_MAX(SweepMaxNanos, BrixGCObservationSweep)
BX_GC_NAMED_OBSERVATION_COUNT(ScavengeSampleCount, BrixGCObservationScavenge)
BX_GC_NAMED_OBSERVATION_PERCENTILE(ScavengeP99Nanos, BrixGCObservationScavenge, 99)
BX_GC_NAMED_OBSERVATION_MAX(ScavengeMaxNanos, BrixGCObservationScavenge)
BX_GC_NAMED_OBSERVATION_COUNT(
    AllocationStallSampleCount,
    BrixGCObservationAllocationStall)
BX_GC_NAMED_OBSERVATION_COUNT(
    FallbackAllocationSampleCount,
    BrixGCObservationFallbackAllocation)
BX_GC_NAMED_OBSERVATION_COUNT(HandshakeSampleCount, BrixGCObservationHandshake)
BX_GC_NAMED_OBSERVATION_PERCENTILE(HandshakeP99Nanos, BrixGCObservationHandshake, 99)
BX_GC_NAMED_OBSERVATION_MAX(HandshakeMaxNanos, BrixGCObservationHandshake)
BX_GC_NAMED_OBSERVATION_COUNT(
    StackMapLookupSampleCount,
    BrixGCObservationStackMapLookup)
BX_GC_NAMED_OBSERVATION_PERCENTILE(
    StackMapLookupP99Nanos,
    BrixGCObservationStackMapLookup,
    99)

#undef BX_GC_NAMED_OBSERVATION_COUNT
#undef BX_GC_NAMED_OBSERVATION_PERCENTILE
#undef BX_GC_NAMED_OBSERVATION_MAX

extern "C" size_t
brixGCRuntimePauseCount() {
    return gcPauseCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRuntimePauseEventCount() {
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    return gcPauseEventStored;
}

static BrixGCPauseEvent
brixGCRuntimePauseEventAt(size_t index) {
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    if (index >= gcPauseEventStored) {
        return BrixGCPauseEvent {};
    }
    size_t firstSequence = gcPauseEventSequence - gcPauseEventStored;
    size_t eventIndex = (firstSequence + index) % kBrixGCPauseEventCapacity;
    return gcPauseEvents[eventIndex];
}

extern "C" size_t
brixGCRuntimePauseEventCycleId(size_t index) {
    return brixGCRuntimePauseEventAt(index).cycleId;
}

extern "C" size_t
brixGCRuntimePauseEventKind(size_t index) {
    return brixGCRuntimePauseEventAt(index).kind;
}

extern "C" size_t
brixGCRuntimePauseEventStartNanos(size_t index) {
    return static_cast<size_t>(brixGCRuntimePauseEventAt(index).startNanos);
}

extern "C" size_t
brixGCRuntimePauseEventDurationNanos(size_t index) {
    return brixGCRuntimePauseEventAt(index).durationNanos;
}

extern "C" size_t
brixGCRuntimePauseEventTasksWaited(size_t index) {
    return brixGCRuntimePauseEventAt(index).tasksWaited;
}

extern "C" size_t
brixGCRuntimePauseEventSlowestTaskNanos(size_t index) {
    return brixGCRuntimePauseEventAt(index).slowestTaskNanos;
}

extern "C" size_t
brixGCRuntimePauseP50Nanos(size_t kind) {
    return brixGCPausePercentile(kind, 50);
}

extern "C" size_t
brixGCRuntimePauseP95Nanos(size_t kind) {
    return brixGCPausePercentile(kind, 95);
}

extern "C" size_t
brixGCRuntimePauseP99Nanos(size_t kind) {
    return brixGCPausePercentile(kind, 99);
}

extern "C" size_t
brixGCRuntimePauseMaxNanos(size_t kind) {
    if (!brixGCValidPauseKind(kind)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    size_t maximum = 0;
    size_t firstSequence = gcPauseEventSequence - gcPauseEventStored;
    for (size_t index = 0; index < gcPauseEventStored; ++index) {
        const BrixGCPauseEvent& event =
            gcPauseEvents[(firstSequence + index) % kBrixGCPauseEventCapacity];
        if (event.kind == kind) {
            maximum = std::max(maximum, event.durationNanos);
        }
    }
    return maximum;
}

extern "C" size_t
brixGCRuntimeCycleSummaryCount() {
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    return gcCycleSummaryStored;
}

static BrixGCCycleSummary
brixGCRuntimeCycleSummaryAt(size_t index) {
    std::lock_guard<std::mutex> lock(gcObservabilityMutex);
    if (index >= gcCycleSummaryStored) {
        return BrixGCCycleSummary {};
    }
    size_t firstSequence = gcCycleSummarySequence - gcCycleSummaryStored;
    return gcCycleSummaries[(firstSequence + index) % kBrixGCCycleSummaryCapacity];
}

#define BX_GC_CYCLE_SUMMARY_ACCESSOR(name, field) \
    extern "C" size_t name(size_t index) { \
        return brixGCRuntimeCycleSummaryAt(index).field; \
    }

BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryCycleId, cycleId)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryTriggerReason, triggerReason)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryHeapAtTrigger, heapAtTrigger)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryHeapGoal, heapGoal)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryHeapActual, heapActual)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryMarkDurationNanos, markDurationNanos)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryScanWork, scanWork)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryAssistWork, assistWork)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryBackgroundWork, backgroundWork)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummarySweepPages, sweepPages)
BX_GC_CYCLE_SUMMARY_ACCESSOR(brixGCRuntimeCycleSummaryScavengedPages, scavengedPages)

#undef BX_GC_CYCLE_SUMMARY_ACCESSOR

static bool
brixAllocSizeOverflow(size_t itemSize, size_t count) {
    return count != 0 && itemSize > std::numeric_limits<size_t>::max() / count;
}

// **********************************************************************************************

extern "C" bool
brixGCAllocationSiteKeyInit(
    BrixGCAllocationSiteKey* key,
    size_t requestedSize,
    void* descriptor,
    size_t pointerKind,
    size_t zeroingPolicy
) {
    return brixGCInitAllocationSiteKey(
        key,
        requestedSize,
        descriptor,
        pointerKind,
        zeroingPolicy);
}

extern "C" void
brixGCSetActiveThreadCache(BrixGCThreadCache* cache) {
    bool hasOwner = cache != nullptr;
    BrixGCThreadCache* previous = brixGCEnsureActiveThreadCache();
    if (previous != cache) {
        brixWBBufFlush();
        brixFlushGCCompilerAllocationBuffer();
    }
    if (!cache) {
        cache = &gcFallbackThreadCache;
        if (
            cache->abiVersion != BrixGCAllocationABIVersion ||
            cache->structSize != sizeof(BrixGCThreadCache)
        ) {
            *cache = BrixGCThreadCache {};
            brixGCInitializeThreadCache(cache);
        }
    }
    if (
        cache->abiVersion != BrixGCAllocationABIVersion ||
        cache->structSize != sizeof(BrixGCThreadCache)
    ) {
        std::fprintf(stderr, "Brix GC thread-cache ABI mismatch\n");
        std::abort();
    }
    brixGCActiveThreadCache = cache;
    // Register observability storage at thread/task attachment, not on the first hot barrier.
    brixGCEnsureThreadBarrierStats();
    cache->ownerEpoch += 1;
    gcActiveAllocationBufferStorage = hasOwner ? &cache->activeBuffer : nullptr;
    brixGCActiveAllocationBuffer = hasOwner ? &cache->activeBuffer.buffer : nullptr;
}

extern "C" BrixGCThreadCache*
brixGCGetActiveThreadCache() {
    return brixGCEnsureActiveThreadCache();
}

extern "C" bool
brixGCTestThreadCacheOwnerFlushOnce() {
    BrixGCThreadCache* previous = brixGCEnsureActiveThreadCache();
    brixFlushGCCompilerAllocationBuffer();

    BrixGCAllocationBatch batch {};
    if (!brixGCTestAllocBatch(16, nullptr, 4, &batch) || !batch.cursor || !batch.slotSize) {
        return false;
    }

    BrixGCThreadCache cache {};
    brixGCInitializeThreadCache(&cache);
    brixGCSetActiveThreadCache(&cache);
    BrixGCRuntimeAllocationBuffer& storage = cache.compilerBuffers[0];
    storage.base = batch.cursor;
    storage.buffer.cursor = batch.cursor + batch.slotSize;
    storage.buffer.limit = batch.limit;
    storage.slotSize = batch.slotSize;
    storage.requestedSize = 16;
    storage.generation = brixGCLoadPublicSweepGeneration();
    storage.sizeClassIndex = 2;
    cache.activeCompilerBufferCount = 1;
    cache.activeCompilerBufferIndices[0] = 0;

    size_t before = gcThreadCacheOwnerFlushCount.load(std::memory_order_relaxed);
    brixGCSetActiveThreadCache(nullptr);
    size_t after = gcThreadCacheOwnerFlushCount.load(std::memory_order_relaxed);
    size_t flushEpoch = cache.flushEpoch;
    brixGCSetActiveThreadCache(nullptr);
    bool unchanged =
        gcThreadCacheOwnerFlushCount.load(std::memory_order_relaxed) == after;
    brixGCSetActiveThreadCache(previous);
    return after == before + 1 && flushEpoch == 1 && unchanged;
}

extern "C" size_t
brixGCTestAdaptiveBatchPreferredCount(size_t compilerBufferIndex) {
    if (!brixGCCompilerAllocationBufferIndexIsValid(compilerBufferIndex)) {
        return 0;
    }
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    return cache->compilerBuffers[compilerBufferIndex - 1]
        .reserved[kBrixGCAdaptiveCountByte];
}

extern "C" size_t
brixGCTestParkedAllocationRangeCount(size_t sizeClassIndex) {
    if (sizeClassIndex >= BrixGCThreadCacheRangeCount) {
        return 0;
    }
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    size_t count = 0;
    for (size_t way = 0; way < BrixGCThreadCacheRangeWayCount; ++way) {
        const BrixGCRuntimeAllocationBuffer& storage =
            brixGCThreadRangeWay(cache, sizeClassIndex, way);
        if (brixGCAllocationRangeHasOwnership(storage) ||
                brixGCAllocationRangeHasPolicySite(storage)) {
            count += 1;
        }
    }
    return count;
}

extern "C" size_t
brixGCTestCompilerRangePacerChargedBytes() {
    return gcCompilerRangePacerChargedBytes;
}

extern "C" size_t
brixGCTestGenericRangePacerChargedBytes() {
    return gcHotAllocationState.genericPacerChargedBytes;
}

extern "C" size_t
brixGCTestGenericRangePacerCollectionCheckCount() {
    return gcHotAllocationState.genericPacerCollectionChecks;
}

extern "C" void
brixGCTestCrossAllocationStatPublicationThreshold() {
    gcHotAllocationState.stats.allocatedObjects = kBrixGCThreadStatFlushObjectCount - 1;
    brixRecordGCPublicAllocation(gcHotAllocationState, 1, brixGCObjectAllocationSize(1));
}

extern "C" void
brixGCSetActiveAllocationBufferStorage(BrixGCRuntimeAllocationBuffer* storage) {
    if (gcActiveAllocationBufferStorage && gcActiveAllocationBufferStorage != storage) {
        brixFlushGCCompilerAllocationBuffer(*gcActiveAllocationBufferStorage);
    }
    if (storage && (
            storage->abiVersion != BrixGCAllocationABIVersion ||
            storage->structSize != sizeof(BrixGCRuntimeAllocationBuffer))) {
        storage = nullptr;
    }
    gcActiveAllocationBufferStorage = storage;
    brixGCActiveAllocationBuffer = storage ? &storage->buffer : nullptr;
}

extern "C" BrixGCAllocationBuffer*
brixGCGetActiveAllocationBuffer() {
    return brixGCActiveAllocationBuffer;
}

extern "C" size_t
brixGCAllocationBufferCursorOffset() {
    return BrixGCAllocationBufferCursorOffset;
}

extern "C" size_t
brixGCAllocationBufferLimitOffset() {
    return BrixGCAllocationBufferLimitOffset;
}

extern "C" size_t
brixGCAllocationBufferSize() {
    return BrixGCAllocationBufferSize;
}

extern "C" size_t
brixGCRuntimeAllocationBufferHotOffset() {
    return BrixGCRuntimeAllocationBufferHotOffset;
}

extern "C" void
brixGCAllocationBufferFlush() {
    brixFlushGCCompilerAllocationBuffer();
    brixPublishGCThreadAllocationStats();
}

extern "C" void
brixGCAllocationBufferPublish() {
    brixPublishGCCompilerAllocationBufferConsumptions();
}

extern "C" void
brixGCAllocationBufferRevoke() {
    brixFlushGCCompilerAllocationBuffer();
}

static bool
brixGCHandoffBatchToThreadRange(
    const BrixGCAllocationSiteKey& key,
    size_t generation,
    BrixGCRuntimeAllocationBuffer& destination,
    bool& reservedBatch
) {
    if (key.sizeClassIndex >= BrixGCThreadCacheRangeCount) {
        return false;
    }
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    bool destinationMatches = brixGCAllocationRangeMatchesSite(
        destination,
        key.requestedSize,
        key.allocatedSize,
        key.descriptor,
        generation);
    if (!destinationMatches) {
        brixGCTakeParkedGCAllocationRange(cache, key, generation, destination);
    }

    if (destination.buffer.cursor && destination.buffer.limit &&
            destination.buffer.cursor < destination.buffer.limit) {
        reservedBatch = false;
        return true;
    }

    size_t selectedCount = brixGCAdaptiveAllocationBatchCount(destination, key);
    brixGCPrepareCompilerRangeVerificationAllocation(key, selectedCount);
    uint64_t reservedControl = brixGCLoadControlWord();
    BrixGCThreadAllocationBatch batch {};
    if (!brixRefillCachedGCAllocationBatch(
            batch,
            key.requestedSize,
            key.allocatedSize,
            key.descriptor,
            generation,
            selectedCount)) {
        return false;
    }
    brixGCChargeCompilerRangePacerBytes(
        static_cast<size_t>(batch.limit - batch.cursor));
    uint64_t chargedControl = brixGCLoadControlWord();
    if (brixGCControlWordPhase(chargedControl) != brixGCControlWordPhase(reservedControl) ||
            brixGCControlWordSweepGeneration(chargedControl) != generation) {
        if (!brixReleaseCachedGCAllocationBatch(batch)) {
            brixDiscardCachedGCAllocationBatch(batch);
        }
        return false;
    }

    unsigned char adaptivePolicy[sizeof(destination.reserved)] {};
    std::memcpy(adaptivePolicy, destination.reserved, sizeof(adaptivePolicy));
    brixGCInitializeEmptyRuntimeAllocationBuffer(destination);
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
    destination.reserved[kBrixGCAdaptiveEnabledByte] = key.descriptor ? 1 : 0;
    destination.reserved[kBrixGCAdaptiveCountByte] = static_cast<unsigned char>(
        std::min(selectedCount, kBrixGCAdaptiveAllocationBatchMaxCount));
    destination.reserved[kBrixGCAdaptiveReservedSlotCountByte] =
        static_cast<unsigned char>(std::min(
            static_cast<size_t>((batch.limit - batch.cursor) / batch.slotSize),
            kBrixGCAdaptiveAllocationBatchMaxCount));
    cache->counters[kBrixGCThreadCounterCompilerRefills] += 1;
    reservedBatch = true;
    return true;
}

static void
brixGCPrepareCompilerRangeVerificationAllocation(
    const BrixGCAllocationSiteKey& key,
    size_t selectedCount
) {
    // Verification modes retain one callback per logical allocation before the refill because
    // their allocation intervals and replay choices are part of the requested deterministic trace.
    if (brixGCVerificationSlowPathsEnabled()) {
        for (size_t index = 0; index < selectedCount; ++index) {
            brixGCCollectForAllocationIfNeeded(key.allocatedSize);
        }
    }
}

static void
brixGCChargeCompilerRangePacerBytes(size_t deliveredBytes) {
    // Inline bump allocations intentionally perform no collector call. Bill the bytes actually
    // handed to the range at refill frequency without putting work back on the object fast path.
    if (brixGCVerificationSlowPathsEnabled()) {
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
    if (gcCompilerRangePacerPendingBytes >= kBrixGCCompilerRangePacerCreditBytes) {
        size_t chargedBytes = gcCompilerRangePacerPendingBytes;
        gcCompilerRangePacerPendingBytes = 0;
        brixGCCollectForAllocationIfNeeded(chargedBytes);
    }
}

static void
brixGCAddGenericRangePacerBytes(BrixGCHotAllocationState& hot, size_t allocatedBytes) {
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
brixGCChargeGenericRangePacerBytes(BrixGCHotAllocationState& hot, size_t allocatedBytes) {
    // Verification modes define behavior at every logical allocation. Ordinary pacing only
    // needs byte-granular progress no coarser than the existing 64 KiB compiler-range credit.
    if (brixGCVerificationSlowPathsEnabled()) {
        brixGCCollectForAllocationIfNeeded(allocatedBytes);
        return;
    }
    brixGCAddGenericRangePacerBytes(hot, allocatedBytes);
    if (hot.genericPacerPendingBytes >= kBrixGCGenericRangePacerCreditBytes) {
        size_t chargedBytes = hot.genericPacerPendingBytes;
        hot.genericPacerPendingBytes = 0;
        ++hot.genericPacerCollectionChecks;
        brixGCCollectForAllocationIfNeeded(chargedBytes);
    }
}

extern "C" char*
brixGCAllocationBufferRefill(const BrixGCAllocationSiteKey* key) {
    if (!key) {
        return nullptr;
    }

    if (!brixGCActiveTaskCanUseFastAllocation()) {
        brixGCDebugUnstoppableFastAllocationRejected();
        brixFlushGCCompilerAllocationBuffer();
        return brixAllocObjectWithSiteKey(key);
    }

    if (!gcActiveAllocationBufferStorage || !brixGCAllocationSiteKeyIsValid(key)) {
        brixFlushGCCompilerAllocationBuffer();
        return brixAllocObjectWithSiteKey(key);
    }

    brixGCSafepointIfRequested();
    brixEnsureGCInitialized();
    BrixGCRuntimeAllocationBuffer& storage = *gcActiveAllocationBufferStorage;
    brixFlushGCCompilerAllocationBuffer(storage);

    unsigned char phase = brixGCLoadPublicPhase();
    if (!brixGCPhaseAllowsFastBatchAllocation(phase)) {
        brixFlushGCCompilerAllocationBuffer();
        return brixAllocObjectWithSiteKey(key);
    }

    size_t generation = brixGCLoadPublicSweepGeneration();
    bool reservedBatch = false;
    if (!brixGCHandoffBatchToThreadRange(
            *key,
            generation,
            storage,
            reservedBatch)) {
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return brixAllocObjectWithSiteKey(key);
    }

    if (
        generation != brixGCLoadPublicSweepGeneration() ||
        !brixGCPhaseAllowsFastBatchAllocation(brixGCLoadPublicPhase())
    ) {
        if (generation != brixGCLoadPublicSweepGeneration()) {
            brixGCResetAdaptiveAllocationBufferPolicy(storage);
        }
        brixFlushGCCompilerAllocationBuffer(storage);
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return brixAllocObjectWithSiteKey(key);
    }

    if (!storage.buffer.cursor || storage.buffer.cursor >= storage.buffer.limit) {
        brixClearGCCompilerAllocationBuffer(storage);
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return brixAllocObjectWithSiteKey(key);
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
brixGCCompilerAllocationBufferRefill(const BrixGCAllocationSiteKey* key) {
    if (!key) {
        return nullptr;
    }

    if (!brixGCActiveTaskCanUseFastAllocation()) {
        brixGCDebugUnstoppableFastAllocationRejected();
        brixFlushGCCompilerAllocationBuffer();
        return brixAllocObjectWithSiteKey(key);
    }

    if (!brixGCAllocationSiteKeyIsValid(key)) {
        brixFlushGCCompilerAllocationBuffer();
        return brixAllocObjectWithSiteKey(key);
    }

    if (!brixGCCompilerAllocationBufferIndexIsValid(key->compilerBufferIndex)) {
        return brixAllocObjectWithSiteKey(key);
    }

    brixGCSafepointIfRequested();
    brixEnsureGCInitialized();
    size_t slot = key->compilerBufferIndex - 1;
    BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
    BrixGCRuntimeAllocationBuffer& storage = cache->compilerBuffers[slot];
    size_t generation = brixGCLoadPublicSweepGeneration();
    bool sameSite =
        brixGCAllocationRangeMatchesSite(
            storage,
            key->requestedSize,
            key->allocatedSize,
            key->descriptor,
            generation);
    if (!sameSite &&
            (brixGCAllocationRangeHasOwnership(storage) ||
             brixGCAllocationRangeHasPolicySite(storage))) {
        gcCompilerBufferSiteSwitchCount.fetch_add(1, std::memory_order_relaxed);
    }
    if (sameSite) {
        brixFlushGCCompilerAllocationBuffer(storage);
    } else {
        brixGCParkCompilerGCAllocationRange(cache, storage, generation);
    }

    unsigned char phase = brixGCLoadPublicPhase();
    if (!brixGCPhaseAllowsFastBatchAllocation(phase)) {
        brixFlushGCCompilerAllocationBuffer();
        return brixAllocObjectWithSiteKey(key);
    }

    generation = brixGCLoadPublicSweepGeneration();
    if (brixGCAllocationRangeHasPolicySite(storage) &&
            storage.generation != generation) {
        brixGCClearStagedGCAllocationRange(storage);
    }
    bool reservedBatch = false;
    if (!brixGCHandoffBatchToThreadRange(
            *key,
            generation,
            storage,
            reservedBatch)) {
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return brixAllocObjectWithSiteKey(key);
    }
    brixMarkGCCompilerAllocationBufferActive(key->compilerBufferIndex);

    if (
        generation != brixGCLoadPublicSweepGeneration() ||
        !brixGCPhaseAllowsFastBatchAllocation(brixGCLoadPublicPhase())
    ) {
        if (generation != brixGCLoadPublicSweepGeneration()) {
            brixGCResetAdaptiveAllocationBufferPolicy(storage);
        }
        brixFlushGCCompilerAllocationBuffer(storage);
        for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
            if (cache->activeCompilerBufferIndices[active] == slot) {
                cache->activeCompilerBufferIndices[active] =
                    cache->activeCompilerBufferIndices[--cache->activeCompilerBufferCount];
                break;
            }
        }
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return brixAllocObjectWithSiteKey(key);
    }

    if (!storage.buffer.cursor || storage.buffer.cursor >= storage.buffer.limit) {
        brixClearGCCompilerAllocationBuffer(storage);
        for (size_t active = 0; active < cache->activeCompilerBufferCount; ++active) {
            if (cache->activeCompilerBufferIndices[active] == slot) {
                cache->activeCompilerBufferIndices[active] =
                    cache->activeCompilerBufferIndices[--cache->activeCompilerBufferCount];
                break;
            }
        }
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        gcHotAllocationState.stats.allocationFastFallbackCount += 1;
        return brixAllocObjectWithSiteKey(key);
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
brixAllocObject(size_t size) {
    BrixGCHotAllocationState& hot = gcHotAllocationState;
    brixGCRecordPublicAllocationCall(hot, BrixGCAllocationCallObject);
    brixGCSafepointIfRequested();
    brixEnsureGCInitialized();
    brixGCChargeGenericRangePacerBytes(hot, brixGCObjectAllocationSize(size));
    if (char* fast = brixTryFastGCAllocation(hot, size, nullptr)) {
        return fast;
    }
    return brixCallGCFallbackAllocation([&]() {
        return brixGCAllocObject(size);
    });
}

extern "C" char*
brixAllocObjectWithSiteKey(const BrixGCAllocationSiteKey* key) {
    if (!key) {
        return nullptr;
    }

    bool validKey = brixGCAllocationSiteKeyIsValid(key);
    BrixGCHotAllocationState& hot = gcHotAllocationState;
    brixGCRecordPublicAllocationCall(hot, key->descriptor
        ? BrixGCAllocationCallDescribedObject
        : BrixGCAllocationCallObject);
    brixGCSafepointIfRequested();
    brixEnsureGCInitialized();
    brixGCChargeGenericRangePacerBytes(hot, brixGCObjectAllocationSize(key->requestedSize));
    if (validKey) {
        if (char* fast = brixTryFastGCAllocationWithSiteKey(hot, *key)) {
            return fast;
        }
    } else {
        gcHotAllocationState.stats.allocationSiteFallbackCount += 1;
        if (char* fast = brixTryFastGCAllocation(hot, key->requestedSize, key->descriptor)) {
            return fast;
        }
    }

    if (!key->descriptor) {
        return brixCallGCFallbackAllocation([&]() {
            return brixGCAllocObject(key->requestedSize);
        });
    }
    return brixCallGCFallbackAllocation([&]() {
        return brixGCAllocObjectDescribed(key->requestedSize, key->descriptor);
    });
}

extern "C" char*
brixAllocObjectDescribed(size_t size, void* descriptor) {
    BrixGCHotAllocationState& hot = gcHotAllocationState;
    brixGCRecordPublicAllocationCall(hot, BrixGCAllocationCallDescribedObject);
    brixGCSafepointIfRequested();
    brixEnsureGCInitialized();
    brixGCChargeGenericRangePacerBytes(hot, brixGCObjectAllocationSize(size));
    if (!descriptor) {
        if (char* fast = brixTryFastGCAllocation(hot, size, nullptr)) {
            return fast;
        }
        return brixCallGCFallbackAllocation([&]() {
            return brixGCAllocObject(size);
        });
    }
    if (char* fast = brixTryFastGCAllocation(hot, size, descriptor)) {
        return fast;
    }
    return brixCallGCFallbackAllocation([&]() {
        return brixGCAllocObjectDescribed(size, descriptor);
    });
}

extern "C" char*
brixAllocBuffer(size_t itemSize, size_t count) {
    if (brixAllocSizeOverflow(itemSize, count)) {
        return NULL;
    }

    size_t size = count * itemSize;
    if (size == 0) {
        return NULL;
    }

    BrixGCHotAllocationState& hot = gcHotAllocationState;
    brixGCRecordPublicAllocationCall(hot, BrixGCAllocationCallBuffer);
    brixGCSafepointIfRequested();
    brixEnsureGCInitialized();
    brixGCChargeGenericRangePacerBytes(hot, brixGCObjectAllocationSize(size));
    if (char* fast = brixTryFastGCAllocation(hot, size, nullptr)) {
        return fast;
    }
    return brixCallGCFallbackAllocation([&]() {
        return brixGCAllocBuffer(itemSize, count);
    });
}

extern "C" char*
brixTryAllocBufferWithoutSafepoint(size_t size) {
    if (size == 0 || !gcInitialized.load(std::memory_order_acquire) ||
            !brixGCActiveTaskCanUseFastAllocation() ||
            brixGCVerificationSlowPathsEnabled()) {
        return nullptr;
    }
    size_t allocSize = brixGCObjectAllocationSize(size);
    if (allocSize > kBrixGCAllocationBatchMaxSize) {
        return nullptr;
    }
    size_t index = brixGCAllocationBatchIndex(allocSize);
    if (index >= kBrixGCAllocationBatchCount) {
        return nullptr;
    }
    BrixGCHotAllocationState& hot = gcHotAllocationState;
    if (hot.genericPacerPendingBytes + allocSize >= kBrixGCGenericRangePacerCreditBytes) {
        return nullptr;
    }
    uint64_t control = brixGCLoadControlWord();
    if (brixGCControlWordHandshakeRequested(control)) {
        return nullptr;
    }
    BrixGCRuntimeAllocationBuffer& range = brixGCEnsureActiveThreadCache()->ranges[index];
    char* result = range.buffer.cursor;
    if (range.generation != brixGCControlWordSweepGeneration(control) ||
            range.descriptor ||
            range.slotSize < allocSize ||
            !result ||
            !range.buffer.limit ||
            result >= range.buffer.limit) {
        return nullptr;
    }
    brixGCRecordPublicAllocationCall(hot, BrixGCAllocationCallBuffer);
    brixGCRecordAllocationDescriptorMix(hot, index, nullptr);
    brixGCAddGenericRangePacerBytes(hot, allocSize);
    range.buffer.cursor += range.slotSize;
    range.base = range.buffer.cursor;
    hot.stats.zeroedBytes += range.slotSize;
    hot.stats.batchConsumedBytes += range.slotSize;
    hot.batchCacheBytes = hot.batchCacheBytes > range.slotSize
        ? hot.batchCacheBytes - range.slotSize
        : 0;
    brixRecordGCPublicAllocation(hot, size, range.slotSize);
    hot.stats.allocationFastHitCount += 1;
    return result;
}

extern "C" char*
brixAllocBufferDescribed(size_t itemSize, size_t count, void* descriptor) {
    if (brixAllocSizeOverflow(itemSize, count)) {
        return NULL;
    }

    size_t size = count * itemSize;
    if (size == 0) {
        return NULL;
    }

    BrixGCHotAllocationState& hot = gcHotAllocationState;
    brixGCRecordPublicAllocationCall(hot, BrixGCAllocationCallDescribedBuffer);
    brixGCSafepointIfRequested();
    brixEnsureGCInitialized();
    brixGCChargeGenericRangePacerBytes(hot, brixGCObjectAllocationSize(size));
    if (!descriptor) {
        if (char* fast = brixTryFastGCAllocation(hot, size, nullptr)) {
            return fast;
        }
        return brixCallGCFallbackAllocation([&]() {
            return brixGCAllocBuffer(itemSize, count);
        });
    }
    if (char* fast = brixTryFastGCAllocation(hot, size, descriptor)) {
        return fast;
    }
    return brixCallGCFallbackAllocation([&]() {
        return brixGCAllocBufferDescribed(itemSize, count, descriptor);
    });
}

extern "C" char*
brixResizeBuffer(char* buf, size_t oldSize, size_t newSize) {
    return brixResizeBufferDescribed(buf, oldSize, newSize, nullptr);
}

extern "C" char*
brixResizeBufferDescribed(char* buf, size_t oldSize, size_t newSize, void* descriptor) {
    if (buf && brixParseRegionOwns(buf)) {
        return brixParseRegionResizeBuffer(buf, oldSize, newSize, descriptor);
    }
    BrixGCHotAllocationState& hot = gcHotAllocationState;
    brixGCRecordPublicAllocationCall(hot, descriptor
        ? BrixGCAllocationCallDescribedResizeBuffer
        : BrixGCAllocationCallResizeBuffer);
    if (newSize == 0) {
        return NULL;
    }

    brixEnsureGCInitialized();
    BrixGCTemporaryRoot oldBufferRoot((buf && oldSize > 0) ? buf : nullptr);
    brixGCSafepointIfRequested();
    if (buf && oldSize > 0 && newSize <= oldSize && !descriptor) {
        return buf;
    }

    if (!buf || oldSize == 0 || newSize > oldSize) {
        brixGCChargeGenericRangePacerBytes(hot, brixGCObjectAllocationSize(newSize));
    }
    bool copiesOldBuffer = buf && oldSize > 0;
    void* replacementDescriptor = descriptor;
    if (copiesOldBuffer) {
        BrixGCBufferResize resize = brixCallGCIgnoringPanic([&]() {
            return brixGCResizeBufferPrepare(
                buf,
                oldSize,
                newSize,
                descriptor,
                &replacementDescriptor);
        });
        if (resize == BrixGCBufferResizeRejected) {
            return NULL;
        }
        if (resize == BrixGCBufferResizeInPlace) {
            return buf;
        }
    }

    char* replacement = brixTryFastGCAllocation(hot, newSize, replacementDescriptor);
    if (!replacement) {
        replacement = brixCallGCFallbackAllocation([&]() {
            return brixGCAllocBufferDescribed(1, newSize, replacementDescriptor);
        });
    }
    if (replacement && copiesOldBuffer) {
        brixCallGCIgnoringPanic([&]() {
            brixGCResizeBufferCopy(replacement, buf, oldSize, newSize, replacementDescriptor);
        });
    }
    return replacement;
}

extern "C" void*
brixAddRoot(char* pointer) {
    brixEnsureGCInitialized();
    // Root publication is not a collection boundary; collection entry points flush batches.
    return brixCallGCIgnoringPanic([&]() {
        return brixGCAddRoot(pointer);
    });
}

extern "C" void*
brixAddTemporaryRoot(char* pointer) {
    brixEnsureGCInitialized();
    return brixGCPushTemporaryRoot(pointer);
}

extern "C" void*
brixAddDescribedRoot(char* pointer, size_t byteSize, void* descriptor) {
    if (brixGCCollectionDisabled()) {
        return nullptr;
    }
    brixEnsureGCInitialized();
    return brixCallGCIgnoringPanic([&]() {
        return brixGCAddDescribedRoot(pointer, byteSize, descriptor);
    });
}

extern "C" void
brixUpdateRoot(void* root, char* pointer) {
    brixEnsureGCInitialized();
    brixCallGCIgnoringPanic([&]() {
        brixGCUpdateRoot(root, pointer);
    });
}

extern "C" void
brixRemoveRoot(void* root) {
    brixEnsureGCInitialized();
    brixCallGCIgnoringPanic([&]() {
        brixGCRemoveRoot(root);
    });
}

extern "C" void
brixRemoveTemporaryRoot(void* root) {
    brixEnsureGCInitialized();
    brixGCPopTemporaryRoot(root);
}

extern "C" void
brixDrainTemporaryRoots(BrixGCTemporaryRootStorage* storage) {
    brixGCDrainTemporaryRoots(storage);
}

extern "C" BrixGCCollectResult
brixCollect() {
    brixGCSafepointIfRequested();
    if (brixGCCollectionDisabled() || brixGCThreadHasLocalCallbackRegistration()) {
        return BrixGCCollectCompleted;
    }
    brixEnsureGCInitialized();
    brixFlushGCAllocationBatches();
    while (!brixGCTryBeginCollectionWork()) {
        // An automatic worker owns a bounded collector operation. Participate at safepoints
        // and yield until that operation publishes the collector admission token.
        brixGCSafepointIfRequested();
        sched_yield();
    }
    gcCollectionMayGrowRequestedGoal.store(false, std::memory_order_release);
    gcPendingCycleTriggerReason.store(BrixGCPacerTriggerExplicit, std::memory_order_release);
    brixCallGCIgnoringPanic([]() {
        if (brixGCLoadPublicPhase() != kBrixGCPhaseIdle) {
            brixGCDrainAutomaticCycleBlocking();
        } else {
            brixGCCollect();
        }
    });
    brixGCFinishAutomaticGrowthWindowIfIdle();
    brixGCEndCollection();
    return BrixGCCollectCompleted;
}

extern "C" void
brixWriteBarrier(char* source, char* value) {
    if (brixGCIsRuntimeCritical()) {
        return;
    }

    brixEnsureGCInitialized();
    brixCallGCIgnoringPanic([&]() {
        brixGCWriteBarrier(source, value);
    });
}

extern "C" void
brixWriteBarrierRecord(char* source, char* value) {
    if (!source && !value) {
        return;
    }
    if (brixGCIsRuntimeCritical()) {
        return;
    }

    unsigned char phase = brixGCLoadPublicPhase();
    if (brixGCPhaseNeedsMarkBarrier(phase) &&
            brixGCActiveTaskCanUseFastAllocation()) {
        BrixGCThreadCache* cache = brixGCEnsureActiveThreadCache();
#if !BX_ENABLE_BENCH_METRICS
        // Instrumented compiler builds count every attempted barrier before the inline phase
        // check. Ordinary builds have no such call, so count the non-Idle record entry here.
        cache->counters[kBrixGCThreadCounterBarrierCalls] += 1;
#endif
        cache->counters[kBrixGCThreadCounterBarrierSlowEntries] += 1;
        char* current = source ? *reinterpret_cast<char**>(source) : nullptr;
        brixWBBufRecordActiveTaskPointer(current);
        brixWBBufRecordActiveTaskPointer(value);
        return;
    }

    if (!brixGCPhaseNeedsSweepPublicationValidation(phase) &&
            !brixGCPhaseNeedsMarkBarrier(phase)) {
        return;
    }

    // Native, FFI-blocked, and sweep-transition callers cannot leave state in a thread-owned
    // buffer: use synchronous publisher admission and Sweep publication validation.
    brixWriteBarrier(source, value);
}

extern "C" void
brixWriteBarrierFast(char* source, char* value) {
    // This compatibility entry is no longer emitted for ordinary scalar stores; native/direct
    // callers retain exact call accounting without putting a shared RMW on any barrier path.
    BrixGCBarrierStatsRecord* stats = brixGCEnsureThreadBarrierStats();
    brixGCAddOwnedBarrierCounter(stats->fastCalls, 1);
    if (!source && !value) {
        return;
    }

    if (brixGCIsRuntimeCritical()) {
        return;
    }

    unsigned char phase = brixGCLoadPublicPhase();
    if (!brixGCPhaseNeedsMarkBarrier(phase) &&
            !brixGCPhaseNeedsSweepPublicationValidation(phase)) {
        return;
    }

    brixEnsureGCInitialized();
    if (brixGCPhaseNeedsMarkBarrier(phase) &&
            brixGCActiveTaskCanUseFastAllocation()) {
        brixWriteBarrierRecord(source, value);
    } else {
        brixGCAddOwnedBarrierCounter(stats->fastSlowEntries, 1);
        brixCallGCIgnoringPanic([&]() {
            brixGCWriteBarrier(source, value);
        });
    }
}

extern "C" void
brixBulkWriteBarrierFast(char* source, char* value, size_t byteSize, void* descriptor) {
    bool countsEveryCall = false;
#if BX_ENABLE_BENCH_METRICS
    countsEveryCall = brixBenchMetricsEnabledFlag != 0;
    if (__builtin_expect(countsEveryCall, 0)) {
        brixGCAddOwnedBarrierCounter(brixGCEnsureThreadBarrierStats()->fastCalls, 1);
    }
#endif
    unsigned char phase = brixGCLoadPublicPhase();
    if (!brixGCPhaseNeedsMarkBarrier(phase) &&
            !brixGCPhaseNeedsSweepPublicationValidation(phase)) {
        return;
    }

    BrixGCBarrierStatsRecord* stats = brixGCEnsureThreadBarrierStats();
    if (!countsEveryCall) {
        brixGCAddOwnedBarrierCounter(stats->fastCalls, 1);
    }
    if ((!source && !value) || byteSize == 0 || !descriptor) {
        return;
    }

    if (brixGCIsRuntimeCritical()) {
        return;
    }

    brixEnsureGCInitialized();
    brixGCAddOwnedBarrierCounter(stats->fastSlowEntries, 1);
    brixCallGCIgnoringPanic([&]() {
        brixGCBulkWriteBarrier(source, value, byteSize, descriptor);
    });
}

extern "C" size_t
brixGCRuntimeMode() {
    return BX_GC_NEVER_COLLECT ? 1 : 2;
}

#else

namespace {

static std::atomic<size_t> noGCAllocatedObjects {0};
static std::atomic<size_t> noGCAllocatedBytes {0};
static std::atomic<size_t> noGCCollectionObserverCount {0};
static BrixCollectionObserverFilter noGCCollectionObserverFilter;
static std::unordered_map<size_t, std::atomic<uintptr_t>*> noGCCollectionObserverSlots;
static std::mutex noGCObjectSidePayloadMutex;
using BrixNoGCObjectSidePayloads = std::array<char*, BrixObjectSidePayloadLaneCount>;
static std::unordered_map<char*, BrixNoGCObjectSidePayloads> noGCObjectSidePayloads;
static std::array<size_t, BrixObjectSidePayloadLaneCount> noGCObjectSidePayloadCounts {};
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
static constexpr size_t kBrixNoGCBumpChunkSize = 1024ull * 1024ull;
static constexpr size_t kBrixNoGCBumpAlignment = 16;

static inline void
brixNoGCRecordAllocation(size_t size) {
#if BX_ENABLE_BENCH_METRICS
    if (__builtin_expect(brixBenchMetricsEnabledFlag, 0)) {
        brixBenchRecordAllocation(size);
        noGCAllocatedObjects.fetch_add(1, std::memory_order_relaxed);
        noGCAllocatedBytes.fetch_add(size, std::memory_order_relaxed);
    }
#else
    (void)size;
#endif
}

static char*
brixNoGCAllocate(size_t size) {
    brixNoGCRecordAllocation(size);

    size_t storageSize = std::max<size_t>(size, 1);
    if (storageSize > std::numeric_limits<size_t>::max() - (kBrixNoGCBumpAlignment - 1)) {
        std::fprintf(stderr, "Brix no-GC bump allocation size overflow\n");
        std::abort();
    }
    storageSize = (storageSize + kBrixNoGCBumpAlignment - 1) &
        ~(kBrixNoGCBumpAlignment - 1);
    size_t remaining = noGCBumpCursor && noGCBumpLimit
        ? static_cast<size_t>(noGCBumpLimit - noGCBumpCursor)
        : 0;
    if (remaining < storageSize) {
        size_t chunkSize = std::max(kBrixNoGCBumpChunkSize, storageSize);
        noGCBumpCursor = static_cast<char*>(BX_MALLOC(chunkSize));
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
brixNoGCAllocationSize(size_t itemSize, size_t count, size_t& size) {
    if (count != 0 && itemSize > std::numeric_limits<size_t>::max() / count) {
        return false;
    }
    size = itemSize * count;
    return true;
}

} // namespace

extern "C" bool
brixGCAllocationSiteKeyInit(
    BrixGCAllocationSiteKey* key,
    size_t requestedSize,
    void* descriptor,
    size_t pointerKind,
    size_t zeroingPolicy
) {
    if (!key || pointerKind > 1 || zeroingPolicy != 0) {
        return false;
    }
    *key = BrixGCAllocationSiteKey {};
    key->abiVersion = BrixGCAllocationABIVersion;
    key->structSize = sizeof(BrixGCAllocationSiteKey);
    key->requestedSize = requestedSize;
    key->allocatedSize = requestedSize;
    key->descriptor = descriptor;
    key->pointerKind = static_cast<unsigned char>(pointerKind);
    key->zeroingPolicy = static_cast<unsigned char>(zeroingPolicy);
    return true;
}

extern "C" void
brixGCSetActiveThreadCache(BrixGCThreadCache* cache) {
    bool hasOwner = cache != nullptr;
    if (!cache) {
        cache = &gcFallbackThreadCache;
    }
    if (
        cache->abiVersion != BrixGCAllocationABIVersion ||
        cache->structSize != sizeof(BrixGCThreadCache)
    ) {
        *cache = BrixGCThreadCache {};
        brixGCInitializeThreadCache(cache);
    }
    brixGCActiveThreadCache = cache;
    brixGCActiveAllocationBuffer = hasOwner ? &cache->activeBuffer.buffer : nullptr;
}

extern "C" BrixGCThreadCache*
brixGCGetActiveThreadCache() {
    if (!brixGCActiveThreadCache) {
        brixGCSetActiveThreadCache(nullptr);
    }
    return brixGCActiveThreadCache;
}

extern "C" bool
brixGCValidateAllocationABI() {
    return true;
}

extern "C" bool
brixGCTestThreadCacheOwnerFlushOnce() {
    return true;
}

extern "C" void
brixGCSetActiveAllocationBufferStorage(BrixGCRuntimeAllocationBuffer* storage) {
    brixGCActiveAllocationBuffer = storage ? &storage->buffer : nullptr;
}

extern "C" BrixGCAllocationBuffer*
brixGCGetActiveAllocationBuffer() {
    return brixGCActiveAllocationBuffer;
}

extern "C" size_t
brixGCAllocationBufferCursorOffset() {
    return BrixGCAllocationBufferCursorOffset;
}

extern "C" size_t
brixGCAllocationBufferLimitOffset() {
    return BrixGCAllocationBufferLimitOffset;
}

extern "C" size_t
brixGCAllocationBufferSize() {
    return BrixGCAllocationBufferSize;
}

extern "C" size_t
brixGCRuntimeAllocationBufferHotOffset() {
    return BrixGCRuntimeAllocationBufferHotOffset;
}

extern "C" void
brixGCAllocationBufferFlush() {
}

extern "C" void
brixGCAllocationBufferPublish() {
}

extern "C" void
brixGCAllocationBufferRevoke() {
}

extern "C" void
brixGCRuntimeFlushThreadAllocationBatches() {
}

extern "C" void
brixGCRuntimeFlushWriteBarrierBuffer() {
}

extern "C" void
brixGCRuntimeFlushActiveAllocationBuffer() {
}

extern "C" void
brixGCRuntimeRevokeActiveAllocationBuffer() {
}

extern "C" bool
brixGCRuntimeIdleCollect(uint64_t) {
    return false;
}

extern "C" void
brixGCFrameBudget(uint64_t) {
}

extern "C" char*
brixGCAllocationBufferRefill(const BrixGCAllocationSiteKey* key) {
    return key ? brixNoGCAllocate(key->requestedSize) : nullptr;
}

extern "C" char*
brixGCCompilerAllocationBufferRefill(const BrixGCAllocationSiteKey* key) {
    return key ? brixNoGCAllocate(key->requestedSize) : nullptr;
}

extern "C" char*
brixAllocObject(size_t size) {
    return brixNoGCAllocate(size);
}

extern "C" char*
brixAllocObjectWithSiteKey(const BrixGCAllocationSiteKey* key) {
    return key ? brixNoGCAllocate(key->requestedSize) : nullptr;
}

extern "C" char*
brixAllocObjectDescribed(size_t size, void*) {
    return brixNoGCAllocate(size);
}

extern "C" char*
brixAllocBuffer(size_t itemSize, size_t count) {
    size_t size = 0;
    if (!brixNoGCAllocationSize(itemSize, count, size) || size == 0) {
        return nullptr;
    }
    return brixNoGCAllocate(size);
}

extern "C" char*
brixTryAllocBufferWithoutSafepoint(size_t size) {
    return brixAllocBuffer(1, size);
}

extern "C" char*
brixAllocBufferDescribed(size_t itemSize, size_t count, void*) {
    return brixAllocBuffer(itemSize, count);
}

extern "C" char*
brixResizeBuffer(char* buffer, size_t oldSize, size_t newSize) {
    return brixResizeBufferDescribed(buffer, oldSize, newSize, nullptr);
}

extern "C" char*
brixResizeBufferDescribed(char* buffer, size_t oldSize, size_t newSize, void* descriptor) {
    if (buffer && brixParseRegionOwns(buffer)) {
        return brixParseRegionResizeBuffer(buffer, oldSize, newSize, descriptor);
    }
    if (newSize == 0) {
        return nullptr;
    }
    char* resized = brixNoGCAllocate(newSize);
    if (buffer && oldSize) {
        std::memcpy(resized, buffer, std::min(oldSize, newSize));
    }
    return resized;
}

extern "C" void*
brixAddRoot(char*) {
    return nullptr;
}

extern "C" void*
brixAddTemporaryRoot(char*) {
    return nullptr;
}

extern "C" void
brixUpdateRoot(void*, char*) {
}

extern "C" void
brixRemoveRoot(void*) {
}

extern "C" void
brixRemoveTemporaryRoot(void*) {
}

extern "C" void
brixDrainTemporaryRoots(BrixGCTemporaryRootStorage*) {
}

extern "C" BrixGCCollectResult
brixCollect() {
    return BrixGCCollectCompleted;
}

extern "C" void
brixWriteBarrier(char*, char*) {
}

extern "C" void
brixWriteBarrierRecord(char*, char*) {
}

extern "C" void
brixBenchRecordBarrierCall() {
}

extern "C" void
brixWriteBarrierFast(char*, char*) {
}

extern "C" void
brixBulkWriteBarrierFast(char*, char*, size_t, void*) {
}

extern "C" void
brixWBBufRecordPointer(char*) {
}

extern "C" void
brixWBBufFlush() {
}

extern "C" size_t
brixGCFlushWriteBarrierEntries(char* const*, size_t) {
    return 0;
}

extern "C" void
brixGCInit() {
}

extern "C" void
brixGCStartConcurrentMark() {
}

extern "C" size_t
brixGCStepConcurrentMark() {
    return 0;
}

extern "C" void
brixGCFinishConcurrentMark() {
}

extern "C" void
brixGCFinishAutomaticCycle() {
}

extern "C" void
brixGCStepIncrementalSweep(size_t) {
}

extern "C" void
brixGCScavenge() {
}

extern "C" void
brixGCStepScavenge(size_t) {
}

extern "C" size_t
brixGCMutatorAssist(size_t) {
    return 0;
}

extern "C" unsigned char
brixGCCollectorPhase() {
    return 0;
}

extern "C" char*
brixGCAllocObject(size_t size) {
    return brixAllocObject(size);
}

extern "C" char*
brixGCAllocObjectDescribed(size_t size, void* descriptor) {
    return brixAllocObjectDescribed(size, descriptor);
}

extern "C" char*
brixGCAllocBuffer(size_t itemSize, size_t count) {
    return brixAllocBuffer(itemSize, count);
}

extern "C" char*
brixGCAllocBufferDescribed(size_t itemSize, size_t count, void* descriptor) {
    return brixAllocBufferDescribed(itemSize, count, descriptor);
}

extern "C" char*
brixGCResizeBuffer(char* buffer, size_t oldSize, size_t newSize) {
    return brixResizeBuffer(buffer, oldSize, newSize);
}

extern "C" char*
brixGCResizeBufferDescribed(char* buffer, size_t oldSize, size_t newSize, void* descriptor) {
    return brixResizeBufferDescribed(buffer, oldSize, newSize, descriptor);
}

extern "C" bool
brixGCAllocBatchForUnregisteredThread(size_t, void*, size_t, BrixGCAllocationBatch*) {
    return false;
}

extern "C" bool
brixGCTestAllocBatch(size_t size, void* descriptor, size_t count, BrixGCAllocationBatch* batch) {
    return brixGCAllocBatchForUnregisteredThread(size, descriptor, count, batch);
}

extern "C" bool
brixGCAllocBatchForRunningTask(size_t, void*, size_t, BrixGCAllocationBatch*) {
    return false;
}

extern "C" bool
brixGCReleaseUnusedBatch(char*, char*, size_t, size_t) {
    return false;
}

extern "C" bool
brixGCDiscardUnusedBatch(char*, char*, size_t, size_t) {
    return false;
}

extern "C" bool
brixGCTestDiscardBatch(char* cursor, char* limit, size_t slotSize, size_t generation) {
    return brixGCDiscardUnusedBatch(cursor, limit, slotSize, generation);
}

extern "C" void*
brixGCAddRoot(char*) {
    return nullptr;
}

extern "C" void*
brixGCAddTemporaryRoot(char*) {
    return nullptr;
}

extern "C" void*
brixGCAddDescribedRoot(char*, size_t, void*) {
    return nullptr;
}

extern "C" void*
brixAddDescribedRoot(char*, size_t, void*) {
    return nullptr;
}

extern "C" void
brixGCUpdateRoot(void*, char*) {
}

extern "C" void
brixGCRemoveRoot(void*) {
}

extern "C" void
brixGCRemoveTemporaryRoot(void*) {
}

extern "C" void
brixGCDrainTemporaryRoots(BrixGCTemporaryRootStorage*) {
}

extern "C" void
brixGCCollect() {
}

extern "C" void
brixGCCollectForAllocation() {
}

extern "C" bool
brixGCVerifyObject(char* pointer) {
    return pointer != nullptr;
}

extern "C" void*
brixGCObjectDescriptor(char*) {
    return nullptr;
}

extern "C" void
brixGCWriteBarrier(char*, char*) {
}

extern "C" void
brixGCBulkWriteBarrier(char*, char*, size_t, void*) {
}

extern "C" void
brixGCSetHeapGoal(size_t) {
}

extern "C" void
brixGCSetMinimumHeapGoal(size_t) {
}

extern "C" size_t
brixGCRegisterOwnerWeak(char* owner, char* target) {
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
brixGCOwnerWeakLoad(size_t handle) {
    std::lock_guard<std::mutex> lock(noGCWeakMutex);
    auto found = noGCOwnerWeakTargets.find(handle);
    return found == noGCOwnerWeakTargets.end() ? nullptr : found->second;
}

extern "C" bool
brixGCReleaseOwnerWeak(size_t handle) {
    std::lock_guard<std::mutex> lock(noGCWeakMutex);
    return noGCOwnerWeakTargets.erase(handle) != 0;
}

extern "C" size_t
brixGCRegisterCleanup(char* target, char* context, BrixGCCleanupCallback callback) {
    if (!target || !context || !callback) {
        return 0;
    }
    size_t handle = noGCNextCleanupHandle.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(noGCCleanupMutex);
    noGCCleanupHandles.insert(handle);
    return handle;
}

extern "C" size_t
brixGCRegisterNativeCleanup(char* target, void* context, BrixGCCleanupCallback callback) {
    return brixGCRegisterCleanup(target, static_cast<char*>(context), callback);
}

extern "C" bool
brixGCCancelCleanup(size_t handle) {
    std::lock_guard<std::mutex> lock(noGCCleanupMutex);
    if (!noGCCleanupHandles.erase(handle)) {
        return false;
    }
    noGCCleanupCancelCount.fetch_add(1, std::memory_order_relaxed);
    return true;
}

extern "C" size_t
brixGCDrainCleanups() {
    return 0;
}

extern "C" void
brixGCQueueFileDescriptorCleanup(int) {
}

extern "C" void
brixGCReportCleanupException() {
    noGCCleanupExceptionCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" char*
brixGCObjectSidePayload(char* object, size_t lane) {
    if (!object || lane >= BrixObjectSidePayloadLaneCount) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(noGCObjectSidePayloadMutex);
    auto found = noGCObjectSidePayloads.find(object);
    return found == noGCObjectSidePayloads.end() ? nullptr : found->second[lane];
}

extern "C" char*
brixGCPublishObjectSidePayload(char* object, size_t lane, char* payload) {
    if (!object || !payload || lane >= BrixObjectSidePayloadLaneCount) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(noGCObjectSidePayloadMutex);
    auto result = noGCObjectSidePayloads.emplace(object, BrixNoGCObjectSidePayloads {});
    char*& published = result.first->second[lane];
    if (!published) {
        published = payload;
        ++noGCObjectSidePayloadCounts[lane];
    }
    return published;
}

extern "C" char*
brixGCPublishCleanupObjectSidePayload(
    char* object,
    size_t lane,
    char* payload,
    BrixGCCleanupCallback callback
) {
    if (!callback) {
        return nullptr;
    }
    // No-GC builds never discover dead owners; explicit detach still clears the same side slot.
    return brixGCPublishObjectSidePayload(object, lane, payload);
}

extern "C" void
brixGCClearObjectSidePayload(char* object, size_t lane) {
    if (!object || lane >= BrixObjectSidePayloadLaneCount) {
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
brixGCClearCleanupObjectSidePayload(char* object, size_t lane) {
    if (!object || lane >= BrixObjectSidePayloadLaneCount) {
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
brixGCHasObjectSidePayload(char* object, size_t lane) {
    return brixGCObjectSidePayload(object, lane) != nullptr;
}

extern "C" size_t
brixGCObjectSidePayloadCount(size_t lane) {
    if (lane >= BrixObjectSidePayloadLaneCount) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(noGCObjectSidePayloadMutex);
    return noGCObjectSidePayloadCounts[lane];
}

extern "C" size_t
brixGCCollectionObserversSidePayloadLane() {
    return BrixObjectSidePayloadCollectionObservers;
}

extern "C" size_t
brixGCMarkupAttachmentsSidePayloadLane() {
    return BrixObjectSidePayloadMarkupAttachments;
}

extern "C" size_t
brixGCStyleAttachmentsSidePayloadLane() {
    return BrixObjectSidePayloadStyleAttachments;
}

extern "C" size_t
brixGCStyleTemplatesSidePayloadLane() {
    return BrixObjectSidePayloadStyleTemplates;
}

extern "C" size_t
brixGCStyleObjectsSidePayloadLane() {
    return BrixObjectSidePayloadStyleObjects;
}

extern "C" size_t
brixGCEventsSidePayloadLane() {
    return BrixObjectSidePayloadEvents;
}

extern "C" size_t
brixCollectionObserverActivate(char* owner) {
    if (!owner) {
        return 0;
    }
    std::atomic<uintptr_t>& slot = noGCCollectionObserverFilter.slot(owner);
    if (!BrixCollectionObserverFilter::retain(slot, owner)) {
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
brixCollectionObserverDeactivate(size_t cleanup) {
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
    if (!brixGCCancelCleanup(cleanup)) {
        return false;
    }
    if (!BrixCollectionObserverFilter::release(*slot)) {
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
brixCollectionMayBeObserved(char* collection) {
    return noGCCollectionObserverFilter.mayBeObserved(collection);
}

extern "C" size_t
brixCollectionObserverActiveCount() {
    return noGCCollectionObserverCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCAllocatedObjectCount() {
    return noGCAllocatedObjects.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCRequestedByteCount() {
    return noGCAllocatedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCAllocatedByteCount() {
    return noGCAllocatedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCLiveByteCount() {
    return noGCAllocatedBytes.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCHeapGoal() {
    return std::numeric_limits<size_t>::max();
}

extern "C" size_t
brixGCBackgroundWorkerCount() {
    return 0;
}

extern "C" size_t
brixGCBackgroundWorkerThreadCapacity() {
    return 0;
}

extern "C" size_t
brixGCBackgroundWorkerDutyPermille() {
    return 0;
}

extern "C" size_t
brixGCBackgroundWorkerWork() {
    return 0;
}

extern "C" size_t
brixGCBackgroundCredit() {
    return 0;
}

extern "C" size_t
brixGCFrameBudgetOfferCount() {
    return 0;
}

extern "C" size_t
brixGCFrameBudgetWorkCount() {
    return 0;
}

extern "C" size_t
brixGCFrameBudgetWorkNanos() {
    return 0;
}

extern "C" size_t
brixGCFrameBudgetOverrunCount() {
    return 0;
}

extern "C" size_t
brixGCCollectionCount() {
    return 0;
}

extern "C" size_t
brixGCWeakClearCount() {
    return 0;
}

extern "C" size_t
brixGCCleanupRegistrationCount() {
    std::lock_guard<std::mutex> lock(noGCCleanupMutex);
    return noGCCleanupHandles.size();
}

extern "C" size_t
brixGCCleanupPendingCount() {
    return 0;
}

extern "C" size_t
brixGCCleanupEnqueueCount() {
    return 0;
}

extern "C" size_t
brixGCCleanupFireCount() {
    return 0;
}

extern "C" size_t
brixGCCleanupCancelCount() {
    return noGCCleanupCancelCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCCleanupExceptionCount() {
    return noGCCleanupExceptionCount.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCBarrierCount() {
    return 0;
}

extern "C" size_t
brixGCMarkParticipantCount() {
    return 0;
}

extern "C" size_t
brixGCActiveMarkerCount() {
    return 0;
}

extern "C" uint64_t
brixGCWorkFlushedEpoch() {
    return 0;
}

extern "C" size_t
brixGCTerminationConfirmCount() {
    return 0;
}

extern "C" size_t
brixGCTerminationConfirmResumeCount() {
    return 0;
}

extern "C" size_t
brixGCAssistCount() {
    return 0;
}

extern "C" size_t
brixGCAssistTimeNanos() {
    return 0;
}

extern "C" size_t
brixGCMarkTimeNanos() {
    return 0;
}

extern "C" size_t
brixGCSweepTimeNanos() {
    return 0;
}

extern "C" size_t
brixGCPauseCount() {
    return 0;
}

extern "C" size_t
brixGCRuntimeMode() {
    return 0;
}

#endif
