#ifndef YU_GC_HEAP_H
#define YU_GC_HEAP_H

#include "YuGCArena.h"
#include "YuGCSpanStore.h"

namespace YuGC {

struct GCHeap {
    std::vector<GCArena*> arenas;
    GCArena* reserveArena = nullptr;
    std::vector<GCArena*> retiredArenas;
    std::vector<std::pair<char*, size_t>> retiredArenaMappings;
    std::vector<GCSpan*> allSpans;
    std::array<GCSpanStore*, kSizeClassesCount> spanStores {};
    GCSpanList largeSpans;
    size_t nextArenaSearch = 0;
    std::atomic<size_t> sweepStoreIndex {0};
    std::atomic<GCSpan*> retiredSpanHead {nullptr};
    std::atomic<size_t> retiredSpanQueueCount {0};
    std::atomic<size_t> retiredSpanDestroyCount {0};
    std::vector<GCSpan*> largeSweepSpans;
    std::atomic<size_t> largeSweepSpanCount {0};
    std::atomic<size_t> largeSweepCursor {0};
    std::atomic<size_t> largeSweepLimit {0};
    std::atomic<uint32_t> largeSweepCursorGen {0};
    std::atomic<uint32_t> sweepGen {0};
    std::atomic<size_t> demandHighWaterPages {0};
    std::atomic<size_t> peakDemandPagesEWMA {0};
    std::mutex pageHeapMutex;
    std::mutex largeSpanMutex;

    GCHeap();

    uint32_t currentSweepGen();

    void beginMarkCycle(bool eagerClear);

    void advanceSweepGeneration();

    void clearMarkBits();

    bool registerArena(GCArena* arena);

    GCArena* allocArena(size_t dedicatedPageCount = 0);

    void freeArenaPagesLocked(GCArena* arena, char* pages, size_t pageCount);

    void retireDedicatedArenaLocked(GCArena* arena);

    void unmapRetiredArenaMemory();

    GCArena* findArena(char* pointer);

    static GCSpan* pointerToSpan(char* pointer) {
        return GCAddressIndex::instance().lookupSpan(pointer);
    }

    bool verifyObject(char* pointer);

    void* objectDescriptor(char* pointer);

    char* allocPages(size_t pageCount, bool bestFit = false, bool* zeroed = nullptr);

    void trackSpan(GCSpan* span);

    void untrackSpan(GCSpan* span);

    GCSpan* createSpan(SizeClass sizeClass);

    GCSpan* createLargeSpanUnpublished(SizeClass sizeClass);

    void publishLargeSpan(GCSpan* span);

    void releaseLargeSpanUnpublished(GCSpan* span);

    GCSpan* findLargeSpanForAllocationLocked(SizeClass sizeClass);

    bool sweepOneLargeClaimable();

    char* allocObject(SizeClass sizeClass, size_t objectSize, void* descriptor);

    size_t allocBatch(
        SizeClass sizeClass,
        size_t objectSize,
        void* descriptor,
        size_t preferredCount,
        char*& pointer,
        bool allocateBlack);

    bool releaseBatch(char* cursor, char* limit, size_t slotSize);

    bool discardBatch(char* cursor, char* limit, size_t slotSize, size_t generation);

    bool verifyLifecycle();

    void freeSpan(GCSpan* span);

    void retireQueuedSpansAtSafepoint();

    void unregisterRetiringSpan(GCSpan* span);

    size_t pendingRetiredSpanCount() const;

    size_t destroyedRetiredSpanCount() const;

    void resetIncrementalSweepCursors();

    void resetLargeSweepCursor();

    void revisitSweepStore(size_t index);

    size_t sweepLargeSpansStep(size_t budget);

    void sweepAll();

    size_t sweepIncrementalStep(size_t workLimit);

    bool incrementalSweepComplete();

    void releaseEmptySpansForScavenge();

    size_t scavengeStep(
        size_t pageBudget,
        bool releaseArenas,
        bool respectRetentionTarget = true);

    size_t scavengeDebtPageCount(bool respectRetentionTarget = true);

    size_t releaseEmptyArenas(size_t budget);

    size_t committedPageCount();

    size_t decommittedPageCount();

    size_t dirtyPageCount();

    size_t freePageCount();

    size_t reservedArenaBytes();

    size_t largeSpanFragmentationBytes();

    size_t fragmentationBytes();

    size_t sizeClassFragmentationBytes(size_t index);

    size_t sizeClassOccupancyBucketCount(size_t index, size_t bucket);

    size_t sizeClassSweepDebt(size_t index);

    size_t largeSpanSweepDebt();

    bool hasLargeSpanSweepDebt();

    bool hasSweepDebt();

    void recordPageDemandLocked();

    void updatePeakDemandLocked();

    size_t retentionTargetPagesLocked() const;
};

} // namespace YuGC

#endif
