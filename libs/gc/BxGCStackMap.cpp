#include <BxGC/BxGCStackMap.h>
#include <BxGC/BxGCAllocationBuffer.h>

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

extern "C" void* brixGetGCStackMapPointer(size_t* out_size);
extern "C" void* brixGetGCRootMapPointer(size_t* out_size);
extern "C" bool brixGCValidateAllocationABI(void);

// *************************************************************************************************

static const uint32_t BxGCMapMagic = 0x4d434759; // YGCM
static const uint16_t BxGCMapVersion = 3;
static const uint32_t BxGCRootMapMagic = 0x52594359; // YCYR
static const uint16_t BxGCRootMapVersion = 1;

#pragma pack(push, 1)
struct BxGCMapHeader {
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

struct BxGCMapFunction {
    uintptr_t address;
    uint32_t byteSize;
    uint32_t firstSafepoint;
    uint32_t safepointCount;
    uint32_t firstSavedRegister;
    uint32_t savedRegisterCount;
    uint32_t reserved;
};

struct BxGCMapSafepoint {
    uint32_t pcOffset;
    uint32_t firstLocation;
    uint32_t locationCount;
};

struct BxGCMapLocation {
    uint8_t kind;
    uint8_t registerNumber;
    uint16_t reserved;
    int32_t offset;
    uint32_t descriptorIndex;
};

struct BxGCMapSavedRegister {
    uint8_t registerNumber;
    uint8_t reserved8;
    uint16_t reserved16;
    int32_t fpOffset;
};

struct BxGCRootMapHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t pointerSize;
    uint32_t recordCount;
    uint32_t reserved;
};

struct BxGCRootMapRecord {
    uint8_t kind;
    uint8_t reserved8;
    uint16_t reserved16;
    uint32_t reserved32;
    uintptr_t base;
    uintptr_t descriptor;
};
#pragma pack(pop)

struct BxGCMapView {
    const BxGCMapHeader* header;
    const uintptr_t* descriptors;
    const BxGCMapFunction* functions;
    const BxGCMapSafepoint* safepoints;
    const BxGCMapLocation* locations;
    const BxGCMapSavedRegister* savedRegisters;
    bool normalizeImagePointers;
};

struct BxGCSafepointIndexEntry {
    uint32_t pcOffset;
    uint32_t ordinal;
    const BxGCMapSafepoint* safepoint;
};

struct BxGCFunctionSafepointIndex {
    const BxGCMapFunction* function;
    std::vector<BxGCSafepointIndexEntry> safepoints;
};

struct BxGCFunctionIndexEntry {
    const BxGCMapView* map;
    const BxGCMapFunction* function;
    size_t safepointIndex;
    size_t ordinal;
    uintptr_t start;
    uintptr_t end;
};

struct BxGCMapSet {
    std::vector<BxGCMapView> maps;
    std::vector<BxGCFunctionIndexEntry> functions;
    std::vector<uintptr_t> functionPrefixMaxEnds;
    std::vector<BxGCFunctionSafepointIndex> safepoints;
    uint64_t generation;
    bool hasOverlappingFunctions;
};

struct BxGCFrameLookup {
    const BxGCMapView* map;
    const BxGCMapFunction* function;
    const BxGCMapSafepoint* safepoint;
};

struct BxGCReturnPCCacheEntry {
    uintptr_t pc;
    uint64_t generation;
    BxGCFrameLookup lookup;
    bool valid;
};

struct BxGCRegisterBank {
    uintptr_t x[29];
};

struct BxGCFrameRecord {
    BxGCFrameRecord* previous;
    void* returnAddress;
};

struct BxGCRootMapView {
    const BxGCRootMapHeader* header;
    const BxGCRootMapRecord* records;
};

struct BxGCRootMapSet {
    std::vector<BxGCRootMapView> maps;
};

static std::once_flag gGCMapSetOnce;
static BxGCMapSet gGCMapSet;
static bool gGCMapSetLoaded = false;
static std::once_flag gGCRootMapSetOnce;
static BxGCRootMapSet gGCRootMapSet;
static bool gGCRootMapSetLoaded = false;
static uint64_t gGCMapSetGeneration = 0;

static constexpr size_t kBxGCReturnPCCacheSize = 16;
static constexpr size_t kBxGCSubphaseStackMapLookup = 11;
static thread_local BxGCReturnPCCacheEntry gGCReturnPCCache[kBxGCReturnPCCacheSize] = {};
static thread_local void* gGCConservativeRootSlot = NULL;
static thread_local bool gGCVisitingConservativeRoot = false;

extern "C" void
brixGCRuntimeRecordSubphaseDuration(
    size_t subphase,
    size_t cycle,
    size_t durationNanos,
    size_t value) __attribute__((weak));

extern "C" void
brixGCRuntimeRecordSubphaseDuration(
    size_t,
    size_t,
    size_t,
    size_t
) {
}

extern "C" size_t
brixGCRuntimeCurrentCycleId(void) __attribute__((weak));

extern "C" size_t
brixGCRuntimeCurrentCycleId(void) {
    return 0;
}

static bool
brixGCValidateStackMaps() {
    static const bool enabled = []() {
        const char* value = std::getenv("BRIX_GC_VALIDATE_STACKMAPS");
        return value && *value && std::strcmp(value, "0") != 0;
    }();
    return enabled;
}

static bool
brixGCAllowMissingStackMaps() {
    static const bool enabled = []() {
        const char* value = std::getenv("BRIX_GC_ALLOW_MISSING_STACKMAPS");
        const char* testMode = std::getenv("BRIX_GC_TEST_ALLOW_MISSING_STACKMAPS");
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
brixGCShouldAbortOnMapFailure() {
    return brixGCValidateStackMaps() || !brixGCAllowMissingStackMaps();
}

static bool
brixGCAlwaysScanConservativeStacks() {
    static const bool enabled = []() {
        const char* value = std::getenv("BRIX_GC_ALWAYS_CONSERVATIVE_STACKS");
        return value && *value && std::strcmp(value, "0") != 0;
    }();
    return enabled;
}

static void
brixGCValidationAbort(const char* message) {
    std::fprintf(stderr, "Brix GC stack-map validation failed: %s\n", message);
    std::abort();
}

static const char*
brixGCSymbolName(uintptr_t address) {
    Dl_info info;
    if (dladdr(reinterpret_cast<const void*>(address), &info) && info.dli_sname) {
        return info.dli_sname;
    }
    return "<unknown>";
}

static bool
brixGCCheckedAddSize(size_t* total, size_t count, size_t itemSize) {
    if (itemSize != 0 && count > (static_cast<size_t>(-1) - *total) / itemSize) {
        return false;
    }
    *total += count * itemSize;
    return true;
}

static bool
brixGCRangeFits(uint32_t first, uint32_t count, uint32_t total) {
    return first <= total && count <= total - first;
}

static bool
brixGCAddressRangeFits(uintptr_t address, uint32_t byteSize) {
    return byteSize != 0 && address <= static_cast<uintptr_t>(-1) - byteSize;
}

static uintptr_t
brixGCMainImageBase() {
#if defined(__APPLE__)
    return reinterpret_cast<uintptr_t>(&_mh_execute_header);
#else
    return 0;
#endif
}

static intptr_t
brixGCMainImageSlide() {
#if defined(__APPLE__)
    static const intptr_t slide = []() -> intptr_t {
        const char* command = reinterpret_cast<const char*>(&_mh_execute_header + 1);
        for (uint32_t index = 0; index < _mh_execute_header.ncmds; ++index) {
            const load_command* load = reinterpret_cast<const load_command*>(command);
            if (load->cmd == LC_SEGMENT_64) {
                const segment_command_64* segment =
                    reinterpret_cast<const segment_command_64*>(command);
                if (std::strncmp(segment->segname, SEG_TEXT, sizeof(segment->segname)) == 0) {
                    return static_cast<intptr_t>(brixGCMainImageBase() - segment->vmaddr);
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
brixGCNormalizeImagePointer(uintptr_t value) {
    if (!value) {
        return 0;
    }

    uintptr_t imageBase = brixGCMainImageBase();
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

    intptr_t slide = brixGCMainImageSlide();
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
brixGCNormalizeMapPointer(const BxGCMapView& map, uintptr_t value) {
    return map.normalizeImagePointers ? brixGCNormalizeImagePointer(value) : value;
}

static uintptr_t
brixGCFunctionAddress(const BxGCMapView& map, const BxGCMapFunction& function) {
    return brixGCNormalizeMapPointer(map, function.address);
}

static uint64_t
brixGCMonotonicNanos() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void
brixGCRecordStackMapLookupDuration(uint64_t startNanos, size_t value) {
    if (!brixGCRuntimeRecordSubphaseDuration) {
        return;
    }

    uint64_t endNanos = brixGCMonotonicNanos();
    if (endNanos <= startNanos) {
        return;
    }
    size_t durationNanos = static_cast<size_t>(endNanos - startNanos);
    size_t cycle = brixGCRuntimeCurrentCycleId ? brixGCRuntimeCurrentCycleId() : 0;
    brixGCRuntimeRecordSubphaseDuration(
        kBxGCSubphaseStackMapLookup,
        cycle,
        durationNanos,
        value);
}

static bool
brixGCCompareStackMapLookupEnabled() {
    static const bool enabled = []() {
        const char* value = std::getenv("BRIX_GC_COMPARE_STACKMAP_LOOKUP");
        return value && *value && std::strcmp(value, "0") != 0;
    }();
    return enabled;
}

static const char*
brixGCSkipMapPadding(const char* cursor, const char* end, uint32_t magic) {
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
brixGCParseMap(const char* cursor, size_t size, BxGCMapView* out, size_t* consumed) {
    if (size < sizeof(BxGCMapHeader)) {
        return false;
    }

    const BxGCMapHeader* header = reinterpret_cast<const BxGCMapHeader*>(cursor);
    if (
        header->magic != BxGCMapMagic ||
        header->version != BxGCMapVersion ||
        header->pointerSize != sizeof(void*) ||
        header->reserved != BrixGCAllocationABIVersion
    ) {
        return false;
    }

    size_t expectedSize = sizeof(BxGCMapHeader);
    if (
        !brixGCCheckedAddSize(&expectedSize, header->descriptorCount, sizeof(uintptr_t)) ||
        !brixGCCheckedAddSize(&expectedSize, header->functionCount, sizeof(BxGCMapFunction)) ||
        !brixGCCheckedAddSize(&expectedSize, header->safepointCount, sizeof(BxGCMapSafepoint)) ||
        !brixGCCheckedAddSize(&expectedSize, header->locationCount, sizeof(BxGCMapLocation)) ||
        !brixGCCheckedAddSize(
            &expectedSize,
            header->savedRegisterCount,
            sizeof(BxGCMapSavedRegister))
    ) {
        return false;
    }

    if (size < expectedSize) {
        return false;
    }

    cursor += sizeof(BxGCMapHeader);
    out->header = header;
    out->descriptors = reinterpret_cast<const uintptr_t*>(cursor);
    cursor += header->descriptorCount * sizeof(uintptr_t);
    out->functions = reinterpret_cast<const BxGCMapFunction*>(cursor);
    cursor += header->functionCount * sizeof(BxGCMapFunction);
    out->safepoints = reinterpret_cast<const BxGCMapSafepoint*>(cursor);
    cursor += header->safepointCount * sizeof(BxGCMapSafepoint);
    out->locations = reinterpret_cast<const BxGCMapLocation*>(cursor);
    cursor += header->locationCount * sizeof(BxGCMapLocation);
    out->savedRegisters = reinterpret_cast<const BxGCMapSavedRegister*>(cursor);
    out->normalizeImagePointers = false;

    for (uint32_t functionIndex = 0; functionIndex < header->functionCount; ++functionIndex) {
        const BxGCMapFunction& function = out->functions[functionIndex];
        if (function.address >= 0x100000000ull) {
            out->normalizeImagePointers = true;
        }
    }

    *consumed = expectedSize;
    return true;
}

static bool
brixGCFunctionMapValid(const BxGCMapView& map, const BxGCMapFunction& function) {
    if (!brixGCAddressRangeFits(function.address, function.byteSize) ||
            !brixGCRangeFits(
                function.firstSafepoint,
                function.safepointCount,
                map.header->safepointCount) ||
            !brixGCRangeFits(
                function.firstSavedRegister,
                function.savedRegisterCount,
                map.header->savedRegisterCount)) {
        return false;
    }

    for (uint32_t i = 0; i < function.safepointCount; ++i) {
        const BxGCMapSafepoint& safepoint =
            map.safepoints[function.firstSafepoint + i];
        if (safepoint.pcOffset >= function.byteSize ||
                !brixGCRangeFits(
                    safepoint.firstLocation,
                    safepoint.locationCount,
                    map.header->locationCount)) {
            return false;
        }
        for (uint32_t j = 0; j < safepoint.locationCount; ++j) {
            const BxGCMapLocation& location =
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
        const BxGCMapSavedRegister& saved =
            map.savedRegisters[function.firstSavedRegister + i];
        if (saved.registerNumber >= 29) {
            return false;
        }
    }
    return true;
}

static void
brixGCBuildMapSetIndexes(BxGCMapSet* out) {
    out->functions.clear();
    out->functionPrefixMaxEnds.clear();
    out->safepoints.clear();

    size_t functionCount = 0;
    for (const BxGCMapView& map : out->maps) {
        functionCount += map.header->functionCount;
    }
    out->functions.reserve(functionCount);
    out->functionPrefixMaxEnds.reserve(functionCount);
    out->safepoints.reserve(functionCount);

    size_t ordinal = 0;
    for (const BxGCMapView& map : out->maps) {
        for (uint32_t functionIndex = 0; functionIndex < map.header->functionCount; ++functionIndex) {
            const BxGCMapFunction& function = map.functions[functionIndex];
            if (!brixGCFunctionMapValid(map, function)) {
                std::fprintf(
                    stderr,
                    "Brix GC rejected invalid stack-map function: address=0x%llx ordinal=%u\n",
                    (unsigned long long)brixGCFunctionAddress(map, function),
                    functionIndex);
                continue;
            }
            BxGCFunctionSafepointIndex safepointIndex;
            safepointIndex.function = &function;
            safepointIndex.safepoints.reserve(function.safepointCount);

            for (uint32_t i = 0; i < function.safepointCount; ++i) {
                const BxGCMapSafepoint& safepoint =
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
                [](const BxGCSafepointIndexEntry& left, const BxGCSafepointIndexEntry& right) {
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
                brixGCFunctionAddress(map, function),
                brixGCFunctionAddress(map, function) + function.byteSize
            });
        }
    }

    std::sort(
        out->functions.begin(),
        out->functions.end(),
        [](const BxGCFunctionIndexEntry& left, const BxGCFunctionIndexEntry& right) {
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
    for (const BxGCFunctionIndexEntry& function : out->functions) {
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
brixGCLoadMapSetUncached(BxGCMapSet* out) {
    out->maps.clear();
    out->functions.clear();
    out->functionPrefixMaxEnds.clear();
    out->safepoints.clear();
    out->generation = 0;
    out->hasOverlappingFunctions = false;
    out->maps.reserve(1);

    size_t size = 0;
    void* data = brixGetGCStackMapPointer(&size);
    if (!data || size < sizeof(BxGCMapHeader)) {
        if (brixGCShouldAbortOnMapFailure()) {
            brixGCValidationAbort("missing or truncated __brix_gcmap section");
        }
        return false;
    }

    const char* cursor = static_cast<const char*>(data);
    const char* end = cursor + size;
    while (cursor < end) {
        cursor = brixGCSkipMapPadding(cursor, end, BxGCMapMagic);
        if (cursor >= end) {
            break;
        }

        BxGCMapView map;
        size_t consumed = 0;
        if (!brixGCParseMap(cursor, static_cast<size_t>(end - cursor), &map, &consumed)) {
            if (brixGCShouldAbortOnMapFailure()) {
                brixGCValidationAbort("invalid or truncated __brix_gcmap payload");
            }
            return false;
        }
        out->maps.push_back(map);
        cursor += consumed;
    }

    if (out->maps.empty()) {
        if (brixGCShouldAbortOnMapFailure()) {
            brixGCValidationAbort("missing __brix_gcmap records");
        }
        return false;
    }
    brixGCBuildMapSetIndexes(out);
    return true;
}

static bool
brixGCParseRootMap(const char* cursor, size_t size, BxGCRootMapView* out, size_t* consumed) {
    if (size < sizeof(BxGCRootMapHeader)) {
        return false;
    }

    const BxGCRootMapHeader* header = reinterpret_cast<const BxGCRootMapHeader*>(cursor);
    if (
        header->magic != BxGCRootMapMagic ||
        header->version != BxGCRootMapVersion ||
        header->pointerSize != sizeof(void*)
    ) {
        return false;
    }

    size_t expectedSize = sizeof(BxGCRootMapHeader);
    if (!brixGCCheckedAddSize(&expectedSize, header->recordCount, sizeof(BxGCRootMapRecord))) {
        return false;
    }

    if (size < expectedSize) {
        return false;
    }

    out->header = header;
    out->records = reinterpret_cast<const BxGCRootMapRecord*>(
        cursor + sizeof(BxGCRootMapHeader));
    *consumed = expectedSize;
    return true;
}

static bool
brixGCLoadRootMapSetUncached(BxGCRootMapSet* out) {
    out->maps.clear();
    out->maps.reserve(1);

    size_t size = 0;
    void* data = brixGetGCRootMapPointer(&size);
    if (!data || size < sizeof(BxGCRootMapHeader)) {
        if (brixGCShouldAbortOnMapFailure()) {
            brixGCValidationAbort("missing or truncated __brix_gcroots section");
        }
        return false;
    }

    const char* cursor = static_cast<const char*>(data);
    const char* end = cursor + size;
    while (cursor < end) {
        cursor = brixGCSkipMapPadding(cursor, end, BxGCRootMapMagic);
        if (cursor >= end) {
            break;
        }

        BxGCRootMapView map;
        size_t consumed = 0;
        if (!brixGCParseRootMap(cursor, static_cast<size_t>(end - cursor), &map, &consumed)) {
            if (brixGCShouldAbortOnMapFailure()) {
                brixGCValidationAbort("invalid or truncated __brix_gcroots payload");
            }
            return false;
        }
        out->maps.push_back(map);
        cursor += consumed;
    }

    if (out->maps.empty()) {
        if (brixGCShouldAbortOnMapFailure()) {
            brixGCValidationAbort("missing __brix_gcroots records");
        }
        return false;
    }
    return true;
}

static void
brixGCInitMapSetOnce() {
    gGCMapSetLoaded = brixGCLoadMapSetUncached(&gGCMapSet);
}

static bool
brixGCGetMapSet(const BxGCMapSet** out) {
    std::call_once(gGCMapSetOnce, brixGCInitMapSetOnce);
    if (!gGCMapSetLoaded) {
        return false;
    }
    *out = &gGCMapSet;
    return true;
}

static void
brixGCInitRootMapSetOnce() {
    gGCRootMapSetLoaded = brixGCLoadRootMapSetUncached(&gGCRootMapSet);
}

static bool
brixGCGetRootMapSet(const BxGCRootMapSet** out) {
    std::call_once(gGCRootMapSetOnce, brixGCInitRootMapSetOnce);
    if (!gGCRootMapSetLoaded) {
        return false;
    }
    *out = &gGCRootMapSet;
    return true;
}

static const BxGCMapFunction*
brixGCFindFunctionLinear(const BxGCMapView& map, uintptr_t pc) {
    for (uint32_t index = 0; index < map.header->functionCount; ++index) {
        const BxGCMapFunction& function = map.functions[index];
        uintptr_t start = brixGCFunctionAddress(map, function);
        uintptr_t end = start + function.byteSize;
        if (start <= pc && pc < end) {
            return &function;
        }
    }
    return NULL;
}

static const BxGCMapSafepoint*
brixGCFindSafepointLinear(const BxGCMapView& map, const BxGCMapFunction* function, uintptr_t pc) {
    if (!function) {
        return NULL;
    }

    uintptr_t functionAddress = brixGCFunctionAddress(map, *function);
    if (pc < functionAddress) {
        return NULL;
    }

    uint32_t pcOffset = static_cast<uint32_t>(pc - functionAddress);
    for (uint32_t i = 0; i < function->safepointCount; ++i) {
        const BxGCMapSafepoint& safepoint = map.safepoints[function->firstSafepoint + i];
        if (safepoint.pcOffset == pcOffset) {
            return &safepoint;
        }
    }
    return NULL;
}

static const BxGCMapSafepoint*
brixGCFindSafepointIndexed(
    const BxGCMapSet& mapSet,
    const BxGCFunctionIndexEntry& functionEntry,
    uintptr_t pc
) {
    if (functionEntry.safepointIndex >= mapSet.safepoints.size() || pc < functionEntry.start) {
        return NULL;
    }

    const BxGCFunctionSafepointIndex& index = mapSet.safepoints[functionEntry.safepointIndex];
    if (!index.function || index.function != functionEntry.function) {
        return NULL;
    }

    uint32_t pcOffset = static_cast<uint32_t>(pc - functionEntry.start);
    auto found = std::lower_bound(
        index.safepoints.begin(),
        index.safepoints.end(),
        pcOffset,
        [](const BxGCSafepointIndexEntry& entry, uint32_t offset) {
            return entry.pcOffset < offset;
        });
    if (found != index.safepoints.end() && found->pcOffset == pcOffset) {
        return found->safepoint;
    }
    return NULL;
}

static BxGCFrameLookup
brixGCFindFrameIndexedUncached(const BxGCMapSet& mapSet, uintptr_t pc) {
    BxGCFrameLookup result { NULL, NULL, NULL };
    if (mapSet.functions.empty()) {
        return result;
    }

    auto found = std::upper_bound(
        mapSet.functions.begin(),
        mapSet.functions.end(),
        pc,
        [](uintptr_t address, const BxGCFunctionIndexEntry& entry) {
            return address < entry.start;
        });
    if (found == mapSet.functions.begin()) {
        return result;
    }

    const BxGCFunctionIndexEntry* functionEntry = NULL;
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
    result.safepoint = brixGCFindSafepointIndexed(mapSet, *functionEntry, pc);
    return result;
}

static BxGCFrameLookup
brixGCFindFrameIndexed(const BxGCMapSet& mapSet, uintptr_t pc, bool* cacheHit) {
    if (cacheHit) {
        *cacheHit = false;
    }

    size_t cacheIndex = (pc >> 4) & (kBxGCReturnPCCacheSize - 1);
    BxGCReturnPCCacheEntry& cached = gGCReturnPCCache[cacheIndex];
    if (cached.valid && cached.generation == mapSet.generation && cached.pc == pc) {
        if (cacheHit) {
            *cacheHit = true;
        }
        return cached.lookup;
    }

    BxGCFrameLookup lookup = brixGCFindFrameIndexedUncached(mapSet, pc);
    cached.pc = pc;
    cached.generation = mapSet.generation;
    cached.lookup = lookup;
    cached.valid = true;
    return lookup;
}

static BxGCFrameLookup
brixGCFindFrameLinear(const BxGCMapSet& mapSet, uintptr_t pc) {
    BxGCFrameLookup result { NULL, NULL, NULL };
    for (const BxGCMapView& map : mapSet.maps) {
        const BxGCMapFunction* function = brixGCFindFunctionLinear(map, pc);
        if (!function) {
            continue;
        }

        result.map = &map;
        result.function = function;
        result.safepoint = brixGCFindSafepointLinear(map, function, pc);
        return result;
    }
    return result;
}

static bool
brixGCFrameLookupsEqual(const BxGCFrameLookup& left, const BxGCFrameLookup& right);

static BxGCFrameLookup
brixGCFindFrame(const BxGCMapSet& mapSet, uintptr_t pc, bool* cacheHit = NULL) {
    uint64_t startNanos = brixGCMonotonicNanos();
    BxGCFrameLookup lookup = brixGCFindFrameIndexed(mapSet, pc, cacheHit);
    brixGCRecordStackMapLookupDuration(startNanos, (cacheHit && *cacheHit) ? 1 : 0);
    if (brixGCCompareStackMapLookupEnabled()) {
        BxGCFrameLookup linear = brixGCFindFrameLinear(mapSet, pc);
        if (!brixGCFrameLookupsEqual(lookup, linear)) {
            std::fprintf(
                stderr,
                "Brix GC stack-map lookup mismatch pc=0x%llx indexed=(map=%p function=%p safepoint=%p) linear=(map=%p function=%p safepoint=%p)\n",
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
brixGCFrameLookupsEqual(const BxGCFrameLookup& left, const BxGCFrameLookup& right) {
    return
        left.map == right.map &&
        left.function == right.function &&
        left.safepoint == right.safepoint;
}

extern "C" void
brixGCDebugClearStackMapLookupCache() {
    for (size_t i = 0; i < kBxGCReturnPCCacheSize; ++i) {
        gGCReturnPCCache[i] = BxGCReturnPCCacheEntry {};
    }
}

extern "C" size_t
brixGCDebugStackMapFunctionIndexCount() {
    const BxGCMapSet* mapSet = NULL;
    if (!brixGCGetMapSet(&mapSet)) {
        return 0;
    }
    return mapSet->functions.size();
}

extern "C" bool
brixGCDebugStackMapLookupSelfTest() {
    const BxGCMapSet* mapSet = NULL;
    if (!brixGCGetMapSet(&mapSet)) {
        return false;
    }

    for (const BxGCMapView& map : mapSet->maps) {
        for (uint32_t functionIndex = 0; functionIndex < map.header->functionCount; ++functionIndex) {
            const BxGCMapFunction& function = map.functions[functionIndex];
            uintptr_t start = brixGCFunctionAddress(map, function);
            if (!brixGCFrameLookupsEqual(
                    brixGCFindFrameIndexedUncached(*mapSet, start),
                    brixGCFindFrameLinear(*mapSet, start))) {
                return false;
            }

            for (uint32_t i = 0; i < function.safepointCount; ++i) {
                const BxGCMapSafepoint& safepoint =
                    map.safepoints[function.firstSafepoint + i];
                uintptr_t pc = start + safepoint.pcOffset;
                if (!brixGCFrameLookupsEqual(
                        brixGCFindFrameIndexedUncached(*mapSet, pc),
                        brixGCFindFrameLinear(*mapSet, pc))) {
                    return false;
                }
            }

            uintptr_t end = start + function.byteSize;
            if (!brixGCFrameLookupsEqual(
                    brixGCFindFrameIndexedUncached(*mapSet, end),
                    brixGCFindFrameLinear(*mapSet, end))) {
                return false;
            }
        }
    }
    return true;
}

extern "C" bool
brixGCDebugStackMapLookup(
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

    const BxGCMapSet* mapSet = NULL;
    if (!brixGCGetMapSet(&mapSet)) {
        return false;
    }

    BxGCFrameLookup lookup = brixGCFindFrameIndexed(*mapSet, pc, cacheHit);
    if (!lookup.function) {
        return false;
    }

    if (functionAddress) {
        *functionAddress = brixGCFunctionAddress(*lookup.map, *lookup.function);
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
brixGCValidationReportMissingSafepoint(
    const BxGCMapView& map,
    const BxGCMapFunction& function,
    uintptr_t returnPC,
    size_t depth,
    BxGCFrameRecord* frame,
    void* callerFramePointer
) {
    uintptr_t functionAddress = brixGCFunctionAddress(map, function);
    uint32_t pcOffset = static_cast<uint32_t>(returnPC - functionAddress);
    uint32_t previousOffset = UINT32_MAX;
    uint32_t nextOffset = UINT32_MAX;

    for (uint32_t i = 0; i < function.safepointCount; ++i) {
        const BxGCMapSafepoint& safepoint = map.safepoints[function.firstSafepoint + i];
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
        "Brix GC stack-map validation failed: Brix frame return PC has no safepoint record\n"
        "  returnPC=0x%llx function=0x%llx+0x%x offset=0x%x depth=%zu\n"
        "  functionSymbol=%s returnSymbol=%s safepoints=%u previous=%s0x%x next=%s0x%x\n"
        "  frame=%p callerFrame=%p\n",
        (unsigned long long)returnPC,
        (unsigned long long)functionAddress,
        function.byteSize,
        pcOffset,
        depth,
        brixGCSymbolName(functionAddress),
        brixGCSymbolName(returnPC),
        function.safepointCount,
        previousOffset == UINT32_MAX ? "<none>" : "",
        previousOffset == UINT32_MAX ? 0 : previousOffset,
        nextOffset == UINT32_MAX ? "<none>" : "",
        nextOffset == UINT32_MAX ? 0 : nextOffset,
        (void*)frame,
        callerFramePointer);
}

static bool
brixGCFrameInStack(void* frame, uintptr_t stackLow, uintptr_t stackHigh) {
    uintptr_t fp = reinterpret_cast<uintptr_t>(frame);
    return fp >= stackLow && fp + sizeof(BxGCFrameRecord) <= stackHigh && (fp & 0xf) == 0;
}

static bool
brixGCAddressRangeInStack(void* address, size_t size, uintptr_t stackLow, uintptr_t stackHigh) {
    uintptr_t start = reinterpret_cast<uintptr_t>(address);
    if (start < stackLow || start > stackHigh) {
        return false;
    }
    return size <= stackHigh - start;
}

__attribute__((no_sanitize("address"))) static void
brixGCScanConservativeStackRange(
    void* stackPointer,
    uintptr_t stackLow,
    uintptr_t stackHigh,
    BxGCStackRootVisitor visitor,
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
brixGCScanConservativeRegisters(
    const BxGCRegisterBank& registers,
    BxGCStackRootVisitor visitor,
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

static BxGCFrameRecord*
brixGCFindResyncFrame(
    const BxGCMapSet& mapSet,
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
        address + sizeof(BxGCFrameRecord) <= limit;
        address += 16
    ) {
        BxGCFrameRecord* candidate = reinterpret_cast<BxGCFrameRecord*>(address);
        void* previous = candidate->previous;
        uintptr_t previousAddress = reinterpret_cast<uintptr_t>(previous);
        if (
            previousAddress <= address ||
            !brixGCFrameInStack(previous, stackLow, stackHigh)
        ) {
            continue;
        }

        uintptr_t returnPC = reinterpret_cast<uintptr_t>(candidate->returnAddress);
        BxGCFrameLookup lookup = brixGCFindFrame(mapSet, returnPC);
        if (lookup.function) {
            return candidate;
        }
    }
    return NULL;
}

extern "C" void*
brixGCDebugConservativeRootSlot() {
    return gGCConservativeRootSlot;
}

extern "C" bool
brixGCVisitingConservativeRoot() {
    return gGCVisitingConservativeRoot;
}

extern "C" void
brixGCStackMapLinkAnchor() {
}

extern "C" bool
brixGCValidateAllocationABI() {
    const BxGCMapSet* mapSet = NULL;
    if (!brixGCGetMapSet(&mapSet) || !mapSet || mapSet->maps.empty()) {
        brixGCValidationAbort("missing or incompatible GC allocation ABI metadata");
    }
    return true;
}

static void
brixGCValidationReportInvalidFrameLink(
    uintptr_t returnPC,
    size_t depth,
    void* frame,
    void* callerFramePointer
) {
    std::fprintf(
        stderr,
        "Brix GC stack-map validation failed: invalid caller frame pointer\n"
        "  returnPC=0x%llx returnSymbol=%s depth=%zu frame=%p callerFrame=%p\n",
        (unsigned long long)returnPC,
        brixGCSymbolName(returnPC),
        depth,
        frame,
        callerFramePointer);
}

static void
brixGCValidationReportInvalidRootSlot(
    uintptr_t returnPC,
    void* framePointer,
    int32_t offset
) {
    std::fprintf(
        stderr,
        "Brix GC stack-map validation failed: root slot is outside stack bounds\n"
        "  returnPC=0x%llx returnSymbol=%s frame=%p offset=%d\n",
        (unsigned long long)returnPC,
        brixGCSymbolName(returnPC),
        framePointer,
        offset);
}

static bool
brixGCTraceSafepointRoots(
    const BxGCMapView& map,
    const BxGCMapSafepoint* safepoint,
    uintptr_t returnPC,
    void* framePointer,
    uintptr_t stackLow,
    uintptr_t stackHigh,
    BxGCRegisterBank& registers,
    BxGCStackRootVisitor visitor,
    void* data
) {
    if (!safepoint) {
        return true;
    }

    bool complete = true;
    for (uint32_t i = 0; i < safepoint->locationCount; ++i) {
        const BxGCMapLocation& location = map.locations[safepoint->firstLocation + i];
        void* root = NULL;
        uintptr_t descriptorAddress = location.descriptorIndex
            ? map.descriptors[location.descriptorIndex - 1]
            : 0;
        void* descriptor = reinterpret_cast<void*>(
            brixGCNormalizeMapPointer(map, descriptorAddress));
        if (location.kind == 1) {
            void** slot = reinterpret_cast<void**>(
                static_cast<char*>(framePointer) + location.offset);
            if (!brixGCAddressRangeInStack(slot, sizeof(void*), stackLow, stackHigh)) {
                brixGCValidationReportInvalidRootSlot(returnPC, framePointer, location.offset);
                complete = false;
                continue;
            }
            root = *slot;
        } else if (location.kind == 2 && location.registerNumber < 29) {
            root = reinterpret_cast<void*>(registers.x[location.registerNumber]);
        } else if (location.kind == 3 && descriptor) {
            root = static_cast<char*>(framePointer) + location.offset;
            if (!brixGCAddressRangeInStack(root, 1, stackLow, stackHigh)) {
                brixGCValidationReportInvalidRootSlot(returnPC, framePointer, location.offset);
                complete = false;
                continue;
            }
        } else if (location.kind == 4 && descriptor) {
            void** slot = reinterpret_cast<void**>(
                static_cast<char*>(framePointer) + location.offset);
            if (!brixGCAddressRangeInStack(slot, sizeof(void*), stackLow, stackHigh)) {
                brixGCValidationReportInvalidRootSlot(returnPC, framePointer, location.offset);
                complete = false;
                continue;
            }
            root = *slot;
        } else if (brixGCShouldAbortOnMapFailure()) {
            brixGCValidationAbort("invalid root location in __brix_gcmap");
        }

        if (root) {
            visitor(root, descriptor, data);
        }
    }
    return complete;
}

static void
brixGCRestoreSavedRegisters(
    const BxGCMapView& map,
    const BxGCMapFunction* function,
    void* framePointer,
    BxGCRegisterBank& registers
) {
    if (!function) {
        return;
    }

    for (uint32_t i = 0; i < function->savedRegisterCount; ++i) {
        const BxGCMapSavedRegister& saved =
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
brixGCScanContextRoots(
    const BrixGCContextSnapshot* context,
    void* stackBase,
    size_t stackSize,
    BxGCStackRootVisitor visitor,
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

    const BxGCMapSet* mapSet = NULL;
    if (!brixGCGetMapSet(&mapSet)) {
        return;
    }

    BxGCRegisterBank registers;
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
    brixGCScanConservativeRegisters(registers, visitor, data);

    uintptr_t stackLow = reinterpret_cast<uintptr_t>(stackBase);
    uintptr_t stackHigh = stackLow + stackSize;
    BxGCFrameRecord* frame = static_cast<BxGCFrameRecord*>(context->fp);
    bool conservativeFallback = false;

    if (context->pc) {
        uintptr_t returnPC = reinterpret_cast<uintptr_t>(context->pc);
        BxGCFrameLookup lookup = brixGCFindFrame(*mapSet, returnPC);
        if (!lookup.function || !lookup.safepoint ||
                !brixGCFrameInStack(context->fp, stackLow, stackHigh)) {
            conservativeFallback = true;
        } else {
            if (!brixGCTraceSafepointRoots(
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
            brixGCRestoreSavedRegisters(
                *lookup.map,
                lookup.function,
                context->fp,
                registers);
            brixGCScanConservativeRegisters(registers, visitor, data);
        }
    }

    size_t maxDepth = stackSize / sizeof(BxGCFrameRecord);
    for (size_t depth = 0; depth < maxDepth; ++depth) {
        if (!brixGCFrameInStack(frame, stackLow, stackHigh)) {
            conservativeFallback = true;
            break;
        }

        void* callerFramePointer = frame->previous;
        uintptr_t returnPC = reinterpret_cast<uintptr_t>(frame->returnAddress);
        BxGCFrameLookup lookup = brixGCFindFrame(*mapSet, returnPC);
        if (lookup.function &&
                lookup.function->safepointCount != 0 &&
                !lookup.safepoint) {
            brixGCValidationReportMissingSafepoint(
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
            callerFramePointer && brixGCFrameInStack(callerFramePointer, stackLow, stackHigh);
        if (callerFramePointer && !callerFrameInStack) {
            if (lookup.function && lookup.function->safepointCount) {
                brixGCValidationReportInvalidFrameLink(
                    returnPC,
                    depth,
                    frame,
                    callerFramePointer);
            }
            conservativeFallback = true;
            break;
        }

        if (callerFrameInStack && lookup.map) {
            if (!brixGCTraceSafepointRoots(
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
            brixGCRestoreSavedRegisters(
                *lookup.map,
                lookup.function,
                callerFramePointer,
                registers);
            brixGCScanConservativeRegisters(registers, visitor, data);
        }

        if (
            !callerFrameInStack ||
            reinterpret_cast<uintptr_t>(callerFramePointer) <= reinterpret_cast<uintptr_t>(frame)
        ) {
            uintptr_t frameEnd = reinterpret_cast<uintptr_t>(frame) + sizeof(BxGCFrameRecord);
            BxGCFrameRecord* resyncFrame =
                brixGCFindResyncFrame(*mapSet, frameEnd, stackLow, stackHigh);
            if (resyncFrame) {
                brixGCScanConservativeStackRange(
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
        frame = static_cast<BxGCFrameRecord*>(callerFramePointer);
    }

    if (conservativeFallback || brixGCAlwaysScanConservativeStacks()) {
        brixGCScanConservativeStackRange(context->sp, stackLow, stackHigh, visitor, data);
    }
}


extern "C" void
brixGCScanConservativeContextRoots(
    const BrixGCContextSnapshot* context,
    void* stackBase,
    size_t stackSize,
    BxGCStackRootVisitor visitor,
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

    BxGCRegisterBank registers;
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
    brixGCScanConservativeRegisters(registers, visitor, data);

    uintptr_t stackLow = reinterpret_cast<uintptr_t>(stackBase);
    uintptr_t stackHigh = stackLow + stackSize;
    brixGCScanConservativeStackRange(context->sp, stackLow, stackHigh, visitor, data);
}

extern "C" size_t
brixGCStorageRootRecordCapacity(size_t taskCount, size_t threadCount) {
    const BxGCRootMapSet* mapSet = NULL;
    if (!brixGCGetRootMapSet(&mapSet)) {
        return 0;
    }

    size_t rootCapacity = 0;
    for (const BxGCRootMapView& map : mapSet->maps) {
        for (uint32_t i = 0; i < map.header->recordCount; ++i) {
            const BxGCRootMapRecord& record = map.records[i];
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
brixGCVisitStorageRootRecords(BrixGCStorageRootRecordVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    const BxGCRootMapSet* mapSet = NULL;
    if (!brixGCGetRootMapSet(&mapSet)) {
        return;
    }

    for (const BxGCRootMapView& map : mapSet->maps) {
        for (uint32_t i = 0; i < map.header->recordCount; ++i) {
            const BxGCRootMapRecord& record = map.records[i];
            void* descriptor = reinterpret_cast<void*>(
                brixGCNormalizeImagePointer(record.descriptor));
            if (!descriptor) {
                if (brixGCShouldAbortOnMapFailure()) {
                    brixGCValidationAbort("storage root record has no descriptor");
                }
                continue;
            }

            if (record.kind == 1) {
                void* base = reinterpret_cast<void*>(brixGCNormalizeImagePointer(record.base));
                if (base) {
                    BrixGCStorageRootRecord root { record.kind, base, descriptor };
                    visitor(&root, data);
                }
            } else if (record.kind == 2 || record.kind == 3) {
                BrixGCStorageRootRecord root { record.kind, NULL, descriptor };
                visitor(&root, data);
            } else if (brixGCShouldAbortOnMapFailure()) {
                brixGCValidationAbort("invalid storage root record kind");
            }
        }
    }
}
