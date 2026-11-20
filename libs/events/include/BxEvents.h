
#ifndef BX_EVENTS_H
#define BX_EVENTS_H

#if defined(__APPLE__)
#define _XOPEN_SOURCE
#define _DARWIN_UNLIMITED_SELECT
#endif

#include <stdlib.h>
#include <stdint.h>
#include <BxEvents/BxGCHandshake.h>

class BxSemaphore;
class BxTaskMutex;
class BxSemaphore;
class BxThread;
class BxTask;
class BxTaskQueue;
struct BxTaskRuntimeContext;

struct BxPathWatchNativeEvent {
    const char* path;
    uint64_t flags;
};

typedef void (*BxTaskStartFunc)(void* closure);
typedef void (*BxThreadLocalFunc)();
typedef void (*BxTaskLocalFunc)();
typedef void (*BxMainTaskQueueHostRunFunc)(void* context);
typedef void (*BxMainTaskQueueHostWakeFunc)(void* context);

enum BxMainTaskDeadlineResult {
    BxMainTaskDeadlineFailed = 0,
    BxMainTaskDeadlineDeferred = 1,
    BxMainTaskDeadlineCompleted = 2
};

enum BxWaitResult {
    BxWaitReady = 0,
    BxWaitCanceled = 1,
    BxWaitTimedOut = 2
};

enum BxIdleWaitResult {
    BxIdleWaitIdle = 0,
    BxIdleWaitCanceled = 1,
    BxIdleWaitUnsupported = 2
};

// *************************************************************************************************

BRIX_WITHOUT_GC BxThread* brixGetActiveThread();
BRIX_WITHOUT_GC void brixSetActiveThread(BxThread* thread);
BRIX_WITHOUT_GC size_t brixGetThreadLocalSize();
BRIX_WITHOUT_GC BxThreadLocalFunc brixGetThreadLocalInit();
BRIX_WITHOUT_GC BxThreadLocalFunc brixGetThreadLocalFlush();

// *************************************************************************************************

extern "C" void brixRunEventLoop(BxTaskStartFunc starter, BxTaskStartFunc closer, void* closure,
    size_t localSize, BxThreadLocalFunc threadLocalInit, BxThreadLocalFunc threadLocalFlush,
    size_t threadLocalSize, BxTaskLocalFunc taskLocalInit);
extern "C" BRIX_WITHOUT_GC bool brixRequestProcessTermination(int exitCode);
extern "C" BRIX_WITHOUT_GC bool brixProcessTerminationRequested();
extern "C" BRIX_WITHOUT_GC int brixGetProcessExitCode();
extern "C" BRIX_WITHOUT_GC void brixRequestTerminationOnSignals();
extern "C" BRIX_WITHOUT_BLOCKING bool brixRunMainTaskQueueHost(
    BxMainTaskQueueHostRunFunc run,
    BxMainTaskQueueHostWakeFunc wake,
    void* context);
extern "C" BRIX_WITHOUT_GC bool brixScheduleMainTaskFromNative(
    BxTaskStartFunc func, void* closure);
extern "C" void brixScheduleMainTaskWhenTaskFinishes(
    uint64_t handle, BxTaskStartFunc callback, void* context);
extern "C" BRIX_WITHOUT_GC uint32_t brixRunMainTaskFromNativeUntilDeadline(
    BxTaskStartFunc func, void* closure, uint64_t deadlineNanos);
extern "C" BRIX_WITHOUT_GC uint32_t brixDrainMainTaskQueue(
    size_t taskBudget, uint64_t deadlineNanos);
extern "C" void brixForkTask(BxTaskStartFunc func, void* closure, size_t localSize);
extern "C" void brixForkMainTask(BxTaskStartFunc func, void* closure, size_t localSize);
const uint32_t BX_TASK_START_PRIORITY = 1 << 0;
const uint32_t BX_TASK_START_SPIN = 1 << 1;
const uint32_t BX_TASK_START_LARGE_STACK = 1 << 2;
const uint32_t BX_TASK_START_HANDLE = 1 << 3;
extern "C" uint64_t brixStartTask(char* worker, BxTaskStartFunc func, void* closure,
    size_t localSize, const char* file, uint32_t line, uint32_t column, uint32_t options);
extern "C" uint64_t brixStartOwnedTask(char* owner, char* worker, BxTaskStartFunc func,
    void* closure, size_t localSize, const char* file, uint32_t line, uint32_t column,
    uint32_t options);
extern "C" void brixCancelOwnedTasks(char* owner);
extern "C" void brixCancelOwnedTasksTo(char* owner, char* heir);
struct BxTaskEnding;
extern "C" void brixTaskEndingAdopt(char* owner, BxTaskEnding* ending);
extern "C" uint64_t brixTaskEndingHandle(BxTaskEnding* ending);
extern "C" uint32_t brixTaskEndingEnd(BxTaskEnding* ending);
extern "C" bool brixTaskEndingWait(BxTaskEnding* ending);
extern "C" bool brixWaitForTask(uint64_t handle);
extern "C" BRIX_WITHOUT_GC void brixSetActiveTaskEnd(uint32_t end);
extern "C" void brixTerminateActiveTask();
extern "C" BRIX_WITHOUT_GC BxTask* brixGetActiveTask();
extern "C" BRIX_WITHOUT_GC BxTask* brixTryGetActiveTask();
extern "C" BRIX_WITHOUT_GC BxTaskRuntimeContext* brixGetActiveTaskRuntimeContext();
extern "C" BRIX_WITHOUT_GC void brixSetActiveTaskRuntimeContext(
    BxTaskRuntimeContext* context);
extern "C" BRIX_WITHOUT_GC uint32_t brixGetActiveTaskId();
extern "C" BRIX_WITHOUT_GC uint64_t brixGetActiveTaskHandle();
extern "C" BRIX_WITHOUT_GC uint32_t brixGetTaskHandleId(uint64_t handle);
extern "C" BRIX_WITHOUT_GC uint32_t brixGetTaskId(BxTask* task);
extern "C" BRIX_WITHOUT_GC uint32_t brixGetTaskParentId(BxTask* task);
extern "C" BRIX_WITHOUT_GC const char* brixGetTaskSpawnFile(BxTask* task);
extern "C" BRIX_WITHOUT_GC uint32_t brixGetTaskSpawnLine(BxTask* task);
extern "C" BRIX_WITHOUT_GC uint32_t brixGetTaskSpawnColumn(BxTask* task);
extern "C" BRIX_WITHOUT_GC uint32_t brixGetTaskFlags(BxTask* task);
extern "C" BRIX_WITHOUT_GC void* brixGetTaskStackBase(BxTask* task);
extern "C" BRIX_WITHOUT_GC size_t brixGetTaskStackSize(BxTask* task);
extern "C" BRIX_WITHOUT_GC bool brixGetTaskFinished(uint64_t handle);
extern "C" BRIX_WITHOUT_GC bool brixGetTaskCanceled(uint64_t handle);
extern "C" BRIX_WITHOUT_GC bool brixGetTaskCancelInForce(uint64_t handle);
extern "C" BRIX_WITHOUT_GC bool brixGetTaskTerminating(uint64_t handle);
extern "C" BRIX_WITHOUT_GC bool brixActiveTaskIsTerminating();
extern "C" BRIX_WITHOUT_GC bool brixTakeActiveTaskTermination();
extern "C" BRIX_WITHOUT_GC char* brixGetActiveTaskMemory();
extern "C" BRIX_WITHOUT_GC char* brixGetActiveThreadMemory();
extern "C" BRIX_WITHOUT_GC int brixActiveTaskQueueRemaining();
extern "C" BRIX_WITHOUT_GC int brixActiveTaskQueueActivity();
extern "C" BRIX_WITHOUT_GC int brixGCBackgroundTaskQueueThreadCount();
extern "C" BRIX_WITHOUT_GC BxTaskQueue* brixGCBackgroundTaskQueue();
extern "C" BRIX_WITHOUT_GC size_t brixGCActiveTaskAssistDebt();
extern "C" BRIX_WITHOUT_GC size_t brixGCActiveTaskAssistDebtCycle();
extern "C" BRIX_WITHOUT_GC void brixGCSetActiveTaskAssistDebt(size_t debt);
extern "C" BRIX_WITHOUT_GC void brixGCSetActiveTaskAssistDebtCycle(size_t cycle);
extern "C" BRIX_WITHOUT_BLOCKING bool brixSuspendTask(uint64_t handle);
extern "C" bool brixResumeTaskNow(uint64_t handle);
extern "C" void brixResumeTaskLater(uint64_t handle);
extern "C" void brixCancelTask(uint64_t handle);
extern "C" void brixCancelTaskLater(uint64_t handle);
extern "C" BRIX_WITHOUT_GC void brixEnterCancelShield();
extern "C" BRIX_WITHOUT_GC void brixLeaveCancelShield();

extern "C" BRIX_WITHOUT_BLOCKING uint32_t brixSuspendActiveTaskUntilReadable(
    int fd, uint64_t deadlineNs);
extern "C" BRIX_WITHOUT_BLOCKING uint32_t brixSuspendActiveTaskUntilWritable(
    int fd, uint64_t deadlineNs);
extern "C" BRIX_WITHOUT_BLOCKING bool brixSuspendActiveTaskUntilProcExit(
    int pid, BxSemaphore* sem);
extern "C" BRIX_WITHOUT_BLOCKING bool brixSuspendActiveTaskUntilFileChange(
    int fd, int type, BxSemaphore* sem);
extern "C" BRIX_WITHOUT_BLOCKING bool brixSuspendActiveTaskUntilSignal(int sig, BxSemaphore* sem);
extern "C" BRIX_WITHOUT_BLOCKING bool brixSuspendActiveTaskUntilTime(long int us, BxSemaphore* sem);
extern "C" uint64_t brixMonotonicTimeNanoseconds();
extern "C" uint64_t brixNextScheduledWorkDeadlineNanoseconds();
extern "C" bool brixHasPendingEventWork();
extern "C" BRIX_WITHOUT_BLOCKING uint32_t brixSuspendActiveTaskUntilIdle();
extern "C" BRIX_WITHOUT_BLOCKING bool brixSuspendActiveTaskUntilAbsoluteTime(
    uint64_t deadlineNs, BxSemaphore* sem);
extern "C" void brixReleaseEventDescriptor(int fd);
extern "C" void brixCancelProcExitEventListener(int pid, BxSemaphore* sem);
extern "C" void brixCancelFileChangeEventListener(int fd, int type, BxSemaphore* sem);
extern "C" void brixCancelSignalEventListener(int sig, BxSemaphore* sem);
extern "C" void brixCancelTimerEventListener(BxSemaphore* sem);

extern "C" void* brixPathWatchCreate(const char* path, bool isRecursive);
extern "C" BRIX_WITHOUT_BLOCKING bool brixSuspendActiveTaskUntilPathWatchEvent(
    void* watcher, BxSemaphore* sem);
extern "C" bool brixPathWatchNextEvent(void* watcher, BxPathWatchNativeEvent* out);
extern "C" void brixCancelPathWatchEventListener(void* watcher, BxSemaphore* sem);
extern "C" void brixPathWatchRelease(void* watcher);

extern "C" BRIX_WITHOUT_GC int64_t brixMainBundleString(const char* key, char* buffer, int64_t capacity);

extern "C" BRIX_WITHOUT_GC int brixSharedMemoryCreate(const char* name, int64_t size);
extern "C" BRIX_WITHOUT_GC int brixSharedMemoryOpen(const char* name);
extern "C" BRIX_WITHOUT_GC bool brixSharedMemoryUnlink(const char* name);
extern "C" BRIX_WITHOUT_GC void* brixSharedMemoryMap(int fd, int64_t size, bool writable);
extern "C" BRIX_WITHOUT_GC void brixSharedMemoryUnmap(void* memory, int64_t size);
extern "C" BRIX_WITHOUT_GC bool brixSocketPair(int* fds);
extern "C" BRIX_WITHOUT_GC uint32_t brixMachRendezvousCheckIn(const char* name);
extern "C" BRIX_WITHOUT_GC uint32_t brixMachRendezvousLookUp(const char* name);
extern "C" BRIX_WITHOUT_GC void brixMachRendezvousClose(uint32_t port);
extern "C" BRIX_WITHOUT_GC void brixMachPortRelease(uint32_t port);
extern "C" BRIX_WITHOUT_GC bool brixMachSendParcel(
    uint32_t destination, const uint32_t* ports, int32_t portCount, const void* bytes, int64_t size);
extern "C" BRIX_WITHOUT_GC bool brixMachReceiveParcel(
    uint32_t port, uint32_t* ports, int32_t portCapacity, void* bytes, int64_t byteCapacity, int32_t* counts);

extern "C" BRIX_WITHOUT_BLOCKING BxSemaphore* brixNewSemaphore();
extern "C" BRIX_WITHOUT_BLOCKING bool brixWaitForSemaphore(
    BxSemaphore* sem, void* buf, size_t capacity, BxTaskMutex* mutex);
extern "C" BRIX_WITHOUT_BLOCKING void brixPostSemaphore(
    BxSemaphore* sem, void* data, size_t sizeOfData);

extern "C" BRIX_WITHOUT_BLOCKING BxTaskMutex* brixNewMutex();
extern "C" BRIX_WITHOUT_BLOCKING void brixLockMutex(BxTaskMutex* mutex);
extern "C" void brixUnlockMutex(BxTaskMutex* mutex);

extern "C" char* brixNewWorker(int threadCount);
extern "C" char* brixActiveWorker();
extern "C" BRIX_WITHOUT_GC int brixUsableCoreCount();
extern "C" BRIX_WITHOUT_BLOCKING int brixClaimLazyGlobal(uint8_t* state);
extern "C" void brixReleaseLazyGlobal(uint8_t* state);
extern "C" void brixAbandonLazyGlobal(uint8_t* state);
extern "C" bool brixForkGCMarkTaskInWorker(
    BxTaskQueue* queue, BxTaskStartFunc func, void* closure);
extern "C" bool brixForkGCCleanupTask(
    BxTaskStartFunc func, void* closure);
extern "C" bool brixWorkerGetStatus(char* worker);
extern "C" void brixWorkerStop(char* worker);

#endif
