
#ifndef BX_THREAD_H
#define BX_THREAD_H

#include <BxEvents/BxEvents.h>
#include <BxEvents/BxContext.h>
#include <BxEvents/BxTask.h>
#include <BxGC/BxGCAllocationBuffer.h>

#include <atomic>
#include <vector>

class BxTaskQueue;
class BxMutex;
struct BxTaskEnding;
struct BxTaskWorker;

const uint32_t BX_THREAD_INTERRUPT_PREEMPT = 1 << 0;
const uint32_t BX_THREAD_INTERRUPT_WARN = 1 << 1;

// *************************************************************************************************

class BxThread {
public:
    BxThread(BxTaskQueue* queue);
    ~BxThread();

    BxTaskQueue* getTaskQueue() { return queue; }
    BxTaskWorker* getWorker() { return worker; }
    void setWorker(BxTaskWorker* aWorker) { worker = aWorker; }
    BxContext* getContext() { return &context; }
    BxTask* getActiveTask() { return activeTask; }
    char* getThreadLocals() { return threadLocals; }
    size_t getThreadLocalsSize() { return threadLocalsSize; }
    BrixGCRuntimeAllocationBuffer* getGCAllocationBufferStorage() {
        return &gcThreadCache.activeBuffer;
    }
    BrixGCThreadCache* getGCThreadCache() { return &gcThreadCache; }

    void initializeThreadLocals();
    void flushThreadLocals(bool bracketTasklessCallback = true);
    void flushThreadLocalCallback(bool bracketTasklessCallback = true);
    bool hasThreadLocalFlush() const;
    void startTask(
        BxTaskStartFunc func,
        void* closure,
        size_t localSize,
        const BxTaskProvenance& provenance = BxTaskProvenance(),
        BxTaskEnding** ending = nullptr,
        uint64_t* handle = nullptr);
    void suspendTask(
        BxTask* task,
        BxMutex* mutex,
        uint32_t gcState = BxTaskGCStateParked,
        bool scheduleAfterSuspend = false);
    void yieldTask(BxTask* task);
    void resumeTask(BxTask* task);
    void runTask(BxTask* task);

    void taskTerminated(BxTask* task);
    void deleteTerminatedTasks();

    uint32_t getSwitchCount() const { return switchCount.load(std::memory_order_seq_cst); }
    uint32_t getInterruptRequests() const {
        return interruptRequests.load(std::memory_order_acquire);
    }
    bool requestInterrupt(uint32_t kind);
    uint32_t takeInterruptRequests(uint32_t kinds);

    uint32_t monitorSwitchCount;
    uint64_t monitorSinceNanos;
    bool monitorWarned;

private:
    void finishSuspendedTask();
    void finishTerminatedTask();
    void finishRun(uint64_t startNanos);

private:
    BxTaskQueue* queue;
    BxTaskWorker* worker;
    BxContext context;
    BxTask* activeTask;
    BxTask* suspendedTask;
    BxTask* terminatedTask;
    std::vector<BxTask*> graveyard;
    BxMutex* suspendedMutex;
    uint32_t suspendedGCState;
    bool scheduleSuspendedTask;
    bool yieldSuspendedTask;
    std::atomic<uint32_t> switchCount;
    std::atomic<uint32_t> interruptRequests;
    uint64_t allocationControl;
    char* threadLocals;
    size_t threadLocalsSize;
    bool threadLocalsInitialized;
    BrixGCThreadCache gcThreadCache;
    void* signalStack;
};

// *************************************************************************************************

#endif
