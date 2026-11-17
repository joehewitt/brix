#include "include/BxMovie.h"
#include "BxGraphics/BxGraphics.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#include <CoreGraphics/CoreGraphics.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <string>
#include <unistd.h>

// **********************************************************************************************

static const int32_t kMovieTicksPerFrame = 600;
static const useconds_t kMovieBackpressurePause = 1000;
static const int32_t kMovieBackpressureLimit = 30000;
static const double kMovieBitsPerPixel = 0.2;

struct BxMovie {
    AVAssetWriter* writer = nil;
    AVAssetWriterInput* input = nil;
    AVAssetWriterInputPixelBufferAdaptor* adaptor = nil;
    CGColorSpaceRef colorSpace = nullptr;
    std::shared_ptr<std::atomic<bool>> written;
    int32_t width = 0;
    int32_t height = 0;
    int32_t timescale = 0;
    int64_t frameCount = 0;
    uint32_t status = kBxMovieClosed;
    std::string error;
};

static BxMovie*
getMovie(void* ptr) {
    return static_cast<BxMovie*>(ptr);
}

static void
movieClose(BxMovie* movie) {
    [movie->adaptor release];
    [movie->input release];
    [movie->writer release];
    movie->adaptor = nil;
    movie->input = nil;
    movie->writer = nil;
}

static void
movieFail(BxMovie* movie, NSString* fallback) {
    NSError* failure = [movie->writer error];
    NSString* text = [failure localizedDescription] ?: fallback;
    movie->error = [text UTF8String] ?: "";
    movie->status = kBxMovieFailed;
}

static void
movieDiscard(BxMovie* movie) {
    NSURL* url = [[movie->writer outputURL] retain];
    if ([movie->writer status] == AVAssetWriterStatusWriting) {
        [movie->writer cancelWriting];
    }
    movieClose(movie);
    if (url) {
        [[NSFileManager defaultManager] removeItemAtURL:url error:nil];
    }
    [url release];
}

static void
movieSettle(BxMovie* movie) {
    if (movie->status != kBxMovieSaving || !movie->written || !movie->written->load()) {
        return;
    }
    if ([movie->writer status] == AVAssetWriterStatusCompleted) {
        movie->status = kBxMovieSaved;
        movieClose(movie);
    } else {
        movieFail(movie, @"The movie could not be saved");
        movieDiscard(movie);
    }
}

// **********************************************************************************************

extern "C" void*
brixMovieCreate(const char* path, int32_t width, int32_t height, double frameRate) {
    BxMovie* movie = new BxMovie();
    double rate = std::max(frameRate, 1.0);
    movie->width = width;
    movie->height = height;
    movie->timescale = static_cast<int32_t>(lround(rate * kMovieTicksPerFrame));
    movie->colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    movie->status = kBxMovieFailed;

    @autoreleasepool {
        if (!path || !path[0] || width < 2 || height < 2 || (width % 2) || (height % 2)) {
            movie->error = "A movie needs a path and an even width and height";
            return movie;
        }
        NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
        [[NSFileManager defaultManager] removeItemAtURL:url error:nil];

        NSError* failure = nil;
        movie->writer = [[AVAssetWriter alloc] initWithURL:url fileType:AVFileTypeMPEG4 error:&failure];
        if (!movie->writer) {
            movie->error = [[failure localizedDescription] UTF8String] ?: "The movie file could not be created";
            return movie;
        }
        movie->writer.shouldOptimizeForNetworkUse = YES;

        NSDictionary* compression = @{
            AVVideoAverageBitRateKey: @(static_cast<double>(width) * height * rate * kMovieBitsPerPixel),
            AVVideoMaxKeyFrameIntervalDurationKey: @2.0,
            AVVideoProfileLevelKey: AVVideoProfileLevelH264HighAutoLevel,
        };
        NSDictionary* color = @{
            AVVideoColorPrimariesKey: AVVideoColorPrimaries_ITU_R_709_2,
            AVVideoTransferFunctionKey: AVVideoTransferFunction_ITU_R_709_2,
            AVVideoYCbCrMatrixKey: AVVideoYCbCrMatrix_ITU_R_709_2,
        };
        NSDictionary* settings = @{
            AVVideoCodecKey: AVVideoCodecTypeH264,
            AVVideoWidthKey: @(width),
            AVVideoHeightKey: @(height),
            AVVideoCompressionPropertiesKey: compression,
            AVVideoColorPropertiesKey: color,
        };
        if (![movie->writer canApplyOutputSettings:settings forMediaType:AVMediaTypeVideo]) {
            movie->error = "This size cannot be encoded as H.264";
            movieClose(movie);
            return movie;
        }
        movie->input = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:settings];
        movie->input.expectsMediaDataInRealTime = NO;
        NSDictionary* buffers = @{
            (NSString*)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
            (NSString*)kCVPixelBufferWidthKey: @(width),
            (NSString*)kCVPixelBufferHeightKey: @(height),
        };
        movie->adaptor = [[AVAssetWriterInputPixelBufferAdaptor alloc]
            initWithAssetWriterInput:movie->input
            sourcePixelBufferAttributes:buffers];
        [movie->writer addInput:movie->input];
        if (![movie->writer startWriting]) {
            movieFail(movie, @"The movie could not be started");
            movieDiscard(movie);
            return movie;
        }
        [movie->writer startSessionAtSourceTime:kCMTimeZero];
        movie->status = kBxMovieWriting;
    }
    return movie;
}

extern "C" void
brixMovieRelease(void* ptr) {
    BxMovie* movie = getMovie(ptr);
    if (!movie) {
        return;
    }
    brixMovieCancel(movie);
    CGColorSpaceRelease(movie->colorSpace);
    delete movie;
}

extern "C" uint32_t
brixMovieGetStatus(void* ptr) {
    BxMovie* movie = getMovie(ptr);
    if (!movie) {
        return kBxMovieClosed;
    }
    @autoreleasepool {
        movieSettle(movie);
    }
    return movie->status;
}

extern "C" bool
brixMovieIsReady(void* ptr) {
    BxMovie* movie = getMovie(ptr);
    if (!movie || movie->status != kBxMovieWriting) {
        return false;
    }
    @autoreleasepool {
        return [movie->input isReadyForMoreMediaData];
    }
}

extern "C" int64_t
brixMovieGetFrameCount(void* ptr) {
    BxMovie* movie = getMovie(ptr);
    return movie ? movie->frameCount : 0;
}

extern "C" const char*
brixMovieGetError(void* ptr) {
    BxMovie* movie = getMovie(ptr);
    return movie ? movie->error.c_str() : "";
}

extern "C" bool
brixMovieAppendCanvas(void* ptr, void* canvas) {
    BxMovie* movie = getMovie(ptr);
    if (!movie || movie->status != kBxMovieWriting) {
        return false;
    }

    @autoreleasepool {
        int32_t waited = 0;
        while (![movie->input isReadyForMoreMediaData]) {
            if ([movie->writer status] != AVAssetWriterStatusWriting || waited >= kMovieBackpressureLimit) {
                movieFail(movie, @"The encoder stopped accepting frames");
                movieDiscard(movie);
                return false;
            }
            usleep(kMovieBackpressurePause);
            waited += 1;
        }

        CGImageRef image = static_cast<CGImageRef>(brixGraphicsCanvasCreateCGImage(canvas));
        if (!image) {
            movieFail(movie, @"The frame has no pixels");
            movieDiscard(movie);
            return false;
        }

        CVPixelBufferRef buffer = nullptr;
        CVPixelBufferPoolRef pool = [movie->adaptor pixelBufferPool];
        if (!pool || CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pool, &buffer) != kCVReturnSuccess) {
            CGImageRelease(image);
            movieFail(movie, @"The encoder has no room for a frame");
            movieDiscard(movie);
            return false;
        }

        CVPixelBufferLockBaseAddress(buffer, 0);
        CGContextRef context = CGBitmapContextCreate(
            CVPixelBufferGetBaseAddress(buffer),
            movie->width,
            movie->height,
            8,
            CVPixelBufferGetBytesPerRow(buffer),
            movie->colorSpace,
            static_cast<uint32_t>(kCGImageAlphaNoneSkipFirst) | static_cast<uint32_t>(kCGBitmapByteOrder32Little));
        CGRect frame = CGRectMake(0, 0, movie->width, movie->height);
        CGContextSetRGBFillColor(context, 1, 1, 1, 1);
        CGContextFillRect(context, frame);
        CGContextSetInterpolationQuality(context, kCGInterpolationHigh);
        CGContextDrawImage(context, frame, image);
        CGContextRelease(context);
        CVPixelBufferUnlockBaseAddress(buffer, 0);
        CGImageRelease(image);

        CMTime time = CMTimeMake(movie->frameCount * kMovieTicksPerFrame, movie->timescale);
        BOOL appended = [movie->adaptor appendPixelBuffer:buffer withPresentationTime:time];
        CVPixelBufferRelease(buffer);
        if (!appended) {
            movieFail(movie, @"A frame could not be encoded");
            movieDiscard(movie);
            return false;
        }
        movie->frameCount += 1;
    }
    return true;
}

extern "C" bool
brixMovieFinish(void* ptr) {
    BxMovie* movie = getMovie(ptr);
    if (!movie || movie->status != kBxMovieWriting) {
        return false;
    }

    @autoreleasepool {
        std::shared_ptr<std::atomic<bool>> written = std::make_shared<std::atomic<bool>>(false);
        movie->written = written;
        movie->status = kBxMovieSaving;
        [movie->input markAsFinished];
        [movie->writer endSessionAtSourceTime:CMTimeMake(movie->frameCount * kMovieTicksPerFrame, movie->timescale)];
        [movie->writer finishWritingWithCompletionHandler:^{
            written->store(true);
        }];
    }
    return true;
}

extern "C" void
brixMovieCancel(void* ptr) {
    BxMovie* movie = getMovie(ptr);
    if (!movie) {
        return;
    }

    @autoreleasepool {
        int32_t waited = 0;
        while (movie->status == kBxMovieSaving && !movie->written->load() && waited < kMovieBackpressureLimit) {
            usleep(kMovieBackpressurePause);
            waited += 1;
        }
        if (movie->status == kBxMovieWriting || movie->status == kBxMovieSaving) {
            movieDiscard(movie);
            movie->status = kBxMovieClosed;
        }
    }
}
