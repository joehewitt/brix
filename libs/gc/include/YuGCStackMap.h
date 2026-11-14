#ifndef YU_GC_STACK_MAP_H
#define YU_GC_STACK_MAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "YuGCAnnotations.h"

#ifdef __cplusplus
extern "C" {
#endif

// *************************************************************************************************

typedef void (*YuGCStackRootVisitor)(void* root, void* descriptor, void* data);

typedef struct YumiGCContextSnapshot {
    void* arg;
    void* sp;
    void* fp;
    void* pc;
    uintptr_t x[29];
} YumiGCContextSnapshot;

typedef struct YumiGCStorageRootRecord {
    uint8_t kind;
    void* base;
    void* descriptor;
} YumiGCStorageRootRecord;

typedef void (*YumiGCStorageRootRecordVisitor)(
    const YumiGCStorageRootRecord* record,
    void* data);

// *************************************************************************************************

YUMI_GC_POLLING void yumiGCScanContextRoots(
    const YumiGCContextSnapshot* context,
    void* stackBase,
    size_t stackSize,
    YuGCStackRootVisitor visitor,
    void* data);
YUMI_GC_POLLING void yumiGCScanConservativeContextRoots(
    const YumiGCContextSnapshot* context,
    void* stackBase,
    size_t stackSize,
    YuGCStackRootVisitor visitor,
    void* data);
YUMI_GC_LEAF size_t yumiGCStorageRootRecordCapacity(size_t taskCount, size_t threadCount);
YUMI_GC_POLLING void yumiGCVisitStorageRootRecords(
    YumiGCStorageRootRecordVisitor visitor,
    void* data);
YUMI_GC_LEAF void* yumiGCDebugConservativeRootSlot(void);
YUMI_GC_LEAF bool yumiGCVisitingConservativeRoot(void);
YUMI_GC_LEAF void yumiGCDebugClearStackMapLookupCache(void);
YUMI_GC_LEAF size_t yumiGCDebugStackMapFunctionIndexCount(void);
YUMI_GC_LEAF bool yumiGCDebugStackMapLookupSelfTest(void);
YUMI_GC_LEAF bool yumiGCDebugStackMapLookup(
    uintptr_t pc,
    uintptr_t* functionAddress,
    uint32_t* functionByteSize,
    uint32_t* safepointOffset,
    uint32_t* locationCount,
    uint32_t* savedRegisterCount,
    bool* cacheHit);

// *************************************************************************************************

#ifdef __cplusplus
}
#endif

#endif
