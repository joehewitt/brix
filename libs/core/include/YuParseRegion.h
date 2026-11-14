#ifndef YU_PARSE_REGION_H
#define YU_PARSE_REGION_H

#include <stddef.h>

// Runtime-only helpers used by the managed allocation facade. Public parser entry points live in
// YuCore.h so generated code has one stable import surface.
char* yumiParseRegionResizeBuffer(
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* descriptor);
void yumiParseRegionResetAll(void);

#endif
