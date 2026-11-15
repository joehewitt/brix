#ifndef YU_TASK_RUNTIME_CONTEXT_H
#define YU_TASK_RUNTIME_CONTEXT_H

#include <stddef.h>
#include <stdint.h>

#include <YuGC/YuGCTemporaryRoots.h>

typedef void (*YuParseRegionAllocationVisitor)(
    void* allocation,
    size_t byteSize,
    void* descriptor,
    void* data);
typedef void (*YuParseRegionScanner)(
    void* region,
    YuParseRegionAllocationVisitor visitor,
    void* data);
typedef void (*YuParseRegionDestroyer)(void* region);

const uint32_t YU_TASK_END_RUNNING = 0;
const uint32_t YU_TASK_END_FINISHED = 1;
const uint32_t YU_TASK_END_RAISED = 2;
const uint32_t YU_TASK_END_PANICKED = 3;
const uint32_t YU_TASK_END_CANCELLED = 4;

// Kept deliberately small and stable: generated AArch64 code reads activePanic at offset zero
// through the task-context register, while C runtime code also owns the GC root handle.
struct YuTaskRuntimeContext {
    void* activePanic;
    void* activePanicRoot;
    // Generated parser code reads these two words directly through x27. Keep their offsets
    // stable and mirrored by parseRegionTaskContext*Offset in yumi/constants.ts.
    size_t parseRegionLogicalUsed;
    size_t parseRegionAllocationScopeDepth;
    char* taskLocals;
    YumiGCTemporaryRootStorage temporaryRoots;
    void* parseRegion;
    YuParseRegionScanner parseRegionScanner;
    YuParseRegionDestroyer parseRegionDestroyer;
    void* pendingPanics;
    void* probeProfileTimers;
    uint32_t taskEnd;
};

static_assert(offsetof(YuTaskRuntimeContext, parseRegionLogicalUsed) == 16,
    "generated parse-region mark offset changed");
static_assert(offsetof(YuTaskRuntimeContext, parseRegionAllocationScopeDepth) == 24,
    "generated parse-region scope offset changed");
static_assert(offsetof(YuTaskRuntimeContext, taskLocals) == 32,
    "generated task-local memory offset changed");

#endif
