#ifndef BX_GC_SPAN_STORE_H
#define BX_GC_SPAN_STORE_H

#include "BxGCLockOrder.h"
#include "BxGCSpan.h"

namespace BxGC {

static const size_t kSpanStoreLaneCount = 16;
static const size_t kStealablePartialSpanCount = 2;
static const size_t kInheritStoreLane = std::numeric_limits<size_t>::max();

struct alignas(128) GCSpanStoreLane {
    GCSpanList partialSpans;
    GCSpanList fullSpans;
    std::atomic<size_t> partialCount {0};
    std::mutex mutex;
};

struct GCSpanStore {
    SizeClass sizeClass {};
    GCSpanStoreLane* lanes = nullptr;
    // Registry ordinals are stable across partial/full re-homing. The quiescent retirement drain
    // may swap-fill an ordinal before metadata destruction, so no slot points at freed metadata.
    std::vector<GCSpan*> sweepSpans;
    std::atomic<size_t> sweepSpanCount {0};
    std::atomic<size_t> sweepCursor {0};
    std::atomic<size_t> sweepLimit {0};
    std::atomic<uint32_t> sweepCursorGen {0};
    std::mutex registryMutex;

    explicit GCSpanStore(SizeClass sizeClass);

    void registerSpan(GCSpan* span);

    char* allocObject(size_t objectSize, void* descriptor);
    size_t allocBatch(
        size_t objectSize,
        void* descriptor,
        size_t preferredCount,
        char*& pointer,
        bool allocateBlack);
    bool releaseBatch(char* cursor, char* limit, size_t slotSize);
    bool discardBatch(char* cursor, char* limit, size_t slotSize, size_t generation);
    void sweep();
    size_t sweepStep(size_t budget);
    bool sweepComplete();
    size_t sweepDebt();
    bool hasSweepDebt();
    void releaseEmptySpansForScavenge();
    void resetSweepCursor();
    void retireSpan(GCSpan* span);
    size_t fragmentationBytes();
    size_t occupancyBucketCount(size_t bucket);
    bool verifyLifecycle();

private:
    void registerSpanLocked(GCSpan* span);
    GCSpanStoreLane& lockSpanLane(GCSpan* span, size_t targetLane, GCStoreLockPair& locks);
    template<typename Allocate>
    bool allocate(Allocate allocateFromSpan);
    template<typename Allocate>
    bool allocateFromLane(GCSpanStoreLane& lane, bool packingOnly, Allocate allocateFromSpan);
    template<typename Allocate>
    bool stealPartial(size_t laneIndex, Allocate allocateFromSpan);
    GCSpan* selectPartialLocked(GCSpanStoreLane& lane, bool packingOnly);
    bool sweepOneClaimable(size_t laneIndex);
    size_t sweepClaimable(size_t budget, size_t targetLane);
    void rehomeSweptSpan(GCSpan* span, size_t targetLane);
};

} // namespace BxGC

#endif
