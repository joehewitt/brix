#ifndef YU_SEMAPHORE_H
#define YU_SEMAPHORE_H

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuMutex.h>

class YuThread;
class YuTask;
class YuTaskMutex;

// *************************************************************************************************

struct YuSemaphoreWaiter {
    YuThread* thread;
    YuTask* task;
    void* buf;
    size_t capacity;
    bool received;
    bool served;
    YuSemaphoreWaiter* next;
};

struct YuSemaphorePoster {
    YuThread* thread;
    YuTask* task;
    void* data;
    size_t size;
    bool taken;
    YuSemaphorePoster* next;
    YuThread* takerThread;
    YuTask* takerTask;
};

class YuSemaphore {
public:
    YuSemaphore();

    bool isWaiting() const { return !!waiters; }

    bool wait(YuThread* thread, YuTask* task, void* buf, size_t capacity, YuTaskMutex* mutex);
    void post(void* data, size_t sizeOfData, bool isCancel);

private:
    static void deliver(void* buf, size_t capacity, void* data, size_t sizeOfData);

    YuMutex lock;
    YuSemaphoreWaiter* waiters;
    YuSemaphoreWaiter* lastWaiter;
    YuSemaphorePoster* posters;
    YuSemaphorePoster* lastPoster;
};

// *************************************************************************************************

#endif
