#ifndef BX_GC_TEMPORARY_ROOTS_H
#define BX_GC_TEMPORARY_ROOTS_H

#include <stddef.h>

static constexpr size_t BrixGCTemporaryRootInlineCapacity = 4;
static constexpr size_t BrixGCTemporaryRootSlotBytes = 48;

struct alignas(max_align_t) BrixGCTemporaryRootSlot {
    unsigned char bytes[BrixGCTemporaryRootSlotBytes];
};

// The scheduler owns this storage as part of the logical task runtime context. The collector keeps
// its private Root representation opaque by using untyped links and fixed-size inline slots.
struct BrixGCTemporaryRootStorage {
    void* head;
    void* freeList;
    void* slabHead;
    size_t depth;
    size_t maxDepth;
    size_t pushCount;
    size_t popCount;
    size_t storageAllocationCount;
    size_t slabGrowthCount;
    bool initialized;
    BrixGCTemporaryRootSlot inlineSlots[BrixGCTemporaryRootInlineCapacity];
};

#endif
