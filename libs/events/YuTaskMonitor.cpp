#include <YuEvents/YuTaskMonitor.h>

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuGCHandshake.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuTaskQueue.h>
#include <YuEvents/YuThread.h>
#include <YuCore/YuCore.h>
#include <YuCore/YuTrace.h>

#include <algorithm>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

// *************************************************************************************************

static const uint64_t kYuMonitorTickNanos = 10000000ull;
static const uint64_t kYuPreemptNanos = 10000000ull;
static const uint64_t kYuLongTaskNanos = 100000000ull;

struct YuTaskMonitor {
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_t thread;
    bool started = false;
    bool stopping = false;
    std::vector<YuThread*> threads;
};

static YuTaskMonitor&
taskMonitor() {
    static YuTaskMonitor* monitor = new YuTaskMonitor();
    return *monitor;
}

std::atomic<bool> gYuTaskMonitorSleeping(false);
static std::atomic<int> gInterruptedThreadCount(0);

// *************************************************************************************************

static void
setSchedulerInterruptFlag() {
    __atomic_fetch_or(
        &yumiInterruptRequestedFlags,
        static_cast<unsigned char>(YumiInterruptSchedulerRequested),
        __ATOMIC_RELEASE);
}

static void
clearSchedulerInterruptFlag() {
    __atomic_fetch_and(
        &yumiInterruptRequestedFlags,
        static_cast<unsigned char>(~YumiInterruptSchedulerRequested),
        __ATOMIC_RELEASE);
}

void
yumiInterruptRequestAdded() {
    if (gInterruptedThreadCount.fetch_add(1, std::memory_order_acq_rel) == 0) {
        setSchedulerInterruptFlag();
    }
}

void
yumiInterruptRequestRemoved() {
    if (gInterruptedThreadCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        clearSchedulerInterruptFlag();
    }
}

static bool
longTaskWarningsEnabled() {
    static int cachedEnabled = -1;
    if (cachedEnabled < 0) {
        const char* value = getenv("YUMI_LONG_TASK_WARNINGS");
        cachedEnabled = value && *value && strcmp(value, "0") != 0;
    }
    return cachedEnabled != 0;
}

static bool
scanMonitoredThreadsLocked(YuTaskMonitor& monitor, uint64_t now, bool request) {
    bool busy = false;
    for (YuThread* thread : monitor.threads) {
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
        YuTaskQueue* queue = thread->getTaskQueue();
        if (queue->isPool()) {
            if (elapsed >= kYuPreemptNanos && queue->hasWaitingTasks()) {
                thread->requestInterrupt(YU_THREAD_INTERRUPT_PREEMPT);
            }
        } else if (longTaskWarningsEnabled() &&
                !thread->monitorWarned &&
                elapsed >= kYuLongTaskNanos &&
                queue->hasWaitingTasks()) {
            thread->monitorWarned = true;
            thread->requestInterrupt(YU_THREAD_INTERRUPT_WARN);
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
    pthread_setname_np("yumi-task-monitor");
#else
    pthread_setname_np(pthread_self(), "yumi-task-mon");
#endif

    YuTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    while (!monitor.stopping) {
        if (gYuTaskMonitorSleeping.load(std::memory_order_seq_cst)) {
            pthread_cond_wait(&monitor.cond, &monitor.lock);
            continue;
        }

#if defined(__APPLE__)
        struct timespec tick = { 0, (long)kYuMonitorTickNanos };
        pthread_cond_timedwait_relative_np(&monitor.cond, &monitor.lock, &tick);
#else
        struct timespec tick;
        clock_gettime(CLOCK_MONOTONIC, &tick);
        tick.tv_nsec += (long)kYuMonitorTickNanos;
        if (tick.tv_nsec >= 1000000000L) {
            tick.tv_sec += 1;
            tick.tv_nsec -= 1000000000L;
        }
        pthread_cond_clockwait(&monitor.cond, &monitor.lock, CLOCK_MONOTONIC, &tick);
#endif
        if (monitor.stopping) {
            break;
        }

        if (scanMonitoredThreadsLocked(monitor, yumiMonotonicTimeNanoseconds(), true)) {
            if (gInterruptedThreadCount.load(std::memory_order_acquire)) {
                setSchedulerInterruptFlag();
            }
            continue;
        }

        gYuTaskMonitorSleeping.store(true, std::memory_order_seq_cst);
        if (scanMonitoredThreadsLocked(monitor, 0, false)) {
            gYuTaskMonitorSleeping.store(false, std::memory_order_seq_cst);
        }
    }
    pthread_mutex_unlock(&monitor.lock);
    return NULL;
}

void
yumiStartTaskMonitor() {
    YuTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    if (!monitor.started) {
        monitor.started = true;
        monitor.stopping = false;
        gYuTaskMonitorSleeping.store(false, std::memory_order_seq_cst);
        pthread_create(&monitor.thread, NULL, runTaskMonitor, NULL);
    }
    pthread_mutex_unlock(&monitor.lock);
}

void
yumiStopTaskMonitor() {
    YuTaskMonitor& monitor = taskMonitor();
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
    gYuTaskMonitorSleeping.store(false, std::memory_order_seq_cst);
    pthread_mutex_unlock(&monitor.lock);
}

void
yumiWakeTaskMonitor() {
    YuTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    if (gYuTaskMonitorSleeping.load(std::memory_order_seq_cst)) {
        gYuTaskMonitorSleeping.store(false, std::memory_order_seq_cst);
        pthread_cond_signal(&monitor.cond);
    }
    pthread_mutex_unlock(&monitor.lock);
}

void
yumiRegisterMonitoredThread(YuThread* thread) {
    YuTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    monitor.threads.push_back(thread);
    pthread_mutex_unlock(&monitor.lock);
}

void
yumiUnregisterMonitoredThread(YuThread* thread) {
    YuTaskMonitor& monitor = taskMonitor();
    pthread_mutex_lock(&monitor.lock);
    monitor.threads.erase(
        std::remove(monitor.threads.begin(), monitor.threads.end(), thread),
        monitor.threads.end());
    pthread_mutex_unlock(&monitor.lock);
}

// *************************************************************************************************

static bool
canPreemptTask(YuThread* thread, YuTask* task) {
    return thread->getWorker() &&
        !task->hasGCManagedCallback() &&
        !task->inGCBlockingContext() &&
        !yumiGCIsRuntimeCritical();
}

void
yumiServiceTaskInterrupts(bool canYield, uintptr_t fp, uintptr_t pc) {
    YuThread* thread = yumiGetActiveThread();
    if (!thread) {
        return;
    }
    uint32_t requests = thread->getInterruptRequests();
    YuTask* task = thread->getActiveTask();
    if (!requests || !task) {
        return;
    }

    if ((requests & YU_THREAD_INTERRUPT_WARN) &&
            thread->takeInterruptRequests(YU_THREAD_INTERRUPT_WARN)) {
        yumiReportLongTask(thread, task, true, fp, pc);
    }

    if ((requests & YU_THREAD_INTERRUPT_PREEMPT) && canYield && canPreemptTask(thread, task)) {
        thread->takeInterruptRequests(YU_THREAD_INTERRUPT_PREEMPT);
        thread->yieldTask(task);
    }
}

void
yumiReportLongTask(YuThread* thread, YuTask* task, bool hasStack, uintptr_t fp, uintptr_t pc) {
    uint64_t now = yumiMonotonicTimeNanoseconds();
    uint64_t since = thread->monitorSinceNanos;
    unsigned long long elapsed = since && now > since
        ? (unsigned long long)((now - since) / 1000000ull)
        : (unsigned long long)(kYuLongTaskNanos / 1000000ull);

    char heading[160];
    snprintf(
        heading,
        sizeof(heading),
        "Warning: task %u ran %llu ms without suspending while other tasks waited",
        task->getId(),
        elapsed);
    if (hasStack) {
        yumiReportTaskStack(heading, task, fp, pc);
    } else {
        yumiReportTaskHeader(heading, task);
    }
}
