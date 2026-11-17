#include "BxGCHeap.h"
#include "BxGCLockOrder.h"

namespace BxGC {

static size_t
arenaSlotCount(size_t dedicatedPageCount) {
    return dedicatedPageCount
        ? (dedicatedPageCount + kArenaAddressPageCount - 1) / kArenaAddressPageCount
        : 1;
}

GCArena::GCArena(size_t dedicatedPageCount)
    : dedicated(dedicatedPageCount != 0),
      pageCapacity(dedicatedPageCount ? dedicatedPageCount : kArenaPageCapacity),
      slotCount(arenaSlotCount(dedicatedPageCount)),
      reservationSize(0),
      pageBitmap(pageCapacity),
      committedBitmap(pageCapacity),
      protectedBitmap(pageCapacity),
      decommittedPageCount(pageCapacity) {
    if (slotCount > kAddressIndexLimit / kArenaSize - 1) {
        return;
    }
    size_t arenaSize = slotCount * kArenaSize;
    int flags = MAP_PRIVATE | MAP_ANON | MAP_NORESERVE;
    size_t mappedSize = arenaSize + kArenaSize;
    void* mapped = mmap(nullptr, mappedSize, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (mapped == MAP_FAILED) {
        std::fprintf(stderr, "Brix GC arena reservation failed; allocation will return null\n");
        return;
    }

    uintptr_t reservationStart = reinterpret_cast<uintptr_t>(mapped);
    uintptr_t reservationEnd = reservationStart + mappedSize;
    uintptr_t alignedStart = (reservationStart + kArenaSize - 1) & ~(kArenaSize - 1);
    size_t prefixSize = static_cast<size_t>(alignedStart - reservationStart);
    size_t suffixSize = static_cast<size_t>(reservationEnd - (alignedStart + arenaSize));
    if (prefixSize && munmap(reinterpret_cast<void*>(reservationStart), prefixSize) != 0) {
        munmap(mapped, mappedSize);
        std::fprintf(stderr, "Brix GC arena prefix trim failed; allocation will return null\n");
        return;
    }
    if (suffixSize &&
            munmap(reinterpret_cast<void*>(alignedStart + arenaSize), suffixSize) != 0) {
        munmap(reinterpret_cast<void*>(alignedStart), reservationEnd - alignedStart);
        std::fprintf(stderr, "Brix GC arena suffix trim failed; allocation will return null\n");
        return;
    }

    addressMetadata.reset(new (std::nothrow) ArenaMeta[slotCount]);
    if (!addressMetadata) {
        munmap(reinterpret_cast<void*>(alignedStart), arenaSize);
        std::fprintf(
            stderr,
            "Brix GC arena metadata allocation failed; allocation will return null\n");
        return;
    }
    memory = reinterpret_cast<char*>(alignedStart);
    reservationSize = arenaSize;
    for (size_t slot = 0; slot < slotCount; ++slot) {
        size_t slotFirstPage = slot * kArenaAddressPageCount;
        addressMetadata[slot].initialize(
            memory + slot * kArenaSize,
            this,
            std::min(kArenaAddressPageCount, pageCapacity - slotFirstPage));
    }
    // Shared arenas keep the historical 2 MiB tail reserved so arena/data-page geometry remains
    // stable during Stage 1. Slot-indexed span mark bits are now the only marking storage.
}

GCArena::~GCArena() {
    if (memory) {
        munmap(memory, reservationSize);
    }
}

GCSpan*
GCArena::spanForPage(size_t page) const {
    size_t slot = page / kArenaAddressPageCount;
    return slot < slotCount
        ? addressMetadata[slot].spanForPage(page % kArenaAddressPageCount)
        : nullptr;
}

bool
GCArena::publishAddressIndex() {
    if (!memory) {
        return false;
    }
    for (size_t slot = 0; slot < slotCount; ++slot) {
        if (!GCAddressIndex::instance().publishArena(&addressMetadata[slot])) {
            while (slot != 0) {
                --slot;
                GCAddressIndex::instance().unpublishArena(&addressMetadata[slot]);
            }
            return false;
        }
    }
    return true;
}

bool
GCArena::unpublishAddressIndex() {
    if (!addressMetadata) {
        return false;
    }
    bool unpublished = true;
    for (size_t slot = 0; slot < slotCount; ++slot) {
        if (!GCAddressIndex::instance().unpublishArena(&addressMetadata[slot])) {
            unpublished = false;
        }
    }
    return unpublished;
}

bool
GCArena::hasPointer(char* pointer) const {
    uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
    uintptr_t base = reinterpret_cast<uintptr_t>(memory);
    return address >= base && address - base < reservationSize;
}

size_t
GCArena::pointerPageOffset(char* pointer) const {
    return static_cast<size_t>(pointer - memory) / kPageSize;
}

size_t
GCArena::markCommittedRange(char* pointer, size_t byteSize) {
    if (!byteSize) {
        return 0;
    }
    size_t start = pointerPageOffset(pointer);
    size_t end = pointerPageOffset(pointer + byteSize - 1) + 1;
    if (end > pageCapacity) {
        end = pageCapacity;
    }
    size_t recommitted = 0;
    for (size_t index = start; index < end; ++index) {
        if (!committedBitmap.get(index)) {
            committedBitmap.set(index);
            ++recommitted;
        }
    }
    return recommitted;
}

char*
GCArena::allocPages(size_t pageCount, bool bestFit, bool* zeroed) {
    if (!pageCount || allocatedPageCount + pageCount > pageCapacity) {
        return nullptr;
    }

    size_t offset = 0;
    bool found = bestFit
        ? pageBitmap.findBestFitConsecutive(pageCount, false, offset)
        : pageBitmap.findFirstConsecutive(pageCount, false, firstFreePage, offset);
    if (!found) {
        return nullptr;
    }

    size_t dirtyBefore = dirtyPagesInRange(offset, offset + pageCount);
    for (size_t index = offset; index < offset + pageCount; ++index) {
        pageBitmap.set(index);
    }
    emptySinceCycle = 0;

    if (offset == firstFreePage) {
        firstFreePage = offset + pageCount;
    }

    char* pointer = memory + offset * kPageSize;
    size_t size = pageCount * kPageSize;
    size_t nativeSize = nativePageSize();
    size_t protectStart = alignDown(reinterpret_cast<size_t>(pointer), nativeSize);
    size_t protectEnd = alignUp(reinterpret_cast<size_t>(pointer) + size, nativeSize);
    size_t protectFirstPage = pointerPageOffset(reinterpret_cast<char*>(protectStart));
    size_t protectLastPage = std::min(
        pageCapacity,
        pointerPageOffset(reinterpret_cast<char*>(protectEnd - 1)) + 1);
    bool needsRecommit = false;
    for (size_t index = protectFirstPage; index < protectLastPage; ++index) {
        if (protectedBitmap.get(index)) {
            needsRecommit = true;
            break;
        }
    }

    // A fresh arena mapping is already readable, writable, and zero-filled. Only pages that
    // scavenging actually protected need the syscall and explicit zeroing on reuse.
    if (needsRecommit) {
        if (!brixGCRuntimeRecommitPages(
                reinterpret_cast<void*>(protectStart),
                protectEnd - protectStart)) {
            for (size_t index = offset; index < offset + pageCount; ++index) {
                pageBitmap.clear(index);
            }
            firstFreePage = std::min(firstFreePage, offset);
            std::fprintf(stderr, "Brix GC arena recommit failed; allocation will return null\n");
            return nullptr;
        }
        // MADV_FREE contents are not guaranteed to be discarded immediately. Zero every
        // protected GC page before its span can publish, including a native-page neighbour
        // brought back by alignment.
        for (size_t index = protectFirstPage; index < protectLastPage; ++index) {
            if (protectedBitmap.get(index)) {
                std::memset(memory + index * kPageSize, 0, kPageSize);
                protectedBitmap.clear(index);
            }
        }
    }

    if (zeroed) {
        bool fresh = true;
        for (size_t index = offset; index < offset + pageCount; ++index) {
            if (committedBitmap.get(index)) {
                fresh = false;
                break;
            }
        }
        *zeroed = fresh;
    }
    size_t recommitted = markCommittedRange(
        reinterpret_cast<char*>(protectStart),
        protectEnd - protectStart);
    committedPageCount += recommitted;
    decommittedPageCount = decommittedPageCount > recommitted
        ? decommittedPageCount - recommitted
        : 0;
    allocatedPageCount += pageCount;
    dirtyPages += dirtyPagesInRange(offset, offset + pageCount);
    dirtyPages -= dirtyBefore;
    return pointer;
}

void
GCArena::freePages(char* pointer, size_t pageCount) {
    size_t offset = pointerPageOffset(pointer);
    size_t dirtyBefore = dirtyPagesInRange(offset, offset + pageCount);
    size_t cleared = 0;
    for (size_t index = offset; index < offset + pageCount; ++index) {
        if (pageBitmap.get(index)) {
            pageBitmap.clear(index);
            ++cleared;
        }
    }
    if (cleared != pageCount) {
        abortGC("GC arena page double free");
    }
    if (allocatedPageCount < cleared) {
        abortGC("GC arena page accounting underflow");
    }
    allocatedPageCount -= cleared;
    dirtyPages += dirtyPagesInRange(offset, offset + pageCount);
    dirtyPages -= dirtyBefore;
    firstFreePage = std::min(firstFreePage, offset);
}

size_t
GCArena::scavengeFreePages(size_t budget) {
    if (!budget) {
        return 0;
    }

    if (committedPageCount == 0 || dirtyPages == 0) {
        return 0;
    }

    size_t unitPages = scavengeUnitPages();
    size_t released = 0;
    size_t index = 0;
    while (index + unitPages <= pageCapacity && released + unitPages <= budget &&
            released < dirtyPages) {
        if (!isDirtyUnit(index, unitPages)) {
            index += unitPages;
            continue;
        }

        size_t runStart = index;
        size_t runPages = 0;
        while (index + unitPages <= pageCapacity &&
                released + runPages + unitPages <= budget) {
            if (!isDirtyUnit(index, unitPages)) {
                break;
            }
            runPages += unitPages;
            index += unitPages;
        }
        if (!runPages) {
            index += unitPages;
            continue;
        }
        char* run = memory + runStart * kPageSize;
        if (!brixGCRuntimeDecommitPages(run, runPages * kPageSize)) {
            std::fprintf(stderr, "Brix GC could not decommit a free page run; retaining pages\n");
            index += unitPages;
            continue;
        }
        for (size_t page = runStart; page < runStart + runPages; ++page) {
            committedBitmap.clear(page);
            protectedBitmap.set(page);
        }
        committedPageCount -= runPages;
        decommittedPageCount += runPages;
        released += runPages;
    }
    dirtyPages -= released;
    return released;
}

size_t
GCArena::dirtyPageCount() const {
    return dirtyPages;
}

size_t
GCArena::dirtyPagesInRange(size_t firstPage, size_t endPage) const {
    size_t unitPages = scavengeUnitPages();
    size_t count = 0;
    for (size_t index = firstPage - firstPage % unitPages;
            index < endPage && index + unitPages <= pageCapacity;
            index += unitPages) {
        if (isDirtyUnit(index, unitPages)) {
            count += unitPages;
        }
    }
    return count;
}

bool
GCArena::isDirtyUnit(size_t firstPage, size_t unitPages) const {
    for (size_t page = firstPage; page < firstPage + unitPages; ++page) {
        if (!committedBitmap.get(page) || pageBitmap.get(page) || spanForPage(page)) {
            return false;
        }
    }
    return true;
}

bool
GCArena::canRelease() const {
    if (allocatedPageCount != 0) {
        return false;
    }
    for (size_t page = 0; page < pageCapacity; ++page) {
        if (spanForPage(page)) {
            return false;
        }
    }
    return true;
}

bool
GCArena::canReleaseAfterCycle(size_t cycle, size_t holdDownCycles) {
    if (!canRelease()) {
        emptySinceCycle = 0;
        return false;
    }
    if (!emptySinceCycle) {
        emptySinceCycle = cycle ? cycle : 1;
        return false;
    }
    size_t current = cycle ? cycle : 1;
    return current >= emptySinceCycle &&
        current - emptySinceCycle >= holdDownCycles;
}

bool
GCArena::publishSpanPages(GCSpan* span, bool publish) {
    if (!span || !memory || span->memory < memory || !span->pageCount) {
        return false;
    }
    size_t firstPage = pointerPageOffset(span->memory);
    if (firstPage >= pageCapacity || span->pageCount > pageCapacity - firstPage) {
        return false;
    }
    size_t endPage = firstPage + span->pageCount;
    size_t dirtyBefore = dirtyPagesInRange(firstPage, endPage);
    for (size_t page = firstPage; page < endPage;) {
        size_t slot = page / kArenaAddressPageCount;
        size_t slotEndPage = std::min(endPage, (slot + 1) * kArenaAddressPageCount);
        char* base = memory + page * kPageSize;
        size_t pageCount = slotEndPage - page;
        bool updated = publish
            ? GCAddressIndex::instance().publishSpan(
                &addressMetadata[slot], base, pageCount, span)
            : GCAddressIndex::instance().unpublishSpan(
                &addressMetadata[slot], base, pageCount, span);
        if (!updated) {
            return false;
        }
        page = slotEndPage;
    }
    dirtyPages += dirtyPagesInRange(firstPage, endPage);
    dirtyPages -= dirtyBefore;
    return true;
}

void
GCArena::registerSpan(GCSpan* span) {
    if (!publishSpanPages(span, true)) {
        abortGC("GC span address-index publication failed");
    }
}

void
GCArena::unregisterSpan(GCSpan* span) {
    if (!publishSpanPages(span, false)) {
        abortGC("GC span address-index unpublication failed");
    }
}

GCSpan*
GCArena::findSpan(char* pointer) {
    return GCAddressIndex::instance().lookupSpan(pointer);
}

} // namespace BxGC
