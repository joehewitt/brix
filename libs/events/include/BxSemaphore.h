#ifndef BX_SEMAPHORE_H
#define BX_SEMAPHORE_H

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxMutex.h>

class BxThread;
class BxTask;
class BxTaskMutex;

// *************************************************************************************************

struct BxSemaphoreWaiter {
    BxThread* thread;
    BxTask* task;
    void* buf;
    size_t capacity;
    bool received;
    bool served;
    BxSemaphoreWaiter* next;
};

struct BxSemaphorePoster {
    BxThread* thread;
    BxTask* task;
    void* data;
    size_t size;
    bool taken;
    BxSemaphorePoster* next;
    BxThread* takerThread;
    BxTask* takerTask;
};

class BxSemaphore {
public:
    BxSemaphore();

    bool isWaiting() const { return !!waiters; }

    bool wait(BxThread* thread, BxTask* task, void* buf, size_t capacity, BxTaskMutex* mutex);
    void post(void* data, size_t sizeOfData, bool isCancel);

private:
    static void deliver(void* buf, size_t capacity, void* data, size_t sizeOfData);

    BxMutex lock;
    BxSemaphoreWaiter* waiters;
    BxSemaphoreWaiter* lastWaiter;
    BxSemaphorePoster* posters;
    BxSemaphorePoster* lastPoster;
};

// *************************************************************************************************

#endif
