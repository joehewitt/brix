
#include <YuEvents/YuEvents.h>
#include <YuEvents/YuThread.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuSemaphore.h>
#include <YuEvents/YuMutex.h>
#include <YuEvents/YuTaskMutex.h>
#include <YuEvents/YuTaskMonitor.h>
#include <YuEvents/YuTaskQueue.h>
#include <YuEvents/YuGCHandshake.h>
#include <YuEvents/YuPoll.h>
#include <YuCore/YuCore.h>

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
#include <YuEvents/YuKQueueLoop.h>
#include <YuEvents/YuSelectLoop.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
typedef YuKQueueLoop YuPollLoop;
#else
#include <YuEvents/YuEpollLoop.h>
#include <ctime>
typedef YuEpollLoop YuPollLoop;
#endif

// *************************************************************************************************

static pthread_key_t gActiveThreadKey = 0;
static pthread_mutex_t gActiveThreadKeyLock = PTHREAD_MUTEX_INITIALIZER;
static std::atomic<bool> gActiveThreadKeyCreated(false);
static thread_local YuTaskRuntimeContext* gActiveTaskRuntimeContext = NULL;
static thread_local YuTaskRuntimeContext gFallbackTaskRuntimeContext = {};
static thread_local size_t gFallbackTaskGCAssistDebt = 0;
static thread_local size_t gFallbackTaskGCAssistDebtCycle = 0;

static YuTaskQueue* gMainQueue = NULL;
static YuTaskQueue* gSpinQueue = NULL;
static YuPollLoop* gPollLoop = NULL;
#if defined(__APPLE__)
static YuSelectLoop* gSelectLoop = NULL;
#endif
static size_t gTaskLocalSize = 0;
static YuTaskLocalFunc gTaskLocalInit = NULL;
static size_t gThreadLocalSize = 0;
static YuThreadLocalFunc gThreadLocalInit = NULL;
static YuThreadLocalFunc gThreadLocalFlush = NULL;
static std::atomic<bool> gProcessTerminationRequested(false);
static std::atomic<int> gProcessExitCode(0);

static std::atomic<uint64_t> gTimerEventIdCounter(1);
static pthread_mutex_t gTimerEventIdsLock = PTHREAD_MUTEX_INITIALIZER;
static std::unordered_map<YuSemaphore*, uint64_t> gTimerEventIds;
#if defined(__APPLE__)
static pthread_once_t gMachTimebaseOnce = PTHREAD_ONCE_INIT;
static mach_timebase_info_data_t gMachTimebaseInfo = {0, 0};
#endif

// *************************************************************************************************

extern "C" void
yumiGCSetActiveThreadCache(YumiGCThreadCache* cache)
    __attribute__((weak));

static void
yumiSetActiveGCThreadCache(YumiGCThreadCache* cache) {
    if (yumiGCSetActiveThreadCache) {
        yumiGCSetActiveThreadCache(cache);
    }
}

// *************************************************************************************************

static void
yumiEnsureActiveThreadKey() {
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
yumiInitMachTimebase() {
    mach_timebase_info(&gMachTimebaseInfo);
    if (!gMachTimebaseInfo.numer || !gMachTimebaseInfo.denom) {
        gMachTimebaseInfo.numer = 1;
        gMachTimebaseInfo.denom = 1;
    }
}

static mach_timebase_info_data_t
yumiMachTimebaseInfo() {
    pthread_once(&gMachTimebaseOnce, yumiInitMachTimebase);
    return gMachTimebaseInfo;
}

static uint64_t
yumiMachTimeToNanoseconds(uint64_t machTime) {
    mach_timebase_info_data_t info = yumiMachTimebaseInfo();
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
yumiRegisterTimerEvent(YuSemaphore* sem) {
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
yumiClearTimerEvent(YuSemaphore* sem, uint64_t id) {
    if (!sem) {
        return;
    }

    pthread_mutex_lock(&gTimerEventIdsLock);
    std::unordered_map<YuSemaphore*, uint64_t>::iterator it = gTimerEventIds.find(sem);
    if (it != gTimerEventIds.end() && it->second == id) {
        gTimerEventIds.erase(it);
    }
    pthread_mutex_unlock(&gTimerEventIdsLock);
}

static uint64_t
yumiTimerEventIdForSemaphore(YuSemaphore* sem) {
    if (!sem) {
        return 0;
    }

    uint64_t id = 0;
    pthread_mutex_lock(&gTimerEventIdsLock);
    std::unordered_map<YuSemaphore*, uint64_t>::iterator it = gTimerEventIds.find(sem);
    if (it != gTimerEventIds.end()) {
        id = it->second;
    }
    pthread_mutex_unlock(&gTimerEventIdsLock);
    return id;
}

static YuTaskProvenance
yumiCreateTaskProvenance(
    const char* file,
    uint32_t line,
    uint32_t column,
    uint32_t flags,
    uint32_t schedulingClass = YU_TASK_SCHEDULING_NORMAL) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* parent = thread ? thread->getActiveTask() : NULL;
    uint32_t parentId = parent ? parent->getId() : 0;
    return YuTaskProvenance(parentId, file, line, column, flags, schedulingClass);
}

// *************************************************************************************************

YuThread*
yumiGetActiveThread() {
    if (!gActiveThreadKeyCreated.load(std::memory_order_acquire)) {
        return NULL;
    }
    return (YuThread*)pthread_getspecific(gActiveThreadKey);
}

void
yumiSetActiveThread(YuThread* thread) {
    yumiEnsureActiveThreadKey();
    pthread_setspecific(gActiveThreadKey, (void*)thread);
    yumiSetActiveGCThreadCache(thread ? thread->getGCThreadCache() : NULL);
}

extern "C" YuTaskRuntimeContext*
yumiGetActiveTaskRuntimeContext() {
    return gActiveTaskRuntimeContext
        ? gActiveTaskRuntimeContext
        : &gFallbackTaskRuntimeContext;
}

extern "C" size_t
yumiGCActiveTaskAssistDebt() {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : nullptr;
    return task ? task->getGCAssistDebt() : gFallbackTaskGCAssistDebt;
}

extern "C" size_t
yumiGCActiveTaskAssistDebtCycle() {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : nullptr;
    return task ? task->getGCAssistDebtCycle() : gFallbackTaskGCAssistDebtCycle;
}

extern "C" void
yumiGCSetActiveTaskAssistDebt(size_t debt) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : nullptr;
    if (task) {
        task->setGCAssistDebt(debt);
    } else {
        gFallbackTaskGCAssistDebt = debt;
    }
}

extern "C" void
yumiGCSetActiveTaskAssistDebtCycle(size_t cycle) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : nullptr;
    if (task) {
        task->setGCAssistDebtCycle(cycle);
    } else {
        gFallbackTaskGCAssistDebtCycle = cycle;
    }
}

extern "C" void
yumiSetActiveTaskRuntimeContext(YuTaskRuntimeContext* context) {
    gActiveTaskRuntimeContext = context;
}

size_t
yumiGetThreadLocalSize() {
    return gThreadLocalSize;
}

YuThreadLocalFunc
yumiGetThreadLocalInit() {
    return gThreadLocalInit;
}

YuThreadLocalFunc
yumiGetThreadLocalFlush() {
    return gThreadLocalFlush;
}

// *************************************************************************************************

extern "C" void
yumiRunEventLoop(
    YuTaskStartFunc starter,
    YuTaskStartFunc closer,
    void* closure,
    size_t localSize,
    YuThreadLocalFunc threadLocalInit,
    YuThreadLocalFunc threadLocalFlush,
    size_t threadLocalSize,
    YuTaskLocalFunc taskLocalInit) {
    register YuTaskRuntimeContext* runtimeContext asm("x27") =
        yumiGetActiveTaskRuntimeContext();
    asm volatile ("" : "+r" (runtimeContext));

#if YU_ENABLE_GC
    GC_INIT();
    // GC_set_find_leak(1);
    // GC_set_all_interior_pointers(1);
    // GC_set_max_heap_size(1024 * 1024 * 100);
    // GC_set_free_space_divisor(4);
    // GC_enable_incremental();
#endif

    yumiEnsureActiveThreadKey();
    gProcessTerminationRequested.store(false, std::memory_order_release);
    gProcessExitCode.store(0, std::memory_order_release);
    __atomic_fetch_and(
        &yumiInterruptRequestedFlags,
        static_cast<unsigned char>(~YumiInterruptTerminationRequested),
        __ATOMIC_RELEASE);
    gThreadLocalSize = threadLocalSize;
    gTaskLocalSize = localSize;
    gTaskLocalInit = taskLocalInit;
    gThreadLocalInit = threadLocalInit;
    gThreadLocalFlush = threadLocalFlush;

    yumiStartTaskMonitor();

    YuPollLoop pollLoop;
    gPollLoop = &pollLoop;
    pollLoop.start();

#if defined(__APPLE__)
    YuSelectLoop selectLoop;
    gSelectLoop = &selectLoop;
    selectLoop.start();
#endif

    YuTaskQueue mainQueue(NULL, 0);
    gMainQueue = &mainQueue;

    int spinThreadCount = getCPUCount() - 3;
    if (spinThreadCount < 1) {
        spinThreadCount = 1;
    }
    gSpinQueue = mainQueue.createQueue(spinThreadCount);

    // starter(closure);
    YuTaskProvenance mainProvenance(0, NULL, 0, 0, YU_TASK_FLAG_MAIN);
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

    yumiStopTaskMonitor();

#if YU_ENABLE_GC
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
yumiForceProcessExit(void*) {
    sleep(kProcessTerminationGraceSeconds);
    _Exit(gProcessExitCode.load(std::memory_order_acquire));
    return NULL;
}

extern "C" bool
yumiRequestProcessTermination(int exitCode) {
    bool wasRequested = gProcessTerminationRequested.exchange(true, std::memory_order_acq_rel);
    if (wasRequested) {
        return false;
    }

    gProcessExitCode.store(exitCode, std::memory_order_release);
    pthread_t watchdog;
    if (pthread_create(&watchdog, NULL, yumiForceProcessExit, NULL) == 0) {
        pthread_detach(watchdog);
    }
    __atomic_fetch_or(
        &yumiInterruptRequestedFlags,
        YumiInterruptTerminationRequested,
        __ATOMIC_RELEASE);
    if (gMainQueue) {
        gMainQueue->requestTermination();
    }
    return true;
}

extern "C" bool
yumiProcessTerminationRequested() {
    return gProcessTerminationRequested.load(std::memory_order_acquire);
}

extern "C" int
yumiGetProcessExitCode() {
    return gProcessExitCode.load(std::memory_order_acquire);
}

static int gTerminationSignalPipe[2] = {-1, -1};
static const int kTerminationSignals[] = {SIGHUP, SIGTERM};

static void
yumiTerminationSignalHandler(int number) {
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
yumiTerminationSignalWatch(void*) {
    unsigned char received = 0;
    ssize_t count;
    do {
        count = read(gTerminationSignalPipe[0], &received, 1);
    } while (count < 0 && errno == EINTR);
    if (count == 1) {
        yumiRequestProcessTermination(128 + received);
    }
    return NULL;
}

extern "C" void
yumiRequestTerminationOnSignals() {
    if (gTerminationSignalPipe[0] >= 0 || pipe(gTerminationSignalPipe) != 0) {
        return;
    }
    for (int descriptor : gTerminationSignalPipe) {
        fcntl(descriptor, F_SETFD, FD_CLOEXEC);
    }

    pthread_t watcher;
    if (pthread_create(&watcher, NULL, yumiTerminationSignalWatch, NULL) != 0) {
        return;
    }
    pthread_detach(watcher);

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = yumiTerminationSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    for (int number : kTerminationSignals) {
        sigaction(number, &action, NULL);
    }
}

extern "C" bool
yumiRunMainTaskQueueHost(
    YuMainTaskQueueHostRunFunc run,
    YuMainTaskQueueHostWakeFunc wake,
    void* context) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : NULL;
    YuTaskQueue* mainQueue = gMainQueue;
    if (!mainQueue || !thread || !task || thread->getTaskQueue() != mainQueue) {
        return false;
    }
    if (!mainQueue->requestHost(task, run, wake, context)) {
        return false;
    }

    thread->suspendTask(task, NULL, YuTaskGCStateParked);
    return !task->getCanceled();
}

struct YuNativeTaskStart {
    YuTaskStartFunc func;
    void* closure;
};

static void
startNativeTask(void* closure) {
    YuNativeTaskStart* start = static_cast<YuNativeTaskStart*>(closure);
    YuTaskStartFunc func = start->func;
    void* funcClosure = start->closure;
    delete start;
    gTaskLocalInit();
    func(funcClosure);
}

extern "C" bool
yumiScheduleMainTaskFromNative(YuTaskStartFunc func, void* closure) {
    YuTaskQueue* mainQueue = gMainQueue;
    if (!mainQueue || !func || yumiProcessTerminationRequested()) {
        return false;
    }
    if (!gTaskLocalInit) {
        return mainQueue->startTaskIfRunning(func, closure, gTaskLocalSize);
    }
    YuNativeTaskStart* start = new YuNativeTaskStart{func, closure};
    if (!mainQueue->startTaskIfRunning(startNativeTask, start, gTaskLocalSize)) {
        delete start;
        return false;
    }
    return true;
}

struct YuTaskFinishCallback {
    YuTaskStartFunc callback;
    void* context;
    void* contextRoot;
};

static void
releaseTaskFinishCallback(YuTaskFinishCallback* record) {
    if (record->contextRoot) {
        yumiRemoveRoot(record->contextRoot);
    }
    delete record;
}

static void
runTaskFinishCallback(void* closure) {
    YuTaskFinishCallback* record = static_cast<YuTaskFinishCallback*>(closure);
    record->callback(record->context);
    releaseTaskFinishCallback(record);
    yumiTerminateActiveTask();
}

static void
scheduleTaskFinishCallback(void* closure) {
    YuTaskFinishCallback* record = static_cast<YuTaskFinishCallback*>(closure);
    if (!yumiScheduleMainTaskFromNative(runTaskFinishCallback, record)) {
        releaseTaskFinishCallback(record);
    }
}

extern "C" void
yumiScheduleMainTaskWhenTaskFinishes(uint64_t handle, YuTaskStartFunc callback, void* context) {
    YuTaskFinishCallback* record = new YuTaskFinishCallback{
        callback,
        context,
        context ? yumiAddRoot(static_cast<char*>(context)) : NULL
    };
    if (!YuTask::addFinishHook(handle, scheduleTaskFinishCallback, record)) {
        scheduleTaskFinishCallback(record);
    }
}

extern "C" uint32_t
yumiRunMainTaskFromNativeUntilDeadline(
    YuTaskStartFunc func, void* closure, uint64_t deadlineNanos) {
    YuThread* thread = yumiGetActiveThread();
    YuTaskQueue* mainQueue = gMainQueue;
    if (!mainQueue || !thread || !func || yumiProcessTerminationRequested()) {
        return YuMainTaskDeadlineFailed;
    }
    if (!gTaskLocalInit) {
        return mainQueue->runHostedTaskUntilDeadline(
            thread, func, closure, gTaskLocalSize, deadlineNanos);
    }
    YuNativeTaskStart* start = new YuNativeTaskStart{func, closure};
    uint32_t result = mainQueue->runHostedTaskUntilDeadline(
        thread, startNativeTask, start, gTaskLocalSize, deadlineNanos);
    if (result == YuMainTaskDeadlineFailed) {
        delete start;
    }
    return result;
}

extern "C" uint32_t
yumiDrainMainTaskQueue(size_t taskBudget, uint64_t deadlineNanos) {
    YuThread* thread = yumiGetActiveThread();
    YuTaskQueue* mainQueue = gMainQueue;
    if (!mainQueue || !thread || !taskBudget) {
        return YuTaskQueueDrainIdle;
    }
    return mainQueue->drainHostedTasks(thread, taskBudget, deadlineNanos);
}

extern "C" void
yumiForkTask(YuTaskStartFunc func, void* closure, size_t localSize) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    YuThread* thread = yumiGetActiveThread();
    if (thread) {
        thread->startTask(func, closure, localSize);
    }
}

extern "C" void
yumiForkMainTask(YuTaskStartFunc func, void* closure, size_t localSize) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    YuThread* thread = yumiGetActiveThread();
    if (thread) {
        thread->startTask(func, closure, localSize, YuTaskProvenance(0, NULL, 0, 0, YU_TASK_FLAG_MAIN));
    }
}

extern "C" void
yumiForkTaskWithInfo(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    YuThread* thread = yumiGetActiveThread();
    if (thread) {
        thread->startTask(
            func,
            closure,
            localSize,
            yumiCreateTaskProvenance(file, line, column, 0));
    }
}

extern "C" void
yumiForkPriorityTask(YuTaskStartFunc func, void* closure, size_t localSize) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    YuThread* thread = yumiGetActiveThread();
    if (thread) {
        thread->startTask(
            func,
            closure,
            localSize,
            YuTaskProvenance(0, NULL, 0, 0, 0, YU_TASK_SCHEDULING_PRIORITY));
    }
}

extern "C" void
yumiForkPriorityTaskWithInfo(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    YuThread* thread = yumiGetActiveThread();
    if (thread) {
        thread->startTask(
            func,
            closure,
            localSize,
            yumiCreateTaskProvenance(file, line, column, 0, YU_TASK_SCHEDULING_PRIORITY));
    }
}

extern "C" void
yumiSpinTask(YuTaskStartFunc func, void* closure, size_t localSize) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    gSpinQueue->startTask(func, closure, localSize);
}

extern "C" void
yumiSpinTaskWithInfo(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    gSpinQueue->startTask(
        func,
        closure,
        localSize,
        yumiCreateTaskProvenance(file, line, column, YU_TASK_FLAG_SPIN));
}

extern "C" void
yumiSpinPriorityTask(YuTaskStartFunc func, void* closure, size_t localSize) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    gSpinQueue->startTask(
        func,
        closure,
        localSize,
        YuTaskProvenance(0, NULL, 0, 0, YU_TASK_FLAG_SPIN, YU_TASK_SCHEDULING_PRIORITY));
}

extern "C" void
yumiSpinPriorityTaskWithInfo(
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column) {
    if (yumiProcessTerminationRequested()) {
        return;
    }

    gSpinQueue->startTask(
        func,
        closure,
        localSize,
        yumiCreateTaskProvenance(
            file,
            line,
            column,
            YU_TASK_FLAG_SPIN,
            YU_TASK_SCHEDULING_PRIORITY));
}

extern "C" void
yumiTerminateActiveTask() {
    YuThread* thread = yumiGetActiveThread();
    if (thread) {
        thread->taskTerminated(thread->getActiveTask());
    }
}

extern "C" YuTask*
yumiGetActiveTask() {
    return yumiGetActiveThread()->getActiveTask();
}

extern "C" YuTask*
yumiTryGetActiveTask() {
    YuThread* thread = yumiGetActiveThread();
    return thread ? thread->getActiveTask() : NULL;
}

extern "C" uint32_t
yumiGetActiveTaskId() {
    return yumiGetActiveThread()->getActiveTask()->getId();
}

extern "C" uint64_t
yumiGetActiveTaskHandle() {
    return yumiGetActiveThread()->getActiveTask()->ensureHandle();
}

extern "C" uint32_t
yumiGetTaskHandleId(uint64_t handle) {
    return static_cast<uint32_t>(handle >> 32);
}

extern "C" uint32_t
yumiGetTaskId(YuTask* task) {
    return task->getId();
}

extern "C" uint32_t
yumiGetTaskParentId(YuTask* task) {
    return task ? task->getParentId() : 0;
}

extern "C" const char*
yumiGetTaskSpawnFile(YuTask* task) {
    return task ? task->getSpawnFile() : NULL;
}

extern "C" uint32_t
yumiGetTaskSpawnLine(YuTask* task) {
    return task ? task->getSpawnLine() : 0;
}

extern "C" uint32_t
yumiGetTaskSpawnColumn(YuTask* task) {
    return task ? task->getSpawnColumn() : 0;
}

extern "C" uint32_t
yumiGetTaskFlags(YuTask* task) {
    return task ? task->getFlags() : 0;
}

extern "C" void*
yumiGetTaskStackBase(YuTask* task) {
    return task ? task->getStackBase() : NULL;
}

extern "C" size_t
yumiGetTaskStackSize(YuTask* task) {
    return task ? task->getStackSize() : 0;
}

static YuTask*
yumiActiveTaskForHandle(uint64_t handle) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : NULL;
    return task && task->getHandle() == handle ? task : NULL;
}

extern "C" bool
yumiGetTaskFinished(uint64_t handle) {
    if (yumiActiveTaskForHandle(handle)) {
        return false;
    }

    YuTask* task = YuTask::pinHandle(handle);
    if (!task) {
        return true;
    }
    task->unpinHandle();
    return false;
}

extern "C" bool
yumiGetTaskCanceled(uint64_t handle) {
    if (YuTask* active = yumiActiveTaskForHandle(handle)) {
        return active->getCanceled();
    }

    YuTask* task = YuTask::pinHandle(handle);
    if (!task) {
        return false;
    }
    bool canceled = task->getCanceled();
    task->unpinHandle();
    return canceled;
}

extern "C" bool
yumiGetTaskTerminating(uint64_t handle) {
    if (YuTask* active = yumiActiveTaskForHandle(handle)) {
        return active->getTerminating();
    }

    YuTask* task = YuTask::pinHandle(handle);
    if (!task) {
        return false;
    }
    bool terminating = task->getTerminating();
    task->unpinHandle();
    return terminating;
}

extern "C" bool
yumiActiveTaskIsTerminating() {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : NULL;
    return task && task->getTerminating();
}

extern "C" char*
yumiGetActiveTaskMemory() {
    return yumiGetActiveThread()->getActiveTask()->getLocals();
}

extern "C" char*
yumiGetActiveThreadMemory() {
    YuThread* thread = yumiGetActiveThread();
    if (!thread || !thread->getThreadLocals()) {
        fprintf(stderr, "Thread local memory is unavailable without an active Yumi thread\n");
        abort();
    }
    return thread->getThreadLocals();
}

extern "C" int
yumiActiveTaskQueueRemaining() {
    YuThread* thread = yumiGetActiveThread();
    YuTaskQueue* queue = thread ? thread->getTaskQueue() : NULL;
    return queue ? queue->remaining() : 0;
}

extern "C" int
yumiActiveTaskQueueActivity() {
    YuThread* thread = yumiGetActiveThread();
    YuTaskQueue* queue = thread ? thread->getTaskQueue() : NULL;
    return queue ? queue->activity() : 0;
}

extern "C" bool
yumiSuspendTask(uint64_t handle) {
    YuTask* task = yumiActiveTaskForHandle(handle);
    if (!task) {
        fprintf(stderr, "Attempted to suspend a task that is not active on this thread\n");
        abort();
    }
    if (task->getCanceled()) {
        return true;
    }

    YuThread* thread = yumiGetActiveThread();
    thread->suspendTask(task, NULL, YuTaskGCStateParked);

    return task->getCanceled();
}

static void
yumiResumePinnedTask(YuTask* task, bool acrossQueues) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* current = thread ? thread->getActiveTask() : NULL;
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
        thread->suspendTask(current, NULL, YuTaskGCStateParked, true);
    }
}

extern "C" bool
yumiResumeTaskNow(uint64_t handle) {
    if (YuTask* task = YuTask::pinHandle(handle)) {
        yumiResumePinnedTask(task, false);
    }

    return yumiGetActiveThread()->getActiveTask()->getCanceled();
}

extern "C" void
yumiResumeTaskLater(uint64_t handle) {
    if (YuTask* task = YuTask::pinHandle(handle)) {
        task->getTaskQueue()->scheduleTask(task);
        task->unpinHandle();
    }
}

extern "C" void
yumiCancelTask(uint64_t handle) {
    if (YuTask* task = YuTask::pinHandle(handle)) {
        task->cancel();
        yumiResumePinnedTask(task, true);
    }
}

// =============================================================================================

static uint32_t
yumiSuspendActiveTaskUntilReady(int fd, int16_t filter, uint64_t deadlineNs) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return YuWaitCanceled;
    }
    if (deadlineNs && deadlineNs <= yumiMonotonicTimeNanoseconds()) {
        return YuWaitTimedOut;
    }

    YuPollWaiter waiter(task, fd, filter);
    YuPollWaitStart start = gPollLoop->beginWait(&waiter, deadlineNs);
    if (start == YuPollWaitStartReady) {
        return YuWaitReady;
    }
#if defined(__APPLE__)
    if (start == YuPollWaitStartSelect) {
        YuSelectWaiter selectWaiter(task, fd, filter == YuPollRead, deadlineNs);
        gSelectLoop->beginWait(&selectWaiter);
        thread->suspendTask(task, NULL, YuTaskGCStateBlocked);
        gSelectLoop->endWait(&selectWaiter);
        if (task->getCanceled()) {
            return YuWaitCanceled;
        }
        return selectWaiter.timedOut ? YuWaitTimedOut : YuWaitReady;
    }
#endif

    thread->suspendTask(task, NULL, YuTaskGCStateBlocked);
    gPollLoop->endWait(&waiter);

    if (task->getCanceled()) {
        return YuWaitCanceled;
    }
    return waiter.timedOut ? YuWaitTimedOut : YuWaitReady;
}

extern "C" uint32_t
yumiSuspendActiveTaskUntilReadable(int fd, uint64_t deadlineNs) {
    return yumiSuspendActiveTaskUntilReady(fd, YuPollRead, deadlineNs);
}

extern "C" uint32_t
yumiSuspendActiveTaskUntilWritable(int fd, uint64_t deadlineNs) {
    return yumiSuspendActiveTaskUntilReady(fd, YuPollWrite, deadlineNs);
}

extern "C" bool
yumiSuspendActiveTaskUntilProcExit(int pid, YuSemaphore* sem) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    gPollLoop->onProcExit(pid, task);

    if (sem) {
        int cancelled = 0;
        sem->wait(thread, task, &cancelled, sizeof(cancelled), NULL);
        return cancelled || task->getCanceled();
    } else {
        thread->suspendTask(task, NULL, YuTaskGCStateBlocked);
        return task->getCanceled();
    }
}

extern "C" bool
yumiSuspendActiveTaskUntilFileChange(int fd, int type, YuSemaphore* sem) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread->getActiveTask();
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
yumiSuspendActiveTaskUntilSignal(int sig, YuSemaphore* sem) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread->getActiveTask();
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
yumiSuspendActiveTaskUntilTime(long int us, YuSemaphore* sem) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    uint64_t now = yumiMonotonicTimeNanoseconds();
    uint64_t delay = us > 0 ? static_cast<uint64_t>(us) * 1000ull : 0;
    uint64_t deadline = delay > UINT64_MAX - now ? UINT64_MAX : now + delay;
    uint64_t id = yumiRegisterTimerEvent(sem);
    gPollLoop->onTimer(deadline, id, task);

    if (sem) {
        int cancelled = 0;
        sem->wait(thread, task, &cancelled, sizeof(cancelled), NULL);
        bool wasCancelled = cancelled || task->getCanceled();
        if (wasCancelled) {
            gPollLoop->cancelTimer(id);
        }
        yumiClearTimerEvent(sem, id);
        return wasCancelled;
    } else {
        return task->getCanceled();
    }
}

extern "C" uint64_t
yumiMonotonicTimeNanoseconds() {
#if defined(__APPLE__)
    return yumiMachTimeToNanoseconds(mach_continuous_time());
#else
    struct timespec now;
    clock_gettime(CLOCK_BOOTTIME, &now);
    return (uint64_t)now.tv_sec * 1000000000ull + (uint64_t)now.tv_nsec;
#endif
}

extern "C" uint64_t
yumiNextScheduledWorkDeadlineNanoseconds() {
    return gPollLoop ? gPollLoop->nextScheduledDeadlineNanoseconds() : 0;
}

extern "C" bool
yumiSuspendActiveTaskUntilAbsoluteTime(uint64_t deadlineNs, YuSemaphore* sem) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread->getActiveTask();
    if (task->getCanceled()) {
        return true;
    }

    if (deadlineNs <= yumiMonotonicTimeNanoseconds()) {
        return false;
    }

    uint64_t id = yumiRegisterTimerEvent(sem);
    gPollLoop->onTimer(deadlineNs, id, task);

    if (sem) {
        int cancelled = 0;
        sem->wait(thread, task, &cancelled, sizeof(cancelled), NULL);
        bool wasCancelled = cancelled || task->getCanceled();
        if (wasCancelled) {
            gPollLoop->cancelTimer(id);
        }
        yumiClearTimerEvent(sem, id);
        return wasCancelled;
    } else {
        return task->getCanceled();
    }
}

extern "C" void
yumiReleaseEventDescriptor(int fd) {
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
yumiCancelProcExitEventListener(int pid, YuSemaphore* sem) {
    gPollLoop->cancelProcExit(pid);

    if (sem) {
        int cancelled = 1;
        sem->post(&cancelled, sizeof(cancelled), true);
    }
}

extern "C" void
yumiCancelFileChangeEventListener(int fd, int type, YuSemaphore* sem) {
    gPollLoop->cancelFileChange(fd, type);

    if (sem) {
        int cancelled = 1;
        sem->post(&cancelled, sizeof(cancelled), true);
    }
}

extern "C" void
yumiCancelSignalEventListener(int sig, YuSemaphore* sem) {
    gPollLoop->cancelSignal(sig);

    if (sem) {
        int cancelled = 1;
        sem->post(&cancelled, sizeof(cancelled), true);
    }
}

extern "C" void
yumiCancelTimerEventListener(YuSemaphore* sem) {
    uint64_t id = yumiTimerEventIdForSemaphore(sem);
    if (id) {
        gPollLoop->cancelTimer(id);
    }

    if (sem) {
        int cancelled = 1;
        sem->post(&cancelled, sizeof(cancelled), true);
    }
}

// =============================================================================================

// static void
// yumiSemaphoreFinalizer(void* obj, void* clientData) {
//     YuSemaphore* sem = static_cast<YuSemaphore*>(obj);
//     sem->~YuSemaphore();
// }

extern "C" YuSemaphore*
yumiNewSemaphore() {
    // YuSemaphore* sem = new YuSemaphore();
    YuSemaphore* sem = static_cast<YuSemaphore*>(YU_MALLOC(sizeof(YuSemaphore)));
    new (sem) YuSemaphore();
    // GC_REGISTER_FINALIZER(sem, yumiSemaphoreFinalizer, 0, 0, 0);
    return sem;
}

extern "C" bool
yumiWaitForSemaphore(YuSemaphore* sem, void* buf, size_t capacity, YuTaskMutex* mutex) {
    YuThread* thread = yumiGetActiveThread();
    return sem->wait(thread, thread->getActiveTask(), buf, capacity, mutex);
}

extern "C" void
yumiPostSemaphore(YuSemaphore* sem, void* data, size_t sizeOfData) {
    sem->post(data, sizeOfData, false);
}

// *************************************************************************************************

static void
yumiMutexFinalizer(void* obj, void* clientData) {
    YuTaskMutex* mutex = static_cast<YuTaskMutex*>(obj);
    mutex->~YuTaskMutex();
}

extern "C" YuTaskMutex*
yumiNewMutex() {
    YuTaskMutex* mutex = static_cast<YuTaskMutex*>(YU_MALLOC(sizeof(YuTaskMutex)));
    new (mutex) YuTaskMutex();
#ifdef YU_ENABLE_GC
    GC_REGISTER_FINALIZER(mutex, yumiMutexFinalizer, 0, 0, 0);
#endif
    return mutex;
}

extern "C" void
yumiLockMutex(YuTaskMutex* mutex) {
    mutex->lock();
}

extern "C" void
yumiUnlockMutex(YuTaskMutex* mutex) {
    mutex->unlock();
}

// =============================================================================================

extern "C" YuTaskQueue*
yumiNewWorker(int threadCount) {
    YuThread* thread = yumiGetActiveThread();
    YuTaskQueue* queue = thread->getTaskQueue();
    return queue->createQueue(threadCount);
}

extern "C" YuTaskQueue*
yumiActiveTaskQueue() {
    return yumiGetActiveThread()->getTaskQueue();
}

extern "C" int
yumiUsableCoreCount() {
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

static const uint8_t kYuLazyGlobalUnclaimed = 0;
static const uint8_t kYuLazyGlobalReady = 1;
static const uint8_t kYuLazyGlobalClaimed = 2;

struct YuLazyGlobalWaiter {
    YuTask* task;
    YuLazyGlobalWaiter* next;
};

struct YuLazyGlobalClaim {
    YuTask* owner;
    YuLazyGlobalWaiter* waiters;
};

static YuMutex&
lazyGlobalLock() {
    static YuMutex* lock = new YuMutex();
    return *lock;
}

static std::unordered_map<uint8_t*, YuLazyGlobalClaim>&
lazyGlobalClaims() {
    static auto* claims = new std::unordered_map<uint8_t*, YuLazyGlobalClaim>();
    return *claims;
}

extern "C" int
yumiClaimLazyGlobal(uint8_t* state) {
    if (__atomic_load_n(state, __ATOMIC_ACQUIRE) == kYuLazyGlobalReady) {
        return kYuLazyGlobalReady;
    }

    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : NULL;
    YuMutex& lock = lazyGlobalLock();
    lock.lock();
    while (true) {
        uint8_t current = __atomic_load_n(state, __ATOMIC_ACQUIRE);
        if (current == kYuLazyGlobalReady) {
            lock.unlock();
            return kYuLazyGlobalReady;
        }
        if (current == kYuLazyGlobalUnclaimed) {
            lazyGlobalClaims()[state] = { task, NULL };
            __atomic_store_n(state, kYuLazyGlobalClaimed, __ATOMIC_RELAXED);
            lock.unlock();
            return kYuLazyGlobalUnclaimed;
        }

        YuLazyGlobalClaim& claim = lazyGlobalClaims()[state];
        if (claim.owner == task) {
            lock.unlock();
            return kYuLazyGlobalClaimed;
        }
        if (!task) {
            lock.unlock();
            sched_yield();
            lock.lock();
            continue;
        }

        YuLazyGlobalWaiter waiter = { task, claim.waiters };
        claim.waiters = &waiter;
        thread->suspendTask(task, &lock, YuTaskGCStateBlocked);
        thread = yumiGetActiveThread();
        lock.lock();
    }
}

static void
yumiWakeLazyGlobalWaitersLocked(
    std::unordered_map<uint8_t*, YuLazyGlobalClaim>::iterator found) {
    YuLazyGlobalWaiter* waiter = found->second.waiters;
    lazyGlobalClaims().erase(found);
    while (waiter) {
        YuLazyGlobalWaiter* next = waiter->next;
        waiter->task->getTaskQueue()->scheduleTask(waiter->task, true);
        waiter = next;
    }
}

extern "C" void
yumiReleaseLazyGlobal(uint8_t* state) {
    YuMutex& lock = lazyGlobalLock();
    lock.lock();
    __atomic_store_n(state, kYuLazyGlobalReady, __ATOMIC_RELEASE);
    auto found = lazyGlobalClaims().find(state);
    if (found != lazyGlobalClaims().end()) {
        yumiWakeLazyGlobalWaitersLocked(found);
    }
    lock.unlock();
}

extern "C" void
yumiAbandonLazyGlobal(uint8_t* state) {
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : NULL;
    YuMutex& lock = lazyGlobalLock();
    lock.lock();
    auto found = lazyGlobalClaims().find(state);
    if (found != lazyGlobalClaims().end() && found->second.owner == task) {
        uint8_t claimed = kYuLazyGlobalClaimed;
        __atomic_compare_exchange_n(
            state,
            &claimed,
            kYuLazyGlobalUnclaimed,
            false,
            __ATOMIC_RELEASE,
            __ATOMIC_RELAXED);
        yumiWakeLazyGlobalWaitersLocked(found);
    }
    lock.unlock();
}

extern "C" int
yumiGCBackgroundTaskQueueThreadCount() {
    return gSpinQueue ? gSpinQueue->getThreadCount() : 0;
}

extern "C" YuTaskQueue*
yumiGCBackgroundTaskQueue() {
    return gSpinQueue;
}

extern "C" bool
yumiForkTaskInWorker(YuTaskQueue* queue, YuTaskStartFunc func, void* closure, size_t localSize) {
    if (!yumiProcessTerminationRequested() && !queue->isStopped()) {
        queue->startTask(func, closure, localSize);
        return true;
    } else {
        return false;
    }
}

extern "C" bool
yumiForkPriorityTaskInWorker(
    YuTaskQueue* queue,
    YuTaskStartFunc func,
    void* closure,
    size_t localSize) {
    if (!yumiProcessTerminationRequested() && !queue->isStopped()) {
        queue->startTask(
            func,
            closure,
            localSize,
            YuTaskProvenance(
                0,
                NULL,
                0,
                0,
                YU_TASK_FLAG_WORKER,
                YU_TASK_SCHEDULING_PRIORITY));
        return true;
    } else {
        return false;
    }
}

extern "C" bool
yumiForkTaskInWorkerWithInfo(
    YuTaskQueue* queue,
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column) {
    if (!yumiProcessTerminationRequested() && !queue->isStopped()) {
        queue->startTask(
            func,
            closure,
            localSize,
            yumiCreateTaskProvenance(file, line, column, YU_TASK_FLAG_WORKER));
        return true;
    } else {
        return false;
    }
}

extern "C" bool
yumiForkPriorityTaskInWorkerWithInfo(
    YuTaskQueue* queue,
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const char* file,
    uint32_t line,
    uint32_t column) {
    if (!yumiProcessTerminationRequested() && !queue->isStopped()) {
        queue->startTask(
            func,
            closure,
            localSize,
            yumiCreateTaskProvenance(
                file,
                line,
                column,
                YU_TASK_FLAG_WORKER,
                YU_TASK_SCHEDULING_PRIORITY));
        return true;
    } else {
        return false;
    }
}

extern "C" bool
yumiForkGCMarkTaskInWorker(
    YuTaskQueue* queue,
    YuTaskStartFunc func,
    void* closure
) {
    if (!queue || !func || yumiProcessTerminationRequested() || queue->isStopped()) {
        return false;
    }
    queue->startTask(
        func,
        closure,
        0,
        YuTaskProvenance(
            0,
            nullptr,
            0,
            0,
            YU_TASK_FLAG_WORKER | YU_TASK_FLAG_GC_MARK_WORKER,
            YU_TASK_SCHEDULING_NORMAL));
    return true;
}

extern "C" bool
yumiForkGCCleanupTask(YuTaskStartFunc func, void* closure) {
    if (!gSpinQueue || !func || yumiProcessTerminationRequested() || gSpinQueue->isStopped()) {
        return false;
    }
    // Cleanup is a safety-net workload, never scheduler-priority work. It receives its own
    // ordinary worker task so callback suspension and panic recovery use the normal task ABI.
    gSpinQueue->startTask(
        func,
        closure,
        0,
        YuTaskProvenance(
            0,
            nullptr,
            0,
            0,
            YU_TASK_FLAG_WORKER,
            YU_TASK_SCHEDULING_NORMAL));
    return true;
}

extern "C" bool
yumiWorkerGetStatus(YuTaskQueue* queue) {
    return queue->isStopped();
}

extern "C" void
yumiWorkerStop(YuTaskQueue* queue) {
    queue->cancelTasks();
    queue->stop();
    // Joining worker pthreads can overlap a collection initiated by a task already running on
    // that worker. Publish the caller as blocked so the handshake never waits on this join.
    yumiGCEnterRuntimeBlock();
    queue->join();
    yumiGCLeaveRuntimeBlock();
}
