#include <YuEvents/YuTaskMutex.h>

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuTaskQueue.h>
#include <YuEvents/YuThread.h>

#include <sched.h>

// *************************************************************************************************

static const uint32_t kYuTaskMutexUnlocked = 0;
static const uint32_t kYuTaskMutexLocked = 1;
static const uint32_t kYuTaskMutexContended = 2;
static const int kYuTaskMutexSpinCount = 64;

struct YuTaskMutexWaiter {
    YuTask* task;
    YuTaskMutexWaiter* next;
    bool queued;
};

// *************************************************************************************************

YuTaskMutex::YuTaskMutex() :
    state(kYuTaskMutexUnlocked),
    firstWaiter(NULL),
    lastWaiter(NULL)
{
}

bool
YuTaskMutex::tryLock() {
    uint32_t expected = kYuTaskMutexUnlocked;
    return state.compare_exchange_strong(
        expected,
        kYuTaskMutexLocked,
        std::memory_order_acquire,
        std::memory_order_relaxed);
}

void
YuTaskMutex::lock() {
    if (!tryLock()) {
        lockContended();
    }
}

void
YuTaskMutex::unlock() {
    if (state.exchange(kYuTaskMutexUnlocked, std::memory_order_release) !=
            kYuTaskMutexContended) {
        return;
    }

    waitersLock.lock();
    YuTaskMutexWaiter* waiter = firstWaiter;
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
YuTaskMutex::lockContended() {
    for (int spin = 0; spin < kYuTaskMutexSpinCount; ++spin) {
        __asm__ volatile("yield");
        if (state.load(std::memory_order_relaxed) == kYuTaskMutexUnlocked && tryLock()) {
            return;
        }
    }

    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : NULL;
    if (!task) {
        while (state.exchange(kYuTaskMutexContended, std::memory_order_acquire) !=
                kYuTaskMutexUnlocked) {
            sched_yield();
        }
        return;
    }

    YuTaskMutexWaiter waiter = { task, NULL, false };
    while (true) {
        thread->flushThreadLocalCallback();
        waitersLock.lock();
        if (state.exchange(kYuTaskMutexContended, std::memory_order_acquire) ==
                kYuTaskMutexUnlocked) {
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
        thread->suspendTask(task, &waitersLock, YuTaskGCStateBlocked);

        thread = yumiGetActiveThread();
        waitersLock.lock();
        if (waiter.queued) {
            removeWaiterLocked(&waiter);
        }
        waitersLock.unlock();
    }
}

void
YuTaskMutex::removeWaiterLocked(YuTaskMutexWaiter* waiter) {
    YuTaskMutexWaiter* previous = NULL;
    for (YuTaskMutexWaiter* current = firstWaiter; current; current = current->next) {
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
