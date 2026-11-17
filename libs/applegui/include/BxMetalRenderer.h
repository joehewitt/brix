#ifndef BX_METAL_RENDERER_H
#define BX_METAL_RENDERER_H

#include <stdbool.h>
#include <stdint.h>

// Scalar-only retained display item. This layout mirrors gui/scene:DisplayItem and is the
// production bulk-encode seam: no managed reference, callback, or scene pointer can appear here.
typedef struct BxGuiMetalDisplayItem {
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
} BxGuiMetalDisplayItem;

typedef struct BxGuiMetalShape {
    float radius;
    float side;
    float offset;
    float size;
    float blur;
    float x;
    float y;
    float spread;
} BxGuiMetalShape;

typedef struct BxGuiMetalRect {
    double x;
    double y;
    double width;
    double height;
} BxGuiMetalRect;

typedef struct BxGuiMetalState {
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
} BxGuiMetalState;

typedef struct BxGuiMetalPathStyle {
    double fill[4];
    double stroke[4];
    double strokeWidth;
    double dash;
    double shadow[3];
    uint32_t paint;
    uint32_t strokeStyle;
    uint32_t gradient;
    uint32_t brush;
} BxGuiMetalPathStyle;

typedef struct BxGuiMetalGradient {
    double start[2];
    double end[2];
    double startRadius;
    double endRadius;
    uint32_t kind;
    uint32_t stopStart;
    uint32_t stopCount;
    uint32_t shader;
} BxGuiMetalGradient;

typedef struct BxGuiMetalGradientStop {
    double offset;
    double color[4];
} BxGuiMetalGradientStop;

enum BxGuiMetalStatField {
    BxGuiMetalStatEncodeNanos = 0,
    BxGuiMetalStatWalkNanos = 1,
    BxGuiMetalStatGpuNanos = 2,
    BxGuiMetalStatDrawCalls = 3,
    BxGuiMetalStatPipelineSwitches = 4,
    BxGuiMetalStatPacketBytes = 5,
    BxGuiMetalStatSkippedOps = 6,
    BxGuiMetalStatRendererAllocations = 7,
    BxGuiMetalStatInstances = 8,
    BxGuiMetalStatCommits = 9,
    BxGuiMetalStatPresents = 10,
    BxGuiMetalStatPacketCapacityBytes = 11,
    BxGuiMetalStatGlyphAtlasBytes = 12,
    BxGuiMetalStatGlyphAtlasUploads = 13,
    BxGuiMetalStatGlyphAtlasUploadBytes = 14,
    BxGuiMetalStatGlyphAtlasEvictions = 15,
    BxGuiMetalStatImageAtlasBytes = 16,
    BxGuiMetalStatRunCount = 17,
    BxGuiMetalStatFallbackRuns = 18,
    BxGuiMetalStatImageAtlasEvictions = 19,
    BxGuiMetalStatDrawableWaitNanos = 20,
    BxGuiMetalStatMaximumFramesPerSecond = 21,
    BxGuiMetalStatFillDraws = 22,
    BxGuiMetalStatFillSegments = 23,
    BxGuiMetalStatFillTiles = 24,
    BxGuiMetalStatFillPieces = 25,
    BxGuiMetalStatFillLines = 26,
    BxGuiMetalStatFillOpaqueTiles = 27,
    BxGuiMetalStatFillOverflows = 28,
    BxGuiMetalStatFillRedrawPending = 29,
    BxGuiMetalStatFillFramesPending = 30,
    BxGuiMetalStatLayerPasses = 31,
    BxGuiMetalStatCoverageClips = 32,
    BxGuiMetalStatImageTextureBytes = 33,
    BxGuiMetalStatImageTextureEvictions = 34,
    BxGuiMetalStatImageTextureUploadBytes = 35,
    BxGuiMetalStatImageTileSourceBytes = 36,
    BxGuiMetalStatImageTileCount = 37,
    BxGuiMetalStatTileGpuNanos = 38,
    BxGuiMetalStatTilesRendered = 39,
};

extern "C" bool brixGuiMetalRendererIsAvailable(void);
extern "C" void brixGuiMetalWarmUp(void);
extern "C" void brixGuiMetalReleaseIdle(void);
// Returns a retained CGImage for a BxGraphics canvas. This narrow bridge keeps Brix callers from
// importing both native headers into one module.
extern "C" void* brixGuiMetalCanvasCreateCGImage(void* canvas);
extern "C" void* brixGuiMetalRendererCreate(
    double width,
    double height,
    double scale,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" bool brixGuiMetalRendererResize(
    void* renderer,
    double width,
    double height,
    double scale,
    double red,
    double green,
    double blue,
    double alpha
);
extern "C" void brixGuiMetalRendererRelease(void* renderer);
extern "C" void brixGuiMetalRendererFinish(void* renderer);
extern "C" const char* brixGuiMetalRendererDeviceName(void* renderer);
extern "C" bool brixGuiMetalRendererWritePNG(void* renderer, const char* path);
extern "C" bool brixGuiMetalRendererReadIntoCanvas(void* renderer, void* canvas);
extern "C" int32_t brixGuiMetalRendererAttachWindow(void* renderer, void* window);
extern "C" int32_t brixGuiMetalRendererAttachPopup(void* renderer, void* popup);
extern "C" int32_t brixGuiMetalRendererAttachStatus(void* renderer);
extern "C" bool brixGuiMetalRendererSetOutputGamut(void* renderer, bool wide, bool opaque);
extern "C" bool brixGuiMetalRendererIsWideGamut(void* renderer);
extern "C" bool brixGuiMetalRendererSetOutputSurface(void* renderer, void* surface);
extern "C" int64_t brixGuiMetalRendererCompletedFrame(void* renderer);
extern "C" bool brixGuiMetalRendererReadColor(void* renderer, int32_t x, int32_t y, double* color);
extern "C" void brixGuiMetalRendererDetachWindow(void* renderer, void* window);

// Immutable append-only frame packet. State and draw ops contain values and resource handles only.
extern "C" void brixGuiMetalPacketBegin(void* renderer);
extern "C" void brixGuiMetalPacketPushClip(
    void* renderer,
    double x,
    double y,
    double width,
    double height
);
extern "C" void brixGuiMetalPacketPushRoundedClip(
    void* renderer,
    double x, double y, double width, double height,
    double left, double top, double right, double bottom
);
extern "C" void brixGuiMetalPacketPushPathClip(void* renderer, int64_t pathSet, uint32_t state, bool evenOdd);
extern "C" void brixGuiMetalPacketPopClip(void* renderer);
extern "C" void brixGuiMetalPacketPushTranslate(void* renderer, double x, double y);
extern "C" void brixGuiMetalPacketPushTransform(
    void* renderer,
    double a,
    double b,
    double c,
    double d,
    double tx,
    double ty
);
extern "C" void brixGuiMetalPacketPopTranslate(void* renderer);
extern "C" void brixGuiMetalPacketPushLayer(void* renderer, double alpha);
extern "C" void brixGuiMetalPacketPushTransformLayer(
    void* renderer,
    double a,
    double b,
    double c,
    double d,
    double tx,
    double ty,
    double alpha
);
extern "C" void brixGuiMetalPacketPushFilterLayer(
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
extern "C" void brixGuiMetalPacketPopLayer(void* renderer);
extern "C" bool brixGuiMetalPacketAppendItems(
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
extern "C" int64_t brixGuiMetalChunkPaths(
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
extern "C" void brixGuiMetalPacketAppendTile(
    void* renderer,
    uint64_t tile,
    double x,
    double y,
    double width,
    double height,
    double opacity
);
extern "C" void brixGuiMetalPacketSetDamageAreas(void* renderer, const double* values, int64_t count);
extern "C" int64_t brixGuiMetalPacketCommit(
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
extern "C" uint64_t brixGuiMetalTileLookup(
    void* renderer,
    uint64_t retainedKey,
    uint32_t generation
);
extern "C" uint64_t brixGuiMetalTileUpsertCanvas(
    void* renderer,
    uint64_t retainedKey,
    uint32_t generation,
    void* canvas,
    double pointsWidth,
    double pointsHeight
);
extern "C" uint64_t brixGuiMetalTileLookupRendered(
    void* renderer,
    uint64_t retainedKey,
    uint32_t generation
);
extern "C" uint64_t brixGuiMetalTileReserve(
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
extern "C" int64_t brixGuiMetalPacketCommitTiles(void* renderer);
extern "C" void brixGuiMetalChunkRelease(void* renderer, uint64_t retainedKey);
extern "C" void brixGuiMetalChunkReleaseAll(void* renderer);

// Record-time extraction through the supported BxGraphics API. The packed return is
// generation<<32 | index; run data survives atlas eviction and is refcounted by chunks.
extern "C" int64_t brixGuiMetalRunRegister(void* renderer, void* textLayout);
extern "C" void brixGuiMetalRunRelease(
    void* renderer,
    uint32_t index,
    uint32_t generation
);
extern "C" double brixGuiMetalRunMaximumWidthDelta(void* renderer);

extern "C" bool brixGuiMetalImageLookup(
    void* renderer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha
);
// Consumes cgImage.
extern "C" bool brixGuiMetalImageUpsert(
    void* renderer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
);
// Consumes cgImage.
extern "C" bool brixGuiMetalImageUpsertSource(
    void* renderer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
);
// Consumes cgImage.
extern "C" bool brixGuiMetalCanvasImageUpsertSource(
    void* renderer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
);
extern "C" bool brixGuiMetalCanvasImageLookup(
    void* renderer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha
);
// Consumes cgImage.
extern "C" bool brixGuiMetalCanvasImageUpsert(
    void* renderer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
);
extern "C" int32_t brixGuiMetalCanvasTextureUpsert(void* renderer, void* canvas);
extern "C" bool brixGuiMetalRendererTakeTileDamage(void* renderer);
extern "C" double brixGuiMetalRendererTileDamageValue(void* renderer, int32_t index);

extern "C" bool brixGuiMetalPathLookup(void* renderer, uint32_t index, uint32_t generation);
extern "C" void brixGuiMetalPathBegin(void* renderer, uint32_t index, uint32_t generation);
extern "C" void brixGuiMetalPathAppend(
    void* renderer,
    const uint8_t* verbs,
    int64_t count,
    const double* coords
);
extern "C" uint32_t brixGuiMetalPathEndPath(void* renderer);
extern "C" void brixGuiMetalPathAddSpan(
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
extern "C" bool brixGuiMetalPathEnd(void* renderer);
extern "C" void brixGuiMetalRendererSetFillTime(void* renderer, double seconds);
extern "C" bool brixGuiMetalRendererTakeFillRedraw(void* renderer);
extern "C" uint64_t brixGuiMetalRendererTakePresentCopiedPixels(void* renderer);

extern "C" void brixGuiMetalRendererStatMark(void* renderer);
extern "C" int64_t brixGuiMetalRendererStatValue(void* renderer, int32_t field);
extern "C" int64_t brixGuiMetalRendererFrameStatValue(
    void* renderer,
    int64_t frame,
    int32_t field
);
extern "C" void brixGuiMetalRendererDiffWithCanvas(void* renderer, void* canvas, int64_t tolerance);
extern "C" int64_t brixGuiMetalRendererDiffValue(void* renderer, int32_t field);

#endif
