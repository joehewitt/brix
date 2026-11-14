#include "YuGCHeap.h"
#include "YuGCLockOrder.h"

#include <cstdlib>
#include <new>

namespace YuGC {

namespace {

std::atomic<size_t> gNextStoreLane {0};
thread_local size_t tStoreLane = kInheritStoreLane;

size_t
currentStoreLane() {
    if (tStoreLane == kInheritStoreLane) {
        tStoreLane = gNextStoreLane.fetch_add(1, std::memory_order_relaxed) % kSpanStoreLaneCount;
    }
    return tStoreLane;
}

void
publishPartialCount(GCSpanStoreLane& lane) {
    lane.partialCount.store(lane.partialSpans.length, std::memory_order_relaxed);
}

void
pushLocked(GCSpanStoreLane& lane, GCSpan* span) {
    if (span->isFull()) {
        lane.fullSpans.push(span);
    } else {
        lane.partialSpans.push(span);
        publishPartialCount(lane);
    }
}

} // namespace

GCSpanStore::GCSpanStore(SizeClass sizeClass)
    : sizeClass(sizeClass) {
    void* memory = nullptr;
    if (posix_memalign(
            &memory,
            alignof(GCSpanStoreLane),
            sizeof(GCSpanStoreLane) * kSpanStoreLaneCount) != 0) {
        abortGC("GC span-store lane allocation failed");
    }
    lanes = static_cast<GCSpanStoreLane*>(memory);
    for (size_t index = 0; index < kSpanStoreLaneCount; ++index) {
        new (&lanes[index]) GCSpanStoreLane();
    }
}

void
GCSpanStore::registerSpan(GCSpan* span) {
    GCStoreLockGuard lock(registryMutex);
    registerSpanLocked(span);
}

void
GCSpanStore::registerSpanLocked(GCSpan* span) {
    if (!span) {
        return;
    }
    if (span->sweepRegistryIndex != kInvalidSweepRegistryIndex) {
        size_t index = span->sweepRegistryIndex;
        if (index >= sweepSpans.size() || sweepSpans[index] != span) {
            abortGC("GC span sweep-registry ordinal mismatch");
        }
        return;
    }
    try {
        span->sweepRegistryIndex = sweepSpans.size();
        sweepSpans.push_back(span);
    } catch (...) {
        span->sweepRegistryIndex = kInvalidSweepRegistryIndex;
        abortGC("GC sweep registry allocation failed");
    }
    sweepSpanCount.store(sweepSpans.size(), std::memory_order_release);
}

GCSpanStoreLane&
GCSpanStore::lockSpanLane(GCSpan* span, size_t targetLane, GCStoreLockPair& locks) {
    while (true) {
        size_t index = span->storeLane.load(std::memory_order_acquire);
        if (targetLane == kInheritStoreLane || targetLane == index) {
            locks.lock(lanes[index].mutex);
        } else {
            locks.lock(
                lanes[std::min(index, targetLane)].mutex,
                lanes[std::max(index, targetLane)].mutex);
        }
        if (span->storeLane.load(std::memory_order_relaxed) == index) {
            return lanes[index];
        }
        locks.unlock();
    }
}

GCSpan*
GCSpanStore::selectPartialLocked(GCSpanStoreLane& lane, bool packingOnly) {
    GCSpan* highest = nullptr;
    GCSpan* lowest = nullptr;
    void* cursor = lane.partialSpans.head;
    size_t inspected = 0;
    while (cursor && inspected < kPartialSpanScanBudget) {
        GCSpan* span = static_cast<GCSpan*>(cursor);
        cursor = span->listNext;
        ++inspected;
        if (!span->canAllocateNow()) {
            lane.partialSpans.remove(span);
            lane.partialSpans.push(span);
            continue;
        }
        if (!span->isRetirementActive() || span->isFull()) {
            continue;
        }
        if (!lowest || span->objectsUsed < lowest->objectsUsed) {
            lowest = span;
        }
        bool packed = span->objectsCapacity != 0 &&
            span->objectsUsed * 100 >=
                span->objectsCapacity * kSpanPackingOccupancyPercent;
        if (packed && (!highest || span->objectsUsed > highest->objectsUsed)) {
            highest = span;
        }
    }
    return highest ? highest : (packingOnly ? nullptr : lowest);
}

void
GCSpanStore::rehomeSweptSpan(GCSpan* span, size_t targetLane) {
    if (!span) {
        return;
    }
    GCStoreLockPair locks;
    GCSpanStoreLane& lane = lockSpanLane(span, targetLane, locks);
    if (!span->isRetirementActive()) {
        return;
    }
    lane.partialSpans.remove(span);
    lane.fullSpans.remove(span);
    publishPartialCount(lane);
    if (span->isEmpty()) {
        heap().freeSpan(span);
        return;
    }
    if (targetLane == kInheritStoreLane) {
        pushLocked(lane, span);
        return;
    }
    span->storeLane.store(targetLane, std::memory_order_release);
    pushLocked(lanes[targetLane], span);
}

bool
GCSpanStore::sweepOneClaimable(size_t laneIndex) {
    if (publicCollectorPhase() != kPhaseSweep) {
        return false;
    }
    // Allocation participates in the same monotonic traversal as background and explicit
    // sweep work. Restarting at registry index zero on every refill makes a tombstone-heavy
    // workload quadratic even though each span itself is claimed only once.
    return sweepClaimable(1, laneIndex) != 0;
}

template<typename Allocate>
bool
GCSpanStore::allocateFromLane(
    GCSpanStoreLane& lane,
    bool packingOnly,
    Allocate allocateFromSpan
) {
    GCStoreLockGuard lock(lane.mutex);
    GCSpan* span = selectPartialLocked(lane, packingOnly);
    if (!span) {
        return false;
    }
    allocateFromSpan(span);
    if (span->isFull()) {
        lane.partialSpans.remove(span);
        lane.fullSpans.push(span);
        publishPartialCount(lane);
    }
    return true;
}

template<typename Allocate>
bool
GCSpanStore::stealPartial(size_t laneIndex, Allocate allocateFromSpan) {
    for (size_t offset = 1; offset < kSpanStoreLaneCount; ++offset) {
        size_t victimIndex = (laneIndex + offset) % kSpanStoreLaneCount;
        GCSpanStoreLane& victim = lanes[victimIndex];
        if (victim.partialCount.load(std::memory_order_relaxed) < kStealablePartialSpanCount) {
            continue;
        }
        GCStoreLockPair locks;
        locks.lock(
            lanes[std::min(laneIndex, victimIndex)].mutex,
            lanes[std::max(laneIndex, victimIndex)].mutex);
        if (victim.partialSpans.length < kStealablePartialSpanCount) {
            continue;
        }
        GCSpan* span = selectPartialLocked(victim, false);
        if (!span) {
            continue;
        }
        victim.partialSpans.remove(span);
        publishPartialCount(victim);
        span->storeLane.store(laneIndex, std::memory_order_release);
        allocateFromSpan(span);
        pushLocked(lanes[laneIndex], span);
        return true;
    }
    return false;
}

template<typename Allocate>
bool
GCSpanStore::allocate(Allocate allocateFromSpan) {
    size_t laneIndex = currentStoreLane();
    GCSpanStoreLane& lane = lanes[laneIndex];
    if (allocateFromLane(lane, true, allocateFromSpan)) {
        return true;
    }
    bool swept = sweepOneClaimable(laneIndex);
    if (allocateFromLane(lane, false, allocateFromSpan)) {
        return true;
    }
    if (!swept && stealPartial(laneIndex, allocateFromSpan)) {
        return true;
    }

    GCSpan* span = heap().createSpan(sizeClass);
    if (!span) {
        return false;
    }
    span->storeLane.store(laneIndex, std::memory_order_relaxed);
    allocateFromSpan(span);
    GCStoreLockGuard lock(lane.mutex);
    pushLocked(lane, span);
    return true;
}

char*
GCSpanStore::allocObject(size_t objectSize, void* descriptor) {
    char* pointer = nullptr;
    bool allocated = allocate([&](GCSpan* span) {
        pointer = span->allocNext(objectSize, descriptor);
        if (!pointer) {
            abortGC("GC span-store object allocation failed");
        }
    });
    return allocated ? pointer : nullptr;
}

size_t
GCSpanStore::allocBatch(
    size_t objectSize,
    void* descriptor,
    size_t preferredCount,
    char*& pointer,
    bool allocateBlack
) {
    size_t count = 0;
    bool allocated = allocate([&](GCSpan* span) {
        count = span->allocBatch(
            objectSize, descriptor, preferredCount, pointer, allocateBlack);
        if (!count || !pointer) {
            abortGC("GC span-store batch allocation failed");
        }
    });
    if (!allocated) {
        pointer = nullptr;
        return 0;
    }
    return count;
}

bool
GCSpanStore::releaseBatch(char* cursor, char* limit, size_t slotSize) {
    GCSpan* span = GCHeap::pointerToSpan(cursor);
    if (!span || span->sizeClass.index != sizeClass.index) {
        return false;
    }
    GCStoreLockPair locks;
    GCSpanStoreLane& lane = lockSpanLane(span, kInheritStoreLane, locks);
    size_t released = span->releaseBatch(cursor, limit, slotSize);
    if (!released) {
        return false;
    }
    if (!span->isFull() && lane.fullSpans.contains(span)) {
        lane.fullSpans.remove(span);
        lane.partialSpans.push(span);
        publishPartialCount(lane);
    }
    yumiGCRuntimeRecordBatchRelease(released, slotSize);
    yumiGCRuntimeRecordSweep(released, released * slotSize);
    return true;
}

bool
GCSpanStore::discardBatch(
    char* cursor,
    char* limit,
    size_t slotSize,
    size_t generation
) {
    if (!cursor || !limit || !slotSize || limit <= cursor) {
        return false;
    }
    size_t byteCount = static_cast<size_t>(limit - cursor);
    if (byteCount % slotSize != 0) {
        return false;
    }

    GCSpan* span = GCHeap::pointerToSpan(cursor);
    if (span && span->sizeClass.index == sizeClass.index) {
        GCStoreLockPair locks;
        GCSpanStoreLane& lane = lockSpanLane(span, kInheritStoreLane, locks);
        if (span->sweepGeneration() == generation) {
            size_t released = span->discardBatch(cursor, limit, slotSize);
            if (released) {
                if (!span->isFull() && lane.fullSpans.contains(span)) {
                    lane.fullSpans.remove(span);
                    lane.partialSpans.push(span);
                    publishPartialCount(lane);
                }
                yumiGCRuntimeRecordSweep(released, released * slotSize);
            }
        }
    }
    yumiGCRuntimeRecordBatchDiscard(byteCount / slotSize, slotSize);
    return true;
}

void
GCSpanStore::sweep() {
    resetSweepCursor();
    while (!sweepComplete()) {
        sweepStep(std::numeric_limits<size_t>::max());
    }
}

void
GCSpanStore::resetSweepCursor() {
    size_t limit = sweepSpanCount.load(std::memory_order_acquire);
    sweepCursor.store(0, std::memory_order_relaxed);
    sweepLimit.store(limit, std::memory_order_relaxed);
    sweepCursorGen.store(
        static_cast<uint32_t>(yumiGCPublicSweepGeneration()),
        std::memory_order_release);
}

size_t
GCSpanStore::sweepStep(size_t budget) {
    return sweepClaimable(budget, kInheritStoreLane);
}

size_t
GCSpanStore::sweepClaimable(size_t budget, size_t targetLane) {
    if (!budget) {
        return 0;
    }
    uint32_t publicGen = static_cast<uint32_t>(yumiGCPublicSweepGeneration());
    if (sweepCursorGen.load(std::memory_order_acquire) != publicGen) {
        resetSweepCursor();
    }

    size_t sweptPages = 0;
    size_t limit = sweepLimit.load(std::memory_order_acquire);
    while (sweptPages < budget) {
        size_t index = sweepCursor.fetch_add(1, std::memory_order_acq_rel);
        if (index >= limit) {
            break;
        }
        GCSpan* span = nullptr;
        {
            GCStoreLockGuard lock(registryMutex);
            if (index < sweepSpans.size()) {
                span = sweepSpans[index];
            }
            if (!span || !span->tryClaimSweep()) {
                span = nullptr;
            }
        }
        if (!span) {
            continue;
        }
        size_t pages = span->pageCount;
        span->sweepClaimed();
        rehomeSweptSpan(span, targetLane);
        sweptPages += pages;
    }
    return sweptPages;
}

bool
GCSpanStore::sweepComplete() {
    uint32_t publicGen = static_cast<uint32_t>(yumiGCPublicSweepGeneration());
    return sweepCursorGen.load(std::memory_order_acquire) == publicGen &&
        sweepCursor.load(std::memory_order_acquire) >=
            sweepLimit.load(std::memory_order_acquire);
}

size_t
GCSpanStore::sweepDebt() {
    GCStoreLockGuard lock(registryMutex);
    size_t debtPages = 0;
    for (GCSpan* span : sweepSpans) {
        if (span && span->isRetirementActive() && span->needsSweepNow()) {
            debtPages += span->pageCount;
        }
    }
    return debtPages;
}

bool
GCSpanStore::hasSweepDebt() {
    GCStoreLockGuard lock(registryMutex);
    size_t count = sweepSpans.size();
    size_t start = sweepCursor.load(std::memory_order_acquire);
    if (start >= count) {
        start = 0;
    }
    for (size_t offset = 0; offset < count; ++offset) {
        size_t index = start + offset;
        if (index >= count) {
            index -= count;
        }
        GCSpan* span = sweepSpans[index];
        if (span && span->isRetirementActive() && span->needsSweepNow()) {
            return true;
        }
    }
    return false;
}

void
GCSpanStore::releaseEmptySpansForScavenge() {
    for (size_t index = 0; index < kSpanStoreLaneCount; ++index) {
        GCSpanStoreLane& lane = lanes[index];
        GCStoreLockGuard lock(lane.mutex);
        void* partial = lane.partialSpans.head;
        while (partial) {
            GCSpan* span = static_cast<GCSpan*>(partial);
            partial = span->listNext;
            if (span->isEmpty() &&
                    span->sweepState.load(std::memory_order_acquire) == kSpanSweepAvailable) {
                lane.partialSpans.remove(span);
                heap().freeSpan(span);
            }
        }
        publishPartialCount(lane);
    }
}

void
GCSpanStore::retireSpan(GCSpan* span) {
    GCStoreLockGuard lock(registryMutex);
    size_t index = span ? span->sweepRegistryIndex : kInvalidSweepRegistryIndex;
    if (index == kInvalidSweepRegistryIndex ||
            index >= sweepSpans.size() || sweepSpans[index] != span) {
        abortGC("GC retiring span missing from sweep registry");
    }
    size_t lastIndex = sweepSpans.size() - 1;
    if (index != lastIndex) {
        GCSpan* moved = sweepSpans.back();
        if (!moved) {
            abortGC("GC sweep registry compaction selected a tombstone");
        }
        sweepSpans[index] = moved;
        moved->sweepRegistryIndex = index;
    }
    sweepSpans.pop_back();
    span->sweepRegistryIndex = kInvalidSweepRegistryIndex;
    size_t compactedSize = sweepSpans.size();
    sweepSpanCount.store(compactedSize, std::memory_order_release);
    // Retirement runs only at a quiescent drain after current-generation debt is clear. A stale
    // cursor may therefore skip or revisit the moved span harmlessly: tryClaimSweep rejects work
    // already completed for the generation. Clamp the completed traversal to the compacted size.
    sweepLimit.store(
        std::min(sweepLimit.load(std::memory_order_relaxed), compactedSize),
        std::memory_order_relaxed);
    sweepCursor.store(
        std::min(sweepCursor.load(std::memory_order_relaxed), compactedSize),
        std::memory_order_relaxed);
}

size_t
GCSpanStore::fragmentationBytes() {
    size_t bytes = 0;
    for (size_t index = 0; index < kSpanStoreLaneCount; ++index) {
        GCSpanStoreLane& lane = lanes[index];
        GCStoreLockGuard lock(lane.mutex);
        void* cursor = lane.partialSpans.head;
        while (cursor) {
            GCSpan* span = static_cast<GCSpan*>(cursor);
            cursor = span->listNext;
            if (!span->isRetirementActive()) {
                continue;
            }
            size_t used = span->objectsUsed * span->sizeClass.size;
            size_t capacity = span->objectsCapacity * span->sizeClass.size;
            if (capacity > used) {
                bytes += capacity - used;
            }
        }
    }
    return bytes;
}

size_t
GCSpanStore::occupancyBucketCount(size_t bucket) {
    if (bucket >= kSpanOccupancyBucketCount) {
        return 0;
    }
    size_t count = 0;
    auto countList = [&](GCSpanList& list) {
        void* cursor = list.head;
        while (cursor) {
            GCSpan* span = static_cast<GCSpan*>(cursor);
            cursor = span->listNext;
            if (!span->isRetirementActive() || !span->objectsCapacity) {
                continue;
            }
            size_t occupancy = std::min<size_t>(
                100,
                span->objectsUsed * 100 / span->objectsCapacity);
            size_t spanBucket = std::min<size_t>(
                kSpanOccupancyBucketCount - 1,
                occupancy / 10);
            if (spanBucket == bucket) {
                ++count;
            }
        }
    };
    for (size_t index = 0; index < kSpanStoreLaneCount; ++index) {
        GCSpanStoreLane& lane = lanes[index];
        GCStoreLockGuard lock(lane.mutex);
        countList(lane.partialSpans);
        countList(lane.fullSpans);
    }
    return count;
}

bool
GCSpanStore::verifyLifecycle() {
    auto laneOwnsList = [](const GCSpanList& list, size_t laneIndex) {
        if (!list.verifyLinks()) {
            return false;
        }
        for (void* cursor = list.head; cursor;
                cursor = static_cast<GCSpan*>(cursor)->listNext) {
            GCSpan* span = static_cast<GCSpan*>(cursor);
            if (span->storeLane.load(std::memory_order_relaxed) != laneIndex) {
                return false;
            }
        }
        return true;
    };
    for (size_t index = 0; index < kSpanStoreLaneCount; ++index) {
        GCSpanStoreLane& lane = lanes[index];
        GCStoreLockGuard lock(lane.mutex);
        if (!laneOwnsList(lane.partialSpans, index) || !laneOwnsList(lane.fullSpans, index) ||
                lane.partialCount.load(std::memory_order_relaxed) != lane.partialSpans.length) {
            return false;
        }
    }
    GCStoreLockGuard lock(registryMutex);
    for (size_t index = 0; index < sweepSpans.size(); ++index) {
        GCSpan* span = sweepSpans[index];
        if (!span || span->sizeClass.index != sizeClass.index ||
                span->sweepRegistryIndex != index) {
            return false;
        }
    }
    return true;
}

} // namespace YuGC
