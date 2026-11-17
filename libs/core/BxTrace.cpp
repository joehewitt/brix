#include "BxCore/BxTrace.h"

#include "BxCore/BxCore.h"
#include "BxCore/BxStackMap.h"

#include <algorithm>
#include <inttypes.h>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>


// *************************************************************************************************

extern "C" void* brixTryGetActiveTask();
extern "C" uint32_t brixGetTaskId(void* task);
extern "C" uint32_t brixGetTaskParentId(void* task);
extern "C" const char* brixGetTaskSpawnFile(void* task);
extern "C" uint32_t brixGetTaskSpawnLine(void* task);
extern "C" uint32_t brixGetTaskSpawnColumn(void* task);
extern "C" uint32_t brixGetTaskFlags(void* task);
extern "C" void* brixGetTaskStackBase(void* task);
extern "C" size_t brixGetTaskStackSize(void* task);

// *************************************************************************************************

static const uint16_t BRIX_TRACE_TRUNCATED = 1 << 0;
static const uint16_t BRIX_TRACE_PARTIAL = 1 << 1;
static const uint32_t BRIX_TASK_FLAG_SPIN = 1 << 0;
static const uint32_t BRIX_TASK_FLAG_WORKER = 1 << 1;
static const uint32_t BRIX_TRACE_METADATA_MAGIC = 0x43525459; // YTRC
static const uint16_t BRIX_TRACE_METADATA_VERSION = 2;

#pragma pack(push, 1)
struct BrixTraceMetadataHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t functionCount;
    uint32_t lineCount;
    uint32_t fileCount;
    uint32_t stringSize;
};

struct BrixTraceFunctionRecord {
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

struct BrixTraceLineRecord {
    uint32_t pcOffset;
    uint32_t fileStringOffset;
    uint32_t line;
    uint16_t column;
    uint16_t flags;
};
#pragma pack(pop)

struct BrixTraceMetadataChunk {
    const BrixTraceMetadataHeader* header;
    const BrixTraceFunctionRecord* functions;
    const BrixTraceLineRecord* lines;
    const char* strings;
    size_t stringSize;
};

struct BrixTraceFunctionLookup {
    const BrixTraceMetadataChunk* chunk;
    const BrixTraceFunctionRecord* function;
};

struct BrixTraceMetadata {
    std::once_flag initialized;
    std::vector<BrixTraceMetadataChunk> chunks;
    std::vector<BrixTraceFunctionLookup> functions;
};

static BrixTraceMetadata gTraceMetadata = {};

static uint16_t
brixTraceMaxFrames() {
    static uint16_t cachedMaxFrames = 0;
    if (cachedMaxFrames) {
        return cachedMaxFrames;
    }

    const char* value = getenv("BRIX_STACK_TRACE_MAX_FRAMES");
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
brixStackTracesEnabled() {
    static int cachedEnabled = -1;
    if (cachedEnabled >= 0) {
        return cachedEnabled != 0;
    }

    const char* value = getenv("BRIX_STACK_TRACES");
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
brixFramePointerInBounds(uintptr_t fp, uintptr_t stackBase, uintptr_t stackEnd) {
    if (!fp || (fp & 0xf) != 0) {
        return false;
    }
    if (!stackBase || !stackEnd) {
        return false;
    }
    return fp >= stackBase && fp + 16 <= stackEnd;
}

static void
brixLoadTraceMetadataLocked() {
    size_t size = 0;
    const uint8_t* ptr = (const uint8_t*)brixGetTraceMetadataPointer(&size);
    if (!ptr || size < sizeof(BrixTraceMetadataHeader)) {
        return;
    }

    const uint8_t* cursor = ptr;
    size_t remaining = size;
    while (remaining >= sizeof(BrixTraceMetadataHeader)) {
        const BrixTraceMetadataHeader* header = (const BrixTraceMetadataHeader*)cursor;
        if (header->magic != BRIX_TRACE_METADATA_MAGIC ||
            header->version != BRIX_TRACE_METADATA_VERSION) {
            ++cursor;
            --remaining;
            continue;
        }

        uint64_t recordsSize =
            (uint64_t)header->functionCount * sizeof(BrixTraceFunctionRecord) +
            (uint64_t)header->lineCount * sizeof(BrixTraceLineRecord);
        uint64_t needed = sizeof(BrixTraceMetadataHeader) + recordsSize + header->stringSize;
        if (needed > remaining) {
            break;
        }

        const uint8_t* recordCursor = cursor + sizeof(BrixTraceMetadataHeader);
        BrixTraceMetadataChunk chunk = {};
        chunk.header = header;
        chunk.functions = (const BrixTraceFunctionRecord*)recordCursor;
        recordCursor += (size_t)header->functionCount * sizeof(BrixTraceFunctionRecord);
        chunk.lines = (const BrixTraceLineRecord*)recordCursor;
        recordCursor += (size_t)header->lineCount * sizeof(BrixTraceLineRecord);
        chunk.strings = (const char*)recordCursor;
        chunk.stringSize = header->stringSize;
        gTraceMetadata.chunks.push_back(chunk);

        cursor += (size_t)needed;
        remaining -= (size_t)needed;
    }

    // Mach-O trace records are emitted in object-text order, but the linker may reorder
    // function atoms in a sufficiently large executable. Build the lookup index only after
    // relocations have resolved so address lookup never depends on linker layout policy.
    for (const BrixTraceMetadataChunk& chunk : gTraceMetadata.chunks) {
        for (uint32_t index = 0; index < chunk.header->functionCount; ++index) {
            gTraceMetadata.functions.push_back({ &chunk, &chunk.functions[index] });
        }
    }
    std::sort(
        gTraceMetadata.functions.begin(),
        gTraceMetadata.functions.end(),
        [](const BrixTraceFunctionLookup& left, const BrixTraceFunctionLookup& right) {
            return left.function->startPc < right.function->startPc;
        });
}

static void
brixLoadTraceMetadata() {
    // Concurrent panics on multiple threads can race to symbolize their first frame; a plain
    // initialized flag let the second thread observe an empty chunk list mid-load.
    std::call_once(gTraceMetadata.initialized, brixLoadTraceMetadataLocked);
}

static const char*
brixTraceMetadataString(const BrixTraceMetadataChunk* chunk, uint32_t offset) {
    if (!chunk || offset >= chunk->stringSize) {
        return "";
    }

    const char* str = chunk->strings + offset;
    size_t remaining = chunk->stringSize - offset;
    return memchr(str, 0, remaining) ? str : "";
}

static BrixTraceFunctionLookup
brixFindTraceFunction(uintptr_t pc) {
    brixLoadTraceMetadata();

    size_t left = 0;
    size_t right = gTraceMetadata.functions.size();
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        const BrixTraceFunctionLookup& lookup = gTraceMetadata.functions[mid];
        const BrixTraceFunctionRecord* record = lookup.function;
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

static const BrixTraceLineRecord*
brixFindTraceLine(
    const BrixTraceMetadataChunk* chunk,
    const BrixTraceFunctionRecord* function,
    uintptr_t pc) {
    if (!chunk || !function || !function->lineRecordCount) {
        return nullptr;
    }

    uintptr_t functionStart = (uintptr_t)function->startPc;
    uint32_t pcOffset = pc > functionStart ? (uint32_t)(pc - functionStart) : 0;
    uint32_t left = function->firstLineRecord;
    uint32_t right = left + function->lineRecordCount;
    const BrixTraceLineRecord* best = nullptr;

    while (left < right) {
        uint32_t mid = left + (right - left) / 2;
        const BrixTraceLineRecord* record = &chunk->lines[mid];
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
brixPrintTraceLocation(FILE* out, const char* file, uint32_t line, uint32_t column) {
    if (!line) {
        fprintf(out, " no file\n");
    } else if (file && file[0]) {
        fprintf(out, " %s:%u:%u\n", file, line, column);
    } else {
        fprintf(out, " no file:%u:%u\n", line, column);
    }
}

static bool
brixPrintSymbolizedFrame(FILE* out, uint16_t index, uintptr_t pc) {
    BrixTraceFunctionLookup lookup = brixFindTraceFunction(pc);
    const BrixTraceMetadataChunk* chunk = lookup.chunk;
    const BrixTraceFunctionRecord* record = lookup.function;
    if (!record) {
        return false;
    }

    const char* name = brixTraceMetadataString(chunk, record->nameStringOffset);
    if (!name[0]) {
        name = "<unknown>";
    }

    fprintf(out, "  %u  %s", index, name);

    const BrixTraceLineRecord* line = brixFindTraceLine(chunk, record, pc);
    if (line && line->line) {
        const char* file = brixTraceMetadataString(chunk, line->fileStringOffset);
        brixPrintTraceLocation(out, file, line->line, line->column);
    } else {
        const char* file = brixTraceMetadataString(chunk, record->fileStringOffset);
        brixPrintTraceLocation(out, file, record->declLine, record->declColumn);
    }

    return true;
}

static void
brixPrintTraceTask(
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
    if (taskFlags & BRIX_TASK_FLAG_WORKER) {
        fprintf(out, " (worker)");
    } else if (taskFlags & BRIX_TASK_FLAG_SPIN) {
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
brixCaptureStackTraceSkipping(uint16_t skipFrames) {
    if (!brixStackTracesEnabled()) {
        return nullptr;
    }

    const uint16_t maxFrames = brixTraceMaxFrames();
    uint16_t frameCount = 0;
    uint16_t flags = 0;
    uint32_t taskId = 0;
    uint32_t parentTaskId = 0;
    const char* spawnFile = nullptr;
    uint32_t spawnLine = 0;
    uint32_t spawnColumn = 0;
    uint32_t taskFlags = 0;
    BrixTraceFrame frames[256];

    void* task = brixTryGetActiveTask();
    if (!task) {
        flags |= BRIX_TRACE_PARTIAL;
    } else {
        taskId = brixGetTaskId(task);
        parentTaskId = brixGetTaskParentId(task);
        spawnFile = brixGetTaskSpawnFile(task);
        spawnLine = brixGetTaskSpawnLine(task);
        spawnColumn = brixGetTaskSpawnColumn(task);
        taskFlags = brixGetTaskFlags(task);

        uintptr_t stackBase = (uintptr_t)brixGetTaskStackBase(task);
        uintptr_t stackEnd = stackBase + (uintptr_t)brixGetTaskStackSize(task);

#if defined(__aarch64__)
        uintptr_t fp = (uintptr_t)__builtin_frame_address(0);

        while (frameCount < maxFrames) {
            if (!brixFramePointerInBounds(fp, stackBase, stackEnd)) {
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

            if (!brixFramePointerInBounds(nextFp, stackBase, stackEnd)) {
                break;
            }
            if (nextFp <= fp) {
                break;
            }

            fp = nextFp;
        }

        if (frameCount == maxFrames && brixFramePointerInBounds(fp, stackBase, stackEnd)) {
            flags |= BRIX_TRACE_TRUNCATED;
        }
#else
        flags |= BRIX_TRACE_PARTIAL;
#endif
    }

    const size_t allocSize = sizeof(BrixStackTrace) + sizeof(BrixTraceFrame) * frameCount;
    BrixStackTrace* trace = (BrixStackTrace*)BX_MALLOC_ATOMIC(allocSize);
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
        memcpy(trace->frames, frames, sizeof(BrixTraceFrame) * frameCount);
    }
    return trace;
}

extern "C" void*
brixCaptureStackTrace() {
    return brixCaptureStackTraceSkipping(0);
}

extern "C" void
brixEnsureStackTrace(void** traceSlot) {
    if (!traceSlot || *traceSlot) {
        return;
    }

    *traceSlot = brixCaptureStackTraceSkipping(1);
}

static void
brixWriteStackTrace(FILE* out, BrixStackTrace* trace) {
    brixPrintTraceTask(
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
        if (brixPrintSymbolizedFrame(out, i, pc)) {
            printedSymbolizedFrame = true;
            continue;
        }

        if (printedSymbolizedFrame && !gTraceMetadata.chunks.empty()) {
            break;
        }

        fprintf(out, "  %u  unsymbolized frame 0x%016" PRIxPTR "\n", i, pc);
    }

    if (trace->flags & BRIX_TRACE_TRUNCATED) {
        fprintf(out, "  ... truncated\n");
    }
}

extern "C" void
brixPrintStackTrace(void* tracePtr) {
    if (!tracePtr) {
        return;
    }
    brixWriteStackTrace(stderr, (BrixStackTrace*)tracePtr);
}

extern "C" char*
brixFormatStackTrace(void* tracePtr) {
    if (!tracePtr) {
        return nullptr;
    }
    char* text = nullptr;
    size_t size = 0;
    FILE* out = open_memstream(&text, &size);
    if (!out) {
        return nullptr;
    }
    brixWriteStackTrace(out, (BrixStackTrace*)tracePtr);
    fclose(out);
    return text;
}

extern "C" double
brixStackTraceTime(void* tracePtr) {
    return tracePtr ? ((BrixStackTrace*)tracePtr)->secondsSinceEpoch : 0;
}

// *************************************************************************************************

struct BrixOverflowFrame {
    uintptr_t pc;
    bool repeated;
};

struct BrixOverflowTrace {
    uint16_t edge;
    uint16_t headCount;
    uint64_t tailCount;
    BrixOverflowFrame head[128];
    BrixOverflowFrame tail[128];

    BrixOverflowFrame* latest() {
        if (tailCount) {
            return &tail[(tailCount - 1) % edge];
        }
        return headCount ? &head[headCount - 1] : nullptr;
    }

    void add(uintptr_t pc) {
        BrixOverflowFrame* last = latest();
        if (last && last->pc == pc) {
            last->repeated = true;
        } else if (headCount < edge) {
            head[headCount++] = { pc, false };
        } else {
            tail[tailCount++ % edge] = { pc, false };
        }
    }
};

struct BrixOverflowPrinter {
    uint16_t index;
    bool printedSymbolizedFrame;

    bool print(const BrixOverflowFrame& frame) {
        if (brixPrintSymbolizedFrame(stderr, index, frame.pc)) {
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
brixReportTaskHeader(const char* heading, void* task) {
    fprintf(stderr, "\n%s\n", heading);
    brixPrintTraceTask(
        stderr,
        brixGetTaskId(task),
        brixGetTaskParentId(task),
        brixGetTaskFlags(task),
        brixGetTaskSpawnFile(task),
        brixGetTaskSpawnLine(task),
        brixGetTaskSpawnColumn(task));
    fflush(stderr);
}

extern "C" void
brixReportTaskStack(const char* heading, void* task, uintptr_t fp, uintptr_t pc) {
    fprintf(stderr, "\n%s\n", heading);
    if (!brixStackTracesEnabled()) {
        fflush(stderr);
        return;
    }

    brixPrintTraceTask(
        stderr,
        brixGetTaskId(task),
        brixGetTaskParentId(task),
        brixGetTaskFlags(task),
        brixGetTaskSpawnFile(task),
        brixGetTaskSpawnLine(task),
        brixGetTaskSpawnColumn(task));
    fprintf(stderr, "\nStack trace:\n");

    BrixOverflowTrace trace;
    trace.edge = std::max<uint16_t>(1, brixTraceMaxFrames() / 2);
    trace.headCount = 0;
    trace.tailCount = 0;
    trace.add(pc);

    uintptr_t stackBase = (uintptr_t)brixGetTaskStackBase(task);
    uintptr_t stackEnd = stackBase + (uintptr_t)brixGetTaskStackSize(task);
    while (brixFramePointerInBounds(fp, stackBase, stackEnd)) {
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

    BrixOverflowPrinter printer = { 0, false };
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
