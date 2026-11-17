
#include "BxCore/BxPanic.h"
#include "BxCore/BxParseRegion.h"

#include "BxCore/BxCore.h"
#include "BxCore/BxProbe.h"
#include "BxCore/BxStackMap.h"
#include "BxCore/BxTrace.h"
#include "BxEvents/BxEvents.h"
#include "BxEvents/BxTaskRuntimeContext.h"
#include "BxEvents/BxThread.h"

#include <algorithm>
#include <fstream>
#if defined(__APPLE__)
#include <os/log.h>
#endif
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

struct BxPendingPanic {
    void* panic;
    void* root;
};

static void
brixCloseProbesBeforeExit() {
    BxThread* thread = brixGetActiveThread();
    if (thread) {
        thread->flushThreadLocals();
    }
    brixProbeClose();
}

static BxTaskRuntimeContext*
brixActivePanicContext() {
    return brixGetActiveTaskRuntimeContext();
}

static std::vector<BxPendingPanic>*
brixPendingPanics(BxTaskRuntimeContext* context) {
    return static_cast<std::vector<BxPendingPanic>*>(context->pendingPanics);
}

extern "C" void
brixDestroyTaskRuntimeContext(BxTaskRuntimeContext* context) {
    brixDrainTemporaryRoots(&context->temporaryRoots);
    if (context->activePanicRoot) {
        brixRemoveRoot(context->activePanicRoot);
    }
    context->activePanic = nullptr;
    context->activePanicRoot = nullptr;
    if (std::vector<BxPendingPanic>* pending = brixPendingPanics(context)) {
        for (const BxPendingPanic& entry : *pending) {
            if (entry.root) {
                brixRemoveRoot(entry.root);
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
brixContinueAfterUnhandledException() {
    const char* value = getenv("BRIXTEST_BATCH_CONTINUE_AFTER_PANIC");
    return value && value[0] == '1';
}

static void
brixUpdateActivePanicRoot(void* panic) {
    BxTaskRuntimeContext* context = brixActivePanicContext();
    if (context->activePanicRoot) {
        brixUpdateRoot(context->activePanicRoot, (char*)panic);
    } else if (panic) {
        context->activePanicRoot = brixAddRoot((char*)panic);
    }
}

static void
brixPublishActivePanicRoot(void* panic) {
    void* activePanic = brixPanicSuspend();
    brixUpdateActivePanicRoot(panic);
    brixPanicResume(activePanic);
}

static BxPendingPanic*
brixPendingCleanupPanic(BxTaskRuntimeContext* context) {
    std::vector<BxPendingPanic>* pending = brixPendingPanics(context);
    if (!pending || pending->empty() || !pending->back().panic) {
        return nullptr;
    }
    return &pending->back();
}

static void
brixResetParseRegionAfterPanic(BxTaskRuntimeContext* context) {
    if (std::vector<BxPendingPanic>* pending = brixPendingPanics(context)) {
        for (const BxPendingPanic& entry : *pending) {
            if (entry.panic) {
                brixParseRegionCancelAllocationScopes();
                return;
            }
        }
    }
    brixParseRegionResetAll();
}

static const char*
brixDefaultUnhandledMessage(const char* kind) {
    return strcmp(kind, "panic") == 0 ? "<panic>" : "<error>";
}

static const char*
brixPreviousColon(const char* source, const char* end) {
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
brixReadSourceLine(const char* fileStart, size_t fileLength, long lineNo, std::string& line) {
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
brixWriteUnhandledException(
    FILE* out,
    const char* kind,
    const char* message,
    const char* source) {
    fprintf(
        out,
        "\nUnhandled %s: %s\n",
        kind,
        message ? message : brixDefaultUnhandledMessage(kind));

    if (!source || !source[0]) {
        fprintf(out, "<no file>\n");
        return;
    }

    const char* columnColon = strrchr(source, ':');
    const char* lineColon = columnColon ? brixPreviousColon(source, columnColon) : nullptr;
    if (!lineColon || !columnColon || lineColon == source) {
        fprintf(out, "<%s>\n", source);
        return;
    }

    fprintf(
        out,
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
    if (!brixReadSourceLine(source, lineColon - source, lineNo, sourceLine)) {
        return;
    }

    fprintf(out, "    %s\n    ", sourceLine.c_str());
    for (long i = 1; i < column; ++i) {
        fputc(' ', out);
    }
    fprintf(out, "^\n");
}

static void
brixPrintUnhandledException(
    const char* kind,
    const char* message,
    const char* source) {
    brixWriteUnhandledException(stderr, kind, message, source);
}

static void
brixLogPanicFault(const char* message, const char* source, void* trace) {
#if defined(__APPLE__)
    char* text = nullptr;
    size_t size = 0;
    FILE* out = open_memstream(&text, &size);
    if (!out) {
        return;
    }
    brixWriteUnhandledException(out, "panic", message, source);
    if (char* stack = brixFormatStackTrace(trace)) {
        fputs(stack, out);
        free(stack);
    }
    fclose(out);
    os_log_fault(OS_LOG_DEFAULT, "%{public}s", text);
    free(text);
#else
    (void)message;
    (void)source;
    (void)trace;
#endif
}

extern "C" void
brixPanicSet(void* panic) {
    // A panic edge bypasses the normal compiler-emitted allocation-scope epilogue. Stop routing
    // the cleanups and handlers that run next into the parser region; the region itself remains
    // intact until the panic value has been observed and promoted.
    brixParseRegionCancelAllocationScopes();
    brixPublishActivePanicRoot(panic);
    brixActivePanicContext()->activePanic = panic;
}

extern "C" void
brixPanicClear() {
    brixActivePanicContext()->activePanic = nullptr;
    brixUpdateActivePanicRoot(nullptr);
}

extern "C" __attribute__((noinline)) void
brixRaisePanic(void* panic, void** traceSlot) {
    if (traceSlot && !*traceSlot) {
        *traceSlot = brixCaptureStackTraceSkipping(2);
    }
    brixPanicSet(panic);
}

extern "C" void*
brixPanicSuspend() {
    BxTaskRuntimeContext* context = brixActivePanicContext();
    void* panic = context->activePanic;
    context->activePanic = nullptr;
    return panic;
}

extern "C" void
brixPanicResume(void* panic) {
    brixActivePanicContext()->activePanic = panic;
}

extern "C" void
brixPanicBeginCleanup() {
    BxTaskRuntimeContext* context = brixActivePanicContext();
    void* panic = context->activePanic;
    context->activePanic = nullptr;

    std::vector<BxPendingPanic>* pending = brixPendingPanics(context);
    if (!pending) {
        pending = new std::vector<BxPendingPanic>();
        context->pendingPanics = pending;
    }
    pending->push_back({panic, panic ? brixAddRoot(static_cast<char*>(panic)) : nullptr});
}

extern "C" void
brixPanicEndCleanup() {
    BxTaskRuntimeContext* context = brixActivePanicContext();
    std::vector<BxPendingPanic>* pending = brixPendingPanics(context);
    if (!pending || pending->empty()) {
        return;
    }

    BxPendingPanic entry = pending->back();
    pending->pop_back();
    void* activePanic = context->activePanic;
    context->activePanic = nullptr;
    if (entry.panic && !activePanic) {
        brixUpdateActivePanicRoot(entry.panic);
        activePanic = entry.panic;
    }
    if (entry.root) {
        brixRemoveRoot(entry.root);
    }
    context->activePanic = activePanic;
}

extern "C" void*
brixTakePanic() {
    BxTaskRuntimeContext* context = brixActivePanicContext();
    void* panic = context->activePanic;
    context->activePanic = nullptr;
    void* promoted = brixParseRegionPromote(static_cast<char*>(panic));
    brixUpdateActivePanicRoot(promoted);
    brixResetParseRegionAfterPanic(context);
    return promoted;
}

extern "C" void*
brixTaskFailure() {
    // The panic ending the active task, without taking it: the active one, or the one set aside
    // while a cleanup runs. It is moved out of the parse region first, as brixTakePanic moves it,
    // since whoever reads it may keep it.
    BxTaskRuntimeContext* context = brixActivePanicContext();
    BxPendingPanic* pending = context->activePanic ? nullptr : brixPendingCleanupPanic(context);
    void* panic = pending ? pending->panic : context->activePanic;
    if (!panic) {
        return nullptr;
    }
    void* promoted = brixParseRegionPromote(static_cast<char*>(panic));
    if (promoted == panic) {
        return panic;
    }
    if (pending) {
        if (pending->root) {
            brixRemoveRoot(pending->root);
        }
        pending->panic = promoted;
        pending->root = brixAddRoot(static_cast<char*>(promoted));
    } else {
        context->activePanic = promoted;
        brixUpdateActivePanicRoot(promoted);
    }
    return promoted;
}

extern "C" void
brixReportUnhandledPanic(
    void* panic,
    const char* message,
    const char* source,
    void* trace) {
    if (!panic) {
        return;
    }

    brixPrintUnhandledException("panic", message, source);
    brixPrintStackTrace(trace);
    bool continues = brixContinueAfterUnhandledException();
    if (!continues) {
        brixLogPanicFault(message, source, trace);
    }

    if (brixActivePanicContext()->taskEnd == BX_TASK_END_RUNNING) {
        brixActivePanicContext()->taskEnd = BX_TASK_END_PANICKED;
    }
    brixActivePanicContext()->activePanic = nullptr;
    brixUpdateActivePanicRoot(nullptr);
    brixResetParseRegionAfterPanic(brixActivePanicContext());
    if (continues) {
        return;
    }

    brixCloseProbesBeforeExit();
    fflush(stderr);
    abort();
}

extern "C" void
brixReportFatalPanic(
    void* panic,
    const char* message,
    const char* source,
    void* trace) {
    if (!panic) {
        return;
    }

    static std::mutex reporting;
    reporting.lock();
    brixPrintUnhandledException("panic", message, source);
    brixPrintStackTrace(trace);
    if (brixContinueAfterUnhandledException()) {
        fflush(stderr);
        _Exit(1);
    }
    brixLogPanicFault(message, source, trace);
    fflush(stderr);
    abort();
}

extern "C" void
brixPrintUnhandledError(const char* message, const char* source, void* trace) {
    brixPrintUnhandledException("error", message, source);
    brixPrintStackTrace(trace);
    fflush(stderr);
}

extern "C" void
brixReportUnhandledError(
    void* error,
    const char* message,
    const char* source,
    void* trace) {
    if (!error) {
        return;
    }

    brixPrintUnhandledError(message, source, trace);
    BxTaskRuntimeContext* context = brixActivePanicContext();
    if (context && context->taskEnd == BX_TASK_END_RUNNING) {
        context->taskEnd = BX_TASK_END_RAISED;
    }
    if (brixContinueAfterUnhandledException()) {
        return;
    }

    brixCloseProbesBeforeExit();
    _Exit(1);
}

extern "C" void
brixImmediateAbort(
    const char* message,
    const char* file,
    uint32_t line,
    uint32_t column) {
    char source[4096];
    source[0] = 0;
    if (file && file[0] && line) {
        snprintf(source, sizeof(source), "%s:%u:%u", file, line, column ? column : 1);
    }

    void* trace = brixCaptureStackTraceSkipping(1);
    brixPrintUnhandledException("abort", message ? message : "Immediate abort", source);
    brixPrintStackTrace(trace);
    abort();
}

namespace {

struct BxPadEntry {
    uintptr_t returnAddress;
    uintptr_t pad;
};

constexpr uint32_t kBxPadsMagic = 0x44415059;
constexpr uint32_t kBxPadsVersion = 1;

uint32_t
brixReadPadWord(const uint8_t* data) {
    uint32_t value;
    memcpy(&value, data, sizeof(value));
    return value;
}

std::vector<BxPadEntry>*
brixBuildPadTable() {
    std::vector<BxPadEntry>* table = new std::vector<BxPadEntry>();
    size_t size = 0;
    const uint8_t* data = static_cast<const uint8_t*>(brixGetPadTablePointer(&size));
    size_t offset = 0;
    while (data && offset + 16 <= size) {
        uint32_t magic = brixReadPadWord(data + offset);
        if (magic == 0) {
            offset += 8;
            continue;
        }
        uint32_t version = brixReadPadWord(data + offset + 4) & 0xffff;
        uint32_t functionCount = brixReadPadWord(data + offset + 8);
        uint32_t entryCount = brixReadPadWord(data + offset + 12);
        size_t records = offset + 16;
        size_t entries = records + static_cast<size_t>(functionCount) * 16;
        size_t end = entries + static_cast<size_t>(entryCount) * 8;
        if (magic != kBxPadsMagic || version != kBxPadsVersion || end > size) {
            fprintf(stderr, "Invalid __brix_pads section\n");
            abort();
        }
        for (uint32_t index = 0; index < functionCount; index += 1) {
            const uint8_t* record = data + records + static_cast<size_t>(index) * 16;
            uintptr_t function;
            memcpy(&function, record, sizeof(function));
            uint32_t first = brixReadPadWord(record + 8);
            uint32_t count = brixReadPadWord(record + 12);
            if (static_cast<uint64_t>(first) + count > entryCount) {
                fprintf(stderr, "Invalid __brix_pads record\n");
                abort();
            }
            for (uint32_t position = first; position < first + count; position += 1) {
                const uint8_t* entry = data + entries + static_cast<size_t>(position) * 8;
                table->push_back({
                    function + brixReadPadWord(entry),
                    function + brixReadPadWord(entry + 4),
                });
            }
        }
        offset = end;
    }
    std::sort(table->begin(), table->end(), [](const BxPadEntry& left, const BxPadEntry& right) {
        return left.returnAddress < right.returnAddress;
    });
    return table;
}

const std::vector<BxPadEntry>&
brixPadTable() {
    static std::once_flag once;
    static std::vector<BxPadEntry>* table = nullptr;
    std::call_once(once, [] { table = brixBuildPadTable(); });
    return *table;
}

}

extern "C" void
brixPanicReturn() {
    if (!brixActivePanicContext()->activePanic) {
        return;
    }
    const std::vector<BxPadEntry>& table = brixPadTable();
    void** frame = static_cast<void**>(*static_cast<void**>(__builtin_frame_address(0)));
    uintptr_t returnAddress = reinterpret_cast<uintptr_t>(frame[1]);
    auto found = std::lower_bound(
        table.begin(),
        table.end(),
        returnAddress,
        [](const BxPadEntry& entry, uintptr_t address) {
            return entry.returnAddress < address;
        });
    if (found != table.end() && found->returnAddress == returnAddress) {
        frame[1] = reinterpret_cast<void*>(found->pad);
    }
}
