#ifndef YU_EPOLLLOOP_H
#define YU_EPOLLLOOP_H

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuPoll.h>

#include <atomic>
#include <pthread.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>

// *************************************************************************************************

struct YuEpollDescriptor {
    bool registered = false;
    bool unpollable = false;
    bool readReady = false;
    bool writeReady = false;
    YuPollWaiterList readers;
    YuPollWaiterList writers;
};

struct YuProcessWatch {
    int descriptor = -1;
    YuTask* task = NULL;
};

// *************************************************************************************************

class YuEpollLoop {
public:
    YuEpollLoop();
    ~YuEpollLoop();

    YuPollWaitStart beginWait(YuPollWaiter* waiter, uint64_t deadlineNanoseconds);
    void endWait(YuPollWaiter* waiter);
    void releaseDescriptor(int fd);

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
    YuEpollDescriptor* descriptorLocked(int fd);
    YuEpollDescriptor* ensureDescriptor(int fd);
    void linkWaiterLocked(YuPollWaiterList& list, YuPollWaiter* waiter);
    void unlinkWaiterLocked(YuPollWaiter* waiter);
    void takeWaitersLocked(YuPollWaiterList& list, std::vector<YuTask*>& tasks);
    void descriptorReadyLocked(int fd, uint32_t events, std::vector<YuTask*>& tasks);
    void processExitedLocked(int pid, std::vector<YuTask*>& tasks);
    void signalsReadyLocked(std::vector<YuTask*>& tasks);
    void expireTimersLocked(uint64_t now, std::vector<YuTask*>& tasks);
    bool addTimerLocked(YuTimerNode* node);
    void armTimer(uint64_t deadlineNanoseconds);
    int watch(int fd, uint32_t events, uint64_t tag);
    void wakeLoop();

private:
    pthread_t tid;
    int epoll;
    int wakeDescriptor;
    int timerDescriptor;
    int signalDescriptor;
    pthread_mutex_t lock;
    std::atomic<bool> stopRequested;
    std::vector<YuEpollDescriptor*> descriptors;
    YuTimerHeap timers;
    std::unordered_map<uint64_t, YuTimerNode*> sleepTimers;
    std::unordered_map<int, YuProcessWatch> processWatches;
    std::unordered_map<int, YuTask*> signalWatches;
    uint64_t armedDeadlineNanoseconds;
    uint64_t timerDeadlineNanoseconds;
};

// *************************************************************************************************

#endif
