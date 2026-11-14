#include <YuEvents/YuEvents.h>
#include <YuEvents/YuTriggerFile.h>

#include <vector>
#include <atomic>
#include <stdint.h>
#include <sys/select.h>
#include <pthread.h>

// *************************************************************************************************

struct YuSelectWaiter {
    YuSelectWaiter(YuTask* task, int fd, bool isRead, uint64_t deadlineNanoseconds):
        task(task), fd(fd), isRead(isRead), deadlineNanoseconds(deadlineNanoseconds) {}

    YuTask* task;
    int fd;
    bool isRead;
    uint64_t deadlineNanoseconds;
    bool timedOut = false;
};

// *************************************************************************************************

class YuSelectLoop {
public:
    YuSelectLoop();
    ~YuSelectLoop();

    void beginWait(YuSelectWaiter* waiter);
    void endWait(YuSelectWaiter* waiter);
    void cancelDescriptor(int fd);

    void start();
    void stop();
    void join();

    void runEventLoop();

private:
    int prepareHandlesLocked(fd_set* readerHandles, fd_set* writerHandles,
        uint64_t& deadlineNanoseconds);
    void takeReadyLocked(fd_set* readerHandles, fd_set* writerHandles, uint64_t now,
        std::vector<YuTask*>& tasks);
    void takeInvalidLocked(std::vector<YuTask*>& tasks);

private:
    pthread_t tid;
    pthread_mutex_t lock;
    std::vector<YuSelectWaiter*> waiters;
    YuTriggerFile trigger;
    std::atomic<bool> isStopped;
};
