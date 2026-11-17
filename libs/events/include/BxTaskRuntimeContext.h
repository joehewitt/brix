#ifndef BX_TASK_RUNTIME_CONTEXT_H
#define BX_TASK_RUNTIME_CONTEXT_H

#include <stddef.h>
#include <stdint.h>

#include <BxGC/BxGCTemporaryRoots.h>

typedef void (*BxParseRegionAllocationVisitor)(
    void* allocation,
    size_t byteSize,
    void* descriptor,
    void* data);
typedef void (*BxParseRegionScanner)(
    void* region,
    BxParseRegionAllocationVisitor visitor,
    void* data);
typedef void (*BxParseRegionDestroyer)(void* region);

const uint32_t BX_TASK_END_RUNNING = 0;
const uint32_t BX_TASK_END_FINISHED = 1;
const uint32_t BX_TASK_END_RAISED = 2;
const uint32_t BX_TASK_END_PANICKED = 3;
const uint32_t BX_TASK_END_CANCELLED = 4;

// Kept deliberately small and stable: generated AArch64 code reads activePanic at offset zero
// through the task-context register, while C runtime code also owns the GC root handle.
struct BxTaskRuntimeContext {
    void* activePanic;
    void* activePanicRoot;
    // Generated parser code reads these two words directly through x27. Keep their offsets
    // stable and mirrored by parseRegionTaskContext*Offset in brix/constants.ts.
    size_t parseRegionLogicalUsed;
    size_t parseRegionAllocationScopeDepth;
    char* taskLocals;
    BrixGCTemporaryRootStorage temporaryRoots;
    void* parseRegion;
    BxParseRegionScanner parseRegionScanner;
    BxParseRegionDestroyer parseRegionDestroyer;
    void* pendingPanics;
    void* probeProfileTimers;
    uint32_t taskEnd;
};

static_assert(offsetof(BxTaskRuntimeContext, parseRegionLogicalUsed) == 16,
    "generated parse-region mark offset changed");
static_assert(offsetof(BxTaskRuntimeContext, parseRegionAllocationScopeDepth) == 24,
    "generated parse-region scope offset changed");
static_assert(offsetof(BxTaskRuntimeContext, taskLocals) == 32,
    "generated task-local memory offset changed");

#endif
