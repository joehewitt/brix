
#include <BxEvents/BxTask.h>
#include <BxEvents/BxThread.h>
#include <BxEvents/BxTaskQueue.h>
#include <BxEvents/BxGCHandshake.h>
#include <BxEvents/BxStack.h>
#include <BxCore/BxCore.h>

#include <atomic>
#include <cstdlib>
#include <sys/mman.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <vector>
// #include <gc.h>

static std::atomic<uint32_t> taskCounter {0};
// uint32_t liveTaskCounter = 0;

struct BxTaskHandleTable {
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
    std::vector<BxTask*> slots {NULL};
    std::vector<uint32_t> freeSlots;
};

static BxTaskHandleTable&
taskHandleTable() {
    static BxTaskHandleTable* table = new BxTaskHandleTable();
    return *table;
}

struct BxTaskHandleTableLock {
    explicit BxTaskHandleTableLock(BxTaskHandleTable& table): table(table) {
        while (table.lock.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    ~BxTaskHandleTableLock() {
        table.lock.clear(std::memory_order_release);
    }

    BxTaskHandleTable& table;
};

extern "C" void __attribute__((weak))
brixDestroyTaskRuntimeContext(BxTaskRuntimeContext*) {
}

// *************************************************************************************************

BxTask::BxTask(
    BxTaskQueue* aQueue,
    BxTaskStartFunc func,
    void* closure,
    size_t localSize,
    const BxTaskProvenance& provenance):
    uid(taskCounter.fetch_add(1, std::memory_order_relaxed) + 1),
    parentId(provenance.parentId),
    spawnFile(provenance.spawnFile),
    spawnLine(provenance.spawnLine),
    spawnColumn(provenance.spawnColumn),
    flags(provenance.flags),
    schedulingClass(provenance.schedulingClass),
    queue(aQueue),
    runtimeContext(),
    locals(NULL),
    localsSize(localSize),
    canceled(false),
    cancelShield(0),
    shieldLocked(false),
    terminating(false),
    terminationRaised(false),
    active(false),
    runState(BxTaskRunIdle),
    gcState(BxTaskGCStateParked),
    gcHandshakeEpoch(0),
    gcLastPollNanos(0),
    gcContextReady(true),
    gcAssistDebt(0),
    gcAssistDebtCycle(0),
    gcBlockingDepth(0),
    gcBlockingContext(),
    gcNestedBlockingContexts(),
    gcManagedCallbackKinds(),
    handle(0),
    handlePins(0),
    ending(NULL),
    runPrev(NULL),
    runNext(NULL),
    runList(NULL),
    liveIndex(0),
    gcRegistryIndex(0),
    gcHandshakeRecordIndex(0)
{
    // printf("task %d of %d\n", ++liveTaskCounter, taskCounter);
    if (localsSize) {
        locals = (char*)calloc(1, localsSize);
        if (!locals) {
            fprintf(stderr, "Unable to allocate Brix task local memory\n");
            abort();
        }
#ifdef BX_ENABLE_GC
        GC_add_roots(locals, locals + localsSize);
#endif
    }
    runtimeContext.taskLocals = locals;

    bool large = flags & (BX_TASK_FLAG_MAIN | BX_TASK_FLAG_LARGE_STACK);
    size_t stackSize = large ? kBxMainTaskStackSize : kBxTaskStackSize;
    brixContextNew(&context, func, closure, stackSize);
    context.x27 = reinterpret_cast<uint64_t>(&runtimeContext);
    brixGCRegisterTask(this);
}

BxTask::~BxTask() {
    // --liveTaskCounter;

    releaseHandle();

    brixGCSetTaskStateAndContext(this, BxTaskGCStateDead, false);
    brixGCUnregisterTask(this);

    brixDestroyTaskRuntimeContext(&runtimeContext);
    brixContextDestroy(&context);

    if (locals) {
#ifdef BX_ENABLE_GC
        GC_remove_roots(locals, locals + localsSize);
#endif
        free(locals);
    }
}

void
BxTask::terminate() {
    // printf("terminate!\n"); fflush(stdout);
}

uint64_t
BxTask::ensureHandle() {
    if (handle) {
        return handle;
    }

    BxTaskHandleTable& table = taskHandleTable();
    BxTaskHandleTableLock lock(table);
    uint32_t slot = 0;
    if (table.freeSlots.empty()) {
        slot = static_cast<uint32_t>(table.slots.size());
        table.slots.push_back(this);
    } else {
        slot = table.freeSlots.back();
        table.freeSlots.pop_back();
        table.slots[slot] = this;
    }
    handle = (static_cast<uint64_t>(uid) << 32) | slot;
    return handle;
}

void
BxTask::releaseHandle() {
    if (!handle) {
        return;
    }

    std::vector<BxTaskFinishHook> hooks;
    BxTaskEnding* ended = NULL;
    {
        BxTaskHandleTable& table = taskHandleTable();
        BxTaskHandleTableLock lock(table);
        uint32_t slot = static_cast<uint32_t>(handle);
        table.slots[slot] = NULL;
        table.freeSlots.push_back(slot);
        handle = 0;
        hooks.swap(finishHooks);
        ended = ending;
        ending = NULL;
    }
    if (ended) {
        uint32_t code = runtimeContext.taskEnd;
        if (code == BX_TASK_END_RUNNING) {
            code = isCancelRequested() ? BX_TASK_END_CANCELLED : BX_TASK_END_FINISHED;
        }
        ended->finish(code);
        ended->release();
    }
    for (const BxTaskFinishHook& hook : hooks) {
        hook.func(hook.closure);
    }
}

BxTaskEnding*
BxTask::attachEnding() {
    uint64_t own = ensureHandle();
    BxTaskHandleTableLock lock(taskHandleTable());
    if (!ending) {
        ending = new BxTaskEnding(own, 1, BX_TASK_END_RUNNING);
    }
    ending->retain();
    return ending;
}

BxTaskEnding*
BxTask::endingForHandle(uint64_t handle) {
    BxTaskHandleTable& table = taskHandleTable();
    uint32_t slot = static_cast<uint32_t>(handle);
    BxTaskHandleTableLock lock(table);
    if (slot >= table.slots.size()) {
        return NULL;
    }
    BxTask* task = table.slots[slot];
    if (!task || task->handle != handle) {
        return NULL;
    }
    if (!task->ending) {
        task->ending = new BxTaskEnding(handle, 1, BX_TASK_END_RUNNING);
    }
    task->ending->retain();
    return task->ending;
}

void
BxTaskEnding::finish(uint32_t code) {
    lock.lock();
    end.store(code, std::memory_order_release);
    BxTaskEndWaiter* waiter = waiters;
    waiters = NULL;
    while (waiter) {
        BxTaskEndWaiter* next = waiter->next;
        BxTask* task = waiter->task;
        task->getTaskQueue()->scheduleTask(task, true);
        waiter = next;
    }
    lock.unlock();
}

bool
BxTask::addFinishHook(uint64_t handle, BxTaskStartFunc func, void* closure) {
    BxTaskHandleTable& table = taskHandleTable();
    uint32_t slot = static_cast<uint32_t>(handle);
    BxTaskHandleTableLock lock(table);
    if (slot >= table.slots.size()) {
        return false;
    }

    BxTask* task = table.slots[slot];
    if (!task || task->handle != handle) {
        return false;
    }

    task->finishHooks.push_back(BxTaskFinishHook{func, closure});
    return true;
}

static void
lockShieldFlag(std::atomic<bool>& flag) {
    while (flag.exchange(true, std::memory_order_acquire)) {
        while (flag.load(std::memory_order_relaxed)) {
        }
    }
}

static void
unlockShieldFlag(std::atomic<bool>& flag) {
    flag.store(false, std::memory_order_release);
}

void
BxTask::enterCancelShield() {
    lockShieldFlag(shieldLocked);
    cancelShield.fetch_add(1, std::memory_order_acq_rel);
    unlockShieldFlag(shieldLocked);
}

void
BxTask::leaveCancelShield() {
    lockShieldFlag(shieldLocked);
    if (cancelShield.load(std::memory_order_acquire) != 0) {
        cancelShield.fetch_sub(1, std::memory_order_acq_rel);
    }
    unlockShieldFlag(shieldLocked);
}

bool
BxTask::claimCancelWake() {
    if (!canceled.load(std::memory_order_seq_cst)) {
        return false;
    }
    lockShieldFlag(shieldLocked);
    bool wakeable = cancelShield.load(std::memory_order_acquire) == 0 || getTerminating();
    uint32_t idle = BxTaskRunIdle;
    bool claimed = wakeable &&
        runState.compare_exchange_strong(idle, BxTaskRunQueued, std::memory_order_seq_cst);
    unlockShieldFlag(shieldLocked);
    return claimed;
}

BxTask*
BxTask::pinHandle(uint64_t handle) {
    BxTaskHandleTable& table = taskHandleTable();
    uint32_t slot = static_cast<uint32_t>(handle);
    BxTaskHandleTableLock lock(table);
    if (slot >= table.slots.size()) {
        return NULL;
    }

    BxTask* task = table.slots[slot];
    if (!task || task->handle != handle) {
        return NULL;
    }

    task->handlePins.fetch_add(1, std::memory_order_relaxed);
    return task;
}
