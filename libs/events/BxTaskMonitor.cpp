#include <BxEvents/BxTaskMonitor.h>

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxGCHandshake.h>
#include <BxEvents/BxTask.h>
#include <BxEvents/BxTaskQueue.h>
#include <BxEvents/BxThread.h>
#include <BxCore/BxCore.h>
#include <BxCore/BxTrace.h>

#include <algorithm>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

// *************************************************************************************************

static const uint64_t kBxMonitorTickNanos = 10000000ull;
static const uint64_t kBxPreemptNanos = 10000000ull;
static const uint64_t kBxLongTaskNanos = 100000000ull;

struct BxTaskMonitor {
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_t thread;
    bool started = false;
    bool stopping = false;
    std::vector<BxThread*> threads;
};

static BxTaskMonitor&
taskMonitor() {
    static BxTaskMonitor* monitor = new BxTaskMonitor();
    return *monitor;
}

std::atomic<bool> gBxTaskMonitorSleeping(false);
static std::atomic<int> gInterruptedThreadCount(0);

// *************************************************************************************************

static void
setSchedulerInterruptFlag() {
    __atomic_fetch_or(
        &brixInterruptRequestedFlags,
        static_cast<unsigned char>(BrixInterruptSchedulerRequested),
        __ATOMIC_RELEASE);
}

static void
clearSchedulerInterruptFlag() {
    __atomic_fetch_and(
        &brixInterruptRequestedFlags,
        static_cast<unsigned char>(~BrixInterruptSchedulerRequested),
        __ATOMIC_RELEASE);
}

void
brixInterruptRequestAdded() {
    if (gInterruptedThreadCount.fetch_add(1, std::memory_order_acq_rel) == 0) {
        setSchedulerInterruptFlag();
    }
}

void
brixInterruptRequestRemoved() {
    if (gInterruptedThreadCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        clearSchedulerInterruptFlag();
    }
}

static bool
longTaskWarningsEnabled() {
    static int cachedEnabled = -1;
    if (cachedEnabled < 0) {
        const char* value = getenv("BRIX_LONG_TASK_WARNINGS");
        cachedEnabled = value && *value && strcmp(value, "0") != 0;
    }
    return cachedEnabled != 0;
}

static bool
scanMonitoredThreadsLocked(BxTaskMonitor& monitor, uint64_t now, bool request) {
    bool busy = false;
    for (BxThread* thread : monitor.threads) {
        uint32_t count = thread->getSwitchCount();
        if (!(count & 1)) {
            thread->monitorSwitchCount = count;
            continue;
        }

        busy = true;
        if (!request) {
            continue;
        }
        if (count != thread->monitorSwitchCount) {
            thread->monitorSwitchCount = count;
            thread->monitorSinceNanos = now;
            thread->monitorWarned = false;
            continue;
        }

        uint64_t elapsed = now - thread->monitorSinceNanos;
        BxTaskQueue* queue = thread->getTaskQueue();
        if (queue->isPool()) {
            if (elapsed >= kBxPreemptNanos && queue->hasWaitingTasks()) {
                thread->requestInterrupt(BX_THREAD_INTERRUPT_PREEMPT);
            }
        } else if (longTaskWarningsEnabled() &&
                !thread->monitorWarned &&
                elapsed >= kBxLongTaskNanos &&
                queue->hasWaitingTasks()) {
            thread->monitorWarned = true;
            thread->requestInterrupt(BX_THREAD_INTERRUPT_WARN);
        }
    }
    return busy;
}

static void*
runTaskMonitor(void*) {
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
#if defined(__APPLE__)
    pthread_setname_np("brix-task-monitor");
#else
    pthread_setname_np(pthread_self(), "brix-task-mon");
#endif

    BxTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    while (!monitor.stopping) {
        if (gBxTaskMonitorSleeping.load(std::memory_order_seq_cst)) {
            pthread_cond_wait(&monitor.cond, &monitor.lock);
            continue;
        }

#if defined(__APPLE__)
        struct timespec tick = { 0, (long)kBxMonitorTickNanos };
        pthread_cond_timedwait_relative_np(&monitor.cond, &monitor.lock, &tick);
#else
        struct timespec tick;
        clock_gettime(CLOCK_MONOTONIC, &tick);
        tick.tv_nsec += (long)kBxMonitorTickNanos;
        if (tick.tv_nsec >= 1000000000L) {
            tick.tv_sec += 1;
            tick.tv_nsec -= 1000000000L;
        }
        pthread_cond_clockwait(&monitor.cond, &monitor.lock, CLOCK_MONOTONIC, &tick);
#endif
        if (monitor.stopping) {
            break;
        }

        if (scanMonitoredThreadsLocked(monitor, brixMonotonicTimeNanoseconds(), true)) {
            if (gInterruptedThreadCount.load(std::memory_order_acquire)) {
                setSchedulerInterruptFlag();
            }
            continue;
        }

        gBxTaskMonitorSleeping.store(true, std::memory_order_seq_cst);
        if (scanMonitoredThreadsLocked(monitor, 0, false)) {
            gBxTaskMonitorSleeping.store(false, std::memory_order_seq_cst);
        }
    }
    pthread_mutex_unlock(&monitor.lock);
    return NULL;
}

void
brixStartTaskMonitor() {
    BxTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    if (!monitor.started) {
        monitor.started = true;
        monitor.stopping = false;
        gBxTaskMonitorSleeping.store(false, std::memory_order_seq_cst);
        pthread_create(&monitor.thread, NULL, runTaskMonitor, NULL);
    }
    pthread_mutex_unlock(&monitor.lock);
}

void
brixStopTaskMonitor() {
    BxTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    if (!monitor.started) {
        pthread_mutex_unlock(&monitor.lock);
        return;
    }
    monitor.stopping = true;
    pthread_cond_signal(&monitor.cond);
    pthread_mutex_unlock(&monitor.lock);

    pthread_join(monitor.thread, NULL);

    pthread_mutex_lock(&monitor.lock);
    monitor.started = false;
    gBxTaskMonitorSleeping.store(false, std::memory_order_seq_cst);
    pthread_mutex_unlock(&monitor.lock);
}

void
brixWakeTaskMonitor() {
    BxTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    if (gBxTaskMonitorSleeping.load(std::memory_order_seq_cst)) {
        gBxTaskMonitorSleeping.store(false, std::memory_order_seq_cst);
        pthread_cond_signal(&monitor.cond);
    }
    pthread_mutex_unlock(&monitor.lock);
}

void
brixRegisterMonitoredThread(BxThread* thread) {
    BxTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    monitor.threads.push_back(thread);
    pthread_mutex_unlock(&monitor.lock);
}

void
brixUnregisterMonitoredThread(BxThread* thread) {
    BxTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    monitor.threads.erase(
        std::remove(monitor.threads.begin(), monitor.threads.end(), thread),
        monitor.threads.end());
    pthread_mutex_unlock(&monitor.lock);
}

// *************************************************************************************************

static bool
canPreemptTask(BxThread* thread, BxTask* task) {
    return thread->getWorker() &&
        !task->hasGCManagedCallback() &&
        !task->inGCBlockingContext() &&
        !brixGCIsRuntimeCritical();
}

void
brixServiceTaskInterrupts(bool canYield, uintptr_t fp, uintptr_t pc) {
    BxThread* thread = brixGetActiveThread();
    if (!thread) {
        return;
    }
    uint32_t requests = thread->getInterruptRequests();
    BxTask* task = thread->getActiveTask();
    if (!requests || !task) {
        return;
    }

    if ((requests & BX_THREAD_INTERRUPT_WARN) &&
            thread->takeInterruptRequests(BX_THREAD_INTERRUPT_WARN)) {
        brixReportLongTask(thread, task, true, fp, pc);
    }

    if ((requests & BX_THREAD_INTERRUPT_PREEMPT) && canYield && canPreemptTask(thread, task)) {
        thread->takeInterruptRequests(BX_THREAD_INTERRUPT_PREEMPT);
        thread->yieldTask(task);
    }
}

void
brixReportLongTask(BxThread* thread, BxTask* task, bool hasStack, uintptr_t fp, uintptr_t pc) {
    uint64_t now = brixMonotonicTimeNanoseconds();
    uint64_t since = thread->monitorSinceNanos;
    unsigned long long elapsed = since && now > since
        ? (unsigned long long)((now - since) / 1000000ull)
        : (unsigned long long)(kBxLongTaskNanos / 1000000ull);

    char heading[160];
    snprintf(
        heading,
        sizeof(heading),
        "Warning: task %u ran %llu ms without suspending while other tasks waited",
        task->getId(),
        elapsed);
    if (hasStack) {
        brixReportTaskStack(heading, task, fp, pc);
    } else {
        brixReportTaskHeader(heading, task);
    }
}
