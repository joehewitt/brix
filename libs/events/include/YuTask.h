
#ifndef YU_TASK_H
#define YU_TASK_H

#include <YuEvents/YuContext.h>
#include <YuEvents/YuMutex.h>
#include <YuEvents/YuGCHandshake.h>
#include <YuEvents/YuTaskRuntimeContext.h>

#include <atomic>
#include <stdint.h>
#include <vector>

// *************************************************************************************************

const uint32_t YU_TASK_FLAG_SPIN = 1 << 0;
const uint32_t YU_TASK_FLAG_WORKER = 1 << 1;
const uint32_t YU_TASK_FLAG_GC_MARK_WORKER = 1 << 2;
const uint32_t YU_TASK_FLAG_MAIN = 1 << 3;
const uint32_t YU_TASK_FLAG_LARGE_STACK = 1 << 4;

const uint32_t YU_TASK_SCHEDULING_NORMAL = 0;
const uint32_t YU_TASK_SCHEDULING_PRIORITY = 1;

enum YuTaskRunState : uint32_t {
    YuTaskRunIdle = 0,
    YuTaskRunQueued = 1,
    YuTaskRunRunning = 2,
    YuTaskRunRunningWoken = 3
};

struct YuTaskProvenance {
    YuTaskProvenance(
        uint32_t parentId = 0,
        const char* spawnFile = NULL,
        uint32_t spawnLine = 0,
        uint32_t spawnColumn = 0,
        uint32_t flags = 0,
        uint32_t schedulingClass = YU_TASK_SCHEDULING_NORMAL):
        parentId(parentId),
        spawnFile(spawnFile),
        spawnLine(spawnLine),
        spawnColumn(spawnColumn),
        flags(flags),
        schedulingClass(schedulingClass)
    {}

    uint32_t parentId;
    const char* spawnFile;
    uint32_t spawnLine;
    uint32_t spawnColumn;
    uint32_t flags;
    uint32_t schedulingClass;
};

class YuTask;

struct YuTaskEndWaiter {
    YuTask* task;
    YuTaskEndWaiter* next;
};

// How a task ended, kept for as long as the task or a value naming it holds a reference, and the
// tasks waiting for it to end.
struct YuTaskEnding {
    YuTaskEnding(uint64_t handle, uint32_t refs, uint32_t end):
        refs(refs),
        end(end),
        handle(handle),
        waiters(NULL)
    {}

    void retain() { refs.fetch_add(1, std::memory_order_relaxed); }
    void release() {
        if (refs.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }
    void finish(uint32_t code);

    std::atomic<uint32_t> refs;
    std::atomic<uint32_t> end;
    uint64_t handle;
    YuMutex lock;
    YuTaskEndWaiter* waiters;
};

struct YuTaskFinishHook {
    YuTaskStartFunc func;
    void* closure;
};

struct YuTaskGCBlockingContext {
    YuTaskGCBlockingContext():
        managedCallbackContext(false),
        hasSnapshot(false),
        entryControl(0),
        framePointer(NULL),
        returnPC(NULL),
        stackPointer(NULL),
        registers {}
    {}

    bool managedCallbackContext;
    bool hasSnapshot;
    uint64_t entryControl;
    void* framePointer;
    void* returnPC;
    void* stackPointer;
    uintptr_t registers[10];
};

// *************************************************************************************************

class YuTaskRunList;

class YuTask {
public:
    YuTask(
        YuTaskQueue* queue,
        YuTaskStartFunc func,
        void* closure,
        size_t localSize,
        const YuTaskProvenance& provenance = YuTaskProvenance());
    ~YuTask();

    YuTaskQueue* getTaskQueue() { return queue; }
    YuContext* getContext() { return &context; }
    YuTaskRuntimeContext* getRuntimeContext() { return &runtimeContext; }
    uint32_t getId() { return uid; }
    uint32_t getParentId() { return parentId; }
    const char* getSpawnFile() { return spawnFile; }
    uint32_t getSpawnLine() { return spawnLine; }
    uint32_t getSpawnColumn() { return spawnColumn; }
    uint32_t getFlags() { return flags; }
    uint32_t getSchedulingClass() { return schedulingClass; }
    void* getStackBase() { return context.stack; }
    size_t getStackSize() { return context.stackSize; }
    char* getLocals() { return locals; }
    size_t getLocalsSize() { return localsSize; }
    bool getCanceled() {
        return canceled.load(std::memory_order_acquire) &&
            (cancelShield.load(std::memory_order_acquire) == 0 || getTerminating());
    }
    bool isCancelRequested() { return canceled.load(std::memory_order_acquire); }
    bool getTerminating() { return terminating.load(std::memory_order_acquire); }
    bool getActive() { return active.load(std::memory_order_acquire); }
    uint32_t getRunState() const { return runState.load(std::memory_order_acquire); }
    bool claimQueued() {
        uint32_t state = runState.load(std::memory_order_acquire);
        while (true) {
            if (state == YuTaskRunIdle) {
                if (runState.compare_exchange_weak(state, YuTaskRunQueued,
                        std::memory_order_acq_rel)) {
                    return true;
                }
            } else if (state == YuTaskRunRunning) {
                if (runState.compare_exchange_weak(state, YuTaskRunRunningWoken,
                        std::memory_order_acq_rel)) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    bool claimRunning() {
        uint32_t state = runState.load(std::memory_order_acquire);
        while (true) {
            if (state == YuTaskRunIdle) {
                if (runState.compare_exchange_weak(state, YuTaskRunRunning,
                        std::memory_order_acq_rel)) {
                    return true;
                }
            } else if (state == YuTaskRunRunning) {
                if (runState.compare_exchange_weak(state, YuTaskRunRunningWoken,
                        std::memory_order_acq_rel)) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    void startQueued() { runState.store(YuTaskRunRunning, std::memory_order_release); }
    bool finishRunning(bool reschedule) {
        if (reschedule) {
            runState.store(YuTaskRunQueued, std::memory_order_release);
            return true;
        }
        uint32_t expected = YuTaskRunRunning;
        if (runState.compare_exchange_strong(expected, YuTaskRunIdle,
                std::memory_order_seq_cst)) {
            return false;
        }
        runState.store(YuTaskRunQueued, std::memory_order_release);
        return true;
    }
    void releaseQueued() { runState.store(YuTaskRunIdle, std::memory_order_release); }
    void enterCancelShield();
    void leaveCancelShield();
    bool claimCancelWake();
    uint32_t getGCState() const { return gcState.load(std::memory_order_acquire); }
    uint64_t getGCHandshakeEpoch() const { return gcHandshakeEpoch.load(std::memory_order_acquire); }
    uint64_t getGCLastPollNanos() const { return gcLastPollNanos.load(std::memory_order_acquire); }
    bool getGCContextReady() const { return gcContextReady.load(std::memory_order_acquire); }
    size_t getGCAssistDebt() const { return gcAssistDebt; }
    size_t getGCAssistDebtCycle() const { return gcAssistDebtCycle; }
    uint32_t enterGCBlockingContext() {
        uint64_t entryControl = getGCBlockingEntryControl();
        uint32_t depth = gcBlockingDepth++;
        YuTaskGCBlockingContext* context = &gcBlockingContext;
        if (depth != 0) {
            gcNestedBlockingContexts.emplace_back();
            context = &gcNestedBlockingContexts.back();
        }
        context->managedCallbackContext = false;
        context->hasSnapshot = false;
        context->entryControl = entryControl;
        return depth;
    }
    bool leaveGCBlockingContext(bool& managedCallbackContext, uint64_t& entryControl) {
        managedCallbackContext = false;
        entryControl = 0;
        if (gcBlockingDepth == 0) {
            return false;
        }
        YuTaskGCBlockingContext* context = gcBlockingDepth == 1
            ? &gcBlockingContext
            : &gcNestedBlockingContexts.back();
        managedCallbackContext = context->managedCallbackContext;
        entryControl = context->entryControl;
        if (gcBlockingDepth != 1) {
            gcNestedBlockingContexts.pop_back();
        }
        return --gcBlockingDepth == 0;
    }
    void markGCBlockingContextManagedCallback() {
        YuTaskGCBlockingContext* context = gcBlockingDepth == 1
            ? &gcBlockingContext
            : &gcNestedBlockingContexts.back();
        context->managedCallbackContext = true;
    }
    void setGCBlockingContextSnapshot(
        void* framePointer,
        void* returnPC,
        void* stackPointer,
        const uintptr_t* registers
    ) {
        if (gcBlockingDepth == 0 || !framePointer || !returnPC || !registers) {
            return;
        }
        YuTaskGCBlockingContext& context = gcBlockingDepth == 1
            ? gcBlockingContext
            : gcNestedBlockingContexts.back();
        context.hasSnapshot = true;
        context.framePointer = framePointer;
        context.returnPC = returnPC;
        context.stackPointer = stackPointer;
        for (size_t index = 0; index < 10; ++index) {
            context.registers[index] = registers[index];
        }
    }
    const YuTaskGCBlockingContext* getGCBlockingContextSnapshot() const {
        if (gcBlockingDepth == 0) {
            return NULL;
        }
        const YuTaskGCBlockingContext* context = gcBlockingDepth == 1
            ? &gcBlockingContext
            : &gcNestedBlockingContexts.back();
        return context->hasSnapshot ? context : NULL;
    }
    bool getGCBlockingContextSnapshot(YuTaskGCBlockingContext& snapshot) const {
        const YuTaskGCBlockingContext* context = getGCBlockingContextSnapshot();
        if (!context) {
            return false;
        }
        snapshot = *context;
        return true;
    }
    uint64_t getGCBlockingEntryControl() const {
        if (gcBlockingDepth == 0) {
            return 0;
        }
        return gcBlockingDepth == 1
            ? gcBlockingContext.entryControl
            : gcNestedBlockingContexts.back().entryControl;
    }
    void setGCBlockingEntryControl(uint64_t control) {
        if (gcBlockingDepth == 0) {
            return;
        }
        YuTaskGCBlockingContext& context = gcBlockingDepth == 1
            ? gcBlockingContext
            : gcNestedBlockingContexts.back();
        context.entryControl = control;
    }
    void pushGCManagedCallbackKind(uint32_t kind) {
        gcManagedCallbackKinds.push_back(kind);
    }
    uint32_t popGCManagedCallbackKind() {
        if (gcManagedCallbackKinds.empty()) {
            return 0;
        }
        uint32_t kind = gcManagedCallbackKinds.back();
        gcManagedCallbackKinds.pop_back();
        return kind;
    }
    bool hasGCManagedCallback() const { return !gcManagedCallbackKinds.empty(); }
    void pushGCManagedCallbackRegister(uint64_t value) {
        gcManagedCallbackRegisters.push_back(value);
    }
    uint64_t popGCManagedCallbackRegister() {
        if (gcManagedCallbackRegisters.empty()) {
            return 0;
        }
        uint64_t value = gcManagedCallbackRegisters.back();
        gcManagedCallbackRegisters.pop_back();
        return value;
    }
    bool inGCBlockingContext() const { return gcBlockingDepth != 0; }

    void cancel() { canceled.store(true, std::memory_order_seq_cst); }
    bool takeTermination() {
        if (!getTerminating() || terminationRaised) {
            return false;
        }
        terminationRaised = true;
        return true;
    }
    void requestTermination() {
        terminating.store(true, std::memory_order_seq_cst);
        canceled.store(true, std::memory_order_seq_cst);
    }
    void setGCState(uint32_t state) { gcState.store(state, std::memory_order_release); }
    void setActive(bool value) { active.store(value, std::memory_order_release); }
    void setGCHandshakeEpoch(uint64_t epoch) {
        gcHandshakeEpoch.store(epoch, std::memory_order_release);
    }
    void setGCLastPollNanos(uint64_t nanos) {
        gcLastPollNanos.store(nanos, std::memory_order_release);
    }
    void setGCContextReady(bool ready) { gcContextReady.store(ready, std::memory_order_release); }
    void setGCAssistDebt(size_t debt) { gcAssistDebt = debt; }
    void setGCAssistDebtCycle(size_t cycle) { gcAssistDebtCycle = cycle; }

    void terminate();

    bool isQueued() const { return runList != NULL; }
    size_t getLiveIndex() const { return liveIndex; }
    void setLiveIndex(size_t index) { liveIndex = index; }
    size_t getGCRegistryIndex() const { return gcRegistryIndex; }
    void setGCRegistryIndex(size_t index) { gcRegistryIndex = index; }
    size_t getGCHandshakeRecordIndex() const { return gcHandshakeRecordIndex; }
    void setGCHandshakeRecordIndex(size_t index) { gcHandshakeRecordIndex = index; }

    uint64_t getHandle() const { return handle; }
    uint64_t ensureHandle();
    void releaseHandle();
    static YuTask* pinHandle(uint64_t handle);
    static bool addFinishHook(uint64_t handle, YuTaskStartFunc func, void* closure);
    YuTaskEnding* attachEnding();
    static YuTaskEnding* endingForHandle(uint64_t handle);
    void unpinHandle() { handlePins.fetch_sub(1, std::memory_order_release); }
    void pin() { handlePins.fetch_add(1, std::memory_order_relaxed); }
    bool isHandlePinned() const { return handlePins.load(std::memory_order_acquire) != 0; }

private:
    uint32_t uid;
    uint32_t parentId;
    const char* spawnFile;
    uint32_t spawnLine;
    uint32_t spawnColumn;
    uint32_t flags;
    uint32_t schedulingClass;
    YuTaskQueue* queue;
    YuContext context;
    YuTaskRuntimeContext runtimeContext;
    char* locals;
    size_t localsSize;
    std::atomic<bool> canceled;
    std::atomic<uint32_t> cancelShield;
    std::atomic<bool> shieldLocked;
    std::atomic<bool> terminating;
    bool terminationRaised;
    std::atomic<bool> active;
    std::atomic<uint32_t> runState;
    std::atomic<uint32_t> gcState;
    std::atomic<uint64_t> gcHandshakeEpoch;
    std::atomic<uint64_t> gcLastPollNanos;
    std::atomic<bool> gcContextReady;
    // Allocation assists are charged to the logical task so debt follows coroutine migration
    // between scheduler threads. Only the active task mutates these fields.
    size_t gcAssistDebt;
    size_t gcAssistDebtCycle;
    // These brackets belong to the task stack and therefore must follow migration between OS
    // threads. Only the active task mutates them; scheduler handshakes observe gcState/context.
    uint32_t gcBlockingDepth;
    YuTaskGCBlockingContext gcBlockingContext;
    std::vector<YuTaskGCBlockingContext> gcNestedBlockingContexts;
    std::vector<uint32_t> gcManagedCallbackKinds;
    std::vector<uint64_t> gcManagedCallbackRegisters;
    uint64_t handle;
    std::atomic<uint32_t> handlePins;
    std::vector<YuTaskFinishHook> finishHooks;
    YuTaskEnding* ending;
    YuTask* runPrev;
    YuTask* runNext;
    YuTaskRunList* runList;
    size_t liveIndex;
    size_t gcRegistryIndex;
    size_t gcHandshakeRecordIndex;

    friend class YuTaskRunList;
};

// *************************************************************************************************

class YuTaskRunList {
public:
    bool empty() const { return !head; }
    size_t size() const { return count; }

    void push(YuTask* task) {
        task->runList = this;
        task->runPrev = tail;
        task->runNext = NULL;
        if (tail) {
            tail->runNext = task;
        } else {
            head = task;
        }
        tail = task;
        ++count;
    }

    YuTask* pop() {
        YuTask* task = head;
        if (task) {
            remove(task);
        }
        return task;
    }

    void remove(YuTask* task) {
        if (task->runList != this) {
            return;
        }
        if (task->runPrev) {
            task->runPrev->runNext = task->runNext;
        } else {
            head = task->runNext;
        }
        if (task->runNext) {
            task->runNext->runPrev = task->runPrev;
        } else {
            tail = task->runPrev;
        }
        task->runPrev = NULL;
        task->runNext = NULL;
        task->runList = NULL;
        --count;
    }

private:
    YuTask* head = NULL;
    YuTask* tail = NULL;
    size_t count = 0;
};

// *************************************************************************************************

#endif
