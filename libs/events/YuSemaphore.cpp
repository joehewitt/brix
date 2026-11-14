
#include <YuEvents/YuSemaphore.h>

#include <YuEvents/YuGCHandshake.h>
#include <YuEvents/YuThread.h>
#include <YuEvents/YuTaskQueue.h>
#include <YuEvents/YuTaskMutex.h>
#include <YuCore/YuCore.h>
#include <YuGC/YuGC.h>
#include <cstdio>
#include <cstring>

// *************************************************************************************************

YuSemaphore::YuSemaphore() :
    buf(NULL),
    capacity(0),
    received(NULL),
    thread(NULL),
    task(NULL)
{
}

void
YuSemaphore::clear() {
    buf = NULL;
    capacity = 0;
    received = NULL;
    thread = NULL;
    task = NULL;
}

bool
YuSemaphore::wait(YuThread* aThread, YuTask* aTask, void* aBuf, size_t aCapacity, YuTaskMutex* mutex) {
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

    YuThread* previousThread = thread;
    YuTask* previousTask = task;
    bool hadData = false;
    buf = aBuf;
    capacity = aCapacity;
    received = &hadData;
    thread = aThread;
    task = aTask;
    if (previousTask) {
        previousThread->getTaskQueue()->scheduleTask(previousTask, true);
    }

    aThread->suspendTask(aTask, &lock, YuTaskGCStateBlocked);

    lock.lock();
    if (task == aTask) {
        clear();
    }
    lock.unlock();
    return hadData;
}

void
YuSemaphore::post(void* data, size_t sizeOfData, bool isCancel) {
    lock.lock();
    if (!task && !isCancel && yumiGetActiveThread()->hasThreadLocalFlush()) {
        lock.unlock();
        yumiGetActiveThread()->flushThreadLocalCallback();
        lock.lock();
    }
    if (!task && !isCancel) {
        YuThread* postingThread = yumiGetActiveThread();
        YuTask* postingTask = postingThread->getActiveTask();
        thread = postingThread;
        task = postingTask;
        postingThread->suspendTask(postingTask, &lock, YuTaskGCStateBlocked);
        lock.lock();
        if (task == postingTask) {
            clear();
            lock.unlock();
            return;
        }
        if (postingTask->getCanceled()) {
            YuThread* waitingThread = thread;
            YuTask* waitingTask = task;
            clear();
            lock.unlock();
            waitingThread->getTaskQueue()->scheduleTask(waitingTask, true);
            return;
        }
    }

    if (task && (capacity != sizeOfData || (data == NULL) != (sizeOfData == 0))) {
        std::fprintf(stderr, "Semaphore payload mismatch: expected %zu bytes, supplied %zu bytes%s\n",
            capacity, sizeOfData, (data == NULL) != (sizeOfData == 0) ? " (invalid sender buffer)" : "");
        std::abort();
    }

    YuThread* waitingThread = thread;
    YuTask* waitingTask = task;
    if (buf && data) {
        yumiBulkWriteBarrierFast(static_cast<char*>(buf), static_cast<char*>(data),
            capacity, yumiGCObjectDescriptor(static_cast<char*>(buf)));
        memcpy(buf, data, capacity);
        *received = true;
    }
    clear();
    lock.unlock();

    if (waitingTask) {
        waitingThread->getTaskQueue()->scheduleTask(waitingTask, true);
    }
}
