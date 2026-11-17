#include <BxEvents/BxSemaphore.h>

#include <BxEvents/BxGCHandshake.h>
#include <BxEvents/BxThread.h>
#include <BxEvents/BxTaskQueue.h>
#include <BxEvents/BxTaskMutex.h>
#include <BxCore/BxCore.h>
#include <BxGC/BxGC.h>
#include <cstdio>
#include <cstring>

// *************************************************************************************************

BxSemaphore::BxSemaphore() :
    waiters(NULL),
    lastWaiter(NULL),
    posters(NULL),
    lastPoster(NULL)
{
}

void
BxSemaphore::deliver(void* buf, size_t capacity, void* data, size_t sizeOfData) {
    if (capacity != sizeOfData || (data == NULL) != (sizeOfData == 0)) {
        std::fprintf(stderr, "Semaphore payload mismatch: expected %zu bytes, supplied %zu bytes%s\n",
            capacity, sizeOfData, (data == NULL) != (sizeOfData == 0) ? " (invalid sender buffer)" : "");
        std::abort();
    }
    if (buf && data) {
        brixBulkWriteBarrierFast(static_cast<char*>(buf), static_cast<char*>(data),
            capacity, brixGCObjectDescriptor(static_cast<char*>(buf)));
        memcpy(buf, data, capacity);
    }
}

bool
BxSemaphore::wait(BxThread* aThread, BxTask* aTask, void* aBuf, size_t aCapacity, BxTaskMutex* mutex) {
    if ((aBuf == NULL) != (aCapacity == 0)) {
        std::fprintf(stderr, "Semaphore payload mismatch: expected %zu bytes, supplied 0 bytes (invalid receiver buffer)\n", aCapacity);
        std::abort();
    }
    if (aTask->getCanceled()) {
        if (mutex) {
            mutex->unlock();
        }
        return false;
    }

    aThread->flushThreadLocalCallback();
    lock.lock();
    if (mutex) {
        mutex->unlock();
    }

    if (posters) {
        BxSemaphorePoster* poster = posters;
        posters = poster->next;
        if (!posters) {
            lastPoster = NULL;
        }
        deliver(aBuf, aCapacity, poster->data, poster->size);
        bool received = aBuf && poster->data;
        poster->taken = true;
        poster->takerThread = aThread;
        poster->takerTask = aTask;
        poster->thread->getTaskQueue()->scheduleTask(poster->task, true);
        aThread->suspendTask(aTask, &lock, BxTaskGCStateBlocked);
        return received;
    }

    BxSemaphoreWaiter waiter = {aThread, aTask, aBuf, aCapacity, false, false, NULL};
    if (lastWaiter) {
        lastWaiter->next = &waiter;
    } else {
        waiters = &waiter;
    }
    lastWaiter = &waiter;

    aThread->suspendTask(aTask, &lock, BxTaskGCStateBlocked);

    lock.lock();
    if (!waiter.served) {
        BxSemaphoreWaiter* previous = NULL;
        for (BxSemaphoreWaiter* each = waiters; each; previous = each, each = each->next) {
            if (each == &waiter) {
                if (previous) {
                    previous->next = each->next;
                } else {
                    waiters = each->next;
                }
                if (lastWaiter == each) {
                    lastWaiter = previous;
                }
                break;
            }
        }
    }
    lock.unlock();
    return waiter.received;
}

void
BxSemaphore::post(void* data, size_t sizeOfData, bool isCancel) {
    BxThread* postingThread = brixGetActiveThread();
    lock.lock();
    if (!waiters && !isCancel && postingThread->hasThreadLocalFlush()) {
        lock.unlock();
        postingThread->flushThreadLocalCallback();
        lock.lock();
    }

    if (waiters) {
        BxSemaphoreWaiter* waiter = waiters;
        waiters = waiter->next;
        if (!waiters) {
            lastWaiter = NULL;
        }
        deliver(waiter->buf, waiter->capacity, data, sizeOfData);
        waiter->received = waiter->buf && data;
        waiter->served = true;
        BxThread* waitingThread = waiter->thread;
        BxTask* waitingTask = waiter->task;
        lock.unlock();
        waitingThread->getTaskQueue()->scheduleTask(waitingTask, true);
        return;
    }
    if (isCancel) {
        lock.unlock();
        return;
    }

    BxTask* postingTask = postingThread->getActiveTask();
    BxSemaphorePoster poster = {postingThread, postingTask, data, sizeOfData, false, NULL, NULL, NULL};
    if (lastPoster) {
        lastPoster->next = &poster;
    } else {
        posters = &poster;
    }
    lastPoster = &poster;

    postingThread->suspendTask(postingTask, &lock, BxTaskGCStateBlocked);

    lock.lock();
    if (!poster.taken) {
        BxSemaphorePoster* previous = NULL;
        for (BxSemaphorePoster* each = posters; each; previous = each, each = each->next) {
            if (each == &poster) {
                if (previous) {
                    previous->next = each->next;
                } else {
                    posters = each->next;
                }
                if (lastPoster == each) {
                    lastPoster = previous;
                }
                break;
            }
        }
    }
    BxThread* takerThread = poster.takerThread;
    BxTask* takerTask = poster.takerTask;
    lock.unlock();
    if (takerTask) {
        takerThread->getTaskQueue()->scheduleTask(takerTask, true);
    }
}
