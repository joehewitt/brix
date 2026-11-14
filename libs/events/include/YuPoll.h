#ifndef YU_POLL_H
#define YU_POLL_H

#include <YuEvents/YuEvents.h>

#include <stdint.h>
#include <vector>

#if defined(__APPLE__)
#include <sys/event.h>
#endif

// *************************************************************************************************

#if defined(__APPLE__)
const int16_t YuPollRead = EVFILT_READ;
const int16_t YuPollWrite = EVFILT_WRITE;
#else
const int16_t YuPollRead = 1;
const int16_t YuPollWrite = 2;
#endif

struct YuPollWaiter;

struct YuTimerNode {
    uint64_t deadlineNanoseconds = 0;
    size_t heapIndex = SIZE_MAX;
    YuTask* task = NULL;
    YuPollWaiter* waiter = NULL;
    uint64_t id = 0;
};

struct YuPollWaiter {
    YuPollWaiter(YuTask* task, int fd, int16_t filter): task(task), fd(fd), filter(filter) {
        timer.waiter = this;
        timer.task = task;
    }

    YuTask* task;
    int fd;
    int16_t filter;
    bool linked = false;
    bool timedOut = false;
    YuPollWaiter* prev = NULL;
    YuPollWaiter* next = NULL;
    YuTimerNode timer;
};

struct YuPollWaiterList {
    YuPollWaiter* head = NULL;
    YuPollWaiter* tail = NULL;
};

enum YuPollWaitStart {
    YuPollWaitStartReady = 0,
    YuPollWaitStartSuspend = 1,
    YuPollWaitStartSelect = 2
};

// *************************************************************************************************

class YuTimerHeap {
public:
    bool empty() const { return timers.empty(); }
    YuTimerNode* top() const { return timers[0]; }

    void add(YuTimerNode* node);
    void remove(YuTimerNode* node);

private:
    void siftUp(size_t index);
    void siftDown(size_t index);
    void swap(size_t a, size_t b);

private:
    std::vector<YuTimerNode*> timers;
};

// *************************************************************************************************

#endif
