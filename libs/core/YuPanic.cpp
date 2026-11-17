
#include "YuCore/YuPanic.h"
#include "YuCore/YuParseRegion.h"

#include "YuCore/YuCore.h"
#include "YuCore/YuProbe.h"
#include "YuCore/YuStackMap.h"
#include "YuCore/YuTrace.h"
#include "YuEvents/YuEvents.h"
#include "YuEvents/YuTaskRuntimeContext.h"
#include "YuEvents/YuThread.h"

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
yumiWriteUnhandledException(
    FILE* out,
    const char* kind,
    const char* message,
    const char* source) {
    fprintf(
        out,
        "\nUnhandled %s: %s\n",
        kind,
        message ? message : yumiDefaultUnhandledMessage(kind));

    if (!source || !source[0]) {
        fprintf(out, "<no file>\n");
        return;
    }

    const char* columnColon = strrchr(source, ':');
    const char* lineColon = columnColon ? yumiPreviousColon(source, columnColon) : nullptr;
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
    if (!yumiReadSourceLine(source, lineColon - source, lineNo, sourceLine)) {
        return;
    }

    fprintf(out, "    %s\n    ", sourceLine.c_str());
    for (long i = 1; i < column; ++i) {
        fputc(' ', out);
    }
    fprintf(out, "^\n");
}

static void
yumiPrintUnhandledException(
    const char* kind,
    const char* message,
    const char* source) {
    yumiWriteUnhandledException(stderr, kind, message, source);
}

static void
yumiLogPanicFault(const char* message, const char* source, void* trace) {
#if defined(__APPLE__)
    char* text = nullptr;
    size_t size = 0;
    FILE* out = open_memstream(&text, &size);
    if (!out) {
        return;
    }
    yumiWriteUnhandledException(out, "panic", message, source);
    if (char* stack = yumiFormatStackTrace(trace)) {
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
yumiPanicSet(void* panic) {
    // A panic edge bypasses the normal compiler-emitted allocation-scope epilogue. Stop routing
    // the cleanups and handlers that run next into the parser region; the region itself remains
    // intact until the panic value has been observed and promoted.
    yumiParseRegionCancelAllocationScopes();
    yumiPublishActivePanicRoot(panic);
    yumiActivePanicContext()->activePanic = panic;
}

extern "C" void
yumiPanicClear() {
    yumiActivePanicContext()->activePanic = nullptr;
    yumiUpdateActivePanicRoot(nullptr);
}

extern "C" __attribute__((noinline)) void
yumiRaisePanic(void* panic, void** traceSlot) {
    if (traceSlot && !*traceSlot) {
        *traceSlot = yumiCaptureStackTraceSkipping(2);
    }
    yumiPanicSet(panic);
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
yumiTakePanic() {
    YuTaskRuntimeContext* context = yumiActivePanicContext();
    void* panic = context->activePanic;
    context->activePanic = nullptr;
    void* promoted = yumiParseRegionPromote(static_cast<char*>(panic));
    yumiUpdateActivePanicRoot(promoted);
    yumiResetParseRegionAfterPanic(context);
    return promoted;
}

extern "C" void*
yumiTaskFailure() {
    // The panic ending the active task, without taking it: the active one, or the one set aside
    // while a cleanup runs. It is moved out of the parse region first, as yumiTakePanic moves it,
    // since whoever reads it may keep it.
    YuTaskRuntimeContext* context = yumiActivePanicContext();
    YuPendingPanic* pending = context->activePanic ? nullptr : yumiPendingCleanupPanic(context);
    void* panic = pending ? pending->panic : context->activePanic;
    if (!panic) {
        return nullptr;
    }
    void* promoted = yumiParseRegionPromote(static_cast<char*>(panic));
    if (promoted == panic) {
        return panic;
    }
    if (pending) {
        if (pending->root) {
            yumiRemoveRoot(pending->root);
        }
        pending->panic = promoted;
        pending->root = yumiAddRoot(static_cast<char*>(promoted));
    } else {
        context->activePanic = promoted;
        yumiUpdateActivePanicRoot(promoted);
    }
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
    bool continues = yumiContinueAfterUnhandledException();
    if (!continues) {
        yumiLogPanicFault(message, source, trace);
    }

    if (yumiActivePanicContext()->taskEnd == YU_TASK_END_RUNNING) {
        yumiActivePanicContext()->taskEnd = YU_TASK_END_PANICKED;
    }
    yumiActivePanicContext()->activePanic = nullptr;
    yumiUpdateActivePanicRoot(nullptr);
    yumiResetParseRegionAfterPanic(yumiActivePanicContext());
    if (continues) {
        return;
    }

    yumiCloseProbesBeforeExit();
    fflush(stderr);
    abort();
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
    if (yumiContinueAfterUnhandledException()) {
        fflush(stderr);
        _Exit(1);
    }
    yumiLogPanicFault(message, source, trace);
    fflush(stderr);
    abort();
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
    YuTaskRuntimeContext* context = yumiActivePanicContext();
    if (context && context->taskEnd == YU_TASK_END_RUNNING) {
        context->taskEnd = YU_TASK_END_RAISED;
    }
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

namespace {

struct YuPadEntry {
    uintptr_t returnAddress;
    uintptr_t pad;
};

constexpr uint32_t kYuPadsMagic = 0x44415059;
constexpr uint32_t kYuPadsVersion = 1;

uint32_t
yumiReadPadWord(const uint8_t* data) {
    uint32_t value;
    memcpy(&value, data, sizeof(value));
    return value;
}

std::vector<YuPadEntry>*
yumiBuildPadTable() {
    std::vector<YuPadEntry>* table = new std::vector<YuPadEntry>();
    size_t size = 0;
    const uint8_t* data = static_cast<const uint8_t*>(yumiGetPadTablePointer(&size));
    size_t offset = 0;
    while (data && offset + 16 <= size) {
        uint32_t magic = yumiReadPadWord(data + offset);
        if (magic == 0) {
            offset += 8;
            continue;
        }
        uint32_t version = yumiReadPadWord(data + offset + 4) & 0xffff;
        uint32_t functionCount = yumiReadPadWord(data + offset + 8);
        uint32_t entryCount = yumiReadPadWord(data + offset + 12);
        size_t records = offset + 16;
        size_t entries = records + static_cast<size_t>(functionCount) * 16;
        size_t end = entries + static_cast<size_t>(entryCount) * 8;
        if (magic != kYuPadsMagic || version != kYuPadsVersion || end > size) {
            fprintf(stderr, "Invalid __yumi_pads section\n");
            abort();
        }
        for (uint32_t index = 0; index < functionCount; index += 1) {
            const uint8_t* record = data + records + static_cast<size_t>(index) * 16;
            uintptr_t function;
            memcpy(&function, record, sizeof(function));
            uint32_t first = yumiReadPadWord(record + 8);
            uint32_t count = yumiReadPadWord(record + 12);
            if (static_cast<uint64_t>(first) + count > entryCount) {
                fprintf(stderr, "Invalid __yumi_pads record\n");
                abort();
            }
            for (uint32_t position = first; position < first + count; position += 1) {
                const uint8_t* entry = data + entries + static_cast<size_t>(position) * 8;
                table->push_back({
                    function + yumiReadPadWord(entry),
                    function + yumiReadPadWord(entry + 4),
                });
            }
        }
        offset = end;
    }
    std::sort(table->begin(), table->end(), [](const YuPadEntry& left, const YuPadEntry& right) {
        return left.returnAddress < right.returnAddress;
    });
    return table;
}

const std::vector<YuPadEntry>&
yumiPadTable() {
    static std::once_flag once;
    static std::vector<YuPadEntry>* table = nullptr;
    std::call_once(once, [] { table = yumiBuildPadTable(); });
    return *table;
}

}

extern "C" void
yumiPanicReturn() {
    if (!yumiActivePanicContext()->activePanic) {
        return;
    }
    const std::vector<YuPadEntry>& table = yumiPadTable();
    void** frame = static_cast<void**>(*static_cast<void**>(__builtin_frame_address(0)));
    uintptr_t returnAddress = reinterpret_cast<uintptr_t>(frame[1]);
    auto found = std::lower_bound(
        table.begin(),
        table.end(),
        returnAddress,
        [](const YuPadEntry& entry, uintptr_t address) {
            return entry.returnAddress < address;
        });
    if (found != table.end() && found->returnAddress == returnAddress) {
        frame[1] = reinterpret_cast<void*>(found->pad);
    }
}
