
#include <BxEvents/BxTaskQueue.h>

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxThread.h>
#include <BxEvents/BxTask.h>
#include <BxEvents/BxMutex.h>
#include <BxEvents/BxGCHandshake.h>
#include <BxEvents/BxStack.h>
#include <BxGC/BxGC.h>
#include <signal.h>

#include <cstdio>
#include <limits>
#include <pthread.h>

// *************************************************************************************************

extern "C" bool brixGCRuntimeIdleCollect(uint64_t budgetNanos) __attribute__((weak));

extern "C" bool
brixGCRuntimeIdleCollect(uint64_t) {
    return false;
}

static const int kPriorityFairnessLimit = 8;
static const uint32_t kInjectedCheckInterval = 61;
static const int kTaskBatchLimit = 32;
static const long kIdleGCRetryNanos = 20000000L;
static const long kIdleWaitRetryNanos = 100000L;
static const uint64_t kIdleWaitPatienceNanos = 1000000000ull;

static std::mutex gQueueTreeMutex;

// *************************************************************************************************

static void
workerSignals(sigset_t* set) {
    sigfillset(set);
    sigdelset(set, SIGSEGV);
    sigdelset(set, SIGBUS);
    sigdelset(set, SIGQUIT);
}

static void*
BxTaskQueueThread(void* input) {
    register BxTaskRuntimeContext* runtimeContext asm("x27") =
        brixGetActiveTaskRuntimeContext();
    asm volatile ("" : "+r" (runtimeContext));

    BxTaskQueue *queue = (BxTaskQueue*)input;
    queue->runEventLoop();
    brixFlushThreadStackCache();

    asm volatile ("" : "+r" (runtimeContext));

    pthread_exit(NULL);
}

// *************************************************************************************************

BxTaskQueue::BxTaskQueue(BxTaskQueue* aParentQueue, int count):
    parentQueue(aParentQueue),
    threads(NULL),
    liveShards(NULL),
    liveShardCount(count > 1 ? count : 1),
    activeCount(0),
    consecutivePriorityTasks(0),
    threadCount(count),
    sleepingThreads(0),
    isFinished(false),
    stopRequested(false),
    stopDrained(false),
    hostedTask(NULL),
    pendingHostRun(NULL),
    activeHostWake(NULL),
    hostContext(NULL),
    hostPending(false),
    hostDrainActive(false),
    workers(NULL),
    startedCount(0),
    nextWorkerIndex(0),
    injectedCount(0),
    idleWorkerCount(0),
    searchingWorkerCount(0),
    idleWorkers(NULL),
    releasable(false),
    unpinned(false),
    ownerHandle(0),
    pins(0)
{
    pthread_mutex_init(&lock, 0);
    pthread_cond_init(&cond, 0);
    liveShards = new BxTaskLiveShard[liveShardCount];

    if (count > 1) {
        workers = new BxTaskWorker[count];
        for (int i = 0; i < count; ++i) {
            pthread_cond_init(&workers[i].cond, 0);
            workers[i].random = 2654435761u * (uint32_t)(i + 1);
        }
    }

    sigset_t set;
    workerSignals(&set);

    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("pthread_sigmask");
        exit(1);
    }
}

BxTaskQueue::~BxTaskQueue() {
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
BxTaskQueue::start() {
    threads = new pthread_t[threadCount];
    joinedThreads.assign(threadCount, false);
    if (workers) {
        return;
    }

    for (int i = 0; i < threadCount; ++i) {
        int err = pthread_create(threads+i, 0, BxTaskQueueThread, (void*)this);
        if (err) {
            printf("Unable to create thread (%d)\n", err);
            exit(-1);
        }
    }
    startedCount.store(threadCount, std::memory_order_release);
}

void
BxTaskQueue::stop() {
    pthread_mutex_lock(&lock);
    isFinished = true;
    pthread_mutex_unlock(&lock);

    signalWaiters();
    wakeAllWorkers();
}

void
BxTaskQueue::stopAfterLiveTasks(BxThread* thread, BxTask* caller) {
    stopRequested.store(true, std::memory_order_seq_cst);
    finishStopIfDrained();

    stopLock.lock();
    while (!stopDrained) {
        stopWaiters.push_back(caller);
        thread->suspendTask(caller, &stopLock, BxTaskGCStateBlocked);
        stopLock.lock();
        for (size_t i = 0; i < stopWaiters.size(); ++i) {
            if (stopWaiters[i] == caller) {
                stopWaiters.erase(stopWaiters.begin() + i);
                break;
            }
        }
    }
    stopLock.unlock();
}

void
BxTaskQueue::finishStopIfDrained() {
    std::vector<BxTask*> waiters;
    stopLock.lock();
    bool finishes = !stopDrained && !hasLiveTasks();
    if (finishes) {
        stopDrained = true;
        waiters.swap(stopWaiters);
    }
    stopLock.unlock();
    if (!finishes) {
        return;
    }

    stop();
    for (BxTask* waiter : waiters) {
        waiter->getTaskQueue()->scheduleTask(waiter, true);
    }
}

void
BxTaskQueue::join() {
    std::lock_guard<std::mutex> guard(joinLock);
    int started = startedCount.load(std::memory_order_acquire);
    pthread_t self = pthread_self();
    for (int i = 0; i < started; ++i) {
        if (joinedThreads[i] || pthread_equal(threads[i], self)) {
            continue;
        }
        pthread_join(threads[i], NULL);
        joinedThreads[i] = true;
    }
}

char*
BxTaskQueue::loadOwner() {
    return brixGCOwnerWeakLoad(ownerHandle.load(std::memory_order_acquire));
}

char*
BxTaskQueue::adoptOwner(char* owner, size_t handle) {
    std::lock_guard<std::mutex> guard(ownerLock);
    char* existing = brixGCOwnerWeakLoad(ownerHandle.load(std::memory_order_relaxed));
    if (existing) {
        return existing;
    }
    ownerHandle.store(handle, std::memory_order_release);
    ++pins;
    return owner;
}

void
BxTaskQueue::pin() {
    std::lock_guard<std::mutex> guard(ownerLock);
    ++pins;
}

void
BxTaskQueue::unpin() {
    {
        std::lock_guard<std::mutex> guard(ownerLock);
        if (--pins) {
            return;
        }
    }
    if (!releasable) {
        return;
    }
    if (isStopped() && !hasLiveTasks()) {
        brixGCEnterRuntimeBlock();
        join();
        brixGCLeaveRuntimeBlock();
    }
    std::lock_guard<std::mutex> tree(gQueueTreeMutex);
    unpinned = true;
    BxTaskQueue* queue = this;
    while (queue && queue->isUnusedLocked()) {
        BxTaskQueue* parent = queue->parentQueue;
        pthread_mutex_lock(&parent->lock);
        std::vector<BxTaskQueue*>& siblings = parent->childQueues;
        for (size_t i = 0; i < siblings.size(); ++i) {
            if (siblings[i] == queue) {
                siblings.erase(siblings.begin() + i);
                break;
            }
        }
        pthread_mutex_unlock(&parent->lock);
        delete queue;
        queue = parent;
    }
}

bool
BxTaskQueue::isUnusedLocked() {
    if (!releasable || !unpinned || !parentQueue || brixProcessTerminationRequested() ||
            !isStopped() || hasLiveTasks()) {
        return false;
    }
    {
        std::lock_guard<std::mutex> guard(ownerLock);
        if (pins) {
            return false;
        }
    }
    {
        std::lock_guard<std::mutex> guard(joinLock);
        int started = startedCount.load(std::memory_order_acquire);
        for (int i = 0; i < started; ++i) {
            if (!joinedThreads[i]) {
                return false;
            }
        }
    }
    pthread_mutex_lock(&lock);
    bool hasChildren = !childQueues.empty();
    pthread_mutex_unlock(&lock);
    return !hasChildren;
}

BxTaskQueue*
BxTaskQueue::createQueue(int threadCount) {
    BxTaskQueue* queue = new BxTaskQueue(this, threadCount);
    pthread_mutex_lock(&lock);
    childQueues.push_back(queue);
    pthread_mutex_unlock(&lock);
    queue->start();
    return queue;
}

BxTask*
BxTaskQueue::createTask(
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const BxTaskProvenance& provenance) {
    BxTask* task = new BxTask(this, func, closure, localSize, provenance);
    addLiveTask(task);
    addActivity();
    return task;
}

void
BxTaskQueue::startTask(
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const BxTaskProvenance& provenance,
    BxTaskEnding** ending,
    uint64_t* handle) {
    startTaskImpl(func, closure, localSize, provenance, false, ending, handle);
}

bool
BxTaskQueue::startTaskIfRunning(
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const BxTaskProvenance& provenance) {
    return startTaskImpl(func, closure, localSize, provenance, true);
}

bool
BxTaskQueue::startTaskImpl(
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const BxTaskProvenance& provenance,
    bool requireRunning,
    BxTaskEnding** ending,
    uint64_t* handle) {
    if (requireRunning && isStopped()) {
        return false;
    }

    BxTask* task = new BxTask(this, func, closure, localSize, provenance);
    if (ending) {
        *ending = task->attachEnding();
    }
    if (handle) {
        *handle = task->ensureHandle();
    }
    addLiveTask(task);
    addActivity();

    if (task->claimQueued()) {
        enqueueClaimedTask(task, false);
    }
    return true;
}

void
BxTaskQueue::scheduleTask(BxTask* task, bool includeCanceled) {
    if (!includeCanceled && task->getCanceled()) {
        return;
    }
    wakeTask(task, false);
}

void
BxTaskQueue::wakeCanceledTask(BxTask* task) {
    wakeTask(task, true);
}

void
BxTaskQueue::wakeTask(BxTask* task, bool forCancel) {
    if (task->getGCState() == BxTaskGCStateDead) {
        return;
    }
    if (workers) {
        if (forCancel ? task->claimCancelWake() : task->claimQueued()) {
            enqueueClaimedTask(task, false);
        }
        return;
    }

    BxTaskQueueHostWakeFunc wake = NULL;
    void* context = NULL;
    bool signal = false;
    pthread_mutex_lock(&lock);
    if (task->getGCState() != BxTaskGCStateDead) {
        if (task == hostedTask) {
            if (!forCancel || task->getCanceled()) {
                wake = activeHostWake;
                context = hostContext;
            }
        } else if (forCancel ? task->claimCancelWake() : task->claimQueued()) {
            brixGCSetTaskStateAndContext(task, BxTaskGCStateParked, true);
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
BxTaskQueue::enqueueClaimedTask(BxTask* task, bool yielded) {
    if (workers) {
        brixGCSetTaskStateAndContext(task, BxTaskGCStateParked, true);
        BxThread* thread = yielded ? NULL : brixGetActiveThread();
        BxTaskWorker* worker =
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

    BxTaskQueueHostWakeFunc wake = NULL;
    void* context = NULL;
    bool signal = false;
    pthread_mutex_lock(&lock);
    if (task == hostedTask) {
        task->releaseQueued();
    } else {
        brixGCSetTaskStateAndContext(task, BxTaskGCStateParked, true);
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
BxTaskQueue::hasWaitingTasks() {
    return remaining() > 0;
}

int
BxTaskQueue::remaining() {
    int count = injectedCount.load(std::memory_order_relaxed);
    if (workers) {
        for (int i = 0; i < threadCount; ++i) {
            count += workers[i].queued.load(std::memory_order_relaxed);
        }
    }
    return count;
}

int
BxTaskQueue::activity() {
    return activeCount.load(std::memory_order_acquire);
}

void
BxTaskQueue::taskTerminated(BxTask* task) {
    brixGCSetTaskStateAndContext(task, BxTaskGCStateDead, false);
    removeLiveTask(task);
    removeActivity();
    if (stopRequested.load(std::memory_order_acquire)) {
        finishStopIfDrained();
    }
}

void
BxTaskQueue::addActivity() {
    if (activeCount.fetch_add(1, std::memory_order_acq_rel) == 0 && parentQueue) {
        parentQueue->addActivity();
    }
}

void
BxTaskQueue::removeActivity() {
    if (activeCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        if (parentQueue) {
            parentQueue->removeActivity();
        } else {
            stop();
        }
    }
}

void
BxTaskQueue::requestTermination() {
    std::lock_guard<std::mutex> tree(gQueueTreeMutex);
    requestTerminationLocked();
}

void
BxTaskQueue::requestTerminationLocked() {
    std::vector<BxTaskQueue*> children;

    pthread_mutex_lock(&lock);
    children = childQueues;
    pthread_mutex_unlock(&lock);

    wakeLiveTasks(true);

    for (BxTaskQueue* child : children) {
        child->requestTerminationLocked();
    }
}

void
BxTaskQueue::cancelTasks() {
    wakeLiveTasks(false);
}

void
BxTaskQueue::wakeLiveTasks(bool terminating) {
    std::vector<BxTask*> claimed;

    pthread_mutex_lock(&lock);
    BxTask* hosted = hostedTask;
    pthread_mutex_unlock(&lock);

    for (int i = 0; i < liveShardCount; ++i) {
        BxTaskLiveShard& shard = liveShards[i];
        shard.lock.lock();
        for (BxTask* task : shard.tasks) {
            if (task->getGCState() == BxTaskGCStateDead) {
                continue;
            }

            bool wakes = false;
            if (terminating) {
                task->requestTermination();
                wakes = task != hosted && task->claimQueued();
            } else {
                task->cancel();
                wakes = task != hosted && task->claimCancelWake();
            }

            if (wakes) {
                claimed.push_back(task);
            }
        }
        shard.lock.unlock();
    }

    pthread_mutex_lock(&lock);
    for (BxTask* task : claimed) {
        brixGCSetTaskStateAndContext(task, BxTaskGCStateParked, true);
        enqueueTaskLocked(task);
    }
    pthread_mutex_unlock(&lock);
    signalWaiters();
    wakeAllWorkers();
}

void
BxTaskQueue::signalWaiters() {
    BxTaskQueueHostWakeFunc wake = NULL;
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
BxTaskQueue::runEventLoop() {
    BxThread thread(this);
    BxTaskWorker* worker = workers
        ? &workers[nextWorkerIndex.fetch_add(1, std::memory_order_relaxed)]
        : NULL;
    thread.setWorker(worker);
    brixSetActiveThread(&thread);
    thread.initializeThreadLocals();

    BxTask* task = NULL;
    bool searching = worker != NULL;
    while (worker ? nextPoolTask(worker, &task, searching) : nextTask(&task)) {
        searching = false;
        thread.runTask(task);
        thread.deleteTerminatedTasks();
        runPendingHost();
    }

    thread.deleteTerminatedTasks();
    thread.flushThreadLocals();
}

bool
BxTaskQueue::requestHost(
    BxTask* task,
    BxTaskQueueHostRunFunc run,
    BxTaskQueueHostWakeFunc wake,
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
BxTaskQueue::runHostedTaskUntilDeadline(
    BxThread* thread,
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    uint64_t deadlineNanos) {
    if (!thread || thread->getTaskQueue() != this || !func || isStopped()) {
        return BxMainTaskDeadlineFailed;
    }

    BxTask* task = createTask(
        func,
        closure,
        localSize,
        BxTaskProvenance(
            0,
            NULL,
            0,
            0,
            0,
            BX_TASK_SCHEDULING_PRIORITY));

    bool runSynchronously = false;
    if (!thread->getActiveTask() &&
        deadlineNanos &&
        brixMonotonicTimeNanoseconds() < deadlineNanos) {
        pthread_mutex_lock(&lock);
        if (!hostDrainActive && hostedTask) {
            hostDrainActive = true;
            runSynchronously = true;
        }
        pthread_mutex_unlock(&lock);
    }

    if (!runSynchronously) {
        scheduleTask(task);
        return BxMainTaskDeadlineDeferred;
    }

    thread->resumeTask(task);
    bool completed = task->getGCState() == BxTaskGCStateDead;

    pthread_mutex_lock(&lock);
    hostDrainActive = false;
    pthread_mutex_unlock(&lock);

    thread->deleteTerminatedTasks();
    return completed
        ? BxMainTaskDeadlineCompleted
        : BxMainTaskDeadlineDeferred;
}

uint32_t
BxTaskQueue::drainHostedTasks(BxThread* thread, size_t taskBudget, uint64_t deadlineNanos) {
    if (!thread || thread->getTaskQueue() != this || thread->getActiveTask()) {
        return BxTaskQueueDrainBusy;
    }

    pthread_mutex_lock(&lock);
    if (hostDrainActive) {
        pthread_mutex_unlock(&lock);
        return BxTaskQueueDrainBusy;
    }
    hostDrainActive = true;
    pthread_mutex_unlock(&lock);

    size_t completed = 0;
    BxTask* task = NULL;
    while (completed < taskBudget && tryNextTask(&task)) {
        thread->runTask(task);
        thread->deleteTerminatedTasks();
        ++completed;
        if (deadlineNanos && brixMonotonicTimeNanoseconds() >= deadlineNanos) {
            break;
        }
    }

    pthread_mutex_lock(&lock);
    hostDrainActive = false;
    bool remaining = hasRunnableTasksLocked();
    pthread_mutex_unlock(&lock);
    return remaining ? BxTaskQueueDrainRemaining : BxTaskQueueDrainIdle;
}

bool
BxTaskQueue::runPendingHost() {
    if (!hostPending.load(std::memory_order_acquire)) {
        return false;
    }

    BxTaskQueueHostRunFunc run = NULL;
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
    BxTask* task = hostedTask;
    hostedTask = NULL;
    activeHostWake = NULL;
    hostContext = NULL;
    pthread_mutex_unlock(&lock);

    if (task && task->getGCState() != BxTaskGCStateDead) {
        scheduleTask(task, true);
    }
    return true;
}

// -------------------------------------------------------------------------------------------------

void
BxTaskQueue::enqueueTaskLocked(BxTask* task) {
    if (task->getSchedulingClass() == BX_TASK_SCHEDULING_PRIORITY) {
        priorityTaskQueue.push(task);
    } else {
        normalTaskQueue.push(task);
    }
    injectedCount.store(
        (int)(normalTaskQueue.size() + priorityTaskQueue.size()),
        std::memory_order_seq_cst);
}

void
BxTaskQueue::addLiveTask(BxTask* task) {
    BxTaskLiveShard& shard = liveShards[task->getId() % (uint32_t)liveShardCount];
    shard.lock.lock();
    task->setLiveIndex(shard.tasks.size());
    shard.tasks.push_back(task);
    shard.lock.unlock();
}

void
BxTaskQueue::removeLiveTask(BxTask* task) {
    BxTaskLiveShard& shard = liveShards[task->getId() % (uint32_t)liveShardCount];
    shard.lock.lock();
    size_t index = task->getLiveIndex();
    if (index < shard.tasks.size() && shard.tasks[index] == task) {
        BxTask* last = shard.tasks.back();
        shard.tasks[index] = last;
        last->setLiveIndex(index);
        shard.tasks.pop_back();
    }
    shard.lock.unlock();
}

bool
BxTaskQueue::hasLiveTasks() {
    for (int i = 0; i < liveShardCount; ++i) {
        BxTaskLiveShard& shard = liveShards[i];
        shard.lock.lock();
        bool live = !shard.tasks.empty();
        shard.lock.unlock();
        if (live) {
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------------------------------------

bool
BxTaskQueue::nextTask(BxTask** outTask) {
    pthread_mutex_lock(&lock);

    bool checkedIdleGC = false;
    bool retriesIdleGC = false;
    uint64_t pendingSince = 0;
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

        if (!idleWaiters.empty()) {
            pthread_mutex_unlock(&lock);
            bool pending = brixHasPendingEventWork();
            uint64_t now = brixMonotonicTimeNanoseconds();
            pthread_mutex_lock(&lock);
            if (hasRunnableTasksLocked() || idleWaiters.empty()) {
                continue;
            }
            if (pending) {
                if (!pendingSince) {
                    pendingSince = now;
                }
                if (now - pendingSince < kIdleWaitPatienceNanos) {
                    waitForPendingWorkLocked();
                    continue;
                }
            }
            pendingSince = 0;
            BxTask* waiter = idleWaiters.back();
            idleWaiters.pop_back();
            pthread_mutex_unlock(&lock);
            scheduleTask(waiter, true);
            pthread_mutex_lock(&lock);
            continue;
        }

        consecutivePriorityTasks = 0;
        if (!checkedIdleGC && !parentQueue) {
            checkedIdleGC = true;
            pthread_mutex_unlock(&lock);
            bool pending = false;
            if (brixGCRuntimeIdleCollect) {
                uint64_t now = brixMonotonicTimeNanoseconds();
                uint64_t idleDeadline = now >
                        std::numeric_limits<uint64_t>::max() - 1000000ull
                    ? std::numeric_limits<uint64_t>::max()
                    : now + 1000000ull;
                uint64_t scheduledDeadline =
                    brixNextScheduledWorkDeadlineNanoseconds();
                if (scheduledDeadline && scheduledDeadline < idleDeadline) {
                    idleDeadline = scheduledDeadline;
                }
                if (idleDeadline > now) {
                    pending = brixGCRuntimeIdleCollect(idleDeadline - now);
                }
            }
            pthread_mutex_lock(&lock);
            retriesIdleGC = pending;
            continue;
        }
        ++sleepingThreads;
        if (retriesIdleGC) {
            retriesIdleGC = false;
            checkedIdleGC = false;
#if defined(__APPLE__)
            struct timespec retry = { 0, kIdleGCRetryNanos };
            pthread_cond_timedwait_relative_np(&cond, &lock, &retry);
#else
            struct timespec retry;
            clock_gettime(CLOCK_MONOTONIC, &retry);
            retry.tv_nsec += kIdleGCRetryNanos;
            if (retry.tv_nsec >= 1000000000L) {
                retry.tv_sec += 1;
                retry.tv_nsec -= 1000000000L;
            }
            pthread_cond_clockwait(&cond, &lock, CLOCK_MONOTONIC, &retry);
#endif
        } else {
            pthread_cond_wait(&cond, &lock);
        }
        --sleepingThreads;
    }
}

uint32_t
BxTaskQueue::waitUntilIdle(BxThread* thread, BxTask* task) {
    if (task->getCanceled()) {
        return BxIdleWaitCanceled;
    }

    pthread_mutex_lock(&lock);
    bool supported = !workers && !hostedTask && !pendingHostRun && !activeHostWake &&
        thread->getTaskQueue() == this;
    if (supported) {
        idleWaiters.push_back(task);
    }
    pthread_mutex_unlock(&lock);
    if (!supported) {
        return BxIdleWaitUnsupported;
    }

    thread->suspendTask(task, NULL, BxTaskGCStateBlocked);

    pthread_mutex_lock(&lock);
    removeIdleWaiterLocked(task);
    pthread_mutex_unlock(&lock);
    return task->getCanceled() ? BxIdleWaitCanceled : BxIdleWaitIdle;
}

void
BxTaskQueue::removeIdleWaiterLocked(BxTask* task) {
    for (size_t index = idleWaiters.size(); index > 0; --index) {
        if (idleWaiters[index - 1] == task) {
            idleWaiters.erase(idleWaiters.begin() + static_cast<long>(index - 1));
            return;
        }
    }
}

void
BxTaskQueue::waitForPendingWorkLocked() {
    ++sleepingThreads;
#if defined(__APPLE__)
    struct timespec retry = { 0, kIdleWaitRetryNanos };
    pthread_cond_timedwait_relative_np(&cond, &lock, &retry);
#else
    struct timespec retry;
    clock_gettime(CLOCK_MONOTONIC, &retry);
    retry.tv_nsec += kIdleWaitRetryNanos;
    if (retry.tv_nsec >= 1000000000L) {
        retry.tv_sec += 1;
        retry.tv_nsec -= 1000000000L;
    }
    pthread_cond_clockwait(&cond, &lock, CLOCK_MONOTONIC, &retry);
#endif
    --sleepingThreads;
}

bool
BxTaskQueue::tryNextTask(BxTask** outTask) {
    pthread_mutex_lock(&lock);
    bool ok = takeNextTaskLocked(outTask);
    pthread_mutex_unlock(&lock);
    if (ok) {
        (*outTask)->startQueued();
    }
    return ok;
}

bool
BxTaskQueue::hasRunnableTasksLocked() {
    return !normalTaskQueue.empty() || !priorityTaskQueue.empty();
}

bool
BxTaskQueue::takeNextTaskLocked(BxTask** outTask) {
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
BxTaskQueue::nextPoolTask(BxTaskWorker* worker, BxTask** outTask, bool searching) {
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
BxTaskQueue::pushLocal(BxTaskWorker* worker, BxTask* task) {
    worker->lock.lock();
    if (task->getSchedulingClass() == BX_TASK_SCHEDULING_PRIORITY) {
        worker->priorityTasks.push(task);
    } else {
        worker->normalTasks.push(task);
    }
    worker->queued.fetch_add(1, std::memory_order_seq_cst);
    worker->lock.unlock();
}

bool
BxTaskQueue::popLocal(BxTaskWorker* worker, BxTask** outTask) {
    if (!worker->queued.load(std::memory_order_relaxed)) {
        return false;
    }

    BxTask* task = NULL;
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
BxTaskQueue::popInjected(BxTaskWorker* worker, BxTask** outTask) {
    if (!injectedCount.load(std::memory_order_relaxed)) {
        return false;
    }

    BxTask* batch[kTaskBatchLimit];
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
            BxTask* task = batch[i];
            if (task->getSchedulingClass() == BX_TASK_SCHEDULING_PRIORITY) {
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
BxTaskQueue::steal(BxTaskWorker* worker, BxTask** outTask) {
    worker->random ^= worker->random << 13;
    worker->random ^= worker->random >> 17;
    worker->random ^= worker->random << 5;
    int first = (int)(worker->random % (uint32_t)threadCount);

    for (int offset = 0; offset < threadCount; ++offset) {
        BxTaskWorker* victim = &workers[(first + offset) % threadCount];
        if (victim == worker || !victim->queued.load(std::memory_order_relaxed)) {
            continue;
        }

        BxTask* batch[kTaskBatchLimit];
        int count = 0;
        victim->lock.lock();
        int take = (victim->queued.load(std::memory_order_relaxed) + 1) / 2;
        if (take > kTaskBatchLimit) {
            take = kTaskBatchLimit;
        }
        while (count < take) {
            BxTask* task = victim->priorityTasks.pop();
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
                BxTask* task = batch[i];
                if (task->getSchedulingClass() == BX_TASK_SCHEDULING_PRIORITY) {
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
BxTaskQueue::hasPoolWork() {
    for (int i = 0; i < threadCount; ++i) {
        if (workers[i].queued.load(std::memory_order_seq_cst)) {
            return true;
        }
    }
    return false;
}

void
BxTaskQueue::wakeWorker() {
    if (!idleWorkerCount.load(std::memory_order_seq_cst)) {
        startWorker();
        return;
    }
    int expected = 0;
    if (!searchingWorkerCount.compare_exchange_strong(expected, 1, std::memory_order_seq_cst)) {
        return;
    }

    pthread_mutex_lock(&lock);
    BxTaskWorker* worker = idleWorkers;
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
BxTaskQueue::startWorker() {
    if (startedCount.load(std::memory_order_acquire) >= threadCount) {
        return;
    }
    int expected = 0;
    if (!searchingWorkerCount.compare_exchange_strong(expected, 1, std::memory_order_seq_cst)) {
        return;
    }

    pthread_mutex_lock(&lock);
    int index = startedCount.load(std::memory_order_relaxed);
    bool starts = index < threadCount && !isFinished.load(std::memory_order_acquire);
    if (starts) {
        sigset_t blocked;
        sigset_t previous;
        workerSignals(&blocked);
        pthread_sigmask(SIG_BLOCK, &blocked, &previous);
        int err = pthread_create(threads+index, 0, BxTaskQueueThread, (void*)this);
        pthread_sigmask(SIG_SETMASK, &previous, NULL);
        if (err) {
            printf("Unable to create thread (%d)\n", err);
            exit(-1);
        }
        startedCount.store(index + 1, std::memory_order_release);
    }
    pthread_mutex_unlock(&lock);

    if (!starts) {
        searchingWorkerCount.fetch_sub(1, std::memory_order_seq_cst);
    }
}

void
BxTaskQueue::wakeAllWorkers() {
    if (!workers) {
        return;
    }

    pthread_mutex_lock(&lock);
    while (BxTaskWorker* worker = idleWorkers) {
        idleWorkers = worker->nextIdle;
        worker->nextIdle = NULL;
        worker->parked = false;
        idleWorkerCount.fetch_sub(1, std::memory_order_seq_cst);
        searchingWorkerCount.fetch_add(1, std::memory_order_seq_cst);
        pthread_cond_signal(&worker->cond);
    }
    pthread_mutex_unlock(&lock);
}
