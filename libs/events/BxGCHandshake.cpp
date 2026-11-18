#include <BxEvents/BxGCHandshake.h>
#include <BxEvents/BxTaskMonitor.h>
#include <BxCore/BxCore.h>

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxLock.h>
#include <BxEvents/BxTask.h>
#include <BxEvents/BxThread.h>
#include <BxGC/BxGCRuntime.h>

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

#define BX_STRINGIFY_VALUE(value) #value
#define BX_STRINGIFY(value) BX_STRINGIFY_VALUE(value)
#define BX_ASM_SYMBOL(name) BX_STRINGIFY(__USER_LABEL_PREFIX__) #name

// *************************************************************************************************

extern "C" void __attribute__((weak))
brixGCRuntimeFlushThreadAllocationBatches(void) {
}

extern "C" void __attribute__((weak))
brixGCRuntimeFlushWriteBarrierBuffer(void) {
}

extern "C" void __attribute__((weak))
brixGCRuntimeRecordPerTaskSetupScan(size_t, size_t) {
}

extern "C" void __attribute__((weak))
brixGCPublishHandshakeRequest(size_t reason, size_t epoch, bool requested) {
    uint64_t current = __atomic_load_n(&brixGCPublicControlWord, __ATOMIC_RELAXED);
    while (true) {
        uint64_t next = brixGCPackControlWordFull(
            brixGCControlWordPhase(current),
            brixGCControlWordSweepGeneration(current),
            requested,
            requested ? reason : 0,
            requested ? epoch : 0) |
            (current & BrixGCControlTerminationRequestedBit);
        if (__atomic_compare_exchange_n(
                &brixGCPublicControlWord,
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
brixGCRuntimeHandshakeTaskWork(uint32_t) {
}

static pthread_mutex_t gGCHandshakeLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gGCHandshakeCond = PTHREAD_COND_INITIALIZER;
static uint32_t gGCHandshakeReason = 0;
static uint64_t gGCHandshakeEpoch = 0;
static BxTask* gGCHandshakeOwnerTask = NULL;
static const void* gGCHandshakeOwnerThread = NULL;
static size_t gGCHandshakeTasksWaited = 0;
static uint64_t gGCHandshakeSlowestTaskNanos = 0;
static size_t gGCHandshakeReportCount = 0;
static const size_t kBxGCTaskShardCount = 64;
struct alignas(64) BxGCTaskShard {
    BxLock lock;
    std::vector<BxTask*> tasks;
};
static BxGCTaskShard gGCTaskShards[kBxGCTaskShardCount];
enum BxGCHandshakeTaskDisposition {
    BxGCHandshakeTaskAwaiting = 0,
    BxGCHandshakeTaskSelfAcknowledged = 1,
    BxGCHandshakeTaskSnapshotAcknowledged = 2,
    BxGCHandshakeTaskDead = 3,
    BxGCHandshakeTaskOwner = 4
};
struct BxGCHandshakeTaskRecord {
    BxTask* task;
    BxGCHandshakeTaskDisposition disposition;
};
static std::vector<BxGCHandshakeTaskRecord> gGCHandshakeTasks;
static std::vector<BxThread*> gGCThreads;
static std::vector<const void*> gGCThreadLocalCallbacks;
static thread_local uint32_t gGCThreadLocalCallbackDepth = 0;
static thread_local uint32_t gGCRuntimeCriticalDepth = 0;
static thread_local uint32_t gGCBlockingDepth = 0;
static thread_local uint64_t gGCBlockingEntryControl = 0;
enum BxGCManagedCallbackKind {
    BxGCManagedCallbackNone = 0,
    BxGCManagedCallbackBlocked = 1,
    BxGCManagedCallbackTaskless = 2,
    BxGCManagedCallbackInZone = 3
};
static thread_local uint32_t gGCManagedCallbackDepth = 0;
static thread_local BxGCManagedCallbackKind gGCManagedCallbackKind =
    BxGCManagedCallbackNone;
static thread_local std::vector<uint64_t> gGCManagedCallbackRegisters;
static thread_local uint64_t gGCTasklessCallbackAllocationControl = 0;

struct BxGCHandshakeWaitSample {
    size_t recordIndex;
    uint64_t startNanos;
};

struct BxGCScanData {
    BxGCStackRootVisitor visitor;
    void* data;
};

struct BxGCStorageRoot {
    void* base;
    void* descriptor;
};

struct BxGCStorageRootCollectData {
    std::vector<BxGCStorageRoot>* roots;
};

struct BxGCTaskSnapshot {
    BxTask* task;
    uint32_t state;
    void* stackBase;
    size_t stackSize;
    void* context;
};

static BxTask* brixGCActiveTask();
static uint64_t brixGCPublicControlSnapshot();
static bool brixGCHandshakeRequestedPublic();
static void brixGCAcknowledgeRunningTask(BxTask* task, uint32_t reason, uint64_t epoch);

static bool
brixGCHandshakeActiveLocked() {
    return gGCHandshakeReason != 0;
}

static BxGCTaskShard&
brixGCTaskShard(BxTask* task) {
    return gGCTaskShards[task->getId() % kBxGCTaskShardCount];
}

template <typename Visit>
static void
brixGCForEachRegisteredTask(Visit visit) {
    for (BxGCTaskShard& shard : gGCTaskShards) {
        shard.lock.lock();
        for (BxTask* task : shard.tasks) {
            visit(task);
        }
        shard.lock.unlock();
    }
}

static size_t
brixGCRegisteredTaskCount() {
    size_t count = 0;
    for (BxGCTaskShard& shard : gGCTaskShards) {
        shard.lock.lock();
        count += shard.tasks.size();
        shard.lock.unlock();
    }
    return count;
}

static BxGCHandshakeTaskRecord*
brixGCHandshakeTaskRecordLocked(BxTask* task) {
    size_t index = task->getGCHandshakeRecordIndex();
    if (index < gGCHandshakeTasks.size() && gGCHandshakeTasks[index].task == task) {
        return &gGCHandshakeTasks[index];
    }
    return nullptr;
}

static void
brixGCAddHandshakeTaskRecordLocked(BxTask* task, BxGCHandshakeTaskDisposition disposition) {
    task->setGCHandshakeRecordIndex(gGCHandshakeTasks.size());
    gGCHandshakeTasks.push_back({task, disposition});
}

static uint64_t
brixGCHandshakeMonotonicNanos() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<uint64_t>(now.tv_sec) * 1000000000ull +
        static_cast<uint64_t>(now.tv_nsec);
}

extern "C" bool
brixGCIsRuntimeCritical() {
    return gGCRuntimeCriticalDepth != 0;
}

static void
brixGCContextSnapshotFromBxContext(const BxContext* context, BrixGCContextSnapshot* snapshot) {
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
brixGCScanEventContextRoots(
    BxContext* context,
    void* stackBase,
    size_t stackSize,
    bool useCurrentSafepoint,
    BxGCStackRootVisitor visitor,
    void* data
) {
    if (!context || !visitor) {
        return;
    }

    BrixGCContextSnapshot snapshot;
    brixGCContextSnapshotFromBxContext(context, &snapshot);
    if (useCurrentSafepoint) {
        snapshot.pc = context->pc;
    }
    brixGCScanContextRoots(&snapshot, stackBase, stackSize, visitor, data);
}

static void
brixGCScanEventConservativeContextRoots(
    BxContext* context,
    void* stackBase,
    size_t stackSize,
    BxGCStackRootVisitor visitor,
    void* data
) {
    if (!context || !visitor) {
        return;
    }

    BrixGCContextSnapshot snapshot;
    brixGCContextSnapshotFromBxContext(context, &snapshot);
    brixGCScanConservativeContextRoots(&snapshot, stackBase, stackSize, visitor, data);
}

static void
brixGCScanTaskRootsVisitor(
    BxTask*,
    uint32_t state,
    void* stackBase,
    size_t stackSize,
    void* context,
    void* data
) {
    BxGCScanData* scan = static_cast<BxGCScanData*>(data);
    brixGCScanEventContextRoots(
        static_cast<BxContext*>(context),
        stackBase,
        stackSize,
        state == BxTaskGCStateInZone || state == BxTaskGCStateBlocked,
        scan->visitor,
        scan->data);
}

static void
brixGCScanTaskConservativeRootsVisitor(
    BxTask*,
    uint32_t,
    void* stackBase,
    size_t stackSize,
    void* context,
    void* data
) {
    BxGCScanData* scan = static_cast<BxGCScanData*>(data);
    brixGCScanEventConservativeContextRoots(
        static_cast<BxContext*>(context),
        stackBase,
        stackSize,
        scan->visitor,
        scan->data);
}

static void
brixGCCollectStorageRootRecord(const BrixGCStorageRootRecord* record, void* data) {
    BxGCStorageRootCollectData* collect = static_cast<BxGCStorageRootCollectData*>(data);
    if (!record || !collect || !collect->roots) {
        return;
    }

    if (record->kind == 1) {
        collect->roots->push_back({ record->base, record->descriptor });
    } else if (record->kind == 2) {
        brixGCForEachRegisteredTask([&](BxTask* task) {
            uint32_t state = task->getGCState();
            if (
                state != BxTaskGCStateDead &&
                task->getLocals() &&
                task->getLocalsSize()
            ) {
                collect->roots->push_back({ task->getLocals(), record->descriptor });
            }
        });
    } else if (record->kind == 3) {
        for (BxThread* thread : gGCThreads) {
            if (thread->getThreadLocals() && thread->getThreadLocalsSize()) {
                collect->roots->push_back({ thread->getThreadLocals(), record->descriptor });
            }
        }
    }
}

// *************************************************************************************************

static bool
brixGCIsStableTaskState(uint32_t state) {
    return state == BxTaskGCStateInZone ||
        state == BxTaskGCStateBlocked ||
        state == BxTaskGCStateParked ||
        state == BxTaskGCStateDead;
}

static bool
brixGCIsEnumerableTaskState(uint32_t state) {
    return
        state == BxTaskGCStateInZone ||
        state == BxTaskGCStateBlocked ||
        state == BxTaskGCStateParked;
}

static void
brixGCPublishBlockingContext(
    uint32_t state,
    const BxTaskGCBlockingContext* managedContext = NULL
) {
    BxTask* task = brixGCActiveTask();
    if (!task) {
        return;
    }

    if (brixGCHandshakeRequestedPublic()) {
        pthread_mutex_lock(&gGCHandshakeLock);
        bool handshakeRequested = brixGCHandshakeActiveLocked();
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
    brixGCRuntimeFlushWriteBarrierBuffer();
    BxContext* context = task->getContext();
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
        brixContextGet(context);
    }
    brixGCSetTaskStateAndContext(task, state, true);
}

static uint64_t
brixGCPublicControlSnapshot() {
    return __atomic_load_n(&brixGCPublicControlWord, __ATOMIC_ACQUIRE);
}

static bool
brixGCHandshakeRequestedPublic() {
    return brixGCControlWordHandshakeRequested(brixGCPublicControlSnapshot());
}

static bool
brixGCTaskAcknowledgedControl(BxTask* task, uint64_t control) {
    uint64_t epochBits = BrixGCControlHandshakeEpochMask >> BrixGCControlHandshakeEpochShift;
    return task &&
        (task->getGCHandshakeEpoch() & epochBits) == brixGCControlWordHandshakeEpoch(control);
}

extern "C" bool
brixGCActiveTaskAcknowledgedHandshake(uint64_t control) {
    return brixGCTaskAcknowledgedControl(brixGCActiveTask(), control);
}

static void
brixGCEnterBlockingContext(
    uint32_t state,
    void* managedFramePointer = NULL,
    void* managedReturnPC = NULL,
    void* managedStackPointer = NULL,
    const uintptr_t* managedRegisters = NULL
) {
    BxTask* task = brixGCActiveTask();
    uint32_t previousDepth = task
        ? task->enterGCBlockingContext()
        : gGCBlockingDepth++;
    const BxTaskGCBlockingContext* publishedContext = NULL;
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
            task->getGCState() == BxTaskGCStateRunning) {
            task->markGCBlockingContextManagedCallback();
            task->setGCBlockingEntryControl(brixGCPublicControlSnapshot());
            brixGCPublishBlockingContext(state, publishedContext);
        }
        return;
    }
    uint64_t control = brixGCPublicControlSnapshot();
    if (task) {
        task->setGCBlockingEntryControl(control);
    } else {
        gGCBlockingEntryControl = control;
    }
    brixGCPublishBlockingContext(state, publishedContext);
}

static void
brixGCResumeFromBlockingContext(BxTask* task, uint64_t entryControl) {
    bool allocationRangesRevoked = false;
    if (brixGCPublicControlSnapshot() != entryControl) {
        brixGCRuntimeFlushThreadAllocationBatches();
        allocationRangesRevoked = true;
    }

    // This may wait for a handshake that began after the comparison above. Recheck before
    // returning to Brix so an Idle-issued range can never be consumed in a later phase.
    brixGCPrepareTaskResume(task);
    if (!allocationRangesRevoked && brixGCPublicControlSnapshot() != entryControl) {
        brixGCRuntimeFlushThreadAllocationBatches();
    }
}

static void
brixGCLeaveBlockingContext() {
    BxTask* task = brixGCActiveTask();
    bool managedCallbackContext = false;
    uint64_t entryControl = 0;
    bool outermost = task
        ? task->leaveGCBlockingContext(managedCallbackContext, entryControl)
        : (gGCBlockingDepth != 0 && --gGCBlockingDepth == 0);
    if (!outermost) {
        if (task && managedCallbackContext) {
            brixGCResumeFromBlockingContext(task, entryControl);
        }
        return;
    }

    if (!task) {
        entryControl = gGCBlockingEntryControl;
        gGCBlockingEntryControl = 0;
    }
    brixGCResumeFromBlockingContext(task, entryControl);
}

static void
brixGCStoreTaskState(BxTask* task, uint32_t state, bool contextReady) {
    if (!contextReady) {
        task->setGCContextReady(false);
        task->setGCState(state);
    } else {
        task->setGCContextReady(true);
        task->setGCState(state);
    }
}

static BxTask*
brixGCActiveTask() {
    BxThread* thread = brixGetActiveThread();
    return thread ? thread->getActiveTask() : NULL;
}

extern "C" bool
brixGCActiveTaskCanUseFastAllocation() {
    BxTask* task = brixGCActiveTask();
    return task && task->getActive() && task->getGCState() == BxTaskGCStateRunning;
}

static bool
brixGCAllTasksAcknowledgedLocked(BxTask* currentTask) {
    for (const BxGCHandshakeTaskRecord& record : gGCHandshakeTasks) {
        BxTask* task = record.task;
        if (task == currentTask ||
                record.disposition == BxGCHandshakeTaskOwner ||
                record.disposition == BxGCHandshakeTaskDead) {
            continue;
        }
        uint32_t state = task->getGCState();
        if (state == BxTaskGCStateDead) {
            continue;
        }
        if (record.disposition == BxGCHandshakeTaskAwaiting ||
                task->getGCHandshakeEpoch() != gGCHandshakeEpoch) {
            return false;
        }
        if (record.disposition == BxGCHandshakeTaskSnapshotAcknowledged &&
                (!task->getGCContextReady() || !brixGCIsEnumerableTaskState(state))) {
            return false;
        }
    }
    return true;
}

static const void*
brixGCCurrentThreadIdentity() {
    return &gGCThreadLocalCallbackDepth;
}

static bool
brixGCThreadLocalCallbacksStableLocked(const void* currentThread) {
    for (const void* thread : gGCThreadLocalCallbacks) {
        if (thread != currentThread) {
            return false;
        }
    }
    return true;
}

static bool
brixGCHandshakeReadyLocked(BxTask* currentTask, const void* currentThread) {
    return brixGCAllTasksAcknowledgedLocked(currentTask) &&
        brixGCThreadLocalCallbacksStableLocked(currentThread);
}

static bool
brixGCTaskWaitCompleteLocked(size_t recordIndex) {
    if (recordIndex >= gGCHandshakeTasks.size()) {
        return true;
    }
    const BxGCHandshakeTaskRecord& record = gGCHandshakeTasks[recordIndex];
    return record.disposition != BxGCHandshakeTaskAwaiting ||
        record.task->getGCState() == BxTaskGCStateDead;
}

static void
brixGCStartTaskWaitSamplesLocked(
    BxTask* currentTask,
    uint64_t startNanos,
    std::vector<BxGCHandshakeWaitSample>& samples
) {
    samples.clear();
    for (size_t index = 0; index < gGCHandshakeTasks.size(); ++index) {
        const BxGCHandshakeTaskRecord& record = gGCHandshakeTasks[index];
        if (record.task == currentTask || record.disposition != BxGCHandshakeTaskAwaiting) {
            continue;
        }
        samples.push_back({index, startNanos});
    }
}

static void
brixGCUpdateTaskWaitSamplesLocked(
    uint64_t nowNanos,
    std::vector<BxGCHandshakeWaitSample>& samples
) {
    for (BxGCHandshakeWaitSample& sample : samples) {
        if (sample.startNanos && brixGCTaskWaitCompleteLocked(sample.recordIndex)) {
            gGCHandshakeSlowestTaskNanos = std::max(
                gGCHandshakeSlowestTaskNanos,
                nowNanos - sample.startNanos);
            sample.startNanos = 0;
        }
    }
}

static const char*
brixGCTaskGCStateName(uint32_t state) {
    switch (state) {
    case BxTaskGCStateRunning: return "Running";
    case BxTaskGCStateInZone: return "InZone";
    case BxTaskGCStateBlocked: return "Blocked";
    case BxTaskGCStateParked: return "Parked";
    case BxTaskGCStateDead: return "Dead";
    default: return "Invalid";
    }
}

static void
brixGCReportUnresponsiveTasksLocked(BxTask* currentTask, uint64_t nowNanos) {
    ++gGCHandshakeReportCount;
    bool reported = false;
    for (const BxGCHandshakeTaskRecord& record : gGCHandshakeTasks) {
        BxTask* task = record.task;
        if (task == currentTask || record.disposition != BxGCHandshakeTaskAwaiting) {
            continue;
        }

        uint32_t state = task->getGCState();
        if (state == BxTaskGCStateDead) {
            continue;
        }
        uint64_t lastPoll = task->getGCLastPollNanos();
        uint64_t sincePoll = lastPoll && nowNanos > lastPoll ? nowNanos - lastPoll : 0;
        std::fprintf(
            stderr,
            "Brix GC handshake waiting: epoch=%llu reason=%u task=%u state=%s "
            "lastPollNanos=%llu sincePollNanos=%llu contextReady=%d spawn=%s:%u:%u\n",
            (unsigned long long)gGCHandshakeEpoch,
            gGCHandshakeReason,
            task->getId(),
            brixGCTaskGCStateName(state),
            (unsigned long long)lastPoll,
            (unsigned long long)sincePoll,
            task->getGCContextReady() ? 1 : 0,
            task->getSpawnFile() ? task->getSpawnFile() : "<unknown>",
            task->getSpawnLine(),
            task->getSpawnColumn());
        reported = true;
    }
    if (!reported && !brixGCThreadLocalCallbacksStableLocked(gGCHandshakeOwnerThread)) {
        std::fprintf(
            stderr,
            "Brix GC handshake waiting: epoch=%llu reason=%u tasklessCallbacks=%zu\n",
            (unsigned long long)gGCHandshakeEpoch,
            gGCHandshakeReason,
            gGCThreadLocalCallbacks.size());
    }
}

static timespec
brixGCHandshakeDeadline(uint64_t timeoutUsec) {
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
brixGCDefaultHandshakeReportAfterUsec() {
    const char* value = std::getenv("BRIX_GC_HANDSHAKE_REPORT_USEC");
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
brixGCAcknowledgeRunningTask(BxTask* task, uint32_t reason, uint64_t epoch) {
    if (!task) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    BxGCHandshakeTaskRecord* record = brixGCHandshakeTaskRecordLocked(task);
    bool shouldAcknowledge =
        brixGCHandshakeActiveLocked() &&
        gGCHandshakeEpoch == epoch &&
        task != gGCHandshakeOwnerTask &&
        record &&
        record->disposition == BxGCHandshakeTaskAwaiting;
    pthread_mutex_unlock(&gGCHandshakeLock);
    if (!shouldAcknowledge) {
        return;
    }

    brixGCRuntimeFlushThreadAllocationBatches();
    if (reason == BxGCHandshakeReasonMarkSetup) {
        brixGCRuntimeHandshakeTaskWork(reason);
    }
    uint64_t nowNanos = brixGCHandshakeMonotonicNanos();

    pthread_mutex_lock(&gGCHandshakeLock);
    record = brixGCHandshakeTaskRecordLocked(task);
    if (record &&
            record->disposition == BxGCHandshakeTaskAwaiting &&
            brixGCHandshakeActiveLocked() &&
            gGCHandshakeEpoch == epoch &&
            task != gGCHandshakeOwnerTask) {
        task->setGCLastPollNanos(nowNanos);
        task->setGCHandshakeEpoch(epoch);
        record->disposition = BxGCHandshakeTaskSelfAcknowledged;
        pthread_cond_broadcast(&gGCHandshakeCond);
    }
    pthread_mutex_unlock(&gGCHandshakeLock);
}

static void
brixGCHandshakeBeginInternal(uint32_t reason, uint64_t reportAfterUsec) {
    BxTask* currentTask = brixGCActiveTask();
    const void* currentThread = brixGCCurrentThreadIdentity();

    // The owner does not execute the ordinary safepoint acknowledgement path. Donate its
    // buffered pointer publications before it becomes the scanning owner for this epoch.
    brixGCRuntimeFlushThreadAllocationBatches();

    while (true) {
        pthread_mutex_lock(&gGCHandshakeLock);
        if (!brixGCHandshakeActiveLocked()) {
            break;
        }
        if (currentTask == gGCHandshakeOwnerTask) {
            pthread_mutex_unlock(&gGCHandshakeLock);
            return;
        }

        uint32_t activeReason = gGCHandshakeReason;
        uint64_t activeEpoch = gGCHandshakeEpoch;
        pthread_mutex_unlock(&gGCHandshakeLock);
        brixGCAcknowledgeRunningTask(currentTask, activeReason, activeEpoch);

        pthread_mutex_lock(&gGCHandshakeLock);
        while (brixGCHandshakeActiveLocked() && gGCHandshakeEpoch == activeEpoch) {
            pthread_cond_wait(&gGCHandshakeCond, &gGCHandshakeLock);
        }
        pthread_mutex_unlock(&gGCHandshakeLock);
    }

    gGCHandshakeReason = reason;
    gGCHandshakeOwnerTask = currentTask;
    gGCHandshakeOwnerThread = currentThread;
    ++gGCHandshakeEpoch;
    if (!(gGCHandshakeEpoch & (BrixGCControlHandshakeEpochMask >> BrixGCControlHandshakeEpochShift))) {
        ++gGCHandshakeEpoch;
    }
    gGCHandshakeTasks.clear();
    gGCHandshakeTasks.reserve(brixGCRegisteredTaskCount());
    uint64_t taskWaitStartNanos = brixGCHandshakeMonotonicNanos();
    brixGCPublishHandshakeRequest(reason, gGCHandshakeEpoch, true);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    brixGCForEachRegisteredTask([&](BxTask* task) {
        uint32_t state = task->getGCState();
        BxGCHandshakeTaskDisposition disposition = BxGCHandshakeTaskAwaiting;
        if (task == currentTask) {
            disposition = BxGCHandshakeTaskOwner;
        } else if (state == BxTaskGCStateDead) {
            disposition = BxGCHandshakeTaskDead;
        } else if (brixGCIsEnumerableTaskState(state) && task->getGCContextReady()) {
            disposition = BxGCHandshakeTaskSnapshotAcknowledged;
        }
        brixGCAddHandshakeTaskRecordLocked(task, disposition);
        if (disposition == BxGCHandshakeTaskAwaiting) {
            // Ordinary Blocked/Running transitions are far too frequent to timestamp merely
            // for a timeout diagnostic. A task becomes interesting only when this handshake
            // must wait for it, so make the request boundary the diagnostic baseline.
            task->setGCLastPollNanos(taskWaitStartNanos);
        } else {
            task->setGCHandshakeEpoch(gGCHandshakeEpoch);
        }
    });

    std::vector<BxGCHandshakeWaitSample> taskWaitSamples;
    gGCHandshakeTasksWaited = 0;
    gGCHandshakeSlowestTaskNanos = 0;
    brixGCStartTaskWaitSamplesLocked(currentTask, taskWaitStartNanos, taskWaitSamples);
    for (const BxGCHandshakeWaitSample& sample : taskWaitSamples) {
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
    while (!brixGCHandshakeReadyLocked(currentTask, currentThread)) {
        uint64_t nowNanos = brixGCHandshakeMonotonicNanos();
        brixGCUpdateTaskWaitSamplesLocked(nowNanos, taskWaitSamples);
        if (nowNanos >= nextReportNanos) {
            brixGCReportUnresponsiveTasksLocked(currentTask, nowNanos);
            nextReportNanos = nowNanos > UINT64_MAX - reportIntervalNanos
                ? UINT64_MAX
                : nowNanos + reportIntervalNanos;
            continue;
        }
        uint64_t remainingNanos = nextReportNanos - nowNanos;
        uint64_t remainingUsec = std::max<uint64_t>(1, remainingNanos / 1000);
        timespec deadline = brixGCHandshakeDeadline(remainingUsec);
        pthread_cond_timedwait(&gGCHandshakeCond, &gGCHandshakeLock, &deadline);
    }

    brixGCUpdateTaskWaitSamplesLocked(
        brixGCHandshakeMonotonicNanos(),
        taskWaitSamples);

    pthread_mutex_unlock(&gGCHandshakeLock);
}

// *************************************************************************************************

void
brixGCRegisterTask(BxTask* task) {
    if (!task) {
        return;
    }

    BxGCTaskShard& shard = brixGCTaskShard(task);
    shard.lock.lock();
    task->setGCRegistryIndex(shard.tasks.size());
    shard.tasks.push_back(task);
    shard.lock.unlock();

    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if (!brixGCHandshakeRequestedPublic()) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    if (brixGCHandshakeActiveLocked() && !brixGCHandshakeTaskRecordLocked(task)) {
        // A task born after the request is already Parked with a complete initial context.
        // Enroll that snapshot in the epoch without enlarging the exact Running ack set.
        brixGCAddHandshakeTaskRecordLocked(task, BxGCHandshakeTaskSnapshotAcknowledged);
        task->setGCHandshakeEpoch(gGCHandshakeEpoch);
    }
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
brixGCUnregisterTask(BxTask* task) {
    if (!task) {
        return;
    }

    BxGCTaskShard& shard = brixGCTaskShard(task);
    shard.lock.lock();
    size_t index = task->getGCRegistryIndex();
    if (index < shard.tasks.size() && shard.tasks[index] == task) {
        BxTask* last = shard.tasks.back();
        shard.tasks[index] = last;
        last->setGCRegistryIndex(index);
        shard.tasks.pop_back();
    }
    shard.lock.unlock();

    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if (!brixGCHandshakeRequestedPublic()) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    while (brixGCHandshakeActiveLocked() && brixGCHandshakeTaskRecordLocked(task)) {
        pthread_cond_wait(&gGCHandshakeCond, &gGCHandshakeLock);
    }
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
brixGCRegisterThread(BxThread* thread) {
    if (!thread) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    gGCThreads.push_back(thread);
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
brixGCUnregisterThread(BxThread* thread) {
    if (!thread) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    gGCThreads.erase(std::remove(gGCThreads.begin(), gGCThreads.end(), thread), gGCThreads.end());
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
brixGCBeginThreadLocalCallback() {
    const void* thread = brixGCCurrentThreadIdentity();
    pthread_mutex_lock(&gGCHandshakeLock);
    while (gGCThreadLocalCallbackDepth == 0 &&
            brixGCHandshakeActiveLocked() &&
            thread != gGCHandshakeOwnerThread) {
        pthread_cond_wait(&gGCHandshakeCond, &gGCHandshakeLock);
    }
    gGCThreadLocalCallbacks.push_back(thread);
    ++gGCThreadLocalCallbackDepth;
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
brixGCEndThreadLocalCallback() {
    const void* thread = brixGCCurrentThreadIdentity();
    pthread_mutex_lock(&gGCHandshakeLock);
    auto found = std::find(
        gGCThreadLocalCallbacks.rbegin(),
        gGCThreadLocalCallbacks.rend(),
        thread);
    if (found == gGCThreadLocalCallbacks.rend() || gGCThreadLocalCallbackDepth == 0) {
        std::fprintf(stderr, "Brix GC thread-local callback accounting underflow\n");
        std::abort();
    }
    gGCThreadLocalCallbacks.erase(std::next(found).base());
    --gGCThreadLocalCallbackDepth;
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

extern "C" bool
brixGCThreadHasLocalCallbackRegistration() {
    return gGCThreadLocalCallbackDepth != 0;
}

void
brixGCSetTaskState(BxTask* task, uint32_t state) {
    brixGCSetTaskStateAndContext(task, state, task ? task->getGCContextReady() : false);
}

void
brixGCSetTaskStateAndContext(BxTask* task, uint32_t state, bool contextReady) {
    if (!task) {
        return;
    }

    if (!brixGCHandshakeRequestedPublic()) {
        brixGCStoreTaskState(task, state, contextReady);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (!brixGCHandshakeRequestedPublic()) {
            return;
        }
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    brixGCStoreTaskState(task, state, contextReady);
    BxGCHandshakeTaskRecord* record = brixGCHandshakeTaskRecordLocked(task);
    if (record &&
            brixGCHandshakeActiveLocked() &&
            brixGCIsStableTaskState(state) &&
            (state == BxTaskGCStateDead || contextReady)) {
        task->setGCHandshakeEpoch(gGCHandshakeEpoch);
        if (state == BxTaskGCStateDead) {
            record->disposition = BxGCHandshakeTaskDead;
        } else if (record->disposition == BxGCHandshakeTaskAwaiting) {
            record->disposition = BxGCHandshakeTaskSnapshotAcknowledged;
        }
        pthread_cond_broadcast(&gGCHandshakeCond);
    } else if (brixGCHandshakeActiveLocked()) {
        pthread_cond_broadcast(&gGCHandshakeCond);
    }
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
brixGCPrepareTaskResume(BxTask* task) {
    if (!task) {
        return;
    }

    if (!brixGCHandshakeRequestedPublic()) {
        uint32_t previousState = task->getGCState();
        bool previousReady = task->getGCContextReady();
        brixGCStoreTaskState(task, BxTaskGCStateRunning, false);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (!brixGCHandshakeRequestedPublic()) {
            return;
        }
        uint32_t reason = 0;
        uint64_t epoch = 0;
        bool selfAcknowledge = false;
        pthread_mutex_lock(&gGCHandshakeLock);
        if (brixGCHandshakeActiveLocked() && task != gGCHandshakeOwnerTask) {
            BxGCHandshakeTaskRecord* record = brixGCHandshakeTaskRecordLocked(task);
            if (record && record->disposition == BxGCHandshakeTaskAwaiting) {
                if (brixGCIsEnumerableTaskState(previousState) && previousReady) {
                    brixGCStoreTaskState(task, previousState, true);
                    record->disposition = BxGCHandshakeTaskSnapshotAcknowledged;
                    task->setGCHandshakeEpoch(gGCHandshakeEpoch);
                    pthread_cond_broadcast(&gGCHandshakeCond);
                } else {
                    reason = gGCHandshakeReason;
                    epoch = gGCHandshakeEpoch;
                    selfAcknowledge = true;
                }
            } else if (record && brixGCIsEnumerableTaskState(previousState) && previousReady) {
                brixGCStoreTaskState(task, previousState, true);
                pthread_cond_broadcast(&gGCHandshakeCond);
            }
        }
        pthread_mutex_unlock(&gGCHandshakeLock);
        if (selfAcknowledge) {
            brixGCAcknowledgeRunningTask(task, reason, epoch);
        }
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    while (brixGCHandshakeActiveLocked()) {
        if (task == gGCHandshakeOwnerTask) {
            task->setGCHandshakeEpoch(gGCHandshakeEpoch);
            pthread_cond_broadcast(&gGCHandshakeCond);
            pthread_mutex_unlock(&gGCHandshakeLock);
            return;
        }
        BxGCHandshakeTaskRecord* record = brixGCHandshakeTaskRecordLocked(task);
        if (brixGCActiveTask() == task &&
                record &&
                record->disposition == BxGCHandshakeTaskAwaiting) {
            uint32_t reason = gGCHandshakeReason;
            uint64_t epoch = gGCHandshakeEpoch;
            brixGCStoreTaskState(task, BxTaskGCStateRunning, false);
            pthread_mutex_unlock(&gGCHandshakeLock);
            brixGCAcknowledgeRunningTask(task, reason, epoch);
            pthread_mutex_lock(&gGCHandshakeLock);
            continue;
        }
        pthread_cond_wait(&gGCHandshakeCond, &gGCHandshakeLock);
    }

    brixGCStoreTaskState(task, BxTaskGCStateRunning, false);
    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

void
brixGCRevalidateThreadAllocation(uint64_t* rangeControl) {
    uint64_t control = brixGCPublicControlSnapshot();
    if (control != *rangeControl) {
        brixGCRuntimeFlushThreadAllocationBatches();
        *rangeControl = control;
    }
}

// *************************************************************************************************

static void
brixGCSafepointHandshake() {
    uint64_t control = brixGCPublicControlSnapshot();
    if (!brixGCControlWordHandshakeRequested(control)) {
        return;
    }

    BxTask* task = brixGCActiveTask();
    if (brixGCTaskAcknowledgedControl(task, control)) {
        return;
    }
    if (task) {
        task->setGCLastPollNanos(brixGCHandshakeMonotonicNanos());
    }
    if (brixGCIsRuntimeCritical()) {
        return;
    }

    if (!task) {
        return;
    }

    pthread_mutex_lock(&gGCHandshakeLock);
    if (!brixGCHandshakeActiveLocked()) {
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
    brixGCAcknowledgeRunningTask(task, reason, epoch);
}

extern "C" __attribute__((noinline)) void
brixGCSafepoint() {
    if (__atomic_load_n(&brixInterruptRequestedFlags, __ATOMIC_RELAXED) &
            BrixInterruptSchedulerRequested) {
        uintptr_t* frame = static_cast<uintptr_t*>(__builtin_frame_address(0));
        brixServiceTaskInterrupts(false, frame[0], frame[1] - 4);
    }
    brixGCSafepointHandshake();
}

extern "C" __attribute__((noinline)) void
brixTaskSafepoint() {
    brixGCSafepointHandshake();
    if (__atomic_load_n(&brixInterruptRequestedFlags, __ATOMIC_RELAXED) &
            BrixInterruptSchedulerRequested) {
        uintptr_t* frame = static_cast<uintptr_t*>(__builtin_frame_address(0));
        brixServiceTaskInterrupts(true, frame[0], frame[1] - 4);
    }
}

extern "C" void
brixGCHandshakeBegin(uint32_t reason) {
    brixGCHandshakeBeginInternal(reason, brixGCDefaultHandshakeReportAfterUsec());
}

extern "C" void
brixGCHandshakeBeginWithReportAfter(uint32_t reason, uint64_t reportAfterUsec) {
    brixGCHandshakeBeginInternal(reason, std::max<uint64_t>(1, reportAfterUsec));
}

extern "C" void
brixGCHandshakeEnd() {
    pthread_mutex_lock(&gGCHandshakeLock);

    brixGCPublishHandshakeRequest(gGCHandshakeReason, gGCHandshakeEpoch, false);
    gGCHandshakeReason = 0;
    gGCHandshakeOwnerTask = NULL;
    gGCHandshakeOwnerThread = NULL;
    gGCHandshakeTasks.clear();

    pthread_cond_broadcast(&gGCHandshakeCond);
    pthread_mutex_unlock(&gGCHandshakeLock);
}

extern "C" uint32_t
brixGCHandshakeReason() {
    pthread_mutex_lock(&gGCHandshakeLock);
    uint32_t reason = gGCHandshakeReason;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return reason;
}

extern "C" uint64_t
brixGCHandshakeEpoch() {
    pthread_mutex_lock(&gGCHandshakeLock);
    uint64_t epoch = gGCHandshakeEpoch;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return epoch;
}

extern "C" size_t
brixGCHandshakeTaskCount() {
    return brixGCRegisteredTaskCount();
}

extern "C" size_t
brixGCHandshakeStableTaskCount() {
    size_t count = 0;
    brixGCForEachRegisteredTask([&](BxTask* task) {
        if (brixGCIsStableTaskState(task->getGCState())) {
            ++count;
        }
    });
    return count;
}

extern "C" size_t
brixGCHandshakeThreadCount() {
    pthread_mutex_lock(&gGCHandshakeLock);
    size_t count = gGCThreads.size();
    pthread_mutex_unlock(&gGCHandshakeLock);
    return count;
}

extern "C" size_t
brixGCHandshakeTasksWaited() {
    pthread_mutex_lock(&gGCHandshakeLock);
    size_t count = gGCHandshakeTasksWaited;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return count;
}

extern "C" uint64_t
brixGCHandshakeSlowestTaskNanos() {
    pthread_mutex_lock(&gGCHandshakeLock);
    uint64_t duration = gGCHandshakeSlowestTaskNanos;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return duration;
}

extern "C" size_t
brixGCHandshakeReportCount() {
    pthread_mutex_lock(&gGCHandshakeLock);
    size_t count = gGCHandshakeReportCount;
    pthread_mutex_unlock(&gGCHandshakeLock);
    return count;
}

extern "C" uint32_t
brixGetTaskGCState(BxTask* task) {
    return task ? task->getGCState() : BxTaskGCStateDead;
}

extern "C" void
brixGCEnumerateTasks(BxGCTaskVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    std::vector<BxGCTaskSnapshot> snapshots;
    pthread_mutex_lock(&gGCHandshakeLock);
    bool activeHandshake = brixGCHandshakeActiveLocked();
    size_t taskCount = activeHandshake
        ? gGCHandshakeTasks.size()
        : brixGCRegisteredTaskCount();
    snapshots.reserve(taskCount);
    auto appendSnapshot = [&](BxTask* task) {
        uint32_t state = task->getGCState();
        if (!task->getGCContextReady() || !brixGCIsEnumerableTaskState(state)) {
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
        for (const BxGCHandshakeTaskRecord& record : gGCHandshakeTasks) {
            if (record.disposition == BxGCHandshakeTaskSnapshotAcknowledged) {
                appendSnapshot(record.task);
            }
        }
    } else {
        brixGCForEachRegisteredTask(appendSnapshot);
    }
    pthread_mutex_unlock(&gGCHandshakeLock);

    for (const BxGCTaskSnapshot& snapshot : snapshots) {
        uint64_t startNanos = brixGCHandshakeMonotonicNanos();
        visitor(
            snapshot.task,
            snapshot.state,
            snapshot.stackBase,
            snapshot.stackSize,
            snapshot.context,
            data);
        uint64_t endNanos = brixGCHandshakeMonotonicNanos();
        brixGCRuntimeRecordPerTaskSetupScan(
            static_cast<size_t>(startNanos),
            static_cast<size_t>(endNanos - startNanos));
    }

}

extern "C" BrixGCTemporaryRootStorage*
brixGCActiveTaskTemporaryRootStorage(void) {
    BxTask* task = brixGCActiveTask();
    if (!task || task->getGCState() != BxTaskGCStateRunning) {
        return nullptr;
    }
    return &task->getRuntimeContext()->temporaryRoots;
}

namespace {

struct BxGCTemporaryRootListScan {
    BxGCTemporaryRootListVisitor visitor;
    void* data;
    BxTask* activeTask;
};

static void
brixGCVisitTaskTemporaryRootList(
    BxTask* task,
    uint32_t,
    void*,
    size_t,
    void*,
    void* data
) {
    BxGCTemporaryRootListScan* scan = static_cast<BxGCTemporaryRootListScan*>(data);
    if (!task || task == scan->activeTask) {
        return;
    }
    scan->visitor(task->getRuntimeContext()->temporaryRoots.head, scan->data);
}

}

extern "C" void
brixGCVisitTaskTemporaryRootLists(BxGCTemporaryRootListVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }
    BxTask* activeTask = brixGCActiveTask();
    if (activeTask) {
        visitor(activeTask->getRuntimeContext()->temporaryRoots.head, data);
    }
    BxGCTemporaryRootListScan scan { visitor, data, activeTask };
    brixGCEnumerateTasks(brixGCVisitTaskTemporaryRootList, &scan);
}

namespace {

struct BxGCParseRegionScan {
    BxParseRegionAllocationVisitor visitor;
    void* data;
    BxTask* activeTask;
};

static void
brixGCVisitTaskParseRegion(
    BxTask* task,
    uint32_t,
    void*,
    size_t,
    void*,
    void* data
) {
    BxGCParseRegionScan* scan = static_cast<BxGCParseRegionScan*>(data);
    if (!task || task == scan->activeTask) {
        return;
    }
    BxTaskRuntimeContext* context = task->getRuntimeContext();
    if (!context->parseRegion || !context->parseRegionScanner) {
        return;
    }
    context->parseRegionScanner(context->parseRegion, scan->visitor, scan->data);
}

}

extern "C" void
brixGCVisitActiveTaskParseRegionAllocations(
    BxParseRegionAllocationVisitor visitor,
    void* data
) {
    if (!visitor) {
        return;
    }
    BxTask* task = brixGCActiveTask();
    if (!task) {
        return;
    }
    BxTaskRuntimeContext* context = task->getRuntimeContext();
    if (context->parseRegion && context->parseRegionScanner) {
        context->parseRegionScanner(context->parseRegion, visitor, data);
    }
}

extern "C" void
brixGCVisitTaskParseRegionAllocations(
    BxParseRegionAllocationVisitor visitor,
    void* data
) {
    if (!visitor) {
        return;
    }
    BxTask* activeTask = brixGCActiveTask();
    brixGCVisitActiveTaskParseRegionAllocations(visitor, data);
    BxGCParseRegionScan scan { visitor, data, activeTask };
    brixGCEnumerateTasks(brixGCVisitTaskParseRegion, &scan);
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
brixGCScanCurrentStack(BxGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    BxContext context;
    brixContextGet(&context);

    BxTask* task = brixGCActiveTask();
    if (task) {
        brixGCScanEventContextRoots(
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
    brixGCScanEventContextRoots(&context, stackBase, stackSize, false, visitor, data);
}

extern "C" void
brixGCScanTaskStacks(BxGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    BxGCScanData scan;
    scan.visitor = visitor;
    scan.data = data;
    brixGCEnumerateTasks(brixGCScanTaskRootsVisitor, &scan);
}

extern "C" void
brixGCScanCurrentStackConservative(BxGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    BxContext context;
    brixContextGet(&context);

    BxTask* task = brixGCActiveTask();
    if (task) {
        brixGCScanEventConservativeContextRoots(
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
    brixGCScanEventConservativeContextRoots(&context, stackBase, stackSize, visitor, data);
}

extern "C" void
brixGCScanTaskStacksConservative(BxGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    BxGCScanData scan;
    scan.visitor = visitor;
    scan.data = data;
    brixGCEnumerateTasks(brixGCScanTaskConservativeRootsVisitor, &scan);
}

extern "C" void
brixGCScanStorageRoots(BxGCStackRootVisitor visitor, void* data) {
    if (!visitor) {
        return;
    }

    size_t taskCount = 0;
    size_t threadCount = 0;
    taskCount = brixGCRegisteredTaskCount();
    pthread_mutex_lock(&gGCHandshakeLock);
    threadCount = gGCThreads.size();
    pthread_mutex_unlock(&gGCHandshakeLock);

    std::vector<BxGCStorageRoot> roots;
    roots.reserve(brixGCStorageRootRecordCapacity(taskCount, threadCount));

    BxGCStorageRootCollectData collect;
    collect.roots = &roots;
    pthread_mutex_lock(&gGCHandshakeLock);
    brixGCVisitStorageRootRecords(brixGCCollectStorageRootRecord, &collect);
    pthread_mutex_unlock(&gGCHandshakeLock);

    for (const BxGCStorageRoot& root : roots) {
        visitor(root.base, root.descriptor, data);
    }
}

extern "C" void
brixGCEnterRuntimeBlock() {
    if (brixGCIsRuntimeCritical()) {
        return;
    }

    brixGCEnterBlockingContext(BxTaskGCStateBlocked);
}

extern "C" void
brixGCLeaveRuntimeBlock() {
    if (brixGCIsRuntimeCritical()) {
        return;
    }

    brixGCLeaveBlockingContext();
}

extern "C" void
brixGCEnterBlockingContextWithRegisters(
    uint32_t state,
    void* managedFramePointer,
    void* managedReturnPC,
    const uintptr_t* managedRegisters,
    void* managedStackPointer
) {
    if (brixGCIsRuntimeCritical()) {
        return;
    }
    brixGCEnterBlockingContext(
        state,
        managedFramePointer,
        managedReturnPC,
        managedStackPointer,
        managedRegisters);
}

extern "C" __attribute__((naked)) void
brixGCEnterFFI() {
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
        "bl " BX_ASM_SYMBOL(brixGCEnterBlockingContextWithRegisters) "\n"
        "ldr x30, [sp, #88]\n"
        "add sp, sp, #96\n"
        "ret\n"
    );
}

extern "C" __attribute__((naked)) void
brixGCEnterZone() {
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
        "bl " BX_ASM_SYMBOL(brixGCEnterBlockingContextWithRegisters) "\n"
        "ldr x30, [sp, #88]\n"
        "add sp, sp, #96\n"
        "ret\n"
    );
}

extern "C" void
brixGCLeaveZone() {
    if (brixGCIsRuntimeCritical()) {
        return;
    }
    brixGCLeaveBlockingContext();
}

extern "C" void
brixGCLeaveFFI() {
    if (brixGCIsRuntimeCritical()) {
        return;
    }

    brixGCLeaveBlockingContext();
}

extern "C" void
brixGCEnterManagedCallback() {
    BxTask* task = brixGCActiveTask();
    if (task) {
        BxGCManagedCallbackKind kind = BxGCManagedCallbackNone;
        uint32_t state = task->getGCState();
        if (state == BxTaskGCStateBlocked || state == BxTaskGCStateInZone) {
            // The outer FFI/zone context may predate one or more phase/generation transitions.
            // Revoke its inactive allocation ranges before making the callback Running when the
            // control word moved since the task stopped running, as an outermost leave does. This
            // also keeps an InZone snapshot from hiding managed frames entered by a native callback.
            brixGCResumeFromBlockingContext(task, task->getGCBlockingEntryControl());
            kind = state == BxTaskGCStateBlocked
                ? BxGCManagedCallbackBlocked
                : BxGCManagedCallbackInZone;
        }
        task->pushGCManagedCallbackKind(kind);
        return;
    }

    if (gGCManagedCallbackDepth++ == 0) {
        brixGCBeginThreadLocalCallback();
        brixGCRevalidateThreadAllocation(&gGCTasklessCallbackAllocationControl);
        gGCManagedCallbackKind = BxGCManagedCallbackTaskless;
    }
}

extern "C" void
brixGCLeaveManagedCallback() {
    BxTask* task = brixGCActiveTask();
    BxGCManagedCallbackKind kind = BxGCManagedCallbackNone;
    if (task) {
        kind = static_cast<BxGCManagedCallbackKind>(task->popGCManagedCallbackKind());
    } else {
        bool outermost = gGCManagedCallbackDepth != 0 && --gGCManagedCallbackDepth == 0;
        if (!outermost) {
            return;
        }
        kind = gGCManagedCallbackKind;
        gGCManagedCallbackKind = BxGCManagedCallbackNone;
    }
    if (task && (kind == BxGCManagedCallbackBlocked || kind == BxGCManagedCallbackInZone)) {
        task->setGCBlockingEntryControl(brixGCPublicControlSnapshot());
    }
    if (kind == BxGCManagedCallbackBlocked) {
        brixGCPublishBlockingContext(
            BxTaskGCStateBlocked,
            task ? task->getGCBlockingContextSnapshot() : NULL);
    } else if (kind == BxGCManagedCallbackInZone) {
        brixGCPublishBlockingContext(
            BxTaskGCStateInZone,
            task ? task->getGCBlockingContextSnapshot() : NULL);
    } else if (kind == BxGCManagedCallbackTaskless) {
        brixGCRuntimeFlushWriteBarrierBuffer();
        brixGCEndThreadLocalCallback();
    }
}

extern "C" BxTaskRuntimeContext*
brixGCEnterForeignCallbackFrom(uint64_t callerRegister) {
    BxTask* task = brixGCActiveTask();
    if (task) {
        task->pushGCManagedCallbackRegister(callerRegister);
    } else {
        gGCManagedCallbackRegisters.push_back(callerRegister);
    }
    brixGCEnterManagedCallback();
    return brixGetActiveTaskRuntimeContext();
}

extern "C" uint64_t
brixGCLeaveForeignCallbackTo() {
    BxTask* task = brixGCActiveTask();
    brixGCLeaveManagedCallback();
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
#define BX_CALLBACK_SYMBOL(name) "_" #name
#define BX_CALLBACK_FUNCTION_TYPE(name) ""
#else
#define BX_CALLBACK_SYMBOL(name) #name
#define BX_CALLBACK_FUNCTION_TYPE(name) ".type " #name ", %function\n"
#endif

asm(
    ".text\n"
    ".p2align 2\n"
    ".globl " BX_CALLBACK_SYMBOL(brixGCEnterForeignCallback) "\n"
    BX_CALLBACK_FUNCTION_TYPE(brixGCEnterForeignCallback)
    BX_CALLBACK_SYMBOL(brixGCEnterForeignCallback) ":\n"
    "stp x29, x30, [sp, #-16]!\n"
    "mov x29, sp\n"
    "mov x0, x27\n"
    "bl " BX_CALLBACK_SYMBOL(brixGCEnterForeignCallbackFrom) "\n"
    "mov x27, x0\n"
    "ldp x29, x30, [sp], #16\n"
    "ret\n"
    ".p2align 2\n"
    ".globl " BX_CALLBACK_SYMBOL(brixGCLeaveForeignCallback) "\n"
    BX_CALLBACK_FUNCTION_TYPE(brixGCLeaveForeignCallback)
    BX_CALLBACK_SYMBOL(brixGCLeaveForeignCallback) ":\n"
    "stp x29, x30, [sp, #-16]!\n"
    "mov x29, sp\n"
    "bl " BX_CALLBACK_SYMBOL(brixGCLeaveForeignCallbackTo) "\n"
    "mov x27, x0\n"
    "ldp x29, x30, [sp], #16\n"
    "ret\n");

extern "C" void
brixGCEnterRuntimeCritical() {
    ++gGCRuntimeCriticalDepth;
}

extern "C" void
brixGCLeaveRuntimeCritical() {
    if (gGCRuntimeCriticalDepth == 0) {
        std::fprintf(stderr, "Brix GC runtime critical section underflow\n");
        std::abort();
    }

    --gGCRuntimeCriticalDepth;
}
