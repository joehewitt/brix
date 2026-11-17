#include "BxGCWork.h"

#include <thread>

namespace BxGC {

static std::atomic<size_t> gRetainedMarkWorkQueueBytes {0};
static thread_local GCMarkParticipant* gCurrentMarkParticipant = nullptr;
static thread_local BrixGCThreadCache* gCurrentMarkParticipantCache = nullptr;
static thread_local GCMarkWorkBuffer* gFallbackMarkPopBuffer = nullptr;
static thread_local bool gFallbackMarkParticipant = false;
static thread_local size_t gMarkParticipantDepth = 0;

GCMarkWork&
markWork() {
    static GCMarkWork instance;
    return instance;
}

extern "C" size_t
brixGCTestMarkWorkBufferSlots(void) {
    return kGCMarkWorkBufferSlots;
}

extern "C" size_t
brixGCRuntimeMarkWorkRetainedBytes(void) {
    return gRetainedMarkWorkQueueBytes.load(std::memory_order_acquire);
}

extern "C" size_t
brixGCTestMarkWorkRetainedBytes(void) {
    return brixGCRuntimeMarkWorkRetainedBytes();
}

extern "C" size_t
brixGCTestMarkWorkParticipantCount(void) {
    return markWork().participantCount();
}

extern "C" size_t
brixGCTestMarkWorkActiveMarkers(void) {
    return markWork().activeMarkerCount();
}

extern "C" uint64_t
brixGCTestMarkWorkFlushedEpoch(void) {
    return markWork().workFlushedEpoch();
}

extern "C" size_t
brixGCTestMarkWorkMaxActiveMarkers(void) {
    return markWork().maxActiveMarkerCount();
}

GCMarkWorkBuffer::GCMarkWorkBuffer() : next(nullptr), begin(0), end(0) {
    std::memset(slots, 0, sizeof(slots));
}

void
GCMarkWorkBuffer::reset() {
    if (!empty()) {
        abortGC("GC returned nonempty WorkBuf to empty list");
    }
    next = nullptr;
    begin = 0;
    end = 0;
}

GCMarkWork::~GCMarkWork() {
    for (GCMarkParticipant* participant : ownedParticipants) {
        delete participant;
    }
    for (GCMarkWorkBuffer* buffer : ownedBuffers) {
        delete buffer;
    }
}

void
GCMarkWork::lockQueue() const {
    while (queueLock.test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
}

void
GCMarkWork::unlockQueue() const {
    queueLock.clear(std::memory_order_release);
}

GCMarkWorkBuffer*
GCMarkWork::takeEmptyLocked() {
    GCMarkWorkBuffer* buffer = emptyHead;
    if (!buffer) {
        abortGC("GC WorkBuf empty-list capacity exhausted");
    }
    emptyHead = buffer->next;
    buffer->next = nullptr;
    if (!buffer->empty()) {
        abortGC("GC WorkBuf empty list contained work");
    }
    return buffer;
}

void
GCMarkWork::returnEmptyLocked(GCMarkWorkBuffer* buffer) {
    if (!buffer) {
        return;
    }
    buffer->reset();
    buffer->next = emptyHead;
    emptyHead = buffer;
}

void
GCMarkWork::enqueueFullLocked(GCMarkWorkBuffer* buffer) {
    if (!buffer || buffer->empty()) {
        abortGC("GC attempted to publish an empty WorkBuf");
    }
    buffer->next = nullptr;
    if (fullTail) {
        fullTail->next = buffer;
    } else {
        fullHead = buffer;
    }
    fullTail = buffer;
}

void
GCMarkWork::publishFullLocked(GCMarkWorkBuffer* buffer) {
    enqueueFullLocked(buffer);
    globalPointerCount += buffer->size();
    workFlushed.fetch_add(1, std::memory_order_seq_cst);
}

GCMarkWorkBuffer*
GCMarkWork::takeFullLocked() {
    GCMarkWorkBuffer* buffer = fullHead;
    if (buffer) {
        fullHead = buffer->next;
        if (!fullHead) {
            fullTail = nullptr;
        }
        buffer->next = nullptr;
    } else if (directBuffer && !directBuffer->empty()) {
        buffer = directBuffer;
        directBuffer = nullptr;
    }
    if (buffer) {
        if (buffer->size() > globalPointerCount) {
            abortGC("GC global WorkBuf accounting underflow");
        }
        globalPointerCount -= buffer->size();
    }
    return buffer;
}

bool
GCMarkWork::provisionOneBufferLocked() {
    GCMarkWorkBuffer* fresh = new (std::nothrow) GCMarkWorkBuffer();
    if (!fresh) {
        std::fprintf(stderr, "Brix GC WorkBuf reserve allocation failed\n");
        return false;
    }
    try {
        ownedBuffers.push_back(fresh);
    } catch (...) {
        delete fresh;
        std::fprintf(stderr, "Brix GC WorkBuf reserve bookkeeping allocation failed\n");
        return false;
    }
    retainedQueueBytes += sizeof(GCMarkWorkBuffer);
    lockQueue();
    returnEmptyLocked(fresh);
    unlockQueue();
    return true;
}

size_t
GCMarkWork::requiredBufferCountLocked() const {
    size_t slotBuffers = activeSlotCapacity / kGCMarkWorkBufferSlots +
        (activeSlotCapacity % kGCMarkWorkBufferSlots != 0 ? 1 : 0);
    if (ownedParticipants.size() >
            (std::numeric_limits<size_t>::max() - slotBuffers - 1) / 2) {
        return std::numeric_limits<size_t>::max();
    }
    return slotBuffers + ownedParticipants.size() * 2 + 1;
}

bool
GCMarkWork::provisionRequiredBuffersLocked() {
    size_t required = requiredBufferCountLocked();
    if (required == std::numeric_limits<size_t>::max()) {
        std::fprintf(stderr, "Brix GC WorkBuf participant reserve overflow\n");
        return false;
    }
    while (ownedBuffers.size() < required) {
        if (!provisionOneBufferLocked()) {
            return false;
        }
    }
    gRetainedMarkWorkQueueBytes.store(retainedQueueBytes, std::memory_order_release);
    return true;
}

bool
GCMarkWork::registerSpanCapacity(size_t slots) {
    if (!slots) {
        return true;
    }
    std::lock_guard<std::mutex> lock(provisionMutex);
    if (slots > std::numeric_limits<size_t>::max() - activeSlotCapacity) {
        std::fprintf(stderr, "Brix GC mark-work active capacity overflow\n");
        return false;
    }
    size_t previousCapacity = activeSlotCapacity;
    activeSlotCapacity += slots;
    if (!provisionRequiredBuffersLocked()) {
        activeSlotCapacity = previousCapacity;
        return false;
    }
    return true;
}

size_t
GCMarkWork::trimSurplusBuffers() {
    std::lock_guard<std::mutex> lock(provisionMutex);
    size_t required = requiredBufferCountLocked();
    if (ownedBuffers.size() <= required) {
        return 0;
    }
    size_t surplus = ownedBuffers.size() - required;
    std::vector<GCMarkWorkBuffer*> released;
    try {
        released.reserve(surplus);
    } catch (...) {
        return 0;
    }
    lockQueue();
    while (released.size() < surplus && emptyHead) {
        released.push_back(takeEmptyLocked());
    }
    unlockQueue();
    std::sort(released.begin(), released.end());
    ownedBuffers.erase(
        std::remove_if(
            ownedBuffers.begin(),
            ownedBuffers.end(),
            [&](GCMarkWorkBuffer* buffer) {
                return std::binary_search(released.begin(), released.end(), buffer);
            }),
        ownedBuffers.end());
    for (GCMarkWorkBuffer* buffer : released) {
        delete buffer;
    }
    retainedQueueBytes -= released.size() * sizeof(GCMarkWorkBuffer);
    gRetainedMarkWorkQueueBytes.store(retainedQueueBytes, std::memory_order_release);
    return released.size() * sizeof(GCMarkWorkBuffer);
}

void
GCMarkWork::unregisterSpanCapacity(size_t slots) {
    if (!slots) {
        return;
    }
    std::lock_guard<std::mutex> lock(provisionMutex);
    if (slots > activeSlotCapacity) {
        abortGC("GC mark-work active capacity underflow");
    }
    activeSlotCapacity -= slots;
}

void
GCMarkWork::ensureParticipantPairLocked(GCMarkParticipant* participant) {
    if (!participant->pushBuffer) {
        participant->pushBuffer = takeEmptyLocked();
    }
    if (!participant->popBuffer) {
        participant->popBuffer = takeEmptyLocked();
    }
}

GCMarkParticipant*
GCMarkWork::participantFor(BrixGCThreadCache* cache) {
    std::lock_guard<std::mutex> lock(provisionMutex);
    auto found = cacheParticipants.find(cache);
    if (found != cacheParticipants.end()) {
        return found->second;
    }

    GCMarkParticipant* participant = new (std::nothrow) GCMarkParticipant();
    if (!participant) {
        std::fprintf(
            stderr,
            "Brix GC mark participant allocation failed; using shared queue fallback\n");
        return nullptr;
    }
    try {
        ownedParticipants.push_back(participant);
        cacheParticipants.emplace(cache, participant);
    } catch (...) {
        if (!ownedParticipants.empty() && ownedParticipants.back() == participant) {
            ownedParticipants.pop_back();
        }
        cacheParticipants.erase(cache);
        delete participant;
        std::fprintf(
            stderr,
            "Brix GC mark participant bookkeeping failed; using shared queue fallback\n");
        return nullptr;
    }
    if (!provisionRequiredBuffersLocked()) {
        cacheParticipants.erase(cache);
        ownedParticipants.pop_back();
        delete participant;
        std::fprintf(
            stderr,
            "Brix GC mark participant reserve provisioning failed; "
            "using shared queue fallback\n");
        return nullptr;
    }
    retainedQueueBytes += sizeof(GCMarkParticipant);
    lockQueue();
    ensureParticipantPairLocked(participant);
    unlockQueue();
    gRetainedMarkWorkQueueBytes.store(retainedQueueBytes, std::memory_order_release);
    return participant;
}

void
GCMarkWork::syncParticipantHandles(
    BrixGCThreadCache* cache,
    GCMarkParticipant* participant
) {
    if (!cache || !participant) {
        return;
    }
    cache->markWorkHandles[0] = participant->pushBuffer;
    cache->markWorkHandles[1] = participant->popBuffer;
}

bool
GCMarkWork::enterParticipant(BrixGCThreadCache* cache) {
    if (gMarkParticipantDepth != 0) {
        ++gMarkParticipantDepth;
        return true;
    }
    size_t state = participantState.load(std::memory_order_acquire);
    while (!(state & kAdmissionClosed)) {
        if ((state & kAdmissionCountMask) == kAdmissionCountMask) {
            abortGC("GC mark participant count overflow");
        }
        if (participantState.compare_exchange_weak(
                state,
                state + 1,
                std::memory_order_seq_cst,
                std::memory_order_acquire)) {
            break;
        }
    }
    if (state & kAdmissionClosed) {
        return false;
    }
    GCMarkParticipant* participant = participantFor(cache);
    gCurrentMarkParticipant = participant;
    gCurrentMarkParticipantCache = cache;
    gFallbackMarkParticipant = participant == nullptr;
    gFallbackMarkPopBuffer = nullptr;
    if (!participant && cache) {
        cache->markWorkHandles[0] = nullptr;
        cache->markWorkHandles[1] = nullptr;
    }
    gMarkParticipantDepth = 1;
    size_t active = state + 1;
    size_t maximum = maxActiveMarkers.load(std::memory_order_relaxed);
    while (active > maximum && !maxActiveMarkers.compare_exchange_weak(
            maximum,
            active,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
    }
    syncParticipantHandles(cache, participant);
    return true;
}

void
GCMarkWork::donateParticipantBufferLocked(GCMarkWorkBuffer*& buffer) {
    if (!buffer) {
        return;
    }
    if (buffer->empty()) {
        returnEmptyLocked(buffer);
        buffer = nullptr;
        return;
    }
    if (buffer->full()) {
        publishFullLocked(buffer);
        buffer = nullptr;
        return;
    }

    // Boundary flushes may donate only one or two pointers at a time. Publishing each
    // partial WorkBuf separately would consume the fixed high-water pool by fragmentation,
    // even though the total pointer bound still fits. Pack partial donations into the single
    // globally visible direct buffer; the queue lock is the transfer/publication edge.
    bool movedWork = false;
    while (!buffer->empty()) {
        if (!directBuffer) {
            directBuffer = takeEmptyLocked();
        }
        size_t available = kGCMarkWorkBufferSlots - directBuffer->end;
        size_t count = std::min(available, buffer->size());
        for (size_t index = 0; index < count; ++index) {
            directBuffer->slots[directBuffer->end + index] =
                buffer->slots[buffer->begin + index];
            buffer->slots[buffer->begin + index] = nullptr;
        }
        directBuffer->end += count;
        buffer->begin += count;
        globalPointerCount += count;
        movedWork = true;
        if (directBuffer->full()) {
            GCMarkWorkBuffer* full = directBuffer;
            directBuffer = nullptr;
            enqueueFullLocked(full);
        }
    }
    returnEmptyLocked(buffer);
    buffer = nullptr;
    if (movedWork) {
        workFlushed.fetch_add(1, std::memory_order_seq_cst);
    }
}

void
GCMarkWork::foldParticipantCounts(GCMarkParticipant* participant) {
    ptrdiff_t delta = participant->pendingOutstanding;
    size_t pushes = participant->pendingPushes;
    if (!delta && !pushes) {
        return;
    }
    participant->pendingOutstanding = 0;
    participant->pendingPushes = 0;
    size_t change = static_cast<size_t>(delta);
    size_t depth = outstanding.fetch_add(change, std::memory_order_acq_rel) + change;
    if (static_cast<ptrdiff_t>(depth) < 0) {
        abortGC("GC mark-work depth underflow");
    }
    if (pushes) {
        brixGCRuntimeRecordMarkQueuePushes(pushes, depth);
    }
}

void
GCMarkWork::flushParticipantLocked(GCMarkParticipant* participant) {
    if (!participant) {
        return;
    }
    foldParticipantCounts(participant);
    donateParticipantBufferLocked(participant->pushBuffer);
    donateParticipantBufferLocked(participant->popBuffer);
    ensureParticipantPairLocked(participant);
}

void
GCMarkWork::flushCurrentParticipant() {
    if (!gCurrentMarkParticipant && !gFallbackMarkParticipant) {
        return;
    }
    lockQueue();
    if (gCurrentMarkParticipant) {
        flushParticipantLocked(gCurrentMarkParticipant);
    } else if (gFallbackMarkPopBuffer) {
        if (gFallbackMarkPopBuffer->empty()) {
            returnEmptyLocked(gFallbackMarkPopBuffer);
        } else {
            publishFullLocked(gFallbackMarkPopBuffer);
        }
        gFallbackMarkPopBuffer = nullptr;
    }
    unlockQueue();
    syncParticipantHandles(gCurrentMarkParticipantCache, gCurrentMarkParticipant);
}

void
GCMarkWork::leaveParticipant() {
    if (gMarkParticipantDepth == 0 ||
            (!gCurrentMarkParticipant && !gFallbackMarkParticipant)) {
        abortGC("GC mark participant leave without enter");
    }
    if (--gMarkParticipantDepth != 0) {
        return;
    }
    flushCurrentParticipant();
    size_t before = participantState.fetch_sub(1, std::memory_order_seq_cst);
    if ((before & kAdmissionCountMask) == 0) {
        abortGC("GC active marker accounting underflow");
    }
    gCurrentMarkParticipant = nullptr;
    gCurrentMarkParticipantCache = nullptr;
    gFallbackMarkParticipant = false;
    gFallbackMarkPopBuffer = nullptr;
}

void
GCMarkWork::flushAllParticipants() {
    size_t expected = 0;
    while (!participantState.compare_exchange_weak(
            expected,
            kAdmissionClosed,
            std::memory_order_seq_cst,
            std::memory_order_acquire)) {
        expected = 0;
        std::this_thread::yield();
    }
    std::lock_guard<std::mutex> lock(provisionMutex);
    lockQueue();
    for (GCMarkParticipant* participant : ownedParticipants) {
        flushParticipantLocked(participant);
    }
    unlockQueue();
    participantState.store(0, std::memory_order_release);
}

void
GCMarkWork::reset() {
    if (!publishersClosed() || activePublisherCount() != 0 || !quiescent()) {
        abortGC("GC mark work reset while work remains");
    }
    publisherState.store(0, std::memory_order_release);
    participantState.store(0, std::memory_order_release);
}

bool
GCMarkWork::tryBeginPublisher() {
    size_t state = publisherState.load(std::memory_order_acquire);
    while (!(state & kAdmissionClosed)) {
        if ((state & kAdmissionCountMask) == kAdmissionCountMask) {
            abortGC("GC mark-work publisher count overflow");
        }
        if (publisherState.compare_exchange_weak(
                state,
                state + 1,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return true;
        }
    }
    return false;
}

void
GCMarkWork::endPublisher() {
    size_t before = publisherState.fetch_sub(1, std::memory_order_acq_rel);
    if ((before & kAdmissionCountMask) == 0) {
        abortGC("GC mark-work publisher accounting underflow");
    }
}

bool
GCMarkWork::tryClosePublishers() {
    size_t participantExpected = 0;
    if (!participantState.compare_exchange_strong(
            participantExpected,
            kAdmissionClosed,
            std::memory_order_seq_cst,
            std::memory_order_acquire)) {
        return false;
    }
    size_t expected = 0;
    if (publisherState.compare_exchange_strong(
            expected,
            kAdmissionClosed,
            std::memory_order_acq_rel,
            std::memory_order_acquire)) {
        return true;
    }
    participantState.store(0, std::memory_order_release);
    return false;
}

void
GCMarkWork::reopenPublishersForDrain() {
    size_t expected = kAdmissionClosed;
    if (!publisherState.compare_exchange_strong(
            expected,
            0,
            std::memory_order_release,
            std::memory_order_acquire)) {
        abortGC("GC mark publisher gate changed while closed");
    }
    expected = kAdmissionClosed;
    if (!participantState.compare_exchange_strong(
            expected,
            0,
            std::memory_order_release,
            std::memory_order_acquire)) {
        abortGC("GC mark participant gate changed while closed");
    }
}

void
GCMarkWork::publish(char* object) {
    if (!object) {
        abortGC("GC null mark-work publication");
    }
    GCMarkParticipant* participant = gCurrentMarkParticipant;
    if (participant) {
        GCMarkWorkBuffer* buffer = participant->pushBuffer;
        if (!buffer || buffer->full()) {
            abortGC("GC mark participant missing writable WorkBuf");
        }
        buffer->slots[buffer->end++] = object;
        ++participant->pendingOutstanding;
        ++participant->pendingPushes;
        if (buffer->full()) {
            foldParticipantCounts(participant);
            lockQueue();
            publishFullLocked(buffer);
            participant->pushBuffer = nullptr;
            ensureParticipantPairLocked(participant);
            unlockQueue();
            syncParticipantHandles(gCurrentMarkParticipantCache, participant);
        }
        return;
    }

    size_t depth = outstanding.fetch_add(1, std::memory_order_relaxed) + 1;
    if (depth == 0) {
        abortGC("GC mark-work depth overflow");
    }
    lockQueue();
    if (!directBuffer) {
        directBuffer = takeEmptyLocked();
    }
    directBuffer->slots[directBuffer->end++] = object;
    ++globalPointerCount;
    workFlushed.fetch_add(1, std::memory_order_seq_cst);
    if (directBuffer->full()) {
        GCMarkWorkBuffer* full = directBuffer;
        directBuffer = nullptr;
        enqueueFullLocked(full);
    }
    unlockQueue();
    brixGCRuntimeRecordMarkQueuePushes(1, depth);
}

char*
GCMarkWork::tryPop() {
    GCMarkParticipant* participant = gCurrentMarkParticipant;
    if (!participant && !gFallbackMarkParticipant) {
        abortGC("GC mark-work pop requires participant");
    }

    GCMarkWorkBuffer* buffer = participant
        ? participant->popBuffer
        : gFallbackMarkPopBuffer;
    if (!buffer || buffer->empty()) {
        if (participant) {
            foldParticipantCounts(participant);
        }
        lockQueue();
        if (buffer) {
            returnEmptyLocked(buffer);
            if (participant) {
                participant->popBuffer = nullptr;
            } else {
                gFallbackMarkPopBuffer = nullptr;
            }
        }
        if (!participant) {
            gFallbackMarkPopBuffer = takeFullLocked();
            buffer = gFallbackMarkPopBuffer;
        } else {
            // Keep newly discovered children local when possible. The other half of the pair
            // becomes the pop buffer and a fresh empty buffer replaces it for subsequent pushes.
            if (participant->pushBuffer && !participant->pushBuffer->empty()) {
                participant->popBuffer = participant->pushBuffer;
                participant->pushBuffer = takeEmptyLocked();
            } else {
                participant->popBuffer = takeFullLocked();
            }
            if (!participant->popBuffer) {
                participant->popBuffer = takeEmptyLocked();
            }
            buffer = participant->popBuffer;
        }
        unlockQueue();
        syncParticipantHandles(gCurrentMarkParticipantCache, participant);
    }
    if (!buffer || buffer->empty()) {
        return nullptr;
    }

    char* object = buffer->slots[buffer->begin];
    buffer->slots[buffer->begin++] = nullptr;
    if (!object) {
        abortGC("GC WorkBuf contained null work");
    }
    if (participant) {
        --participant->pendingOutstanding;
        return object;
    }
    size_t before = outstanding.fetch_sub(1, std::memory_order_acq_rel);
    if (before == 0) {
        abortGC("GC mark-work depth underflow");
    }
    return object;
}

bool
GCMarkWork::globalEmpty() const {
    lockQueue();
    bool result = globalPointerCount == 0;
    unlockQueue();
    return result;
}

bool
GCMarkWork::participantLocalsEmpty() const {
    if (activeMarkerCount() != 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(provisionMutex);
    lockQueue();
    bool result = true;
    for (GCMarkParticipant* participant : ownedParticipants) {
        if ((participant->pushBuffer && !participant->pushBuffer->empty()) ||
                (participant->popBuffer && !participant->popBuffer->empty())) {
            result = false;
            break;
        }
    }
    unlockQueue();
    return result;
}

bool
GCMarkWork::empty() const {
    return outstanding.load(std::memory_order_acquire) == 0 &&
        activePublisherCount() == 0;
}

bool
GCMarkWork::quiescent() const {
    return empty() && activeMarkerCount() == 0 && globalEmpty() &&
        participantLocalsEmpty();
}

size_t
GCMarkWork::depth() const {
    return outstanding.load(std::memory_order_acquire);
}

size_t
GCMarkWork::activePublisherCount() const {
    return publisherState.load(std::memory_order_acquire) & kAdmissionCountMask;
}

bool
GCMarkWork::publishersClosed() const {
    return (publisherState.load(std::memory_order_acquire) & kAdmissionClosed) != 0;
}

size_t
GCMarkWork::activeMarkerCount() const {
    return participantState.load(std::memory_order_seq_cst) & kAdmissionCountMask;
}

size_t
GCMarkWork::maxActiveMarkerCount() const {
    return maxActiveMarkers.load(std::memory_order_relaxed);
}

uint64_t
GCMarkWork::workFlushedEpoch() const {
    return workFlushed.load(std::memory_order_seq_cst);
}

size_t
GCMarkWork::participantCount() const {
    std::lock_guard<std::mutex> lock(provisionMutex);
    return ownedParticipants.size();
}

} // namespace BxGC
