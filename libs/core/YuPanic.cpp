
#include "YuCore/YuPanic.h"
#include "YuCore/YuParseRegion.h"

#include "YuCore/YuCore.h"
#include "YuCore/YuProbe.h"
#include "YuCore/YuTrace.h"
#include "YuEvents/YuEvents.h"
#include "YuEvents/YuTaskRuntimeContext.h"
#include "YuEvents/YuThread.h"

#include <fstream>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

struct YuPendingPanic {
    void* panic;
    void* root;
};

static void
yumiCloseProbesBeforeExit() {
    YuThread* thread = yumiGetActiveThread();
    if (thread) {
        thread->flushThreadLocals();
    }
    yumiProbeClose();
}

static YuTaskRuntimeContext*
yumiActivePanicContext() {
    return yumiGetActiveTaskRuntimeContext();
}

static std::vector<YuPendingPanic>*
yumiPendingPanics(YuTaskRuntimeContext* context) {
    return static_cast<std::vector<YuPendingPanic>*>(context->pendingPanics);
}

extern "C" void
yumiDestroyTaskRuntimeContext(YuTaskRuntimeContext* context) {
    yumiDrainTemporaryRoots(&context->temporaryRoots);
    if (context->activePanicRoot) {
        yumiRemoveRoot(context->activePanicRoot);
    }
    context->activePanic = nullptr;
    context->activePanicRoot = nullptr;
    if (std::vector<YuPendingPanic>* pending = yumiPendingPanics(context)) {
        for (const YuPendingPanic& entry : *pending) {
            if (entry.root) {
                yumiRemoveRoot(entry.root);
            }
        }
        delete pending;
    }
    context->pendingPanics = nullptr;
    context->parseRegionLogicalUsed = 0;
    context->parseRegionAllocationScopeDepth = 0;
    if (context->parseRegion && context->parseRegionDestroyer) {
        context->parseRegionDestroyer(context->parseRegion);
    }
    context->parseRegion = nullptr;
    context->parseRegionScanner = nullptr;
    context->parseRegionDestroyer = nullptr;
    free(context->probeProfileTimers);
    context->probeProfileTimers = nullptr;
}

static bool
yumiContinueAfterUnhandledException() {
    const char* value = getenv("YUMITEST_BATCH_CONTINUE_AFTER_PANIC");
    return value && value[0] == '1';
}

static void
yumiUpdateActivePanicRoot(void* panic) {
    YuTaskRuntimeContext* context = yumiActivePanicContext();
    if (context->activePanicRoot) {
        yumiUpdateRoot(context->activePanicRoot, (char*)panic);
    } else if (panic) {
        context->activePanicRoot = yumiAddRoot((char*)panic);
    }
}

static void
yumiPublishActivePanicRoot(void* panic) {
    void* activePanic = yumiPanicSuspend();
    yumiUpdateActivePanicRoot(panic);
    yumiPanicResume(activePanic);
}

static YuPendingPanic*
yumiPendingCleanupPanic(YuTaskRuntimeContext* context) {
    std::vector<YuPendingPanic>* pending = yumiPendingPanics(context);
    if (!pending || pending->empty() || !pending->back().panic) {
        return nullptr;
    }
    return &pending->back();
}

static void
yumiResetParseRegionAfterPanic(YuTaskRuntimeContext* context) {
    if (std::vector<YuPendingPanic>* pending = yumiPendingPanics(context)) {
        for (const YuPendingPanic& entry : *pending) {
            if (entry.panic) {
                yumiParseRegionCancelAllocationScopes();
                return;
            }
        }
    }
    yumiParseRegionResetAll();
}

static const char*
yumiDefaultUnhandledMessage(const char* kind) {
    return strcmp(kind, "panic") == 0 ? "<panic>" : "<error>";
}

static const char*
yumiPreviousColon(const char* source, const char* end) {
    const char* p = end;
    while (p > source) {
        --p;
        if (*p == ':') {
            return p;
        }
    }
    return nullptr;
}

static bool
yumiReadSourceLine(const char* fileStart, size_t fileLength, long lineNo, std::string& line) {
    if (!fileStart || !fileLength || lineNo < 1) {
        return false;
    }

    std::string filePath(fileStart, fileLength);
    if (filePath == "no file") {
        return false;
    }

    std::ifstream input(filePath);
    if (!input.is_open()) {
        return false;
    }

    for (long currentLine = 1; std::getline(input, line); ++currentLine) {
        if (currentLine == lineNo) {
            if (!line.empty() && line[line.size() - 1] == '\r') {
                line.resize(line.size() - 1);
            }
            return true;
        }
    }

    return false;
}

static void
yumiPrintUnhandledException(
    const char* kind,
    const char* message,
    const char* source) {
    fprintf(
        stderr,
        "\nUnhandled %s: %s\n",
        kind,
        message ? message : yumiDefaultUnhandledMessage(kind));

    if (!source || !source[0]) {
        fprintf(stderr, "<no file>\n");
        return;
    }

    const char* columnColon = strrchr(source, ':');
    const char* lineColon = columnColon ? yumiPreviousColon(source, columnColon) : nullptr;
    if (!lineColon || !columnColon || lineColon == source) {
        fprintf(stderr, "<%s>\n", source);
        return;
    }

    fprintf(
        stderr,
        "<%.*s:%.*s>\n",
        (int)(lineColon - source),
        source,
        (int)(columnColon - lineColon - 1),
        lineColon + 1);

    char* end = nullptr;
    long lineNo = strtol(lineColon + 1, &end, 10);
    if (end == lineColon + 1 || end != columnColon || lineNo < 1) {
        lineNo = 0;
    }

    long column = strtol(columnColon + 1, &end, 10);
    if (end == columnColon + 1 || column < 1) {
        column = 1;
    }

    std::string sourceLine;
    if (!yumiReadSourceLine(source, lineColon - source, lineNo, sourceLine)) {
        return;
    }

    fprintf(stderr, "    %s\n    ", sourceLine.c_str());
    for (long i = 1; i < column; ++i) {
        fputc(' ', stderr);
    }
    fprintf(stderr, "^\n");
}

extern "C" void
yumiPanicSet(void* panic) {
    // A panic edge bypasses the normal compiler-emitted allocation-scope epilogue. Stop routing
    // unrelated recovery work into the parser region; the region itself remains intact until the
    // panic value has been observed and promoted.
    yumiParseRegionCancelAllocationScopes();
    yumiPublishActivePanicRoot(panic);
    yumiActivePanicContext()->activePanic = panic;
}

extern "C" void*
yumiPanicGet() {
    return yumiActivePanicContext()->activePanic;
}

extern "C" void
yumiPanicClear() {
    yumiActivePanicContext()->activePanic = nullptr;
    yumiUpdateActivePanicRoot(nullptr);
}

extern "C" void*
yumiPanicSuspend() {
    YuTaskRuntimeContext* context = yumiActivePanicContext();
    void* panic = context->activePanic;
    context->activePanic = nullptr;
    return panic;
}

extern "C" void
yumiPanicResume(void* panic) {
    yumiActivePanicContext()->activePanic = panic;
}

extern "C" void
yumiPanicBeginCleanup() {
    YuTaskRuntimeContext* context = yumiActivePanicContext();
    void* panic = context->activePanic;
    context->activePanic = nullptr;

    std::vector<YuPendingPanic>* pending = yumiPendingPanics(context);
    if (!pending) {
        pending = new std::vector<YuPendingPanic>();
        context->pendingPanics = pending;
    }
    pending->push_back({panic, panic ? yumiAddRoot(static_cast<char*>(panic)) : nullptr});
}

extern "C" void
yumiPanicEndCleanup() {
    YuTaskRuntimeContext* context = yumiActivePanicContext();
    std::vector<YuPendingPanic>* pending = yumiPendingPanics(context);
    if (!pending || pending->empty()) {
        return;
    }

    YuPendingPanic entry = pending->back();
    pending->pop_back();
    void* activePanic = context->activePanic;
    context->activePanic = nullptr;
    if (entry.panic && !activePanic) {
        yumiUpdateActivePanicRoot(entry.panic);
        activePanic = entry.panic;
    }
    if (entry.root) {
        yumiRemoveRoot(entry.root);
    }
    context->activePanic = activePanic;
}

extern "C" void*
yumiRecover() {
    YuTaskRuntimeContext* context = yumiActivePanicContext();
    YuPendingPanic* pending = context->activePanic ? nullptr : yumiPendingCleanupPanic(context);
    void* panic = pending ? pending->panic : context->activePanic;
    if (panic && yumiActiveTaskIsTerminating()) {
        return panic;
    }

    if (pending) {
        pending->panic = nullptr;
    }
    context->activePanic = nullptr;
    void* promoted = yumiParseRegionPromote(static_cast<char*>(panic));
    yumiUpdateActivePanicRoot(promoted);
    yumiResetParseRegionAfterPanic(context);
    return promoted;
}

extern "C" void
yumiReportUnhandledPanic(
    void* panic,
    const char* message,
    const char* source,
    void* trace) {
    if (!panic) {
        return;
    }

    yumiPrintUnhandledException("panic", message, source);
    yumiPrintStackTrace(trace);

    yumiActivePanicContext()->activePanic = nullptr;
    yumiUpdateActivePanicRoot(nullptr);
    yumiResetParseRegionAfterPanic(yumiActivePanicContext());
    if (yumiContinueAfterUnhandledException()) {
        return;
    }

    yumiCloseProbesBeforeExit();
    fflush(stderr);
    _Exit(1);
}

extern "C" void
yumiReportFatalPanic(
    void* panic,
    const char* message,
    const char* source,
    void* trace) {
    if (!panic) {
        return;
    }

    static std::mutex reporting;
    reporting.lock();
    yumiPrintUnhandledException("panic", message, source);
    yumiPrintStackTrace(trace);
    fflush(stderr);
    _Exit(1);
}

extern "C" void
yumiPrintUnhandledError(const char* message, const char* source, void* trace) {
    yumiPrintUnhandledException("error", message, source);
    yumiPrintStackTrace(trace);
    fflush(stderr);
}

extern "C" void
yumiReportUnhandledError(
    void* error,
    const char* message,
    const char* source,
    void* trace) {
    if (!error) {
        return;
    }

    yumiPrintUnhandledError(message, source, trace);
    if (yumiContinueAfterUnhandledException()) {
        return;
    }

    yumiCloseProbesBeforeExit();
    _Exit(1);
}

extern "C" void
yumiImmediateAbort(
    const char* message,
    const char* file,
    uint32_t line,
    uint32_t column) {
    char source[4096];
    source[0] = 0;
    if (file && file[0] && line) {
        snprintf(source, sizeof(source), "%s:%u:%u", file, line, column ? column : 1);
    }

    void* trace = yumiCaptureStackTraceSkipping(1);
    yumiPrintUnhandledException("abort", message ? message : "Immediate abort", source);
    yumiPrintStackTrace(trace);
    abort();
}
