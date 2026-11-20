#ifndef BX_EPOLLLOOP_H
#define BX_EPOLLLOOP_H

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxPoll.h>

#include <atomic>
#include <pthread.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>

// *************************************************************************************************

struct BxEpollDescriptor {
    bool registered = false;
    bool unpollable = false;
    bool readReady = false;
    bool writeReady = false;
    BxPollWaiterList readers;
    BxPollWaiterList writers;
};

// *************************************************************************************************

class BxEpollLoop {
public:
    BxEpollLoop();
    ~BxEpollLoop();

    BxPollWaitStart beginWait(BxPollWaiter* waiter, uint64_t deadlineNanoseconds);
    void endWait(BxPollWaiter* waiter);
    void releaseDescriptor(int fd);

    bool watchProcExit(int pid);
    void onFileChange(int fd, int type, BxTask* task);
    void onSignal(int sig, BxTask* task);
    void onTimer(uint64_t deadlineNanoseconds, uint64_t id, BxTask* task);

    void unwatchProcExit(int pid);
    void cancelFileChange(int fd, int type);
    void cancelTimer(uint64_t id);
    void cancelSignal(int sig);

    void start();
    void stop();
    void join();

    void runEventLoop();
    uint64_t nextScheduledDeadlineNanoseconds();
    bool hasPendingWork();

private:
    BxEpollDescriptor* descriptorLocked(int fd);
    BxEpollDescriptor* ensureDescriptor(int fd);
    void linkWaiterLocked(BxPollWaiterList& list, BxPollWaiter* waiter);
    void unlinkWaiterLocked(BxPollWaiter* waiter);
    void takeWaitersLocked(BxPollWaiterList& list, std::vector<BxTask*>& tasks);
    void descriptorReadyLocked(int fd, uint32_t events, std::vector<BxTask*>& tasks);
    void processExitedLocked(int pid, std::vector<int>& exited);
    void signalsReadyLocked(std::vector<BxTask*>& tasks);
    void expireTimersLocked(uint64_t now, std::vector<BxTask*>& tasks);
    bool addTimerLocked(BxTimerNode* node);
    void armTimer(uint64_t deadlineNanoseconds);
    int watch(int fd, uint32_t events, uint64_t tag);
    void wakeLoop();
    void holdTasksLocked(std::vector<BxTask*>& tasks);
    void scheduleHeldTasks(std::vector<BxTask*>& tasks);

private:
    pthread_t tid;
    int epoll;
    int wakeDescriptor;
    int timerDescriptor;
    int signalDescriptor;
    pthread_mutex_t lock;
    std::atomic<bool> stopRequested;
    std::vector<BxEpollDescriptor*> descriptors;
    BxTimerHeap timers;
    std::unordered_map<uint64_t, BxTimerNode*> sleepTimers;
    std::unordered_map<int, int> processWatches;
    std::unordered_map<int, BxTask*> signalWatches;
    uint64_t armedDeadlineNanoseconds;
    uint64_t timerDeadlineNanoseconds;
    std::atomic<size_t> heldTaskCount;
};

// *************************************************************************************************

#endif
