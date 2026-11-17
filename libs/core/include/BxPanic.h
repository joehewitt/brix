#ifndef BX_EXCEPTIONS_H
#define BX_EXCEPTIONS_H

#include <stdint.h>
#include <BxGC/BxGCAnnotations.h>

// *************************************************************************************************

extern "C" BRIX_WITHOUT_GC void brixPanicSet(void* panic);
extern "C" BRIX_WITHOUT_GC void brixPanicClear();
extern "C" BRIX_WITHOUT_GC void* brixPanicSuspend();
extern "C" BRIX_WITHOUT_GC void brixPanicResume(void* panic);
extern "C" BRIX_WITHOUT_GC void brixPanicBeginCleanup();
extern "C" BRIX_WITHOUT_GC void brixPanicEndCleanup();
extern "C" BRIX_WITHOUT_GC void brixPanicReturn();
extern "C" BRIX_WITHOUT_GC void brixRaisePanic(void* panic, void** traceSlot);
extern "C" void* brixTakePanic();
extern "C" void* brixTaskFailure();
extern "C" void brixReportUnhandledPanic(
    void* panic,
    const char* message,
    const char* source,
    void* trace);
extern "C" void brixReportFatalPanic(
    void* panic,
    const char* message,
    const char* source,
    void* trace);
extern "C" void brixReportUnhandledError(
    void* error,
    const char* message,
    const char* source,
    void* trace);
extern "C" void brixPrintUnhandledError(const char* message, const char* source, void* trace);
extern "C" void brixImmediateAbort(
    const char* message,
    const char* file,
    uint32_t line,
    uint32_t column);

#endif
