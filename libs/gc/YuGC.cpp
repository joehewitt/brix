#include "YuGCHeap.h"
#include "YuGCLockOrder.h"
#include <YuGC/YuGCCollectionObserverFilter.h>
#include <YuGC/YuGCTemporaryRoots.h>
#include "YuGCWork.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <new>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <sys/resource.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach/mach.h>
#endif

#ifndef YU_GC_NEVER_COLLECT
#define YU_GC_NEVER_COLLECT 0
#endif

extern "C" void yumiGCScanCurrentStack(void (*visitor)(void*, void*, void*), void* data);
extern "C" void yumiGCScanTaskStacks(void (*visitor)(void*, void*, void*), void* data);
extern "C" void __attribute__((weak)) yumiGCScanCurrentStackConservative(
    void (*visitor)(void*, void*, void*),
    void* data) {
    (void)visitor;
    (void)data;
}

extern "C" void __attribute__((weak)) yumiGCScanTaskStacksConservative(
    void (*visitor)(void*, void*, void*),
    void* data) {
    (void)visitor;
    (void)data;
}

extern "C" void* __attribute__((weak)) yumiGCDebugConservativeRootSlot(void) {
    return nullptr;
}

extern "C" bool __attribute__((weak)) yumiGCVisitingConservativeRoot(void) {
    return false;
}
extern "C" void yumiGCScanStorageRoots(void (*visitor)(void*, void*, void*), void* data);
extern "C" YumiGCTemporaryRootStorage* __attribute__((weak))
yumiGCActiveTaskTemporaryRootStorage(void) {
    return nullptr;
}
extern "C" void __attribute__((weak)) yumiGCVisitTaskTemporaryRootLists(
    void (*)(void*, void*),
    void*) {
}
extern "C" void __attribute__((weak)) yumiGCVisitTaskParseRegionAllocations(
    void (*)(void*, size_t, void*, void*),
    void*) {
}
extern "C" void __attribute__((weak)) yumiGCVisitActiveTaskParseRegionAllocations(
    void (*)(void*, size_t, void*, void*),
    void*) {
}
extern "C" void __attribute__((weak)) yumiArenaVisitTracedReservations(
    void (*)(void*, size_t, void*, void*),
    void*) {
}
extern "C" void yumiGCStackMapLinkAnchor(void);
extern "C" void yumiGCHandshakeBegin(uint32_t reason);
extern "C" void yumiGCHandshakeEnd(void);
extern "C" size_t __attribute__((weak)) yumiGCHandshakeTasksWaited(void) {
    return 0;
}
extern "C" uint64_t __attribute__((weak)) yumiGCHandshakeSlowestTaskNanos(void) {
    return 0;
}
extern "C" bool __attribute__((weak)) yumiForkGCCleanupTask(
    void (*)(void*),
    void*) {
    return false;
}
extern "C" void yumiTerminateActiveTask(void) __attribute__((weak));

// Test harnesses may stop the sweep coordinator at the exact closed-publisher/public-
// MarkComplete seam. The production definition is deliberately empty.
extern "C" void __attribute__((weak)) yumiGCDebugSweepTransitionAfterPublisherClose(void) {
}
extern "C" void __attribute__((weak)) yumiGCDebugExternalMarkObservedClosedGate(void) {
}
extern "C" void __attribute__((weak)) yumiGCDebugSweepTransitionBeforeCursorReset(void) {
}
extern "C" void __attribute__((weak)) yumiGCDebugSweepTransitionAfterPublicSweep(void) {
}
extern "C" void __attribute__((weak)) yumiGCDebugAllocationObservedUnpublishedSweep(void) {
}
extern "C" void __attribute__((weak)) yumiGCDebugAllocationAfterAdmission(void) {
}
extern "C" void __attribute__((weak)) yumiGCDebugMetadataRetirementWaitingForAllocation(void) {
}
extern "C" void __attribute__((weak)) yumiGCDebugBatchReservationAfterSpanPublication(
    char*,
    size_t,
    size_t) {
}
extern "C" void __attribute__((weak)) yumiGCDebugTerminationBeforeConfirm(void) {
}
namespace YuGC {

static constexpr bool
yumiGCCollectionDisabled() {
    return YU_GC_NEVER_COLLECT != 0;
}

static std::atomic<size_t> gCollectionObserverActiveCount {0};
static YumiCollectionObserverFilter gCollectionObserverFilter;
static std::array<std::atomic<size_t>, kObjectSidePayloadLaneCount>
    gObjectSidePayloadCounts {};
static std::array<std::atomic<YumiGCCleanupCallback>, kObjectSidePayloadLaneCount>
    gObjectSidePayloadCleanupCallbacks {};
static std::atomic<size_t> gObjectSideCleanupPayloadCount {0};

void
recordObjectSidePayloadPublish(size_t lane) {
    if (lane < kObjectSidePayloadLaneCount) {
        gObjectSidePayloadCounts[lane].fetch_add(1, std::memory_order_relaxed);
    }
}

void
recordObjectSidePayloadClear(size_t lane) {
    if (lane >= kObjectSidePayloadLaneCount) {
        return;
    }
    size_t count = gObjectSidePayloadCounts[lane].load(std::memory_order_relaxed);
    while (count) {
        if (gObjectSidePayloadCounts[lane].compare_exchange_weak(
                count,
                count - 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return;
        }
    }
    abortGC("GC side-payload count underflow");
}

void
recordObjectSideCleanupPayloadPublish() {
    gObjectSideCleanupPayloadCount.fetch_add(1, std::memory_order_relaxed);
}

void
recordObjectSideCleanupPayloadClear() {
    size_t count = gObjectSideCleanupPayloadCount.load(std::memory_order_relaxed);
    while (count) {
        if (gObjectSideCleanupPayloadCount.compare_exchange_weak(
                count,
                count - 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return;
        }
    }
    abortGC("GC cleanup side-payload count underflow");
}

size_t
objectSidePayloadCount(size_t lane) {
    return lane < kObjectSidePayloadLaneCount
        ? gObjectSidePayloadCounts[lane].load(std::memory_order_relaxed)
        : 0;
}

size_t
objectSideCleanupPayloadCount() {
    return gObjectSideCleanupPayloadCount.load(std::memory_order_acquire);
}

static YumiGCCleanupCallback
objectSidePayloadCleanupCallback(size_t lane) {
    return lane < kObjectSidePayloadLaneCount
        ? gObjectSidePayloadCleanupCallbacks[lane].load(std::memory_order_acquire)
        : nullptr;
}

static bool
installObjectSidePayloadCleanupCallback(size_t lane, YumiGCCleanupCallback callback) {
    if (lane >= kObjectSidePayloadLaneCount || !callback) {
        return false;
    }
    YumiGCCleanupCallback existing = objectSidePayloadCleanupCallback(lane);
    if (existing) {
        return existing == callback;
    }
    return gObjectSidePayloadCleanupCallbacks[lane].compare_exchange_strong(
        existing,
        callback,
        std::memory_order_release,
        std::memory_order_acquire) || existing == callback;
}

static size_t
envSizeIfSet(const char* name) {
    const char* value = std::getenv(name);
    if (!value || !*value) {
        return 0;
    }
    char* end = nullptr;
    unsigned long long parsed = std::strtoull(value, &end, 10);
    return end && end != value ? static_cast<size_t>(parsed) : 0;
}

struct BatchCacheCaps {
    size_t threadBytes;
    size_t globalBytes;
    size_t effectiveBytes;
};

static const BatchCacheCaps&
batchCacheCaps() {
    static const BatchCacheCaps caps = []() {
        size_t threadBytes = envSizeIfSet("YUMI_GC_THREAD_BATCH_CACHE_BYTES");
        size_t globalBytes = envSizeIfSet("YUMI_GC_GLOBAL_BATCH_CACHE_BYTES");
        size_t effectiveBytes = threadBytes;
        if (globalBytes) {
            effectiveBytes = effectiveBytes
                ? std::min(effectiveBytes, globalBytes)
                : globalBytes;
        }
        return BatchCacheCaps { threadBytes, globalBytes, effectiveBytes };
    }();
    return caps;
}

static bool
envFlagIsSet(const char* name) {
    const char* value = std::getenv(name);
    return value && *value && std::strcmp(value, "0") != 0;
}

static void
clampBatchCountToEnvCaps(size_t slotSize, size_t& count) {
    if (!slotSize || !count) {
        return;
    }
    size_t byteCap = batchCacheCaps().effectiveBytes;
    if (!byteCap) {
        return;
    }
    size_t capCount = byteCap / slotSize;
    if (capCount && count > capCount) {
        count = capCount;
    }
}

static void
clampBatchCountToHeapGoal(size_t slotSize, size_t& count) {
    if (!slotSize || !count) {
        return;
    }
    size_t goal = yumiGCRuntimeHeapGoal();
    if (!goal) {
        return;
    }
    size_t byteLimit = std::max(slotSize, goal / 8);
    size_t capCount = byteLimit / slotSize;
    if (capCount && count > capCount) {
        count = capCount;
    }
}

static bool
reserveBatchCacheForCount(size_t slotSize, size_t& count) {
    while (count) {
        if (count > std::numeric_limits<size_t>::max() / slotSize) {
            count /= 2;
            continue;
        }
        if (yumiGCRuntimeCanReserveBatchCache(count * slotSize)) {
            return true;
        }
        count /= 2;
    }
    return false;
}

} // namespace YuGC

namespace {

using namespace YuGC;

static void markPointer(char* pointer);
static void traceDescriptor(char* object, void* descriptor, size_t actualObjectSize);

struct Root {
    char* pointer = nullptr;
    void* descriptor = nullptr;
    size_t byteSize = 0;
    Root** previousLink = nullptr;
    Root* next = nullptr;
    YumiGCTemporaryRootStorage* temporaryStorage = nullptr;
};

static constexpr size_t kTemporaryRootSlabCapacity = 32;

struct TemporaryRootSlab {
    TemporaryRootSlab* next = nullptr;
    Root roots[kTemporaryRootSlabCapacity];
};

static_assert(
    sizeof(Root) <= YumiGCTemporaryRootSlotBytes,
    "temporary-root inline slots must fit the collector Root representation");
static_assert(
    alignof(Root) <= alignof(YumiGCTemporaryRootSlot),
    "temporary-root inline slots must preserve Root alignment");

static void
initializeTemporaryRootStorage(YumiGCTemporaryRootStorage* storage) {
    if (storage->initialized) {
        return;
    }
    for (size_t index = 0; index < YumiGCTemporaryRootInlineCapacity; ++index) {
        Root* root = new (storage->inlineSlots[index].bytes) Root();
        root->temporaryStorage = storage;
        root->next = static_cast<Root*>(storage->freeList);
        storage->freeList = root;
    }
    storage->initialized = true;
}

static Root*
acquireTemporaryRoot(YumiGCTemporaryRootStorage* storage) {
    initializeTemporaryRootStorage(storage);
    if (!storage->freeList) {
        TemporaryRootSlab* slab = new (std::nothrow) TemporaryRootSlab();
        if (!slab) {
            return nullptr;
        }
        slab->next = static_cast<TemporaryRootSlab*>(storage->slabHead);
        storage->slabHead = slab;
        ++storage->storageAllocationCount;
        ++storage->slabGrowthCount;
        for (Root& root : slab->roots) {
            root.temporaryStorage = storage;
            root.next = static_cast<Root*>(storage->freeList);
            storage->freeList = &root;
        }
    }
    Root* root = static_cast<Root*>(storage->freeList);
    storage->freeList = root->next;
    root->pointer = nullptr;
    root->descriptor = nullptr;
    root->byteSize = 0;
    root->previousLink = nullptr;
    root->next = nullptr;
    root->temporaryStorage = storage;
    return root;
}

static void
releaseTemporaryRoot(Root* root) {
    YumiGCTemporaryRootStorage* storage = root->temporaryStorage;
    root->pointer = nullptr;
    root->descriptor = nullptr;
    root->byteSize = 0;
    root->previousLink = nullptr;
    root->next = static_cast<Root*>(storage->freeList);
    storage->freeList = root;
}

static void
destroyTemporaryRootStorage(YumiGCTemporaryRootStorage* storage) {
    TemporaryRootSlab* slab = static_cast<TemporaryRootSlab*>(storage->slabHead);
    while (slab) {
        TemporaryRootSlab* next = slab->next;
        delete slab;
        slab = next;
    }
    storage->slabHead = nullptr;
    storage->freeList = nullptr;
    if (storage->initialized) {
        for (size_t index = 0; index < YumiGCTemporaryRootInlineCapacity; ++index) {
            reinterpret_cast<Root*>(storage->inlineSlots[index].bytes)->~Root();
        }
        storage->initialized = false;
    }
}

static constexpr size_t kLifetimeRegistryShardCount = 32;

static_assert(sizeof(size_t) == sizeof(uint64_t), "owner-weak handles need 64 bits");
static constexpr size_t kOwnerWeakShardBits = 5;
static_assert(
    (size_t {1} << kOwnerWeakShardBits) == kLifetimeRegistryShardCount,
    "owner-weak shard bits drifted");
static constexpr size_t kOwnerWeakSlotBits = 32;
static constexpr size_t kOwnerWeakGenerationShift = kOwnerWeakShardBits + kOwnerWeakSlotBits;
static constexpr uint32_t kOwnerWeakGenerationLimit =
    uint32_t {1} << (64 - kOwnerWeakGenerationShift);
static constexpr uint32_t kOwnerWeakRetiredGeneration = 0;
static constexpr size_t kOwnerWeakFirstSegmentShift = 8;
static constexpr size_t kOwnerWeakSegmentCount = kOwnerWeakSlotBits - kOwnerWeakFirstSegmentShift;
static constexpr uint32_t kOwnerWeakNoFreeSlot = std::numeric_limits<uint32_t>::max();

struct OwnerWeakSlot {
    std::atomic<uint32_t> generation {0};
    uint32_t nextFree = kOwnerWeakNoFreeSlot;
    std::atomic<char*> target {nullptr};
    char* owner = nullptr;
    GCSpan* ownerSpan = nullptr;
    GCSpan* targetSpan = nullptr;
};

struct OwnerWeakShard {
    std::array<std::atomic<OwnerWeakSlot*>, kOwnerWeakSegmentCount> segments {};
    uint32_t slotCount = 0;
    uint32_t freeHead = kOwnerWeakNoFreeSlot;
};

struct CleanupRegistration {
    size_t handle = 0;
    char* target = nullptr;
    GCSpan* targetSpan = nullptr;
    char* context = nullptr;
    void* contextRoot = nullptr;
    YumiGCCleanupCallback callback = nullptr;
};

struct CleanupInvocation {
    char* context = nullptr;
    void* contextRoot = nullptr;
    YumiGCCleanupCallback callback = nullptr;
};

static std::array<std::mutex, kLifetimeRegistryShardCount> gWeakRegistryMutexes;
static std::array<OwnerWeakShard, kLifetimeRegistryShardCount> gOwnerWeakShards;
static std::array<std::mutex, kLifetimeRegistryShardCount> gCleanupRegistryMutexes;
static std::array<std::vector<CleanupRegistration>, kLifetimeRegistryShardCount>
    gCleanupRegistries;
static std::array<std::unordered_map<size_t, size_t>, kLifetimeRegistryShardCount>
    gCleanupRegistryIndices;
static std::mutex gCleanupQueueMutex;
static std::recursive_mutex gCleanupDrainMutex;
static std::deque<CleanupInvocation> gCleanupQueue;
static std::vector<CleanupInvocation> gInFlightCleanups; // guarded by gCleanupQueueMutex
static thread_local bool gInvokingCleanup = false;
// Managed cleanup callbacks cannot safely enter blocking C calls while returning
// through the GC callback ABI. File cleanup queues one descriptor here; the
// serialized drain closes it immediately after the managed callback returns.
static thread_local int gCleanupFileDescriptor = -1;
static std::atomic<size_t> gNextCleanupHandle {1};
static std::atomic<bool> gCleanupTaskScheduled {false};
static std::atomic<size_t> gOwnerWeakRegistrationCount {0};
static std::atomic<size_t> gWeakClearCount {0};
static std::atomic<size_t> gCleanupRegistrationCount {0};
static std::atomic<size_t> gCleanupPendingCount {0};
static std::atomic<size_t> gCleanupEnqueueCount {0};
static std::atomic<size_t> gCleanupFireCount {0};
static std::atomic<size_t> gCleanupCancelCount {0};
static std::atomic<size_t> gCleanupExceptionCount {0};
static std::atomic<size_t> gCleanupSidePublishCount {0};
static std::atomic<size_t> gCleanupSideValidationNanos {0};
static std::atomic<size_t> gCleanupSideValidationObjectCount {0};
static std::atomic<size_t> gCleanupSideValidationEdgeCount {0};
static std::atomic<size_t> gTestCleanupCancelLookupCount {0};
static std::atomic<size_t> gTestRootUnlinkCount {0};
static std::atomic<size_t> gTemporaryRootPushCount {0};
static std::atomic<size_t> gTemporaryRootPopCount {0};
static std::atomic<size_t> gTemporaryRootStorageAllocationCount {0};
static std::atomic<size_t> gTemporaryRootSlabGrowthCount {0};
static std::atomic<size_t> gTemporaryRootMaxDepth {0};

static size_t
lifetimeRegistryShard(const void* pointer) {
    uintptr_t value = reinterpret_cast<uintptr_t>(pointer);
    return (value >> 3) % kLifetimeRegistryShardCount;
}

static size_t
ownerWeakHandle(size_t shard, uint32_t index, uint32_t generation) {
    return (size_t {generation} << kOwnerWeakGenerationShift) |
        (size_t {index} << kOwnerWeakShardBits) |
        shard;
}

static size_t
ownerWeakHandleShard(size_t handle) {
    return handle & (kLifetimeRegistryShardCount - 1);
}

static uint32_t
ownerWeakHandleIndex(size_t handle) {
    return static_cast<uint32_t>(handle >> kOwnerWeakShardBits);
}

static uint32_t
ownerWeakHandleGeneration(size_t handle) {
    return static_cast<uint32_t>(handle >> kOwnerWeakGenerationShift);
}

static size_t
ownerWeakSegmentTop(uint32_t index) {
    size_t biased = size_t {index} + (size_t {1} << kOwnerWeakFirstSegmentShift);
    return 63 - static_cast<size_t>(__builtin_clzll(biased));
}

static OwnerWeakSlot*
ownerWeakSlot(const OwnerWeakShard& shard, uint32_t index) {
    size_t top = ownerWeakSegmentTop(index);
    size_t segment = top - kOwnerWeakFirstSegmentShift;
    if (segment >= kOwnerWeakSegmentCount) {
        return nullptr;
    }
    OwnerWeakSlot* slots = shard.segments[segment].load(std::memory_order_acquire);
    if (!slots) {
        return nullptr;
    }
    return slots + (size_t {index} + (size_t {1} << kOwnerWeakFirstSegmentShift) -
        (size_t {1} << top));
}

static OwnerWeakSlot*
allocateOwnerWeakSlot(OwnerWeakShard& shard, uint32_t& index) {
    if (shard.freeHead != kOwnerWeakNoFreeSlot) {
        index = shard.freeHead;
        OwnerWeakSlot* slot = ownerWeakSlot(shard, index);
        shard.freeHead = slot->nextFree;
        slot->nextFree = kOwnerWeakNoFreeSlot;
        return slot;
    }
    index = shard.slotCount;
    size_t top = ownerWeakSegmentTop(index);
    size_t segment = top - kOwnerWeakFirstSegmentShift;
    if (segment >= kOwnerWeakSegmentCount) {
        return nullptr;
    }
    if (!shard.segments[segment].load(std::memory_order_relaxed)) {
        OwnerWeakSlot* slots = new (std::nothrow) OwnerWeakSlot[size_t {1} << top];
        if (!slots) {
            return nullptr;
        }
        shard.segments[segment].store(slots, std::memory_order_release);
    }
    shard.slotCount = index + 1;
    OwnerWeakSlot* slot = ownerWeakSlot(shard, index);
    slot->generation.store(1, std::memory_order_relaxed);
    return slot;
}

static void
freeOwnerWeakSlot(OwnerWeakShard& shard, OwnerWeakSlot& slot, uint32_t index) {
    uint32_t generation = slot.generation.load(std::memory_order_relaxed) + 1;
    if (generation >= kOwnerWeakGenerationLimit) {
        generation = kOwnerWeakRetiredGeneration;
    }
    slot.generation.store(generation, std::memory_order_release);
    slot.target.store(nullptr, std::memory_order_relaxed);
    slot.owner = nullptr;
    slot.ownerSpan = nullptr;
    slot.targetSpan = nullptr;
    if (generation != kOwnerWeakRetiredGeneration) {
        slot.nextFree = shard.freeHead;
        shard.freeHead = index;
    }
}

struct LifetimeTerminationLocks {
    std::array<std::unique_lock<std::mutex>, kLifetimeRegistryShardCount> weak;
    std::array<std::unique_lock<std::mutex>, kLifetimeRegistryShardCount> cleanup;
    bool scanWeak;
    bool scanCleanup;

    LifetimeTerminationLocks()
        : scanWeak(gOwnerWeakRegistrationCount.load(std::memory_order_acquire) != 0),
          scanCleanup(gCleanupRegistrationCount.load(std::memory_order_acquire) != 0) {
        if (scanWeak) {
            for (size_t index = 0; index < kLifetimeRegistryShardCount; ++index) {
                weak[index] = std::unique_lock<std::mutex>(gWeakRegistryMutexes[index]);
            }
        }
        if (scanCleanup) {
            for (size_t index = 0; index < kLifetimeRegistryShardCount; ++index) {
                cleanup[index] = std::unique_lock<std::mutex>(gCleanupRegistryMutexes[index]);
            }
        }
    }
};

static std::mutex gRootMutex;
static Root* gFirstRoot = nullptr;
static std::atomic<char*> gCheckmarkTestExtraRoot {nullptr};
static std::atomic<bool> gInitialized {false};
static unsigned char gPhase = kPhaseIdle;
static uint32_t gCycle = 0;
static size_t gActiveAllocationCount = 0;
static GCMarkWork& gMarkWork = markWork();
static std::atomic<size_t> gTerminationConfirmCount {0};
static std::atomic<size_t> gTerminationConfirmResumeCount {0};
static std::atomic<size_t> gTerminationConfirmTaskCount {0};
static constexpr size_t kSweepTransitionAccepting = size_t {1} <<
    (sizeof(size_t) * 8 - 1);
static constexpr size_t kSweepTransitionCountMask = kSweepTransitionAccepting - 1;
static std::atomic<size_t> gSweepTransitionPublishers {0};

static void
setMarkQueueDepth() {
    yumiGCRuntimeSetMarkQueueDepth(gMarkWork.depth());
}

static GCVecState*
vecState(const GCVec& vec) {
    if (!vec.stateSlot) {
        return nullptr;
    }
    return *vec.stateSlot;
}

static size_t
vecLength(const GCVec& vec) {
    GCVecState* state = vecState(vec);
    return state ? state->length : 0;
}

static size_t
vecSizeAt(const GCVec& vec, size_t index) {
    GCVecState* state = vecState(vec);
    if (!state || index >= state->length || !state->items) {
        return 0;
    }
    return static_cast<size_t*>(state->items)[index];
}

static void*
vecPointerAt(const GCVec& vec, size_t index) {
    GCVecState* state = vecState(vec);
    if (!state || index >= state->length || !state->items) {
        return nullptr;
    }
    return static_cast<void**>(state->items)[index];
}

struct DescriptorWalkFrame {
    char* object;
    void* descriptor;
    size_t actualSize;
    char* owner;
};

struct GCVecView {
    void* items;
    size_t length;
};

static GCVecView
vecView(const GCVec& vec) {
    GCVecView view = {nullptr, 0};
    GCVecState* state = vecState(vec);
    if (state && state->items) {
        view.items = state->items;
        view.length = state->length;
    }
    return view;
}

static constexpr size_t kDescriptorWalkNativeDepth = 8;

template<typename Visitor>
static void walkDescriptorChild(
    std::vector<DescriptorWalkFrame>& spill,
    const DescriptorWalkFrame& child,
    size_t depth,
    Visitor& visitor);

template<typename Visitor>
static void
walkDescriptorFrame(
    std::vector<DescriptorWalkFrame>& spill,
    const DescriptorWalkFrame& frame,
    size_t depth,
    Visitor& visitor
) {
    GCDescriptor* desc = static_cast<GCDescriptor*>(frame.descriptor);
    size_t effectiveSize = desc->objectSize == 0 && desc->isBuffer
        ? frame.actualSize
        : desc->objectSize;
    if (effectiveSize == 0 || effectiveSize > frame.actualSize) {
        return;
    }

    GCVecView offsets = vecView(desc->pointerOffsets);
    const size_t* offsetItems = static_cast<const size_t*>(offsets.items);
    for (size_t index = 0; index < offsets.length; ++index) {
        visitor(frame.object, frame.descriptor, effectiveSize, offsetItems[index], frame.owner);
    }

    GCVecView ranges = vecView(desc->unionRanges);
    for (size_t index = 0; index < ranges.length; ++index) {
        GCUnionRange* range = static_cast<GCUnionRange**>(ranges.items)[index];
        if (!range || range->tagOffset >= effectiveSize ||
                range->payloadOffset > effectiveSize) {
            continue;
        }
        size_t tag = static_cast<unsigned char>(frame.object[range->tagOffset]);
        GCVecView cases = vecView(range->caseDescriptors);
        if (tag >= cases.length) {
            continue;
        }
        void* childDescriptor = static_cast<void**>(cases.items)[tag];
        if (childDescriptor) {
            DescriptorWalkFrame child = {
                frame.object + range->payloadOffset,
                childDescriptor,
                effectiveSize - range->payloadOffset,
                frame.owner
            };
            walkDescriptorChild(spill, child, depth, visitor);
        }
    }

    if (!desc->isBuffer || desc->elementSize == 0) {
        return;
    }
    GCVecView elementOffsets = vecView(desc->elementPointerOffsets);
    const size_t* elementOffsetItems = static_cast<const size_t*>(elementOffsets.items);
    void* elementDescriptor = desc->elementDescriptor;
    for (size_t elementOffset = 0;
            elementOffset + desc->elementSize <= effectiveSize;
            elementOffset += desc->elementSize) {
        char* element = frame.object + elementOffset;
        for (size_t index = 0; index < elementOffsets.length; ++index) {
            visitor(
                element,
                frame.descriptor,
                desc->elementSize,
                elementOffsetItems[index],
                frame.owner);
        }
        if (elementDescriptor) {
            DescriptorWalkFrame child = {element, elementDescriptor, desc->elementSize, frame.owner};
            walkDescriptorChild(spill, child, depth, visitor);
        }
    }
}

template<typename Visitor>
static void
walkDescriptorChild(
    std::vector<DescriptorWalkFrame>& spill,
    const DescriptorWalkFrame& child,
    size_t depth,
    Visitor& visitor
) {
    if (depth + 1 < kDescriptorWalkNativeDepth) {
        walkDescriptorFrame(spill, child, depth + 1, visitor);
    } else {
        spill.push_back(child);
    }
}

// Descriptor nesting is language data, not control flow. Nesting past a fixed native depth
// continues from an explicit worklist so deeply nested buffer/union descriptors cannot consume
// the native C stack.
template<typename Visitor>
static void
walkDescriptorFields(
    char* object,
    void* descriptor,
    size_t actualObjectSize,
    char* owner,
    Visitor visitor
) {
    if (!object || !descriptor || actualObjectSize == 0) {
        return;
    }
    std::vector<DescriptorWalkFrame> spill;
    DescriptorWalkFrame root = {object, descriptor, actualObjectSize, owner};
    walkDescriptorFrame(spill, root, 0, visitor);
    while (!spill.empty()) {
        DescriptorWalkFrame frame = spill.back();
        spill.pop_back();
        walkDescriptorFrame(spill, frame, 0, visitor);
    }
}

static bool descriptorValidForSize(void* descriptor, size_t actualObjectSize);
static constexpr size_t kValidatedDescriptorSlots = 1u << 16;
static std::atomic<void*> gValidatedDescriptors[kValidatedDescriptorSlots];

static bool
descriptorValidForSize(void* descriptor, size_t actualObjectSize) {
    if (!descriptor) {
        return true;
    }

    struct Frame {
        void* descriptor;
        size_t actualSize;
        bool leaving;
    };
    struct Key {
        void* descriptor;
        size_t actualSize;
        bool operator==(const Key& other) const {
            return descriptor == other.descriptor && actualSize == other.actualSize;
        }
    };
    struct KeyHash {
        size_t operator()(const Key& key) const {
            return std::hash<void*>()(key.descriptor) ^
                (std::hash<size_t>()(key.actualSize) << 1);
        }
    };

    std::vector<Frame> frames;
    std::unordered_set<Key, KeyHash> active;
    std::unordered_set<Key, KeyHash> complete;
    frames.push_back({descriptor, actualObjectSize, false});
    while (!frames.empty()) {
        Frame frame = frames.back();
        frames.pop_back();
        if (!frame.descriptor) {
            continue;
        }
        Key key {frame.descriptor, frame.actualSize};
        if (frame.leaving) {
            active.erase(key);
            complete.insert(key);
            continue;
        }
        if (complete.find(key) != complete.end()) {
            continue;
        }
        if (!active.insert(key).second) {
            return false;
        }
        frames.push_back({frame.descriptor, frame.actualSize, true});

        GCDescriptor* desc = static_cast<GCDescriptor*>(frame.descriptor);
        size_t effectiveSize = desc->objectSize == 0 && desc->isBuffer
            ? frame.actualSize
            : desc->objectSize;
        if (effectiveSize == 0 || effectiveSize > frame.actualSize) {
            return false;
        }
        for (size_t index = 0; index < vecLength(desc->pointerOffsets); ++index) {
            size_t offset = vecSizeAt(desc->pointerOffsets, index);
            if (effectiveSize < sizeof(char*) || offset > effectiveSize - sizeof(char*)) {
                return false;
            }
        }
        for (size_t index = 0; index < vecLength(desc->unionRanges); ++index) {
            GCUnionRange* range = static_cast<GCUnionRange*>(
                vecPointerAt(desc->unionRanges, index));
            if (!range || range->tagOffset >= effectiveSize ||
                    range->payloadOffset > effectiveSize) {
                return false;
            }
            for (size_t caseIndex = 0;
                    caseIndex < vecLength(range->caseDescriptors);
                    ++caseIndex) {
                void* child = vecPointerAt(range->caseDescriptors, caseIndex);
                if (child) {
                    frames.push_back({child, effectiveSize - range->payloadOffset, false});
                }
            }
        }
        if (desc->isBuffer) {
            if (desc->elementSize == 0 &&
                    (vecLength(desc->elementPointerOffsets) != 0 || desc->elementDescriptor)) {
                return false;
            }
            if (desc->elementSize != 0) {
                if (effectiveSize % desc->elementSize != 0) {
                    return false;
                }
                for (size_t index = 0;
                        index < vecLength(desc->elementPointerOffsets);
                        ++index) {
                    size_t offset = vecSizeAt(desc->elementPointerOffsets, index);
                    if (desc->elementSize < sizeof(char*) ||
                            offset > desc->elementSize - sizeof(char*)) {
                        return false;
                    }
                }
                if (desc->elementDescriptor) {
                    frames.push_back({desc->elementDescriptor, desc->elementSize, false});
                }
            }
        } else if (desc->elementSize != 0 ||
                vecLength(desc->elementPointerOffsets) != 0 ||
                desc->elementDescriptor) {
            return false;
        }
    }
    return true;
}

static bool
validateDescriptorForUse(void* descriptor, size_t actualObjectSize) {
    if (!descriptor) {
        return true;
    }
    GCDescriptor* desc = static_cast<GCDescriptor*>(descriptor);
    size_t effectiveSize = desc->objectSize == 0 && desc->isBuffer
        ? actualObjectSize
        : desc->objectSize;
    if (effectiveSize == 0 || effectiveSize > actualObjectSize ||
            (desc->isBuffer && desc->elementSize != 0 &&
             effectiveSize % desc->elementSize != 0)) {
        std::fprintf(
            stderr,
            "Yumi GC rejected invalid descriptor %p for %zu-byte object\n",
            descriptor,
            actualObjectSize);
        return false;
    }

    size_t slot = (reinterpret_cast<uintptr_t>(descriptor) >> 3) &
        (kValidatedDescriptorSlots - 1);
    for (size_t probe = 0; probe < kValidatedDescriptorSlots; ++probe) {
        std::atomic<void*>& entry =
            gValidatedDescriptors[(slot + probe) & (kValidatedDescriptorSlots - 1)];
        void* current = entry.load(std::memory_order_acquire);
        if (current == descriptor) {
            return true;
        }
        if (current) {
            continue;
        }
        bool valid = false;
        try {
            valid = descriptorValidForSize(descriptor, actualObjectSize);
        } catch (...) {
            std::fprintf(
                stderr,
                "Yumi GC could not validate descriptor %p; allocation will return null\n",
                descriptor);
            return false;
        }
        if (!valid) {
            std::fprintf(
                stderr,
                "Yumi GC rejected invalid descriptor %p for %zu-byte object\n",
                descriptor,
                actualObjectSize);
            return false;
        }
        void* empty = nullptr;
        if (entry.compare_exchange_strong(
                empty,
                descriptor,
                std::memory_order_release,
                std::memory_order_acquire) || empty == descriptor) {
            return true;
        }
        // A racing distinct descriptor took this slot. Continue probing; redundant cold-path
        // validation is harmless and never reaches the tracing/allocation hot path again.
    }
    std::fprintf(
        stderr,
        "Yumi GC descriptor-validation cache is full; continuing with validated descriptor %p\n",
        descriptor);
    return true;
}

struct GCMarkPublisherScope {
    bool admitted = false;

    explicit GCMarkPublisherScope(bool shouldAdmit = true) {
        if (shouldAdmit) {
            admitted = gMarkWork.tryBeginPublisher();
        }
    }

    ~GCMarkPublisherScope() {
        if (admitted) {
            gMarkWork.endPublisher();
        }
    }
};

struct GCMarkParticipantScope {
    bool active = false;

    GCMarkParticipantScope() {
        active = gMarkWork.enterParticipant(yumiGCGetActiveThreadCache());
    }

    ~GCMarkParticipantScope() {
        if (active) {
            gMarkWork.leaveParticipant();
        }
    }
};

static bool
pushMarkAdmitted(char* pointer) {
    if (!pointer) {
        return false;
    }
    GCSpan* span = GCHeap::pointerToSpan(pointer);
    char* object = nullptr;
    if (!span || !span->tryMarkContaining(pointer, object)) {
        return false;
    }
    gMarkWork.publish(object);
    return true;
}

static bool
pushMark(char* pointer) {
    GCMarkPublisherScope publisher;
    if (!publisher.admitted) {
        return false;
    }
    // Admission precedes the phase load so termination cannot pass in the gap between a
    // positive phase check and reserving publication state. MarkComplete remains provisional:
    // mutators may publish late work until the sweep-transition handshake closes this gate.
    unsigned char phase = publicCollectorPhase();
    if (phase < kPhaseMarkSetup || phase > kPhaseMarkComplete) {
        return false;
    }
    return pushMarkAdmitted(pointer);
}

static void
markPointer(char* pointer) {
    pushMark(pointer);
}

static void
tracePointerFieldAdmitted(char* object, size_t objectSize, size_t offset) {
    if (!object || objectSize < sizeof(char*) || offset > objectSize - sizeof(char*)) {
        return;
    }
    pushMarkAdmitted(*reinterpret_cast<char**>(object + offset));
}

static void
traceDescriptorAdmitted(char* object, void* descriptor, size_t actualObjectSize) {
    walkDescriptorFields(
        object,
        descriptor,
        actualObjectSize,
        nullptr,
        [](char* fieldObject, void*, size_t objectSize, size_t offset, char*) {
            tracePointerFieldAdmitted(fieldObject, objectSize, offset);
        });
}

static void
traceDescriptor(char* object, void* descriptor, size_t actualObjectSize) {
    // One admission covers the complete descriptor. Termination cannot close the publisher
    // gate until this scope releases it, so every child may use the admitted publication path.
    GCMarkPublisherScope publisher;
    if (!publisher.admitted || !phaseNeedsMarkBarrier(publicCollectorPhase())) {
        return;
    }
    traceDescriptorAdmitted(object, descriptor, actualObjectSize);
}

static void recordDescriptor(char* object, void* descriptor, size_t actualObjectSize);

static void
recordPointerField(char* object, size_t objectSize, size_t offset) {
    if (!object || objectSize < sizeof(char*) || offset > objectSize - sizeof(char*)) {
        return;
    }
    yumiWBBufRecordPointer(*reinterpret_cast<char**>(object + offset));
}

// Read a described value before its publication store and append each non-null edge to the
// current thread's deletion/insertion buffer. The caller owns the publication ordering; a full
// buffer may synchronously donate its entries to the global mark-work queue.
static void
recordDescriptor(char* object, void* descriptor, size_t actualObjectSize) {
    walkDescriptorFields(
        object,
        descriptor,
        actualObjectSize,
        nullptr,
        [](char* fieldObject, void*, size_t objectSize, size_t offset, char*) {
            recordPointerField(fieldObject, objectSize, offset);
        });
}

static bool sweepPublicationValidationEnabled();

static void
validateSweepPublicationPointer(char* pointer) {
    if (!pointer || !sweepPublicationValidationEnabled()) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    if (gPhase != kPhaseSweep) {
        return;
    }
    GCSpan* span = GCHeap::pointerToSpan(pointer);
    if (!span) {
        return;
    }
    char* object = span->objectBase(pointer);
    if (!object) {
        return;
    }
    if (span->sweepGeneration() != yumiGCPublicSweepGeneration() && !span->isMarked(object)) {
        std::fprintf(
            stderr,
            "Yumi GC sweep publication detail: pointer=%p object=%p spanSweepGen=%u "
            "publicSweepGen=%zu phase=%u marked=%u descriptor=%p objectSize=%zu\n",
            pointer,
            object,
            span->sweepGeneration(),
            yumiGCPublicSweepGeneration(),
            static_cast<unsigned>(gPhase),
            span->isMarked(object) ? 1u : 0u,
            span->descriptor(object),
            span->objectSize(object));
        abortGC("GC sweep publication of old white object");
    }
}

enum class ExternalMarkResult : unsigned char {
    Inactive,
    Published,
    AlreadyMarked,
    Transition,
    TrackedTransition
};

static bool
isExternalMarkTransition(ExternalMarkResult result) {
    return result == ExternalMarkResult::Transition ||
        result == ExternalMarkResult::TrackedTransition;
}

static bool
sweepTransitionValidates(ExternalMarkResult result) {
    return isExternalMarkTransition(result) && sweepPublicationValidationEnabled();
}

static void
finishExternalMark(ExternalMarkResult result) {
    if (result != ExternalMarkResult::TrackedTransition) {
        return;
    }
    size_t previous = gSweepTransitionPublishers.fetch_sub(1, std::memory_order_acq_rel);
    if ((previous & kSweepTransitionCountMask) == 0) {
        abortGC("GC sweep-transition publisher accounting underflow");
    }
}

static void validateSweepPublicationDescriptor(char* object, void* descriptor, size_t actualSize);
static size_t verifyMarkedGraphDescriptor(char* object, void* descriptor, size_t actualSize);

// External publication first tries the ordinary lock-free mark edge. Losing admission while
// public MarkComplete means the sweep coordinator closed the gate but has not published Sweep
// yet; callers must not mistake that transition for an inactive barrier.
static ExternalMarkResult
pushExternalMark(char* pointer) {
    while (true) {
        unsigned char before = publicCollectorPhase();
        if (before < kPhaseMarkSetup || before > kPhaseMarkComplete) {
            return before == kPhaseSweep
                ? ExternalMarkResult::Transition
                : ExternalMarkResult::Inactive;
        }

        GCMarkPublisherScope publisher;
        if (publisher.admitted) {
            unsigned char admittedPhase = publicCollectorPhase();
            if (admittedPhase < kPhaseMarkSetup || admittedPhase > kPhaseMarkComplete) {
                return admittedPhase == kPhaseSweep
                    ? ExternalMarkResult::Transition
                    : ExternalMarkResult::Inactive;
            }
            return pushMarkAdmitted(pointer)
                ? ExternalMarkResult::Published
                : ExternalMarkResult::AlreadyMarked;
        }

        size_t transition = gSweepTransitionPublishers.load(std::memory_order_acquire);
        while (transition & kSweepTransitionAccepting) {
            if ((transition & kSweepTransitionCountMask) == kSweepTransitionCountMask) {
                abortGC("GC sweep-transition publisher count overflow");
            }
            if (gSweepTransitionPublishers.compare_exchange_weak(
                    transition,
                    transition + 1,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                yumiGCDebugExternalMarkObservedClosedGate();
                return ExternalMarkResult::TrackedTransition;
            }
        }

        unsigned char after = publicCollectorPhase();
        if (after == kPhaseSweep) {
            return ExternalMarkResult::Transition;
        }
        if (after == kPhaseIdle) {
            return ExternalMarkResult::Inactive;
        }
        // A closed gate with public MarkComplete is the intentional unpublished Sweep window.
        // Yield until the coordinator publishes Sweep; if it instead reopens for late work, the
        // next iteration enters through the ordinary lock-free publisher admission.
        std::this_thread::yield();
    }
}

static void
validateSweepPublicationField(char* object, size_t objectSize, size_t offset) {
    if (!object || objectSize < sizeof(char*) || offset > objectSize - sizeof(char*)) {
        abortGC("GC invalid sweep publication field");
    }
    validateSweepPublicationPointer(*reinterpret_cast<char**>(object + offset));
}

static void
validateSweepPublicationDescriptor(char* object, void* descriptor, size_t actualSize) {
    if (!object || !descriptor || !sweepPublicationValidationEnabled()) {
        return;
    }
    walkDescriptorFields(
        object,
        descriptor,
        actualSize,
        nullptr,
        [](char* fieldObject, void*, size_t objectSize, size_t offset, char*) {
            validateSweepPublicationField(fieldObject, objectSize, offset);
        });
}

static size_t
verifyMarkedGraphPointer(
    char* owner,
    void* ownerDescriptor,
    size_t ownerSize,
    size_t offset,
    char* pointer
) {
    if (!pointer) {
        return 0;
    }

    GCSpan* childSpan = GCHeap::pointerToSpan(pointer);
    if (!childSpan) {
        return 0;
    }

    char* childObject = childSpan->objectBase(pointer);
    if (!childObject) {
        return 0;
    }

    if (childSpan->isMarked(childObject)) {
        return 1;
    }

    GCSpan* ownerSpan = GCHeap::pointerToSpan(owner);
    char* ownerObject = ownerSpan ? ownerSpan->objectBase(owner) : nullptr;
    std::fprintf(
        stderr,
        "Yumi GC marked graph violation: owner=%p ownerObject=%p ownerDescriptor=%p "
        "ownerSize=%zu fieldOffset=%zu childPointer=%p childObject=%p childDescriptor=%p "
        "childSize=%zu childMarked=%u childSweepGen=%u publicSweepGen=%zu phase=%u cycle=%u\n",
        owner,
        ownerObject,
        ownerDescriptor,
        ownerSize,
        offset,
        pointer,
        childObject,
        childSpan->descriptor(childObject),
        childSpan->objectSize(childObject),
        childSpan->isMarked(childObject) ? 1u : 0u,
        childSpan->sweepGeneration(),
        yumiGCPublicSweepGeneration(),
        static_cast<unsigned>(gPhase),
        gCycle);
    abortGC("GC marked graph contains white child");
    return 0;
}

static size_t
verifyMarkedGraphField(
    char* object,
    void* descriptor,
    size_t objectSize,
    size_t offset
) {
    if (!object || objectSize < sizeof(char*) || offset > objectSize - sizeof(char*)) {
        return 0;
    }
    return verifyMarkedGraphPointer(
        object,
        descriptor,
        objectSize,
        offset,
        *reinterpret_cast<char**>(object + offset));
}

static size_t
verifyMarkedGraphDescriptor(char* object, void* descriptor, size_t actualSize) {
    if (!object || !descriptor || actualSize == 0) {
        return 0;
    }
    size_t edgeCount = 0;
    walkDescriptorFields(
        object,
        descriptor,
        actualSize,
        nullptr,
        [&edgeCount](
            char* fieldObject,
            void* fieldDescriptor,
            size_t objectSize,
            size_t offset,
            char*
        ) {
            edgeCount += verifyMarkedGraphField(
                fieldObject,
                fieldDescriptor,
                objectSize,
                offset);
        });
    return edgeCount;
}

static size_t
verifyMarkedHeapGraph() {
    std::vector<GCSpan*> spans;
    {
        GCPageHeapLockGuard lock(heap().pageHeapMutex);
        spans = heap().allSpans;
    }

    size_t edgeCount = 0;
    for (GCSpan* span : spans) {
        if (!span) {
            continue;
        }
        for (size_t index = 0; index < span->objectsCapacity; ++index) {
            if (!span->allocBits.getAcquire(index)) {
                continue;
            }
            char* object = span->memory + span->sizeClass.size * index;
            if (!span->isMarkedSlot(index)) {
                continue;
            }
            void* descriptor = span->descriptors[index];
            if (!descriptor) {
                continue;
            }
            edgeCount += verifyMarkedGraphDescriptor(object, descriptor, span->sizes[index]);
        }
    }
    return edgeCount;
}

static bool
verifyMarkedHeapGraphEnabled() {
    static const bool enabled = envFlagIsSet("YUMI_GC_VERIFY_MARKED_GRAPH");
    return enabled;
}

static bool
checkmarkEnabled() {
    static const bool enabled = envFlagIsSet("YUMI_GC_CHECKMARK");
    return enabled;
}

static bool
sweepPublicationValidationEnabled() {
    return checkmarkEnabled() || yumiGCRuntimeVerifyEachSweep();
}

static bool
verifyConservativeStackRootsEnabled() {
    static const bool enabled = envFlagIsSet("YUMI_GC_VERIFY_CONSERVATIVE_STACK_ROOTS");
    return enabled;
}

static bool
conservativeCurrentStackRootsEnabled() {
    static const bool enabled = envFlagIsSet("YUMI_GC_CONSERVATIVE_CURRENT_STACK_ROOTS");
    return enabled;
}

static bool
conservativeAllStackRootsEnabled() {
    static const bool enabled = envFlagIsSet("YUMI_GC_CONSERVATIVE_ALL_STACK_ROOTS");
    return enabled;
}

static bool
conservativeStackRootSafetyNetEnabled() {
    static const bool enabled = envFlagIsSet("YUMI_GC_CONSERVATIVE_STACK_ROOT_SAFETY_NET");
    return enabled;
}

static void
conservativeStackRootVisitor(void* root, void*, void*) {
    if (!root) {
        return;
    }
    char* pointer = static_cast<char*>(root);
    GCSpan* span = GCHeap::pointerToSpan(pointer);
    if (!span) {
        return;
    }
    char* object = span->objectBase(pointer);
    if (!object) {
        return;
    }
    markPointer(object);
}

struct PreciseRootLookup {
    char* object;
    size_t hits;
};

static void
explainPreciseRootVisitor(void* root, void* descriptor, void* data) {
    if (!root || !data) {
        return;
    }
    PreciseRootLookup* lookup = static_cast<PreciseRootLookup*>(data);
    char* pointer = static_cast<char*>(root);
    GCSpan* span = GCHeap::pointerToSpan(pointer);
    if (!span) {
        return;
    }
    char* object = span->objectBase(pointer);
    if (object != lookup->object) {
        return;
    }
    ++lookup->hits;
    std::fprintf(
        stderr,
        "  precise root hit root=%p object=%p descriptorArg=%p objectDescriptor=%p\n",
        root,
        object,
        descriptor,
        span->descriptor(object));
}

static void
verifyConservativeStackRootVisitor(void* root, void*, void*) {
    if (!root) {
        return;
    }
    char* pointer = static_cast<char*>(root);
    GCSpan* span = GCHeap::pointerToSpan(pointer);
    if (!span) {
        return;
    }
    char* object = span->objectBase(pointer);
    if (!object || span->isMarked(object)) {
        return;
    }
    if (pointer != object) {
        return;
    }
    void* descriptor = span->descriptor(object);
    if (!descriptor) {
        return;
    }
    size_t objectSize = span->objectSize(object);
    if (objectSize < 128) {
        return;
    }
    PreciseRootLookup lookup { object, 0 };
    yumiGCScanCurrentStack(explainPreciseRootVisitor, &lookup);
    yumiGCScanTaskStacks(explainPreciseRootVisitor, &lookup);
    std::fprintf(
        stderr,
        "Yumi GC conservative stack root missed by precise scan: root=%p object=%p slot=%p "
        "descriptor=%p objectSize=%zu preciseHits=%zu normalMarked=%u sweepGen=%u publicSweepGen=%zu "
        "phase=%u cycle=%u\n",
        root,
        object,
        yumiGCDebugConservativeRootSlot(),
        descriptor,
        objectSize,
        lookup.hits,
        span->isMarked(object) ? 1u : 0u,
        span->sweepGeneration(),
        yumiGCPublicSweepGeneration(),
        static_cast<unsigned>(gPhase),
        gCycle);
    abortGC("GC precise stack scan missed conservative root");
}

static void
verifyConservativeStackRoots() {
    yumiGCScanCurrentStackConservative(verifyConservativeStackRootVisitor, nullptr);
    yumiGCScanTaskStacksConservative(verifyConservativeStackRootVisitor, nullptr);
}

static void
stackRootVisitor(void* root, void* descriptor, void*) {
    if (!root) {
        return;
    }
    char* pointer = static_cast<char*>(root);
    if (!descriptor) {
        markPointer(pointer);
        return;
    }
    if (GCHeap::pointerToSpan(pointer)) {
        markPointer(pointer);
        return;
    }
    GCDescriptor* desc = static_cast<GCDescriptor*>(descriptor);
    if (!validateDescriptorForUse(descriptor, desc->objectSize)) {
        return;
    }
    traceDescriptor(pointer, descriptor, desc->objectSize);
}

static void
traceRegionAllocation(
    void* allocation,
    size_t byteSize,
    void* descriptor,
    void*
) {
    if (!allocation) {
        return;
    }
    if (descriptor) {
        traceDescriptor(static_cast<char*>(allocation), descriptor, byteSize);
    } else {
        markPointer(static_cast<char*>(allocation));
    }
}

static void
traceCurrentStack() {
    yumiGCScanCurrentStack(stackRootVisitor, nullptr);
    if (conservativeStackRootSafetyNetEnabled() || conservativeCurrentStackRootsEnabled()) {
        yumiGCScanCurrentStackConservative(conservativeStackRootVisitor, nullptr);
    }
}

static void
traceTemporaryRootList(void* head, void*) {
    for (Root* root = static_cast<Root*>(head); root; root = root->next) {
        markPointer(root->pointer);
    }
}

extern "C" void
yumiGCRuntimeHandshakeTaskWork(uint32_t reason) {
    if (reason != kHandshakeReasonMarkSetup) {
        return;
    }
    uint64_t startNanos = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    {
        GCMarkParticipantScope participant;
        YumiGCTemporaryRootStorage* storage =
            yumiGCActiveTaskTemporaryRootStorage();
        if (storage) {
            traceTemporaryRootList(storage->head, nullptr);
        }
        traceCurrentStack();
        yumiGCVisitActiveTaskParseRegionAllocations(
            traceRegionAllocation,
            nullptr);
    }
    uint64_t endNanos = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    yumiGCRuntimeRecordPerTaskSetupScan(
        static_cast<size_t>(startNanos),
        static_cast<size_t>(endNanos - startNanos));
}

static void
traceOwnerStableAndStorageRootsLocked() {
    Root* current = gFirstRoot;
    while (current) {
        if (current->descriptor) {
            traceDescriptor(current->pointer, current->descriptor, current->byteSize);
        } else {
            markPointer(current->pointer);
        }
        current = current->next;
    }
    yumiArenaVisitTracedReservations(traceRegionAllocation, nullptr);
    // A claimed owner-linked cleanup payload is retained by its queue entry instead of a
    // per-registration Root. Trace queued and in-flight contexts on every later cycle until their
    // callback drains, so a delayed cleanup task cannot lose its state between collections.
    {
        std::lock_guard<std::mutex> cleanupLock(gCleanupQueueMutex);
        for (const CleanupInvocation& invocation : gCleanupQueue) {
            markPointer(invocation.context);
        }
        for (const CleanupInvocation& invocation : gInFlightCleanups) {
            markPointer(invocation.context);
        }
    }
    yumiGCVisitTaskTemporaryRootLists(traceTemporaryRootList, nullptr);
    traceCurrentStack();
    yumiGCScanTaskStacks(stackRootVisitor, nullptr);
    if (conservativeStackRootSafetyNetEnabled() || conservativeAllStackRootsEnabled()) {
        yumiGCScanTaskStacksConservative(conservativeStackRootVisitor, nullptr);
    }
    yumiGCScanStorageRoots(stackRootVisitor, nullptr);
    yumiGCVisitTaskParseRegionAllocations(traceRegionAllocation, nullptr);
}

static bool
validObjectSidePayloadLane(size_t lane) {
    return lane < kObjectSidePayloadLaneCount;
}

static char*
objectSidePayloadUnchecked(GCSpan* span, size_t objectIndex, size_t lane) {
    if (!span || !validObjectSidePayloadLane(lane)) {
        return nullptr;
    }
    ObjectSideMetadataTable* table = span->objectSideMetadata.load(std::memory_order_acquire);
    if (!table || objectIndex >= table->capacity) {
        return nullptr;
    }
    std::atomic<char*>* payloads = table->lane(lane);
    if (!payloads) {
        return nullptr;
    }
    return payloads[objectIndex].load(std::memory_order_acquire);
}

static GCSpan*
objectSidePayloadSpan(char* object, size_t& objectIndex) {
    if (!object) {
        return nullptr;
    }
    GCSpan* span = GCHeap::pointerToSpan(object);
    if (!span || !span->objectIndex(object, objectIndex) ||
            !span->allocBits.getAcquire(objectIndex)) {
        return nullptr;
    }
    return span;
}

static ObjectSideMetadataTable*
ensureObjectSideMetadataTable(GCSpan* span) {
    if (!span) {
        return nullptr;
    }
    ObjectSideMetadataTable* table = span->objectSideMetadata.load(std::memory_order_acquire);
    if (table) {
        return table;
    }
    table = new (std::nothrow) ObjectSideMetadataTable(span->objectsCapacity);
    if (!table) {
        return nullptr;
    }
    ObjectSideMetadataTable* expected = nullptr;
    if (span->objectSideMetadata.compare_exchange_strong(
            expected,
            table,
            std::memory_order_release,
            std::memory_order_acquire)) {
        return table;
    }
    delete table;
    return expected;
}

static bool
traceObjectSidePayloadsAdmitted(GCSpan* span, size_t objectIndex) {
    if (!span) {
        return false;
    }
    ObjectSideMetadataTable* table = span->objectSideMetadata.load(std::memory_order_acquire);
    if (!table || objectIndex >= table->capacity) {
        return false;
    }
    bool traced = false;
    for (size_t lane = 0; lane < kObjectSidePayloadLaneCount; ++lane) {
        std::atomic<char*>* payloads = table->lane(lane);
        if (!payloads) {
            continue;
        }
        char* payload = payloads[objectIndex].load(std::memory_order_acquire);
        if (!payload) {
            continue;
        }
        pushMarkAdmitted(payload);
        traced = true;
    }
    return traced;
}

static bool
traceObjectSidePayloads(GCSpan* span, size_t objectIndex) {
    GCMarkPublisherScope publisher;
    if (!publisher.admitted || !phaseNeedsMarkBarrier(publicCollectorPhase())) {
        return false;
    }
    return traceObjectSidePayloadsAdmitted(span, objectIndex);
}

static bool
managedGraphContains(
    char* state,
    char* target,
    size_t* visitedCount = nullptr,
    size_t* edgeCount = nullptr
) {
    if (visitedCount) {
        *visitedCount = 0;
    }
    if (edgeCount) {
        *edgeCount = 0;
    }
    GCSpan* targetSpan = GCHeap::pointerToSpan(target);
    char* targetBase = targetSpan ? targetSpan->objectBase(target) : nullptr;
    if (!targetBase) {
        return false;
    }

    // Cleanup publication can be a mount hot path. Reuse native scratch storage so retention
    // validation does not turn every safe, constant-sized context into allocator traffic.
    static thread_local std::vector<char*> work;
    static thread_local std::unordered_set<char*> visited;
    work.clear();
    visited.clear();
    size_t edges = 0;
    work.push_back(state);
    while (!work.empty()) {
        char* pointer = work.back();
        work.pop_back();
        GCSpan* span = GCHeap::pointerToSpan(pointer);
        char* object = span ? span->objectBase(pointer) : nullptr;
        size_t objectIndex = 0;
        if (!object || !span->objectIndex(object, objectIndex) ||
                !span->allocBits.getAcquire(objectIndex) ||
                !visited.insert(object).second) {
            continue;
        }
        if (object == targetBase) {
            if (visitedCount) {
                *visitedCount = visited.size();
            }
            if (edgeCount) {
                *edgeCount = edges;
            }
            return true;
        }
        void* descriptor = span->descriptor(object);
        size_t objectSize = span->objectSize(object);
        walkDescriptorFields(
            object,
            descriptor,
            objectSize,
            nullptr,
            [&edges](char* fieldObject, void*, size_t size, size_t offset, char*) {
                if (fieldObject && size >= sizeof(char*) && offset <= size - sizeof(char*)) {
                    ++edges;
                    work.push_back(*reinterpret_cast<char**>(fieldObject + offset));
                }
            });
        ObjectSideMetadataTable* table = span->objectSideMetadata.load(
            std::memory_order_acquire);
        if (!table || objectIndex >= table->capacity) {
            continue;
        }
        for (size_t lane = 0; lane < kObjectSidePayloadLaneCount; ++lane) {
            std::atomic<char*>* payloads = table->lane(lane);
            if (payloads) {
                ++edges;
                work.push_back(payloads[objectIndex].load(std::memory_order_acquire));
            }
        }
    }
    if (visitedCount) {
        *visitedCount = visited.size();
    }
    if (edgeCount) {
        *edgeCount = edges;
    }
    return false;
}

static bool
claimDeadObjectSideCleanupsAtTermination(const std::vector<GCSpan*>& spans) {
    if (objectSideCleanupPayloadCount() == 0) {
        return false;
    }

    std::vector<CleanupInvocation> enqueued;
    bool publishedMarkWork = false;
    for (GCSpan* span : spans) {
        if (!span || !span->hasCleanupSidePayloads()) {
            continue;
        }
        ObjectSideMetadataTable* table = span->objectSideMetadata.load(
            std::memory_order_acquire);
        if (!table) {
            abortGC("GC cleanup side-payload span has no side metadata");
        }
        size_t wordCount = (table->capacity + 63) / 64;
        for (size_t lane = 0; lane < kObjectSidePayloadLaneCount; ++lane) {
            YumiGCCleanupCallback callback = objectSidePayloadCleanupCallback(lane);
            std::atomic<char*>* payloads = callback ? table->lane(lane) : nullptr;
            std::atomic<uint64_t>* occupancy = callback
                ? table->cleanupOccupancy(lane)
                : nullptr;
            if (!payloads || !occupancy) {
                continue;
            }
            for (size_t wordIndex = 0; wordIndex < wordCount; ++wordIndex) {
                uint64_t bits = occupancy[wordIndex].load(std::memory_order_acquire);
                while (bits) {
                    size_t bitIndex = static_cast<size_t>(__builtin_ctzll(bits));
                    size_t objectIndex = wordIndex * 64 + bitIndex;
                    bits &= bits - 1;
                    if (objectIndex >= table->capacity ||
                            span->isMarkedSlot(objectIndex)) {
                        continue;
                    }

                    char* payload = payloads[objectIndex].exchange(
                        nullptr,
                        std::memory_order_acq_rel);
                    if (!table->clearCleanupOccupied(lane, objectIndex) || !payload) {
                        abortGC("GC cleanup side-payload occupancy mismatch");
                    }
                    span->removeCleanupSidePayload();
                    recordObjectSideCleanupPayloadClear();
                    recordObjectSidePayloadClear(lane);

                    GCSpan* payloadSpan = GCHeap::pointerToSpan(payload);
                    char* context = payloadSpan ? payloadSpan->objectBase(payload) : nullptr;
                    if (!context || !payloadSpan->ownsObject(context)) {
                        abortGC("GC cleanup side payload is not a managed object");
                    }
                    if (!payloadSpan->isMarked(context)) {
                        if (!pushMarkAdmitted(context) && !payloadSpan->isMarked(context)) {
                            abortGC("GC cleanup side payload could not be retained");
                        }
                        publishedMarkWork = true;
                    }

                    CleanupInvocation invocation;
                    invocation.context = context;
                    invocation.callback = callback;
                    enqueued.push_back(invocation);
                    gCleanupPendingCount.fetch_add(1, std::memory_order_relaxed);
                    gCleanupEnqueueCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
    }
    if (!enqueued.empty()) {
        std::lock_guard<std::mutex> lock(gCleanupQueueMutex);
        for (CleanupInvocation& invocation : enqueued) {
            gCleanupQueue.push_back(invocation);
        }
    }
    return publishedMarkWork;
}

static void
processLifetimeRegistriesAtTermination(bool scanWeak, bool scanCleanup) {
    // Every non-empty registry's shards are held by LifetimeTerminationLocks, always weak
    // before cleanup. A zero-count registry is skipped without touching its vectors; a racing
    // registration shades its target and becomes eligible for processing next cycle.
    // Mark publisher admission is closed and quiescent. Clearing therefore linearizes before
    // enqueue, and both complete before the public Sweep store.
    if (scanWeak) {
        for (size_t shard = 0; shard < kLifetimeRegistryShardCount; ++shard) {
            OwnerWeakShard& shardState = gOwnerWeakShards[shard];
            uint32_t remaining = shardState.slotCount;
            uint32_t index = 0;
            for (size_t segment = 0; remaining; ++segment) {
                OwnerWeakSlot* slots = shardState.segments[segment].load(
                    std::memory_order_relaxed);
                uint32_t segmentSize = uint32_t {1} << (segment + kOwnerWeakFirstSegmentShift);
                uint32_t count = std::min(segmentSize, remaining);
                for (uint32_t offset = 0; offset < count; ++offset, ++index) {
                    OwnerWeakSlot& slot = slots[offset];
                    if (!slot.owner) {
                        continue;
                    }
                    if (!slot.ownerSpan->isMarked(slot.owner)) {
                        freeOwnerWeakSlot(shardState, slot, index);
                        gOwnerWeakRegistrationCount.fetch_sub(1, std::memory_order_relaxed);
                        continue;
                    }
                    char* target = slot.target.load(std::memory_order_relaxed);
                    if (target && !slot.targetSpan->isMarked(target)) {
                        slot.target.store(nullptr, std::memory_order_relaxed);
                        slot.targetSpan = nullptr;
                        gWeakClearCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
                remaining -= count;
            }
        }
    }

    std::vector<CleanupInvocation> enqueued;
    if (scanCleanup) {
        for (size_t shard = 0; shard < kLifetimeRegistryShardCount; ++shard) {
            auto& entries = gCleanupRegistries[shard];
            auto& indices = gCleanupRegistryIndices[shard];
            size_t write = 0;
            for (size_t read = 0; read < entries.size(); ++read) {
                CleanupRegistration entry = entries[read];
                if (!entry.targetSpan || !entry.targetSpan->hasCleanupRegistrations()) {
                    abortGC("GC cleanup registry/span count mismatch");
                }
                char* target = entry.targetSpan
                    ? entry.targetSpan->objectBase(entry.target)
                    : nullptr;
                if (target && entry.targetSpan->isMarked(target)) {
                    entries[write] = entry;
                    indices[entry.handle] = write;
                    write += 1;
                    continue;
                }
                indices.erase(entry.handle);
                if (entry.targetSpan) {
                    entry.targetSpan->removeCleanupRegistration();
                }
                CleanupInvocation invocation;
                invocation.context = entry.context;
                invocation.contextRoot = entry.contextRoot;
                invocation.callback = entry.callback;
                enqueued.push_back(invocation);
                gCleanupRegistrationCount.fetch_sub(1, std::memory_order_relaxed);
                gCleanupPendingCount.fetch_add(1, std::memory_order_relaxed);
                gCleanupEnqueueCount.fetch_add(1, std::memory_order_relaxed);
            }
            entries.resize(write);
        }
    }
    if (!enqueued.empty()) {
        std::lock_guard<std::mutex> lock(gCleanupQueueMutex);
        for (CleanupInvocation& invocation : enqueued) {
            gCleanupQueue.push_back(invocation);
        }
    }
}

static void scheduleCleanupTaskIfNeeded();

static void
cleanupTaskMain(void*) {
    yumiGCDrainCleanups();
    gCleanupTaskScheduled.store(false, std::memory_order_release);
    scheduleCleanupTaskIfNeeded();
    if (yumiTerminateActiveTask) {
        yumiTerminateActiveTask();
    }
}

static void
scheduleCleanupTaskIfNeeded() {
    {
        std::lock_guard<std::mutex> lock(gCleanupQueueMutex);
        if (gCleanupQueue.empty()) {
            return;
        }
    }
    bool expected = false;
    if (!gCleanupTaskScheduled.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel,
            std::memory_order_acquire)) {
        return;
    }
    if (!yumiForkGCCleanupTask || !yumiForkGCCleanupTask(cleanupTaskMain, nullptr)) {
        gCleanupTaskScheduled.store(false, std::memory_order_release);
    }
}

struct CheckmarkEdge {
    char* parent = nullptr;
    size_t offset = 0;

    CheckmarkEdge() = default;
    CheckmarkEdge(char* parent, size_t offset)
        : parent(parent), offset(offset) {}
};

struct CheckmarkSpan {
    GCSpan* span = nullptr;
    GCBitmap marks;

    CheckmarkSpan(GCSpan* span, size_t capacity)
        : span(span), marks(capacity) {}
};

struct CheckmarkState {
    std::vector<CheckmarkSpan> spans;
    std::unordered_map<GCSpan*, size_t> spanIndices;
    std::unordered_map<char*, CheckmarkEdge> edges;
    std::vector<char*> work;
    char* firstNormalMiss = nullptr;
    size_t normalMissCount = 0;
};

static void checkmarkTraceDescriptor(
    CheckmarkState& state,
    char* object,
    void* descriptor,
    size_t actualSize,
    char* owner);

static void
checkmarkPush(
    CheckmarkState& state,
    char* pointer,
    char* parent = nullptr,
    size_t offset = 0
) {
    if (!pointer) {
        return;
    }
    GCSpan* span = GCHeap::pointerToSpan(pointer);
    if (!span) {
        return;
    }
    char* object = span->objectBase(pointer);
    size_t objectIndex = 0;
    auto found = state.spanIndices.find(span);
    if (!object || found == state.spanIndices.end() ||
            !span->objectIndex(object, objectIndex) ||
            !span->allocBits.getAcquire(objectIndex)) {
        return;
    }
    CheckmarkSpan& shadow = state.spans[found->second];
    if (shadow.marks.get(objectIndex)) {
        return;
    }
    shadow.marks.set(objectIndex);
    state.edges.emplace(object, CheckmarkEdge {parent, offset});
    state.work.push_back(object);
    if (!span->isMarked(object)) {
        if (!state.firstNormalMiss) {
            state.firstNormalMiss = object;
        }
        ++state.normalMissCount;
    }
}

static void
checkmarkTraceField(
    CheckmarkState& state,
    char* object,
    size_t objectSize,
    size_t offset,
    char* owner
) {
    if (!object || objectSize < sizeof(char*) || offset > objectSize - sizeof(char*)) {
        return;
    }
    size_t ownerOffset = owner && object >= owner
        ? static_cast<size_t>((object + offset) - owner)
        : offset;
    checkmarkPush(state, *reinterpret_cast<char**>(object + offset), owner, ownerOffset);
}

static void
checkmarkTraceDescriptor(
    CheckmarkState& state,
    char* object,
    void* descriptor,
    size_t actualSize,
    char* owner
) {
    if (!object || !descriptor || actualSize == 0) {
        return;
    }
    char* edgeOwner = owner ? owner : object;
    walkDescriptorFields(
        object,
        descriptor,
        actualSize,
        edgeOwner,
        [&state](
            char* fieldObject,
            void*,
            size_t objectSize,
            size_t offset,
            char* fieldOwner
        ) {
            checkmarkTraceField(
                state,
                fieldObject,
                objectSize,
                offset,
                fieldOwner);
        });
}

static void
checkmarkRootVisitor(void* root, void* descriptor, void* data) {
    if (!root || !data || yumiGCVisitingConservativeRoot()) {
        return;
    }
    CheckmarkState& state = *static_cast<CheckmarkState*>(data);
    char* pointer = static_cast<char*>(root);
    if (!descriptor || GCHeap::pointerToSpan(pointer)) {
        checkmarkPush(state, pointer);
        return;
    }
    GCDescriptor* desc = static_cast<GCDescriptor*>(descriptor);
    checkmarkTraceDescriptor(state, pointer, descriptor, desc->objectSize, nullptr);
}

static void
checkmarkRegionAllocation(
    void* allocation,
    size_t byteSize,
    void* descriptor,
    void* data
) {
    if (!allocation || !data) {
        return;
    }
    CheckmarkState& state = *static_cast<CheckmarkState*>(data);
    if (descriptor) {
        checkmarkTraceDescriptor(
            state,
            static_cast<char*>(allocation),
            descriptor,
            byteSize,
            nullptr);
    } else {
        checkmarkPush(state, static_cast<char*>(allocation));
    }
}

static void
checkmarkTemporaryRootList(void* head, void* data) {
    if (!data) {
        return;
    }
    CheckmarkState& state = *static_cast<CheckmarkState*>(data);
    for (Root* root = static_cast<Root*>(head); root; root = root->next) {
        checkmarkPush(state, root->pointer);
    }
}

static void
checkmarkRoots(CheckmarkState& state) {
    std::lock_guard<std::mutex> rootLock(gRootMutex);
    for (Root* current = gFirstRoot; current; current = current->next) {
        if (current->descriptor) {
            checkmarkTraceDescriptor(
                state,
                current->pointer,
                current->descriptor,
                current->byteSize,
                nullptr);
        } else {
            checkmarkPush(state, current->pointer);
        }
    }
    yumiArenaVisitTracedReservations(checkmarkRegionAllocation, &state);
    {
        std::lock_guard<std::mutex> cleanupLock(gCleanupQueueMutex);
        for (const CleanupInvocation& invocation : gCleanupQueue) {
            checkmarkPush(state, invocation.context);
        }
        for (const CleanupInvocation& invocation : gInFlightCleanups) {
            checkmarkPush(state, invocation.context);
        }
    }
    yumiGCVisitTaskTemporaryRootLists(checkmarkTemporaryRootList, &state);
    yumiGCScanCurrentStack(checkmarkRootVisitor, &state);
    yumiGCScanTaskStacks(checkmarkRootVisitor, &state);
    yumiGCScanStorageRoots(checkmarkRootVisitor, &state);
    yumiGCVisitTaskParseRegionAllocations(checkmarkRegionAllocation, &state);
    checkmarkPush(state, gCheckmarkTestExtraRoot.load(std::memory_order_acquire));
}

static void
checkmarkTraceObject(CheckmarkState& state, char* object) {
    GCSpan* span = GCHeap::pointerToSpan(object);
    if (!span || !span->ownsObject(object)) {
        return;
    }
    size_t objectIndex = 0;
    if (!span->objectIndex(object, objectIndex)) {
        return;
    }
    void* descriptor = span->descriptor(object);
    if (descriptor) {
        checkmarkTraceDescriptor(
            state,
            object,
            descriptor,
            span->objectSize(object),
            object);
    }
    ObjectSideMetadataTable* table = span->objectSideMetadata.load(std::memory_order_acquire);
    if (!table || objectIndex >= table->capacity) {
        return;
    }
    for (size_t lane = 0; lane < kObjectSidePayloadLaneCount; ++lane) {
        std::atomic<char*>* payloads = table->lane(lane);
        if (payloads) {
            checkmarkPush(
                state,
                payloads[objectIndex].load(std::memory_order_acquire),
                object,
                span->objectSize(object) + lane * sizeof(char*));
        }
    }
}

static void
verifyCheckmark() {
    CheckmarkState state;
    {
        GCPageHeapLockGuard spanLock(heap().pageHeapMutex);
        state.spans.reserve(heap().allSpans.size());
        for (GCSpan* span : heap().allSpans) {
            if (!span) {
                continue;
            }
            size_t index = state.spans.size();
            state.spanIndices.emplace(span, index);
            state.spans.push_back(CheckmarkSpan(span, span->objectsCapacity));
        }
    }

    checkmarkRoots(state);
    while (!state.work.empty()) {
        char* object = state.work.back();
        state.work.pop_back();
        checkmarkTraceObject(state, object);
    }

    if (!state.firstNormalMiss) {
        return;
    }
    char* object = state.firstNormalMiss;
    GCSpan* span = GCHeap::pointerToSpan(object);
    std::fprintf(
        stderr,
        "Yumi GC checkmark detail: cycle=%u shadowReachable=%zu firstMiss=%p "
        "normalMarked=%u descriptor=%p objectSize=%zu\n",
        gCycle,
        state.normalMissCount,
        object,
        span && span->isMarked(object) ? 1u : 0u,
        span ? span->descriptor(object) : nullptr,
        span ? span->objectSize(object) : 0);
    for (size_t depth = 0; object && depth < 16; ++depth) {
        auto found = state.edges.find(object);
        if (found == state.edges.end()) {
            break;
        }
        std::fprintf(
            stderr,
            "  shadow path depth=%zu object=%p parent=%p offset=%zu\n",
            depth,
            object,
            found->second.parent,
            found->second.offset);
        object = found->second.parent;
    }
    abortGC("GC checkmark found object missed by normal mark");
}

static size_t
drainLimitUnmeasured(size_t workLimit) {
    if (!workLimit) {
        return 0;
    }
    GCMarkParticipantScope participant;
    if (!participant.active) {
        return 0;
    }
    size_t drained = 0;
    char* batch[kCollectorStepWork];
    while (drained < workLimit) {
        size_t batchCount = 0;
        {
            std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
            if (!phaseNeedsMarkBarrier(gPhase)) {
                setMarkQueueDepth();
                // A marker becomes quiescent only after its mutator buffer has donated every
                // edge and its local gcWork pair is force-published by the participant scope.
                yumiGCRuntimeFlushWriteBarrierBuffer();
                return drained;
            }
            size_t batchLimit = std::min(kCollectorStepWork, workLimit - drained);
            while (batchCount < batchLimit) {
                char* pointer = gMarkWork.tryPop();
                if (!pointer) {
                    break;
                }
                batch[batchCount++] = pointer;
            }
            if (batchCount) {
                yumiGCRuntimeRecordMarkQueuePopBatch(batchCount, gMarkWork.depth());
            }
            setMarkQueueDepth();
        }

        // A producer may have won a mark but not yet release-published its WorkBuf slot.
        // Do not spin inside one bounded drain request; publisher accounting keeps
        // termination open and a later driver pass retries the queue head.
        if (!batchCount) {
            yumiGCRuntimeFlushWriteBarrierBuffer();
            return drained;
        }

        // The participant lease is stronger than a per-edge publisher lease: mark termination
        // and the MarkComplete-to-Sweep transition both require active markers to reach zero.
        // Publish every child discovered by this batch directly under that existing lease.
        for (size_t index = 0; index < batchCount; ++index) {
            char* pointer = batch[index];
            GCSpan* span = GCHeap::pointerToSpan(pointer);
            size_t objectIndex = 0;
            if (!span || !span->objectIndex(pointer, objectIndex) ||
                    !span->allocBits.getAcquire(objectIndex)) {
                continue;
            }
            void* descriptor = span->descriptors[objectIndex];
            if (descriptor) {
                traceDescriptorAdmitted(pointer, descriptor, span->sizes[objectIndex]);
            }
            traceObjectSidePayloadsAdmitted(span, objectIndex);
        }

        drained += batchCount;
    }
    yumiGCRuntimeFlushWriteBarrierBuffer();
    return drained;
}

static size_t
drainLimit(size_t workLimit) {
    if (!workLimit) {
        return 0;
    }
    yumiGCRuntimeSubphaseStart(YumiGCObservationMarkWork, gCycle, 0);
    size_t drained = drainLimitUnmeasured(workLimit);
    if (drained) {
        yumiGCRuntimeSubphaseEnd(YumiGCObservationMarkWork, drained);
    } else {
        yumiGCRuntimeSubphaseCancel(YumiGCObservationMarkWork);
    }
    return drained;
}

static void
drainAll() {
    while (!gMarkWork.empty()) {
        if (drainLimit(kCollectorStepWork) == 0) {
            std::this_thread::yield();
        }
    }
}

extern "C" bool
yumiGCTestConcurrentMarkParticipants(size_t count) {
    if (count < 2 || count > 64 || !phaseNeedsMarkBarrier(publicCollectorPhase())) {
        return false;
    }
    std::atomic<size_t> ready {0};
    std::atomic<bool> start {false};
    std::vector<std::thread> threads;
    threads.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        threads.emplace_back([&]() {
            bool admitted = gMarkWork.enterParticipant(yumiGCGetActiveThreadCache());
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (!admitted) {
                return;
            }
            while (!gMarkWork.empty()) {
                (void)drainLimit(kCollectorStepWork);
            }
            gMarkWork.leaveParticipant();
        });
    }
    while (ready.load(std::memory_order_acquire) != count) {
        std::this_thread::yield();
    }
    start.store(true, std::memory_order_release);
    for (std::thread& thread : threads) {
        thread.join();
    }
    return gMarkWork.activeMarkerCount() == 0 && gMarkWork.empty() &&
        gMarkWork.maxActiveMarkerCount() >= count;
}

static bool
finishMarkLocked() {
    if (gPhase != kPhaseMarkTermination || gActiveAllocationCount != 0 ||
            gMarkWork.activeMarkerCount() != 0) {
        return false;
    }
    // Closing publisher admission is the mark-completion linearization point. A racing
    // edge either incremented the same atomic first (and this returns false) or observes
    // the closed bit and cannot mutate mark/work state after completion.
    if (!gMarkWork.publishersClosed() && !gMarkWork.tryClosePublishers()) {
        return false;
    }
    // A publisher admitted before the close may have completed between the first checks
    // and the close CAS. Reopen before draining: tracing that queued object must be able
    // to publish newly discovered children through the ordinary mark edge.
    if (!gMarkWork.quiescent()) {
        gMarkWork.reopenPublishersForDrain();
        return false;
    }
    // MarkComplete is observable while mutators run, so reopen admission before publishing it.
    // Any later winner makes completion provisional; the sweep-transition handshake closes the
    // same gate and drains that work through the existing termination protocol.
    gMarkWork.reopenPublishersForDrain();
    gPhase = kPhaseMarkComplete;
    publishPhase(gPhase);
    yumiGCRuntimeSetMarkQueueDepth(0);
    yumiGCRuntimeSetAssistDebt(0);
    return true;
}

static bool
beginMark(bool eagerClear) {
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    if (gPhase != kPhaseIdle) {
        return false;
    }
    if (gActiveAllocationCount != 0) {
        return false;
    }
    ++gCycle;
    heap().beginMarkCycle(eagerClear);
    // Span construction maintains and provisions the active-slot bound. Reset is O(1): it
    // snapshots that bound while the setup handshake has allocators quiescent.
    gMarkWork.reset();
    setMarkQueueDepth();
    yumiGCRuntimeCycleStart(gCycle);
    gPhase = kPhaseMarkSetup;
    publishPhase(gPhase);
    return true;
}

static void
enterConcurrentMark() {
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    if (gPhase == kPhaseMarkSetup) {
        gPhase = kPhaseConcurrentMark;
        publishPhase(gPhase);
    }
}

static void
enterMarkTermination() {
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    if (gPhase == kPhaseConcurrentMark) {
        gPhase = kPhaseMarkTermination;
        publishPhase(gPhase);
    }
}

static bool
beginSweepAtSafepoint() {
    // The caller owns the sweep handshake. It closes MarkComplete publication, reconciles any
    // late work with the old termination driver, and keeps mutators parked while cursor state is
    // initialized. Store locks are deliberately taken only after releasing gCollectorMutex.
    std::vector<GCSpan*> cleanupSideSpans;
    if (objectSideCleanupPayloadCount() != 0) {
        GCPageHeapLockGuard spanLock(heap().pageHeapMutex);
        cleanupSideSpans = heap().allSpans;
    }
    while (true) {
        bool drainLateWork = false;
        bool waitForPublisher = false;
        // Registry shards precede the collector lock. Weak loads take one shard while shading,
        // so this common order prevents a load/termination inversion at MarkComplete.
        LifetimeTerminationLocks lifetimeLocks;
        {
            std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
            if (gPhase == kPhaseMarkTermination) {
                if (!finishMarkLocked()) {
                    drainLateWork = true;
                }
            }
            if (!drainLateWork && gPhase != kPhaseMarkComplete) {
                return false;
            }
            if (!drainLateWork) {
                if (!gMarkWork.publishersClosed() && !gMarkWork.tryClosePublishers()) {
                    waitForPublisher = true;
                }
                if (!waitForPublisher &&
                        (!gMarkWork.quiescent() ||
                        gActiveAllocationCount != 0)) {
                    gPhase = kPhaseMarkTermination;
                    publishPhase(gPhase);
                    gMarkWork.reopenPublishersForDrain();
                    drainLateWork = true;
                } else if (!waitForPublisher && !drainLateWork) {
                    // Claim dead-owner cleanup lanes before weak processing. Any newly retained
                    // context reopens ordinary marking and completes to a fixed point before this
                    // loop is allowed to clear weak state or enter Sweep.
                    if (claimDeadObjectSideCleanupsAtTermination(cleanupSideSpans)) {
                        gPhase = kPhaseMarkTermination;
                        publishPhase(gPhase);
                        gMarkWork.reopenPublishersForDrain();
                        drainLateWork = true;
                    } else {
                        processLifetimeRegistriesAtTermination(
                            lifetimeLocks.scanWeak,
                            lifetimeLocks.scanCleanup);
                    }
                }
                if (!waitForPublisher && !drainLateWork &&
                        gPhase == kPhaseMarkComplete) {
                    // Internal Sweep prevents a second coordinator from starting this transition.
                    // Public phase remains MarkComplete until all sweep cursors are ready.
                    gPhase = kPhaseSweep;
                    yumiGCRuntimeMarkEnd();
                    yumiGCRuntimeSweepStart();
                    heap().advanceSweepGeneration();
                    size_t transitionExpected = 0;
                    if (!gSweepTransitionPublishers.compare_exchange_strong(
                            transitionExpected,
                            kSweepTransitionAccepting,
                            std::memory_order_release,
                            std::memory_order_acquire)) {
                        abortGC("GC sweep transition publisher gate was not quiescent");
                    }
                    yumiGCDebugSweepTransitionAfterPublisherClose();
                }
            }
        }

        if (drainLateWork) {
            drainAll();
            std::this_thread::yield();
            continue;
        }
        if (waitForPublisher) {
            std::this_thread::yield();
            continue;
        }
        break;
    }

    yumiGCDebugSweepTransitionBeforeCursorReset();
    heap().resetIncrementalSweepCursors();
    {
        std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
        if (gPhase != kPhaseSweep || !gMarkWork.publishersClosed() ||
                !gMarkWork.quiescent() ||
                gActiveAllocationCount != 0) {
            abortGC("GC sweep transition lost quiescence");
        }
        publishPhase(gPhase);
        // Publishing Sweep before sealing the transition gives every racing external API a
        // complete handoff: it either joined the tracked pre-Sweep set or observes Sweep and
        // enters verification-only publication validation.
        size_t transition = gSweepTransitionPublishers.load(std::memory_order_acquire);
        while (transition & kSweepTransitionAccepting) {
            size_t sealed = transition & kSweepTransitionCountMask;
            if (gSweepTransitionPublishers.compare_exchange_weak(
                    transition,
                    sealed,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                break;
            }
        }
    }
    yumiGCDebugSweepTransitionAfterPublicSweep();
    while (gSweepTransitionPublishers.load(std::memory_order_acquire) != 0) {
        std::this_thread::yield();
    }
    return true;
}

static void
finishSweep() {
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    if (gPhase != kPhaseSweep) {
        return;
    }
    if (heap().pendingRetiredSpanCount() != 0) {
        abortGC("GC span retirement crossed the sweep-to-idle boundary");
    }
    yumiGCRuntimeSetMarkQueueDepth(0);
    yumiGCRuntimeSetAssistDebt(0);
    yumiGCRuntimeSweepEnd();
    yumiGCRuntimeFinishCollection();
    // Idle is the public completion boundary. Publish it only after cycle observability and
    // automatic-driver state have been finalized, so an acquire reader cannot observe Idle with
    // a live cycle age or outstanding debt.
    gPhase = kPhaseIdle;
    publishPhase(gPhase);
}

static void
retireMetadataAtSafepoint() {
    // Scheduler handshakes do not stop native non-task threads. Let an allocation that was
    // already admitted finish, then keep admission closed with gCollectorMutex while retired
    // span metadata is physically reclaimed. A new allocation either finishes before this
    // critical section or begins after every retired address has left the index.
    std::unique_lock<std::recursive_mutex> lock(gCollectorMutex);
    while (gActiveAllocationCount != 0) {
        lock.unlock();
        yumiGCDebugMetadataRetirementWaitingForAllocation();
        std::this_thread::yield();
        lock.lock();
    }
    if (!gMarkWork.quiescent()) {
        abortGC("GC metadata retirement requires quiescent marker state");
    }
    heap().retireQueuedSpansAtSafepoint();
}

static void
endRetirementHandshake() {
    yumiGCHandshakeEnd();
    yumiGCRuntimeHandshakeEnd();
    heap().unmapRetiredArenaMemory();
}

static void
retireMetadataWithSweepHandshake() {
    yumiGCRuntimeHandshakeStart();
    yumiGCHandshakeBegin(kHandshakeReasonSweep);
    yumiGCRuntimeHandshakeSetReason(kHandshakeReasonSweep);
    yumiGCRuntimeHandshakeWaitEnd();
    yumiGCRuntimeRecordPauseTaskWait(
        yumiGCHandshakeTasksWaited(),
        static_cast<size_t>(yumiGCHandshakeSlowestTaskNanos()));
    retireMetadataAtSafepoint();
    endRetirementHandshake();
}

static bool
beginAllocationActivity(bool rejectMarkSetup, bool* allocateBlack = nullptr) {
    if (allocateBlack) {
        *allocateBlack = false;
    }
    while (true) {
        std::unique_lock<std::recursive_mutex> lock(gCollectorMutex);
        if (rejectMarkSetup && gPhase == kPhaseMarkSetup) {
            return false;
        }
        // beginSweepAtSafepoint changes the internal phase before resetting the sweep
        // cursors, then release-publishes Sweep. A raw native thread is not parked by the
        // scheduler handshake, so do not let it enter that cursor-reset interval. Once public
        // Sweep is visible, generation-aware allocation may safely proceed alongside sweeping.
        if (gPhase == kPhaseSweep && publicCollectorPhase() != kPhaseSweep) {
            lock.unlock();
            yumiGCDebugAllocationObservedUnpublishedSweep();
            std::this_thread::yield();
            continue;
        }
        if (allocateBlack) {
            *allocateBlack = phaseNeedsMarkBarrier(gPhase);
        }
        ++gActiveAllocationCount;
        return true;
    }
}

static void
beginAllocation() {
    (void)beginAllocationActivity(false);
}

static bool
beginBatchReservation(bool* allocateBlack = nullptr) {
    return beginAllocationActivity(true, allocateBlack);
}

static void
finishAllocation(char* pointer) {
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    if (gActiveAllocationCount == 0) {
        abortGC("GC active allocation accounting underflow");
    }
    if (pointer && phaseNeedsMarkBarrier(gPhase)) {
        GCSpan* span = GCHeap::pointerToSpan(pointer);
        if (!span || !span->ownsObject(pointer)) {
            abortGC("GC allocated object missing from heap");
        }
        (void)span->tryMark(pointer);
        yumiGCRuntimeRecordAllocatedDuringMark();
    }
    --gActiveAllocationCount;
    finishMarkLocked();
}

static void
finishBatchReservation(
    char* pointer = nullptr,
    size_t count = 0,
    size_t slotSize = 0
) {
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    if (gActiveAllocationCount == 0) {
        abortGC("GC batch reservation accounting underflow");
    }
    if (phaseNeedsMarkBarrier(gPhase) && pointer && count) {
        GCSpan* span = GCHeap::pointerToSpan(pointer);
        if (!span || !span->markAllocatedRange(pointer, count, slotSize)) {
            abortGC("GC batch allocation range missing from heap");
        }
    }
    --gActiveAllocationCount;
    finishMarkLocked();
}

static void
ensureInitialized() {
    // Every scalar write barrier enters here. Keep its ordinary edge path free of the
    // collector mutex after the runtime has been initialized; the locked path below is
    // startup-only publication.
    if (gInitialized.load(std::memory_order_acquire)) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    if (gInitialized.load(std::memory_order_relaxed)) {
        return;
    }
    yumiGCStackMapLinkAnchor();
    gPhase = kPhaseIdle;
    publishPhase(gPhase);
    publishSweepGeneration(0);
    (void)heap();
    gInitialized.store(true, std::memory_order_release);
}

static void
startConcurrentMark(bool eagerClear) {
    ensureInitialized();
    if (publicCollectorPhase() != kPhaseIdle) {
        return;
    }
    yumiGCRuntimeFlushThreadAllocationBatches();

    uint32_t cycle = gCycle + 1;
    yumiGCRuntimeHandshakeStart();
    yumiGCRuntimeSubphaseStart(YumiGCObservationMarkSetup, cycle, 0);
    if (!beginMark(eagerClear)) {
        yumiGCRuntimeSubphaseEnd(YumiGCObservationMarkSetup, 0);
        yumiGCRuntimeHandshakeEnd();
        return;
    }
    yumiGCRuntimeSubphaseEnd(YumiGCObservationMarkSetup, 0);
    yumiGCRuntimeMarkStart();

    // Mark state exists before the request becomes visible, so each Running task can scan
    // its own live continuation, acknowledge this epoch, and continue immediately. Do not hold
    // the root-list mutex while waiting: allocation buffer growth registers temporary roots,
    // and a Running task blocked on that mutex must remain able to reach this handshake.
    yumiGCHandshakeBegin(kHandshakeReasonMarkSetup);
    yumiGCRuntimeHandshakeSetReason(kHandshakeReasonMarkSetup);
    yumiGCRuntimeHandshakeWaitEnd();
    yumiGCRuntimeRecordPauseTaskWait(
        yumiGCHandshakeTasksWaited(),
        static_cast<size_t>(yumiGCHandshakeSlowestTaskNanos()));
    yumiGCRuntimeSubphaseStart(YumiGCObservationRootScan, gCycle, 0);
    {
        std::lock_guard<std::mutex> rootLock(gRootMutex);
        GCMarkParticipantScope participant;
        traceOwnerStableAndStorageRootsLocked();
    }
    yumiGCRuntimeSubphaseEnd(YumiGCObservationRootScan, 0);
    enterConcurrentMark();
    yumiGCHandshakeEnd();
    yumiGCRuntimeHandshakeEnd();
}

static void
finishConcurrentMark() {
    ensureInitialized();
    if (publicCollectorPhase() != kPhaseConcurrentMark) {
        return;
    }

    // Complete the ordinary graph walk while mutators are still running. Any concurrent
    // deletion/insertion record is admitted through the same publisher gate and either joins
    // this drain or is donated by the flush-only confirmation below. Heap-sized work is not
    // charged to the termination pause.
    drainAll();
    uint64_t confirmBaseline = gMarkWork.workFlushedEpoch();
    yumiGCDebugTerminationBeforeConfirm();
    yumiGCRuntimeFlushThreadAllocationBatches();

    yumiGCRuntimeHandshakeStart();
    yumiGCHandshakeBegin(kHandshakeReasonMarkTermination);
    yumiGCRuntimeHandshakeSetReason(kHandshakeReasonMarkTermination);
    yumiGCRuntimeHandshakeWaitEnd();
    yumiGCRuntimeRecordPauseTaskWait(
        yumiGCHandshakeTasksWaited(),
        static_cast<size_t>(yumiGCHandshakeSlowestTaskNanos()));
    gTerminationConfirmCount.fetch_add(1, std::memory_order_relaxed);
    gTerminationConfirmTaskCount.fetch_add(
        yumiGCHandshakeTasksWaited(),
        std::memory_order_relaxed);

    enterMarkTermination();
    // Every non-owner task acknowledged only after flushing wbBuf/ycache state, then continued
    // independently. A marker that owned a batch when the request became visible may finish
    // that bounded batch before it reaches its poll. Keep the handshake closed until its
    // participant scope donates local work and leaves; no new scheduled marker can enter.
    while (gMarkWork.activeMarkerCount() != 0) {
        std::this_thread::yield();
    }
    // Inactive marker locals are now stable; this O(participants) pass force-publishes any
    // boundary handoff before the epoch check.
    gMarkWork.flushAllParticipants();
    bool confirmProducedWork =
        gMarkWork.workFlushedEpoch() != confirmBaseline || !gMarkWork.empty();
    size_t confirmDepth = gMarkWork.depth();
    if (confirmDepth != 0) {
        gTerminationConfirmResumeCount.fetch_add(1, std::memory_order_relaxed);
    }
    yumiGCRuntimeSubphaseStart(YumiGCObservationMarkTermination, gCycle, gMarkWork.depth());
    size_t confirmDrained = 0;
    if (confirmProducedWork) {
        size_t drained = 0;
        do {
            drained = drainLimit(kCollectorStepWork);
            confirmDrained += drained;
            if (!drained && !gMarkWork.empty()) {
                std::this_thread::yield();
            }
        } while (!gMarkWork.empty());
    }
    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
            if (gPhase == kPhaseMarkComplete || finishMarkLocked()) {
                break;
            }
        }
        size_t drained = drainLimit(kCollectorStepWork);
        confirmDrained += drained;
        if (!drained) {
            std::this_thread::yield();
        }
    }
    if (confirmDepth != 0 && confirmDrained == 0) {
        abortGC("GC termination confirmation resumed without consuming gray work");
    }
    if (checkmarkEnabled()) {
        // Checkmark computes its own root-to-heap fixed point in shadow bitmaps and compares
        // without publishing into normal gcWork. It cannot repair a missed normal edge.
        verifyCheckmark();
    }
    if (verifyMarkedHeapGraphEnabled()) {
        verifyMarkedHeapGraph();
    }
    if (verifyConservativeStackRootsEnabled()) {
        verifyConservativeStackRoots();
    }
    yumiGCRuntimeSubphaseEnd(YumiGCObservationMarkTermination, gMarkWork.depth());
    yumiGCHandshakeEnd();
    yumiGCRuntimeHandshakeEnd();
}

static void
sweepAll() {
    yumiGCRuntimeFlushThreadAllocationBatches();
    yumiGCRuntimeHandshakeStart();
    yumiGCHandshakeBegin(kHandshakeReasonSweep);
    yumiGCRuntimeHandshakeSetReason(kHandshakeReasonSweep);
    yumiGCRuntimeHandshakeWaitEnd();
    yumiGCRuntimeRecordPauseTaskWait(
        yumiGCHandshakeTasksWaited(),
        static_cast<size_t>(yumiGCHandshakeSlowestTaskNanos()));
    if (!beginSweepAtSafepoint()) {
        yumiGCHandshakeEnd();
        yumiGCRuntimeHandshakeEnd();
        return;
    }
    heap().sweepAll();
    if (sweepPublicationValidationEnabled() &&
            !heap().verifyLifecycle()) {
        abortGC("GC lifecycle verification failed");
    }
    retireMetadataAtSafepoint();
    endRetirementHandshake();
    scheduleCleanupTaskIfNeeded();
    finishSweep();
}

static bool
sweepDebtCleared() {
    return !heap().hasSweepDebt();
}

static void
startIncrementalSweep(size_t workLimit) {
    yumiGCRuntimeFlushThreadAllocationBatches();
    yumiGCRuntimeHandshakeStart();
    yumiGCHandshakeBegin(kHandshakeReasonSweep);
    yumiGCRuntimeHandshakeSetReason(kHandshakeReasonSweep);
    yumiGCRuntimeHandshakeWaitEnd();
    yumiGCRuntimeRecordPauseTaskWait(
        yumiGCHandshakeTasksWaited(),
        static_cast<size_t>(yumiGCHandshakeSlowestTaskNanos()));
    bool beganSweep = beginSweepAtSafepoint();
    yumiGCHandshakeEnd();
    yumiGCRuntimeHandshakeEnd();
    if (beganSweep) {
        scheduleCleanupTaskIfNeeded();
    }
    if (!beganSweep) {
        return;
    }
    heap().sweepLargeSpansStep(workLimit);
    heap().sweepIncrementalStep(workLimit);
    if (heap().incrementalSweepComplete()) {
        retireMetadataWithSweepHandshake();
        finishSweep();
    }
}

static void
sweepIncrementalStep(size_t workLimit) {
    if (workLimit == 0) {
        return;
    }
    if (publicCollectorPhase() == kPhaseMarkComplete) {
        startIncrementalSweep(workLimit);
        return;
    }
    if (publicCollectorPhase() != kPhaseSweep) {
        return;
    }
    yumiGCRuntimeSweepStart();
    heap().sweepLargeSpansStep(workLimit);
    heap().sweepIncrementalStep(workLimit);
    bool forceComplete = workLimit == std::numeric_limits<size_t>::max() && sweepDebtCleared();
    if (heap().incrementalSweepComplete() || forceComplete) {
        retireMetadataWithSweepHandshake();
        finishSweep();
    } else {
        yumiGCRuntimeSweepEnd();
    }
}

static char*
allocDescribed(size_t size, void* descriptor, bool recordAllocation) {
    ensureInitialized();
    if (!validateDescriptorForUse(descriptor, size)) {
        return nullptr;
    }
    size_t allocSize = objectAllocationSize(size);
    SizeClass sizeClass = sizeClassForSize(allocSize);
    size_t metadataSize = descriptor ? size : sizeClass.size;
    beginAllocation();
    yumiGCDebugAllocationAfterAdmission();
    char* pointer = heap().allocObject(sizeClass, metadataSize, descriptor);
    // GCSpan zeroes the slot before release-publishing its allocation bit. Record that
    // work here once, without rewriting the already-zeroed object after publication.
    yumiGCRuntimeRecordZeroedBytes(sizeClass.size);
    if (recordAllocation) {
        yumiGCRuntimeRecordAllocation(size, sizeClass.size);
    }
    finishAllocation(pointer);
    return pointer;
}

static bool
allocBatch(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    YumiGCAllocationBatch* batch,
    bool runningTask
) {
    if (!batch || size == 0 || preferredCount == 0) {
        return false;
    }
    ensureInitialized();
    if (!validateDescriptorForUse(descriptor, size)) {
        return false;
    }
    size_t allocSize = objectAllocationSize(size);
    SizeClass sizeClass = sizeClassForSize(allocSize);
    size_t metadataSize = descriptor ? size : sizeClass.size;
    size_t count = preferredCount;
    // The ycache owns bounded ranges whose refill cadence also drives incremental progress.
    // Honor the caller's class/descriptor-aware range size instead of silently inflating a
    // described eight-slot range into a 512/2048-slot legacy p-cache reservation.
    clampBatchCountToEnvCaps(sizeClass.size, count);
    clampBatchCountToHeapGoal(sizeClass.size, count);
    if (!reserveBatchCacheForCount(sizeClass.size, count)) {
        return false;
    }
    bool allocateBlack = false;
    if (runningTask) {
        // A Running task may acknowledge a scheduler handshake and continue until its next
        // safepoint while the handshake remains open. Reject that rare refill: a new handshake
        // that begins after this acquire-load must still wait for this unacknowledged task, so
        // the ordinary task path remains generation-safe without the native admission mutex.
        uint64_t control = __atomic_load_n(&yumiGCPublicControlWord, __ATOMIC_ACQUIRE);
        if (yumiGCControlWordHandshakeRequested(control)) {
            yumiGCRuntimeCancelBatchCacheReservation(count * sizeClass.size);
            return false;
        }
        allocateBlack = phaseNeedsMarkBarrier(yumiGCControlWordPhase(control));
    } else if (!beginBatchReservation(&allocateBlack)) {
        yumiGCRuntimeCancelBatchCacheReservation(count * sizeClass.size);
        return false;
    }

    char* pointer = nullptr;
    size_t allocatedCount = heap().allocBatch(
        sizeClass, metadataSize, descriptor, count, pointer, allocateBlack);
    if (!allocatedCount || !pointer) {
        if (!runningTask) {
            finishBatchReservation();
        }
        yumiGCRuntimeCancelBatchCacheReservation(count * sizeClass.size);
        return false;
    }
    yumiGCDebugBatchReservationAfterSpanPublication(
        pointer,
        allocatedCount,
        sizeClass.size);
    yumiGCRuntimeRecordBatchReservation(allocatedCount, sizeClass.size);
    if (!runningTask) {
        finishBatchReservation(pointer, allocatedCount, sizeClass.size);
    }
    batch->cursor = pointer;
    batch->limit = pointer + allocatedCount * sizeClass.size;
    batch->slotSize = sizeClass.size;
    return true;
}

static void
scavengeStep(
    size_t pageBudget,
    bool releaseArenas,
    bool respectRetentionTarget
) {
    if (pageBudget == 0 || publicCollectorPhase() != kPhaseIdle) {
        return;
    }
    yumiGCRuntimeFlushThreadAllocationBatches();
    yumiGCRuntimeHandshakeStart();
    yumiGCHandshakeBegin(kHandshakeReasonSweep);
    yumiGCRuntimeHandshakeSetReason(kHandshakeReasonSweep);
    yumiGCRuntimeHandshakeWaitEnd();
    yumiGCRuntimeRecordPauseTaskWait(
        yumiGCHandshakeTasksWaited(),
        static_cast<size_t>(yumiGCHandshakeSlowestTaskNanos()));
    yumiGCRuntimeSubphaseStart(
        YumiGCObservationScavenge,
        yumiGCRuntimeCurrentCycleId(),
        0);
    heap().releaseEmptySpansForScavenge();
    retireMetadataAtSafepoint();
    size_t released = heap().scavengeStep(
        pageBudget,
        releaseArenas,
        respectRetentionTarget);
    retireMetadataAtSafepoint();
    if (released) {
        yumiGCRuntimeRecordScavengePages(released);
        yumiGCRuntimeSubphaseEnd(YumiGCObservationScavenge, released);
    } else {
        yumiGCRuntimeSubphaseCancel(YumiGCObservationScavenge);
    }
    endRetirementHandshake();
}

static void
finishAutomaticCycle() {
    ensureInitialized();
    unsigned char phase = publicCollectorPhase();
    if (phase == kPhaseConcurrentMark) {
        finishConcurrentMark();
    }
    if (publicCollectorPhase() == kPhaseMarkComplete) {
        startIncrementalSweep(kLazySweepStepBudget);
    } else if (publicCollectorPhase() == kPhaseSweep) {
        sweepIncrementalStep(kLazySweepStepBudget);
    }
}

static size_t gSinglePointerOffsets[] = {0};
static GCVecState gSinglePointerOffsetState = {gSinglePointerOffsets, 1, 1};
static GCVecState* gSinglePointerOffsetSlot = &gSinglePointerOffsetState;
static GCVecState* gEmptySlot = nullptr;
static GCDescriptor gSinglePointerDescriptor = {
    sizeof(void*) * 2,
    {&gSinglePointerOffsetSlot},
    {&gEmptySlot},
    0,
    {&gEmptySlot},
    nullptr,
    {&gEmptySlot},
    false,
    false
};
static GCDescriptor gAlternateSinglePointerDescriptor = {
    sizeof(void*) * 2,
    {&gSinglePointerOffsetSlot},
    {&gEmptySlot},
    0,
    {&gEmptySlot},
    nullptr,
    {&gEmptySlot},
    false,
    false
};

} // namespace

extern "C" void
yumiGCVisitDescriptorPointerSlots(
    char* object,
    size_t byteSize,
    void* descriptor,
    YumiGCDescriptorPointerSlotVisitor visitor,
    void* data
) {
    if (!object || !byteSize || !descriptor || !visitor ||
            !validateDescriptorForUse(descriptor, byteSize)) {
        return;
    }
    walkDescriptorFields(
        object,
        descriptor,
        byteSize,
        nullptr,
        [visitor, data](char* fieldObject, void*, size_t objectSize, size_t offset, char*) {
            if (fieldObject && objectSize >= sizeof(char*) &&
                    offset <= objectSize - sizeof(char*)) {
                visitor(reinterpret_cast<char**>(fieldObject + offset), data);
            }
        });
}

extern "C" bool
yumiGCIsManagedAddress(char* pointer) {
    return pointer && GCHeap::pointerToSpan(pointer);
}

using namespace YuGC;

extern "C" void
yumiGCInit(void) {
    ensureInitialized();
}

extern "C" void
yumiGCStartConcurrentMark(void) {
    if (yumiGCCollectionDisabled()) {
        return;
    }
    startConcurrentMark(false);
}

extern "C" size_t
yumiGCStepConcurrentMark(void) {
    if (yumiGCCollectionDisabled()) {
        return 0;
    }
    ensureInitialized();
    return drainLimit(kCollectorStepWork);
}

extern "C" void
yumiGCFinishConcurrentMark(void) {
    if (yumiGCCollectionDisabled()) {
        return;
    }
    finishConcurrentMark();
}

extern "C" void
yumiGCFinishAutomaticCycle(void) {
    if (yumiGCCollectionDisabled()) {
        return;
    }
    finishAutomaticCycle();
}

extern "C" void
yumiGCStepIncrementalSweep(size_t workLimit) {
    if (yumiGCCollectionDisabled()) {
        return;
    }
    ensureInitialized();
    sweepIncrementalStep(workLimit);
}

extern "C" void
yumiGCScavenge(void) {
    if (yumiGCCollectionDisabled()) {
        return;
    }
    ensureInitialized();
    scavengeStep(std::numeric_limits<size_t>::max(), true, false);
}

extern "C" void
yumiGCStepScavenge(size_t pageBudget) {
    if (yumiGCCollectionDisabled()) {
        return;
    }
    ensureInitialized();
    scavengeStep(pageBudget, false, true);
}

extern "C" size_t
yumiGCScavengeMinimumStepPages(void) {
    return scavengeUnitPages();
}

extern "C" size_t
yumiGCMutatorAssist(size_t workLimit) {
    if (yumiGCCollectionDisabled()) {
        return 0;
    }
    ensureInitialized();
    if (!workLimit) {
        return 0;
    }
    bool marking = false;
    {
        std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
        marking = phaseNeedsMarkBarrier(gPhase);
        if (marking) {
            yumiGCRuntimeRecordAssist();
        }
    }
    if (marking) {
        yumiGCRuntimeSubphaseStart(YumiGCObservationAssist, gCycle, 0);
        size_t drained = drainLimit(workLimit);
        if (drained) {
            yumiGCRuntimeSubphaseEnd(YumiGCObservationAssist, drained);
        } else {
            yumiGCRuntimeSubphaseCancel(YumiGCObservationAssist);
        }
        return drained;
    }
    return 0;
}

extern "C" unsigned char
yumiGCCollectorPhase(void) {
    if (yumiGCCollectionDisabled()) {
        return 0;
    }
    return publicCollectorPhase();
}

extern "C" char*
yumiGCAllocObject(size_t size) {
    return allocDescribed(size, nullptr, true);
}

extern "C" char*
yumiGCAllocObjectDescribed(size_t size, void* descriptor) {
    return allocDescribed(size, descriptor, true);
}

extern "C" char*
yumiGCAllocBuffer(size_t itemSize, size_t count) {
    if (itemSize != 0 && count > std::numeric_limits<size_t>::max() / itemSize) {
        return nullptr;
    }
    size_t size = itemSize * count;
    return size ? allocDescribed(size, nullptr, true) : nullptr;
}

extern "C" char*
yumiGCAllocBufferDescribed(size_t itemSize, size_t count, void* descriptor) {
    if (itemSize != 0 && count > std::numeric_limits<size_t>::max() / itemSize) {
        return nullptr;
    }
    size_t size = itemSize * count;
    return size ? allocDescribed(size, descriptor, true) : nullptr;
}

extern "C" char*
yumiGCResizeBuffer(char* buffer, size_t oldSize, size_t newSize) {
    return yumiGCResizeBufferDescribed(buffer, oldSize, newSize, nullptr);
}

extern "C" YumiGCBufferResize
yumiGCResizeBufferPrepare(
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* descriptor,
    void** replacementDescriptor
) {
    ensureInitialized();
    if (!validateDescriptorForUse(descriptor, newSize ? newSize : oldSize)) {
        return YumiGCBufferResizeRejected;
    }
    if (newSize == 0) {
        return YumiGCBufferResizeRejected;
    }
    if (!buffer || oldSize == 0) {
        *replacementDescriptor = descriptor;
        return YumiGCBufferResizeReplace;
    }

    GCSpan* span = GCHeap::pointerToSpan(buffer);
    void* existingDescriptor = span ? span->descriptor(buffer) : nullptr;
    if (span && span->ownsObject(buffer) && newSize <= span->sizeClass.size) {
        if (descriptor && (newSize < oldSize || descriptor != existingDescriptor)) {
            yumiGCBulkWriteBarrier(buffer, nullptr, oldSize, existingDescriptor);
        }
        if (newSize > oldSize) {
            std::memset(buffer + oldSize, 0, newSize - oldSize);
            yumiGCRuntimeRecordZeroedBytes(newSize - oldSize);
        }
        span->updateObjectMetadata(buffer, newSize, descriptor ? descriptor : existingDescriptor);
        return YumiGCBufferResizeInPlace;
    }

    if (descriptor && descriptor != existingDescriptor) {
        yumiGCBulkWriteBarrier(buffer, nullptr, oldSize, existingDescriptor);
    }
    void* resizedDescriptor = descriptor ? descriptor : existingDescriptor;
    size_t copySize = std::min(oldSize, newSize);
    if (copySize && resizedDescriptor) {
        // Runtime memcpy has no compiler-emitted element stores; record the source contents
        // before another sweeper can reclaim copied children.
        yumiGCBulkWriteBarrier(nullptr, buffer, copySize, resizedDescriptor);
    }
    *replacementDescriptor = resizedDescriptor;
    return YumiGCBufferResizeReplace;
}

extern "C" void
yumiGCResizeBufferCopy(
    char* replacement,
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* replacementDescriptor
) {
    if (!buffer || oldSize == 0) {
        return;
    }
    size_t copySize = std::min(oldSize, newSize);
    if (copySize && replacementDescriptor) {
        // Allocation may service the MarkSetup handshake. Re-sample and record immediately
        // before memcpy so a pre-allocation Idle observation cannot publish an unrecorded edge
        // into the allocate-black replacement.
        yumiGCBulkWriteBarrier(replacement, buffer, copySize, replacementDescriptor);
    }
    std::memcpy(replacement, buffer, copySize);
}

extern "C" char*
yumiGCResizeBufferDescribed(char* buffer, size_t oldSize, size_t newSize, void* descriptor) {
    void* replacementDescriptor = nullptr;
    YumiGCBufferResize resize = yumiGCResizeBufferPrepare(
        buffer,
        oldSize,
        newSize,
        descriptor,
        &replacementDescriptor);
    if (resize == YumiGCBufferResizeRejected) {
        return nullptr;
    }
    if (resize == YumiGCBufferResizeInPlace) {
        return buffer;
    }
    char* resized = allocDescribed(newSize, replacementDescriptor, true);
    if (!resized) {
        return nullptr;
    }
    yumiGCResizeBufferCopy(resized, buffer, oldSize, newSize, replacementDescriptor);
    return resized;
}

extern "C" bool
yumiGCAllocBatchForUnregisteredThread(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    YumiGCAllocationBatch* batch
) {
    return allocBatch(size, descriptor, preferredCount, batch, false);
}

extern "C" bool
yumiGCTestAllocBatch(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    YumiGCAllocationBatch* batch
) {
    return yumiGCAllocBatchForUnregisteredThread(
        size,
        descriptor,
        preferredCount,
        batch);
}

extern "C" bool
yumiGCAllocBatchForRunningTask(
    size_t size,
    void* descriptor,
    size_t preferredCount,
    YumiGCAllocationBatch* batch
) {
    return allocBatch(size, descriptor, preferredCount, batch, true);
}

extern "C" bool
yumiGCReleaseUnusedBatch(
    char* cursor,
    char* limit,
    size_t slotSize,
    size_t generation
) {
    ensureInitialized();
    if (generation != yumiGCPublicSweepGeneration()) {
        return false;
    }
    if (!beginBatchReservation()) {
        return false;
    }
    bool released = heap().releaseBatch(cursor, limit, slotSize);
    finishBatchReservation();
    return released;
}

extern "C" bool
yumiGCDiscardUnusedBatch(char* cursor, char* limit, size_t slotSize, size_t generation) {
    ensureInitialized();
    if (!beginBatchReservation()) {
        return false;
    }
    {
        // Sweep takes this same lock around each bitmap flip. Classify the unused tail before
        // its old-generation span flips, or observe the new sweep generation and leave the
        // already-completed flip untouched. Do not hold the collector lock while entering the
        // p-cache/store locks below; sweep uses the opposite nesting order.
        std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
        GCSpan* span = GCHeap::pointerToSpan(cursor);
        if (gPhase != kPhaseIdle && span && span->sweepGeneration() == generation) {
            (void)span->deferDiscardedBatch(cursor, limit, slotSize);
        }
    }
    bool discarded = heap().discardBatch(cursor, limit, slotSize, generation);
    finishBatchReservation();
    return discarded;
}

extern "C" bool
yumiGCTestDiscardBatch(char* cursor, char* limit, size_t slotSize, size_t generation) {
    return yumiGCDiscardUnusedBatch(cursor, limit, slotSize, generation);
}

extern "C" void*
yumiGCAddRoot(char* pointer) {
    ensureInitialized();
    Root* root = new (std::nothrow) Root();
    if (!root) {
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(gRootMutex);
        ExternalMarkResult markResult = pushExternalMark(pointer);
        std::unique_lock<std::recursive_mutex> collectorLock(
            gCollectorMutex,
            std::defer_lock);
        if (isExternalMarkTransition(markResult)) {
            collectorLock.lock();
            validateSweepPublicationPointer(pointer);
        }
        root->pointer = pointer;
        root->previousLink = &gFirstRoot;
        root->next = gFirstRoot;
        if (gFirstRoot) {
            gFirstRoot->previousLink = &root->next;
        }
        gFirstRoot = root;
        finishExternalMark(markResult);
    }
    return root;
}

static void
linkTemporaryRoot(YumiGCTemporaryRootStorage* storage, Root* root, char* pointer) {
    Root** head = reinterpret_cast<Root**>(&storage->head);
    root->pointer = pointer;
    root->previousLink = head;
    root->next = *head;
    root->temporaryStorage = storage;
    if (*head) {
        (*head)->previousLink = &root->next;
    }
    *head = root;
    ++storage->pushCount;
    ++storage->depth;
    storage->maxDepth = std::max(storage->maxDepth, storage->depth);
}

extern "C" void*
yumiGCAddTemporaryRoot(char* pointer) {
    ensureInitialized();
    YumiGCTemporaryRootStorage* storage = yumiGCActiveTaskTemporaryRootStorage();
    if (!storage) {
        return yumiGCAddRoot(pointer);
    }
    Root* root = acquireTemporaryRoot(storage);
    if (!root) {
        return nullptr;
    }
    ExternalMarkResult markResult = pushExternalMark(pointer);
    std::unique_lock<std::recursive_mutex> collectorLock(
        gCollectorMutex,
        std::defer_lock);
    if (sweepTransitionValidates(markResult)) {
        collectorLock.lock();
        validateSweepPublicationPointer(pointer);
    }
    linkTemporaryRoot(storage, root, pointer);
    finishExternalMark(markResult);
    return root;
}

extern "C" void*
yumiGCAddIdleTemporaryRoot(char* pointer) {
    if (!gInitialized.load(std::memory_order_acquire) ||
            publicCollectorPhase() != kPhaseIdle) {
        return nullptr;
    }
    YumiGCTemporaryRootStorage* storage = yumiGCActiveTaskTemporaryRootStorage();
    if (!storage) {
        return nullptr;
    }
    Root* root = acquireTemporaryRoot(storage);
    if (!root) {
        return nullptr;
    }
    linkTemporaryRoot(storage, root, pointer);
    return root;
}

extern "C" void*
yumiGCAddDescribedRoot(char* pointer, size_t byteSize, void* descriptor) {
    ensureInitialized();
    if (!pointer || !descriptor || byteSize == 0 ||
        !validateDescriptorForUse(descriptor, byteSize)) {
        return nullptr;
    }
    Root* root = new (std::nothrow) Root();
    if (!root) {
        return nullptr;
    }
    // Registering externally owned descriptor storage publishes every managed edge in the
    // buffer. Reuse the bulk-barrier transition protocol so a registration racing mark
    // termination or sweep cannot hide an old white object from the collector.
    {
        std::lock_guard<std::mutex> lock(gRootMutex);
        yumiGCBulkWriteBarrier(nullptr, pointer, byteSize, descriptor);
        root->pointer = pointer;
        root->descriptor = descriptor;
        root->byteSize = byteSize;
        root->previousLink = &gFirstRoot;
        root->next = gFirstRoot;
        if (gFirstRoot) {
            gFirstRoot->previousLink = &root->next;
        }
        gFirstRoot = root;
    }
    return root;
}

extern "C" void
yumiGCUpdateRoot(void* rootHandle, char* pointer) {
    ensureInitialized();
    if (!rootHandle) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(gRootMutex);
        Root* root = static_cast<Root*>(rootHandle);
        if (root->descriptor) {
            yumiGCBulkWriteBarrier(nullptr, pointer, root->byteSize, root->descriptor);
            root->pointer = pointer;
            return;
        }
        ExternalMarkResult markResult = pushExternalMark(pointer);
        std::unique_lock<std::recursive_mutex> collectorLock(
            gCollectorMutex,
            std::defer_lock);
        if (sweepTransitionValidates(markResult)) {
            collectorLock.lock();
            validateSweepPublicationPointer(pointer);
        }
        root->pointer = pointer;
        finishExternalMark(markResult);
    }
}

extern "C" void
yumiGCRemoveRoot(void* rootHandle) {
    ensureInitialized();
    if (!rootHandle) {
        return;
    }
    Root* root = static_cast<Root*>(rootHandle);
    std::lock_guard<std::mutex> lock(gRootMutex);
    gTestRootUnlinkCount.fetch_add(1, std::memory_order_relaxed);
    *root->previousLink = root->next;
    if (root->next) {
        root->next->previousLink = root->previousLink;
    }
    root->previousLink = nullptr;
    root->next = nullptr;
    delete root;
}

static void
unlinkTemporaryRoot(YumiGCTemporaryRootStorage* storage, Root* root) {
    *root->previousLink = root->next;
    if (root->next) {
        root->next->previousLink = root->previousLink;
    }
    root->previousLink = nullptr;
    root->next = nullptr;
    ++storage->popCount;
    if (storage->depth) {
        --storage->depth;
    }
    releaseTemporaryRoot(root);
}

extern "C" void
yumiGCRemoveTemporaryRoot(void* rootHandle) {
    if (!rootHandle) {
        return;
    }
    Root* root = static_cast<Root*>(rootHandle);
    YumiGCTemporaryRootStorage* storage = root->temporaryStorage;
    if (!storage) {
        yumiGCRemoveRoot(rootHandle);
        return;
    }
    unlinkTemporaryRoot(storage, root);
}

extern "C" bool
yumiGCRemoveTaskTemporaryRoot(void* rootHandle) {
    if (!rootHandle) {
        return true;
    }
    Root* root = static_cast<Root*>(rootHandle);
    YumiGCTemporaryRootStorage* storage = root->temporaryStorage;
    if (!storage) {
        return false;
    }
    unlinkTemporaryRoot(storage, root);
    return true;
}

extern "C" void
yumiGCDrainTemporaryRoots(YumiGCTemporaryRootStorage* storage) {
    if (!storage) {
        return;
    }
    Root*& head = *reinterpret_cast<Root**>(&storage->head);
    while (head) {
        Root* root = head;
        head = root->next;
        ++storage->popCount;
        if (storage->depth) {
            --storage->depth;
        }
        releaseTemporaryRoot(root);
    }
    destroyTemporaryRootStorage(storage);
    gTemporaryRootPushCount.fetch_add(storage->pushCount, std::memory_order_relaxed);
    gTemporaryRootPopCount.fetch_add(storage->popCount, std::memory_order_relaxed);
    gTemporaryRootStorageAllocationCount.fetch_add(
        storage->storageAllocationCount,
        std::memory_order_relaxed);
    gTemporaryRootSlabGrowthCount.fetch_add(
        storage->slabGrowthCount,
        std::memory_order_relaxed);
    size_t observedMax = gTemporaryRootMaxDepth.load(std::memory_order_relaxed);
    while (observedMax < storage->maxDepth &&
        !gTemporaryRootMaxDepth.compare_exchange_weak(
            observedMax,
            storage->maxDepth,
            std::memory_order_relaxed)) {
    }
    storage->depth = 0;
    storage->maxDepth = 0;
    storage->pushCount = 0;
    storage->popCount = 0;
    storage->storageAllocationCount = 0;
    storage->slabGrowthCount = 0;
}

extern "C" void
yumiGCCollect(void) {
    if (yumiGCCollectionDisabled()) {
        return;
    }
    ensureInitialized();
    startConcurrentMark(false);
    finishConcurrentMark();
    if (publicCollectorPhase() == kPhaseMarkComplete) {
        sweepAll();
    }
}

extern "C" void
yumiGCCollectForAllocation(void) {
    if (yumiGCCollectionDisabled()) {
        return;
    }
    yumiGCCollect();
}

extern "C" bool
yumiGCVerifyObject(char* pointer) {
    return heap().verifyObject(pointer);
}

extern "C" void*
yumiGCObjectDescriptor(char* pointer) {
    return heap().objectDescriptor(pointer);
}

static char*
shadeWeakTargetForLoad(char* target) {
    if (!target) {
        return nullptr;
    }
    unsigned char phase = publicCollectorPhase();
    if (phase == kPhaseIdle) {
        return target;
    }
    if (phase == kPhaseSweep) {
        validateSweepPublicationPointer(target);
        return target;
    }
    GCSpan* span = GCHeap::pointerToSpan(target);
    if (span && span->isMarked(target)) {
        return target;
    }
    ExternalMarkResult markResult = pushExternalMark(target);
    std::unique_lock<std::recursive_mutex> collectorLock(gCollectorMutex, std::defer_lock);
    if (sweepTransitionValidates(markResult)) {
        collectorLock.lock();
        validateSweepPublicationPointer(target);
    }
    finishExternalMark(markResult);
    return target;
}

extern "C" size_t
yumiGCRegisterOwnerWeak(char* owner, char* target) {
    ensureInitialized();
    GCSpan* ownerSpan = GCHeap::pointerToSpan(owner);
    char* ownerObject = ownerSpan ? ownerSpan->objectBase(owner) : nullptr;
    GCSpan* targetSpan = GCHeap::pointerToSpan(target);
    char* targetObject = targetSpan ? targetSpan->objectBase(target) : nullptr;
    if (!ownerObject || !targetObject) {
        return 0;
    }

    size_t shard = lifetimeRegistryShard(ownerObject);
    OwnerWeakShard& shardState = gOwnerWeakShards[shard];
    std::lock_guard<std::mutex> lock(gWeakRegistryMutexes[shard]);
    uint32_t index = 0;
    OwnerWeakSlot* slot = allocateOwnerWeakSlot(shardState, index);
    if (!slot) {
        return 0;
    }
    ExternalMarkResult markResult = pushExternalMark(targetObject);
    std::unique_lock<std::recursive_mutex> collectorLock(gCollectorMutex, std::defer_lock);
    if (isExternalMarkTransition(markResult)) {
        collectorLock.lock();
        validateSweepPublicationPointer(targetObject);
    }
    slot->owner = ownerObject;
    slot->ownerSpan = ownerSpan;
    slot->targetSpan = targetSpan;
    slot->target.store(targetObject, std::memory_order_release);
    size_t handle = ownerWeakHandle(
        shard,
        index,
        slot->generation.load(std::memory_order_relaxed));
    gOwnerWeakRegistrationCount.fetch_add(1, std::memory_order_relaxed);
    finishExternalMark(markResult);
    return handle;
}

extern "C" char*
yumiGCOwnerWeakLoad(size_t handle) {
    if (!handle) {
        return nullptr;
    }
    size_t shard = ownerWeakHandleShard(handle);
    uint32_t generation = ownerWeakHandleGeneration(handle);
    OwnerWeakSlot* slot = ownerWeakSlot(gOwnerWeakShards[shard], ownerWeakHandleIndex(handle));
    if (!slot) {
        return nullptr;
    }
    uint64_t control = __atomic_load_n(&yumiGCPublicControlWord, __ATOMIC_ACQUIRE);
    if (yumiGCControlWordPhase(control) == kPhaseIdle) {
        char* target = slot->target.load(std::memory_order_acquire);
        if (slot->generation.load(std::memory_order_relaxed) == generation &&
                __atomic_load_n(&yumiGCPublicControlWord, __ATOMIC_RELAXED) == control) {
            return target;
        }
    }
    std::lock_guard<std::mutex> lock(gWeakRegistryMutexes[shard]);
    if (slot->generation.load(std::memory_order_relaxed) != generation) {
        return nullptr;
    }
    return shadeWeakTargetForLoad(slot->target.load(std::memory_order_relaxed));
}

extern "C" bool
yumiGCReleaseOwnerWeak(size_t handle) {
    if (!handle) {
        return false;
    }
    size_t shard = ownerWeakHandleShard(handle);
    uint32_t index = ownerWeakHandleIndex(handle);
    OwnerWeakShard& shardState = gOwnerWeakShards[shard];
    std::lock_guard<std::mutex> lock(gWeakRegistryMutexes[shard]);
    OwnerWeakSlot* slot = ownerWeakSlot(shardState, index);
    if (!slot || !slot->owner ||
            slot->generation.load(std::memory_order_relaxed) !=
                ownerWeakHandleGeneration(handle)) {
        return false;
    }
    freeOwnerWeakSlot(shardState, *slot, index);
    gOwnerWeakRegistrationCount.fetch_sub(1, std::memory_order_relaxed);
    return true;
}

static size_t
registerCleanup(
    char* target,
    char* context,
    YumiGCCleanupCallback callback,
    bool managedContext
) {
    // Managed callbacks root and validate their state. State-free native callbacks use a null
    // context so registration itself adds no managed allocation or retaining path.
    ensureInitialized();
    GCSpan* targetSpan = GCHeap::pointerToSpan(target);
    char* targetObject = targetSpan ? targetSpan->objectBase(target) : nullptr;
    if (!targetObject || !callback || (managedContext && !context)) {
        return 0;
    }

    void* contextRoot = nullptr;
    if (managedContext) {
        contextRoot = yumiGCAddRoot(context);
        if (!contextRoot) {
            return 0;
        }
        bool retainsTarget = false;
        {
            std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
            retainsTarget = managedGraphContains(context, targetObject);
        }
        if (retainsTarget) {
            yumiGCRemoveRoot(contextRoot);
            return 0;
        }
    }

    size_t shard = lifetimeRegistryShard(targetObject);
    size_t sequence = gNextCleanupHandle.fetch_add(1, std::memory_order_relaxed);
    if (!sequence || sequence > (std::numeric_limits<size_t>::max() - shard) /
            kLifetimeRegistryShardCount) {
        if (contextRoot) {
            yumiGCRemoveRoot(contextRoot);
        }
        return 0;
    }
    size_t handle = sequence * kLifetimeRegistryShardCount + shard;
    {
        std::lock_guard<std::mutex> lock(gCleanupRegistryMutexes[shard]);
        ExternalMarkResult markResult = pushExternalMark(targetObject);
        std::unique_lock<std::recursive_mutex> collectorLock(
            gCollectorMutex,
            std::defer_lock);
        if (isExternalMarkTransition(markResult)) {
            collectorLock.lock();
            validateSweepPublicationPointer(targetObject);
        }
        targetSpan->addCleanupRegistration();
        CleanupRegistration entry;
        entry.handle = handle;
        entry.target = targetObject;
        entry.targetSpan = targetSpan;
        entry.context = context;
        entry.contextRoot = contextRoot;
        entry.callback = callback;
        auto& entries = gCleanupRegistries[shard];
        entries.push_back(entry);
        gCleanupRegistryIndices[shard][handle] = entries.size() - 1;
        finishExternalMark(markResult);
    }
    gCleanupRegistrationCount.fetch_add(1, std::memory_order_relaxed);
    return handle;
}

extern "C" size_t
yumiGCRegisterCleanup(
    char* target,
    char* context,
    YumiGCCleanupCallback callback
) {
    return registerCleanup(target, context, callback, true);
}

static bool
cancelCleanup(size_t handle, char** context) {
    if (!handle) {
        return false;
    }
    size_t shard = handle % kLifetimeRegistryShardCount;
    void* contextRoot = nullptr;
    bool canceled = false;
    {
        std::lock_guard<std::mutex> lock(gCleanupRegistryMutexes[shard]);
        auto& entries = gCleanupRegistries[shard];
        auto& indices = gCleanupRegistryIndices[shard];
        gTestCleanupCancelLookupCount.fetch_add(1, std::memory_order_relaxed);
        auto found = indices.find(handle);
        if (found != indices.end()) {
            size_t index = found->second;
            CleanupRegistration entry = entries[index];
            indices.erase(found);
            if (index + 1 != entries.size()) {
                CleanupRegistration moved = entries.back();
                entries[index] = moved;
                indices[moved.handle] = index;
            }
            entries.pop_back();
            entry.targetSpan->removeCleanupRegistration();
            contextRoot = entry.contextRoot;
            if (context) {
                *context = entry.context;
            }
            canceled = true;
            gCleanupRegistrationCount.fetch_sub(1, std::memory_order_relaxed);
            gCleanupCancelCount.fetch_add(1, std::memory_order_relaxed);
        }
    }
    if (contextRoot) {
        yumiGCRemoveRoot(contextRoot);
    }
    return canceled;
}

extern "C" bool
yumiGCCancelCleanup(size_t handle) {
    return cancelCleanup(handle, nullptr);
}

extern "C" void
yumiGCTestResetLifetimeLookupCounters(void) {
    gTestCleanupCancelLookupCount.store(0, std::memory_order_relaxed);
    gTestRootUnlinkCount.store(0, std::memory_order_relaxed);
}

extern "C" size_t
yumiGCTestCleanupCancelLookupCount(void) {
    return gTestCleanupCancelLookupCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCTestRootUnlinkCount(void) {
    return gTestRootUnlinkCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCTestRootRegistrationCount(void) {
    std::lock_guard<std::mutex> lock(gRootMutex);
    size_t count = 0;
    for (Root* root = gFirstRoot; root; root = root->next) {
        ++count;
    }
    return count;
}

static size_t
temporaryRootCounterWithActive(
    const std::atomic<size_t>& completed,
    size_t YumiGCTemporaryRootStorage::*member
) {
    size_t value = completed.load(std::memory_order_relaxed);
    YumiGCTemporaryRootStorage* storage = yumiGCActiveTaskTemporaryRootStorage();
    return storage ? value + storage->*member : value;
}

extern "C" size_t
yumiGCTemporaryRootPushCount(void) {
    return temporaryRootCounterWithActive(
        gTemporaryRootPushCount,
        &YumiGCTemporaryRootStorage::pushCount);
}

extern "C" size_t
yumiGCTemporaryRootPopCount(void) {
    return temporaryRootCounterWithActive(
        gTemporaryRootPopCount,
        &YumiGCTemporaryRootStorage::popCount);
}

extern "C" size_t
yumiGCTemporaryRootStorageAllocationCount(void) {
    return temporaryRootCounterWithActive(
        gTemporaryRootStorageAllocationCount,
        &YumiGCTemporaryRootStorage::storageAllocationCount);
}

extern "C" size_t
yumiGCTemporaryRootSlabGrowthCount(void) {
    return temporaryRootCounterWithActive(
        gTemporaryRootSlabGrowthCount,
        &YumiGCTemporaryRootStorage::slabGrowthCount);
}

extern "C" size_t
yumiGCTemporaryRootMaxDepth(void) {
    size_t value = gTemporaryRootMaxDepth.load(std::memory_order_relaxed);
    YumiGCTemporaryRootStorage* storage = yumiGCActiveTaskTemporaryRootStorage();
    return storage ? std::max(value, storage->maxDepth) : value;
}

extern "C" void
yumiGCResetActiveTemporaryRootMetrics(void) {
    YumiGCTemporaryRootStorage* storage = yumiGCActiveTaskTemporaryRootStorage();
    if (!storage) {
        return;
    }
    storage->maxDepth = storage->depth;
    storage->pushCount = 0;
    storage->popCount = 0;
    storage->storageAllocationCount = 0;
    storage->slabGrowthCount = 0;
}

extern "C" void
yumiGCResetTemporaryRootMetrics(void) {
    gTemporaryRootPushCount.store(0, std::memory_order_relaxed);
    gTemporaryRootPopCount.store(0, std::memory_order_relaxed);
    gTemporaryRootStorageAllocationCount.store(0, std::memory_order_relaxed);
    gTemporaryRootSlabGrowthCount.store(0, std::memory_order_relaxed);
    gTemporaryRootMaxDepth.store(0, std::memory_order_relaxed);
    yumiGCResetActiveTemporaryRootMetrics();
}

static bool
validateTemporaryRootList(YumiGCTemporaryRootStorage* storage, size_t expectedCount) {
    Root** expectedLink = reinterpret_cast<Root**>(&storage->head);
    Root* root = static_cast<Root*>(storage->head);
    size_t count = 0;
    while (root) {
        if (root->previousLink != expectedLink || root->temporaryStorage != storage) {
            return false;
        }
        expectedLink = &root->next;
        root = root->next;
        ++count;
    }
    return count == expectedCount;
}

extern "C" bool
yumiGCTestTemporaryRootStorage(char* pointer) {
    YumiGCTemporaryRootStorage* storage = yumiGCActiveTaskTemporaryRootStorage();
    if (!storage || !pointer) {
        return false;
    }
    yumiGCResetActiveTemporaryRootMetrics();
    void* roots[kTemporaryRootSlabCapacity];
    for (size_t index = 0; index < 4; ++index) {
        roots[index] = yumiGCAddTemporaryRoot(pointer);
        if (!roots[index] || !validateTemporaryRootList(storage, index + 1)) {
            return false;
        }
        yumiGCCollect();
        if (!yumiGCVerifyObject(pointer)) {
            return false;
        }
    }
    yumiGCRemoveTemporaryRoot(roots[1]);
    if (!validateTemporaryRootList(storage, 3) ||
        static_cast<Root*>(roots[1])->pointer) {
        return false;
    }
    yumiGCRemoveTemporaryRoot(roots[3]);
    yumiGCRemoveTemporaryRoot(roots[0]);
    yumiGCRemoveTemporaryRoot(roots[2]);
    if (!validateTemporaryRootList(storage, 0) || storage->depth != 0) {
        return false;
    }

    for (size_t index = 0; index < kTemporaryRootSlabCapacity; ++index) {
        roots[index] = yumiGCAddTemporaryRoot(pointer);
        if (!roots[index]) {
            return false;
        }
    }
    if (!validateTemporaryRootList(storage, kTemporaryRootSlabCapacity) ||
        storage->storageAllocationCount != 1 || storage->slabGrowthCount != 1) {
        return false;
    }
    yumiGCCollect();
    for (size_t index = kTemporaryRootSlabCapacity; index > 0; --index) {
        yumiGCRemoveTemporaryRoot(roots[index - 1]);
    }
    size_t allocationCount = storage->storageAllocationCount;
    size_t slabGrowthCount = storage->slabGrowthCount;
    for (size_t index = 0; index < kTemporaryRootSlabCapacity; ++index) {
        roots[index] = yumiGCAddTemporaryRoot(pointer);
        if (!roots[index]) {
            return false;
        }
    }
    for (size_t index = 0; index < kTemporaryRootSlabCapacity; ++index) {
        yumiGCRemoveTemporaryRoot(roots[index]);
    }
    return validateTemporaryRootList(storage, 0) && storage->depth == 0 &&
        storage->maxDepth == kTemporaryRootSlabCapacity &&
        storage->storageAllocationCount == allocationCount &&
        storage->slabGrowthCount == slabGrowthCount;
}

extern "C" size_t
yumiGCTestOwnerWeakRegistrationCount(void) {
    return gOwnerWeakRegistrationCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCDrainCleanups(void) {
    std::lock_guard<std::recursive_mutex> drainLock(gCleanupDrainMutex);
    size_t drained = 0;
    while (true) {
        CleanupInvocation invocation;
        {
            std::lock_guard<std::mutex> lock(gCleanupQueueMutex);
            if (gCleanupQueue.empty()) {
                break;
            }
            invocation = gCleanupQueue.front();
            gCleanupQueue.pop_front();
            gInFlightCleanups.push_back(invocation);
        }
        // The language wrapper contains recoverable exceptions and reports them before
        // returning through the C callback ABI. Continue draining regardless of its token.
        gInvokingCleanup = true;
        invocation.callback(invocation.context);
        gInvokingCleanup = false;
        if (gCleanupFileDescriptor >= 0) {
            close(gCleanupFileDescriptor);
            gCleanupFileDescriptor = -1;
        }
        if (invocation.contextRoot) {
            yumiGCRemoveRoot(invocation.contextRoot);
        }
        gCleanupPendingCount.fetch_sub(1, std::memory_order_relaxed);
        gCleanupFireCount.fetch_add(1, std::memory_order_relaxed);
        ++drained;
        {
            std::lock_guard<std::mutex> lock(gCleanupQueueMutex);
            gInFlightCleanups.pop_back();
        }
    }
    return drained;
}

extern "C" void
yumiGCQueueFileDescriptorCleanup(int fd) {
    if (gInvokingCleanup && fd >= 0 && gCleanupFileDescriptor < 0) {
        gCleanupFileDescriptor = fd;
    }
}

extern "C" void
yumiGCReportCleanupException(void) {
    gCleanupExceptionCount.fetch_add(1, std::memory_order_relaxed);
    std::fprintf(stderr, "Yumi GC cleanup callback raised an exception\n");
}

extern "C" char*
yumiGCObjectSidePayload(char* object, size_t lane) {
    if (!validObjectSidePayloadLane(lane)) {
        return nullptr;
    }
    size_t objectIndex = 0;
    GCSpan* span = objectSidePayloadSpan(object, objectIndex);
    return span ? objectSidePayloadUnchecked(span, objectIndex, lane) : nullptr;
}

static char*
yumiGCPublishObjectSidePayloadInternal(
    char* object,
    size_t lane,
    char* payload,
    YumiGCCleanupCallback cleanupCallback
) {
    ensureInitialized();
    if (!validObjectSidePayloadLane(lane) || !object || !payload) {
        return nullptr;
    }

    char* published = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
        size_t objectIndex = 0;
        GCSpan* span = objectSidePayloadSpan(object, objectIndex);
        ObjectSideMetadataTable* table = ensureObjectSideMetadataTable(span);
        if (!table) {
            return nullptr;
        }
        std::atomic<char*>* payloads = table->ensureLane(lane);
        if (!payloads) {
            return nullptr;
        }
        char* existing = payloads[objectIndex].load(std::memory_order_acquire);
        if (existing) {
            return !cleanupCallback || table->cleanupOccupied(lane, objectIndex)
                ? existing
                : nullptr;
        }
        if (cleanupCallback) {
            if (!installObjectSidePayloadCleanupCallback(lane, cleanupCallback) ||
                    !table->ensureCleanupOccupancy(lane)) {
                return nullptr;
            }
            size_t validationStart = static_cast<size_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            size_t visited = 0;
            size_t edges = 0;
            bool retainsOwner = managedGraphContains(
                payload,
                object,
                &visited,
                &edges);
            size_t validationEnd = static_cast<size_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            gCleanupSidePublishCount.fetch_add(1, std::memory_order_relaxed);
            gCleanupSideValidationNanos.fetch_add(
                validationEnd - validationStart,
                std::memory_order_relaxed);
            gCleanupSideValidationObjectCount.fetch_add(visited, std::memory_order_relaxed);
            gCleanupSideValidationEdgeCount.fetch_add(edges, std::memory_order_relaxed);
            if (retainsOwner) {
                return nullptr;
            }
        }
        // Strong side lanes are GC-visible pointer stores. Record the incoming value before
        // release publication; the recursive collector lock preserves that ordering.
        yumiGCWriteBarrier(nullptr, payload);
        payloads[objectIndex].store(payload, std::memory_order_release);
        if (cleanupCallback) {
            table->setCleanupOccupied(lane, objectIndex);
            span->addCleanupSidePayload();
            recordObjectSideCleanupPayloadPublish();
        }
        recordObjectSidePayloadPublish(lane);
        published = payload;
    }
    return published;
}

extern "C" char*
yumiGCPublishObjectSidePayload(char* object, size_t lane, char* payload) {
    return yumiGCPublishObjectSidePayloadInternal(object, lane, payload, nullptr);
}

extern "C" char*
yumiGCPublishCleanupObjectSidePayload(
    char* object,
    size_t lane,
    char* payload,
    YumiGCCleanupCallback callback
) {
    return yumiGCPublishObjectSidePayloadInternal(object, lane, payload, callback);
}

static bool
clearObjectSidePayload(char* object, size_t lane, bool cleanupOnly) {
    if (!validObjectSidePayloadLane(lane) || !object) {
        return false;
    }
    std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
    size_t objectIndex = 0;
    GCSpan* span = objectSidePayloadSpan(object, objectIndex);
    if (!span) {
        return false;
    }
    ObjectSideMetadataTable* table = span->objectSideMetadata.load(std::memory_order_acquire);
    if (!table || objectIndex >= table->capacity) {
        return false;
    }
    std::atomic<char*>* payloads = table->lane(lane);
    if (!payloads) {
        return false;
    }
    char* existing = payloads[objectIndex].load(std::memory_order_acquire);
    bool cleanup = table->cleanupOccupied(lane, objectIndex);
    if (cleanupOnly && !cleanup) {
        return false;
    }
    // Record the deletion half before clearing the strong lane. A task may have loaded this
    // payload after its setup stack scan, so the owner's later scan is not sufficient.
    yumiGCWriteBarrier(nullptr, existing);
    if (existing) {
        existing = payloads[objectIndex].exchange(nullptr, std::memory_order_acq_rel);
    }
    if (existing) {
        if (cleanup) {
            if (!table->clearCleanupOccupied(lane, objectIndex)) {
                abortGC("GC cleanup side-payload occupancy disappeared during clear");
            }
            span->removeCleanupSidePayload();
            recordObjectSideCleanupPayloadClear();
        }
        recordObjectSidePayloadClear(lane);
    }
    return existing != nullptr;
}

extern "C" void
yumiGCClearObjectSidePayload(char* object, size_t lane) {
    (void)clearObjectSidePayload(object, lane, false);
}

extern "C" bool
yumiGCClearCleanupObjectSidePayload(char* object, size_t lane) {
    return clearObjectSidePayload(object, lane, true);
}

extern "C" bool
yumiGCHasObjectSidePayload(char* object, size_t lane) {
    return yumiGCObjectSidePayload(object, lane) != nullptr;
}

extern "C" size_t
yumiGCObjectSidePayloadCount(size_t lane) {
    return validObjectSidePayloadLane(lane) ? objectSidePayloadCount(lane) : 0;
}

extern "C" size_t
yumiGCCollectionObserversSidePayloadLane(void) {
    return YumiObjectSidePayloadCollectionObservers;
}

extern "C" size_t
yumiGCMarkupAttachmentsSidePayloadLane(void) {
    return YumiObjectSidePayloadMarkupAttachments;
}

extern "C" size_t
yumiGCStyleAttachmentsSidePayloadLane(void) {
    return YumiObjectSidePayloadStyleAttachments;
}

extern "C" size_t
yumiGCStyleTemplatesSidePayloadLane(void) {
    return YumiObjectSidePayloadStyleTemplates;
}

extern "C" size_t
yumiGCStyleObjectsSidePayloadLane(void) {
    return YumiObjectSidePayloadStyleObjects;
}

extern "C" size_t
yumiGCEventsSidePayloadLane(void) {
    return YumiObjectSidePayloadEvents;
}

static void
deactivateCollectionObserver(char* context) {
    if (!YumiCollectionObserverFilter::release(
            *reinterpret_cast<std::atomic<uintptr_t>*>(context))) {
        abortGC("GC collection observer filter underflow");
    }
    size_t count = gCollectionObserverActiveCount.load(std::memory_order_relaxed);
    while (count) {
        if (gCollectionObserverActiveCount.compare_exchange_weak(
                count,
                count - 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return;
        }
    }
    abortGC("GC collection observer count underflow");
}

static size_t
reclaimCollectionObserver(void* context) {
    deactivateCollectionObserver(static_cast<char*>(context));
    return 1;
}

extern "C" size_t
yumiCollectionObserverActivate(char* owner) {
    std::atomic<uintptr_t>& slot = gCollectionObserverFilter.slot(owner);
    if (!YumiCollectionObserverFilter::retain(slot, owner)) {
        abortGC("GC collection observer filter overflow");
    }
    size_t cleanup = registerCleanup(
        owner,
        reinterpret_cast<char*>(&slot),
        reclaimCollectionObserver,
        false);
    if (!cleanup) {
        YumiCollectionObserverFilter::release(slot);
        return 0;
    }
    gCollectionObserverActiveCount.fetch_add(1, std::memory_order_relaxed);
    return cleanup;
}

extern "C" bool
yumiCollectionObserverDeactivate(size_t cleanup) {
    char* context = nullptr;
    if (!cancelCleanup(cleanup, &context)) {
        return false;
    }
    deactivateCollectionObserver(context);
    return true;
}

extern "C" bool
yumiCollectionMayBeObserved(char* collection) {
    return gCollectionObserverFilter.mayBeObserved(collection);
}

extern "C" size_t
yumiCollectionObserverActiveCount(void) {
    return gCollectionObserverActiveCount.load(std::memory_order_relaxed);
}

extern "C" void
yumiGCDebugDescribePointer(char* pointer) {
    GCSpan* span = GCHeap::pointerToSpan(pointer);
    char* object = span ? span->objectBase(pointer) : nullptr;
    std::fprintf(
        stderr,
        "Yumi GC pointer: pointer=%p span=%p object=%p owns=%u descriptor=%p objectSize=%zu "
        "marked=%u sweepGen=%u publicSweepGen=%zu phase=%u cycle=%u\n",
        pointer,
        span,
        object,
        span && span->ownsObject(pointer) ? 1u : 0u,
        object ? span->descriptor(object) : nullptr,
        object ? span->objectSize(object) : 0,
        object && span->isMarked(object) ? 1u : 0u,
        span ? span->sweepGeneration() : 0,
        yumiGCPublicSweepGeneration(),
        static_cast<unsigned>(gPhase),
        gCycle);
    if (object) {
        size_t wordCount = std::min<size_t>(span->objectSize(object) / sizeof(uintptr_t), 8);
        std::fprintf(stderr, "Yumi GC pointer words:");
        uintptr_t* words = reinterpret_cast<uintptr_t*>(object);
        for (size_t index = 0; index < wordCount; ++index) {
            std::fprintf(stderr, " 0x%llx", static_cast<unsigned long long>(words[index]));
        }
        std::fprintf(stderr, "\n");
    }
}

extern "C" bool
yumiGCTestDeepDescriptorTrace(size_t depth) {
    if (depth == 0 || depth > (1u << 20)) {
        return false;
    }
    GCDescriptor* descriptors = new (std::nothrow) GCDescriptor[depth];
    if (!descriptors) {
        return false;
    }
    // Descriptor validation is process-lifetime, just like compiler-emitted descriptor images.
    // Keep this test image alive so cached validation can never alias recycled memory.
    static GCVecState* emptyState = nullptr;
    GCVec empty = {&emptyState};
    for (size_t index = depth; index-- > 0;) {
        descriptors[index] = {
            0,
            empty,
            empty,
            1,
            empty,
            index + 1 < depth ? &descriptors[index + 1] : nullptr,
            empty,
            true,
            false
        };
    }
    char object = 0;
    if (!validateDescriptorForUse(&descriptors[0], sizeof(object))) {
        return false;
    }
    traceDescriptor(&object, &descriptors[0], sizeof(object));
    recordDescriptor(&object, &descriptors[0], sizeof(object));
    CheckmarkState checkmarkState;
    checkmarkTraceDescriptor(
        checkmarkState,
        &object,
        &descriptors[0],
        sizeof(object),
        nullptr);
    if (verifyMarkedGraphDescriptor(
            &object,
            &descriptors[0],
            sizeof(object)) != 0) {
        return false;
    }
    return true;
}

extern "C" size_t
yumiGCFlushWriteBarrierEntries(char* const* entries, size_t count) {
    ensureInitialized();
    if (!entries || count == 0) {
        return 0;
    }

    size_t shades = 0;
    {
        GCMarkParticipantScope participant;
        GCMarkPublisherScope publisher;
        if (publisher.admitted) {
            unsigned char phase = publicCollectorPhase();
            if (phase >= kPhaseMarkSetup && phase <= kPhaseMarkComplete) {
                for (size_t index = 0; index < count; ++index) {
                    if (pushMarkAdmitted(entries[index])) {
                        ++shades;
                    }
                }
                return shades;
            }
        }
    }

    // A full buffer may race the existing MarkComplete -> Sweep admission close. The common
    // external-publisher path preserves that linearization and the verification-only Sweep
    // diagnostic;
    // this is a cold boundary path, not the ordinary scalar-store fast path.
    for (size_t index = 0; index < count; ++index) {
        char* pointer = entries[index];
        ExternalMarkResult result = pushExternalMark(pointer);
        if (result == ExternalMarkResult::Published) {
            ++shades;
        }
        if (sweepTransitionValidates(result)) {
            std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
            validateSweepPublicationPointer(pointer);
        }
        finishExternalMark(result);
    }
    return shades;
}

extern "C" void
yumiGCWriteBarrier(char* source, char* value) {
    ensureInitialized();
    yumiGCRuntimeRecordBarrier(0, 0);
    char* current = source ? *reinterpret_cast<char**>(source) : nullptr;
    unsigned char initialPhase = publicCollectorPhase();
    ExternalMarkResult currentResult = source
        ? pushExternalMark(current)
        : ExternalMarkResult::Inactive;
    ExternalMarkResult valueResult = pushExternalMark(value);
    bool transition = isExternalMarkTransition(currentResult) ||
        isExternalMarkTransition(valueResult);
    if (transition) {
        // A valid managed store cannot introduce an old white object after termination.
        // Validation builds diagnose that invariant; production performs no Sweep tracing.
        std::unique_lock<std::recursive_mutex> lock(gCollectorMutex, std::defer_lock);
        if (sweepPublicationValidationEnabled()) {
            lock.lock();
            validateSweepPublicationPointer(current);
            validateSweepPublicationPointer(value);
        }
        finishExternalMark(currentResult);
        finishExternalMark(valueResult);
        return;
    }
    if (!phaseNeedsMarkBarrier(initialPhase) &&
            currentResult == ExternalMarkResult::Inactive &&
            valueResult == ExternalMarkResult::Inactive) {
        return;
    }

    yumiGCRuntimeRecordBarrierSlow();
    size_t shadeCount = 0;
    if (currentResult == ExternalMarkResult::Published) {
        ++shadeCount;
    }
    if (valueResult == ExternalMarkResult::Published) {
        ++shadeCount;
    }
    for (size_t index = 0; index < shadeCount; ++index) {
        yumiGCRuntimeRecordBarrierShade();
    }
    finishExternalMark(currentResult);
    finishExternalMark(valueResult);
}

extern "C" void
yumiGCBulkWriteBarrier(char* source, char* value, size_t byteSize, void* descriptor) {
    ensureInitialized();
    if ((!source && !value) || byteSize == 0 || !descriptor) {
        return;
    }
    if (!validateDescriptorForUse(descriptor, byteSize)) {
        return;
    }

    yumiGCRuntimeRecordBarrier(0, 0);
    unsigned char phase = publicCollectorPhase();
    if (phase == kPhaseSweep) {
        if (sweepPublicationValidationEnabled()) {
            std::lock_guard<std::recursive_mutex> lock(gCollectorMutex);
            if (gPhase == kPhaseSweep) {
                if (source) {
                    validateSweepPublicationDescriptor(source, descriptor, byteSize);
                }
                if (value) {
                    validateSweepPublicationDescriptor(value, descriptor, byteSize);
                }
            }
        }
        return;
    }

    if (phaseNeedsMarkBarrier(phase)) {
        yumiGCRuntimeRecordBarrierSlow();
        if (source) {
            recordDescriptor(source, descriptor, byteSize);
        }
        if (value) {
            recordDescriptor(value, descriptor, byteSize);
        }
    }
}

extern "C" void
yumiGCSetHeapGoal(size_t goal) {
    if (yumiGCCollectionDisabled()) {
        yumiGCRuntimeSetHeapGoal(std::numeric_limits<size_t>::max());
        return;
    }
    ensureInitialized();
    yumiGCRuntimeSetHeapGoal(goal);
}

#define YU_GC_RUNTIME_COUNTER(name, runtimeName) \
    extern "C" size_t name(void) { return runtimeName(); }

YU_GC_RUNTIME_COUNTER(yumiGCBarrierCount, yumiGCRuntimeBarrierCount)
YU_GC_RUNTIME_COUNTER(yumiGCAssistCount, yumiGCRuntimeAssistCount)
YU_GC_RUNTIME_COUNTER(yumiGCAssistTimeNanos, yumiGCRuntimeAssistTimeNanos)
YU_GC_RUNTIME_COUNTER(yumiGCAllocatedObjectCount, yumiGCRuntimeAllocatedObjectCount)
YU_GC_RUNTIME_COUNTER(yumiGCRequestedByteCount, yumiGCRuntimeRequestedByteCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocatedByteCount, yumiGCRuntimeAllocatedByteCount)
YU_GC_RUNTIME_COUNTER(yumiGCLiveByteCount, yumiGCRuntimeLiveByteCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocatedSpanCount, yumiGCRuntimeAllocatedSpanCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocatedArenaCount, yumiGCRuntimeAllocatedArenaCount)
YU_GC_RUNTIME_COUNTER(yumiGCHeapGoal, yumiGCRuntimeHeapGoal)
YU_GC_RUNTIME_COUNTER(yumiGCPacerGrowthPercent, yumiGCRuntimePacerGrowthPercent)
YU_GC_RUNTIME_COUNTER(yumiGCBackgroundWorkerCount, yumiGCRuntimeBackgroundWorkerCount)
YU_GC_RUNTIME_COUNTER(yumiGCBackgroundWorkerThreadCapacity, yumiGCRuntimeBackgroundWorkerThreadCapacity)
YU_GC_RUNTIME_COUNTER(yumiGCBackgroundWorkerDutyPermille, yumiGCRuntimeBackgroundWorkerDutyPermille)
YU_GC_RUNTIME_COUNTER(yumiGCBackgroundWorkerWork, yumiGCRuntimeBackgroundWorkerWork)
YU_GC_RUNTIME_COUNTER(yumiGCBackgroundCredit, yumiGCRuntimeBackgroundCredit)
YU_GC_RUNTIME_COUNTER(yumiGCFrameBudgetOfferCount, yumiGCRuntimeFrameBudgetOfferCount)
YU_GC_RUNTIME_COUNTER(yumiGCFrameBudgetWorkCount, yumiGCRuntimeFrameBudgetWorkCount)
YU_GC_RUNTIME_COUNTER(yumiGCFrameBudgetWorkNanos, yumiGCRuntimeFrameBudgetWorkNanos)
YU_GC_RUNTIME_COUNTER(yumiGCFrameBudgetOverrunCount, yumiGCRuntimeFrameBudgetOverrunCount)
YU_GC_RUNTIME_COUNTER(yumiGCPacerHardCap, yumiGCRuntimePacerHardCap)
YU_GC_RUNTIME_COUNTER(yumiGCPacerSoftGoal, yumiGCRuntimePacerSoftGoal)
YU_GC_RUNTIME_COUNTER(yumiGCPacerHardGoal, yumiGCRuntimePacerHardGoal)
YU_GC_RUNTIME_COUNTER(yumiGCPacerLastTriggerReason, yumiGCRuntimePacerLastTriggerReason)
YU_GC_RUNTIME_COUNTER(yumiGCCollectionCount, yumiGCRuntimeCollectionCount)
YU_GC_RUNTIME_COUNTER(yumiGCMarkTimeNanos, yumiGCRuntimeMarkTimeNanos)
YU_GC_RUNTIME_COUNTER(yumiGCSweepTimeNanos, yumiGCRuntimeSweepTimeNanos)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationStallCount, yumiGCRuntimeAllocationStallCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationStallTimeNanos, yumiGCRuntimeAllocationStallTimeNanos)
YU_GC_RUNTIME_COUNTER(yumiGCHandshakeCount, yumiGCRuntimeHandshakeCount)
YU_GC_RUNTIME_COUNTER(yumiGCHandshakeLatencyNanos, yumiGCRuntimeHandshakeLatencyNanos)
YU_GC_RUNTIME_COUNTER(yumiGCCurrentCycleId, yumiGCRuntimeCurrentCycleId)
YU_GC_RUNTIME_COUNTER(yumiGCActiveCycleAgeNanos, yumiGCRuntimeActiveCycleAgeNanos)
YU_GC_RUNTIME_COUNTER(yumiGCMarkQueueDepth, yumiGCRuntimeMarkQueueDepth)
YU_GC_RUNTIME_COUNTER(yumiGCAssistDebt, yumiGCRuntimeAssistDebt)
YU_GC_RUNTIME_COUNTER(yumiGCMarkQueuePushCount, yumiGCRuntimeMarkQueuePushCount)
YU_GC_RUNTIME_COUNTER(yumiGCMarkQueuePopCount, yumiGCRuntimeMarkQueuePopCount)
YU_GC_RUNTIME_COUNTER(yumiGCMarkQueuePopBatchCount, yumiGCRuntimeMarkQueuePopBatchCount)
YU_GC_RUNTIME_COUNTER(yumiGCMarkQueueMaxPopBatch, yumiGCRuntimeMarkQueueMaxPopBatch)
YU_GC_RUNTIME_COUNTER(yumiGCBatchReservedBytes, yumiGCRuntimeBatchReservedBytes)
YU_GC_RUNTIME_COUNTER(yumiGCBatchConsumedBytes, yumiGCRuntimeBatchConsumedBytes)
YU_GC_RUNTIME_COUNTER(yumiGCBatchReleasedBytes, yumiGCRuntimeBatchReleasedBytes)
YU_GC_RUNTIME_COUNTER(yumiGCBatchDiscardedBytes, yumiGCRuntimeBatchDiscardedBytes)
YU_GC_RUNTIME_COUNTER(yumiGCBatchCacheBytes, yumiGCRuntimeBatchCacheBytes)
YU_GC_RUNTIME_COUNTER(yumiGCBatchUnusedBytes, yumiGCRuntimeBatchUnusedBytes)
YU_GC_RUNTIME_COUNTER(yumiGCThreadBatchCacheBytes, yumiGCRuntimeThreadBatchCacheBytes)
YU_GC_RUNTIME_COUNTER(yumiGCThreadPendingAllocatedObjectCount, yumiGCRuntimeThreadPendingAllocatedObjectCount)
YU_GC_RUNTIME_COUNTER(yumiGCThreadPendingBatchConsumedBytes, yumiGCRuntimeThreadPendingBatchConsumedBytes)
YU_GC_RUNTIME_COUNTER(yumiGCStaleBatchDropCount, yumiGCRuntimeStaleBatchDropCount)
YU_GC_RUNTIME_COUNTER(yumiGCFallbackAllocationCount, yumiGCRuntimeFallbackAllocationCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationFastHitCount, yumiGCRuntimeAllocationFastHitCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationFastRefillCount, yumiGCRuntimeAllocationFastRefillCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationFastFallbackCount, yumiGCRuntimeAllocationFastFallbackCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationLargeCount, yumiGCRuntimeAllocationLargeCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationZeroedBytes, yumiGCRuntimeAllocationZeroedBytes)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationStatPublicationCount, yumiGCRuntimeAllocationStatPublicationCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationAssistCheckCount, yumiGCRuntimeAllocationAssistCheckCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationAssistCallCount, yumiGCRuntimeAllocationAssistCallCount)
YU_GC_RUNTIME_COUNTER(yumiGCFastBarrierCallCount, yumiGCRuntimeFastBarrierCallCount)
YU_GC_RUNTIME_COUNTER(yumiGCFastBarrierSlowEntryCount, yumiGCRuntimeFastBarrierSlowEntryCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationSiteHitCount, yumiGCRuntimeAllocationSiteHitCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationSiteMissCount, yumiGCRuntimeAllocationSiteMissCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationSiteRefillCount, yumiGCRuntimeAllocationSiteRefillCount)
YU_GC_RUNTIME_COUNTER(yumiGCAllocationSiteFallbackCount, yumiGCRuntimeAllocationSiteFallbackCount)
YU_GC_RUNTIME_COUNTER(yumiGCAutomaticFallbackCount, yumiGCRuntimeAutomaticFallbackCount)
YU_GC_RUNTIME_COUNTER(yumiGCAutomaticFallbackReason, yumiGCRuntimeAutomaticFallbackReason)
YU_GC_RUNTIME_COUNTER(yumiGCAutomaticDrainGiveUpCount, yumiGCRuntimeAutomaticDrainGiveUpCount)
#undef YU_GC_RUNTIME_COUNTER

extern "C" size_t
yumiGCMarkParticipantCount(void) {
    return gMarkWork.participantCount();
}

extern "C" size_t
yumiGCActiveMarkerCount(void) {
    return gMarkWork.activeMarkerCount();
}

extern "C" uint64_t
yumiGCWorkFlushedEpoch(void) {
    return gMarkWork.workFlushedEpoch();
}

extern "C" size_t
yumiGCTerminationConfirmCount(void) {
    return gTerminationConfirmCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCTerminationConfirmResumeCount(void) {
    return gTerminationConfirmResumeCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCWeakClearCount(void) {
    return gWeakClearCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupRegistrationCount(void) {
    return gCleanupRegistrationCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupPendingCount(void) {
    return gCleanupPendingCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupEnqueueCount(void) {
    return gCleanupEnqueueCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupFireCount(void) {
    return gCleanupFireCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupCancelCount(void) {
    return gCleanupCancelCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupExceptionCount(void) {
    return gCleanupExceptionCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupSidePublishCount(void) {
    return gCleanupSidePublishCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupSideValidationNanos(void) {
    return gCleanupSideValidationNanos.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupSideValidationObjectCount(void) {
    return gCleanupSideValidationObjectCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCCleanupSideValidationEdgeCount(void) {
    return gCleanupSideValidationEdgeCount.load(std::memory_order_relaxed);
}

extern "C" size_t
yumiGCAllocationCallCount(size_t kind) {
    return yumiGCRuntimeAllocationCallCount(kind);
}

extern "C" size_t
yumiGCAllocationPhaseCount(size_t phase) {
    return yumiGCRuntimeAllocationPhaseCount(phase);
}

extern "C" size_t
yumiGCAllocationBatchMissCount(size_t reason) {
    return yumiGCRuntimeAllocationBatchMissCount(reason);
}

extern "C" size_t
yumiGCAllocationDescriptorMixCount(size_t index, size_t described) {
    return yumiGCRuntimeAllocationDescriptorMixCount(index, described);
}

extern "C" size_t
yumiGCRssBytes(void) {
#if defined(__APPLE__)
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(
            mach_task_self(),
            TASK_VM_INFO,
            reinterpret_cast<task_info_t>(&info),
            &count) == KERN_SUCCESS) {
        return static_cast<size_t>(info.phys_footprint);
    }
#else
    if (FILE* statm = std::fopen("/proc/self/statm", "r")) {
        unsigned long long total = 0;
        unsigned long long resident = 0;
        int scanned = std::fscanf(statm, "%llu %llu", &total, &resident);
        std::fclose(statm);
        if (scanned == 2) {
            return static_cast<size_t>(resident) * static_cast<size_t>(sysconf(_SC_PAGESIZE));
        }
    }
#endif
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
#if defined(__APPLE__)
        return static_cast<size_t>(usage.ru_maxrss);
#else
        return static_cast<size_t>(usage.ru_maxrss) * 1024;
#endif
    }
    return 0;
}

extern "C" size_t
yumiGCCommittedBytes(void) {
    return heap().committedPageCount() * kPageSize;
}

extern "C" size_t
yumiGCCommittedPageCount(void) {
    return heap().committedPageCount();
}

extern "C" size_t
yumiGCDecommittedPageCount(void) {
    return heap().decommittedPageCount();
}

extern "C" size_t
yumiGCDirtyPageCount(void) {
    return heap().dirtyPageCount();
}

extern "C" size_t
yumiGCFreePageCount(void) {
    return heap().freePageCount();
}

extern "C" size_t
yumiGCReservedArenaBytes(void) {
    return heap().reservedArenaBytes();
}

extern "C" size_t
yumiGCLargeSpanFragmentationBytes(void) {
    return heap().largeSpanFragmentationBytes();
}

extern "C" size_t
yumiGCFragmentationBytes(void) {
    return heap().fragmentationBytes();
}

extern "C" size_t
yumiGCSizeClassOccupancyBucketCount(size_t index, size_t bucket) {
    return heap().sizeClassOccupancyBucketCount(index, bucket);
}

extern "C" size_t
yumiGCIncrementalSweepDebtPages(void) {
    size_t debt = 0;
    for (size_t index = 0; index < kSizeClassesCount; ++index) {
        debt += heap().sizeClassSweepDebt(index);
    }
    return debt + heap().largeSpanSweepDebt();
}

extern "C" bool
yumiGCIncrementalSweepHasDebt(void) {
    return heap().hasSweepDebt();
}

extern "C" size_t
yumiGCScavengeDebt(void) {
    return heap().scavengeDebtPageCount(true);
}

extern "C" size_t
yumiGCSizeClassSweepDebt(size_t index) {
    return heap().sizeClassSweepDebt(index);
}

extern "C" size_t
yumiGCLargeSpanSweepDebt(void) {
    return heap().largeSpanSweepDebt();
}

#define YU_GC_NAMED_OBSERVATION_COUNTER(name, runtimeName) \
    extern "C" size_t name(void) { return runtimeName(); }

YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCMarkSetupSampleCount, yumiGCRuntimeMarkSetupSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCMarkSetupP50Nanos, yumiGCRuntimeMarkSetupP50Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCMarkSetupMaxNanos, yumiGCRuntimeMarkSetupMaxNanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCRootScanSampleCount, yumiGCRuntimeRootScanSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCRootScanP95Nanos, yumiGCRuntimeRootScanP95Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCRootScanP99Nanos, yumiGCRuntimeRootScanP99Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCRootScanMaxNanos, yumiGCRuntimeRootScanMaxNanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCMarkWorkSampleCount, yumiGCRuntimeMarkWorkSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCMarkWorkP95Nanos, yumiGCRuntimeMarkWorkP95Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCMarkWorkP99Nanos, yumiGCRuntimeMarkWorkP99Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCMarkWorkMaxNanos, yumiGCRuntimeMarkWorkMaxNanos)
YU_GC_NAMED_OBSERVATION_COUNTER(
    yumiGCMarkTerminationSampleCount,
    yumiGCRuntimeMarkTerminationSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(
    yumiGCMarkTerminationP99Nanos,
    yumiGCRuntimeMarkTerminationP99Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(
    yumiGCMarkTerminationMaxNanos,
    yumiGCRuntimeMarkTerminationMaxNanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCSweepSampleCount, yumiGCRuntimeSweepSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCSweepP99Nanos, yumiGCRuntimeSweepP99Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCSweepMaxNanos, yumiGCRuntimeSweepMaxNanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCScavengeSampleCount, yumiGCRuntimeScavengeSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCScavengeP99Nanos, yumiGCRuntimeScavengeP99Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCScavengeMaxNanos, yumiGCRuntimeScavengeMaxNanos)
YU_GC_NAMED_OBSERVATION_COUNTER(
    yumiGCAllocationStallSampleCount,
    yumiGCRuntimeAllocationStallSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(
    yumiGCFallbackAllocationSampleCount,
    yumiGCRuntimeFallbackAllocationSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCHandshakeSampleCount, yumiGCRuntimeHandshakeSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCHandshakeP99Nanos, yumiGCRuntimeHandshakeP99Nanos)
YU_GC_NAMED_OBSERVATION_COUNTER(yumiGCHandshakeMaxNanos, yumiGCRuntimeHandshakeMaxNanos)
YU_GC_NAMED_OBSERVATION_COUNTER(
    yumiGCStackMapLookupSampleCount,
    yumiGCRuntimeStackMapLookupSampleCount)
YU_GC_NAMED_OBSERVATION_COUNTER(
    yumiGCStackMapLookupP99Nanos,
    yumiGCRuntimeStackMapLookupP99Nanos)

#undef YU_GC_NAMED_OBSERVATION_COUNTER

extern "C" size_t
yumiGCPauseCount(void) {
    return yumiGCRuntimePauseCount();
}

extern "C" size_t
yumiGCPauseEventCount(void) {
    return yumiGCRuntimePauseEventCount();
}

extern "C" size_t
yumiGCPauseEventCycleId(size_t index) {
    return yumiGCRuntimePauseEventCycleId(index);
}

extern "C" size_t
yumiGCPauseEventKind(size_t index) {
    return yumiGCRuntimePauseEventKind(index);
}

extern "C" size_t
yumiGCPauseEventStartNanos(size_t index) {
    return yumiGCRuntimePauseEventStartNanos(index);
}

extern "C" size_t
yumiGCPauseEventDurationNanos(size_t index) {
    return yumiGCRuntimePauseEventDurationNanos(index);
}

extern "C" size_t
yumiGCPauseEventTasksWaited(size_t index) {
    return yumiGCRuntimePauseEventTasksWaited(index);
}

extern "C" size_t
yumiGCPauseEventSlowestTaskNanos(size_t index) {
    return yumiGCRuntimePauseEventSlowestTaskNanos(index);
}

extern "C" size_t
yumiGCPauseP50Nanos(size_t kind) {
    return yumiGCRuntimePauseP50Nanos(kind);
}

extern "C" size_t
yumiGCPauseP95Nanos(size_t kind) {
    return yumiGCRuntimePauseP95Nanos(kind);
}

extern "C" size_t
yumiGCPauseP99Nanos(size_t kind) {
    return yumiGCRuntimePauseP99Nanos(kind);
}

extern "C" size_t
yumiGCPauseMaxNanos(size_t kind) {
    return yumiGCRuntimePauseMaxNanos(kind);
}

extern "C" size_t
yumiGCCycleSummaryCount(void) {
    return yumiGCRuntimeCycleSummaryCount();
}

#define YU_GC_CYCLE_SUMMARY_FORWARDER(name, runtimeName) \
    extern "C" size_t name(size_t index) { return runtimeName(index); }

YU_GC_CYCLE_SUMMARY_FORWARDER(yumiGCCycleSummaryCycleId, yumiGCRuntimeCycleSummaryCycleId)
YU_GC_CYCLE_SUMMARY_FORWARDER(
    yumiGCCycleSummaryTriggerReason,
    yumiGCRuntimeCycleSummaryTriggerReason)
YU_GC_CYCLE_SUMMARY_FORWARDER(
    yumiGCCycleSummaryHeapAtTrigger,
    yumiGCRuntimeCycleSummaryHeapAtTrigger)
YU_GC_CYCLE_SUMMARY_FORWARDER(yumiGCCycleSummaryHeapGoal, yumiGCRuntimeCycleSummaryHeapGoal)
YU_GC_CYCLE_SUMMARY_FORWARDER(yumiGCCycleSummaryHeapActual, yumiGCRuntimeCycleSummaryHeapActual)
YU_GC_CYCLE_SUMMARY_FORWARDER(
    yumiGCCycleSummaryMarkDurationNanos,
    yumiGCRuntimeCycleSummaryMarkDurationNanos)
YU_GC_CYCLE_SUMMARY_FORWARDER(yumiGCCycleSummaryScanWork, yumiGCRuntimeCycleSummaryScanWork)
YU_GC_CYCLE_SUMMARY_FORWARDER(yumiGCCycleSummaryAssistWork, yumiGCRuntimeCycleSummaryAssistWork)
YU_GC_CYCLE_SUMMARY_FORWARDER(
    yumiGCCycleSummaryBackgroundWork,
    yumiGCRuntimeCycleSummaryBackgroundWork)
YU_GC_CYCLE_SUMMARY_FORWARDER(yumiGCCycleSummarySweepPages, yumiGCRuntimeCycleSummarySweepPages)
YU_GC_CYCLE_SUMMARY_FORWARDER(
    yumiGCCycleSummaryScavengedPages,
    yumiGCRuntimeCycleSummaryScavengedPages)

#undef YU_GC_CYCLE_SUMMARY_FORWARDER

extern "C" void*
yumiGCTestSinglePointerDescriptorPointer(void) {
    return &gSinglePointerDescriptor;
}

extern "C" void*
yumiGCTestAlternateSinglePointerDescriptorPointer(void) {
    return &gAlternateSinglePointerDescriptor;
}

extern "C" void
yumiGCTestSetCheckmarkExtraRoot(void* pointer) {
    gCheckmarkTestExtraRoot.store(static_cast<char*>(pointer), std::memory_order_release);
}

extern "C" void*
yumiGCTestAddressIndexArena(void* pointer) {
    ArenaMeta* metadata = GCAddressIndex::instance().lookupArena(pointer);
    return metadata ? metadata->base : nullptr;
}

extern "C" void*
yumiGCTestAddressIndexSpan(void* pointer) {
    return GCAddressIndex::instance().lookupSpan(pointer);
}

extern "C" size_t
yumiGCTestAddressIndexLookupLoads(void* pointer) {
    return GCAddressIndex::instance().lookup(pointer).indexLoads;
}

extern "C" bool
yumiGCTestObjectSpanSwept(void* pointer) {
    GCSpan* span = GCHeap::pointerToSpan(static_cast<char*>(pointer));
    return span && span->sweepGeneration() == yumiGCPublicSweepGeneration();
}

extern "C" bool
yumiGCTestObjectMarked(void* pointer) {
    GCSpan* span = GCHeap::pointerToSpan(static_cast<char*>(pointer));
    return span && span->isMarked(static_cast<char*>(pointer));
}

extern "C" size_t
yumiGCTestSizeClassIndexForSize(size_t size) {
    return sizeClassForSize(size).index;
}

extern "C" size_t
yumiGCTestSizeClassSweepCursor(size_t index) {
    return index < kSizeClassesCount
        ? heap().spanStores[index]->sweepCursor.load(std::memory_order_acquire)
        : 0;
}

extern "C" size_t
yumiGCTestSizeClassSweepLimit(size_t index) {
    return index < kSizeClassesCount
        ? heap().spanStores[index]->sweepLimit.load(std::memory_order_acquire)
        : 0;
}

extern "C" size_t
yumiGCTestSizeClassSweepRegistrySize(size_t index) {
    return index < kSizeClassesCount
        ? heap().spanStores[index]->sweepSpanCount.load(std::memory_order_acquire)
        : 0;
}

extern "C" size_t
yumiGCTestLargeSweepCursor(void) {
    return heap().largeSweepCursor.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCTestLargeSweepLimit(void) {
    return heap().largeSweepLimit.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCTestLargeSweepRegistrySize(void) {
    return heap().largeSweepSpanCount.load(std::memory_order_acquire);
}

extern "C" size_t
yumiGCTestObjectSpanAllocatedCount(void* pointer) {
    GCSpan* span = GCHeap::pointerToSpan(static_cast<char*>(pointer));
    if (!span) {
        return 0;
    }
    size_t allocated = 0;
    for (size_t index = 0; index < span->objectsCapacity; ++index) {
        allocated += span->allocBits.getAcquire(index) ? 1 : 0;
    }
    return allocated;
}

extern "C" bool
yumiGCTestSweepUntilObjectSpanSwept(void* pointer) {
    ensureInitialized();
    if (publicCollectorPhase() != kPhaseSweep) {
        return false;
    }
    GCSpan* target = GCHeap::pointerToSpan(static_cast<char*>(pointer));
    if (!target) {
        return false;
    }

    // Exercise the production incremental heap/store sweep, but deliberately leave cycle
    // completion to the public driver. This gives the regression fixture a deterministic
    // post-flip, still-Sweep point at which to test verification-only publication diagnostics.
    static const size_t kMaxSweepSteps = 1u << 20;
    for (size_t step = 0; step < kMaxSweepSteps; ++step) {
        if (target->sweepGeneration() == yumiGCPublicSweepGeneration()) {
            return publicCollectorPhase() == kPhaseSweep;
        }
        heap().sweepIncrementalStep(1);
    }
    return false;
}

extern "C" bool
yumiGCTestDeferredSpanRetirement(void) {
    ensureInitialized();
    if (publicCollectorPhase() != kPhaseIdle || heap().pendingRetiredSpanCount() != 0) {
        return false;
    }

    static const size_t kProducerCount = 4;
    static const size_t kRetiredSpanCount = 16;
    SizeClass sizeClass = sizeClassForSize(kObjectAlignment);
    std::array<GCSpan*, kRetiredSpanCount> retiredSpans {};
    std::array<char*, kRetiredSpanCount> retiredPages {};
    std::array<uintptr_t, kRetiredSpanCount> retiredSpanAddresses {};
    for (size_t index = 0; index < kRetiredSpanCount; ++index) {
        GCSpan* span = heap().createSpan(sizeClass);
        if (!span || GCHeap::pointerToSpan(span->memory) != span) {
            return false;
        }
        retiredSpans[index] = span;
        retiredPages[index] = span->memory;
        retiredSpanAddresses[index] = reinterpret_cast<uintptr_t>(span);
    }
    GCSpan* held = retiredSpans[0];
    char* heldMemory = retiredPages[0];

    std::atomic<unsigned char> readerPhase {0};
    std::atomic<bool> readerValid {true};
    std::thread reader([&]() {
        GCSpan* acquired = GCHeap::pointerToSpan(heldMemory);
        if (acquired != held) {
            readerValid.store(false, std::memory_order_relaxed);
        }
        readerPhase.store(1, std::memory_order_release);
        while (readerPhase.load(std::memory_order_acquire) < 2) {
            std::this_thread::yield();
        }
        // The reader has acquired SpanMeta before logical retirement and has not crossed a
        // quiescent boundary. The record and its dependent fields must still be safe to read.
        if (acquired != held || acquired->memory != heldMemory ||
                acquired->pageCount == 0 || acquired->sizeClass.size != sizeClass.size) {
            readerValid.store(false, std::memory_order_relaxed);
        }
        readerPhase.store(3, std::memory_order_release);
    });
    while (readerPhase.load(std::memory_order_acquire) < 1) {
        std::this_thread::yield();
    }

    size_t destroyedBefore = heap().destroyedRetiredSpanCount();
    std::array<std::thread, kProducerCount> producers;
    for (size_t producer = 0; producer < kProducerCount; ++producer) {
        producers[producer] = std::thread([&, producer]() {
            for (size_t index = producer; index < kRetiredSpanCount;
                    index += kProducerCount) {
                heap().freeSpan(retiredSpans[index]);
                heap().freeSpan(retiredSpans[index]);
            }
        });
    }
    for (std::thread& producer : producers) {
        producer.join();
    }

    bool validBeforeBoundary =
        heap().pendingRetiredSpanCount() == kRetiredSpanCount &&
        heap().destroyedRetiredSpanCount() == destroyedBefore;
    for (size_t index = 0; index < kRetiredSpanCount && validBeforeBoundary; ++index) {
        GCSpan* span = retiredSpans[index];
        validBeforeBoundary = GCHeap::pointerToSpan(retiredPages[index]) == span &&
            !span->isRetired() && span->memory == retiredPages[index];
    }

    // Queued pages remain allocated and indexed until the quiescent boundary, so a new
    // metadata record cannot alias either the held SpanMeta or its arena pages yet.
    GCSpan* preBoundary = heap().createSpan(sizeClass);
    bool distinctBeforeBoundary = preBoundary != nullptr;
    if (preBoundary) {
        uintptr_t address = reinterpret_cast<uintptr_t>(preBoundary);
        for (size_t index = 0; index < kRetiredSpanCount; ++index) {
            if (address == retiredSpanAddresses[index] ||
                    preBoundary->memory == retiredPages[index]) {
                distinctBeforeBoundary = false;
            }
        }
    }

    readerPhase.store(2, std::memory_order_release);
    reader.join();

    retireMetadataWithSweepHandshake();
    bool exactMPSCDrain = heap().pendingRetiredSpanCount() == 0 &&
        heap().destroyedRetiredSpanCount() == destroyedBefore + kRetiredSpanCount;
    for (char* page : retiredPages) {
        if (GCHeap::pointerToSpan(page) != nullptr) {
            exactMPSCDrain = false;
        }
    }

    // The retired slab records are at the front of the dedicated allocator's free list and the
    // arena page cursor points into the just-freed range. Refill the same number of spans to force
    // both identities through reuse, and require each reused page to resolve only to its new owner.
    std::array<GCSpan*, kRetiredSpanCount> replacements {};
    bool sawReusedPage = false;
    bool sawReusedSpanRecord = false;
    bool replacementMappingsValid = true;
    for (size_t index = 0; index < kRetiredSpanCount; ++index) {
        GCSpan* replacement = heap().createSpan(sizeClass);
        replacements[index] = replacement;
        if (!replacement || GCHeap::pointerToSpan(replacement->memory) != replacement) {
            replacementMappingsValid = false;
            continue;
        }
        uintptr_t address = reinterpret_cast<uintptr_t>(replacement);
        for (size_t retired = 0; retired < kRetiredSpanCount; ++retired) {
            if (address == retiredSpanAddresses[retired]) {
                sawReusedSpanRecord = true;
            }
            if (replacement->memory == retiredPages[retired]) {
                sawReusedPage = true;
                if (GCHeap::pointerToSpan(retiredPages[retired]) != replacement) {
                    replacementMappingsValid = false;
                }
            }
        }
    }

    for (GCSpan* replacement : replacements) {
        heap().freeSpan(replacement);
    }
    heap().freeSpan(preBoundary);
    retireMetadataWithSweepHandshake();

    return validBeforeBoundary && distinctBeforeBoundary && exactMPSCDrain &&
        readerValid.load(std::memory_order_relaxed) && sawReusedPage &&
        sawReusedSpanRecord && replacementMappingsValid &&
        heap().pendingRetiredSpanCount() == 0;
}
