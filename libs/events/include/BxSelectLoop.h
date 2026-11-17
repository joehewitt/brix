#include <BxEvents/BxEvents.h>
#include <BxEvents/BxTriggerFile.h>

#include <vector>
#include <atomic>
#include <stdint.h>
#include <sys/select.h>
#include <pthread.h>

// *************************************************************************************************

struct BxSelectWaiter {
    BxSelectWaiter(BxTask* task, int fd, bool isRead, uint64_t deadlineNanoseconds):
        task(task), fd(fd), isRead(isRead), deadlineNanoseconds(deadlineNanoseconds) {}

    BxTask* task;
    int fd;
    bool isRead;
    uint64_t deadlineNanoseconds;
    bool timedOut = false;
};

// *************************************************************************************************

class BxSelectLoop {
public:
    BxSelectLoop();
    ~BxSelectLoop();

    void beginWait(BxSelectWaiter* waiter);
    void endWait(BxSelectWaiter* waiter);
    void cancelDescriptor(int fd);

    void start();
    void stop();
    void join();

    void runEventLoop();

private:
    int prepareHandlesLocked(fd_set* readerHandles, fd_set* writerHandles,
        uint64_t& deadlineNanoseconds);
    void takeReadyLocked(fd_set* readerHandles, fd_set* writerHandles, uint64_t now,
        std::vector<BxTask*>& tasks);
    void takeInvalidLocked(std::vector<BxTask*>& tasks);

private:
    pthread_t tid;
    pthread_mutex_t lock;
    std::vector<BxSelectWaiter*> waiters;
    BxTriggerFile trigger;
    std::atomic<bool> isStopped;
};
