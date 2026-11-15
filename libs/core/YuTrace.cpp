#include "YuCore/YuTrace.h"

#include "YuCore/YuCore.h"
#include "YuCore/YuStackMap.h"

#include <algorithm>
#include <inttypes.h>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>


// *************************************************************************************************

extern "C" void* yumiTryGetActiveTask();
extern "C" uint32_t yumiGetTaskId(void* task);
extern "C" uint32_t yumiGetTaskParentId(void* task);
extern "C" const char* yumiGetTaskSpawnFile(void* task);
extern "C" uint32_t yumiGetTaskSpawnLine(void* task);
extern "C" uint32_t yumiGetTaskSpawnColumn(void* task);
extern "C" uint32_t yumiGetTaskFlags(void* task);
extern "C" void* yumiGetTaskStackBase(void* task);
extern "C" size_t yumiGetTaskStackSize(void* task);

// *************************************************************************************************

static const uint16_t YUMI_TRACE_TRUNCATED = 1 << 0;
static const uint16_t YUMI_TRACE_PARTIAL = 1 << 1;
static const uint32_t YUMI_TASK_FLAG_SPIN = 1 << 0;
static const uint32_t YUMI_TASK_FLAG_WORKER = 1 << 1;
static const uint32_t YUMI_TRACE_METADATA_MAGIC = 0x43525459; // YTRC
static const uint16_t YUMI_TRACE_METADATA_VERSION = 2;

#pragma pack(push, 1)
struct YumiTraceMetadataHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t functionCount;
    uint32_t lineCount;
    uint32_t fileCount;
    uint32_t stringSize;
};

struct YumiTraceFunctionRecord {
    uint64_t startPc;
    uint32_t byteSize;
    uint32_t nameStringOffset;
    uint32_t fileStringOffset;
    uint32_t declLine;
    uint32_t declColumn;
    uint32_t firstLineRecord;
    uint32_t lineRecordCount;
    uint32_t reserved;
};

struct YumiTraceLineRecord {
    uint32_t pcOffset;
    uint32_t fileStringOffset;
    uint32_t line;
    uint16_t column;
    uint16_t flags;
};
#pragma pack(pop)

struct YumiTraceMetadataChunk {
    const YumiTraceMetadataHeader* header;
    const YumiTraceFunctionRecord* functions;
    const YumiTraceLineRecord* lines;
    const char* strings;
    size_t stringSize;
};

struct YumiTraceFunctionLookup {
    const YumiTraceMetadataChunk* chunk;
    const YumiTraceFunctionRecord* function;
};

struct YumiTraceMetadata {
    std::once_flag initialized;
    std::vector<YumiTraceMetadataChunk> chunks;
    std::vector<YumiTraceFunctionLookup> functions;
};

static YumiTraceMetadata gTraceMetadata = {};

static uint16_t
yumiTraceMaxFrames() {
    static uint16_t cachedMaxFrames = 0;
    if (cachedMaxFrames) {
        return cachedMaxFrames;
    }

    const char* value = getenv("YUMI_STACK_TRACE_MAX_FRAMES");
    long maxFrames = value ? strtol(value, nullptr, 10) : 64;
    if (maxFrames < 1) {
        maxFrames = 1;
    } else if (maxFrames > 256) {
        maxFrames = 256;
    }

    cachedMaxFrames = (uint16_t)maxFrames;
    return cachedMaxFrames;
}

static bool
yumiStackTracesEnabled() {
    static int cachedEnabled = -1;
    if (cachedEnabled >= 0) {
        return cachedEnabled != 0;
    }

    const char* value = getenv("YUMI_STACK_TRACES");
    cachedEnabled =
        !value ||
        (strcmp(value, "0") != 0 &&
         strcmp(value, "false") != 0 &&
         strcmp(value, "FALSE") != 0 &&
         strcmp(value, "off") != 0 &&
         strcmp(value, "OFF") != 0 &&
         strcmp(value, "no") != 0 &&
         strcmp(value, "NO") != 0);
    return cachedEnabled != 0;
}

static bool
yumiFramePointerInBounds(uintptr_t fp, uintptr_t stackBase, uintptr_t stackEnd) {
    if (!fp || (fp & 0xf) != 0) {
        return false;
    }
    if (!stackBase || !stackEnd) {
        return false;
    }
    return fp >= stackBase && fp + 16 <= stackEnd;
}

static void
yumiLoadTraceMetadataLocked() {
    size_t size = 0;
    const uint8_t* ptr = (const uint8_t*)yumiGetTraceMetadataPointer(&size);
    if (!ptr || size < sizeof(YumiTraceMetadataHeader)) {
        return;
    }

    const uint8_t* cursor = ptr;
    size_t remaining = size;
    while (remaining >= sizeof(YumiTraceMetadataHeader)) {
        const YumiTraceMetadataHeader* header = (const YumiTraceMetadataHeader*)cursor;
        if (header->magic != YUMI_TRACE_METADATA_MAGIC ||
            header->version != YUMI_TRACE_METADATA_VERSION) {
            ++cursor;
            --remaining;
            continue;
        }

        uint64_t recordsSize =
            (uint64_t)header->functionCount * sizeof(YumiTraceFunctionRecord) +
            (uint64_t)header->lineCount * sizeof(YumiTraceLineRecord);
        uint64_t needed = sizeof(YumiTraceMetadataHeader) + recordsSize + header->stringSize;
        if (needed > remaining) {
            break;
        }

        const uint8_t* recordCursor = cursor + sizeof(YumiTraceMetadataHeader);
        YumiTraceMetadataChunk chunk = {};
        chunk.header = header;
        chunk.functions = (const YumiTraceFunctionRecord*)recordCursor;
        recordCursor += (size_t)header->functionCount * sizeof(YumiTraceFunctionRecord);
        chunk.lines = (const YumiTraceLineRecord*)recordCursor;
        recordCursor += (size_t)header->lineCount * sizeof(YumiTraceLineRecord);
        chunk.strings = (const char*)recordCursor;
        chunk.stringSize = header->stringSize;
        gTraceMetadata.chunks.push_back(chunk);

        cursor += (size_t)needed;
        remaining -= (size_t)needed;
    }

    // Mach-O trace records are emitted in object-text order, but the linker may reorder
    // function atoms in a sufficiently large executable. Build the lookup index only after
    // relocations have resolved so address lookup never depends on linker layout policy.
    for (const YumiTraceMetadataChunk& chunk : gTraceMetadata.chunks) {
        for (uint32_t index = 0; index < chunk.header->functionCount; ++index) {
            gTraceMetadata.functions.push_back({ &chunk, &chunk.functions[index] });
        }
    }
    std::sort(
        gTraceMetadata.functions.begin(),
        gTraceMetadata.functions.end(),
        [](const YumiTraceFunctionLookup& left, const YumiTraceFunctionLookup& right) {
            return left.function->startPc < right.function->startPc;
        });
}

static void
yumiLoadTraceMetadata() {
    // Concurrent panics on multiple threads can race to symbolize their first frame; a plain
    // initialized flag let the second thread observe an empty chunk list mid-load.
    std::call_once(gTraceMetadata.initialized, yumiLoadTraceMetadataLocked);
}

static const char*
yumiTraceMetadataString(const YumiTraceMetadataChunk* chunk, uint32_t offset) {
    if (!chunk || offset >= chunk->stringSize) {
        return "";
    }

    const char* str = chunk->strings + offset;
    size_t remaining = chunk->stringSize - offset;
    return memchr(str, 0, remaining) ? str : "";
}

static YumiTraceFunctionLookup
yumiFindTraceFunction(uintptr_t pc) {
    yumiLoadTraceMetadata();

    size_t left = 0;
    size_t right = gTraceMetadata.functions.size();
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        const YumiTraceFunctionLookup& lookup = gTraceMetadata.functions[mid];
        const YumiTraceFunctionRecord* record = lookup.function;
        uintptr_t start = (uintptr_t)record->startPc;
        if (pc < start) {
            right = mid;
        } else if (pc >= start + record->byteSize) {
            left = mid + 1;
        } else {
            return lookup;
        }
    }

    return {};
}

static const YumiTraceLineRecord*
yumiFindTraceLine(
    const YumiTraceMetadataChunk* chunk,
    const YumiTraceFunctionRecord* function,
    uintptr_t pc) {
    if (!chunk || !function || !function->lineRecordCount) {
        return nullptr;
    }

    uintptr_t functionStart = (uintptr_t)function->startPc;
    uint32_t pcOffset = pc > functionStart ? (uint32_t)(pc - functionStart) : 0;
    uint32_t left = function->firstLineRecord;
    uint32_t right = left + function->lineRecordCount;
    const YumiTraceLineRecord* best = nullptr;

    while (left < right) {
        uint32_t mid = left + (right - left) / 2;
        const YumiTraceLineRecord* record = &chunk->lines[mid];
        if (record->pcOffset <= pcOffset) {
            best = record;
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    return best;
}

static void
yumiPrintTraceLocation(FILE* out, const char* file, uint32_t line, uint32_t column) {
    if (!line) {
        fprintf(out, " no file\n");
    } else if (file && file[0]) {
        fprintf(out, " %s:%u:%u\n", file, line, column);
    } else {
        fprintf(out, " no file:%u:%u\n", line, column);
    }
}

static bool
yumiPrintSymbolizedFrame(FILE* out, uint16_t index, uintptr_t pc) {
    YumiTraceFunctionLookup lookup = yumiFindTraceFunction(pc);
    const YumiTraceMetadataChunk* chunk = lookup.chunk;
    const YumiTraceFunctionRecord* record = lookup.function;
    if (!record) {
        return false;
    }

    const char* name = yumiTraceMetadataString(chunk, record->nameStringOffset);
    if (!name[0]) {
        name = "<unknown>";
    }

    fprintf(out, "  %u  %s", index, name);

    const YumiTraceLineRecord* line = yumiFindTraceLine(chunk, record, pc);
    if (line && line->line) {
        const char* file = yumiTraceMetadataString(chunk, line->fileStringOffset);
        yumiPrintTraceLocation(out, file, line->line, line->column);
    } else {
        const char* file = yumiTraceMetadataString(chunk, record->fileStringOffset);
        yumiPrintTraceLocation(out, file, record->declLine, record->declColumn);
    }

    return true;
}

static void
yumiPrintTraceTask(
    FILE* out,
    uint32_t taskId,
    uint32_t parentTaskId,
    uint32_t taskFlags,
    const char* spawnFile,
    uint32_t spawnLine,
    uint32_t spawnColumn) {
    if (!taskId) {
        return;
    }

    fprintf(out, "\nTask %u", taskId);
    if (taskFlags & YUMI_TASK_FLAG_WORKER) {
        fprintf(out, " (worker)");
    } else if (taskFlags & YUMI_TASK_FLAG_SPIN) {
        fprintf(out, " (thread)");
    }
    if (parentTaskId) {
        fprintf(out, ", spawned by task %u", parentTaskId);
    }
    if (spawnLine) {
        fprintf(
            out,
            " from %s:%u:%u",
            spawnFile && spawnFile[0] ? spawnFile : "no file",
            spawnLine,
            spawnColumn);
    }
    fprintf(out, "\n");
}

// *************************************************************************************************

extern "C" void*
yumiCaptureStackTraceSkipping(uint16_t skipFrames) {
    if (!yumiStackTracesEnabled()) {
        return nullptr;
    }

    const uint16_t maxFrames = yumiTraceMaxFrames();
    uint16_t frameCount = 0;
    uint16_t flags = 0;
    uint32_t taskId = 0;
    uint32_t parentTaskId = 0;
    const char* spawnFile = nullptr;
    uint32_t spawnLine = 0;
    uint32_t spawnColumn = 0;
    uint32_t taskFlags = 0;
    YumiTraceFrame frames[256];

    void* task = yumiTryGetActiveTask();
    if (!task) {
        flags |= YUMI_TRACE_PARTIAL;
    } else {
        taskId = yumiGetTaskId(task);
        parentTaskId = yumiGetTaskParentId(task);
        spawnFile = yumiGetTaskSpawnFile(task);
        spawnLine = yumiGetTaskSpawnLine(task);
        spawnColumn = yumiGetTaskSpawnColumn(task);
        taskFlags = yumiGetTaskFlags(task);

        uintptr_t stackBase = (uintptr_t)yumiGetTaskStackBase(task);
        uintptr_t stackEnd = stackBase + (uintptr_t)yumiGetTaskStackSize(task);

#if defined(__aarch64__)
        uintptr_t fp = (uintptr_t)__builtin_frame_address(0);

        while (frameCount < maxFrames) {
            if (!yumiFramePointerInBounds(fp, stackBase, stackEnd)) {
                break;
            }

            uintptr_t* frameRecord = (uintptr_t*)fp;
            uintptr_t nextFp = frameRecord[0];
            uintptr_t lr = frameRecord[1];
            if (!lr) {
                break;
            }

            if (skipFrames) {
                skipFrames--;
            } else {
                frames[frameCount++].pc = lr >= 4 ? lr - 4 : lr;
            }

            if (!yumiFramePointerInBounds(nextFp, stackBase, stackEnd)) {
                break;
            }
            if (nextFp <= fp) {
                break;
            }

            fp = nextFp;
        }

        if (frameCount == maxFrames && yumiFramePointerInBounds(fp, stackBase, stackEnd)) {
            flags |= YUMI_TRACE_TRUNCATED;
        }
#else
        flags |= YUMI_TRACE_PARTIAL;
#endif
    }

    const size_t allocSize = sizeof(YumiStackTrace) + sizeof(YumiTraceFrame) * frameCount;
    YumiStackTrace* trace = (YumiStackTrace*)YU_MALLOC_ATOMIC(allocSize);
    memset(trace, 0, allocSize);
    trace->frameCount = frameCount;
    trace->flags = flags;
    trace->maxFrames = maxFrames;
    trace->taskId = taskId;
    trace->parentTaskId = parentTaskId;
    trace->spawnFile = spawnFile;
    trace->spawnLine = spawnLine;
    trace->spawnColumn = spawnColumn;
    trace->taskFlags = taskFlags;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    trace->secondsSinceEpoch = (double)now.tv_sec + (double)now.tv_nsec / 1e9;
    if (frameCount) {
        memcpy(trace->frames, frames, sizeof(YumiTraceFrame) * frameCount);
    }
    return trace;
}

extern "C" void*
yumiCaptureStackTrace() {
    return yumiCaptureStackTraceSkipping(0);
}

extern "C" void
yumiEnsureStackTrace(void** traceSlot) {
    if (!traceSlot || *traceSlot) {
        return;
    }

    *traceSlot = yumiCaptureStackTraceSkipping(1);
}

static void
yumiWriteStackTrace(FILE* out, YumiStackTrace* trace) {
    yumiPrintTraceTask(
        out,
        trace->taskId,
        trace->parentTaskId,
        trace->taskFlags,
        trace->spawnFile,
        trace->spawnLine,
        trace->spawnColumn);

    fprintf(out, "\nStack trace:\n");
    if (!trace->frameCount) {
        fprintf(out, "  <no frames>\n");
        return;
    }

    bool printedSymbolizedFrame = false;
    for (uint16_t i = 0; i < trace->frameCount; ++i) {
        uintptr_t pc = trace->frames[i].pc;
        if (yumiPrintSymbolizedFrame(out, i, pc)) {
            printedSymbolizedFrame = true;
            continue;
        }

        if (printedSymbolizedFrame && !gTraceMetadata.chunks.empty()) {
            break;
        }

        fprintf(out, "  %u  unsymbolized frame 0x%016" PRIxPTR "\n", i, pc);
    }

    if (trace->flags & YUMI_TRACE_TRUNCATED) {
        fprintf(out, "  ... truncated\n");
    }
}

extern "C" void
yumiPrintStackTrace(void* tracePtr) {
    if (!tracePtr) {
        return;
    }
    yumiWriteStackTrace(stderr, (YumiStackTrace*)tracePtr);
}

extern "C" char*
yumiFormatStackTrace(void* tracePtr) {
    if (!tracePtr) {
        return nullptr;
    }
    char* text = nullptr;
    size_t size = 0;
    FILE* out = open_memstream(&text, &size);
    if (!out) {
        return nullptr;
    }
    yumiWriteStackTrace(out, (YumiStackTrace*)tracePtr);
    fclose(out);
    return text;
}

extern "C" double
yumiStackTraceTime(void* tracePtr) {
    return tracePtr ? ((YumiStackTrace*)tracePtr)->secondsSinceEpoch : 0;
}

// *************************************************************************************************

struct YumiOverflowFrame {
    uintptr_t pc;
    bool repeated;
};

struct YumiOverflowTrace {
    uint16_t edge;
    uint16_t headCount;
    uint64_t tailCount;
    YumiOverflowFrame head[128];
    YumiOverflowFrame tail[128];

    YumiOverflowFrame* latest() {
        if (tailCount) {
            return &tail[(tailCount - 1) % edge];
        }
        return headCount ? &head[headCount - 1] : nullptr;
    }

    void add(uintptr_t pc) {
        YumiOverflowFrame* last = latest();
        if (last && last->pc == pc) {
            last->repeated = true;
        } else if (headCount < edge) {
            head[headCount++] = { pc, false };
        } else {
            tail[tailCount++ % edge] = { pc, false };
        }
    }
};

struct YumiOverflowPrinter {
    uint16_t index;
    bool printedSymbolizedFrame;

    bool print(const YumiOverflowFrame& frame) {
        if (yumiPrintSymbolizedFrame(stderr, index, frame.pc)) {
            printedSymbolizedFrame = true;
        } else if (printedSymbolizedFrame && !gTraceMetadata.chunks.empty()) {
            return false;
        } else {
            fprintf(stderr, "  %u  unsymbolized frame 0x%016" PRIxPTR "\n", index, frame.pc);
        }
        index++;
        if (frame.repeated) {
            fprintf(stderr, "  ... recursive calls\n");
        }
        return true;
    }
};

extern "C" void
yumiReportTaskHeader(const char* heading, void* task) {
    fprintf(stderr, "\n%s\n", heading);
    yumiPrintTraceTask(
        stderr,
        yumiGetTaskId(task),
        yumiGetTaskParentId(task),
        yumiGetTaskFlags(task),
        yumiGetTaskSpawnFile(task),
        yumiGetTaskSpawnLine(task),
        yumiGetTaskSpawnColumn(task));
    fflush(stderr);
}

extern "C" void
yumiReportTaskStack(const char* heading, void* task, uintptr_t fp, uintptr_t pc) {
    fprintf(stderr, "\n%s\n", heading);
    if (!yumiStackTracesEnabled()) {
        fflush(stderr);
        return;
    }

    yumiPrintTraceTask(
        stderr,
        yumiGetTaskId(task),
        yumiGetTaskParentId(task),
        yumiGetTaskFlags(task),
        yumiGetTaskSpawnFile(task),
        yumiGetTaskSpawnLine(task),
        yumiGetTaskSpawnColumn(task));
    fprintf(stderr, "\nStack trace:\n");

    YumiOverflowTrace trace;
    trace.edge = std::max<uint16_t>(1, yumiTraceMaxFrames() / 2);
    trace.headCount = 0;
    trace.tailCount = 0;
    trace.add(pc);

    uintptr_t stackBase = (uintptr_t)yumiGetTaskStackBase(task);
    uintptr_t stackEnd = stackBase + (uintptr_t)yumiGetTaskStackSize(task);
    while (yumiFramePointerInBounds(fp, stackBase, stackEnd)) {
        uintptr_t* frameRecord = (uintptr_t*)fp;
        uintptr_t nextFp = frameRecord[0];
        uintptr_t lr = frameRecord[1];
        if (!lr) {
            break;
        }
        trace.add(lr >= 4 ? lr - 4 : lr);
        if (nextFp <= fp) {
            break;
        }
        fp = nextFp;
    }

    YumiOverflowPrinter printer = { 0, false };
    for (uint16_t i = 0; i < trace.headCount; ++i) {
        if (!printer.print(trace.head[i])) {
            fflush(stderr);
            return;
        }
    }

    uint64_t first = trace.tailCount > trace.edge ? trace.tailCount - trace.edge : 0;
    if (first) {
        fprintf(stderr, "  ... %llu frames omitted\n", (unsigned long long)first);
    }
    for (uint64_t i = first; i < trace.tailCount; ++i) {
        if (!printer.print(trace.tail[i % trace.edge])) {
            break;
        }
    }
    fflush(stderr);
}
