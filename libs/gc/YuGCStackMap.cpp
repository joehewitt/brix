#include <YuGC/YuGCStackMap.h>
#include <YuGC/YuGCAllocationBuffer.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <ctime>
#include <mutex>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/ldsyms.h>
#include <mach-o/loader.h>
#endif

// *************************************************************************************************

extern "C" void* yumiGetGCStackMapPointer(size_t* out_size);
extern "C" void* yumiGetGCRootMapPointer(size_t* out_size);
extern "C" bool yumiGCValidateAllocationABI(void);

// *************************************************************************************************

static const uint32_t YuGCMapMagic = 0x4d434759; // YGCM
static const uint16_t YuGCMapVersion = 3;
static const uint32_t YuGCRootMapMagic = 0x52594359; // YCYR
static const uint16_t YuGCRootMapVersion = 1;

#pragma pack(push, 1)
struct YuGCMapHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t pointerSize;
    uint32_t functionCount;
    uint32_t safepointCount;
    uint32_t locationCount;
    uint32_t savedRegisterCount;
    uint32_t descriptorCount;
    uint32_t reserved;
};

struct YuGCMapFunction {
    uintptr_t address;
    uint32_t byteSize;
    uint32_t firstSafepoint;
    uint32_t safepointCount;
    uint32_t firstSavedRegister;
    uint32_t savedRegisterCount;
    uint32_t reserved;
};

struct YuGCMapSafepoint {
    uint32_t pcOffset;
    uint32_t firstLocation;
    uint32_t locationCount;
};

struct YuGCMapLocation {
    uint8_t kind;
    uint8_t registerNumber;
    uint16_t reserved;
    int32_t offset;
    uint32_t descriptorIndex;
};

struct YuGCMapSavedRegister {
    uint8_t registerNumber;
    uint8_t reserved8;
    uint16_t reserved16;
    int32_t fpOffset;
};

struct YuGCRootMapHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t pointerSize;
    uint32_t recordCount;
    uint32_t reserved;
};

struct YuGCRootMapRecord {
    uint8_t kind;
    uint8_t reserved8;
    uint16_t reserved16;
    uint32_t reserved32;
    uintptr_t base;
    uintptr_t descriptor;
};
#pragma pack(pop)

struct YuGCMapView {
    const YuGCMapHeader* header;
    const uintptr_t* descriptors;
    const YuGCMapFunction* functions;
    const YuGCMapSafepoint* safepoints;
    const YuGCMapLocation* locations;
    const YuGCMapSavedRegister* savedRegisters;
    bool normalizeImagePointers;
};

struct YuGCSafepointIndexEntry {
    uint32_t pcOffset;
    uint32_t ordinal;
    const YuGCMapSafepoint* safepoint;
};

struct YuGCFunctionSafepointIndex {
    const YuGCMapFunction* function;
    std::vector<YuGCSafepointIndexEntry> safepoints;
};

struct YuGCFunctionIndexEntry {
    const YuGCMapView* map;
    const YuGCMapFunction* function;
    size_t safepointIndex;
    size_t ordinal;
    uintptr_t start;
    uintptr_t end;
};

struct YuGCMapSet {
    std::vector<YuGCMapView> maps;
    std::vector<YuGCFunctionIndexEntry> functions;
    std::vector<uintptr_t> functionPrefixMaxEnds;
    std::vector<YuGCFunctionSafepointIndex> safepoints;
    uint64_t generation;
    bool hasOverlappingFunctions;
};

struct YuGCFrameLookup {
    const YuGCMapView* map;
    const YuGCMapFunction* function;
    const YuGCMapSafepoint* safepoint;
};

struct YuGCReturnPCCacheEntry {
    uintptr_t pc;
    uint64_t generation;
    YuGCFrameLookup lookup;
    bool valid;
};

struct YuGCRegisterBank {
    uintptr_t x[29];
};

struct YuGCFrameRecord {
    YuGCFrameRecord* previous;
    void* returnAddress;
};

struct YuGCRootMapView {
    const YuGCRootMapHeader* header;
    const YuGCRootMapRecord* records;
};

struct YuGCRootMapSet {
    std::vector<YuGCRootMapView> maps;
};

static std::once_flag gGCMapSetOnce;
static YuGCMapSet gGCMapSet;
static bool gGCMapSetLoaded = false;
static std::once_flag gGCRootMapSetOnce;
static YuGCRootMapSet gGCRootMapSet;
static bool gGCRootMapSetLoaded = false;
static uint64_t gGCMapSetGeneration = 0;

static constexpr size_t kYuGCReturnPCCacheSize = 16;
static constexpr size_t kYuGCSubphaseStackMapLookup = 11;
static thread_local YuGCReturnPCCacheEntry gGCReturnPCCache[kYuGCReturnPCCacheSize] = {};
static thread_local void* gGCConservativeRootSlot = NULL;
static thread_local bool gGCVisitingConservativeRoot = false;

extern "C" void
yumiGCRuntimeRecordSubphaseDuration(
    size_t subphase,
    size_t cycle,
    size_t durationNanos,
    size_t value) __attribute__((weak));

extern "C" void
yumiGCRuntimeRecordSubphaseDuration(
    size_t,
    size_t,
    size_t,
    size_t
) {
}

extern "C" size_t
yumiGCRuntimeCurrentCycleId(void) __attribute__((weak));

extern "C" size_t
yumiGCRuntimeCurrentCycleId(void) {
    return 0;
}

static bool
yumiGCValidateStackMaps() {
    static const bool enabled = []() {
        const char* value = std::getenv("YUMI_GC_VALIDATE_STACKMAPS");
        return value && *value && std::strcmp(value, "0") != 0;
    }();
    return enabled;
}

static bool
yumiGCAllowMissingStackMaps() {
    static const bool enabled = []() {
        const char* value = std::getenv("YUMI_GC_ALLOW_MISSING_STACKMAPS");
        const char* testMode = std::getenv("YUMI_GC_TEST_ALLOW_MISSING_STACKMAPS");
        return
            value &&
            *value &&
            std::strcmp(value, "0") != 0 &&
            testMode &&
            *testMode &&
            std::strcmp(testMode, "0") != 0;
    }();
    return enabled;
}

static bool
yumiGCShouldAbortOnMapFailure() {
    return yumiGCValidateStackMaps() || !yumiGCAllowMissingStackMaps();
}

static bool
yumiGCAlwaysScanConservativeStacks() {
    static const bool enabled = []() {
        const char* value = std::getenv("YUMI_GC_ALWAYS_CONSERVATIVE_STACKS");
        return value && *value && std::strcmp(value, "0") != 0;
    }();
    return enabled;
}

static void
yumiGCValidationAbort(const char* message) {
    std::fprintf(stderr, "Yumi GC stack-map validation failed: %s\n", message);
    std::abort();
}

static const char*
yumiGCSymbolName(uintptr_t address) {
    Dl_info info;
    if (dladdr(reinterpret_cast<const void*>(address), &info) && info.dli_sname) {
        return info.dli_sname;
    }
    return "<unknown>";
}

static bool
yumiGCCheckedAddSize(size_t* total, size_t count, size_t itemSize) {
    if (itemSize != 0 && count > (static_cast<size_t>(-1) - *total) / itemSize) {
        return false;
    }
    *total += count * itemSize;
    return true;
}

static bool
yumiGCRangeFits(uint32_t first, uint32_t count, uint32_t total) {
    return first <= total && count <= total - first;
}

static bool
yumiGCAddressRangeFits(uintptr_t address, uint32_t byteSize) {
    return byteSize != 0 && address <= static_cast<uintptr_t>(-1) - byteSize;
}

static uintptr_t
yumiGCMainImageBase() {
#if defined(__APPLE__)
    return reinterpret_cast<uintptr_t>(&_mh_execute_header);
#else
    return 0;
#endif
}

static intptr_t
yumiGCMainImageSlide() {
#if defined(__APPLE__)
    static const intptr_t slide = []() -> intptr_t {
        const char* command = reinterpret_cast<const char*>(&_mh_execute_header + 1);
        for (uint32_t index = 0; index < _mh_execute_header.ncmds; ++index) {
            const load_command* load = reinterpret_cast<const load_command*>(command);
            if (load->cmd == LC_SEGMENT_64) {
                const segment_command_64* segment =
                    reinterpret_cast<const segment_command_64*>(command);
                if (std::strncmp(segment->segname, SEG_TEXT, sizeof(segment->segname)) == 0) {
                    return static_cast<intptr_t>(yumiGCMainImageBase() - segment->vmaddr);
                }
            }
            command += load->cmdsize;
        }
        return 0;
    }();
    return slide;
#else
    return 0;
#endif
}

static uintptr_t
yumiGCNormalizeImagePointer(uintptr_t value) {
    if (!value) {
        return 0;
    }

    uintptr_t imageBase = yumiGCMainImageBase();
    if (!imageBase) {
        return value;
    }

    // Some Mach-O data relocations in the custom GC map can arrive as image-relative
    // addresses. Convert them to the runtime address before the GC treats them as
    // function or descriptor pointers.
    static const uintptr_t kPreferredMachOImageBase = 0x100000000ull;
    if (value < kPreferredMachOImageBase) {
        return imageBase + value;
    }

    intptr_t slide = yumiGCMainImageSlide();
    if (!slide) {
        return value;
    }

    uintptr_t preferredBase = static_cast<uintptr_t>(
        static_cast<intptr_t>(imageBase) - slide);
    if (slide > 0 && value >= preferredBase && value < imageBase) {
        return static_cast<uintptr_t>(static_cast<intptr_t>(value) + slide);
    }
    if (slide < 0 && value >= imageBase && value < preferredBase) {
        return static_cast<uintptr_t>(static_cast<intptr_t>(value) + slide);
    }
    return value;
}

static uintptr_t
yumiGCNormalizeMapPointer(const YuGCMapView& map, uintptr_t value) {
    return map.normalizeImagePointers ? yumiGCNormalizeImagePointer(value) : value;
}

static uintptr_t
yumiGCFunctionAddress(const YuGCMapView& map, const YuGCMapFunction& function) {
    return yumiGCNormalizeMapPointer(map, function.address);
}

static uint64_t
yumiGCMonotonicNanos() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void
yumiGCRecordStackMapLookupDuration(uint64_t startNanos, size_t value) {
    if (!yumiGCRuntimeRecordSubphaseDuration) {
        return;
    }

    uint64_t endNanos = yumiGCMonotonicNanos();
    if (endNanos <= startNanos) {
        return;
    }
    size_t durationNanos = static_cast<size_t>(endNanos - startNanos);
    size_t cycle = yumiGCRuntimeCurrentCycleId ? yumiGCRuntimeCurrentCycleId() : 0;
    yumiGCRuntimeRecordSubphaseDuration(
        kYuGCSubphaseStackMapLookup,
        cycle,
        durationNanos,
        value);
}

static bool
yumiGCCompareStackMapLookupEnabled() {
    static const bool enabled = []() {
        const char* value = std::getenv("YUMI_GC_COMPARE_STACKMAP_LOOKUP");
        return value && *value && std::strcmp(value, "0") != 0;
    }();
    return enabled;
}

static const char*
yumiGCSkipMapPadding(const char* cursor, const char* end, uint32_t magic) {
    while (cursor < end) {
        if (static_cast<size_t>(end - cursor) >= sizeof(uint32_t)) {
            uint32_t nextMagic = 0;
            std::memcpy(&nextMagic, cursor, sizeof(nextMagic));
            if (nextMagic == magic) {
                break;
            }
        }
        if (*cursor != 0) {
            break;
        }
        ++cursor;
    }
    return cursor;
}

static bool
yumiGCParseMap(const char* cursor, size_t size, YuGCMapView* out, size_t* consumed) {
    if (size < sizeof(YuGCMapHeader)) {
        return false;
    }

    const YuGCMapHeader* header = reinterpret_cast<const YuGCMapHeader*>(cursor);
    if (
        header->magic != YuGCMapMagic ||
        header->version != YuGCMapVersion ||
        header->pointerSize != sizeof(void*) ||
        header->reserved != YumiGCAllocationABIVersion
    ) {
        return false;
    }

    size_t expectedSize = sizeof(YuGCMapHeader);
    if (
        !yumiGCCheckedAddSize(&expectedSize, header->descriptorCount, sizeof(uintptr_t)) ||
        !yumiGCCheckedAddSize(&expectedSize, header->functionCount, sizeof(YuGCMapFunction)) ||
        !yumiGCCheckedAddSize(&expectedSize, header->safepointCount, sizeof(YuGCMapSafepoint)) ||
        !yumiGCCheckedAddSize(&expectedSize, header->locationCount, sizeof(YuGCMapLocation)) ||
        !yumiGCCheckedAddSize(
            &expectedSize,
            header->savedRegisterCount,
            sizeof(YuGCMapSavedRegister))
    ) {
        return false;
    }

    if (size < expectedSize) {
        return false;
    }

    cursor += sizeof(YuGCMapHeader);
    out->header = header;
    out->descriptors = reinterpret_cast<const uintptr_t*>(cursor);
    cursor += header->descriptorCount * sizeof(uintptr_t);
    out->functions = reinterpret_cast<const YuGCMapFunction*>(cursor);
    cursor += header->functionCount * sizeof(YuGCMapFunction);
    out->safepoints = reinterpret_cast<const YuGCMapSafepoint*>(cursor);
    cursor += header->safepointCount * sizeof(YuGCMapSafepoint);
    out->locations = reinterpret_cast<const YuGCMapLocation*>(cursor);
    cursor += header->locationCount * sizeof(YuGCMapLocation);
    out->savedRegisters = reinterpret_cast<const YuGCMapSavedRegister*>(cursor);
    out->normalizeImagePointers = false;

    for (uint32_t functionIndex = 0; functionIndex < header->functionCount; ++functionIndex) {
        const YuGCMapFunction& function = out->functions[functionIndex];
        if (function.address >= 0x100000000ull) {
            out->normalizeImagePointers = true;
        }
    }

    *consumed = expectedSize;
    return true;
}

static bool
yumiGCFunctionMapValid(const YuGCMapView& map, const YuGCMapFunction& function) {
    if (!yumiGCAddressRangeFits(function.address, function.byteSize) ||
            !yumiGCRangeFits(
                function.firstSafepoint,
                function.safepointCount,
                map.header->safepointCount) ||
            !yumiGCRangeFits(
                function.firstSavedRegister,
                function.savedRegisterCount,
                map.header->savedRegisterCount)) {
        return false;
    }

    for (uint32_t i = 0; i < function.safepointCount; ++i) {
        const YuGCMapSafepoint& safepoint =
            map.safepoints[function.firstSafepoint + i];
        if (safepoint.pcOffset >= function.byteSize ||
                !yumiGCRangeFits(
                    safepoint.firstLocation,
                    safepoint.locationCount,
                    map.header->locationCount)) {
            return false;
        }
        for (uint32_t j = 0; j < safepoint.locationCount; ++j) {
            const YuGCMapLocation& location =
                map.locations[safepoint.firstLocation + j];
            if (location.kind < 1 ||
                    location.kind > 4 ||
                    (location.kind == 2 && location.registerNumber >= 29) ||
                    location.descriptorIndex > map.header->descriptorCount) {
                return false;
            }
        }
    }

    for (uint32_t i = 0; i < function.savedRegisterCount; ++i) {
        const YuGCMapSavedRegister& saved =
            map.savedRegisters[function.firstSavedRegister + i];
        if (saved.registerNumber >= 29) {
            return false;
        }
    }
    return true;
}

static void
yumiGCBuildMapSetIndexes(YuGCMapSet* out) {
    out->functions.clear();
    out->functionPrefixMaxEnds.clear();
    out->safepoints.clear();

    size_t functionCount = 0;
    for (const YuGCMapView& map : out->maps) {
        functionCount += map.header->functionCount;
    }
    out->functions.reserve(functionCount);
    out->functionPrefixMaxEnds.reserve(functionCount);
    out->safepoints.reserve(functionCount);

    size_t ordinal = 0;
    for (const YuGCMapView& map : out->maps) {
        for (uint32_t functionIndex = 0; functionIndex < map.header->functionCount; ++functionIndex) {
            const YuGCMapFunction& function = map.functions[functionIndex];
            if (!yumiGCFunctionMapValid(map, function)) {
                std::fprintf(
                    stderr,
                    "Yumi GC rejected invalid stack-map function: address=0x%llx ordinal=%u\n",
                    (unsigned long long)yumiGCFunctionAddress(map, function),
                    functionIndex);
                continue;
            }
            YuGCFunctionSafepointIndex safepointIndex;
            safepointIndex.function = &function;
            safepointIndex.safepoints.reserve(function.safepointCount);

            for (uint32_t i = 0; i < function.safepointCount; ++i) {
                const YuGCMapSafepoint& safepoint =
                    map.safepoints[function.firstSafepoint + i];
                safepointIndex.safepoints.push_back({
                    safepoint.pcOffset,
                    i,
                    &safepoint
                });
            }
            std::sort(
                safepointIndex.safepoints.begin(),
                safepointIndex.safepoints.end(),
                [](const YuGCSafepointIndexEntry& left, const YuGCSafepointIndexEntry& right) {
                    if (left.pcOffset != right.pcOffset) {
                        return left.pcOffset < right.pcOffset;
                    }
                    return left.ordinal < right.ordinal;
                });

            size_t index = out->safepoints.size();
            out->safepoints.push_back(std::move(safepointIndex));
            out->functions.push_back({
                &map,
                &function,
                index,
                ordinal++,
                yumiGCFunctionAddress(map, function),
                yumiGCFunctionAddress(map, function) + function.byteSize
            });
        }
    }

    std::sort(
        out->functions.begin(),
        out->functions.end(),
        [](const YuGCFunctionIndexEntry& left, const YuGCFunctionIndexEntry& right) {
            if (left.start != right.start) {
                return left.start < right.start;
            }
            if (left.end != right.end) {
                return left.end < right.end;
            }
            return left.function < right.function;
        });

    out->hasOverlappingFunctions = false;
    uintptr_t maxEnd = 0;
    bool haveFunction = false;
    for (const YuGCFunctionIndexEntry& function : out->functions) {
        if (haveFunction && function.start < maxEnd) {
            out->hasOverlappingFunctions = true;
        }
        if (!haveFunction || function.end > maxEnd) {
            maxEnd = function.end;
        }
        out->functionPrefixMaxEnds.push_back(maxEnd);
        haveFunction = true;
    }

    out->generation = ++gGCMapSetGeneration;
    if (!out->generation) {
        out->generation = ++gGCMapSetGeneration;
    }
}

static bool
yumiGCLoadMapSetUncached(YuGCMapSet* out) {
    out->maps.clear();
    out->functions.clear();
    out->functionPrefixMaxEnds.clear();
    out->safepoints.clear();
    out->generation = 0;
    out->hasOverlappingFunctions = false;
    out->maps.reserve(1);

    size_t size = 0;
    void* data = yumiGetGCStackMapPointer(&size);
    if (!data || size < sizeof(YuGCMapHeader)) {
        if (yumiGCShouldAbortOnMapFailure()) {
            yumiGCValidationAbort("missing or truncated __yumi_gcmap section");
        }
        return false;
    }

    const char* cursor = static_cast<const char*>(data);
    const char* end = cursor + size;
    while (cursor < end) {
        cursor = yumiGCSkipMapPadding(cursor, end, YuGCMapMagic);
        if (cursor >= end) {
            break;
        }

        YuGCMapView map;
        size_t consumed = 0;
        if (!yumiGCParseMap(cursor, static_cast<size_t>(end - cursor), &map, &consumed)) {
            if (yumiGCShouldAbortOnMapFailure()) {
                yumiGCValidationAbort("invalid or truncated __yumi_gcmap payload");
            }
            return false;
        }
        out->maps.push_back(map);
        cursor += consumed;
    }

    if (out->maps.empty()) {
        if (yumiGCShouldAbortOnMapFailure()) {
            yumiGCValidationAbort("missing __yumi_gcmap records");
        }
        return false;
    }
    yumiGCBuildMapSetIndexes(out);
    return true;
}

static bool
yumiGCParseRootMap(const char* cursor, size_t size, YuGCRootMapView* out, size_t* consumed) {
    if (size < sizeof(YuGCRootMapHeader)) {
        return false;
    }

    const YuGCRootMapHeader* header = reinterpret_cast<const YuGCRootMapHeader*>(cursor);
    if (
        header->magic != YuGCRootMapMagic ||
        header->version != YuGCRootMapVersion ||
        header->pointerSize != sizeof(void*)
    ) {
        return false;
    }

    size_t expectedSize = sizeof(YuGCRootMapHeader);
    if (!yumiGCCheckedAddSize(&expectedSize, header->recordCount, sizeof(YuGCRootMapRecord))) {
        return false;
    }

    if (size < expectedSize) {
        return false;
    }

    out->header = header;
    out->records = reinterpret_cast<const YuGCRootMapRecord*>(
        cursor + sizeof(YuGCRootMapHeader));
    *consumed = expectedSize;
    return true;
}

static bool
yumiGCLoadRootMapSetUncached(YuGCRootMapSet* out) {
    out->maps.clear();
    out->maps.reserve(1);

    size_t size = 0;
    void* data = yumiGetGCRootMapPointer(&size);
    if (!data || size < sizeof(YuGCRootMapHeader)) {
        if (yumiGCShouldAbortOnMapFailure()) {
            yumiGCValidationAbort("missing or truncated __yumi_gcroots section");
        }
        return false;
    }

    const char* cursor = static_cast<const char*>(data);
    const char* end = cursor + size;
    while (cursor < end) {
        cursor = yumiGCSkipMapPadding(cursor, end, YuGCRootMapMagic);
        if (cursor >= end) {
            break;
        }

        YuGCRootMapView map;
        size_t consumed = 0;
        if (!yumiGCParseRootMap(cursor, static_cast<size_t>(end - cursor), &map, &consumed)) {
            if (yumiGCShouldAbortOnMapFailure()) {
                yumiGCValidationAbort("invalid or truncated __yumi_gcroots payload");
            }
            return false;
        }
        out->maps.push_back(map);
        cursor += consumed;
    }

    if (out->maps.empty()) {
        if (yumiGCShouldAbortOnMapFailure()) {
            yumiGCValidationAbort("missing __yumi_gcroots records");
        }
        return false;
    }
    return true;
}

static void
yumiGCInitMapSetOnce() {
    gGCMapSetLoaded = yumiGCLoadMapSetUncached(&gGCMapSet);
}

static bool
yumiGCGetMapSet(const YuGCMapSet** out) {
    std::call_once(gGCMapSetOnce, yumiGCInitMapSetOnce);
    if (!gGCMapSetLoaded) {
        return false;
    }
    *out = &gGCMapSet;
    return true;
}

static void
yumiGCInitRootMapSetOnce() {
    gGCRootMapSetLoaded = yumiGCLoadRootMapSetUncached(&gGCRootMapSet);
}

static bool
yumiGCGetRootMapSet(const YuGCRootMapSet** out) {
    std::call_once(gGCRootMapSetOnce, yumiGCInitRootMapSetOnce);
    if (!gGCRootMapSetLoaded) {
        return false;
    }
    *out = &gGCRootMapSet;
    return true;
}

static const YuGCMapFunction*
yumiGCFindFunctionLinear(const YuGCMapView& map, uintptr_t pc) {
    for (uint32_t index = 0; index < map.header->functionCount; ++index) {
        const YuGCMapFunction& function = map.functions[index];
        uintptr_t start = yumiGCFunctionAddress(map, function);
        uintptr_t end = start + function.byteSize;
        if (start <= pc && pc < end) {
            return &function;
        }
    }
    return NULL;
}

static const YuGCMapSafepoint*
yumiGCFindSafepointLinear(const YuGCMapView& map, const YuGCMapFunction* function, uintptr_t pc) {
    if (!function) {
        return NULL;
    }

    uintptr_t functionAddress = yumiGCFunctionAddress(map, *function);
    if (pc < functionAddress) {
        return NULL;
    }

    uint32_t pcOffset = static_cast<uint32_t>(pc - functionAddress);
    for (uint32_t i = 0; i < function->safepointCount; ++i) {
        const YuGCMapSafepoint& safepoint = map.safepoints[function->firstSafepoint + i];
        if (safepoint.pcOffset == pcOffset) {
            return &safepoint;
        }
    }
    return NULL;
}

static const YuGCMapSafepoint*
yumiGCFindSafepointIndexed(
    const YuGCMapSet& mapSet,
    const YuGCFunctionIndexEntry& functionEntry,
    uintptr_t pc
) {
    if (functionEntry.safepointIndex >= mapSet.safepoints.size() || pc < functionEntry.start) {
        return NULL;
    }

    const YuGCFunctionSafepointIndex& index = mapSet.safepoints[functionEntry.safepointIndex];
    if (!index.function || index.function != functionEntry.function) {
        return NULL;
    }

    uint32_t pcOffset = static_cast<uint32_t>(pc - functionEntry.start);
    auto found = std::lower_bound(
        index.safepoints.begin(),
        index.safepoints.end(),
        pcOffset,
        [](const YuGCSafepointIndexEntry& entry, uint32_t offset) {
            return entry.pcOffset < offset;
        });
    if (found != index.safepoints.end() && found->pcOffset == pcOffset) {
        return found->safepoint;
    }
    return NULL;
}

static YuGCFrameLookup
yumiGCFindFrameIndexedUncached(const YuGCMapSet& mapSet, uintptr_t pc) {
    YuGCFrameLookup result { NULL, NULL, NULL };
    if (mapSet.functions.empty()) {
        return result;
    }

    auto found = std::upper_bound(
        mapSet.functions.begin(),
        mapSet.functions.end(),
        pc,
        [](uintptr_t address, const YuGCFunctionIndexEntry& entry) {
            return address < entry.start;
        });
    if (found == mapSet.functions.begin()) {
        return result;
    }

    const YuGCFunctionIndexEntry* functionEntry = NULL;
    if (!mapSet.hasOverlappingFunctions) {
        --found;
        if (pc < found->start || pc >= found->end) {
            return result;
        }
        functionEntry = &*found;
    } else {
        size_t bestOrdinal = static_cast<size_t>(-1);
        size_t index = static_cast<size_t>(found - mapSet.functions.begin());
        while (found != mapSet.functions.begin()) {
            --found;
            --index;
            if (pc >= found->start && pc < found->end && found->ordinal < bestOrdinal) {
                functionEntry = &*found;
                bestOrdinal = found->ordinal;
            }
            if (index == 0 || mapSet.functionPrefixMaxEnds[index - 1] <= pc) {
                break;
            }
        }
        if (!functionEntry) {
            return result;
        }
    }

    result.map = functionEntry->map;
    result.function = functionEntry->function;
    result.safepoint = yumiGCFindSafepointIndexed(mapSet, *functionEntry, pc);
    return result;
}

static YuGCFrameLookup
yumiGCFindFrameIndexed(const YuGCMapSet& mapSet, uintptr_t pc, bool* cacheHit) {
    if (cacheHit) {
        *cacheHit = false;
    }

    size_t cacheIndex = (pc >> 4) & (kYuGCReturnPCCacheSize - 1);
    YuGCReturnPCCacheEntry& cached = gGCReturnPCCache[cacheIndex];
    if (cached.valid && cached.generation == mapSet.generation && cached.pc == pc) {
        if (cacheHit) {
            *cacheHit = true;
        }
        return cached.lookup;
    }

    YuGCFrameLookup lookup = yumiGCFindFrameIndexedUncached(mapSet, pc);
    cached.pc = pc;
    cached.generation = mapSet.generation;
    cached.lookup = lookup;
    cached.valid = true;
    return lookup;
}

static YuGCFrameLookup
yumiGCFindFrameLinear(const YuGCMapSet& mapSet, uintptr_t pc) {
    YuGCFrameLookup result { NULL, NULL, NULL };
    for (const YuGCMapView& map : mapSet.maps) {
        const YuGCMapFunction* function = yumiGCFindFunctionLinear(map, pc);
        if (!function) {
            continue;
        }

        result.map = &map;
        result.function = function;
        result.safepoint = yumiGCFindSafepointLinear(map, function, pc);
        return result;
    }
    return result;
}

static bool
yumiGCFrameLookupsEqual(const YuGCFrameLookup& left, const YuGCFrameLookup& right);

static YuGCFrameLookup
yumiGCFindFrame(const YuGCMapSet& mapSet, uintptr_t pc, bool* cacheHit = NULL) {
    uint64_t startNanos = yumiGCMonotonicNanos();
    YuGCFrameLookup lookup = yumiGCFindFrameIndexed(mapSet, pc, cacheHit);
    yumiGCRecordStackMapLookupDuration(startNanos, (cacheHit && *cacheHit) ? 1 : 0);
    if (yumiGCCompareStackMapLookupEnabled()) {
        YuGCFrameLookup linear = yumiGCFindFrameLinear(mapSet, pc);
        if (!yumiGCFrameLookupsEqual(lookup, linear)) {
            std::fprintf(
                stderr,
                "Yumi GC stack-map lookup mismatch pc=0x%llx indexed=(map=%p function=%p safepoint=%p) linear=(map=%p function=%p safepoint=%p)\n",
                (unsigned long long)pc,
                (const void*)lookup.map,
                (const void*)lookup.function,
                (const void*)lookup.safepoint,
                (const void*)linear.map,
                (const void*)linear.function,
                (const void*)linear.safepoint);
            std::abort();
        }
    }
    return lookup;
}

static bool
yumiGCFrameLookupsEqual(const YuGCFrameLookup& left, const YuGCFrameLookup& right) {
    return
        left.map == right.map &&
        left.function == right.function &&
        left.safepoint == right.safepoint;
}

extern "C" void
yumiGCDebugClearStackMapLookupCache() {
    for (size_t i = 0; i < kYuGCReturnPCCacheSize; ++i) {
        gGCReturnPCCache[i] = YuGCReturnPCCacheEntry {};
    }
}

extern "C" size_t
yumiGCDebugStackMapFunctionIndexCount() {
    const YuGCMapSet* mapSet = NULL;
    if (!yumiGCGetMapSet(&mapSet)) {
        return 0;
    }
    return mapSet->functions.size();
}

extern "C" bool
yumiGCDebugStackMapLookupSelfTest() {
    const YuGCMapSet* mapSet = NULL;
    if (!yumiGCGetMapSet(&mapSet)) {
        return false;
    }

    for (const YuGCMapView& map : mapSet->maps) {
        for (uint32_t functionIndex = 0; functionIndex < map.header->functionCount; ++functionIndex) {
            const YuGCMapFunction& function = map.functions[functionIndex];
            uintptr_t start = yumiGCFunctionAddress(map, function);
            if (!yumiGCFrameLookupsEqual(
                    yumiGCFindFrameIndexedUncached(*mapSet, start),
                    yumiGCFindFrameLinear(*mapSet, start))) {
                return false;
            }

            for (uint32_t i = 0; i < function.safepointCount; ++i) {
                const YuGCMapSafepoint& safepoint =
                    map.safepoints[function.firstSafepoint + i];
                uintptr_t pc = start + safepoint.pcOffset;
                if (!yumiGCFrameLookupsEqual(
                        yumiGCFindFrameIndexedUncached(*mapSet, pc),
                        yumiGCFindFrameLinear(*mapSet, pc))) {
                    return false;
                }
            }

            uintptr_t end = start + function.byteSize;
            if (!yumiGCFrameLookupsEqual(
                    yumiGCFindFrameIndexedUncached(*mapSet, end),
                    yumiGCFindFrameLinear(*mapSet, end))) {
                return false;
            }
        }
    }
    return true;
}

extern "C" bool
yumiGCDebugStackMapLookup(
    uintptr_t pc,
    uintptr_t* functionAddress,
    uint32_t* functionByteSize,
    uint32_t* safepointOffset,
    uint32_t* locationCount,
    uint32_t* savedRegisterCount,
    bool* cacheHit
) {
    if (functionAddress) {
        *functionAddress = 0;
    }
    if (functionByteSize) {
        *functionByteSize = 0;
    }
    if (safepointOffset) {
        *safepointOffset = UINT32_MAX;
    }
    if (locationCount) {
        *locationCount = 0;
    }
    if (savedRegisterCount) {
        *savedRegisterCount = 0;
    }
    if (cacheHit) {
        *cacheHit = false;
    }

    const YuGCMapSet* mapSet = NULL;
    if (!yumiGCGetMapSet(&mapSet)) {
        return false;
    }

    YuGCFrameLookup lookup = yumiGCFindFrameIndexed(*mapSet, pc, cacheHit);
    if (!lookup.function) {
        return false;
    }

    if (functionAddress) {
        *functionAddress = yumiGCFunctionAddress(*lookup.map, *lookup.function);
    }
    if (functionByteSize) {
        *functionByteSize = lookup.function->byteSize;
    }
    if (savedRegisterCount) {
        *savedRegisterCount = lookup.function->savedRegisterCount;
    }
    if (lookup.safepoint) {
        if (safepointOffset) {
            *safepointOffset = lookup.safepoint->pcOffset;
        }
        if (locationCount) {
            *locationCount = lookup.safepoint->locationCount;
        }
    }
    return true;
}

static void
yumiGCValidationReportMissingSafepoint(
    const YuGCMapView& map,
    const YuGCMapFunction& function,
    uintptr_t returnPC,
    size_t depth,
    YuGCFrameRecord* frame,
    void* callerFramePointer
) {
    uintptr_t functionAddress = yumiGCFunctionAddress(map, function);
    uint32_t pcOffset = static_cast<uint32_t>(returnPC - functionAddress);
    uint32_t previousOffset = UINT32_MAX;
    uint32_t nextOffset = UINT32_MAX;

    for (uint32_t i = 0; i < function.safepointCount; ++i) {
        const YuGCMapSafepoint& safepoint = map.safepoints[function.firstSafepoint + i];
        if (safepoint.pcOffset < pcOffset) {
            if (previousOffset == UINT32_MAX || safepoint.pcOffset > previousOffset) {
                previousOffset = safepoint.pcOffset;
            }
        } else if (safepoint.pcOffset > pcOffset) {
            if (nextOffset == UINT32_MAX || safepoint.pcOffset < nextOffset) {
                nextOffset = safepoint.pcOffset;
            }
        }
    }

    std::fprintf(
        stderr,
        "Yumi GC stack-map validation failed: Yumi frame return PC has no safepoint record\n"
        "  returnPC=0x%llx function=0x%llx+0x%x offset=0x%x depth=%zu\n"
        "  functionSymbol=%s returnSymbol=%s safepoints=%u previous=%s0x%x next=%s0x%x\n"
        "  frame=%p callerFrame=%p\n",
        (unsigned long long)returnPC,
        (unsigned long long)functionAddress,
        function.byteSize,
        pcOffset,
        depth,
        yumiGCSymbolName(functionAddress),
        yumiGCSymbolName(returnPC),
        function.safepointCount,
        previousOffset == UINT32_MAX ? "<none>" : "",
        previousOffset == UINT32_MAX ? 0 : previousOffset,
        nextOffset == UINT32_MAX ? "<none>" : "",
        nextOffset == UINT32_MAX ? 0 : nextOffset,
        (void*)frame,
        callerFramePointer);
}

static bool
yumiGCFrameInStack(void* frame, uintptr_t stackLow, uintptr_t stackHigh) {
    uintptr_t fp = reinterpret_cast<uintptr_t>(frame);
    return fp >= stackLow && fp + sizeof(YuGCFrameRecord) <= stackHigh && (fp & 0xf) == 0;
}

static bool
yumiGCAddressRangeInStack(void* address, size_t size, uintptr_t stackLow, uintptr_t stackHigh) {
    uintptr_t start = reinterpret_cast<uintptr_t>(address);
    if (start < stackLow || start > stackHigh) {
        return false;
    }
    return size <= stackHigh - start;
}

__attribute__((no_sanitize("address"))) static void
yumiGCScanConservativeStackRange(
    void* stackPointer,
    uintptr_t stackLow,
    uintptr_t stackHigh,
    YuGCStackRootVisitor visitor,
    void* data
) {
    if (!visitor || stackLow >= stackHigh) {
        return;
    }

    uintptr_t start = reinterpret_cast<uintptr_t>(stackPointer);
    if (start < stackLow || start > stackHigh) {
        start = stackLow;
    }
    start = (start + sizeof(void*) - 1) & ~(sizeof(void*) - 1);

    for (uintptr_t address = start; address + sizeof(void*) <= stackHigh; address += sizeof(void*)) {
        void* candidate = *reinterpret_cast<void**>(address);
        if (candidate) {
            gGCConservativeRootSlot = reinterpret_cast<void*>(address);
            gGCVisitingConservativeRoot = true;
            visitor(candidate, NULL, data);
            gGCVisitingConservativeRoot = false;
            gGCConservativeRootSlot = NULL;
        }
    }
}

static void
yumiGCScanConservativeRegisters(
    const YuGCRegisterBank& registers,
    YuGCStackRootVisitor visitor,
    void* data
) {
    if (!visitor) {
        return;
    }

    gGCVisitingConservativeRoot = true;
    for (uintptr_t value : registers.x) {
        if (value) {
            visitor(reinterpret_cast<void*>(value), NULL, data);
        }
    }
    gGCVisitingConservativeRoot = false;
}

static YuGCFrameRecord*
yumiGCFindResyncFrame(
    const YuGCMapSet& mapSet,
    uintptr_t start,
    uintptr_t stackLow,
    uintptr_t stackHigh
) {
    if (start >= stackHigh) {
        return NULL;
    }

    constexpr uintptr_t kMaxSearchBytes = 1024 * 1024;
    uintptr_t limit = stackHigh;
    if (limit - start > kMaxSearchBytes) {
        limit = start + kMaxSearchBytes;
    }
    start = (start + 0xf) & ~uintptr_t(0xf);

    for (
        uintptr_t address = start;
        address + sizeof(YuGCFrameRecord) <= limit;
        address += 16
    ) {
        YuGCFrameRecord* candidate = reinterpret_cast<YuGCFrameRecord*>(address);
        void* previous = candidate->previous;
        uintptr_t previousAddress = reinterpret_cast<uintptr_t>(previous);
        if (
            previousAddress <= address ||
            !yumiGCFrameInStack(previous, stackLow, stackHigh)
        ) {
            continue;
        }

        uintptr_t returnPC = reinterpret_cast<uintptr_t>(candidate->returnAddress);
        YuGCFrameLookup lookup = yumiGCFindFrame(mapSet, returnPC);
        if (lookup.function) {
            return candidate;
        }
    }
    return NULL;
}

extern "C" void*
yumiGCDebugConservativeRootSlot() {
    return gGCConservativeRootSlot;
}

extern "C" bool
yumiGCVisitingConservativeRoot() {
    return gGCVisitingConservativeRoot;
}

extern "C" void
yumiGCStackMapLinkAnchor() {
}

extern "C" bool
yumiGCValidateAllocationABI() {
    const YuGCMapSet* mapSet = NULL;
    if (!yumiGCGetMapSet(&mapSet) || !mapSet || mapSet->maps.empty()) {
        yumiGCValidationAbort("missing or incompatible GC allocation ABI metadata");
    }
    return true;
}

static void
yumiGCValidationReportInvalidFrameLink(
    uintptr_t returnPC,
    size_t depth,
    void* frame,
    void* callerFramePointer
) {
    std::fprintf(
        stderr,
        "Yumi GC stack-map validation failed: invalid caller frame pointer\n"
        "  returnPC=0x%llx returnSymbol=%s depth=%zu frame=%p callerFrame=%p\n",
        (unsigned long long)returnPC,
        yumiGCSymbolName(returnPC),
        depth,
        frame,
        callerFramePointer);
}

static void
yumiGCValidationReportInvalidRootSlot(
    uintptr_t returnPC,
    void* framePointer,
    int32_t offset
) {
    std::fprintf(
        stderr,
        "Yumi GC stack-map validation failed: root slot is outside stack bounds\n"
        "  returnPC=0x%llx returnSymbol=%s frame=%p offset=%d\n",
        (unsigned long long)returnPC,
        yumiGCSymbolName(returnPC),
        framePointer,
        offset);
}

static bool
yumiGCTraceSafepointRoots(
    const YuGCMapView& map,
    const YuGCMapSafepoint* safepoint,
    uintptr_t returnPC,
    void* framePointer,
    uintptr_t stackLow,
    uintptr_t stackHigh,
    YuGCRegisterBank& registers,
    YuGCStackRootVisitor visitor,
    void* data
) {
    if (!safepoint) {
        return true;
    }

    bool complete = true;
    for (uint32_t i = 0; i < safepoint->locationCount; ++i) {
        const YuGCMapLocation& location = map.locations[safepoint->firstLocation + i];
        void* root = NULL;
        uintptr_t descriptorAddress = location.descriptorIndex
            ? map.descriptors[location.descriptorIndex - 1]
            : 0;
        void* descriptor = reinterpret_cast<void*>(
            yumiGCNormalizeMapPointer(map, descriptorAddress));
        if (location.kind == 1) {
            void** slot = reinterpret_cast<void**>(
                static_cast<char*>(framePointer) + location.offset);
            if (!yumiGCAddressRangeInStack(slot, sizeof(void*), stackLow, stackHigh)) {
                yumiGCValidationReportInvalidRootSlot(returnPC, framePointer, location.offset);
                complete = false;
                continue;
            }
            root = *slot;
        } else if (location.kind == 2 && location.registerNumber < 29) {
            root = reinterpret_cast<void*>(registers.x[location.registerNumber]);
        } else if (location.kind == 3 && descriptor) {
            root = static_cast<char*>(framePointer) + location.offset;
            if (!yumiGCAddressRangeInStack(root, 1, stackLow, stackHigh)) {
                yumiGCValidationReportInvalidRootSlot(returnPC, framePointer, location.offset);
                complete = false;
                continue;
            }
        } else if (location.kind == 4 && descriptor) {
            void** slot = reinterpret_cast<void**>(
                static_cast<char*>(framePointer) + location.offset);
            if (!yumiGCAddressRangeInStack(slot, sizeof(void*), stackLow, stackHigh)) {
                yumiGCValidationReportInvalidRootSlot(returnPC, framePointer, location.offset);
                complete = false;
                continue;
            }
            root = *slot;
        } else if (yumiGCShouldAbortOnMapFailure()) {
            yumiGCValidationAbort("invalid root location in __yumi_gcmap");
        }

        if (root) {
            visitor(root, descriptor, data);
        }
    }
    return complete;
}

static void
yumiGCRestoreSavedRegisters(
    const YuGCMapView& map,
    const YuGCMapFunction* function,
    void* framePointer,
    YuGCRegisterBank& registers
) {
    if (!function) {
        return;
    }

    for (uint32_t i = 0; i < function->savedRegisterCount; ++i) {
        const YuGCMapSavedRegister& saved =
            map.savedRegisters[function->firstSavedRegister + i];
        if (saved.registerNumber >= 29) {
            continue;
        }

        uintptr_t* slot = reinterpret_cast<uintptr_t*>(
            static_cast<char*>(framePointer) + saved.fpOffset);
        registers.x[saved.registerNumber] = *slot;
    }
}

extern "C" void
yumiGCScanContextRoots(
    const YumiGCContextSnapshot* context,
    void* stackBase,
    size_t stackSize,
    YuGCStackRootVisitor visitor,
    void* data
) {
    if (!context || !visitor) {
        return;
    }

    if (context->arg) {
        visitor(context->arg, NULL, data);
    }
    if (!stackBase) {
        return;
    }

    const YuGCMapSet* mapSet = NULL;
    if (!yumiGCGetMapSet(&mapSet)) {
        return;
    }

    YuGCRegisterBank registers;
    std::memset(&registers, 0, sizeof(registers));
    registers.x[19] = context->x[19];
    registers.x[20] = context->x[20];
    registers.x[21] = context->x[21];
    registers.x[22] = context->x[22];
    registers.x[23] = context->x[23];
    registers.x[24] = context->x[24];
    registers.x[25] = context->x[25];
    registers.x[26] = context->x[26];
    registers.x[27] = context->x[27];
    registers.x[28] = context->x[28];
    yumiGCScanConservativeRegisters(registers, visitor, data);

    uintptr_t stackLow = reinterpret_cast<uintptr_t>(stackBase);
    uintptr_t stackHigh = stackLow + stackSize;
    YuGCFrameRecord* frame = static_cast<YuGCFrameRecord*>(context->fp);
    bool conservativeFallback = false;

    if (context->pc) {
        uintptr_t returnPC = reinterpret_cast<uintptr_t>(context->pc);
        YuGCFrameLookup lookup = yumiGCFindFrame(*mapSet, returnPC);
        if (!lookup.function || !lookup.safepoint ||
                !yumiGCFrameInStack(context->fp, stackLow, stackHigh)) {
            conservativeFallback = true;
        } else {
            if (!yumiGCTraceSafepointRoots(
                    *lookup.map,
                    lookup.safepoint,
                    returnPC,
                    context->fp,
                    stackLow,
                    stackHigh,
                    registers,
                    visitor,
                    data)) {
                conservativeFallback = true;
            }
            yumiGCRestoreSavedRegisters(
                *lookup.map,
                lookup.function,
                context->fp,
                registers);
            yumiGCScanConservativeRegisters(registers, visitor, data);
        }
    }

    size_t maxDepth = stackSize / sizeof(YuGCFrameRecord);
    for (size_t depth = 0; depth < maxDepth; ++depth) {
        if (!yumiGCFrameInStack(frame, stackLow, stackHigh)) {
            conservativeFallback = true;
            break;
        }

        void* callerFramePointer = frame->previous;
        uintptr_t returnPC = reinterpret_cast<uintptr_t>(frame->returnAddress);
        YuGCFrameLookup lookup = yumiGCFindFrame(*mapSet, returnPC);
        if (lookup.function &&
                lookup.function->safepointCount != 0 &&
                !lookup.safepoint) {
            yumiGCValidationReportMissingSafepoint(
                *lookup.map,
                *lookup.function,
                returnPC,
                depth,
                frame,
                callerFramePointer);
            conservativeFallback = true;
            break;
        }

        bool callerFrameInStack =
            callerFramePointer && yumiGCFrameInStack(callerFramePointer, stackLow, stackHigh);
        if (callerFramePointer && !callerFrameInStack) {
            if (lookup.function && lookup.function->safepointCount) {
                yumiGCValidationReportInvalidFrameLink(
                    returnPC,
                    depth,
                    frame,
                    callerFramePointer);
            }
            conservativeFallback = true;
            break;
        }

        if (callerFrameInStack && lookup.map) {
            if (!yumiGCTraceSafepointRoots(
                *lookup.map,
                lookup.safepoint,
                returnPC,
                callerFramePointer,
                stackLow,
                stackHigh,
                registers,
                visitor,
                data)) {
                conservativeFallback = true;
                break;
            }
            yumiGCRestoreSavedRegisters(
                *lookup.map,
                lookup.function,
                callerFramePointer,
                registers);
            yumiGCScanConservativeRegisters(registers, visitor, data);
        }

        if (
            !callerFrameInStack ||
            reinterpret_cast<uintptr_t>(callerFramePointer) <= reinterpret_cast<uintptr_t>(frame)
        ) {
            uintptr_t frameEnd = reinterpret_cast<uintptr_t>(frame) + sizeof(YuGCFrameRecord);
            YuGCFrameRecord* resyncFrame =
                yumiGCFindResyncFrame(*mapSet, frameEnd, stackLow, stackHigh);
            if (resyncFrame) {
                yumiGCScanConservativeStackRange(
                    reinterpret_cast<void*>(frameEnd),
                    frameEnd,
                    reinterpret_cast<uintptr_t>(resyncFrame),
                    visitor,
                    data);
                frame = resyncFrame;
                continue;
            }
            break;
        }
        frame = static_cast<YuGCFrameRecord*>(callerFramePointer);
    }

    if (conservativeFallback || yumiGCAlwaysScanConservativeStacks()) {
        yumiGCScanConservativeStackRange(context->sp, stackLow, stackHigh, visitor, data);
    }
}


extern "C" void
yumiGCScanConservativeContextRoots(
    const YumiGCContextSnapshot* context,
    void* stackBase,
    size_t stackSize,
    YuGCStackRootVisitor visitor,
    void* data
) {
    if (!context || !visitor) {
        return;
    }

    if (context->arg) {
        visitor(context->arg, NULL, data);
    }
    if (!stackBase) {
        return;
    }

    YuGCRegisterBank registers;
    std::memset(&registers, 0, sizeof(registers));
    registers.x[19] = context->x[19];
    registers.x[20] = context->x[20];
    registers.x[21] = context->x[21];
    registers.x[22] = context->x[22];
    registers.x[23] = context->x[23];
    registers.x[24] = context->x[24];
    registers.x[25] = context->x[25];
    registers.x[26] = context->x[26];
    registers.x[27] = context->x[27];
    registers.x[28] = context->x[28];
    yumiGCScanConservativeRegisters(registers, visitor, data);

    uintptr_t stackLow = reinterpret_cast<uintptr_t>(stackBase);
    uintptr_t stackHigh = stackLow + stackSize;
    yumiGCScanConservativeStackRange(context->sp, stackLow, stackHigh, visitor, data);
}

extern "C" size_t
yumiGCStorageRootRecordCapacity(size_t taskCount, size_t threadCount) {
    const YuGCRootMapSet* mapSet = NULL;
    if (!yumiGCGetRootMapSet(&mapSet)) {
        return 0;
    }

    size_t rootCapacity = 0;
    for (const YuGCRootMapView& map : mapSet->maps) {
        for (uint32_t i = 0; i < map.header->recordCount; ++i) {
            const YuGCRootMapRecord& record = map.records[i];
            if (record.kind == 1) {
                ++rootCapacity;
            } else if (record.kind == 2) {
                rootCapacity += taskCount;
            } else if (record.kind == 3) {
                rootCapacity += threadCount;
            }
        }
    }
    return rootCapacity;
}

extern "C" void
yumiGCVisitStorageRootRecords(YumiGCStorageRootRecordVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    const YuGCRootMapSet* mapSet = NULL;
    if (!yumiGCGetRootMapSet(&mapSet)) {
        return;
    }

    for (const YuGCRootMapView& map : mapSet->maps) {
        for (uint32_t i = 0; i < map.header->recordCount; ++i) {
            const YuGCRootMapRecord& record = map.records[i];
            void* descriptor = reinterpret_cast<void*>(
                yumiGCNormalizeImagePointer(record.descriptor));
            if (!descriptor) {
                if (yumiGCShouldAbortOnMapFailure()) {
                    yumiGCValidationAbort("storage root record has no descriptor");
                }
                continue;
            }

            if (record.kind == 1) {
                void* base = reinterpret_cast<void*>(yumiGCNormalizeImagePointer(record.base));
                if (base) {
                    YumiGCStorageRootRecord root { record.kind, base, descriptor };
                    visitor(&root, data);
                }
            } else if (record.kind == 2 || record.kind == 3) {
                YumiGCStorageRootRecord root { record.kind, NULL, descriptor };
                visitor(&root, data);
            } else if (yumiGCShouldAbortOnMapFailure()) {
                yumiGCValidationAbort("invalid storage root record kind");
            }
        }
    }
}
