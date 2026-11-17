#include "include/BxMedia.h"

#include <AudioToolbox/AudioToolbox.h>
#include <TargetConditionals.h>

#if TARGET_OS_IPHONE
#include "BxAudioSession.h"
#endif

#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>


struct BxMediaRing {
    float* data = nullptr;
    int64_t capacity = 0;
    int64_t width = 0;
    std::atomic<int64_t> written{0};
    std::atomic<int64_t> consumed{0};
};

struct BxAudioOutput {
    AudioComponentInstance unit = nullptr;
    BxMediaRing* ring = nullptr;
    std::atomic<bool> playing{false};
    std::atomic<int64_t> framesPlayed{0};
    std::atomic<int64_t> underrunFrames{0};
    std::atomic<int64_t> sliceFrames{0};
    std::atomic<uint64_t> hostTime{0};
    std::string error;
};

static BxMediaRing*
getRing(void* ptr) {
    return static_cast<BxMediaRing*>(ptr);
}

static BxAudioOutput*
getOutput(void* ptr) {
    return static_cast<BxAudioOutput*>(ptr);
}


extern "C" void*
brixMediaRingCreate(int64_t capacity, int64_t width) {
    BxMediaRing* ring = new BxMediaRing();
    ring->capacity = std::max<int64_t>(capacity, 1);
    ring->width = std::max<int64_t>(width, 1);
    ring->data = static_cast<float*>(calloc(ring->capacity * ring->width, sizeof(float)));
    return ring;
}

extern "C" void
brixMediaRingRelease(void* ptr) {
    BxMediaRing* ring = getRing(ptr);
    if (!ring) {
        return;
    }
    free(ring->data);
    delete ring;
}

extern "C" float*
brixMediaRingGetData(void* ptr) {
    return getRing(ptr)->data;
}

extern "C" int64_t
brixMediaRingGetWritten(void* ptr) {
    return getRing(ptr)->written.load(std::memory_order_acquire);
}

extern "C" int64_t
brixMediaRingGetConsumed(void* ptr) {
    return getRing(ptr)->consumed.load(std::memory_order_acquire);
}

extern "C" void
brixMediaRingPublish(void* ptr, int64_t written) {
    getRing(ptr)->written.store(written, std::memory_order_release);
}

extern "C" void
brixMediaRingConsume(void* ptr, int64_t consumed) {
    getRing(ptr)->consumed.store(consumed, std::memory_order_release);
}


static OSStatus
audioOutputRender(void* refCon, AudioUnitRenderActionFlags* flags, const AudioTimeStamp* time,
                  UInt32 bus, UInt32 frames, AudioBufferList* buffers) {
    BxAudioOutput* output = getOutput(refCon);
    BxMediaRing* ring = output->ring;
    float* out = static_cast<float*>(buffers->mBuffers[0].mData);
    int64_t width = ring->width;
    int64_t consumed = ring->consumed.load(std::memory_order_relaxed);
    int64_t written = ring->written.load(std::memory_order_acquire);
    int64_t count = std::min<int64_t>(written - consumed, frames);
    int64_t copied = 0;
    while (copied < count) {
        int64_t slot = (consumed + copied) % ring->capacity;
        int64_t run = std::min<int64_t>(count - copied, ring->capacity - slot);
        memcpy(out + copied * width, ring->data + slot * width, run * width * sizeof(float));
        copied += run;
    }
    if (count < frames) {
        memset(out + count * width, 0, (frames - count) * width * sizeof(float));
        if (written > 0) {
            output->underrunFrames.fetch_add(frames - count, std::memory_order_relaxed);
        }
    }
    ring->consumed.store(consumed + count, std::memory_order_release);
    output->framesPlayed.fetch_add(frames, std::memory_order_relaxed);
    output->sliceFrames.store(frames, std::memory_order_relaxed);
    output->hostTime.store(time->mHostTime, std::memory_order_release);
    return noErr;
}

static bool
audioOutputCheck(BxAudioOutput* output, OSStatus status, const char* step) {
    if (status == noErr) {
        return true;
    }
    output->error = std::string(step) + " failed with status " + std::to_string(status);
    return false;
}

extern "C" void*
brixAudioOutputOpen(void* ringPtr, double sampleRate) {
    BxAudioOutput* output = new BxAudioOutput();
    output->ring = getRing(ringPtr);

    AudioComponentDescription description = {};
    description.componentType = kAudioUnitType_Output;
#if TARGET_OS_IPHONE
    description.componentSubType = kAudioUnitSubType_RemoteIO;
    if (!bxAudioSessionActivate(output->error)) {
        return output;
    }
#else
    description.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif
    description.componentManufacturer = kAudioUnitManufacturer_Apple;
    AudioComponent component = AudioComponentFindNext(nullptr, &description);
    if (!component) {
        output->error = "No default audio output";
        return output;
    }
    if (!audioOutputCheck(output, AudioComponentInstanceNew(component, &output->unit),
                          "Creating the output unit")) {
        output->unit = nullptr;
        return output;
    }

    AudioStreamBasicDescription format = {};
    format.mSampleRate = sampleRate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mChannelsPerFrame = static_cast<UInt32>(output->ring->width);
    format.mBitsPerChannel = 32;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = format.mChannelsPerFrame * sizeof(float);
    format.mBytesPerPacket = format.mBytesPerFrame;
    if (!audioOutputCheck(output,
                          AudioUnitSetProperty(output->unit, kAudioUnitProperty_StreamFormat,
                                               kAudioUnitScope_Input, 0, &format,
                                               sizeof(format)),
                          "Setting the stream format")) {
        return output;
    }

    AURenderCallbackStruct callback = {};
    callback.inputProc = audioOutputRender;
    callback.inputProcRefCon = output;
    if (!audioOutputCheck(output,
                          AudioUnitSetProperty(output->unit,
                                               kAudioUnitProperty_SetRenderCallback,
                                               kAudioUnitScope_Input, 0, &callback,
                                               sizeof(callback)),
                          "Setting the render callback")) {
        return output;
    }
    if (!audioOutputCheck(output, AudioUnitInitialize(output->unit),
                          "Initializing the output unit")) {
        return output;
    }
    if (!audioOutputCheck(output, AudioOutputUnitStart(output->unit),
                          "Starting the output unit")) {
        return output;
    }
    output->playing.store(true, std::memory_order_release);
#if TARGET_OS_IPHONE
    bxAudioSessionAttach(output->unit);
#endif
    return output;
}

extern "C" void
brixAudioOutputRelease(void* ptr) {
    BxAudioOutput* output = getOutput(ptr);
    if (!output) {
        return;
    }
    if (output->unit) {
#if TARGET_OS_IPHONE
        bxAudioSessionDetach(output->unit);
#endif
        if (output->playing.load(std::memory_order_acquire)) {
            AudioOutputUnitStop(output->unit);
        }
        AudioUnitUninitialize(output->unit);
        AudioComponentInstanceDispose(output->unit);
    }
    delete output;
}

extern "C" const char*
brixAudioOutputGetError(void* ptr) {
    return getOutput(ptr)->error.c_str();
}

extern "C" bool
brixAudioOutputIsPlaying(void* ptr) {
    return getOutput(ptr)->playing.load(std::memory_order_acquire);
}

extern "C" int64_t
brixAudioOutputGetFramesPlayed(void* ptr) {
    return getOutput(ptr)->framesPlayed.load(std::memory_order_relaxed);
}

extern "C" int64_t
brixAudioOutputGetUnderrunFrames(void* ptr) {
    return getOutput(ptr)->underrunFrames.load(std::memory_order_relaxed);
}

extern "C" int64_t
brixAudioOutputGetSliceFrames(void* ptr) {
    return getOutput(ptr)->sliceFrames.load(std::memory_order_relaxed);
}

extern "C" uint64_t
brixAudioOutputGetHostTime(void* ptr) {
    return getOutput(ptr)->hostTime.load(std::memory_order_acquire);
}
