#ifndef BX_TRACE_H
#define BX_TRACE_H

#include <stdint.h>
#include <BxGC/BxGCAnnotations.h>

// *************************************************************************************************

struct BrixTraceFrame {
    uintptr_t pc;
};

struct BrixStackTrace {
    uint16_t frameCount;
    uint16_t flags;
    uint16_t maxFrames;
    uint16_t reserved;
    uint32_t taskId;
    uint32_t parentTaskId;
    const char* spawnFile;
    uint32_t spawnLine;
    uint32_t spawnColumn;
    uint32_t taskFlags;
    double secondsSinceEpoch;
    BrixTraceFrame frames[];
};

// *************************************************************************************************

extern "C" void* brixCaptureStackTrace();
extern "C" void* brixCaptureStackTraceSkipping(uint16_t skipFrames);
extern "C" BRIX_WITHOUT_GC void brixEnsureStackTrace(void** traceSlot);
extern "C" void brixPrintStackTrace(void* trace);
extern "C" char* brixFormatStackTrace(void* trace);
extern "C" BRIX_WITHOUT_GC double brixStackTraceTime(void* trace);
extern "C" void brixReportTaskHeader(const char* heading, void* task);
extern "C" void brixReportTaskStack(const char* heading, void* task, uintptr_t fp, uintptr_t pc);

#endif
