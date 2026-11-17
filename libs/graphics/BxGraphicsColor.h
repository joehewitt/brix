#pragma once

#include <IOSurface/IOSurfaceRef.h>

#include <cstddef>
#include <cstdint>

enum : int32_t {
    BxGraphicsSpaceSRGB = 0,
    BxGraphicsSpaceDisplayP3 = 1,
    BxGraphicsSpaceLinearSRGB = 2,
    BxGraphicsSpaceLinearDisplayP3 = 3,
    BxGraphicsSpaceLinearRec2020 = 4,
    BxGraphicsSpaceCount = 5,
};

enum : int32_t {
    BxGraphicsFormatRGBA8 = 0,
    BxGraphicsFormatRGBA16Float = 1,
};

inline size_t
bxGraphicsFormatBytesPerPixel(int32_t format) {
    return format == BxGraphicsFormatRGBA16Float ? 8 : 4;
}

void* bxGraphicsGPUCreate(int32_t width, int32_t height, int32_t format);
void* bxGraphicsGPUCreateWithSurface(IOSurfaceRef surface);
void bxGraphicsGPURelease(void* store);
void* bxGraphicsGPUTexture(void* store, bool mipmaps);
bool bxGraphicsGPUUpload(void* store, const uint8_t* pixels, size_t bytesPerRow, const int32_t* region);
bool bxGraphicsGPUReadback(void* store, uint8_t* pixels, size_t bytesPerRow, const int32_t* region);
bool bxGraphicsGPURender(
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
bool bxGraphicsShaderFilter(
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
void* bxGraphicsShaderBatchCreate();
void bxGraphicsShaderBatchReset(void* batch);
void bxGraphicsShaderBatchRelease(void* batch);
size_t bxGraphicsShaderBatchBytes(void* batch);
bool bxGraphicsShaderBatchFinish(void* batch);
void* bxGraphicsShaderBatchImage(void* batch, int64_t job, void* colorSpace);
int64_t bxGraphicsShaderBatchPaint(
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
int64_t bxGraphicsShaderBatchDabs(
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
    const BxGraphicsDab* dabs,
    int64_t dabCount,
    const double* args,
    int64_t argCount,
    int32_t format,
    int32_t space
);
void* bxGraphicsShaderRaster(
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
