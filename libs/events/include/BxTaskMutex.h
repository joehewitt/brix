#ifndef BX_TASK_MUTEX_H
#define BX_TASK_MUTEX_H

#include <BxEvents/BxMutex.h>

#include <atomic>
#include <stdint.h>

struct BxTaskMutexWaiter;

// *************************************************************************************************

class BxTaskMutex {
public:
    BxTaskMutex();

    bool tryLock();
    void lock();
    void unlock();

private:
    void lockContended();
    void removeWaiterLocked(BxTaskMutexWaiter* waiter);

    std::atomic<uint32_t> state;
    BxMutex waitersLock;
    BxTaskMutexWaiter* firstWaiter;
    BxTaskMutexWaiter* lastWaiter;
};

// *************************************************************************************************

#endif
