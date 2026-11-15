#include <YuEvents/YuStack.h>
#include <YuEvents/YuEvents.h>
#include <YuEvents/YuLock.h>
#include <YuEvents/YuTask.h>
#include <YuEvents/YuThread.h>
#include <YuCore/YuTrace.h>

#include <mutex>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <vector>

#if defined(__APPLE__)
#include <mach/arm/thread_status.h>
#endif

#if defined(__has_include) && __has_include(<sanitizer/asan_interface.h>)
#include <sanitizer/asan_interface.h>
#else
#define ASAN_UNPOISON_MEMORY_REGION(address, size) ((void)(address), (void)(size))
#endif

// *************************************************************************************************

static const size_t kYuPooledStackLimit = 256;
static const size_t kYuHotStackCount = 16;
static const size_t kYuStackWarmSize = 0x10000;
static const size_t kYuThreadStackCacheLimit = 8;
static const size_t kYuThreadStackTransferCount = 4;

struct YuPooledStack {
    char* stack;
    bool checked;
    bool trimmed;
};

struct YuStackPool {
    YuLock lock;
    std::vector<YuPooledStack> stacks;
};

struct YuThreadStackCache {
    YuPooledStack stacks[kYuThreadStackCacheLimit];
    size_t count;
};

static YuStackPool&
stackPool() {
    static YuStackPool* pool = new YuStackPool();
    return *pool;
}

static thread_local YuThreadStackCache gThreadStackCache;

static struct sigaction gPreviousBusAction;
static struct sigaction gPreviousSegvAction;
static bool gStackOverflowExits = false;

// *************************************************************************************************

static char*
mapStack(size_t size) {
    char* allocation = (char*)mmap(0, size + kYuStackGuardSize,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (allocation == MAP_FAILED) {
        return NULL;
    }

    if (mprotect(allocation, kYuStackGuardSize, PROT_NONE) == -1) {
        munmap(allocation, size + kYuStackGuardSize);
        return NULL;
    }

    return allocation + kYuStackGuardSize;
}

static void
unmapStack(char* stack, size_t size) {
    ASAN_UNPOISON_MEMORY_REGION(stack - kYuStackGuardSize, size + kYuStackGuardSize);
    munmap(stack - kYuStackGuardSize, size + kYuStackGuardSize);
}

static bool
trimStack(char* stack, size_t size) {
    size_t pageSize = (size_t)getpagesize();
#if defined(__APPLE__)
    char state = 0;
    if (mincore(stack + size - kYuStackWarmSize - pageSize, pageSize, &state) != 0 ||
        !(state & (MINCORE_MODIFIED | MINCORE_PAGED_OUT))) {
        return false;
    }
    madvise(stack, size - kYuStackWarmSize, MADV_FREE_REUSABLE);
#else
    unsigned char state = 0;
    if (mincore(stack + size - kYuStackWarmSize - pageSize, pageSize, &state) != 0 || !(state & 1)) {
        return false;
    }
    madvise(stack, size - kYuStackWarmSize, MADV_FREE);
#endif
    return true;
}

static void
releaseToPool(YuPooledStack* stacks, size_t count) {
    char* excess[kYuThreadStackCacheLimit];
    size_t excessCount = 0;
    YuStackPool& pool = stackPool();
    pool.lock.lock();
    for (size_t i = 0; i < count; ++i) {
        size_t pooled = pool.stacks.size();
        if (pooled >= kYuPooledStackLimit) {
            excess[excessCount++] = stacks[i].stack;
            continue;
        }
        pool.stacks.push_back(stacks[i]);
        if (pooled >= kYuHotStackCount) {
            YuPooledStack& cold = pool.stacks[pooled - kYuHotStackCount];
            if (!cold.checked) {
                cold.checked = true;
                cold.trimmed = trimStack(cold.stack, kYuTaskStackSize);
            }
        }
    }
    pool.lock.unlock();

    for (size_t i = 0; i < excessCount; ++i) {
        unmapStack(excess[i], kYuTaskStackSize);
    }
}

static void
refillFromPool(YuThreadStackCache& cache) {
    YuStackPool& pool = stackPool();
    pool.lock.lock();
    size_t pooled = pool.stacks.size();
    size_t count = pooled < kYuThreadStackTransferCount ? pooled : kYuThreadStackTransferCount;
    for (size_t i = 0; i < count; ++i) {
        cache.stacks[i] = pool.stacks[pooled - count + i];
    }
    pool.stacks.resize(pooled - count);
    pool.lock.unlock();
    cache.count = count;
}

char*
yumiAcquireStack(size_t size) {
    if (size != kYuTaskStackSize) {
        return mapStack(size);
    }

    YuThreadStackCache& cache = gThreadStackCache;
    if (!cache.count) {
        refillFromPool(cache);
        if (!cache.count) {
            return mapStack(size);
        }
    }

    YuPooledStack pooled = cache.stacks[--cache.count];
#if defined(__APPLE__)
    if (pooled.trimmed) {
        madvise(pooled.stack, size - kYuStackWarmSize, MADV_FREE_REUSE);
    }
#endif
    return pooled.stack;
}

void
yumiReleaseStack(char* stack, size_t size) {
    if (size != kYuTaskStackSize) {
        unmapStack(stack, size);
        return;
    }

    ASAN_UNPOISON_MEMORY_REGION(stack, size);
    YuThreadStackCache& cache = gThreadStackCache;
    if (cache.count == kYuThreadStackCacheLimit) {
        releaseToPool(cache.stacks, kYuThreadStackTransferCount);
        cache.count -= kYuThreadStackTransferCount;
        memmove(
            cache.stacks,
            cache.stacks + kYuThreadStackTransferCount,
            cache.count * sizeof(YuPooledStack));
    }
    cache.stacks[cache.count++] = { stack, false, false };
}

void
yumiFlushThreadStackCache() {
    YuThreadStackCache& cache = gThreadStackCache;
    releaseToPool(cache.stacks, cache.count);
    cache.count = 0;
}

// *************************************************************************************************

static bool
yumiInStackGuard(YuTask* task, uintptr_t address) {
    uintptr_t stack = reinterpret_cast<uintptr_t>(task->getStackBase());
    return address < stack && address >= stack - kYuStackGuardSize;
}

static void
yumiStackFaultHandler(int signal, siginfo_t* info, void* data) {
    ucontext_t* context = static_cast<ucontext_t*>(data);
    YuThread* thread = yumiGetActiveThread();
    YuTask* task = thread ? thread->getActiveTask() : NULL;
    if (task && context) {
        uintptr_t address = reinterpret_cast<uintptr_t>(info->si_addr);
#if defined(__APPLE__)
        uintptr_t sp = arm_thread_state64_get_sp(context->uc_mcontext->__ss);
        uintptr_t fp = arm_thread_state64_get_fp(context->uc_mcontext->__ss);
        uintptr_t pc = arm_thread_state64_get_pc(context->uc_mcontext->__ss);
#else
        uintptr_t sp = context->uc_mcontext.sp;
        uintptr_t fp = context->uc_mcontext.regs[29];
        uintptr_t pc = context->uc_mcontext.pc;
#endif
        if (yumiInStackGuard(task, address) || yumiInStackGuard(task, sp)) {
            yumiReportTaskStack("Stack overflow", task, fp, pc);
            if (gStackOverflowExits) {
                _exit(1);
            }
            struct sigaction fallback;
            memset(&fallback, 0, sizeof(fallback));
            fallback.sa_handler = SIG_DFL;
            sigemptyset(&fallback.sa_mask);
            sigaction(SIGBUS, &fallback, NULL);
            sigaction(SIGSEGV, &fallback, NULL);
            return;
        }
    }

    sigaction(signal, signal == SIGBUS ? &gPreviousBusAction : &gPreviousSegvAction, NULL);
}

void
yumiInstallStackOverflowHandler() {
    static std::once_flag once;
    std::call_once(once, []() {
        const char* continues = getenv("YUMITEST_BATCH_CONTINUE_AFTER_PANIC");
        gStackOverflowExits = continues && continues[0] == '1';
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_sigaction = yumiStackFaultHandler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sigaction(SIGBUS, &action, &gPreviousBusAction);
        sigaction(SIGSEGV, &action, &gPreviousSegvAction);
    });
}

void*
yumiInstallSignalStack() {
    stack_t current;
    if (sigaltstack(NULL, &current) == 0 && !(current.ss_flags & SS_DISABLE)) {
        return NULL;
    }

    void* memory = mmap(0, SIGSTKSZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return NULL;
    }

    stack_t signalStack;
    signalStack.ss_sp = memory;
    signalStack.ss_size = SIGSTKSZ;
    signalStack.ss_flags = 0;
    if (sigaltstack(&signalStack, NULL) != 0) {
        munmap(memory, SIGSTKSZ);
        return NULL;
    }
    return memory;
}

void
yumiRemoveSignalStack(void* signalStack) {
    if (!signalStack) {
        return;
    }

    stack_t disabled;
    disabled.ss_sp = NULL;
    disabled.ss_size = 0;
    disabled.ss_flags = SS_DISABLE;
    sigaltstack(&disabled, NULL);
    munmap(signalStack, SIGSTKSZ);
}
