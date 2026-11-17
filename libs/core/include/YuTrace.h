#ifndef YU_TRACE_H
#define YU_TRACE_H

#include <stdint.h>
#include <YuGC/YuGCAnnotations.h>

// *************************************************************************************************

struct YumiTraceFrame {
    uintptr_t pc;
};

struct YumiStackTrace {
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
    YumiTraceFrame frames[];
};

// *************************************************************************************************

extern "C" void* yumiCaptureStackTrace();
extern "C" void* yumiCaptureStackTraceSkipping(uint16_t skipFrames);
extern "C" YUMI_WITHOUT_GC void yumiEnsureStackTrace(void** traceSlot);
extern "C" void yumiPrintStackTrace(void* trace);
extern "C" char* yumiFormatStackTrace(void* trace);
extern "C" YUMI_WITHOUT_GC double yumiStackTraceTime(void* trace);
extern "C" void yumiReportTaskHeader(const char* heading, void* task);
extern "C" void yumiReportTaskStack(const char* heading, void* task, uintptr_t fp, uintptr_t pc);

#endif
