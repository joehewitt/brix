#ifndef BX_GRAPHICS_H
#define BX_GRAPHICS_H

#include <stdbool.h>
#include <stdint.h>

#include <BxGC/BxGCAnnotations.h>

// **********************************************************************************************

extern "C" void* brixGraphicsCanvasCreate(int32_t width, int32_t height);
extern "C" void* brixGraphicsCanvasCreateScaled(double width, double height, double scale);
extern "C" void* brixGraphicsCanvasCreateAlphaOnly(double width, double height, double scale);
extern "C" void* brixGraphicsCanvasCreateScaledWithColorSpace(
    double width,
    double height,
    double scale,
    void* colorSpace
);
extern "C" void* brixGraphicsCanvasCreateFormatted(
    int32_t width,
    int32_t height,
    double scale,
    int32_t space,
    int32_t format
);
extern "C" void* brixGraphicsCanvasCreateWithSurface(void* surface, double scale);
extern "C" BRIX_WITHOUT_GC void brixGraphicsCanvasSurfaceChanged(void* canvas);
extern "C" void* brixGraphicsSurfaceCreate(int32_t pixelWidth, int32_t pixelHeight);
extern "C" void brixGraphicsSurfaceRelease(void* surface);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsSurfaceGetPixelWidth(void* surface);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsSurfaceGetPixelHeight(void* surface);
extern "C" uint32_t brixGraphicsSurfaceCreateMachPort(void* surface);
extern "C" void* brixGraphicsSurfaceLookUpMachPort(uint32_t port);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsCanvasGetColorSpaceKind(void* canvas);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsCanvasGetPixelFormat(void* canvas);
extern "C" void brixGraphicsCanvasRelease(void* canvas);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsCanvasGetWidth(void* canvas);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsCanvasGetHeight(void* canvas);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsCanvasGetPixelWidth(void* canvas);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsCanvasGetPixelHeight(void* canvas);
extern "C" BRIX_WITHOUT_GC double brixGraphicsCanvasGetScale(void* canvas);
extern "C" void* brixGraphicsCanvasCreateCGImage(void* canvas);
extern "C" BRIX_WITHOUT_GC uint64_t brixGraphicsCanvasContentSerial(void* canvas);
extern "C" BRIX_WITHOUT_GC uint64_t brixGraphicsCanvasIdentity(void* canvas);
extern "C" BRIX_WITHOUT_GC void brixGraphicsCanvasObserveChanges(void* canvas);
extern "C" BRIX_WITHOUT_GC bool brixGraphicsCanvasChangedRegion(void* canvas, uint64_t since, int32_t* region);
extern "C" void brixGraphicsSetCanvasReleaseObserver(void (*observer)(uint64_t identity));
extern "C" void* brixGraphicsCanvasRetainPixelStore(void* canvas);
extern "C" void* brixGraphicsPixelStoreData(void* store);
extern "C" uint64_t brixGraphicsPixelStoreLength(void* store);
extern "C" void brixGraphicsPixelStoreRelease(void* store);
extern "C" BRIX_WITHOUT_GC void* brixGraphicsCanvasGetPixelData(void* canvas);
extern "C" BRIX_WITHOUT_GC uint64_t brixGraphicsCanvasGetBytesPerRow(void* canvas);
extern "C" BRIX_WITHOUT_GC int32_t brixGraphicsCanvasGetBytesPerPixel(void* canvas);
extern "C" BRIX_WITHOUT_GC void* brixGraphicsCanvasGetColorSpace(void* canvas);
extern "C" void brixGraphicsCanvasPixelsChanged(void* canvas);
extern "C" bool brixGraphicsCanvasWritePNG(void* canvas, const char* path, bool compact);
extern "C" void* brixGraphicsCanvasEncodePNG(void* canvas, bool compact);
extern "C" uint64_t brixGraphicsEncodedLength(void* encoded);
extern "C" void brixGraphicsEncodedCopy(void* encoded, uint8_t* bytes, uint64_t length);
extern "C" void brixGraphicsEncodedRelease(void* encoded);
extern "C" void* brixGraphicsCanvasEncodeJPEG(void* canvas, double quality);
extern "C" void* brixGraphicsImageDecodeData(
    const uint8_t* bytes,
    uint64_t length,
    int32_t maxPixelSize,
    int32_t space,
    int32_t format,
    bool prefersEmbedded
);
extern "C" bool brixGraphicsImageMeasureData(const uint8_t* bytes, uint64_t length, double* measures);
extern "C" void* brixGraphicsImageDecodeFile(
    const char* path,
    int32_t maxPixelSize,
    int32_t space,
    int32_t format,
    bool prefersEmbedded
);
extern "C" uint64_t brixGraphicsCanvasCountNonTransparentPixels(
    void* canvas,
    double x,
    double y,
    double width,
    double height
);
extern "C" bool brixGraphicsCanvasOpaqueBounds(void* canvas, int32_t minimumAlpha, double* bounds);
extern "C" uint64_t brixGraphicsCanvasCountDifferentPixels(void* canvas, void* otherCanvas);
extern "C" bool brixGraphicsCanvasCopyPixels(void* canvas, uint8_t* out, uint64_t size);
extern "C" bool brixGraphicsCanvasWritePixels(void* canvas, const uint8_t* in, uint64_t size);
extern "C" BRIX_WITHOUT_GC uint32_t brixGraphicsCanvasGetPixel(void* canvas, double x, double y);

// **********************************************************************************************

extern "C" BRIX_WITHOUT_GC void brixGraphicsSave(void* canvas);
extern "C" BRIX_WITHOUT_GC void brixGraphicsRestore(void* canvas);
extern "C" BRIX_WITHOUT_GC void brixGraphicsTransform(
    void* canvas,
    double a,
    double c,
    double b,
    double d,
    double tx,
    double ty
);
extern "C" BRIX_WITHOUT_GC void brixGraphicsTranslate(void* canvas, double x, double y);
extern "C" void brixGraphicsClipPath(void* canvas, int32_t fillRule);
extern "C" void brixGraphicsClearRect(void* canvas, double x, double y, double width, double height);
extern "C" BRIX_WITHOUT_GC void brixGraphicsSetBlendMode(void* canvas, int32_t mode);
extern "C" BRIX_WITHOUT_GC void brixGraphicsClipRect(
    void* canvas,
    double x,
    double y,
    double width,
    double height
);

// **********************************************************************************************

extern "C" void brixGraphicsClear(
    void* canvas,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" BRIX_WITHOUT_GC void brixGraphicsSetFillColor(
    void* canvas,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" BRIX_WITHOUT_GC void brixGraphicsSetStrokeColor(
    void* canvas,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" BRIX_WITHOUT_GC void brixGraphicsSetStrokeWidth(void* canvas, double width);
extern "C" BRIX_WITHOUT_GC void brixGraphicsSetStrokeCap(void* canvas, int32_t cap);
extern "C" BRIX_WITHOUT_GC void brixGraphicsSetStrokeJoin(void* canvas, int32_t join);
extern "C" BRIX_WITHOUT_GC void brixGraphicsSetStrokeDash(void* canvas, double length);
extern "C" void brixGraphicsFillGradient(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    int32_t kind,
    const double* geometry,
    const double* components,
    const double* locations,
    int64_t stopCount
);
extern "C" void brixGraphicsFillShader(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    uint32_t shader,
    const double* args,
    int64_t argCount
);
extern "C" void* brixGraphicsBeginFilter(void* canvas, double x, double y, double width, double height);
extern "C" void brixGraphicsEndFilter(
    void* canvas,
    void* layer,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    void* const* images,
    const double* imageScales,
    int64_t imageCount,
    double time,
    double opacity,
    int32_t blendMode
);
extern "C" const char* brixGraphicsShaderColor();
extern "C" const char* brixGraphicsShaderTypes();
extern "C" const char* brixGraphicsShaderArgsType();
extern "C" const char* brixGraphicsShaderInputTypes();
extern "C" const char* brixGraphicsShaderDabSource();
extern "C" void* brixGraphicsMetalDevice();
extern "C" void* brixGraphicsMetalCommandBuffer();
extern "C" bool brixGraphicsMetalReleaseIdleQueue();
extern "C" void* brixGraphicsCanvasGPUTexture(void* canvas, bool mipmaps);
extern "C" bool brixGraphicsCanvasIsGPUResident(void* canvas);
extern "C" bool brixGraphicsCanvasRender(
    void* canvas,
    double scale,
    double x,
    double y,
    double width,
    double height,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    void* const* images,
    const double* imageScales,
    int64_t imageCount
);
extern "C" uint32_t brixGraphicsShaderRegister(const char* source);
extern "C" const char* brixGraphicsShaderError(uint32_t shader);
extern "C" const char* brixGraphicsShaderSource(uint32_t shader);

// **********************************************************************************************

extern "C" void brixGraphicsDrawRect(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    bool fill,
    bool stroke
);
extern "C" void brixGraphicsDrawEllipse(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    bool fill,
    bool stroke
);
extern "C" void brixGraphicsDrawOutline(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    double radius,
    int32_t side,
    double offset,
    double size,
    bool fill,
    bool stroke
);
extern "C" void brixGraphicsClipOutline(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    double radius,
    int32_t side,
    double offset,
    double size
);
extern "C" void brixGraphicsDrawOutlineShadow(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    double radius,
    int32_t side,
    double offset,
    double size,
    double red,
    double green,
    double blue,
    double alpha,
    double blur,
    double offsetX,
    double offsetY,
    double spread
);
extern "C" void brixGraphicsDrawPathShadow(
    void* canvas,
    double red,
    double green,
    double blue,
    double alpha,
    double blur,
    double offsetX,
    double offsetY
);
extern "C" void brixGraphicsDrawImage(
    void* canvas,
    void* imageCanvas,
    double x,
    double y,
    double width,
    double height,
    bool templateAlpha,
    int32_t sampling
);
extern "C" void brixGraphicsFillImage(
    void* canvas,
    void* imageCanvas,
    double x,
    double y,
    double width,
    double height,
    double tileX,
    double tileY,
    double tileWidth,
    double tileHeight,
    int32_t sampling
);
extern "C" void* brixGraphicsBeginMask(void* canvas);
extern "C" void brixGraphicsEndMask(void* canvas, void* mask);
extern "C" void* brixGraphicsBeginBlur(void* canvas, double radius);
extern "C" void brixGraphicsEndBlur(void* canvas, void* layer, int32_t blendMode);
extern "C" void* brixGraphicsBeginOpacity(void* canvas);
extern "C" void brixGraphicsEndOpacity(void* canvas, void* layer, double opacity, int32_t blendMode);
extern "C" void brixGraphicsBackdropBlur(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    double radius
);
extern "C" void brixGraphicsBackdropShader(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double time,
    double radius
);
extern "C" void brixGraphicsClipMask(
    void* canvas,
    void* imageCanvas,
    double x,
    double y,
    double width,
    double height
);

// **********************************************************************************************

extern "C" BRIX_WITHOUT_GC void brixGraphicsBeginPath(void* canvas);
extern "C" BRIX_WITHOUT_GC bool brixGraphicsPathContainsPoint(void* canvas, double x, double y, int32_t fillRule);
extern "C" BRIX_WITHOUT_GC void brixGraphicsMoveTo(void* canvas, double x, double y);
extern "C" BRIX_WITHOUT_GC void brixGraphicsLineTo(void* canvas, double x, double y);
extern "C" BRIX_WITHOUT_GC void brixGraphicsCurveTo(
    void* canvas,
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3
);
extern "C" BRIX_WITHOUT_GC void brixGraphicsClosePath(void* canvas);
extern "C" BRIX_WITHOUT_GC void brixGraphicsAppendPath(
    void* canvas,
    const uint8_t* verbs,
    int64_t verbCount,
    const double* coords
);
extern "C" void brixGraphicsDrawPath(
    void* canvas,
    bool fill,
    bool stroke,
    int32_t fillRule
);
extern "C" void brixGraphicsFillPathShader(
    void* canvas,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double time,
    int32_t fillRule
);
extern "C" void brixGraphicsFillPathPaint(
    void* canvas,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double red,
    double green,
    double blue,
    double alpha,
    int32_t fillRule
);

typedef struct BxGraphicsDab {
    float x;
    float y;
    float angle;
    float distance;
    float length;
    float along;
} BxGraphicsDab;

extern "C" int64_t brixGraphicsBrushDabs(
    const uint8_t* verbs,
    int64_t verbCount,
    const double* coords,
    bool close,
    double width,
    double spacing,
    BxGraphicsDab* dabs,
    int64_t capacity,
    double* total
);
extern "C" void brixGraphicsStrokePathBrush(
    void* canvas,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double red,
    double green,
    double blue,
    double alpha,
    double width,
    bool fill,
    int32_t fillRule
);

// **********************************************************************************************

extern "C" void* brixGraphicsTextLayoutCreate(
    const char* text,
    const char* fontFamily,
    double fontSize,
    double weight,
    bool italic,
    bool hasMaximumWidth,
    double maximumWidth,
    bool hasMaximumHeight,
    double maximumHeight,
    int32_t truncation,
    bool wordWrap,
    double lineSpacing
);
extern "C" bool brixGraphicsTextLayoutSimpleMetrics(
    const char* text,
    const char* fontFamily,
    double fontSize,
    double weight,
    bool italic,
    bool hasMaximumWidth,
    int32_t truncation,
    bool wordWrap,
    double* metrics
);
extern "C" void brixGraphicsTextLayoutRelease(void* layout);

enum BxGraphicsTextPathVerb {
    BxGraphicsTextPathMove,
    BxGraphicsTextPathLine,
    BxGraphicsTextPathQuadratic,
    BxGraphicsTextPathCubic,
    BxGraphicsTextPathClose,
};

extern "C" int64_t brixGraphicsTextLayoutGetPathCommandCount(void* layout);
extern "C" void brixGraphicsTextLayoutGetPathCommands(
    void* layout,
    uint8_t* verbs,
    double* coordinates,
    int64_t capacity
);

extern "C" void* brixGraphicsStyledTextCreate(
    const char* fontFamily,
    double fontSize,
    double weight,
    bool italic
);
extern "C" void brixGraphicsStyledTextRelease(void* styled);
extern "C" void brixGraphicsStyledTextAppendText(
    void* styled,
    const char* text,
    const char* fontFamily,
    double fontSize,
    double weight,
    bool italic,
    bool hasColor,
    double red,
    double green,
    double blue,
    double alpha,
    int64_t tag
);
extern "C" void brixGraphicsStyledTextAppendObject(
    void* styled,
    double width,
    double ascent,
    double descent,
    int64_t tag,
    bool joinBefore,
    bool joinAfter
);
extern "C" void* brixGraphicsStyledTextLayoutCreate(
    void* styled,
    bool hasMaximumWidth,
    double maximumWidth,
    bool hasMaximumHeight,
    double maximumHeight,
    int32_t truncation,
    bool wordWrap,
    double lineSpacing
);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetFragmentCount(void* layout);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetFragmentTag(
    void* layout,
    int64_t index
);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetFragmentLine(
    void* layout,
    int64_t index
);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetFragmentX(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetFragmentWidth(
    void* layout,
    int64_t index
);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetFragmentAscent(
    void* layout,
    int64_t index
);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetFragmentDescent(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextFontCreatedCount(void);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetLastFontCreateMissCount(void);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetWidth(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetNaturalWidth(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetConstrainedWidth(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetHeight(void* layout);
extern "C" BRIX_WITHOUT_GC uint64_t brixGraphicsTextLayoutGetSerial(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetAscent(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetDescent(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLeading(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetBaseline(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLastBaseline(void* layout);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetLineCount(void* layout);
extern "C" BRIX_WITHOUT_GC bool brixGraphicsTextLayoutIsTruncated(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetTypographicX(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetTypographicY(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetInkX(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetInkY(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetInkWidth(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetInkHeight(void* layout);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineTypographicX(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineTypographicY(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineWidth(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineHeight(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetLineStart(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetLineEnd(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetLineForIndex(
    void* layout,
    int64_t index
);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetXForIndex(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetIndexAtPoint(void* layout, double x, double y);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineInkX(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineInkY(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineInkWidth(
    void* layout,
    int64_t index
);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineInkHeight(
    void* layout,
    int64_t index
);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineBaseline(
    void* layout,
    int64_t index
);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineAscent(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineDescent(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC double brixGraphicsTextLayoutGetLineLeading(void* layout, int64_t index);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetRunCount(void* layout);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetRunGlyphCount(void* layout, int64_t runIndex);
extern "C" BRIX_WITHOUT_GC int64_t brixGraphicsTextLayoutGetRunLine(void* layout, int64_t runIndex);
extern "C" BRIX_WITHOUT_GC void brixGraphicsTextLayoutGetRunGlyphs(
    void* layout,
    int64_t runIndex,
    uint16_t* glyphs,
    int64_t capacity
);
extern "C" BRIX_WITHOUT_GC void brixGraphicsTextLayoutGetRunPositions(
    void* layout,
    int64_t runIndex,
    double* xyPairs,
    int64_t capacity
);
// Borrowed CTFontRef. Callers retaining data beyond this operation must retain the font.
extern "C" BRIX_WITHOUT_GC void* brixGraphicsTextLayoutGetRunFont(void* layout, int64_t runIndex);
extern "C" BRIX_WITHOUT_GC bool brixGraphicsTextLayoutIsRunObject(void* layout, int64_t runIndex);
extern "C" BRIX_WITHOUT_GC bool brixGraphicsTextLayoutGetRunColor(void* layout, int64_t runIndex, uint32_t* rgba8);
extern "C" void brixGraphicsDrawTextLayout(
    void* canvas,
    void* layout,
    double x,
    double y,
    double width,
    double height,
    int32_t alignment,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" void brixGraphicsDrawTextLayoutShader(
    void* canvas,
    void* layout,
    double x,
    double y,
    double width,
    double height,
    int32_t alignment,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double time
);
extern "C" void* brixGraphicsFontRegister(const char* path);
extern "C" void brixGraphicsFontRegistrationRelease(void* registration);
extern "C" bool brixGraphicsFontRegistrationIsValid(void* registration);
extern "C" const char* brixGraphicsFontRegistrationGetFamily(void* registration);

#endif
