#ifndef BX_GC_ADDRESS_INDEX_H
#define BX_GC_ADDRESS_INDEX_H

#include "BxGCCommon.h"

namespace BxGC {

// Address-index metadata is address-stable for as long as its arena is published. The full
// 8192-entry table covers every 8 KiB page in one aligned 64 MiB slot; only the first
// pageCapacity entries are data pages on which spans may be published.
struct ArenaMeta {
    char* base;
    GCArena* arena;
    size_t pageCapacity;
    std::array<std::atomic<GCSpan*>, kArenaAddressPageCount> pageToSpan;

    explicit ArenaMeta(char* base = nullptr, GCArena* arena = nullptr)
        : base(base), arena(arena), pageCapacity(0) {
        for (std::atomic<GCSpan*>& entry : pageToSpan) {
            entry.store(nullptr, std::memory_order_relaxed);
        }
    }

    ArenaMeta(const ArenaMeta&) = delete;
    ArenaMeta& operator=(const ArenaMeta&) = delete;

    void initialize(char* slotBase, GCArena* arenaOwner, size_t slotPageCapacity) {
        base = slotBase;
        arena = arenaOwner;
        pageCapacity = slotPageCapacity;
    }

    GCSpan* spanForPage(size_t page) const {
        return page < pageToSpan.size()
            ? pageToSpan[page].load(std::memory_order_acquire)
            : nullptr;
    }
};

struct GCAddressLookup {
    ArenaMeta* arena = nullptr;
    GCSpan* span = nullptr;

    // Counts the L1 and L2 directory loads. A successful arena lookup has exactly two;
    // loading pageToSpan is deliberately reported separately.
    uint8_t indexLoads = 0;
    uint8_t pageLoads = 0;
};

class GCAddressIndex {
public:
    static GCAddressIndex& instance() {
        // The directory and every published L2 table live until process exit. Arena retirement can
        // therefore clear an L2 slot without racing reclamation of the directory traversed by readers.
        static GCAddressIndex index;
        return index;
    }

    // Publication is serialized by the page-heap owner. An occupied slot is never replaced.
    bool publishArena(ArenaMeta* metadata);
    bool unpublishArena(ArenaMeta* metadata);

    // Span metadata must be fully initialized before publication. Only retained data pages may
    // hold spans; bitmap-reserved pages remain permanently null.
    bool publishSpan(
        ArenaMeta* metadata,
        char* spanBase,
        size_t pageCount,
        GCSpan* span);
    bool unpublishSpan(
        ArenaMeta* metadata,
        char* spanBase,
        size_t pageCount,
        GCSpan* span);

    GCAddressLookup lookup(const void* address) const {
        GCAddressLookup result;
        uintptr_t candidate = reinterpret_cast<uintptr_t>(address);
        if (!candidate || candidate >= kAddressIndexLimit) {
            return result;
        }

        size_t key = static_cast<size_t>(candidate >> kHeapArenaShift);
        size_t l1Index = key >> kAddressIndexBits;
        size_t l2Index = key & (kAddressIndexEntries - 1);
        L2* table = l1[l1Index].load(std::memory_order_acquire);
        result.indexLoads = 1;
        if (!table) {
            return result;
        }

        result.arena = table->entries[l2Index].load(std::memory_order_acquire);
        result.indexLoads = 2;
        if (!result.arena) {
            return result;
        }

        uintptr_t base = reinterpret_cast<uintptr_t>(result.arena->base);
        if (candidate < base || candidate - base >= kArenaSize) {
            result.arena = nullptr;
            return result;
        }

        size_t page = static_cast<size_t>((candidate - base) / kPageSize);
        result.span = result.arena->pageToSpan[page].load(std::memory_order_acquire);
        result.pageLoads = 1;
        return result;
    }

    ArenaMeta* lookupArena(const void* address) const {
        return lookup(address).arena;
    }

    GCSpan* lookupSpan(const void* address) const {
        return lookup(address).span;
    }

private:
    struct L2 {
        std::array<std::atomic<ArenaMeta*>, kAddressIndexEntries> entries;

        L2();
    };

    std::array<std::atomic<L2*>, kAddressIndexEntries> l1;

    GCAddressIndex();
    ~GCAddressIndex() = default;
    GCAddressIndex(const GCAddressIndex&) = delete;
    GCAddressIndex& operator=(const GCAddressIndex&) = delete;

    L2* ensureL2(size_t index);
};

} // namespace BxGC

#endif
