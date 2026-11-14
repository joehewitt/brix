#pragma once

#include <IOSurface/IOSurfaceRef.h>

#include <cstddef>
#include <cstdint>

enum : int32_t {
    YuGraphicsSpaceSRGB = 0,
    YuGraphicsSpaceDisplayP3 = 1,
    YuGraphicsSpaceLinearSRGB = 2,
    YuGraphicsSpaceLinearDisplayP3 = 3,
    YuGraphicsSpaceLinearRec2020 = 4,
    YuGraphicsSpaceCount = 5,
};

enum : int32_t {
    YuGraphicsFormatRGBA8 = 0,
    YuGraphicsFormatRGBA16Float = 1,
};

inline size_t
yuGraphicsFormatBytesPerPixel(int32_t format) {
    return format == YuGraphicsFormatRGBA16Float ? 8 : 4;
}

void* yuGraphicsGPUCreate(int32_t width, int32_t height, int32_t format);
void* yuGraphicsGPUCreateWithSurface(IOSurfaceRef surface);
void yuGraphicsGPURelease(void* store);
void* yuGraphicsGPUTexture(void* store, bool mipmaps);
bool yuGraphicsGPUUpload(void* store, const uint8_t* pixels, size_t bytesPerRow, const int32_t* region);
bool yuGraphicsGPUReadback(void* store, uint8_t* pixels, size_t bytesPerRow, const int32_t* region);
bool yuGraphicsGPURender(
    void* store,
    const int32_t* region,
    double scale,
    const double* size,
    int32_t space,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    void* const* images,
    const double* frames,
    int32_t imageCount
);
bool yuGraphicsShaderFilter(
    uint32_t shader,
    uint8_t* pixels,
    int32_t columns,
    int32_t rows,
    size_t bytesPerRow,
    int32_t format,
    int32_t space,
    const double* origin,
    const double* size,
    double scale,
    const double* args,
    int64_t argCount,
    void* const* images,
    const double* frames,
    int32_t imageCount,
    double time
);
void* yuGraphicsShaderBatchCreate();
void yuGraphicsShaderBatchReset(void* batch);
void yuGraphicsShaderBatchRelease(void* batch);
size_t yuGraphicsShaderBatchBytes(void* batch);
bool yuGraphicsShaderBatchFinish(void* batch);
void* yuGraphicsShaderBatchImage(void* batch, int64_t job, void* colorSpace);
int64_t yuGraphicsShaderBatchPaint(
    void* batch,
    uint32_t shader,
    int32_t columns,
    int32_t rows,
    const double* local,
    const double* paper,
    double pixel,
    const double* color,
    const double* args,
    int64_t argCount,
    int32_t format,
    int32_t space
);
int64_t yuGraphicsShaderBatchDabs(
    void* batch,
    uint32_t shader,
    int32_t columns,
    int32_t rows,
    const double* raster,
    const double* paper,
    const double* color,
    const double* style,
    double width,
    double total,
    const YuGraphicsDab* dabs,
    int64_t dabCount,
    const double* args,
    int64_t argCount,
    int32_t format,
    int32_t space
);
void* yuGraphicsShaderRaster(
    uint32_t shader,
    int32_t columns,
    int32_t rows,
    const double* transform,
    double width,
    double height,
    const double* args,
    int64_t argCount,
    double time,
    int32_t format,
    int32_t space,
    void* colorSpace
);
