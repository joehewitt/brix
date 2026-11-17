
#ifndef YU_THREAD_H
#define YU_THREAD_H

#include <YuEvents/YuEvents.h>
#include <YuEvents/YuContext.h>
#include <YuEvents/YuTask.h>
#include <YuGC/YuGCAllocationBuffer.h>

#include <atomic>
#include <vector>

class YuTaskQueue;
class YuMutex;
struct YuTaskEnding;
struct YuTaskWorker;

const uint32_t YU_THREAD_INTERRUPT_PREEMPT = 1 << 0;
const uint32_t YU_THREAD_INTERRUPT_WARN = 1 << 1;

// *************************************************************************************************

class YuThread {
public:
    YuThread(YuTaskQueue* queue);
    ~YuThread();

    YuTaskQueue* getTaskQueue() { return queue; }
    YuTaskWorker* getWorker() { return worker; }
    void setWorker(YuTaskWorker* aWorker) { worker = aWorker; }
    YuContext* getContext() { return &context; }
    YuTask* getActiveTask() { return activeTask; }
    char* getThreadLocals() { return threadLocals; }
    size_t getThreadLocalsSize() { return threadLocalsSize; }
    YumiGCRuntimeAllocationBuffer* getGCAllocationBufferStorage() {
        return &gcThreadCache.activeBuffer;
    }
    YumiGCThreadCache* getGCThreadCache() { return &gcThreadCache; }

    void initializeThreadLocals();
    void flushThreadLocals(bool bracketTasklessCallback = true);
    void flushThreadLocalCallback(bool bracketTasklessCallback = true);
    bool hasThreadLocalFlush() const;
    void startTask(
        YuTaskStartFunc func,
        void* closure,
        size_t localSize,
        const YuTaskProvenance& provenance = YuTaskProvenance(),
        YuTaskEnding** ending = nullptr,
        uint64_t* handle = nullptr);
    void suspendTask(
        YuTask* task,
        YuMutex* mutex,
        uint32_t gcState = YuTaskGCStateParked,
        bool scheduleAfterSuspend = false);
    void yieldTask(YuTask* task);
    void resumeTask(YuTask* task);
    void runTask(YuTask* task);

    void taskTerminated(YuTask* task);
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
    YuTaskQueue* queue;
    YuTaskWorker* worker;
    YuContext context;
    YuTask* activeTask;
    YuTask* suspendedTask;
    YuTask* terminatedTask;
    std::vector<YuTask*> graveyard;
    YuMutex* suspendedMutex;
    uint32_t suspendedGCState;
    bool scheduleSuspendedTask;
    bool yieldSuspendedTask;
    std::atomic<uint32_t> switchCount;
    std::atomic<uint32_t> interruptRequests;
    uint64_t allocationControl;
    char* threadLocals;
    size_t threadLocalsSize;
    bool threadLocalsInitialized;
    YumiGCThreadCache gcThreadCache;
    void* signalStack;
};

// *************************************************************************************************

#endif
