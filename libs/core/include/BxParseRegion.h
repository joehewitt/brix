#ifndef BX_PARSE_REGION_H
#define BX_PARSE_REGION_H

#include <stddef.h>

// Runtime-only helpers used by the managed allocation facade. Public parser entry points live in
// BxCore.h so generated code has one stable import surface.
char* brixParseRegionResizeBuffer(
    char* buffer,
    size_t oldSize,
    size_t newSize,
    void* descriptor);
void brixParseRegionResetAll(void);

#endif
