#ifndef BX_PROBE_H
#define BX_PROBE_H

#include <cstdlib>
#include <stdint.h>

#include <BxGC/BxGCAnnotations.h>

extern "C" bool brixProbeOpen(
    const char* path,
    uint64_t segmentSize,
    uint64_t segmentPool,
    uint64_t taskBufferSize,
    uint64_t flushThreshold,
    uint64_t flushIntervalNs,
    uint64_t segmentTableCapacity);
extern "C" void brixProbeClose();
extern "C" size_t brixProbeTaskBufferSize();
extern "C" size_t brixProbeFlushThreshold();
extern "C" uint64_t brixProbeFlushIntervalNs();
extern "C" void brixProbePublish(const void* data, size_t size, uint64_t nowNs);
extern "C" BRIX_WITHOUT_GC uint64_t brixProbeMonotonicNowNs();
extern "C" BRIX_WITHOUT_GC uint64_t brixProbeNowNs();
extern "C" unsigned char brixProbeTracing;
extern "C" BRIX_WITHOUT_GC void brixProbeCounterSetup(size_t slotCount);
extern "C" BRIX_WITHOUT_GC void brixProbeCounterEnsure(size_t slotCount);
extern "C" BRIX_WITHOUT_GC size_t brixProbeCounterCount();
extern "C" BRIX_WITHOUT_GC void brixProbeCounterAdd(uint16_t id, uint64_t delta);
extern "C" BRIX_WITHOUT_GC uint64_t brixProbeCounterValue(uint16_t id);
extern "C" BRIX_WITHOUT_GC bool brixProbeCounterTouched(uint16_t id);
extern "C" BRIX_WITHOUT_GC void brixProbeObjectCounterAdd(
    uint16_t id,
    uint32_t objectId,
    uint16_t classType,
    uint64_t delta);
extern "C" BRIX_WITHOUT_GC size_t brixProbeObjectCounterCollect();
extern "C" BRIX_WITHOUT_GC uint16_t brixProbeObjectCounterId(size_t index);
extern "C" BRIX_WITHOUT_GC uint32_t brixProbeObjectCounterObjectId(size_t index);
extern "C" BRIX_WITHOUT_GC uint16_t brixProbeObjectCounterClassType(size_t index);
extern "C" BRIX_WITHOUT_GC uint64_t brixProbeObjectCounterValue(size_t index);
extern "C" void brixProbeProfileSetup(size_t metricCount);
extern "C" BRIX_WITHOUT_GC void brixProbeProfileCounterAdd(uint16_t id, uint64_t delta);
extern "C" BRIX_WITHOUT_GC uint64_t brixProbeProfileCounterValue(uint16_t id);
extern "C" BRIX_WITHOUT_GC void brixProbeProfileTimerBegin(uint16_t id);
extern "C" BRIX_WITHOUT_GC void brixProbeProfileTimerEnd(uint16_t id);
extern "C" BRIX_WITHOUT_GC void brixProbeProfileTimerRead(uint16_t id, uint64_t nowNs, uint64_t* values);
extern "C" size_t brixGCCollectionCount(void);
extern "C" size_t brixGCPauseCount(void);
extern "C" size_t brixGCAssistTimeNanos(void);
extern "C" bool brixProbeControlOpen(const char* path, const char* token);
extern "C" void brixProbeControlClose();
extern "C" bool brixProbeControlAvailable();
extern "C" BRIX_WITHOUT_GC bool brixProbeLiveTryLock();
extern "C" void brixProbeLiveLock();
extern "C" BRIX_WITHOUT_GC void brixProbeLiveUnlock();
extern "C" size_t brixProbeControlRead(uint8_t* dst, size_t capacity);
extern "C" bool brixProbeControlWrite(const uint8_t* data, size_t size);

#endif
