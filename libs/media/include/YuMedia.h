#ifndef YU_MEDIA_H
#define YU_MEDIA_H

#include <stdbool.h>
#include <stdint.h>

#include <YuGC/YuGCAnnotations.h>


extern "C" void* yumiMediaRingCreate(int64_t capacity, int64_t width);
extern "C" void yumiMediaRingRelease(void* ring);
extern "C" YUMI_WITHOUT_GC float* yumiMediaRingGetData(void* ring);
extern "C" YUMI_WITHOUT_GC int64_t yumiMediaRingGetWritten(void* ring);
extern "C" YUMI_WITHOUT_GC int64_t yumiMediaRingGetConsumed(void* ring);
extern "C" YUMI_WITHOUT_GC void yumiMediaRingPublish(void* ring, int64_t written);
extern "C" YUMI_WITHOUT_GC void yumiMediaRingConsume(void* ring, int64_t consumed);

extern "C" void* yumiAudioOutputOpen(void* ring, double sampleRate);
extern "C" void yumiAudioOutputRelease(void* output);
extern "C" const char* yumiAudioOutputGetError(void* output);
extern "C" YUMI_WITHOUT_GC bool yumiAudioOutputIsPlaying(void* output);
extern "C" YUMI_WITHOUT_GC int64_t yumiAudioOutputGetFramesPlayed(void* output);
extern "C" YUMI_WITHOUT_GC int64_t yumiAudioOutputGetUnderrunFrames(void* output);
extern "C" YUMI_WITHOUT_GC int64_t yumiAudioOutputGetSliceFrames(void* output);
extern "C" YUMI_WITHOUT_GC uint64_t yumiAudioOutputGetHostTime(void* output);

#endif
