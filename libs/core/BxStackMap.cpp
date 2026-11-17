#include "BxCore/BxStackMap.h"

#include <stdint.h>

#if defined(__APPLE__)

#include <mach-o/getsect.h>
#include <mach-o/loader.h>

extern struct mach_header_64 _mh_execute_header;

static void*
sectionData(const char* name, size_t* outSize) {
    unsigned long size = 0;
    void* data = getsectiondata(&_mh_execute_header, "__DATA", name, &size);
    if (outSize) {
        *outSize = size;
    }
    return data;
}

#define BX_SECTION_DATA(name) sectionData("__" #name, outSize)

#else

#define BX_DECLARE_SECTION(name) \
    extern "C" __attribute__((weak)) uint8_t __start_##name[]; \
    extern "C" __attribute__((weak)) uint8_t __stop_##name[];

BX_DECLARE_SECTION(brix_trace)
BX_DECLARE_SECTION(brix_gcmap)
BX_DECLARE_SECTION(brix_gcroots)
BX_DECLARE_SECTION(brix_pads)

static void*
sectionData(uint8_t* start, uint8_t* stop, size_t* outSize) {
    if (outSize) {
        *outSize = start && stop ? (size_t)(stop - start) : 0;
    }
    return start;
}

#define BX_SECTION_DATA(name) sectionData(__start_##name, __stop_##name, outSize)

#endif

extern "C" void*
brixGetGCStackMapPointer(size_t* outSize) {
    return BX_SECTION_DATA(brix_gcmap);
}

extern "C" void*
brixGetGCRootMapPointer(size_t* outSize) {
    return BX_SECTION_DATA(brix_gcroots);
}

extern "C" void*
brixGetTraceMetadataPointer(size_t* outSize) {
    return BX_SECTION_DATA(brix_trace);
}

extern "C" void*
brixGetPadTablePointer(size_t* outSize) {
    return BX_SECTION_DATA(brix_pads);
}
