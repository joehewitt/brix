#ifndef BX_GC_HANDSHAKE_H
#define BX_GC_HANDSHAKE_H

#include <stddef.h>
#include <stdint.h>

#include <BxGC/BxGCAnnotations.h>
#include <BxGC/BxGCStackMap.h>
#include <BxGC/BxGCTemporaryRoots.h>
#include <BxEvents/BxTaskRuntimeContext.h>

class BxTask;
class BxThread;

// *************************************************************************************************

enum BxTaskGCState {
    BxTaskGCStateRunning = 0,
    BxTaskGCStateInZone = 1,
    BxTaskGCStateBlocked = 2,
    BxTaskGCStateParked = 3,
    BxTaskGCStateDead = 4
};

enum BxGCHandshakeReason {
    BxGCHandshakeReasonMarkSetup = 1,
    BxGCHandshakeReasonMarkTermination = 2,
    BxGCHandshakeReasonSweep = 3
};

typedef void (*BxGCTaskVisitor)(
    BxTask* task,
    uint32_t state,
    void* stackBase,
    size_t stackSize,
    void* context,
    void* data);
typedef void (*BxGCTemporaryRootListVisitor)(void* head, void* data);

// *************************************************************************************************

extern "C" BRIX_WITHOUT_GC void brixGCSafepoint(void);
extern "C" BRIX_WITHOUT_GC void brixTaskSafepoint(void);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCHandshakeBegin(uint32_t reason);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCHandshakeBeginWithReportAfter(
    uint32_t reason, uint64_t reportAfterUsec);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCHandshakeEnd(void);
extern "C" BRIX_WITHOUT_GC uint32_t brixGCHandshakeReason(void);
extern "C" BRIX_WITHOUT_GC uint64_t brixGCHandshakeEpoch(void);
extern "C" BRIX_WITHOUT_GC size_t brixGCHandshakeTaskCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixGCHandshakeStableTaskCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixGCHandshakeThreadCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixGCHandshakeTasksWaited(void);
extern "C" BRIX_WITHOUT_GC uint64_t brixGCHandshakeSlowestTaskNanos(void);
extern "C" BRIX_WITHOUT_GC size_t brixGCHandshakeReportCount(void);
extern "C" BRIX_WITHOUT_GC uint32_t brixGetTaskGCState(BxTask* task);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCEnumerateTasks(BxGCTaskVisitor visitor, void* data);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCScanCurrentStack(
    BxGCStackRootVisitor visitor, void* data);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCScanTaskStacks(
    BxGCStackRootVisitor visitor, void* data);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCScanStorageRoots(
    BxGCStackRootVisitor visitor, void* data);
extern "C" BRIX_WITHOUT_GC BrixGCTemporaryRootStorage*
brixGCActiveTaskTemporaryRootStorage(void);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCVisitTaskTemporaryRootLists(
    BxGCTemporaryRootListVisitor visitor, void* data);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCVisitTaskParseRegionAllocations(
    BxParseRegionAllocationVisitor visitor, void* data);
extern "C" BRIX_WITHOUT_BLOCKING void brixGCVisitActiveTaskParseRegionAllocations(
    BxParseRegionAllocationVisitor visitor, void* data);
extern "C" BRIX_WITHOUT_GC void brixGCEnterRuntimeBlock(void);
extern "C" BRIX_WITHOUT_GC void brixGCLeaveRuntimeBlock(void);
extern "C" BRIX_WITHOUT_GC void brixGCEnterFFI(void);
extern "C" BRIX_WITHOUT_GC void brixGCLeaveFFI(void);
extern "C" BRIX_WITHOUT_GC void brixGCEnterZone(void);
extern "C" BRIX_WITHOUT_GC void brixGCLeaveZone(void);
// Native frameworks that invoke managed function pointers from an outer FFI call or GC-free zone
// must bracket the callback so its live stack participates in phase-change handshakes.
extern "C" BRIX_WITHOUT_GC void brixGCEnterManagedCallback(void);
extern "C" BRIX_WITHOUT_GC void brixGCLeaveManagedCallback(void);
extern "C" BRIX_WITHOUT_GC void brixGCEnterForeignCallback(void);
extern "C" BRIX_WITHOUT_GC void brixGCLeaveForeignCallback(void);
extern "C" BRIX_WITHOUT_GC void brixGCEnterRuntimeCritical(void);
extern "C" BRIX_WITHOUT_GC void brixGCLeaveRuntimeCritical(void);
extern "C" BRIX_WITHOUT_GC bool brixGCIsRuntimeCritical(void);
// True only while the calling thread owns a Running task that a phase-change handshake
// must stop. Blocked/runtime/FFI states and non-task threads require collector admission.
extern "C" BRIX_WITHOUT_GC bool brixGCActiveTaskCanUseFastAllocation(void);
extern "C" BRIX_WITHOUT_GC bool brixGCActiveTaskAcknowledgedHandshake(uint64_t control);

BRIX_WITHOUT_GC void brixGCRegisterTask(BxTask* task);
BRIX_WITHOUT_GC void brixGCUnregisterTask(BxTask* task);
BRIX_WITHOUT_GC void brixGCRegisterThread(BxThread* thread);
BRIX_WITHOUT_GC void brixGCUnregisterThread(BxThread* thread);
// Generated thread-local init/flush callbacks can execute managed code before a worker owns a
// task. Bracket those taskless callbacks so a scheduler handshake either waits for their final
// publication or completes before they begin.
BRIX_WITHOUT_GC void brixGCBeginThreadLocalCallback(BxThread* thread);
BRIX_WITHOUT_GC void brixGCEndThreadLocalCallback(BxThread* thread);
extern "C" BRIX_WITHOUT_GC bool brixGCThreadHasLocalCallbackRegistration(void);
BRIX_WITHOUT_GC void brixGCSetTaskState(BxTask* task, uint32_t state);
BRIX_WITHOUT_GC void brixGCSetTaskStateAndContext(
    BxTask* task, uint32_t state, bool contextReady);
BRIX_WITHOUT_GC void brixGCPrepareTaskResume(BxTask* task);
BRIX_WITHOUT_GC void brixGCRevalidateThreadAllocation(uint64_t* rangeControl);

// *************************************************************************************************

#endif
