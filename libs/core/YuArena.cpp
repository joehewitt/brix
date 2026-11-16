#include "YuCore/YuCore.h"
#include "YuCore/YuPanic.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>

namespace {

static const size_t kDefaultArenaBlockBytes = 4096;
static const size_t kDefaultArenaAlignment = 16;
static const size_t kMaximumArenaAlignment = 1u << 20;
static const size_t kArenaSlotsPerPage = 64;
static_assert(sizeof(size_t) >= 8, "Arena marks require a 64-bit size_t");
static const size_t kArenaMarkOffsetBits = 48;
static const size_t kArenaMarkOffsetMask = (size_t(1) << kArenaMarkOffsetBits) - 1;

struct ArenaBlock {
    ArenaBlock* next;
    void* storage;
    char* payload;
    size_t capacity;
    size_t used;
    size_t maximumAlignment;
    bool dedicated;
};

struct ArenaTracedReservation {
    char* pointer;
    size_t byteSize;
    void* descriptor;
    size_t endOffset;
};

struct ArenaSlot {
    std::atomic<size_t> incarnation;
    size_t generation;
    std::atomic<bool> live;
    ArenaSlot* nextFree;
    ArenaBlock* activeFirst;
    ArenaBlock* activeLast;
    ArenaBlock* retainedFirst;
    std::mutex tracedMutex;
    ArenaTracedReservation* traced;
    size_t tracedCount;
    size_t tracedCapacity;
    size_t activeBlockCount;
    size_t retainedBlockCount;
    std::atomic<size_t> activeBytes;
    size_t reservedBytes;
    size_t retainedBytes;
    size_t peakActiveBytes;
    size_t nextBlockCapacity;
    size_t systemBlockAllocationCount;
    size_t resetCount;
    size_t rewindCount;
};

static_assert(offsetof(ArenaSlot, incarnation) == YumiArenaIncarnationOffset, "Arena incarnation ABI");
static_assert(offsetof(ArenaSlot, generation) == YumiArenaGenerationOffset, "Arena generation ABI");
static_assert(offsetof(ArenaSlot, live) == YumiArenaLiveOffset, "Arena live ABI");
static_assert(sizeof(std::atomic<bool>) == 1, "Arena live flag is one byte");

struct ArenaSlotPage {
    ArenaSlotPage* next;
    ArenaSlot slots[kArenaSlotsPerPage];
};

static std::atomic<size_t> gArenaSystemBlockAllocationCount {0};
static std::atomic<size_t> gArenaSystemBlockFreeCount {0};
static std::atomic<size_t> gArenaMeasuredActiveBytes {0};
static std::atomic<size_t> gArenaReservedBytes {0};
static std::atomic<size_t> gArenaRetainedBytes {0};
static std::atomic<size_t> gArenaPeakActiveBytes {0};
static std::atomic<size_t> gArenaMarkCount {0};
static std::atomic<size_t> gArenaRewindCount {0};
static std::atomic<size_t> gArenaResetCount {0};
static std::atomic<size_t> gArenaManagedAllocationCount {0};
static std::atomic<size_t> gArenaSlotCount {0};
static std::atomic<size_t> gArenaLiveSlotCount {0};
static std::mutex gArenaSlotMutex;
static ArenaSlotPage* gArenaSlotPages = nullptr;
static ArenaSlot* gArenaFreeSlots = nullptr;

static bool
isPowerOfTwo(size_t value) {
    return value && (value & (value - 1)) == 0;
}

static bool
checkedAdd(size_t left, size_t right, size_t* result) {
    if (left > std::numeric_limits<size_t>::max() - right) {
        return false;
    }
    *result = left + right;
    return true;
}

static size_t
alignOffset(uintptr_t address, size_t alignment) {
    const size_t mask = alignment - 1;
    return static_cast<size_t>((alignment - (address & mask)) & mask);
}

static size_t
arenaMarkEpoch(const ArenaSlot* arena, size_t incarnation) {
    const uintptr_t address = reinterpret_cast<uintptr_t>(arena);
    size_t epoch = ((address >> 4) ^ incarnation ^ arena->generation) & 0xffff;
    return epoch ? epoch : 1;
}

static void
updatePeak(std::atomic<size_t>& peak, size_t value) {
    size_t previous = peak.load(std::memory_order_relaxed);
    while (previous < value &&
           !peak.compare_exchange_weak(previous, value, std::memory_order_relaxed)) {
    }
}

static size_t
activeBytesOf(const ArenaSlot* arena) {
    return arena->activeBytes.load(std::memory_order_relaxed);
}

static void
setActiveBytes(ArenaSlot* arena, size_t value) {
    arena->activeBytes.store(value, std::memory_order_relaxed);
}

static bool
measuresPeak() {
    return __builtin_expect(yumiBenchMetricsEnabledFlag != 0, 0);
}

static void
measureRemoved(size_t removedBytes) {
    if (measuresPeak()) {
        gArenaMeasuredActiveBytes.fetch_sub(removedBytes, std::memory_order_relaxed);
    }
}

static ArenaBlock*
newArenaBlock(size_t capacity, size_t alignment, bool dedicated) {
    size_t storageSize = 0;
    if (!checkedAdd(capacity, alignment - 1, &storageSize)) {
        return nullptr;
    }
    void* storage = std::malloc(storageSize ? storageSize : 1);
    if (!storage) {
        return nullptr;
    }
    ArenaBlock* block = new (std::nothrow) ArenaBlock();
    if (!block) {
        std::free(storage);
        return nullptr;
    }
    const uintptr_t base = reinterpret_cast<uintptr_t>(storage);
    const size_t adjustment = alignOffset(base, alignment);
    block->next = nullptr;
    block->storage = storage;
    block->payload = static_cast<char*>(storage) + adjustment;
    block->capacity = capacity;
    block->used = 0;
    block->maximumAlignment = alignment;
    block->dedicated = dedicated;
    return block;
}

static void
freeArenaBlock(ArenaBlock* block) {
    if (!block) {
        return;
    }
    std::free(block->storage);
    delete block;
    gArenaSystemBlockFreeCount.fetch_add(1, std::memory_order_relaxed);
}

static void
appendActive(ArenaSlot* arena, ArenaBlock* block) {
    block->next = nullptr;
    if (arena->activeLast) {
        arena->activeLast->next = block;
    } else {
        arena->activeFirst = block;
    }
    arena->activeLast = block;
    ++arena->activeBlockCount;
}

static void
appendRetained(ArenaSlot* arena, ArenaBlock* block) {
    block->used = 0;
    block->next = arena->retainedFirst;
    arena->retainedFirst = block;
    ++arena->retainedBlockCount;
    arena->retainedBytes += block->capacity;
    gArenaRetainedBytes.fetch_add(block->capacity, std::memory_order_relaxed);
}

static ArenaBlock*
takeRetained(ArenaSlot* arena, size_t capacity, size_t alignment) {
    ArenaBlock** link = &arena->retainedFirst;
    while (*link) {
        ArenaBlock* block = *link;
        if (block->capacity >= capacity && block->maximumAlignment >= alignment) {
            *link = block->next;
            block->next = nullptr;
            --arena->retainedBlockCount;
            arena->retainedBytes -= block->capacity;
            gArenaRetainedBytes.fetch_sub(block->capacity, std::memory_order_relaxed);
            return block;
        }
        link = &block->next;
    }
    return nullptr;
}

static ArenaBlock*
acquireBlock(ArenaSlot* arena, size_t byteCount, size_t alignment) {
    const bool dedicated = byteCount > arena->nextBlockCapacity || alignment > kDefaultArenaAlignment;
    size_t capacity = dedicated ? byteCount : std::max(arena->nextBlockCapacity, byteCount);
    // A retained block only needs to fit the pending reservation.  Requiring it to
    // match the allocator's next growth target defeats reset/rewind reuse whenever
    // that target has grown beyond the retained block's capacity.
    ArenaBlock* block = takeRetained(arena, byteCount, alignment);
    if (!block) {
        block = newArenaBlock(capacity, std::max(alignment, kDefaultArenaAlignment), dedicated);
        if (!block) {
            return nullptr;
        }
        ++arena->systemBlockAllocationCount;
        arena->reservedBytes += block->capacity;
        gArenaSystemBlockAllocationCount.fetch_add(1, std::memory_order_relaxed);
        gArenaReservedBytes.fetch_add(block->capacity, std::memory_order_relaxed);
        if (!dedicated &&
            arena->nextBlockCapacity <= std::numeric_limits<size_t>::max() / 2) {
            arena->nextBlockCapacity = std::max(arena->nextBlockCapacity * 2, capacity);
        }
    }
    appendActive(arena, block);
    return block;
}

static char*
reserveFromBlock(ArenaSlot* arena, ArenaBlock* block, size_t byteCount, size_t alignment) {
    const uintptr_t cursor = reinterpret_cast<uintptr_t>(block->payload + block->used);
    const size_t padding = alignOffset(cursor, alignment);
    size_t end = 0;
    if (!checkedAdd(block->used, padding, &end) || !checkedAdd(end, byteCount, &end) ||
        end > block->capacity) {
        return nullptr;
    }
    char* result = block->payload + block->used + padding;
    const size_t growth = end - block->used;
    block->used = end;
    const size_t active = activeBytesOf(arena) + growth;
    setActiveBytes(arena, active);
    arena->peakActiveBytes = std::max(arena->peakActiveBytes, active);
    if (measuresPeak()) {
        updatePeak(
            gArenaPeakActiveBytes,
            gArenaMeasuredActiveBytes.fetch_add(growth, std::memory_order_relaxed) + growth);
    }
    return result;
}

static void
retainActiveSuffix(ArenaSlot* arena, ArenaBlock* first) {
    ArenaBlock* block = first;
    while (block) {
        ArenaBlock* next = block->next;
        appendRetained(arena, block);
        block = next;
    }
}

static void
freeBlockList(ArenaBlock* block) {
    while (block) {
        ArenaBlock* next = block->next;
        freeArenaBlock(block);
        block = next;
    }
}

static bool
appendTraced(ArenaSlot* arena, char* pointer, size_t byteSize, void* descriptor) {
    std::lock_guard<std::mutex> lock(arena->tracedMutex);
    if (arena->tracedCount == arena->tracedCapacity) {
        const size_t capacity = arena->tracedCapacity ? arena->tracedCapacity * 2 : 64;
        if (capacity > std::numeric_limits<size_t>::max() / sizeof(ArenaTracedReservation)) {
            return false;
        }
        void* grown = std::realloc(arena->traced, capacity * sizeof(ArenaTracedReservation));
        if (!grown) {
            return false;
        }
        arena->traced = static_cast<ArenaTracedReservation*>(grown);
        arena->tracedCapacity = capacity;
    }
    arena->traced[arena->tracedCount++] = {pointer, byteSize, descriptor, activeBytesOf(arena)};
    return true;
}

static void
dropTracedAfter(ArenaSlot* arena, size_t activeBytes) {
    std::lock_guard<std::mutex> lock(arena->tracedMutex);
    while (arena->tracedCount && arena->traced[arena->tracedCount - 1].endOffset > activeBytes) {
        --arena->tracedCount;
    }
}

static void
freeTraced(ArenaSlot* arena) {
    std::lock_guard<std::mutex> lock(arena->tracedMutex);
    std::free(arena->traced);
    arena->traced = nullptr;
    arena->tracedCount = 0;
    arena->tracedCapacity = 0;
}

static ArenaSlot*
slot(void* opaque, size_t incarnation) {
    ArenaSlot* candidate = static_cast<ArenaSlot*>(opaque);
    return candidate && candidate->live.load(std::memory_order_acquire) &&
                   candidate->incarnation.load(std::memory_order_relaxed) == incarnation
               ? candidate
               : nullptr;
}

static ArenaSlot*
takeArenaSlot() {
    std::lock_guard<std::mutex> lock(gArenaSlotMutex);
    if (!gArenaFreeSlots) {
        ArenaSlotPage* page = new (std::nothrow) ArenaSlotPage();
        if (!page) {
            return nullptr;
        }
        page->next = gArenaSlotPages;
        gArenaSlotPages = page;
        for (ArenaSlot& fresh : page->slots) {
            fresh.incarnation.store(1, std::memory_order_relaxed);
            fresh.generation = 0;
            fresh.live.store(false, std::memory_order_relaxed);
            fresh.traced = nullptr;
            fresh.tracedCount = 0;
            fresh.tracedCapacity = 0;
            fresh.nextFree = gArenaFreeSlots;
            gArenaFreeSlots = &fresh;
        }
        gArenaSlotCount.fetch_add(kArenaSlotsPerPage, std::memory_order_relaxed);
    }
    ArenaSlot* result = gArenaFreeSlots;
    gArenaFreeSlots = result->nextFree;
    result->nextFree = nullptr;
    return result;
}

static void
initializeArenaSlot(ArenaSlot* arena) {
    arena->activeFirst = nullptr;
    arena->activeLast = nullptr;
    arena->retainedFirst = nullptr;
    ++arena->generation;
    arena->activeBlockCount = 0;
    arena->retainedBlockCount = 0;
    setActiveBytes(arena, 0);
    arena->reservedBytes = 0;
    arena->retainedBytes = 0;
    arena->peakActiveBytes = 0;
    arena->nextBlockCapacity = kDefaultArenaBlockBytes;
    arena->systemBlockAllocationCount = 0;
    arena->resetCount = 0;
    arena->rewindCount = 0;
}

static void
recycleArenaSlot(ArenaSlot* arena) {
    std::lock_guard<std::mutex> lock(gArenaSlotMutex);
    arena->live.store(false, std::memory_order_release);
    ++arena->generation;
    gArenaLiveSlotCount.fetch_sub(1, std::memory_order_relaxed);
    const size_t incarnation = arena->incarnation.load(std::memory_order_relaxed);
    if (incarnation == std::numeric_limits<size_t>::max()) {
        return;
    }
    arena->incarnation.store(incarnation + 1, std::memory_order_relaxed);
    arena->nextFree = gArenaFreeSlots;
    gArenaFreeSlots = arena;
}

} // namespace

extern "C" YUMI_GC_LEAF void*
yumiArenaCreate(size_t initialCapacity) {
    ArenaSlot* arena = takeArenaSlot();
    if (!arena) {
        return nullptr;
    }
    initializeArenaSlot(arena);

    if (initialCapacity) {
        ArenaBlock* block = newArenaBlock(
            initialCapacity, kDefaultArenaAlignment, initialCapacity > kDefaultArenaBlockBytes);
        if (!block) {
            std::lock_guard<std::mutex> lock(gArenaSlotMutex);
            arena->nextFree = gArenaFreeSlots;
            gArenaFreeSlots = arena;
            return nullptr;
        }
        ++arena->systemBlockAllocationCount;
        arena->reservedBytes = block->capacity;
        gArenaSystemBlockAllocationCount.fetch_add(1, std::memory_order_relaxed);
        gArenaReservedBytes.fetch_add(block->capacity, std::memory_order_relaxed);
        appendRetained(arena, block);
    }
    arena->live.store(true, std::memory_order_release);
    gArenaLiveSlotCount.fetch_add(1, std::memory_order_relaxed);
    return arena;
}

extern "C" YUMI_GC_LEAF size_t
yumiArenaIncarnation(void* opaque) {
    ArenaSlot* arena = static_cast<ArenaSlot*>(opaque);
    return arena ? arena->incarnation.load(std::memory_order_acquire) : 0;
}

extern "C" YUMI_GC_LEAF char*
yumiArenaReserve(
    void* opaque,
    size_t incarnation,
    size_t byteCount,
    size_t alignment,
    bool zeroInitialize) {
    ArenaSlot* arena = slot(opaque, incarnation);
    if (!arena || !isPowerOfTwo(alignment) ||
        alignment > kMaximumArenaAlignment) {
        return nullptr;
    }
    if (!byteCount) {
        return static_cast<char*>(opaque);
    }

    ArenaBlock* block = arena->activeLast;
    char* result = block ? reserveFromBlock(arena, block, byteCount, alignment) : nullptr;
    if (!result) {
        block = acquireBlock(arena, byteCount, alignment);
        if (!block) {
            return nullptr;
        }
        result = reserveFromBlock(arena, block, byteCount, alignment);
        if (!result) {
            return nullptr;
        }
    }
    if (zeroInitialize) {
        std::memset(result, 0, byteCount);
    }
    return result;
}

extern "C" YUMI_GC_LEAF void
yumiArenaAllocationFailed(void* opaque, size_t incarnation, size_t alignment) {
    const char* message = "Arena allocation failed";
    if (!slot(opaque, incarnation)) {
        message = "Arena allocation failed: the Arena is released";
    } else if (!isPowerOfTwo(alignment) || alignment > kMaximumArenaAlignment) {
        message = "Arena allocation failed: the alignment is invalid";
    }
    yumiImmediateAbort(message, nullptr, 0, 0);
    std::abort();
}

extern "C" char*
yumiArenaReserveDescribed(
    void* opaque,
    size_t incarnation,
    size_t byteCount,
    size_t alignment,
    bool zeroInitialize,
    void* descriptor) {
    ArenaSlot* arena = slot(opaque, incarnation);
    if (!arena || !descriptor) {
        return nullptr;
    }
    char* result = yumiArenaReserve(
        opaque,
        incarnation,
        byteCount,
        alignment,
        zeroInitialize);
    if (!result || !byteCount) {
        return result;
    }
    if (!appendTraced(arena, result, byteCount, descriptor)) {
        return nullptr;
    }
    gArenaManagedAllocationCount.fetch_add(1, std::memory_order_relaxed);
    return result;
}

extern "C" void
yumiArenaVisitTracedReservations(
    void (*visitor)(void*, size_t, void*, void*),
    void* data) {
    if (!visitor) {
        return;
    }
    std::lock_guard<std::mutex> slots(gArenaSlotMutex);
    for (ArenaSlotPage* page = gArenaSlotPages; page; page = page->next) {
        for (ArenaSlot& arena : page->slots) {
            std::lock_guard<std::mutex> lock(arena.tracedMutex);
            for (size_t index = 0; index < arena.tracedCount; ++index) {
                const ArenaTracedReservation& reservation = arena.traced[index];
                visitor(reservation.pointer, reservation.byteSize, reservation.descriptor, data);
            }
        }
    }
}

extern "C" YUMI_GC_LEAF const YumiStringData*
yumiArenaCopyString(void* opaque, size_t incarnation, const char* value) {
    ArenaSlot* arena = slot(opaque, incarnation);
    if (!arena || !value) {
        return nullptr;
    }

    const char* source = value - sizeof(YumiStringHeader);
    const YumiStringHeader* sourceHeader =
        reinterpret_cast<const YumiStringHeader*>(source);
    size_t allocationSize = 0;
    if (!checkedAdd(sizeof(YumiStringHeader), sourceHeader->byteLength, &allocationSize) ||
        !checkedAdd(allocationSize, 1, &allocationSize)) {
        return nullptr;
    }

    char* copy = yumiArenaReserve(
        opaque, incarnation, allocationSize, alignof(YumiStringHeader), false);
    if (!copy) {
        return nullptr;
    }
    std::memcpy(copy, source, allocationSize);
    return reinterpret_cast<const YumiStringData*>(copy + sizeof(YumiStringHeader));
}

extern "C" YUMI_GC_LEAF const YumiStringData*
yumiArenaCopyCodepointSpan(
    void* opaque,
    size_t incarnation,
    const uint32_t* source,
    size_t sourceLength,
    size_t start,
    size_t end,
    bool stopAtNull) {
    if (!slot(opaque, incarnation) || (!source && sourceLength) || start > end || end > sourceLength ||
        end - start > UINT32_MAX) {
        return nullptr;
    }

    size_t byteLength = 0;
    bool isASCII = true;
    for (size_t index = start; index < end; ++index) {
        const uint32_t codepoint = source[index];
        if (!codepoint && stopAtNull) {
            end = index;
            break;
        }
        if (!codepoint || codepoint >= 0x110000 ||
            (codepoint >= 0xd800 && codepoint < 0xe000)) {
            return nullptr;
        }
        const size_t encodedLength =
            1 + (codepoint >= 0x80) + (codepoint >= 0x800) + (codepoint >= 0x10000);
        if (!checkedAdd(byteLength, encodedLength, &byteLength)) {
            return nullptr;
        }
        isASCII = isASCII && codepoint < 0x80;
    }

    size_t allocationSize = 0;
    if (!checkedAdd(sizeof(YumiStringHeader), byteLength, &allocationSize) ||
        !checkedAdd(allocationSize, 1, &allocationSize)) {
        return nullptr;
    }
    char* allocation = yumiArenaReserve(
        opaque, incarnation, allocationSize, alignof(YumiStringHeader), false);
    if (!allocation) {
        return nullptr;
    }

    YumiStringHeader* header = reinterpret_cast<YumiStringHeader*>(allocation);
    header->byteLength = byteLength;
    header->characterLength = static_cast<uint32_t>(end - start);
    header->flags =
        (isASCII ? YUMI_STRING_FLAG_ASCII : 0) | YUMI_STRING_FLAG_VALID_UTF8;
    uint8_t* output = reinterpret_cast<uint8_t*>(allocation + sizeof(YumiStringHeader));
    size_t offset = 0;
    for (size_t index = start; index < end; ++index) {
        const uint32_t codepoint = source[index];
        if (codepoint < 0x80) {
            output[offset++] = static_cast<uint8_t>(codepoint);
        } else if (codepoint < 0x800) {
            output[offset++] = static_cast<uint8_t>(0xc0 | (codepoint >> 6));
            output[offset++] = static_cast<uint8_t>(0x80 | (codepoint & 0x3f));
        } else if (codepoint < 0x10000) {
            output[offset++] = static_cast<uint8_t>(0xe0 | (codepoint >> 12));
            output[offset++] = static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3f));
            output[offset++] = static_cast<uint8_t>(0x80 | (codepoint & 0x3f));
        } else {
            output[offset++] = static_cast<uint8_t>(0xf0 | (codepoint >> 18));
            output[offset++] = static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3f));
            output[offset++] = static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3f));
            output[offset++] = static_cast<uint8_t>(0x80 | (codepoint & 0x3f));
        }
    }
    output[offset] = 0;
    return reinterpret_cast<const YumiStringData*>(output);
}

extern "C" YUMI_GC_LEAF bool
yumiArenaOwns(void* opaque, size_t incarnation, const void* value) {
    ArenaSlot* arena = slot(opaque, incarnation);
    if (!arena || !value) {
        return false;
    }
    const uintptr_t address = reinterpret_cast<uintptr_t>(value);
    for (ArenaBlock* block = arena->activeFirst; block; block = block->next) {
        const uintptr_t begin = reinterpret_cast<uintptr_t>(block->payload);
        if (address >= begin && address - begin < block->used) {
            return true;
        }
    }
    return false;
}

extern "C" YUMI_GC_LEAF size_t
yumiArenaMark(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    if (!arena || activeBytesOf(arena) > kArenaMarkOffsetMask) {
        return 0;
    }
    gArenaMarkCount.fetch_add(1, std::memory_order_relaxed);
    return (arenaMarkEpoch(arena, incarnation) << kArenaMarkOffsetBits) |
           activeBytesOf(arena);
}

extern "C" YUMI_GC_LEAF size_t
yumiArenaRewind(
    void* opaque,
    size_t incarnation,
    size_t mark) {
    ArenaSlot* arena = slot(opaque, incarnation);
    if (!arena) {
        return 1;
    }
    if (!mark || mark >> kArenaMarkOffsetBits != arenaMarkEpoch(arena, incarnation)) {
        return 2;
    }

    const size_t markActiveBytes = mark & kArenaMarkOffsetMask;
    const size_t activeBytes = activeBytesOf(arena);
    if (markActiveBytes > activeBytes) {
        return 3;
    }

    ArenaBlock* selected = nullptr;
    size_t markBlockUsed = 0;
    size_t markActiveBlockCount = 0;
    if (markActiveBytes) {
        const size_t activeLastStart = activeBytes - arena->activeLast->used;
        if (markActiveBytes > activeLastStart) {
            selected = arena->activeLast;
            markActiveBlockCount = arena->activeBlockCount;
            markBlockUsed = markActiveBytes - activeLastStart;
        } else {
            ArenaBlock* current = arena->activeFirst;
            size_t priorBytes = 0;
            while (current) {
                ++markActiveBlockCount;
                const size_t nextBytes = priorBytes + current->used;
                if (markActiveBytes <= nextBytes) {
                    selected = current;
                    markBlockUsed = markActiveBytes - priorBytes;
                    break;
                }
                priorBytes = nextBytes;
                current = current->next;
            }
            if (!selected) {
                return 3;
            }
        }
    }

    ArenaBlock* suffix = nullptr;
    if (selected) {
        yumiArrayViewsInvalidate(selected->payload + markBlockUsed, selected->used - markBlockUsed);
        suffix = selected->next;
        selected->next = nullptr;
        selected->used = markBlockUsed;
        arena->activeLast = selected;
    } else {
        suffix = arena->activeFirst;
        arena->activeFirst = nullptr;
        arena->activeLast = nullptr;
    }
    for (ArenaBlock* block = suffix; block; block = block->next) {
        yumiArrayViewsInvalidate(block->payload, block->used);
    }
    dropTracedAfter(arena, markActiveBytes);
    setActiveBytes(arena, markActiveBytes);
    arena->activeBlockCount = markActiveBlockCount;
    measureRemoved(activeBytes - markActiveBytes);
    retainActiveSuffix(arena, suffix);

    ++arena->rewindCount;
    gArenaRewindCount.fetch_add(1, std::memory_order_relaxed);
    return 0;
}

extern "C" YUMI_GC_LEAF void
yumiArenaReset(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    if (!arena) {
        return;
    }
    const size_t removedBytes = activeBytesOf(arena);
    dropTracedAfter(arena, 0);
    ArenaBlock* blocks = arena->activeFirst;
    arena->activeFirst = nullptr;
    arena->activeLast = nullptr;
    arena->activeBlockCount = 0;
    setActiveBytes(arena, 0);
    measureRemoved(removedBytes);
    retainActiveSuffix(arena, blocks);
    ++arena->generation;
    ++arena->resetCount;
    gArenaResetCount.fetch_add(1, std::memory_order_relaxed);
}

extern "C" YUMI_GC_LEAF void
yumiArenaRelease(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    if (!arena) {
        return;
    }
    freeTraced(arena);
    measureRemoved(activeBytesOf(arena));
    gArenaRetainedBytes.fetch_sub(arena->retainedBytes, std::memory_order_relaxed);
    gArenaReservedBytes.fetch_sub(arena->reservedBytes, std::memory_order_relaxed);
    freeBlockList(arena->activeFirst);
    freeBlockList(arena->retainedFirst);
    arena->activeFirst = nullptr;
    arena->activeLast = nullptr;
    arena->retainedFirst = nullptr;
    arena->activeBlockCount = 0;
    arena->retainedBlockCount = 0;
    setActiveBytes(arena, 0);
    arena->reservedBytes = 0;
    arena->retainedBytes = 0;
    recycleArenaSlot(arena);
}

extern "C" YUMI_GC_LEAF size_t yumiArenaGeneration(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->generation : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaState(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return !arena ? 2 : arena->resetCount ? 1 : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaActiveBlockCount(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->activeBlockCount : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaRetainedBlockCount(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->retainedBlockCount : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaActiveBytes(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? activeBytesOf(arena) : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaReservedBytes(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->reservedBytes : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaRetainedBytes(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->retainedBytes : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaPeakActiveBytes(void* opaque, size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->peakActiveBytes : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaOwnerSystemBlockAllocationCount(
    void* opaque,
    size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->systemBlockAllocationCount : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaOwnerResetCount(
    void* opaque,
    size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->resetCount : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaOwnerRewindCount(
    void* opaque,
    size_t incarnation) {
    ArenaSlot* arena = slot(opaque, incarnation);
    return arena ? arena->rewindCount : 0;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaSystemBlockAllocationCount(void) {
    return gArenaSystemBlockAllocationCount.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaSystemBlockFreeCount(void) {
    return gArenaSystemBlockFreeCount.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaRuntimeActiveBytes(void) {
    std::lock_guard<std::mutex> lock(gArenaSlotMutex);
    size_t total = 0;
    for (ArenaSlotPage* page = gArenaSlotPages; page; page = page->next) {
        for (const ArenaSlot& candidate : page->slots) {
            total += activeBytesOf(&candidate);
        }
    }
    return total;
}
extern "C" YUMI_GC_LEAF size_t yumiArenaRuntimeReservedBytes(void) {
    return gArenaReservedBytes.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaRuntimeRetainedBytes(void) {
    return gArenaRetainedBytes.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaRuntimePeakActiveBytes(void) {
    return gArenaPeakActiveBytes.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaMarkCount(void) {
    return gArenaMarkCount.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaRewindCount(void) {
    return gArenaRewindCount.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaResetCount(void) {
    return gArenaResetCount.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaSlotCount(void) {
    return gArenaSlotCount.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaLiveSlotCount(void) {
    return gArenaLiveSlotCount.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaManagedAllocationCount(void) {
    return gArenaManagedAllocationCount.load(std::memory_order_relaxed);
}
extern "C" YUMI_GC_LEAF size_t yumiArenaZoneTransitionCount(void) {
    return 0;
}

extern "C" YUMI_GC_LEAF void*
yumiArenaFindOwner(const void* data) {
    const uintptr_t address = reinterpret_cast<uintptr_t>(data);
    std::lock_guard<std::mutex> lock(gArenaSlotMutex);
    for (ArenaSlotPage* page = gArenaSlotPages; page; page = page->next) {
        for (ArenaSlot& arena : page->slots) {
            if (!arena.live.load(std::memory_order_acquire)) {
                continue;
            }
            for (ArenaBlock* block = arena.activeFirst; block; block = block->next) {
                const uintptr_t start = reinterpret_cast<uintptr_t>(block->payload);
                if (address >= start && address - start < block->used) {
                    return &arena;
                }
            }
        }
    }
    return nullptr;
}
