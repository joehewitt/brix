#include "BxCore/BxCore.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <limits>
#include <string>
#include <sys/resource.h>
#include <time.h>
#include <vector>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <malloc/malloc.h>
#endif

static constexpr size_t kBenchSmallSizeBinCount = 128;
static constexpr size_t kBenchLargeSizeBinSize = 1024;
static constexpr size_t kBenchLargeSizeBinCount = 64;
static constexpr size_t kBenchSizeBinCount =
    kBenchSmallSizeBinCount + kBenchLargeSizeBinCount + 1;
static constexpr size_t kBenchOverflowSizeBin = kBenchSizeBinCount - 1;
static constexpr size_t kGCPageSize = 8192;
static constexpr size_t kGCPauseMarkSetupTotal = 1;
static constexpr size_t kGCPauseTermConfirm = 3;
static constexpr size_t kGCPauseSweepTotal = 4;
static constexpr size_t kBenchAllocationSiteCapacity = 4096;
static constexpr size_t kBenchAllocationSiteClaim = std::numeric_limits<size_t>::max();
static constexpr size_t kBenchMutatorPhaseCapacity = 16;

struct BrixBenchPauseTraceSummary {
    size_t totalEvents;
    size_t storedEvents;
    size_t capacity;
    size_t summaryEvents;
    size_t tasksWaited;
    size_t slowestTaskNanos;
};

struct BrixBenchSizeClassSummary {
    size_t total;
    bool available;
};

struct BrixBenchAllocationSiteEntry {
    std::atomic<size_t> siteId {0};
    std::atomic<size_t> count {0};
    const char* source = nullptr;
    size_t line = 0;
    size_t column = 0;
    const char* function = nullptr;
    const char* type = nullptr;
    const char* operation = nullptr;
    const char* pointerKind = nullptr;
};

struct BrixBenchAllocationSiteSummary {
    size_t operationCount;
    size_t supplementalOperationCount;
    size_t entryCount;
    size_t tableOverflowCount;
    size_t hashCollisionCount;
    size_t probeCollisionCount;
};

struct BrixBenchMutatorPhaseEntry {
    const char* name = nullptr;
    size_t startBarrierCalls = 0;
    size_t startBarrierSlowEntries = 0;
    size_t startAllocationRefills = 0;
    size_t startAssists = 0;
    size_t barrierCalls = 0;
    size_t barrierSlowEntries = 0;
    size_t allocationRefills = 0;
    size_t assists = 0;
    bool completed = false;
};

static std::atomic<size_t> bumpAllocatedObjects {0};
static std::atomic<size_t> bumpAllocatedBytes {0};
static std::array<std::atomic<size_t>, kBenchSizeBinCount> bumpSizeBins {};
static std::array<BrixBenchAllocationSiteEntry, kBenchAllocationSiteCapacity>
    benchmarkAllocationSites {};
static std::atomic<size_t> benchmarkAllocationSiteTableOverflowCount {0};
static std::atomic<size_t> benchmarkAllocationSiteHashCollisionCount {0};
static std::atomic<size_t> benchmarkAllocationSiteProbeCollisionCount {0};
static std::array<BrixBenchMutatorPhaseEntry, kBenchMutatorPhaseCapacity>
    benchmarkMutatorPhases {};
static std::atomic<size_t> benchmarkMutatorPhaseCount {0};
static std::atomic<size_t> benchmarkMutatorPhaseErrorCount {0};
static std::atomic<size_t> benchmarkTemporaryRootShimPushCount {0};
static struct timespec benchmarkStartTime {};

typedef size_t (*BrixBenchCounterFn)(void);
typedef size_t (*BrixBenchIndexedCounterFn)(size_t);
typedef void (*BrixBenchVoidFn)(void);

extern "C" unsigned char brixBenchMetricsEnabledFlag = 0;

template <typename Function>
static Function
brixBenchFunction(const char* name) {
    return reinterpret_cast<Function>(dlsym(RTLD_DEFAULT, name));
}

static size_t
brixBenchCounter(const char* name) {
    BrixBenchCounterFn counter = brixBenchFunction<BrixBenchCounterFn>(name);
    return counter ? counter() : 0;
}

static size_t
brixBenchIndexedCounter(const char* name, size_t index) {
    BrixBenchIndexedCounterFn counter =
        brixBenchFunction<BrixBenchIndexedCounterFn>(name);
    return counter ? counter(index) : 0;
}

static size_t
brixBenchMonotonicNanos() {
    struct timespec now {};
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }
    size_t seconds = static_cast<size_t>(now.tv_sec - benchmarkStartTime.tv_sec);
    long nanos = now.tv_nsec - benchmarkStartTime.tv_nsec;
    if (nanos < 0) {
        --seconds;
        nanos += 1000000000L;
    }
    return seconds * 1000000000ull + static_cast<size_t>(nanos);
}

static size_t
brixBenchSizeBin(size_t size) {
    if (size > 0 && size <= 1024) {
        return (size - 1) / 8;
    }
    if (size > 1024 && size <= kBenchLargeSizeBinCount * kBenchLargeSizeBinSize) {
        return kBenchSmallSizeBinCount + (size - 1025) / kBenchLargeSizeBinSize;
    }
    return kBenchOverflowSizeBin;
}

static size_t
brixBenchSizeBinUpperBound(size_t index) {
    if (index < kBenchSmallSizeBinCount) {
        return (index + 1) * 8;
    }
    if (index < kBenchOverflowSizeBin) {
        return 1024 +
            (index - kBenchSmallSizeBinCount + 1) * kBenchLargeSizeBinSize;
    }
    return static_cast<size_t>(-1);
}

static size_t
brixBenchPercentile(std::vector<size_t> values, size_t percentile) {
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    return values[((values.size() - 1) * percentile) / 100];
}

static bool
brixBenchStringsEqual(const char* left, const char* right) {
    return left && right && std::strcmp(left, right) == 0;
}

static void
brixBenchWriteJSONString(FILE* file, const char* value) {
    std::fputc('"', file);
    for (const unsigned char* cursor =
             reinterpret_cast<const unsigned char*>(value ? value : "");
         *cursor;
         ++cursor) {
        switch (*cursor) {
            case '"': std::fputs("\\\"", file); break;
            case '\\': std::fputs("\\\\", file); break;
            case '\b': std::fputs("\\b", file); break;
            case '\f': std::fputs("\\f", file); break;
            case '\n': std::fputs("\\n", file); break;
            case '\r': std::fputs("\\r", file); break;
            case '\t': std::fputs("\\t", file); break;
            default:
                if (*cursor < 0x20) {
                    std::fprintf(file, "\\u%04x", static_cast<unsigned>(*cursor));
                } else {
                    std::fputc(*cursor, file);
                }
                break;
        }
    }
    std::fputc('"', file);
}

static size_t
brixBenchWriteSizeHistogram(FILE* file, bool hasGC) {
    std::fprintf(file, "\"allocationSizeHistogramBytes\":[");
    bool first = true;
    size_t total = 0;
    size_t binCount = hasGC
        ? brixBenchCounter("brixGCRuntimeBenchAllocationSizeBinCount")
        : kBenchSizeBinCount;
    for (size_t index = 0; index < binCount; ++index) {
        size_t count = hasGC
            ? brixBenchIndexedCounter(
                "brixGCRuntimeBenchAllocationSizeBinValue",
                index)
            : bumpSizeBins[index].load(std::memory_order_relaxed);
        if (!count) {
            continue;
        }
        size_t bound = hasGC
            ? brixBenchIndexedCounter(
                "brixGCRuntimeBenchAllocationSizeBinUpperBound",
                index)
            : brixBenchSizeBinUpperBound(index);
        std::fprintf(file, "%s{", first ? "" : ",");
        if (bound == static_cast<size_t>(-1)) {
            std::fprintf(file, "\"upperBoundBytes\":null,\"overflow\":true,");
        } else {
            std::fprintf(file, "\"upperBoundBytes\":%zu,", bound);
        }
        std::fprintf(file, "\"count\":%zu}", count);
        total += count;
        first = false;
    }
    std::fprintf(file, "]");
    return total;
}

static BrixBenchSizeClassSummary
brixBenchWriteSizeClassHistogram(FILE* file, bool hasGC) {
    if (!hasGC) {
        std::fprintf(
            file,
            "\"allocationSizeClassHistogramBytes\":null,"
            "\"allocationSizeClassHistogramInapplicableReason\":"
            "\"historical no-GC bump allocation has no GC size classes\"");
        return {0, true};
    }

    BrixBenchCounterFn countFn = brixBenchFunction<BrixBenchCounterFn>(
        "brixGCRuntimeBenchAllocatedSizeClassCount");
    BrixBenchIndexedCounterFn upperBoundFn =
        brixBenchFunction<BrixBenchIndexedCounterFn>(
            "brixGCRuntimeBenchAllocatedSizeClassUpperBound");
    BrixBenchIndexedCounterFn valueFn =
        brixBenchFunction<BrixBenchIndexedCounterFn>(
            "brixGCRuntimeBenchAllocatedSizeClassValue");
    const bool available = countFn && upperBoundFn && valueFn && countFn() > 0;
    const size_t count = available ? countFn() : 0;
    size_t total = 0;
    bool first = true;
    std::fprintf(file, "\"allocationSizeClassHistogramBytes\":[");
    for (size_t index = 0; index < count; ++index) {
        const size_t value = valueFn(index);
        if (!value) {
            continue;
        }
        const size_t upperBound = upperBoundFn(index);
        std::fprintf(
            file,
            "%s{\"sizeClassIndex\":%zu,",
            first ? "" : ",",
            index);
        if (upperBound == std::numeric_limits<size_t>::max()) {
            std::fprintf(file, "\"upperBoundBytes\":null,\"large\":true,");
        } else {
            std::fprintf(file, "\"upperBoundBytes\":%zu,", upperBound);
        }
        std::fprintf(file, "\"count\":%zu}", value);
        total += value;
        first = false;
    }
    std::fprintf(file, "]");
    return {total, available};
}

static bool
brixBenchWriteLifetimeHistogram(FILE* file, const char* mode) {
    BrixBenchCounterFn binCount =
        brixBenchFunction<BrixBenchCounterFn>("brixGCBenchLifetimeBinCount");
    BrixBenchIndexedCounterFn upperBound =
        brixBenchFunction<BrixBenchIndexedCounterFn>("brixGCBenchLifetimeBinUpperBound");
    BrixBenchIndexedCounterFn value =
        brixBenchFunction<BrixBenchIndexedCounterFn>("brixGCBenchLifetimeBinValue");
    BrixBenchCounterFn sampleEvery =
        brixBenchFunction<BrixBenchCounterFn>("brixGCBenchLifetimeSampleEvery");
    BrixBenchCounterFn sampledReservations =
        brixBenchFunction<BrixBenchCounterFn>("brixGCBenchLifetimeSampledReservationCount");
    BrixBenchCounterFn clearedReservations =
        brixBenchFunction<BrixBenchCounterFn>("brixGCBenchLifetimeClearedReservationCount");
    BrixBenchCounterFn reclaimed =
        brixBenchFunction<BrixBenchCounterFn>("brixGCBenchLifetimeReclaimedCount");
    BrixBenchCounterFn outstanding =
        brixBenchFunction<BrixBenchCounterFn>("brixGCBenchLifetimeOutstandingCount");
    BrixBenchIndexedCounterFn censored = brixBenchFunction<BrixBenchIndexedCounterFn>(
        "brixGCBenchLifetimeCensoredBinValue");
    if (!binCount || !upperBound || !value || !sampleEvery) {
        std::fprintf(
            file,
            "\"allocationLifetimeHistogramCycles\":null,"
            "\"allocationLifetimeInapplicableReason\":\"%s\"",
            mode && std::string(mode) == "no-gc"
                ? "no collection exists in the no-GC variant"
                : "allocation lifetime sampling is unavailable");
        return mode && std::string(mode) == "no-gc";
    }

    std::fprintf(
        file,
        "\"allocationLifetimeSampleEveryReservedSlots\":%zu,"
        "\"allocationLifetimeSampledReservations\":%zu,"
        "\"allocationLifetimeClearedReservations\":%zu,"
        "\"allocationLifetimeReclaimedSamples\":%zu,"
        "\"allocationLifetimeOutstandingSamples\":%zu,"
        "\"allocationLifetimeHistogramCycles\":[",
        sampleEvery(),
        sampledReservations ? sampledReservations() : 0,
        clearedReservations ? clearedReservations() : 0,
        reclaimed ? reclaimed() : 0,
        outstanding ? outstanding() : 0);
    size_t count = binCount();
    for (size_t index = 0; index < count; ++index) {
        size_t bound = upperBound(index);
        std::fprintf(file, "%s{", index ? "," : "");
        if (bound == static_cast<size_t>(-1)) {
            std::fprintf(file, "\"upperBoundCycles\":null,\"overflow\":true,");
        } else {
            std::fprintf(file, "\"upperBoundCycles\":%zu,", bound);
        }
        std::fprintf(file, "\"sampledCount\":%zu}", value(index));
    }
    std::fprintf(file, "],\"allocationLifetimeCensoredHistogramCycles\":[");
    for (size_t index = 0; index < count; ++index) {
        size_t bound = upperBound(index);
        std::fprintf(file, "%s{", index ? "," : "");
        if (bound == static_cast<size_t>(-1)) {
            std::fprintf(file, "\"upperBoundCycles\":null,\"overflow\":true,");
        } else {
            std::fprintf(file, "\"upperBoundCycles\":%zu,", bound);
        }
        std::fprintf(
            file,
            "\"sampledCount\":%zu}",
            censored ? censored(index) : 0);
    }
    std::fprintf(file, "]");
    size_t sampled = sampledReservations ? sampledReservations() : 0;
    size_t cleared = clearedReservations ? clearedReservations() : 0;
    size_t reclaimedCount = reclaimed ? reclaimed() : 0;
    size_t outstandingCount = outstanding ? outstanding() : 0;
    bool complete = sampled == cleared + reclaimedCount + outstandingCount;
    std::fprintf(
        file,
        ",\"allocationLifetimeInvariantValid\":%s",
        complete ? "true" : "false");
    return complete;
}

static BrixBenchPauseTraceSummary
brixBenchWritePauseTrace(FILE* file, std::vector<size_t>& durations) {
    BrixBenchCounterFn countFn =
        brixBenchFunction<BrixBenchCounterFn>("brixGCRuntimePauseEventCount");
    const size_t count = countFn ? countFn() : 0;
    BrixBenchPauseTraceSummary summary {
        brixBenchCounter("brixGCRuntimePauseCount"),
        count,
        brixBenchCounter("brixGCRuntimePauseEventCapacity"),
        0,
        0,
        0
    };
    std::fprintf(file, "\"gcPauseTrace\":[");
    for (size_t index = 0; index < count; ++index) {
        size_t kind = brixBenchIndexedCounter("brixGCRuntimePauseEventKind", index);
        size_t duration =
            brixBenchIndexedCounter("brixGCRuntimePauseEventDurationNanos", index);
        size_t tasksWaited =
            brixBenchIndexedCounter("brixGCRuntimePauseEventTasksWaited", index);
        size_t slowestTaskNanos =
            brixBenchIndexedCounter("brixGCRuntimePauseEventSlowestTaskNanos", index);
        summary.tasksWaited += tasksWaited;
        summary.slowestTaskNanos = std::max(summary.slowestTaskNanos, slowestTaskNanos);
        if (
            kind == kGCPauseMarkSetupTotal ||
            kind == kGCPauseTermConfirm ||
            kind == kGCPauseSweepTotal
        ) {
            durations.push_back(duration);
            ++summary.summaryEvents;
        }
        std::fprintf(
            file,
            "%s{\"cycleId\":%zu,\"kind\":%zu,\"startNanos\":%zu,"
            "\"durationNanos\":%zu,\"tasksWaited\":%zu,"
            "\"slowestTaskNanos\":%zu}",
            index ? "," : "",
            brixBenchIndexedCounter("brixGCRuntimePauseEventCycleId", index),
            kind,
            brixBenchIndexedCounter("brixGCRuntimePauseEventStartNanos", index),
            duration,
            tasksWaited,
            slowestTaskNanos);
    }
    std::fprintf(file, "]");
    return summary;
}

static void
brixBenchWriteCycleSummaries(FILE* file) {
    BrixBenchCounterFn countFn =
        brixBenchFunction<BrixBenchCounterFn>("brixGCRuntimeCycleSummaryCount");
    const size_t count = countFn ? countFn() : 0;
    std::fprintf(file, "\"gcCycleSummaries\":[");
    for (size_t index = 0; index < count; ++index) {
        std::fprintf(
            file,
            "%s{\"cycleId\":%zu,\"triggerReason\":%zu,\"heapAtTrigger\":%zu,"
            "\"heapGoal\":%zu,\"heapActual\":%zu,\"markDurationNanos\":%zu,"
            "\"scanWork\":%zu,\"assistWork\":%zu,\"backgroundWork\":%zu,"
            "\"sweepPages\":%zu,\"scavengedPages\":%zu}",
            index ? "," : "",
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryCycleId", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryTriggerReason", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryHeapAtTrigger", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryHeapGoal", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryHeapActual", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryMarkDurationNanos", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryScanWork", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryAssistWork", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryBackgroundWork", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummarySweepPages", index),
            brixBenchIndexedCounter("brixGCRuntimeCycleSummaryScavengedPages", index));
    }
    std::fprintf(file, "]");
}

static size_t
brixBenchWriteAllocationBuffers(FILE* file, size_t runtimeNanos) {
    BrixBenchCounterFn countFn = brixBenchFunction<BrixBenchCounterFn>(
        "brixGCRuntimeBenchAllocationSiteCount");
    BrixBenchIndexedCounterFn valueFn = brixBenchFunction<BrixBenchIndexedCounterFn>(
        "brixGCRuntimeBenchAllocationSiteValue");
    if (!countFn || !valueFn) {
        std::fprintf(
            file,
            "\"allocationBufferRates\":null,"
            "\"allocationBufferInapplicableReason\":"
            "\"historical no-GC codegen has no GC allocation buffers\"");
        return 0;
    }
    std::fprintf(file, "\"allocationBufferRates\":[");
    bool first = true;
    size_t total = 0;
    for (size_t index = 0; index < countFn(); ++index) {
        size_t count = valueFn(index);
        if (!count) {
            continue;
        }
        double rate = runtimeNanos
            ? static_cast<double>(count) * 1000000000.0 /
                static_cast<double>(runtimeNanos)
            : 0.0;
        std::fprintf(
            file,
            "%s{\"bufferId\":%zu,\"count\":%zu,\"allocationsPerSecond\":%.6f}",
            first ? "" : ",",
            index,
            count,
            rate);
        total += count;
        first = false;
    }
    std::fprintf(file, "]");
    return total;
}

static void
brixBenchWriteThreadRefills(FILE* file) {
    BrixBenchCounterFn countFn =
        brixBenchFunction<BrixBenchCounterFn>("brixGCRuntimeBenchThreadRefillCount");
    BrixBenchIndexedCounterFn valueFn =
        brixBenchFunction<BrixBenchIndexedCounterFn>("brixGCRuntimeBenchThreadRefillValue");
    std::fprintf(file, "\"allocationRefillsByThread\":[");
    const size_t count = countFn && valueFn ? countFn() : 0;
    for (size_t index = 0; index < count; ++index) {
        std::fprintf(
            file,
            "%s{\"threadOrdinal\":%zu,\"refillCount\":%zu}",
            index ? "," : "",
            index,
            valueFn(index));
    }
    std::fprintf(file, "]");
}

static BrixBenchAllocationSiteSummary
brixBenchWriteAllocationSites(FILE* file, size_t runtimeNanos) {
    BrixBenchAllocationSiteSummary summary {
        0,
        0,
        0,
        benchmarkAllocationSiteTableOverflowCount.load(std::memory_order_relaxed),
        benchmarkAllocationSiteHashCollisionCount.load(std::memory_order_relaxed),
        benchmarkAllocationSiteProbeCollisionCount.load(std::memory_order_relaxed)
    };
    std::fprintf(file, "\"allocationSiteRates\":[");
    bool first = true;
    for (const auto& entry : benchmarkAllocationSites) {
        size_t siteId = entry.siteId.load(std::memory_order_acquire);
        if (!siteId || siteId == kBenchAllocationSiteClaim) {
            continue;
        }
        size_t count = entry.count.load(std::memory_order_relaxed);
        bool supplemental = brixBenchStringsEqual(entry.pointerKind, "supplemental");
        double rate = runtimeNanos
            ? static_cast<double>(count) * 1000000000.0 /
                static_cast<double>(runtimeNanos)
            : 0.0;
        std::fprintf(file, "%s{\"siteId\":\"%016zx\",\"source\":", first ? "" : ",", siteId);
        brixBenchWriteJSONString(file, entry.source);
        std::fprintf(file, ",\"line\":%zu,\"column\":%zu,\"function\":", entry.line, entry.column);
        brixBenchWriteJSONString(file, entry.function);
        std::fprintf(file, ",\"type\":");
        brixBenchWriteJSONString(file, entry.type);
        std::fprintf(file, ",\"operation\":");
        brixBenchWriteJSONString(file, entry.operation);
        std::fprintf(file, ",\"pointerKind\":");
        brixBenchWriteJSONString(file, entry.pointerKind);
        std::fprintf(
            file,
            ",\"supplemental\":%s,\"count\":%zu,\"operationsPerSecond\":%.6f}",
            supplemental ? "true" : "false",
            count,
            rate);
        if (supplemental) {
            summary.supplementalOperationCount += count;
        } else {
            summary.operationCount += count;
        }
        ++summary.entryCount;
        first = false;
    }
    std::fprintf(file, "]");
    return summary;
}

struct BrixBenchProcessMemory {
    size_t footprintBytes = 0;
    size_t footprintPeakBytes = 0;
    size_t gpuBytes = 0;
    size_t surfaceBytes = 0;
    size_t mallocInUseBytes = 0;
    size_t mallocAllocatedBytes = 0;
};

static BrixBenchProcessMemory
brixBenchProcessMemory() {
    BrixBenchProcessMemory memory;
#if defined(__APPLE__)
    task_vm_info_data_t info {};
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(
            mach_task_self(),
            TASK_VM_INFO,
            reinterpret_cast<task_info_t>(&info),
            &count) == KERN_SUCCESS) {
        memory.footprintBytes = static_cast<size_t>(info.phys_footprint);
        if (count >= TASK_VM_INFO_REV3_COUNT) {
            memory.footprintPeakBytes = static_cast<size_t>(info.ledger_phys_footprint_peak);
        }
    }
    vm_address_t address = 0;
    natural_t depth = 0;
    while (true) {
        vm_size_t size = 0;
        vm_region_submap_info_data_64_t region {};
        mach_msg_type_number_t regionCount = VM_REGION_SUBMAP_INFO_COUNT_64;
        if (vm_region_recurse_64(
                mach_task_self(),
                &address,
                &size,
                &depth,
                reinterpret_cast<vm_region_recurse_info_64_t>(&region),
                &regionCount) != KERN_SUCCESS) {
            break;
        }
        if (region.is_submap) {
            ++depth;
            continue;
        }
        if (region.user_tag == VM_MEMORY_IOACCELERATOR ||
                region.user_tag == VM_MEMORY_IOSURFACE) {
            int state = 0;
            bool reclaimable = vm_purgable_control(
                    mach_task_self(),
                    address,
                    VM_PURGABLE_GET_STATE,
                    &state) == KERN_SUCCESS &&
                ((state & VM_PURGABLE_STATE_MASK) == VM_PURGABLE_VOLATILE ||
                    (state & VM_PURGABLE_STATE_MASK) == VM_PURGABLE_EMPTY);
            size_t resident = static_cast<size_t>(
                region.pages_resident + region.pages_swapped_out) *
                static_cast<size_t>(vm_page_size);
            if (!resident && region.user_tag == VM_MEMORY_IOACCELERATOR) {
                resident = static_cast<size_t>(size);
            }
            if (!reclaimable) {
                if (region.user_tag == VM_MEMORY_IOACCELERATOR) {
                    memory.gpuBytes += resident;
                } else {
                    memory.surfaceBytes += resident;
                }
            }
        }
        address += size;
    }
    malloc_statistics_t stats {};
    malloc_zone_statistics(nullptr, &stats);
    memory.mallocInUseBytes = stats.size_in_use;
    memory.mallocAllocatedBytes = stats.size_allocated;
#endif
    return memory;
}

static BrixBenchProcessMemory recordedProcessMemory;
static bool hasRecordedProcessMemory = false;

extern "C" void
brixBenchRecordProcessMemory(void) {
    recordedProcessMemory = brixBenchProcessMemory();
    hasRecordedProcessMemory = true;
}

static size_t
brixBenchPeakRSSBytes() {
    struct rusage usage {};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
#if defined(__APPLE__)
    return static_cast<size_t>(usage.ru_maxrss);
#else
    return static_cast<size_t>(usage.ru_maxrss) * 1024;
#endif
}

extern "C" void
brixBenchRecordAllocationSite(
    size_t siteId,
    const char* source,
    size_t line,
    size_t column,
    const char* function,
    const char* type,
    const char* operation,
    const char* pointerKind
) {
    if (
        !brixBenchMetricsEnabledFlag ||
        !siteId ||
        siteId == kBenchAllocationSiteClaim
    ) {
        return;
    }

    size_t index = siteId % benchmarkAllocationSites.size();
    for (size_t probe = 0; probe < benchmarkAllocationSites.size(); ++probe) {
        auto& entry = benchmarkAllocationSites[index];
        size_t publishedId = entry.siteId.load(std::memory_order_acquire);
        while (publishedId == kBenchAllocationSiteClaim) {
            publishedId = entry.siteId.load(std::memory_order_acquire);
        }
        if (publishedId == siteId) {
            if (
                !brixBenchStringsEqual(entry.source, source) ||
                entry.line != line ||
                entry.column != column ||
                !brixBenchStringsEqual(entry.function, function) ||
                !brixBenchStringsEqual(entry.type, type) ||
                !brixBenchStringsEqual(entry.operation, operation) ||
                !brixBenchStringsEqual(entry.pointerKind, pointerKind)
            ) {
                benchmarkAllocationSiteHashCollisionCount.fetch_add(
                    1,
                    std::memory_order_relaxed);
            }
            entry.count.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        if (!publishedId) {
            size_t expected = 0;
            if (entry.siteId.compare_exchange_strong(
                    expected,
                    kBenchAllocationSiteClaim,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                entry.source = source;
                entry.line = line;
                entry.column = column;
                entry.function = function;
                entry.type = type;
                entry.operation = operation;
                entry.pointerKind = pointerKind;
                entry.count.store(1, std::memory_order_relaxed);
                entry.siteId.store(siteId, std::memory_order_release);
                return;
            }
            continue;
        }
        benchmarkAllocationSiteProbeCollisionCount.fetch_add(1, std::memory_order_relaxed);
        index = (index + 1) % benchmarkAllocationSites.size();
    }
    benchmarkAllocationSiteTableOverflowCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
brixBenchMutatorPhaseStart(const char* name) {
    if (!brixBenchMetricsEnabledFlag || !name || !*name) {
        return;
    }
    size_t index = benchmarkMutatorPhaseCount.fetch_add(1, std::memory_order_acq_rel);
    if (index >= benchmarkMutatorPhases.size()) {
        benchmarkMutatorPhaseErrorCount.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    auto& phase = benchmarkMutatorPhases[index];
    phase.name = name;
    phase.startBarrierCalls = brixBenchCounter("brixGCRuntimeFastBarrierCallCount");
    phase.startBarrierSlowEntries =
        brixBenchCounter("brixGCRuntimeFastBarrierSlowEntryCount");
    phase.startAllocationRefills =
        brixBenchCounter("brixGCRuntimeAllocationFastRefillCount");
    phase.startAssists = brixBenchCounter("brixGCRuntimeAssistCount");
}

extern "C" void
brixBenchMutatorPhaseEnd(const char* name) {
    if (!brixBenchMetricsEnabledFlag || !name || !*name) {
        return;
    }
    size_t count = std::min(
        benchmarkMutatorPhaseCount.load(std::memory_order_acquire),
        benchmarkMutatorPhases.size());
    for (size_t reverse = count; reverse > 0; --reverse) {
        auto& phase = benchmarkMutatorPhases[reverse - 1];
        if (phase.completed || !brixBenchStringsEqual(phase.name, name)) {
            continue;
        }
        size_t current = brixBenchCounter("brixGCRuntimeFastBarrierCallCount");
        phase.barrierCalls = current >= phase.startBarrierCalls
            ? current - phase.startBarrierCalls
            : 0;
        current = brixBenchCounter("brixGCRuntimeFastBarrierSlowEntryCount");
        phase.barrierSlowEntries = current >= phase.startBarrierSlowEntries
            ? current - phase.startBarrierSlowEntries
            : 0;
        current = brixBenchCounter("brixGCRuntimeAllocationFastRefillCount");
        phase.allocationRefills = current >= phase.startAllocationRefills
            ? current - phase.startAllocationRefills
            : 0;
        current = brixBenchCounter("brixGCRuntimeAssistCount");
        phase.assists = current >= phase.startAssists ? current - phase.startAssists : 0;
        phase.completed = true;
        return;
    }
    benchmarkMutatorPhaseErrorCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" size_t
brixBenchTemporaryRootShim(char* pointer, size_t rootCount, size_t iterations) {
    static constexpr size_t kMaximumTemporaryRoots = 64;
    if (rootCount > kMaximumTemporaryRoots) {
        return 0;
    }
    void* roots[kMaximumTemporaryRoots];
    size_t checksum = 0;
    for (size_t iteration = 0; iteration < iterations; ++iteration) {
        size_t added = 0;
        for (; added < rootCount; ++added) {
            roots[added] = brixAddTemporaryRoot(pointer);
            if (!roots[added]) {
                break;
            }
        }
        checksum += added;
        while (added) {
            brixRemoveTemporaryRoot(roots[--added]);
        }
    }
    benchmarkTemporaryRootShimPushCount.fetch_add(checksum, std::memory_order_relaxed);
    return checksum;
}

extern "C" void
brixBenchResetTemporaryRootMetrics(void) {
    benchmarkTemporaryRootShimPushCount.store(0, std::memory_order_relaxed);
    BrixBenchVoidFn reset =
        brixBenchFunction<BrixBenchVoidFn>("brixGCResetTemporaryRootMetrics");
    if (reset) {
        reset();
    }
}

extern "C" void
brixBenchResetActiveTemporaryRootMetrics(void) {
    BrixBenchVoidFn reset =
        brixBenchFunction<BrixBenchVoidFn>("brixGCResetActiveTemporaryRootMetrics");
    if (reset) {
        reset();
    }
}

static bool
brixBenchWriteMutatorPhases(FILE* file) {
    size_t rawCount = benchmarkMutatorPhaseCount.load(std::memory_order_acquire);
    size_t count = std::min(rawCount, benchmarkMutatorPhases.size());
    bool complete = rawCount <= benchmarkMutatorPhases.size() &&
        benchmarkMutatorPhaseErrorCount.load(std::memory_order_relaxed) == 0;
    std::fprintf(
        file,
        "\"mutatorPhaseCounterScope\":{"
        "\"barrierCalls\":\"process-global\","
        "\"barrierSlowEntries\":\"process-global\","
        "\"allocationRefills\":\"calling-mutator-published-TLS\","
        "\"assists\":\"calling-mutator-published-TLS\"},"
        "\"mutatorPhases\":[");
    for (size_t index = 0; index < count; ++index) {
        const auto& phase = benchmarkMutatorPhases[index];
        std::fprintf(file, "%s{\"name\":", index ? "," : "");
        brixBenchWriteJSONString(file, phase.name);
        std::fprintf(
            file,
            ",\"barrierCallCount\":%zu,\"barrierSlowEntryCount\":%zu,"
            "\"allocationRefillCount\":%zu,\"assistCount\":%zu,"
            "\"complete\":%s}",
            phase.barrierCalls,
            phase.barrierSlowEntries,
            phase.allocationRefills,
            phase.assists,
            phase.completed ? "true" : "false");
        complete = complete && phase.completed;
    }
    std::fprintf(
        file,
        "],\"mutatorPhaseErrorCount\":%zu,\"mutatorPhasesComplete\":%s",
        benchmarkMutatorPhaseErrorCount.load(std::memory_order_relaxed),
        complete ? "true" : "false");
    return complete;
}

extern "C" void
brixBenchInitMetrics() {
    const char* path = std::getenv("BRIX_BENCH_METRICS_PATH");
    brixBenchMetricsEnabledFlag = path && *path ? 1 : 0;
    bumpAllocatedObjects.store(0, std::memory_order_relaxed);
    bumpAllocatedBytes.store(0, std::memory_order_relaxed);
    for (auto& bin : bumpSizeBins) {
        bin.store(0, std::memory_order_relaxed);
    }
    for (auto& entry : benchmarkAllocationSites) {
        entry.count.store(0, std::memory_order_relaxed);
        entry.source = nullptr;
        entry.line = 0;
        entry.column = 0;
        entry.function = nullptr;
        entry.type = nullptr;
        entry.operation = nullptr;
        entry.siteId.store(0, std::memory_order_release);
    }
    benchmarkAllocationSiteTableOverflowCount.store(0, std::memory_order_relaxed);
    benchmarkAllocationSiteHashCollisionCount.store(0, std::memory_order_relaxed);
    benchmarkAllocationSiteProbeCollisionCount.store(0, std::memory_order_relaxed);
    for (auto& phase : benchmarkMutatorPhases) {
        phase = BrixBenchMutatorPhaseEntry {};
    }
    benchmarkMutatorPhaseCount.store(0, std::memory_order_relaxed);
    benchmarkMutatorPhaseErrorCount.store(0, std::memory_order_relaxed);
    BrixBenchVoidFn resetAllocationHistograms =
        brixBenchFunction<BrixBenchVoidFn>(
            "brixGCRuntimeBenchResetAllocationHistograms");
    if (resetAllocationHistograms) {
        resetAllocationHistograms();
    }
    BrixBenchVoidFn resetLifetime =
        brixBenchFunction<BrixBenchVoidFn>("brixGCBenchLifetimeReset");
    if (resetLifetime) {
        resetLifetime();
    }
    clock_gettime(CLOCK_MONOTONIC, &benchmarkStartTime);
}

extern "C" void
brixBenchRecordAllocation(size_t size) {
    if (!brixBenchMetricsEnabledFlag) {
        return;
    }
    bumpAllocatedObjects.fetch_add(1, std::memory_order_relaxed);
    bumpAllocatedBytes.fetch_add(size, std::memory_order_relaxed);
    bumpSizeBins[brixBenchSizeBin(size)].fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
brixBenchMaybeWriteMetrics() {
    const char* path = std::getenv("BRIX_BENCH_METRICS_PATH");
    if (!path || !*path) {
        return;
    }

    const char* mode = std::getenv("BRIX_GC_MODE");
    if (!mode || !*mode) {
        mode = "normal";
    }
    const char* runNonce = std::getenv("BRIX_BENCH_RUN_NONCE");
    if (!runNonce) {
        runNonce = "";
    }
    const size_t rssPeakBytes = brixBenchPeakRSSBytes();
    const size_t rssEndBytes = brixBenchCounter("brixGCRssBytes");
    const BrixBenchProcessMemory processMemory = hasRecordedProcessMemory
        ? recordedProcessMemory
        : brixBenchProcessMemory();
    const size_t scavengeDebtBytes = brixBenchCounter("brixGCScavengeDebtBytes");
    BrixBenchCounterFn gcAllocatedObjectCount =
        brixBenchFunction<BrixBenchCounterFn>("brixGCRuntimeAllocatedObjectCount");
    BrixBenchCounterFn gcAllocatedByteCount =
        brixBenchFunction<BrixBenchCounterFn>("brixGCRuntimeAllocatedByteCount");
    BrixBenchCounterFn gcRequestedByteCount =
        brixBenchFunction<BrixBenchCounterFn>("brixGCRuntimeRequestedByteCount");
    BrixBenchCounterFn gcAllocatedSpanCount =
        brixBenchFunction<BrixBenchCounterFn>("brixGCRuntimeAllocatedSpanCount");
    const bool hasGC = gcAllocatedObjectCount && gcAllocatedByteCount &&
        gcRequestedByteCount && gcAllocatedSpanCount;

    const char* source = hasGC ? "gc" : "bump-public";
    size_t allocationCount = hasGC
        ? gcAllocatedObjectCount()
        : bumpAllocatedObjects.load(std::memory_order_relaxed);
    size_t allocatedBytes = hasGC
        ? gcAllocatedByteCount()
        : bumpAllocatedBytes.load(std::memory_order_relaxed);
    size_t requestedBytes = hasGC ? gcRequestedByteCount() : allocatedBytes;
    size_t allocatedSpanCount = hasGC ? gcAllocatedSpanCount() : 0;
    size_t parseRegionObjectAllocationCount =
        brixBenchCounter("brixParseRegionObjectAllocationCount");
    size_t parseRegionBufferAllocationCount =
        brixBenchCounter("brixParseRegionBufferAllocationCount");
    size_t parseRegionPromotionCount =
        brixBenchCounter("brixParseRegionPromotionCount");
    size_t arenaSystemBlockAllocationCount =
        brixBenchCounter("brixArenaSystemBlockAllocationCount");
    size_t arenaSystemBlockFreeCount =
        brixBenchCounter("brixArenaSystemBlockFreeCount");
    size_t arenaActiveBytes = brixBenchCounter("brixArenaRuntimeActiveBytes");
    size_t arenaReservedBytes = brixBenchCounter("brixArenaRuntimeReservedBytes");
    size_t arenaRetainedBytes = brixBenchCounter("brixArenaRuntimeRetainedBytes");
    size_t arenaPeakActiveBytes = brixBenchCounter("brixArenaRuntimePeakActiveBytes");
    size_t arenaMarkCount = brixBenchCounter("brixArenaMarkCount");
    size_t arenaRewindCount = brixBenchCounter("brixArenaRewindCount");
    size_t arenaResetCount = brixBenchCounter("brixArenaResetCount");
    size_t arenaSlotCount = brixBenchCounter("brixArenaSlotCount");
    size_t arenaLiveSlotCount = brixBenchCounter("brixArenaLiveSlotCount");
    size_t arenaManagedAllocationCount =
        brixBenchCounter("brixArenaManagedAllocationCount");
    size_t arenaZoneTransitionCount =
        brixBenchCounter("brixArenaZoneTransitionCount");
    size_t gcCycleCount = hasGC
        ? brixBenchCounter("brixGCRuntimeCollectionCount")
        : 0;
    size_t runtimeNanos = brixBenchMonotonicNanos();
    size_t assistNanos = brixBenchCounter("brixGCRuntimeAssistTimeNanos");
    size_t gcCpuNanos = brixBenchCounter("brixGCRuntimeBenchGCCpuNanos");
    double gcCpuPct = runtimeNanos
        ? static_cast<double>(gcCpuNanos) * 100.0 / static_cast<double>(runtimeNanos)
        : 0.0;
    size_t committedBytes = brixBenchCounter("brixGCCommittedBytes");
    if (!committedBytes) {
        committedBytes = brixBenchCounter("brixGCCommittedPageCount") * kGCPageSize;
    }
    size_t liveBytes = brixBenchCounter("brixGCRuntimeLiveByteCount");
    size_t heapOverheadBytes = committedBytes > liveBytes ? committedBytes - liveBytes : 0;
    double heapOverheadPct = liveBytes
        ? static_cast<double>(heapOverheadBytes) * 100.0 / static_cast<double>(liveBytes)
        : 0.0;
    size_t markWorkRetainedBytes = hasGC
        ? brixBenchCounter("brixGCRuntimeMarkWorkRetainedBytes")
        : 0;
    size_t spanMetadataBytes = hasGC
        ? brixBenchCounter("brixGCRuntimeSpanMetadataBytes")
        : 0;
    size_t addressIndexMetadataBytes = hasGC
        ? brixBenchCounter("brixGCRuntimeAddressIndexMetadataBytes")
        : 0;
    size_t gcMetadataBytes = markWorkRetainedBytes + spanMetadataBytes +
        addressIndexMetadataBytes;
    size_t fragmentationBytes = hasGC
        ? brixBenchCounter("brixGCFragmentationBytes")
        : 0;
    size_t metadataAndFragmentationBytes = gcMetadataBytes;
    if (metadataAndFragmentationBytes <=
            std::numeric_limits<size_t>::max() - fragmentationBytes) {
        metadataAndFragmentationBytes += fragmentationBytes;
    } else {
        metadataAndFragmentationBytes = std::numeric_limits<size_t>::max();
    }
    double metadataAndFragmentationPct = liveBytes
        ? static_cast<double>(metadataAndFragmentationBytes) * 100.0 /
            static_cast<double>(liveBytes)
        : 0.0;
    const char* gcMetadataBasis = hasGC
        ? "workbuf-high-water-plus-span-slab-active-flat-plus-address-index"
        : "inapplicable-no-gc";
    size_t frameBudgetOfferCount =
        brixBenchCounter("brixGCRuntimeFrameBudgetOfferCount");
    size_t frameBudgetWorkCount =
        brixBenchCounter("brixGCRuntimeFrameBudgetWorkCount");
    size_t frameBudgetWorkNanos =
        brixBenchCounter("brixGCRuntimeFrameBudgetWorkNanos");
    size_t frameBudgetOverrunCount =
        brixBenchCounter("brixGCRuntimeFrameBudgetOverrunCount");
    size_t frameBudgetMaxOverrunNanos =
        brixBenchCounter("brixGCRuntimeFrameBudgetMaxOverrunNanos");
    size_t memoryReturnNanos = brixBenchCounter("brixGCRuntimeScavengeMaxNanos");
    size_t allocationRefillCount =
        brixBenchCounter("brixGCRuntimeAllocationFastRefillCount");
    size_t batchReleasedBytes = brixBenchCounter("brixGCRuntimeBatchReleasedBytes");
    size_t batchDiscardedBytes = brixBenchCounter("brixGCRuntimeBatchDiscardedBytes");
    size_t adaptiveBatchGrowthCount =
        brixBenchCounter("brixGCRuntimeAdaptiveBatchGrowthCount");
    size_t adaptiveBatchShrinkCount =
        brixBenchCounter("brixGCRuntimeAdaptiveBatchShrinkCount");
    size_t adaptiveBatchReturnedUnusedSlotCount =
        brixBenchCounter("brixGCRuntimeAdaptiveBatchReturnedUnusedSlotCount");
    size_t compilerIndexedRefillCount =
        brixBenchCounter("brixGCRuntimeCompilerIndexedRefillCount");
    size_t activeBufferRefillCount =
        brixBenchCounter("brixGCRuntimeActiveBufferRefillCount");
    size_t genericDescribedRefillCount =
        brixBenchCounter("brixGCRuntimeGenericDescribedRefillCount");
    size_t genericDescriptorlessRefillCount =
        brixBenchCounter("brixGCRuntimeGenericDescriptorlessRefillCount");
    size_t compilerBufferSiteSwitchCount =
        brixBenchCounter("brixGCRuntimeCompilerBufferSiteSwitchCount");
    size_t fullAllocationRangeFlushCount =
        brixBenchCounter("brixGCRuntimeFullAllocationRangeFlushCount");
    size_t fullAllocationRangeFlushedBufferCount =
        brixBenchCounter("brixGCRuntimeFullAllocationRangeFlushedBufferCount");
    size_t batchCachePressureFlushCount =
        brixBenchCounter("brixGCRuntimeBatchCachePressureFlushCount");
    size_t allocationBatchMissGeneration =
        brixBenchIndexedCounter("brixGCRuntimeAllocationBatchMissCount", 0);
    size_t allocationBatchMissDescriptor =
        brixBenchIndexedCounter("brixGCRuntimeAllocationBatchMissCount", 1);
    size_t allocationBatchMissRequestedSize =
        brixBenchIndexedCounter("brixGCRuntimeAllocationBatchMissCount", 2);
    size_t allocationBatchMissEmpty =
        brixBenchIndexedCounter("brixGCRuntimeAllocationBatchMissCount", 3);
    size_t allocationBatchMissCap =
        brixBenchIndexedCounter("brixGCRuntimeAllocationBatchMissCount", 4);
    size_t rootScanSampleCount = brixBenchCounter("brixGCRuntimeRootScanSampleCount");
    size_t rootScanP95Nanos = brixBenchCounter("brixGCRuntimeRootScanP95Nanos");
    size_t rootScanP99Nanos = brixBenchCounter("brixGCRuntimeRootScanP99Nanos");
    size_t rootScanMaxNanos = brixBenchCounter("brixGCRuntimeRootScanMaxNanos");
    size_t rootRemoveCount = brixBenchCounter("brixGCTestRootUnlinkCount");
    size_t externalRootLiveCount = brixBenchCounter("brixGCTestRootRegistrationCount");
    size_t rootAddCount = rootRemoveCount + externalRootLiveCount;
    size_t temporaryRootPushCount =
        brixBenchCounter("brixGCTemporaryRootPushCount");
    size_t temporaryRootShimPushCount =
        benchmarkTemporaryRootShimPushCount.load(std::memory_order_relaxed);
    size_t temporaryRootPopCount =
        brixBenchCounter("brixGCTemporaryRootPopCount");
    size_t temporaryRootStorageAllocationCount =
        brixBenchCounter("brixGCTemporaryRootStorageAllocationCount");
    size_t temporaryRootSlabGrowthCount =
        brixBenchCounter("brixGCTemporaryRootSlabGrowthCount");
    size_t temporaryRootMaxDepth =
        brixBenchCounter("brixGCTemporaryRootMaxDepth");
    size_t cleanupSidePublishCount =
        brixBenchCounter("brixGCCleanupSidePublishCount");
    size_t cleanupSideValidationNanos =
        brixBenchCounter("brixGCCleanupSideValidationNanos");
    size_t cleanupSideValidationObjectCount =
        brixBenchCounter("brixGCCleanupSideValidationObjectCount");
    size_t cleanupSideValidationEdgeCount =
        brixBenchCounter("brixGCCleanupSideValidationEdgeCount");
    size_t barrierCallCount = brixBenchCounter("brixGCRuntimeFastBarrierCallCount");
    size_t barrierSlowEntryCount =
        brixBenchCounter("brixGCRuntimeFastBarrierSlowEntryCount");
    size_t bufferedBarrierSharedRMWCount =
        brixBenchCounter("brixGCRuntimeBufferedBarrierSharedRMWCount");
    double barrierSlowEntryPct = barrierCallCount
        ? static_cast<double>(barrierSlowEntryCount) * 100.0 /
            static_cast<double>(barrierCallCount)
        : 0.0;
    std::vector<size_t> pauseDurations;

    FILE* file = std::fopen(path, "w");
    if (!file) {
        return;
    }

    std::fprintf(
        file,
        "{\"schemaVersion\":2,\"source\":\"%s\",\"variant\":\"%s\","
        "\"runNonce\":\"%s\",\"rssObservationPass\":\"instrumented\","
        "\"rssObservedBeforeSerialization\":true,"
        "\"runtimeModeId\":%zu,\"runtimeNanos\":%zu,"
        "\"allocationCount\":%zu,\"allocatedBytes\":%zu,"
        "\"requestedBytes\":%zu,\"allocatedSpanCount\":%zu,"
        "\"parseRegionObjectAllocationCount\":%zu,"
        "\"parseRegionBufferAllocationCount\":%zu,"
        "\"parseRegionPromotionCount\":%zu,"
        "\"arenaSystemBlockAllocationCount\":%zu,"
        "\"arenaSystemBlockFreeCount\":%zu,"
        "\"arenaActiveBytes\":%zu,"
        "\"arenaReservedBytes\":%zu,"
        "\"arenaRetainedBytes\":%zu,"
        "\"arenaPeakActiveBytes\":%zu,"
        "\"arenaMarkCount\":%zu,"
        "\"arenaRewindCount\":%zu,"
        "\"arenaResetCount\":%zu,"
        "\"arenaSlotCount\":%zu,"
        "\"arenaLiveSlotCount\":%zu,"
        "\"arenaManagedAllocationCount\":%zu,"
        "\"arenaZoneTransitionCount\":%zu,",
        source,
        mode,
        runNonce,
        brixBenchCounter("brixGCRuntimeMode"),
        runtimeNanos,
        allocationCount,
        allocatedBytes,
        requestedBytes,
        allocatedSpanCount,
        parseRegionObjectAllocationCount,
        parseRegionBufferAllocationCount,
        parseRegionPromotionCount,
        arenaSystemBlockAllocationCount,
        arenaSystemBlockFreeCount,
        arenaActiveBytes,
        arenaReservedBytes,
        arenaRetainedBytes,
        arenaPeakActiveBytes,
        arenaMarkCount,
        arenaRewindCount,
        arenaResetCount,
        arenaSlotCount,
        arenaLiveSlotCount,
        arenaManagedAllocationCount,
        arenaZoneTransitionCount);
    size_t histogramCount = brixBenchWriteSizeHistogram(file, hasGC);
    std::fprintf(
        file,
        ",\"allocationSizeHistogramBasis\":\"requestedBytes\","
        "\"allocationSizeHistogramCount\":%zu,"
        "\"allocationSizeHistogramComplete\":%s,",
        histogramCount,
        histogramCount == allocationCount ? "true" : "false");
    BrixBenchSizeClassSummary sizeClasses =
        brixBenchWriteSizeClassHistogram(file, hasGC);
    const bool sizeClassesComplete = sizeClasses.available &&
        (!hasGC || sizeClasses.total == allocationCount);
    std::fprintf(
        file,
        ",\"allocationSizeClassHistogramBasis\":\"%s\","
        "\"allocationSizeClassHistogramCount\":%zu,"
        "\"allocationSizeClassHistogramComplete\":%s,",
        hasGC ? "allocatedBytes-v1-size-class" : "inapplicable-no-gc",
        sizeClasses.total,
        sizeClassesComplete ? "true" : "false");
    BrixBenchAllocationSiteSummary allocationSites =
        brixBenchWriteAllocationSites(file, runtimeNanos);
    size_t unattributedAllocations = allocationCount > allocationSites.operationCount
        ? allocationCount - allocationSites.operationCount
        : 0;
    size_t nonAllocatingOperations = allocationSites.operationCount > allocationCount
        ? allocationSites.operationCount - allocationCount
        : 0;
    bool allocationSitesComplete =
        !allocationSites.tableOverflowCount &&
        !allocationSites.hashCollisionCount &&
        !nonAllocatingOperations;
    std::fprintf(
        file,
        ",\"allocationSiteIdScheme\":\"fnv1a64-utf16le-v1\","
        "\"allocationSiteCoverage\":\"compiler-visible-successful-operations\","
        "\"allocationSiteTableCapacity\":%zu,"
        "\"allocationSiteEntryCount\":%zu,"
        "\"allocationSiteCount\":%zu,"
        "\"allocationSiteSupplementalOperationCount\":%zu,"
        "\"allocationSiteUnattributedAllocationCount\":%zu,"
        "\"allocationSiteNonAllocatingOperationCount\":%zu,"
        "\"allocationSiteTableOverflowCount\":%zu,"
        "\"allocationSiteHashCollisionCount\":%zu,"
        "\"allocationSiteProbeCollisionCount\":%zu,"
        "\"allocationSiteRatesComplete\":%s,",
        kBenchAllocationSiteCapacity,
        allocationSites.entryCount,
        allocationSites.operationCount,
        allocationSites.supplementalOperationCount,
        unattributedAllocations,
        nonAllocatingOperations,
        allocationSites.tableOverflowCount,
        allocationSites.hashCollisionCount,
        allocationSites.probeCollisionCount,
        allocationSitesComplete ? "true" : "false");
    size_t allocationBufferCount = brixBenchWriteAllocationBuffers(file, runtimeNanos);
    if (hasGC) {
        std::fprintf(file, ",\"allocationBufferCount\":%zu", allocationBufferCount);
    }
    std::fprintf(file, ",");
    brixBenchWriteThreadRefills(file);
    std::fprintf(file, ",");
    bool mutatorPhasesComplete = brixBenchWriteMutatorPhases(file);
    std::fprintf(file, ",");
    bool lifetimeInvariantValid = brixBenchWriteLifetimeHistogram(file, mode);
    std::fprintf(file, ",");
    BrixBenchPauseTraceSummary pauseTrace =
        brixBenchWritePauseTrace(file, pauseDurations);
    std::fprintf(file, ",");
    brixBenchWriteCycleSummaries(file);

    size_t pauseP50 = brixBenchPercentile(pauseDurations, 50);
    size_t pauseP99 = brixBenchPercentile(pauseDurations, 99);
    size_t pauseMax = pauseDurations.empty()
        ? 0
        : *std::max_element(pauseDurations.begin(), pauseDurations.end());
    std::fprintf(
        file,
        ",\"gcPauseTraceTotalEvents\":%zu,\"gcPauseTraceStoredEvents\":%zu,"
        "\"gcPauseTraceCapacity\":%zu,\"gcPauseTraceDroppedEvents\":%zu,"
        "\"gcPauseSummaryKinds\":[1,3,4],\"gcPauseSummaryStoredEvents\":%zu,"
        "\"gcPauseSummaryComplete\":%s,"
        "\"gcHandshakeTasksWaited\":%zu,\"gcHandshakeSlowestTaskNanos\":%zu"
        ",\"gcPauseP50Us\":%.3f,\"gcPauseP99Us\":%.3f,"
        "\"gcPauseMaxUs\":%.3f,\"gcCpuNanos\":%zu,\"gcCpuPct\":%.6f,"
        "\"rssPeakBytes\":%zu,\"rssEndBytes\":%zu,"
        "\"committedBytes\":%zu,\"liveBytes\":%zu,"
        "\"heapOverheadBasis\":\"committed-arena-pages-minus-live-bytes\","
        "\"markWorkRetainedBytes\":%zu,\"spanMetadataBytes\":%zu,"
        "\"addressIndexMetadataBytes\":%zu,\"gcMetadataBytes\":%zu,"
        "\"fragmentationBytes\":%zu,"
        "\"metadataAndFragmentationBytes\":%zu,"
        "\"metadataAndFragmentationPct\":%.6f,"
        "\"gcMetadataBasis\":\"%s\","
        "\"frameBudgetOfferCount\":%zu,\"frameBudgetWorkCount\":%zu,"
        "\"frameBudgetWorkNanos\":%zu,\"frameBudgetOverrunCount\":%zu,"
        "\"frameBudgetMaxOverrunNanos\":%zu,"
        "\"assistCount\":%zu,\"assistNanos\":%zu,"
        "\"allocationRefillCount\":%zu,\"barrierCallCount\":%zu,"
        "\"batchReleasedBytes\":%zu,\"batchDiscardedBytes\":%zu,"
        "\"adaptiveBatchGrowthCount\":%zu,\"adaptiveBatchShrinkCount\":%zu,"
        "\"adaptiveBatchReturnedUnusedSlotCount\":%zu,"
        "\"compilerIndexedRefillCount\":%zu,\"activeBufferRefillCount\":%zu,"
        "\"genericDescribedRefillCount\":%zu,"
        "\"genericDescriptorlessRefillCount\":%zu,"
        "\"compilerBufferSiteSwitchCount\":%zu,"
        "\"fullAllocationRangeFlushCount\":%zu,"
        "\"fullAllocationRangeFlushedBufferCount\":%zu,"
        "\"batchCachePressureFlushCount\":%zu,"
        "\"allocationBatchMissGeneration\":%zu,"
        "\"allocationBatchMissDescriptor\":%zu,"
        "\"allocationBatchMissRequestedSize\":%zu,"
        "\"allocationBatchMissEmpty\":%zu,\"allocationBatchMissCap\":%zu,"
        "\"rootScanSampleCount\":%zu,\"rootScanP95Nanos\":%zu,"
        "\"rootScanP99Nanos\":%zu,\"rootScanMaxNanos\":%zu,"
        "\"rootAddCount\":%zu,\"rootRemoveCount\":%zu,"
        "\"temporaryRootPushCount\":%zu,\"temporaryRootShimPushCount\":%zu,"
        "\"temporaryRootPopCount\":%zu,"
        "\"temporaryRootStorageAllocationCount\":%zu,"
        "\"temporaryRootSlabGrowthCount\":%zu,"
        "\"temporaryRootMaxDepth\":%zu,"
        "\"externalRootLiveCount\":%zu,"
        "\"cleanupSidePublishCount\":%zu,"
        "\"cleanupSideValidationNanos\":%zu,"
        "\"cleanupSideValidationObjectCount\":%zu,"
        "\"cleanupSideValidationEdgeCount\":%zu,"
        "\"barrierSlowEntryCount\":%zu,\"barrierSlowEntryPct\":%.9f,"
        "\"bufferedBarrierSharedRMWCount\":%zu,"
        "\"heapOverheadBytes\":%zu,\"heapOverheadPct\":%.6f,"
        "\"memoryReturnLatencyUs\":%.3f,\"gcCycleCount\":%zu,"
        "\"footprintBytes\":%zu,\"footprintPeakBytes\":%zu,"
        "\"gpuBytes\":%zu,\"surfaceBytes\":%zu,"
        "\"mallocInUseBytes\":%zu,\"mallocAllocatedBytes\":%zu,"
        "\"scavengeDebtBytes\":%zu,"
        "\"metricsInvariantValid\":%s}\n",
        pauseTrace.totalEvents,
        pauseTrace.storedEvents,
        pauseTrace.capacity,
        pauseTrace.totalEvents > pauseTrace.storedEvents
            ? pauseTrace.totalEvents - pauseTrace.storedEvents
            : 0,
        pauseTrace.summaryEvents,
        pauseTrace.totalEvents == pauseTrace.storedEvents ? "true" : "false",
        pauseTrace.tasksWaited,
        pauseTrace.slowestTaskNanos,
        static_cast<double>(pauseP50) / 1000.0,
        static_cast<double>(pauseP99) / 1000.0,
        static_cast<double>(pauseMax) / 1000.0,
        gcCpuNanos,
        gcCpuPct,
        rssPeakBytes,
        rssEndBytes,
        committedBytes,
        liveBytes,
        markWorkRetainedBytes,
        spanMetadataBytes,
        addressIndexMetadataBytes,
        gcMetadataBytes,
        fragmentationBytes,
        metadataAndFragmentationBytes,
        metadataAndFragmentationPct,
        gcMetadataBasis,
        frameBudgetOfferCount,
        frameBudgetWorkCount,
        frameBudgetWorkNanos,
        frameBudgetOverrunCount,
        frameBudgetMaxOverrunNanos,
        brixBenchCounter("brixGCRuntimeAssistCount"),
        assistNanos,
        allocationRefillCount,
        barrierCallCount,
        batchReleasedBytes,
        batchDiscardedBytes,
        adaptiveBatchGrowthCount,
        adaptiveBatchShrinkCount,
        adaptiveBatchReturnedUnusedSlotCount,
        compilerIndexedRefillCount,
        activeBufferRefillCount,
        genericDescribedRefillCount,
        genericDescriptorlessRefillCount,
        compilerBufferSiteSwitchCount,
        fullAllocationRangeFlushCount,
        fullAllocationRangeFlushedBufferCount,
        batchCachePressureFlushCount,
        allocationBatchMissGeneration,
        allocationBatchMissDescriptor,
        allocationBatchMissRequestedSize,
        allocationBatchMissEmpty,
        allocationBatchMissCap,
        rootScanSampleCount,
        rootScanP95Nanos,
        rootScanP99Nanos,
        rootScanMaxNanos,
        rootAddCount,
        rootRemoveCount,
        temporaryRootPushCount,
        temporaryRootShimPushCount,
        temporaryRootPopCount,
        temporaryRootStorageAllocationCount,
        temporaryRootSlabGrowthCount,
        temporaryRootMaxDepth,
        externalRootLiveCount,
        cleanupSidePublishCount,
        cleanupSideValidationNanos,
        cleanupSideValidationObjectCount,
        cleanupSideValidationEdgeCount,
        barrierSlowEntryCount,
        barrierSlowEntryPct,
        bufferedBarrierSharedRMWCount,
        heapOverheadBytes,
        heapOverheadPct,
        static_cast<double>(memoryReturnNanos) / 1000.0,
        gcCycleCount,
        processMemory.footprintBytes,
        processMemory.footprintPeakBytes,
        processMemory.gpuBytes,
        processMemory.surfaceBytes,
        processMemory.mallocInUseBytes,
        processMemory.mallocAllocatedBytes,
        scavengeDebtBytes,
        (histogramCount == allocationCount &&
            sizeClassesComplete &&
            allocationSitesComplete &&
            mutatorPhasesComplete &&
            lifetimeInvariantValid &&
            pauseTrace.totalEvents == pauseTrace.storedEvents)
            ? "true"
            : "false");
    std::fclose(file);
}
