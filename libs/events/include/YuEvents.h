
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

YUMI_WITHOUT_GC YuThread* yumiGetActiveThread();
YUMI_WITHOUT_GC void yumiSetActiveThread(YuThread* thread);
YUMI_WITHOUT_GC size_t yumiGetThreadLocalSize();
YUMI_WITHOUT_GC YuThreadLocalFunc yumiGetThreadLocalInit();
YUMI_WITHOUT_GC YuThreadLocalFunc yumiGetThreadLocalFlush();

// *************************************************************************************************

extern "C" void yumiRunEventLoop(YuTaskStartFunc starter, YuTaskStartFunc closer, void* closure,
    size_t localSize, YuThreadLocalFunc threadLocalInit, YuThreadLocalFunc threadLocalFlush,
    size_t threadLocalSize, YuTaskLocalFunc taskLocalInit);
extern "C" YUMI_WITHOUT_GC bool yumiRequestProcessTermination(int exitCode);
extern "C" YUMI_WITHOUT_GC bool yumiProcessTerminationRequested();
extern "C" YUMI_WITHOUT_GC int yumiGetProcessExitCode();
extern "C" YUMI_WITHOUT_GC void yumiRequestTerminationOnSignals();
extern "C" YUMI_WITHOUT_BLOCKING bool yumiRunMainTaskQueueHost(
    YuMainTaskQueueHostRunFunc run,
    YuMainTaskQueueHostWakeFunc wake,
    void* context);
extern "C" YUMI_WITHOUT_GC bool yumiScheduleMainTaskFromNative(
    YuTaskStartFunc func, void* closure);
extern "C" void yumiScheduleMainTaskWhenTaskFinishes(
    uint64_t handle, YuTaskStartFunc callback, void* context);
extern "C" YUMI_WITHOUT_GC uint32_t yumiRunMainTaskFromNativeUntilDeadline(
    YuTaskStartFunc func, void* closure, uint64_t deadlineNanos);
extern "C" YUMI_WITHOUT_GC uint32_t yumiDrainMainTaskQueue(
    size_t taskBudget, uint64_t deadlineNanos);
extern "C" void yumiForkTask(YuTaskStartFunc func, void* closure, size_t localSize);
extern "C" void yumiForkMainTask(YuTaskStartFunc func, void* closure, size_t localSize);
const uint32_t YU_TASK_START_PRIORITY = 1 << 0;
const uint32_t YU_TASK_START_SPIN = 1 << 1;
const uint32_t YU_TASK_START_LARGE_STACK = 1 << 2;
const uint32_t YU_TASK_START_HANDLE = 1 << 3;
extern "C" uint64_t yumiStartTask(YuTaskQueue* queue, YuTaskStartFunc func, void* closure,
    size_t localSize, const char* file, uint32_t line, uint32_t column, uint32_t options);
extern "C" uint64_t yumiStartOwnedTask(char* owner, YuTaskQueue* queue, YuTaskStartFunc func,
    void* closure, size_t localSize, const char* file, uint32_t line, uint32_t column,
    uint32_t options);
extern "C" void yumiCancelOwnedTasks(char* owner);
extern "C" void yumiCancelOwnedTasksTo(char* owner, char* heir);
struct YuTaskEnding;
extern "C" void yumiTaskEndingAdopt(char* owner, YuTaskEnding* ending);
extern "C" uint64_t yumiTaskEndingHandle(YuTaskEnding* ending);
extern "C" uint32_t yumiTaskEndingEnd(YuTaskEnding* ending);
extern "C" bool yumiTaskEndingWait(YuTaskEnding* ending);
extern "C" bool yumiWaitForTask(uint64_t handle);
extern "C" YUMI_WITHOUT_GC void yumiSetActiveTaskEnd(uint32_t end);
extern "C" void yumiTerminateActiveTask();
extern "C" YUMI_WITHOUT_GC YuTask* yumiGetActiveTask();
extern "C" YUMI_WITHOUT_GC YuTask* yumiTryGetActiveTask();
extern "C" YUMI_WITHOUT_GC YuTaskRuntimeContext* yumiGetActiveTaskRuntimeContext();
extern "C" YUMI_WITHOUT_GC void yumiSetActiveTaskRuntimeContext(
    YuTaskRuntimeContext* context);
extern "C" YUMI_WITHOUT_GC uint32_t yumiGetActiveTaskId();
extern "C" YUMI_WITHOUT_GC uint64_t yumiGetActiveTaskHandle();
extern "C" YUMI_WITHOUT_GC uint32_t yumiGetTaskHandleId(uint64_t handle);
extern "C" YUMI_WITHOUT_GC uint32_t yumiGetTaskId(YuTask* task);
extern "C" YUMI_WITHOUT_GC uint32_t yumiGetTaskParentId(YuTask* task);
extern "C" YUMI_WITHOUT_GC const char* yumiGetTaskSpawnFile(YuTask* task);
extern "C" YUMI_WITHOUT_GC uint32_t yumiGetTaskSpawnLine(YuTask* task);
extern "C" YUMI_WITHOUT_GC uint32_t yumiGetTaskSpawnColumn(YuTask* task);
extern "C" YUMI_WITHOUT_GC uint32_t yumiGetTaskFlags(YuTask* task);
extern "C" YUMI_WITHOUT_GC void* yumiGetTaskStackBase(YuTask* task);
extern "C" YUMI_WITHOUT_GC size_t yumiGetTaskStackSize(YuTask* task);
extern "C" YUMI_WITHOUT_GC bool yumiGetTaskFinished(uint64_t handle);
extern "C" YUMI_WITHOUT_GC bool yumiGetTaskCanceled(uint64_t handle);
extern "C" YUMI_WITHOUT_GC bool yumiGetTaskCancelInForce(uint64_t handle);
extern "C" YUMI_WITHOUT_GC bool yumiGetTaskTerminating(uint64_t handle);
extern "C" YUMI_WITHOUT_GC bool yumiActiveTaskIsTerminating();
extern "C" YUMI_WITHOUT_GC bool yumiTakeActiveTaskTermination();
extern "C" YUMI_WITHOUT_GC char* yumiGetActiveTaskMemory();
extern "C" YUMI_WITHOUT_GC char* yumiGetActiveThreadMemory();
extern "C" YUMI_WITHOUT_GC int yumiActiveTaskQueueRemaining();
extern "C" YUMI_WITHOUT_GC int yumiActiveTaskQueueActivity();
extern "C" YUMI_WITHOUT_GC int yumiGCBackgroundTaskQueueThreadCount();
extern "C" YUMI_WITHOUT_GC YuTaskQueue* yumiGCBackgroundTaskQueue();
extern "C" YUMI_WITHOUT_GC size_t yumiGCActiveTaskAssistDebt();
extern "C" YUMI_WITHOUT_GC size_t yumiGCActiveTaskAssistDebtCycle();
extern "C" YUMI_WITHOUT_GC void yumiGCSetActiveTaskAssistDebt(size_t debt);
extern "C" YUMI_WITHOUT_GC void yumiGCSetActiveTaskAssistDebtCycle(size_t cycle);
extern "C" YUMI_WITHOUT_BLOCKING bool yumiSuspendTask(uint64_t handle);
extern "C" bool yumiResumeTaskNow(uint64_t handle);
extern "C" void yumiResumeTaskLater(uint64_t handle);
extern "C" void yumiCancelTask(uint64_t handle);
extern "C" void yumiCancelTaskLater(uint64_t handle);
extern "C" YUMI_WITHOUT_GC void yumiEnterCancelShield();
extern "C" YUMI_WITHOUT_GC void yumiLeaveCancelShield();

extern "C" YUMI_WITHOUT_BLOCKING uint32_t yumiSuspendActiveTaskUntilReadable(
    int fd, uint64_t deadlineNs);
extern "C" YUMI_WITHOUT_BLOCKING uint32_t yumiSuspendActiveTaskUntilWritable(
    int fd, uint64_t deadlineNs);
extern "C" YUMI_WITHOUT_BLOCKING bool yumiSuspendActiveTaskUntilProcExit(
    int pid, YuSemaphore* sem);
extern "C" YUMI_WITHOUT_BLOCKING bool yumiSuspendActiveTaskUntilFileChange(
    int fd, int type, YuSemaphore* sem);
extern "C" YUMI_WITHOUT_BLOCKING bool yumiSuspendActiveTaskUntilSignal(int sig, YuSemaphore* sem);
extern "C" YUMI_WITHOUT_BLOCKING bool yumiSuspendActiveTaskUntilTime(long int us, YuSemaphore* sem);
extern "C" uint64_t yumiMonotonicTimeNanoseconds();
extern "C" uint64_t yumiNextScheduledWorkDeadlineNanoseconds();
extern "C" YUMI_WITHOUT_BLOCKING bool yumiSuspendActiveTaskUntilAbsoluteTime(
    uint64_t deadlineNs, YuSemaphore* sem);
extern "C" void yumiReleaseEventDescriptor(int fd);
extern "C" void yumiCancelProcExitEventListener(int pid, YuSemaphore* sem);
extern "C" void yumiCancelFileChangeEventListener(int fd, int type, YuSemaphore* sem);
extern "C" void yumiCancelSignalEventListener(int sig, YuSemaphore* sem);
extern "C" void yumiCancelTimerEventListener(YuSemaphore* sem);

extern "C" void* yumiPathWatchCreate(const char* path, bool isRecursive);
extern "C" YUMI_WITHOUT_BLOCKING bool yumiSuspendActiveTaskUntilPathWatchEvent(
    void* watcher, YuSemaphore* sem);
extern "C" bool yumiPathWatchNextEvent(void* watcher, YuPathWatchNativeEvent* out);
extern "C" void yumiCancelPathWatchEventListener(void* watcher, YuSemaphore* sem);
extern "C" void yumiPathWatchRelease(void* watcher);

extern "C" YUMI_WITHOUT_GC int64_t yumiMainBundleString(const char* key, char* buffer, int64_t capacity);

extern "C" YUMI_WITHOUT_GC int yumiSharedMemoryCreate(const char* name, int64_t size);
extern "C" YUMI_WITHOUT_GC int yumiSharedMemoryOpen(const char* name);
extern "C" YUMI_WITHOUT_GC bool yumiSharedMemoryUnlink(const char* name);
extern "C" YUMI_WITHOUT_GC void* yumiSharedMemoryMap(int fd, int64_t size, bool writable);
extern "C" YUMI_WITHOUT_GC void yumiSharedMemoryUnmap(void* memory, int64_t size);
extern "C" YUMI_WITHOUT_GC bool yumiSocketPair(int* fds);
extern "C" YUMI_WITHOUT_GC uint32_t yumiMachRendezvousCheckIn(const char* name);
extern "C" YUMI_WITHOUT_GC uint32_t yumiMachRendezvousLookUp(const char* name);
extern "C" YUMI_WITHOUT_GC void yumiMachRendezvousClose(uint32_t port);
extern "C" YUMI_WITHOUT_GC void yumiMachPortRelease(uint32_t port);
extern "C" YUMI_WITHOUT_GC bool yumiMachSendParcel(
    uint32_t destination, const uint32_t* ports, int32_t portCount, const void* bytes, int64_t size);
extern "C" YUMI_WITHOUT_GC bool yumiMachReceiveParcel(
    uint32_t port, uint32_t* ports, int32_t portCapacity, void* bytes, int64_t byteCapacity, int32_t* counts);

extern "C" YuSemaphore* yumiNewSemaphore();
extern "C" YUMI_WITHOUT_BLOCKING bool yumiWaitForSemaphore(
    YuSemaphore* sem, void* buf, size_t capacity, YuTaskMutex* mutex);
extern "C" YUMI_WITHOUT_BLOCKING void yumiPostSemaphore(
    YuSemaphore* sem, void* data, size_t sizeOfData);

extern "C" YuTaskMutex* yumiNewMutex();
extern "C" YUMI_WITHOUT_BLOCKING void yumiLockMutex(YuTaskMutex* mutex);
extern "C" void yumiUnlockMutex(YuTaskMutex* mutex);

extern "C" YuTaskQueue* yumiNewWorker(int threadCount);
extern "C" YuTaskQueue* yumiActiveTaskQueue();
extern "C" YUMI_WITHOUT_GC int yumiUsableCoreCount();
extern "C" YUMI_WITHOUT_BLOCKING int yumiClaimLazyGlobal(uint8_t* state);
extern "C" void yumiReleaseLazyGlobal(uint8_t* state);
extern "C" void yumiAbandonLazyGlobal(uint8_t* state);
extern "C" bool yumiForkGCMarkTaskInWorker(
    YuTaskQueue* queue, YuTaskStartFunc func, void* closure);
extern "C" bool yumiForkGCCleanupTask(
    YuTaskStartFunc func, void* closure);
extern "C" bool yumiWorkerGetStatus(YuTaskQueue* queue);
extern "C" void yumiWorkerStop(YuTaskQueue* queue);

#endif
