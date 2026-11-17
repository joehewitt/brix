#ifndef YU_EXCEPTIONS_H
#define YU_EXCEPTIONS_H

#include <stdint.h>
#include <YuGC/YuGCAnnotations.h>

// *************************************************************************************************

extern "C" YUMI_WITHOUT_GC void yumiPanicSet(void* panic);
extern "C" YUMI_WITHOUT_GC void yumiPanicClear();
extern "C" YUMI_WITHOUT_GC void* yumiPanicSuspend();
extern "C" YUMI_WITHOUT_GC void yumiPanicResume(void* panic);
extern "C" YUMI_WITHOUT_GC void yumiPanicBeginCleanup();
extern "C" YUMI_WITHOUT_GC void yumiPanicEndCleanup();
extern "C" YUMI_WITHOUT_GC void yumiPanicReturn();
extern "C" YUMI_WITHOUT_GC void yumiRaisePanic(void* panic, void** traceSlot);
extern "C" void* yumiTakePanic();
extern "C" void* yumiTaskFailure();
extern "C" void yumiReportUnhandledPanic(
    void* panic,
    const char* message,
    const char* source,
    void* trace);
extern "C" void yumiReportFatalPanic(
    void* panic,
    const char* message,
    const char* source,
    void* trace);
extern "C" void yumiReportUnhandledError(
    void* error,
    const char* message,
    const char* source,
    void* trace);
extern "C" void yumiPrintUnhandledError(const char* message, const char* source, void* trace);
extern "C" void yumiImmediateAbort(
    const char* message,
    const char* file,
    uint32_t line,
    uint32_t column);

#endif
