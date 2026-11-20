
#ifndef BX_KQUEUELOOP_H
#define BX_KQUEUELOOP_H

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxPoll.h>

#include <sys/event.h>
#include <atomic>
#include <pthread.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>

// *************************************************************************************************

struct BxPollDescriptor {
    bool usesSelect = false;
    bool readRegistered = false;
    bool writeRegistered = false;
    bool readReady = false;
    bool writeReady = false;
    BxPollWaiterList readers;
    BxPollWaiterList writers;
};

// *************************************************************************************************

class BxKQueueLoop {
public:
    BxKQueueLoop();
    ~BxKQueueLoop();

    BxPollWaitStart beginWait(BxPollWaiter* waiter, uint64_t deadlineNanoseconds);
    void endWait(BxPollWaiter* waiter);
    bool releaseDescriptor(int fd);

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
    BxPollDescriptor* descriptorLocked(int fd);
    BxPollDescriptor* ensureDescriptor(int fd);
    void linkWaiterLocked(BxPollWaiterList& list, BxPollWaiter* waiter);
    void unlinkWaiterLocked(BxPollWaiter* waiter);
    void takeWaitersLocked(BxPollWaiterList& list, std::vector<BxTask*>& tasks);
    void descriptorReadyLocked(int fd, int16_t filter, std::vector<BxTask*>& tasks);
    void expireTimersLocked(uint64_t now, std::vector<BxTask*>& tasks);
    bool addTimerLocked(BxTimerNode* node);
    int registerFilter(int fd, int16_t filter);
    void submit(struct kevent& change);
    void wakeLoop();
    void holdTasksLocked(std::vector<BxTask*>& tasks);
    void scheduleHeldTasks(std::vector<BxTask*>& tasks);

private:
    pthread_t tid;
    int kq;
    pthread_mutex_t lock;
    std::vector<BxPollDescriptor*> descriptors;
    BxTimerHeap timers;
    std::unordered_map<uint64_t, BxTimerNode*> sleepTimers;
    uint64_t armedDeadlineNanoseconds;
    std::atomic<size_t> heldTaskCount;
};

// *************************************************************************************************

#endif
