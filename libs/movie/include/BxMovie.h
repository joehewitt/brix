#ifndef BX_MOVIE_H
#define BX_MOVIE_H

#include <stdbool.h>
#include <stdint.h>

#include <BxGC/BxGCAnnotations.h>

// **********************************************************************************************

enum {
    kBxMovieClosed = 0,
    kBxMovieWriting = 1,
    kBxMovieSaving = 2,
    kBxMovieSaved = 3,
    kBxMovieFailed = 4,
};

extern "C" void* brixMovieCreate(const char* path, int32_t width, int32_t height, double frameRate);
extern "C" void brixMovieRelease(void* movie);
extern "C" uint32_t brixMovieGetStatus(void* movie);
extern "C" BRIX_WITHOUT_GC bool brixMovieIsReady(void* movie);
extern "C" BRIX_WITHOUT_GC int64_t brixMovieGetFrameCount(void* movie);
extern "C" const char* brixMovieGetError(void* movie);
extern "C" bool brixMovieAppendCanvas(void* movie, void* canvas);
extern "C" bool brixMovieFinish(void* movie);
extern "C" void brixMovieCancel(void* movie);

#endif
