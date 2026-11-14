#ifndef YU_PROBE_H
#define YU_PROBE_H

#include <cstdlib>
#include <stdint.h>

#include <YuGC/YuGCAnnotations.h>

extern "C" bool yumiProbeOpen(
    const char* path,
    uint64_t segmentSize,
    uint64_t segmentPool,
    uint64_t taskBufferSize,
    uint64_t flushThreshold,
    uint64_t flushIntervalNs,
    uint64_t segmentTableCapacity);
extern "C" void yumiProbeClose();
extern "C" size_t yumiProbeTaskBufferSize();
extern "C" size_t yumiProbeFlushThreshold();
extern "C" uint64_t yumiProbeFlushIntervalNs();
extern "C" void yumiProbePublish(const void* data, size_t size, uint64_t nowNs);
extern "C" YUMI_GC_LEAF uint64_t yumiProbeMonotonicNowNs();
extern "C" YUMI_GC_LEAF uint64_t yumiProbeNowNs();
extern "C" unsigned char yumiProbeTracing;
extern "C" YUMI_GC_LEAF void yumiProbeCounterSetup(size_t slotCount);
extern "C" YUMI_GC_LEAF void yumiProbeCounterEnsure(size_t slotCount);
extern "C" YUMI_GC_LEAF size_t yumiProbeCounterCount();
extern "C" YUMI_GC_LEAF void yumiProbeCounterAdd(uint16_t id, uint64_t delta);
extern "C" YUMI_GC_LEAF uint64_t yumiProbeCounterValue(uint16_t id);
extern "C" YUMI_GC_LEAF bool yumiProbeCounterTouched(uint16_t id);
extern "C" YUMI_GC_LEAF void yumiProbeObjectCounterAdd(
    uint16_t id,
    uint32_t objectId,
    uint16_t classType,
    uint64_t delta);
extern "C" YUMI_GC_LEAF size_t yumiProbeObjectCounterCollect();
extern "C" YUMI_GC_LEAF uint16_t yumiProbeObjectCounterId(size_t index);
extern "C" YUMI_GC_LEAF uint32_t yumiProbeObjectCounterObjectId(size_t index);
extern "C" YUMI_GC_LEAF uint16_t yumiProbeObjectCounterClassType(size_t index);
extern "C" YUMI_GC_LEAF uint64_t yumiProbeObjectCounterValue(size_t index);
extern "C" void yumiProbeProfileSetup(size_t metricCount);
extern "C" YUMI_GC_LEAF void yumiProbeProfileCounterAdd(uint16_t id, uint64_t delta);
extern "C" YUMI_GC_LEAF uint64_t yumiProbeProfileCounterValue(uint16_t id);
extern "C" YUMI_GC_LEAF void yumiProbeProfileTimerBegin(uint16_t id);
extern "C" YUMI_GC_LEAF void yumiProbeProfileTimerEnd(uint16_t id);
extern "C" YUMI_GC_LEAF void yumiProbeProfileTimerRead(uint16_t id, uint64_t nowNs, uint64_t* values);
extern "C" size_t yumiGCCollectionCount(void);
extern "C" size_t yumiGCPauseCount(void);
extern "C" size_t yumiGCAssistTimeNanos(void);
extern "C" bool yumiProbeControlOpen(const char* path, const char* token);
extern "C" void yumiProbeControlClose();
extern "C" bool yumiProbeControlAvailable();
extern "C" YUMI_GC_LEAF bool yumiProbeLiveTryLock();
extern "C" void yumiProbeLiveLock();
extern "C" YUMI_GC_LEAF void yumiProbeLiveUnlock();
extern "C" size_t yumiProbeControlRead(uint8_t* dst, size_t capacity);
extern "C" bool yumiProbeControlWrite(const uint8_t* data, size_t size);

#endif
