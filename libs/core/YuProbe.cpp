#include "YuCore/YuProbe.h"
#include "YuEvents/YuTaskRuntimeContext.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <new>
#include <unordered_map>
#include <vector>

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// *************************************************************************************************

extern "C" YuTaskRuntimeContext* yumiGetActiveTaskRuntimeContext();

static const uint32_t YUMI_PROBE_FILE_MAGIC = 0x32504d59;
static const uint32_t YUMI_PROBE_SEGMENT_MAGIC = 0x32475350;
static const uint32_t YUMI_PROBE_TRANSPORT_VERSION = 2;

static const size_t YUMI_PROBE_FILE_HEADER_SIZE = 4096;
static const size_t YUMI_PROBE_SEGMENT_HEADER_SIZE = 64;
static const size_t YUMI_PROBE_RECORD_HEADER_SIZE = 64;
static const size_t YUMI_PROBE_RECORD_ALIGNMENT = 8;

static const size_t YUMI_PROBE_FILE_MAGIC_OFFSET = 0;
static const size_t YUMI_PROBE_FILE_VERSION_OFFSET = 4;
static const size_t YUMI_PROBE_FILE_HEADER_SIZE_OFFSET = 8;
static const size_t YUMI_PROBE_FILE_SEGMENT_SIZE_OFFSET = 16;
static const size_t YUMI_PROBE_FILE_SEGMENT_COUNT_OFFSET = 24;
static const size_t YUMI_PROBE_FILE_ACTIVE_SEGMENT_OFFSET = 32;
static const size_t YUMI_PROBE_FILE_RECORD_HEADER_SIZE_OFFSET = 40;
static const size_t YUMI_PROBE_FILE_DONE_OFFSET = 48;
static const size_t YUMI_PROBE_FILE_NEXT_SEQUENCE_OFFSET = 56;
static const size_t YUMI_PROBE_FILE_TOTAL_RECORDS_OFFSET = 72;
static const size_t YUMI_PROBE_FILE_READY_OFFSET = 80;

static const size_t YUMI_PROBE_SEGMENT_MAGIC_OFFSET = 0;
static const size_t YUMI_PROBE_SEGMENT_HEADER_SIZE_OFFSET = 4;
static const size_t YUMI_PROBE_SEGMENT_SIZE_OFFSET = 8;
static const size_t YUMI_PROBE_SEGMENT_RESERVE_CURSOR_OFFSET = 16;
static const size_t YUMI_PROBE_SEGMENT_SEALED_CURSOR_OFFSET = 24;
static const size_t YUMI_PROBE_SEGMENT_INDEX_OFFSET = 32;

static const size_t YUMI_PROBE_RECORD_STATE_OFFSET = 0;
static const size_t YUMI_PROBE_RECORD_HEADER_SIZE_OFFSET = 4;
static const size_t YUMI_PROBE_RECORD_LENGTH_OFFSET = 8;
static const size_t YUMI_PROBE_RECORD_PAYLOAD_LENGTH_OFFSET = 16;
static const size_t YUMI_PROBE_RECORD_SEQUENCE_OFFSET = 24;
static const size_t YUMI_PROBE_RECORD_TIME_SEC_OFFSET = 32;
static const size_t YUMI_PROBE_RECORD_TIME_NSEC_OFFSET = 40;
static const size_t YUMI_PROBE_RECORD_TASK_ID_OFFSET = 48;
static const size_t YUMI_PROBE_RECORD_THREAD_ID_OFFSET = 56;

static const uint32_t YUMI_PROBE_RECORD_STATE_WRITING = 1;
static const uint32_t YUMI_PROBE_RECORD_STATE_COMMITTED = 2;

typedef struct {
    pthread_mutex_t lock;
    int fd;
    char* header;
    size_t headerSize;
    size_t segmentSize;
    size_t taskBufferSize;
    size_t flushThreshold;
    uint64_t flushIntervalNs;
    uint64_t segmentTableCapacity;
    uint64_t activeSegment;
    uint64_t nextSequence;
    uint64_t totalRecords;
    std::vector<char*> segments;
} YumiProbeTransport;

static YumiProbeTransport gProbeTransport = {
    PTHREAD_MUTEX_INITIALIZER,
    -1,
    NULL,
    YUMI_PROBE_FILE_HEADER_SIZE,
    67108864,
    65536,
    16384,
    16000000,
    65536,
    0,
    1,
    0,
    std::vector<char*>()
};

static uint32_t gProbeOpen = 0;
extern "C" unsigned char yumiProbeTracing = 0;

static const size_t YUMI_PROBE_CACHE_LINE_SIZE = 128;

static size_t gProbeCounterSlotCount = 0;
static size_t gProbeProfileSlotCount = 0;

struct YumiProbeObjectCounter {
    uint16_t id;
    uint16_t classType;
    uint32_t objectId;
    uint64_t value;
};

struct YumiProbeProfileTimerTotals {
    uint64_t count;
    uint64_t totalNs;
    uint64_t activeCount;
    uint64_t activeStartSum;
};

struct YumiProbeThreadCounters {
    uint64_t sequence;
    uint8_t objectCounterLock;
    size_t counterSlotCount;
    size_t profileSlotCount;
    uint64_t* counters;
    uint8_t* touched;
    uint64_t* profileCounters;
    YumiProbeProfileTimerTotals* timers;
    std::unordered_map<uint64_t, YumiProbeObjectCounter> objectCounters;
};

struct YumiProbeThreadCountersOwner {
    YumiProbeThreadCounters* counters;
    ~YumiProbeThreadCountersOwner();
};

static pthread_mutex_t gProbeThreadCountersLock = PTHREAD_MUTEX_INITIALIZER;
static std::vector<YumiProbeThreadCounters*>& gProbeThreadCounters =
    *new std::vector<YumiProbeThreadCounters*>();
static YumiProbeThreadCounters* gProbeRetiredCounters = NULL;
static std::vector<YumiProbeObjectCounter>& gProbeObjectCounterSnapshot =
    *new std::vector<YumiProbeObjectCounter>();
static thread_local YumiProbeThreadCounters* gProbeActiveThreadCounters = NULL;
static thread_local YumiProbeThreadCountersOwner gProbeThreadCountersOwner;

static void*
yumiProbeAllocZeroed(size_t bytes) {
    size_t size = bytes ? bytes : 1;
    size_t rem = size % YUMI_PROBE_CACHE_LINE_SIZE;
    if (rem) {
        size += YUMI_PROBE_CACHE_LINE_SIZE - rem;
    }
    void* memory = NULL;
    if (posix_memalign(&memory, YUMI_PROBE_CACHE_LINE_SIZE, size) != 0) {
        fprintf(stderr, "Unable to allocate probe counters\n");
        abort();
    }
    memset(memory, 0, size);
    return memory;
}

static YumiProbeThreadCounters*
yumiProbeThreadCountersCreate() {
    void* memory = yumiProbeAllocZeroed(sizeof(YumiProbeThreadCounters));
    return new (memory) YumiProbeThreadCounters();
}

static void
yumiProbeThreadCountersDestroy(YumiProbeThreadCounters* counters) {
    free(counters->counters);
    free(counters->touched);
    free(counters->profileCounters);
    free(counters->timers);
    counters->~YumiProbeThreadCounters();
    free(counters);
}

static void
yumiProbeCounterSlotsGrowLocked(YumiProbeThreadCounters* counters, size_t slotCount) {
    if (slotCount <= counters->counterSlotCount) {
        return;
    }
    uint64_t* values = static_cast<uint64_t*>(yumiProbeAllocZeroed(slotCount * sizeof(uint64_t)));
    uint8_t* touched = static_cast<uint8_t*>(yumiProbeAllocZeroed(slotCount * sizeof(uint8_t)));
    if (counters->counterSlotCount) {
        memcpy(values, counters->counters, counters->counterSlotCount * sizeof(uint64_t));
        memcpy(touched, counters->touched, counters->counterSlotCount * sizeof(uint8_t));
    }
    free(counters->counters);
    free(counters->touched);
    counters->counters = values;
    counters->touched = touched;
    counters->counterSlotCount = slotCount;
}

static void
yumiProbeProfileSlotsGrowLocked(YumiProbeThreadCounters* counters, size_t slotCount) {
    if (slotCount <= counters->profileSlotCount) {
        return;
    }
    uint64_t* values = static_cast<uint64_t*>(yumiProbeAllocZeroed(slotCount * sizeof(uint64_t)));
    YumiProbeProfileTimerTotals* timers = static_cast<YumiProbeProfileTimerTotals*>(
        yumiProbeAllocZeroed(slotCount * sizeof(YumiProbeProfileTimerTotals)));
    if (counters->profileSlotCount) {
        memcpy(values, counters->profileCounters, counters->profileSlotCount * sizeof(uint64_t));
        memcpy(timers, counters->timers, counters->profileSlotCount * sizeof(YumiProbeProfileTimerTotals));
    }
    free(counters->profileCounters);
    free(counters->timers);
    counters->profileCounters = values;
    counters->timers = timers;
    counters->profileSlotCount = slotCount;
}

__attribute__((noinline)) static bool
yumiProbeCounterSlotsGrow(YumiProbeThreadCounters* counters, uint16_t id) {
    size_t slotCount = gProbeCounterSlotCount;
    if (id >= slotCount) {
        return false;
    }
    pthread_mutex_lock(&gProbeThreadCountersLock);
    yumiProbeCounterSlotsGrowLocked(counters, slotCount);
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    return true;
}

__attribute__((noinline)) static bool
yumiProbeProfileSlotsGrow(YumiProbeThreadCounters* counters, uint16_t id) {
    size_t slotCount = gProbeProfileSlotCount;
    if (id >= slotCount) {
        return false;
    }
    pthread_mutex_lock(&gProbeThreadCountersLock);
    yumiProbeProfileSlotsGrowLocked(counters, slotCount);
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    return true;
}

__attribute__((noinline)) static YumiProbeThreadCounters*
yumiProbeThreadCountersStart() {
    YumiProbeThreadCounters* counters = yumiProbeThreadCountersCreate();
    pthread_mutex_lock(&gProbeThreadCountersLock);
    gProbeThreadCounters.push_back(counters);
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    gProbeThreadCountersOwner.counters = counters;
    gProbeActiveThreadCounters = counters;
    return counters;
}

__attribute__((always_inline)) static inline YumiProbeThreadCounters*
yumiProbeActiveThreadCounters() {
    YumiProbeThreadCounters* counters = gProbeActiveThreadCounters;
    return counters ? counters : yumiProbeThreadCountersStart();
}

static void
yumiProbeObjectCountersLock(YumiProbeThreadCounters* counters) {
    while (__atomic_exchange_n(&counters->objectCounterLock, (uint8_t)1, __ATOMIC_ACQUIRE)) {
        sched_yield();
    }
}

static void
yumiProbeObjectCountersUnlock(YumiProbeThreadCounters* counters) {
    __atomic_store_n(&counters->objectCounterLock, (uint8_t)0, __ATOMIC_RELEASE);
}

static void
yumiProbeObjectCountersMerge(std::unordered_map<uint64_t, YumiProbeObjectCounter>& into,
        const std::unordered_map<uint64_t, YumiProbeObjectCounter>& from) {
    for (const auto& entry : from) {
        auto inserted = into.emplace(entry.first, entry.second);
        if (!inserted.second) {
            inserted.first->second.value += entry.second.value;
        }
    }
}

static void
yumiProbeThreadCountersRetire(YumiProbeThreadCounters* counters) {
    pthread_mutex_lock(&gProbeThreadCountersLock);
    if (!gProbeRetiredCounters) {
        gProbeRetiredCounters = yumiProbeThreadCountersCreate();
        gProbeThreadCounters.push_back(gProbeRetiredCounters);
    }
    YumiProbeThreadCounters* retired = gProbeRetiredCounters;
    yumiProbeCounterSlotsGrowLocked(retired, counters->counterSlotCount);
    yumiProbeProfileSlotsGrowLocked(retired, counters->profileSlotCount);
    for (size_t id = 0; id < counters->counterSlotCount; id++) {
        retired->counters[id] += counters->counters[id];
        retired->touched[id] |= counters->touched[id];
    }
    for (size_t id = 0; id < counters->profileSlotCount; id++) {
        retired->profileCounters[id] += counters->profileCounters[id];
        retired->timers[id].count += counters->timers[id].count;
        retired->timers[id].totalNs += counters->timers[id].totalNs;
        retired->timers[id].activeCount += counters->timers[id].activeCount;
        retired->timers[id].activeStartSum += counters->timers[id].activeStartSum;
    }
    yumiProbeObjectCountersMerge(retired->objectCounters, counters->objectCounters);
    for (size_t index = 0; index < gProbeThreadCounters.size(); index++) {
        if (gProbeThreadCounters[index] == counters) {
            gProbeThreadCounters.erase(gProbeThreadCounters.begin() + index);
            break;
        }
    }
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    yumiProbeThreadCountersDestroy(counters);
}

YumiProbeThreadCountersOwner::~YumiProbeThreadCountersOwner() {
    YumiProbeThreadCounters* owned = counters;
    if (!owned) {
        return;
    }
    counters = NULL;
    gProbeActiveThreadCounters = NULL;
    yumiProbeThreadCountersRetire(owned);
}

static void
yumiProbeProfileTimerUpdate(uint16_t id, uint64_t count, uint64_t totalNs, uint64_t activeCount,
        uint64_t activeStartSum) {
    YumiProbeThreadCounters* counters = yumiProbeActiveThreadCounters();
    if (id >= counters->profileSlotCount && !yumiProbeProfileSlotsGrow(counters, id)) {
        return;
    }

    YumiProbeProfileTimerTotals& totals = counters->timers[id];
    uint64_t sequence = counters->sequence;
    __atomic_store_n(&counters->sequence, sequence + 1, __ATOMIC_RELAXED);
    __atomic_thread_fence(__ATOMIC_RELEASE);
    __atomic_store_n(&totals.count, totals.count + count, __ATOMIC_RELAXED);
    __atomic_store_n(&totals.totalNs, totals.totalNs + totalNs, __ATOMIC_RELAXED);
    __atomic_store_n(&totals.activeCount, totals.activeCount + activeCount, __ATOMIC_RELAXED);
    __atomic_store_n(&totals.activeStartSum, totals.activeStartSum + activeStartSum, __ATOMIC_RELAXED);
    __atomic_store_n(&counters->sequence, sequence + 2, __ATOMIC_RELEASE);
}

static YumiProbeProfileTimerTotals
yumiProbeProfileTimerTotalsOf(YumiProbeThreadCounters* counters, uint16_t id) {
    YumiProbeProfileTimerTotals totals = {};
    if (id >= counters->profileSlotCount) {
        return totals;
    }

    YumiProbeProfileTimerTotals& source = counters->timers[id];
    while (true) {
        uint64_t before = __atomic_load_n(&counters->sequence, __ATOMIC_ACQUIRE);
        if (before & 1) {
            continue;
        }
        totals.count = __atomic_load_n(&source.count, __ATOMIC_RELAXED);
        totals.totalNs = __atomic_load_n(&source.totalNs, __ATOMIC_RELAXED);
        totals.activeCount = __atomic_load_n(&source.activeCount, __ATOMIC_RELAXED);
        totals.activeStartSum = __atomic_load_n(&source.activeStartSum, __ATOMIC_RELAXED);
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        if (__atomic_load_n(&counters->sequence, __ATOMIC_RELAXED) == before) {
            return totals;
        }
    }
}
struct YumiProbeProfileTimerFrame {
    uint16_t id;
    uint64_t startNs;
};

struct YumiProbeProfileTimerStack {
    size_t size;
    size_t capacity;
    YumiProbeProfileTimerFrame frames[1];
};

static void
yumiProbeProfileTimerStackPush(uint16_t id, uint64_t startNs) {
    YuTaskRuntimeContext* context = yumiGetActiveTaskRuntimeContext();
    YumiProbeProfileTimerStack* stack =
        static_cast<YumiProbeProfileTimerStack*>(context->probeProfileTimers);
    if (!stack || stack->size == stack->capacity) {
        size_t capacity = stack ? stack->capacity * 2 : 64;
        size_t bytes = sizeof(YumiProbeProfileTimerStack)
            + (capacity - 1) * sizeof(YumiProbeProfileTimerFrame);
        void* next = realloc(stack, bytes);
        if (!next) {
            fprintf(stderr, "Unable to allocate probe profile timer stack\n");
            abort();
        }
        if (!stack) {
            static_cast<YumiProbeProfileTimerStack*>(next)->size = 0;
        }
        stack = static_cast<YumiProbeProfileTimerStack*>(next);
        stack->capacity = capacity;
        context->probeProfileTimers = stack;
    }
    stack->frames[stack->size++] = { id, startNs };
}

static bool
yumiProbeProfileTimerStackPop(uint16_t id, uint64_t* startNs) {
    YumiProbeProfileTimerStack* stack = static_cast<YumiProbeProfileTimerStack*>(
        yumiGetActiveTaskRuntimeContext()->probeProfileTimers);
    if (!stack || !stack->size) {
        return false;
    }

    YumiProbeProfileTimerFrame frame = stack->frames[stack->size - 1];
    if (frame.id != id) {
        return false;
    }

    stack->size--;
    *startNs = frame.startNs;
    return true;
}

extern "C" uint64_t
yumiProbeNowNs() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return ((uint64_t)t.tv_sec * 1000000000ULL) + (uint64_t)t.tv_nsec;
}

extern "C" uint64_t
yumiProbeMonotonicNowNs() {
    struct timespec t;
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
#else
    clock_gettime(CLOCK_MONOTONIC, &t);
#endif
    return ((uint64_t)t.tv_sec * 1000000000ULL) + (uint64_t)t.tv_nsec;
}

extern "C" void
yumiProbeCounterEnsure(size_t slotCount) {
    size_t count = slotCount < 65536 ? slotCount : 65536;
    if (count > gProbeCounterSlotCount) {
        gProbeCounterSlotCount = count;
    }
}

extern "C" void
yumiProbeCounterSetup(size_t slotCount) {
    pthread_mutex_lock(&gProbeThreadCountersLock);
    for (YumiProbeThreadCounters* counters : gProbeThreadCounters) {
        if (counters->counterSlotCount) {
            memset(counters->counters, 0, counters->counterSlotCount * sizeof(uint64_t));
            memset(counters->touched, 0, counters->counterSlotCount * sizeof(uint8_t));
        }
        yumiProbeObjectCountersLock(counters);
        counters->objectCounters.clear();
        yumiProbeObjectCountersUnlock(counters);
    }
    gProbeObjectCounterSnapshot.clear();
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    gProbeCounterSlotCount = 0;
    yumiProbeCounterEnsure(slotCount);
}

extern "C" size_t
yumiProbeCounterCount() {
    return gProbeCounterSlotCount;
}

extern "C" void
yumiProbeCounterAdd(uint16_t id, uint64_t delta) {
    YumiProbeThreadCounters* counters = yumiProbeActiveThreadCounters();
    if (id >= counters->counterSlotCount && !yumiProbeCounterSlotsGrow(counters, id)) {
        return;
    }
    uint64_t* slot = &counters->counters[id];
    __atomic_store_n(slot, __atomic_load_n(slot, __ATOMIC_RELAXED) + delta, __ATOMIC_RELAXED);
    __atomic_store_n(&counters->touched[id], (uint8_t)1, __ATOMIC_RELAXED);
}

extern "C" uint64_t
yumiProbeCounterValue(uint16_t id) {
    uint64_t value = 0;
    pthread_mutex_lock(&gProbeThreadCountersLock);
    for (YumiProbeThreadCounters* counters : gProbeThreadCounters) {
        if (id < counters->counterSlotCount) {
            value += __atomic_load_n(&counters->counters[id], __ATOMIC_RELAXED);
        }
    }
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    return value;
}

extern "C" bool
yumiProbeCounterTouched(uint16_t id) {
    bool touched = false;
    pthread_mutex_lock(&gProbeThreadCountersLock);
    for (YumiProbeThreadCounters* counters : gProbeThreadCounters) {
        if (id < counters->counterSlotCount && __atomic_load_n(&counters->touched[id], __ATOMIC_RELAXED)) {
            touched = true;
            break;
        }
    }
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    return touched;
}

extern "C" void
yumiProbeObjectCounterAdd(uint16_t id, uint32_t objectId, uint16_t classType, uint64_t delta) {
    YumiProbeThreadCounters* counters = yumiProbeActiveThreadCounters();
    uint64_t key = ((uint64_t)id << 32) | objectId;
    YumiProbeObjectCounter counter = {id, classType, objectId, 0};
    yumiProbeObjectCountersLock(counters);
    counters->objectCounters.emplace(key, counter).first->second.value += delta;
    yumiProbeObjectCountersUnlock(counters);
}

extern "C" size_t
yumiProbeObjectCounterCollect() {
    std::unordered_map<uint64_t, YumiProbeObjectCounter> merged;
    pthread_mutex_lock(&gProbeThreadCountersLock);
    for (YumiProbeThreadCounters* counters : gProbeThreadCounters) {
        yumiProbeObjectCountersLock(counters);
        yumiProbeObjectCountersMerge(merged, counters->objectCounters);
        yumiProbeObjectCountersUnlock(counters);
    }
    gProbeObjectCounterSnapshot.clear();
    for (const auto& entry : merged) {
        gProbeObjectCounterSnapshot.push_back(entry.second);
    }
    std::sort(gProbeObjectCounterSnapshot.begin(), gProbeObjectCounterSnapshot.end(),
        [](const YumiProbeObjectCounter& a, const YumiProbeObjectCounter& b) {
            return a.id != b.id ? a.id < b.id : a.objectId < b.objectId;
        });
    size_t count = gProbeObjectCounterSnapshot.size();
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    return count;
}

static YumiProbeObjectCounter
yumiProbeObjectCounterAt(size_t index) {
    YumiProbeObjectCounter counter = {0, 0, 0, 0};
    pthread_mutex_lock(&gProbeThreadCountersLock);
    if (index < gProbeObjectCounterSnapshot.size()) {
        counter = gProbeObjectCounterSnapshot[index];
    }
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    return counter;
}

extern "C" uint16_t
yumiProbeObjectCounterId(size_t index) {
    return yumiProbeObjectCounterAt(index).id;
}

extern "C" uint32_t
yumiProbeObjectCounterObjectId(size_t index) {
    return yumiProbeObjectCounterAt(index).objectId;
}

extern "C" uint16_t
yumiProbeObjectCounterClassType(size_t index) {
    return yumiProbeObjectCounterAt(index).classType;
}

extern "C" uint64_t
yumiProbeObjectCounterValue(size_t index) {
    return yumiProbeObjectCounterAt(index).value;
}

extern "C" void
yumiProbeProfileSetup(size_t metricCount) {
    size_t slotCount = metricCount + 1;
    pthread_mutex_lock(&gProbeThreadCountersLock);
    for (YumiProbeThreadCounters* counters : gProbeThreadCounters) {
        if (counters->profileSlotCount) {
            memset(counters->profileCounters, 0, counters->profileSlotCount * sizeof(uint64_t));
            memset(counters->timers, 0, counters->profileSlotCount * sizeof(YumiProbeProfileTimerTotals));
        }
    }
    gProbeProfileSlotCount = slotCount < 65536 ? slotCount : 65536;
    pthread_mutex_unlock(&gProbeThreadCountersLock);
}

extern "C" void
yumiProbeProfileCounterAdd(uint16_t id, uint64_t delta) {
    YumiProbeThreadCounters* counters = yumiProbeActiveThreadCounters();
    if (id >= counters->profileSlotCount && !yumiProbeProfileSlotsGrow(counters, id)) {
        return;
    }
    uint64_t* slot = &counters->profileCounters[id];
    __atomic_store_n(slot, __atomic_load_n(slot, __ATOMIC_RELAXED) + delta, __ATOMIC_RELAXED);
}

extern "C" uint64_t
yumiProbeProfileCounterValue(uint16_t id) {
    uint64_t value = 0;
    pthread_mutex_lock(&gProbeThreadCountersLock);
    for (YumiProbeThreadCounters* counters : gProbeThreadCounters) {
        if (id < counters->profileSlotCount) {
            value += __atomic_load_n(&counters->profileCounters[id], __ATOMIC_RELAXED);
        }
    }
    pthread_mutex_unlock(&gProbeThreadCountersLock);
    return value;
}

extern "C" void
yumiProbeProfileTimerBegin(uint16_t id) {
    if (id >= gProbeProfileSlotCount) {
        return;
    }

    uint64_t now = yumiProbeMonotonicNowNs();
    yumiProbeProfileTimerStackPush(id, now);
    yumiProbeProfileTimerUpdate(id, 0, 0, 1, now);
}

extern "C" void
yumiProbeProfileTimerEnd(uint16_t id) {
    if (id >= gProbeProfileSlotCount) {
        return;
    }

    uint64_t now = yumiProbeMonotonicNowNs();
    uint64_t startNs = 0;
    if (!yumiProbeProfileTimerStackPop(id, &startNs)) {
        return;
    }

    yumiProbeProfileTimerUpdate(id, 1, now - startNs, (uint64_t)-1, (uint64_t)0 - startNs);
}

extern "C" void
yumiProbeProfileTimerRead(uint16_t id, uint64_t nowNs, uint64_t* values) {
    YumiProbeProfileTimerTotals sum = {};
    pthread_mutex_lock(&gProbeThreadCountersLock);
    for (YumiProbeThreadCounters* counters : gProbeThreadCounters) {
        YumiProbeProfileTimerTotals totals = yumiProbeProfileTimerTotalsOf(counters, id);
        sum.count += totals.count;
        sum.totalNs += totals.totalNs;
        sum.activeCount += totals.activeCount;
        sum.activeStartSum += totals.activeStartSum;
    }
    pthread_mutex_unlock(&gProbeThreadCountersLock);

    int64_t active = (int64_t)sum.activeCount;
    int64_t elapsed = active > 0 ? (int64_t)(nowNs * sum.activeCount - sum.activeStartSum) : 0;
    values[0] = sum.count;
    values[1] = sum.totalNs;
    values[2] = active > 0 ? (uint64_t)active : 0;
    values[3] = elapsed > 0 ? (uint64_t)elapsed : 0;
}

static size_t
yumiProbeAlign(size_t size) {
    size_t rem = size % YUMI_PROBE_RECORD_ALIGNMENT;
    return rem ? size + YUMI_PROBE_RECORD_ALIGNMENT - rem : size;
}

static size_t
yumiProbePageAlign(size_t size) {
    size_t pageSize = (size_t)getpagesize();
    if (size < pageSize) {
        size = pageSize;
    }
    size_t rem = size % pageSize;
    return rem ? size + pageSize - rem : size;
}

static uint64_t
yumiProbeFileSize(uint64_t segmentCount) {
    return (uint64_t)gProbeTransport.headerSize + segmentCount * (uint64_t)gProbeTransport.segmentSize;
}

static uint64_t
yumiProbeSegmentFileOffset(uint64_t segmentIndex) {
    return (uint64_t)gProbeTransport.headerSize + segmentIndex * (uint64_t)gProbeTransport.segmentSize;
}

static void
yumiProbeFatal(const char* message) {
    fprintf(stderr, "Fatal probe transport error: %s\n", message);
    abort();
}

static void
yumiProbeWriteU32(char* base, size_t offset, uint32_t value) {
    *((uint32_t*)(base + offset)) = value;
}

static void
yumiProbeWriteU64(char* base, size_t offset, uint64_t value) {
    *((uint64_t*)(base + offset)) = value;
}

static void
yumiProbeStoreReleaseU32(char* base, size_t offset, uint32_t value) {
    __atomic_store_n((uint32_t*)(base + offset), value, __ATOMIC_RELEASE);
}

static void
yumiProbeStoreReleaseU64(char* base, size_t offset, uint64_t value) {
    __atomic_store_n((uint64_t*)(base + offset), value, __ATOMIC_RELEASE);
}

static uint64_t
yumiProbeLoadAcquireU64(char* base, size_t offset) {
    return __atomic_load_n((uint64_t*)(base + offset), __ATOMIC_ACQUIRE);
}

static char*
yumiProbeMapSegmentLocked(uint64_t segmentIndex) {
    if (segmentIndex >= gProbeTransport.segmentTableCapacity) {
        yumiProbeFatal("Probe segment table capacity exceeded");
    }

    uint64_t fileSize = yumiProbeFileSize(segmentIndex + 1);
    if (ftruncate(gProbeTransport.fd, (off_t)fileSize) != 0) {
        yumiProbeFatal("Unable to grow probe trace file");
    }

    void* segment = mmap(
        NULL,
        gProbeTransport.segmentSize,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        gProbeTransport.fd,
        (off_t)yumiProbeSegmentFileOffset(segmentIndex));
    if (segment == MAP_FAILED) {
        yumiProbeFatal("Unable to map probe trace segment");
    }

    char* segmentMap = (char*)segment;
    memset(segmentMap, 0, YUMI_PROBE_SEGMENT_HEADER_SIZE);
    yumiProbeWriteU32(segmentMap, YUMI_PROBE_SEGMENT_MAGIC_OFFSET, YUMI_PROBE_SEGMENT_MAGIC);
    yumiProbeWriteU32(
        segmentMap,
        YUMI_PROBE_SEGMENT_HEADER_SIZE_OFFSET,
        YUMI_PROBE_SEGMENT_HEADER_SIZE);
    yumiProbeWriteU64(segmentMap, YUMI_PROBE_SEGMENT_SIZE_OFFSET, gProbeTransport.segmentSize);
    yumiProbeStoreReleaseU64(
        segmentMap,
        YUMI_PROBE_SEGMENT_RESERVE_CURSOR_OFFSET,
        YUMI_PROBE_SEGMENT_HEADER_SIZE);
    yumiProbeStoreReleaseU64(segmentMap, YUMI_PROBE_SEGMENT_SEALED_CURSOR_OFFSET, 0);
    yumiProbeWriteU64(segmentMap, YUMI_PROBE_SEGMENT_INDEX_OFFSET, segmentIndex);

    if (gProbeTransport.segments.size() <= segmentIndex) {
        gProbeTransport.segments.resize(segmentIndex + 1, NULL);
    }
    gProbeTransport.segments[segmentIndex] = segmentMap;
    yumiProbeStoreReleaseU64(
        gProbeTransport.header,
        YUMI_PROBE_FILE_SEGMENT_COUNT_OFFSET,
        segmentIndex + 1);
    return segmentMap;
}

static char*
yumiProbeEnsureSegmentLocked(uint64_t segmentIndex) {
    while (gProbeTransport.segments.size() <= segmentIndex ||
           !gProbeTransport.segments[segmentIndex]) {
        yumiProbeMapSegmentLocked(gProbeTransport.segments.size());
    }
    return gProbeTransport.segments[segmentIndex];
}

static bool
yumiProbeIsOpenAcquire() {
    return __atomic_load_n(&gProbeOpen, __ATOMIC_ACQUIRE) != 0;
}

static void
yumiProbePublishPayload(const void* data, size_t payloadSize, uint64_t nowNs) {
    if (!data || !payloadSize || !yumiProbeIsOpenAcquire()) {
        return;
    }

    size_t recordSize = yumiProbeAlign(YUMI_PROBE_RECORD_HEADER_SIZE + payloadSize);
    if (recordSize > gProbeTransport.segmentSize - YUMI_PROBE_SEGMENT_HEADER_SIZE) {
        yumiProbeFatal("Probe record exceeds mapped segment size");
    }

    pthread_mutex_lock(&gProbeTransport.lock);

    if (!yumiProbeIsOpenAcquire() || gProbeTransport.fd < 0 || !gProbeTransport.header) {
        pthread_mutex_unlock(&gProbeTransport.lock);
        return;
    }

    while (true) {
        char* segment = yumiProbeEnsureSegmentLocked(gProbeTransport.activeSegment);
        uint64_t cursor = yumiProbeLoadAcquireU64(segment, YUMI_PROBE_SEGMENT_RESERVE_CURSOR_OFFSET);
        if (cursor + recordSize <= gProbeTransport.segmentSize) {
            char* record = segment + cursor;
            uint64_t nextCursor = cursor + recordSize;
            uint64_t sequence = gProbeTransport.nextSequence++;
            uint64_t sec = nowNs / 1000000000ULL;
            uint64_t nsec = nowNs % 1000000000ULL;

            yumiProbeStoreReleaseU32(record, YUMI_PROBE_RECORD_STATE_OFFSET, YUMI_PROBE_RECORD_STATE_WRITING);
            yumiProbeWriteU32(record, YUMI_PROBE_RECORD_HEADER_SIZE_OFFSET, YUMI_PROBE_RECORD_HEADER_SIZE);
            yumiProbeWriteU64(record, YUMI_PROBE_RECORD_LENGTH_OFFSET, recordSize);
            yumiProbeWriteU64(record, YUMI_PROBE_RECORD_PAYLOAD_LENGTH_OFFSET, payloadSize);
            yumiProbeWriteU64(record, YUMI_PROBE_RECORD_SEQUENCE_OFFSET, sequence);
            yumiProbeWriteU64(record, YUMI_PROBE_RECORD_TIME_SEC_OFFSET, sec);
            yumiProbeWriteU64(record, YUMI_PROBE_RECORD_TIME_NSEC_OFFSET, nsec);
            yumiProbeWriteU32(record, YUMI_PROBE_RECORD_TASK_ID_OFFSET, 0);
            yumiProbeWriteU64(
                record,
                YUMI_PROBE_RECORD_THREAD_ID_OFFSET,
                (uint64_t)(uintptr_t)pthread_self());
            memcpy(record + YUMI_PROBE_RECORD_HEADER_SIZE, data, payloadSize);
            __atomic_thread_fence(__ATOMIC_RELEASE);
            yumiProbeStoreReleaseU32(record, YUMI_PROBE_RECORD_STATE_OFFSET, YUMI_PROBE_RECORD_STATE_COMMITTED);
            yumiProbeStoreReleaseU64(segment, YUMI_PROBE_SEGMENT_RESERVE_CURSOR_OFFSET, nextCursor);

            gProbeTransport.totalRecords += 1;
            yumiProbeStoreReleaseU64(
                gProbeTransport.header,
                YUMI_PROBE_FILE_NEXT_SEQUENCE_OFFSET,
                gProbeTransport.nextSequence);
            yumiProbeStoreReleaseU64(
                gProbeTransport.header,
                YUMI_PROBE_FILE_TOTAL_RECORDS_OFFSET,
                gProbeTransport.totalRecords);
            break;
        }

        yumiProbeStoreReleaseU64(segment, YUMI_PROBE_SEGMENT_SEALED_CURSOR_OFFSET, cursor);
        gProbeTransport.activeSegment += 1;
        yumiProbeEnsureSegmentLocked(gProbeTransport.activeSegment);
        yumiProbeStoreReleaseU64(
            gProbeTransport.header,
            YUMI_PROBE_FILE_ACTIVE_SEGMENT_OFFSET,
            gProbeTransport.activeSegment);
    }

    pthread_mutex_unlock(&gProbeTransport.lock);
}

extern "C" bool
yumiProbeOpen(
    const char* path,
    uint64_t segmentSize,
    uint64_t segmentPool,
    uint64_t taskBufferSize,
    uint64_t flushThreshold,
    uint64_t flushIntervalNs,
    uint64_t segmentTableCapacity) {
    if (!path || !path[0]) {
        return false;
    }

    pthread_mutex_lock(&gProbeTransport.lock);

    if (yumiProbeIsOpenAcquire()) {
        pthread_mutex_unlock(&gProbeTransport.lock);
        return true;
    }

    gProbeTransport.headerSize = yumiProbePageAlign(YUMI_PROBE_FILE_HEADER_SIZE);
    gProbeTransport.segmentSize = yumiProbePageAlign((size_t)segmentSize);
    gProbeTransport.taskBufferSize = taskBufferSize ? (size_t)taskBufferSize : 65536;
    gProbeTransport.flushThreshold = flushThreshold ? (size_t)flushThreshold : 16384;
    gProbeTransport.flushIntervalNs = flushIntervalNs ? flushIntervalNs : 16000000ULL;
    gProbeTransport.segmentTableCapacity = segmentTableCapacity ? segmentTableCapacity : 65536;
    if (!segmentPool) {
        segmentPool = 1;
    }
    if (gProbeTransport.segmentTableCapacity < segmentPool) {
        gProbeTransport.segmentTableCapacity = segmentPool;
    }
    if (gProbeTransport.flushThreshold > gProbeTransport.taskBufferSize) {
        gProbeTransport.flushThreshold = gProbeTransport.taskBufferSize;
    }
    size_t maxPayload =
        gProbeTransport.segmentSize - YUMI_PROBE_SEGMENT_HEADER_SIZE - YUMI_PROBE_RECORD_HEADER_SIZE;
    if (gProbeTransport.flushThreshold > maxPayload) {
        gProbeTransport.flushThreshold = maxPayload;
    }

    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if (fd < 0) {
        pthread_mutex_unlock(&gProbeTransport.lock);
        return false;
    }

    gProbeTransport.fd = fd;
    gProbeTransport.activeSegment = 0;
    gProbeTransport.nextSequence = 1;
    gProbeTransport.totalRecords = 0;
    gProbeTransport.segments.clear();

    if (ftruncate(fd, (off_t)yumiProbeFileSize(segmentPool)) != 0) {
        close(fd);
        gProbeTransport.fd = -1;
        pthread_mutex_unlock(&gProbeTransport.lock);
        return false;
    }

    void* header = mmap(
        NULL,
        gProbeTransport.headerSize,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);
    if (header == MAP_FAILED) {
        close(fd);
        gProbeTransport.fd = -1;
        pthread_mutex_unlock(&gProbeTransport.lock);
        return false;
    }

    gProbeTransport.header = (char*)header;
    memset(gProbeTransport.header, 0, gProbeTransport.headerSize);

    yumiProbeWriteU64(gProbeTransport.header, YUMI_PROBE_FILE_HEADER_SIZE_OFFSET, gProbeTransport.headerSize);
    yumiProbeWriteU64(gProbeTransport.header, YUMI_PROBE_FILE_SEGMENT_SIZE_OFFSET, gProbeTransport.segmentSize);
    yumiProbeWriteU64(
        gProbeTransport.header,
        YUMI_PROBE_FILE_RECORD_HEADER_SIZE_OFFSET,
        YUMI_PROBE_RECORD_HEADER_SIZE);
    yumiProbeStoreReleaseU64(gProbeTransport.header, YUMI_PROBE_FILE_DONE_OFFSET, 0);
    yumiProbeStoreReleaseU64(gProbeTransport.header, YUMI_PROBE_FILE_NEXT_SEQUENCE_OFFSET, 1);
    yumiProbeStoreReleaseU64(gProbeTransport.header, YUMI_PROBE_FILE_TOTAL_RECORDS_OFFSET, 0);
    yumiProbeStoreReleaseU64(gProbeTransport.header, YUMI_PROBE_FILE_READY_OFFSET, 0);

    for (uint64_t segmentIndex = 0; segmentIndex < segmentPool; ++segmentIndex) {
        yumiProbeMapSegmentLocked(segmentIndex);
    }

    yumiProbeStoreReleaseU64(gProbeTransport.header, YUMI_PROBE_FILE_ACTIVE_SEGMENT_OFFSET, 0);
    __atomic_store_n(&gProbeOpen, 1u, __ATOMIC_RELEASE);
    __atomic_store_n(&yumiProbeTracing, 1, __ATOMIC_RELEASE);
    yumiProbeStoreReleaseU64(gProbeTransport.header, YUMI_PROBE_FILE_READY_OFFSET, 1);
    yumiProbeStoreReleaseU32(
        gProbeTransport.header,
        YUMI_PROBE_FILE_VERSION_OFFSET,
        YUMI_PROBE_TRANSPORT_VERSION);
    yumiProbeStoreReleaseU32(gProbeTransport.header, YUMI_PROBE_FILE_MAGIC_OFFSET, YUMI_PROBE_FILE_MAGIC);

    pthread_mutex_unlock(&gProbeTransport.lock);
    return true;
}

extern "C" void
yumiProbeClose() {
    pthread_mutex_lock(&gProbeTransport.lock);
    if (!yumiProbeIsOpenAcquire()) {
        pthread_mutex_unlock(&gProbeTransport.lock);
        return;
    }

    __atomic_store_n(&yumiProbeTracing, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&gProbeOpen, 0u, __ATOMIC_RELEASE);
    if (gProbeTransport.header) {
        yumiProbeStoreReleaseU64(gProbeTransport.header, YUMI_PROBE_FILE_DONE_OFFSET, 1);
    }

    for (char* segment : gProbeTransport.segments) {
        if (segment) {
            munmap(segment, gProbeTransport.segmentSize);
        }
    }
    gProbeTransport.segments.clear();

    if (gProbeTransport.header) {
        munmap(gProbeTransport.header, gProbeTransport.headerSize);
        gProbeTransport.header = NULL;
    }

    if (gProbeTransport.fd >= 0) {
        close(gProbeTransport.fd);
        gProbeTransport.fd = -1;
    }

    pthread_mutex_unlock(&gProbeTransport.lock);
}

extern "C" size_t
yumiProbeTaskBufferSize() {
    return gProbeTransport.taskBufferSize;
}

extern "C" size_t
yumiProbeFlushThreshold() {
    return gProbeTransport.flushThreshold;
}

extern "C" uint64_t
yumiProbeFlushIntervalNs() {
    return gProbeTransport.flushIntervalNs;
}

extern "C" void
yumiProbePublish(const void* data, size_t size, uint64_t nowNs) {
    yumiProbePublishPayload(data, size, nowNs ? nowNs : yumiProbeNowNs());
}
