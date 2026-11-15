#ifndef YU_STACK_MAP_H
#define YU_STACK_MAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* yumiGetGCStackMapPointer(size_t* outSize);
void* yumiGetGCRootMapPointer(size_t* outSize);
void* yumiGetTraceMetadataPointer(size_t* outSize);
void* yumiGetPadTablePointer(size_t* outSize);

#ifdef __cplusplus
}
#endif

#endif
