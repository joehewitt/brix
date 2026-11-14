#ifndef YU_GRAPHICS_H
#define YU_GRAPHICS_H

#include <stdbool.h>
#include <stdint.h>

#include <YuGC/YuGCAnnotations.h>

// **********************************************************************************************

extern "C" void* yumiGraphicsCanvasCreate(int32_t width, int32_t height);
extern "C" void* yumiGraphicsCanvasCreateScaled(double width, double height, double scale);
extern "C" void* yumiGraphicsCanvasCreateAlphaOnly(double width, double height, double scale);
extern "C" void* yumiGraphicsCanvasCreateScaledWithColorSpace(
    double width,
    double height,
    double scale,
    void* colorSpace
);
extern "C" void* yumiGraphicsCanvasCreateFormatted(
    int32_t width,
    int32_t height,
    double scale,
    int32_t space,
    int32_t format
);
extern "C" void* yumiGraphicsCanvasCreateWithSurface(void* surface, double scale);
extern "C" YUMI_GC_LEAF void yumiGraphicsCanvasSurfaceChanged(void* canvas);
extern "C" void* yumiGraphicsSurfaceCreate(int32_t pixelWidth, int32_t pixelHeight);
extern "C" void yumiGraphicsSurfaceRelease(void* surface);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsSurfaceGetPixelWidth(void* surface);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsSurfaceGetPixelHeight(void* surface);
extern "C" uint32_t yumiGraphicsSurfaceCreateMachPort(void* surface);
extern "C" void* yumiGraphicsSurfaceLookUpMachPort(uint32_t port);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsCanvasGetColorSpaceKind(void* canvas);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsCanvasGetPixelFormat(void* canvas);
extern "C" void yumiGraphicsCanvasRelease(void* canvas);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsCanvasGetWidth(void* canvas);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsCanvasGetHeight(void* canvas);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsCanvasGetPixelWidth(void* canvas);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsCanvasGetPixelHeight(void* canvas);
extern "C" YUMI_GC_LEAF double yumiGraphicsCanvasGetScale(void* canvas);
extern "C" void* yumiGraphicsCanvasCreateCGImage(void* canvas);
extern "C" YUMI_GC_LEAF uint64_t yumiGraphicsCanvasContentSerial(void* canvas);
extern "C" YUMI_GC_LEAF uint64_t yumiGraphicsCanvasIdentity(void* canvas);
extern "C" YUMI_GC_LEAF void yumiGraphicsCanvasObserveChanges(void* canvas);
extern "C" YUMI_GC_LEAF bool yumiGraphicsCanvasChangedRegion(void* canvas, uint64_t since, int32_t* region);
extern "C" void yumiGraphicsSetCanvasReleaseObserver(void (*observer)(uint64_t identity));
extern "C" void* yumiGraphicsCanvasRetainPixelStore(void* canvas);
extern "C" void* yumiGraphicsPixelStoreData(void* store);
extern "C" uint64_t yumiGraphicsPixelStoreLength(void* store);
extern "C" void yumiGraphicsPixelStoreRelease(void* store);
extern "C" YUMI_GC_LEAF void* yumiGraphicsCanvasGetPixelData(void* canvas);
extern "C" YUMI_GC_LEAF uint64_t yumiGraphicsCanvasGetBytesPerRow(void* canvas);
extern "C" YUMI_GC_LEAF int32_t yumiGraphicsCanvasGetBytesPerPixel(void* canvas);
extern "C" YUMI_GC_LEAF void* yumiGraphicsCanvasGetColorSpace(void* canvas);
extern "C" void yumiGraphicsCanvasPixelsChanged(void* canvas);
extern "C" bool yumiGraphicsCanvasWritePNG(void* canvas, const char* path, bool compact);
extern "C" void* yumiGraphicsCanvasEncodePNG(void* canvas, bool compact);
extern "C" uint64_t yumiGraphicsEncodedLength(void* encoded);
extern "C" void yumiGraphicsEncodedCopy(void* encoded, uint8_t* bytes, uint64_t length);
extern "C" void yumiGraphicsEncodedRelease(void* encoded);
extern "C" void* yumiGraphicsCanvasEncodeJPEG(void* canvas, double quality);
extern "C" void* yumiGraphicsImageDecodeData(
    const uint8_t* bytes,
    uint64_t length,
    int32_t maxPixelSize,
    int32_t space,
    int32_t format,
    bool prefersEmbedded
);
extern "C" bool yumiGraphicsImageMeasureData(const uint8_t* bytes, uint64_t length, double* measures);
extern "C" void* yumiGraphicsImageDecodeFile(
    const char* path,
    int32_t maxPixelSize,
    int32_t space,
    int32_t format,
    bool prefersEmbedded
);
extern "C" uint64_t yumiGraphicsCanvasCountNonTransparentPixels(
    void* canvas,
    double x,
    double y,
    double width,
    double height
);
extern "C" bool yumiGraphicsCanvasOpaqueBounds(void* canvas, int32_t minimumAlpha, double* bounds);
extern "C" uint64_t yumiGraphicsCanvasCountDifferentPixels(void* canvas, void* otherCanvas);
extern "C" bool yumiGraphicsCanvasCopyPixels(void* canvas, uint8_t* out, uint64_t size);
extern "C" bool yumiGraphicsCanvasWritePixels(void* canvas, const uint8_t* in, uint64_t size);
extern "C" YUMI_GC_LEAF uint32_t yumiGraphicsCanvasGetPixel(void* canvas, double x, double y);

// **********************************************************************************************

extern "C" YUMI_GC_LEAF void yumiGraphicsSave(void* canvas);
extern "C" YUMI_GC_LEAF void yumiGraphicsRestore(void* canvas);
extern "C" YUMI_GC_LEAF void yumiGraphicsTransform(
    void* canvas,
    double a,
    double c,
    double b,
    double d,
    double tx,
    double ty
);
extern "C" YUMI_GC_LEAF void yumiGraphicsTranslate(void* canvas, double x, double y);
extern "C" void yumiGraphicsClipPath(void* canvas, int32_t fillRule);
extern "C" void yumiGraphicsClearRect(void* canvas, double x, double y, double width, double height);
extern "C" YUMI_GC_LEAF void yumiGraphicsSetBlendMode(void* canvas, int32_t mode);
extern "C" YUMI_GC_LEAF void yumiGraphicsClipRect(
    void* canvas,
    double x,
    double y,
    double width,
    double height
);

// **********************************************************************************************

extern "C" void yumiGraphicsClear(
    void* canvas,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" YUMI_GC_LEAF void yumiGraphicsSetFillColor(
    void* canvas,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" YUMI_GC_LEAF void yumiGraphicsSetStrokeColor(
    void* canvas,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" YUMI_GC_LEAF void yumiGraphicsSetStrokeWidth(void* canvas, double width);
extern "C" YUMI_GC_LEAF void yumiGraphicsSetStrokeCap(void* canvas, int32_t cap);
extern "C" YUMI_GC_LEAF void yumiGraphicsSetStrokeJoin(void* canvas, int32_t join);
extern "C" YUMI_GC_LEAF void yumiGraphicsSetStrokeDash(void* canvas, double length);
extern "C" void yumiGraphicsFillGradient(
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
extern "C" void yumiGraphicsFillShader(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    uint32_t shader,
    const double* args,
    int64_t argCount
);
extern "C" void* yumiGraphicsBeginFilter(void* canvas, double x, double y, double width, double height);
extern "C" void yumiGraphicsEndFilter(
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
extern "C" const char* yumiGraphicsShaderColor();
extern "C" const char* yumiGraphicsShaderTypes();
extern "C" const char* yumiGraphicsShaderArgsType();
extern "C" const char* yumiGraphicsShaderInputTypes();
extern "C" const char* yumiGraphicsShaderDabSource();
extern "C" void* yumiGraphicsMetalDevice();
extern "C" void* yumiGraphicsMetalQueue();
extern "C" void* yumiGraphicsCanvasGPUTexture(void* canvas, bool mipmaps);
extern "C" bool yumiGraphicsCanvasIsGPUResident(void* canvas);
extern "C" bool yumiGraphicsCanvasRender(
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
extern "C" uint32_t yumiGraphicsShaderRegister(const char* source);
extern "C" const char* yumiGraphicsShaderError(uint32_t shader);
extern "C" const char* yumiGraphicsShaderSource(uint32_t shader);

// **********************************************************************************************

extern "C" void yumiGraphicsDrawRect(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    bool fill,
    bool stroke
);
extern "C" void yumiGraphicsDrawEllipse(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    bool fill,
    bool stroke
);
extern "C" void yumiGraphicsDrawOutline(
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
extern "C" void yumiGraphicsClipOutline(
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
extern "C" void yumiGraphicsDrawOutlineShadow(
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
extern "C" void yumiGraphicsDrawPathShadow(
    void* canvas,
    double red,
    double green,
    double blue,
    double alpha,
    double blur,
    double offsetX,
    double offsetY
);
extern "C" void yumiGraphicsDrawImage(
    void* canvas,
    void* imageCanvas,
    double x,
    double y,
    double width,
    double height,
    bool templateAlpha,
    int32_t sampling
);
extern "C" void yumiGraphicsFillImage(
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
extern "C" void* yumiGraphicsBeginMask(void* canvas);
extern "C" void yumiGraphicsEndMask(void* canvas, void* mask);
extern "C" void* yumiGraphicsBeginBlur(void* canvas, double radius);
extern "C" void yumiGraphicsEndBlur(void* canvas, void* layer, int32_t blendMode);
extern "C" void* yumiGraphicsBeginOpacity(void* canvas);
extern "C" void yumiGraphicsEndOpacity(void* canvas, void* layer, double opacity, int32_t blendMode);
extern "C" void yumiGraphicsBackdropBlur(
    void* canvas,
    double x,
    double y,
    double width,
    double height,
    double radius
);
extern "C" void yumiGraphicsBackdropShader(
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
extern "C" void yumiGraphicsClipMask(
    void* canvas,
    void* imageCanvas,
    double x,
    double y,
    double width,
    double height
);

// **********************************************************************************************

extern "C" YUMI_GC_LEAF void yumiGraphicsBeginPath(void* canvas);
extern "C" YUMI_GC_LEAF bool yumiGraphicsPathContainsPoint(void* canvas, double x, double y, int32_t fillRule);
extern "C" YUMI_GC_LEAF void yumiGraphicsMoveTo(void* canvas, double x, double y);
extern "C" YUMI_GC_LEAF void yumiGraphicsLineTo(void* canvas, double x, double y);
extern "C" YUMI_GC_LEAF void yumiGraphicsCurveTo(
    void* canvas,
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3
);
extern "C" YUMI_GC_LEAF void yumiGraphicsClosePath(void* canvas);
extern "C" YUMI_GC_LEAF void yumiGraphicsAppendPath(
    void* canvas,
    const uint8_t* verbs,
    int64_t verbCount,
    const double* coords
);
extern "C" void yumiGraphicsDrawPath(
    void* canvas,
    bool fill,
    bool stroke,
    int32_t fillRule
);
extern "C" void yumiGraphicsFillPathShader(
    void* canvas,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double time,
    int32_t fillRule
);
extern "C" void yumiGraphicsFillPathPaint(
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

typedef struct YuGraphicsDab {
    float x;
    float y;
    float angle;
    float distance;
    float length;
    float along;
} YuGraphicsDab;

extern "C" int64_t yumiGraphicsBrushDabs(
    const uint8_t* verbs,
    int64_t verbCount,
    const double* coords,
    bool close,
    double width,
    double spacing,
    YuGraphicsDab* dabs,
    int64_t capacity,
    double* total
);
extern "C" void yumiGraphicsStrokePathBrush(
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

extern "C" void* yumiGraphicsTextLayoutCreate(
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
extern "C" bool yumiGraphicsTextLayoutSimpleMetrics(
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
extern "C" void yumiGraphicsTextLayoutRelease(void* layout);

enum YuGraphicsTextPathVerb {
    YuGraphicsTextPathMove,
    YuGraphicsTextPathLine,
    YuGraphicsTextPathQuadratic,
    YuGraphicsTextPathCubic,
    YuGraphicsTextPathClose,
};

extern "C" int64_t yumiGraphicsTextLayoutGetPathCommandCount(void* layout);
extern "C" void yumiGraphicsTextLayoutGetPathCommands(
    void* layout,
    uint8_t* verbs,
    double* coordinates,
    int64_t capacity
);

extern "C" void* yumiGraphicsStyledTextCreate(
    const char* fontFamily,
    double fontSize,
    double weight,
    bool italic
);
extern "C" void yumiGraphicsStyledTextRelease(void* styled);
extern "C" void yumiGraphicsStyledTextAppendText(
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
extern "C" void yumiGraphicsStyledTextAppendObject(
    void* styled,
    double width,
    double ascent,
    double descent,
    int64_t tag,
    bool joinBefore,
    bool joinAfter
);
extern "C" void* yumiGraphicsStyledTextLayoutCreate(
    void* styled,
    bool hasMaximumWidth,
    double maximumWidth,
    bool hasMaximumHeight,
    double maximumHeight,
    int32_t truncation,
    bool wordWrap,
    double lineSpacing
);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetFragmentCount(void* layout);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetFragmentTag(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetFragmentLine(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetFragmentX(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetFragmentWidth(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetFragmentAscent(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetFragmentDescent(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextFontCreatedCount(void);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetLastFontCreateMissCount(void);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetWidth(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetNaturalWidth(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetConstrainedWidth(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetHeight(void* layout);
extern "C" YUMI_GC_LEAF uint64_t yumiGraphicsTextLayoutGetSerial(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetAscent(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetDescent(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLeading(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetBaseline(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLastBaseline(void* layout);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetLineCount(void* layout);
extern "C" YUMI_GC_LEAF bool yumiGraphicsTextLayoutIsTruncated(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetTypographicX(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetTypographicY(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetInkX(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetInkY(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetInkWidth(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetInkHeight(void* layout);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineTypographicX(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineTypographicY(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineWidth(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineHeight(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetLineStart(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetLineEnd(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetLineForIndex(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetXForIndex(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetIndexAtPoint(void* layout, double x, double y);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineInkX(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineInkY(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineInkWidth(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineInkHeight(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineBaseline(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineAscent(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineDescent(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF double yumiGraphicsTextLayoutGetLineLeading(void* layout, int64_t index);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetRunCount(void* layout);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetRunGlyphCount(void* layout, int64_t runIndex);
extern "C" YUMI_GC_LEAF int64_t yumiGraphicsTextLayoutGetRunLine(void* layout, int64_t runIndex);
extern "C" YUMI_GC_LEAF void yumiGraphicsTextLayoutGetRunGlyphs(
    void* layout,
    int64_t runIndex,
    uint16_t* glyphs,
    int64_t capacity
);
extern "C" YUMI_GC_LEAF void yumiGraphicsTextLayoutGetRunPositions(
    void* layout,
    int64_t runIndex,
    double* xyPairs,
    int64_t capacity
);
// Borrowed CTFontRef. Callers retaining data beyond this operation must retain the font.
extern "C" YUMI_GC_LEAF void* yumiGraphicsTextLayoutGetRunFont(void* layout, int64_t runIndex);
extern "C" YUMI_GC_LEAF bool yumiGraphicsTextLayoutIsRunObject(void* layout, int64_t runIndex);
extern "C" YUMI_GC_LEAF bool yumiGraphicsTextLayoutGetRunColor(void* layout, int64_t runIndex, uint32_t* rgba8);
extern "C" void yumiGraphicsDrawTextLayout(
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
extern "C" void yumiGraphicsDrawTextLayoutShader(
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
extern "C" void* yumiGraphicsFontRegister(const char* path);
extern "C" void yumiGraphicsFontRegistrationRelease(void* registration);
extern "C" bool yumiGraphicsFontRegistrationIsValid(void* registration);
extern "C" const char* yumiGraphicsFontRegistrationGetFamily(void* registration);

#endif
