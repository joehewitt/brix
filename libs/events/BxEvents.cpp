
#include <BxEvents/BxEvents.h>
#include <BxEvents/BxThread.h>
#include <BxEvents/BxTask.h>
#include <BxEvents/BxSemaphore.h>
#include <BxEvents/BxMutex.h>
#include <BxEvents/BxTaskMutex.h>
#include <BxEvents/BxTaskMonitor.h>
#include <BxEvents/BxTaskQueue.h>
#include <BxEvents/BxGCHandshake.h>
#include <BxEvents/BxPoll.h>
#include <BxCore/BxCore.h>
#include <BxGC/BxGC.h>
#include <BxGC/BxGCRuntime.h>

#include <stdlib.h>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/param.h>
#include <stdint.h>
#include <unordered_map>
// #include <gc.h>

#if defined(__APPLE__)
#include <BxEvents/BxKQueueLoop.h>
#include <BxEvents/BxSelectLoop.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
typedef BxKQueueLoop BxPollLoop;
#else
#include <BxEvents/BxEpollLoop.h>
#include <ctime>
typedef BxEpollLoop BxPollLoop;
#endif

// *************************************************************************************************

static pthread_key_t gActiveThreadKey = 0;
static pthread_mutex_t gActiveThreadKeyLock = PTHREAD_MUTEX_INITIALIZER;
static std::atomic<bool> gActiveThreadKeyCreated(false);
static thread_local BxTaskRuntimeContext* gActiveTaskRuntimeContext = NULL;
static thread_local BxTaskRuntimeContext gFallbackTaskRuntimeContext = {};
static thread_local size_t gFallbackTaskGCAssistDebt = 0;
static thread_local size_t gFallbackTaskGCAssistDebtCycle = 0;

static BxTaskQueue* gMainQueue = NULL;
static BxTaskQueue* gSpinQueue = NULL;
static BxPollLoop* gPollLoop = NULL;
#if defined(__APPLE__)
static BxSelectLoop* gSelectLoop = NULL;
#endif
static size_t gTaskLocalSize = 0;
static BxTaskLocalFunc gTaskLocalInit = NULL;
static size_t gThreadLocalSize = 0;
static BxThreadLocalFunc gThreadLocalInit = NULL;
static BxThreadLocalFunc gThreadLocalFlush = NULL;
static std::atomic<bool> gProcessTerminationRequested(false);
static std::atomic<int> gProcessExitCode(0);

static std::atomic<uint64_t> gTimerEventIdCounter(1);
static pthread_mutex_t gTimerEventIdsLock = PTHREAD_MUTEX_INITIALIZER;
static std::unordered_map<BxSemaphore*, uint64_t> gTimerEventIds;
#if defined(__APPLE__)
static pthread_once_t gMachTimebaseOnce = PTHREAD_ONCE_INIT;
static mach_timebase_info_data_t gMachTimebaseInfo = {0, 0};
#endif

// *************************************************************************************************

extern "C" void
brixGCSetActiveThreadCache(BrixGCThreadCache* cache)
    __attribute__((weak));

static void
brixSetActiveGCThreadCache(BrixGCThreadCache* cache) {
    if (brixGCSetActiveThreadCache) {
        brixGCSetActiveThreadCache(cache);
    }
}

// *************************************************************************************************

static void
brixEnsureActiveThreadKey() {
    if (gActiveThreadKeyCreated.load(std::memory_order_acquire)) {
        return;
    }

    pthread_mutex_lock(&gActiveThreadKeyLock);
    if (!gActiveThreadKeyCreated.load(std::memory_order_relaxed)) {
        pthread_key_create(&gActiveThreadKey, NULL);
        gActiveThreadKeyCreated.store(true, std::memory_order_release);
    }
    pthread_mutex_unlock(&gActiveThreadKeyLock);
}

#if defined(__APPLE__)

static int
getCPUCount() {
    int mib[4];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;

    int cpuCount;
    size_t len = sizeof(cpuCount);
    if (sysctl(mib, 2, &cpuCount, &len, NULL, 0) != 0 || cpuCount < 1) {
        return 1;
    }

    return cpuCount;
}

static void
brixInitMachTimebase() {
    mach_timebase_info(&gMachTimebaseInfo);
    if (!gMachTimebaseInfo.numer || !gMachTimebaseInfo.denom) {
        gMachTimebaseInfo.numer = 1;
        gMachTimebaseInfo.denom = 1;
    }
}

static mach_timebase_info_data_t
brixMachTimebaseInfo() {
    pthread_once(&gMachTimebaseOnce, brixInitMachTimebase);
    return gMachTimebaseInfo;
}

static uint64_t
brixMachTimeToNanoseconds(uint64_t machTime) {
    mach_timebase_info_data_t info = brixMachTimebaseInfo();
    return (uint64_t)(((__uint128_t)machTime * info.numer) / info.denom);
}

#else

static int
cgroupCPULimit() {
    FILE* membership = fopen("/proc/self/cgroup", "r");
    if (!membership) {
        return 0;
    }
    char line[512];
    char path[600] = "";
    while (fgets(line, sizeof(line), membership)) {
        if (strncmp(line, "0::", 3) == 0) {
            line[strcspn(line, "\n")] = 0;
            snprintf(path, sizeof(path), "/sys/fs/cgroup%s/cpu.max", line + 3);
            break;
        }
    }
    fclose(membership);

    FILE* limit = path[0] ? fopen(path, "r") : NULL;
    if (!limit) {
        return 0;
    }
    char quota[32] = "";
    unsigned long long period = 0;
    int count = 0;
    if (fscanf(limit, "%31s %llu", quota, &period) == 2 && strcmp(quota, "max") != 0 && period > 0) {
        unsigned long long cpus = (strtoull(quota, NULL, 10) + period - 1) / period;
        count = cpus > 0 && cpus < 0x10000 ? (int)cpus : 0;
    }
    fclose(limit);
    return count;
}

static int
getCPUCount() {
    cpu_set_t set;
    int count = sched_getaffinity(0, sizeof(set), &set) == 0 ? CPU_COUNT(&set) : 0;
    if (count < 1) {
        long online = sysconf(_SC_NPROCESSORS_ONLN);
        count = online > 0 ? (int)online : 1;
    }
    int limit = cgroupCPULimit();
    return limit > 0 && limit < count ? limit : count;
}

#endif

static uint64_t
brixRegisterTimerEvent(BxSemaphore* sem) {
    uint64_t id = gTimerEventIdCounter.fetch_add(1, std::memory_order_relaxed);
    if (!id) {
        id = gTimerEventIdCounter.fetch_add(1, std::memory_order_relaxed);
    }

    if (sem) {
        pthread_mutex_lock(&gTimerEventIdsLock);
        gTimerEventIds[sem] = id;
        pthread_mutex_unlock(&gTimerEventIdsLock);
    }

    return id;
}

static void
brixClearTimerEvent(BxSemaphore* sem, uint64_t id) {
    if (!sem) {
        return;
    }

    pthread_mutex_lock(&gTimerEventIdsLock);
    std::unordered_map<BxSemaphore*, uint64_t>::iterator it = gTimerEventIds.find(sem);
    if (it != gTimerEventIds.end() && it->second == id) {
        gTimerEventIds.erase(it);
    }
    pthread_mutex_unlock(&gTimerEventIdsLock);
}

static uint64_t
brixTimerEventIdForSemaphore(BxSemaphore* sem) {
    if (!sem) {
        return 0;
    }

    uint64_t id = 0;
    pthread_mutex_lock(&gTimerEventIdsLock);
    std::unordered_map<BxSemaphore*, uint64_t>::iterator it = gTimerEventIds.find(sem);
    if (it != gTimerEventIds.end()) {
        id = it->second;
    }
    pthread_mutex_unlock(&gTimerEventIdsLock);
    return id;
}

static BxTaskProvenance
brixCreateTaskProvenance(
    const char* file,
    uint32_t line,
    uint32_t column,
    uint32_t flags,
    uint32_t schedulingClass = BX_TASK_SCHEDULING_NORMAL) {
    BxThread* thread = brixGetActiveThread();
    BxTask* parent = thread ? thread->getActiveTask() : NULL;
    uint32_t parentId = parent ? parent->getId() : 0;
    return BxTaskProvenance(parentId, file, line, column, flags, schedulingClass);
}

// *************************************************************************************************

BxThread*
brixGetActiveThread() {
    if (!gActiveThreadKeyCreated.load(std::memory_order_acquire)) {
        return NULL;
    }
    return (BxThread*)pthread_getspecific(gActiveThreadKey);
}

void
brixSetActiveThread(BxThread* thread) {
    brixEnsureActiveThreadKey();
    pthread_setspecific(gActiveThreadKey, (void*)thread);
    brixSetActiveGCThreadCache(thread ? thread->getGCThreadCache() : NULL);
}

extern "C" BxTaskRuntimeContext*
brixGetActiveTaskRuntimeContext() {
    return gActiveTaskRuntimeContext
        ? gActiveTaskRuntimeContext
        : &gFallbackTaskRuntimeContext;
}

extern "C" size_t
brixGCActiveTaskAssistDebt() {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : nullptr;
    return task ? task->getGCAssistDebt() : gFallbackTaskGCAssistDebt;
}

extern "C" size_t
brixGCActiveTaskAssistDebtCycle() {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : nullptr;
    return task ? task->getGCAssistDebtCycle() : gFallbackTaskGCAssistDebtCycle;
}

extern "C" void
brixGCSetActiveTaskAssistDebt(size_t debt) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : nullptr;
    if (task) {
        task->setGCAssistDebt(debt);
    } else {
        gFallbackTaskGCAssistDebt = debt;
    }
}

extern "C" void
brixGCSetActiveTaskAssistDebtCycle(size_t cycle) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : nullptr;
    if (task) {
        task->setGCAssistDebtCycle(cycle);
    } else {
        gFallbackTaskGCAssistDebtCycle = cycle;
    }
}

extern "C" void
brixSetActiveTaskRuntimeContext(BxTaskRuntimeContext* context) {
    gActiveTaskRuntimeContext = context;
}

size_t
brixGetThreadLocalSize() {
    return gThreadLocalSize;
}

BxThreadLocalFunc
brixGetThreadLocalInit() {
    return gThreadLocalInit;
}

BxThreadLocalFunc
brixGetThreadLocalFlush() {
    return gThreadLocalFlush;
}

// *************************************************************************************************

extern "C" void
brixRunEventLoop(
    BxTaskStartFunc starter,
    BxTaskStartFunc closer,
    void* closure,
    size_t localSize,
    BxThreadLocalFunc threadLocalInit,
    BxThreadLocalFunc threadLocalFlush,
    size_t threadLocalSize,
    BxTaskLocalFunc taskLocalInit) {
    register BxTaskRuntimeContext* runtimeContext asm("x27") =
        brixGetActiveTaskRuntimeContext();
    asm volatile ("" : "+r" (runtimeContext));

#if BX_ENABLE_GC
    GC_INIT();
    // GC_set_find_leak(1);
    // GC_set_all_interior_pointers(1);
    // GC_set_max_heap_size(1024 * 1024 * 100);
    // GC_set_free_space_divisor(4);
    // GC_enable_incremental();
#endif

    brixEnsureActiveThreadKey();
    gProcessTerminationRequested.store(false, std::memory_order_release);
    gProcessExitCode.store(0, std::memory_order_release);
    __atomic_fetch_and(
        &brixInterruptRequestedFlags,
        static_cast<unsigned char>(~BrixInterruptTerminationRequested),
        __ATOMIC_RELEASE);
    __atomic_fetch_and(
        &brixGCPublicControlWord,
        ~static_cast<uint64_t>(BrixGCControlTerminationRequestedBit),
        __ATOMIC_RELEASE);
    gThreadLocalSize = threadLocalSize;
    gTaskLocalSize = localSize;
    gTaskLocalInit = taskLocalInit;
    gThreadLocalInit = threadLocalInit;
    gThreadLocalFlush = threadLocalFlush;

    brixStartTaskMonitor();

    BxPollLoop pollLoop;
    gPollLoop = &pollLoop;
    pollLoop.start();

#if defined(__APPLE__)
    BxSelectLoop selectLoop;
    gSelectLoop = &selectLoop;
    selectLoop.start();
#endif

    BxTaskQueue mainQueue(NULL, 0);
    gMainQueue = &mainQueue;

    int spinThreadCount = getCPUCount() - 3;
    if (spinThreadCount < 1) {
        spinThreadCount = 1;
    }
    gSpinQueue = mainQueue.createQueue(spinThreadCount);

    // starter(closure);
    BxTaskProvenance mainProvenance(0, NULL, 0, 0, BX_TASK_FLAG_MAIN);
    mainQueue.startTask(starter, closure, localSize, mainProvenance);
    mainQueue.runEventLoop();

    if (closer) {
        mainQueue.startTask(closer, closure, localSize, mainProvenance);
        mainQueue.runEventLoop();
    }

    gSpinQueue->stop();
    gSpinQueue->join();
    delete gSpinQueue;

#if defined(__APPLE__)
    selectLoop.stop();
    selectLoop.join();
    gSelectLoop = NULL;
#endif

    pollLoop.stop();
    pollLoop.join();
    gPollLoop = NULL;

    brixStopTaskMonitor();

#if BX_ENABLE_GC
    // Arbitrary number of collection loops to try and get all the
    // finalizers - this is not the final solution obviously
    for (int i = 0; i < 10; ++i) {
        GC_gcollect();
    }
    GC_invoke_finalizers();
#endif

    gMainQueue = NULL;
    gTaskLocalSize = 0;
    gTaskLocalInit = NULL;

    asm volatile ("" : "+r" (runtimeContext));
    // printf("done.\n");
}

static const unsigned int kProcessTerminationGraceSeconds = 5;

static void*
brixForceProcessExit(void*) {
    sleep(kProcessTerminationGraceSeconds);
    _Exit(gProcessExitCode.load(std::memory_order_acquire));
    return NULL;
}

extern "C" bool
brixRequestProcessTermination(int exitCode) {
    bool wasRequested = gProcessTerminationRequested.exchange(true, std::memory_order_acq_rel);
    if (wasRequested) {
        return false;
    }

    gProcessExitCode.store(exitCode, std::memory_order_release);
    pthread_t watchdog;
    if (pthread_create(&watchdog, NULL, brixForceProcessExit, NULL) == 0) {
        pthread_detach(watchdog);
    }
    __atomic_fetch_or(
        &brixInterruptRequestedFlags,
        BrixInterruptTerminationRequested,
        __ATOMIC_RELEASE);
    __atomic_fetch_or(
        &brixGCPublicControlWord,
        static_cast<uint64_t>(BrixGCControlTerminationRequestedBit),
        __ATOMIC_RELEASE);
    if (gMainQueue) {
        gMainQueue->requestTermination();
    }
    return true;
}

extern "C" bool
brixProcessTerminationRequested() {
    return gProcessTerminationRequested.load(std::memory_order_acquire);
}

extern "C" int
brixGetProcessExitCode() {
    return gProcessExitCode.load(std::memory_order_acquire);
}

static int gTerminationSignalPipe[2] = {-1, -1};
static const int kTerminationSignals[] = {SIGHUP, SIGTERM};

static void
brixTerminationSignalHandler(int number) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigaction(number, &action, NULL);

    unsigned char received = static_cast<unsigned char>(number);
    ssize_t written = write(gTerminationSignalPipe[1], &received, 1);
    (void)written;
}

static void*
brixTerminationSignalWatch(void*) {
    unsigned char received = 0;
    ssize_t count;
    do {
        count = read(gTerminationSignalPipe[0], &received, 1);
    } while (count < 0 && errno == EINTR);
    if (count == 1) {
        brixRequestProcessTermination(128 + received);
    }
    return NULL;
}

extern "C" void
brixRequestTerminationOnSignals() {
    if (gTerminationSignalPipe[0] >= 0 || pipe(gTerminationSignalPipe) != 0) {
        return;
    }
    for (int descriptor : gTerminationSignalPipe) {
        fcntl(descriptor, F_SETFD, FD_CLOEXEC);
    }

    pthread_t watcher;
    if (pthread_create(&watcher, NULL, brixTerminationSignalWatch, NULL) != 0) {
        return;
    }
    pthread_detach(watcher);

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = brixTerminationSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    for (int number : kTerminationSignals) {
        sigaction(number, &action, NULL);
    }
}

extern "C" bool
brixRunMainTaskQueueHost(
    BxMainTaskQueueHostRunFunc run,
    BxMainTaskQueueHostWakeFunc wake,
    void* context) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : NULL;
    BxTaskQueue* mainQueue = gMainQueue;
    if (!mainQueue || !thread || !task || thread->getTaskQueue() != mainQueue) {
        return false;
    }
    if (!mainQueue->requestHost(task, run, wake, context)) {
        return false;
    }

    thread->suspendTask(task, NULL, BxTaskGCStateParked);
    return !task->getCanceled();
}

struct BxNativeTaskStart {
    BxTaskStartFunc func;
    void* closure;
};

static void
startNativeTask(void* closure) {
    BxNativeTaskStart* start = static_cast<BxNativeTaskStart*>(closure);
    BxTaskStartFunc func = start->func;
    void* funcClosure = start->closure;
    delete start;
    gTaskLocalInit();
    func(funcClosure);
}

extern "C" bool
brixScheduleMainTaskFromNative(BxTaskStartFunc func, void* closure) {
    BxTaskQueue* mainQueue = gMainQueue;
    if (!mainQueue || !func || brixProcessTerminationRequested()) {
        return false;
    }
    if (!gTaskLocalInit) {
        return mainQueue->startTaskIfRunning(func, closure, gTaskLocalSize);
    }
    BxNativeTaskStart* start = new BxNativeTaskStart{func, closure};
    if (!mainQueue->startTaskIfRunning(startNativeTask, start, gTaskLocalSize)) {
        delete start;
        return false;
    }
    return true;
}

struct BxTaskFinishCallback {
    BxTaskStartFunc callback;
    void* context;
    void* contextRoot;
};

static void
releaseTaskFinishCallback(BxTaskFinishCallback* record) {
    if (record->contextRoot) {
        brixRemoveRoot(record->contextRoot);
    }
    delete record;
}

static void
runTaskFinishCallback(void* closure) {
    BxTaskFinishCallback* record = static_cast<BxTaskFinishCallback*>(closure);
    record->callback(record->context);
    releaseTaskFinishCallback(record);
    brixTerminateActiveTask();
}

static void
scheduleTaskFinishCallback(void* closure) {
    BxTaskFinishCallback* record = static_cast<BxTaskFinishCallback*>(closure);
    if (!brixScheduleMainTaskFromNative(runTaskFinishCallback, record)) {
        releaseTaskFinishCallback(record);
    }
}

extern "C" void
brixScheduleMainTaskWhenTaskFinishes(uint64_t handle, BxTaskStartFunc callback, void* context) {
    BxTaskFinishCallback* record = new BxTaskFinishCallback{
        callback,
        context,
        context ? brixAddRoot(static_cast<char*>(context)) : NULL
    };
    if (!BxTask::addFinishHook(handle, scheduleTaskFinishCallback, record)) {
        scheduleTaskFinishCallback(record);
    }
}

extern "C" uint32_t
brixRunMainTaskFromNativeUntilDeadline(
    BxTaskStartFunc func, void* closure, uint64_t deadlineNanos) {
    BxThread* thread = brixGetActiveThread();
    BxTaskQueue* mainQueue = gMainQueue;
    if (!mainQueue || !thread || !func || brixProcessTerminationRequested()) {
        return BxMainTaskDeadlineFailed;
    }
    if (!gTaskLocalInit) {
        return mainQueue->runHostedTaskUntilDeadline(
            thread, func, closure, gTaskLocalSize, deadlineNanos);
    }
    BxNativeTaskStart* start = new BxNativeTaskStart{func, closure};
    uint32_t result = mainQueue->runHostedTaskUntilDeadline(
        thread, startNativeTask, start, gTaskLocalSize, deadlineNanos);
    if (result == BxMainTaskDeadlineFailed) {
        delete start;
    }
    return result;
}

extern "C" uint32_t
brixDrainMainTaskQueue(size_t taskBudget, uint64_t deadlineNanos) {
    BxThread* thread = brixGetActiveThread();
    BxTaskQueue* mainQueue = gMainQueue;
    if (!mainQueue || !thread || !taskBudget) {
        return BxTaskQueueDrainIdle;
    }
    return mainQueue->drainHostedTasks(thread, taskBudget, deadlineNanos);
}

extern "C" void
brixForkTask(BxTaskStartFunc func, void* closure, size_t localSize) {
    if (brixProcessTerminationRequested()) {
        return;
    }

    BxThread* thread = brixGetActiveThread();
    if (thread) {
        thread->startTask(func, closure, localSize);
    }
}

extern "C" void
brixForkMainTask(BxTaskStartFunc func, void* closure, size_t localSize) {
    if (brixProcessTerminationRequested()) {
        return;
    }

    BxThread* thread = brixGetActiveThread();
    if (thread) {
        thread->startTask(func, closure, localSize, BxTaskProvenance(0, NULL, 0, 0, BX_TASK_FLAG_MAIN));
    }
}

static BxTaskEnding*
unstartedEnding() {
    return new BxTaskEnding(0, 1, BX_TASK_END_CANCELLED);
}

static uint64_t
startTask(
    BxTaskQueue* queue,
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column,
    uint32_t options,
    uint64_t* handle) {
    bool wantsEnding = options & BX_TASK_START_HANDLE;
    if (brixProcessTerminationRequested()) {
        return wantsEnding ? reinterpret_cast<uint64_t>(unstartedEnding()) : 0;
    }

    uint32_t flags = 0;
    if (options & BX_TASK_START_SPIN) {
        flags |= BX_TASK_FLAG_SPIN;
    }
    if (options & BX_TASK_START_LARGE_STACK) {
        flags |= BX_TASK_FLAG_LARGE_STACK;
    }
    if (queue) {
        flags |= BX_TASK_FLAG_WORKER;
    }
    uint32_t schedulingClass =
        options & BX_TASK_START_PRIORITY ? BX_TASK_SCHEDULING_PRIORITY : BX_TASK_SCHEDULING_NORMAL;
    BxTaskProvenance provenance =
        brixCreateTaskProvenance(file, line, column, flags, schedulingClass);

    BxTaskEnding* ending = NULL;
    BxTaskEnding** wanted = wantsEnding ? &ending : nullptr;
    BxThread* thread = brixGetActiveThread();
    if (queue) {
        if (queue->isStopped()) {
            return wantsEnding ? reinterpret_cast<uint64_t>(unstartedEnding()) : 0;
        }
        queue->startTask(func, closure, localSize, provenance, wanted, handle);
    } else if (options & BX_TASK_START_SPIN) {
        gSpinQueue->startTask(func, closure, localSize, provenance, wanted, handle);
    } else if (thread) {
        thread->startTask(func, closure, localSize, provenance, wanted, handle);
    } else {
        return wantsEnding ? reinterpret_cast<uint64_t>(unstartedEnding()) : 0;
    }
    return wantsEnding ? reinterpret_cast<uint64_t>(ending) : 1;
}

extern "C" uint64_t
brixStartTask(
    BxTaskQueue* queue,
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column,
    uint32_t options) {
    return startTask(queue, func, closure, localSize, file, line, column, options, nullptr);
}

struct BxTaskOwner {
    BxMutex lock;
    std::vector<uint64_t> handles;
    size_t pruneAt = 8;
};

static size_t
deleteTaskOwner(void* context) {
    delete static_cast<BxTaskOwner*>(context);
    return 1;
}

static BxTaskOwner*
ownedTasksOf(char* owner) {
    char* holder = brixGCObjectSidePayload(owner, BrixObjectSidePayloadOwnedTasks);
    return holder ? *reinterpret_cast<BxTaskOwner**>(holder) : nullptr;
}

static BxTaskOwner*
ensureOwnedTasks(char* owner) {
    if (BxTaskOwner* found = ownedTasksOf(owner)) {
        return found;
    }
#if BX_ENABLE_BRIX_GC
    if (!brixGCIsManagedAddress(owner)) {
        return nullptr;
    }
#endif
    void* ownerRoot = brixAddTemporaryRoot(owner);
    char* holder = brixGCAllocObject(sizeof(BxTaskOwner*));
    void* holderRoot = brixAddTemporaryRoot(holder);
    BxTaskOwner* made = new BxTaskOwner();
    *reinterpret_cast<BxTaskOwner**>(holder) = made;
    BxTaskOwner* kept = nullptr;
    if (brixGCRegisterNativeCleanup(holder, made, deleteTaskOwner)) {
        char* published = brixGCPublishObjectSidePayload(
            owner, BrixObjectSidePayloadOwnedTasks, holder);
        kept = published ? *reinterpret_cast<BxTaskOwner**>(published) : nullptr;
    } else {
        delete made;
    }
    if (holderRoot) {
        brixRemoveTemporaryRoot(holderRoot);
    }
    if (ownerRoot) {
        brixRemoveTemporaryRoot(ownerRoot);
    }
    return kept;
}

static bool
taskHandleIsLive(uint64_t handle) {
    BxTask* task = BxTask::pinHandle(handle);
    if (!task) {
        return false;
    }
    task->unpinHandle();
    return true;
}

static void
addOwnedTask(BxTaskOwner* tasks, uint64_t handle) {
    tasks->lock.lock();
    if (tasks->handles.size() >= tasks->pruneAt) {
        size_t kept = 0;
        for (uint64_t held : tasks->handles) {
            if (taskHandleIsLive(held)) {
                tasks->handles[kept++] = held;
            }
        }
        tasks->handles.resize(kept);
        tasks->pruneAt = kept * 2 > 8 ? kept * 2 : 8;
    }
    tasks->handles.push_back(handle);
    tasks->lock.unlock();
}

extern "C" uint64_t
brixStartOwnedTask(
    char* owner,
    BxTaskQueue* queue,
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column,
    uint32_t options) {
    uint64_t handle = 0;
    uint64_t started =
        startTask(queue, func, closure, localSize, file, line, column, options, &handle);
    if (handle && owner) {
        if (BxTaskOwner* tasks = ensureOwnedTasks(owner)) {
            addOwnedTask(tasks, handle);
        }
    }
    return started;
}

static void
cancelTaskWithoutYielding(BxTask* task) {
    task->cancel();
    task->getTaskQueue()->wakeCanceledTask(task);
}

static void
cancelOwnedTasks(char* owner, char* heir) {
    BxTaskOwner* tasks = owner ? ownedTasksOf(owner) : nullptr;
    if (!tasks) {
        return;
    }
    std::vector<uint64_t> handles;
    tasks->lock.lock();
    handles.swap(tasks->handles);
    tasks->pruneAt = 8;
    tasks->lock.unlock();
    BxTask* current = brixTryGetActiveTask();
    uint64_t closing = 0;
    for (uint64_t handle : handles) {
        BxTask* task = BxTask::pinHandle(handle);
        if (!task) {
            continue;
        }
        if (task == current) {
            closing = handle;
        } else {
            cancelTaskWithoutYielding(task);
        }
        task->unpinHandle();
    }
    if (closing) {
        BxTaskOwner* keeper = heir ? ensureOwnedTasks(heir) : nullptr;
        addOwnedTask(keeper ? keeper : tasks, closing);
    }
}

extern "C" void
brixCancelOwnedTasks(char* owner) {
    cancelOwnedTasks(owner, nullptr);
}

extern "C" void
brixCancelOwnedTasksTo(char* owner, char* heir) {
    cancelOwnedTasks(owner, heir);
}

static size_t
releaseTaskEnding(void* context) {
    static_cast<BxTaskEnding*>(context)->release();
    return 1;
}

extern "C" void
brixTaskEndingAdopt(char* owner, BxTaskEnding* ending) {
    if (!brixGCRegisterNativeCleanup(owner, ending, releaseTaskEnding)) {
        ending->release();
    }
}

extern "C" uint64_t
brixTaskEndingHandle(BxTaskEnding* ending) {
    return ending->handle;
}

extern "C" uint32_t
brixTaskEndingEnd(BxTaskEnding* ending) {
    return ending->end.load(std::memory_order_acquire);
}

static bool
waitForTaskEnding(BxTaskEnding* ending) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    ending->lock.lock();
    if (ending->end.load(std::memory_order_acquire) != BX_TASK_END_RUNNING) {
        ending->lock.unlock();
        return false;
    }
    BxTaskEndWaiter waiter{task, ending->waiters};
    ending->waiters = &waiter;
    thread->suspendTask(task, &ending->lock, BxTaskGCStateBlocked);

    ending->lock.lock();
    for (BxTaskEndWaiter** link = &ending->waiters; *link; link = &(*link)->next) {
        if (*link == &waiter) {
            *link = waiter.next;
            break;
        }
    }
    ending->lock.unlock();
    return task->getCanceled();
}

extern "C" bool
brixTaskEndingWait(BxTaskEnding* ending) {
    return waitForTaskEnding(ending);
}

extern "C" bool
brixWaitForTask(uint64_t handle) {
    BxTaskEnding* ending = BxTask::endingForHandle(handle);
    if (!ending) {
        return brixGetActiveThread()->getActiveTask()->getCanceled();
    }
    bool canceled = waitForTaskEnding(ending);
    ending->release();
    return canceled;
}

extern "C" void
brixSetActiveTaskEnd(uint32_t end) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : NULL;
    if (task && task->getRuntimeContext()->taskEnd == BX_TASK_END_RUNNING) {
        task->getRuntimeContext()->taskEnd = end;
    }
}

extern "C" void
brixTerminateActiveTask() {
    BxThread* thread = brixGetActiveThread();
    if (thread) {
        thread->taskTerminated(thread->getActiveTask());
    }
}

extern "C" BxTask*
brixGetActiveTask() {
    return brixGetActiveThread()->getActiveTask();
}

extern "C" BxTask*
brixTryGetActiveTask() {
    BxThread* thread = brixGetActiveThread();
    return thread ? thread->getActiveTask() : NULL;
}

extern "C" uint32_t
brixGetActiveTaskId() {
    return brixGetActiveThread()->getActiveTask()->getId();
}

extern "C" uint64_t
brixGetActiveTaskHandle() {
    return brixGetActiveThread()->getActiveTask()->ensureHandle();
}

extern "C" uint32_t
brixGetTaskHandleId(uint64_t handle) {
    return static_cast<uint32_t>(handle >> 32);
}

extern "C" uint32_t
brixGetTaskId(BxTask* task) {
    return task->getId();
}

extern "C" uint32_t
brixGetTaskParentId(BxTask* task) {
    return task ? task->getParentId() : 0;
}

extern "C" const char*
brixGetTaskSpawnFile(BxTask* task) {
    return task ? task->getSpawnFile() : NULL;
}

extern "C" uint32_t
brixGetTaskSpawnLine(BxTask* task) {
    return task ? task->getSpawnLine() : 0;
}

extern "C" uint32_t
brixGetTaskSpawnColumn(BxTask* task) {
    return task ? task->getSpawnColumn() : 0;
}

extern "C" uint32_t
brixGetTaskFlags(BxTask* task) {
    return task ? task->getFlags() : 0;
}

extern "C" void*
brixGetTaskStackBase(BxTask* task) {
    return task ? task->getStackBase() : NULL;
}

extern "C" size_t
brixGetTaskStackSize(BxTask* task) {
    return task ? task->getStackSize() : 0;
}

static BxTask*
brixActiveTaskForHandle(uint64_t handle) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : NULL;
    return task && task->getHandle() == handle ? task : NULL;
}

extern "C" bool
brixGetTaskFinished(uint64_t handle) {
    if (brixActiveTaskForHandle(handle)) {
        return false;
    }

    BxTask* task = BxTask::pinHandle(handle);
    if (!task) {
        return true;
    }
    task->unpinHandle();
    return false;
}

static bool
readTaskCancel(uint64_t handle, bool inForce) {
    if (BxTask* active = brixActiveTaskForHandle(handle)) {
        return inForce ? active->getCanceled() : active->isCancelRequested();
    }

    BxTask* task = BxTask::pinHandle(handle);
    if (!task) {
        return false;
    }
    bool canceled = inForce ? task->getCanceled() : task->isCancelRequested();
    task->unpinHandle();
    return canceled;
}

extern "C" bool
brixGetTaskCanceled(uint64_t handle) {
    return readTaskCancel(handle, false);
}

extern "C" bool
brixGetTaskCancelInForce(uint64_t handle) {
    return readTaskCancel(handle, true);
}

extern "C" bool
brixGetTaskTerminating(uint64_t handle) {
    if (BxTask* active = brixActiveTaskForHandle(handle)) {
        return active->getTerminating();
    }

    BxTask* task = BxTask::pinHandle(handle);
    if (!task) {
        return false;
    }
    bool terminating = task->getTerminating();
    task->unpinHandle();
    return terminating;
}

extern "C" bool
brixActiveTaskIsTerminating() {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : NULL;
    return task && task->getTerminating();
}

extern "C" bool
brixTakeActiveTaskTermination() {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : NULL;
    return task && task->takeTermination();
}

extern "C" char*
brixGetActiveTaskMemory() {
    return brixGetActiveThread()->getActiveTask()->getLocals();
}

extern "C" char*
brixGetActiveThreadMemory() {
    BxThread* thread = brixGetActiveThread();
    if (!thread || !thread->getThreadLocals()) {
        fprintf(stderr, "Thread local memory is unavailable without an active Brix thread\n");
        abort();
    }
    return thread->getThreadLocals();
}

extern "C" int
brixActiveTaskQueueRemaining() {
    BxThread* thread = brixGetActiveThread();
    BxTaskQueue* queue = thread ? thread->getTaskQueue() : NULL;
    return queue ? queue->remaining() : 0;
}

extern "C" int
brixActiveTaskQueueActivity() {
    BxThread* thread = brixGetActiveThread();
    BxTaskQueue* queue = thread ? thread->getTaskQueue() : NULL;
    return queue ? queue->activity() : 0;
}

extern "C" bool
brixSuspendTask(uint64_t handle) {
    BxTask* task = brixActiveTaskForHandle(handle);
    if (!task) {
        fprintf(stderr, "Attempted to suspend a task that is not active on this thread\n");
        abort();
    }
    if (task->getCanceled()) {
        return true;
    }

    BxThread* thread = brixGetActiveThread();
    thread->suspendTask(task, NULL, BxTaskGCStateParked);

    return task->getCanceled();
}

static void
brixResumePinnedTask(BxTask* task, bool acrossQueues) {
    BxThread* thread = brixGetActiveThread();
    BxTask* current = thread ? thread->getActiveTask() : NULL;
    if (!current) {
        if (thread) {
            thread->resumeTask(task);
        } else {
            task->getTaskQueue()->scheduleTask(task, acrossQueues);
        }
        task->unpinHandle();
        return;
    }

    bool yields = task != current &&
        (acrossQueues || task->getTaskQueue() == thread->getTaskQueue());
    if (task != current) {
        task->getTaskQueue()->scheduleTask(task, yields);
    }
    task->unpinHandle();
    if (yields) {
        thread->suspendTask(current, NULL, BxTaskGCStateParked, true);
    }
}

extern "C" bool
brixResumeTaskNow(uint64_t handle) {
    if (BxTask* task = BxTask::pinHandle(handle)) {
        brixResumePinnedTask(task, false);
    }

    return brixGetActiveThread()->getActiveTask()->getCanceled();
}

extern "C" void
brixResumeTaskLater(uint64_t handle) {
    if (BxTask* task = BxTask::pinHandle(handle)) {
        task->getTaskQueue()->scheduleTask(task);
        task->unpinHandle();
    }
}

extern "C" void
brixCancelTask(uint64_t handle) {
    BxTask* task = BxTask::pinHandle(handle);
    if (!task) {
        return;
    }
    cancelTaskWithoutYielding(task);
    BxThread* thread = brixGetActiveThread();
    BxTask* current = thread ? thread->getActiveTask() : NULL;
    task->unpinHandle();
    if (current && task != current) {
        thread->suspendTask(current, NULL, BxTaskGCStateParked, true);
    }
}

extern "C" void
brixCancelTaskLater(uint64_t handle) {
    BxTask* task = BxTask::pinHandle(handle);
    if (!task) {
        return;
    }
    cancelTaskWithoutYielding(task);
    task->unpinHandle();
}

extern "C" void
brixEnterCancelShield() {
    brixGetActiveThread()->getActiveTask()->enterCancelShield();
}

extern "C" void
brixLeaveCancelShield() {
    brixGetActiveThread()->getActiveTask()->leaveCancelShield();
}

// =============================================================================================

static uint32_t
brixSuspendActiveTaskUntilReady(int fd, int16_t filter, uint64_t deadlineNs) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return BxWaitCanceled;
    }
    if (deadlineNs && deadlineNs <= brixMonotonicTimeNanoseconds()) {
        return BxWaitTimedOut;
    }

    BxPollWaiter waiter(task, fd, filter);
    BxPollWaitStart start = gPollLoop->beginWait(&waiter, deadlineNs);
    if (start == BxPollWaitStartReady) {
        return BxWaitReady;
    }
#if defined(__APPLE__)
    if (start == BxPollWaitStartSelect) {
        BxSelectWaiter selectWaiter(task, fd, filter == BxPollRead, deadlineNs);
        gSelectLoop->beginWait(&selectWaiter);
        thread->suspendTask(task, NULL, BxTaskGCStateBlocked);
        gSelectLoop->endWait(&selectWaiter);
        if (task->getCanceled()) {
            return BxWaitCanceled;
        }
        return selectWaiter.timedOut ? BxWaitTimedOut : BxWaitReady;
    }
#endif

    thread->suspendTask(task, NULL, BxTaskGCStateBlocked);
    gPollLoop->endWait(&waiter);

    if (task->getCanceled()) {
        return BxWaitCanceled;
    }
    return waiter.timedOut ? BxWaitTimedOut : BxWaitReady;
}

extern "C" uint32_t
brixSuspendActiveTaskUntilReadable(int fd, uint64_t deadlineNs) {
    return brixSuspendActiveTaskUntilReady(fd, BxPollRead, deadlineNs);
}

extern "C" uint32_t
brixSuspendActiveTaskUntilWritable(int fd, uint64_t deadlineNs) {
    return brixSuspendActiveTaskUntilReady(fd, BxPollWrite, deadlineNs);
}

extern "C" bool
brixSuspendActiveTaskUntilProcExit(int pid, BxSemaphore* sem) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    gPollLoop->onProcExit(pid, task);

    if (sem) {
        int cancelled = 0;
        sem->wait(thread, task, &cancelled, sizeof(cancelled), NULL);
        return cancelled || task->getCanceled();
    } else {
        thread->suspendTask(task, NULL, BxTaskGCStateBlocked);
        return task->getCanceled();
    }
}

extern "C" bool
brixSuspendActiveTaskUntilFileChange(int fd, int type, BxSemaphore* sem) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    gPollLoop->onFileChange(fd, type, task);

    if (sem) {
        int cancelled = 0;
        sem->wait(thread, task, &cancelled, sizeof(cancelled), NULL);
        return cancelled || task->getCanceled();
    } else {
        return task->getCanceled();
    }
}

extern "C" bool
brixSuspendActiveTaskUntilSignal(int sig, BxSemaphore* sem) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    gPollLoop->onSignal(sig, task);

    if (sem) {
        int cancelled = 0;
        sem->wait(thread, task, &cancelled, sizeof(cancelled), NULL);
        return cancelled || task->getCanceled();
    } else {
        return task->getCanceled();
    }
}

extern "C" bool
brixSuspendActiveTaskUntilTime(long int us, BxSemaphore* sem) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    uint64_t now = brixMonotonicTimeNanoseconds();
    uint64_t delay = us > 0 ? static_cast<uint64_t>(us) * 1000ull : 0;
    uint64_t deadline = delay > UINT64_MAX - now ? UINT64_MAX : now + delay;
    uint64_t id = brixRegisterTimerEvent(sem);
    gPollLoop->onTimer(deadline, id, task);

    if (sem) {
        int cancelled = 0;
        sem->wait(thread, task, &cancelled, sizeof(cancelled), NULL);
        bool wasCancelled = cancelled || task->getCanceled();
        if (wasCancelled) {
            gPollLoop->cancelTimer(id);
        }
        brixClearTimerEvent(sem, id);
        return wasCancelled;
    } else {
        return task->getCanceled();
    }
}

extern "C" uint64_t
brixMonotonicTimeNanoseconds() {
#if defined(__APPLE__)
    return brixMachTimeToNanoseconds(mach_continuous_time());
#else
    struct timespec now;
    clock_gettime(CLOCK_BOOTTIME, &now);
    return (uint64_t)now.tv_sec * 1000000000ull + (uint64_t)now.tv_nsec;
#endif
}

extern "C" uint64_t
brixNextScheduledWorkDeadlineNanoseconds() {
    return gPollLoop ? gPollLoop->nextScheduledDeadlineNanoseconds() : 0;
}

extern "C" bool
brixSuspendActiveTaskUntilAbsoluteTime(uint64_t deadlineNs, BxSemaphore* sem) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    if (deadlineNs <= brixMonotonicTimeNanoseconds()) {
        return false;
    }

    uint64_t id = brixRegisterTimerEvent(sem);
    gPollLoop->onTimer(deadlineNs, id, task);

    if (sem) {
        int cancelled = 0;
        sem->wait(thread, task, &cancelled, sizeof(cancelled), NULL);
        bool wasCancelled = cancelled || task->getCanceled();
        if (wasCancelled) {
            gPollLoop->cancelTimer(id);
        }
        brixClearTimerEvent(sem, id);
        return wasCancelled;
    } else {
        return task->getCanceled();
    }
}

extern "C" void
brixReleaseEventDescriptor(int fd) {
#if defined(__APPLE__)
    if (gPollLoop && gPollLoop->releaseDescriptor(fd) && gSelectLoop) {
        gSelectLoop->cancelDescriptor(fd);
    }
#else
    if (gPollLoop) {
        gPollLoop->releaseDescriptor(fd);
    }
#endif
}

extern "C" void
brixCancelProcExitEventListener(int pid, BxSemaphore* sem) {
    gPollLoop->cancelProcExit(pid);

    if (sem) {
        int cancelled = 1;
        sem->post(&cancelled, sizeof(cancelled), true);
    }
}

extern "C" void
brixCancelFileChangeEventListener(int fd, int type, BxSemaphore* sem) {
    gPollLoop->cancelFileChange(fd, type);

    if (sem) {
        int cancelled = 1;
        sem->post(&cancelled, sizeof(cancelled), true);
    }
}

extern "C" void
brixCancelSignalEventListener(int sig, BxSemaphore* sem) {
    gPollLoop->cancelSignal(sig);

    if (sem) {
        int cancelled = 1;
        sem->post(&cancelled, sizeof(cancelled), true);
    }
}

extern "C" void
brixCancelTimerEventListener(BxSemaphore* sem) {
    uint64_t id = brixTimerEventIdForSemaphore(sem);
    if (id) {
        gPollLoop->cancelTimer(id);
    }

    if (sem) {
        int cancelled = 1;
        sem->post(&cancelled, sizeof(cancelled), true);
    }
}

// =============================================================================================

extern "C" BxSemaphore*
brixNewSemaphore() {
    BxSemaphore* sem = reinterpret_cast<BxSemaphore*>(brixAllocObject(sizeof(BxSemaphore)));
    new (sem) BxSemaphore();
    return sem;
}

extern "C" bool
brixWaitForSemaphore(BxSemaphore* sem, void* buf, size_t capacity, BxTaskMutex* mutex) {
    BxThread* thread = brixGetActiveThread();
    return sem->wait(thread, thread->getActiveTask(), buf, capacity, mutex);
}

extern "C" void
brixPostSemaphore(BxSemaphore* sem, void* data, size_t sizeOfData) {
    sem->post(data, sizeOfData, false);
}

// *************************************************************************************************

extern "C" BxTaskMutex*
brixNewMutex() {
    BxTaskMutex* mutex = reinterpret_cast<BxTaskMutex*>(brixAllocObject(sizeof(BxTaskMutex)));
    new (mutex) BxTaskMutex();
    return mutex;
}

extern "C" void
brixLockMutex(BxTaskMutex* mutex) {
    mutex->lock();
}

extern "C" void
brixUnlockMutex(BxTaskMutex* mutex) {
    mutex->unlock();
}

// =============================================================================================

extern "C" BxTaskQueue*
brixNewWorker(int threadCount) {
    BxThread* thread = brixGetActiveThread();
    BxTaskQueue* queue = thread->getTaskQueue();
    return queue->createQueue(threadCount);
}

extern "C" BxTaskQueue*
brixActiveTaskQueue() {
    return brixGetActiveThread()->getTaskQueue();
}

extern "C" int
brixUsableCoreCount() {
#if !defined(__APPLE__)
    return getCPUCount();
#else
    int levels = 0;
    size_t size = sizeof(levels);
    int total = 0;
    if (sysctlbyname("hw.nperflevels", &levels, &size, nullptr, 0) == 0) {
        for (int level = 0; level < levels; ++level) {
            char key[64];
            char name[32] = "";
            size_t nameSize = sizeof(name);
            snprintf(key, sizeof(key), "hw.perflevel%d.name", level);
            if (sysctlbyname(key, name, &nameSize, nullptr, 0) == 0 && strcmp(name, "Efficiency") == 0) {
                continue;
            }
            int levelCount = 0;
            size = sizeof(levelCount);
            snprintf(key, sizeof(key), "hw.perflevel%d.logicalcpu", level);
            if (sysctlbyname(key, &levelCount, &size, nullptr, 0) == 0 && levelCount > 0) {
                total += levelCount;
            }
        }
    }
    if (total > 0) {
        return total;
    }
    int count = 0;
    size = sizeof(count);
    if (sysctlbyname("hw.logicalcpu", &count, &size, nullptr, 0) == 0 && count > 0) {
        return count;
    }
    long online = sysconf(_SC_NPROCESSORS_ONLN);
    return online > 0 ? (int)online : 1;
#endif
}

static const uint8_t kBxLazyGlobalUnclaimed = 0;
static const uint8_t kBxLazyGlobalReady = 1;
static const uint8_t kBxLazyGlobalClaimed = 2;

struct BxLazyGlobalWaiter {
    BxTask* task;
    BxLazyGlobalWaiter* next;
    bool queued;
};

struct BxLazyGlobalClaim {
    BxTask* owner;
    BxLazyGlobalWaiter* waiters;
};

static BxMutex&
lazyGlobalLock() {
    static BxMutex* lock = new BxMutex();
    return *lock;
}

static std::unordered_map<uint8_t*, BxLazyGlobalClaim>&
lazyGlobalClaims() {
    static auto* claims = new std::unordered_map<uint8_t*, BxLazyGlobalClaim>();
    return *claims;
}

static void
brixRemoveLazyGlobalWaiterLocked(uint8_t* state, BxLazyGlobalWaiter* waiter) {
    auto found = lazyGlobalClaims().find(state);
    if (found == lazyGlobalClaims().end()) {
        return;
    }
    for (BxLazyGlobalWaiter** link = &found->second.waiters; *link; link = &(*link)->next) {
        if (*link == waiter) {
            *link = waiter->next;
            waiter->queued = false;
            return;
        }
    }
}

extern "C" int
brixClaimLazyGlobal(uint8_t* state) {
    if (__atomic_load_n(state, __ATOMIC_ACQUIRE) == kBxLazyGlobalReady) {
        return kBxLazyGlobalReady;
    }

    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : NULL;
    BxMutex& lock = lazyGlobalLock();
    lock.lock();
    while (true) {
        uint8_t current = __atomic_load_n(state, __ATOMIC_ACQUIRE);
        if (current == kBxLazyGlobalReady) {
            lock.unlock();
            return kBxLazyGlobalReady;
        }
        if (current == kBxLazyGlobalUnclaimed) {
            lazyGlobalClaims()[state] = { task, NULL };
            __atomic_store_n(state, kBxLazyGlobalClaimed, __ATOMIC_RELAXED);
            lock.unlock();
            return kBxLazyGlobalUnclaimed;
        }

        BxLazyGlobalClaim& claim = lazyGlobalClaims()[state];
        if (claim.owner == task) {
            lock.unlock();
            return kBxLazyGlobalClaimed;
        }
        if (!task) {
            lock.unlock();
            sched_yield();
            lock.lock();
            continue;
        }

        BxLazyGlobalWaiter waiter = { task, claim.waiters, true };
        claim.waiters = &waiter;
        thread->suspendTask(task, &lock, BxTaskGCStateBlocked);
        thread = brixGetActiveThread();
        lock.lock();
        if (waiter.queued) {
            brixRemoveLazyGlobalWaiterLocked(state, &waiter);
        }
    }
}

static void
brixWakeLazyGlobalWaitersLocked(
    std::unordered_map<uint8_t*, BxLazyGlobalClaim>::iterator found) {
    BxLazyGlobalWaiter* waiter = found->second.waiters;
    lazyGlobalClaims().erase(found);
    while (waiter) {
        BxLazyGlobalWaiter* next = waiter->next;
        BxTask* task = waiter->task;
        waiter->queued = false;
        task->getTaskQueue()->scheduleTask(task, true);
        waiter = next;
    }
}

extern "C" void
brixReleaseLazyGlobal(uint8_t* state) {
    BxMutex& lock = lazyGlobalLock();
    lock.lock();
    __atomic_store_n(state, kBxLazyGlobalReady, __ATOMIC_RELEASE);
    auto found = lazyGlobalClaims().find(state);
    if (found != lazyGlobalClaims().end()) {
        brixWakeLazyGlobalWaitersLocked(found);
    }
    lock.unlock();
}

extern "C" void
brixAbandonLazyGlobal(uint8_t* state) {
    BxThread* thread = brixGetActiveThread();
    BxTask* task = thread ? thread->getActiveTask() : NULL;
    BxMutex& lock = lazyGlobalLock();
    lock.lock();
    auto found = lazyGlobalClaims().find(state);
    if (found != lazyGlobalClaims().end() && found->second.owner == task) {
        uint8_t claimed = kBxLazyGlobalClaimed;
        __atomic_compare_exchange_n(
            state,
            &claimed,
            kBxLazyGlobalUnclaimed,
            false,
            __ATOMIC_RELEASE,
            __ATOMIC_RELAXED);
        brixWakeLazyGlobalWaitersLocked(found);
    }
    lock.unlock();
}

extern "C" int
brixGCBackgroundTaskQueueThreadCount() {
    return gSpinQueue ? gSpinQueue->getThreadCount() : 0;
}

extern "C" BxTaskQueue*
brixGCBackgroundTaskQueue() {
    return gSpinQueue;
}

extern "C" bool
brixForkGCMarkTaskInWorker(
    BxTaskQueue* queue,
    BxTaskStartFunc func,
    void* closure
) {
    if (!queue || !func || brixProcessTerminationRequested() || queue->isStopped()) {
        return false;
    }
    queue->startTask(
        func,
        closure,
        0,
        BxTaskProvenance(
            0,
            nullptr,
            0,
            0,
            BX_TASK_FLAG_WORKER | BX_TASK_FLAG_GC_MARK_WORKER,
            BX_TASK_SCHEDULING_NORMAL));
    return true;
}

extern "C" bool
brixForkGCCleanupTask(BxTaskStartFunc func, void* closure) {
    if (!gSpinQueue || !func || brixProcessTerminationRequested() || gSpinQueue->isStopped()) {
        return false;
    }
    // Cleanup is a safety-net workload, never scheduler-priority work. It receives its own
    // ordinary worker task so callback suspension and panics use the normal task ABI.
    gSpinQueue->startTask(
        func,
        closure,
        0,
        BxTaskProvenance(
            0,
            nullptr,
            0,
            0,
            BX_TASK_FLAG_WORKER,
            BX_TASK_SCHEDULING_NORMAL));
    return true;
}

extern "C" bool
brixWorkerGetStatus(BxTaskQueue* queue) {
    return queue->isStopped();
}

extern "C" void
brixWorkerStop(BxTaskQueue* queue) {
    queue->cancelTasks();
    BxThread* thread = brixGetActiveThread();
    BxTask* caller = thread ? thread->getActiveTask() : NULL;
    if (caller && thread->getTaskQueue() != queue) {
        queue->stopAfterLiveTasks(thread, caller);
    } else {
        queue->stop();
    }
    // Joining worker pthreads can overlap a collection initiated by a task already running on
    // that worker. Publish the caller as blocked so the handshake never waits on this join.
    brixGCEnterRuntimeBlock();
    queue->join();
    brixGCLeaveRuntimeBlock();
}
