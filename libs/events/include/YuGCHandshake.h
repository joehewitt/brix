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

extern "C" YUMI_GC_LEAF void yumiGCSafepoint(void);
extern "C" YUMI_GC_LEAF void yumiTaskSafepoint(void);
extern "C" YUMI_GC_POLLING void yumiGCHandshakeBegin(uint32_t reason);
extern "C" YUMI_GC_POLLING void yumiGCHandshakeBeginWithReportAfter(
    uint32_t reason, uint64_t reportAfterUsec);
extern "C" YUMI_GC_POLLING void yumiGCHandshakeEnd(void);
extern "C" YUMI_GC_LEAF uint32_t yumiGCHandshakeReason(void);
extern "C" YUMI_GC_LEAF uint64_t yumiGCHandshakeEpoch(void);
extern "C" YUMI_GC_LEAF size_t yumiGCHandshakeTaskCount(void);
extern "C" YUMI_GC_LEAF size_t yumiGCHandshakeStableTaskCount(void);
extern "C" YUMI_GC_LEAF size_t yumiGCHandshakeThreadCount(void);
extern "C" YUMI_GC_LEAF size_t yumiGCHandshakeTasksWaited(void);
extern "C" YUMI_GC_LEAF uint64_t yumiGCHandshakeSlowestTaskNanos(void);
extern "C" YUMI_GC_LEAF size_t yumiGCHandshakeReportCount(void);
extern "C" YUMI_GC_LEAF uint32_t yumiGetTaskGCState(YuTask* task);
extern "C" YUMI_GC_POLLING void yumiGCEnumerateTasks(YuGCTaskVisitor visitor, void* data);
extern "C" YUMI_GC_POLLING void yumiGCScanCurrentStack(
    YuGCStackRootVisitor visitor, void* data);
extern "C" YUMI_GC_POLLING void yumiGCScanTaskStacks(
    YuGCStackRootVisitor visitor, void* data);
extern "C" YUMI_GC_POLLING void yumiGCScanStorageRoots(
    YuGCStackRootVisitor visitor, void* data);
extern "C" YUMI_GC_LEAF YumiGCTemporaryRootStorage*
yumiGCActiveTaskTemporaryRootStorage(void);
extern "C" YUMI_GC_POLLING void yumiGCVisitTaskTemporaryRootLists(
    YuGCTemporaryRootListVisitor visitor, void* data);
extern "C" YUMI_GC_POLLING void yumiGCVisitTaskParseRegionAllocations(
    YuParseRegionAllocationVisitor visitor, void* data);
extern "C" YUMI_GC_POLLING void yumiGCVisitActiveTaskParseRegionAllocations(
    YuParseRegionAllocationVisitor visitor, void* data);
extern "C" YUMI_GC_LEAF void yumiGCEnterRuntimeBlock(void);
extern "C" YUMI_GC_LEAF void yumiGCLeaveRuntimeBlock(void);
extern "C" YUMI_GC_LEAF void yumiGCEnterFFI(void);
extern "C" YUMI_GC_LEAF void yumiGCLeaveFFI(void);
extern "C" YUMI_GC_LEAF void yumiGCEnterZone(void);
extern "C" YUMI_GC_LEAF void yumiGCLeaveZone(void);
// Native frameworks that invoke managed function pointers from an outer FFI call or GC-free zone
// must bracket the callback so its live stack participates in phase-change handshakes.
extern "C" YUMI_GC_LEAF void yumiGCEnterManagedCallback(void);
extern "C" YUMI_GC_LEAF void yumiGCLeaveManagedCallback(void);
extern "C" YUMI_GC_LEAF void yumiGCEnterForeignCallback(void);
extern "C" YUMI_GC_LEAF void yumiGCLeaveForeignCallback(void);
extern "C" YUMI_GC_LEAF void yumiGCEnterRuntimeCritical(void);
extern "C" YUMI_GC_LEAF void yumiGCLeaveRuntimeCritical(void);
extern "C" YUMI_GC_LEAF bool yumiGCIsRuntimeCritical(void);
// True only while the calling thread owns a Running task that a phase-change handshake
// must stop. Blocked/runtime/FFI states and non-task threads require collector admission.
extern "C" YUMI_GC_LEAF bool yumiGCActiveTaskCanUseFastAllocation(void);
extern "C" YUMI_GC_LEAF bool yumiGCActiveTaskAcknowledgedHandshake(uint64_t control);

YUMI_GC_LEAF void yumiGCRegisterTask(YuTask* task);
YUMI_GC_LEAF void yumiGCUnregisterTask(YuTask* task);
YUMI_GC_LEAF void yumiGCRegisterThread(YuThread* thread);
YUMI_GC_LEAF void yumiGCUnregisterThread(YuThread* thread);
// Generated thread-local init/flush callbacks can execute managed code before a worker owns a
// task. Bracket those taskless callbacks so a scheduler handshake either waits for their final
// publication or completes before they begin.
YUMI_GC_LEAF void yumiGCBeginThreadLocalCallback(YuThread* thread);
YUMI_GC_LEAF void yumiGCEndThreadLocalCallback(YuThread* thread);
extern "C" YUMI_GC_LEAF bool yumiGCThreadHasLocalCallbackRegistration(void);
YUMI_GC_LEAF void yumiGCSetTaskState(YuTask* task, uint32_t state);
YUMI_GC_LEAF void yumiGCSetTaskStateAndContext(
    YuTask* task, uint32_t state, bool contextReady);
YUMI_GC_LEAF void yumiGCPrepareTaskResume(YuTask* task);
YUMI_GC_LEAF void yumiGCRevalidateThreadAllocation(uint64_t* rangeControl);

// *************************************************************************************************

#endif
