#ifndef YU_METAL_RENDERER_H
#define YU_METAL_RENDERER_H

#include <stdbool.h>
#include <stdint.h>

// Scalar-only retained display item. This layout mirrors gui/scene:DisplayItem and is the
// production bulk-encode seam: no managed reference, callback, or scene pointer can appear here.
typedef struct YuGuiMetalDisplayItem {
    uint8_t kind;
    uint8_t flags;
    uint16_t aux;
    float x;
    float y;
    float width;
    float height;
    float value;
    uint32_t data0;
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
} YuGuiMetalDisplayItem;

typedef struct YuGuiMetalShape {
    float radius;
    float side;
    float offset;
    float size;
    float blur;
    float x;
    float y;
    float spread;
} YuGuiMetalShape;

typedef struct YuGuiMetalRect {
    double x;
    double y;
    double width;
    double height;
} YuGuiMetalRect;

typedef struct YuGuiMetalState {
    uint32_t parent;
    uint32_t kind;
    double matrix[6];
    double rect[4];
    double opacity;
    double radius;
    double side;
    double offset;
    double size;
    uint32_t pathStart;
    uint32_t pathCount;
    uint32_t coordStart;
    uint32_t fillRule;
    uint64_t serial;
    uint32_t source;
    int32_t blend;
} YuGuiMetalState;

typedef struct YuGuiMetalPathStyle {
    double fill[4];
    double stroke[4];
    double strokeWidth;
    double dash;
    double shadow[3];
    uint32_t paint;
    uint32_t strokeStyle;
    uint32_t gradient;
    uint32_t brush;
} YuGuiMetalPathStyle;

typedef struct YuGuiMetalGradient {
    double start[2];
    double end[2];
    double startRadius;
    double endRadius;
    uint32_t kind;
    uint32_t stopStart;
    uint32_t stopCount;
    uint32_t shader;
} YuGuiMetalGradient;

typedef struct YuGuiMetalGradientStop {
    double offset;
    double color[4];
} YuGuiMetalGradientStop;

enum YuGuiMetalStatField {
    YuGuiMetalStatEncodeNanos = 0,
    YuGuiMetalStatWalkNanos = 1,
    YuGuiMetalStatGpuNanos = 2,
    YuGuiMetalStatDrawCalls = 3,
    YuGuiMetalStatPipelineSwitches = 4,
    YuGuiMetalStatPacketBytes = 5,
    YuGuiMetalStatSkippedOps = 6,
    YuGuiMetalStatRendererAllocations = 7,
    YuGuiMetalStatInstances = 8,
    YuGuiMetalStatCommits = 9,
    YuGuiMetalStatPresents = 10,
    YuGuiMetalStatPacketCapacityBytes = 11,
    YuGuiMetalStatGlyphAtlasBytes = 12,
    YuGuiMetalStatGlyphAtlasUploads = 13,
    YuGuiMetalStatGlyphAtlasUploadBytes = 14,
    YuGuiMetalStatGlyphAtlasEvictions = 15,
    YuGuiMetalStatImageAtlasBytes = 16,
    YuGuiMetalStatRunCount = 17,
    YuGuiMetalStatFallbackRuns = 18,
    YuGuiMetalStatImageAtlasEvictions = 19,
    YuGuiMetalStatDrawableWaitNanos = 20,
    YuGuiMetalStatMaximumFramesPerSecond = 21,
    YuGuiMetalStatFillDraws = 22,
    YuGuiMetalStatFillSegments = 23,
    YuGuiMetalStatFillTiles = 24,
    YuGuiMetalStatFillPieces = 25,
    YuGuiMetalStatFillLines = 26,
    YuGuiMetalStatFillOpaqueTiles = 27,
    YuGuiMetalStatFillOverflows = 28,
    YuGuiMetalStatFillRedrawPending = 29,
    YuGuiMetalStatFillFramesPending = 30,
    YuGuiMetalStatLayerPasses = 31,
    YuGuiMetalStatCoverageClips = 32,
    YuGuiMetalStatImageTextureBytes = 33,
    YuGuiMetalStatImageTextureEvictions = 34,
    YuGuiMetalStatImageTextureUploadBytes = 35,
    YuGuiMetalStatImageTileSourceBytes = 36,
    YuGuiMetalStatImageTileCount = 37,
    YuGuiMetalStatTileGpuNanos = 38,
    YuGuiMetalStatTilesRendered = 39,
};

extern "C" bool yumiGuiMetalRendererIsAvailable(void);
extern "C" void yumiGuiMetalWarmUp(void);
extern "C" void yumiGuiMetalReleaseIdle(void);
// Returns a retained CGImage for a YuGraphics canvas. This narrow bridge keeps Yumi callers from
// importing both native headers into one module.
extern "C" void* yumiGuiMetalCanvasCreateCGImage(void* canvas);
extern "C" void* yumiGuiMetalRendererCreate(
    double width,
    double height,
    double scale,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" bool yumiGuiMetalRendererResize(
    void* renderer,
    double width,
    double height,
    double scale,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" void yumiGuiMetalRendererRelease(void* renderer);
extern "C" void yumiGuiMetalRendererFinish(void* renderer);
extern "C" const char* yumiGuiMetalRendererDeviceName(void* renderer);
extern "C" bool yumiGuiMetalRendererWritePNG(void* renderer, const char* path);
extern "C" bool yumiGuiMetalRendererReadIntoCanvas(void* renderer, void* canvas);
extern "C" int32_t yumiGuiMetalRendererAttachWindow(void* renderer, void* window);
extern "C" int32_t yumiGuiMetalRendererAttachPopup(void* renderer, void* popup);
extern "C" int32_t yumiGuiMetalRendererAttachStatus(void* renderer);
extern "C" bool yumiGuiMetalRendererSetOutputGamut(void* renderer, bool wide, bool opaque);
extern "C" bool yumiGuiMetalRendererIsWideGamut(void* renderer);
extern "C" bool yumiGuiMetalRendererSetOutputSurface(void* renderer, void* surface);
extern "C" int64_t yumiGuiMetalRendererCompletedFrame(void* renderer);
extern "C" bool yumiGuiMetalRendererReadColor(void* renderer, int32_t x, int32_t y, double* color);
extern "C" void yumiGuiMetalRendererDetachWindow(void* renderer, void* window);

// Immutable append-only frame packet. State and draw ops contain values and resource handles only.
extern "C" void yumiGuiMetalPacketBegin(void* renderer);
extern "C" void yumiGuiMetalPacketPushClip(
    void* renderer,
    double x,
    double y,
    double width,
    double height
);
extern "C" void yumiGuiMetalPacketPushRoundedClip(
    void* renderer,
    double x, double y, double width, double height,
    double left, double top, double right, double bottom
);
extern "C" void yumiGuiMetalPacketPushPathClip(void* renderer, int64_t pathSet, uint32_t state, bool evenOdd);
extern "C" void yumiGuiMetalPacketPopClip(void* renderer);
extern "C" void yumiGuiMetalPacketPushTranslate(void* renderer, double x, double y);
extern "C" void yumiGuiMetalPacketPushTransform(
    void* renderer,
    double a,
    double b,
    double c,
    double d,
    double tx,
    double ty
);
extern "C" void yumiGuiMetalPacketPopTranslate(void* renderer);
extern "C" void yumiGuiMetalPacketPushLayer(void* renderer, double alpha);
extern "C" void yumiGuiMetalPacketPushTransformLayer(
    void* renderer,
    double a,
    double b,
    double c,
    double d,
    double tx,
    double ty,
    double alpha
);
extern "C" void yumiGuiMetalPacketPushFilterLayer(
    void* renderer,
    double x,
    double y,
    double width,
    double height,
    uint32_t shader,
    const double* args,
    uint64_t argCount,
    void* const* images,
    const double* imageScales,
    uint64_t imageCount,
    double time,
    double alpha
);
extern "C" void yumiGuiMetalPacketPopLayer(void* renderer);
extern "C" bool yumiGuiMetalPacketAppendItems(
    void* renderer,
    const void* scalarItemBytes,
    uint64_t count,
    const void* clipBytes,
    uint64_t clipCount,
    const uint32_t* palette,
    uint64_t paletteCount,
    const void* shapeBytes,
    uint64_t shapeCount,
    const double* transforms,
    uint64_t transformCount,
    const uint32_t* itemTransforms,
    const double* colors,
    uint64_t colorCount,
    const uint32_t* itemStrokes,
    const float* itemDashes,
    const void* stateBytes,
    uint64_t stateCount,
    const uint32_t* itemStates,
    const uint64_t* imageIdentities,
    uint64_t imageCount,
    int64_t pathSet
);
extern "C" int64_t yumiGuiMetalChunkPaths(
    void* renderer,
    uint64_t key,
    uint32_t generation,
    const void* scalarItemBytes,
    uint64_t count,
    const uint8_t* verbs,
    uint64_t verbCount,
    const double* coords,
    uint64_t coordCount,
    const void* styleBytes,
    uint64_t styleCount,
    const void* gradientBytes,
    uint64_t gradientCount,
    const void* stopBytes,
    uint64_t stopCount,
    const double* shaderArgs,
    uint64_t shaderArgCount,
    const void* stateBytes,
    uint64_t stateCount
);
extern "C" void yumiGuiMetalPacketAppendTile(
    void* renderer,
    uint64_t tile,
    double x,
    double y,
    double width,
    double height,
    double opacity
);
extern "C" void yumiGuiMetalPacketSetDamageAreas(void* renderer, const double* values, int64_t count);
extern "C" int64_t yumiGuiMetalPacketCommit(
    void* renderer,
    int32_t mode,
    double damageX,
    double damageY,
    double damageWidth,
    double damageHeight
);

// Opaque chunks upload pixels only when their retained record generation changes. Packets carry
// only the returned one-based integer handle; zero means missing or failed, and the borrowed scene
// key never enters packet storage.
extern "C" uint64_t yumiGuiMetalTileLookup(
    void* renderer,
    uint64_t retainedKey,
    uint32_t generation
);
extern "C" uint64_t yumiGuiMetalTileUpsertCanvas(
    void* renderer,
    uint64_t retainedKey,
    uint32_t generation,
    void* canvas,
    double pointsWidth,
    double pointsHeight
);
extern "C" uint64_t yumiGuiMetalTileLookupRendered(
    void* renderer,
    uint64_t retainedKey,
    uint32_t generation
);
extern "C" uint64_t yumiGuiMetalTileReserve(
    void* renderer,
    uint64_t retainedKey,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    double pointsWidth,
    double pointsHeight,
    int32_t slotX,
    int32_t slotY
);
extern "C" int64_t yumiGuiMetalPacketCommitTiles(void* renderer);
extern "C" void yumiGuiMetalChunkRelease(void* renderer, uint64_t retainedKey);
extern "C" void yumiGuiMetalChunkReleaseAll(void* renderer);

// Record-time extraction through the supported YuGraphics API. The packed return is
// generation<<32 | index; run data survives atlas eviction and is refcounted by chunks.
extern "C" int64_t yumiGuiMetalRunRegister(void* renderer, void* textLayout);
extern "C" void yumiGuiMetalRunRelease(
    void* renderer,
    uint32_t index,
    uint32_t generation
);
extern "C" double yumiGuiMetalRunMaximumWidthDelta(void* renderer);

extern "C" bool yumiGuiMetalImageLookup(
    void* renderer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha
);
// Consumes cgImage.
extern "C" bool yumiGuiMetalImageUpsert(
    void* renderer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
);
// Consumes cgImage.
extern "C" bool yumiGuiMetalImageUpsertSource(
    void* renderer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
);
// Consumes cgImage.
extern "C" bool yumiGuiMetalCanvasImageUpsertSource(
    void* renderer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
);
extern "C" bool yumiGuiMetalCanvasImageLookup(
    void* renderer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha
);
// Consumes cgImage.
extern "C" bool yumiGuiMetalCanvasImageUpsert(
    void* renderer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
);
extern "C" int32_t yumiGuiMetalCanvasTextureUpsert(void* renderer, void* canvas);
extern "C" bool yumiGuiMetalRendererTakeTileDamage(void* renderer);
extern "C" double yumiGuiMetalRendererTileDamageValue(void* renderer, int32_t index);

extern "C" bool yumiGuiMetalPathLookup(void* renderer, uint32_t index, uint32_t generation);
extern "C" void yumiGuiMetalPathBegin(void* renderer, uint32_t index, uint32_t generation);
extern "C" void yumiGuiMetalPathAppend(
    void* renderer,
    const uint8_t* verbs,
    int64_t count,
    const double* coords
);
extern "C" uint32_t yumiGuiMetalPathEndPath(void* renderer);
extern "C" void yumiGuiMetalPathAddSpan(
    void* renderer,
    uint32_t path,
    uint8_t fillRole,
    uint32_t fillIndex,
    uint8_t fillRule,
    uint8_t strokeRole,
    uint32_t strokeIndex,
    double strokeWidth,
    uint8_t strokeCap,
    uint8_t strokeJoin,
    uint8_t strokeAlignment
);
extern "C" bool yumiGuiMetalPathEnd(void* renderer);
extern "C" void yumiGuiMetalRendererSetFillTime(void* renderer, double seconds);
extern "C" bool yumiGuiMetalRendererTakeFillRedraw(void* renderer);
extern "C" uint64_t yumiGuiMetalRendererTakePresentCopiedPixels(void* renderer);

extern "C" void yumiGuiMetalRendererStatMark(void* renderer);
extern "C" int64_t yumiGuiMetalRendererStatValue(void* renderer, int32_t field);
extern "C" int64_t yumiGuiMetalRendererFrameStatValue(
    void* renderer,
    int64_t frame,
    int32_t field
);
extern "C" void yumiGuiMetalRendererDiffWithCanvas(void* renderer, void* canvas, int64_t tolerance);
extern "C" int64_t yumiGuiMetalRendererDiffValue(void* renderer, int32_t field);

#endif
