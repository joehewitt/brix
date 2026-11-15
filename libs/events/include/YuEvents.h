
#ifndef YU_EVENTS_H
#define YU_EVENTS_H

#if defined(__APPLE__)
#define _XOPEN_SOURCE
#define _DARWIN_UNLIMITED_SELECT
#endif

#include <stdlib.h>
#include <stdint.h>
#include <YuEvents/YuGCHandshake.h>

class YuSemaphore;
class YuTaskMutex;
class YuSemaphore;
class YuThread;
class YuTask;
class YuTaskQueue;
struct YuTaskRuntimeContext;

struct YuPathWatchNativeEvent {
    const char* path;
    uint64_t flags;
};

typedef void (*YuTaskStartFunc)(void* closure);
typedef void (*YuThreadLocalFunc)();
typedef void (*YuTaskLocalFunc)();
typedef void (*YuMainTaskQueueHostRunFunc)(void* context);
typedef void (*YuMainTaskQueueHostWakeFunc)(void* context);

enum YuMainTaskDeadlineResult {
    YuMainTaskDeadlineFailed = 0,
    YuMainTaskDeadlineDeferred = 1,
    YuMainTaskDeadlineCompleted = 2
};

enum YuWaitResult {
    YuWaitReady = 0,
    YuWaitCanceled = 1,
    YuWaitTimedOut = 2
};

// *************************************************************************************************

YUMI_GC_LEAF YuThread* yumiGetActiveThread();
YUMI_GC_LEAF void yumiSetActiveThread(YuThread* thread);
YUMI_GC_LEAF size_t yumiGetThreadLocalSize();
YUMI_GC_LEAF YuThreadLocalFunc yumiGetThreadLocalInit();
YUMI_GC_LEAF YuThreadLocalFunc yumiGetThreadLocalFlush();

// *************************************************************************************************

extern "C" void yumiRunEventLoop(YuTaskStartFunc starter, YuTaskStartFunc closer, void* closure,
    size_t localSize, YuThreadLocalFunc threadLocalInit, YuThreadLocalFunc threadLocalFlush,
    size_t threadLocalSize, YuTaskLocalFunc taskLocalInit);
extern "C" YUMI_GC_LEAF bool yumiRequestProcessTermination(int exitCode);
extern "C" YUMI_GC_LEAF bool yumiProcessTerminationRequested();
extern "C" YUMI_GC_LEAF int yumiGetProcessExitCode();
extern "C" YUMI_GC_LEAF void yumiRequestTerminationOnSignals();
extern "C" YUMI_GC_POLLING bool yumiRunMainTaskQueueHost(
    YuMainTaskQueueHostRunFunc run,
    YuMainTaskQueueHostWakeFunc wake,
    void* context);
extern "C" YUMI_GC_LEAF bool yumiScheduleMainTaskFromNative(
    YuTaskStartFunc func, void* closure);
extern "C" void yumiScheduleMainTaskWhenTaskFinishes(
    uint64_t handle, YuTaskStartFunc callback, void* context);
extern "C" YUMI_GC_LEAF uint32_t yumiRunMainTaskFromNativeUntilDeadline(
    YuTaskStartFunc func, void* closure, uint64_t deadlineNanos);
extern "C" YUMI_GC_LEAF uint32_t yumiDrainMainTaskQueue(
    size_t taskBudget, uint64_t deadlineNanos);
extern "C" void yumiForkTask(YuTaskStartFunc func, void* closure, size_t localSize);
extern "C" void yumiForkMainTask(YuTaskStartFunc func, void* closure, size_t localSize);
extern "C" void yumiForkTaskWithInfo(YuTaskStartFunc func, void* closure, size_t localSize,
    const char* file, uint32_t line, uint32_t column);
extern "C" void yumiForkPriorityTask(YuTaskStartFunc func, void* closure, size_t localSize);
extern "C" void yumiForkPriorityTaskWithInfo(YuTaskStartFunc func, void* closure,
    size_t localSize, const char* file, uint32_t line, uint32_t column);
extern "C" void yumiSpinTask(YuTaskStartFunc func, void* closure, size_t localSize);
extern "C" void yumiSpinTaskWithInfo(YuTaskStartFunc func, void* closure, size_t localSize,
    const char* file, uint32_t line, uint32_t column);
extern "C" void yumiSpinPriorityTask(YuTaskStartFunc func, void* closure, size_t localSize);
extern "C" void yumiSpinPriorityTaskWithInfo(YuTaskStartFunc func, void* closure,
    size_t localSize, const char* file, uint32_t line, uint32_t column);
const uint32_t YU_TASK_START_PRIORITY = 1 << 0;
const uint32_t YU_TASK_START_SPIN = 1 << 1;
const uint32_t YU_TASK_START_LARGE_STACK = 1 << 2;
const uint32_t YU_TASK_START_HANDLE = 1 << 3;
extern "C" uint64_t yumiStartTask(YuTaskQueue* queue, YuTaskStartFunc func, void* closure,
    size_t localSize, const char* file, uint32_t line, uint32_t column, uint32_t options);
struct YuTaskEnding;
extern "C" void yumiTaskEndingAdopt(char* owner, YuTaskEnding* ending);
extern "C" uint64_t yumiTaskEndingHandle(YuTaskEnding* ending);
extern "C" uint32_t yumiTaskEndingEnd(YuTaskEnding* ending);
extern "C" bool yumiTaskEndingWait(YuTaskEnding* ending);
extern "C" bool yumiWaitForTask(uint64_t handle);
extern "C" YUMI_GC_LEAF void yumiSetActiveTaskEnd(uint32_t end);
extern "C" void yumiTerminateActiveTask();
extern "C" YUMI_GC_LEAF YuTask* yumiGetActiveTask();
extern "C" YUMI_GC_LEAF YuTask* yumiTryGetActiveTask();
extern "C" YUMI_GC_LEAF YuTaskRuntimeContext* yumiGetActiveTaskRuntimeContext();
extern "C" YUMI_GC_LEAF void yumiSetActiveTaskRuntimeContext(
    YuTaskRuntimeContext* context);
extern "C" YUMI_GC_LEAF uint32_t yumiGetActiveTaskId();
extern "C" YUMI_GC_LEAF uint64_t yumiGetActiveTaskHandle();
extern "C" YUMI_GC_LEAF uint32_t yumiGetTaskHandleId(uint64_t handle);
extern "C" YUMI_GC_LEAF uint32_t yumiGetTaskId(YuTask* task);
extern "C" YUMI_GC_LEAF uint32_t yumiGetTaskParentId(YuTask* task);
extern "C" YUMI_GC_LEAF const char* yumiGetTaskSpawnFile(YuTask* task);
extern "C" YUMI_GC_LEAF uint32_t yumiGetTaskSpawnLine(YuTask* task);
extern "C" YUMI_GC_LEAF uint32_t yumiGetTaskSpawnColumn(YuTask* task);
extern "C" YUMI_GC_LEAF uint32_t yumiGetTaskFlags(YuTask* task);
extern "C" YUMI_GC_LEAF void* yumiGetTaskStackBase(YuTask* task);
extern "C" YUMI_GC_LEAF size_t yumiGetTaskStackSize(YuTask* task);
extern "C" YUMI_GC_LEAF bool yumiGetTaskFinished(uint64_t handle);
extern "C" YUMI_GC_LEAF bool yumiGetTaskCanceled(uint64_t handle);
extern "C" YUMI_GC_LEAF bool yumiGetTaskTerminating(uint64_t handle);
extern "C" YUMI_GC_LEAF bool yumiActiveTaskIsTerminating();
extern "C" YUMI_GC_LEAF bool yumiTakeActiveTaskTermination();
extern "C" YUMI_GC_LEAF char* yumiGetActiveTaskMemory();
extern "C" YUMI_GC_LEAF char* yumiGetActiveThreadMemory();
extern "C" YUMI_GC_LEAF int yumiActiveTaskQueueRemaining();
extern "C" YUMI_GC_LEAF int yumiActiveTaskQueueActivity();
extern "C" YUMI_GC_LEAF int yumiGCBackgroundTaskQueueThreadCount();
extern "C" YUMI_GC_LEAF YuTaskQueue* yumiGCBackgroundTaskQueue();
extern "C" YUMI_GC_LEAF size_t yumiGCActiveTaskAssistDebt();
extern "C" YUMI_GC_LEAF size_t yumiGCActiveTaskAssistDebtCycle();
extern "C" YUMI_GC_LEAF void yumiGCSetActiveTaskAssistDebt(size_t debt);
extern "C" YUMI_GC_LEAF void yumiGCSetActiveTaskAssistDebtCycle(size_t cycle);
extern "C" YUMI_GC_POLLING bool yumiSuspendTask(uint64_t handle);
extern "C" bool yumiResumeTaskNow(uint64_t handle);
extern "C" void yumiResumeTaskLater(uint64_t handle);
extern "C" void yumiCancelTask(uint64_t handle);

extern "C" YUMI_GC_POLLING uint32_t yumiSuspendActiveTaskUntilReadable(
    int fd, uint64_t deadlineNs);
extern "C" YUMI_GC_POLLING uint32_t yumiSuspendActiveTaskUntilWritable(
    int fd, uint64_t deadlineNs);
extern "C" YUMI_GC_POLLING bool yumiSuspendActiveTaskUntilProcExit(
    int pid, YuSemaphore* sem);
extern "C" YUMI_GC_POLLING bool yumiSuspendActiveTaskUntilFileChange(
    int fd, int type, YuSemaphore* sem);
extern "C" YUMI_GC_POLLING bool yumiSuspendActiveTaskUntilSignal(int sig, YuSemaphore* sem);
extern "C" YUMI_GC_POLLING bool yumiSuspendActiveTaskUntilTime(long int us, YuSemaphore* sem);
extern "C" uint64_t yumiMonotonicTimeNanoseconds();
extern "C" uint64_t yumiNextScheduledWorkDeadlineNanoseconds();
extern "C" YUMI_GC_POLLING bool yumiSuspendActiveTaskUntilAbsoluteTime(
    uint64_t deadlineNs, YuSemaphore* sem);
extern "C" void yumiReleaseEventDescriptor(int fd);
extern "C" void yumiCancelProcExitEventListener(int pid, YuSemaphore* sem);
extern "C" void yumiCancelFileChangeEventListener(int fd, int type, YuSemaphore* sem);
extern "C" void yumiCancelSignalEventListener(int sig, YuSemaphore* sem);
extern "C" void yumiCancelTimerEventListener(YuSemaphore* sem);

extern "C" void* yumiPathWatchCreate(const char* path, bool isRecursive);
extern "C" YUMI_GC_POLLING bool yumiSuspendActiveTaskUntilPathWatchEvent(
    void* watcher, YuSemaphore* sem);
extern "C" bool yumiPathWatchNextEvent(void* watcher, YuPathWatchNativeEvent* out);
extern "C" void yumiCancelPathWatchEventListener(void* watcher, YuSemaphore* sem);
extern "C" void yumiPathWatchRelease(void* watcher);

extern "C" YUMI_GC_LEAF int64_t yumiMainBundleString(const char* key, char* buffer, int64_t capacity);

extern "C" YUMI_GC_LEAF int yumiSharedMemoryCreate(const char* name, int64_t size);
extern "C" YUMI_GC_LEAF int yumiSharedMemoryOpen(const char* name);
extern "C" YUMI_GC_LEAF bool yumiSharedMemoryUnlink(const char* name);
extern "C" YUMI_GC_LEAF void* yumiSharedMemoryMap(int fd, int64_t size, bool writable);
extern "C" YUMI_GC_LEAF void yumiSharedMemoryUnmap(void* memory, int64_t size);
extern "C" YUMI_GC_LEAF bool yumiSocketPair(int* fds);
extern "C" YUMI_GC_LEAF uint32_t yumiMachRendezvousCheckIn(const char* name);
extern "C" YUMI_GC_LEAF uint32_t yumiMachRendezvousLookUp(const char* name);
extern "C" YUMI_GC_LEAF void yumiMachRendezvousClose(uint32_t port);
extern "C" YUMI_GC_LEAF void yumiMachPortRelease(uint32_t port);
extern "C" YUMI_GC_LEAF bool yumiMachSendParcel(
    uint32_t destination, const uint32_t* ports, int32_t portCount, const void* bytes, int64_t size);
extern "C" YUMI_GC_LEAF bool yumiMachReceiveParcel(
    uint32_t port, uint32_t* ports, int32_t portCapacity, void* bytes, int64_t byteCapacity, int32_t* counts);

extern "C" YuSemaphore* yumiNewSemaphore();
extern "C" YUMI_GC_POLLING bool yumiWaitForSemaphore(
    YuSemaphore* sem, void* buf, size_t capacity, YuTaskMutex* mutex);
extern "C" YUMI_GC_POLLING void yumiPostSemaphore(
    YuSemaphore* sem, void* data, size_t sizeOfData);

extern "C" YuTaskMutex* yumiNewMutex();
extern "C" YUMI_GC_POLLING void yumiLockMutex(YuTaskMutex* mutex);
extern "C" void yumiUnlockMutex(YuTaskMutex* mutex);

extern "C" YuTaskQueue* yumiNewWorker(int threadCount);
extern "C" YuTaskQueue* yumiActiveTaskQueue();
extern "C" YUMI_GC_LEAF int yumiUsableCoreCount();
extern "C" YUMI_GC_POLLING int yumiClaimLazyGlobal(uint8_t* state);
extern "C" void yumiReleaseLazyGlobal(uint8_t* state);
extern "C" void yumiAbandonLazyGlobal(uint8_t* state);
extern "C" bool yumiForkTaskInWorker(YuTaskQueue* queue, YuTaskStartFunc func, void* closure,
    size_t localSize);
extern "C" bool yumiForkTaskInWorkerWithInfo(YuTaskQueue* queue, YuTaskStartFunc func,
    void* closure, size_t localSize, const char* file, uint32_t line, uint32_t column);
extern "C" bool yumiForkPriorityTaskInWorker(YuTaskQueue* queue, YuTaskStartFunc func,
    void* closure, size_t localSize);
extern "C" bool yumiForkPriorityTaskInWorkerWithInfo(YuTaskQueue* queue, YuTaskStartFunc func,
    void* closure, size_t localSize, const char* file, uint32_t line, uint32_t column);
extern "C" bool yumiForkGCMarkTaskInWorker(
    YuTaskQueue* queue, YuTaskStartFunc func, void* closure);
extern "C" bool yumiForkGCCleanupTask(
    YuTaskStartFunc func, void* closure);
extern "C" bool yumiWorkerGetStatus(YuTaskQueue* queue);
extern "C" void yumiWorkerStop(YuTaskQueue* queue);

#endif
