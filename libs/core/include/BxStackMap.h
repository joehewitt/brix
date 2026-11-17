#ifndef BX_STACK_MAP_H
#define BX_STACK_MAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* brixGetGCStackMapPointer(size_t* outSize);
void* brixGetGCRootMapPointer(size_t* outSize);
void* brixGetTraceMetadataPointer(size_t* outSize);
void* brixGetPadTablePointer(size_t* outSize);

#ifdef __cplusplus
}
#endif

#endif
