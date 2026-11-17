#ifndef BX_LOCK_H
#define BX_LOCK_H

#include <stdint.h>

#if defined(__APPLE__)
#include <os/lock.h>
#else
#include <atomic>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

// *************************************************************************************************

class BxLock {
public:
#if defined(__APPLE__)
    void lock() { os_unfair_lock_lock(&state); }
    void unlock() { os_unfair_lock_unlock(&state); }

private:
    os_unfair_lock state = OS_UNFAIR_LOCK_INIT;
#else
    void lock() {
        uint32_t current = 0;
        if (state.compare_exchange_strong(current, 1, std::memory_order_acquire)) {
            return;
        }
        if (current != 2) {
            current = state.exchange(2, std::memory_order_acquire);
        }
        while (current != 0) {
            syscall(SYS_futex, &state, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
            current = state.exchange(2, std::memory_order_acquire);
        }
    }

    void unlock() {
        if (state.exchange(0, std::memory_order_release) == 2) {
            syscall(SYS_futex, &state, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
        }
    }

private:
    std::atomic<uint32_t> state {0};
#endif
};

// *************************************************************************************************

#endif
