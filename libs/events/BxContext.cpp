
#include <BxCore/BxCore.h>
#include <BxEvents/BxEvents.h>
#include <BxEvents/BxContext.h>
#include <BxEvents/BxStack.h>

#include <cstring>

#if defined(__APPLE__)
#define BX_SAVE_THREAD_REGISTER "mrs x17, tpidr_el0\n" "str x17, [x16], #8\n"
#define BX_LOAD_THREAD_REGISTER "ldr x15, [x16], #8\n" "msr tpidr_el0, x15\n"
#else
#define BX_SAVE_THREAD_REGISTER "add x16, x16, #8\n"
#define BX_LOAD_THREAD_REGISTER "add x16, x16, #8\n"
#endif

// **********************************************************************************************

void brixContextNew(BxContext* context, BxTaskStartFunc startFunc, void* closure, size_t stackSize) {
    memset(context, 0, sizeof(BxContext));

    context->lr = (void*)exit;
    context->pc = (void*)exit;
    context->func = startFunc;
    context->arg = closure;
    context->stackSize = stackSize;
}

int brixContextEnsureStack(BxContext* context) {
    if (context->stack) {
        return 0;
    }

    char* stack = brixAcquireStack(context->stackSize);
    if (stack == NULL) {
        return -1;
    }

    uintptr_t stackTop = __builtin_align_up((uintptr_t)(stack + context->stackSize), 16);
    context->sp = (void*)(stackTop - 16);
    context->stack = stack;
    return 0;
}

void brixContextDestroy(BxContext* context) {
    if (context->stack) {
        brixReleaseStack((char*)context->stack, context->stackSize);
    }
}

void brixContextGet(BxContext* context) {
    context->func = NULL;
    context->arg = NULL;

    asm volatile (
        "mov x16, %0\n"
        "str x29, [x16], #8\n" // frame pointer
        "str x30, [x16], #8\n" // link register
        "mov x17, sp\n" // stack pointer
        "str x17, [x16], #8\n"
        "adr x17, 1f\n" // program counter
        "str x17, [x16], #8\n"
        BX_SAVE_THREAD_REGISTER
        "str x19, [x16], #8\n"
        "str x20, [x16], #8\n"
        "str x21, [x16], #8\n"
        "str x22, [x16], #8\n"
        "str x23, [x16], #8\n"
        "str x24, [x16], #8\n"
        "str x25, [x16], #8\n"
        "str x26, [x16], #8\n"
        "str x27, [x16], #8\n"
        "str x28, [x16], #8\n"
        "str d8, [x16], #8\n"
        "str d9, [x16], #8\n"
        "str d10, [x16], #8\n"
        "str d11, [x16], #8\n"
        "str d12, [x16], #8\n"
        "str d13, [x16], #8\n"
        "str d14, [x16], #8\n"
        "str d15, [x16], #8\n"

        "1:\n"

        :
        : "r" (context)
        : "memory", "x16", "x17", "x19", "x20", "x21", "x22", "x23", "x24",
        "x25", "x26", "x27", "x28", "x30"
    );
}

void brixContextSet(BxContext* context) {
    asm volatile (
        "mov x16, %0\n"
        "ldr x29, [x16], #8\n" // frame pointer
        "ldr x30, [x16], #8\n" // link register
        "ldr x17, [x16], #8\n" // stack pointer
        "mov sp, x17\n"
        "ldr x17, [x16], #8\n" // program counter
        BX_LOAD_THREAD_REGISTER
        "ldr x19, [x16], #8\n"
        "ldr x20, [x16], #8\n"
        "ldr x21, [x16], #8\n"
        "ldr x22, [x16], #8\n"
        "ldr x23, [x16], #8\n"
        "ldr x24, [x16], #8\n"
        "ldr x25, [x16], #8\n"
        "ldr x26, [x16], #8\n"
        "ldr x27, [x16], #8\n"
        "ldr x28, [x16], #8\n"
        "ldr d8, [x16], #8\n"
        "ldr d9, [x16], #8\n"
        "ldr d10, [x16], #8\n"
        "ldr d11, [x16], #8\n"
        "ldr d12, [x16], #8\n"
        "ldr d13, [x16], #8\n"
        "ldr d14, [x16], #8\n"
        "ldr d15, [x16], #8\n"

        "br x17\n"

        :
        : "r" (context)
        : "memory", "x16", "x17", "x19", "x20", "x21", "x22", "x23", "x24",
        "x25", "x26", "x27", "x28", "x30"
    );
}

void brixContextSwap(BxContext* oldContext, BxContext* newContext) {
    __sync_synchronize();

    asm volatile (
        // Save stack-related registers
        "mov x16, %0\n"
        "str x29, [x16], #8\n" // frame pointer
        "str x30, [x16], #8\n" // link register
        "mov x17, sp\n" // stack pointer
        "str x17, [x16], #8\n"
        "adr x17, 2f\n" // program counter
        "str x17, [x16], #8\n"
        BX_SAVE_THREAD_REGISTER
        "str x19, [x16], #8\n"
        "str x20, [x16], #8\n"
        "str x21, [x16], #8\n"
        "str x22, [x16], #8\n"
        "str x23, [x16], #8\n"
        "str x24, [x16], #8\n"
        "str x25, [x16], #8\n"
        "str x26, [x16], #8\n"
        "str x27, [x16], #8\n"
        "str x28, [x16], #8\n"
        "str d8, [x16], #8\n"
        "str d9, [x16], #8\n"
        "str d10, [x16], #8\n"
        "str d11, [x16], #8\n"
        "str d12, [x16], #8\n"
        "str d13, [x16], #8\n"
        "str d14, [x16], #8\n"
        "str d15, [x16], #8\n"

        // Load stack-related registers
        "mov x16, %1\n"
        "ldr x29, [x16], #8\n" // frame pointer
        "ldr x30, [x16], #8\n" // link register
        "ldr x17, [x16], #8\n" // stack pointer
        "mov sp, x17\n"
        "ldr x17, [x16], #8\n"  // program counter (don't jump yet)
        BX_LOAD_THREAD_REGISTER
        "ldr x19, [x16], #8\n"
        "ldr x20, [x16], #8\n"
        "ldr x21, [x16], #8\n"
        "ldr x22, [x16], #8\n"
        "ldr x23, [x16], #8\n"
        "ldr x24, [x16], #8\n"
        "ldr x25, [x16], #8\n"
        "ldr x26, [x16], #8\n"
        "ldr x27, [x16], #8\n"
        "ldr x28, [x16], #8\n"
        "ldr d8, [x16], #8\n"
        "ldr d9, [x16], #8\n"
        "ldr d10, [x16], #8\n"
        "ldr d11, [x16], #8\n"
        "ldr d12, [x16], #8\n"
        "ldr d13, [x16], #8\n"
        "ldr d14, [x16], #8\n"
        "ldr d15, [x16], #8\n"

        // Call function if this is first context activation
        "ldr x1, [x16]\n"
        "cbz x1, 1f\n"
        "ldr x0, [x16, #8]\n"
        "str xzr, [x16]\n"
        "str xzr, [x16, #8]\n"
        "br x1\n"

        "1:\n"
        // Jump to program counter
        "br x17\n"

        "2:\n"
        :
        : "r" (oldContext), "r" (newContext)
        : "memory", "x0", "x1", "x16", "x17", "x19", "x20", "x21", "x22", "x23", "x24",
        "x25", "x26", "x27", "x28", "x30"
    );
}
