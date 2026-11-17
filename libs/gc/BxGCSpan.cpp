#include "BxGCHeap.h"
#include "BxGCLockOrder.h"
#include "BxGCWork.h"

namespace BxGC {

static_assert(
    ATOMIC_LONG_LOCK_FREE == 2 && ATOMIC_LLONG_LOCK_FREE == 2,
    "GC slot bitmaps require lock-free 64-bit atomics");

static size_t
spanBitmapWordCount(size_t bitCount) {
    return (bitCount + kSpanBitmapWordBits - 1) / kSpanBitmapWordBits;
}

static size_t
checkedMetadataAdd(size_t left, size_t right) {
    if (right > std::numeric_limits<size_t>::max() - left) {
        abortGC("GC span metadata size overflow");
    }
    return left + right;
}

static size_t
checkedMetadataMultiply(size_t left, size_t right) {
    if (left && right > std::numeric_limits<size_t>::max() / left) {
        abortGC("GC span metadata size overflow");
    }
    return left * right;
}

static size_t
alignMetadataOffset(size_t offset, size_t alignment) {
    size_t remainder = offset % alignment;
    return remainder ? checkedMetadataAdd(offset, alignment - remainder) : offset;
}

GCSpanAtomicBitmapView::GCSpanAtomicBitmapView(
    size_t count,
    std::atomic<uint64_t>* words
) : bitCount(count), words(words) {}

void
GCSpanAtomicBitmapView::set(size_t index) {
    if (index < bitCount) {
        words[index / kSpanBitmapWordBits].fetch_or(
            uint64_t {1} << (index % kSpanBitmapWordBits),
            std::memory_order_release);
    }
}

static uint64_t
spanBitmapRangeMask(size_t word, size_t start, size_t end) {
    size_t wordStart = word * kSpanBitmapWordBits;
    size_t firstBit = start > wordStart ? start - wordStart : 0;
    size_t lastBit = std::min(end - wordStart, kSpanBitmapWordBits);
    uint64_t highMask = lastBit == kSpanBitmapWordBits
        ? ~uint64_t {0}
        : (uint64_t {1} << lastBit) - 1;
    return (~uint64_t {0} << firstBit) & highMask;
}

void
GCSpanAtomicBitmapView::setRange(size_t start, size_t count, std::memory_order order) {
    if (!count || start >= bitCount || count > bitCount - start) {
        return;
    }
    size_t end = start + count;
    size_t lastWord = (end - 1) / kSpanBitmapWordBits;
    for (size_t word = start / kSpanBitmapWordBits; word <= lastWord; ++word) {
        words[word].fetch_or(spanBitmapRangeMask(word, start, end), order);
    }
}

void
GCSpanAtomicBitmapView::clear(size_t index) {
    if (index < bitCount) {
        words[index / kSpanBitmapWordBits].fetch_and(
            ~(uint64_t {1} << (index % kSpanBitmapWordBits)),
            std::memory_order_relaxed);
    }
}

void
GCSpanAtomicBitmapView::clearRange(size_t start, size_t count) {
    if (!count || start >= bitCount || count > bitCount - start) {
        return;
    }
    size_t end = start + count;
    size_t lastWord = (end - 1) / kSpanBitmapWordBits;
    for (size_t word = start / kSpanBitmapWordBits; word <= lastWord; ++word) {
        words[word].fetch_and(~spanBitmapRangeMask(word, start, end), std::memory_order_relaxed);
    }
}

void
GCSpanAtomicBitmapView::clearAll() {
    size_t wordCount = spanBitmapWordCount(bitCount);
    for (size_t word = 0; word < wordCount; ++word) {
        words[word].store(0, std::memory_order_relaxed);
    }
}

bool
GCSpanAtomicBitmapView::findFrom(bool value, size_t start, size_t& result) const {
    if (!bitCount) {
        return false;
    }
    if (start >= bitCount) {
        start = 0;
    }
    size_t wordCount = spanBitmapWordCount(bitCount);
    size_t startWord = start / kSpanBitmapWordBits;
    size_t startBit = start % kSpanBitmapWordBits;
    for (size_t scanned = 0; scanned < wordCount; ++scanned) {
        size_t wordIndex = startWord + scanned;
        if (wordIndex >= wordCount) {
            wordIndex -= wordCount;
        }
        uint64_t word = words[wordIndex].load(std::memory_order_acquire);
        uint64_t candidates = value ? word : ~word;
        if (scanned == 0 && startBit) {
            candidates &= ~((uint64_t {1} << startBit) - 1);
        }
        if (wordIndex + 1 == wordCount && bitCount % kSpanBitmapWordBits) {
            candidates &= (uint64_t {1} << (bitCount % kSpanBitmapWordBits)) - 1;
        }
        if (!candidates) {
            continue;
        }
        size_t bit = static_cast<size_t>(__builtin_ctzll(candidates));
        result = wordIndex * kSpanBitmapWordBits + bit;
        return result < bitCount;
    }
    // The first word may also have candidates before startBit after the wrap.
    if (startBit) {
        uint64_t word = words[startWord].load(std::memory_order_acquire);
        uint64_t candidates = value ? word : ~word;
        candidates &= (uint64_t {1} << startBit) - 1;
        if (startWord + 1 == wordCount && bitCount % kSpanBitmapWordBits) {
            candidates &= (uint64_t {1} << (bitCount % kSpanBitmapWordBits)) - 1;
        }
        if (candidates) {
            result = startWord * kSpanBitmapWordBits +
                static_cast<size_t>(__builtin_ctzll(candidates));
            return result < bitCount;
        }
    }
    return false;
}

size_t
GCSpanAtomicBitmapView::clearRunFrom(size_t start, size_t maxCount) const {
    if (start >= bitCount) {
        return 0;
    }
    size_t end = start + std::min(maxCount, bitCount - start);
    size_t index = start;
    while (index < end) {
        size_t bit = index % kSpanBitmapWordBits;
        uint64_t word = words[index / kSpanBitmapWordBits].load(std::memory_order_acquire) >> bit;
        if (word) {
            index += static_cast<size_t>(__builtin_ctzll(word));
            break;
        }
        index += kSpanBitmapWordBits - bit;
    }
    return std::min(index, end) - start;
}

static std::atomic<size_t> gActiveFlatSpanMetadataBytes {0};
static std::atomic<size_t> gCachedFlatSpanMetadataBytes {0};
static std::atomic<size_t> gRetainedSpanMetadataSlabBytes {0};

static void* acquireFlatSpanMetadata(size_t allocationSize, bool& zeroed);
static void releaseFlatSpanMetadata(void* allocation, size_t allocationSize);

GCSpanFlatMetadata::GCSpanFlatMetadata(size_t capacity) {
    size_t bitmapBytes = checkedMetadataMultiply(spanBitmapWordCount(capacity), sizeof(uint64_t));
    size_t allocationOffset = 0;
    size_t markOffset = checkedMetadataAdd(allocationOffset, bitmapBytes);
    size_t discardedReservationOffset = checkedMetadataAdd(markOffset, bitmapBytes);
    size_t descriptorOffset = alignMetadataOffset(
        checkedMetadataAdd(discardedReservationOffset, bitmapBytes),
        alignof(void*));
    size_t sizeOffset = alignMetadataOffset(
        checkedMetadataAdd(
            descriptorOffset,
            checkedMetadataMultiply(capacity, sizeof(void*))),
        alignof(size_t));
    allocationSize = checkedMetadataAdd(
        sizeOffset,
        checkedMetadataMultiply(capacity, sizeof(size_t)));
    bool zeroed = false;
    allocation = acquireFlatSpanMetadata(allocationSize, zeroed);
    if (!allocation) {
        std::fprintf(stderr, "Brix GC span metadata allocation failed; allocation will return null\n");
        return;
    }
    if (!zeroed) {
        std::memset(allocation, 0, allocationSize);
    }
    gActiveFlatSpanMetadataBytes.fetch_add(allocationSize, std::memory_order_relaxed);
    char* base = static_cast<char*>(allocation);
    allocationWords = reinterpret_cast<std::atomic<uint64_t>*>(base + allocationOffset);
    markWords = reinterpret_cast<std::atomic<uint64_t>*>(base + markOffset);
    discardedReservationWords = reinterpret_cast<std::atomic<uint64_t>*>(
        base + discardedReservationOffset);
    descriptors = reinterpret_cast<void**>(base + descriptorOffset);
    sizes = reinterpret_cast<size_t*>(base + sizeOffset);
    size_t wordCount = spanBitmapWordCount(capacity);
    for (size_t word = 0; word < wordCount; ++word) {
        new (&allocationWords[word]) std::atomic<uint64_t>(0);
        new (&markWords[word]) std::atomic<uint64_t>(0);
        new (&discardedReservationWords[word]) std::atomic<uint64_t>(0);
    }
}

GCSpanFlatMetadata::~GCSpanFlatMetadata() {
    if (allocation) {
        size_t wordCount = 0;
        if (allocationWords && markWords) {
            wordCount = static_cast<size_t>(
                reinterpret_cast<char*>(markWords) -
                reinterpret_cast<char*>(allocationWords)) / sizeof(uint64_t);
        }
        for (size_t word = 0; word < wordCount; ++word) {
            allocationWords[word].~atomic();
            markWords[word].~atomic();
            discardedReservationWords[word].~atomic();
        }
        gActiveFlatSpanMetadataBytes.fetch_sub(allocationSize, std::memory_order_relaxed);
        releaseFlatSpanMetadata(allocation, allocationSize);
    }
}

struct GCSpanFlatMetadataChunk;

struct GCSpanFlatMetadataBlock {
    GCSpanFlatMetadataChunk* chunk;
    GCSpanFlatMetadataBlock* next;
};

struct GCSpanFlatMetadataChunk {
    GCSpanFlatMetadataChunk* next;
    GCSpanFlatMetadataChunk* previous;
    GCSpanFlatMetadataBlock* freeBlocks;
    size_t stride;
    size_t carvedBytes;
    size_t dirtyBytes;
    size_t liveCount;
    bool listed;
};

static constexpr size_t kFlatMetadataBlockAlignment = 16;
static constexpr size_t kFlatMetadataChunkBytes = 1024 * 1024;
static constexpr size_t kFlatMetadataChunkMinimumBlocks = 8;
static constexpr size_t kFlatMetadataSpareByteLimit = 4 * 1024 * 1024;

static_assert(
    sizeof(GCSpanFlatMetadataBlock) % kFlatMetadataBlockAlignment == 0,
    "flat metadata block header must preserve payload alignment");

struct GCSpanMetadataFreeSlot {
    GCSpanMetadataFreeSlot* next;
};

static_assert(
    sizeof(GCSpan) >= sizeof(GCSpanMetadataFreeSlot),
    "GCSpan slab record must hold an intrusive free-list link");
static_assert(
    alignof(GCSpan) >= alignof(GCSpanMetadataFreeSlot),
    "GCSpan slab record must align an intrusive free-list link");

struct GCSpanMetadataSlab {
    GCSpanMetadataSlab* next;
    size_t byteSize;
};

struct GCSpanMetadataAllocatorState {
    std::mutex mutex;
    GCSpanMetadataSlab* slabs = nullptr;
    GCSpanMetadataFreeSlot* freeSlots = nullptr;
    std::unordered_map<size_t, GCSpanFlatMetadataChunk*> flatMetadataChunks;
    GCSpanFlatMetadataChunk* spareFlatMetadataChunks = nullptr;
    size_t spareFlatMetadataBytes = 0;
};

static GCSpanMetadataAllocatorState&
spanMetadataAllocatorState() {
    // Slabs intentionally live for the runtime's lifetime. Individual records are recycled
    // only after deferred retirement establishes that no address-index reader can retain them.
    static GCSpanMetadataAllocatorState state;
    return state;
}

static size_t
flatMetadataChunkHeaderBytes() {
    return alignMetadataOffset(sizeof(GCSpanFlatMetadataChunk), kFlatMetadataBlockAlignment);
}

static size_t
flatMetadataStride(size_t allocationSize) {
    return alignMetadataOffset(
        checkedMetadataAdd(sizeof(GCSpanFlatMetadataBlock), allocationSize),
        kFlatMetadataBlockAlignment);
}

static bool
flatMetadataFitsChunk(size_t stride) {
    return stride <=
        (kFlatMetadataChunkBytes - flatMetadataChunkHeaderBytes()) / kFlatMetadataChunkMinimumBlocks;
}

static void
linkFlatMetadataChunk(GCSpanFlatMetadataChunk*& head, GCSpanFlatMetadataChunk* chunk) {
    chunk->previous = nullptr;
    chunk->next = head;
    if (head) {
        head->previous = chunk;
    }
    head = chunk;
    chunk->listed = true;
}

static void
unlinkFlatMetadataChunk(GCSpanFlatMetadataChunk*& head, GCSpanFlatMetadataChunk* chunk) {
    if (chunk->previous) {
        chunk->previous->next = chunk->next;
    } else {
        head = chunk->next;
    }
    if (chunk->next) {
        chunk->next->previous = chunk->previous;
    }
    chunk->previous = nullptr;
    chunk->next = nullptr;
    chunk->listed = false;
}

static GCSpanFlatMetadataChunk*
takeFlatMetadataChunkLocked(GCSpanMetadataAllocatorState& state) {
    GCSpanFlatMetadataChunk* chunk = state.spareFlatMetadataChunks;
    if (chunk) {
        state.spareFlatMetadataChunks = chunk->next;
        state.spareFlatMetadataBytes -= kFlatMetadataChunkBytes;
        chunk->next = nullptr;
        return chunk;
    }
    void* memory = mmap(
        nullptr,
        kFlatMetadataChunkBytes,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);
    if (memory == MAP_FAILED) {
        return nullptr;
    }
    chunk = static_cast<GCSpanFlatMetadataChunk*>(memory);
    chunk->carvedBytes = flatMetadataChunkHeaderBytes();
    chunk->dirtyBytes = chunk->carvedBytes;
    gCachedFlatSpanMetadataBytes.fetch_add(chunk->dirtyBytes, std::memory_order_relaxed);
    return chunk;
}

static void*
acquireFlatSpanMetadata(size_t allocationSize, bool& zeroed) {
    size_t stride = flatMetadataStride(allocationSize);
    if (!flatMetadataFitsChunk(stride)) {
        void* memory = mmap(
            nullptr,
            stride,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANON,
            -1,
            0);
        if (memory == MAP_FAILED) {
            return nullptr;
        }
        auto* block = static_cast<GCSpanFlatMetadataBlock*>(memory);
        block->chunk = nullptr;
        zeroed = true;
        return block + 1;
    }

    GCSpanMetadataAllocatorState& state = spanMetadataAllocatorState();
    std::lock_guard<std::mutex> lock(state.mutex);
    GCSpanFlatMetadataChunk** head = nullptr;
    try {
        head = &state.flatMetadataChunks[stride];
    } catch (...) {
        return nullptr;
    }
    GCSpanFlatMetadataChunk* chunk = *head;
    if (!chunk) {
        chunk = takeFlatMetadataChunkLocked(state);
        if (!chunk) {
            return nullptr;
        }
        chunk->stride = stride;
        linkFlatMetadataChunk(*head, chunk);
    }
    GCSpanFlatMetadataBlock* block = chunk->freeBlocks;
    if (block) {
        chunk->freeBlocks = block->next;
        zeroed = false;
    } else {
        block = reinterpret_cast<GCSpanFlatMetadataBlock*>(
            reinterpret_cast<char*>(chunk) + chunk->carvedBytes);
        zeroed = chunk->carvedBytes >= chunk->dirtyBytes;
        chunk->carvedBytes += stride;
        if (chunk->carvedBytes > chunk->dirtyBytes) {
            gCachedFlatSpanMetadataBytes.fetch_add(
                chunk->carvedBytes - chunk->dirtyBytes,
                std::memory_order_relaxed);
            chunk->dirtyBytes = chunk->carvedBytes;
        }
    }
    block->chunk = chunk;
    block->next = nullptr;
    ++chunk->liveCount;
    if (!chunk->freeBlocks && chunk->carvedBytes + stride > kFlatMetadataChunkBytes) {
        unlinkFlatMetadataChunk(*head, chunk);
    }
    gCachedFlatSpanMetadataBytes.fetch_sub(allocationSize, std::memory_order_relaxed);
    return block + 1;
}

static void
releaseFlatSpanMetadata(void* allocation, size_t allocationSize) {
    GCSpanFlatMetadataBlock* block = static_cast<GCSpanFlatMetadataBlock*>(allocation) - 1;
    GCSpanFlatMetadataChunk* chunk = block->chunk;
    if (!chunk) {
        munmap(block, flatMetadataStride(allocationSize));
        return;
    }

    GCSpanFlatMetadataChunk* surplus = nullptr;
    {
        GCSpanMetadataAllocatorState& state = spanMetadataAllocatorState();
        std::lock_guard<std::mutex> lock(state.mutex);
        gCachedFlatSpanMetadataBytes.fetch_add(allocationSize, std::memory_order_relaxed);
        if (chunk->liveCount == 0) {
            abortGC("GC span metadata block released twice");
        }
        --chunk->liveCount;
        auto found = state.flatMetadataChunks.find(chunk->stride);
        if (found == state.flatMetadataChunks.end()) {
            abortGC("GC span metadata chunk lost its size bucket");
        }
        if (chunk->liveCount != 0) {
            block->next = chunk->freeBlocks;
            chunk->freeBlocks = block;
            if (!chunk->listed) {
                linkFlatMetadataChunk(found->second, chunk);
            }
            return;
        }
        if (chunk->listed) {
            unlinkFlatMetadataChunk(found->second, chunk);
        }
        chunk->freeBlocks = nullptr;
        chunk->stride = 0;
        chunk->carvedBytes = flatMetadataChunkHeaderBytes();
        if (state.spareFlatMetadataBytes + kFlatMetadataChunkBytes <= kFlatMetadataSpareByteLimit) {
            chunk->next = state.spareFlatMetadataChunks;
            state.spareFlatMetadataChunks = chunk;
            state.spareFlatMetadataBytes += kFlatMetadataChunkBytes;
            return;
        }
        gCachedFlatSpanMetadataBytes.fetch_sub(chunk->dirtyBytes, std::memory_order_relaxed);
        surplus = chunk;
    }
    munmap(surplus, kFlatMetadataChunkBytes);
}

size_t
releaseSpareSpanMetadata() {
    GCSpanFlatMetadataChunk* spares = nullptr;
    {
        GCSpanMetadataAllocatorState& state = spanMetadataAllocatorState();
        std::lock_guard<std::mutex> lock(state.mutex);
        spares = state.spareFlatMetadataChunks;
        state.spareFlatMetadataChunks = nullptr;
        state.spareFlatMetadataBytes = 0;
        for (GCSpanFlatMetadataChunk* chunk = spares; chunk; chunk = chunk->next) {
            gCachedFlatSpanMetadataBytes.fetch_sub(chunk->dirtyBytes, std::memory_order_relaxed);
        }
    }
    size_t released = 0;
    while (spares) {
        GCSpanFlatMetadataChunk* next = spares->next;
        munmap(spares, kFlatMetadataChunkBytes);
        released += kFlatMetadataChunkBytes;
        spares = next;
    }
    return released;
}

static size_t
spanMetadataSlotStride() {
    return alignMetadataOffset(sizeof(GCSpan), alignof(GCSpan));
}

static bool
growSpanMetadataAllocator(GCSpanMetadataAllocatorState& state) {
    static constexpr size_t kSpanMetadataSlotsPerSlab = 64;
    size_t slotsOffset = alignMetadataOffset(sizeof(GCSpanMetadataSlab), alignof(GCSpan));
    size_t byteSize = checkedMetadataAdd(
        slotsOffset,
        checkedMetadataMultiply(kSpanMetadataSlotsPerSlab, spanMetadataSlotStride()));
    void* memory = mmap(
        nullptr,
        byteSize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0);
    if (memory == MAP_FAILED) {
        return false;
    }

    auto* slab = static_cast<GCSpanMetadataSlab*>(memory);
    slab->next = state.slabs;
    slab->byteSize = byteSize;
    state.slabs = slab;
    gRetainedSpanMetadataSlabBytes.fetch_add(byteSize, std::memory_order_relaxed);
    char* slots = static_cast<char*>(memory) + slotsOffset;
    for (size_t index = 0; index < kSpanMetadataSlotsPerSlab; ++index) {
        auto* slot = reinterpret_cast<GCSpanMetadataFreeSlot*>(
            slots + index * spanMetadataSlotStride());
        slot->next = state.freeSlots;
        state.freeSlots = slot;
    }
    return true;
}

static void
recycleSpanMetadataSlot(void* storage) {
    GCSpanMetadataAllocatorState& state = spanMetadataAllocatorState();
    std::lock_guard<std::mutex> lock(state.mutex);
    auto* slot = static_cast<GCSpanMetadataFreeSlot*>(storage);
    slot->next = state.freeSlots;
    state.freeSlots = slot;
}

GCSpan*
constructGCSpan(
    GCArena* arena,
    char* memory,
    size_t pageCount,
    SizeClass sizeClass
) {
    GCSpanMetadataAllocatorState& state = spanMetadataAllocatorState();
    void* storage = nullptr;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (!state.freeSlots && !growSpanMetadataAllocator(state)) {
            return nullptr;
        }
        GCSpanMetadataFreeSlot* slot = state.freeSlots;
        state.freeSlots = slot->next;
        storage = slot;
    }
    try {
        GCSpan* span = new (storage) GCSpan(arena, memory, pageCount, sizeClass);
        if (!span->valid()) {
            span->~GCSpan();
            recycleSpanMetadataSlot(storage);
            return nullptr;
        }
        return span;
    } catch (...) {
        recycleSpanMetadataSlot(storage);
        throw;
    }
}

void
destroyGCSpan(GCSpan* span) {
    if (!span) {
        return;
    }
    span->~GCSpan();
    recycleSpanMetadataSlot(span);
}

extern "C" size_t
brixGCRuntimeSpanMetadataBytes(void) {
    return gActiveFlatSpanMetadataBytes.load(std::memory_order_acquire) +
        gCachedFlatSpanMetadataBytes.load(std::memory_order_acquire) +
        gRetainedSpanMetadataSlabBytes.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeActiveSpanMetadataBytes(void) {
    return gActiveFlatSpanMetadataBytes.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCRuntimeCachedSpanMetadataBytes(void) {
    return gCachedFlatSpanMetadataBytes.load(std::memory_order_acquire);
}

#if BX_ENABLE_BENCH_METRICS

extern "C" unsigned char brixBenchMetricsEnabledFlag;

static constexpr size_t kBenchLifetimeSampleEvery = 64;
static constexpr size_t kBenchLifetimeBinCount = 9;
static constexpr size_t kBenchLifetimeBinUpperBounds[kBenchLifetimeBinCount] = {
    0,
    1,
    2,
    3,
    4,
    7,
    15,
    31,
    std::numeric_limits<size_t>::max()
};

static std::atomic<uint64_t> gBenchLifetimeAllocationSequence {0};
static std::array<std::atomic<size_t>, kBenchLifetimeBinCount> gBenchLifetimeBins {};
static std::atomic<size_t> gBenchLifetimeSampledReservations {0};
static std::atomic<size_t> gBenchLifetimeClearedReservations {0};
static std::atomic<size_t> gBenchLifetimeReclaimed {0};
static std::atomic<size_t> gBenchLifetimeOutstanding {0};
static std::mutex gBenchLifetimeOutstandingMutex;
static std::unordered_map<uint32_t, size_t> gBenchLifetimeOutstandingEpochs;

static bool
benchLifetimeEnabled() {
    return __builtin_expect(brixBenchMetricsEnabledFlag != 0, 0);
}

static std::vector<GCBenchLifetimeStamp>::iterator
benchLifetimeStampAt(GCSpan& span, size_t objectIndex) {
    uint32_t index = static_cast<uint32_t>(objectIndex);
    return std::lower_bound(
        span.benchLifetimeStamps.begin(),
        span.benchLifetimeStamps.end(),
        index,
        [](const GCBenchLifetimeStamp& stamp, uint32_t value) {
            return stamp.objectIndex < value;
        });
}

static size_t
benchLifetimeBinFor(size_t survived) {
    size_t bin = 0;
    while (bin + 1 < kBenchLifetimeBinCount &&
            survived > kBenchLifetimeBinUpperBounds[bin]) {
        ++bin;
    }
    return bin;
}

static void
addBenchLifetimeOutstanding(uint32_t epoch) {
    std::lock_guard<std::mutex> lock(gBenchLifetimeOutstandingMutex);
    ++gBenchLifetimeOutstandingEpochs[epoch];
    gBenchLifetimeOutstanding.fetch_add(1, std::memory_order_relaxed);
}

static void
removeBenchLifetimeOutstanding(uint32_t epoch) {
    std::lock_guard<std::mutex> lock(gBenchLifetimeOutstandingMutex);
    auto found = gBenchLifetimeOutstandingEpochs.find(epoch);
    if (found != gBenchLifetimeOutstandingEpochs.end()) {
        if (--found->second == 0) {
            gBenchLifetimeOutstandingEpochs.erase(found);
        }
        gBenchLifetimeOutstanding.fetch_sub(1, std::memory_order_relaxed);
    }
}

static void
clearBenchLifetimeStamp(GCSpan& span, size_t objectIndex) {
    if (span.benchLifetimeStamps.empty()) {
        return;
    }
    auto stamp = benchLifetimeStampAt(span, objectIndex);
    if (stamp != span.benchLifetimeStamps.end() && stamp->objectIndex == objectIndex) {
        removeBenchLifetimeOutstanding(stamp->allocationEpoch);
        gBenchLifetimeClearedReservations.fetch_add(1, std::memory_order_relaxed);
        span.benchLifetimeStamps.erase(stamp);
    }
}

static void
stampBenchLifetimeAllocation(GCSpan& span, size_t objectIndex, uint64_t sequence) {
    if ((sequence + 1) % kBenchLifetimeSampleEvery != 0) {
        return;
    }
    clearBenchLifetimeStamp(span, objectIndex);

    GCBenchLifetimeStamp stamp {
        static_cast<uint32_t>(objectIndex),
        static_cast<uint32_t>(brixGCRuntimeCollectionCount())
    };
    span.benchLifetimeStamps.insert(benchLifetimeStampAt(span, objectIndex), stamp);
    gBenchLifetimeSampledReservations.fetch_add(1, std::memory_order_relaxed);
    addBenchLifetimeOutstanding(stamp.allocationEpoch);
}

static void
stampBenchLifetimeAllocationRange(GCSpan& span, size_t start, size_t count) {
    if (!benchLifetimeEnabled()) {
        return;
    }
    uint64_t first = gBenchLifetimeAllocationSequence.fetch_add(
        count,
        std::memory_order_relaxed);
    size_t sequenceRemainder = static_cast<size_t>((first + 1) % kBenchLifetimeSampleEvery);
    size_t offset = sequenceRemainder == 0 ? 0 : kBenchLifetimeSampleEvery - sequenceRemainder;
    for (; offset < count; offset += kBenchLifetimeSampleEvery) {
        stampBenchLifetimeAllocation(span, start + offset, first + offset);
    }
}

static void
recordBenchLifetimeReclamation(GCSpan& span, size_t objectIndex) {
    if (!benchLifetimeEnabled() || span.benchLifetimeStamps.empty()) {
        return;
    }
    auto stamp = benchLifetimeStampAt(span, objectIndex);
    if (stamp == span.benchLifetimeStamps.end() || stamp->objectIndex != objectIndex) {
        return;
    }

    uint32_t completedCollections = static_cast<uint32_t>(brixGCRuntimeCollectionCount());
    size_t survived = static_cast<uint32_t>(completedCollections - stamp->allocationEpoch);
    size_t bin = benchLifetimeBinFor(survived);
    gBenchLifetimeBins[bin].fetch_add(1, std::memory_order_relaxed);
    gBenchLifetimeReclaimed.fetch_add(1, std::memory_order_relaxed);
    removeBenchLifetimeOutstanding(stamp->allocationEpoch);
    span.benchLifetimeStamps.erase(stamp);
}

static size_t
benchLifetimeCensoredBinValue(size_t bin) {
    if (bin >= kBenchLifetimeBinCount) {
        return 0;
    }
    uint32_t completedCollections = static_cast<uint32_t>(brixGCRuntimeCollectionCount());
    size_t count = 0;
    std::lock_guard<std::mutex> lock(gBenchLifetimeOutstandingMutex);
    for (const auto& entry : gBenchLifetimeOutstandingEpochs) {
        size_t survived = static_cast<uint32_t>(completedCollections - entry.first);
        if (benchLifetimeBinFor(survived) == bin) {
            count += entry.second;
        }
    }
    return count;
}

static void
resetBenchLifetime() {
    // The benchmark harness calls reset before starting (or while quiescent between) passes.
    // Clear span-local samples too so an old pass cannot later publish into fresh counters.
    {
        std::lock_guard<std::recursive_mutex> collectorLock(gCollectorMutex);
        GCHeap& gcHeap = heap();
        GCPageHeapLockGuard spanLock(gcHeap.pageHeapMutex);
        for (GCSpan* span : gcHeap.allSpans) {
            if (span) {
                span->benchLifetimeStamps.clear();
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(gBenchLifetimeOutstandingMutex);
        gBenchLifetimeOutstandingEpochs.clear();
    }
    gBenchLifetimeAllocationSequence.store(0, std::memory_order_relaxed);
    gBenchLifetimeSampledReservations.store(0, std::memory_order_relaxed);
    gBenchLifetimeClearedReservations.store(0, std::memory_order_relaxed);
    gBenchLifetimeReclaimed.store(0, std::memory_order_relaxed);
    gBenchLifetimeOutstanding.store(0, std::memory_order_relaxed);
    for (auto& bin : gBenchLifetimeBins) {
        bin.store(0, std::memory_order_relaxed);
    }
}

#endif

static void
clearObjectSidePayload(GCSpan* span, size_t lane, char* payload, bool cleanup) {
    if (!payload) {
        return;
    }
    if (cleanup) {
        span->removeCleanupSidePayload();
        recordObjectSideCleanupPayloadClear();
    }
    recordObjectSidePayloadClear(lane);
}

ObjectSideMetadataTable::ObjectSideMetadataTable(size_t capacity)
    : capacity(capacity) {
    for (size_t lane = 0; lane < kObjectSidePayloadLaneCount; ++lane) {
        lanePayloads[lane].store(nullptr, std::memory_order_relaxed);
        laneCleanupOccupancy[lane].store(nullptr, std::memory_order_relaxed);
    }
}

ObjectSideMetadataTable::~ObjectSideMetadataTable() {
    for (size_t lane = 0; lane < kObjectSidePayloadLaneCount; ++lane) {
        delete[] lanePayloads[lane].load(std::memory_order_relaxed);
        delete[] laneCleanupOccupancy[lane].load(std::memory_order_relaxed);
    }
}

std::atomic<char*>*
ObjectSideMetadataTable::lane(size_t lane) const {
    return lanePayloads[lane].load(std::memory_order_acquire);
}

std::atomic<char*>*
ObjectSideMetadataTable::ensureLane(size_t lane) {
    std::atomic<char*>* existing = lanePayloads[lane].load(std::memory_order_acquire);
    if (existing) {
        return existing;
    }
    // Value-initialization zero-fills the array, leaving every slot null (inactive)
    std::atomic<char*>* fresh = new (std::nothrow) std::atomic<char*>[capacity]();
    if (!fresh) {
        return nullptr;
    }
    std::atomic<char*>* expected = nullptr;
    if (lanePayloads[lane].compare_exchange_strong(
            expected,
            fresh,
            std::memory_order_release,
            std::memory_order_acquire)) {
        return fresh;
    }
    delete[] fresh;
    return expected;
}

std::atomic<uint64_t>*
ObjectSideMetadataTable::cleanupOccupancy(size_t lane) const {
    return laneCleanupOccupancy[lane].load(std::memory_order_acquire);
}

std::atomic<uint64_t>*
ObjectSideMetadataTable::ensureCleanupOccupancy(size_t lane) {
    std::atomic<uint64_t>* existing = cleanupOccupancy(lane);
    if (existing) {
        return existing;
    }
    size_t wordCount = (capacity + 63) / 64;
    std::atomic<uint64_t>* fresh = new (std::nothrow) std::atomic<uint64_t>[wordCount]();
    if (!fresh) {
        return nullptr;
    }
    std::atomic<uint64_t>* expected = nullptr;
    if (laneCleanupOccupancy[lane].compare_exchange_strong(
            expected,
            fresh,
            std::memory_order_release,
            std::memory_order_acquire)) {
        return fresh;
    }
    delete[] fresh;
    return expected;
}

bool
ObjectSideMetadataTable::cleanupOccupied(size_t lane, size_t index) const {
    std::atomic<uint64_t>* words = cleanupOccupancy(lane);
    return words && index < capacity &&
        (words[index / 64].load(std::memory_order_acquire) &
         (uint64_t {1} << (index % 64))) != 0;
}

void
ObjectSideMetadataTable::setCleanupOccupied(size_t lane, size_t index) {
    std::atomic<uint64_t>* words = cleanupOccupancy(lane);
    if (!words || index >= capacity) {
        abortGC("GC cleanup side-payload occupancy is unavailable");
    }
    words[index / 64].fetch_or(uint64_t {1} << (index % 64), std::memory_order_release);
}

bool
ObjectSideMetadataTable::clearCleanupOccupied(size_t lane, size_t index) {
    std::atomic<uint64_t>* words = cleanupOccupancy(lane);
    if (!words || index >= capacity) {
        return false;
    }
    uint64_t mask = uint64_t {1} << (index % 64);
    return (words[index / 64].fetch_and(~mask, std::memory_order_acq_rel) & mask) != 0;
}

bool
GCSpan::needsSweepNow() const {
    return sweepGeneration() != brixGCPublicSweepGeneration();
}

unsigned char
GCSpan::reuseState() const {
    return needsSweepNow() ? kSpanReuseNeedsSweep : rawReuseState;
}

void
GCSpan::setListState(void* prev, void* next, void* owner) {
    listPrev = prev;
    listNext = next;
    listOwner = owner;
}

void
GCSpan::setListPrev(void* pointer) {
    listPrev = pointer;
}

void
GCSpan::setListNext(void* pointer) {
    listNext = pointer;
}

void
GCSpan::markReleased() {
    rawReuseState = kSpanReuseReleased;
}

bool
GCSpan::queueForRetirement() {
    unsigned char expected = kSpanRetirementActive;
    return retirementState.compare_exchange_strong(
        expected,
        kSpanRetirementQueued,
        std::memory_order_acq_rel,
        std::memory_order_acquire);
}

bool
GCSpan::markRetired() {
    unsigned char expected = kSpanRetirementQueued;
    return retirementState.compare_exchange_strong(
        expected,
        kSpanRetirementRetired,
        std::memory_order_acq_rel,
        std::memory_order_acquire);
}

bool
GCSpan::isRetired() const {
    return retirementState.load(std::memory_order_acquire) == kSpanRetirementRetired;
}

void
GCSpan::updateObjectMetadata(char* pointer, size_t objectSize, void* descriptor) {
    size_t index = 0;
    if (!objectIndex(pointer, index) || !allocBits.getAcquire(index)) {
        abortGC("GC invalid object metadata update");
    }
    sizes[index] = objectSize;
    descriptors[index] = descriptor;
}

GCSpan::GCSpan(GCArena* arena, char* memory, size_t pageCount, SizeClass sizeClass)
    : memory(memory),
      arena(arena),
      pageCount(pageCount),
      sizeClass(sizeClass),
      objectsCapacity(sizeClass.divideSlotOffset(pageCount * kPageSize)),
      sweepGen(static_cast<uint32_t>(brixGCPublicSweepGeneration())),
      flatMetadata(objectsCapacity),
      allocBits(objectsCapacity, flatMetadata.allocationWords),
      markBits(objectsCapacity, flatMetadata.markWords),
      discardedReservations(objectsCapacity, flatMetadata.discardedReservationWords),
      descriptors(flatMetadata.descriptors),
      sizes(flatMetadata.sizes) {
    if (flatMetadata.valid()) {
        // Provision persistent queue capacity before this span can publish into the address index.
        // WorkBuf objects never move, so span growth during marking cannot invalidate a participant.
        markCapacityRegistered = markWork().registerSpanCapacity(objectsCapacity);
    }
}

GCSpan::~GCSpan() {
    if (cleanupRegistrationCount.load(std::memory_order_acquire) != 0) {
        abortGC("GC retired a span with cleanup registrations");
    }
    if (cleanupSidePayloadCount.load(std::memory_order_acquire) != 0) {
        abortGC("GC retired a span with cleanup side payloads");
    }
    if (sweepRegistryIndex != kInvalidSweepRegistryIndex) {
        abortGC("GC retired a span still linked to its sweep registry");
    }
    markReleased();
    delete objectSideMetadata.load(std::memory_order_relaxed);
    if (markCapacityRegistered) {
        markWork().unregisterSpanCapacity(objectsCapacity);
    }
}

bool
GCSpan::isMarked(char* pointer) const {
    size_t index = 0;
    return containingObjectIndex(pointer, index) &&
        allocBits.getAcquire(index) && markBits.get(index);
}

bool
GCSpan::isMarkedSlot(size_t index) const {
    return allocBits.getAcquire(index) && markBits.get(index);
}

void
GCSpan::addCleanupRegistration() {
    cleanupRegistrationCount.fetch_add(1, std::memory_order_relaxed);
}

void
GCSpan::removeCleanupRegistration() {
    size_t count = cleanupRegistrationCount.load(std::memory_order_relaxed);
    while (count) {
        if (cleanupRegistrationCount.compare_exchange_weak(
                count,
                count - 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return;
        }
    }
    abortGC("GC cleanup registration count underflow");
}

bool
GCSpan::hasCleanupRegistrations() const {
    return cleanupRegistrationCount.load(std::memory_order_acquire) != 0;
}

void
GCSpan::addCleanupSidePayload() {
    cleanupSidePayloadCount.fetch_add(1, std::memory_order_relaxed);
}

void
GCSpan::removeCleanupSidePayload() {
    size_t count = cleanupSidePayloadCount.load(std::memory_order_relaxed);
    while (count) {
        if (cleanupSidePayloadCount.compare_exchange_weak(
                count,
                count - 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return;
        }
    }
    abortGC("GC cleanup side-payload count underflow");
}

bool
GCSpan::hasCleanupSidePayloads() const {
    return cleanupSidePayloadCount.load(std::memory_order_acquire) != 0;
}

char*
GCSpan::allocNext(size_t objectSize, void* descriptor) {
    if (!canAllocateNow()) {
        return nullptr;
    }
    size_t offset = 0;
    if (allocBits.findFrom(false, nextFreeIndex, offset)) {
        clearObjectSideMetadata(offset);
        discardedReservations.clear(offset);
        descriptors[offset] = descriptor;
        sizes[offset] = objectSize;
        if (needsZero.load(std::memory_order_relaxed)) {
            std::memset(memory + offset * sizeClass.size, 0, sizeClass.size);
        }
        allocBits.set(offset);
        nextFreeIndex = offset + 1;
        if (nextFreeIndex >= objectsCapacity) {
            nextFreeIndex = 0;
        }
        ++objectsUsed;
        rawReuseState = kSpanReuseActive;
#if BX_ENABLE_BENCH_METRICS
        stampBenchLifetimeAllocationRange(*this, offset, 1);
#endif
        return memory + offset * sizeClass.size;
    }
    objectsUsed = objectsCapacity;
    return nullptr;
}

size_t
GCSpan::allocBatch(
    size_t objectSize,
    void* descriptor,
    size_t preferredCount,
    char*& pointer,
    bool allocateBlack
) {
    pointer = nullptr;
    if (!preferredCount || !canAllocateNow()) {
        return 0;
    }

    size_t start = nextFreeIndex;
    size_t count = allocBits.clearRunFrom(start, preferredCount);

    if (count == 0) {
        size_t found = 0;
        if (!allocBits.findFrom(false, nextFreeIndex, found)) {
            objectsUsed = objectsCapacity;
            return 0;
        }
        start = found;
        count = allocBits.clearRunFrom(start, preferredCount);
    }

    if (count == 0) {
        return 0;
    }

    size_t end = start + count;
    if (objectSideMetadata.load(std::memory_order_acquire)) {
        for (size_t index = start; index < end; ++index) {
            clearObjectSideMetadata(index);
        }
    }
    discardedReservations.clearRange(start, count);
    void** slotDescriptors = descriptors;
    size_t* slotSizes = sizes;
    for (size_t index = start; index < end; ++index) {
        slotDescriptors[index] = descriptor;
        slotSizes[index] = objectSize;
    }
    if (needsZero.load(std::memory_order_relaxed)) {
        std::memset(memory + start * sizeClass.size, 0, count * sizeClass.size);
    }
    // Mark the complete reservation before release-publishing any allocation bit. This is the
    // allocate-black handoff invariant; unused tails are separately classified for sweep.
    if (allocateBlack) {
        markBits.setRange(start, count, std::memory_order_relaxed);
    }
    allocBits.setRange(start, count, std::memory_order_release);

    nextFreeIndex = end;
    if (nextFreeIndex >= objectsCapacity) {
        nextFreeIndex = 0;
    }
    objectsUsed += count;
    rawReuseState = kSpanReuseActive;
#if BX_ENABLE_BENCH_METRICS
    stampBenchLifetimeAllocationRange(*this, start, count);
#endif
    pointer = memory + start * sizeClass.size;
    return count;
}

bool
GCSpan::markAllocatedRange(char* pointer, size_t count, size_t slotSize) {
    if (!count || slotSize != sizeClass.size) {
        return false;
    }
    size_t start = 0;
    if (!objectIndex(pointer, start) || count > objectsCapacity - start) {
        return false;
    }
    for (size_t index = start; index < start + count; ++index) {
        if (!allocBits.getAcquire(index)) {
            return false;
        }
    }
    markBits.setRange(start, count, std::memory_order_relaxed);
    return true;
}

size_t
GCSpan::releaseBatch(char* cursor, char* limit, size_t slotSize) {
    if (slotSize != sizeClass.size || !cursor || !limit || limit <= cursor) {
        return 0;
    }
    // Once marking starts, an admitted scanner may retain this slot after its allocation-bit
    // acquire. Keep unconsumed reservations white and let the single sweeper reclaim them;
    // immediate reuse could otherwise race descriptor/object reinitialization before Idle.
    if (publicCollectorPhase() != kPhaseIdle) {
        return 0;
    }
    if (sweepGeneration() != brixGCPublicSweepGeneration()) {
        return 0;
    }
    size_t start = 0;
    if (!objectIndex(cursor, start)) {
        return 0;
    }
    size_t byteCount = static_cast<size_t>(limit - cursor);
    if (byteCount % slotSize != 0) {
        return 0;
    }
    size_t count = byteCount / slotSize;
    if (start + count > objectsCapacity) {
        return 0;
    }
    size_t released = 0;
    for (size_t index = start; index < start + count; ++index) {
        if (!allocBits.getAcquire(index)) {
            continue;
        }
        clearObjectSideMetadata(index);
        allocBits.clear(index);
        discardedReservations.clear(index);
        markBits.clear(index);
        descriptors[index] = nullptr;
        sizes[index] = 0;
#if BX_ENABLE_BENCH_METRICS
        // Batch release/discard returns slots that were reserved but never became public
        // allocations, so remove their samples without reporting artificial lifetimes.
        clearBenchLifetimeStamp(*this, index);
#endif
        ++released;
    }
    if (!released) {
        return 0;
    }
    if (objectsUsed < released) {
        abortGC("GC span object accounting underflow");
    }
    objectsUsed -= released;
    rawReuseState = objectsUsed == 0 ? kSpanReuseReset : kSpanReuseActive;
    nextFreeIndex = start;
    return released;
}

size_t
GCSpan::discardBatch(char* cursor, char* limit, size_t slotSize) {
    return releaseBatch(cursor, limit, slotSize);
}

bool
GCSpan::deferDiscardedBatch(char* cursor, char* limit, size_t slotSize) {
    if (slotSize != sizeClass.size || !cursor || !limit || limit <= cursor) {
        return false;
    }
    size_t start = 0;
    if (!objectIndex(cursor, start)) {
        return false;
    }
    size_t byteCount = static_cast<size_t>(limit - cursor);
    if (byteCount % slotSize != 0) {
        return false;
    }
    size_t count = byteCount / slotSize;
    if (start + count > objectsCapacity) {
        return false;
    }
    for (size_t index = start; index < start + count; ++index) {
        if (!allocBits.getAcquire(index)) {
            return false;
        }
    }
    // The caller holds the collector lock and has revalidated the batch generation, which
    // serializes this owner-only classification against the span flip even during Sweep.
    // Keep allocate-black marks intact: clearing them while allocBits remain visible would
    // reopen the mark-winner race. Sweep masks the classified tail out of its survivor set.
    for (size_t index = start; index < start + count; ++index) {
        discardedReservations.set(index);
    }
#if BX_ENABLE_BENCH_METRICS
    // The core passes only the unused tail of a discarded cache reservation. It remains
    // allocated/white until sweep, but is not a public object and must not acquire a lifetime.
    for (size_t index = start; index < start + count; ++index) {
        clearBenchLifetimeStamp(*this, index);
    }
#endif
    return true;
}

void
GCSpan::clearObjectSideMetadata(size_t index) {
    ObjectSideMetadataTable* table = objectSideMetadata.load(std::memory_order_acquire);
    if (!table || index >= table->capacity) {
        return;
    }
    for (size_t laneIndex = 0; laneIndex < kObjectSidePayloadLaneCount; ++laneIndex) {
        std::atomic<char*>* payloads = table->lane(laneIndex);
        if (!payloads) {
            continue;
        }
        char* payload = payloads[index].exchange(nullptr, std::memory_order_acq_rel);
        bool cleanup = table->clearCleanupOccupied(laneIndex, index);
        clearObjectSidePayload(this, laneIndex, payload, cleanup);
    }
}

void
GCSpan::clearMarkBits() {
    markBits.clearAll();
}

#if !defined(BX_RELEASE)
void
GCSpan::poisonObject(size_t index) {
    std::memset(memory + sizeClass.size * index, kSweptPoisonByte, sizeClass.size);
}
#endif

bool
GCSpan::tryClaimSweep() {
    if (!isRetirementActive() || !needsSweepNow()) {
        return false;
    }
    unsigned char expected = kSpanSweepAvailable;
    return sweepState.compare_exchange_strong(
        expected,
        kSpanSweepClaimed,
        std::memory_order_acq_rel,
        std::memory_order_acquire);
}

void
GCSpan::sweepClaimed() {
    if (sweepState.load(std::memory_order_acquire) != kSpanSweepClaimed) {
        abortGC("GC span sweep without claim");
    }
    // Claiming occurs without the store lock. The collector lock serializes descriptor and
    // object cleanup with verification and metadata publication; the short store re-home is
    // performed only after this flip has release-published its generation.
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    uint32_t publicGeneration = static_cast<uint32_t>(brixGCPublicSweepGeneration());
    if (sweepGeneration() == publicGeneration) {
        sweepState.store(kSpanSweepAvailable, std::memory_order_release);
        return;
    }
    brixGCRuntimeRecordSweepPages(pageCount);

    // Sticky mode deliberately retains both allocation and mark bits across this boundary.
    // It is the generational retrofit seam; Stage 1 keeps it compile-time disabled.
    static constexpr bool kStickyMarkBitsEnabled = false;
    size_t newObjectsUsed = 0;
    size_t newNextFreeIndex = 0;
    bool foundFree = false;
    size_t freedObjects = 0;
    size_t wordCount = spanBitmapWordCount(objectsCapacity);
    for (size_t wordIndex = 0; wordIndex < wordCount; ++wordIndex) {
        uint64_t validBits = ~uint64_t {0};
        if (wordIndex + 1 == wordCount && objectsCapacity % kSpanBitmapWordBits) {
            validBits = (uint64_t {1} << (objectsCapacity % kSpanBitmapWordBits)) - 1;
        }
        uint64_t allocated = flatMetadata.allocationWords[wordIndex].load(
            std::memory_order_relaxed) & validBits;
        uint64_t discarded = flatMetadata.discardedReservationWords[wordIndex].load(
            std::memory_order_relaxed) & allocated;
        uint64_t marked = flatMetadata.markWords[wordIndex].load(
            std::memory_order_relaxed) & allocated & ~discarded;
        uint64_t dead = kStickyMarkBitsEnabled ? discarded : allocated & ~marked;
        while (dead) {
            size_t bit = static_cast<size_t>(__builtin_ctzll(dead));
            size_t index = wordIndex * kSpanBitmapWordBits + bit;
            clearObjectSideMetadata(index);
#if !defined(BX_RELEASE)
            poisonObject(index);
#endif
            needsZero.store(true, std::memory_order_relaxed);
            descriptors[index] = nullptr;
            sizes[index] = 0;
#if BX_ENABLE_BENCH_METRICS
            recordBenchLifetimeReclamation(*this, index);
#endif
            ++freedObjects;
            dead &= dead - 1;
        }

        uint64_t nextAllocated = kStickyMarkBitsEnabled
            ? allocated & ~discarded
            : marked;
        flatMetadata.allocationWords[wordIndex].store(
            nextAllocated,
            std::memory_order_release);
        flatMetadata.markWords[wordIndex].store(
            kStickyMarkBitsEnabled ? marked : 0,
            std::memory_order_relaxed);
        flatMetadata.discardedReservationWords[wordIndex].store(
            0,
            std::memory_order_relaxed);
        newObjectsUsed += static_cast<size_t>(__builtin_popcountll(nextAllocated));
        uint64_t freeBits = (~nextAllocated) & validBits;
        if (!foundFree && freeBits) {
            newNextFreeIndex = wordIndex * kSpanBitmapWordBits +
                static_cast<size_t>(__builtin_ctzll(freeBits));
            foundFree = true;
        }
    }
    if (freedObjects) {
        brixGCRuntimeRecordSweep(freedObjects, freedObjects * sizeClass.size);
    }
    objectsUsed = newObjectsUsed;
    nextFreeIndex = newNextFreeIndex;
    // Release-publish the completed metadata flip to the next span owner.
    sweepGen.store(publicGeneration, std::memory_order_release);
    rawReuseState = objectsUsed == 0 ? kSpanReuseReset : kSpanReuseSwept;
    sweepState.store(kSpanSweepAvailable, std::memory_order_release);
}

bool
GCSpan::sweep() {
    if (!tryClaimSweep()) {
        return false;
    }
    sweepClaimed();
    return true;
}

bool
GCSpanList::isEmpty() const {
    return length == 0;
}

bool
GCSpanList::contains(GCSpan* span) const {
    return span && span->listOwner == this;
}

void
GCSpanList::push(GCSpan* span) {
    if (!span) {
        abortGC("GC null span list push");
    }
    if (span->listOwner == this) {
        return;
    }
    if (span->listOwner) {
        abortGC("GC span belongs to another list");
    }
    span->setListState(tail, nullptr, this);
    if (tail) {
        static_cast<GCSpan*>(tail)->setListNext(span);
    } else {
        head = span;
    }
    tail = span;
    ++length;
}

GCSpan*
GCSpanList::pop() {
    if (!tail) {
        return nullptr;
    }
    GCSpan* span = static_cast<GCSpan*>(tail);
    remove(span);
    return span;
}

void
GCSpanList::remove(GCSpan* span) {
    if (!span || span->listOwner != this) {
        return;
    }
    void* prev = span->listPrev;
    void* next = span->listNext;
    if (prev) {
        static_cast<GCSpan*>(prev)->setListNext(next);
    } else {
        head = next;
    }
    if (next) {
        static_cast<GCSpan*>(next)->setListPrev(prev);
    } else {
        tail = prev;
    }
    span->setListState(nullptr, nullptr, nullptr);
    --length;
}

size_t
GCSpanList::countOf(GCSpan* span) const {
    size_t count = 0;
    void* current = head;
    while (current) {
        if (current == span) {
            ++count;
        }
        current = static_cast<GCSpan*>(current)->listNext;
    }
    return count;
}

bool
GCSpanList::verifyLinks() const {
    if (!head && tail) {
        return false;
    }
    if (head && !tail) {
        return false;
    }
    void* current = head;
    void* previous = nullptr;
    size_t count = 0;
    while (current) {
        if (count >= length) {
            return false;
        }
        GCSpan* span = static_cast<GCSpan*>(current);
        if (span->listOwner != this || span->listPrev != previous) {
            return false;
        }
        previous = current;
        current = span->listNext;
        ++count;
    }
    return previous == tail && count == length;
}

} // namespace BxGC

#if BX_ENABLE_BENCH_METRICS

extern "C" size_t
brixGCBenchLifetimeBinCount(void) {
    return BxGC::kBenchLifetimeBinCount;
}

extern "C" size_t
brixGCBenchLifetimeBinUpperBound(size_t index) {
    return index < BxGC::kBenchLifetimeBinCount
        ? BxGC::kBenchLifetimeBinUpperBounds[index]
        : std::numeric_limits<size_t>::max();
}

extern "C" size_t
brixGCBenchLifetimeBinValue(size_t index) {
    return index < BxGC::kBenchLifetimeBinCount
        ? BxGC::gBenchLifetimeBins[index].load(std::memory_order_relaxed)
        : 0;
}

extern "C" size_t
brixGCBenchLifetimeCensoredBinValue(size_t index) {
    return BxGC::benchLifetimeCensoredBinValue(index);
}

extern "C" size_t
brixGCBenchLifetimeSampledReservationCount(void) {
    return BxGC::gBenchLifetimeSampledReservations.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCBenchLifetimeClearedReservationCount(void) {
    return BxGC::gBenchLifetimeClearedReservations.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCBenchLifetimeReclaimedCount(void) {
    return BxGC::gBenchLifetimeReclaimed.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCBenchLifetimeOutstandingCount(void) {
    return BxGC::gBenchLifetimeOutstanding.load(std::memory_order_relaxed);
}

extern "C" size_t
brixGCBenchLifetimeSampleEvery(void) {
    return BxGC::kBenchLifetimeSampleEvery;
}

extern "C" void
brixGCBenchLifetimeReset(void) {
    BxGC::resetBenchLifetime();
}

#endif
