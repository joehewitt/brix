
#include <BxEvents/BxThread.h>

#include <BxGC/BxGCRuntime.h>
#include <BxEvents/BxGCHandshake.h>
#include <BxEvents/BxMutex.h>
#include <BxEvents/BxStack.h>
#include <BxEvents/BxTask.h>
#include <BxEvents/BxTaskQueue.h>
#include <BxEvents/BxTaskMonitor.h>

#include <stdlib.h>
#include <cstdio>
#include <cstring>

// *************************************************************************************************

extern "C" void
brixGCSetActiveThreadCache(BrixGCThreadCache* cache)
    __attribute__((weak));
extern "C" void
brixGCRuntimeFlushWriteBarrierBuffer(void) __attribute__((weak));
extern "C" void
brixGCRuntimeFlushActiveAllocationBuffer(void) __attribute__((weak));

static void
brixSetActiveGCThreadCacheIfAvailable(BrixGCThreadCache* cache) {
    if (brixGCSetActiveThreadCache) {
        brixGCSetActiveThreadCache(cache);
    }
}

static void
brixFlushActiveGCTaskStateIfAvailable() {
    if (brixGCRuntimeFlushWriteBarrierBuffer) {
        brixGCRuntimeFlushWriteBarrierBuffer();
    }
    if (brixGCRuntimeFlushActiveAllocationBuffer) {
        brixGCRuntimeFlushActiveAllocationBuffer();
    }
}

// *************************************************************************************************

BxThread::BxThread(BxTaskQueue* aQueue) :
    monitorSwitchCount(0),
    monitorSinceNanos(0),
    monitorWarned(false),
    queue(aQueue),
    worker(NULL),
    activeTask(NULL),
    suspendedTask(NULL),
    terminatedTask(NULL),
    suspendedMutex(NULL),
    suspendedGCState(BxTaskGCStateParked),
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
    brixInstallStackOverflowHandler();
    signalStack = brixInstallSignalStack();
    brixGCInitializeThreadCache(&gcThreadCache);
    brixContextGet(&context);
    brixGCRegisterThread(this);
    brixRegisterMonitoredThread(this);
}

BxThread::~BxThread() {
    brixUnregisterMonitoredThread(this);
    takeInterruptRequests(BX_THREAD_INTERRUPT_PREEMPT | BX_THREAD_INTERRUPT_WARN);
    bool taskless = activeTask == NULL;
    if (taskless) {
        // Keep teardown in the taskless-callback participant set until the thread-local root can
        // no longer be snapshotted. MarkSetup must not retain a base that teardown then frees.
        brixGCBeginThreadLocalCallback();
    }
    flushThreadLocals(false);
    brixSetActiveGCThreadCacheIfAvailable(NULL);

    if (threadLocals) {
#ifdef BX_ENABLE_GC
        GC_remove_roots(threadLocals, threadLocals + threadLocalsSize);
#endif
        free(threadLocals);
        threadLocals = NULL;
    }
    brixGCUnregisterThread(this);
    if (taskless) {
        brixGCEndThreadLocalCallback();
    }
    brixRemoveSignalStack(signalStack);
}

void
BxThread::initializeThreadLocals() {
    if (threadLocalsInitialized) {
        return;
    }

    threadLocalsSize = brixGetThreadLocalSize();
    BxThreadLocalFunc init = brixGetThreadLocalInit();
    BxThreadLocalFunc flush = brixGetThreadLocalFlush();
    if (!threadLocalsSize && !init && !flush) {
        return;
    }

    if (threadLocalsSize) {
        threadLocals = (char*)calloc(1, threadLocalsSize);
        if (!threadLocals) {
            fprintf(stderr, "Unable to allocate Brix thread local memory\n");
            abort();
        }
#ifdef BX_ENABLE_GC
        GC_add_roots(threadLocals, threadLocals + threadLocalsSize);
#endif
    }

    threadLocalsInitialized = true;

    if (init) {
        bool taskless = activeTask == NULL;
        if (taskless) {
            brixGCBeginThreadLocalCallback();
        }
        init();
        if (taskless) {
            brixGCEndThreadLocalCallback();
        }
    }
}

void
BxThread::flushThreadLocals(bool bracketTasklessCallback) {
    flushThreadLocalCallback(bracketTasklessCallback);

    // The callback can execute Brix code and publish pointer stores. Donate its last buffered
    // edge, and the pre-existing active compiler allocation range, before detaching the task.
    brixFlushActiveGCTaskStateIfAvailable();
}

void
BxThread::flushThreadLocalCallback(bool bracketTasklessCallback) {
    if (!hasThreadLocalFlush()) {
        return;
    }
    bool taskless = bracketTasklessCallback && activeTask == NULL;
    if (taskless) {
        brixGCBeginThreadLocalCallback();
    }
    brixGetThreadLocalFlush()();
    if (taskless) {
        brixGCEndThreadLocalCallback();
    }
}

bool
BxThread::hasThreadLocalFlush() const {
    return threadLocalsInitialized && brixGetThreadLocalFlush() != NULL;
}

void
BxThread::startTask(
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const BxTaskProvenance& provenance,
    BxTaskEnding** ending,
    uint64_t* handle) {
    BxTask* newTask = queue->createTask(func, closure, localSize, provenance);
    if (ending) {
        *ending = newTask->attachEnding();
    }
    if (handle) {
        *handle = newTask->ensureHandle();
    }
    queue->scheduleTask(newTask);
}

void
BxThread::finishTerminatedTask() {
    BxTask* task = terminatedTask;
    if (!task) {
        return;
    }

    terminatedTask = NULL;
    graveyard.push_back(task);
}

void
BxThread::deleteTerminatedTasks() {
    if (graveyard.empty() || brixProcessTerminationRequested()) {
        return;
    }

    size_t kept = 0;
    for (BxTask* task : graveyard) {
        if (task->isHandlePinned()) {
            graveyard[kept++] = task;
        } else {
            delete task;
        }
    }
    graveyard.resize(kept);
}

void
BxThread::finishSuspendedTask() {
    BxTask* task = suspendedTask;
    if (!task) {
        return;
    }

    BxMutex* mutex = suspendedMutex;
    uint32_t gcState = suspendedGCState;
    bool shouldSchedule = scheduleSuspendedTask;
    bool yielded = yieldSuspendedTask;

    suspendedTask = NULL;
    suspendedMutex = NULL;
    suspendedGCState = BxTaskGCStateParked;
    scheduleSuspendedTask = false;
    yieldSuspendedTask = false;

    if (!shouldSchedule) {
        brixGCSetTaskStateAndContext(task, gcState, true);
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
BxThread::suspendTask(
    BxTask* task,
    BxMutex* mutex,
    uint32_t gcState,
    bool scheduleAfterSuspend) {
    if (mutex) {
        brixFlushActiveGCTaskStateIfAvailable();
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
    brixSetActiveTaskRuntimeContext(NULL);

    brixContextSwap(task->getContext(), &context);
}

void
BxThread::yieldTask(BxTask* task) {
    yieldSuspendedTask = true;
    suspendTask(task, NULL, BxTaskGCStateParked, true);
}

void
BxThread::resumeTask(BxTask* task) {
    BxTask* previousTask = activeTask;
    if (task == previousTask) {
        return;
    }

    if (previousTask) {
        task->getTaskQueue()->scheduleTask(task, true);
        suspendTask(previousTask, NULL, BxTaskGCStateParked, true);
        return;
    }

    if (task->claimRunning()) {
        runTask(task);
    }
}

void
BxThread::runTask(BxTask* task) {
    brixGCPrepareTaskResume(task);
    brixGCRevalidateThreadAllocation(&allocationControl);
    if (brixContextEnsureStack(task->getContext()) != 0) {
        fprintf(stderr, "Unable to allocate Brix task stack\n");
        abort();
    }
    task->setActive(true);
    activeTask = task;
    brixSetActiveTaskRuntimeContext(task->getRuntimeContext());
    switchCount.store(switchCount.load(std::memory_order_relaxed) + 1, std::memory_order_seq_cst);
    brixNoteMonitoredThreadRunning();
    brixContextSwap(&context, activeTask->getContext());
    switchCount.store(switchCount.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    brixSetActiveTaskRuntimeContext(NULL);
    if (interruptRequests.load(std::memory_order_relaxed)) {
        uint32_t requests =
            takeInterruptRequests(BX_THREAD_INTERRUPT_PREEMPT | BX_THREAD_INTERRUPT_WARN);
        if (requests & BX_THREAD_INTERRUPT_WARN) {
            brixReportLongTask(this, task, false);
        }
    }
    finishTerminatedTask();
    finishSuspendedTask();
}

bool
BxThread::requestInterrupt(uint32_t kind) {
    uint32_t previous = interruptRequests.fetch_or(kind, std::memory_order_acq_rel);
    if (!previous) {
        brixInterruptRequestAdded();
    }
    return !(previous & kind);
}

uint32_t
BxThread::takeInterruptRequests(uint32_t kinds) {
    uint32_t previous = interruptRequests.fetch_and(~kinds, std::memory_order_acq_rel);
    if (previous && !(previous & ~kinds)) {
        brixInterruptRequestRemoved();
    }
    return previous & kinds;
}

void
BxThread::taskTerminated(BxTask* task) {
    task->releaseHandle();
    if (task == activeTask) {
        // Dead is a handshake-stable state. Donate this task's last thread-owned pointer
        // publications before exposing the state change or detaching the cache owner.
        brixFlushActiveGCTaskStateIfAvailable();
        brixGCSetTaskStateAndContext(task, BxTaskGCStateDead, false);
        task->setActive(false);
        activeTask = NULL;
        brixSetActiveTaskRuntimeContext(NULL);
    }

    queue->taskTerminated(task);
    terminatedTask = task;
    task->terminate();
    brixContextSet(&context);
}
