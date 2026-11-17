#ifndef BX_TASKQUEUE_H
#define BX_TASKQUEUE_H

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxTask.h>

#include <atomic>
#include <BxEvents/BxLock.h>
#include <pthread.h>
#include <vector>

class BxMutex;
class BxThread;

typedef void (*BxTaskQueueHostRunFunc)(void* context);
typedef void (*BxTaskQueueHostWakeFunc)(void* context);

enum BxTaskQueueDrainResult {
    BxTaskQueueDrainIdle = 0,
    BxTaskQueueDrainRemaining = 1,
    BxTaskQueueDrainBusy = 2
};

// *************************************************************************************************

struct BxTaskWorker {
    BxLock lock;
    BxTaskRunList normalTasks;
    BxTaskRunList priorityTasks;
    std::atomic<int> queued {0};
    int consecutivePriorityTasks = 0;
    uint32_t ticks = 0;
    uint32_t random = 0;
    pthread_cond_t cond;
    bool parked = false;
    BxTaskWorker* nextIdle = NULL;
};

struct BxTaskLiveShard {
    BxLock lock;
    std::vector<BxTask*> tasks;
};

// *************************************************************************************************

struct BxTaskEnding;
class BxTaskQueue {
public:
    BxTaskQueue(BxTaskQueue* parentQueue, int threadCount);
    ~BxTaskQueue();

    bool isStopped() { return isFinished.load(std::memory_order_acquire); }
    bool isPool() const { return workers != NULL; }
    int remaining();
    int getThreadCount() const { return threadCount; }
    int activity();
    void requestTermination();
    void cancelTasks();

    void start();
    void stop();
    void join();

    BxTaskQueue* createQueue(int threadCount);
    BxTask* createTask(
        BxTaskStartFunc func,
        void* closure,
        size_t localSize,
        const BxTaskProvenance& provenance = BxTaskProvenance());

    void startTask(
        BxTaskStartFunc func,
        void* closure,
        size_t localSize,
        const BxTaskProvenance& provenance = BxTaskProvenance(),
        BxTaskEnding** ending = nullptr,
        uint64_t* handle = nullptr);
    bool startTaskIfRunning(
        BxTaskStartFunc func,
        void* closure,
        size_t localSize,
        const BxTaskProvenance& provenance = BxTaskProvenance());
    void scheduleTask(BxTask* task, bool includeCanceled = false);
    void wakeCanceledTask(BxTask* task);
    void enqueueClaimedTask(BxTask* task, bool yielded);
    bool hasWaitingTasks();

    void taskTerminated(BxTask* task);

    void addActivity();
    void removeActivity();

    void runEventLoop();
    bool requestHost(
        BxTask* task,
        BxTaskQueueHostRunFunc run,
        BxTaskQueueHostWakeFunc wake,
        void* context);
    uint32_t runHostedTaskUntilDeadline(
        BxThread* thread,
        BxTaskStartFunc func,
        void* closure,
        size_t localSize,
        uint64_t deadlineNanos);
    uint32_t drainHostedTasks(BxThread* thread, size_t taskBudget, uint64_t deadlineNanos);

private:
    void wakeLiveTasks(bool terminating);
    void wakeTask(BxTask* task, bool forCancel);
    bool nextTask(BxTask** task);
    bool nextPoolTask(BxTaskWorker* worker, BxTask** task, bool searching);
    bool tryNextTask(BxTask** task);
    bool takeNextTaskLocked(BxTask** task);
    bool hasRunnableTasksLocked();
    void enqueueTaskLocked(BxTask* task);
    void requeueTaskLocked(BxTask* task);
    void addLiveTask(BxTask* task);
    void removeLiveTask(BxTask* task);
    void signalWaiters();
    bool runPendingHost();
    bool startTaskImpl(
        BxTaskStartFunc func,
        void* closure,
        size_t localSize,
        const BxTaskProvenance& provenance,
        bool requireRunning,
        BxTaskEnding** ending = nullptr,
        uint64_t* handle = nullptr);

    void pushLocal(BxTaskWorker* worker, BxTask* task);
    bool popLocal(BxTaskWorker* worker, BxTask** task);
    bool popInjected(BxTaskWorker* worker, BxTask** task);
    bool steal(BxTaskWorker* worker, BxTask** task);
    bool hasPoolWork();
    void wakeWorker();
    void startWorker();
    void wakeAllWorkers();

private:
    BxTaskQueue* parentQueue;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t* threads;
    BxTaskRunList normalTaskQueue;
    BxTaskRunList priorityTaskQueue;
    BxTaskLiveShard* liveShards;
    int liveShardCount;
    std::atomic<int> activeCount;
    std::vector<BxTaskQueue*> childQueues;
    int consecutivePriorityTasks;
    int threadCount;
    int sleepingThreads;
    std::atomic<bool> isFinished;
    BxTask* hostedTask;
    BxTaskQueueHostRunFunc pendingHostRun;
    BxTaskQueueHostWakeFunc activeHostWake;
    void* hostContext;
    std::atomic<bool> hostPending;
    bool hostDrainActive;

    BxTaskWorker* workers;
    std::atomic<int> startedCount;
    std::atomic<int> nextWorkerIndex;
    std::atomic<int> injectedCount;
    std::atomic<int> idleWorkerCount;
    std::atomic<int> searchingWorkerCount;
    BxTaskWorker* idleWorkers;
};

// *************************************************************************************************

#endif
