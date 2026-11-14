#ifndef YU_GC_ARENA_H
#define YU_GC_ARENA_H

#include "YuGCAddressIndex.h"
#include "YuGCBitmap.h"

#include <memory>

namespace YuGC {

struct GCArena {
    char* memory = nullptr;
    bool dedicated;
    size_t pageCapacity;
    size_t slotCount;
    size_t reservationSize;
    GCBitmap pageBitmap;
    GCBitmap committedBitmap;
    GCBitmap protectedBitmap;
    std::unique_ptr<ArenaMeta[]> addressMetadata;
    size_t allocatedPageCount = 0;
    size_t committedPageCount = 0;
    size_t decommittedPageCount;
    size_t nextPageSearch = 0;
    size_t emptySinceCycle = 0;
    size_t dirtyPages = 0;

    explicit GCArena(size_t dedicatedPageCount = 0);

    ~GCArena();

    GCSpan* spanForPage(size_t page) const;

    bool publishAddressIndex();

    bool unpublishAddressIndex();

    bool hasPointer(char* pointer) const;

    size_t pointerPageOffset(char* pointer) const;

    size_t markCommittedRange(char* pointer, size_t byteSize);

    char* allocPages(size_t pageCount, bool bestFit = false, bool* zeroed = nullptr);

    void freePages(char* pointer, size_t pageCount);

    size_t scavengeFreePages(size_t budget);

    size_t dirtyPageCount() const;

    size_t dirtyPagesInRange(size_t firstPage, size_t endPage) const;

    bool isDirtyUnit(size_t firstPage, size_t unitPages) const;

    bool canRelease() const;

    bool canReleaseAfterCycle(size_t cycle, size_t holdDownCycles);

    bool publishSpanPages(GCSpan* span, bool publish);
    void registerSpan(GCSpan* span);
    void unregisterSpan(GCSpan* span);
    GCSpan* findSpan(char* pointer);
};

} // namespace YuGC

#endif
