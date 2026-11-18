#ifndef BX_POLL_H
#define BX_POLL_H

#include <BxEvents/BxEvents.h>

#include <stdint.h>
#include <vector>

#if defined(__APPLE__)
#include <sys/event.h>
#endif

// *************************************************************************************************

#if defined(__APPLE__)
const int16_t BxPollRead = EVFILT_READ;
const int16_t BxPollWrite = EVFILT_WRITE;
#else
const int16_t BxPollRead = 1;
const int16_t BxPollWrite = 2;
#endif

struct BxPollWaiter;

struct BxTimerNode {
    uint64_t deadlineNanoseconds = 0;
    size_t heapIndex = SIZE_MAX;
    BxTask* task = NULL;
    BxPollWaiter* waiter = NULL;
    uint64_t id = 0;
};

struct BxPollWaiter {
    BxPollWaiter(BxTask* task, int fd, int16_t filter): task(task), fd(fd), filter(filter) {
        timer.waiter = this;
        timer.task = task;
    }

    BxTask* task;
    int fd;
    int16_t filter;
    bool linked = false;
    bool timedOut = false;
    BxPollWaiter* prev = NULL;
    BxPollWaiter* next = NULL;
    BxTimerNode timer;
};

struct BxPollWaiterList {
    BxPollWaiter* head = NULL;
    BxPollWaiter* tail = NULL;
};

enum BxPollWaitStart {
    BxPollWaitStartReady = 0,
    BxPollWaitStartSuspend = 1,
    BxPollWaitStartSelect = 2
};

void brixProcessExited(int pid);

// *************************************************************************************************

class BxTimerHeap {
public:
    bool empty() const { return timers.empty(); }
    BxTimerNode* top() const { return timers[0]; }

    void add(BxTimerNode* node);
    void remove(BxTimerNode* node);

private:
    void siftUp(size_t index);
    void siftDown(size_t index);
    void swap(size_t a, size_t b);

private:
    std::vector<BxTimerNode*> timers;
};

// *************************************************************************************************

#endif
