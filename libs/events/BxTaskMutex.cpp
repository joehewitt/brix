#include <BxEvents/BxTaskMutex.h>

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxTask.h>
#include <BxEvents/BxTaskQueue.h>
#include <BxEvents/BxThread.h>

#include <sched.h>

// *************************************************************************************************

static const uint32_t kBxTaskMutexUnlocked = 0;
static const uint32_t kBxTaskMutexLocked = 1;
static const uint32_t kBxTaskMutexContended = 2;
static const int kBxTaskMutexSpinCount = 64;

struct BxTaskMutexWaiter {
    BxTask* task;
    BxTaskMutexWaiter* next;
    bool queued;
};

// *************************************************************************************************

BxTaskMutex::BxTaskMutex() :
    state(kBxTaskMutexUnlocked),
    firstWaiter(NULL),
    lastWaiter(NULL)
{
}

bool
BxTaskMutex::tryLock() {
    uint32_t expected = kBxTaskMutexUnlocked;
    return state.compare_exchange_strong(
        expected,
        kBxTaskMutexLocked,
        std::memory_order_acquire,
        std::memory_order_relaxed);
}

void
BxTaskMutex::lock() {
    if (!tryLock()) {
        lockContended();
    }
}

void
BxTaskMutex::unlock() {
    if (state.exchange(kBxTaskMutexUnlocked, std::memory_order_release) !=
            kBxTaskMutexContended) {
        return;
    }

    waitersLock.lock();
    BxTaskMutexWaiter* waiter = firstWaiter;
    if (waiter) {
        firstWaiter = waiter->next;
        if (!firstWaiter) {
            lastWaiter = NULL;
        }
        waiter->queued = false;
        waiter->task->getTaskQueue()->scheduleTask(waiter->task, true);
    }
    waitersLock.unlock();
}

void
BxTaskMutex::lockContended() {
    for (int spin = 0; spin < kBxTaskMutexSpinCount; ++spin) {
        __asm__ volatile("yield");
        if (state.load(std::memory_order_relaxed) == kBxTaskMutexUnlocked && tryLock()) {
            return;
        }
    }

    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : NULL;
    if (!task) {
        while (state.exchange(kBxTaskMutexContended, std::memory_order_acquire) !=
                kBxTaskMutexUnlocked) {
            sched_yield();
        }
        return;
    }

    BxTaskMutexWaiter waiter = { task, NULL, false };
    while (true) {
        thread->flushThreadLocalCallback();
        waitersLock.lock();
        if (state.exchange(kBxTaskMutexContended, std::memory_order_acquire) ==
                kBxTaskMutexUnlocked) {
            waitersLock.unlock();
            return;
        }

        waiter.next = NULL;
        waiter.queued = true;
        if (lastWaiter) {
            lastWaiter->next = &waiter;
        } else {
            firstWaiter = &waiter;
        }
        lastWaiter = &waiter;
        thread->suspendTask(task, &waitersLock, BxTaskGCStateBlocked);

        thread = brixGetActiveThread();
        waitersLock.lock();
        if (waiter.queued) {
            removeWaiterLocked(&waiter);
        }
        waitersLock.unlock();
    }
}

void
BxTaskMutex::removeWaiterLocked(BxTaskMutexWaiter* waiter) {
    BxTaskMutexWaiter* previous = NULL;
    for (BxTaskMutexWaiter* current = firstWaiter; current; current = current->next) {
        if (current == waiter) {
            if (previous) {
                previous->next = waiter->next;
            } else {
                firstWaiter = waiter->next;
            }
            if (lastWaiter == waiter) {
                lastWaiter = previous;
            }
            waiter->queued = false;
            return;
        }
        previous = current;
    }
}
