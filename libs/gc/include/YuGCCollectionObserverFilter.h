#ifndef YU_GC_COLLECTION_OBSERVER_FILTER_H
#define YU_GC_COLLECTION_OBSERVER_FILTER_H

#include <array>
#include <atomic>
#include <stddef.h>
#include <stdint.h>

class YumiCollectionObserverFilter {
public:
    std::atomic<uintptr_t>& slot(char* collection) {
        uint64_t key = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(collection)) *
            0x9e3779b97f4a7c15ull;
        return slots[key >> (64 - kSlotBits)];
    }

    bool mayBeObserved(char* collection) {
        uintptr_t entry = slot(collection).load(std::memory_order_relaxed);
        return ((entry == reinterpret_cast<uintptr_t>(collection)) | (entry & 1)) != 0;
    }

    static bool retain(std::atomic<uintptr_t>& slot, char* collection) {
        uintptr_t key = reinterpret_cast<uintptr_t>(collection);
        uintptr_t entry = slot.load(std::memory_order_relaxed);
        while (true) {
            uintptr_t count = sharers(entry);
            if (count == UINTPTR_MAX >> 1) {
                return false;
            }
            uintptr_t next = !count && key && !(key & 1) ? key : ((count + 1) << 1) | 1;
            if (slot.compare_exchange_weak(
                    entry,
                    next,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed)) {
                return true;
            }
        }
    }

    static bool release(std::atomic<uintptr_t>& slot) {
        uintptr_t entry = slot.load(std::memory_order_relaxed);
        while (true) {
            uintptr_t count = sharers(entry);
            if (!count) {
                return false;
            }
            uintptr_t next = count == 1 ? 0 : ((count - 1) << 1) | 1;
            if (slot.compare_exchange_weak(
                    entry,
                    next,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed)) {
                return true;
            }
        }
    }

private:
    static constexpr size_t kSlotBits = 12;

    static uintptr_t sharers(uintptr_t entry) {
        return entry & 1 ? entry >> 1 : entry != 0;
    }

    std::array<std::atomic<uintptr_t>, size_t {1} << kSlotBits> slots {};
};

#endif
