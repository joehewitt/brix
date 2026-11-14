#ifndef YU_GC_TEMPORARY_ROOTS_H
#define YU_GC_TEMPORARY_ROOTS_H

#include <stddef.h>

static constexpr size_t YumiGCTemporaryRootInlineCapacity = 4;
static constexpr size_t YumiGCTemporaryRootSlotBytes = 48;

struct alignas(max_align_t) YumiGCTemporaryRootSlot {
    unsigned char bytes[YumiGCTemporaryRootSlotBytes];
};

// The scheduler owns this storage as part of the logical task runtime context. The collector keeps
// its private Root representation opaque by using untyped links and fixed-size inline slots.
struct YumiGCTemporaryRootStorage {
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
    YumiGCTemporaryRootSlot inlineSlots[YumiGCTemporaryRootInlineCapacity];
};

#endif
