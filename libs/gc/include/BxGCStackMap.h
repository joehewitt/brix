#ifndef BX_GC_STACK_MAP_H
#define BX_GC_STACK_MAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "BxGCAnnotations.h"

#ifdef __cplusplus
extern "C" {
#endif

// *************************************************************************************************

typedef void (*BxGCStackRootVisitor)(void* root, void* descriptor, void* data);

typedef struct BrixGCContextSnapshot {
    void* arg;
    void* sp;
    void* fp;
    void* pc;
    uintptr_t x[29];
} BrixGCContextSnapshot;

typedef struct BrixGCStorageRootRecord {
    uint8_t kind;
    void* base;
    void* descriptor;
} BrixGCStorageRootRecord;

typedef void (*BrixGCStorageRootRecordVisitor)(
    const BrixGCStorageRootRecord* record,
    void* data);

// *************************************************************************************************

BRIX_WITHOUT_BLOCKING void brixGCScanContextRoots(
    const BrixGCContextSnapshot* context,
    void* stackBase,
    size_t stackSize,
    BxGCStackRootVisitor visitor,
    void* data);
BRIX_WITHOUT_BLOCKING void brixGCScanConservativeContextRoots(
    const BrixGCContextSnapshot* context,
    void* stackBase,
    size_t stackSize,
    BxGCStackRootVisitor visitor,
    void* data);
BRIX_WITHOUT_GC size_t brixGCStorageRootRecordCapacity(size_t taskCount, size_t threadCount);
BRIX_WITHOUT_BLOCKING void brixGCVisitStorageRootRecords(
    BrixGCStorageRootRecordVisitor visitor,
    void* data);
BRIX_WITHOUT_GC void* brixGCDebugConservativeRootSlot(void);
BRIX_WITHOUT_GC bool brixGCVisitingConservativeRoot(void);
BRIX_WITHOUT_GC void brixGCDebugClearStackMapLookupCache(void);
BRIX_WITHOUT_GC size_t brixGCDebugStackMapFunctionIndexCount(void);
BRIX_WITHOUT_GC bool brixGCDebugStackMapLookupSelfTest(void);
BRIX_WITHOUT_GC bool brixGCDebugStackMapLookup(
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
