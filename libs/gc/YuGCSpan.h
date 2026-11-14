#ifndef YU_GC_SPAN_H
#define YU_GC_SPAN_H

#include "YuGCBitmap.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace YuGC {

static constexpr size_t kObjectSidePayloadLaneCount = 8;
static constexpr size_t kSpanBitmapWordBits = sizeof(uint64_t) * 8;

static constexpr unsigned char kSpanRetirementActive = 0;
static constexpr unsigned char kSpanRetirementQueued = 1;
static constexpr unsigned char kSpanRetirementRetired = 2;
static constexpr size_t kInvalidSweepRegistryIndex = std::numeric_limits<size_t>::max();
static constexpr size_t kInvalidHeapSpanIndex = std::numeric_limits<size_t>::max();

// Allocation admission and marking both need word-granular atomic access: a marker can test
// one slot while a span owner changes another slot in the same word. Mark transitions use the
// returned value from exactly one relaxed fetch_or to elect the work-publication winner.
struct GCSpanAtomicBitmapView {
    size_t bitCount = 0;
    std::atomic<uint64_t>* words = nullptr;

    GCSpanAtomicBitmapView() = default;
    GCSpanAtomicBitmapView(size_t count, std::atomic<uint64_t>* words);

    bool get(size_t index) const {
        if (index >= bitCount) {
            return false;
        }
        uint64_t word = words[index / kSpanBitmapWordBits].load(std::memory_order_relaxed);
        return (word & (uint64_t {1} << (index % kSpanBitmapWordBits))) != 0;
    }

    bool getAcquire(size_t index) const {
        if (index >= bitCount) {
            return false;
        }
        uint64_t word = words[index / kSpanBitmapWordBits].load(std::memory_order_acquire);
        return (word & (uint64_t {1} << (index % kSpanBitmapWordBits))) != 0;
    }

    bool trySet(size_t index) {
        if (index >= bitCount) {
            return false;
        }
        uint64_t mask = uint64_t {1} << (index % kSpanBitmapWordBits);
        uint64_t previous = words[index / kSpanBitmapWordBits].fetch_or(
            mask,
            std::memory_order_relaxed);
        return (previous & mask) == 0;
    }

    void set(size_t index);
    void setRange(size_t start, size_t count, std::memory_order order);
    void clear(size_t index);
    void clearRange(size_t start, size_t count);
    void clearAll();
    bool findFrom(bool value, size_t start, size_t& result) const;
    size_t clearRunFrom(size_t start, size_t maxCount) const;
};

// Owns all hot per-slot span metadata in one zero-filled allocation. The views and side-table
// pointers published by GCSpan never move for the lifetime of the span.
struct GCSpanFlatMetadata {
    void* allocation = nullptr;
    size_t allocationSize = 0;
    std::atomic<uint64_t>* allocationWords = nullptr;
    std::atomic<uint64_t>* markWords = nullptr;
    std::atomic<uint64_t>* discardedReservationWords = nullptr;
    void** descriptors = nullptr;
    size_t* sizes = nullptr;

    explicit GCSpanFlatMetadata(size_t capacity);
    ~GCSpanFlatMetadata();
    bool valid() const { return allocation != nullptr; }
    GCSpanFlatMetadata(const GCSpanFlatMetadata&) = delete;
    GCSpanFlatMetadata& operator=(const GCSpanFlatMetadata&) = delete;
};

// Per-span side-payload storage, struct-of-arrays by lane: each lane allocates one pointer slot
// per object capacity only once something publishes to that lane on this span, so a span pays
// 8 bytes per slot per *active* lane instead of a fixed per-object entry covering every lane.
// A payload slot is active exactly when it holds a non-null pointer.
struct ObjectSideMetadataTable {
    size_t capacity;
    std::atomic<std::atomic<char*>*> lanePayloads[kObjectSidePayloadLaneCount];
    std::atomic<std::atomic<uint64_t>*> laneCleanupOccupancy[kObjectSidePayloadLaneCount];

    explicit ObjectSideMetadataTable(size_t capacity);
    ~ObjectSideMetadataTable();
    ObjectSideMetadataTable(const ObjectSideMetadataTable&) = delete;
    ObjectSideMetadataTable& operator=(const ObjectSideMetadataTable&) = delete;

    std::atomic<char*>* lane(size_t lane) const;
    std::atomic<char*>* ensureLane(size_t lane);
    std::atomic<uint64_t>* cleanupOccupancy(size_t lane) const;
    std::atomic<uint64_t>* ensureCleanupOccupancy(size_t lane);
    bool cleanupOccupied(size_t lane, size_t index) const;
    void setCleanupOccupied(size_t lane, size_t index);
    bool clearCleanupOccupied(size_t lane, size_t index);
};

#if YU_ENABLE_BENCH_METRICS
// Benchmark builds sample one GC-reserved slot in every fixed-size sequence. Keeping only
// sampled epochs makes lifetime instrumentation sparse while retaining exact ages for every
// sample that becomes an allocation.
struct GCBenchLifetimeStamp {
    uint32_t objectIndex;
    uint32_t allocationEpoch;
};
#endif

struct GCSpan {
    char* memory = nullptr;
    GCArena* arena = nullptr;
    size_t pageCount = 0;
    SizeClass sizeClass {};
    size_t objectsCapacity = 0;
    void* listPrev = nullptr;
    void* listNext = nullptr;
    void* listOwner = nullptr;
    std::atomic<size_t> storeLane {0};
    // Intrusive retirement linkage is written before the owning heap release-publishes this
    // span on its atomic retired head. Metadata destruction is deferred until a quiescent drain.
    GCSpan* retirementNext = nullptr;
    std::atomic<unsigned char> retirementState {kSpanRetirementActive};
    // Stable ordinal in the owning sweep registry: its size-class store's, or the heap's for a
    // large span. Quiescent retirement swap-fills this slot, so rehome and retirement
    // validate/unlink in O(1) without tombstones or a search.
    size_t sweepRegistryIndex = kInvalidSweepRegistryIndex;
    size_t heapSpanIndex = kInvalidHeapSpanIndex;
    size_t objectsUsed = 0;
    std::atomic<uint32_t> sweepGen {0};
    // The generation says whether work is due; this independent CAS elects exactly one owner
    // for the bitmap flip in that generation.
    std::atomic<unsigned char> sweepState {kSpanSweepAvailable};
    // Explicit cleanup registrations are keyed by target. The count is the per-span
    // common-case filter: zero means termination and sweep need no target lookup here.
    std::atomic<size_t> cleanupRegistrationCount {0};
    // Cleanup-enabled side payloads use sparse per-lane occupancy bitmaps. The aggregate
    // count keeps ordinary spans out of termination discovery without a global registry.
    std::atomic<size_t> cleanupSidePayloadCount {0};
    // The central range handoff owns zeroing. Stage 1 keeps this sticky after first use so a
    // recycled slot is never exposed without one pre-publication clear.
    std::atomic<bool> needsZero {true};
    unsigned char rawReuseState = kSpanReuseNew;
    size_t nextFreeIndex = 0;
    GCSpanFlatMetadata flatMetadata;
    bool markCapacityRegistered = false;
    GCSpanAtomicBitmapView allocBits;
    GCSpanAtomicBitmapView markBits;
    // A reservation is allocate-black while marking so conservative candidates cannot
    // consume mark-work capacity. If its unused tail is discarded, this owner-only bitmap
    // tells sweep to reclaim it even though its mark bit intentionally remains set.
    GCSpanAtomicBitmapView discardedReservations;
    void** descriptors = nullptr;
    size_t* sizes = nullptr;
    std::atomic<ObjectSideMetadataTable*> objectSideMetadata {nullptr};
#if YU_ENABLE_BENCH_METRICS
    std::vector<GCBenchLifetimeStamp> benchLifetimeStamps;
#endif

    GCSpan(GCArena* arena, char* memory, size_t pageCount, SizeClass sizeClass);
    ~GCSpan();

    bool valid() const { return flatMetadata.valid() && markCapacityRegistered; }

    bool isFull() const {
        return objectsUsed == objectsCapacity;
    }

    bool isEmpty() const {
        return objectsUsed == 0;
    }

    bool needsSweepNow() const;

    unsigned char reuseState() const;

    bool canAllocateNow() const {
        // sweepGen is published before sweep finishes rewriting owner-only metadata.
        // The final Available release is the handoff that makes objectsUsed, cursors,
        // descriptors, and side metadata safe for allocation to consume again.
        return sweepState.load(std::memory_order_acquire) == kSpanSweepAvailable &&
            sweepGeneration() == yumiGCPublicSweepGeneration();
    }

    void setListState(void* prev, void* next, void* owner);

    void setListPrev(void* pointer);

    void setListNext(void* pointer);

    void markReleased();

    bool queueForRetirement();

    bool markRetired();

    bool isRetired() const;

    bool isRetirementActive() const {
        return retirementState.load(std::memory_order_acquire) == kSpanRetirementActive;
    }

    bool containsPointer(char* pointer) const {
        uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
        uintptr_t base = reinterpret_cast<uintptr_t>(memory);
        return address >= base && address - base < pageCount * kPageSize;
    }

    bool objectIndex(char* pointer, size_t& index) const {
        if (!containsPointer(pointer)) {
            return false;
        }
        size_t relative = static_cast<size_t>(
            reinterpret_cast<uintptr_t>(pointer) - reinterpret_cast<uintptr_t>(memory));
        index = sizeClass.divideSlotOffset(relative);
        if (index * sizeClass.size != relative) {
            return false;
        }
        return index < objectsCapacity;
    }

    bool containingObjectIndex(char* pointer, size_t& index) const {
        if (!containsPointer(pointer)) {
            return false;
        }
        size_t relative = static_cast<size_t>(
            reinterpret_cast<uintptr_t>(pointer) - reinterpret_cast<uintptr_t>(memory));
        index = sizeClass.divideSlotOffset(relative);
        return index < objectsCapacity;
    }

    char* objectBase(char* pointer) const {
        size_t index = 0;
        if (!containingObjectIndex(pointer, index) || !allocBits.getAcquire(index)) {
            return nullptr;
        }
        return memory + sizeClass.size * index;
    }

    bool ownsObject(char* pointer) const {
        size_t index = 0;
        return objectIndex(pointer, index) && allocBits.getAcquire(index);
    }

    void* descriptor(char* pointer) const {
        size_t index = 0;
        if (!objectIndex(pointer, index)) {
            return nullptr;
        }
        return descriptors[index];
    }

    size_t objectSize(char* pointer) const {
        size_t index = 0;
        if (!objectIndex(pointer, index)) {
            return 0;
        }
        return sizes[index];
    }

    void updateObjectMetadata(char* pointer, size_t objectSize, void* descriptor);

    // Accepts base or interior pointers. The hot path reads only immutable geometry and the
    // atomic allocation word, then performs one relaxed mark-word fetch_or.
    bool tryMark(char* pointer) {
        size_t index = 0;
        if (!containingObjectIndex(pointer, index) || !allocBits.getAcquire(index)) {
            return false;
        }
        return markBits.trySet(index);
    }

    bool tryMarkContaining(char* pointer, char*& object) {
        size_t index = 0;
        if (!containingObjectIndex(pointer, index) || !allocBits.getAcquire(index)) {
            return false;
        }
        object = memory + sizeClass.size * index;
        return markBits.trySet(index);
    }

    bool isMarked(char* pointer) const;
    bool isMarkedSlot(size_t index) const;
    void addCleanupRegistration();
    void removeCleanupRegistration();
    bool hasCleanupRegistrations() const;
    void addCleanupSidePayload();
    void removeCleanupSidePayload();
    bool hasCleanupSidePayloads() const;
    uint32_t sweepGeneration() const {
        return sweepGen.load(std::memory_order_acquire);
    }
    char* allocNext(size_t objectSize, void* descriptor);
    size_t allocBatch(
        size_t objectSize,
        void* descriptor,
        size_t preferredCount,
        char*& pointer,
        bool allocateBlack = false);
    bool markAllocatedRange(char* pointer, size_t count, size_t slotSize);
    size_t releaseBatch(char* cursor, char* limit, size_t slotSize);
    size_t discardBatch(char* cursor, char* limit, size_t slotSize);
    bool deferDiscardedBatch(char* cursor, char* limit, size_t slotSize);
    void clearObjectSideMetadata(size_t index);
    void clearMarkBits();
#if !defined(YU_RELEASE)
    void poisonObject(size_t index);
#endif
    bool tryClaimSweep();
    void sweepClaimed();
    bool sweep();
};

// GCSpan records come from an mmap-backed metadata slab instead of the general C++ heap.
// Destruction returns a record to the slab only after the heap's quiescent retirement drain.
GCSpan* constructGCSpan(
    GCArena* arena,
    char* memory,
    size_t pageCount,
    SizeClass sizeClass);
void destroyGCSpan(GCSpan* span);

struct GCSpanList {
    void* head = nullptr;
    void* tail = nullptr;
    size_t length = 0;

    bool isEmpty() const;

    bool contains(GCSpan* span) const;

    void push(GCSpan* span);

    GCSpan* pop();

    void remove(GCSpan* span);

    size_t countOf(GCSpan* span) const;

    bool verifyLinks() const;
};

} // namespace YuGC

#endif
