#ifndef YU_TASKQUEUE_H
#define YU_TASKQUEUE_H

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuTask.h>

#include <atomic>
#include <YuEvents/YuLock.h>
#include <pthread.h>
#include <vector>

class YuMutex;
class YuThread;

typedef void (*YuTaskQueueHostRunFunc)(void* context);
typedef void (*YuTaskQueueHostWakeFunc)(void* context);

enum YuTaskQueueDrainResult {
    YuTaskQueueDrainIdle = 0,
    YuTaskQueueDrainRemaining = 1,
    YuTaskQueueDrainBusy = 2
};

// *************************************************************************************************

struct YuTaskWorker {
    YuLock lock;
    YuTaskRunList normalTasks;
    YuTaskRunList priorityTasks;
    std::atomic<int> queued {0};
    int consecutivePriorityTasks = 0;
    uint32_t ticks = 0;
    uint32_t random = 0;
    pthread_cond_t cond;
    bool parked = false;
    YuTaskWorker* nextIdle = NULL;
};

struct YuTaskLiveShard {
    YuLock lock;
    std::vector<YuTask*> tasks;
};

// *************************************************************************************************

class YuTaskQueue {
public:
    YuTaskQueue(YuTaskQueue* parentQueue, int threadCount);
    ~YuTaskQueue();

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

    YuTaskQueue* createQueue(int threadCount);
    YuTask* createTask(
        YuTaskStartFunc func,
        void* closure,
        size_t localSize,
        const YuTaskProvenance& provenance = YuTaskProvenance());

    void startTask(
        YuTaskStartFunc func,
        void* closure,
        size_t localSize,
        const YuTaskProvenance& provenance = YuTaskProvenance());
    bool startTaskIfRunning(
        YuTaskStartFunc func,
        void* closure,
        size_t localSize,
        const YuTaskProvenance& provenance = YuTaskProvenance());
    void scheduleTask(YuTask* task, bool includeCanceled = false);
    void enqueueClaimedTask(YuTask* task, bool yielded);
    bool hasWaitingTasks();

    void taskTerminated(YuTask* task);

    void addActivity();
    void removeActivity();

    void runEventLoop();
    bool requestHost(
        YuTask* task,
        YuTaskQueueHostRunFunc run,
        YuTaskQueueHostWakeFunc wake,
        void* context);
    uint32_t runHostedTaskUntilDeadline(
        YuThread* thread,
        YuTaskStartFunc func,
        void* closure,
        size_t localSize,
        uint64_t deadlineNanos);
    uint32_t drainHostedTasks(YuThread* thread, size_t taskBudget, uint64_t deadlineNanos);

private:
    void wakeLiveTasks(bool terminating);
    bool nextTask(YuTask** task);
    bool nextPoolTask(YuTaskWorker* worker, YuTask** task, bool searching);
    bool tryNextTask(YuTask** task);
    bool takeNextTaskLocked(YuTask** task);
    bool hasRunnableTasksLocked();
    void enqueueTaskLocked(YuTask* task);
    void requeueTaskLocked(YuTask* task);
    void addLiveTask(YuTask* task);
    void removeLiveTask(YuTask* task);
    void signalWaiters();
    bool runPendingHost();
    bool startTaskImpl(
        YuTaskStartFunc func,
        void* closure,
        size_t localSize,
        const YuTaskProvenance& provenance,
        bool requireRunning);

    void pushLocal(YuTaskWorker* worker, YuTask* task);
    bool popLocal(YuTaskWorker* worker, YuTask** task);
    bool popInjected(YuTaskWorker* worker, YuTask** task);
    bool steal(YuTaskWorker* worker, YuTask** task);
    bool hasPoolWork();
    void wakeWorker();
    void startWorker();
    void wakeAllWorkers();

private:
    YuTaskQueue* parentQueue;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t* threads;
    YuTaskRunList normalTaskQueue;
    YuTaskRunList priorityTaskQueue;
    YuTaskLiveShard* liveShards;
    int liveShardCount;
    std::atomic<int> activeCount;
    std::vector<YuTaskQueue*> childQueues;
    int consecutivePriorityTasks;
    int threadCount;
    int sleepingThreads;
    std::atomic<bool> isFinished;
    YuTask* hostedTask;
    YuTaskQueueHostRunFunc pendingHostRun;
    YuTaskQueueHostWakeFunc activeHostWake;
    void* hostContext;
    std::atomic<bool> hostPending;
    bool hostDrainActive;

    YuTaskWorker* workers;
    std::atomic<int> startedCount;
    std::atomic<int> nextWorkerIndex;
    std::atomic<int> injectedCount;
    std::atomic<int> idleWorkerCount;
    std::atomic<int> searchingWorkerCount;
    YuTaskWorker* idleWorkers;
};

// *************************************************************************************************

#endif
