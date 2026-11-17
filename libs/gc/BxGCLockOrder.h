#ifndef BX_GC_LOCK_ORDER_H
#define BX_GC_LOCK_ORDER_H

#include <mutex>

// Debug builds opt in automatically. An explicit definition lets focused tests enable or
// disable the checks independently of the rest of the runtime's debug configuration.
#ifndef BX_GC_DEBUG_LOCK_ORDER
#if defined(DEBUG) || defined(_DEBUG)
#define BX_GC_DEBUG_LOCK_ORDER 1
#else
#define BX_GC_DEBUG_LOCK_ORDER 0
#endif
#endif

namespace BxGC {

// Ranked GC locks must be acquired from left to right. A rank can contain multiple distinct
// locks, but acquiring the same ranked lock recursively is always an error.
// gCollectorMutex and the v1 global pcache mutex remain transitional in Stage 1: this seam
// ranks the page heap, arena ownership, span registries/stores, and large-span store without
// changing the allocation-cache protocol that the next stage replaces.
enum class GCLockRank : unsigned char {
    Scheduler = 1,
    PageHeap = 2,
    Store = 3,
    GCWork = 4,
};

#if BX_GC_DEBUG_LOCK_ORDER

void enterGCLockOrder(GCLockRank rank, const void* lockAddress);
void leaveGCLockOrder(GCLockRank rank, const void* lockAddress);

struct GCLockOrderScope {
    GCLockRank rank;
    const void* lockAddress;

    GCLockOrderScope(GCLockRank rank, const void* lockAddress)
        : rank(rank), lockAddress(lockAddress) {
        enterGCLockOrder(rank, lockAddress);
    }

    ~GCLockOrderScope() {
        leaveGCLockOrder(rank, lockAddress);
    }

    GCLockOrderScope(const GCLockOrderScope&) = delete;
    GCLockOrderScope& operator=(const GCLockOrderScope&) = delete;
};

#else

struct GCLockOrderScope {
    GCLockOrderScope(GCLockRank, const void*) {}
};

#endif

// The order scope is constructed before the mutex guard and destroyed after it, so debug builds
// validate the requested acquisition before blocking and keep the rank active until after unlock.
template<GCLockRank Rank, typename Mutex>
struct GCLockGuard {
    GCLockOrderScope orderScope;
    std::lock_guard<Mutex> mutexGuard;

    explicit GCLockGuard(Mutex& mutex)
        : orderScope(Rank, &mutex), mutexGuard(mutex) {}

    GCLockGuard(const GCLockGuard&) = delete;
    GCLockGuard& operator=(const GCLockGuard&) = delete;
};

template<GCLockRank Rank, typename Mutex>
struct GCLockPair {
    Mutex* first = nullptr;
    Mutex* second = nullptr;

    GCLockPair() {}

    ~GCLockPair() {
        unlock();
    }

    void lock(Mutex& mutex) {
        acquire(mutex);
        first = &mutex;
    }

    void lock(Mutex& lower, Mutex& upper) {
        acquire(lower);
        first = &lower;
        acquire(upper);
        second = &upper;
    }

    void unlock() {
        release(second);
        release(first);
    }

    GCLockPair(const GCLockPair&) = delete;
    GCLockPair& operator=(const GCLockPair&) = delete;

private:
    static void acquire(Mutex& mutex) {
#if BX_GC_DEBUG_LOCK_ORDER
        enterGCLockOrder(Rank, &mutex);
#endif
        mutex.lock();
    }

    static void release(Mutex*& mutex) {
        if (!mutex) {
            return;
        }
        Mutex* held = mutex;
        mutex = nullptr;
        held->unlock();
#if BX_GC_DEBUG_LOCK_ORDER
        leaveGCLockOrder(Rank, held);
#endif
    }
};

typedef GCLockGuard<GCLockRank::Scheduler, std::mutex> GCSchedulerLockGuard;
typedef GCLockGuard<GCLockRank::PageHeap, std::mutex> GCPageHeapLockGuard;
typedef GCLockGuard<GCLockRank::Store, std::mutex> GCStoreLockGuard;
typedef GCLockGuard<GCLockRank::GCWork, std::mutex> GCGCWorkLockGuard;
typedef GCLockPair<GCLockRank::Store, std::mutex> GCStoreLockPair;

} // namespace BxGC

#endif
