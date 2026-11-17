#ifndef YU_GC_HANDSHAKE_H
#define YU_GC_HANDSHAKE_H

#include <stddef.h>
#include <stdint.h>

#include <YuGC/YuGCAnnotations.h>
#include <YuGC/YuGCStackMap.h>
#include <YuGC/YuGCTemporaryRoots.h>
#include <YuEvents/YuTaskRuntimeContext.h>

class YuTask;
class YuThread;

// *************************************************************************************************

enum YuTaskGCState {
    YuTaskGCStateRunning = 0,
    YuTaskGCStateInZone = 1,
    YuTaskGCStateBlocked = 2,
    YuTaskGCStateParked = 3,
    YuTaskGCStateDead = 4
};

enum YuGCHandshakeReason {
    YuGCHandshakeReasonMarkSetup = 1,
    YuGCHandshakeReasonMarkTermination = 2,
    YuGCHandshakeReasonSweep = 3
};

typedef void (*YuGCTaskVisitor)(
    YuTask* task,
    uint32_t state,
    void* stackBase,
    size_t stackSize,
    void* context,
    void* data);
typedef void (*YuGCTemporaryRootListVisitor)(void* head, void* data);

// *************************************************************************************************

extern "C" YUMI_WITHOUT_GC void yumiGCSafepoint(void);
extern "C" YUMI_WITHOUT_GC void yumiTaskSafepoint(void);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCHandshakeBegin(uint32_t reason);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCHandshakeBeginWithReportAfter(
    uint32_t reason, uint64_t reportAfterUsec);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCHandshakeEnd(void);
extern "C" YUMI_WITHOUT_GC uint32_t yumiGCHandshakeReason(void);
extern "C" YUMI_WITHOUT_GC uint64_t yumiGCHandshakeEpoch(void);
extern "C" YUMI_WITHOUT_GC size_t yumiGCHandshakeTaskCount(void);
extern "C" YUMI_WITHOUT_GC size_t yumiGCHandshakeStableTaskCount(void);
extern "C" YUMI_WITHOUT_GC size_t yumiGCHandshakeThreadCount(void);
extern "C" YUMI_WITHOUT_GC size_t yumiGCHandshakeTasksWaited(void);
extern "C" YUMI_WITHOUT_GC uint64_t yumiGCHandshakeSlowestTaskNanos(void);
extern "C" YUMI_WITHOUT_GC size_t yumiGCHandshakeReportCount(void);
extern "C" YUMI_WITHOUT_GC uint32_t yumiGetTaskGCState(YuTask* task);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCEnumerateTasks(YuGCTaskVisitor visitor, void* data);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCScanCurrentStack(
    YuGCStackRootVisitor visitor, void* data);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCScanTaskStacks(
    YuGCStackRootVisitor visitor, void* data);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCScanStorageRoots(
    YuGCStackRootVisitor visitor, void* data);
extern "C" YUMI_WITHOUT_GC YumiGCTemporaryRootStorage*
yumiGCActiveTaskTemporaryRootStorage(void);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCVisitTaskTemporaryRootLists(
    YuGCTemporaryRootListVisitor visitor, void* data);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCVisitTaskParseRegionAllocations(
    YuParseRegionAllocationVisitor visitor, void* data);
extern "C" YUMI_WITHOUT_BLOCKING void yumiGCVisitActiveTaskParseRegionAllocations(
    YuParseRegionAllocationVisitor visitor, void* data);
extern "C" YUMI_WITHOUT_GC void yumiGCEnterRuntimeBlock(void);
extern "C" YUMI_WITHOUT_GC void yumiGCLeaveRuntimeBlock(void);
extern "C" YUMI_WITHOUT_GC void yumiGCEnterFFI(void);
extern "C" YUMI_WITHOUT_GC void yumiGCLeaveFFI(void);
extern "C" YUMI_WITHOUT_GC void yumiGCEnterZone(void);
extern "C" YUMI_WITHOUT_GC void yumiGCLeaveZone(void);
// Native frameworks that invoke managed function pointers from an outer FFI call or GC-free zone
// must bracket the callback so its live stack participates in phase-change handshakes.
extern "C" YUMI_WITHOUT_GC void yumiGCEnterManagedCallback(void);
extern "C" YUMI_WITHOUT_GC void yumiGCLeaveManagedCallback(void);
extern "C" YUMI_WITHOUT_GC void yumiGCEnterForeignCallback(void);
extern "C" YUMI_WITHOUT_GC void yumiGCLeaveForeignCallback(void);
extern "C" YUMI_WITHOUT_GC void yumiGCEnterRuntimeCritical(void);
extern "C" YUMI_WITHOUT_GC void yumiGCLeaveRuntimeCritical(void);
extern "C" YUMI_WITHOUT_GC bool yumiGCIsRuntimeCritical(void);
// True only while the calling thread owns a Running task that a phase-change handshake
// must stop. Blocked/runtime/FFI states and non-task threads require collector admission.
extern "C" YUMI_WITHOUT_GC bool yumiGCActiveTaskCanUseFastAllocation(void);
extern "C" YUMI_WITHOUT_GC bool yumiGCActiveTaskAcknowledgedHandshake(uint64_t control);

YUMI_WITHOUT_GC void yumiGCRegisterTask(YuTask* task);
YUMI_WITHOUT_GC void yumiGCUnregisterTask(YuTask* task);
YUMI_WITHOUT_GC void yumiGCRegisterThread(YuThread* thread);
YUMI_WITHOUT_GC void yumiGCUnregisterThread(YuThread* thread);
// Generated thread-local init/flush callbacks can execute managed code before a worker owns a
// task. Bracket those taskless callbacks so a scheduler handshake either waits for their final
// publication or completes before they begin.
YUMI_WITHOUT_GC void yumiGCBeginThreadLocalCallback(YuThread* thread);
YUMI_WITHOUT_GC void yumiGCEndThreadLocalCallback(YuThread* thread);
extern "C" YUMI_WITHOUT_GC bool yumiGCThreadHasLocalCallbackRegistration(void);
YUMI_WITHOUT_GC void yumiGCSetTaskState(YuTask* task, uint32_t state);
YUMI_WITHOUT_GC void yumiGCSetTaskStateAndContext(
    YuTask* task, uint32_t state, bool contextReady);
YUMI_WITHOUT_GC void yumiGCPrepareTaskResume(YuTask* task);
YUMI_WITHOUT_GC void yumiGCRevalidateThreadAllocation(uint64_t* rangeControl);

// *************************************************************************************************

#endif
