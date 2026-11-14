#include "YuGCAddressIndex.h"

namespace YuGC {

static_assert(kArenaSize == (1ull << kHeapArenaShift), "arena index shift must match size");
static_assert(
    ATOMIC_POINTER_LOCK_FREE == 2,
    "GC address-index pointers must be lock-free on every supported target");
static_assert(kAddressIndexEntries == 8192, "address index geometry changed");
static_assert(kArenaAddressPageCount == 8192, "arena address-page geometry changed");
static_assert(kArenaPageCapacity == 7936, "retained arena data-page geometry changed");
static_assert(
    kAddressIndexEntries * kAddressIndexEntries * kArenaSize == kAddressIndexLimit,
    "address index must cover exactly the low 2^52 bytes");

static std::atomic<size_t> gAddressIndexMetadataBytes {sizeof(GCAddressIndex)};

GCAddressIndex::L2::L2() {
    for (std::atomic<ArenaMeta*>& entry : entries) {
        entry.store(nullptr, std::memory_order_relaxed);
    }
}

GCAddressIndex::GCAddressIndex() {
    for (std::atomic<L2*>& entry : l1) {
        entry.store(nullptr, std::memory_order_relaxed);
    }
}

GCAddressIndex::L2*
GCAddressIndex::ensureL2(size_t index) {
    L2* table = l1[index].load(std::memory_order_acquire);
    if (table) {
        return table;
    }

    L2* candidate = new (std::nothrow) L2();
    if (!candidate) {
        return nullptr;
    }

    L2* empty = nullptr;
    if (l1[index].compare_exchange_strong(
            empty,
            candidate,
            std::memory_order_acq_rel,
            std::memory_order_acquire)) {
        gAddressIndexMetadataBytes.fetch_add(sizeof(L2), std::memory_order_relaxed);
        return candidate;
    }

    // This table was never published, so no reader can observe it. The winning table remains
    // process-lifetime, as do all other tables that ever become reachable from L1.
    delete candidate;
    return empty;
}

bool
GCAddressIndex::publishArena(ArenaMeta* metadata) {
    if (!metadata || !metadata->base) {
        return false;
    }

    uintptr_t address = reinterpret_cast<uintptr_t>(metadata->base);
    if (address >= kAddressIndexLimit || address % kArenaSize != 0) {
        return false;
    }

    size_t key = static_cast<size_t>(address >> kHeapArenaShift);
    size_t l1Index = key >> kAddressIndexBits;
    size_t l2Index = key & (kAddressIndexEntries - 1);
    L2* table = ensureL2(l1Index);
    if (!table) {
        return false;
    }

    ArenaMeta* empty = nullptr;
    bool published = table->entries[l2Index].compare_exchange_strong(
        empty,
        metadata,
        std::memory_order_acq_rel,
        std::memory_order_acquire);
    if (published) {
        gAddressIndexMetadataBytes.fetch_add(sizeof(ArenaMeta), std::memory_order_relaxed);
    }
    return published;
}

bool
GCAddressIndex::unpublishArena(ArenaMeta* metadata) {
    if (!metadata || !metadata->base) {
        return false;
    }

    uintptr_t address = reinterpret_cast<uintptr_t>(metadata->base);
    if (address >= kAddressIndexLimit || address % kArenaSize != 0) {
        return false;
    }

    size_t key = static_cast<size_t>(address >> kHeapArenaShift);
    size_t l1Index = key >> kAddressIndexBits;
    size_t l2Index = key & (kAddressIndexEntries - 1);
    L2* table = l1[l1Index].load(std::memory_order_acquire);
    if (!table) {
        return false;
    }

    ArenaMeta* expected = metadata;
    bool unpublished = table->entries[l2Index].compare_exchange_strong(
        expected,
        nullptr,
        std::memory_order_acq_rel,
        std::memory_order_acquire);
    if (unpublished) {
        gAddressIndexMetadataBytes.fetch_sub(sizeof(ArenaMeta), std::memory_order_relaxed);
    }
    return unpublished;
}

static bool
spanPageRange(
    ArenaMeta* metadata,
    char* spanBase,
    size_t pageCount,
    size_t& firstPage) {
    if (!metadata || !metadata->base || !spanBase || !pageCount) {
        return false;
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(metadata->base);
    uintptr_t address = reinterpret_cast<uintptr_t>(spanBase);
    if (address < base) {
        return false;
    }
    uintptr_t offset = address - base;
    if (offset % kPageSize != 0) {
        return false;
    }
    firstPage = static_cast<size_t>(offset / kPageSize);
    return firstPage < metadata->pageCapacity &&
        pageCount <= metadata->pageCapacity - firstPage;
}

bool
GCAddressIndex::publishSpan(
    ArenaMeta* metadata,
    char* spanBase,
    size_t pageCount,
    GCSpan* span) {
    size_t firstPage = 0;
    if (!span || !spanPageRange(metadata, spanBase, pageCount, firstPage)) {
        return false;
    }

    for (size_t page = firstPage; page < firstPage + pageCount; ++page) {
        if (metadata->pageToSpan[page].load(std::memory_order_acquire)) {
            return false;
        }
    }
    for (size_t page = firstPage; page < firstPage + pageCount; ++page) {
        metadata->pageToSpan[page].store(span, std::memory_order_release);
    }
    return true;
}

bool
GCAddressIndex::unpublishSpan(
    ArenaMeta* metadata,
    char* spanBase,
    size_t pageCount,
    GCSpan* span) {
    size_t firstPage = 0;
    if (!span || !spanPageRange(metadata, spanBase, pageCount, firstPage)) {
        return false;
    }

    for (size_t page = firstPage; page < firstPage + pageCount; ++page) {
        if (metadata->pageToSpan[page].load(std::memory_order_acquire) != span) {
            return false;
        }
    }
    for (size_t page = firstPage; page < firstPage + pageCount; ++page) {
        metadata->pageToSpan[page].store(nullptr, std::memory_order_release);
    }
    return true;
}

extern "C" size_t
yumiGCRuntimeAddressIndexMetadataBytes(void) {
    return gAddressIndexMetadataBytes.load(std::memory_order_acquire);
}

} // namespace YuGC
