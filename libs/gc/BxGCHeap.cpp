#include "BxGCHeap.h"
#include "BxGCLockOrder.h"

namespace BxGC {

static bool
scavengeRetentionOverrideBytes(size_t& bytes) {
    const char* value = std::getenv("BRIX_GC_SCAVENGE_RETAIN_BYTES");
    if (!value || !*value) {
        return false;
    }
    char* end = nullptr;
    unsigned long long parsed = std::strtoull(value, &end, 10);
    if (end && *end) {
        return false;
    }
    bytes = static_cast<size_t>(parsed);
    return true;
}

static size_t
scavengeRetentionSlackPercent() {
    const char* value = std::getenv("BRIX_GC_SCAVENGE_SLACK_PERCENT");
    if (!value || !*value) {
        return kScavengeRetentionSlackPercent;
    }
    char* end = nullptr;
    unsigned long long parsed = std::strtoull(value, &end, 10);
    return end && *end
        ? kScavengeRetentionSlackPercent
        : std::min<size_t>(static_cast<size_t>(parsed), 10000);
}

GCHeap&
heap() {
    static GCHeap instance;
    return instance;
}

GCHeap::GCHeap() {
    for (size_t index = 0; index < spanStores.size(); ++index) {
        spanStores[index] = new GCSpanStore(sizeClassForIndex(index));
    }
    // Reserve one virtual arena before mutator pressure begins. It is address-space backed and
    // committed by page on demand, giving allocation/collector metadata progress a bounded
    // fallback even if a later arena reservation fails.
    reserveArena = allocArena();
}

uint32_t
GCHeap::currentSweepGen() {
    return sweepGen.load(std::memory_order_acquire);
}

void
GCHeap::beginMarkCycle(bool eagerClear) {
    if (eagerClear) {
        clearMarkBits();
    }
}

void
GCHeap::advanceSweepGeneration() {
    uint32_t generation = sweepGen.fetch_add(1, std::memory_order_acq_rel) + 1;
    publishSweepGeneration(generation);
}

void
GCHeap::clearMarkBits() {
    GCPageHeapLockGuard lock(pageHeapMutex);
    for (GCSpan* span : allSpans) {
        if (span) {
            span->clearMarkBits();
        }
    }
}

bool
GCHeap::registerArena(GCArena* arena) {
    return arena && arena->memory && arena->publishAddressIndex();
}

GCArena*
GCHeap::allocArena(size_t dedicatedPageCount) {
    GCArena* arena = nullptr;
    try {
        arena = new GCArena(dedicatedPageCount);
    } catch (...) {
        return nullptr;
    }
    if (!registerArena(arena)) {
        delete arena;
        return nullptr;
    }
    try {
        arenas.push_back(arena);
    } catch (...) {
        arena->unpublishAddressIndex();
        delete arena;
        return nullptr;
    }
    if (!arena->dedicated) {
        nextArenaSearch = arenas.size() - 1;
    }
    brixGCRuntimeRecordArenaAllocation();
    return arena;
}

void
GCHeap::freeArenaPagesLocked(GCArena* arena, char* pages, size_t pageCount) {
    arena->freePages(pages, pageCount);
    if (arena->dedicated) {
        retireDedicatedArenaLocked(arena);
    }
}

void
GCHeap::retireDedicatedArenaLocked(GCArena* arena) {
    auto found = std::find(arenas.begin(), arenas.end(), arena);
    if (found == arenas.end()) {
        abortGC("GC dedicated arena not tracked");
    }
    if (!arena->unpublishAddressIndex()) {
        abortGC("GC dedicated arena address-index unpublication failed");
    }
    *found = arenas.back();
    arenas.pop_back();
    if (nextArenaSearch >= arenas.size()) {
        nextArenaSearch = 0;
    }
    retiredArenaMappings.push_back(std::make_pair(arena->memory, arena->reservationSize));
    arena->memory = nullptr;
    retiredArenas.push_back(arena);
}

void
GCHeap::unmapRetiredArenaMemory() {
    std::vector<std::pair<char*, size_t>> mappings;
    {
        GCPageHeapLockGuard lock(pageHeapMutex);
        mappings.swap(retiredArenaMappings);
    }
    for (const std::pair<char*, size_t>& mapping : mappings) {
        munmap(mapping.first, mapping.second);
    }
}

GCArena*
GCHeap::findArena(char* pointer) {
    ArenaMeta* metadata = GCAddressIndex::instance().lookupArena(pointer);
    return metadata ? metadata->arena : nullptr;
}

bool
GCHeap::verifyObject(char* pointer) {
    GCSpan* span = pointerToSpan(pointer);
    return span && span->ownsObject(pointer);
}

void*
GCHeap::objectDescriptor(char* pointer) {
    GCSpan* span = pointerToSpan(pointer);
    return span ? span->descriptor(pointer) : nullptr;
}

void
GCHeap::recordPageDemandLocked() {
    size_t pages = 0;
    for (GCArena* arena : arenas) {
        pages += arena ? arena->allocatedPageCount : 0;
    }
    size_t high = demandHighWaterPages.load(std::memory_order_relaxed);
    while (pages > high && !demandHighWaterPages.compare_exchange_weak(
            high,
            pages,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
    }
}

void
GCHeap::updatePeakDemandLocked() {
    size_t currentDemand = 0;
    for (GCArena* arena : arenas) {
        currentDemand += arena ? arena->allocatedPageCount : 0;
    }
    size_t sample = std::max(
        currentDemand,
        demandHighWaterPages.exchange(currentDemand, std::memory_order_acq_rel));
    size_t previous = peakDemandPagesEWMA.load(std::memory_order_relaxed);
    size_t next = previous ? previous - previous / 8 + sample / 8 : sample;
    peakDemandPagesEWMA.store(next, std::memory_order_release);
}

size_t
GCHeap::retentionTargetPagesLocked() const {
    size_t overrideBytes = 0;
    if (scavengeRetentionOverrideBytes(overrideBytes)) {
        return overrideBytes > std::numeric_limits<size_t>::max() - (kPageSize - 1)
            ? std::numeric_limits<size_t>::max()
            : (overrideBytes + kPageSize - 1) / kPageSize;
    }
    size_t goal = brixGCRuntimeHeapGoal();
    size_t slack = scavengeRetentionSlackPercent();
    size_t goalWithSlack = goal;
    if (slack) {
        goalWithSlack = goal <= std::numeric_limits<size_t>::max() / (100 + slack)
            ? goal * (100 + slack) / 100
            : std::numeric_limits<size_t>::max();
    }
    size_t goalPages = goalWithSlack == std::numeric_limits<size_t>::max()
        ? std::numeric_limits<size_t>::max()
        : (goalWithSlack + kPageSize - 1) / kPageSize;
    size_t peakPages = std::max(
        peakDemandPagesEWMA.load(std::memory_order_acquire),
        demandHighWaterPages.load(std::memory_order_acquire));
    return std::max(goalPages, peakPages);
}

char*
GCHeap::allocPages(size_t pageCount, bool bestFit, bool* zeroed) {
    if (!pageCount) {
        return nullptr;
    }
    GCPageHeapLockGuard lock(pageHeapMutex);
    if (pageCount > kArenaPageCapacity) {
        GCArena* arena = allocArena(pageCount);
        char* page = arena ? arena->allocPages(pageCount, false, zeroed) : nullptr;
        if (page) {
            recordPageDemandLocked();
        }
        return page;
    }
    for (GCArena* candidate : arenas) {
        if (candidate == reserveArena || candidate->dedicated) {
            continue;
        }
        char* page = candidate->allocPages(pageCount, bestFit, zeroed);
        if (page) {
            recordPageDemandLocked();
            return page;
        }
    }
    GCArena* arena = allocArena();
    if (!arena) {
        // Keep the startup reserve out of ordinary use until acquiring a new arena fails.
        // Once consumed it becomes a normal arena and is eligible for later scavenging.
        if (!reserveArena) {
            return nullptr;
        }
        char* page = reserveArena->allocPages(pageCount, bestFit, zeroed);
        if (page) {
            reserveArena = nullptr;
            recordPageDemandLocked();
        }
        return page;
    }
    char* page = arena->allocPages(pageCount, bestFit, zeroed);
    if (page) {
        recordPageDemandLocked();
    }
    return page;
}

void
GCHeap::trackSpan(GCSpan* span) {
    // The caller holds pageHeapMutex and publishes allSpans with the address index.
    span->heapSpanIndex = allSpans.size();
    allSpans.push_back(span);
}

void
GCHeap::untrackSpan(GCSpan* span) {
    // The caller holds pageHeapMutex after marker quiescence.
    size_t index = span->heapSpanIndex;
    if (index >= allSpans.size() || allSpans[index] != span) {
        abortGC("GC span not tracked");
    }
    GCSpan* moved = allSpans.back();
    allSpans[index] = moved;
    moved->heapSpanIndex = index;
    allSpans.pop_back();
    span->heapSpanIndex = kInvalidHeapSpanIndex;
}

GCSpan*
GCHeap::createSpan(SizeClass sizeClass) {
    size_t pageCount = sizeClassPageCount(sizeClass.index);
    bool zeroed = false;
    char* pages = allocPages(pageCount, false, &zeroed);
    if (!pages) {
        return nullptr;
    }
    GCArena* arena = findArena(pages);
    GCSpan* span = constructGCSpan(arena, pages, pageCount, sizeClass);
    if (!span) {
        GCPageHeapLockGuard pageHeapLock(pageHeapMutex);
        arena->freePages(pages, pageCount);
        return nullptr;
    }
    if (zeroed) {
        span->needsZero.store(false, std::memory_order_relaxed);
    }
    {
        // Page-table publication and active-span ownership change under the page-heap lock.
        // The record and all of its flat metadata are immutable/stable before this release edge.
        GCPageHeapLockGuard pageHeapLock(pageHeapMutex);
        arena->registerSpan(span);
        trackSpan(span);
    }
    brixGCRuntimeRecordSpanAllocation();
    spanStores[sizeClass.index]->registerSpan(span);
    return span;
}

GCSpan*
GCHeap::createLargeSpanUnpublished(SizeClass sizeClass) {
    size_t pageCount = largeSpanPageCount(sizeClass.size);
    bool zeroed = false;
    char* pages = allocPages(pageCount, true, &zeroed);
    if (!pages) {
        return nullptr;
    }
    GCArena* arena = findArena(pages);
    GCSpan* span = constructGCSpan(arena, pages, pageCount, sizeClass);
    if (!span) {
        {
            GCPageHeapLockGuard pageHeapLock(pageHeapMutex);
            freeArenaPagesLocked(arena, pages, pageCount);
        }
        unmapRetiredArenaMemory();
        return nullptr;
    }
    if (zeroed) {
        span->needsZero.store(false, std::memory_order_relaxed);
    }
    return span;
}

void
GCHeap::publishLargeSpan(GCSpan* span) {
    if (!span || !span->sizeClass.isLarge()) {
        abortGC("GC invalid large span publication");
    }
    if (span->isEmpty() || span->listOwner) {
        abortGC("GC invalid large span state");
    }
    GCPageHeapLockGuard pageHeapLock(pageHeapMutex);
    GCStoreLockGuard largeLock(largeSpanMutex);
    span->arena->registerSpan(span);
    trackSpan(span);
    brixGCRuntimeRecordSpanAllocation();
    largeSpans.push(span);
    try {
        span->sweepRegistryIndex = largeSweepSpans.size();
        largeSweepSpans.push_back(span);
    } catch (...) {
        span->sweepRegistryIndex = kInvalidSweepRegistryIndex;
        abortGC("GC large sweep registry allocation failed");
    }
    largeSweepSpanCount.store(largeSweepSpans.size(), std::memory_order_release);
}

void
GCHeap::releaseLargeSpanUnpublished(GCSpan* span) {
    if (!span) {
        return;
    }
    {
        GCPageHeapLockGuard pageHeapLock(pageHeapMutex);
        freeArenaPagesLocked(span->arena, span->memory, span->pageCount);
    }
    unmapRetiredArenaMemory();
    destroyGCSpan(span);
}

GCSpan*
GCHeap::findLargeSpanForAllocationLocked(SizeClass sizeClass) {
    GCSpan* best = nullptr;
    void* large = largeSpans.head;
    while (large) {
        GCSpan* span = static_cast<GCSpan*>(large);
        large = span->listNext;
        unsigned char reuse = span->reuseState();
        bool canReuse = reuse == kSpanReuseNew || reuse == kSpanReuseActive ||
            reuse == kSpanReuseReset;
        if (span->sizeClass.size == sizeClass.size && span->canAllocateNow() &&
                !span->isFull() && canReuse) {
            // Exact slot geometry is required, then highest occupancy is the best fit: it
            // consumes the least remaining capacity and packs survivors before sparse spans.
            if (!best || span->objectsUsed > best->objectsUsed) {
                best = span;
            }
        }
    }
    return best;
}

bool
GCHeap::sweepOneLargeClaimable() {
    if (publicCollectorPhase() != kPhaseSweep) {
        return false;
    }
    return sweepLargeSpansStep(1) != 0;
}

char*
GCHeap::allocObject(SizeClass sizeClass, size_t objectSize, void* descriptor) {
    if (!sizeClass.isLarge()) {
        GCSpanStore* store = spanStores[sizeClass.index];
        return store ? store->allocObject(objectSize, descriptor) : nullptr;
    }

    char* pointer = nullptr;
    {
        GCStoreLockGuard lock(largeSpanMutex);
        GCSpan* span = findLargeSpanForAllocationLocked(sizeClass);
        if (span) {
            pointer = span->allocNext(objectSize, descriptor);
            if (!pointer && !span->isFull() && span->canAllocateNow()) {
                abortGC("GC large object allocation failed");
            }
        }
    }
    if (pointer) {
        return pointer;
    }

    sweepOneLargeClaimable();
    {
        GCStoreLockGuard lock(largeSpanMutex);
        GCSpan* swept = findLargeSpanForAllocationLocked(sizeClass);
        if (swept) {
            pointer = swept->allocNext(objectSize, descriptor);
        }
    }
    if (pointer) {
        return pointer;
    }

    GCSpan* span = createLargeSpanUnpublished(sizeClass);
    if (!span) {
        return nullptr;
    }
    pointer = span->allocNext(objectSize, descriptor);
    if (!pointer) {
        releaseLargeSpanUnpublished(span);
        return nullptr;
    }
    publishLargeSpan(span);
    return pointer;
}

size_t
GCHeap::allocBatch(
    SizeClass sizeClass,
    size_t objectSize,
    void* descriptor,
    size_t preferredCount,
    char*& pointer,
    bool allocateBlack
) {
    if (!sizeClass.isLarge()) {
        GCSpanStore* store = spanStores[sizeClass.index];
        return store
            ? store->allocBatch(
                objectSize, descriptor, preferredCount, pointer, allocateBlack)
            : 0;
    }

    if (!preferredCount) {
        pointer = nullptr;
        return 0;
    }
    size_t count = 0;
    {
        GCStoreLockGuard lock(largeSpanMutex);
        GCSpan* span = findLargeSpanForAllocationLocked(sizeClass);
        if (span) {
            count = span->allocBatch(
                objectSize, descriptor, preferredCount, pointer, allocateBlack);
            if (!count && !span->isFull() && span->canAllocateNow()) {
                abortGC("GC large batch allocation failed");
            }
        }
    }
    if (count) {
        return count;
    }

    sweepOneLargeClaimable();
    {
        GCStoreLockGuard lock(largeSpanMutex);
        GCSpan* swept = findLargeSpanForAllocationLocked(sizeClass);
        if (swept) {
            count = swept->allocBatch(
                objectSize, descriptor, preferredCount, pointer, allocateBlack);
        }
    }
    if (count) {
        return count;
    }

    GCSpan* span = createLargeSpanUnpublished(sizeClass);
    if (!span) {
        pointer = nullptr;
        return 0;
    }
    count = span->allocBatch(
        objectSize, descriptor, preferredCount, pointer, allocateBlack);
    if (!count) {
        releaseLargeSpanUnpublished(span);
        pointer = nullptr;
        return 0;
    }
    publishLargeSpan(span);
    return count;
}

bool
GCHeap::releaseBatch(char* cursor, char* limit, size_t slotSize) {
    SizeClass sizeClass = sizeClassForSize(slotSize);
    if (!sizeClass.isLarge()) {
        GCSpanStore* store = spanStores[sizeClass.index];
        return store && store->releaseBatch(cursor, limit, slotSize);
    }

    GCStoreLockGuard lock(largeSpanMutex);
    GCSpan* span = pointerToSpan(cursor);
    if (!span || !span->sizeClass.isLarge()) {
        return false;
    }
    size_t released = span->releaseBatch(cursor, limit, slotSize);
    if (!released) {
        return false;
    }
    brixGCRuntimeRecordBatchRelease(released, slotSize);
    brixGCRuntimeRecordSweep(released, released * slotSize);
    return true;
}

bool
GCHeap::discardBatch(char* cursor, char* limit, size_t slotSize, size_t generation) {
    if (!cursor || !limit || !slotSize || limit <= cursor) {
        return false;
    }
    size_t byteCount = static_cast<size_t>(limit - cursor);
    if (byteCount % slotSize != 0) {
        return false;
    }
    SizeClass sizeClass = sizeClassForSize(slotSize);
    if (!sizeClass.isLarge()) {
        GCSpanStore* store = spanStores[sizeClass.index];
        return store && store->discardBatch(cursor, limit, slotSize, generation);
    }

    GCStoreLockGuard lock(largeSpanMutex);
    GCSpan* span = pointerToSpan(cursor);
    if (span && span->sizeClass.isLarge() && span->sweepGeneration() == generation) {
        size_t released = span->discardBatch(cursor, limit, slotSize);
        if (released) {
            brixGCRuntimeRecordSweep(released, released * slotSize);
        }
    }
    brixGCRuntimeRecordBatchDiscard(byteCount / slotSize, slotSize);
    return true;
}

bool
GCHeap::verifyLifecycle() {
    for (GCSpanStore* store : spanStores) {
        if (!store || !store->verifyLifecycle()) {
            return false;
        }
    }
    GCStoreLockGuard lock(largeSpanMutex);
    return largeSpans.verifyLinks();
}

void
GCHeap::freeSpan(GCSpan* span) {
    if (!span) {
        return;
    }
    if (!span->queueForRetirement()) {
        return;
    }
    // Publish the pending count before the intrusive stack link. A coordinator that observes
    // the count and starts a handshake then waits for this mutator to finish the release-CAS.
    retiredSpanQueueCount.fetch_add(1, std::memory_order_relaxed);
    GCSpan* head = retiredSpanHead.load(std::memory_order_relaxed);
    do {
        span->retirementNext = head;
    } while (!retiredSpanHead.compare_exchange_weak(
        head,
        span,
        std::memory_order_release,
        std::memory_order_relaxed));
}

void
GCHeap::unregisterRetiringSpan(GCSpan* span) {
    if (!span) {
        return;
    }
    if (!span->sizeClass.isLarge()) {
        spanStores[span->sizeClass.index]->retireSpan(span);
        return;
    }
    GCStoreLockGuard lock(largeSpanMutex);
    size_t index = span->sweepRegistryIndex;
    if (index == kInvalidSweepRegistryIndex ||
            index >= largeSweepSpans.size() || largeSweepSpans[index] != span) {
        abortGC("GC retiring large span missing from sweep registry");
    }
    size_t lastIndex = largeSweepSpans.size() - 1;
    if (index != lastIndex) {
        GCSpan* moved = largeSweepSpans.back();
        if (!moved) {
            abortGC("GC large sweep registry compaction selected a tombstone");
        }
        largeSweepSpans[index] = moved;
        moved->sweepRegistryIndex = index;
    }
    largeSweepSpans.pop_back();
    span->sweepRegistryIndex = kInvalidSweepRegistryIndex;
    size_t compactedSize = largeSweepSpans.size();
    largeSweepSpanCount.store(compactedSize, std::memory_order_release);
    // The quiescent retirement drain has cleared current-generation debt. Clamp the completed
    // traversal; a moved span skipped or revisited by a stale ordinal is generation/CAS-safe.
    largeSweepLimit.store(
        std::min(largeSweepLimit.load(std::memory_order_relaxed), compactedSize),
        std::memory_order_relaxed);
    largeSweepCursor.store(
        std::min(largeSweepCursor.load(std::memory_order_relaxed), compactedSize),
        std::memory_order_relaxed);
}

void
GCHeap::retireQueuedSpansAtSafepoint() {
    std::vector<GCArena*> arenasToDestroy;
    {
        GCPageHeapLockGuard pageHeapLock(pageHeapMutex);
        arenasToDestroy.swap(retiredArenas);
    }
    // Queue publication is a release-CAS performed by a stopped mutator or by the coordinator.
    // Stabilize the head/count pair before reclaiming so an observed count can never be left
    // behind merely because its producer had not yet completed the head CAS.
    while (retiredSpanQueueCount.load(std::memory_order_acquire) != 0) {
        GCSpan* spans = retiredSpanHead.exchange(nullptr, std::memory_order_acq_rel);
        if (!spans) {
            abortGC("GC deferred span queue publication incomplete at safepoint");
        }
        while (spans) {
            GCSpan* span = spans;
            spans = span->retirementNext;
            span->retirementNext = nullptr;
            if (!span->isEmpty() || span->listOwner) {
                abortGC("GC invalid deferred span retirement state");
            }
            // The stable sweep registry is detached under its store lock before page-heap
            // ownership changes. No store lock is ever nested beneath the page-heap lock.
            unregisterRetiringSpan(span);
            {
                GCPageHeapLockGuard pageHeapLock(pageHeapMutex);
                untrackSpan(span);
                span->arena->unregisterSpan(span);
                freeArenaPagesLocked(span->arena, span->memory, span->pageCount);
            }
            if (!span->markRetired()) {
                abortGC("GC deferred span retired more than once");
            }
            destroyGCSpan(span);
            retiredSpanQueueCount.fetch_sub(1, std::memory_order_relaxed);
            retiredSpanDestroyCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    for (GCArena* arena : arenasToDestroy) {
        delete arena;
    }
}

size_t
GCHeap::pendingRetiredSpanCount() const {
    return retiredSpanQueueCount.load(std::memory_order_acquire);
}

size_t
GCHeap::destroyedRetiredSpanCount() const {
    return retiredSpanDestroyCount.load(std::memory_order_acquire);
}

void
GCHeap::resetIncrementalSweepCursors() {
    for (GCSpanStore* store : spanStores) {
        store->resetSweepCursor();
    }
    sweepStoreIndex.store(0, std::memory_order_relaxed);
    resetLargeSweepCursor();
}

void
GCHeap::resetLargeSweepCursor() {
    size_t limit = largeSweepSpanCount.load(std::memory_order_acquire);
    largeSweepCursor.store(0, std::memory_order_relaxed);
    largeSweepLimit.store(limit, std::memory_order_relaxed);
    largeSweepCursorGen.store(currentSweepGen(), std::memory_order_release);
}

void
GCHeap::revisitSweepStore(size_t index) {
    size_t current = sweepStoreIndex.load(std::memory_order_relaxed);
    while (index < current &&
            !sweepStoreIndex.compare_exchange_weak(
                current,
                index,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
    }
}

size_t
GCHeap::sweepLargeSpansStep(size_t budget) {
    if (!budget) {
        return 0;
    }
    uint32_t heapGeneration = currentSweepGen();
    if (largeSweepCursorGen.load(std::memory_order_acquire) != heapGeneration) {
        resetLargeSweepCursor();
    }
    size_t sweptPages = 0;
    size_t limit = largeSweepLimit.load(std::memory_order_acquire);
    while (sweptPages < budget) {
        size_t index = largeSweepCursor.fetch_add(1, std::memory_order_acq_rel);
        if (index >= limit) {
            break;
        }
        GCSpan* span = nullptr;
        {
            GCStoreLockGuard lock(largeSpanMutex);
            if (index < largeSweepSpans.size()) {
                span = largeSweepSpans[index];
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
        {
            GCStoreLockGuard lock(largeSpanMutex);
            if (span->isRetirementActive()) {
                largeSpans.remove(span);
                if (span->isEmpty()) {
                    freeSpan(span);
                } else {
                    largeSpans.push(span);
                }
            }
        }
        sweptPages += pages;
    }
    return sweptPages;
}

void
GCHeap::sweepAll() {
    for (GCSpanStore* store : spanStores) {
        store->sweep();
    }
    resetLargeSweepCursor();
    while (largeSweepCursor.load(std::memory_order_acquire) <
            largeSweepLimit.load(std::memory_order_acquire)) {
        sweepLargeSpansStep(std::numeric_limits<size_t>::max());
    }
}

size_t
GCHeap::sweepIncrementalStep(size_t workLimit) {
    size_t remaining = workLimit;
    size_t inspected = 0;

    while (remaining != 0) {
        size_t storeIndex = sweepStoreIndex.load(std::memory_order_relaxed);
        if (storeIndex >= kSizeClassesCount) {
            break;
        }
        GCSpanStore* store = spanStores[storeIndex];
        size_t storeWork = store->sweepStep(remaining);
        inspected += storeWork;
        remaining = storeWork < remaining ? remaining - storeWork : 0;
        if (store->sweepComplete()) {
            size_t expected = storeIndex;
            sweepStoreIndex.compare_exchange_strong(
                expected,
                storeIndex + 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed);
        } else if (storeWork == 0) {
            remaining = 0;
        }
    }

    size_t largeBudget = std::min(workLimit, kLazySweepLargeSpanBudget);
    size_t largePages = sweepLargeSpansStep(largeBudget);
    return inspected + largePages;
}

bool
GCHeap::incrementalSweepComplete() {
    while (true) {
        size_t storeIndex = sweepStoreIndex.load(std::memory_order_relaxed);
        if (storeIndex >= kSizeClassesCount) {
            break;
        }
        GCSpanStore* store = spanStores[storeIndex];
        if (!store->sweepComplete() && store->hasSweepDebt()) {
            return false;
        }
        size_t expected = storeIndex;
        sweepStoreIndex.compare_exchange_strong(
            expected,
            storeIndex + 1,
            std::memory_order_relaxed,
            std::memory_order_relaxed);
    }
    for (GCSpanStore* store : spanStores) {
        if (store->hasSweepDebt()) {
            return false;
        }
    }
    return !hasLargeSpanSweepDebt() &&
        largeSweepCursorGen.load(std::memory_order_acquire) == currentSweepGen() &&
        largeSweepCursor.load(std::memory_order_acquire) >=
            largeSweepLimit.load(std::memory_order_acquire);
}

void
GCHeap::releaseEmptySpansForScavenge() {
    for (GCSpanStore* store : spanStores) {
        store->releaseEmptySpansForScavenge();
    }
}

size_t
GCHeap::scavengeStep(
    size_t pageBudget,
    bool releaseArenas,
    bool respectRetentionTarget
) {
    if (!pageBudget) {
        return 0;
    }
    size_t released = 0;
    size_t remaining = pageBudget;
    size_t arenaCount = 0;
    size_t start = 0;
    {
        GCPageHeapLockGuard lock(pageHeapMutex);
        size_t committed = 0;
        size_t reclaimable = 0;
        for (GCArena* arena : arenas) {
            if (!arena) {
                continue;
            }
            committed += arena->committedPageCount;
            reclaimable += arena->dirtyPageCount();
        }
        updatePeakDemandLocked();
        size_t retention = respectRetentionTarget ? retentionTargetPagesLocked() : 0;
        size_t excess = committed > retention ? committed - retention : 0;
        size_t allowed = respectRetentionTarget ? std::min(reclaimable, excess) : reclaimable;
        remaining = std::min(remaining, allowed);
        remaining -= remaining % scavengeUnitPages();
        arenaCount = arenas.size();
        start = nextArenaSearch < arenaCount ? nextArenaSearch : 0;
    }
    for (size_t step = 0; step < arenaCount && remaining != 0; ++step) {
        size_t arenaReleased = 0;
        {
            GCPageHeapLockGuard lock(pageHeapMutex);
            if (arenas.empty()) {
                break;
            }
            size_t index = (start + step) % arenas.size();
            GCArena* arena = arenas[index];
            nextArenaSearch = index;
            arenaReleased = arena ? arena->scavengeFreePages(remaining) : 0;
        }
        released += arenaReleased;
        remaining = arenaReleased < remaining ? remaining - arenaReleased : 0;
    }
    if (releaseArenas) {
        releaseEmptyArenas(kScavengeEmptyArenaReleaseBudget);
    }
    return released;
}

size_t
GCHeap::scavengeDebtPageCount(bool respectRetentionTarget) {
    GCPageHeapLockGuard lock(pageHeapMutex);
    updatePeakDemandLocked();
    size_t committed = 0;
    size_t reclaimable = 0;
    for (GCArena* arena : arenas) {
        if (!arena) {
            continue;
        }
        committed += arena->committedPageCount;
        reclaimable += arena->dirtyPageCount();
    }
    if (!respectRetentionTarget) {
        return reclaimable;
    }
    size_t retention = retentionTargetPagesLocked();
    size_t excess = committed > retention ? committed - retention : 0;
    size_t debt = std::min(reclaimable, excess);
    return debt - debt % scavengeUnitPages();
}

GCPageStats
GCHeap::pageStats() {
    GCPageHeapLockGuard lock(pageHeapMutex);
    GCPageStats stats;
    for (GCArena* arena : arenas) {
        if (!arena) {
            continue;
        }
        stats.committedPages += arena->committedPageCount;
        stats.allocatedPages += arena->allocatedPageCount;
        stats.freeCommittedPages += arena->dirtyPageCount();
        size_t run = 0;
        for (size_t page = 0; page <= arena->pageCapacity; ++page) {
            if (page < arena->pageCapacity &&
                    arena->committedBitmap.get(page) &&
                    !arena->pageBitmap.get(page) &&
                    !arena->spanForPage(page)) {
                ++run;
                continue;
            }
            if (run) {
                ++stats.freeCommittedRuns;
                stats.largestFreeCommittedRun = std::max(stats.largestFreeCommittedRun, run);
                run = 0;
            }
        }
    }
    stats.peakDemandPages = std::max(
        peakDemandPagesEWMA.load(std::memory_order_acquire),
        demandHighWaterPages.load(std::memory_order_acquire));
    stats.retentionPages = retentionTargetPagesLocked();
    size_t excess = stats.committedPages > stats.retentionPages
        ? stats.committedPages - stats.retentionPages
        : 0;
    stats.debtPages = std::min(stats.freeCommittedPages, excess);
    stats.debtPages -= stats.debtPages % scavengeUnitPages();
    return stats;
}

size_t
GCHeap::releaseEmptyArenas(size_t budget) {
    size_t released = 0;
    GCPageHeapLockGuard lock(pageHeapMutex);
    size_t index = 0;
    while (index < arenas.size() && released < budget) {
        GCArena* arena = arenas[index];
        if (arena == reserveArena) {
            ++index;
            continue;
        }
        if (arena->canReleaseAfterCycle(
                brixGCRuntimeCurrentCycleId(),
                kScavengeArenaHoldDownCycles)) {
            if (!arena->unpublishAddressIndex()) {
                abortGC("GC empty arena address-index unpublication failed");
            }
            arenas[index] = arenas.back();
            arenas.pop_back();
            // The caller holds the sweep/scavenge handshake. Destruction occurs in the second
            // quiescent drain after the release-clear has made new lookups fail.
            retiredArenas.push_back(arena);
            ++released;
        } else {
            ++index;
        }
    }
    if (arenas.empty() || nextArenaSearch >= arenas.size()) {
        nextArenaSearch = 0;
    }
    return released;
}

size_t
GCHeap::committedPageCount() {
    GCPageHeapLockGuard lock(pageHeapMutex);
    size_t count = 0;
    for (GCArena* arena : arenas) {
        count += arena->committedPageCount;
    }
    return count;
}

size_t
GCHeap::decommittedPageCount() {
    GCPageHeapLockGuard lock(pageHeapMutex);
    size_t count = 0;
    for (GCArena* arena : arenas) {
        count += arena->decommittedPageCount;
    }
    return count;
}

size_t
GCHeap::dirtyPageCount() {
    GCPageHeapLockGuard lock(pageHeapMutex);
    size_t count = 0;
    for (GCArena* arena : arenas) {
        count += arena->dirtyPageCount();
    }
    return count;
}

size_t
GCHeap::freePageCount() {
    GCPageHeapLockGuard lock(pageHeapMutex);
    size_t count = 0;
    for (GCArena* arena : arenas) {
        count += arena->pageCapacity - arena->allocatedPageCount;
    }
    return count;
}

size_t
GCHeap::reservedArenaBytes() {
    GCPageHeapLockGuard lock(pageHeapMutex);
    size_t bytes = 0;
    for (GCArena* arena : arenas) {
        bytes += arena->reservationSize;
    }
    return bytes;
}

size_t
GCHeap::largeSpanFragmentationBytes() {
    GCStoreLockGuard lock(largeSpanMutex);
    size_t bytes = 0;
    void* large = largeSpans.head;
    while (large) {
        GCSpan* span = static_cast<GCSpan*>(large);
        size_t used = span->objectsUsed * span->sizeClass.size;
        size_t capacity = span->pageCount * kPageSize;
        if (capacity > used) {
            bytes += capacity - used;
        }
        large = span->listNext;
    }
    return bytes;
}

size_t
GCHeap::sizeClassFragmentationBytes(size_t index) {
    return index < kSizeClassesCount
        ? spanStores[index]->fragmentationBytes()
        : 0;
}

size_t
GCHeap::sizeClassOccupancyBucketCount(size_t index, size_t bucket) {
    return index < kSizeClassesCount
        ? spanStores[index]->occupancyBucketCount(bucket)
        : 0;
}

size_t
GCHeap::fragmentationBytes() {
    size_t bytes = largeSpanFragmentationBytes();
    for (size_t index = 0; index < kSizeClassesCount; ++index) {
        bytes += sizeClassFragmentationBytes(index);
    }
    return bytes;
}

size_t
GCHeap::sizeClassSweepDebt(size_t index) {
    if (index >= kSizeClassesCount) {
        return 0;
    }
    return spanStores[index]->sweepDebt();
}

size_t
GCHeap::largeSpanSweepDebt() {
    GCStoreLockGuard lock(largeSpanMutex);
    size_t debtPages = 0;
    for (GCSpan* span : largeSweepSpans) {
        if (span && span->isRetirementActive() && span->needsSweepNow()) {
            debtPages += span->pageCount;
        }
    }
    return debtPages;
}

bool
GCHeap::hasLargeSpanSweepDebt() {
    GCStoreLockGuard lock(largeSpanMutex);
    for (GCSpan* span : largeSweepSpans) {
        if (span && span->isRetirementActive() && span->needsSweepNow()) {
            return true;
        }
    }
    return false;
}

bool
GCHeap::hasSweepDebt() {
    size_t start = sweepStoreIndex.load(std::memory_order_relaxed);
    if (start >= kSizeClassesCount) {
        start = 0;
    }
    for (size_t offset = 0; offset < kSizeClassesCount; ++offset) {
        size_t index = start + offset;
        if (index >= kSizeClassesCount) {
            index -= kSizeClassesCount;
        }
        if (spanStores[index]->hasSweepDebt()) {
            return true;
        }
    }
    return hasLargeSpanSweepDebt();
}

} // namespace BxGC
