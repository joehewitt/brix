#include "YuCore/YuStackMap.h"

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

#define YU_SECTION_DATA(name) sectionData("__" #name, outSize)

#else

#define YU_DECLARE_SECTION(name) \
    extern "C" __attribute__((weak)) uint8_t __start_##name[]; \
    extern "C" __attribute__((weak)) uint8_t __stop_##name[];

YU_DECLARE_SECTION(yumi_trace)
YU_DECLARE_SECTION(yumi_gcmap)
YU_DECLARE_SECTION(yumi_gcroots)
YU_DECLARE_SECTION(yumi_pads)

static void*
sectionData(uint8_t* start, uint8_t* stop, size_t* outSize) {
    if (outSize) {
        *outSize = start && stop ? (size_t)(stop - start) : 0;
    }
    return start;
}

#define YU_SECTION_DATA(name) sectionData(__start_##name, __stop_##name, outSize)

#endif

extern "C" void*
yumiGetGCStackMapPointer(size_t* outSize) {
    return YU_SECTION_DATA(yumi_gcmap);
}

extern "C" void*
yumiGetGCRootMapPointer(size_t* outSize) {
    return YU_SECTION_DATA(yumi_gcroots);
}

extern "C" void*
yumiGetTraceMetadataPointer(size_t* outSize) {
    return YU_SECTION_DATA(yumi_trace);
}

extern "C" void*
yumiGetPadTablePointer(size_t* outSize) {
    return YU_SECTION_DATA(yumi_pads);
}
