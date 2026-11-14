
#ifndef YU_SEMAPHORE_H
#define YU_SEMAPHORE_H

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuMutex.h>

class YuThread;
class YuTask;
class YuTaskMutex;

// *************************************************************************************************

class YuSemaphore {
public:
    YuSemaphore();

    bool isWaiting() const { return !!thread; }

    bool wait(YuThread* thread, YuTask* task, void* buf, size_t capacity, YuTaskMutex* mutex);
    void post(void* data, size_t sizeOfData, bool isCancel);

private:
    void clear();

    YuMutex lock;
    void* buf;
    size_t capacity;
    bool* received;
    YuThread* thread;
    YuTask* task;
};

// *************************************************************************************************

#endif
