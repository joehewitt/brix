#ifndef BX_MEDIA_H
#define BX_MEDIA_H

#include <stdbool.h>
#include <stdint.h>

#include <BxGC/BxGCAnnotations.h>


extern "C" void* brixMediaRingCreate(int64_t capacity, int64_t width);
extern "C" void brixMediaRingRelease(void* ring);
extern "C" BRIX_WITHOUT_GC float* brixMediaRingGetData(void* ring);
extern "C" BRIX_WITHOUT_GC int64_t brixMediaRingGetWritten(void* ring);
extern "C" BRIX_WITHOUT_GC int64_t brixMediaRingGetConsumed(void* ring);
extern "C" BRIX_WITHOUT_GC void brixMediaRingPublish(void* ring, int64_t written);
extern "C" BRIX_WITHOUT_GC void brixMediaRingConsume(void* ring, int64_t consumed);

extern "C" void* brixAudioOutputOpen(void* ring, double sampleRate);
extern "C" void brixAudioOutputRelease(void* output);
extern "C" const char* brixAudioOutputGetError(void* output);
extern "C" BRIX_WITHOUT_GC bool brixAudioOutputIsPlaying(void* output);
extern "C" BRIX_WITHOUT_GC int64_t brixAudioOutputGetFramesPlayed(void* output);
extern "C" BRIX_WITHOUT_GC int64_t brixAudioOutputGetUnderrunFrames(void* output);
extern "C" BRIX_WITHOUT_GC int64_t brixAudioOutputGetSliceFrames(void* output);
extern "C" BRIX_WITHOUT_GC uint64_t brixAudioOutputGetHostTime(void* output);

#endif
