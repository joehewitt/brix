#include "BxGCLockOrder.h"

#if BX_GC_DEBUG_LOCK_ORDER

#include "BxGCCommon.h"

namespace BxGC {

namespace {

static const size_t kMaxRankedLockDepth = 32;

struct GCLockOrderFrame {
    GCLockRank rank;
    const void* lockAddress;
};

thread_local GCLockOrderFrame gLockOrderStack[kMaxRankedLockDepth];
thread_local size_t gLockOrderDepth = 0;

} // namespace

void
enterGCLockOrder(GCLockRank rank, const void* lockAddress) {
    if (!lockAddress) {
        abortGC("GC ranked lock has no address");
    }
    if (gLockOrderDepth == kMaxRankedLockDepth) {
        abortGC("GC ranked lock stack overflow");
    }
    for (size_t index = 0; index < gLockOrderDepth; ++index) {
        if (gLockOrderStack[index].lockAddress == lockAddress) {
            abortGC("GC ranked lock recursively acquired");
        }
    }
    if (gLockOrderDepth != 0 &&
            static_cast<unsigned char>(rank) <
                static_cast<unsigned char>(gLockOrderStack[gLockOrderDepth - 1].rank)) {
        abortGC("GC lock order inversion");
    }
    gLockOrderStack[gLockOrderDepth++] = GCLockOrderFrame {rank, lockAddress};
}

void
leaveGCLockOrder(GCLockRank rank, const void* lockAddress) {
    if (gLockOrderDepth == 0) {
        abortGC("GC ranked lock stack underflow");
    }
    const GCLockOrderFrame& frame = gLockOrderStack[gLockOrderDepth - 1];
    if (frame.rank != rank || frame.lockAddress != lockAddress) {
        abortGC("GC ranked lock release is not LIFO");
    }
    --gLockOrderDepth;
}

} // namespace BxGC

#endif
