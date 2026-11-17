#ifndef BX_STACK_H
#define BX_STACK_H

#include <stddef.h>

// *************************************************************************************************

const size_t kBxTaskStackSize = 0x200000;
const size_t kBxMainTaskStackSize = 0x800000;
const size_t kBxStackGuardSize = 0x10000;

// *************************************************************************************************

char* brixAcquireStack(size_t size);
void brixReleaseStack(char* stack, size_t size);
void brixFlushThreadStackCache();

void brixInstallStackOverflowHandler();
void* brixInstallSignalStack();
void brixRemoveSignalStack(void* signalStack);

#endif
