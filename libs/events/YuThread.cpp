
#include <YuEvents/YuThread.h>

#include <YuGC/YuGCRuntime.h>
#include <YuEvents/YuGCHandshake.h>
#include <YuEvents/YuMutex.h>
#include <YuEvents/YuStack.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuTaskQueue.h>
#include <YuEvents/YuTaskMonitor.h>

#include <stdlib.h>
#include <cstdio>
#include <cstring>

// *************************************************************************************************

extern "C" void
yumiGCSetActiveThreadCache(YumiGCThreadCache* cache)
    __attribute__((weak));
extern "C" void
yumiGCRuntimeFlushWriteBarrierBuffer(void) __attribute__((weak));
extern "C" void
yumiGCRuntimeFlushActiveAllocationBuffer(void) __attribute__((weak));

static void
yumiSetActiveGCThreadCacheIfAvailable(YumiGCThreadCache* cache) {
    if (yumiGCSetActiveThreadCache) {
        yumiGCSetActiveThreadCache(cache);
    }
}

static void
yumiFlushActiveGCTaskStateIfAvailable() {
    if (yumiGCRuntimeFlushWriteBarrierBuffer) {
        yumiGCRuntimeFlushWriteBarrierBuffer();
    }
    if (yumiGCRuntimeFlushActiveAllocationBuffer) {
        yumiGCRuntimeFlushActiveAllocationBuffer();
    }
}

// *************************************************************************************************

YuThread::YuThread(YuTaskQueue* aQueue) :
    monitorSwitchCount(0),
    monitorSinceNanos(0),
    monitorWarned(false),
    queue(aQueue),
    worker(NULL),
    activeTask(NULL),
    suspendedTask(NULL),
    terminatedTask(NULL),
    suspendedMutex(NULL),
    suspendedGCState(YuTaskGCStateParked),
    scheduleSuspendedTask(false),
    yieldSuspendedTask(false),
    switchCount(0),
    interruptRequests(0),
    allocationControl(0),
    threadLocals(NULL),
    threadLocalsSize(0),
    threadLocalsInitialized(false),
    gcThreadCache({}),
    signalStack(NULL)
{
    yumiInstallStackOverflowHandler();
    signalStack = yumiInstallSignalStack();
    yumiGCInitializeThreadCache(&gcThreadCache);
    yumiContextGet(&context);
    yumiGCRegisterThread(this);
    yumiRegisterMonitoredThread(this);
}

YuThread::~YuThread() {
    yumiUnregisterMonitoredThread(this);
    takeInterruptRequests(YU_THREAD_INTERRUPT_PREEMPT | YU_THREAD_INTERRUPT_WARN);
    bool taskless = activeTask == NULL;
    if (taskless) {
        // Keep teardown in the taskless-callback participant set until the thread-local root can
        // no longer be snapshotted. MarkSetup must not retain a base that teardown then frees.
        yumiGCBeginThreadLocalCallback(this);
    }
    flushThreadLocals(false);
    yumiSetActiveGCThreadCacheIfAvailable(NULL);

    if (threadLocals) {
#ifdef YU_ENABLE_GC
        GC_remove_roots(threadLocals, threadLocals + threadLocalsSize);
#endif
        free(threadLocals);
        threadLocals = NULL;
    }
    yumiGCUnregisterThread(this);
    if (taskless) {
        yumiGCEndThreadLocalCallback(this);
    }
    yumiRemoveSignalStack(signalStack);
}

void
YuThread::initializeThreadLocals() {
    if (threadLocalsInitialized) {
        return;
    }

    threadLocalsSize = yumiGetThreadLocalSize();
    YuThreadLocalFunc init = yumiGetThreadLocalInit();
    YuThreadLocalFunc flush = yumiGetThreadLocalFlush();
    if (!threadLocalsSize && !init && !flush) {
        return;
    }

    if (threadLocalsSize) {
        threadLocals = (char*)calloc(1, threadLocalsSize);
        if (!threadLocals) {
            fprintf(stderr, "Unable to allocate Yumi thread local memory\n");
            abort();
        }
#ifdef YU_ENABLE_GC
        GC_add_roots(threadLocals, threadLocals + threadLocalsSize);
#endif
    }

    threadLocalsInitialized = true;

    if (init) {
        bool taskless = activeTask == NULL;
        if (taskless) {
            yumiGCBeginThreadLocalCallback(this);
        }
        init();
        if (taskless) {
            yumiGCEndThreadLocalCallback(this);
        }
    }
}

void
YuThread::flushThreadLocals(bool bracketTasklessCallback) {
    flushThreadLocalCallback(bracketTasklessCallback);

    // The callback can execute Yumi code and publish pointer stores. Donate its last buffered
    // edge, and the pre-existing active compiler allocation range, before detaching the task.
    yumiFlushActiveGCTaskStateIfAvailable();
}

void
YuThread::flushThreadLocalCallback(bool bracketTasklessCallback) {
    if (!hasThreadLocalFlush()) {
        return;
    }
    bool taskless = bracketTasklessCallback && activeTask == NULL;
    if (taskless) {
        yumiGCBeginThreadLocalCallback(this);
    }
    yumiGetThreadLocalFlush()();
    if (taskless) {
        yumiGCEndThreadLocalCallback(this);
    }
}

bool
YuThread::hasThreadLocalFlush() const {
    return threadLocalsInitialized && yumiGetThreadLocalFlush() != NULL;
}

void
YuThread::startTask(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const YuTaskProvenance& provenance,
    YuTaskEnding** ending,
    uint64_t* handle) {
    YuTask* newTask = queue->createTask(func, closure, localSize, provenance);
    if (ending) {
        *ending = newTask->attachEnding();
    }
    if (handle) {
        *handle = newTask->ensureHandle();
    }
    queue->scheduleTask(newTask);
}

void
YuThread::finishTerminatedTask() {
    YuTask* task = terminatedTask;
    if (!task) {
        return;
    }

    terminatedTask = NULL;
    graveyard.push_back(task);
}

void
YuThread::deleteTerminatedTasks() {
    if (graveyard.empty() || yumiProcessTerminationRequested()) {
        return;
    }

    size_t kept = 0;
    for (YuTask* task : graveyard) {
        if (task->isHandlePinned()) {
            graveyard[kept++] = task;
        } else {
            delete task;
        }
    }
    graveyard.resize(kept);
}

void
YuThread::finishSuspendedTask() {
    YuTask* task = suspendedTask;
    if (!task) {
        return;
    }

    YuMutex* mutex = suspendedMutex;
    uint32_t gcState = suspendedGCState;
    bool shouldSchedule = scheduleSuspendedTask;
    bool yielded = yieldSuspendedTask;

    suspendedTask = NULL;
    suspendedMutex = NULL;
    suspendedGCState = YuTaskGCStateParked;
    scheduleSuspendedTask = false;
    yieldSuspendedTask = false;

    if (!shouldSchedule) {
        yumiGCSetTaskStateAndContext(task, gcState, true);
    }
    task->pin();
    bool parked = !task->finishRunning(shouldSchedule);
    if (!parked) {
        task->getTaskQueue()->enqueueClaimedTask(task, yielded);
    }

    if (mutex) {
        mutex->unlock();
    }
    if (parked) {
        task->getTaskQueue()->wakeCanceledTask(task);
    }
    task->unpinHandle();
}

void
YuThread::suspendTask(
    YuTask* task,
    YuMutex* mutex,
    uint32_t gcState,
    bool scheduleAfterSuspend) {
    if (mutex) {
        yumiFlushActiveGCTaskStateIfAvailable();
    } else {
        flushThreadLocals();
    }

    if (task != activeTask) {
        fprintf(stderr, "Attempted to suspend a task that is not active on this thread\n");
        abort();
    }

    suspendedTask = task;
    suspendedMutex = mutex;
    suspendedGCState = gcState;
    scheduleSuspendedTask = scheduleAfterSuspend;
    // Running remains visible until the saved continuation is complete. The final
    // Parked/Blocked state is published only after the context switch returns here.
    task->setGCContextReady(false);
    task->setActive(false);
    activeTask = NULL;
    yumiSetActiveTaskRuntimeContext(NULL);

    yumiContextSwap(task->getContext(), &context);
}

void
YuThread::yieldTask(YuTask* task) {
    yieldSuspendedTask = true;
    suspendTask(task, NULL, YuTaskGCStateParked, true);
}

void
YuThread::resumeTask(YuTask* task) {
    YuTask* previousTask = activeTask;
    if (task == previousTask) {
        return;
    }

    if (previousTask) {
        task->getTaskQueue()->scheduleTask(task, true);
        suspendTask(previousTask, NULL, YuTaskGCStateParked, true);
        return;
    }

    if (task->claimRunning()) {
        runTask(task);
    }
}

void
YuThread::runTask(YuTask* task) {
    yumiGCPrepareTaskResume(task);
    yumiGCRevalidateThreadAllocation(&allocationControl);
    if (yumiContextEnsureStack(task->getContext()) != 0) {
        fprintf(stderr, "Unable to allocate Yumi task stack\n");
        abort();
    }
    task->setActive(true);
    activeTask = task;
    yumiSetActiveTaskRuntimeContext(task->getRuntimeContext());
    switchCount.store(switchCount.load(std::memory_order_relaxed) + 1, std::memory_order_seq_cst);
    yumiNoteMonitoredThreadRunning();
    yumiContextSwap(&context, activeTask->getContext());
    switchCount.store(switchCount.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    yumiSetActiveTaskRuntimeContext(NULL);
    if (interruptRequests.load(std::memory_order_relaxed)) {
        uint32_t requests =
            takeInterruptRequests(YU_THREAD_INTERRUPT_PREEMPT | YU_THREAD_INTERRUPT_WARN);
        if (requests & YU_THREAD_INTERRUPT_WARN) {
            yumiReportLongTask(this, task, false);
        }
    }
    finishTerminatedTask();
    finishSuspendedTask();
}

bool
YuThread::requestInterrupt(uint32_t kind) {
    uint32_t previous = interruptRequests.fetch_or(kind, std::memory_order_acq_rel);
    if (!previous) {
        yumiInterruptRequestAdded();
    }
    return !(previous & kind);
}

uint32_t
YuThread::takeInterruptRequests(uint32_t kinds) {
    uint32_t previous = interruptRequests.fetch_and(~kinds, std::memory_order_acq_rel);
    if (previous && !(previous & ~kinds)) {
        yumiInterruptRequestRemoved();
    }
    return previous & kinds;
}

void
YuThread::taskTerminated(YuTask* task) {
    task->releaseHandle();
    if (task == activeTask) {
        // Dead is a handshake-stable state. Donate this task's last thread-owned pointer
        // publications before exposing the state change or detaching the cache owner.
        yumiFlushActiveGCTaskStateIfAvailable();
        yumiGCSetTaskStateAndContext(task, YuTaskGCStateDead, false);
        task->setActive(false);
        activeTask = NULL;
        yumiSetActiveTaskRuntimeContext(NULL);
    }

    queue->taskTerminated(task);
    terminatedTask = task;
    task->terminate();
    yumiContextSet(&context);
}
