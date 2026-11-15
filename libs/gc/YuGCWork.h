#ifndef YU_GC_WORK_H
#define YU_GC_WORK_H

#include "YuGCCommon.h"

namespace YuGC {

static_assert(__atomic_always_lock_free(sizeof(char*), nullptr),
    "GC mark-work pointer publication must be lock-free");
static_assert(__atomic_always_lock_free(sizeof(size_t), nullptr),
    "GC mark-work accounting must be lock-free");

static constexpr size_t kGCMarkWorkBufferSlots = 256;

// A WorkBuf has one owner while local and is transferred through the full/empty lists under
// the queue spinlock. Pointer entries are therefore plain: the lock is the publication edge.
struct GCMarkWorkBuffer {
    char* slots[kGCMarkWorkBufferSlots];
    GCMarkWorkBuffer* next;
    size_t begin;
    size_t end;

    GCMarkWorkBuffer();

    bool empty() const { return begin >= end; }
    bool full() const { return end == kGCMarkWorkBufferSlots; }
    size_t size() const { return end - begin; }
    void reset();
};

static_assert(kGCMarkWorkBufferSlots == 256,
    "GC WorkBuf capacity is part of the Stage-3 queue contract");

// Each marking participant owns a push/pop pair. The record is process-lifetime and is
// referenced by the owning thread cache only at participant boundaries; queue storage never
// moves while a marker can address it.
struct GCMarkParticipant {
    GCMarkWorkBuffer* pushBuffer;
    GCMarkWorkBuffer* popBuffer;
    ptrdiff_t pendingOutstanding;
    size_t pendingPushes;

    GCMarkParticipant()
        : pushBuffer(nullptr), popBuffer(nullptr), pendingOutstanding(0), pendingPushes(0) {}
};

class GCMarkWork {
public:
    GCMarkWork() = default;
    ~GCMarkWork();
    GCMarkWork(const GCMarkWork&) = delete;
    GCMarkWork& operator=(const GCMarkWork&) = delete;

    // Span construction grows a persistent high-water WorkBuf pool outside the mark edge.
    bool registerSpanCapacity(size_t slots);
    void unregisterSpanCapacity(size_t slots);
    size_t trimSurplusBuffers();

    // Called during mark setup after the previous queue and every participant are quiescent.
    // The reset is O(1); drained buffers are already on the empty list.
    void reset();

    // Marker participation is nestable on one OS thread. Outermost leave force-publishes both
    // local buffers before the seq_cst active-marker decrement.
    bool enterParticipant(YumiGCThreadCache* cache);
    void leaveParticipant();
    void flushCurrentParticipant();
    void flushAllParticipants();

    // Brackets address resolution, the mark RMW, and WorkBuf publication. Termination closes
    // this packed gate at count zero and then checks participant/global quiescence.
    bool tryBeginPublisher();
    void endPublisher();
    bool tryClosePublishers();
    void reopenPublishersForDrain();

    // The ordinary marking edge appends to the current participant's local buffer. A caller
    // without a participant uses the globally visible direct buffer (cold boundary path).
    void publish(char* object);
    char* tryPop();

    bool empty() const;
    bool globalEmpty() const;
    bool participantLocalsEmpty() const;
    bool quiescent() const;
    size_t depth() const;
    size_t activePublisherCount() const;
    bool publishersClosed() const;
    size_t activeMarkerCount() const;
    size_t maxActiveMarkerCount() const;
    uint64_t workFlushedEpoch() const;
    size_t participantCount() const;

private:
    mutable std::atomic_flag queueLock = ATOMIC_FLAG_INIT;
    GCMarkWorkBuffer* fullHead = nullptr;
    GCMarkWorkBuffer* fullTail = nullptr;
    GCMarkWorkBuffer* emptyHead = nullptr;
    GCMarkWorkBuffer* directBuffer = nullptr;
    size_t globalPointerCount = 0;

    mutable std::mutex provisionMutex;
    std::vector<GCMarkWorkBuffer*> ownedBuffers;
    std::vector<GCMarkParticipant*> ownedParticipants;
    std::unordered_map<YumiGCThreadCache*, GCMarkParticipant*> cacheParticipants;
    size_t activeSlotCapacity = 0;
    size_t retainedQueueBytes = 0;

    std::atomic<size_t> outstanding {0};
    static constexpr size_t kAdmissionClosed = size_t(1) << (sizeof(size_t) * 8 - 1);
    static constexpr size_t kAdmissionCountMask = kAdmissionClosed - 1;
    std::atomic<size_t> participantState {kAdmissionClosed};
    std::atomic<size_t> maxActiveMarkers {0};
    std::atomic<uint64_t> workFlushed {0};

    std::atomic<size_t> publisherState {kAdmissionClosed};

    void lockQueue() const;
    void unlockQueue() const;
    size_t requiredBufferCountLocked() const;
    bool provisionRequiredBuffersLocked();
    bool provisionOneBufferLocked();
    GCMarkParticipant* participantFor(YumiGCThreadCache* cache);
    void syncParticipantHandles(YumiGCThreadCache* cache, GCMarkParticipant* participant);

    GCMarkWorkBuffer* takeEmptyLocked();
    void returnEmptyLocked(GCMarkWorkBuffer* buffer);
    void enqueueFullLocked(GCMarkWorkBuffer* buffer);
    void publishFullLocked(GCMarkWorkBuffer* buffer);
    GCMarkWorkBuffer* takeFullLocked();
    void ensureParticipantPairLocked(GCMarkParticipant* participant);
    void donateParticipantBufferLocked(GCMarkWorkBuffer*& buffer);
    void flushParticipantLocked(GCMarkParticipant* participant);
    void foldParticipantCounts(GCMarkParticipant* participant);
};

GCMarkWork& markWork();

} // namespace YuGC

#endif
