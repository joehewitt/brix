
#ifndef YU_KQUEUELOOP_H
#define YU_KQUEUELOOP_H

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuPoll.h>

#include <sys/event.h>
#include <pthread.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>

// *************************************************************************************************

struct YuPollDescriptor {
    bool usesSelect = false;
    bool readRegistered = false;
    bool writeRegistered = false;
    bool readReady = false;
    bool writeReady = false;
    YuPollWaiterList readers;
    YuPollWaiterList writers;
};

// *************************************************************************************************

class YuKQueueLoop {
public:
    YuKQueueLoop();
    ~YuKQueueLoop();

    YuPollWaitStart beginWait(YuPollWaiter* waiter, uint64_t deadlineNanoseconds);
    void endWait(YuPollWaiter* waiter);
    bool releaseDescriptor(int fd);

    void onProcExit(int pid, YuTask* task);
    void onFileChange(int fd, int type, YuTask* task);
    void onSignal(int sig, YuTask* task);
    void onTimer(uint64_t deadlineNanoseconds, uint64_t id, YuTask* task);

    void cancelProcExit(int pid);
    void cancelFileChange(int fd, int type);
    void cancelTimer(uint64_t id);
    void cancelSignal(int sig);

    void start();
    void stop();
    void join();

    void runEventLoop();
    uint64_t nextScheduledDeadlineNanoseconds();

private:
    YuPollDescriptor* descriptorLocked(int fd);
    YuPollDescriptor* ensureDescriptor(int fd);
    void linkWaiterLocked(YuPollWaiterList& list, YuPollWaiter* waiter);
    void unlinkWaiterLocked(YuPollWaiter* waiter);
    void takeWaitersLocked(YuPollWaiterList& list, std::vector<YuTask*>& tasks);
    void descriptorReadyLocked(int fd, int16_t filter, std::vector<YuTask*>& tasks);
    void expireTimersLocked(uint64_t now, std::vector<YuTask*>& tasks);
    bool addTimerLocked(YuTimerNode* node);
    int registerFilter(int fd, int16_t filter);
    void submit(struct kevent& change);
    void wakeLoop();

private:
    pthread_t tid;
    int kq;
    pthread_mutex_t lock;
    std::vector<YuPollDescriptor*> descriptors;
    YuTimerHeap timers;
    std::unordered_map<uint64_t, YuTimerNode*> sleepTimers;
    uint64_t armedDeadlineNanoseconds;
};

// *************************************************************************************************

#endif
