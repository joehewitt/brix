
#ifndef YU_CONTEXT_H
#define YU_CONTEXT_H

#include <YuEvents/YuEvents.h>

// **********************************************************************************************

typedef struct {
    void* fp;              // x29 (frame pointer)
    void* lr;              // x30 (link register)
    void* sp;              // stack pointer
    void* pc;              // program counter
    uint64_t tpidr_el0;    // thread local storage
    // A task switch crosses normal call boundaries, so preserve the AArch64 callee-saved registers.
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t d8;
    uint64_t d9;
    uint64_t d10;
    uint64_t d11;
    uint64_t d12;
    uint64_t d13;
    uint64_t d14;
    uint64_t d15;

    void (*func)(void*);
    void* arg;
    void* stack;
    size_t stackSize;
} YuContext;

// **********************************************************************************************

void yumiContextNew(YuContext* context, YuTaskStartFunc startFunc, void* closure, size_t stackSize);
int yumiContextEnsureStack(YuContext* context);
void yumiContextDestroy(YuContext* context);
void yumiContextGet(YuContext* context);
void yumiContextSet(YuContext* context);
void yumiContextSwap(YuContext* oldContext, YuContext* newContext);

#endif
