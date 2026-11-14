#ifndef YU_STACK_H
#define YU_STACK_H

#include <stddef.h>

// *************************************************************************************************

const size_t kYuTaskStackSize = 0x200000;
const size_t kYuMainTaskStackSize = 0x800000;
const size_t kYuStackGuardSize = 0x10000;

// *************************************************************************************************

char* yumiAcquireStack(size_t size);
void yumiReleaseStack(char* stack, size_t size);
void yumiFlushThreadStackCache();

void yumiInstallStackOverflowHandler();
void* yumiInstallSignalStack();
void yumiRemoveSignalStack(void* signalStack);

#endif
