#ifndef YU_TASK_MUTEX_H
#define YU_TASK_MUTEX_H

#include <YuEvents/YuMutex.h>

#include <atomic>
#include <stdint.h>

struct YuTaskMutexWaiter;

// *************************************************************************************************

class YuTaskMutex {
public:
    YuTaskMutex();

    bool tryLock();
    void lock();
    void unlock();

private:
    void lockContended();
    void removeWaiterLocked(YuTaskMutexWaiter* waiter);

    std::atomic<uint32_t> state;
    YuMutex waitersLock;
    YuTaskMutexWaiter* firstWaiter;
    YuTaskMutexWaiter* lastWaiter;
};

// *************************************************************************************************

#endif
