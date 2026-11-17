#ifndef BX_CORE_H
#define BX_CORE_H

#include <cstdlib>
#include <stdint.h>
#include <stddef.h>
#include <sys/dir.h>
#include <BxGC/BxGCAnnotations.h>
#include <BxGC/BxGCAllocationBuffer.h>
#include <BxGC/BxGCTemporaryRoots.h>
#include <BxCore/BxString.h>

#ifndef BX_ENABLE_BRIX_GC
#define BX_ENABLE_BRIX_GC 1
#endif

#ifndef BX_GC_NEVER_COLLECT
#define BX_GC_NEVER_COLLECT 0
#endif

#if !BX_ENABLE_BRIX_GC && BX_GC_NEVER_COLLECT
#error "BX_GC_NEVER_COLLECT requires BX_ENABLE_BRIX_GC"
#endif

#ifndef BX_ENABLE_BENCH_METRICS
#define BX_ENABLE_BENCH_METRICS 0
#endif

#define BX_MALLOC brixMalloc
#define BX_MALLOC_ATOMIC brixMallocAtomic
#define BX_REALLOC brixRealloc

// **********************************************************************************************

enum : unsigned char {
    BrixInterruptTerminationRequested = 1u << 1,
    BrixInterruptSchedulerRequested = 1u << 2,
};

extern "C" unsigned char brixInterruptRequestedFlags;
extern "C" void brixZeroStackFrame(void* address, size_t byteCount);

// **********************************************************************************************

void* brixMalloc(size_t size);
void* brixMallocAtomic(size_t size);
void* brixRealloc(void* buf, size_t size);

// **********************************************************************************************

extern "C" BRIX_WITHOUT_GC void brixSaveArgs(int argc, char** argv);
extern "C" BRIX_WITHOUT_GC bool brixHostIsLittleEndian();
extern "C" BRIX_WITHOUT_GC int brixGetArgC();
extern "C" BRIX_WITHOUT_GC char** brixGetArgV();

// *************************************************************************************************

extern "C" BRIX_WITHOUT_BLOCKING char* brixAllocObject(size_t size);
extern "C" BRIX_WITHOUT_BLOCKING char* brixAllocObjectDescribed(size_t size, void* descriptor);
extern "C" BRIX_WITHOUT_BLOCKING char* brixAllocBuffer(size_t itemSize, size_t count);
extern "C" BRIX_WITHOUT_GC char* brixTryAllocBufferWithoutSafepoint(size_t size);
extern "C" BRIX_WITHOUT_BLOCKING char* brixAllocBufferDescribed(
    size_t itemSize, size_t count, void* descriptor);
extern "C" BRIX_WITHOUT_BLOCKING char* brixResizeBuffer(
    char* buf, size_t oldSize, size_t newSize);
extern "C" BRIX_WITHOUT_BLOCKING char* brixResizeBufferDescribed(
    char* buf, size_t oldSize, size_t newSize, void* descriptor);

// Explicit native Arena storage. Handles carry a stable slot pointer plus an incarnation. All
// entry points are GC leaves and never allocate managed memory, publish roots, or cross a GC zone.
enum : size_t {
    BrixArenaIncarnationOffset = 0,
    BrixArenaGenerationOffset = 8,
    BrixArenaLiveOffset = 16,
};

extern "C" BRIX_WITHOUT_GC void* brixArenaCreate(size_t initialCapacity);
extern "C" BRIX_WITHOUT_GC size_t brixArenaIncarnation(void* arena);
extern "C" BRIX_WITHOUT_GC char* brixArenaReserve(
    void* arena,
    size_t incarnation,
    size_t byteCount,
    size_t alignment,
    bool zeroInitialize);
extern "C" BRIX_WITHOUT_GC void brixArenaAllocationFailed(
    void* arena,
    size_t incarnation,
    size_t alignment);
extern "C" BRIX_WITHOUT_BLOCKING char* brixArenaReserveDescribed(
    void* arena,
    size_t incarnation,
    size_t byteCount,
    size_t alignment,
    bool zeroInitialize,
    void* descriptor);
extern "C" BRIX_WITHOUT_GC const BrixStringData* brixArenaCopyString(
    void* arena,
    size_t incarnation,
    const char* value);
extern "C" BRIX_WITHOUT_GC const BrixStringData* brixArenaCopyCodepointSpan(
    void* arena,
    size_t incarnation,
    const uint32_t* source,
    size_t sourceLength,
    size_t start,
    size_t end,
    bool stopAtNull);
extern "C" BRIX_WITHOUT_GC const BrixStringData* brixArenaCopyByteSpan(
    void* arena,
    size_t incarnation,
    const uint8_t* source,
    size_t sourceLength,
    size_t start,
    size_t end);
extern "C" BRIX_WITHOUT_GC bool brixArenaOwns(
    void* arena,
    size_t incarnation,
    const void* value);
extern "C" BRIX_WITHOUT_GC size_t brixArenaMark(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaRewind(
    void* arena,
    size_t incarnation,
    size_t mark);
extern "C" BRIX_WITHOUT_GC void brixArenaReset(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC void brixArenaRelease(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaGeneration(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaState(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaActiveBlockCount(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaRetainedBlockCount(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaActiveBytes(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaReservedBytes(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaRetainedBytes(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaPeakActiveBytes(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaOwnerSystemBlockAllocationCount(
    void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaOwnerResetCount(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaOwnerRewindCount(void* arena, size_t incarnation);
extern "C" BRIX_WITHOUT_GC size_t brixArenaSystemBlockAllocationCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaSystemBlockFreeCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaRuntimeActiveBytes(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaRuntimeReservedBytes(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaRuntimeRetainedBytes(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaRuntimePeakActiveBytes(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaMarkCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaRewindCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaResetCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaSlotCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaLiveSlotCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaManagedAllocationCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixArenaZoneTransitionCount(void);
// Per-task storage for parser-only values. Marks are logical cursors: resetting a mark invalidates
// every later region allocation after observable results have been promoted to the GC heap.
extern "C" BRIX_WITHOUT_GC size_t brixParseRegionMark(void);
extern "C" BRIX_WITHOUT_GC void brixParseRegionReset(size_t mark);
extern "C" BRIX_WITHOUT_GC void brixParseRegionEnterAllocationScope(
    size_t line, size_t column);
extern "C" BRIX_WITHOUT_GC void brixParseRegionLeaveAllocationScope(void);
extern "C" BRIX_WITHOUT_GC size_t brixParseRegionSuspendAllocationScopes(void);
extern "C" BRIX_WITHOUT_GC void brixParseRegionResumeAllocationScopes(size_t depth);
extern "C" BRIX_WITHOUT_GC void brixParseRegionCancelAllocationScopes(void);
extern "C" BRIX_WITHOUT_GC char* brixParseRegionAllocObject(
    size_t size, void* descriptor);
extern "C" BRIX_WITHOUT_GC char* brixParseRegionAllocBuffer(
    size_t itemSize, size_t count, void* descriptor);
extern "C" BRIX_WITHOUT_BLOCKING char* brixParseRegionPromote(char* pointer);
extern "C" BRIX_WITHOUT_BLOCKING void brixParseRegionPromoteValue(
    char* value, size_t byteSize, void* descriptor);
extern "C" BRIX_WITHOUT_GC bool brixParseRegionAllocationActive(void);
extern "C" BRIX_WITHOUT_GC bool brixParseRegionOwns(char* pointer);
extern "C" BRIX_WITHOUT_GC size_t brixParseRegionObjectAllocationCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixParseRegionBufferAllocationCount(void);
extern "C" BRIX_WITHOUT_GC size_t brixParseRegionPromotionCount(void);
extern "C" BRIX_WITHOUT_GC void brixParseRegionValidateStore(
    char* source, char* value, size_t line, size_t column);
extern "C" BRIX_WITHOUT_GC void brixParseRegionValidateBulkStore(
    char* source,
    char* value,
    size_t byteSize,
    void* descriptor,
    size_t line,
    size_t column);

extern "C" BRIX_WITHOUT_GC bool brixGCAllocationSiteKeyInit(
    BrixGCAllocationSiteKey* key,
    size_t requestedSize,
    void* descriptor,
    size_t pointerKind,
    size_t zeroingPolicy);
extern "C" BRIX_WITHOUT_BLOCKING char* brixAllocObjectWithSiteKey(
    const BrixGCAllocationSiteKey* key);
extern "C" {
extern thread_local BrixGCAllocationBuffer* brixGCActiveAllocationBuffer;
extern thread_local BrixGCThreadCache* brixGCActiveThreadCache;
extern uint64_t brixGCPublicControlWord;
}
extern "C" BRIX_WITHOUT_GC void brixGCSetActiveThreadCache(BrixGCThreadCache* cache);
extern "C" BRIX_WITHOUT_GC BrixGCThreadCache* brixGCGetActiveThreadCache();
extern "C" BRIX_WITHOUT_GC void brixGCSetActiveAllocationBufferStorage(
    BrixGCRuntimeAllocationBuffer* storage);
extern "C" BRIX_WITHOUT_GC BrixGCAllocationBuffer* brixGCGetActiveAllocationBuffer();
extern "C" BRIX_WITHOUT_BLOCKING char* brixGCAllocationBufferRefill(
    const BrixGCAllocationSiteKey* key);
extern "C" BRIX_WITHOUT_BLOCKING char* brixGCCompilerAllocationBufferRefill(
    const BrixGCAllocationSiteKey* key);
extern "C" BRIX_WITHOUT_GC void brixGCAllocationBufferFlush();
extern "C" BRIX_WITHOUT_GC void brixGCAllocationBufferPublish();
extern "C" BRIX_WITHOUT_GC void brixGCAllocationBufferRevoke();
extern "C" BRIX_WITHOUT_GC size_t brixGCAllocationBufferCursorOffset();
extern "C" BRIX_WITHOUT_GC size_t brixGCAllocationBufferLimitOffset();
extern "C" BRIX_WITHOUT_GC size_t brixGCAllocationBufferSize();
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeAllocationBufferHotOffset();
extern "C" void* brixAddRoot(char* pointer);
extern "C" void* brixAddTemporaryRoot(char* pointer);
extern "C" void* brixAddDescribedRoot(char* pointer, size_t byteSize, void* descriptor);
extern "C" void brixUpdateRoot(void* root, char* pointer);
extern "C" void brixRemoveRoot(void* root);
extern "C" void brixRemoveTemporaryRoot(void* root);
extern "C" void brixDrainTemporaryRoots(BrixGCTemporaryRootStorage* storage);
#ifndef BX_GC_COLLECT_RESULT_TYPE
#define BX_GC_COLLECT_RESULT_TYPE
typedef size_t BrixGCCollectResult;
enum : BrixGCCollectResult {
    BrixGCCollectCompleted = 0
};
#endif
// Non-zero results encode a stalled task as task-id + 1, so zero is unambiguously completed.
extern "C" BRIX_WITHOUT_BLOCKING BrixGCCollectResult brixCollect();
extern "C" BRIX_WITHOUT_BLOCKING void brixGCFrameBudget(uint64_t remainingNanos);
extern "C" BRIX_WITHOUT_BLOCKING bool brixGCRuntimeIdleCollect(uint64_t budgetNanos);
// Internal scalar weak subscriptions used by core property tracking. These live in the GC
// runtime, but the narrow declarations here keep core modules from importing the broad GC API.
extern "C" BRIX_WITHOUT_BLOCKING size_t brixGCRegisterOwnerWeak(char* owner, char* target);
extern "C" BRIX_WITHOUT_BLOCKING char* brixGCOwnerWeakLoad(size_t handle);
extern "C" BRIX_WITHOUT_BLOCKING bool brixGCReleaseOwnerWeak(size_t handle);
// Narrow benchmark observability kept here so core workloads need not import the broad GC API.
extern "C" BRIX_WITHOUT_GC size_t brixGCObjectSidePayloadCount(size_t lane);
// Compiler-elided outside the internal A/B/C benchmark runtime.
extern "C" void brixBenchCollect();
extern "C" BRIX_WITHOUT_GC void brixWriteBarrier(char* source, char* value);
extern "C" BRIX_WITHOUT_GC void brixWriteBarrierRecord(char* source, char* value);
extern "C" void brixBenchRecordBarrierCall(void);
extern "C" BRIX_WITHOUT_GC void brixBulkWriteBarrierFast(
    char* source, char* value, size_t byteSize, void* descriptor);

extern "C" BRIX_WITHOUT_GC unsigned int brixGenerateUID();

// *************************************************************************************************

extern "C" BRIX_WITHOUT_BLOCKING char* brixArrayCopySlice(
    char* self, const void* source, size_t size);
extern "C" BRIX_WITHOUT_BLOCKING char* brixArrayCopySliceDescribed(
    char* self, const void* source, size_t size, void* descriptor);
extern "C" char* brixArraySlice(char* self, size_t start, size_t end);

// *************************************************************************************************

extern "C" int brixTerminalEnter(int fd, struct termios* oldState);
extern "C" int brixTerminalExit(int fd, struct termios* state);
extern "C" int brixTerminalSetSignals(int fd, int enabled);

struct BxDirEntry {
    uint64_t inode;
    uint32_t nameLength;
    uint32_t kind;
};

extern "C" bool brixReadDirEntry(DIR* dir, char* buf, BxDirEntry* out);

// *************************************************************************************************

extern "C" BRIX_WITHOUT_GC int brixGetErrNo();

extern "C" void brixPrint(const char* text);
extern "C" void brixTerminate();
extern "C" BRIX_WITHOUT_GC void brixCrash();
extern "C" unsigned char brixBenchMetricsEnabledFlag;
extern "C" void brixBenchInitMetrics();
extern "C" void brixBenchRecordAllocation(size_t size);
extern "C" BRIX_WITHOUT_GC void brixBenchRecordAllocationSite(
    size_t siteId,
    const char* source,
    size_t line,
    size_t column,
    const char* function,
    const char* type,
    const char* operation,
    const char* pointerKind);
extern "C" BRIX_WITHOUT_GC void brixBenchMutatorPhaseStart(const char* name);
extern "C" BRIX_WITHOUT_GC void brixBenchMutatorPhaseEnd(const char* name);
extern "C" void brixBenchMaybeWriteMetrics();
extern "C" void brixBenchRecordProcessMemory(void);
extern "C" BRIX_WITHOUT_BLOCKING size_t brixBenchTemporaryRootShim(
    char* pointer, size_t rootCount, size_t iterations);
extern "C" BRIX_WITHOUT_GC void brixBenchResetTemporaryRootMetrics(void);
extern "C" BRIX_WITHOUT_GC void brixBenchResetActiveTemporaryRootMetrics(void);
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeMode();
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchGCCpuNanos();
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchAllocationSiteCount();
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchAllocationSiteValue(size_t index);
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchAllocationSizeBinCount();
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchAllocationSizeBinUpperBound(size_t index);
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchAllocationSizeBinValue(size_t index);
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchAllocatedSizeClassCount();
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchAllocatedSizeClassUpperBound(size_t index);
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimeBenchAllocatedSizeClassValue(size_t index);
extern "C" BRIX_WITHOUT_GC void brixGCRuntimeBenchResetAllocationHistograms();
extern "C" BRIX_WITHOUT_GC size_t brixGCRuntimePauseEventCapacity();

// **********************************************************************************************

extern "C" BRIX_WITHOUT_GC uint32_t brixObjectPointer(const char* object);
extern "C" BRIX_WITHOUT_GC uint32_t brixStringHash(const char* key, size_t length);
extern "C" BRIX_WITHOUT_GC double getInfinity();

// **********************************************************************************************

extern "C" int brixopen(const char* fd, int mode, int perms);
extern "C" int brixfcntl(int fd, int cmd, int arg);
extern "C" int brixioctl(int fd, unsigned long request, struct winsize* size);
extern "C" int brixSpawnPty(
    const char* path,
    char* const* argv,
    char* const* envp,
    const char* directory,
    int columns,
    int rows,
    int* outMaster);

// **********************************************************************************************

extern "C" void dumpbuf(char* buf, size_t size);

#endif

extern "C" BRIX_WITHOUT_BLOCKING char* brixArrayViewCreate(char* data, size_t byteSize);
extern "C" BRIX_WITHOUT_GC bool brixArrayViewValid(char* owner);
extern "C" BRIX_WITHOUT_BLOCKING void brixArrayViewsInvalidate(char* data, size_t byteSize);
extern "C" BRIX_WITHOUT_GC void* brixArenaFindOwner(const void* data);
extern "C" BRIX_WITHOUT_BLOCKING void brixArrayViewsReplace(
    char* previous, size_t previousSize, char* next, size_t nextSize);
