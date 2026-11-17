#ifndef YU_MOVIE_H
#define YU_MOVIE_H

#include <stdbool.h>
#include <stdint.h>

#include <YuGC/YuGCAnnotations.h>

// **********************************************************************************************

enum {
    kYuMovieClosed = 0,
    kYuMovieWriting = 1,
    kYuMovieSaving = 2,
    kYuMovieSaved = 3,
    kYuMovieFailed = 4,
};

extern "C" void* yumiMovieCreate(const char* path, int32_t width, int32_t height, double frameRate);
extern "C" void yumiMovieRelease(void* movie);
extern "C" uint32_t yumiMovieGetStatus(void* movie);
extern "C" YUMI_WITHOUT_GC bool yumiMovieIsReady(void* movie);
extern "C" YUMI_WITHOUT_GC int64_t yumiMovieGetFrameCount(void* movie);
extern "C" const char* yumiMovieGetError(void* movie);
extern "C" bool yumiMovieAppendCanvas(void* movie, void* canvas);
extern "C" bool yumiMovieFinish(void* movie);
extern "C" void yumiMovieCancel(void* movie);

#endif
