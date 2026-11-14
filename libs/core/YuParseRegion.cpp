#include "YuCore/YuCore.h"
#include "YuCore/YuParseRegion.h"

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuTaskRuntimeContext.h>
#include <YuGC/YuGCRuntime.h>
#include <YuGC/YuGCTemporaryRoots.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>

namespace {

static const size_t kParseRegionBlockBytes = 256 * 1024;
static const size_t kParseRegionAlignment = 16;
static const uint64_t kParseRegionRecordMagic = 0x59554d4950524547ull;

enum ParseRegionAllocationKind : uint32_t {
    ParseRegionObject = 1,
    ParseRegionBuffer = 2
};

struct alignas(kParseRegionAlignment) ParseRegionRecord {
    uint64_t magic;
    size_t payloadSize;
    void* descriptor;
    char* forwarding;
    size_t itemSize;
    size_t itemCount;
    uint32_t kind;
    uint32_t reserved;
};

static_assert(
    sizeof(ParseRegionRecord) % kParseRegionAlignment == 0,
    "parse-region payloads must retain their requested alignment");

struct ParseRegionBlock {
    ParseRegionBlock* next;
    size_t capacity;
    size_t used;
    size_t logicalBase;
    alignas(kParseRegionAlignment) unsigned char data[1];
};

struct ParseRegion {
    ParseRegionBlock* first;
    ParseRegionBlock* current;
    size_t logicalUsed;
    size_t allocationScopeLine;
    size_t allocationScopeColumn;
};

static std::atomic<size_t> gParseRegionObjectAllocationCount {0};
static std::atomic<size_t> gParseRegionBufferAllocationCount {0};
static std::atomic<size_t> gParseRegionPromotionCount {0};

static size_t
alignRegionSize(size_t value) {
    if (value > std::numeric_limits<size_t>::max() - (kParseRegionAlignment - 1)) {
        return 0;
    }
    return (value + kParseRegionAlignment - 1) & ~(kParseRegionAlignment - 1);
}

static ParseRegionBlock*
newRegionBlock(size_t minimumCapacity) {
    size_t capacity = std::max(kParseRegionBlockBytes, minimumCapacity);
    if (capacity > std::numeric_limits<size_t>::max() - offsetof(ParseRegionBlock, data)) {
        return nullptr;
    }
    void* storage = std::malloc(offsetof(ParseRegionBlock, data) + capacity);
    if (!storage) {
        return nullptr;
    }
    ParseRegionBlock* block = static_cast<ParseRegionBlock*>(storage);
    block->next = nullptr;
    block->capacity = capacity;
    block->used = 0;
    block->logicalBase = 0;
    return block;
}

static void
destroyRegion(void* opaque) {
    ParseRegion* region = static_cast<ParseRegion*>(opaque);
    if (!region) {
        return;
    }
    ParseRegionBlock* block = region->first;
    while (block) {
        ParseRegionBlock* next = block->next;
        std::free(block);
        block = next;
    }
    delete region;
}

static void
scanRegion(
    void* opaque,
    YuParseRegionAllocationVisitor visitor,
    void* data
) {
    ParseRegion* region = static_cast<ParseRegion*>(opaque);
    if (!region || !visitor) {
        return;
    }
    for (ParseRegionBlock* block = region->first; block; block = block->next) {
        size_t offset = 0;
        while (offset < block->used) {
            ParseRegionRecord* record = reinterpret_cast<ParseRegionRecord*>(block->data + offset);
            if (record->magic != kParseRegionRecordMagic) {
                std::abort();
            }
            char* payload = reinterpret_cast<char*>(record + 1);
            if (record->descriptor) {
                visitor(payload, record->payloadSize, record->descriptor, data);
            }
            if (record->forwarding) {
                visitor(record->forwarding, 0, nullptr, data);
            }
            size_t recordSize = alignRegionSize(sizeof(ParseRegionRecord) + record->payloadSize);
            if (!recordSize || recordSize > block->used - offset) {
                std::abort();
            }
            offset += recordSize;
        }
    }
}

static ParseRegion*
activeRegion(bool create) {
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    if (!context) {
        return nullptr;
    }
    ParseRegion* region = static_cast<ParseRegion*>(context->parseRegion);
    if (!region && create) {
        region = new (std::nothrow) ParseRegion { nullptr, nullptr, 0, 0, 0 };
        if (!region) {
            return nullptr;
        }
        context->parseRegion = region;
        context->parseRegionScanner = scanRegion;
        context->parseRegionDestroyer = destroyRegion;
    }
    return region;
}

static ParseRegionRecord*
findRecord(ParseRegion* region, char* pointer, size_t* interiorOffset = nullptr) {
    if (!region || !pointer) {
        return nullptr;
    }
    for (ParseRegionBlock* block = region->first; block; block = block->next) {
        size_t offset = 0;
        while (offset < block->used) {
            ParseRegionRecord* record = reinterpret_cast<ParseRegionRecord*>(block->data + offset);
            if (record->magic != kParseRegionRecordMagic) {
                std::abort();
            }
            char* payload = reinterpret_cast<char*>(record + 1);
            uintptr_t payloadAddress = reinterpret_cast<uintptr_t>(payload);
            uintptr_t pointerAddress = reinterpret_cast<uintptr_t>(pointer);
            if (pointerAddress >= payloadAddress &&
                    pointerAddress - payloadAddress < record->payloadSize) {
                if (interiorOffset) {
                    *interiorOffset = pointerAddress - payloadAddress;
                }
                return record;
            }
            size_t recordSize = alignRegionSize(sizeof(ParseRegionRecord) + record->payloadSize);
            if (!recordSize || recordSize > block->used - offset) {
                std::abort();
            }
            offset += recordSize;
        }
    }
    return nullptr;
}

static bool
regionOwnsAddress(ParseRegion* region, char* pointer) {
    if (!region || !pointer) {
        return false;
    }
    uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
    for (ParseRegionBlock* block = region->first; block; block = block->next) {
        uintptr_t begin = reinterpret_cast<uintptr_t>(block->data);
        if (address >= begin && address - begin < block->used) {
            return true;
        }
    }
    return false;
}

static ParseRegionRecord*
findBaseRecord(ParseRegion* region, char* pointer) {
    if (!regionOwnsAddress(region, pointer)) {
        return nullptr;
    }
    ParseRegionRecord* record = reinterpret_cast<ParseRegionRecord*>(pointer) - 1;
    if (record->magic != kParseRegionRecordMagic ||
            reinterpret_cast<char*>(record + 1) != pointer) {
        return nullptr;
    }
    return record;
}

static char*
allocateRegion(
    ParseRegion* region,
    size_t payloadSize,
    void* descriptor,
    ParseRegionAllocationKind kind,
    size_t itemSize,
    size_t itemCount
) {
    if (!region || payloadSize == 0 ||
            payloadSize > std::numeric_limits<size_t>::max() - sizeof(ParseRegionRecord)) {
        return nullptr;
    }
    size_t recordSize = alignRegionSize(sizeof(ParseRegionRecord) + payloadSize);
    if (!recordSize) {
        return nullptr;
    }

    ParseRegionBlock* block = region->current;
    if (!block || recordSize > block->capacity - block->used) {
        ParseRegionBlock* candidate = block ? block->next : region->first;
        if (candidate && candidate->capacity >= recordSize) {
            block = candidate;
            block->used = 0;
            block->logicalBase = region->logicalUsed;
        } else {
            ParseRegionBlock* created = newRegionBlock(recordSize);
            if (!created) {
                return nullptr;
            }
            created->logicalBase = region->logicalUsed;
            if (block) {
                created->next = block->next;
                block->next = created;
            } else {
                created->next = region->first;
                region->first = created;
            }
            block = created;
        }
        region->current = block;
    }

    ParseRegionRecord* record = reinterpret_cast<ParseRegionRecord*>(block->data + block->used);
    record->magic = kParseRegionRecordMagic;
    record->payloadSize = payloadSize;
    record->descriptor = descriptor;
    record->forwarding = nullptr;
    record->itemSize = itemSize;
    record->itemCount = itemCount;
    record->kind = kind;
    record->reserved = 0;
    char* payload = reinterpret_cast<char*>(record + 1);
    std::memset(payload, 0, payloadSize);
    block->used += recordSize;
    region->logicalUsed += recordSize;
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    if (context) {
        context->parseRegionLogicalUsed = region->logicalUsed;
    }
    (kind == ParseRegionObject
        ? gParseRegionObjectAllocationCount
        : gParseRegionBufferAllocationCount).fetch_add(1, std::memory_order_relaxed);
    return payload;
}

struct PromotionVisit {
    ParseRegion* region;
    void* graphRoot;
};

static char* promoteRegionPointer(PromotionVisit* visit, char* pointer);

static void
promotePointerSlot(char** slot, void* data) {
    if (!slot || !data || !*slot) {
        return;
    }
    PromotionVisit* visit = static_cast<PromotionVisit*>(data);
    char* promoted = promoteRegionPointer(visit, *slot);
    if (promoted == *slot) {
        return;
    }
    *slot = promoted;
    yumiWriteBarrier(reinterpret_cast<char*>(slot), promoted);
}

static char*
promoteRegionPointer(PromotionVisit* visit, char* pointer) {
    if (!visit) {
        return pointer;
    }
    ParseRegionRecord* record = findBaseRecord(visit->region, pointer);
    if (!record) {
        return pointer;
    }
    if (record->forwarding) {
        return record->forwarding;
    }

    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    size_t savedDepth = context ? context->parseRegionAllocationScopeDepth : 0;
    if (context) {
        context->parseRegionAllocationScopeDepth = 0;
    }
    char* copy = record->kind == ParseRegionBuffer
        ? yumiAllocBufferDescribed(record->itemSize, record->itemCount, record->descriptor)
        : yumiAllocObjectDescribed(record->payloadSize, record->descriptor);
    if (context) {
        context->parseRegionAllocationScopeDepth = savedDepth;
    }
    if (!copy) {
        return nullptr;
    }
    gParseRegionPromotionCount.fetch_add(1, std::memory_order_relaxed);
    record->forwarding = copy;
    if (!visit->graphRoot) {
        visit->graphRoot = yumiAddTemporaryRoot(copy);
    }
    char* payload = reinterpret_cast<char*>(record + 1);
    std::memcpy(copy, payload, record->payloadSize);
    if (record->descriptor) {
        yumiGCVisitDescriptorPointerSlots(
            copy,
            record->payloadSize,
            record->descriptor,
            promotePointerSlot,
            visit);
    }
    return copy;
}

struct PromotionValueVisit {
    ParseRegion* region;
};

static void
promoteValuePointerSlot(char** slot, void* data) {
    if (!slot || !data || !*slot) {
        return;
    }
    PromotionValueVisit* valueVisit = static_cast<PromotionValueVisit*>(data);
    PromotionVisit graphVisit { valueVisit->region, nullptr };
    char* promoted = promoteRegionPointer(&graphVisit, *slot);
    if (promoted != *slot) {
        *slot = promoted;
        yumiWriteBarrier(reinterpret_cast<char*>(slot), promoted);
    }
    if (graphVisit.graphRoot) {
        yumiRemoveTemporaryRoot(graphVisit.graphRoot);
    }
}

}

extern "C" size_t
yumiParseRegionMark(void) {
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    return context ? context->parseRegionLogicalUsed : 0;
}

extern "C" void
yumiParseRegionReset(size_t mark) {
    ParseRegion* region = activeRegion(false);
    if (!region || mark > region->logicalUsed) {
        return;
    }
    ParseRegionBlock* target = nullptr;
    for (ParseRegionBlock* block = region->first; block; block = block->next) {
        if (block->used && mark >= block->logicalBase &&
                mark <= block->logicalBase + block->used) {
            target = block;
            break;
        }
    }
    if (!target && mark != 0) {
        return;
    }
    bool afterTarget = mark == 0;
    for (ParseRegionBlock* block = region->first; block; block = block->next) {
        if (afterTarget) {
            block->used = 0;
            continue;
        }
        if (block == target) {
            block->used = mark - block->logicalBase;
            afterTarget = true;
        }
    }
    region->current = target;
    region->logicalUsed = mark;
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    if (context) {
        context->parseRegionLogicalUsed = mark;
    }
}

extern "C" void
yumiParseRegionEnterAllocationScope(size_t line, size_t column) {
    ParseRegion* region = activeRegion(true);
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    if (region && context) {
        if (!context->parseRegionAllocationScopeDepth) {
            region->allocationScopeLine = line;
            region->allocationScopeColumn = column;
        }
        ++context->parseRegionAllocationScopeDepth;
    }
}

extern "C" void
yumiParseRegionLeaveAllocationScope(void) {
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    if (context && context->parseRegionAllocationScopeDepth) {
        --context->parseRegionAllocationScopeDepth;
    }
}

extern "C" size_t
yumiParseRegionSuspendAllocationScopes(void) {
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    if (!context) {
        return 0;
    }
    size_t depth = context->parseRegionAllocationScopeDepth;
    context->parseRegionAllocationScopeDepth = 0;
    return depth;
}

extern "C" void
yumiParseRegionResumeAllocationScopes(size_t depth) {
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    if (depth) {
        activeRegion(true);
    }
    if (context) {
        context->parseRegionAllocationScopeDepth = depth;
    }
}

extern "C" bool
yumiParseRegionAllocationActive(void) {
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    return context && context->parseRegionAllocationScopeDepth != 0;
}

extern "C" char*
yumiParseRegionAllocObject(size_t size, void* descriptor) {
    return allocateRegion(activeRegion(true), size, descriptor, ParseRegionObject, size, 1);
}

extern "C" char*
yumiParseRegionAllocBuffer(size_t itemSize, size_t count, void* descriptor) {
    if (!itemSize || !count || count > std::numeric_limits<size_t>::max() / itemSize) {
        return nullptr;
    }
    return allocateRegion(
        activeRegion(true),
        itemSize * count,
        descriptor,
        ParseRegionBuffer,
        itemSize,
        count);
}

char*
yumiParseRegionResizeBuffer(
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* descriptor
) {
    ParseRegion* region = activeRegion(false);
    size_t interiorOffset = 0;
    ParseRegionRecord* record = findRecord(region, buffer, &interiorOffset);
    if (!record || interiorOffset != 0) {
        return nullptr;
    }
    if (!newSize) {
        return nullptr;
    }
    if (newSize <= oldSize && !descriptor) {
        return buffer;
    }
    char* resized = allocateRegion(
        region,
        newSize,
        descriptor,
        ParseRegionBuffer,
        1,
        newSize);
    if (resized && buffer && oldSize) {
        std::memcpy(resized, buffer, std::min(oldSize, newSize));
    }
    return resized;
}

extern "C" char*
yumiParseRegionPromote(char* pointer) {
    PromotionVisit visit { activeRegion(false), nullptr };
    char* promoted = promoteRegionPointer(&visit, pointer);
    if (visit.graphRoot) {
        yumiRemoveTemporaryRoot(visit.graphRoot);
    }
    return promoted;
}

extern "C" void
yumiParseRegionPromoteValue(char* value, size_t byteSize, void* descriptor) {
    ParseRegion* region = activeRegion(false);
    if (!region || !value || !byteSize || !descriptor) {
        return;
    }
    PromotionValueVisit visit { region };
    yumiGCVisitDescriptorPointerSlots(
        value,
        byteSize,
        descriptor,
        promoteValuePointerSlot,
        &visit);
}

extern "C" bool
yumiParseRegionOwns(char* pointer) {
    return regionOwnsAddress(activeRegion(false), pointer);
}

extern "C" void
yumiParseRegionValidateStore(char* source, char* value, size_t line, size_t column) {
    if (source && value && yumiGCIsManagedAddress(source) &&
            regionOwnsAddress(activeRegion(false), value)) {
        ParseRegion* region = activeRegion(false);
        std::fprintf(
            stderr,
            "Yumi parse-region invariant failed at %zu:%zu: GC heap points into parser "
            "region (active allocation scope began at %zu:%zu)\n",
            line,
            column,
            region ? region->allocationScopeLine : 0,
            region ? region->allocationScopeColumn : 0);
        std::abort();
    }
}

struct ValidateBulkStoreVisit {
    ParseRegion* region;
    bool invalid;
};

static void
validateBulkStorePointer(char** slot, void* data) {
    ValidateBulkStoreVisit* visit = static_cast<ValidateBulkStoreVisit*>(data);
    if (slot && visit && regionOwnsAddress(visit->region, *slot)) {
        visit->invalid = true;
    }
}

extern "C" void
yumiParseRegionValidateBulkStore(
    char* source,
    char* value,
    size_t byteSize,
    void* descriptor,
    size_t line,
    size_t column
) {
    if (!source || !value || !byteSize || !descriptor ||
            !yumiGCIsManagedAddress(source)) {
        return;
    }
    ValidateBulkStoreVisit visit { activeRegion(false), false };
    yumiGCVisitDescriptorPointerSlots(
        value,
        byteSize,
        descriptor,
        validateBulkStorePointer,
        &visit);
    if (visit.invalid) {
        std::fprintf(
            stderr,
            "Yumi parse-region invariant failed at %zu:%zu: aggregate GC heap store "
            "contains a parser-region pointer\n",
            line,
            column);
        std::abort();
    }
}

extern "C" void
yumiParseRegionCancelAllocationScopes(void) {
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    if (context) {
        context->parseRegionAllocationScopeDepth = 0;
    }
}

extern "C" size_t
yumiParseRegionObjectAllocationCount(void) {
    return gParseRegionObjectAllocationCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiParseRegionBufferAllocationCount(void) {
    return gParseRegionBufferAllocationCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiParseRegionPromotionCount(void) {
    return gParseRegionPromotionCount.load(std::memory_order_relaxed);
}

void
yumiParseRegionResetAll(void) {
    yumiParseRegionCancelAllocationScopes();
    yumiParseRegionReset(0);
}
