#include "YuCore/YuCore.h"

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

struct YumiBenchPauseTraceSummary {
    size_t totalEvents;
    size_t storedEvents;
    size_t capacity;
    size_t summaryEvents;
    size_t tasksWaited;
    size_t slowestTaskNanos;
};

struct YumiBenchSizeClassSummary {
    size_t total;
    bool available;
};

struct YumiBenchAllocationSiteEntry {
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

struct YumiBenchAllocationSiteSummary {
    size_t operationCount;
    size_t supplementalOperationCount;
    size_t entryCount;
    size_t tableOverflowCount;
    size_t hashCollisionCount;
    size_t probeCollisionCount;
};

struct YumiBenchMutatorPhaseEntry {
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
static std::array<YumiBenchAllocationSiteEntry, kBenchAllocationSiteCapacity>
    benchmarkAllocationSites {};
static std::atomic<size_t> benchmarkAllocationSiteTableOverflowCount {0};
static std::atomic<size_t> benchmarkAllocationSiteHashCollisionCount {0};
static std::atomic<size_t> benchmarkAllocationSiteProbeCollisionCount {0};
static std::array<YumiBenchMutatorPhaseEntry, kBenchMutatorPhaseCapacity>
    benchmarkMutatorPhases {};
static std::atomic<size_t> benchmarkMutatorPhaseCount {0};
static std::atomic<size_t> benchmarkMutatorPhaseErrorCount {0};
static std::atomic<size_t> benchmarkTemporaryRootShimPushCount {0};
static struct timespec benchmarkStartTime {};

typedef size_t (*YumiBenchCounterFn)(void);
typedef size_t (*YumiBenchIndexedCounterFn)(size_t);
typedef void (*YumiBenchVoidFn)(void);

extern "C" unsigned char yumiBenchMetricsEnabledFlag = 0;

template <typename Function>
static Function
yumiBenchFunction(const char* name) {
    return reinterpret_cast<Function>(dlsym(RTLD_DEFAULT, name));
}

static size_t
yumiBenchCounter(const char* name) {
    YumiBenchCounterFn counter = yumiBenchFunction<YumiBenchCounterFn>(name);
    return counter ? counter() : 0;
}

static size_t
yumiBenchIndexedCounter(const char* name, size_t index) {
    YumiBenchIndexedCounterFn counter =
        yumiBenchFunction<YumiBenchIndexedCounterFn>(name);
    return counter ? counter(index) : 0;
}

static size_t
yumiBenchMonotonicNanos() {
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
yumiBenchSizeBin(size_t size) {
    if (size > 0 && size <= 1024) {
        return (size - 1) / 8;
    }
    if (size > 1024 && size <= kBenchLargeSizeBinCount * kBenchLargeSizeBinSize) {
        return kBenchSmallSizeBinCount + (size - 1025) / kBenchLargeSizeBinSize;
    }
    return kBenchOverflowSizeBin;
}

static size_t
yumiBenchSizeBinUpperBound(size_t index) {
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
yumiBenchPercentile(std::vector<size_t> values, size_t percentile) {
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    return values[((values.size() - 1) * percentile) / 100];
}

static bool
yumiBenchStringsEqual(const char* left, const char* right) {
    return left && right && std::strcmp(left, right) == 0;
}

static void
yumiBenchWriteJSONString(FILE* file, const char* value) {
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
yumiBenchWriteSizeHistogram(FILE* file, bool hasGC) {
    std::fprintf(file, "\"allocationSizeHistogramBytes\":[");
    bool first = true;
    size_t total = 0;
    size_t binCount = hasGC
        ? yumiBenchCounter("yumiGCRuntimeBenchAllocationSizeBinCount")
        : kBenchSizeBinCount;
    for (size_t index = 0; index < binCount; ++index) {
        size_t count = hasGC
            ? yumiBenchIndexedCounter(
                "yumiGCRuntimeBenchAllocationSizeBinValue",
                index)
            : bumpSizeBins[index].load(std::memory_order_relaxed);
        if (!count) {
            continue;
        }
        size_t bound = hasGC
            ? yumiBenchIndexedCounter(
                "yumiGCRuntimeBenchAllocationSizeBinUpperBound",
                index)
            : yumiBenchSizeBinUpperBound(index);
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

static YumiBenchSizeClassSummary
yumiBenchWriteSizeClassHistogram(FILE* file, bool hasGC) {
    if (!hasGC) {
        std::fprintf(
            file,
            "\"allocationSizeClassHistogramBytes\":null,"
            "\"allocationSizeClassHistogramInapplicableReason\":"
            "\"historical no-GC bump allocation has no GC size classes\"");
        return {0, true};
    }

    YumiBenchCounterFn countFn = yumiBenchFunction<YumiBenchCounterFn>(
        "yumiGCRuntimeBenchAllocatedSizeClassCount");
    YumiBenchIndexedCounterFn upperBoundFn =
        yumiBenchFunction<YumiBenchIndexedCounterFn>(
            "yumiGCRuntimeBenchAllocatedSizeClassUpperBound");
    YumiBenchIndexedCounterFn valueFn =
        yumiBenchFunction<YumiBenchIndexedCounterFn>(
            "yumiGCRuntimeBenchAllocatedSizeClassValue");
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
yumiBenchWriteLifetimeHistogram(FILE* file, const char* mode) {
    YumiBenchCounterFn binCount =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCBenchLifetimeBinCount");
    YumiBenchIndexedCounterFn upperBound =
        yumiBenchFunction<YumiBenchIndexedCounterFn>("yumiGCBenchLifetimeBinUpperBound");
    YumiBenchIndexedCounterFn value =
        yumiBenchFunction<YumiBenchIndexedCounterFn>("yumiGCBenchLifetimeBinValue");
    YumiBenchCounterFn sampleEvery =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCBenchLifetimeSampleEvery");
    YumiBenchCounterFn sampledReservations =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCBenchLifetimeSampledReservationCount");
    YumiBenchCounterFn clearedReservations =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCBenchLifetimeClearedReservationCount");
    YumiBenchCounterFn reclaimed =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCBenchLifetimeReclaimedCount");
    YumiBenchCounterFn outstanding =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCBenchLifetimeOutstandingCount");
    YumiBenchIndexedCounterFn censored = yumiBenchFunction<YumiBenchIndexedCounterFn>(
        "yumiGCBenchLifetimeCensoredBinValue");
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

static YumiBenchPauseTraceSummary
yumiBenchWritePauseTrace(FILE* file, std::vector<size_t>& durations) {
    YumiBenchCounterFn countFn =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCRuntimePauseEventCount");
    const size_t count = countFn ? countFn() : 0;
    YumiBenchPauseTraceSummary summary {
        yumiBenchCounter("yumiGCRuntimePauseCount"),
        count,
        yumiBenchCounter("yumiGCRuntimePauseEventCapacity"),
        0,
        0,
        0
    };
    std::fprintf(file, "\"gcPauseTrace\":[");
    for (size_t index = 0; index < count; ++index) {
        size_t kind = yumiBenchIndexedCounter("yumiGCRuntimePauseEventKind", index);
        size_t duration =
            yumiBenchIndexedCounter("yumiGCRuntimePauseEventDurationNanos", index);
        size_t tasksWaited =
            yumiBenchIndexedCounter("yumiGCRuntimePauseEventTasksWaited", index);
        size_t slowestTaskNanos =
            yumiBenchIndexedCounter("yumiGCRuntimePauseEventSlowestTaskNanos", index);
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
            yumiBenchIndexedCounter("yumiGCRuntimePauseEventCycleId", index),
            kind,
            yumiBenchIndexedCounter("yumiGCRuntimePauseEventStartNanos", index),
            duration,
            tasksWaited,
            slowestTaskNanos);
    }
    std::fprintf(file, "]");
    return summary;
}

static void
yumiBenchWriteCycleSummaries(FILE* file) {
    YumiBenchCounterFn countFn =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCRuntimeCycleSummaryCount");
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
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryCycleId", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryTriggerReason", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryHeapAtTrigger", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryHeapGoal", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryHeapActual", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryMarkDurationNanos", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryScanWork", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryAssistWork", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryBackgroundWork", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummarySweepPages", index),
            yumiBenchIndexedCounter("yumiGCRuntimeCycleSummaryScavengedPages", index));
    }
    std::fprintf(file, "]");
}

static size_t
yumiBenchWriteAllocationBuffers(FILE* file, size_t runtimeNanos) {
    YumiBenchCounterFn countFn = yumiBenchFunction<YumiBenchCounterFn>(
        "yumiGCRuntimeBenchAllocationSiteCount");
    YumiBenchIndexedCounterFn valueFn = yumiBenchFunction<YumiBenchIndexedCounterFn>(
        "yumiGCRuntimeBenchAllocationSiteValue");
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
yumiBenchWriteThreadRefills(FILE* file) {
    YumiBenchCounterFn countFn =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCRuntimeBenchThreadRefillCount");
    YumiBenchIndexedCounterFn valueFn =
        yumiBenchFunction<YumiBenchIndexedCounterFn>("yumiGCRuntimeBenchThreadRefillValue");
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

static YumiBenchAllocationSiteSummary
yumiBenchWriteAllocationSites(FILE* file, size_t runtimeNanos) {
    YumiBenchAllocationSiteSummary summary {
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
        bool supplemental = yumiBenchStringsEqual(entry.pointerKind, "supplemental");
        double rate = runtimeNanos
            ? static_cast<double>(count) * 1000000000.0 /
                static_cast<double>(runtimeNanos)
            : 0.0;
        std::fprintf(file, "%s{\"siteId\":\"%016zx\",\"source\":", first ? "" : ",", siteId);
        yumiBenchWriteJSONString(file, entry.source);
        std::fprintf(file, ",\"line\":%zu,\"column\":%zu,\"function\":", entry.line, entry.column);
        yumiBenchWriteJSONString(file, entry.function);
        std::fprintf(file, ",\"type\":");
        yumiBenchWriteJSONString(file, entry.type);
        std::fprintf(file, ",\"operation\":");
        yumiBenchWriteJSONString(file, entry.operation);
        std::fprintf(file, ",\"pointerKind\":");
        yumiBenchWriteJSONString(file, entry.pointerKind);
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

struct YumiBenchProcessMemory {
    size_t footprintBytes = 0;
    size_t footprintPeakBytes = 0;
    size_t gpuBytes = 0;
    size_t surfaceBytes = 0;
    size_t mallocInUseBytes = 0;
    size_t mallocAllocatedBytes = 0;
};

static YumiBenchProcessMemory
yumiBenchProcessMemory() {
    YumiBenchProcessMemory memory;
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

static YumiBenchProcessMemory recordedProcessMemory;
static bool hasRecordedProcessMemory = false;

extern "C" void
yumiBenchRecordProcessMemory(void) {
    recordedProcessMemory = yumiBenchProcessMemory();
    hasRecordedProcessMemory = true;
}

static size_t
yumiBenchPeakRSSBytes() {
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
yumiBenchRecordAllocationSite(
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
        !yumiBenchMetricsEnabledFlag ||
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
                !yumiBenchStringsEqual(entry.source, source) ||
                entry.line != line ||
                entry.column != column ||
                !yumiBenchStringsEqual(entry.function, function) ||
                !yumiBenchStringsEqual(entry.type, type) ||
                !yumiBenchStringsEqual(entry.operation, operation) ||
                !yumiBenchStringsEqual(entry.pointerKind, pointerKind)
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
yumiBenchMutatorPhaseStart(const char* name) {
    if (!yumiBenchMetricsEnabledFlag || !name || !*name) {
        return;
    }
    size_t index = benchmarkMutatorPhaseCount.fetch_add(1, std::memory_order_acq_rel);
    if (index >= benchmarkMutatorPhases.size()) {
        benchmarkMutatorPhaseErrorCount.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    auto& phase = benchmarkMutatorPhases[index];
    phase.name = name;
    phase.startBarrierCalls = yumiBenchCounter("yumiGCRuntimeFastBarrierCallCount");
    phase.startBarrierSlowEntries =
        yumiBenchCounter("yumiGCRuntimeFastBarrierSlowEntryCount");
    phase.startAllocationRefills =
        yumiBenchCounter("yumiGCRuntimeAllocationFastRefillCount");
    phase.startAssists = yumiBenchCounter("yumiGCRuntimeAssistCount");
}

extern "C" void
yumiBenchMutatorPhaseEnd(const char* name) {
    if (!yumiBenchMetricsEnabledFlag || !name || !*name) {
        return;
    }
    size_t count = std::min(
        benchmarkMutatorPhaseCount.load(std::memory_order_acquire),
        benchmarkMutatorPhases.size());
    for (size_t reverse = count; reverse > 0; --reverse) {
        auto& phase = benchmarkMutatorPhases[reverse - 1];
        if (phase.completed || !yumiBenchStringsEqual(phase.name, name)) {
            continue;
        }
        size_t current = yumiBenchCounter("yumiGCRuntimeFastBarrierCallCount");
        phase.barrierCalls = current >= phase.startBarrierCalls
            ? current - phase.startBarrierCalls
            : 0;
        current = yumiBenchCounter("yumiGCRuntimeFastBarrierSlowEntryCount");
        phase.barrierSlowEntries = current >= phase.startBarrierSlowEntries
            ? current - phase.startBarrierSlowEntries
            : 0;
        current = yumiBenchCounter("yumiGCRuntimeAllocationFastRefillCount");
        phase.allocationRefills = current >= phase.startAllocationRefills
            ? current - phase.startAllocationRefills
            : 0;
        current = yumiBenchCounter("yumiGCRuntimeAssistCount");
        phase.assists = current >= phase.startAssists ? current - phase.startAssists : 0;
        phase.completed = true;
        return;
    }
    benchmarkMutatorPhaseErrorCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" size_t
yumiBenchTemporaryRootShim(char* pointer, size_t rootCount, size_t iterations) {
    static constexpr size_t kMaximumTemporaryRoots = 64;
    if (rootCount > kMaximumTemporaryRoots) {
        return 0;
    }
    void* roots[kMaximumTemporaryRoots];
    size_t checksum = 0;
    for (size_t iteration = 0; iteration < iterations; ++iteration) {
        size_t added = 0;
        for (; added < rootCount; ++added) {
            roots[added] = yumiAddTemporaryRoot(pointer);
            if (!roots[added]) {
                break;
            }
        }
        checksum += added;
        while (added) {
            yumiRemoveTemporaryRoot(roots[--added]);
        }
    }
    benchmarkTemporaryRootShimPushCount.fetch_add(checksum, std::memory_order_relaxed);
    return checksum;
}

extern "C" void
yumiBenchResetTemporaryRootMetrics(void) {
    benchmarkTemporaryRootShimPushCount.store(0, std::memory_order_relaxed);
    YumiBenchVoidFn reset =
        yumiBenchFunction<YumiBenchVoidFn>("yumiGCResetTemporaryRootMetrics");
    if (reset) {
        reset();
    }
}

extern "C" void
yumiBenchResetActiveTemporaryRootMetrics(void) {
    YumiBenchVoidFn reset =
        yumiBenchFunction<YumiBenchVoidFn>("yumiGCResetActiveTemporaryRootMetrics");
    if (reset) {
        reset();
    }
}

static bool
yumiBenchWriteMutatorPhases(FILE* file) {
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
        yumiBenchWriteJSONString(file, phase.name);
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
yumiBenchInitMetrics() {
    const char* path = std::getenv("YUMI_BENCH_METRICS_PATH");
    yumiBenchMetricsEnabledFlag = path && *path ? 1 : 0;
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
        phase = YumiBenchMutatorPhaseEntry {};
    }
    benchmarkMutatorPhaseCount.store(0, std::memory_order_relaxed);
    benchmarkMutatorPhaseErrorCount.store(0, std::memory_order_relaxed);
    YumiBenchVoidFn resetAllocationHistograms =
        yumiBenchFunction<YumiBenchVoidFn>(
            "yumiGCRuntimeBenchResetAllocationHistograms");
    if (resetAllocationHistograms) {
        resetAllocationHistograms();
    }
    YumiBenchVoidFn resetLifetime =
        yumiBenchFunction<YumiBenchVoidFn>("yumiGCBenchLifetimeReset");
    if (resetLifetime) {
        resetLifetime();
    }
    clock_gettime(CLOCK_MONOTONIC, &benchmarkStartTime);
}

extern "C" void
yumiBenchRecordAllocation(size_t size) {
    if (!yumiBenchMetricsEnabledFlag) {
        return;
    }
    bumpAllocatedObjects.fetch_add(1, std::memory_order_relaxed);
    bumpAllocatedBytes.fetch_add(size, std::memory_order_relaxed);
    bumpSizeBins[yumiBenchSizeBin(size)].fetch_add(1, std::memory_order_relaxed);
}

extern "C" void
yumiBenchMaybeWriteMetrics() {
    const char* path = std::getenv("YUMI_BENCH_METRICS_PATH");
    if (!path || !*path) {
        return;
    }

    const char* mode = std::getenv("YUMI_GC_MODE");
    if (!mode || !*mode) {
        mode = "normal";
    }
    const char* runNonce = std::getenv("YUMI_BENCH_RUN_NONCE");
    if (!runNonce) {
        runNonce = "";
    }
    const size_t rssPeakBytes = yumiBenchPeakRSSBytes();
    const size_t rssEndBytes = yumiBenchCounter("yumiGCRssBytes");
    const YumiBenchProcessMemory processMemory = hasRecordedProcessMemory
        ? recordedProcessMemory
        : yumiBenchProcessMemory();
    const size_t scavengeDebtBytes = yumiBenchCounter("yumiGCScavengeDebtBytes");
    YumiBenchCounterFn gcAllocatedObjectCount =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCRuntimeAllocatedObjectCount");
    YumiBenchCounterFn gcAllocatedByteCount =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCRuntimeAllocatedByteCount");
    YumiBenchCounterFn gcRequestedByteCount =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCRuntimeRequestedByteCount");
    YumiBenchCounterFn gcAllocatedSpanCount =
        yumiBenchFunction<YumiBenchCounterFn>("yumiGCRuntimeAllocatedSpanCount");
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
        yumiBenchCounter("yumiParseRegionObjectAllocationCount");
    size_t parseRegionBufferAllocationCount =
        yumiBenchCounter("yumiParseRegionBufferAllocationCount");
    size_t parseRegionPromotionCount =
        yumiBenchCounter("yumiParseRegionPromotionCount");
    size_t arenaSystemBlockAllocationCount =
        yumiBenchCounter("yumiArenaSystemBlockAllocationCount");
    size_t arenaSystemBlockFreeCount =
        yumiBenchCounter("yumiArenaSystemBlockFreeCount");
    size_t arenaActiveBytes = yumiBenchCounter("yumiArenaRuntimeActiveBytes");
    size_t arenaReservedBytes = yumiBenchCounter("yumiArenaRuntimeReservedBytes");
    size_t arenaRetainedBytes = yumiBenchCounter("yumiArenaRuntimeRetainedBytes");
    size_t arenaPeakActiveBytes = yumiBenchCounter("yumiArenaRuntimePeakActiveBytes");
    size_t arenaMarkCount = yumiBenchCounter("yumiArenaMarkCount");
    size_t arenaRewindCount = yumiBenchCounter("yumiArenaRewindCount");
    size_t arenaResetCount = yumiBenchCounter("yumiArenaResetCount");
    size_t arenaSlotCount = yumiBenchCounter("yumiArenaSlotCount");
    size_t arenaLiveSlotCount = yumiBenchCounter("yumiArenaLiveSlotCount");
    size_t arenaManagedAllocationCount =
        yumiBenchCounter("yumiArenaManagedAllocationCount");
    size_t arenaZoneTransitionCount =
        yumiBenchCounter("yumiArenaZoneTransitionCount");
    size_t gcCycleCount = hasGC
        ? yumiBenchCounter("yumiGCRuntimeCollectionCount")
        : 0;
    size_t runtimeNanos = yumiBenchMonotonicNanos();
    size_t assistNanos = yumiBenchCounter("yumiGCRuntimeAssistTimeNanos");
    size_t gcCpuNanos = yumiBenchCounter("yumiGCRuntimeBenchGCCpuNanos");
    double gcCpuPct = runtimeNanos
        ? static_cast<double>(gcCpuNanos) * 100.0 / static_cast<double>(runtimeNanos)
        : 0.0;
    size_t committedBytes = yumiBenchCounter("yumiGCCommittedBytes");
    if (!committedBytes) {
        committedBytes = yumiBenchCounter("yumiGCCommittedPageCount") * kGCPageSize;
    }
    size_t liveBytes = yumiBenchCounter("yumiGCRuntimeLiveByteCount");
    size_t heapOverheadBytes = committedBytes > liveBytes ? committedBytes - liveBytes : 0;
    double heapOverheadPct = liveBytes
        ? static_cast<double>(heapOverheadBytes) * 100.0 / static_cast<double>(liveBytes)
        : 0.0;
    size_t markWorkRetainedBytes = hasGC
        ? yumiBenchCounter("yumiGCRuntimeMarkWorkRetainedBytes")
        : 0;
    size_t spanMetadataBytes = hasGC
        ? yumiBenchCounter("yumiGCRuntimeSpanMetadataBytes")
        : 0;
    size_t addressIndexMetadataBytes = hasGC
        ? yumiBenchCounter("yumiGCRuntimeAddressIndexMetadataBytes")
        : 0;
    size_t gcMetadataBytes = markWorkRetainedBytes + spanMetadataBytes +
        addressIndexMetadataBytes;
    size_t fragmentationBytes = hasGC
        ? yumiBenchCounter("yumiGCFragmentationBytes")
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
        yumiBenchCounter("yumiGCRuntimeFrameBudgetOfferCount");
    size_t frameBudgetWorkCount =
        yumiBenchCounter("yumiGCRuntimeFrameBudgetWorkCount");
    size_t frameBudgetWorkNanos =
        yumiBenchCounter("yumiGCRuntimeFrameBudgetWorkNanos");
    size_t frameBudgetOverrunCount =
        yumiBenchCounter("yumiGCRuntimeFrameBudgetOverrunCount");
    size_t frameBudgetMaxOverrunNanos =
        yumiBenchCounter("yumiGCRuntimeFrameBudgetMaxOverrunNanos");
    size_t memoryReturnNanos = yumiBenchCounter("yumiGCRuntimeScavengeMaxNanos");
    size_t allocationRefillCount =
        yumiBenchCounter("yumiGCRuntimeAllocationFastRefillCount");
    size_t batchReleasedBytes = yumiBenchCounter("yumiGCRuntimeBatchReleasedBytes");
    size_t batchDiscardedBytes = yumiBenchCounter("yumiGCRuntimeBatchDiscardedBytes");
    size_t adaptiveBatchGrowthCount =
        yumiBenchCounter("yumiGCRuntimeAdaptiveBatchGrowthCount");
    size_t adaptiveBatchShrinkCount =
        yumiBenchCounter("yumiGCRuntimeAdaptiveBatchShrinkCount");
    size_t adaptiveBatchReturnedUnusedSlotCount =
        yumiBenchCounter("yumiGCRuntimeAdaptiveBatchReturnedUnusedSlotCount");
    size_t compilerIndexedRefillCount =
        yumiBenchCounter("yumiGCRuntimeCompilerIndexedRefillCount");
    size_t activeBufferRefillCount =
        yumiBenchCounter("yumiGCRuntimeActiveBufferRefillCount");
    size_t genericDescribedRefillCount =
        yumiBenchCounter("yumiGCRuntimeGenericDescribedRefillCount");
    size_t genericDescriptorlessRefillCount =
        yumiBenchCounter("yumiGCRuntimeGenericDescriptorlessRefillCount");
    size_t compilerBufferSiteSwitchCount =
        yumiBenchCounter("yumiGCRuntimeCompilerBufferSiteSwitchCount");
    size_t fullAllocationRangeFlushCount =
        yumiBenchCounter("yumiGCRuntimeFullAllocationRangeFlushCount");
    size_t fullAllocationRangeFlushedBufferCount =
        yumiBenchCounter("yumiGCRuntimeFullAllocationRangeFlushedBufferCount");
    size_t batchCachePressureFlushCount =
        yumiBenchCounter("yumiGCRuntimeBatchCachePressureFlushCount");
    size_t allocationBatchMissGeneration =
        yumiBenchIndexedCounter("yumiGCRuntimeAllocationBatchMissCount", 0);
    size_t allocationBatchMissDescriptor =
        yumiBenchIndexedCounter("yumiGCRuntimeAllocationBatchMissCount", 1);
    size_t allocationBatchMissRequestedSize =
        yumiBenchIndexedCounter("yumiGCRuntimeAllocationBatchMissCount", 2);
    size_t allocationBatchMissEmpty =
        yumiBenchIndexedCounter("yumiGCRuntimeAllocationBatchMissCount", 3);
    size_t allocationBatchMissCap =
        yumiBenchIndexedCounter("yumiGCRuntimeAllocationBatchMissCount", 4);
    size_t rootScanSampleCount = yumiBenchCounter("yumiGCRuntimeRootScanSampleCount");
    size_t rootScanP95Nanos = yumiBenchCounter("yumiGCRuntimeRootScanP95Nanos");
    size_t rootScanP99Nanos = yumiBenchCounter("yumiGCRuntimeRootScanP99Nanos");
    size_t rootScanMaxNanos = yumiBenchCounter("yumiGCRuntimeRootScanMaxNanos");
    size_t rootRemoveCount = yumiBenchCounter("yumiGCTestRootUnlinkCount");
    size_t externalRootLiveCount = yumiBenchCounter("yumiGCTestRootRegistrationCount");
    size_t rootAddCount = rootRemoveCount + externalRootLiveCount;
    size_t temporaryRootPushCount =
        yumiBenchCounter("yumiGCTemporaryRootPushCount");
    size_t temporaryRootShimPushCount =
        benchmarkTemporaryRootShimPushCount.load(std::memory_order_relaxed);
    size_t temporaryRootPopCount =
        yumiBenchCounter("yumiGCTemporaryRootPopCount");
    size_t temporaryRootStorageAllocationCount =
        yumiBenchCounter("yumiGCTemporaryRootStorageAllocationCount");
    size_t temporaryRootSlabGrowthCount =
        yumiBenchCounter("yumiGCTemporaryRootSlabGrowthCount");
    size_t temporaryRootMaxDepth =
        yumiBenchCounter("yumiGCTemporaryRootMaxDepth");
    size_t cleanupSidePublishCount =
        yumiBenchCounter("yumiGCCleanupSidePublishCount");
    size_t cleanupSideValidationNanos =
        yumiBenchCounter("yumiGCCleanupSideValidationNanos");
    size_t cleanupSideValidationObjectCount =
        yumiBenchCounter("yumiGCCleanupSideValidationObjectCount");
    size_t cleanupSideValidationEdgeCount =
        yumiBenchCounter("yumiGCCleanupSideValidationEdgeCount");
    size_t barrierCallCount = yumiBenchCounter("yumiGCRuntimeFastBarrierCallCount");
    size_t barrierSlowEntryCount =
        yumiBenchCounter("yumiGCRuntimeFastBarrierSlowEntryCount");
    size_t bufferedBarrierSharedRMWCount =
        yumiBenchCounter("yumiGCRuntimeBufferedBarrierSharedRMWCount");
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
        yumiBenchCounter("yumiGCRuntimeMode"),
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
    size_t histogramCount = yumiBenchWriteSizeHistogram(file, hasGC);
    std::fprintf(
        file,
        ",\"allocationSizeHistogramBasis\":\"requestedBytes\","
        "\"allocationSizeHistogramCount\":%zu,"
        "\"allocationSizeHistogramComplete\":%s,",
        histogramCount,
        histogramCount == allocationCount ? "true" : "false");
    YumiBenchSizeClassSummary sizeClasses =
        yumiBenchWriteSizeClassHistogram(file, hasGC);
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
    YumiBenchAllocationSiteSummary allocationSites =
        yumiBenchWriteAllocationSites(file, runtimeNanos);
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
    size_t allocationBufferCount = yumiBenchWriteAllocationBuffers(file, runtimeNanos);
    if (hasGC) {
        std::fprintf(file, ",\"allocationBufferCount\":%zu", allocationBufferCount);
    }
    std::fprintf(file, ",");
    yumiBenchWriteThreadRefills(file);
    std::fprintf(file, ",");
    bool mutatorPhasesComplete = yumiBenchWriteMutatorPhases(file);
    std::fprintf(file, ",");
    bool lifetimeInvariantValid = yumiBenchWriteLifetimeHistogram(file, mode);
    std::fprintf(file, ",");
    YumiBenchPauseTraceSummary pauseTrace =
        yumiBenchWritePauseTrace(file, pauseDurations);
    std::fprintf(file, ",");
    yumiBenchWriteCycleSummaries(file);

    size_t pauseP50 = yumiBenchPercentile(pauseDurations, 50);
    size_t pauseP99 = yumiBenchPercentile(pauseDurations, 99);
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
        yumiBenchCounter("yumiGCRuntimeAssistCount"),
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
