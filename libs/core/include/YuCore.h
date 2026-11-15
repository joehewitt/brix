#ifndef YU_CORE_H
#define YU_CORE_H

#include <cstdlib>
#include <stdint.h>
#include <stddef.h>
#include <sys/dir.h>
#include <YuGC/YuGCAnnotations.h>
#include <YuGC/YuGCAllocationBuffer.h>
#include <YuGC/YuGCTemporaryRoots.h>
#include <YuCore/YuString.h>

#ifndef YU_ENABLE_YUMI_GC
#define YU_ENABLE_YUMI_GC 1
#endif

#ifndef YU_GC_NEVER_COLLECT
#define YU_GC_NEVER_COLLECT 0
#endif

#if !YU_ENABLE_YUMI_GC && YU_GC_NEVER_COLLECT
#error "YU_GC_NEVER_COLLECT requires YU_ENABLE_YUMI_GC"
#endif

#ifndef YU_ENABLE_BENCH_METRICS
#define YU_ENABLE_BENCH_METRICS 0
#endif

#define YU_MALLOC yumiMalloc
#define YU_MALLOC_ATOMIC yumiMallocAtomic
#define YU_REALLOC yumiRealloc

// **********************************************************************************************

enum : unsigned char {
    YumiInterruptTerminationRequested = 1u << 1,
    YumiInterruptSchedulerRequested = 1u << 2,
};

extern "C" unsigned char yumiInterruptRequestedFlags;
extern "C" void yumiZeroStackFrame(void* address, size_t byteCount);

// **********************************************************************************************

void* yumiMalloc(size_t size);
void* yumiMallocAtomic(size_t size);
void* yumiRealloc(void* buf, size_t size);

// **********************************************************************************************

extern "C" YUMI_GC_LEAF void yumiSaveArgs(int argc, char** argv);
extern "C" YUMI_GC_LEAF bool yumiHostIsLittleEndian();
extern "C" YUMI_GC_LEAF int yumiGetArgC();
extern "C" YUMI_GC_LEAF char** yumiGetArgV();

// *************************************************************************************************

extern "C" YUMI_GC_POLLING char* yumiAllocObject(size_t size);
extern "C" YUMI_GC_POLLING char* yumiAllocObjectDescribed(size_t size, void* descriptor);
extern "C" YUMI_GC_POLLING char* yumiAllocBuffer(size_t itemSize, size_t count);
extern "C" YUMI_GC_LEAF char* yumiTryAllocBufferWithoutSafepoint(size_t size);
extern "C" YUMI_GC_POLLING char* yumiAllocBufferDescribed(
    size_t itemSize, size_t count, void* descriptor);
extern "C" YUMI_GC_POLLING char* yumiResizeBuffer(
    char* buf, size_t oldSize, size_t newSize);
extern "C" YUMI_GC_POLLING char* yumiResizeBufferDescribed(
    char* buf, size_t oldSize, size_t newSize, void* descriptor);

// Explicit native Arena storage. Handles carry a stable slot pointer plus an incarnation. All
// entry points are GC leaves and never allocate managed memory, publish roots, or cross a GC zone.
enum : size_t {
    YumiArenaIncarnationOffset = 0,
    YumiArenaGenerationOffset = 8,
    YumiArenaLiveOffset = 16,
};

extern "C" YUMI_GC_LEAF void* yumiArenaCreate(size_t initialCapacity);
extern "C" YUMI_GC_LEAF size_t yumiArenaIncarnation(void* arena);
extern "C" YUMI_GC_LEAF char* yumiArenaReserve(
    void* arena,
    size_t incarnation,
    size_t byteCount,
    size_t alignment,
    bool zeroInitialize);
extern "C" YUMI_GC_LEAF void yumiArenaAllocationFailed(
    void* arena,
    size_t incarnation,
    size_t alignment);
extern "C" YUMI_GC_POLLING char* yumiArenaReserveDescribed(
    void* arena,
    size_t incarnation,
    size_t byteCount,
    size_t alignment,
    bool zeroInitialize,
    void* descriptor);
extern "C" YUMI_GC_LEAF const YumiStringData* yumiArenaCopyString(
    void* arena,
    size_t incarnation,
    const char* value);
extern "C" YUMI_GC_LEAF const YumiStringData* yumiArenaCopyCodepointSpan(
    void* arena,
    size_t incarnation,
    const uint32_t* source,
    size_t sourceLength,
    size_t start,
    size_t end,
    bool stopAtNull);
extern "C" YUMI_GC_LEAF const YumiStringData* yumiArenaCopyByteSpan(
    void* arena,
    size_t incarnation,
    const uint8_t* source,
    size_t sourceLength,
    size_t start,
    size_t end);
extern "C" YUMI_GC_LEAF bool yumiArenaOwns(
    void* arena,
    size_t incarnation,
    const void* value);
extern "C" YUMI_GC_LEAF size_t yumiArenaMark(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaRewind(
    void* arena,
    size_t incarnation,
    size_t mark);
extern "C" YUMI_GC_LEAF void yumiArenaReset(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF void yumiArenaRelease(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaGeneration(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaState(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaActiveBlockCount(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaRetainedBlockCount(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaActiveBytes(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaReservedBytes(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaRetainedBytes(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaPeakActiveBytes(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaOwnerSystemBlockAllocationCount(
    void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaOwnerResetCount(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaOwnerRewindCount(void* arena, size_t incarnation);
extern "C" YUMI_GC_LEAF size_t yumiArenaSystemBlockAllocationCount(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaSystemBlockFreeCount(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaRuntimeActiveBytes(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaRuntimeReservedBytes(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaRuntimeRetainedBytes(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaRuntimePeakActiveBytes(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaMarkCount(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaRewindCount(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaResetCount(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaSlotCount(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaLiveSlotCount(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaManagedAllocationCount(void);
extern "C" YUMI_GC_LEAF size_t yumiArenaZoneTransitionCount(void);
// Per-task storage for parser-only values. Marks are logical cursors: resetting a mark invalidates
// every later region allocation after observable results have been promoted to the GC heap.
extern "C" YUMI_GC_LEAF size_t yumiParseRegionMark(void);
extern "C" YUMI_GC_LEAF void yumiParseRegionReset(size_t mark);
extern "C" YUMI_GC_LEAF void yumiParseRegionEnterAllocationScope(
    size_t line, size_t column);
extern "C" YUMI_GC_LEAF void yumiParseRegionLeaveAllocationScope(void);
extern "C" YUMI_GC_LEAF size_t yumiParseRegionSuspendAllocationScopes(void);
extern "C" YUMI_GC_LEAF void yumiParseRegionResumeAllocationScopes(size_t depth);
extern "C" YUMI_GC_LEAF void yumiParseRegionCancelAllocationScopes(void);
extern "C" YUMI_GC_LEAF char* yumiParseRegionAllocObject(
    size_t size, void* descriptor);
extern "C" YUMI_GC_LEAF char* yumiParseRegionAllocBuffer(
    size_t itemSize, size_t count, void* descriptor);
extern "C" YUMI_GC_POLLING char* yumiParseRegionPromote(char* pointer);
extern "C" YUMI_GC_POLLING void yumiParseRegionPromoteValue(
    char* value, size_t byteSize, void* descriptor);
extern "C" YUMI_GC_LEAF bool yumiParseRegionAllocationActive(void);
extern "C" YUMI_GC_LEAF bool yumiParseRegionOwns(char* pointer);
extern "C" YUMI_GC_LEAF size_t yumiParseRegionObjectAllocationCount(void);
extern "C" YUMI_GC_LEAF size_t yumiParseRegionBufferAllocationCount(void);
extern "C" YUMI_GC_LEAF size_t yumiParseRegionPromotionCount(void);
extern "C" YUMI_GC_LEAF void yumiParseRegionValidateStore(
    char* source, char* value, size_t line, size_t column);
extern "C" YUMI_GC_LEAF void yumiParseRegionValidateBulkStore(
    char* source,
    char* value,
    size_t byteSize,
    void* descriptor,
    size_t line,
    size_t column);

extern "C" YUMI_GC_LEAF bool yumiGCAllocationSiteKeyInit(
    YumiGCAllocationSiteKey* key,
    size_t requestedSize,
    void* descriptor,
    size_t pointerKind,
    size_t zeroingPolicy);
extern "C" YUMI_GC_POLLING char* yumiAllocObjectWithSiteKey(
    const YumiGCAllocationSiteKey* key);
extern "C" {
extern thread_local YumiGCAllocationBuffer* yumiGCActiveAllocationBuffer;
extern thread_local YumiGCThreadCache* yumiGCActiveThreadCache;
extern uint64_t yumiGCPublicControlWord;
}
extern "C" YUMI_GC_LEAF void yumiGCSetActiveThreadCache(YumiGCThreadCache* cache);
extern "C" YUMI_GC_LEAF YumiGCThreadCache* yumiGCGetActiveThreadCache();
extern "C" YUMI_GC_LEAF void yumiGCSetActiveAllocationBufferStorage(
    YumiGCRuntimeAllocationBuffer* storage);
extern "C" YUMI_GC_LEAF YumiGCAllocationBuffer* yumiGCGetActiveAllocationBuffer();
extern "C" YUMI_GC_POLLING char* yumiGCAllocationBufferRefill(
    const YumiGCAllocationSiteKey* key);
extern "C" YUMI_GC_POLLING char* yumiGCCompilerAllocationBufferRefill(
    const YumiGCAllocationSiteKey* key);
extern "C" YUMI_GC_LEAF void yumiGCAllocationBufferFlush();
extern "C" YUMI_GC_LEAF void yumiGCAllocationBufferPublish();
extern "C" YUMI_GC_LEAF void yumiGCAllocationBufferRevoke();
extern "C" YUMI_GC_LEAF size_t yumiGCAllocationBufferCursorOffset();
extern "C" YUMI_GC_LEAF size_t yumiGCAllocationBufferLimitOffset();
extern "C" YUMI_GC_LEAF size_t yumiGCAllocationBufferSize();
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeAllocationBufferHotOffset();
extern "C" void* yumiAddRoot(char* pointer);
extern "C" void* yumiAddTemporaryRoot(char* pointer);
extern "C" void* yumiAddDescribedRoot(char* pointer, size_t byteSize, void* descriptor);
extern "C" void yumiUpdateRoot(void* root, char* pointer);
extern "C" void yumiRemoveRoot(void* root);
extern "C" void yumiRemoveTemporaryRoot(void* root);
extern "C" void yumiDrainTemporaryRoots(YumiGCTemporaryRootStorage* storage);
#ifndef YU_GC_COLLECT_RESULT_TYPE
#define YU_GC_COLLECT_RESULT_TYPE
typedef size_t YumiGCCollectResult;
enum : YumiGCCollectResult {
    YumiGCCollectCompleted = 0
};
#endif
// Non-zero results encode a stalled task as task-id + 1, so zero is unambiguously completed.
extern "C" YUMI_GC_POLLING YumiGCCollectResult yumiCollect();
extern "C" YUMI_GC_POLLING void yumiGCFrameBudget(uint64_t remainingNanos);
extern "C" YUMI_GC_POLLING bool yumiGCRuntimeIdleCollect(uint64_t budgetNanos);
// Internal scalar weak subscriptions used by core property tracking. These live in the GC
// runtime, but the narrow declarations here keep core modules from importing the broad GC API.
extern "C" YUMI_GC_POLLING size_t yumiGCRegisterOwnerWeak(char* owner, char* target);
extern "C" YUMI_GC_POLLING char* yumiGCOwnerWeakLoad(size_t handle);
extern "C" YUMI_GC_POLLING bool yumiGCReleaseOwnerWeak(size_t handle);
// Narrow benchmark observability kept here so core workloads need not import the broad GC API.
extern "C" YUMI_GC_LEAF size_t yumiGCObjectSidePayloadCount(size_t lane);
// Compiler-elided outside the internal A/B/C benchmark runtime.
extern "C" void yumiBenchCollect();
extern "C" YUMI_GC_LEAF void yumiWriteBarrier(char* source, char* value);
extern "C" YUMI_GC_LEAF void yumiWriteBarrierRecord(char* source, char* value);
extern "C" void yumiBenchRecordBarrierCall(void);
extern "C" YUMI_GC_LEAF void yumiBulkWriteBarrierFast(
    char* source, char* value, size_t byteSize, void* descriptor);

extern "C" YUMI_GC_LEAF unsigned int yumiGenerateUID();

// *************************************************************************************************

extern "C" YUMI_GC_POLLING char* yumiArrayCopySlice(
    char* self, const void* source, size_t size);
extern "C" YUMI_GC_POLLING char* yumiArrayCopySliceDescribed(
    char* self, const void* source, size_t size, void* descriptor);
extern "C" char* yumiArraySlice(char* self, size_t start, size_t end);

// *************************************************************************************************

extern "C" int yumiTerminalEnter(int fd, struct termios* oldState);
extern "C" int yumiTerminalExit(int fd, struct termios* state);
extern "C" int yumiTerminalSetSignals(int fd, int enabled);

struct YuDirEntry {
    uint64_t inode;
    uint32_t nameLength;
    uint32_t kind;
};

extern "C" bool yumiReadDirEntry(DIR* dir, char* buf, YuDirEntry* out);

// *************************************************************************************************

extern "C" YUMI_GC_LEAF int yumiGetErrNo();

extern "C" void yumiPrint(const char* text);
extern "C" void yumiTerminate();
extern "C" YUMI_GC_LEAF void yumiCrash();
extern "C" unsigned char yumiBenchMetricsEnabledFlag;
extern "C" void yumiBenchInitMetrics();
extern "C" void yumiBenchRecordAllocation(size_t size);
extern "C" YUMI_GC_LEAF void yumiBenchRecordAllocationSite(
    size_t siteId,
    const char* source,
    size_t line,
    size_t column,
    const char* function,
    const char* type,
    const char* operation,
    const char* pointerKind);
extern "C" YUMI_GC_LEAF void yumiBenchMutatorPhaseStart(const char* name);
extern "C" YUMI_GC_LEAF void yumiBenchMutatorPhaseEnd(const char* name);
extern "C" void yumiBenchMaybeWriteMetrics();
extern "C" void yumiBenchRecordProcessMemory(void);
extern "C" YUMI_GC_POLLING size_t yumiBenchTemporaryRootShim(
    char* pointer, size_t rootCount, size_t iterations);
extern "C" YUMI_GC_LEAF void yumiBenchResetTemporaryRootMetrics(void);
extern "C" YUMI_GC_LEAF void yumiBenchResetActiveTemporaryRootMetrics(void);
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeMode();
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchGCCpuNanos();
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchAllocationSiteCount();
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchAllocationSiteValue(size_t index);
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchAllocationSizeBinCount();
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchAllocationSizeBinUpperBound(size_t index);
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchAllocationSizeBinValue(size_t index);
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchAllocatedSizeClassCount();
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchAllocatedSizeClassUpperBound(size_t index);
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimeBenchAllocatedSizeClassValue(size_t index);
extern "C" YUMI_GC_LEAF void yumiGCRuntimeBenchResetAllocationHistograms();
extern "C" YUMI_GC_LEAF size_t yumiGCRuntimePauseEventCapacity();

// **********************************************************************************************

extern "C" YUMI_GC_LEAF uint32_t yumiObjectPointer(const char* object);
extern "C" YUMI_GC_LEAF uint32_t yumiStringHash(const char* key, size_t length);
extern "C" YUMI_GC_LEAF double getInfinity();

// **********************************************************************************************

extern "C" int yumiopen(const char* fd, int mode, int perms);
extern "C" int yumifcntl(int fd, int cmd, int arg);
extern "C" int yumiioctl(int fd, unsigned long request, struct winsize* size);
extern "C" int yumiSpawnPty(
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

extern "C" YUMI_GC_POLLING char* yumiArrayViewCreate(char* data, size_t byteSize);
extern "C" YUMI_GC_LEAF bool yumiArrayViewValid(char* owner);
extern "C" YUMI_GC_POLLING void yumiArrayViewsInvalidate(char* data, size_t byteSize);
extern "C" YUMI_GC_LEAF void* yumiArenaFindOwner(const void* data);
extern "C" YUMI_GC_POLLING void yumiArrayViewsReplace(
    char* previous, size_t previousSize, char* next, size_t nextSize);
