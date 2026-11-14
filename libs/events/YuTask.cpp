
#include <YuEvents/YuTask.h>
#include <YuEvents/YuThread.h>
#include <YuEvents/YuGCHandshake.h>
#include <YuEvents/YuStack.h>
#include <YuCore/YuCore.h>

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

struct YuTaskHandleTable {
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
    std::vector<YuTask*> slots {NULL};
    std::vector<uint32_t> freeSlots;
};

static YuTaskHandleTable&
taskHandleTable() {
    static YuTaskHandleTable* table = new YuTaskHandleTable();
    return *table;
}

struct YuTaskHandleTableLock {
    explicit YuTaskHandleTableLock(YuTaskHandleTable& table): table(table) {
        while (table.lock.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    ~YuTaskHandleTableLock() {
        table.lock.clear(std::memory_order_release);
    }

    YuTaskHandleTable& table;
};

extern "C" void __attribute__((weak))
yumiDestroyTaskRuntimeContext(YuTaskRuntimeContext*) {
}

// *************************************************************************************************

YuTask::YuTask(
    YuTaskQueue* aQueue,
    YuTaskStartFunc func,
    void* closure,
    size_t localSize,
    const YuTaskProvenance& provenance):
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
    terminating(false),
    active(false),
    runState(YuTaskRunIdle),
    gcState(YuTaskGCStateParked),
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
            fprintf(stderr, "Unable to allocate Yumi task local memory\n");
            abort();
        }
#ifdef YU_ENABLE_GC
        GC_add_roots(locals, locals + localsSize);
#endif
    }
    runtimeContext.taskLocals = locals;

    size_t stackSize = flags & YU_TASK_FLAG_MAIN ? kYuMainTaskStackSize : kYuTaskStackSize;
    yumiContextNew(&context, func, closure, stackSize);
    context.x27 = reinterpret_cast<uint64_t>(&runtimeContext);
    yumiGCRegisterTask(this);
}

YuTask::~YuTask() {
    // --liveTaskCounter;

    releaseHandle();

    yumiGCSetTaskStateAndContext(this, YuTaskGCStateDead, false);
    yumiGCUnregisterTask(this);

    yumiDestroyTaskRuntimeContext(&runtimeContext);
    yumiContextDestroy(&context);

    if (locals) {
#ifdef YU_ENABLE_GC
        GC_remove_roots(locals, locals + localsSize);
#endif
        free(locals);
    }
}

void
YuTask::terminate() {
    // printf("terminate!\n"); fflush(stdout);
}

uint64_t
YuTask::ensureHandle() {
    if (handle) {
        return handle;
    }

    YuTaskHandleTable& table = taskHandleTable();
    YuTaskHandleTableLock lock(table);
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
YuTask::releaseHandle() {
    if (!handle) {
        return;
    }

    std::vector<YuTaskFinishHook> hooks;
    {
        YuTaskHandleTable& table = taskHandleTable();
        YuTaskHandleTableLock lock(table);
        uint32_t slot = static_cast<uint32_t>(handle);
        table.slots[slot] = NULL;
        table.freeSlots.push_back(slot);
        handle = 0;
        hooks.swap(finishHooks);
    }
    for (const YuTaskFinishHook& hook : hooks) {
        hook.func(hook.closure);
    }
}

bool
YuTask::addFinishHook(uint64_t handle, YuTaskStartFunc func, void* closure) {
    YuTaskHandleTable& table = taskHandleTable();
    uint32_t slot = static_cast<uint32_t>(handle);
    YuTaskHandleTableLock lock(table);
    if (slot >= table.slots.size()) {
        return false;
    }

    YuTask* task = table.slots[slot];
    if (!task || task->handle != handle) {
        return false;
    }

    task->finishHooks.push_back(YuTaskFinishHook{func, closure});
    return true;
}

YuTask*
YuTask::pinHandle(uint64_t handle) {
    YuTaskHandleTable& table = taskHandleTable();
    uint32_t slot = static_cast<uint32_t>(handle);
    YuTaskHandleTableLock lock(table);
    if (slot >= table.slots.size()) {
        return NULL;
    }

    YuTask* task = table.slots[slot];
    if (!task || task->handle != handle) {
        return NULL;
    }

    task->handlePins.fetch_add(1, std::memory_order_relaxed);
    return task;
}
