#include <YuEvents/YuGCHandshake.h>
#include <YuEvents/YuTaskMonitor.h>
#include <YuCore/YuCore.h>

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuLock.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuThread.h>
#include <YuGC/YuGCRuntime.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <pthread.h>
#include <vector>

#define YU_STRINGIFY_VALUE(value) #value
#define YU_STRINGIFY(value) YU_STRINGIFY_VALUE(value)
#define YU_ASM_SYMBOL(name) YU_STRINGIFY(__USER_LABEL_PREFIX__) #name

// *************************************************************************************************

extern "C" void __attribute__((weak))
yumiGCRuntimeFlushThreadAllocationBatches(void) {
}

extern "C" void __attribute__((weak))
yumiGCRuntimeFlushWriteBarrierBuffer(void) {
}

extern "C" void __attribute__((weak))
yumiGCRuntimeRecordPerTaskSetupScan(size_t, size_t) {
}

extern "C" void __attribute__((weak))
yumiGCPublishHandshakeRequest(size_t reason, size_t epoch, bool requested) {
    uint64_t current = __atomic_load_n(&yumiGCPublicControlWord, __ATOMIC_RELAXED);
    while (true) {
        uint64_t next = yumiGCPackControlWordFull(
            yumiGCControlWordPhase(current),
            yumiGCControlWordSweepGeneration(current),
            requested,
            requested ? reason : 0,
            requested ? epoch : 0) |
            (current & YumiGCControlTerminationRequestedBit);
        if (__atomic_compare_exchange_n(
                &yumiGCPublicControlWord,
                &current,
                next,
                false,
                __ATOMIC_RELEASE,
                __ATOMIC_RELAXED)) {
            return;
        }
    }
}

// The collector supplies MarkSetup work. Keeping this hook at the scheduler boundary lets a
// Running task scan its own continuation before acknowledging the epoch and immediately
// continuing, without making the events runtime depend on collector internals.
extern "C" void __attribute__((weak))
yumiGCRuntimeHandshakeTaskWork(uint32_t) {
}

static pthread_mutex_t gGCHandshakeLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gGCHandshakeCond = PTHREAD_COND_INITIALIZER;
static uint32_t gGCHandshakeReason = 0;
static uint64_t gGCHandshakeEpoch = 0;
static YuTask* gGCHandshakeOwnerTask = NULL;
static YuThread* gGCHandshakeOwnerThread = NULL;
static size_t gGCHandshakeTasksWaited = 0;
static uint64_t gGCHandshakeSlowestTaskNanos = 0;
static size_t gGCHandshakeReportCount = 0;
static const size_t kYuGCTaskShardCount = 64;
struct alignas(64) YuGCTaskShard {
    YuLock lock;
    std::vector<YuTask*> tasks;
};
static YuGCTaskShard gGCTaskShards[kYuGCTaskShardCount];
enum YuGCHandshakeTaskDisposition {
    YuGCHandshakeTaskAwaiting = 0,
    YuGCHandshakeTaskSelfAcknowledged = 1,
    YuGCHandshakeTaskSnapshotAcknowledged = 2,
    YuGCHandshakeTaskDead = 3,
    YuGCHandshakeTaskOwner = 4
};
struct YuGCHandshakeTaskRecord {
    YuTask* task;
    YuGCHandshakeTaskDisposition disposition;
};
static std::vector<YuGCHandshakeTaskRecord> gGCHandshakeTasks;
static std::vector<YuThread*> gGCThreads;
static std::vector<YuThread*> gGCThreadLocalCallbacks;
static thread_local uint32_t gGCThreadLocalCallbackDepth = 0;
static thread_local uint32_t gGCRuntimeCriticalDepth = 0;
static thread_local uint32_t gGCBlockingDepth = 0;
static thread_local uint64_t gGCBlockingEntryControl = 0;
enum YuGCManagedCallbackKind {
    YuGCManagedCallbackNone = 0,
    YuGCManagedCallbackBlocked = 1,
    YuGCManagedCallbackTaskless = 2,
    YuGCManagedCallbackInZone = 3
};
static thread_local uint32_t gGCManagedCallbackDepth = 0;
static thread_local YuGCManagedCallbackKind gGCManagedCallbackKind =
    YuGCManagedCallbackNone;
static thread_local std::vector<uint64_t> gGCManagedCallbackRegisters;

struct YuGCHandshakeWaitSample {
    size_t recordIndex;
    uint64_t startNanos;
};

struct YuGCScanData {
    YuGCStackRootVisitor visitor;
    void* data;
};

struct YuGCStorageRoot {
    void* base;
    void* descriptor;
};

struct YuGCStorageRootCollectData {
    std::vector<YuGCStorageRoot>* roots;
};

struct YuGCTaskSnapshot {
    YuTask* task;
    uint32_t state;
    void* stackBase;
    size_t stackSize;
    void* context;
};

static YuTask* yumiGCActiveTask();
static uint64_t yumiGCPublicControlSnapshot();
static bool yumiGCHandshakeRequestedPublic();
static void yumiGCAcknowledgeRunningTask(YuTask* task, uint32_t reason, uint64_t epoch);

static bool
yumiGCHandshakeActiveLocked() {
    return gGCHandshakeReason != 0;
}

static YuGCTaskShard&
yumiGCTaskShard(YuTask* task) {
    return gGCTaskShards[task->getId() % kYuGCTaskShardCount];
}

template <typename Visit>
static void
yumiGCForEachRegisteredTask(Visit visit) {
    for (YuGCTaskShard& shard : gGCTaskShards) {
        shard.lock.lock();
        for (YuTask* task : shard.tasks) {
            visit(task);
        }
        shard.lock.unlock();
    }
}

static size_t
yumiGCRegisteredTaskCount() {
    size_t count = 0;
    for (YuGCTaskShard& shard : gGCTaskShards) {
        shard.lock.lock();
        count += shard.tasks.size();
        shard.lock.unlock();
    }
    return count;
}

static YuGCHandshakeTaskRecord*
yumiGCHandshakeTaskRecordLocked(YuTask* task) {
    size_t index = task->getGCHandshakeRecordIndex();
    if (index < gGCHandshakeTasks.size() && gGCHandshakeTasks[index].task == task) {
        return &gGCHandshakeTasks[index];
    }
    return nullptr;
}

static void
yumiGCAddHandshakeTaskRecordLocked(YuTask* task, YuGCHandshakeTaskDisposition disposition) {
    task->setGCHandshakeRecordIndex(gGCHandshakeTasks.size());
    gGCHandshakeTasks.push_back({task, disposition});
}

static uint64_t
yumiGCHandshakeMonotonicNanos() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<uint64_t>(now.tv_sec) * 1000000000ull +
        static_cast<uint64_t>(now.tv_nsec);
}

extern "C" bool
yumiGCIsRuntimeCritical() {
    return gGCRuntimeCriticalDepth != 0;
}

static void
yumiGCContextSnapshotFromYuContext(const YuContext* context, YumiGCContextSnapshot* snapshot) {
    std::memset(snapshot, 0, sizeof(*snapshot));
    if (!context) {
        return;
    }

    snapshot->arg = context->arg;
    snapshot->sp = context->sp;
    snapshot->fp = context->fp;
    snapshot->pc = NULL;
    snapshot->x[19] = context->x19;
    snapshot->x[20] = context->x20;
    snapshot->x[21] = context->x21;
    snapshot->x[22] = context->x22;
    snapshot->x[23] = context->x23;
    snapshot->x[24] = context->x24;
    snapshot->x[25] = context->x25;
    snapshot->x[26] = context->x26;
    snapshot->x[27] = context->x27;
    snapshot->x[28] = context->x28;
}

static void
yumiGCScanEventContextRoots(
    YuContext* context,
    void* stackBase,
    size_t stackSize,
    bool useCurrentSafepoint,
    YuGCStackRootVisitor visitor,
    void* data
) {
    if (!context || !visitor) {
        return;
    }

    YumiGCContextSnapshot snapshot;
    yumiGCContextSnapshotFromYuContext(context, &snapshot);
    if (useCurrentSafepoint) {
        snapshot.pc = context->pc;
    }
    yumiGCScanContextRoots(&snapshot, stackBase, stackSize, visitor, data);
}

static void
yumiGCScanEventConservativeContextRoots(
    YuContext* context,
    void* stackBase,
    size_t stackSize,
    YuGCStackRootVisitor visitor,
    void* data
) {
    if (!context || !visitor) {
        return;
    }

    YumiGCContextSnapshot snapshot;
    yumiGCContextSnapshotFromYuContext(context, &snapshot);
    yumiGCScanConservativeContextRoots(&snapshot, stackBase, stackSize, visitor, data);
}

static void
yumiGCScanTaskRootsVisitor(
    YuTask*,
    uint32_t state,
    void* stackBase,
    size_t stackSize,
    void* context,
    void* data
) {
    YuGCScanData* scan = static_cast<YuGCScanData*>(data);
    yumiGCScanEventContextRoots(
        static_cast<YuContext*>(context),
        stackBase,
        stackSize,
        state == YuTaskGCStateInZone || state == YuTaskGCStateBlocked,
        scan->visitor,
        scan->data);
}

static void
yumiGCScanTaskConservativeRootsVisitor(
    YuTask*,
    uint32_t,
    void* stackBase,
    size_t stackSize,
    void* context,
    void* data
) {
    YuGCScanData* scan = static_cast<YuGCScanData*>(data);
    yumiGCScanEventConservativeContextRoots(
        static_cast<YuContext*>(context),
        stackBase,
        stackSize,
        scan->visitor,
        scan->data);
}

static void
yumiGCCollectStorageRootRecord(const YumiGCStorageRootRecord* record, void* data) {
    YuGCStorageRootCollectData* collect = static_cast<YuGCStorageRootCollectData*>(data);
    if (!record || !collect || !collect->roots) {
        return;
    }

    if (record->kind == 1) {
        collect->roots->push_back({ record->base, record->descriptor });
    } else if (record->kind == 2) {
        yumiGCForEachRegisteredTask([&](YuTask* task) {
            uint32_t state = task->getGCState();
            if (
                state != YuTaskGCStateDead &&
                task->getLocals() &&
                task->getLocalsSize()
            ) {
                collect->roots->push_back({ task->getLocals(), record->descriptor });
            }
        });
    } else if (record->kind == 3) {
        for (YuThread* thread : gGCThreads) {
            if (thread->getThreadLocals() && thread->getThreadLocalsSize()) {
                collect->roots->push_back({ thread->getThreadLocals(), record->descriptor });
            }
        }
    }
}

// *************************************************************************************************

static bool
yumiGCIsStableTaskState(uint32_t state) {
    return state == YuTaskGCStateInZone ||
        state == YuTaskGCStateBlocked ||
        state == YuTaskGCStateParked ||
        state == YuTaskGCStateDead;
}

static bool
yumiGCIsEnumerableTaskState(uint32_t state) {
    return
        state == YuTaskGCStateInZone ||
        state == YuTaskGCStateBlocked ||
        state == YuTaskGCStateParked;
}

static void
yumiGCPublishBlockingContext(
    uint32_t state,
    const YuTaskGCBlockingContext* managedContext = NULL
) {
    YuTask* task = yumiGCActiveTask();
    if (!task) {
        return;
    }

    if (yumiGCHandshakeRequestedPublic()) {
        pthread_mutex_lock(&gGCHandshakeLock);
        bool handshakeRequested = yumiGCHandshakeActiveLocked();
        bool isHandshakeOwner = handshakeRequested && task == gGCHandshakeOwnerTask;
        pthread_mutex_unlock(&gGCHandshakeLock);
        if (isHandshakeOwner) {
            return;
        }
    }

    // InZone/Blocked entry is a pointer-publication boundary. Donate buffered edges before
    // capturing the continuation, but retain thread-owned allocation ranges while the packed
    // control word is stable. Leave/resume revokes them if phase or generation changed, and a
    // collector handshake still performs a full-thread range flush.
    yumiGCRuntimeFlushWriteBarrierBuffer();
    YuContext* context = task->getContext();
    if (managedContext && managedContext->hasSnapshot) {
        context->func = NULL;
        context->arg = NULL;
        context->sp = managedContext->stackPointer;
        context->fp = managedContext->framePointer;
        context->lr = managedContext->returnPC;
        context->pc = managedContext->returnPC;
        context->x19 = managedContext->registers[0];
        context->x20 = managedContext->registers[1];
        context->x21 = managedContext->registers[2];
        context->x22 = managedContext->registers[3];
        context->x23 = managedContext->registers[4];
        context->x24 = managedContext->registers[5];
        context->x25 = managedContext->registers[6];
        context->x26 = managedContext->registers[7];
        context->x27 = managedContext->registers[8];
        context->x28 = managedContext->registers[9];
    } else {
        yumiContextGet(context);
    }
    yumiGCSetTaskStateAndContext(task, state, true);
}

static uint64_t
yumiGCPublicControlSnapshot() {
    return __atomic_load_n(&yumiGCPublicControlWord, __ATOMIC_ACQUIRE);
}

static bool
yumiGCHandshakeRequestedPublic() {
    return yumiGCControlWordHandshakeRequested(yumiGCPublicControlSnapshot());
}

static bool
yumiGCTaskAcknowledgedControl(YuTask* task, uint64_t control) {
    uint64_t epochBits = YumiGCControlHandshakeEpochMask >> YumiGCControlHandshakeEpochShift;
    return task &&
        (task->getGCHandshakeEpoch() & epochBits) == yumiGCControlWordHandshakeEpoch(control);
}

extern "C" bool
yumiGCActiveTaskAcknowledgedHandshake(uint64_t control) {
    return yumiGCTaskAcknowledgedControl(yumiGCActiveTask(), control);
}

static void
yumiGCEnterBlockingContext(
    uint32_t state,
    void* managedFramePointer = NULL,
    void* managedReturnPC = NULL,
    void* managedStackPointer = NULL,
    const uintptr_t* managedRegisters = NULL
) {
    YuTask* task = yumiGCActiveTask();
    uint32_t previousDepth = task
        ? task->enterGCBlockingContext()
        : gGCBlockingDepth++;
    const YuTaskGCBlockingContext* publishedContext = NULL;
    if (task) {
        task->setGCBlockingContextSnapshot(
            managedFramePointer,
            managedReturnPC,
            managedStackPointer,
            managedRegisters);
        publishedContext = task->getGCBlockingContextSnapshot();
    }
    if (previousDepth != 0) {
        // A managed callback temporarily resumes the task inside an outer FFI bracket. A
        // blocking call made by that callback needs its own enumerable context; inheriting the
        // outer depth would otherwise leave the task Running while native code is blocked.
        if (task && task->hasGCManagedCallback() &&
            task->getGCState() == YuTaskGCStateRunning) {
            task->markGCBlockingContextManagedCallback();
            task->setGCBlockingEntryControl(yumiGCPublicControlSnapshot());
            yumiGCPublishBlockingContext(state, publishedContext);
        }
        return;
    }
    uint64_t control = yumiGCPublicControlSnapshot();
    if (task) {
        task->setGCBlockingEntryControl(control);
    } else {
        gGCBlockingEntryControl = control;
    }
    yumiGCPublishBlockingContext(state, publishedContext);
}

static void
yumiGCResumeFromBlockingContext(YuTask* task, uint64_t entryControl) {
    bool allocationRangesRevoked = false;
    if (yumiGCPublicControlSnapshot() != entryControl) {
        yumiGCRuntimeFlushThreadAllocationBatches();
        allocationRangesRevoked = true;
    }

    // This may wait for a handshake that began after the comparison above. Recheck before
    // returning to Yumi so an Idle-issued range can never be consumed in a later phase.
    yumiGCPrepareTaskResume(task);
    if (!allocationRangesRevoked && yumiGCPublicControlSnapshot() != entryControl) {
        yumiGCRuntimeFlushThreadAllocationBatches();
    }
}

static void
yumiGCLeaveBlockingContext() {
    YuTask* task = yumiGCActiveTask();
    bool managedCallbackContext = false;
    uint64_t entryControl = 0;
    bool outermost = task
        ? task->leaveGCBlockingContext(managedCallbackContext, entryControl)
        : (gGCBlockingDepth != 0 && --gGCBlockingDepth == 0);
    if (!outermost) {
        if (task && managedCallbackContext) {
            yumiGCResumeFromBlockingContext(task, entryControl);
        }
        return;
    }

    if (!task) {
        entryControl = gGCBlockingEntryControl;
        gGCBlockingEntryControl = 0;
    }
    yumiGCResumeFromBlockingContext(task, entryControl);
}

static void
yumiGCStoreTaskState(YuTask* task, uint32_t state, bool contextReady) {
    if (!contextReady) {
        task->setGCContextReady(false);
        task->setGCState(state);
    } else {
        task->setGCContextReady(true);
        task->setGCState(state);
    }
}

static YuTask*
yumiGCActiveTask() {
    YuThread* thread = yumiGetActiveThread();
    return thread ? thread->getActiveTask() : NULL;
}

extern "C" bool
yumiGCActiveTaskCanUseFastAllocation() {
    YuTask* task = yumiGCActiveTask();
    return task && task->getActive() && task->getGCState() == YuTaskGCStateRunning;
}

static bool
yumiGCAllTasksAcknowledgedLocked(YuTask* currentTask) {
    for (const YuGCHandshakeTaskRecord& record : gGCHandshakeTasks) {
        YuTask* task = record.task;
        if (task == currentTask ||
                record.disposition == YuGCHandshakeTaskOwner ||
                record.disposition == YuGCHandshakeTaskDead) {
            continue;
        }
        uint32_t state = task->getGCState();
        if (state == YuTaskGCStateDead) {
            continue;
        }
        if (record.disposition == YuGCHandshakeTaskAwaiting ||
                task->getGCHandshakeEpoch() != gGCHandshakeEpoch) {
            return false;
        }
        if (record.disposition == YuGCHandshakeTaskSnapshotAcknowledged &&
                (!task->getGCContextReady() || !yumiGCIsEnumerableTaskState(state))) {
            return false;
        }
    }
    return true;
}

static bool
yumiGCThreadLocalCallbacksStableLocked(YuThread* currentThread) {
    for (YuThread* thread : gGCThreadLocalCallbacks) {
        if (thread != currentThread) {
            return false;
        }
    }
    return true;
}

static bool
yumiGCHandshakeReadyLocked(YuTask* currentTask, YuThread* currentThread) {
    return yumiGCAllTasksAcknowledgedLocked(currentTask) &&
        yumiGCThreadLocalCallbacksStableLocked(currentThread);
}

static bool
yumiGCTaskWaitCompleteLocked(size_t recordIndex) {
    if (recordIndex >= gGCHandshakeTasks.size()) {
        return true;
    }
    const YuGCHandshakeTaskRecord& record = gGCHandshakeTasks[recordIndex];
    return record.disposition != YuGCHandshakeTaskAwaiting ||
        record.task->getGCState() == YuTaskGCStateDead;
}

static void
yumiGCStartTaskWaitSamplesLocked(
    YuTask* currentTask,
    uint64_t startNanos,
    std::vector<YuGCHandshakeWaitSample>& samples
) {
    samples.clear();
    for (size_t index = 0; index < gGCHandshakeTasks.size(); ++index) {
        const YuGCHandshakeTaskRecord& record = gGCHandshakeTasks[index];
        if (record.task == currentTask || record.disposition != YuGCHandshakeTaskAwaiting) {
            continue;
        }
        samples.push_back({index, startNanos});
    }
}

static void
yumiGCUpdateTaskWaitSamplesLocked(
    uint64_t nowNanos,
    std::vector<YuGCHandshakeWaitSample>& samples
) {
    for (YuGCHandshakeWaitSample& sample : samples) {
        if (sample.startNanos && yumiGCTaskWaitCompleteLocked(sample.recordIndex)) {
            gGCHandshakeSlowestTaskNanos = std::max(
                gGCHandshakeSlowestTaskNanos,
                nowNanos - sample.startNanos);
            sample.startNanos = 0;
        }
    }
}

static const char*
yumiGCTaskGCStateName(uint32_t state) {
    switch (state) {
    case YuTaskGCStateRunning: return "Running";
    case YuTaskGCStateInZone: return "InZone";
    case YuTaskGCStateBlocked: return "Blocked";
    case YuTaskGCStateParked: return "Parked";
    case YuTaskGCStateDead: return "Dead";
    default: return "Invalid";
    }
}

static void
yumiGCReportUnresponsiveTasksLocked(YuTask* currentTask, uint64_t nowNanos) {
    ++gGCHandshakeReportCount;
    bool reported = false;
    for (const YuGCHandshakeTaskRecord& record : gGCHandshakeTasks) {
        YuTask* task = record.task;
        if (task == currentTask || record.disposition != YuGCHandshakeTaskAwaiting) {
            continue;
        }

        uint32_t state = task->getGCState();
        if (state == YuTaskGCStateDead) {
            continue;
        }
        uint64_t lastPoll = task->getGCLastPollNanos();
        uint64_t sincePoll = lastPoll && nowNanos > lastPoll ? nowNanos - lastPoll : 0;
        std::fprintf(
            stderr,
            "Yumi GC handshake waiting: epoch=%llu reason=%u task=%u state=%s "
            "lastPollNanos=%llu sincePollNanos=%llu contextReady=%d spawn=%s:%u:%u\n",
            (unsigned long long)gGCHandshakeEpoch,
            gGCHandshakeReason,
            task->getId(),
            yumiGCTaskGCStateName(state),
            (unsigned long long)lastPoll,
            (unsigned long long)sincePoll,
            task->getGCContextReady() ? 1 : 0,
            task->getSpawnFile() ? task->getSpawnFile() : "<unknown>",
            task->getSpawnLine(),
            task->getSpawnColumn());
        reported = true;
    }
    if (!reported && !yumiGCThreadLocalCallbacksStableLocked(gGCHandshakeOwnerThread)) {
        std::fprintf(
            stderr,
            "Yumi GC handshake waiting: epoch=%llu reason=%u tasklessCallbacks=%zu\n",
            (unsigned long long)gGCHandshakeEpoch,
            gGCHandshakeReason,
            gGCThreadLocalCallbacks.size());
    }
}

static timespec
yumiGCHandshakeDeadline(uint64_t timeoutUsec) {
    timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);

    deadline.tv_sec += timeoutUsec / 1000000;
    deadline.tv_nsec += (timeoutUsec % 1000000) * 1000;
    if (deadline.tv_nsec >= 1000000000) {
        deadline.tv_sec += deadline.tv_nsec / 1000000000;
        deadline.tv_nsec %= 1000000000;
    }

    return deadline;
}

static uint64_t
yumiGCDefaultHandshakeReportAfterUsec() {
    const char* value = std::getenv("YUMI_GC_HANDSHAKE_REPORT_USEC");
    if (!value || !*value) {
        return 100000;
    }

    char* end = NULL;
    unsigned long long parsed = std::strtoull(value, &end, 10);
    if (!parsed || (end && *end)) {
        return 100000;
    }

    return (uint64_t)parsed;
}

static void
yumiGCAcknowledgeRunningTask(YuTask* task, uint32_t reason, uint64_t epoch) {
    if (!task) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    YuGCHandshakeTaskRecord* record = yumiGCHandshakeTaskRecordLocked(task);
    bool shouldAcknowledge =
        yumiGCHandshakeActiveLocked() &&
        gGCHandshakeEpoch == epoch &&
        task != gGCHandshakeOwnerTask &&
        record &&
        record->disposition == YuGCHandshakeTaskAwaiting;
    pthread_mutex_unlock(&gGCHandshakeLock);
    if (!shouldAcknowledge) {
        return;
    }

    yumiGCRuntimeFlushThreadAllocationBatches();
    if (reason == YuGCHandshakeReasonMarkSetup) {
        yumiGCRuntimeHandshakeTaskWork(reason);
    }
    uint64_t nowNanos = yumiGCHandshakeMonotonicNanos();

    pthread_mutex_lock(&gGCHandshakeLock);
    record = yumiGCHandshakeTaskRecordLocked(task);
    if (record &&
            record->disposition == YuGCHandshakeTaskAwaiting &&
            yumiGCHandshakeActiveLocked() &&
            gGCHandshakeEpoch == epoch &&
            task != gGCHandshakeOwnerTask) {
        task->setGCLastPollNanos(nowNanos);
        task->setGCHandshakeEpoch(epoch);
        record->disposition = YuGCHandshakeTaskSelfAcknowledged;
        pthread_cond_broadcast(&gGCHandshakeCond);
    }
    pthread_mutex_unlock(&gGCHandshakeLock);
}

static void
yumiGCHandshakeBeginInternal(uint32_t reason, uint64_t reportAfterUsec) {
    YuTask* currentTask = yumiGCActiveTask();
    YuThread* currentThread = yumiGetActiveThread();

    // The owner does not execute the ordinary safepoint acknowledgement path. Donate its
    // buffered pointer publications before it becomes the scanning owner for this epoch.
    yumiGCRuntimeFlushThreadAllocationBatches();

    while (true) {
        pthread_mutex_lock(&gGCHandshakeLock);
        if (!yumiGCHandshakeActiveLocked()) {
            break;
        }
        if (currentTask == gGCHandshakeOwnerTask) {
            pthread_mutex_unlock(&gGCHandshakeLock);
            return;
        }

        uint32_t activeReason = gGCHandshakeReason;
        uint64_t activeEpoch = gGCHandshakeEpoch;
        pthread_mutex_unlock(&gGCHandshakeLock);
        yumiGCAcknowledgeRunningTask(currentTask, activeReason, activeEpoch);

        pthread_mutex_lock(&gGCHandshakeLock);
        while (yumiGCHandshakeActiveLocked() && gGCHandshakeEpoch == activeEpoch) {
            pthread_cond_wait(&gGCHandshakeCond, &gGCHandshakeLock);
        }
        pthread_mutex_unlock(&gGCHandshakeLock);
    }

    gGCHandshakeReason = reason;
    gGCHandshakeOwnerTask = currentTask;
    gGCHandshakeOwnerThread = currentThread;
    ++gGCHandshakeEpoch;
    if (!(gGCHandshakeEpoch & (YumiGCControlHandshakeEpochMask >> YumiGCControlHandshakeEpochShift))) {
        ++gGCHandshakeEpoch;
    }
    gGCHandshakeTasks.clear();
    gGCHandshakeTasks.reserve(yumiGCRegisteredTaskCount());
    uint64_t taskWaitStartNanos = yumiGCHandshakeMonotonicNanos();
    yumiGCPublishHandshakeRequest(reason, gGCHandshakeEpoch, true);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    yumiGCForEachRegisteredTask([&](YuTask* task) {
        uint32_t state = task->getGCState();
        YuGCHandshakeTaskDisposition disposition = YuGCHandshakeTaskAwaiting;
        if (task == currentTask) {
            disposition = YuGCHandshakeTaskOwner;
        } else if (state == YuTaskGCStateDead) {
            disposition = YuGCHandshakeTaskDead;
        } else if (yumiGCIsEnumerableTaskState(state) && task->getGCContextReady()) {
            disposition = YuGCHandshakeTaskSnapshotAcknowledged;
        }
        yumiGCAddHandshakeTaskRecordLocked(task, disposition);
        if (disposition == YuGCHandshakeTaskAwaiting) {
            // Ordinary Blocked/Running transitions are far too frequent to timestamp merely
            // for a timeout diagnostic. A task becomes interesting only when this handshake
            // must wait for it, so make the request boundary the diagnostic baseline.
            task->setGCLastPollNanos(taskWaitStartNanos);
        } else {
            task->setGCHandshakeEpoch(gGCHandshakeEpoch);
        }
    });

    std::vector<YuGCHandshakeWaitSample> taskWaitSamples;
    gGCHandshakeTasksWaited = 0;
    gGCHandshakeSlowestTaskNanos = 0;
    yumiGCStartTaskWaitSamplesLocked(currentTask, taskWaitStartNanos, taskWaitSamples);
    for (const YuGCHandshakeWaitSample& sample : taskWaitSamples) {
        if (sample.startNanos) {
            ++gGCHandshakeTasksWaited;
        }
    }

    if (currentTask) {
        currentTask->setGCHandshakeEpoch(gGCHandshakeEpoch);
        currentTask->setGCLastPollNanos(taskWaitStartNanos);
    }

    pthread_cond_broadcast(&gGCHandshakeCond);

    uint64_t reportIntervalNanos = reportAfterUsec > UINT64_MAX / 1000
        ? UINT64_MAX
        : reportAfterUsec * 1000;
    uint64_t nextReportNanos = taskWaitStartNanos > UINT64_MAX - reportIntervalNanos
        ? UINT64_MAX
        : taskWaitStartNanos + reportIntervalNanos;
    while (!yumiGCHandshakeReadyLocked(currentTask, currentThread)) {
        uint64_t nowNanos = yumiGCHandshakeMonotonicNanos();
        yumiGCUpdateTaskWaitSamplesLocked(nowNanos, taskWaitSamples);
        if (nowNanos >= nextReportNanos) {
            yumiGCReportUnresponsiveTasksLocked(currentTask, nowNanos);
            nextReportNanos = nowNanos > UINT64_MAX - reportIntervalNanos
                ? UINT64_MAX
                : nowNanos + reportIntervalNanos;
            continue;
        }
        uint64_t remainingNanos = nextReportNanos - nowNanos;
        uint64_t remainingUsec = std::max<uint64_t>(1, remainingNanos / 1000);
        timespec deadline = yumiGCHandshakeDeadline(remainingUsec);
        pthread_cond_timedwait(&gGCHandshakeCond, &gGCHandshakeLock, &deadline);
    }

    yumiGCUpdateTaskWaitSamplesLocked(
        yumiGCHandshakeMonotonicNanos(),
        taskWaitSamples);

    pthread_mutex_unlock(&gGCHandshakeLock);
}

// *************************************************************************************************

void
yumiGCRegisterTask(YuTask* task) {
    if (!task) {
        return;
    }

    YuGCTaskShard& shard = yumiGCTaskShard(task);
    shard.lock.lock();
    task->setGCRegistryIndex(shard.tasks.size());
    shard.tasks.push_back(task);
    shard.lock.unlock();

    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if (!yumiGCHandshakeRequestedPublic()) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    if (yumiGCHandshakeActiveLocked() && !yumiGCHandshakeTaskRecordLocked(task)) {
        // A task born after the request is already Parked with a complete initial context.
        // Enroll that snapshot in the epoch without enlarging the exact Running ack set.
        yumiGCAddHandshakeTaskRecordLocked(task, YuGCHandshakeTaskSnapshotAcknowledged);
        task->setGCHandshakeEpoch(gGCHandshakeEpoch);
    }
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
yumiGCUnregisterTask(YuTask* task) {
    if (!task) {
        return;
    }

    YuGCTaskShard& shard = yumiGCTaskShard(task);
    shard.lock.lock();
    size_t index = task->getGCRegistryIndex();
    if (index < shard.tasks.size() && shard.tasks[index] == task) {
        YuTask* last = shard.tasks.back();
        shard.tasks[index] = last;
        last->setGCRegistryIndex(index);
        shard.tasks.pop_back();
    }
    shard.lock.unlock();

    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if (!yumiGCHandshakeRequestedPublic()) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    while (yumiGCHandshakeActiveLocked() && yumiGCHandshakeTaskRecordLocked(task)) {
        pthread_cond_wait(&gGCHandshakeCond, &gGCHandshakeLock);
    }
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
yumiGCRegisterThread(YuThread* thread) {
    if (!thread) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    gGCThreads.push_back(thread);
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
yumiGCUnregisterThread(YuThread* thread) {
    if (!thread) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    gGCThreads.erase(std::remove(gGCThreads.begin(), gGCThreads.end(), thread), gGCThreads.end());
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
yumiGCBeginThreadLocalCallback(YuThread* thread) {
    if (!thread) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    bool alreadyRegistered = gGCThreadLocalCallbackDepth != 0 &&
        std::find(
            gGCThreadLocalCallbacks.begin(),
            gGCThreadLocalCallbacks.end(),
            thread) != gGCThreadLocalCallbacks.end();
    while (!alreadyRegistered &&
            yumiGCHandshakeActiveLocked() &&
            thread != gGCHandshakeOwnerThread) {
        pthread_cond_wait(&gGCHandshakeCond, &gGCHandshakeLock);
    }
    gGCThreadLocalCallbacks.push_back(thread);
    ++gGCThreadLocalCallbackDepth;
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
yumiGCEndThreadLocalCallback(YuThread* thread) {
    if (!thread) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    auto found = std::find(
        gGCThreadLocalCallbacks.rbegin(),
        gGCThreadLocalCallbacks.rend(),
        thread);
    if (found == gGCThreadLocalCallbacks.rend() || gGCThreadLocalCallbackDepth == 0) {
        std::fprintf(stderr, "Yumi GC thread-local callback accounting underflow\n");
        std::abort();
    }
    gGCThreadLocalCallbacks.erase(std::next(found).base());
    --gGCThreadLocalCallbackDepth;
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

extern "C" bool
yumiGCThreadHasLocalCallbackRegistration() {
    return gGCThreadLocalCallbackDepth != 0;
}

void
yumiGCSetTaskState(YuTask* task, uint32_t state) {
    yumiGCSetTaskStateAndContext(task, state, task ? task->getGCContextReady() : false);
}

void
yumiGCSetTaskStateAndContext(YuTask* task, uint32_t state, bool contextReady) {
    if (!task) {
        return;
    }

    if (!yumiGCHandshakeRequestedPublic()) {
        yumiGCStoreTaskState(task, state, contextReady);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (!yumiGCHandshakeRequestedPublic()) {
            return;
        }
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    yumiGCStoreTaskState(task, state, contextReady);
    YuGCHandshakeTaskRecord* record = yumiGCHandshakeTaskRecordLocked(task);
    if (record &&
            yumiGCHandshakeActiveLocked() &&
            yumiGCIsStableTaskState(state) &&
            (state == YuTaskGCStateDead || contextReady)) {
        task->setGCHandshakeEpoch(gGCHandshakeEpoch);
        if (state == YuTaskGCStateDead) {
            record->disposition = YuGCHandshakeTaskDead;
        } else if (record->disposition == YuGCHandshakeTaskAwaiting) {
            record->disposition = YuGCHandshakeTaskSnapshotAcknowledged;
        }
        pthread_cond_broadcast(&gGCHandshakeCond);
    } else if (yumiGCHandshakeActiveLocked()) {
        pthread_cond_broadcast(&gGCHandshakeCond);
    }
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
yumiGCPrepareTaskResume(YuTask* task) {
    if (!task) {
        return;
    }

    if (!yumiGCHandshakeRequestedPublic()) {
        uint32_t previousState = task->getGCState();
        bool previousReady = task->getGCContextReady();
        yumiGCStoreTaskState(task, YuTaskGCStateRunning, false);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (!yumiGCHandshakeRequestedPublic()) {
            return;
        }
        uint32_t reason = 0;
        uint64_t epoch = 0;
        bool selfAcknowledge = false;
        pthread_mutex_lock(&gGCHandshakeLock);
        if (yumiGCHandshakeActiveLocked() && task != gGCHandshakeOwnerTask) {
            YuGCHandshakeTaskRecord* record = yumiGCHandshakeTaskRecordLocked(task);
            if (record && record->disposition == YuGCHandshakeTaskAwaiting) {
                if (yumiGCIsEnumerableTaskState(previousState) && previousReady) {
                    yumiGCStoreTaskState(task, previousState, true);
                    record->disposition = YuGCHandshakeTaskSnapshotAcknowledged;
                    task->setGCHandshakeEpoch(gGCHandshakeEpoch);
                    pthread_cond_broadcast(&gGCHandshakeCond);
                } else {
                    reason = gGCHandshakeReason;
                    epoch = gGCHandshakeEpoch;
                    selfAcknowledge = true;
                }
            } else if (record && yumiGCIsEnumerableTaskState(previousState) && previousReady) {
                yumiGCStoreTaskState(task, previousState, true);
                pthread_cond_broadcast(&gGCHandshakeCond);
            }
        }
        pthread_mutex_unlock(&gGCHandshakeLock);
        if (selfAcknowledge) {
            yumiGCAcknowledgeRunningTask(task, reason, epoch);
        }
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    while (yumiGCHandshakeActiveLocked()) {
        if (task == gGCHandshakeOwnerTask) {
            task->setGCHandshakeEpoch(gGCHandshakeEpoch);
            pthread_cond_broadcast(&gGCHandshakeCond);
            pthread_mutex_unlock(&gGCHandshakeLock);
            return;
        }
        YuGCHandshakeTaskRecord* record = yumiGCHandshakeTaskRecordLocked(task);
        if (yumiGCActiveTask() == task &&
                record &&
                record->disposition == YuGCHandshakeTaskAwaiting) {
            uint32_t reason = gGCHandshakeReason;
            uint64_t epoch = gGCHandshakeEpoch;
            yumiGCStoreTaskState(task, YuTaskGCStateRunning, false);
            pthread_mutex_unlock(&gGCHandshakeLock);
            yumiGCAcknowledgeRunningTask(task, reason, epoch);
            pthread_mutex_lock(&gGCHandshakeLock);
            continue;
        }
        pthread_cond_wait(&gGCHandshakeCond, &gGCHandshakeLock);
    }

    yumiGCStoreTaskState(task, YuTaskGCStateRunning, false);
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
yumiGCRevalidateThreadAllocation(uint64_t* rangeControl) {
    uint64_t control = yumiGCPublicControlSnapshot();
    if (control != *rangeControl) {
        yumiGCRuntimeFlushThreadAllocationBatches();
        *rangeControl = control;
    }
}

// *************************************************************************************************

static void
yumiGCSafepointHandshake() {
    uint64_t control = yumiGCPublicControlSnapshot();
    if (!yumiGCControlWordHandshakeRequested(control)) {
        return;
    }

    YuTask* task = yumiGCActiveTask();
    if (yumiGCTaskAcknowledgedControl(task, control)) {
        return;
    }
    if (task) {
        task->setGCLastPollNanos(yumiGCHandshakeMonotonicNanos());
    }
    if (yumiGCIsRuntimeCritical()) {
        return;
    }

    if (!task) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    if (!yumiGCHandshakeActiveLocked()) {
        pthread_mutex_unlock(&gGCHandshakeLock);
        return;
    }
    if (task == gGCHandshakeOwnerTask) {
        task->setGCHandshakeEpoch(gGCHandshakeEpoch);
        pthread_cond_broadcast(&gGCHandshakeCond);
        pthread_mutex_unlock(&gGCHandshakeLock);
        return;
    }
    uint32_t reason = gGCHandshakeReason;
    uint64_t epoch = gGCHandshakeEpoch;
    pthread_mutex_unlock(&gGCHandshakeLock);
    yumiGCAcknowledgeRunningTask(task, reason, epoch);
}

extern "C" __attribute__((noinline)) void
yumiGCSafepoint() {
    if (__atomic_load_n(&yumiInterruptRequestedFlags, __ATOMIC_RELAXED) &
            YumiInterruptSchedulerRequested) {
        uintptr_t* frame = static_cast<uintptr_t*>(__builtin_frame_address(0));
        yumiServiceTaskInterrupts(false, frame[0], frame[1] - 4);
    }
    yumiGCSafepointHandshake();
}

extern "C" __attribute__((noinline)) void
yumiTaskSafepoint() {
    yumiGCSafepointHandshake();
    if (__atomic_load_n(&yumiInterruptRequestedFlags, __ATOMIC_RELAXED) &
            YumiInterruptSchedulerRequested) {
        uintptr_t* frame = static_cast<uintptr_t*>(__builtin_frame_address(0));
        yumiServiceTaskInterrupts(true, frame[0], frame[1] - 4);
    }
}

extern "C" void
yumiGCHandshakeBegin(uint32_t reason) {
    yumiGCHandshakeBeginInternal(reason, yumiGCDefaultHandshakeReportAfterUsec());
}

extern "C" void
yumiGCHandshakeBeginWithReportAfter(uint32_t reason, uint64_t reportAfterUsec) {
    yumiGCHandshakeBeginInternal(reason, std::max<uint64_t>(1, reportAfterUsec));
}

extern "C" void
yumiGCHandshakeEnd() {
    pthread_mutex_lock(&gGCHandshakeLock);

    yumiGCPublishHandshakeRequest(gGCHandshakeReason, gGCHandshakeEpoch, false);
    gGCHandshakeReason = 0;
    gGCHandshakeOwnerTask = NULL;
    gGCHandshakeOwnerThread = NULL;
    gGCHandshakeTasks.clear();

    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

extern "C" uint32_t
yumiGCHandshakeReason() {
    pthread_mutex_lock(&gGCHandshakeLock);
    uint32_t reason = gGCHandshakeReason;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return reason;
}

extern "C" uint64_t
yumiGCHandshakeEpoch() {
    pthread_mutex_lock(&gGCHandshakeLock);
    uint64_t epoch = gGCHandshakeEpoch;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return epoch;
}

extern "C" size_t
yumiGCHandshakeTaskCount() {
    return yumiGCRegisteredTaskCount();
}

extern "C" size_t
yumiGCHandshakeStableTaskCount() {
    size_t count = 0;
    yumiGCForEachRegisteredTask([&](YuTask* task) {
        if (yumiGCIsStableTaskState(task->getGCState())) {
            ++count;
        }
    });
    return count;
}

extern "C" size_t
yumiGCHandshakeThreadCount() {
    pthread_mutex_lock(&gGCHandshakeLock);
    size_t count = gGCThreads.size();
    pthread_mutex_unlock(&gGCHandshakeLock);
    return count;
}

extern "C" size_t
yumiGCHandshakeTasksWaited() {
    pthread_mutex_lock(&gGCHandshakeLock);
    size_t count = gGCHandshakeTasksWaited;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return count;
}

extern "C" uint64_t
yumiGCHandshakeSlowestTaskNanos() {
    pthread_mutex_lock(&gGCHandshakeLock);
    uint64_t duration = gGCHandshakeSlowestTaskNanos;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return duration;
}

extern "C" size_t
yumiGCHandshakeReportCount() {
    pthread_mutex_lock(&gGCHandshakeLock);
    size_t count = gGCHandshakeReportCount;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return count;
}

extern "C" uint32_t
yumiGetTaskGCState(YuTask* task) {
    return task ? task->getGCState() : YuTaskGCStateDead;
}

extern "C" void
yumiGCEnumerateTasks(YuGCTaskVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    std::vector<YuGCTaskSnapshot> snapshots;
    pthread_mutex_lock(&gGCHandshakeLock);
    bool activeHandshake = yumiGCHandshakeActiveLocked();
    size_t taskCount = activeHandshake
        ? gGCHandshakeTasks.size()
        : yumiGCRegisteredTaskCount();
    snapshots.reserve(taskCount);
    auto appendSnapshot = [&](YuTask* task) {
        uint32_t state = task->getGCState();
        if (!task->getGCContextReady() || !yumiGCIsEnumerableTaskState(state)) {
            return;
        }

        snapshots.push_back({
            task,
            state,
            task->getStackBase(),
            task->getStackSize(),
            task->getContext()
        });
    };
    if (activeHandshake) {
        for (const YuGCHandshakeTaskRecord& record : gGCHandshakeTasks) {
            if (record.disposition == YuGCHandshakeTaskSnapshotAcknowledged) {
                appendSnapshot(record.task);
            }
        }
    } else {
        yumiGCForEachRegisteredTask(appendSnapshot);
    }
    pthread_mutex_unlock(&gGCHandshakeLock);

    for (const YuGCTaskSnapshot& snapshot : snapshots) {
        uint64_t startNanos = yumiGCHandshakeMonotonicNanos();
        visitor(
            snapshot.task,
            snapshot.state,
            snapshot.stackBase,
            snapshot.stackSize,
            snapshot.context,
            data);
        uint64_t endNanos = yumiGCHandshakeMonotonicNanos();
        yumiGCRuntimeRecordPerTaskSetupScan(
            static_cast<size_t>(startNanos),
            static_cast<size_t>(endNanos - startNanos));
    }

}

extern "C" YumiGCTemporaryRootStorage*
yumiGCActiveTaskTemporaryRootStorage(void) {
    YuTask* task = yumiGCActiveTask();
    if (!task || task->getGCState() != YuTaskGCStateRunning) {
        return nullptr;
    }
    return &task->getRuntimeContext()->temporaryRoots;
}

namespace {

struct YuGCTemporaryRootListScan {
    YuGCTemporaryRootListVisitor visitor;
    void* data;
    YuTask* activeTask;
};

static void
yumiGCVisitTaskTemporaryRootList(
    YuTask* task,
    uint32_t,
    void*,
    size_t,
    void*,
    void* data
) {
    YuGCTemporaryRootListScan* scan = static_cast<YuGCTemporaryRootListScan*>(data);
    if (!task || task == scan->activeTask) {
        return;
    }
    scan->visitor(task->getRuntimeContext()->temporaryRoots.head, scan->data);
}

}

extern "C" void
yumiGCVisitTaskTemporaryRootLists(YuGCTemporaryRootListVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }
    YuTask* activeTask = yumiGCActiveTask();
    if (activeTask) {
        visitor(activeTask->getRuntimeContext()->temporaryRoots.head, data);
    }
    YuGCTemporaryRootListScan scan { visitor, data, activeTask };
    yumiGCEnumerateTasks(yumiGCVisitTaskTemporaryRootList, &scan);
}

namespace {

struct YuGCParseRegionScan {
    YuParseRegionAllocationVisitor visitor;
    void* data;
    YuTask* activeTask;
};

static void
yumiGCVisitTaskParseRegion(
    YuTask* task,
    uint32_t,
    void*,
    size_t,
    void*,
    void* data
) {
    YuGCParseRegionScan* scan = static_cast<YuGCParseRegionScan*>(data);
    if (!task || task == scan->activeTask) {
        return;
    }
    YuTaskRuntimeContext* context = task->getRuntimeContext();
    if (!context->parseRegion || !context->parseRegionScanner) {
        return;
    }
    context->parseRegionScanner(context->parseRegion, scan->visitor, scan->data);
}

}

extern "C" void
yumiGCVisitActiveTaskParseRegionAllocations(
    YuParseRegionAllocationVisitor visitor,
    void* data
) {
    if (!visitor) {
        return;
    }
    YuTask* task = yumiGCActiveTask();
    if (!task) {
        return;
    }
    YuTaskRuntimeContext* context = task->getRuntimeContext();
    if (context->parseRegion && context->parseRegionScanner) {
        context->parseRegionScanner(context->parseRegion, visitor, data);
    }
}

extern "C" void
yumiGCVisitTaskParseRegionAllocations(
    YuParseRegionAllocationVisitor visitor,
    void* data
) {
    if (!visitor) {
        return;
    }
    YuTask* activeTask = yumiGCActiveTask();
    yumiGCVisitActiveTaskParseRegionAllocations(visitor, data);
    YuGCParseRegionScan scan { visitor, data, activeTask };
    yumiGCEnumerateTasks(yumiGCVisitTaskParseRegion, &scan);
}

static void
currentThreadStack(void** outBase, size_t* outSize) {
#if defined(__APPLE__)
    size_t stackSize = pthread_get_stacksize_np(pthread_self());
    *outBase = static_cast<char*>(pthread_get_stackaddr_np(pthread_self())) - stackSize;
    *outSize = stackSize;
#else
    static thread_local void* stackBase = NULL;
    static thread_local size_t stackSize = 0;
    if (!stackBase) {
        pthread_attr_t attributes;
        if (pthread_getattr_np(pthread_self(), &attributes) == 0) {
            pthread_attr_getstack(&attributes, &stackBase, &stackSize);
            pthread_attr_destroy(&attributes);
        }
    }
    *outBase = stackBase;
    *outSize = stackSize;
#endif
}

extern "C" void
yumiGCScanCurrentStack(YuGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    YuContext context;
    yumiContextGet(&context);

    YuTask* task = yumiGCActiveTask();
    if (task) {
        yumiGCScanEventContextRoots(
            &context,
            task->getStackBase(),
            task->getStackSize(),
            false,
            visitor,
            data);
        return;
    }

    void* stackBase = NULL;
    size_t stackSize = 0;
    currentThreadStack(&stackBase, &stackSize);
    yumiGCScanEventContextRoots(&context, stackBase, stackSize, false, visitor, data);
}

extern "C" void
yumiGCScanTaskStacks(YuGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    YuGCScanData scan;
    scan.visitor = visitor;
    scan.data = data;
    yumiGCEnumerateTasks(yumiGCScanTaskRootsVisitor, &scan);
}

extern "C" void
yumiGCScanCurrentStackConservative(YuGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    YuContext context;
    yumiContextGet(&context);

    YuTask* task = yumiGCActiveTask();
    if (task) {
        yumiGCScanEventConservativeContextRoots(
            &context,
            task->getStackBase(),
            task->getStackSize(),
            visitor,
            data);
        return;
    }

    void* stackBase = NULL;
    size_t stackSize = 0;
    currentThreadStack(&stackBase, &stackSize);
    yumiGCScanEventConservativeContextRoots(&context, stackBase, stackSize, visitor, data);
}

extern "C" void
yumiGCScanTaskStacksConservative(YuGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    YuGCScanData scan;
    scan.visitor = visitor;
    scan.data = data;
    yumiGCEnumerateTasks(yumiGCScanTaskConservativeRootsVisitor, &scan);
}

extern "C" void
yumiGCScanStorageRoots(YuGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    size_t taskCount = 0;
    size_t threadCount = 0;
    taskCount = yumiGCRegisteredTaskCount();
    pthread_mutex_lock(&gGCHandshakeLock);
    threadCount = gGCThreads.size();
    pthread_mutex_unlock(&gGCHandshakeLock);

    std::vector<YuGCStorageRoot> roots;
    roots.reserve(yumiGCStorageRootRecordCapacity(taskCount, threadCount));

    YuGCStorageRootCollectData collect;
    collect.roots = &roots;
    pthread_mutex_lock(&gGCHandshakeLock);
    yumiGCVisitStorageRootRecords(yumiGCCollectStorageRootRecord, &collect);
    pthread_mutex_unlock(&gGCHandshakeLock);

    for (const YuGCStorageRoot& root : roots) {
        visitor(root.base, root.descriptor, data);
    }
}

extern "C" void
yumiGCEnterRuntimeBlock() {
    if (yumiGCIsRuntimeCritical()) {
        return;
    }

    yumiGCEnterBlockingContext(YuTaskGCStateBlocked);
}

extern "C" void
yumiGCLeaveRuntimeBlock() {
    if (yumiGCIsRuntimeCritical()) {
        return;
    }

    yumiGCLeaveBlockingContext();
}

extern "C" void
yumiGCEnterBlockingContextWithRegisters(
    uint32_t state,
    void* managedFramePointer,
    void* managedReturnPC,
    const uintptr_t* managedRegisters,
    void* managedStackPointer
) {
    if (yumiGCIsRuntimeCritical()) {
        return;
    }
    yumiGCEnterBlockingContext(
        state,
        managedFramePointer,
        managedReturnPC,
        managedStackPointer,
        managedRegisters);
}

extern "C" __attribute__((naked)) void
yumiGCEnterFFI() {
    asm volatile (
        "sub sp, sp, #96\n"
        "stp x19, x20, [sp, #0]\n"
        "stp x21, x22, [sp, #16]\n"
        "stp x23, x24, [sp, #32]\n"
        "stp x25, x26, [sp, #48]\n"
        "stp x27, x28, [sp, #64]\n"
        "stp x29, x30, [sp, #80]\n"
        "mov x1, x29\n"
        "mov x2, x30\n"
        "mov x3, sp\n"
        "add x4, sp, #96\n"
        "mov w0, #2\n"
        "bl " YU_ASM_SYMBOL(yumiGCEnterBlockingContextWithRegisters) "\n"
        "ldr x30, [sp, #88]\n"
        "add sp, sp, #96\n"
        "ret\n"
    );
}

extern "C" __attribute__((naked)) void
yumiGCEnterZone() {
    asm volatile (
        "sub sp, sp, #96\n"
        "stp x19, x20, [sp, #0]\n"
        "stp x21, x22, [sp, #16]\n"
        "stp x23, x24, [sp, #32]\n"
        "stp x25, x26, [sp, #48]\n"
        "stp x27, x28, [sp, #64]\n"
        "stp x29, x30, [sp, #80]\n"
        "mov x1, x29\n"
        "mov x2, x30\n"
        "mov x3, sp\n"
        "add x4, sp, #96\n"
        "mov w0, #1\n"
        "bl " YU_ASM_SYMBOL(yumiGCEnterBlockingContextWithRegisters) "\n"
        "ldr x30, [sp, #88]\n"
        "add sp, sp, #96\n"
        "ret\n"
    );
}

extern "C" void
yumiGCLeaveZone() {
    if (yumiGCIsRuntimeCritical()) {
        return;
    }
    yumiGCLeaveBlockingContext();
}

extern "C" void
yumiGCLeaveFFI() {
    if (yumiGCIsRuntimeCritical()) {
        return;
    }

    yumiGCLeaveBlockingContext();
}

extern "C" void
yumiGCEnterManagedCallback() {
    YuTask* task = yumiGCActiveTask();
    if (task) {
        YuGCManagedCallbackKind kind = YuGCManagedCallbackNone;
        uint32_t state = task->getGCState();
        if (state == YuTaskGCStateBlocked || state == YuTaskGCStateInZone) {
            // The outer FFI/zone context may predate one or more phase/generation transitions.
            // Revoke its inactive allocation ranges before making the callback Running when the
            // control word moved since the task stopped running, as an outermost leave does. This
            // also keeps an InZone snapshot from hiding managed frames entered by a native callback.
            yumiGCResumeFromBlockingContext(task, task->getGCBlockingEntryControl());
            kind = state == YuTaskGCStateBlocked
                ? YuGCManagedCallbackBlocked
                : YuGCManagedCallbackInZone;
        }
        task->pushGCManagedCallbackKind(kind);
        return;
    }

    uint32_t previousDepth = gGCManagedCallbackDepth++;
    if (previousDepth == 0) {
        YuThread* thread = yumiGetActiveThread();
        if (thread) {
            yumiGCBeginThreadLocalCallback(thread);
            gGCManagedCallbackKind = YuGCManagedCallbackTaskless;
        }
    }
}

extern "C" void
yumiGCLeaveManagedCallback() {
    YuTask* task = yumiGCActiveTask();
    YuGCManagedCallbackKind kind = YuGCManagedCallbackNone;
    if (task) {
        kind = static_cast<YuGCManagedCallbackKind>(task->popGCManagedCallbackKind());
    } else {
        bool outermost = gGCManagedCallbackDepth != 0 && --gGCManagedCallbackDepth == 0;
        if (!outermost) {
            return;
        }
        kind = gGCManagedCallbackKind;
        gGCManagedCallbackKind = YuGCManagedCallbackNone;
    }
    if (task && (kind == YuGCManagedCallbackBlocked || kind == YuGCManagedCallbackInZone)) {
        task->setGCBlockingEntryControl(yumiGCPublicControlSnapshot());
    }
    if (kind == YuGCManagedCallbackBlocked) {
        yumiGCPublishBlockingContext(
            YuTaskGCStateBlocked,
            task ? task->getGCBlockingContextSnapshot() : NULL);
    } else if (kind == YuGCManagedCallbackInZone) {
        yumiGCPublishBlockingContext(
            YuTaskGCStateInZone,
            task ? task->getGCBlockingContextSnapshot() : NULL);
    } else if (kind == YuGCManagedCallbackTaskless) {
        YuThread* thread = yumiGetActiveThread();
        if (thread) {
            yumiGCEndThreadLocalCallback(thread);
        }
    }
}

extern "C" YuTaskRuntimeContext*
yumiGCEnterForeignCallbackFrom(uint64_t callerRegister) {
    YuTask* task = yumiGCActiveTask();
    if (task) {
        task->pushGCManagedCallbackRegister(callerRegister);
    } else {
        gGCManagedCallbackRegisters.push_back(callerRegister);
    }
    yumiGCEnterManagedCallback();
    return yumiGetActiveTaskRuntimeContext();
}

extern "C" uint64_t
yumiGCLeaveForeignCallbackTo() {
    YuTask* task = yumiGCActiveTask();
    yumiGCLeaveManagedCallback();
    if (task) {
        return task->popGCManagedCallbackRegister();
    }
    if (gGCManagedCallbackRegisters.empty()) {
        return 0;
    }
    uint64_t value = gGCManagedCallbackRegisters.back();
    gGCManagedCallbackRegisters.pop_back();
    return value;
}

#if defined(__APPLE__)
#define YU_CALLBACK_SYMBOL(name) "_" #name
#define YU_CALLBACK_FUNCTION_TYPE(name) ""
#else
#define YU_CALLBACK_SYMBOL(name) #name
#define YU_CALLBACK_FUNCTION_TYPE(name) ".type " #name ", %function\n"
#endif

asm(
    ".text\n"
    ".p2align 2\n"
    ".globl " YU_CALLBACK_SYMBOL(yumiGCEnterForeignCallback) "\n"
    YU_CALLBACK_FUNCTION_TYPE(yumiGCEnterForeignCallback)
    YU_CALLBACK_SYMBOL(yumiGCEnterForeignCallback) ":\n"
    "stp x29, x30, [sp, #-16]!\n"
    "mov x29, sp\n"
    "mov x0, x27\n"
    "bl " YU_CALLBACK_SYMBOL(yumiGCEnterForeignCallbackFrom) "\n"
    "mov x27, x0\n"
    "ldp x29, x30, [sp], #16\n"
    "ret\n"
    ".p2align 2\n"
    ".globl " YU_CALLBACK_SYMBOL(yumiGCLeaveForeignCallback) "\n"
    YU_CALLBACK_FUNCTION_TYPE(yumiGCLeaveForeignCallback)
    YU_CALLBACK_SYMBOL(yumiGCLeaveForeignCallback) ":\n"
    "stp x29, x30, [sp, #-16]!\n"
    "mov x29, sp\n"
    "bl " YU_CALLBACK_SYMBOL(yumiGCLeaveForeignCallbackTo) "\n"
    "mov x27, x0\n"
    "ldp x29, x30, [sp], #16\n"
    "ret\n");

extern "C" void
yumiGCEnterRuntimeCritical() {
    ++gGCRuntimeCriticalDepth;
}

extern "C" void
yumiGCLeaveRuntimeCritical() {
    if (gGCRuntimeCriticalDepth == 0) {
        std::fprintf(stderr, "Yumi GC runtime critical section underflow\n");
        std::abort();
    }

    --gGCRuntimeCriticalDepth;
}
