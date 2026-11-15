
#include <YuEvents/YuTaskQueue.h>

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuThread.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuMutex.h>
#include <YuEvents/YuGCHandshake.h>
#include <YuEvents/YuStack.h>
#include <signal.h>

#include <cstdio>
#include <limits>
#include <pthread.h>

// *************************************************************************************************

extern "C" void yumiGCRuntimeIdleCollect(uint64_t deadlineNanos) __attribute__((weak));

extern "C" void
yumiGCRuntimeIdleCollect(uint64_t) {
}

static const int kPriorityFairnessLimit = 8;
static const uint32_t kInjectedCheckInterval = 61;
static const int kTaskBatchLimit = 32;

// *************************************************************************************************

static void*
YuTaskQueueThread(void* input) {
    register YuTaskRuntimeContext* runtimeContext asm("x27") =
        yumiGetActiveTaskRuntimeContext();
    asm volatile ("" : "+r" (runtimeContext));

    YuTaskQueue *queue = (YuTaskQueue*)input;
    queue->runEventLoop();
    yumiFlushThreadStackCache();

    asm volatile ("" : "+r" (runtimeContext));

    pthread_exit(NULL);
}

// *************************************************************************************************

YuTaskQueue::YuTaskQueue(YuTaskQueue* aParentQueue, int count):
    parentQueue(aParentQueue),
    threads(NULL),
    liveShards(NULL),
    liveShardCount(count > 1 ? count : 1),
    activeCount(0),
    consecutivePriorityTasks(0),
    threadCount(count),
    sleepingThreads(0),
    isFinished(false),
    hostedTask(NULL),
    pendingHostRun(NULL),
    activeHostWake(NULL),
    hostContext(NULL),
    hostPending(false),
    hostDrainActive(false),
    workers(NULL),
    nextWorkerIndex(0),
    injectedCount(0),
    idleWorkerCount(0),
    searchingWorkerCount(0),
    idleWorkers(NULL)
{
    pthread_mutex_init(&lock, 0);
    pthread_cond_init(&cond, 0);
    liveShards = new YuTaskLiveShard[liveShardCount];

    if (count > 1) {
        workers = new YuTaskWorker[count];
        for (int i = 0; i < count; ++i) {
            pthread_cond_init(&workers[i].cond, 0);
            workers[i].random = 2654435761u * (uint32_t)(i + 1);
        }
    }

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGSEGV);
    sigdelset(&set, SIGBUS);
    sigdelset(&set, SIGQUIT);

    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("pthread_sigmask");
        exit(1);
    }
}

YuTaskQueue::~YuTaskQueue() {
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    if (workers) {
        for (int i = 0; i < threadCount; ++i) {
            pthread_cond_destroy(&workers[i].cond);
        }
        delete[] workers;
    }
    delete[] liveShards;
    delete[] threads;
}

void
YuTaskQueue::start() {
    threads = new pthread_t[threadCount];

    for (int i = 0; i < threadCount; ++i) {
        int err = pthread_create(threads+i, 0, YuTaskQueueThread, (void*)this);
        if (err) {
            printf("Unable to create thread (%d)\n", err);
            exit(-1);
        }
    }
}

void
YuTaskQueue::stop() {
    pthread_mutex_lock(&lock);
    isFinished = true;
    pthread_mutex_unlock(&lock);

    signalWaiters();
    wakeAllWorkers();
}

void
YuTaskQueue::join() {
    for (int i = 0; i < threadCount; ++i) {
        pthread_t thread = threads[i];
        pthread_join(thread, NULL);
    }
}

YuTaskQueue*
YuTaskQueue::createQueue(int threadCount) {
    YuTaskQueue* queue = new YuTaskQueue(this, threadCount);
    pthread_mutex_lock(&lock);
    childQueues.push_back(queue);
    pthread_mutex_unlock(&lock);
    queue->start();
    return queue;
}

YuTask*
YuTaskQueue::createTask(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const YuTaskProvenance& provenance) {
    YuTask* task = new YuTask(this, func, closure, localSize, provenance);
    addLiveTask(task);
    addActivity();
    return task;
}

void
YuTaskQueue::startTask(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const YuTaskProvenance& provenance) {
    startTaskImpl(func, closure, localSize, provenance, false);
}

bool
YuTaskQueue::startTaskIfRunning(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const YuTaskProvenance& provenance) {
    return startTaskImpl(func, closure, localSize, provenance, true);
}

bool
YuTaskQueue::startTaskImpl(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const YuTaskProvenance& provenance,
    bool requireRunning) {
    if (requireRunning && isFinished.load(std::memory_order_acquire)) {
        return false;
    }

    YuTask* task = new YuTask(this, func, closure, localSize, provenance);
    addLiveTask(task);
    addActivity();

    if (task->claimQueued()) {
        enqueueClaimedTask(task, false);
    }
    return true;
}

void
YuTaskQueue::scheduleTask(YuTask* task, bool includeCanceled) {
    if (task->getGCState() == YuTaskGCStateDead) {
        return;
    }
    if (!includeCanceled && task->getCanceled()) {
        return;
    }
    if (workers) {
        if (task->claimQueued()) {
            enqueueClaimedTask(task, false);
        }
        return;
    }

    YuTaskQueueHostWakeFunc wake = NULL;
    void* context = NULL;
    bool signal = false;
    pthread_mutex_lock(&lock);
    if (task->getGCState() != YuTaskGCStateDead) {
        if (task == hostedTask) {
            wake = activeHostWake;
            context = hostContext;
        } else if (task->claimQueued()) {
            yumiGCSetTaskStateAndContext(task, YuTaskGCStateParked, true);
            enqueueTaskLocked(task);
            signal = sleepingThreads != 0;
            wake = activeHostWake;
            context = hostContext;
        }
    }
    pthread_mutex_unlock(&lock);

    if (signal) {
        pthread_cond_signal(&cond);
    }
    if (wake) {
        wake(context);
    }
}

void
YuTaskQueue::enqueueClaimedTask(YuTask* task, bool yielded) {
    if (workers) {
        yumiGCSetTaskStateAndContext(task, YuTaskGCStateParked, true);
        YuThread* thread = yielded ? NULL : yumiGetActiveThread();
        YuTaskWorker* worker =
            thread && thread->getTaskQueue() == this ? thread->getWorker() : NULL;
        if (worker) {
            pushLocal(worker, task);
        } else {
            pthread_mutex_lock(&lock);
            enqueueTaskLocked(task);
            pthread_mutex_unlock(&lock);
        }
        wakeWorker();
        return;
    }

    YuTaskQueueHostWakeFunc wake = NULL;
    void* context = NULL;
    bool signal = false;
    pthread_mutex_lock(&lock);
    if (task == hostedTask) {
        task->releaseQueued();
    } else {
        yumiGCSetTaskStateAndContext(task, YuTaskGCStateParked, true);
        enqueueTaskLocked(task);
        signal = sleepingThreads != 0;
    }
    wake = activeHostWake;
    context = hostContext;
    pthread_mutex_unlock(&lock);

    if (signal) {
        pthread_cond_signal(&cond);
    }
    if (wake) {
        wake(context);
    }
}

bool
YuTaskQueue::hasWaitingTasks() {
    return remaining() > 0;
}

int
YuTaskQueue::remaining() {
    int count = injectedCount.load(std::memory_order_relaxed);
    if (workers) {
        for (int i = 0; i < threadCount; ++i) {
            count += workers[i].queued.load(std::memory_order_relaxed);
        }
    }
    return count;
}

int
YuTaskQueue::activity() {
    return activeCount.load(std::memory_order_acquire);
}

void
YuTaskQueue::taskTerminated(YuTask* task) {
    yumiGCSetTaskStateAndContext(task, YuTaskGCStateDead, false);
    removeLiveTask(task);
    removeActivity();
}

void
YuTaskQueue::addActivity() {
    if (activeCount.fetch_add(1, std::memory_order_acq_rel) == 0 && parentQueue) {
        parentQueue->addActivity();
    }
}

void
YuTaskQueue::removeActivity() {
    if (activeCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        if (parentQueue) {
            parentQueue->removeActivity();
        } else {
            stop();
        }
    }
}

void
YuTaskQueue::requestTermination() {
    std::vector<YuTaskQueue*> children;

    pthread_mutex_lock(&lock);
    children = childQueues;
    pthread_mutex_unlock(&lock);

    wakeLiveTasks(true);

    for (YuTaskQueue* child : children) {
        child->requestTermination();
    }
}

void
YuTaskQueue::cancelTasks() {
    wakeLiveTasks(false);
}

void
YuTaskQueue::wakeLiveTasks(bool terminating) {
    std::vector<YuTask*> claimed;

    pthread_mutex_lock(&lock);
    YuTask* hosted = hostedTask;
    pthread_mutex_unlock(&lock);

    for (int i = 0; i < liveShardCount; ++i) {
        YuTaskLiveShard& shard = liveShards[i];
        shard.lock.lock();
        for (YuTask* task : shard.tasks) {
            if (task->getGCState() == YuTaskGCStateDead) {
                continue;
            }

            if (terminating) {
                task->requestTermination();
            } else {
                task->cancel();
            }

            if (task != hosted && task->claimQueued()) {
                claimed.push_back(task);
            }
        }
        shard.lock.unlock();
    }

    pthread_mutex_lock(&lock);
    for (YuTask* task : claimed) {
        yumiGCSetTaskStateAndContext(task, YuTaskGCStateParked, true);
        enqueueTaskLocked(task);
    }
    pthread_mutex_unlock(&lock);
    signalWaiters();
    wakeAllWorkers();
}

void
YuTaskQueue::signalWaiters() {
    YuTaskQueueHostWakeFunc wake = NULL;
    void* context = NULL;

    pthread_mutex_lock(&lock);
    pthread_cond_broadcast(&cond);
    wake = activeHostWake;
    context = hostContext;
    pthread_mutex_unlock(&lock);

    if (wake) {
        wake(context);
    }
}

void
YuTaskQueue::runEventLoop() {
    YuThread thread(this);
    YuTaskWorker* worker = workers
        ? &workers[nextWorkerIndex.fetch_add(1, std::memory_order_relaxed)]
        : NULL;
    thread.setWorker(worker);
    yumiSetActiveThread(&thread);
    thread.initializeThreadLocals();

    YuTask* task = NULL;
    while (worker ? nextPoolTask(worker, &task) : nextTask(&task)) {
        thread.runTask(task);
        thread.deleteTerminatedTasks();
        runPendingHost();
    }

    thread.deleteTerminatedTasks();
    thread.flushThreadLocals();
}

bool
YuTaskQueue::requestHost(
    YuTask* task,
    YuTaskQueueHostRunFunc run,
    YuTaskQueueHostWakeFunc wake,
    void* context) {
    if (!task || !run || parentQueue) {
        return false;
    }

    pthread_mutex_lock(&lock);
    bool accepted = !hostedTask && !pendingHostRun && !activeHostWake;
    if (accepted) {
        hostedTask = task;
        pendingHostRun = run;
        activeHostWake = wake;
        hostContext = context;
        hostPending.store(true, std::memory_order_release);
    }
    pthread_mutex_unlock(&lock);
    return accepted;
}

uint32_t
YuTaskQueue::runHostedTaskUntilDeadline(
    YuThread* thread,
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    uint64_t deadlineNanos) {
    if (!thread || thread->getTaskQueue() != this || !func || isStopped()) {
        return YuMainTaskDeadlineFailed;
    }

    YuTask* task = createTask(
        func,
        closure,
        localSize,
        YuTaskProvenance(
            0,
            NULL,
            0,
            0,
            0,
            YU_TASK_SCHEDULING_PRIORITY));

    bool runSynchronously = false;
    if (!thread->getActiveTask() &&
        deadlineNanos &&
        yumiMonotonicTimeNanoseconds() < deadlineNanos) {
        pthread_mutex_lock(&lock);
        if (!hostDrainActive && hostedTask) {
            hostDrainActive = true;
            runSynchronously = true;
        }
        pthread_mutex_unlock(&lock);
    }

    if (!runSynchronously) {
        scheduleTask(task);
        return YuMainTaskDeadlineDeferred;
    }

    thread->resumeTask(task);
    bool completed = task->getGCState() == YuTaskGCStateDead;

    pthread_mutex_lock(&lock);
    hostDrainActive = false;
    pthread_mutex_unlock(&lock);

    thread->deleteTerminatedTasks();
    return completed
        ? YuMainTaskDeadlineCompleted
        : YuMainTaskDeadlineDeferred;
}

uint32_t
YuTaskQueue::drainHostedTasks(YuThread* thread, size_t taskBudget, uint64_t deadlineNanos) {
    if (!thread || thread->getTaskQueue() != this || thread->getActiveTask()) {
        return YuTaskQueueDrainBusy;
    }

    pthread_mutex_lock(&lock);
    if (hostDrainActive) {
        pthread_mutex_unlock(&lock);
        return YuTaskQueueDrainBusy;
    }
    hostDrainActive = true;
    pthread_mutex_unlock(&lock);

    size_t completed = 0;
    YuTask* task = NULL;
    while (completed < taskBudget && tryNextTask(&task)) {
        thread->runTask(task);
        thread->deleteTerminatedTasks();
        ++completed;
        if (deadlineNanos && yumiMonotonicTimeNanoseconds() >= deadlineNanos) {
            break;
        }
    }

    pthread_mutex_lock(&lock);
    hostDrainActive = false;
    bool remaining = hasRunnableTasksLocked();
    pthread_mutex_unlock(&lock);
    return remaining ? YuTaskQueueDrainRemaining : YuTaskQueueDrainIdle;
}

bool
YuTaskQueue::runPendingHost() {
    if (!hostPending.load(std::memory_order_acquire)) {
        return false;
    }

    YuTaskQueueHostRunFunc run = NULL;
    void* context = NULL;

    pthread_mutex_lock(&lock);
    run = pendingHostRun;
    context = hostContext;
    pendingHostRun = NULL;
    hostPending.store(false, std::memory_order_relaxed);
    pthread_mutex_unlock(&lock);

    if (!run) {
        return false;
    }

    run(context);

    pthread_mutex_lock(&lock);
    YuTask* task = hostedTask;
    hostedTask = NULL;
    activeHostWake = NULL;
    hostContext = NULL;
    pthread_mutex_unlock(&lock);

    if (task && task->getGCState() != YuTaskGCStateDead) {
        scheduleTask(task, true);
    }
    return true;
}

// -------------------------------------------------------------------------------------------------

void
YuTaskQueue::enqueueTaskLocked(YuTask* task) {
    if (task->getSchedulingClass() == YU_TASK_SCHEDULING_PRIORITY) {
        priorityTaskQueue.push(task);
    } else {
        normalTaskQueue.push(task);
    }
    injectedCount.store(
        (int)(normalTaskQueue.size() + priorityTaskQueue.size()),
        std::memory_order_seq_cst);
}

void
YuTaskQueue::addLiveTask(YuTask* task) {
    YuTaskLiveShard& shard = liveShards[task->getId() % (uint32_t)liveShardCount];
    shard.lock.lock();
    task->setLiveIndex(shard.tasks.size());
    shard.tasks.push_back(task);
    shard.lock.unlock();
}

void
YuTaskQueue::removeLiveTask(YuTask* task) {
    YuTaskLiveShard& shard = liveShards[task->getId() % (uint32_t)liveShardCount];
    shard.lock.lock();
    size_t index = task->getLiveIndex();
    if (index < shard.tasks.size() && shard.tasks[index] == task) {
        YuTask* last = shard.tasks.back();
        shard.tasks[index] = last;
        last->setLiveIndex(index);
        shard.tasks.pop_back();
    }
    shard.lock.unlock();
}

// -------------------------------------------------------------------------------------------------

bool
YuTaskQueue::nextTask(YuTask** outTask) {
    pthread_mutex_lock(&lock);

    bool checkedIdleGC = false;
    while (true) {
        if (takeNextTaskLocked(outTask)) {
            pthread_mutex_unlock(&lock);
            (*outTask)->startQueued();
            return true;
        }
        if (isFinished) {
            pthread_mutex_unlock(&lock);
            return false;
        }

        consecutivePriorityTasks = 0;
        if (!checkedIdleGC && !parentQueue) {
            checkedIdleGC = true;
            pthread_mutex_unlock(&lock);
            if (yumiGCRuntimeIdleCollect) {
                uint64_t now = yumiMonotonicTimeNanoseconds();
                uint64_t idleDeadline = now >
                        std::numeric_limits<uint64_t>::max() - 1000000ull
                    ? std::numeric_limits<uint64_t>::max()
                    : now + 1000000ull;
                uint64_t scheduledDeadline =
                    yumiNextScheduledWorkDeadlineNanoseconds();
                if (scheduledDeadline && scheduledDeadline < idleDeadline) {
                    idleDeadline = scheduledDeadline;
                }
                if (idleDeadline > now) {
                    yumiGCRuntimeIdleCollect(idleDeadline);
                }
            }
            pthread_mutex_lock(&lock);
            continue;
        }
        ++sleepingThreads;
        pthread_cond_wait(&cond, &lock);
        --sleepingThreads;
    }
}

bool
YuTaskQueue::tryNextTask(YuTask** outTask) {
    pthread_mutex_lock(&lock);
    bool ok = takeNextTaskLocked(outTask);
    pthread_mutex_unlock(&lock);
    if (ok) {
        (*outTask)->startQueued();
    }
    return ok;
}

bool
YuTaskQueue::hasRunnableTasksLocked() {
    return !normalTaskQueue.empty() || !priorityTaskQueue.empty();
}

bool
YuTaskQueue::takeNextTaskLocked(YuTask** outTask) {
    bool ok = false;

    bool hasPriorityTasks = !priorityTaskQueue.empty();
    bool hasNormalTasks = !normalTaskQueue.empty();

    if (
        hasPriorityTasks &&
        (!hasNormalTasks || consecutivePriorityTasks < kPriorityFairnessLimit)
    ) {
        ok = true;
        *outTask = priorityTaskQueue.pop();
        ++consecutivePriorityTasks;
    } else if (hasNormalTasks) {
        ok = true;
        *outTask = normalTaskQueue.pop();
        consecutivePriorityTasks = 0;
    }

    if (ok) {
        injectedCount.store(
            (int)(normalTaskQueue.size() + priorityTaskQueue.size()),
            std::memory_order_relaxed);
    }
    return ok;
}

// -------------------------------------------------------------------------------------------------

bool
YuTaskQueue::nextPoolTask(YuTaskWorker* worker, YuTask** outTask) {
    bool searching = false;
    while (true) {
        bool found = false;
        if (++worker->ticks % kInjectedCheckInterval == 0) {
            found = popInjected(worker, outTask);
        }
        if (!found) {
            found = popLocal(worker, outTask) || popInjected(worker, outTask);
        }
        if (!found) {
            if (!searching) {
                searching = true;
                searchingWorkerCount.fetch_add(1, std::memory_order_seq_cst);
            }
            found = steal(worker, outTask);
        }

        if (found) {
            (*outTask)->startQueued();
            if (searching &&
                    searchingWorkerCount.fetch_sub(1, std::memory_order_seq_cst) == 1 &&
                    (injectedCount.load(std::memory_order_relaxed) || hasPoolWork())) {
                wakeWorker();
            }
            return true;
        }

        if (searching) {
            searching = false;
            searchingWorkerCount.fetch_sub(1, std::memory_order_seq_cst);
        }

        pthread_mutex_lock(&lock);
        if (isFinished) {
            pthread_mutex_unlock(&lock);
            return false;
        }
        worker->parked = true;
        worker->nextIdle = idleWorkers;
        idleWorkers = worker;
        idleWorkerCount.fetch_add(1, std::memory_order_seq_cst);
        if (hasRunnableTasksLocked() || hasPoolWork()) {
            idleWorkers = worker->nextIdle;
            worker->nextIdle = NULL;
            worker->parked = false;
            idleWorkerCount.fetch_sub(1, std::memory_order_seq_cst);
            pthread_mutex_unlock(&lock);
            continue;
        }
        while (worker->parked) {
            pthread_cond_wait(&worker->cond, &lock);
        }
        searching = true;
        pthread_mutex_unlock(&lock);
    }
}

void
YuTaskQueue::pushLocal(YuTaskWorker* worker, YuTask* task) {
    worker->lock.lock();
    if (task->getSchedulingClass() == YU_TASK_SCHEDULING_PRIORITY) {
        worker->priorityTasks.push(task);
    } else {
        worker->normalTasks.push(task);
    }
    worker->queued.fetch_add(1, std::memory_order_seq_cst);
    worker->lock.unlock();
}

bool
YuTaskQueue::popLocal(YuTaskWorker* worker, YuTask** outTask) {
    if (!worker->queued.load(std::memory_order_relaxed)) {
        return false;
    }

    YuTask* task = NULL;
    worker->lock.lock();
    bool hasPriorityTasks = !worker->priorityTasks.empty();
    bool hasNormalTasks = !worker->normalTasks.empty();
    if (
        hasPriorityTasks &&
        (!hasNormalTasks || worker->consecutivePriorityTasks < kPriorityFairnessLimit)
    ) {
        task = worker->priorityTasks.pop();
        ++worker->consecutivePriorityTasks;
    } else if (hasNormalTasks) {
        task = worker->normalTasks.pop();
        worker->consecutivePriorityTasks = 0;
    }
    if (task) {
        worker->queued.fetch_sub(1, std::memory_order_relaxed);
    }
    worker->lock.unlock();

    *outTask = task;
    return task != NULL;
}

bool
YuTaskQueue::popInjected(YuTaskWorker* worker, YuTask** outTask) {
    if (!injectedCount.load(std::memory_order_relaxed)) {
        return false;
    }

    YuTask* batch[kTaskBatchLimit];
    int count = 0;
    pthread_mutex_lock(&lock);
    if (!takeNextTaskLocked(outTask)) {
        pthread_mutex_unlock(&lock);
        return false;
    }
    int share = injectedCount.load(std::memory_order_relaxed) / threadCount;
    if (share > kTaskBatchLimit) {
        share = kTaskBatchLimit;
    }
    while (count < share && takeNextTaskLocked(&batch[count])) {
        ++count;
    }
    pthread_mutex_unlock(&lock);

    if (count) {
        worker->lock.lock();
        for (int i = 0; i < count; ++i) {
            YuTask* task = batch[i];
            if (task->getSchedulingClass() == YU_TASK_SCHEDULING_PRIORITY) {
                worker->priorityTasks.push(task);
            } else {
                worker->normalTasks.push(task);
            }
        }
        worker->queued.fetch_add(count, std::memory_order_seq_cst);
        worker->lock.unlock();
    }
    return true;
}

bool
YuTaskQueue::steal(YuTaskWorker* worker, YuTask** outTask) {
    worker->random ^= worker->random << 13;
    worker->random ^= worker->random >> 17;
    worker->random ^= worker->random << 5;
    int first = (int)(worker->random % (uint32_t)threadCount);

    for (int offset = 0; offset < threadCount; ++offset) {
        YuTaskWorker* victim = &workers[(first + offset) % threadCount];
        if (victim == worker || !victim->queued.load(std::memory_order_relaxed)) {
            continue;
        }

        YuTask* batch[kTaskBatchLimit];
        int count = 0;
        victim->lock.lock();
        int take = (victim->queued.load(std::memory_order_relaxed) + 1) / 2;
        if (take > kTaskBatchLimit) {
            take = kTaskBatchLimit;
        }
        while (count < take) {
            YuTask* task = victim->priorityTasks.pop();
            if (!task) {
                task = victim->normalTasks.pop();
            }
            if (!task) {
                break;
            }
            batch[count++] = task;
        }
        victim->queued.fetch_sub(count, std::memory_order_relaxed);
        victim->lock.unlock();

        if (!count) {
            continue;
        }

        *outTask = batch[0];
        if (count > 1) {
            worker->lock.lock();
            for (int i = 1; i < count; ++i) {
                YuTask* task = batch[i];
                if (task->getSchedulingClass() == YU_TASK_SCHEDULING_PRIORITY) {
                    worker->priorityTasks.push(task);
                } else {
                    worker->normalTasks.push(task);
                }
            }
            worker->queued.fetch_add(count - 1, std::memory_order_seq_cst);
            worker->lock.unlock();
        }
        return true;
    }
    return false;
}

bool
YuTaskQueue::hasPoolWork() {
    for (int i = 0; i < threadCount; ++i) {
        if (workers[i].queued.load(std::memory_order_seq_cst)) {
            return true;
        }
    }
    return false;
}

void
YuTaskQueue::wakeWorker() {
    if (!idleWorkerCount.load(std::memory_order_seq_cst)) {
        return;
    }
    int expected = 0;
    if (!searchingWorkerCount.compare_exchange_strong(expected, 1, std::memory_order_seq_cst)) {
        return;
    }

    pthread_mutex_lock(&lock);
    YuTaskWorker* worker = idleWorkers;
    if (worker) {
        idleWorkers = worker->nextIdle;
        worker->nextIdle = NULL;
        worker->parked = false;
        idleWorkerCount.fetch_sub(1, std::memory_order_seq_cst);
    } else {
        searchingWorkerCount.fetch_sub(1, std::memory_order_seq_cst);
    }
    pthread_mutex_unlock(&lock);

    if (worker) {
        pthread_cond_signal(&worker->cond);
    }
}

void
YuTaskQueue::wakeAllWorkers() {
    if (!workers) {
        return;
    }

    pthread_mutex_lock(&lock);
    while (YuTaskWorker* worker = idleWorkers) {
        idleWorkers = worker->nextIdle;
        worker->nextIdle = NULL;
        worker->parked = false;
        idleWorkerCount.fetch_sub(1, std::memory_order_seq_cst);
        searchingWorkerCount.fetch_add(1, std::memory_order_seq_cst);
        pthread_cond_signal(&worker->cond);
    }
    pthread_mutex_unlock(&lock);
}
