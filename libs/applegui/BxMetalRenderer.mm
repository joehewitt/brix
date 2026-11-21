#include "BxAppleGUI/BxMetalRenderer.h"
#include "BxAppleGUI/BxAppleGUI.h"
#include "BxGraphics/BxGraphics.h"

#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#import <IOSurface/IOSurfaceRef.h>
#import <ImageIO/ImageIO.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <TargetConditionals.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <condition_variable>
#include <dispatch/dispatch.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" void brixGraphicsShaderSetRegisterHook(void (*hook)(uint32_t shader, const char* source));

static_assert(sizeof(BxGuiMetalDisplayItem) == 40, "DisplayItem wire layout must stay 40 bytes");
static_assert(sizeof(BxGuiMetalState) == 160, "DisplayStateWire layout must stay 160 bytes");
static_assert(sizeof(BxGuiMetalPathStyle) == 120, "BxGuiMetalPathStyle layout");
static_assert(sizeof(BxGuiMetalGradient) == 64, "BxGuiMetalGradient layout");
static_assert(sizeof(BxGuiMetalGradientStop) == 40, "BxGuiMetalGradientStop layout");

namespace {

static uint64_t nowNanos() {
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
}

enum PacketOpKind : uint8_t {
    PacketOpQuad = 1,
    PacketOpBorder,
    PacketOpTile,
    PacketOpGlyphRun,
    PacketOpImage,
    PacketOpPushClip,
    PacketOpPopClip,
    PacketOpPushTranslate,
    PacketOpPopTransform,
    PacketOpPushLayer,
    PacketOpPopLayer,
    PacketOpUnsupported,
    PacketOpPushRoundedClip,
    PacketOpShape,
    PacketOpFill,
    PacketOpPushTransform,
    PacketOpPath,
    PacketOpPushPathClip,
    PacketOpPushMaskClip,
    PacketOpPushMaskLayer,
    PacketOpPushLayerMaskClip,
    PacketOpPushBlend,
    PacketOpPopBlend,
    PacketOpPushBlurLayer,
    PacketOpBackdrop,
    PacketOpPushFilterLayer,
    PacketOpSprite,
    PacketOpPushTransformLayer,
    PacketOpBackdropShader,
    PacketOpQuads,
    PacketOpRasterTile,
    PacketOpPushContentTransform,
};

struct QuadOp {
    float x, y, width, height, cornerRadius;
    uint32_t color;
};

static_assert(sizeof(QuadOp) == 24, "Quad wire layout must stay 24 bytes");

struct BorderOp {
    float x, y, width, height, cornerRadius, borderWidth;
    uint32_t color;
};

struct ShapeOp {
    float x, y, width, height;
    float radius, strokeWidth;
    float side, offset, size;
    float blur, shadowX, shadowY, spread;
    uint32_t color;
    uint32_t kind;
};

struct TileOp {
    uint64_t handle;
    float x, y, width, height, opacity;
};

struct RasterTileOp {
    uint64_t handle;
    float x, y, width, height;
    float u, v, uWidth, vHeight;
    uint32_t exact;
};

struct SpriteOp {
    uint64_t key;
    uint32_t generation;
    uint32_t sampling;
    float x, y, opacity;
};

struct GlyphRunOp {
    uint32_t index;
    uint32_t generation;
    float x, y, width, height;
    uint32_t alignment;
    uint32_t color;
    float contentMask[4];
    uint32_t paint;
    uint32_t pathSlot;
    uint32_t pathSerial;
};

struct GlyphPaint {
    uint32_t record = 0;
    uint32_t shader = 0;
};

struct FillOp {
    uint32_t index;
    uint32_t generation;
    double transform[6];
    float contentMask[4];
    uint32_t paletteCount;
    uint32_t flags;
    float strokeWidth;
    uint32_t strokeStyle;
    float dash;
};

struct ImageOp {
    uint64_t identity;
    uint32_t index;
    uint32_t generation;
    float x, y, width, height;
    uint32_t color;
    uint32_t templateAlpha;
    uint32_t canvas;
    uint32_t sampling;
    uint32_t pattern;
    float tile[4];
    float contentMask[4];
};

struct ClipOp {
    float x, y, width, height;
};

struct MaskClipOp {
    uint64_t serial;
    float x, y, width, height;
};

struct MaskOp {
    uint32_t id;
};

struct BlendOp {
    uint32_t mode;
};

struct RoundedClipOp {
    ClipOp bounds;
    float radii[4];
};

struct RoundedClip {
    float bounds[4];
    float radii[4];
    uint32_t parent;
    uint32_t padding[3];
};

struct TranslateOp {
    float x, y;
};

struct TransformOp {
    double matrix[6];
};

struct LayerOp {
    float alpha;
};

struct TransformLayerOp {
    double matrix[6];
    float alpha;
};

struct BlurLayerOp {
    float radius;
};

static const uint32_t kMaxFilterKeeps = 4;

struct FilterLayerOp {
    float x, y, width, height, alpha, time;
    uint32_t shader;
    uint32_t argCount;
    uint32_t imageStart;
    uint32_t imageCount;
    float resolution[2];
    uint32_t keepCount;
    float keeps[kMaxFilterKeeps * 4];
};

struct BackdropOp {
    float x, y, width, height, radius;
};

struct BackdropShaderOp {
    float x, y, width, height;
    uint32_t slot;
    uint32_t serial;
    uint32_t gradient;
    float radius;
};

struct PathOp {
    uint32_t slot;
    uint32_t serial;
    uint32_t span;
    float contentMask[4];
};

struct PathClipOp {
    uint32_t slot;
    uint32_t serial;
    uint32_t path;
    uint32_t fillRule;
};

struct UnsupportedOp {
    uint8_t displayItemKind;
};

struct FramePacket {
    std::vector<uint8_t> bytes;
    uint32_t opCount = 0;
    std::vector<id<MTLTexture>> images;
};

struct Instance {
    float rect[4];
    float params[4];
    uint32_t color;
    uint32_t kind;
    float opacity;
    uint32_t roundedClip;
    float contentMask[4];
    uint32_t coverageClip;
    uint32_t blend;
    uint32_t padding[2];
};

enum PipelineIndex : uint32_t {
    PipelineTier1 = 0,
    PipelineMask = 1,
    PipelineRGBA = 2,
    PipelineClear = 3,
    PipelineFill = 4,
    PipelineBackdrop = 5,
    PipelineBackdropBlur = 6,
    PipelineDeferredLayer = 7,
    PipelineDab = 8,
};

struct Batch {
    uint32_t pipeline;
    id<MTLTexture> texture;
    MTLScissorRect scissor;
    uint32_t start;
    uint32_t count;
    bool blend;
    uint32_t shader;
};

struct LayerPass {
    id<MTLTexture> texture = nil;
    id<MTLTexture> depth = nil;
    std::vector<Batch> batches;
    double bounds[4] = {0, 0, 0, 0};
    bool empty = true;
    float alpha = 1;
    uint32_t origin[2] = {0, 0};
    uint32_t size[2] = {0, 0};
    bool mask = false;
    uint32_t maskId = 0;
    uint32_t page = 0;
    uint32_t sampledPages = 0;
    uint32_t blend = 0;
    uint32_t blur = 0;
    bool blurred = false;
    bool clipOverflow = false;
    bool deferred = false;
    uint32_t filter = 0;
    std::vector<float> filterArgs;
    std::vector<id<MTLTexture>> filterImages;
    std::vector<float> filterFrames;
    float filterTime = 0;
    double filterRegion[4] = {0, 0, 0, 0};
    double filterScale = 1;
    float filterResolution[2] = {1, 1};
    uint32_t filterKeepCount = 0;
    float filterKeeps[kMaxFilterKeeps * 4] = {};
    bool transformed = false;
    double composite[6] = {1, 0, 0, 1, 0, 0};
};

static const uint32_t kMaskPages = 4;

struct MaskRegion {
    bool valid = false;
    uint32_t page = 0;
    uint32_t bounds[4] = {0, 0, 0, 0};
};

struct ShadowBlurGPU {
    float target[4];
    float source[2];
    uint32_t radius;
    uint32_t vertical;
};

struct LayerBlurGPU {
    float target[4];
    float source[2];
    uint32_t radius;
    uint32_t vertical;
    float limit[4];
};

struct FilterComposeGPU {
    float target[4];
    float origin[2];
    float resolution[2];
    float reduced[2];
    float scale;
    uint32_t keepCount;
    float keeps[kMaxFilterKeeps * 4];
};

static const uint32_t kMaxLayerBlur = 256;
static const uint32_t kMaxBackdropBlur = 512;
static const int32_t kBackdropLevels = 8;
static const MTLPixelFormat kBackdropFormat = MTLPixelFormatRGBA16Float;

struct BackdropGatherGPU {
    int32_t origin[2];
    int32_t sourceOrigin[2];
    int32_t sourceSize[2];
    int32_t regionOrigin[2];
    int32_t regionSize[2];
    int32_t factor;
    int32_t transformed;
    float axisX[2];
    float axisY[2];
    float offset[2];
};

struct BackdropBlurGPU {
    float center;
    uint32_t pairs;
    uint32_t vertical;
    uint32_t padding;
    float weights[8];
    float offsets[8];
};

struct BackdropTexture {
    id<MTLTexture> texture = nil;
    int64_t lastUsedFrame = -1;
    bool used = false;
};

struct BackdropRecord {
    int32_t level = 0;
    int32_t region[4] = {};
    int64_t firstX = 0;
    int64_t firstY = 0;
    uint32_t columns = 0;
    uint32_t rows = 0;
    BackdropBlurGPU blur = {};
    id<MTLTexture> levels[kBackdropLevels] = {};
    id<MTLTexture> scratch = nil;
    id<MTLTexture> shaded = nil;
    uint32_t shader = 0;
    uint32_t argStart = 0;
    uint32_t argCount = 0;
    float time = 0;
    float shaderOrigin[2] = {};
    float shaderSize[2] = {};
    float shaderScale = 1;
    bool blurred = false;
    bool transformed = false;
    double sourceTransform[6] = {1, 0, 0, 1, 0, 0};
};

struct ShadowRecord {
    ShadowBlurGPU horizontal;
    ShadowBlurGPU vertical;
    uint32_t layer;
    uint32_t generation;
};

static const uint32_t kShadowLayers = kMaskPages * 4;

struct TileRecord {
    id<MTLTexture> texture = nil;
    uint64_t key = 0;
    uint32_t generation = 0;
    float pointsWidth = 0;
    float pointsHeight = 0;
    uint32_t pixelWidth = 0;
    uint32_t pixelHeight = 0;
    int64_t lastUsedFrame = -1;
    int64_t renderedFrame = -1;
    int64_t reservedFrame = -1;
    bool pyramid = false;
    bool settle = false;
};

struct TileRejection {
    uint64_t key = 0;
    uint32_t generation = 0;
    uint32_t tiles = 0;
};

struct TileCopy {
    uint32_t handle;
    uint32_t x;
    uint32_t y;
};

struct RetiredTexture {
    id<MTLTexture> texture = nil;
    int64_t lastUsedFrame = -1;
};

static uint64_t deviceMemoryShare(uint64_t divisor, uint64_t most) {
    return std::min<uint64_t>(most, [NSProcessInfo processInfo].physicalMemory / divisor);
}

static uint64_t tileBudgetBytes() {
    static const uint64_t budget = deviceMemoryShare(32, 256ull * 1024 * 1024);
    return budget;
}

static uint64_t pyramidBudgetBytes() {
    return tileBudgetBytes() / 4 * 3;
}

static uint64_t imageTextureBudgetBytes() {
    static const uint64_t budget = deviceMemoryShare(8, 1024ull * 1024 * 1024);
    return budget;
}

static id<MTLCommandBuffer> rendererCommandBuffer() {
    return (__bridge id<MTLCommandBuffer>)brixGraphicsMetalCommandBuffer();
}

static std::atomic<uint64_t> memoryPressureCount {0};

static void noteMemoryPressure(void*) {
    memoryPressureCount.fetch_add(1, std::memory_order_release);
}

static void watchMemoryPressure() {
    static std::once_flag once;
    std::call_once(once, [] {
        dispatch_source_t source = dispatch_source_create(
            DISPATCH_SOURCE_TYPE_MEMORYPRESSURE,
            0,
            DISPATCH_MEMORYPRESSURE_WARN | DISPATCH_MEMORYPRESSURE_CRITICAL,
            dispatch_get_global_queue(QOS_CLASS_UTILITY, 0));
        if (source) {
            dispatch_source_set_event_handler_f(source, noteMemoryPressure);
            dispatch_resume(source);
        }
    });
}

static const size_t kRetiredTileLimit = 4;
static const size_t kTileRejectionLimit = 256;

static const int kAtlasSize = 1024;
static const double kGlyphPenSteps = 1024;
static const int kGlyphAtlasPages = 4;
static const int kImageAtlasPages = 2;
static const uint64_t kFrameUploadCap = 256 * 1024;

static const uint32_t kImageTextureSingleLimit = 2048;
static const uint32_t kImageTextureSizeLimit = 16384;
static const uint64_t kImageTextureUploadFrameBytes = 32ull * 1024 * 1024;
static const uint32_t kImageTilePreviewSize = 1024;
static const uint32_t kImageTileContent = 508;
static const uint32_t kImageTileGutter = 2;
static const uint32_t kImageTileSize = kImageTileContent + kImageTileGutter * 2;
static const uint32_t kInstanceAffine = 0x1000u;
static const uint32_t kInstanceEdges = 0x1f00u;
static const uint32_t kInstanceTemplate = 0x2000u;
static const uint32_t kInstanceLinear = 0x4000u;
static const uint32_t kInstanceSpaceShift = 16;

static uint32_t imageOpKind(const ImageOp& op, uint32_t kind) {
    return op.templateAlpha ? kind | kInstanceTemplate : kind;
}

static const uint64_t kImageTileFrameBytes = 256ull * 1024 * 1024;
static const size_t kImageTilePoolLimit = 16;
enum : int32_t { CanvasTextureMissing = 0, CanvasTextureReady = 1, CanvasTexturePending = 2 };

struct ImageTextureTile {
    id<MTLTexture> texture = nil;
    std::vector<id<MTLTexture>> levels;
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t bytes = 0;
};

struct ImageTile {
    ImageTextureTile surface;
    int64_t lastUsedFrame = -1;
    bool ready = false;
    bool dirty = false;
    uint32_t dirtyRect[4] = {};
};

struct ImageTexture {
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t serial = 0;
    int64_t lastUsedFrame = -1;
    int32_t space = 0;
    int32_t format = 0;
    bool external = false;
    bool tiled = false;
    ImageTextureTile single;
    bool updating = false;
    uint32_t update[4] = {};
    id<MTLBuffer> source = nil;
    uint32_t sourceRow = 0;
    uint32_t levels = 0;
    std::unordered_map<uint64_t, ImageTile> tiles;
    ImageTextureTile preview;
    bool previewReady = false;
    int64_t previewUsedFrame = -1;
};

struct ImageTileParams {
    int32_t originX;
    int32_t originY;
    uint32_t regionLeft;
    uint32_t regionTop;
    uint32_t regionRight;
    uint32_t regionBottom;
    uint32_t width;
    uint32_t height;
    uint32_t level;
    uint32_t row;
    float stepX;
    float stepY;
    uint32_t preview;
    uint32_t format;
};

struct ImageTileJob {
    ImageTextureTile* surface;
    id<MTLBuffer> source;
    ImageTileParams params;
};

static void releaseImageTextureTile(ImageTextureTile& tile) {
    tile.levels.clear();
    tile.texture = nil;
}

struct AtlasEntry {
    uint16_t page = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    int16_t offsetX = 0;
    int16_t offsetTopY = 0;
    float scale = 0;
    uint64_t lastUsedFrame = 0;
    bool valid = false;
};

struct AtlasPage {
    id<MTLTexture> texture = nil;
    int shelfX = 0;
    int shelfY = 0;
    int shelfHeight = 0;
    int64_t lastUsedFrame = -1;
};

struct MaskAtlas {
    std::vector<AtlasPage> pages;
    uint32_t maxPages = 0;
    MTLPixelFormat pixelFormat = MTLPixelFormatA8Unorm;
    uint32_t bytesPerPixel = 1;
    uint64_t evictions = 0;
};

struct RunGlyph {
    CGGlyph glyph = 0;
    float x = 0;
    float y = 0;
    uint16_t fontIndex = 0;
    bool hasColor = false;
    uint32_t color = 0;
};

struct RunLine {
    float width = 0;
    float baseline = 0;
    uint32_t glyphStart = 0;
    uint32_t glyphCount = 0;
};

struct RunRecord {
    uint64_t layoutSerial = 0;
    uint32_t generation = 1;
    uint32_t references = 0;
    bool alive = false;
    float height = 0;
    float naturalWidth = 0;
    double maximumWidthDelta = 0;
    std::vector<RunGlyph> glyphs;
    std::vector<RunLine> lines;
    std::vector<CTFontRef> fonts;
    std::vector<AtlasEntry*> entries;
    double entryScale = 0;
    uint64_t entryEpoch = 0;
    double drawnScale = 0;
    int64_t drawnFrame = -1;
    bool scaleChanging = false;
    bool moving = false;
    float emSize = 0;
    float xHeight = 0;
    bool tinted = false;
};

struct FrameStats {
    uint64_t encodeNanos = 0;
    uint64_t walkNanos = 0;
    int64_t gpuNanos = -1;
    uint32_t drawCalls = 0;
    uint32_t pipelineSwitches = 0;
    uint32_t instances = 0;
    uint32_t skippedOps = 0;
    uint64_t packetBytes = 0;
    uint32_t glyphUploads = 0;
    uint64_t glyphUploadBytes = 0;
    uint32_t fallbackRuns = 0;
    uint64_t drawableWaitNanos = 0;
    uint32_t layerPasses = 0;
    uint32_t coverageClips = 0;
    int64_t fillDraws = 0;
    int64_t fillSegments = 0;
    int64_t fillTiles = 0;
    int64_t fillPieces = 0;
    int64_t fillLines = 0;
    int64_t fillOpaqueTiles = 0;
};

struct PresentationRecord {
    int64_t frame = -1;
    int64_t gpuNanos = -1;
    uint64_t drawableWaitNanos = 0;
    uint64_t presenterWaitNanos = 0;
    uint64_t presentedTimeNanos = 0;
};

struct PresentationState {
    std::mutex mutex;
    std::vector<PresentationRecord> records;
    std::atomic<int32_t> pending{0};
};

struct PresentGate {
    int32_t outstanding = 0;
    void* window = nullptr;
    void (*setPending)(void*, bool) = nullptr;
};

constexpr size_t kPresentDamageHistory = 8;
constexpr size_t kMaxDamageAreas = 4;
constexpr size_t kMaxPresentCopies = 8;
constexpr uint64_t kPresentBufferStaleNanos = 500000000;

struct PresentBuffer {
    uint32_t surface = 0;
    int64_t serial = -1;
    uint64_t usedNanos = 0;
};

struct PresentDamage {
    std::mutex mutex;
    int64_t serial = 0;
    std::array<MTLScissorRect, kPresentDamageHistory * kMaxDamageAreas> areas = {};
    std::array<size_t, kPresentDamageHistory> areaCounts = {};
    std::array<PresentBuffer, 5> buffers = {};
    std::atomic<uint64_t> copiedPixels{0};
};

enum : uint32_t {
    FillSegmentKindMask = 3,
    FillSegmentImplicit = 4,
    FillSegmentStart = 8,
    FillSegmentEnd = 16,
    FillSegmentClosed = 32,
};

enum : uint32_t {
    FillStrokeInherit = 255,
    FillStrokeCapButt = 0,
    FillStrokeCapRound = 1,
    FillStrokeCapSquare = 2,
    FillStrokeJoinMiter = 0,
    FillStrokeJoinRound = 1,
};
enum : uint32_t { FillAlignCenter = 0, FillAlignInside = 1, FillAlignOutside = 2 };
enum : uint32_t {
    FillClipNone = 0,
    FillClipInside = 1,
    FillClipOutside = 2,
    FillClipSource = 3,
    FillClipEvenOdd = 4,
    FillClipMask = 8,
    FillClipLayerMask = 16,
    FillClipShadow = 32,
};

constexpr double fillMiterLimit = 10;
constexpr double fillClosingSnap = 1e-6;
constexpr double fillDashLimit = 16384;

struct FillSegmentGPU {
    float p[8];
    uint32_t kind;
    uint32_t link;
    float arcStart;
    float arcEnd;
};
static_assert(sizeof(FillSegmentGPU) == 48, "FillSegment layout");

struct FillDrawGPU {
    float color[4];
    float mask[4];
    float extent[4];
    float a, b, c, d, tx, ty;
    uint32_t segmentStart;
    uint32_t segmentCount;
    uint32_t segmentBase;
    uint32_t fillRule;
    int32_t tileX;
    int32_t tileY;
    uint32_t tileW;
    uint32_t tileH;
    uint32_t tileBase;
    uint32_t rowBase;
    uint32_t batch;
    uint32_t roundedClip;
    float halfWidth;
    uint32_t strokeStyle;
    uint32_t clip;
    uint32_t coverage;
    float dash;
    uint32_t paint;
};
static_assert(sizeof(FillDrawGPU) == 144, "FillDraw layout");

struct FillPaintGPU {
    float a, b, c, d, tx, ty;
    float p0, p1, p2, p3;
    uint32_t kind;
    uint32_t stopStart;
    uint32_t stopCount;
    uint32_t padding;
    float paperX, paperY;
};
static_assert(sizeof(FillPaintGPU) == 64, "FillPaint layout");

struct FillStopGPU {
    float color[4];
    float offset;
    float padding[3];
};
static_assert(sizeof(FillStopGPU) == 32, "FillStop layout");

static const uint32_t FillPaintTranslucent = 0x80000000u;

struct CoverageClipRecord {
    uint32_t draw;
    uint32_t parent;
    uint32_t mode;
    uint32_t bounds;
};

struct FillParamsGPU {
    uint32_t drawCount;
    uint32_t tileCount;
    uint32_t segmentCount;
    uint32_t rowCount;
    uint32_t pieceCapacity;
    uint32_t batchCount;
    float tolerance;
    float viewportWidth;
    float viewportHeight;
    float miterLimit;
    float originX;
    float originY;
    uint32_t gridWidth;
    uint32_t gridHeight;
};
static_assert(sizeof(FillParamsGPU) == 56, "FillParams layout");

struct FillPathRecord {
    uint32_t segmentStart = 0;
    uint32_t segmentCount = 0;
    double originX = 0;
    double originY = 0;
    double width = 0;
    double height = 0;
    double miterScale = 1;
    double length = 0;
    double area = 0;
};

struct DabCacheEntry {
    std::vector<BxGraphicsDab> dabs;
    double total = 0;
    uint32_t refs = 0;
    int64_t lastUsed = 0;
};

struct FillSpanRecord {
    uint32_t path = 0;
    uint32_t paletteIndex = 0;
    uint8_t role = 2;
    uint8_t fillRule = 0;
    uint8_t strokeRole = 2;
    uint8_t strokeCap = FillStrokeInherit;
    uint8_t strokeJoin = FillStrokeInherit;
    uint8_t strokeAlignment = FillAlignCenter;
    uint32_t strokePaletteIndex = 0;
    uint32_t paint = 0;
    double strokeWidth = -1;
    double dash = 0;
    bool shadow = false;
    double shadowOffset[2] = {0, 0};
    double shadowBlur = 0;
    uint32_t brush = 0;
    uint64_t dabKey = 0;
    DabCacheEntry* dabs = nullptr;
};

struct FillGeometryRecord {
    uint32_t generation = 0;
    bool resident = false;
    std::vector<FillPathRecord> paths;
    std::vector<FillSpanRecord> spans;
};

struct ChunkPathSet {
    uint64_t key = 0;
    uint32_t generation = 0;
    uint32_t serial = 0;
    FillGeometryRecord geometry;
    std::vector<float> palette;
    std::vector<BxGuiMetalGradient> gradients;
    std::vector<FillStopGPU> stops;
    std::vector<float> shaderArgs;
    std::vector<uint32_t> statePaths;
    std::vector<uint32_t> spanItems;
};

struct FillPendingSegment {
    double p[8];
    uint32_t kind;
    uint32_t link;
};

struct FrameFillDraw {
    uint32_t segmentStart;
    uint32_t segmentCount;
    double originX;
    double originY;
    double bounds[4];
    double device[6];
    float color[4];
    float mask[4];
    uint32_t fillRule;
    uint32_t batch;
    uint32_t roundedClip;
    float halfWidth;
    uint32_t strokeStyle;
    uint32_t clip;
    uint32_t coverage;
    float dash;
    uint32_t paint;
    uint32_t blend;
    uint32_t shader;
};

static const uint32_t kBrushParams = 12;

struct DabGPU {
    float x;
    float y;
    float angle;
    float distance;
    float length;
    float along;
    uint32_t draw;
    uint32_t index;
};
static_assert(sizeof(DabGPU) == 32, "Dab layout");

struct DabStyleGPU {
    float width;
    float scatter;
    float sizeJitter;
    float angleJitter;
    float roundness;
    float angle;
    float flow;
    float taperStart;
    float taperEnd;
    float seed;
    float total;
    uint32_t fixedAngle;
};

struct DabDrawGPU {
    float color[4];
    float mask[4];
    float a, b, c, d, tx, ty;
    DabStyleGPU style;
    float paper;
    uint32_t argStart;
    uint32_t argCount;
    uint32_t roundedClip;
    uint32_t coverage;
    uint32_t blend;
    float paperX, paperY;
};
static_assert(sizeof(DabDrawGPU) == 136, "DabDraw layout");

struct FillBatchInfo {
    MTLScissorRect scissor;
    uint32_t drawStart;
    uint32_t drawEnd;
    bool paints;
    bool blend;
    uint32_t shader;
};

static const uint32_t kBatchRects = 8;

struct BatchRects {
    float rects[kBatchRects][4];
    uint32_t count;
};

struct BatchGroup {
    uint32_t head;
    uint32_t tail;
    uint32_t members;
    BatchRects bounds;
};

static const uint32_t kMaxFilterArgs = 1000;
static const uint32_t kMaxFilterImages = 8;

struct FilterParamsGPU {
    float target[4];
    float origin[2];
    float size[2];
    float scale;
    uint32_t argCount;
    uint32_t imageCount;
    float time;
    float sourceOrigin[2];
    float sourceScale;
    float padding;
    float resolution[2];
    float tail[2];
};

static std::string rendererColorSource() {
    return std::string(brixGraphicsShaderColor()) +
        "constant uint kBrixOutputSpace = 0u;\n"
        "constant bool kBrixExtendedOutput [[function_constant(1)]];\n";
}

static const char* filterShaderSource = R"METAL(
#include <metal_stdlib>
using namespace metal;
)METAL";

static const char* filterPassSource = R"METAL(
float4 shade(float2 position, ShaderLayer layer, ShaderArgs args);

struct BrixFilterParams {
    float4 target;
    float2 origin;
    float2 size;
    float scale;
    uint argCount;
    uint imageCount;
    float time;
    float2 sourceOrigin;
    float sourceScale;
    float2 resolution;
};

struct BrixFilterVertexOut {
    float4 position [[position]];
};

vertex BrixFilterVertexOut brixFilterVertex(
    uint vid [[vertex_id]],
    constant BrixFilterParams& params [[buffer(0)]]
) {
    float2 corner = float2(float(vid & 1u), float(vid >> 1u));
    BrixFilterVertexOut out;
    out.position = float4(mix(params.target.xy, params.target.zw, corner), 0.0, 1.0);
    return out;
}

fragment float4 brixFilterFragment(
    BrixFilterVertexOut in [[stage_in]],
    texture2d<float> source [[texture(0)]],
    array<texture2d<float>, 8> images [[texture(1)]],
    constant BrixFilterParams& params [[buffer(0)]],
    device const float* args [[buffer(1)]],
    constant float4* frames [[buffer(2)]]
) {
    ShaderLayer layer = {
        source, params.sourceOrigin, params.sourceScale, params.size, kBrixOutputSpace, params.resolution
    };
    ShaderArgs shaderArgs = {args, params.argCount, images, frames, params.imageCount, kBrixOutputSpace, params.time};
    float2 position = (in.position.xy / params.resolution - params.origin) / params.scale;
    return brixShaderOutput(shade(position, layer, shaderArgs), kBrixOutputSpace, kBrixExtendedOutput ? 0u : 1u);
}
)METAL";

struct FormatPipeline {
    MTLPixelFormat format;
    id<MTLRenderPipelineState> render;
    id<MTLRenderPipelineState> blend;
};

struct ShaderPipelines {
    id<MTLLibrary> library = nil;
    std::vector<FormatPipeline> variants;
    bool failed = false;
    bool pending = false;
};

enum ShaderRole : uint8_t {
    ShaderRoleFill,
    ShaderRoleText,
    ShaderRoleDab,
    ShaderRoleFilter,
};

struct ShaderRequest {
    ShaderRole role;
    uint32_t shader;
    MTLPixelFormat format;
};

struct ShaderJob {
    ShaderRole role;
    uint32_t shader;
    MTLPixelFormat format;
    std::string source;
    ShaderPipelines pipelines;
};

struct ShaderCompiles {
    std::mutex mutex;
    std::vector<ShaderJob> finished;
    std::atomic<int64_t> pending{0};
    std::atomic<bool> landed{false};
};

struct SurfacePipelines {
    MTLPixelFormat format = MTLPixelFormatInvalid;
    id<MTLRenderPipelineState> pipelines[6] = {nil, nil, nil, nil, nil, nil};
    id<MTLRenderPipelineState> blendPipelines[6] = {nil, nil, nil, nil, nil, nil};
    id<MTLRenderPipelineState> fillRender = nil;
    id<MTLRenderPipelineState> fillSolid = nil;
    id<MTLRenderPipelineState> fillBlend = nil;
    id<MTLRenderPipelineState> shadowBlurPipelines[5] = {nil, nil, nil, nil, nil};
    id<MTLRenderPipelineState> layerBlurPipeline = nil;
    id<MTLRenderPipelineState> filterComposePipeline = nil;
};

struct FillSlot {
    id<MTLBuffer> draws = nil;
    id<MTLBuffer> segmentBases = nil;
    id<MTLBuffer> tileBases = nil;
    id<MTLBuffer> rowBases = nil;
    id<MTLBuffer> ranges = nil;
    id<MTLBuffer> stats = nil;
    id<MTLBuffer> coverageClips = nil;
    id<MTLBuffer> paints = nil;
    id<MTLBuffer> stops = nil;
    id<MTLBuffer> dabs = nil;
    id<MTLBuffer> dabDraws = nil;
};

enum FillKernel : uint32_t {
    FillKernelInit = 0,
    FillKernelCount,
    FillKernelAllocate,
    FillKernelWrite,
    FillKernelBackdrop,
    FillKernelOcclude,
    FillKernelMark,
    FillKernelScanGroups,
    FillKernelScatter,
    FillKernelBatchBounds,
    FillKernelBatchArgs,
    FillKernelTotal,
};

struct SharedPipelines {
    id<MTLDevice> device = nil;
    id<MTLLibrary> library = nil;
    std::mutex surfaceMutex;
    std::vector<std::unique_ptr<SurfacePipelines>> surfaces;
    id<MTLComputePipelineState> fillKernels[FillKernelTotal] = {};
    id<MTLComputePipelineState> imageMipKernel = nil;
    id<MTLComputePipelineState> imageTileKernel = nil;
    id<MTLComputePipelineState> readbackKernel = nil;
    id<MTLRenderPipelineState> backdropGatherPipeline = nil;
    id<MTLRenderPipelineState> backdropDownPipeline = nil;
    id<MTLRenderPipelineState> backdropBlurPipeline = nil;
    id<MTLDepthStencilState> depthNone = nil;
    id<MTLDepthStencilState> depthTest = nil;
    id<MTLDepthStencilState> depthWrite = nil;
};

struct MetalRenderer {
    id<MTLDevice> device = nil;
    SharedPipelines* shared = nullptr;
    MTLPixelFormat outputFormat = MTLPixelFormatBGRA8Unorm;
    MTLPixelFormat layerFormat = MTLPixelFormatBGRA8Unorm;
    id<MTLTexture> target = nil;
    double width = 0;
    double height = 0;
    int32_t pixelWidth = 0;
    int32_t pixelHeight = 0;
    double scale = 1;
    float background[4] = {0, 0, 0, 0};
    char deviceName[256] = {};

    dispatch_semaphore_t inflight = nullptr;
    FramePacket packets[3];
    id<MTLBuffer> instanceBuffers[3] = {nil, nil, nil};
    id<MTLBuffer> roundedClipBuffers[3] = {nil, nil, nil};
    std::vector<RoundedClip> roundedClips;
    int packetIndex = -1;
    FramePacket* currentPacket = nullptr;
    uint64_t packetBeginNanos = 0;

    std::vector<Instance> instances;
    std::vector<Batch> batches;
    std::vector<std::unique_ptr<LayerPass>> layers;
    uint32_t layerCount = 0;
    std::vector<uint32_t> stateMaskIds;
    uint32_t maskSequence = 0;
    std::vector<MaskRegion> maskRegions;
    std::vector<std::array<uint32_t, 4>> maskPlaced[kMaskPages];
    id<MTLTexture> maskPages[kMaskPages] = {nil, nil, nil, nil};
    id<MTLTexture> maskDepth = nil;
    uint8_t maskPageModes[kMaskPages] = {0, 0, 0, 0};
    std::vector<std::vector<Batch>> shadowBatches;
    uint32_t shadowGenerations = 0;
    std::vector<ShadowRecord> shadowRecords;
    uint32_t shadowShelf[3] = {0, 0, 0};
    uint32_t shadowPageShelves[kShadowLayers][3] = {};
    id<MTLTexture> shadowSource = nil;
    id<MTLTexture> shadowBlur = nil;
    id<MTLBuffer> shadowBlurBuffers[3] = {nil, nil, nil};
    std::vector<BackdropTexture> backdropTextures;
    std::vector<BackdropRecord> backdrops;
    std::vector<float> backdropArgs;
    std::vector<uint32_t> layerOrder;
    std::vector<uint32_t> deferredLayers;
    std::vector<double> stateMatrices;
    std::vector<uint8_t> stateShapes;
    std::vector<uint32_t> statePath;
    std::vector<uint32_t> stateStack;

    std::vector<TileRecord> tiles;
    std::unordered_map<uint64_t, uint32_t> tileByKey;
    std::vector<uint32_t> freeTiles;
    std::vector<RetiredTexture> retiredTiles;
    uint64_t tileBytes = 0;
    uint64_t pyramidBytes = 0;
    int64_t lastWindowFrame = -1;
    bool pyramidFull = false;
    std::vector<TileRejection> tileRejections;
    TileRejection takenRejection;
    id<MTLTexture> tileStaging = nil;
    std::vector<TileCopy> tileCopies;
    std::atomic<int64_t> tileOverflowFrames[4]{{-1}, {-1}, {-1}, {-1}};
    int64_t tileOverflowSeen[4] = {-1, -1, -1, -1};
    std::atomic<int64_t> tileGpuNanos{0};
    int64_t tilesRendered = 0;
    std::atomic<int64_t> completedFrame{-1};
    std::array<id<MTLCommandBuffer>, 4> committedBuffers;
    std::array<int64_t, 4> committedFrames{{-1, -1, -1, -1}};
    std::vector<RunRecord> runs;
    std::vector<uint32_t> freeRuns;
    std::unordered_map<uint64_t, uint32_t> runByLayout;
    MaskAtlas glyphAtlas;
    MaskAtlas imageAtlas;
    std::unordered_map<uint64_t, AtlasEntry> glyphEntries;
    std::unordered_map<uint64_t, uint64_t> glyphProxies;
    bool glyphUploadsDeferred = false;
    bool glyphAtlasFull = false;
    std::unordered_map<uint64_t, AtlasEntry> imageEntries;
    std::unordered_map<uint64_t, ImageTexture> imageTextures;
    uint64_t imageTextureBytes = 0;
    uint64_t imageTextureEvictions = 0;
    uint64_t imageTextureUploadBytes = 0;
    uint64_t imageTileSourceBytes = 0;
    uint64_t frameTileBytes = 0;
    std::vector<ImageTextureTile> imageTilePool;
    std::vector<ImageTileJob> imageTileJobs;
    std::vector<uint64_t> releasedCanvases;
    bool hasTileDamage = false;
    double tileDamage[4] = {};
    double takenTileDamage[4] = {};
    std::vector<uint8_t> rasterScratch;
    std::vector<id<MTLTexture>> fallbackTextures[3];
    uint64_t frameUploadBytes = 0;
    uint64_t frameTextureUploadBytes = 0;
    double maximumRunWidthDelta = 0;
    CAMetalLayer* presentationLayer = nil;
    IOSurfaceRef outputSurface = nullptr;
    id<MTLTexture> outputTexture = nil;
    void* presentationWindow = nullptr;
    void (*resetPresentationLayer)(void*) = nullptr;
    int32_t (*presentationVisibility)(void*) = nullptr;
    bool presentationWasUnoccluded = false;
    dispatch_queue_t presentQueue = nullptr;
    std::shared_ptr<PresentGate> presentGate;
    int32_t attachStatus = 0;
    int32_t maximumFramesPerSecond = 0;
    uint64_t presentCount = 0;
    std::shared_ptr<PresentationState> presentationState;
    std::shared_ptr<PresentDamage> presentDamage = std::make_shared<PresentDamage>();
    FrameStats frames[3];
    int64_t frameCount = 0;
    uint64_t memoryPressureSeen = 0;
    bool targetVolatile = false;

    uint64_t rendererAllocations = 0;
    uint64_t markedRendererAllocations = 0;
    int64_t lastFrame = -1;
    int64_t diffValues[4] = {-1, -1, 0, -1};
    std::unordered_map<uint32_t, ShaderPipelines> fillShaders;
    std::unordered_map<uint32_t, ShaderPipelines> textShaders;
    std::unordered_map<uint32_t, ShaderPipelines> dabShaders;
    std::unordered_map<uint32_t, ShaderPipelines> filterShaders;
    std::vector<ShaderRequest> shaderRequests;
    std::shared_ptr<ShaderCompiles> shaderCompiles = std::make_shared<ShaderCompiles>();
    bool skippedPendingShader = false;
    bool tileGlyphsDeferred = false;
    bool tileGlyphsMissing = false;
    double fillTime = 0;
    id<MTLTexture> depthTexture = nil;
    std::vector<FillGeometryRecord> fillGeometries;
    std::vector<ChunkPathSet> chunkPaths;
    std::unordered_map<uint64_t, uint32_t> chunkPathByKey;
    std::vector<uint32_t> freeChunkPaths;
    uint32_t chunkPathSerial = 0;
    std::vector<uint8_t> chunkPathVerbs;
    std::vector<double> chunkPathCoords;
    std::vector<FillSegmentGPU> fillSegments;
    id<MTLBuffer> fillSegmentBuffer = nil;
    size_t fillSegmentsUploaded = 0;
    size_t fillDeadSegments = 0;
    FillGeometryRecord fillBuild;
    uint32_t fillBuildIndex = 0;
    bool fillBuilding = false;
    std::vector<FillPendingSegment> fillPending;
    double fillCurrent[2] = {0, 0};
    double fillSubpath[2] = {0, 0};
    bool fillHasSubpath = false;
    size_t fillSubpathFirst = 0;
    std::vector<FrameFillDraw> fillDraws;
    std::vector<FillPaintGPU> fillPaints;
    std::vector<FillStopGPU> fillStops;
    std::vector<FillBatchInfo> fillBatches;
    std::vector<DabGPU> brushDabs;
    std::vector<DabDrawGPU> brushDraws;
    std::unordered_map<uint64_t, DabCacheEntry> dabCache;
    std::vector<CoverageClipRecord> coverageClips;
    std::vector<uint32_t> fillDrawSlots;
    std::vector<uint32_t> fillOrder;
    std::vector<uint32_t> instanceGather;
    uint32_t instanceGatherCount = 0;
    std::vector<BatchGroup> batchGroups;
    std::vector<BatchRects> batchRects;
    std::vector<uint32_t> batchNext;
    std::vector<uint32_t> fillGroupHeads;
    FillGeometryRecord unitRect;
    id<MTLBuffer> emptyBuffer = nil;
    id<MTLTexture> blankTexture = nil;
    FillSlot fillSlots[3];
    id<MTLBuffer> fillTileDraws = nil;
    id<MTLBuffer> fillCounts = nil;
    id<MTLBuffer> fillBackdrops = nil;
    id<MTLBuffer> fillStarts = nil;
    id<MTLBuffer> fillCursors = nil;
    id<MTLBuffer> fillPieces = nil;
    id<MTLBuffer> fillGroupSums = nil;
    id<MTLBuffer> fillGroupOpaque = nil;
    id<MTLBuffer> fillTileKinds = nil;
    id<MTLBuffer> fillOccluders = nil;
    id<MTLBuffer> fillVisibleTiles = nil;
    id<MTLBuffer> fillOpaqueTiles = nil;
    id<MTLBuffer> fillArgs = nil;
    uint32_t fillPieceCapacity = 0;
    FillParamsGPU fillParams = {};
    bool fillActive = false;
    std::atomic<uint64_t> fillPieceHigh{0};
    std::atomic<bool> fillRedraw{false};
    std::atomic<bool> glyphRedraw{false};
    std::atomic<int64_t> fillFramesPending{0};
    std::atomic<uint64_t> fillOverflows{0};
    int32_t encodeMode = 0;
    double encodeDamage[4] = {0, 0, 0, 0};
    std::vector<double> pendingDamageAreas;
    std::vector<double> encodeDamageAreas;
    bool canReencode = false;
};

static void countAllocation(MetalRenderer* renderer) {
    ++renderer->rendererAllocations;
}

static bool restoreTarget(MetalRenderer* renderer) {
    if (!renderer->targetVolatile) {
        return false;
    }
    renderer->targetVolatile = false;
    return [renderer->target setPurgeableState:MTLPurgeableStateNonVolatile] == MTLPurgeableStateEmpty;
}

template <typename Value>
static void ensureVectorCapacity(
    MetalRenderer* renderer,
    std::vector<Value>& values,
    size_t required
) {
    if (required <= values.capacity()) {
        return;
    }
    size_t capacity = values.capacity() ? values.capacity() * 2 : 64;
    while (capacity < required) {
        capacity *= 2;
    }
    values.reserve(capacity);
    countAllocation(renderer);
}

template <typename Value>
static void appendPacketOp(MetalRenderer* renderer, PacketOpKind kind, const Value& value) {
    FramePacket* packet = renderer->currentPacket;
    if (!packet) {
        return;
    }
    size_t offset = packet->bytes.size();
    size_t required = offset + 1 + sizeof(Value);
    ensureVectorCapacity(renderer, packet->bytes, required);
    packet->bytes.resize(required);
    packet->bytes[offset] = static_cast<uint8_t>(kind);
    memcpy(packet->bytes.data() + offset + 1, &value, sizeof(Value));
    ++packet->opCount;
}

static void appendQuadsPacketOp(MetalRenderer* renderer, const QuadOp* quads, uint32_t count) {
    FramePacket* packet = renderer->currentPacket;
    if (!packet || !count) {
        return;
    }
    size_t offset = packet->bytes.size();
    size_t bytes = static_cast<size_t>(count) * sizeof(QuadOp);
    size_t required = offset + 1 + sizeof(count) + bytes;
    ensureVectorCapacity(renderer, packet->bytes, required);
    packet->bytes.resize(required);
    packet->bytes[offset] = static_cast<uint8_t>(PacketOpQuads);
    memcpy(packet->bytes.data() + offset + 1, &count, sizeof(count));
    memcpy(packet->bytes.data() + offset + 1 + sizeof(count), quads, bytes);
    ++packet->opCount;
}

static void appendBarePacketOp(MetalRenderer* renderer, PacketOpKind kind) {
    FramePacket* packet = renderer->currentPacket;
    if (!packet) {
        return;
    }
    ensureVectorCapacity(renderer, packet->bytes, packet->bytes.size() + 1);
    packet->bytes.push_back(static_cast<uint8_t>(kind));
    ++packet->opCount;
}

static uint32_t packColor(double red, double green, double blue, double alpha) {
    auto channel = [](double value) -> uint32_t {
        return static_cast<uint32_t>(
            std::max(0.0, std::min(1.0, value)) * 255.0 + 0.5
        );
    };
    return channel(red) | (channel(green) << 8) | (channel(blue) << 16) |
        (channel(alpha) << 24);
}

static const char* commonShaderSource = R"METAL(
#include <metal_stdlib>
using namespace metal;

struct Instance {
    float4 rect;
    float4 params;
    uint color;
    uint kind;
    float opacity;
    uint roundedClip;
    float4 contentMask;
    uint coverageClip;
    uint blend;
    uint padding1;
    uint padding2;
};

struct RoundedClip {
    float4 bounds;
    float4 radii;
    uint parent;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct VertexOut {
    float4 position [[position]];
    float2 local;
    float2 halfSize [[flat]];
    float4 params [[flat]];
    float4 color [[flat]];
    uint kind [[flat]];
    float opacity [[flat]];
    float2 uvBase [[flat]];
    float2 uvSize [[flat]];
    float4 contentMask [[flat]];
    float2 pixelPosition;
    uint roundedClip [[flat]];
    uint coverageClip [[flat]];
    uint blend [[flat]];
    uint paint [[flat]];
};

constant uint kInstanceAffine = 0x1000u;
constant uint kInstanceEdges = 0x1f00u;
constant uint kInstanceTemplate = 0x2000u;
constant uint kInstanceLinear = 0x4000u;
constant uint kInstanceSpaceShift = 16u;

static float4 unpackColor(uint color) {
    return float4(
        (color >> 0) & 0xffu,
        (color >> 8) & 0xffu,
        (color >> 16) & 0xffu,
        (color >> 24) & 0xffu
    ) / 255.0;
}

constant bool kBlendDraw [[function_constant(0)]];
constant uint kBlendBounded = (1u << 16) | (1u << 17) | (1u << 18) | (1u << 19) | (1u << 22) | (1u << 24);
constant float kBlendUnbounded = 1.0e6;

static float blendLum(float3 color) {
    return dot(color, float3(0.3, 0.59, 0.11));
}

static float3 blendClipColor(float3 color) {
    float lum = blendLum(color);
    float low = min(min(color.r, color.g), color.b);
    float high = max(max(color.r, color.g), color.b);
    if (low < 0.0) {
        color = lum + (color - lum) * lum / (lum - low);
    }
    if (high > 1.0) {
        color = lum + (color - lum) * (1.0 - lum) / (high - lum);
    }
    return color;
}

static float3 blendSetLum(float3 color, float lum) {
    return blendClipColor(color + (lum - blendLum(color)));
}

static float blendSat(float3 color) {
    return max(max(color.r, color.g), color.b) - min(min(color.r, color.g), color.b);
}

static float3 blendSetSat(float3 color, float sat) {
    float low = min(min(color.r, color.g), color.b);
    float high = max(max(color.r, color.g), color.b);
    return high > low ? (color - low) * sat / (high - low) : float3(0.0);
}

static float3 blendHardLight(float3 backdrop, float3 source) {
    return select(
        1.0 - 2.0 * (1.0 - source) * (1.0 - backdrop),
        2.0 * source * backdrop,
        source <= 0.5
    );
}

static float3 blendMix(float3 backdrop, float3 source, uint mode) {
    switch (mode) {
    case 1u:
        return backdrop * source;
    case 2u:
        return backdrop + source - backdrop * source;
    case 3u:
        return blendHardLight(source, backdrop);
    case 4u:
        return min(backdrop, source);
    case 5u:
        return max(backdrop, source);
    case 6u:
        return select(
            select(backdrop / (1.0 - source), float3(kBlendUnbounded), source >= 1.0),
            float3(0.0),
            backdrop <= 0.0
        );
    case 7u:
        return select(
            select(1.0 - (1.0 - backdrop) / source, float3(-kBlendUnbounded), source <= 0.0),
            float3(1.0),
            backdrop >= 1.0
        );
    case 8u:
        return backdrop * backdrop * (1.0 - 2.0 * source) + 2.0 * source * backdrop;
    case 9u:
        return blendHardLight(backdrop, source);
    case 10u:
        return abs(backdrop - source);
    case 11u:
        return backdrop + source - 2.0 * backdrop * source;
    case 12u:
        return blendSetLum(blendSetSat(source, blendSat(backdrop)), blendLum(backdrop));
    case 13u:
        return blendSetLum(blendSetSat(backdrop, blendSat(source)), blendLum(backdrop));
    case 14u:
        return blendSetLum(source, blendLum(backdrop));
    case 15u:
        return blendSetLum(backdrop, blendLum(source));
    default:
        return source;
    }
}

static float4 blendColors(float4 source, float4 dest, uint mode) {
    float sa = source.a;
    float da = dest.a;
    if (mode <= 15u) {
        float3 cs = sa > 0.0 ? source.rgb / sa : float3(0.0);
        float3 cb = da > 0.0 ? dest.rgb / da : float3(0.0);
        float alpha = sa + da - sa * da;
        float3 color = source.rgb * (1.0 - da) + dest.rgb * (1.0 - sa) + sa * da * blendMix(cb, cs, mode);
        return float4(clamp(color, 0.0, alpha), alpha);
    }
    switch (mode) {
    case 16u:
        return float4(0.0);
    case 17u:
        return source;
    case 18u:
        return source * da;
    case 19u:
        return source * (1.0 - da);
    case 20u:
        return source * da + dest * (1.0 - sa);
    case 21u:
        return source * (1.0 - da) + dest;
    case 22u:
        return dest * sa;
    case 23u:
        return dest * (1.0 - sa);
    case 24u:
        return source * (1.0 - da) + dest * sa;
    case 25u:
        return source * (1.0 - da) + dest * (1.0 - sa);
    case 26u: {
        float alpha = min(sa + da, 1.0);
        return float4(max(alpha - ((da - dest.rgb) + (sa - source.rgb)), 0.0), alpha);
    }
    default:
        return min(source + dest, 1.0);
    }
}

static float4 blendResult(float4 source, float coverage, float4 dest, uint mode) {
    if (mode < 32u && ((kBlendBounded >> mode) & 1u) != 0u) {
        return mix(dest, blendColors(source, dest, mode), coverage);
    }
    return blendColors(source * coverage, dest, mode);
}

vertex VertexOut brixGuiMetalVertex(
    uint vertexId [[vertex_id]],
    uint instanceId [[instance_id]],
    constant float4& viewport [[buffer(0)]],
    const device Instance* instances [[buffer(1)]]
) {
    Instance instance = instances[instanceId];
    float2 corner = float2(vertexId & 1u, vertexId >> 1u);
    uint shapeKind = instance.kind & 0xffu;
    float pad = 0.0;
    if (shapeKind == 0u) {
        pad = 1.0;
    } else if (shapeKind == 1u) {
        pad = instance.params.y * 0.5 + 1.0;
    } else if (shapeKind == 4u) {
        pad = instance.params.w + 1.0;
    } else if (shapeKind == 5u) {
        pad = instance.params.w + 1.0;
    } else if (shapeKind == 6u) {
        float reach = max(abs(instance.contentMask.x), abs(instance.contentMask.y));
        pad = instance.params.y * 3.0 + max(instance.contentMask.z, 0.0) + reach +
            instance.params.w + 1.0;
    } else if (shapeKind >= 7u && shapeKind <= 10u && (instance.kind & kInstanceEdges) != 0u) {
        pad = 1.0;
    }
    VertexOut out;
    if ((instance.kind & kInstanceAffine) != 0u) {
        float2 axisU = instance.rect.zw;
        float2 axisV = instance.params.xy;
        float area = abs(axisU.x * axisV.y - axisU.y * axisV.x);
        float2 extent = float2(area / max(length(axisV), 1e-6), area / max(length(axisU), 1e-6));
        float2 margin = pad / max(extent, float2(1e-6));
        float2 st = -margin + corner * (1.0 + margin * 2.0);
        float2 point = instance.rect.xy + st.x * axisU + st.y * axisV;
        out.position = float4(
            (point.x - viewport.x) / viewport.z * 2.0 - 1.0,
            1.0 - (point.y - viewport.y) / viewport.w * 2.0,
            0,
            1
        );
        out.local = (st - 0.5) * extent;
        out.halfSize = extent * 0.5;
        out.params = instance.params;
        out.color = unpackColor(instance.color);
        out.kind = instance.kind;
        out.opacity = instance.opacity;
        out.uvBase = instance.params.zw;
        out.uvSize = float2(as_type<float>(instance.padding1), as_type<float>(instance.padding2));
        out.contentMask = instance.contentMask;
        out.pixelPosition = point;
        out.roundedClip = instance.roundedClip;
        out.coverageClip = instance.coverageClip;
        out.blend = instance.blend;
        out.paint = 0u;
        return out;
    }
    float2 point = instance.rect.xy - pad + corner * (instance.rect.zw + pad * 2.0);
    out.position = float4(
        (point.x - viewport.x) / viewport.z * 2.0 - 1.0,
        1.0 - (point.y - viewport.y) / viewport.w * 2.0,
        0,
        1
    );
    out.local = point - (instance.rect.xy + instance.rect.zw * 0.5);
    out.halfSize = instance.rect.zw * 0.5;
    out.params = instance.params;
    out.color = unpackColor(instance.color);
    out.kind = instance.kind;
    out.opacity = instance.opacity;
    out.uvBase = instance.params.xy;
    out.uvSize = instance.params.zw;
    out.contentMask = instance.contentMask;
    out.pixelPosition = point;
    out.roundedClip = instance.roundedClip;
    out.coverageClip = instance.coverageClip;
    out.blend = instance.blend;
    out.paint = shapeKind == 2u ? instance.padding1 : 0u;
    return out;
}

static float roundedRectDistance(float2 point, float2 halfSize, float radius) {
    float2 distance = abs(point) - halfSize + radius;
    return length(max(distance, 0.0)) + min(max(distance.x, distance.y), 0.0) - radius;
}

static float triangleDistance(float2 p, float2 p0, float2 p1, float2 p2) {
    float2 e0 = p1 - p0;
    float2 e1 = p2 - p1;
    float2 e2 = p0 - p2;
    float2 v0 = p - p0;
    float2 v1 = p - p1;
    float2 v2 = p - p2;
    float2 pq0 = v0 - e0 * saturate(dot(v0, e0) / dot(e0, e0));
    float2 pq1 = v1 - e1 * saturate(dot(v1, e1) / dot(e1, e1));
    float2 pq2 = v2 - e2 * saturate(dot(v2, e2) / dot(e2, e2));
    float s = sign(e0.x * e2.y - e0.y * e2.x);
    float2 d = min(
        min(
            float2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
            float2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))
        ),
        float2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x))
    );
    return -sqrt(d.x) * sign(d.y);
}

static float arrowDistance(float2 p, float2 halfSize, float radius, uint side, float offset, float size) {
    if (side == 0u || size <= 0.0) {
        return 1.0e9;
    }
    bool across = side <= 2u;
    float span = across ? halfSize.x * 2.0 : halfSize.y * 2.0;
    float base = size;
    float low = radius + size;
    float high = span - radius - size;
    float tip = clamp(offset, low, high);
    if (high < low) {
        base = max(0.0, span * 0.5 - radius);
        tip = span * 0.5;
    }
    if (base <= 0.0) {
        return 1.0e9;
    }
    float along = tip - span * 0.5;
    float2 a;
    float2 b;
    float2 c;
    if (side == 1u) {
        a = float2(along - base, -halfSize.y);
        b = float2(along + base, -halfSize.y);
        c = float2(along, -halfSize.y - size);
    } else if (side == 2u) {
        a = float2(along + base, halfSize.y);
        b = float2(along - base, halfSize.y);
        c = float2(along, halfSize.y + size);
    } else if (side == 3u) {
        a = float2(-halfSize.x, along + base);
        b = float2(-halfSize.x, along - base);
        c = float2(-halfSize.x - size, along);
    } else {
        a = float2(halfSize.x, along - base);
        b = float2(halfSize.x, along + base);
        c = float2(halfSize.x + size, along);
    }
    float depth = across ? halfSize.y : halfSize.x;
    float reach = (size + depth) / size;
    float band = across ? abs(p.x) - halfSize.x : abs(p.y) - halfSize.y;
    return max(triangleDistance(p, c + (a - c) * reach, c + (b - c) * reach, c), band);
}

static float outlineDistance(float2 p, float2 halfSize, float radius, uint side, float offset, float size) {
    float r = clamp(radius, 0.0, min(halfSize.x, halfSize.y));
    return min(
        roundedRectDistance(p, halfSize, r),
        arrowDistance(p, halfSize, r, side, offset, size)
    );
}

static float2 erf2(float2 x) {
    float2 s = sign(x);
    float2 a = abs(x);
    float2 t = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
    t *= t;
    return s - s / (t * t);
}

static float erf1(float x) {
    return erf2(float2(x, 0.0)).x;
}

static float boxShadowRow(float x, float y, float sigma, float corner, float2 halfSize) {
    float delta = min(halfSize.y - corner - abs(y), 0.0);
    float curved = halfSize.x - corner + sqrt(max(0.0, corner * corner - delta * delta));
    float2 integral = 0.5 + 0.5 * erf2((x + float2(-curved, curved)) * (0.70710678 / sigma));
    return integral.y - integral.x;
}

static float roundedBoxShadow(float2 point, float2 halfSize, float sigma, float corner) {
    float low = point.y - halfSize.y;
    float high = point.y + halfSize.y;
    float start = clamp(-3.0 * sigma, low, high);
    float end = clamp(3.0 * sigma, low, high);
    float step = (end - start) / 4.0;
    float y = start + step * 0.5;
    float value = 0.0;
    for (int index = 0; index < 4; ++index) {
        float weight = exp(-(y * y) / (2.0 * sigma * sigma)) / (2.5066282746 * sigma);
        value += boxShadowRow(point.x, point.y - y, sigma, corner, halfSize) * weight * step;
        y += step;
    }
    return value;
}

static float shapeShadowCoverage(float2 local, float2 halfSize, float4 params, float4 extra, uint side) {
    float radius = params.x;
    float sigma = params.y;
    float body = saturate(0.5 - outlineDistance(
        local, halfSize, radius, side, params.z, params.w
    ));
    if (body >= 1.0) {
        return 0.0;
    }
    float spread = extra.z;
    float2 shadowHalf = max(halfSize + spread, float2(0.0));
    float shadowRadius = radius > 0.0 ? max(0.0, radius + spread) : 0.0;
    shadowRadius = min(shadowRadius, min(shadowHalf.x, shadowHalf.y));
    float2 point = local - extra.xy;
    float shadowSize = max(0.0, params.w + spread * 0.41421356);
    float coverage;
    if (sigma < 0.25) {
        coverage = saturate(0.5 - outlineDistance(
            point, shadowHalf, shadowRadius, side, params.z + spread, shadowSize
        ));
    } else {
        coverage = roundedBoxShadow(point, shadowHalf, sigma, shadowRadius);
        float arrow = arrowDistance(point, shadowHalf, shadowRadius, side, params.z + spread, shadowSize);
        if (arrow < 1.0e8) {
            coverage = max(coverage, 0.5 - 0.5 * erf1(arrow * 0.70710678 / sigma));
        }
    }
    return coverage * (1.0 - body);
}

static float roundedClipCoverage(float2 point, uint index, const device RoundedClip* clips) {
    float coverage = 1.0;
    while (index != 0u) {
        RoundedClip clip = clips[index - 1u];
        float2 start = clip.bounds.xy;
        float2 end = clip.bounds.zw;
        float rx = point.x < start.x + clip.radii.x ? clip.radii.x : clip.radii.z;
        float ry = point.y < start.y + clip.radii.y ? clip.radii.y : clip.radii.w;
        float cx = point.x < start.x + clip.radii.x ? start.x + rx : end.x - rx;
        float cy = point.y < start.y + clip.radii.y ? start.y + ry : end.y - ry;
        bool cornerX = point.x < start.x + clip.radii.x || point.x > end.x - clip.radii.z;
        bool cornerY = point.y < start.y + clip.radii.y || point.y > end.y - clip.radii.w;
        if (cornerX && cornerY && rx > 0.0 && ry > 0.0) {
            float2 q = (point - float2(cx, cy)) / float2(rx, ry);
            float magnitude = length(q);
            float gradient = length(q / float2(rx, ry)) / max(magnitude, 0.00001);
            float distance = (magnitude - 1.0) / max(gradient, 0.00001);
            coverage *= saturate(0.5 - distance);
        }
        index = clip.parent;
    }
    return coverage;
}

)METAL";

static const char* effectShaderSource = R"METAL(
struct ShadowBlur {
    float4 target;
    float2 source;
    uint radius;
    uint vertical;
};

struct ShadowBlurOut {
    float4 position [[position]];
    float2 offset [[flat]];
    uint radius [[flat]];
    uint vertical [[flat]];
};

vertex ShadowBlurOut brixGuiMetalShadowBlurVertex(
    uint vertexId [[vertex_id]],
    uint instanceId [[instance_id]],
    const device ShadowBlur* blurs [[buffer(0)]],
    constant float2& viewport [[buffer(1)]]
) {
    ShadowBlur blur = blurs[instanceId];
    float2 corner = float2(float(vertexId & 1u), float(vertexId >> 1u));
    float2 pixel = blur.target.xy + corner * blur.target.zw;
    ShadowBlurOut out;
    out.position = float4(pixel / viewport * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    out.offset = blur.source - blur.target.xy;
    out.radius = blur.radius;
    out.vertical = blur.vertical;
    return out;
}

fragment float4 brixGuiMetalShadowBlurFragment(
    ShadowBlurOut in [[stage_in]],
    texture2d<float> source [[texture(0)]]
) {
    int2 texel = int2(floor(in.position.xy + in.offset));
    int radius = int(in.radius);
    int2 step = in.vertical != 0u ? int2(0, 1) : int2(1, 0);
    float sigma = max(float(radius) * 0.5, 0.5);
    float falloff = -0.5 / (sigma * sigma);
    float sum = 0.0;
    float total = 0.0;
    for (int tap = -radius; tap <= radius; ++tap) {
        float weight = exp(float(tap * tap) * falloff);
        sum += weight * source.read(uint2(texel + step * tap)).a;
        total += weight;
    }
    return float4(sum / total);
}

struct LayerBlur {
    float4 target;
    float2 source;
    uint radius;
    uint vertical;
    float4 limit;
};

struct LayerBlurOut {
    float4 position [[position]];
};

vertex LayerBlurOut brixGuiMetalLayerBlurVertex(
    uint vertexId [[vertex_id]],
    constant LayerBlur& blur [[buffer(0)]],
    constant float2& viewport [[buffer(1)]]
) {
    float2 corner = float2(float(vertexId & 1u), float(vertexId >> 1u));
    float2 pixel = blur.target.xy + corner * blur.target.zw;
    LayerBlurOut out;
    out.position = float4(pixel / viewport * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return out;
}

fragment float4 brixGuiMetalLayerBlurFragment(
    LayerBlurOut in [[stage_in]],
    constant LayerBlur& blur [[buffer(0)]],
    texture2d<float> source [[texture(0)]]
) {
    int2 texel = int2(floor(in.position.xy + blur.source - blur.target.xy));
    int radius = int(blur.radius);
    int2 step = blur.vertical != 0u ? int2(0, 1) : int2(1, 0);
    int2 low = int2(blur.limit.xy);
    int2 high = int2(blur.limit.zw);
    float sigma = max(float(radius) * 0.5, 0.5);
    float falloff = -0.5 / (sigma * sigma);
    float4 sum = float4(0.0);
    float total = 0.0;
    for (int tap = -radius; tap <= radius; ++tap) {
        float weight = exp(float(tap * tap) * falloff);
        int2 at = texel + step * tap;
        if (all(at >= low) && all(at < high)) {
            sum += weight * source.read(uint2(at));
        }
        total += weight;
    }
    return sum / total;
}

struct FilterCompose {
    float4 target;
    float2 origin;
    float2 resolution;
    float2 reduced;
    float scale;
    uint keepCount;
    float4 keeps[4];
};

vertex LayerBlurOut brixGuiMetalFilterComposeVertex(
    uint vertexId [[vertex_id]],
    constant FilterCompose& compose [[buffer(0)]],
    constant float2& viewport [[buffer(1)]]
) {
    float2 corner = float2(float(vertexId & 1u), float(vertexId >> 1u));
    float2 pixel = compose.target.xy + corner * compose.target.zw;
    LayerBlurOut out;
    out.position = float4(pixel / viewport * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return out;
}

fragment float4 brixGuiMetalFilterComposeFragment(
    LayerBlurOut in [[stage_in]],
    constant FilterCompose& compose [[buffer(0)]],
    texture2d<float> source [[texture(0)]]
) {
    float2 point = (in.position.xy - compose.origin) / compose.scale;
    for (uint index = 0; index < compose.keepCount; ++index) {
        float4 keep = compose.keeps[index];
        if (point.x >= keep.x && point.y >= keep.y && point.x < keep.x + keep.z && point.y < keep.y + keep.w) {
            discard_fragment();
        }
    }
    constexpr sampler pixels(coord::pixel, address::clamp_to_edge, filter::linear);
    float2 at = clamp(in.position.xy * compose.resolution, float2(0.5), compose.reduced - 0.5);
    return source.sample(pixels, at);
}

struct BackdropGather {
    int2 origin;
    int2 sourceOrigin;
    int2 sourceSize;
    int2 regionOrigin;
    int2 regionSize;
    int factor;
    int transformed;
    float2 axisX;
    float2 axisY;
    float2 offset;
};

struct BackdropBlur {
    float center;
    uint pairs;
    uint vertical;
    uint padding;
    float weights[8];
    float offsets[8];
};

struct BackdropOut {
    float4 position [[position]];
};

vertex BackdropOut brixGuiMetalBackdropVertex(
    uint vertexId [[vertex_id]],
    constant float4& region [[buffer(0)]],
    constant float2& viewport [[buffer(1)]]
) {
    float2 corner = float2(float(vertexId & 1u), float(vertexId >> 1u));
    float2 pixel = region.xy + corner * region.zw;
    BackdropOut out;
    out.position = float4(pixel / viewport * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return out;
}

int2 mirrorBackdrop(int2 value, int2 start, int2 size) {
    int2 period = size * 2;
    int2 offset = (value - start) % period;
    offset = select(offset, offset + period, offset < 0);
    return start + select(period - 1 - offset, offset, offset < size);
}

fragment float4 brixGuiMetalBackdropGatherFragment(
    BackdropOut in [[stage_in]],
    constant BackdropGather& gather [[buffer(0)]],
    texture2d<float> source [[texture(0)]]
) {
    constexpr sampler pixels(coord::pixel, address::clamp_to_edge, filter::linear);
    int2 base = (gather.origin + int2(floor(in.position.xy))) * gather.factor;
    float2 limit = float2(gather.sourceSize);
    float4 sum = float4(0.0);
    for (int dy = 0; dy < gather.factor; ++dy) {
        for (int dx = 0; dx < gather.factor; ++dx) {
            int2 pixel = mirrorBackdrop(base + int2(dx, dy), gather.regionOrigin, gather.regionSize);
            if (gather.transformed != 0) {
                float2 center = float2(pixel) + 0.5;
                float2 local = gather.axisX * center.x + gather.axisY * center.y + gather.offset -
                    float2(gather.sourceOrigin);
                if (all(local >= 0.0) && all(local <= limit)) {
                    sum += source.sample(pixels, clamp(local, float2(0.5), limit - 0.5));
                }
                continue;
            }
            int2 local = pixel - gather.sourceOrigin;
            if (all(local >= 0) && all(local < gather.sourceSize)) {
                sum += source.read(uint2(local));
            }
        }
    }
    return sum / float(gather.factor * gather.factor);
}

fragment float4 brixGuiMetalBackdropDownFragment(
    BackdropOut in [[stage_in]],
    texture2d<float> source [[texture(0)]]
) {
    uint2 texel = uint2(floor(in.position.xy)) * 2u;
    return (source.read(texel) + source.read(texel + uint2(1, 0)) +
        source.read(texel + uint2(0, 1)) + source.read(texel + uint2(1, 1))) * 0.25;
}

fragment float4 brixGuiMetalBackdropBlurFragment(
    BackdropOut in [[stage_in]],
    constant BackdropBlur& blur [[buffer(0)]],
    texture2d<float> source [[texture(0)]]
) {
    constexpr sampler linear(coord::pixel, address::clamp_to_edge, filter::linear);
    float2 at = floor(in.position.xy) + 0.5;
    float2 step = blur.vertical != 0u ? float2(0.0, 1.0) : float2(1.0, 0.0);
    float4 sum = source.sample(linear, at) * blur.center;
    for (uint index = 0; index < blur.pairs; ++index) {
        float2 offset = step * blur.offsets[index];
        sum += (source.sample(linear, at + offset) + source.sample(linear, at - offset)) *
            blur.weights[index];
    }
    return sum;
}

)METAL";

static const char* fillTypesSource = R"METAL(
struct FillSegment {
    float2 p0;
    float2 p1;
    float2 p2;
    float2 p3;
    uint kind;
    uint link;
    float arcStart;
    float arcEnd;
};

struct FillDraw {
    float4 color;
    float4 mask;
    float4 extent;
    float a, b, c, d, tx, ty;
    uint segmentStart;
    uint segmentCount;
    uint segmentBase;
    uint fillRule;
    int tileX;
    int tileY;
    uint tileW;
    uint tileH;
    uint tileBase;
    uint rowBase;
    uint batch;
    uint roundedClip;
    float halfWidth;
    uint strokeStyle;
    uint clip;
    uint coverage;
    float dash;
    uint paint;
};

struct FillPaint {
    float a, b, c, d, tx, ty;
    float p0, p1, p2, p3;
    uint kind;
    uint stopStart;
    uint stopCount;
    uint padding;
    float paperX;
    float paperY;
};

struct FillStop {
    float4 color;
    float offset;
    float padding0;
    float padding1;
    float padding2;
};

#ifdef BRIX_USER_SHADER
struct ShaderArgs {
    device const FillStop* stops;
    uint start;
    uint count;
    float time;

    float operator[](uint index) const {
        return index < count ? stops[start + index / 4u].color[index % 4u] : 0.0;
    }

    float4 color(uint index) const {
        float4 rgba = float4(
            index < count ? stops[start + index / 4u].color[index % 4u] : 0.0,
            index + 1u < count ? stops[start + (index + 1u) / 4u].color[(index + 1u) % 4u] : 0.0,
            index + 2u < count ? stops[start + (index + 2u) / 4u].color[(index + 2u) % 4u] : 0.0,
            index + 3u < count ? stops[start + (index + 3u) / 4u].color[(index + 3u) % 4u] : 0.0
        );
        return float4(brixLinearColor(rgba.rgb, 0u, kBrixOutputSpace), rgba.a);
    }
};

#if defined(BRIX_USER_PAINT)
float4 paint(PaintInput input, ShaderArgs args);
#elif defined(BRIX_USER_DAB)
float4 dab(DabInput input, ShaderArgs args);
#else
float4 shade(float2 position, float2 size, ShaderArgs args);
#endif
#endif

struct CoverageClip {
    uint draw;
    uint parent;
    uint mode;
    uint bounds;
};

struct FillParams {
    uint drawCount;
    uint tileCount;
    uint segmentCount;
    uint rowCount;
    uint pieceCapacity;
    uint batchCount;
    float tolerance;
    float viewportWidth;
    float viewportHeight;
    float miterLimit;
    float originX;
    float originY;
    uint gridWidth;
    uint gridHeight;
};

struct FillContext {
    int gridX;
    int gridY;
    int tileW;
    int tileH;
    uint tileBase;
    uint pieceCapacity;
    device atomic_uint* counts;
    device atomic_int* backdrops;
    device atomic_uint* cursors;
    device const uint* starts;
    device float4* pieces;
    device atomic_uint* stats;
};

constant float kTile = 16.0;
constant uint kSegmentKindMask = 3u;
constant uint kSegmentImplicit = 4u;
constant uint kSegmentStart = 8u;
constant uint kSegmentEnd = 16u;
constant uint kSegmentClosed = 32u;
constant uint kCapRound = 1u;
constant uint kCapSquare = 2u;
constant uint kJoinMiter = 0u;
constant uint kJoinRound = 1u;
constant float kDashLimit = 16384.0;
constant uint kClipMode = 3u;
constant uint kClipInside = 1u;
constant uint kClipSource = 3u;
constant uint kClipEvenOdd = 4u;
constant uint kClipMask = 8u;
constant uint kClipLayerMask = 16u;
constant uint kClipShadow = 32u;

)METAL";

static const char* fillBinSource = R"METAL(
static uint findDraw(device const uint* bases, uint count, uint value) {
    uint low = 0;
    uint high = count;
    while (high - low > 1) {
        uint middle = (low + high) / 2;
        if (bases[middle] <= value) {
            low = middle;
        } else {
            high = middle;
        }
    }
    return low;
}

static float2 transformPoint(FillDraw draw, float2 point) {
    return float2(
        draw.a * point.x + draw.c * point.y + draw.tx,
        draw.b * point.x + draw.d * point.y + draw.ty
    );
}

template <bool Write>
static void emitPiece(thread FillContext& context, int tx, int ty, float2 from, float2 to) {
    if (tx < 0 || ty < 0 || tx >= context.tileW || ty >= context.tileH || from.y == to.y) {
        return;
    }
    uint tile = context.tileBase + uint(ty) * uint(context.tileW) + uint(tx);
    if (!Write) {
        atomic_fetch_add_explicit(&context.counts[tile], 1u, memory_order_relaxed);
        return;
    }
    float2 origin = float2(context.gridX + tx, context.gridY + ty) * kTile;
    uint slot = context.starts[tile] +
        atomic_fetch_add_explicit(&context.cursors[tile], 1u, memory_order_relaxed);
    if (slot < context.pieceCapacity) {
        context.pieces[slot] = float4(from - origin, to - origin);
    }
}

template <bool Write>
static void emitStep(thread FillContext& context, int tx, int ty, float y, float sign) {
    float top = float(context.gridY + ty) * kTile;
    float local = clamp(y - top, 0.0, kTile);
    if (local >= kTile) {
        return;
    }
    float2 origin = float2(float(context.gridX + tx) * kTile, top);
    float2 upper = origin + float2(0.0, local);
    float2 lower = origin + float2(0.0, kTile);
    if (sign > 0.0) {
        emitPiece<Write>(context, tx, ty, upper, lower);
    } else {
        emitPiece<Write>(context, tx, ty, lower, upper);
    }
}

static void emitBackdrop(thread FillContext& context, int column, int row, int direction) {
    int target = column + 1;
    if (row < 0 || row >= context.tileH || target < 0 || target >= context.tileW) {
        return;
    }
    uint tile = context.tileBase + uint(row) * uint(context.tileW) + uint(target);
    atomic_fetch_add_explicit(&context.backdrops[tile], direction, memory_order_relaxed);
}

template <bool Write>
static void walkLine(thread FillContext& context, float2 from, float2 to) {
    float2 delta = to - from;
    int cx = int(floor(from.x / kTile)) - context.gridX;
    int cy = int(floor(from.y / kTile)) - context.gridY;
    int ex = int(floor(to.x / kTile)) - context.gridX;
    int ey = int(floor(to.y / kTile)) - context.gridY;
    int stepX = ex > cx ? 1 : (ex < cx ? -1 : 0);
    int stepY = ey > cy ? 1 : (ey < cy ? -1 : 0);
    int remainingX = abs(ex - cx);
    int remainingY = abs(ey - cy);
    if (remainingX > context.tileW + 2 || remainingY > context.tileH + 2) {
        return;
    }
    float2 current = from;
    while (remainingX + remainingY > 0) {
        float boundaryX = float(context.gridX + cx + (stepX > 0 ? 1 : 0)) * kTile;
        float boundaryY = float(context.gridY + cy + (stepY > 0 ? 1 : 0)) * kTile;
        float tX = remainingX > 0 ? (boundaryX - from.x) / delta.x : INFINITY;
        float tY = remainingY > 0 ? (boundaryY - from.y) / delta.y : INFINITY;
        bool crossX = remainingY == 0 || (remainingX > 0 && tX <= tY);
        float2 next;
        if (crossX) {
            float rowTop = float(context.gridY + cy) * kTile;
            next = float2(boundaryX, clamp(from.y + delta.y * saturate(tX), rowTop, rowTop + kTile));
        } else {
            float columnLeft = float(context.gridX + cx) * kTile;
            next = float2(clamp(from.x + delta.x * saturate(tY), columnLeft, columnLeft + kTile), boundaryY);
        }
        emitPiece<Write>(context, cx, cy, current, next);
        if (crossX) {
            int crossed = stepX > 0 ? cx + 1 : cx;
            emitStep<Write>(context, crossed, cy, next.y, -float(stepX));
            cx += stepX;
            remainingX -= 1;
        } else {
            if (!Write) {
                emitBackdrop(context, cx, stepY > 0 ? cy + 1 : cy, stepY);
            }
            cy += stepY;
            remainingY -= 1;
        }
        current = next;
    }
    emitPiece<Write>(context, cx, cy, current, to);
}

template <bool Write>
static void clipLine(thread FillContext& context, float2 low, float2 high, float2 from, float2 to) {
    if (!all(isfinite(from)) || !all(isfinite(to))) {
        return;
    }
    float2 gridLow = low + 1.0;
    float2 gridHigh = high - 1.0;
    if ((from.x >= gridHigh.x && to.x >= gridHigh.x) ||
        (from.y < gridLow.y && to.y < gridLow.y) ||
        (from.y >= gridHigh.y && to.y >= gridHigh.y)) {
        return;
    }
    float2 delta = to - from;
    float cuts[6];
    int cutCount = 0;
    cuts[cutCount++] = 0.0;
    float edges[4] = {low.x, high.x, low.y, high.y};
    for (int edge = 0; edge < 4; ++edge) {
        float origin = edge < 2 ? from.x : from.y;
        float span = edge < 2 ? delta.x : delta.y;
        if (span != 0.0) {
            float t = (edges[edge] - origin) / span;
            if (t > 0.0 && t < 1.0) {
                cuts[cutCount++] = t;
            }
        }
    }
    cuts[cutCount++] = 1.0;
    for (int i = 1; i < cutCount - 1; ++i) {
        for (int j = i + 1; j < cutCount - 1; ++j) {
            if (cuts[j] < cuts[i]) {
                float swap = cuts[i];
                cuts[i] = cuts[j];
                cuts[j] = swap;
            }
        }
    }
    float2 previous = clamp(from, low, high);
    for (int i = 1; i < cutCount; ++i) {
        float2 point = i == cutCount - 1 ? to : from + delta * cuts[i];
        point = clamp(point, low, high);
        bool outside = (previous.x >= gridHigh.x && point.x >= gridHigh.x) ||
            (previous.y < gridLow.y && point.y < gridLow.y) ||
            (previous.y >= gridHigh.y && point.y >= gridHigh.y);
        if (!outside && any(previous != point)) {
            walkLine<Write>(context, previous, point);
        }
        previous = point;
    }
}

static float2 strokeDirection(float2 vector, float2 fallback) {
    float length2 = dot(vector, vector);
    return length2 > 1e-20 ? vector * rsqrt(length2) : fallback;
}

static float2 strokeNormal(float2 tangent) {
    return float2(-tangent.y, tangent.x);
}

static float2 strokeOffset(float2 point, float2 tangent, float side, float halfWidth) {
    return point + (side * halfWidth) * strokeNormal(tangent);
}

static float2 strokeStartTangent(FillSegment segment) {
    if ((segment.kind & kSegmentKindMask) != 0) {
        float2 first = segment.p1 - segment.p0;
        if (dot(first, first) > 1e-20) {
            return strokeDirection(first, float2(1.0, 0.0));
        }
        float2 second = segment.p2 - segment.p0;
        if (dot(second, second) > 1e-20) {
            return strokeDirection(second, float2(1.0, 0.0));
        }
    }
    return strokeDirection(segment.p3 - segment.p0, float2(1.0, 0.0));
}

static float2 strokeEndTangent(FillSegment segment) {
    if ((segment.kind & kSegmentKindMask) != 0) {
        float2 last = segment.p3 - segment.p2;
        if (dot(last, last) > 1e-20) {
            return strokeDirection(last, float2(1.0, 0.0));
        }
        float2 second = segment.p3 - segment.p1;
        if (dot(second, second) > 1e-20) {
            return strokeDirection(second, float2(1.0, 0.0));
        }
    }
    return strokeDirection(segment.p3 - segment.p0, float2(1.0, 0.0));
}

static float2 cubicPoint(FillSegment segment, float t) {
    float u = 1.0 - t;
    return u * u * u * segment.p0 + 3.0 * u * u * t * segment.p1 +
        3.0 * u * t * t * segment.p2 + t * t * t * segment.p3;
}

static float2 cubicTangent(FillSegment segment, float t, float2 fallback) {
    float u = 1.0 - t;
    float2 derivative = 3.0 * u * u * (segment.p1 - segment.p0) +
        6.0 * u * t * (segment.p2 - segment.p1) + 3.0 * t * t * (segment.p3 - segment.p2);
    return strokeDirection(derivative, fallback);
}

static float turnBetween(float2 first, float2 second) {
    float lengths = length(first) * length(second);
    if (!(lengths > 1e-12)) {
        return 0.0;
    }
    return acos(clamp(dot(first, second) / lengths, -1.0, 1.0));
}

struct StrokeEmitter {
    FillDraw draw;
    float2 low;
    float2 high;
    uint lines;
};

template <bool Write>
static void strokeLine(thread FillContext& context, thread StrokeEmitter& emitter, float2 from, float2 to) {
    clipLine<Write>(
        context,
        emitter.low,
        emitter.high,
        transformPoint(emitter.draw, from),
        transformPoint(emitter.draw, to)
    );
    emitter.lines += 1;
}

template <bool Write>
static void strokeArc(
    thread FillContext& context,
    thread StrokeEmitter& emitter,
    float2 center,
    float2 from,
    float2 to,
    float sweep,
    float deviceRadius,
    float tolerance
) {
    float step = deviceRadius > tolerance ? 2.0 * acos(1.0 - tolerance / deviceRadius) : M_PI_F;
    float steps = ceil(abs(sweep) / max(step, 1e-3));
    uint count = isfinite(steps) ? uint(clamp(steps, 1.0, 64.0)) : 1u;
    float2 radial = from - center;
    float2 previous = from;
    for (uint i = 1; i <= count; ++i) {
        float angle = sweep * float(i) / float(count);
        float c = cos(angle);
        float s = sin(angle);
        float2 point = i == count ? to : center + float2(radial.x * c - radial.y * s, radial.x * s + radial.y * c);
        strokeLine<Write>(context, emitter, previous, point);
        previous = point;
    }
}

template <bool Write>
static void strokeCap(
    thread FillContext& context,
    thread StrokeEmitter& emitter,
    float2 center,
    float2 from,
    float2 to,
    float2 outward,
    float halfWidth,
    uint cap,
    float deviceRadius,
    float tolerance
) {
    if (cap == kCapRound) {
        strokeArc<Write>(context, emitter, center, from, to, -M_PI_F, deviceRadius, tolerance);
    } else if (cap == kCapSquare) {
        float2 extend = outward * halfWidth;
        strokeLine<Write>(context, emitter, from, from + extend);
        strokeLine<Write>(context, emitter, from + extend, to + extend);
        strokeLine<Write>(context, emitter, to + extend, to);
    } else {
        strokeLine<Write>(context, emitter, from, to);
    }
}

template <bool Write>
static void strokeOuterJoin(
    thread FillContext& context,
    thread StrokeEmitter& emitter,
    float2 center,
    float2 from,
    float2 to,
    float2 fromNormal,
    float2 toNormal,
    float halfWidth,
    uint join,
    float miterLimit,
    float deviceRadius,
    float tolerance
) {
    float alignment = clamp(dot(fromNormal, toNormal), -1.0, 1.0);
    if (join == kJoinRound) {
        strokeArc<Write>(context, emitter, center, from, to, -acos(alignment), deviceRadius, tolerance);
        return;
    }
    float spread = (1.0 + alignment) * 0.5;
    if (join == kJoinMiter && spread * miterLimit * miterLimit >= 1.0 && spread > 1e-6) {
        float2 tip = center + halfWidth * (fromNormal + toNormal) / (1.0 + alignment);
        strokeLine<Write>(context, emitter, from, tip);
        strokeLine<Write>(context, emitter, tip, to);
        return;
    }
    strokeLine<Write>(context, emitter, from, to);
}

template <bool Write>
static void strokeJoin(
    thread FillContext& context,
    thread StrokeEmitter& emitter,
    float2 center,
    float2 incoming,
    float2 outgoing,
    float halfWidth,
    uint join,
    float miterLimit,
    float deviceRadius,
    float tolerance
) {
    float2 inNormal = strokeNormal(incoming);
    float2 outNormal = strokeNormal(outgoing);
    float2 leftFrom = strokeOffset(center, incoming, 1.0, halfWidth);
    float2 leftTo = strokeOffset(center, outgoing, 1.0, halfWidth);
    float2 rightFrom = strokeOffset(center, outgoing, -1.0, halfWidth);
    float2 rightTo = strokeOffset(center, incoming, -1.0, halfWidth);
    float turn = incoming.x * outgoing.y - incoming.y * outgoing.x;
    if (abs(turn) < 1e-5 && dot(incoming, outgoing) > 0.0) {
        strokeLine<Write>(context, emitter, leftFrom, leftTo);
        strokeLine<Write>(context, emitter, rightFrom, rightTo);
        return;
    }
    if (turn >= 0.0) {
        strokeLine<Write>(context, emitter, leftFrom, center);
        strokeLine<Write>(context, emitter, center, leftTo);
        strokeOuterJoin<Write>(
            context, emitter, center, rightFrom, rightTo, -outNormal, -inNormal,
            halfWidth, join, miterLimit, deviceRadius, tolerance
        );
    } else {
        strokeLine<Write>(context, emitter, rightFrom, center);
        strokeLine<Write>(context, emitter, center, rightTo);
        strokeOuterJoin<Write>(
            context, emitter, center, leftFrom, leftTo, inNormal, outNormal,
            halfWidth, join, miterLimit, deviceRadius, tolerance
        );
    }
}

static float2 strokeSample(FillSegment segment, uint index, uint count, float2 startTangent, float2 endTangent, float side, float halfWidth) {
    if (index == 0) {
        return strokeOffset(segment.p0, startTangent, side, halfWidth);
    }
    if (index == count) {
        return strokeOffset(segment.p3, endTangent, side, halfWidth);
    }
    float t = float(index) / float(count);
    return strokeOffset(cubicPoint(segment, t), cubicTangent(segment, t, startTangent), side, halfWidth);
}

static bool dashContinues(float s, float dash) {
    float period = dash * 2.0;
    float start = floor(s / period) * period;
    return start < s && s < start + dash;
}

static float2 strokeCenter(FillSegment segment, uint index, uint count) {
    if (index == 0) {
        return segment.p0;
    }
    if (index == count) {
        return segment.p3;
    }
    return cubicPoint(segment, float(index) / float(count));
}

static float2 strokeTangent(FillSegment segment, uint index, uint count, float2 startTangent, float2 endTangent) {
    if (index == 0) {
        return startTangent;
    }
    if (index == count) {
        return endTangent;
    }
    return cubicTangent(segment, float(index) / float(count), startTangent);
}

template <bool Write>
static bool strokeDashes(
    thread FillContext& context,
    thread StrokeEmitter& emitter,
    FillSegment segment,
    uint count,
    float2 startTangent,
    float2 endTangent,
    float halfWidth,
    uint cap,
    float deviceRadius,
    float tolerance,
    bool ends
) {
    float dash = emitter.draw.dash;
    float period = dash * 2.0;
    float total = 0.0;
    float2 previous = segment.p0;
    for (uint i = 1; i <= count; ++i) {
        float2 point = strokeCenter(segment, i, count);
        total += length(point - previous);
        previous = point;
    }
    float scale = total > 0.0 ? (segment.arcEnd - segment.arcStart) / total : 0.0;
    float travelled = 0.0;
    float s0 = segment.arcStart;
    float2 c0 = segment.p0;
    float2 t0 = startTangent;
    float2 l0 = strokeOffset(c0, t0, 1.0, halfWidth);
    float2 r0 = strokeOffset(c0, t0, -1.0, halfWidth);
    bool continuesStart = dashContinues(s0, dash);
    bool continuesEnd = false;
    for (uint i = 1; i <= count; ++i) {
        float2 c1 = strokeCenter(segment, i, count);
        float2 t1 = strokeTangent(segment, i, count, startTangent, endTangent);
        travelled += length(c1 - c0);
        float s1 = i == count ? segment.arcEnd : segment.arcStart + travelled * scale;
        float2 l1 = strokeOffset(c1, t1, 1.0, halfWidth);
        float2 r1 = strokeOffset(c1, t1, -1.0, halfWidth);
        continuesEnd = !(ends && i == count) && dashContinues(s1, dash);
        bool touchesStart = false;
        bool touchesEnd = false;
        float span = s1 - s0;
        if (span > 0.0) {
            float last = floor(s1 / period) + 1.0;
            for (float k = floor(s0 / period) - 1.0; k <= last; k += 1.0) {
                float start = k * period;
                float u0 = max(start, s0);
                float u1 = min(start + dash, s1);
                if (!(u1 > u0)) {
                    continue;
                }
                bool atStart = u0 == s0;
                bool atEnd = u1 == s1;
                touchesStart = touchesStart || atStart;
                touchesEnd = touchesEnd || atEnd;
                float f0 = (u0 - s0) / span;
                float f1 = (u1 - s0) / span;
                float2 la = mix(l0, l1, f0);
                float2 lb = mix(l0, l1, f1);
                float2 ra = mix(r0, r1, f0);
                float2 rb = mix(r0, r1, f1);
                strokeLine<Write>(context, emitter, la, lb);
                strokeLine<Write>(context, emitter, rb, ra);
                if (!(atStart && continuesStart)) {
                    float2 tangent = strokeDirection(mix(t0, t1, f0), t0);
                    strokeCap<Write>(
                        context, emitter, mix(c0, c1, f0), ra, la, -tangent,
                        halfWidth, cap, deviceRadius, tolerance
                    );
                }
                if (!(atEnd && continuesEnd)) {
                    float2 tangent = strokeDirection(mix(t0, t1, f1), t1);
                    strokeCap<Write>(
                        context, emitter, mix(c0, c1, f1), lb, rb, tangent,
                        halfWidth, cap, deviceRadius, tolerance
                    );
                }
            }
        }
        if (continuesStart && !touchesStart) {
            strokeLine<Write>(context, emitter, l0, r0);
        }
        if (continuesEnd && !touchesEnd) {
            strokeLine<Write>(context, emitter, r1, l1);
        }
        s0 = s1;
        c0 = c1;
        t0 = t1;
        l0 = l1;
        r0 = r1;
        continuesStart = continuesEnd;
    }
    return continuesEnd;
}

template <bool Write>
static uint strokeSegment(
    constant FillParams& params,
    FillDraw draw,
    device const FillSegment* segments,
    uint index,
    thread FillContext& context,
    float2 low,
    float2 high
) {
    FillSegment segment = segments[index];
    uint kind = segment.kind;
    if ((kind & kSegmentImplicit) != 0) {
        return 0;
    }
    StrokeEmitter emitter;
    emitter.draw = draw;
    emitter.low = low;
    emitter.high = high;
    emitter.lines = 0;
    float halfWidth = draw.halfWidth;
    uint cap = draw.strokeStyle & 15u;
    uint join = (draw.strokeStyle >> 4) & 15u;
    float deviceScale = sqrt(draw.a * draw.a + draw.b * draw.b + draw.c * draw.c + draw.d * draw.d);
    float deviceRadius = halfWidth * deviceScale;
    float tolerance = params.tolerance;
    float2 startTangent = strokeStartTangent(segment);
    float2 endTangent = strokeEndTangent(segment);
    uint count = 1;
    if ((kind & kSegmentKindMask) != 0) {
        float2 d0 = transformPoint(draw, segment.p0);
        float2 d1 = transformPoint(draw, segment.p1);
        float2 d2 = transformPoint(draw, segment.p2);
        float2 d3 = transformPoint(draw, segment.p3);
        float bend = max(length(d0 - 2.0 * d1 + d2), length(d1 - 2.0 * d2 + d3));
        float centerSteps = ceil(sqrt(1.5 * bend / tolerance));
        float turning = turnBetween(d1 - d0, d2 - d1) + turnBetween(d2 - d1, d3 - d2);
        if (dot(d1 - d0, d1 - d0) < 1e-12 || dot(d3 - d2, d3 - d2) < 1e-12) {
            turning = max(turning, turnBetween(d2 - d0, d3 - d1));
        }
        float offsetSteps = ceil(turning / sqrt(4.0 * tolerance / max(deviceRadius, tolerance)));
        float steps = max(centerSteps, offsetSteps);
        count = isfinite(steps) ? uint(clamp(steps, 1.0, 1024.0)) : 1u;
    }
    float dashes = (segment.arcEnd - segment.arcStart) / (draw.dash * 2.0);
    if (draw.dash > 0.0 && dashes <= kDashLimit) {
        bool ends = (kind & kSegmentEnd) != 0;
        bool continues = strokeDashes<Write>(
            context, emitter, segment, count, startTangent, endTangent,
            halfWidth, cap, deviceRadius, tolerance, ends
        );
        uint next = index + 1;
        if (ends || !continues || next >= draw.segmentStart + draw.segmentCount) {
            return emitter.lines;
        }
        strokeJoin<Write>(
            context, emitter, segment.p3, endTangent, strokeStartTangent(segments[next]),
            halfWidth, join, params.miterLimit, deviceRadius, tolerance
        );
        return emitter.lines;
    }
    float2 previous = strokeSample(segment, 0, count, startTangent, endTangent, 1.0, halfWidth);
    for (uint i = 1; i <= count; ++i) {
        float2 point = strokeSample(segment, i, count, startTangent, endTangent, 1.0, halfWidth);
        strokeLine<Write>(context, emitter, previous, point);
        previous = point;
    }
    previous = strokeSample(segment, count, count, startTangent, endTangent, -1.0, halfWidth);
    for (uint i = count; i > 0; --i) {
        float2 point = strokeSample(segment, i - 1, count, startTangent, endTangent, -1.0, halfWidth);
        strokeLine<Write>(context, emitter, previous, point);
        previous = point;
    }
    if ((kind & kSegmentStart) != 0 && (kind & kSegmentClosed) == 0) {
        strokeCap<Write>(
            context, emitter, segment.p0,
            strokeOffset(segment.p0, startTangent, -1.0, halfWidth),
            strokeOffset(segment.p0, startTangent, 1.0, halfWidth),
            -startTangent, halfWidth, cap, deviceRadius, tolerance
        );
    }
    if ((kind & kSegmentEnd) != 0 && (kind & kSegmentClosed) == 0) {
        strokeCap<Write>(
            context, emitter, segment.p3,
            strokeOffset(segment.p3, endTangent, 1.0, halfWidth),
            strokeOffset(segment.p3, endTangent, -1.0, halfWidth),
            endTangent, halfWidth, cap, deviceRadius, tolerance
        );
        return emitter.lines;
    }
    uint next = (kind & kSegmentEnd) != 0 ? index - min(segment.link, index) : index + 1;
    if (next < draw.segmentStart || next >= draw.segmentStart + draw.segmentCount) {
        return emitter.lines;
    }
    strokeJoin<Write>(
        context, emitter, segment.p3, endTangent, strokeStartTangent(segments[next]),
        halfWidth, join, params.miterLimit, deviceRadius, tolerance
    );
    return emitter.lines;
}

template <bool Write>
static void binSegment(
    constant FillParams& params,
    device const FillDraw* draws,
    device const uint* segmentBases,
    device const FillSegment* segments,
    thread FillContext& context,
    uint id
) {
    uint drawIndex = findDraw(segmentBases, params.drawCount, id);
    FillDraw draw = draws[drawIndex];
    FillSegment segment = segments[draw.segmentStart + (id - draw.segmentBase)];
    context.gridX = draw.tileX;
    context.gridY = draw.tileY;
    context.tileW = int(draw.tileW);
    context.tileH = int(draw.tileH);
    context.tileBase = draw.tileBase;
    float2 low = float2(draw.tileX, draw.tileY) * kTile - 1.0;
    float2 high = float2(draw.tileX + int(draw.tileW), draw.tileY + int(draw.tileH)) * kTile + 1.0;
    float2 gridLow = low + 1.0;
    float2 gridHigh = high - 1.0;
    if (draw.halfWidth > 0.0) {
        uint lines = strokeSegment<Write>(
            params,
            draw,
            segments,
            draw.segmentStart + (id - draw.segmentBase),
            context,
            low,
            high
        );
        if (!Write) {
            atomic_fetch_add_explicit(&context.stats[2], lines, memory_order_relaxed);
        }
        return;
    }
    float2 p0 = transformPoint(draw, segment.p0);
    float2 p3 = transformPoint(draw, segment.p3);
    if ((segment.kind & kSegmentKindMask) == 0) {
        clipLine<Write>(context, low, high, p0, p3);
        if (!Write) {
            atomic_fetch_add_explicit(&context.stats[2], 1u, memory_order_relaxed);
        }
        return;
    }
    float2 p1 = transformPoint(draw, segment.p1);
    float2 p2 = transformPoint(draw, segment.p2);
    float2 lowest = min(min(p0, p1), min(p2, p3));
    float2 highest = max(max(p0, p1), max(p2, p3));
    if (lowest.x >= gridHigh.x || highest.y < gridLow.y || lowest.y >= gridHigh.y) {
        return;
    }
    uint count = 1;
    if (highest.x < gridLow.x) {
        clipLine<Write>(context, low, high, p0, p3);
    } else {
        float2 bend0 = p0 - 2.0 * p1 + p2;
        float2 bend1 = p1 - 2.0 * p2 + p3;
        float bend = max(length(bend0), length(bend1));
        float steps = ceil(sqrt(0.75 * bend / params.tolerance));
        count = isfinite(steps) ? uint(clamp(steps, 1.0, 1024.0)) : 1u;
        float2 previous = p0;
        for (uint i = 1; i <= count; ++i) {
            float t = float(i) / float(count);
            float u = 1.0 - t;
            float2 point = i == count ? p3 :
                u * u * u * p0 + 3.0 * u * u * t * p1 + 3.0 * u * t * t * p2 + t * t * t * p3;
            clipLine<Write>(context, low, high, previous, point);
            previous = point;
        }
    }
    if (!Write) {
        atomic_fetch_add_explicit(&context.stats[2], count, memory_order_relaxed);
    }
}
)METAL";

static const char* fillCountSource = R"METAL(
kernel void brixGuiMetalFillCount(
    constant FillParams& params [[buffer(0)]],
    device const FillDraw* draws [[buffer(1)]],
    device const uint* segmentBases [[buffer(2)]],
    device const FillSegment* segments [[buffer(3)]],
    device atomic_uint* counts [[buffer(4)]],
    device atomic_int* backdrops [[buffer(5)]],
    device atomic_uint* stats [[buffer(6)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.segmentCount) {
        return;
    }
    FillContext context;
    context.pieceCapacity = params.pieceCapacity;
    context.counts = counts;
    context.backdrops = backdrops;
    context.cursors = nullptr;
    context.starts = nullptr;
    context.pieces = nullptr;
    context.stats = stats;
    binSegment<false>(params, draws, segmentBases, segments, context, id);
}
)METAL";

static const char* fillWriteSource = R"METAL(
kernel void brixGuiMetalFillWrite(
    constant FillParams& params [[buffer(0)]],
    device const FillDraw* draws [[buffer(1)]],
    device const uint* segmentBases [[buffer(2)]],
    device const FillSegment* segments [[buffer(3)]],
    device const uint* starts [[buffer(4)]],
    device atomic_uint* cursors [[buffer(5)]],
    device float4* pieces [[buffer(6)]],
    device atomic_uint* stats [[buffer(7)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.segmentCount) {
        return;
    }
    FillContext context;
    context.pieceCapacity = params.pieceCapacity;
    context.counts = nullptr;
    context.backdrops = nullptr;
    context.cursors = cursors;
    context.starts = starts;
    context.pieces = pieces;
    context.stats = stats;
    binSegment<true>(params, draws, segmentBases, segments, context, id);
}
)METAL";

static const char* fillKernelSource = R"METAL(
kernel void brixGuiMetalImageMip(
    texture2d<float, access::read> source [[texture(0)]],
    texture2d<float, access::write> target [[texture(1)]],
    constant uint4& region [[buffer(0)]],
    uint2 id [[thread_position_in_grid]]
) {
    uint2 texel = region.xy + id;
    if (texel.x >= region.z || texel.y >= region.w) {
        return;
    }
    float2 ratio = float2(source.get_width(), source.get_height()) /
        float2(target.get_width(), target.get_height());
    float2 low = float2(texel) * ratio;
    float2 high = float2(texel + 1) * ratio;
    uint2 first = uint2(floor(low));
    uint2 last = min(uint2(ceil(high)), uint2(source.get_width(), source.get_height()));
    float4 sum = 0;
    float total = 0;
    for (uint y = first.y; y < last.y; ++y) {
        float wy = min(high.y, float(y + 1)) - max(low.y, float(y));
        for (uint x = first.x; x < last.x; ++x) {
            float weight = wy * (min(high.x, float(x + 1)) - max(low.x, float(x)));
            sum += source.read(uint2(x, y)) * weight;
            total += weight;
        }
    }
    target.write(total > 0 ? sum / total : float4(0), texel);
}

struct ImageTileParams {
    int originX;
    int originY;
    uint regionLeft;
    uint regionTop;
    uint regionRight;
    uint regionBottom;
    uint width;
    uint height;
    uint level;
    uint row;
    float stepX;
    float stepY;
    uint preview;
    uint format;
};

static float4 imageTileTexel(device const uchar4* source, ulong index, uint format) {
    return format != 0u ? float4(((device const half4*)source)[index]) : float4(source[index].zyxw);
}

kernel void brixGuiMetalImageTile(
    device const uchar4* source [[buffer(0)]],
    constant ImageTileParams& params [[buffer(1)]],
    texture2d<float, access::write> target [[texture(0)]],
    uint2 id [[thread_position_in_grid]]
) {
    uint2 texel = uint2(params.regionLeft, params.regionTop) + id;
    if (texel.x >= params.regionRight || texel.y >= params.regionBottom) {
        return;
    }
    uint2 size = uint2(params.width, params.height);
    float4 color;
    if (params.preview != 0u) {
        float2 center = (float2(texel) + 0.5) * float2(params.stepX, params.stepY);
        uint2 low = min(uint2(center), size - 2u);
        ulong top = ulong(low.y) * params.row + low.x;
        ulong bottom = top + params.row;
        color = imageTileTexel(source, top, params.format) + imageTileTexel(source, top + 1, params.format) +
            imageTileTexel(source, bottom, params.format) + imageTileTexel(source, bottom + 1, params.format);
        color = params.format != 0u ? color / 4.0 : floor((color + 2.0) / 4.0);
    } else {
        uint scale = 1u << params.level;
        int2 extent = int2((size + scale - 1u) / scale);
        int2 pixel = clamp(int2(params.originX, params.originY) + int2(texel), int2(0), extent - 1);
        uint2 low = uint2(pixel) * scale;
        uint2 high = min(low + scale, size);
        float4 sum = 0;
        for (uint y = low.y; y < high.y; ++y) {
            ulong start = ulong(y) * params.row;
            for (uint x = low.x; x < high.x; ++x) {
                sum += imageTileTexel(source, start + x, params.format);
            }
        }
        color = sum / float((high.x - low.x) * (high.y - low.y));
    }
    target.write(params.format != 0u ? color : color / 255.0, texel);
}

kernel void brixGuiMetalReadback(
    texture2d<float, access::read> source [[texture(0)]],
    device float4* target [[buffer(0)]],
    constant uint4& region [[buffer(1)]],
    uint2 id [[thread_position_in_grid]]
) {
    if (id.x >= region.z || id.y >= region.w) {
        return;
    }
    target[id.y * region.z + id.x] = source.read(region.xy + id);
}

kernel void brixGuiMetalFillInit(
    constant FillParams& params [[buffer(0)]],
    device const uint* tileBases [[buffer(1)]],
    device uint* tileDraws [[buffer(2)]],
    device uint* counts [[buffer(3)]],
    device int* backdrops [[buffer(4)]],
    device uint* cursors [[buffer(5)]],
    device uint* occluders [[buffer(6)]],
    uint id [[thread_position_in_grid]]
) {
    if (id < params.gridWidth * params.gridHeight) {
        occluders[id] = 0;
    }
    if (id >= params.tileCount) {
        return;
    }
    tileDraws[id] = findDraw(tileBases, params.drawCount, id);
    counts[id] = 0;
    backdrops[id] = 0;
    cursors[id] = 0;
}

kernel void brixGuiMetalFillAllocate(
    constant FillParams& params [[buffer(0)]],
    device const uint* counts [[buffer(1)]],
    device uint* starts [[buffer(2)]],
    device atomic_uint* stats [[buffer(3)]],
    uint id [[thread_position_in_grid]],
    uint lane [[thread_position_in_threadgroup]]
) {
    threadgroup uint scratch[256];
    threadgroup uint base;
    uint count = id < params.tileCount ? counts[id] : 0;
    scratch[lane] = count;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint offset = 1; offset < 256; offset <<= 1) {
        uint value = lane >= offset ? scratch[lane - offset] : 0;
        threadgroup_barrier(mem_flags::mem_threadgroup);
        scratch[lane] += value;
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    if (lane == 255) {
        base = atomic_fetch_add_explicit(&stats[0], scratch[255], memory_order_relaxed);
        if (base + scratch[255] > params.pieceCapacity) {
            atomic_store_explicit(&stats[1], 1u, memory_order_relaxed);
        }
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (id < params.tileCount) {
        starts[id] = base + scratch[lane] - count;
    }
}

kernel void brixGuiMetalFillBackdrop(
    constant FillParams& params [[buffer(0)]],
    device const FillDraw* draws [[buffer(1)]],
    device const uint* rowBases [[buffer(2)]],
    device int* backdrops [[buffer(3)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.rowCount) {
        return;
    }
    FillDraw draw = draws[findDraw(rowBases, params.drawCount, id)];
    uint start = draw.tileBase + (id - draw.rowBase) * draw.tileW;
    int sum = 0;
    for (uint column = 0; column < draw.tileW; ++column) {
        sum += backdrops[start + column];
        backdrops[start + column] = sum;
    }
}
)METAL";

static const char* fillCoverageSource = R"METAL(
static float resolveCoverage(float winding, uint fillRule) {
    return fillRule == 0 ? min(abs(winding), 1.0) : abs(winding - 2.0 * round(winding * 0.5));
}

static float2 tileOrigin(FillDraw draw, uint tile) {
    uint local = tile - draw.tileBase;
    return float2(
        float(draw.tileX + int(local % draw.tileW)),
        float(draw.tileY + int(local / draw.tileW))
    ) * kTile;
}

static float clipFactor(uint clip, float coverage) {
    return (clip & kClipMode) == kClipInside ? coverage : 1.0 - coverage;
}

static int coverageTile(int2 cell, CoverageClip clip, device const FillDraw* draws, uint drawCount) {
    if (clip.draw >= drawCount) {
        return -1;
    }
    FillDraw source = draws[clip.draw];
    int2 local = cell - int2(source.tileX, source.tileY);
    if (local.x < 0 || local.y < 0 || local.x >= int(source.tileW) || local.y >= int(source.tileH)) {
        return -1;
    }
    return int(source.tileBase + uint(local.y) * source.tileW + uint(local.x));
}

)METAL";

static const char* fillTileSource = R"METAL(
static uint drawTileClass(
    uint tile,
    FillDraw draw,
    device const uint* counts,
    device const int* backdrops
) {
    if (counts[tile] > 0) {
        return 1;
    }
    float coverage = resolveCoverage(float(backdrops[tile]), draw.fillRule & 0xffu);
    if (coverage <= 0.0) {
        return 0;
    }
    if (coverage < 1.0 || draw.color.a < 1.0 || draw.roundedClip != 0u || (draw.paint >> 31u) != 0u ||
            (draw.fillRule >> 8u) != 0u) {
        return 2;
    }
    float2 origin = tileOrigin(draw, tile);
    bool inside = origin.x >= draw.mask.x && origin.y >= draw.mask.y &&
        origin.x + kTile <= draw.mask.z && origin.y + kTile <= draw.mask.w;
    return inside ? 3 : 2;
}

static uint tileClass(
    uint tile,
    constant FillParams& params,
    device const FillDraw* draws,
    device const uint* tileDraws,
    device const uint* counts,
    device const int* backdrops,
    device const CoverageClip* clips
) {
    FillDraw draw = draws[tileDraws[tile]];
    if ((draw.clip & kClipMode) == kClipSource) {
        return 0;
    }
    uint kind = drawTileClass(tile, draw, counts, backdrops);
    uint index = draw.coverage;
    if (kind == 0u || index == 0u) {
        return kind;
    }
    uint local = tile - draw.tileBase;
    int2 cell = int2(draw.tileX + int(local % draw.tileW), draw.tileY + int(local / draw.tileW));
    while (index != 0u) {
        CoverageClip clip = clips[index - 1u];
        if ((clip.mode & (kClipMask | kClipLayerMask | kClipShadow)) != 0u) {
            kind = min(kind, 2u);
            index = min(clip.parent, index - 1u);
            continue;
        }
        int clipTile = coverageTile(cell, clip, draws, params.drawCount);
        if (clipTile >= 0 && counts[clipTile] > 0) {
            kind = 1;
        } else if (clipFactor(clip.mode, clipTile >= 0 ? resolveCoverage(
                float(backdrops[clipTile]),
                (clip.mode & kClipEvenOdd) != 0u ? 1u : 0u
            ) : 0.0) <= 0.0) {
            return 0;
        }
        index = min(clip.parent, index - 1u);
    }
    return kind;
}

static int tileCell(uint tile, FillDraw draw, constant FillParams& params) {
    uint local = tile - draw.tileBase;
    int x = draw.tileX + int(local % draw.tileW);
    int y = draw.tileY + int(local / draw.tileW);
    if (x < 0 || y < 0 || x >= int(params.gridWidth) || y >= int(params.gridHeight)) {
        return -1;
    }
    return y * int(params.gridWidth) + x;
}

kernel void brixGuiMetalFillOcclude(
    constant FillParams& params [[buffer(0)]],
    device const FillDraw* draws [[buffer(1)]],
    device const uint* tileDraws [[buffer(2)]],
    device const uint* counts [[buffer(3)]],
    device const int* backdrops [[buffer(4)]],
    device uchar* kinds [[buffer(5)]],
    device atomic_uint* occluders [[buffer(6)]],
    device const CoverageClip* clips [[buffer(9)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.tileCount) {
        return;
    }
    uint kind = tileClass(id, params, draws, tileDraws, counts, backdrops, clips);
    kinds[id] = uchar(kind);
    if (kind != 3u) {
        return;
    }
    uint index = tileDraws[id];
    int cell = tileCell(id, draws[index], params);
    if (cell >= 0) {
        atomic_fetch_max_explicit(&occluders[cell], index + 1u, memory_order_relaxed);
    }
}

static uint visibleKind(
    uint tile,
    constant FillParams& params,
    device const FillDraw* draws,
    device const uint* tileDraws,
    device const uchar* kinds,
    device const uint* occluders
) {
    uint kind = kinds[tile];
    if (kind == 0u) {
        return 0u;
    }
    uint index = tileDraws[tile];
    FillDraw draw = draws[index];
    int cell = tileCell(tile, draw, params);
    if (cell < 0) {
        return kind;
    }
    uint top = occluders[cell];
    return top > index + 1u && draws[top - 1u].batch == draw.batch ? 0u : kind;
}

kernel void brixGuiMetalFillMark(
    constant FillParams& params [[buffer(0)]],
    device const FillDraw* draws [[buffer(1)]],
    device const uint* tileDraws [[buffer(2)]],
    device const uchar* kinds [[buffer(3)]],
    device const uint* occluders [[buffer(4)]],
    device uint* groupSums [[buffer(5)]],
    device atomic_uint* stats [[buffer(6)]],
    device uint* groupOpaque [[buffer(7)]],
    uint id [[thread_position_in_grid]],
    uint lane [[thread_position_in_threadgroup]],
    uint group [[threadgroup_position_in_grid]]
) {
    threadgroup uint scratch[256];
    threadgroup uint solid[256];
    uint kind = id < params.tileCount ?
        visibleKind(id, params, draws, tileDraws, kinds, occluders) : 0;
    scratch[lane] = kind == 1 ? 1u : (kind == 2 ? 1u << 16 : 0u);
    solid[lane] = kind == 3 ? 1u : 0u;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = 128; stride > 0; stride >>= 1) {
        if (lane < stride) {
            scratch[lane] += scratch[lane + stride];
            solid[lane] += solid[lane + stride];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    if (lane == 0) {
        uint edges = scratch[0] & 0xffffu;
        uint translucent = scratch[0] >> 16;
        groupSums[group] = edges + translucent;
        groupOpaque[group] = solid[0];
        atomic_fetch_add_explicit(&stats[3], edges, memory_order_relaxed);
        atomic_fetch_add_explicit(&stats[4], translucent + solid[0], memory_order_relaxed);
        atomic_fetch_add_explicit(&stats[5], solid[0], memory_order_relaxed);
    }
}

kernel void brixGuiMetalFillScanGroups(
    device uint* groupSums [[buffer(1)]],
    device uint* total [[buffer(2)]],
    constant uint& groupCount [[buffer(3)]],
    uint lane [[thread_position_in_threadgroup]]
) {
    threadgroup uint partial[256];
    uint per = (groupCount + 255) / 256;
    uint begin = min(lane * per, groupCount);
    uint end = min(begin + per, groupCount);
    uint sum = 0;
    for (uint i = begin; i < end; ++i) {
        sum += groupSums[i];
    }
    partial[lane] = sum;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint offset = 1; offset < 256; offset <<= 1) {
        uint value = lane >= offset ? partial[lane - offset] : 0;
        threadgroup_barrier(mem_flags::mem_threadgroup);
        partial[lane] += value;
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    uint running = partial[lane] - sum;
    for (uint i = begin; i < end; ++i) {
        uint value = groupSums[i];
        groupSums[i] = running;
        running += value;
    }
    if (lane == 255) {
        total[0] = partial[255];
    }
}

kernel void brixGuiMetalFillScatter(
    constant FillParams& params [[buffer(0)]],
    device const FillDraw* draws [[buffer(1)]],
    device const uint* tileDraws [[buffer(2)]],
    device const uchar* kinds [[buffer(3)]],
    device const uint* occluders [[buffer(4)]],
    device const uint* groupOffsets [[buffer(5)]],
    device uint* visibleTiles [[buffer(6)]],
    device const uint* opaqueOffsets [[buffer(7)]],
    device uint* opaqueTiles [[buffer(8)]],
    uint id [[thread_position_in_grid]],
    uint lane [[thread_position_in_threadgroup]],
    uint group [[threadgroup_position_in_grid]]
) {
    threadgroup uint scratch[256];
    uint kind = id < params.tileCount ?
        visibleKind(id, params, draws, tileDraws, kinds, occluders) : 0;
    uint flag = kind == 1 || kind == 2 ? 1u : (kind == 3 ? 1u << 16 : 0u);
    scratch[lane] = flag;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint offset = 1; offset < 256; offset <<= 1) {
        uint value = lane >= offset ? scratch[lane - offset] : 0;
        threadgroup_barrier(mem_flags::mem_threadgroup);
        scratch[lane] += value;
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    if (kind == 1 || kind == 2) {
        visibleTiles[groupOffsets[group] + (scratch[lane] & 0xffffu) - 1] = id;
    } else if (kind == 3) {
        opaqueTiles[opaqueOffsets[group] + (scratch[lane] >> 16) - 1] = id;
    }
}

kernel void brixGuiMetalFillBatchBounds(
    constant FillParams& params [[buffer(0)]],
    device const FillDraw* draws [[buffer(1)]],
    device const uint* tileDraws [[buffer(2)]],
    device const uint* visibleTiles [[buffer(3)]],
    device const uint* opaqueTiles [[buffer(4)]],
    device const uint* stats [[buffer(5)]],
    device uint* ranges [[buffer(6)]],
    uint id [[thread_position_in_grid]]
) {
    uint visible = stats[6];
    uint opaque = stats[7];
    if (id < visible) {
        uint batch = draws[tileDraws[visibleTiles[id]]].batch;
        if (id == 0 || draws[tileDraws[visibleTiles[id - 1]]].batch != batch) {
            ranges[batch * 4u] = id;
        }
        if (id + 1 == visible || draws[tileDraws[visibleTiles[id + 1]]].batch != batch) {
            ranges[batch * 4u + 1u] = id + 1;
        }
    }
    if (id < opaque) {
        uint batch = draws[tileDraws[opaqueTiles[id]]].batch;
        if (id == 0 || draws[tileDraws[opaqueTiles[id - 1]]].batch != batch) {
            ranges[batch * 4u + 2u] = id;
        }
        if (id + 1 == opaque || draws[tileDraws[opaqueTiles[id + 1]]].batch != batch) {
            ranges[batch * 4u + 3u] = id + 1;
        }
    }
}

kernel void brixGuiMetalFillBatchArgs(
    constant FillParams& params [[buffer(0)]],
    device const uint4* ranges [[buffer(1)]],
    device const uint* stats [[buffer(2)]],
    device uint* args [[buffer(3)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.batchCount) {
        return;
    }
    uint4 range = ranges[id];
    bool overflow = stats[1] != 0u;
    uint visibleTotal = min(stats[6], params.tileCount);
    uint opaqueTotal = min(stats[7], params.tileCount);
    bool visibleValid = !overflow && range.x < range.y && range.y <= visibleTotal;
    bool opaqueValid = !overflow && range.z < range.w && range.w <= opaqueTotal;
    device uint* visible = args + id * 8;
    visible[0] = 4;
    visible[1] = visibleValid ? range.y - range.x : 0u;
    visible[2] = 0;
    visible[3] = 0;
    visible[4] = 4;
    visible[5] = opaqueValid ? range.w - range.z : 0u;
    visible[6] = 0;
    visible[7] = 0;
}

)METAL";

static const char* fillDrawSource = R"METAL(
struct FillVertexOut {
    float4 position [[position]];
    float2 origin [[flat]];
    float4 color [[flat]];
    float4 mask [[flat]];
    uint tile [[flat]];
    uint fillRule [[flat]];
    uint roundedClip [[flat]];
    uint coverage [[flat]];
    uint paint [[flat]];
    uint blend [[flat]];
};

vertex FillVertexOut brixGuiMetalFillVertex(
    uint vertexId [[vertex_id]],
    uint instanceId [[instance_id]],
    constant FillParams& params [[buffer(0)]],
    device const FillDraw* draws [[buffer(1)]],
    device const uint* tileDraws [[buffer(2)]],
    device const uint* tiles [[buffer(3)]],
    device const uint4* ranges [[buffer(4)]],
    constant uint2& selection [[buffer(5)]]
) {
    uint4 range = ranges[selection.x];
    uint index = selection.y != 0u ? range.w - 1u - instanceId : range.x + instanceId;
    uint tile = tiles[min(index, params.tileCount - 1u)];
    tile = min(tile, params.tileCount - 1u);
    uint drawIndex = min(tileDraws[tile], params.drawCount - 1u);
    FillDraw draw = draws[drawIndex];
    float2 origin = tileOrigin(draw, tile);
    float2 low = max(origin, draw.extent.xy);
    float2 high = max(min(origin + kTile, draw.extent.zw), low);
    float2 corner = float2(float(vertexId & 1u), float(vertexId >> 1u));
    float2 pixel = mix(low, high, corner);
    FillVertexOut out;
    out.position = float4(
        (pixel.x - params.originX) / params.viewportWidth * 2.0 - 1.0,
        1.0 - (pixel.y - params.originY) / params.viewportHeight * 2.0,
        1.0 - float(drawIndex + 1) / float(params.drawCount + 1),
        1.0
    );
    out.origin = origin;
    out.color = draw.color;
    out.mask = draw.mask;
    out.tile = tile;
    out.fillRule = draw.fillRule & 0xffu;
    out.blend = draw.fillRule >> 8u;
    out.roundedClip = draw.roundedClip;
    out.coverage = draw.coverage;
    out.paint = draw.paint;
    return out;
}

constant float kConicSeam = 1.0 / 1290.0;
constant float kImageTaps[9] = {0.0, 0.0625, 0.125, 0.25, 0.5, 0.75, 0.875, 0.9375, 1.0};

static float4 stopColor(float t, uint start, uint count, device const FillStop* stops) {
    FillStop first = stops[start];
    if (t <= first.offset) {
        return first.color;
    }
    for (uint i = 1u; i < count; ++i) {
        FillStop next = stops[start + i];
        if (t < next.offset) {
            FillStop previous = stops[start + i - 1u];
            float span = next.offset - previous.offset;
            return span > 0.0 ? mix(previous.color, next.color, (t - previous.offset) / span) : next.color;
        }
    }
    return stops[start + count - 1u].color;
}

static float4 paintColor(
    float2 point,
    uint paint,
    device const FillPaint* paints,
    device const FillStop* stops,
    texture2d<float> atlas0,
    texture2d<float> atlas1
) {
    FillPaint record = paints[(paint & 0x7fffffffu) - 1u];
    float2 q = float2(
        record.a * point.x + record.c * point.y + record.tx,
        record.b * point.x + record.d * point.y + record.ty
    );
#if defined(BRIX_USER_SHADER) && !defined(BRIX_USER_DAB)
    if (record.kind == 6u || record.kind == 7u) {
        ShaderArgs args = {stops, record.stopStart, record.stopCount, record.kind == 6u ? record.p2 : 0.0};
#if defined(BRIX_USER_PAINT)
        PaintInput input;
        input.position = q;
        input.paper = (point - float2(record.paperX, record.paperY)) * as_type<float>(record.padding);
        input.color = float4(brixLinearColor(float3(record.p0, record.p1, record.p2), 0u, kBrixOutputSpace), record.p3);
        input.pixel = sqrt(abs(record.a * record.d - record.b * record.c));
        float4 color = ::paint(input, args);
#else
        float4 color = shade(q, float2(record.p0, record.p1), args);
#endif
        float3 encoded = brixEncodeColor(color.rgb, kBrixOutputSpace);
        return float4(kBrixExtendedOutput ? encoded : saturate(encoded), saturate(color.a));
    }
#endif
    if (record.kind >= 4u) {
        constexpr sampler bilinear(coord::normalized, address::clamp_to_edge, filter::linear);
        float2 origin = float2(record.p0, record.p1);
        float2 extent = float2(record.p2, record.p3);
        float2 size = float2(atlas0.get_width(), atlas0.get_height());
        float2 texelPosition = clamp((origin + q * extent) * size, origin * size + 0.5, (origin + extent) * size - 0.5);
        float2 base = floor(texelPosition - 0.5);
        uint2 step = uint2(floor((texelPosition - 0.5 - base) * 8.0 + 0.5));
        float2 coord = (base + 0.5 + float2(kImageTaps[step.x], kImageTaps[step.y])) / size;
        float4 texel = record.padding == 0u ? atlas0.sample(bilinear, coord) : atlas1.sample(bilinear, coord);
        float2 width = float2(abs(record.a) + abs(record.c), abs(record.b) + abs(record.d));
        float2 ramp = saturate(0.5 + min(q, 1.0 - q) / width);
        float edge = ramp.x * ramp.y;
        if (record.kind == 5u) {
            return float4(1.0, 1.0, 1.0, texel.a * edge);
        }
        return texel.a > 0.0 ? float4(texel.rgb / texel.a, texel.a * edge) : float4(0.0);
    }
    float t;
    if (record.kind == 1u) {
        t = q.x * record.p0 + q.y * record.p1;
    } else if (record.kind == 2u) {
        float2 delta = float2(record.p0, record.p1);
        float r0 = record.p2;
        float dr = record.p3;
        float a = dot(delta, delta) - dr * dr;
        float b = dot(q, delta) + r0 * dr;
        float c = dot(q, q) - r0 * r0;
        if (abs(a) < 1e-6 * max(dot(delta, delta), 1.0)) {
            if (b == 0.0) {
                return float4(0.0);
            }
            t = c / (2.0 * b);
            if (r0 + t * dr < 0.0) {
                return float4(0.0);
            }
        } else {
            float discriminant = b * b - a * c;
            if (discriminant < 0.0) {
                return float4(0.0);
            }
            float root = sqrt(discriminant);
            float high = max((b + root) / a, (b - root) / a);
            float low = min((b + root) / a, (b - root) / a);
            if (r0 + high * dr >= 0.0) {
                t = high;
            } else if (r0 + low * dr >= 0.0) {
                t = low;
            } else {
                return float4(0.0);
            }
        }
    } else {
        t = fract((atan2(q.y, q.x) - record.p0) * 0.15915494309189535);
        float seam = (t - (1.0 - kConicSeam)) / kConicSeam;
        if (seam > 0.0) {
            return mix(
                stopColor(t, record.stopStart, record.stopCount, stops),
                stopColor(0.0, record.stopStart, record.stopCount, stops),
                min(seam, 1.0)
            );
        }
    }
    return stopColor(t, record.stopStart, record.stopCount, stops);
}

static bool paintDithers(uint paint, device const FillPaint* paints) {
    uint kind = paints[(paint & 0x7fffffffu) - 1u].kind;
    return !kBrixExtendedOutput && kind >= 1u && kind <= 3u;
}

static float ditherNoise(float2 point) {
    return fract(52.9829189 * fract(dot(floor(point), float2(0.06711056, 0.00583715)))) - 0.5;
}

fragment float4 brixGuiMetalFillSolidFragment(
    FillVertexOut in [[stage_in]],
    constant FillParams& params [[buffer(5)]],
    device const FillPaint* paints [[buffer(9)]],
    device const FillStop* stops [[buffer(10)]],
    texture2d<float> atlas0 [[texture(1)]],
    texture2d<float> atlas1 [[texture(2)]]
) {
    if (in.paint == 0u) {
        return float4(in.color.rgb, 1.0);
    }
    float2 point = in.position.xy + float2(params.originX, params.originY);
    float3 rgb = in.color.rgb * paintColor(point, in.paint, paints, stops, atlas0, atlas1).rgb;
    if (paintDithers(in.paint, paints)) {
        rgb += ditherNoise(point) / 255.0;
    }
    return float4(rgb, 1.0);
}

static float rightArea(float u) {
    return u <= 0.0 ? u : (u >= 1.0 ? 0.5 : u - 0.5 * u * u);
}

static float pieceCoverage(float4 piece, float px, float py) {
    float y0 = clamp(piece.y, py, py + 1.0);
    float y1 = clamp(piece.w, py, py + 1.0);
    float dy = y1 - y0;
    if (dy == 0.0) {
        return 0.0;
    }
    float slope = (piece.z - piece.x) / (piece.w - piece.y);
    float u0 = piece.x + (y0 - piece.y) * slope - px;
    float u1 = piece.x + (y1 - piece.y) * slope - px;
    float span = u1 - u0;
    float average = abs(span) < 1e-5 ?
        clamp(1.0 - 0.5 * (u0 + u1), 0.0, 1.0) :
        (rightArea(u1) - rightArea(u0)) / span;
    return dy * average;
}

static bool pieceTouches(float4 piece, float2 local) {
    return max(piece.y, piece.w) > local.y && min(piece.y, piece.w) < local.y + 1.0 &&
        min(piece.x, piece.z) < local.x + 1.0;
}

static bool pieceSpans(float4 piece, float2 local) {
    return max(piece.x, piece.z) <= local.x && min(piece.y, piece.w) <= local.y &&
        max(piece.y, piece.w) >= local.y + 1.0;
}

static float3 stackSpan(float3 stack, float4 piece, float2 local) {
    float low = clamp(min(piece.y, piece.w) - local.y, 0.0, 1.0);
    float high = clamp(max(piece.y, piece.w) - local.y, 0.0, 1.0);
    return float3(stack.x + high - low, min(stack.y, low), max(stack.z, high));
}

static bool stackOverlaps(float3 stack) {
    return stack.x > max(stack.z - stack.y, 0.0) + 1e-3;
}

constant uint kStackLimit = 8u;
constant uint kStackRows = 16u;

static float stackedCoverage(
    uint fillRule,
    uint tile,
    float2 local,
    device const float4* pieces,
    uint start,
    uint count,
    device const int* backdrops
) {
    float rows = float(kStackRows);
    float coverage = 0.0;
    for (uint row = 0; row < kStackRows; ++row) {
        float top = local.y + float(row) / rows;
        float bottom = top + 1.0 / rows;
        float level = float(backdrops[tile]);
        float edges[kStackLimit];
        float steps[kStackLimit];
        uint found = 0;
        for (uint i = 0; i < count; ++i) {
            float4 piece = pieces[start + i];
            float low = max(min(piece.y, piece.w), top);
            float high = min(max(piece.y, piece.w), bottom);
            if (high <= low || min(piece.x, piece.z) >= local.x + 1.0) {
                continue;
            }
            float weight = (high - low) * rows * (piece.w > piece.y ? 1.0 : -1.0);
            float middle = 0.5 * (low + high);
            float edge = piece.x + (middle - piece.y) * (piece.z - piece.x) / (piece.w - piece.y) - local.x;
            if (edge <= 0.0) {
                level += weight;
            } else if (edge >= 1.0) {
                continue;
            } else if (found < kStackLimit) {
                edges[found] = edge;
                steps[found] = weight;
                ++found;
            } else {
                uint nearest = 0;
                for (uint j = 1; j < found; ++j) {
                    if (abs(edges[j] - edge) < abs(edges[nearest] - edge)) {
                        nearest = j;
                    }
                }
                steps[nearest] += weight;
            }
        }
        float first = 1.0;
        for (uint i = 0; i < found; ++i) {
            first = min(first, edges[i]);
        }
        float span = first * resolveCoverage(level, fillRule);
        for (uint i = 0; i < found; ++i) {
            float reached = level;
            float next = 1.0;
            bool repeated = false;
            for (uint j = 0; j < found; ++j) {
                if (edges[j] <= edges[i]) {
                    reached += steps[j];
                    repeated = repeated || (j < i && edges[j] == edges[i]);
                } else {
                    next = min(next, edges[j]);
                }
            }
            if (!repeated) {
                span += (next - edges[i]) * resolveCoverage(reached, fillRule);
            }
        }
        coverage += span / rows;
    }
    return coverage;
}

static float tileCoverage(
    uint tile,
    float2 local,
    uint fillRule,
    device const float4* pieces,
    device const uint* starts,
    device const uint* counts,
    device const int* backdrops,
    constant FillParams& params
) {
    float winding = float(backdrops[tile]);
    uint start = min(starts[tile], params.pieceCapacity);
    uint count = min(counts[tile], params.pieceCapacity - start);
    float3 rising = float3(0.0, 1.0, 0.0);
    float3 falling = float3(0.0, 1.0, 0.0);
    for (uint i = 0; i < count; ++i) {
        float4 piece = pieces[start + i];
        if (!pieceTouches(piece, local)) {
            continue;
        }
        winding += pieceCoverage(piece, local.x, local.y);
        if (piece.w == piece.y || pieceSpans(piece, local)) {
            continue;
        }
        if (piece.w > piece.y) {
            rising = stackSpan(rising, piece, local);
        } else {
            falling = stackSpan(falling, piece, local);
        }
    }
    if (!stackOverlaps(rising) && !stackOverlaps(falling)) {
        return resolveCoverage(winding, fillRule);
    }
    return stackedCoverage(fillRule, tile, local, pieces, start, count, backdrops);
}

static float coverageFactor(
    float2 point,
    uint index,
    device const CoverageClip* clips,
    device const FillDraw* draws,
    device const float4* pieces,
    device const uint* starts,
    device const uint* counts,
    device const int* backdrops,
    constant FillParams& params,
    device const FillPaint* paints,
    device const FillStop* stops,
    texture2d<float> atlas0,
    texture2d<float> atlas1,
    array<texture2d<float>, 4> maskPages
) {
    int2 cell = int2(floor(point / kTile));
    float2 local = floor(point - float2(cell) * kTile);
    float coverage = 1.0;
    while (index != 0u && coverage > 0.0) {
        CoverageClip clip = clips[index - 1u];
        if ((clip.mode & kClipShadow) != 0u) {
            int2 low = int2(clip.bounds & 0xffffu, clip.bounds >> 16u);
            int2 origin = int2(clip.draw & 0xffffu, clip.draw >> 16u);
            int2 texel = max(int2(floor(point)) - low + origin, int2(0));
            coverage *= maskPages[(clip.mode >> 10u) & 3u].read(uint2(texel))[(clip.mode >> 8u) & 3u];
            index = min(clip.parent, index - 1u);
            continue;
        }
        if ((clip.mode & kClipLayerMask) != 0u) {
            uint2 texel = uint2(max(point, 0.0));
            uint2 low = uint2(clip.draw & 0xffffu, clip.draw >> 16u);
            uint2 high = uint2(clip.bounds & 0xffffu, clip.bounds >> 16u);
            bool inside = all(texel >= low) && all(texel < high);
            coverage *= inside ? maskPages[(clip.mode >> 8u) & 3u].read(texel).a : 0.0;
            index = min(clip.parent, index - 1u);
            continue;
        }
        if ((clip.mode & kClipMask) != 0u) {
            coverage *= paintColor(point, clip.draw, paints, stops, atlas0, atlas1).a;
            index = min(clip.parent, index - 1u);
            continue;
        }
        int clipTile = coverageTile(cell, clip, draws, params.drawCount);
        uint clipRule = (clip.mode & kClipEvenOdd) != 0u ? 1u : 0u;
        float clipCoverage = clipTile >= 0 ?
            tileCoverage(uint(clipTile), local, clipRule, pieces, starts, counts, backdrops, params) : 0.0;
        coverage *= clipFactor(clip.mode, clipCoverage);
        index = min(clip.parent, index - 1u);
    }
    return coverage;
}

fragment float4 brixGuiMetalFillFragment(
    FillVertexOut in [[stage_in]],
    const device RoundedClip* clips [[buffer(0)]],
    device const float4* pieces [[buffer(1)]],
    device const uint* starts [[buffer(2)]],
    device const uint* counts [[buffer(3)]],
    device const int* backdrops [[buffer(4)]],
    constant FillParams& params [[buffer(5)]],
    device const FillDraw* draws [[buffer(6)]],
    device const CoverageClip* coverageClips [[buffer(7)]],
    device const FillPaint* paints [[buffer(9)]],
    device const FillStop* stops [[buffer(10)]],
    texture2d<float> atlas0 [[texture(1)]],
    texture2d<float> atlas1 [[texture(2)]],
    array<texture2d<float>, 4> maskPages [[texture(3)]],
    float4 dest [[color(0), function_constant(kBlendDraw)]]
) {
    float2 point = in.position.xy + float2(params.originX, params.originY);
    float2 local = floor(point - in.origin);
    float fill = tileCoverage(in.tile, local, in.fillRule, pieces, starts, counts, backdrops, params);
    float inside = step(in.mask.x, point.x) * step(in.mask.y, point.y) *
        step(point.x, in.mask.z) * step(point.y, in.mask.w);
    float4 color = in.paint != 0u
        ? in.color * paintColor(point, in.paint, paints, stops, atlas0, atlas1)
        : in.color;
    float shape = fill * inside *
        roundedClipCoverage(point, in.roundedClip, clips);
    if (in.coverage != 0u) {
        shape *= coverageFactor(
            point,
            in.coverage,
            coverageClips,
            draws,
            pieces,
            starts,
            counts,
            backdrops,
            params,
            paints,
            stops,
            atlas0,
            atlas1,
            maskPages
        );
    }
    if (kBlendDraw) {
        return blendResult(float4(color.rgb * color.a, color.a), shape, dest, in.blend);
    }
    float alpha = color.a * shape;
    float3 rgb = color.rgb * alpha;
    if (in.paint != 0u && paintDithers(in.paint, paints)) {
        rgb += ditherNoise(point) * shape / 255.0;
    }
    return float4(rgb, alpha);
}
)METAL";

static const char* instanceFragmentSource = R"METAL(
static float instanceCoverage(
    float2 point,
    uint index,
    device const float4* pieces,
    device const uint* starts,
    device const uint* counts,
    device const int* backdrops,
    constant FillParams& params,
    device const FillDraw* draws,
    device const CoverageClip* coverageClips,
    device const uint* stats,
    device const FillPaint* paints,
    device const FillStop* stops,
    texture2d<float> atlas0,
    texture2d<float> atlas1,
    array<texture2d<float>, 4> maskPages
) {
    if (index == 0u) {
        return 1.0;
    }
    if (stats[1] != 0u) {
        return 0.0;
    }
    return coverageFactor(
        point,
        index,
        coverageClips,
        draws,
        pieces,
        starts,
        counts,
        backdrops,
        params,
        paints,
        stops,
        atlas0,
        atlas1,
        maskPages
    );
}

fragment float4 brixGuiMetalTier1Fragment(
    VertexOut in [[stage_in]],
    const device RoundedClip* clips [[buffer(0)]],
    device const float4* pieces [[buffer(1)]],
    device const uint* starts [[buffer(2)]],
    device const uint* counts [[buffer(3)]],
    device const int* backdrops [[buffer(4)]],
    constant FillParams& params [[buffer(5)]],
    device const FillDraw* draws [[buffer(6)]],
    device const CoverageClip* coverageClips [[buffer(7)]],
    device const uint* stats [[buffer(8)]],
    device const FillPaint* paints [[buffer(9)]],
    device const FillStop* stops [[buffer(10)]],
    texture2d<float> atlas0 [[texture(1)]],
    texture2d<float> atlas1 [[texture(2)]],
    array<texture2d<float>, 4> maskPages [[texture(3)]],
    float4 dest [[color(0), function_constant(kBlendDraw)]]
) {
    float radius = in.params.x;
    float borderWidth = in.params.y;
    uint shapeKind = in.kind & 0xffu;
    uint side = in.kind >> 8u;
    float coverage;
    if (shapeKind == 0u) {
        float distance = roundedRectDistance(in.local, in.halfSize, radius);
        coverage = saturate(0.5 - distance);
    } else if (shapeKind == 4u) {
        float distance = outlineDistance(in.local, in.halfSize, radius, side, in.params.z, in.params.w);
        coverage = saturate(0.5 - distance);
    } else if (shapeKind == 5u) {
        float distance = outlineDistance(in.local, in.halfSize, radius, side, in.params.z, in.params.w);
        coverage = saturate(0.5 - distance) * (1.0 - saturate(0.5 - (distance + borderWidth)));
    } else if (shapeKind == 6u) {
        coverage = shapeShadowCoverage(in.local, in.halfSize, in.params, in.contentMask, side);
    } else {
        float outer = roundedRectDistance(
            in.local,
            in.halfSize + borderWidth * 0.5,
            radius > 0.0 ? radius + borderWidth * 0.5 : 0.0
        );
        float inner = roundedRectDistance(
            in.local,
            in.halfSize - borderWidth * 0.5,
            max(radius - borderWidth * 0.5, 0.0)
        );
        coverage = saturate(0.5 - outer) * (1.0 - saturate(0.5 - inner));
    }
    float shape = coverage *
        roundedClipCoverage(in.pixelPosition, in.roundedClip, clips) *
        instanceCoverage(
            in.pixelPosition,
            in.coverageClip,
            pieces,
            starts,
            counts,
            backdrops,
            params,
            draws,
            coverageClips,
            stats,
            paints,
            stops,
            atlas0,
            atlas1,
            maskPages
        );
    float alpha = in.color.a * in.opacity;
    if (kBlendDraw) {
        return blendResult(float4(in.color.rgb * alpha, alpha), shape, dest, in.blend);
    }
    alpha *= shape;
    return float4(in.color.rgb * alpha, alpha);
}

fragment float4 brixGuiMetalMaskFragment(
    VertexOut in [[stage_in]],
    texture2d<float> texture [[texture(0)]],
    const device RoundedClip* clips [[buffer(0)]],
    device const float4* pieces [[buffer(1)]],
    device const uint* starts [[buffer(2)]],
    device const uint* counts [[buffer(3)]],
    device const int* backdrops [[buffer(4)]],
    constant FillParams& params [[buffer(5)]],
    device const FillDraw* draws [[buffer(6)]],
    device const CoverageClip* coverageClips [[buffer(7)]],
    device const uint* stats [[buffer(8)]],
    device const FillPaint* paints [[buffer(9)]],
    device const FillStop* stops [[buffer(10)]],
    texture2d<float> atlas0 [[texture(1)]],
    texture2d<float> atlas1 [[texture(2)]],
    array<texture2d<float>, 4> maskPages [[texture(3)]],
    float4 dest [[color(0), function_constant(kBlendDraw)]]
) {
    constexpr sampler textureSampler(
        coord::normalized,
        address::clamp_to_edge,
        filter::nearest
    );
    constexpr sampler smoothSampler(
        coord::normalized,
        address::clamp_to_edge,
        filter::linear
    );
    float2 uv = in.uvBase + (in.local / (in.halfSize * 2.0) + 0.5) * in.uvSize;
    float mask = (in.kind & kInstanceLinear) != 0u
        ? texture.sample(smoothSampler, uv).a
        : texture.sample(textureSampler, uv).a;
    float inside = step(in.contentMask.x, in.pixelPosition.x) *
        step(in.contentMask.y, in.pixelPosition.y) *
        step(in.pixelPosition.x, in.contentMask.z) *
        step(in.pixelPosition.y, in.contentMask.w);
    float shape = mask * inside *
        roundedClipCoverage(in.pixelPosition, in.roundedClip, clips) *
        instanceCoverage(
            in.pixelPosition,
            in.coverageClip,
            pieces,
            starts,
            counts,
            backdrops,
            params,
            draws,
            coverageClips,
            stats,
            paints,
            stops,
            atlas0,
            atlas1,
            maskPages
        );
    float4 color = in.paint != 0u
        ? paintColor(in.pixelPosition, in.paint, paints, stops, atlas0, atlas1)
        : in.color;
    float alpha = color.a * in.opacity;
    if (kBlendDraw) {
        return blendResult(float4(color.rgb * alpha, alpha), shape, dest, in.blend);
    }
    alpha *= shape;
    return float4(color.rgb * alpha, alpha);
}

static float4 imageAreaSample(texture2d<float> texture, float2 uv, float2 spread, bool wraps) {
    float2 base = float2(texture.get_width(), texture.get_height());
    float lod = floor(log2(max(max(spread.x * base.x, spread.y * base.y), 1.0)));
    uint level = uint(min(lod, float(texture.get_num_mip_levels() - 1u)));
    int2 size = int2(texture.get_width(level), texture.get_height(level));
    float2 reach = min(spread * float2(size) * 0.5, 2.0);
    float2 center = uv * float2(size);
    float2 low = center - reach;
    float2 high = center + reach;
    int2 first = int2(floor(low));
    int2 last = int2(ceil(high));
    float4 sum = 0.0;
    float total = 0.0;
    for (int y = first.y; y < last.y; ++y) {
        float wy = min(high.y, float(y + 1)) - max(low.y, float(y));
        int row = wraps ? ((y % size.y) + size.y) % size.y : clamp(y, 0, size.y - 1);
        for (int x = first.x; x < last.x; ++x) {
            float weight = wy * (min(high.x, float(x + 1)) - max(low.x, float(x)));
            int column = wraps ? ((x % size.x) + size.x) % size.x : clamp(x, 0, size.x - 1);
            sum += texture.read(uint2(column, row), level) * weight;
            total += weight;
        }
    }
    return total > 0.0 ? sum / total : float4(0.0);
}

static float4 imageTextureSample(texture2d<float> texture, float2 uv, uint kind, bool affine) {
    constexpr sampler trilinear(
        coord::normalized,
        address::clamp_to_edge,
        filter::linear,
        mip_filter::linear
    );
    constexpr sampler repeating(
        coord::normalized,
        address::repeat,
        filter::linear,
        mip_filter::linear
    );
    float2 dx = dfdx(uv);
    float2 dy = dfdy(uv);
    float2 size = float2(texture.get_width(), texture.get_height());
    float2 footprint = max((abs(dx) + abs(dy)) * size, float2(1e-6));
    if (kind == 8u || kind == 10u) {
        float2 texel = uv * size;
        float2 seam = floor(texel + 0.5);
        uv = (seam + clamp((texel - seam) / footprint, -0.5, 0.5)) / size;
        return kind == 10u
            ? texture.sample(repeating, uv, gradient2d(dx, dy))
            : texture.sample(trilinear, uv, gradient2d(dx, dy));
    }
    bool wraps = kind == 9u && affine;
    if (kind == 9u) {
        uv = fract(uv);
    }
    if (max(footprint.x, footprint.y) < 1.0) {
        float2 texel = wraps ? uv * size : clamp(uv * size, 0.5, size - 0.5);
        float2 base = floor(texel - 0.5);
        uint2 step = uint2(floor((texel - 0.5 - base) * 8.0 + 0.5));
        uv = (base + 0.5 + float2(kImageTaps[step.x], kImageTaps[step.y])) / size;
        return wraps
            ? texture.sample(repeating, uv, level(0.0))
            : texture.sample(trilinear, uv, level(0.0));
    }
    return imageAreaSample(texture, uv, abs(dx) + abs(dy), wraps);
}

fragment float4 brixGuiMetalRGBAFragment(
    VertexOut in [[stage_in]],
    texture2d<float> texture [[texture(0)]],
    const device RoundedClip* clips [[buffer(0)]],
    device const float4* pieces [[buffer(1)]],
    device const uint* starts [[buffer(2)]],
    device const uint* counts [[buffer(3)]],
    device const int* backdrops [[buffer(4)]],
    constant FillParams& params [[buffer(5)]],
    device const FillDraw* draws [[buffer(6)]],
    device const CoverageClip* coverageClips [[buffer(7)]],
    device const uint* stats [[buffer(8)]],
    device const FillPaint* paints [[buffer(9)]],
    device const FillStop* stops [[buffer(10)]],
    texture2d<float> atlas0 [[texture(1)]],
    texture2d<float> atlas1 [[texture(2)]],
    array<texture2d<float>, 4> maskPages [[texture(3)]],
    float4 dest [[color(0), function_constant(kBlendDraw)]]
) {
    constexpr sampler textureSampler(
        coord::normalized,
        address::clamp_to_edge,
        filter::nearest
    );
    float2 uv = in.uvBase + (in.local / (in.halfSize * 2.0) + 0.5) * in.uvSize;
    uint imageKind = in.kind & 0xffu;
    float4 sampled = (imageKind >= 7u && imageKind <= 10u
        ? imageTextureSample(texture, uv, imageKind, (in.kind & kInstanceAffine) != 0u)
        : texture.sample(textureSampler, uv)) * in.opacity;
    uint imageSpace = in.kind >> kInstanceSpaceShift;
    if (imageSpace != kBrixOutputSpace && sampled.a > 0.0) {
        float3 converted = brixConvertColor(sampled.rgb / sampled.a, imageSpace, kBrixOutputSpace);
        sampled.rgb = (kBrixExtendedOutput ? converted : saturate(converted)) * sampled.a;
    }
    if ((in.kind & kInstanceTemplate) != 0u) {
        sampled = float4(in.color.rgb * in.color.a, in.color.a) * sampled.a;
    }
    float edge = 1.0;
    if (imageKind >= 7u && imageKind <= 10u) {
        uint edges = (in.kind >> 8u) & 0xfu;
        float2 low = in.local + in.halfSize;
        float2 high = in.halfSize - in.local;
        float left = (edges & 1u) != 0u ? saturate(low.x + 0.5) : step(0.0, low.x);
        float top = (edges & 2u) != 0u ? saturate(low.y + 0.5) : step(0.0, low.y);
        float right = (edges & 4u) != 0u ? saturate(high.x + 0.5) : step(0.0, high.x);
        float bottom = (edges & 8u) != 0u ? saturate(high.y + 0.5) : step(0.0, high.y);
        edge = left * top * right * bottom;
    }
    float inside = edge * step(in.contentMask.x, in.pixelPosition.x) *
        step(in.contentMask.y, in.pixelPosition.y) *
        step(in.pixelPosition.x, in.contentMask.z) *
        step(in.pixelPosition.y, in.contentMask.w);
    float shape = inside * roundedClipCoverage(in.pixelPosition, in.roundedClip, clips) *
        instanceCoverage(
            in.pixelPosition,
            in.coverageClip,
            pieces,
            starts,
            counts,
            backdrops,
            params,
            draws,
            coverageClips,
            stats,
            paints,
            stops,
            atlas0,
            atlas1,
            maskPages
        );
    if (kBlendDraw) {
        return blendResult(sampled, shape, dest, in.blend);
    }
    return sampled * shape;
}

fragment float4 brixGuiMetalBackdropFragment(
    VertexOut in [[stage_in]],
    texture2d<float> texture [[texture(0)]],
    const device RoundedClip* clips [[buffer(0)]],
    device const float4* pieces [[buffer(1)]],
    device const uint* starts [[buffer(2)]],
    device const uint* counts [[buffer(3)]],
    device const int* backdrops [[buffer(4)]],
    constant FillParams& params [[buffer(5)]],
    device const FillDraw* draws [[buffer(6)]],
    device const CoverageClip* coverageClips [[buffer(7)]],
    device const uint* stats [[buffer(8)]],
    device const FillPaint* paints [[buffer(9)]],
    device const FillStop* stops [[buffer(10)]],
    texture2d<float> atlas0 [[texture(1)]],
    texture2d<float> atlas1 [[texture(2)]],
    array<texture2d<float>, 4> maskPages [[texture(3)]],
    float4 dest [[color(0), function_constant(kBlendDraw)]]
) {
    constexpr sampler linear(
        coord::normalized,
        address::clamp_to_edge,
        filter::linear
    );
    float2 uv = in.uvBase + (in.local / (in.halfSize * 2.0) + 0.5) * in.uvSize;
    float4 sampled = texture.sample(linear, uv) * in.opacity;
    float inside = step(in.contentMask.x, in.pixelPosition.x) *
        step(in.contentMask.y, in.pixelPosition.y) *
        step(in.pixelPosition.x, in.contentMask.z) *
        step(in.pixelPosition.y, in.contentMask.w);
    float shape = inside * roundedClipCoverage(in.pixelPosition, in.roundedClip, clips) *
        instanceCoverage(
            in.pixelPosition,
            in.coverageClip,
            pieces,
            starts,
            counts,
            backdrops,
            params,
            draws,
            coverageClips,
            stats,
            paints,
            stops,
            atlas0,
            atlas1,
            maskPages
        );
    if (kBlendDraw) {
        return blendResult(sampled, shape, dest, in.blend);
    }
    return sampled * shape;
}
)METAL";


static const char* dabShaderSource = R"METAL(
struct DabDraw {
    float color[4];
    float mask[4];
    float a, b, c, d, tx, ty;
    DabStyle style;
    float paper;
    uint argStart;
    uint argCount;
    uint roundedClip;
    uint coverage;
    uint blend;
    float paperX;
    float paperY;
};

struct DabVertexOut {
    float4 position [[position]];
    float2 offset;
    float2 local;
    float pressure [[flat]];
    float random [[flat]];
    float distance [[flat]];
    float along [[flat]];
    float pixel [[flat]];
    uint draw [[flat]];
};

vertex DabVertexOut brixGuiMetalDabVertex(
    uint vertexId [[vertex_id]],
    uint instanceId [[instance_id]],
    constant float4& viewport [[buffer(0)]],
    device const DabRecord* dabs [[buffer(1)]],
    device const DabDraw* draws [[buffer(2)]]
) {
    DabRecord dab = dabs[instanceId];
    DabDraw draw = draws[dab.draw];
    DabPlacement placement = dabPlace(dab, draw.style, vertexId);
    float2 pixel = float2(
        draw.a * placement.local.x + draw.c * placement.local.y + draw.tx,
        draw.b * placement.local.x + draw.d * placement.local.y + draw.ty
    );
    float scale = sqrt(abs(draw.a * draw.d - draw.b * draw.c));
    DabVertexOut out;
    out.position = float4(
        (pixel.x - viewport.x) / viewport.z * 2.0 - 1.0,
        1.0 - (pixel.y - viewport.y) / viewport.w * 2.0,
        0.0,
        1.0
    );
    out.offset = placement.offset;
    out.local = placement.local;
    out.pressure = placement.pressure;
    out.random = placement.random;
    out.distance = dab.along;
    out.along = placement.along;
    out.pixel = 1.0 / max(placement.radius * scale, 1e-3);
    out.draw = dab.draw;
    return out;
}

fragment float4 brixGuiMetalDabFragment(
    DabVertexOut in [[stage_in]],
    const device RoundedClip* clips [[buffer(0)]],
    device const float4* pieces [[buffer(1)]],
    device const uint* starts [[buffer(2)]],
    device const uint* counts [[buffer(3)]],
    device const int* backdrops [[buffer(4)]],
    constant FillParams& params [[buffer(5)]],
    device const FillDraw* draws [[buffer(6)]],
    device const CoverageClip* coverageClips [[buffer(7)]],
    device const FillPaint* paints [[buffer(9)]],
    device const FillStop* stops [[buffer(10)]],
    device const DabDraw* dabDraws [[buffer(11)]],
    texture2d<float> atlas0 [[texture(1)]],
    texture2d<float> atlas1 [[texture(2)]],
    array<texture2d<float>, 4> maskPages [[texture(3)]],
    float4 dest [[color(0), function_constant(kBlendDraw)]]
) {
    DabDraw draw = dabDraws[in.draw];
    float2 point = in.position.xy + float2(params.originX, params.originY);
    float shape = step(draw.mask[0], point.x) * step(draw.mask[1], point.y) *
        step(point.x, draw.mask[2]) * step(point.y, draw.mask[3]) *
        roundedClipCoverage(point, draw.roundedClip, clips);
    if (draw.coverage != 0u && shape > 0.0) {
        shape *= coverageFactor(
            point,
            draw.coverage,
            coverageClips,
            draws,
            pieces,
            starts,
            counts,
            backdrops,
            params,
            paints,
            stops,
            atlas0,
            atlas1,
            maskPages
        );
    }
    if (shape <= 0.0) {
        discard_fragment();
    }
    DabInput input;
    input.offset = in.offset;
    input.position = in.local;
    input.paper = (point - float2(draw.paperX, draw.paperY)) * draw.paper;
    input.color = float4(
        brixLinearColor(float3(draw.color[0], draw.color[1], draw.color[2]), 0u, kBrixOutputSpace),
        draw.color[3]
    );
    input.along = in.along;
    input.distance = in.distance;
    input.pressure = in.pressure;
    input.random = in.random;
    input.pixel = in.pixel;
    ShaderArgs args = {stops, draw.argStart, draw.argCount};
    float4 color = ::dab(input, args);
    float3 encoded = brixEncodeColor(color.rgb, kBrixOutputSpace);
    color = float4(kBrixExtendedOutput ? encoded : saturate(encoded), saturate(color.a * draw.style.flow));
    if (kBlendDraw) {
        return blendResult(float4(color.rgb * color.a, color.a), shape, dest, draw.blend);
    }
    float alpha = color.a * shape;
    return float4(color.rgb * alpha, alpha);
}
)METAL";

static id<MTLRenderPipelineState> buildRenderPipeline(
    id<MTLDevice> device,
    id<MTLLibrary> library,
    id<MTLFunction> vertex,
    NSString* name,
    MTLFunctionConstantValues* values,
    bool blending,
    MTLColorWriteMask writeMask = MTLColorWriteMaskAll,
    MTLPixelFormat colorFormat = MTLPixelFormatBGRA8Unorm,
    MTLPixelFormat depthFormat = MTLPixelFormatDepth32Float
) {
    NSError* error = nil;
    id<MTLFunction> fragment = values
        ? [library newFunctionWithName:name constantValues:values error:&error]
        : [library newFunctionWithName:name];
    if (!fragment) {
        fprintf(
            stderr,
            "brix metal: fragment function failed: %s\n",
            error.localizedDescription.UTF8String
        );
        return nil;
    }
    MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
    descriptor.vertexFunction = vertex;
    descriptor.fragmentFunction = fragment;
    descriptor.depthAttachmentPixelFormat = depthFormat;
    MTLRenderPipelineColorAttachmentDescriptor* attachment = descriptor.colorAttachments[0];
    attachment.pixelFormat = colorFormat;
    attachment.blendingEnabled = blending;
    attachment.writeMask = writeMask;
    attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
    attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
    attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    id<MTLRenderPipelineState> pipeline =
        [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
    if (!pipeline) {
        fprintf(
            stderr,
            "brix metal: pipeline creation failed: %s\n",
            error.localizedDescription.UTF8String
        );
    }
    return pipeline;
}

static id<MTLRenderPipelineState> buildBlendDrawPipeline(
    id<MTLDevice> device,
    id<MTLLibrary> library,
    id<MTLFunction> vertex,
    NSString* name,
    MTLFunctionConstantValues* plain,
    MTLFunctionConstantValues* blended,
    MTLPixelFormat colorFormat
) {
#if TARGET_OS_SIMULATOR
    return buildRenderPipeline(device, library, vertex, name, plain, true, MTLColorWriteMaskAll, colorFormat);
#else
    return buildRenderPipeline(device, library, vertex, name, blended, false, MTLColorWriteMaskAll, colorFormat);
#endif
}

static MTLFunctionConstantValues* newSurfaceConstants(MTLPixelFormat format, bool blend) {
    MTLFunctionConstantValues* values = [MTLFunctionConstantValues new];
    bool extended = format != MTLPixelFormatBGRA8Unorm;
    [values setConstantValue:&blend type:MTLDataTypeBool atIndex:0];
    [values setConstantValue:&extended type:MTLDataTypeBool atIndex:1];
    return values;
}

static void releaseSurfacePipelines(SurfacePipelines& set) {
    for (int index = 0; index < 6; ++index) {
        set.pipelines[index] = nil;
        set.blendPipelines[index] = nil;
    }
    for (auto& pipeline : set.shadowBlurPipelines) {
        pipeline = nil;
    }
    set.fillRender = nil;
    set.fillSolid = nil;
    set.fillBlend = nil;
    set.layerBlurPipeline = nil;
}

struct RenderPipelineJob {
    __strong id<MTLRenderPipelineState>* target;
    id<MTLFunction> vertex;
    NSString* fragment;
    MTLFunctionConstantValues* values;
    bool blending;
    MTLColorWriteMask writeMask;
    MTLPixelFormat colorFormat;
    MTLPixelFormat depthFormat;
};

struct KernelJob {
    __strong id<MTLComputePipelineState>* target;
    NSString* name;
};

static RenderPipelineJob renderJob(
    __strong id<MTLRenderPipelineState>* target,
    id<MTLFunction> vertex,
    NSString* fragment,
    MTLFunctionConstantValues* values,
    bool blending,
    MTLColorWriteMask writeMask,
    MTLPixelFormat colorFormat,
    MTLPixelFormat depthFormat = MTLPixelFormatDepth32Float
) {
    return {target, vertex, fragment, values, blending, writeMask, colorFormat, depthFormat};
}

static RenderPipelineJob blendDrawJob(
    __strong id<MTLRenderPipelineState>* target,
    id<MTLFunction> vertex,
    NSString* fragment,
    MTLFunctionConstantValues* plain,
    MTLFunctionConstantValues* blended,
    MTLPixelFormat colorFormat
) {
#if TARGET_OS_SIMULATOR
    return renderJob(target, vertex, fragment, plain, true, MTLColorWriteMaskAll, colorFormat);
#else
    return renderJob(target, vertex, fragment, blended, false, MTLColorWriteMaskAll, colorFormat);
#endif
}

static void runConcurrently(size_t count, void (^work)(size_t index)) {
    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);
    for (size_t index = 0; index < count; ++index) {
        dispatch_group_async(group, queue, ^{
            @autoreleasepool {
                work(index);
            }
        });
    }
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
}

static bool buildRenderPipelines(id<MTLDevice> device, id<MTLLibrary> library, std::vector<RenderPipelineJob>& jobs) {
    RenderPipelineJob* list = jobs.data();
    runConcurrently(jobs.size(), ^(size_t index) {
        const RenderPipelineJob& job = list[index];
        *job.target = buildRenderPipeline(
            device, library, job.vertex, job.fragment, job.values, job.blending, job.writeMask, job.colorFormat,
            job.depthFormat
        );
    });
    for (const RenderPipelineJob& job : jobs) {
        if (!*job.target) {
            return false;
        }
    }
    return true;
}

static bool buildKernels(id<MTLDevice> device, id<MTLLibrary> library, std::vector<KernelJob>& jobs) {
    KernelJob* list = jobs.data();
    runConcurrently(jobs.size(), ^(size_t index) {
        const KernelJob& job = list[index];
        NSError* error = nil;
        id<MTLFunction> function = [library newFunctionWithName:job.name];
        *job.target = function ? [device newComputePipelineStateWithFunction:function error:&error] : nil;
        if (!*job.target) {
            fprintf(
                stderr,
                "brix metal: kernel %s creation failed: %s\n",
                job.name.UTF8String,
                error ? error.localizedDescription.UTF8String : "missing function"
            );
        }
    });
    for (const KernelJob& job : jobs) {
        if (!*job.target) {
            return false;
        }
    }
    return true;
}

static bool buildSurfacePipelines(SharedPipelines& shared, SurfacePipelines& set) {
    id<MTLLibrary> library = shared.library;
    MTLFunctionConstantValues* plain = newSurfaceConstants(set.format, false);
    MTLFunctionConstantValues* blended = newSurfaceConstants(set.format, true);
    id<MTLFunction> vertex = [library newFunctionWithName:@"brixGuiMetalVertex"];
    id<MTLFunction> fillVertex = [library newFunctionWithName:@"brixGuiMetalFillVertex"];
    id<MTLFunction> blurVertex = [library newFunctionWithName:@"brixGuiMetalShadowBlurVertex"];
    id<MTLFunction> layerBlurVertex = [library newFunctionWithName:@"brixGuiMetalLayerBlurVertex"];
    id<MTLFunction> composeVertex = [library newFunctionWithName:@"brixGuiMetalFilterComposeVertex"];
    NSString* fragments[6] = {
        @"brixGuiMetalTier1Fragment",
        @"brixGuiMetalMaskFragment",
        @"brixGuiMetalRGBAFragment",
        @"brixGuiMetalTier1Fragment",
        nil,
        @"brixGuiMetalBackdropFragment",
    };
    std::vector<RenderPipelineJob> jobs;
    for (int index = 0; index < 6; ++index) {
        if (!fragments[index]) {
            continue;
        }
        jobs.push_back(renderJob(
            &set.pipelines[index], vertex, fragments[index], plain, index != PipelineClear, MTLColorWriteMaskAll,
            set.format
        ));
        if (index != PipelineClear) {
            jobs.push_back(blendDrawJob(&set.blendPipelines[index], vertex, fragments[index], plain, blended, set.format));
        }
    }
    jobs.push_back(renderJob(
        &set.fillRender, fillVertex, @"brixGuiMetalFillFragment", plain, true, MTLColorWriteMaskAll, set.format
    ));
    jobs.push_back(renderJob(
        &set.fillSolid, fillVertex, @"brixGuiMetalFillSolidFragment", plain, false, MTLColorWriteMaskAll, set.format
    ));
    jobs.push_back(blendDrawJob(&set.fillBlend, fillVertex, @"brixGuiMetalFillFragment", plain, blended, set.format));
    MTLColorWriteMask channels[5] = {
        MTLColorWriteMaskRed,
        MTLColorWriteMaskGreen,
        MTLColorWriteMaskBlue,
        MTLColorWriteMaskAlpha,
        MTLColorWriteMaskAll,
    };
    for (int index = 0; index < 5; ++index) {
        jobs.push_back(renderJob(
            &set.shadowBlurPipelines[index], blurVertex, @"brixGuiMetalShadowBlurFragment", nil, false, channels[index],
            set.format
        ));
    }
    jobs.push_back(renderJob(
        &set.layerBlurPipeline, layerBlurVertex, @"brixGuiMetalLayerBlurFragment", nil, false, MTLColorWriteMaskAll,
        set.format
    ));
    jobs.push_back(renderJob(
        &set.filterComposePipeline, composeVertex, @"brixGuiMetalFilterComposeFragment", nil, false,
        MTLColorWriteMaskAll, set.format
    ));
    return buildRenderPipelines(shared.device, library, jobs);
}

static const SurfacePipelines* surfacePipelinesFor(SharedPipelines* shared, MTLPixelFormat format) {
    std::lock_guard<std::mutex> lock(shared->surfaceMutex);
    for (const std::unique_ptr<SurfacePipelines>& set : shared->surfaces) {
        if (set->format == format) {
            return set.get();
        }
    }
    std::unique_ptr<SurfacePipelines> set(new SurfacePipelines());
    set->format = format;
    if (!buildSurfacePipelines(*shared, *set)) {
        releaseSurfacePipelines(*set);
        return nullptr;
    }
    shared->surfaces.push_back(std::move(set));
    return shared->surfaces.back().get();
}

static const SurfacePipelines* surfacePipelinesFor(MetalRenderer* renderer, MTLPixelFormat format) {
    return surfacePipelinesFor(renderer->shared, format);
}

static id<MTLLibrary> newSharedLibrary(id<MTLDevice> device, const std::string& source) {
    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                                                  options:nil
                                                    error:&error];
    if (!library) {
        fprintf(
            stderr,
            "brix metal: shader compilation failed: %s\n",
            error.localizedDescription.UTF8String
        );
    }
    return library;
}

static bool buildKernelLibrary(SharedPipelines& shared, const std::string& source, std::vector<KernelJob> jobs) {
    id<MTLLibrary> library = newSharedLibrary(shared.device, source);
    return library && buildKernels(shared.device, library, jobs);
}

static bool buildRenderLibrary(SharedPipelines& shared, const std::string& source) {
    shared.library = newSharedLibrary(shared.device, source);
    if (!shared.library || !surfacePipelinesFor(&shared, MTLPixelFormatBGRA8Unorm)) {
        return false;
    }
    std::vector<KernelJob> kernels = {
        {&shared.fillKernels[FillKernelOcclude], @"brixGuiMetalFillOcclude"},
        {&shared.fillKernels[FillKernelMark], @"brixGuiMetalFillMark"},
        {&shared.fillKernels[FillKernelScanGroups], @"brixGuiMetalFillScanGroups"},
        {&shared.fillKernels[FillKernelScatter], @"brixGuiMetalFillScatter"},
        {&shared.fillKernels[FillKernelBatchBounds], @"brixGuiMetalFillBatchBounds"},
        {&shared.fillKernels[FillKernelBatchArgs], @"brixGuiMetalFillBatchArgs"},
    };
    id<MTLFunction> backdropVertex = [shared.library newFunctionWithName:@"brixGuiMetalBackdropVertex"];
    std::vector<RenderPipelineJob> backdrops = {
        renderJob(
            &shared.backdropGatherPipeline, backdropVertex, @"brixGuiMetalBackdropGatherFragment", nil, false,
            MTLColorWriteMaskAll, MTLPixelFormatRGBA16Float, MTLPixelFormatInvalid
        ),
        renderJob(
            &shared.backdropDownPipeline, backdropVertex, @"brixGuiMetalBackdropDownFragment", nil, false,
            MTLColorWriteMaskAll, MTLPixelFormatRGBA16Float, MTLPixelFormatInvalid
        ),
        renderJob(
            &shared.backdropBlurPipeline, backdropVertex, @"brixGuiMetalBackdropBlurFragment", nil, false,
            MTLColorWriteMaskAll, MTLPixelFormatRGBA16Float, MTLPixelFormatInvalid
        ),
    };
    return buildKernels(shared.device, shared.library, kernels) &&
        buildRenderPipelines(shared.device, shared.library, backdrops);
}

static bool buildSharedPipelines(SharedPipelines& shared) {
    SharedPipelines* target = &shared;
    std::string fill = std::string(commonShaderSource) + rendererColorSource() + fillTypesSource + fillBinSource;
    std::string sources[4] = {
        fill + fillWriteSource,
        fill + fillCountSource,
        fill + fillKernelSource,
        std::string(commonShaderSource) + effectShaderSource + rendererColorSource() + fillTypesSource +
            fillCoverageSource + fillTileSource + fillDrawSource + instanceFragmentSource,
    };
    const std::string* source = sources;
    bool results[4] = {false, false, false, false};
    bool* built = results;
    runConcurrently(4, ^(size_t part) {
        switch (part) {
            case 0:
                built[part] = buildKernelLibrary(*target, source[part], {
                    {&target->fillKernels[FillKernelWrite], @"brixGuiMetalFillWrite"},
                });
                break;
            case 1:
                built[part] = buildKernelLibrary(*target, source[part], {
                    {&target->fillKernels[FillKernelCount], @"brixGuiMetalFillCount"},
                });
                break;
            case 2:
                built[part] = buildKernelLibrary(*target, source[part], {
                    {&target->fillKernels[FillKernelInit], @"brixGuiMetalFillInit"},
                    {&target->fillKernels[FillKernelAllocate], @"brixGuiMetalFillAllocate"},
                    {&target->fillKernels[FillKernelBackdrop], @"brixGuiMetalFillBackdrop"},
                    {&target->imageMipKernel, @"brixGuiMetalImageMip"},
                    {&target->imageTileKernel, @"brixGuiMetalImageTile"},
                    {&target->readbackKernel, @"brixGuiMetalReadback"},
                });
                break;
            default:
                built[part] = buildRenderLibrary(*target, source[part]);
                break;
        }
    });
    bool didBuild = built[0] && built[1] && built[2] && built[3];
    MTLCompareFunction compares[3] = {
        MTLCompareFunctionAlways,
        MTLCompareFunctionLess,
        MTLCompareFunctionLess,
    };
    __strong id<MTLDepthStencilState>* depthTargets[3] = {
        &shared.depthNone,
        &shared.depthTest,
        &shared.depthWrite,
    };
    for (int index = 0; index < 3 && didBuild; ++index) {
        MTLDepthStencilDescriptor* descriptor = [MTLDepthStencilDescriptor new];
        descriptor.depthCompareFunction = compares[index];
        descriptor.depthWriteEnabled = index == 2;
        *depthTargets[index] = [shared.device newDepthStencilStateWithDescriptor:descriptor];
        didBuild = *depthTargets[index] != nil;
    }
    return didBuild;
}

static SharedPipelines* sharedPipelines() {
    static std::once_flag once;
    static SharedPipelines* shared = nullptr;
    std::call_once(once, [] {
        id<MTLDevice> device = (__bridge id<MTLDevice>)brixGraphicsMetalDevice();
        if (!device) {
            return;
        }
        SharedPipelines* built = new SharedPipelines();
        built->device = device;
        if (buildSharedPipelines(*built)) {
            shared = built;
        } else {
            delete built;
        }
    });
    return shared;
}

struct ClipState {
    double left, top, right, bottom;
};

static const uint32_t kExpandDepth = 64;

struct ExpandState {
    MetalRenderer* renderer;
    double matrix[6] = {1, 0, 0, 1, 0, 0};
    ClipState clip;
    ClipState clipStack[kExpandDepth];
    uint32_t roundedClipStack[kExpandDepth];
    uint32_t coverageClipStack[kExpandDepth];
    uint32_t clipDepth = 0;
    uint32_t clipOverflow = 0;
    uint32_t roundedClip = 0;
    uint32_t coverageClip = 0;
    double matrixStack[kExpandDepth][6];
    double paper[2] = {0, 0};
    double paperStack[kExpandDepth][2];
    uint32_t matrixDepth = 0;
    uint32_t matrixOverflow = 0;
    uint32_t layerStack[kExpandDepth];
    std::vector<Batch>* layerSinks[kExpandDepth];
    uint32_t layerDepth = 0;
    uint32_t layerOverflow = 0;
    uint32_t skippedMasks = 0;
    uint32_t blend = 0;
    uint32_t blendStack[kExpandDepth];
    uint32_t blendDepth = 0;
    uint32_t blendOverflow = 0;
    bool inlineLayer = false;
    bool tileFrame = false;
    std::vector<Batch>* batchSink;
};

static const uint32_t kBlendSkip = UINT32_MAX;
static const uint32_t kBlendCopy = 17;

static bool blendUnbounded(uint32_t mode) {
    return mode == 16 || mode == 17 || mode == 18 || mode == 19 || mode == 22 || mode == 24;
}

static uint32_t drawBlend(const ExpandState& state) {
    uint32_t mode = state.blend;
    if (!mode || !state.layerDepth || !state.renderer->layers[state.layerStack[state.layerDepth - 1]]->mask) {
        return mode;
    }
    switch (mode) {
    case 18:
        return 17;
    case 19:
        return 16;
    case 20:
        return 0;
    case 21:
        return kBlendSkip;
    case 24:
        return 22;
    case 25:
        return 23;
    default:
        return mode;
    }
}

static void transformBounds(const double* matrix, const double* rect, double* out) {
    double corners[4][2] = {
        {rect[0], rect[1]}, {rect[2], rect[1]}, {rect[0], rect[3]}, {rect[2], rect[3]},
    };
    out[0] = INFINITY;
    out[1] = INFINITY;
    out[2] = -INFINITY;
    out[3] = -INFINITY;
    for (auto& corner : corners) {
        double x = matrix[0] * corner[0] + matrix[2] * corner[1] + matrix[4];
        double y = matrix[1] * corner[0] + matrix[3] * corner[1] + matrix[5];
        out[0] = std::min(out[0], x);
        out[1] = std::min(out[1], y);
        out[2] = std::max(out[2], x);
        out[3] = std::max(out[3], y);
    }
}

static void includeLayerBounds(ExpandState& state, double left, double top, double right, double bottom) {
    if (!state.layerDepth || !(left < right) || !(top < bottom)) {
        return;
    }
    LayerPass& layer = *state.renderer->layers[state.layerStack[state.layerDepth - 1]];
    if (layer.empty) {
        layer.bounds[0] = left;
        layer.bounds[1] = top;
        layer.bounds[2] = right;
        layer.bounds[3] = bottom;
        layer.empty = false;
        return;
    }
    layer.bounds[0] = std::min(layer.bounds[0], left);
    layer.bounds[1] = std::min(layer.bounds[1], top);
    layer.bounds[2] = std::max(layer.bounds[2], right);
    layer.bounds[3] = std::max(layer.bounds[3], bottom);
}

static double instancePad(const Instance& instance) {
    uint32_t kind = instance.kind & 0xffu;
    if (kind == 0u) {
        return 1.0;
    }
    if (kind == 1u) {
        return instance.params[1] * 0.5 + 1.0;
    }
    if (kind == 4u || kind == 5u) {
        return instance.params[3] + 1.0;
    }
    if (kind == 6u) {
        double reach = std::max(std::fabs(instance.contentMask[0]), std::fabs(instance.contentMask[1]));
        return instance.params[1] * 3.0 + std::max(instance.contentMask[2], 0.0f) + reach +
            instance.params[3] + 1.0;
    }
    if (kind >= 7u && kind <= 10u && (instance.kind & kInstanceEdges) != 0u) {
        return 1.0;
    }
    return 0;
}

static void instanceBounds(const Instance& instance, double* bounds) {
    double pad = instancePad(instance);
    if (!(instance.kind & kInstanceAffine)) {
        bounds[0] = instance.rect[0] - pad;
        bounds[1] = instance.rect[1] - pad;
        bounds[2] = instance.rect[0] + instance.rect[2] + pad;
        bounds[3] = instance.rect[1] + instance.rect[3] + pad;
        return;
    }
    double x = instance.rect[0];
    double y = instance.rect[1];
    double ux = instance.rect[2];
    double uy = instance.rect[3];
    double vx = instance.params[0];
    double vy = instance.params[1];
    bounds[0] = x + std::min(0.0, ux) + std::min(0.0, vx) - pad * 2;
    bounds[1] = y + std::min(0.0, uy) + std::min(0.0, vy) - pad * 2;
    bounds[2] = x + std::max(0.0, ux) + std::max(0.0, vx) + pad * 2;
    bounds[3] = y + std::max(0.0, uy) + std::max(0.0, vy) + pad * 2;
}

static MTLScissorRect clipToScissor(MetalRenderer* renderer, const ClipState& clip) {
    double left = std::max(0.0, floor(clip.left * renderer->scale));
    double top = std::max(0.0, floor(clip.top * renderer->scale));
    double right = std::min(
        static_cast<double>(renderer->pixelWidth),
        ceil(clip.right * renderer->scale)
    );
    double bottom = std::min(
        static_cast<double>(renderer->pixelHeight),
        ceil(clip.bottom * renderer->scale)
    );
    if (right <= left || bottom <= top) {
        return MTLScissorRect{0, 0, 0, 0};
    }
    return MTLScissorRect{
        static_cast<NSUInteger>(left),
        static_cast<NSUInteger>(top),
        static_cast<NSUInteger>(right - left),
        static_cast<NSUInteger>(bottom - top),
    };
}

static bool scissorsEqual(const MTLScissorRect& left, const MTLScissorRect& right) {
    return left.x == right.x && left.y == right.y && left.width == right.width &&
        left.height == right.height;
}

static void setFullContentMask(MetalRenderer* renderer, Instance& instance) {
    instance.contentMask[0] = 0;
    instance.contentMask[1] = 0;
    instance.contentMask[2] = static_cast<float>(renderer->pixelWidth);
    instance.contentMask[3] = static_cast<float>(renderer->pixelHeight);
}

static void includeBatchRect(BatchRects& bounds, const float* rect) {
    if (!(rect[0] < rect[2] && rect[1] < rect[3])) {
        return;
    }
    for (uint32_t index = 0; index < bounds.count; ++index) {
        float* existing = bounds.rects[index];
        if (rect[0] <= existing[2] + 2 && existing[0] <= rect[2] + 2 &&
                rect[1] <= existing[3] + 2 && existing[1] <= rect[3] + 2) {
            existing[0] = std::min(existing[0], rect[0]);
            existing[1] = std::min(existing[1], rect[1]);
            existing[2] = std::max(existing[2], rect[2]);
            existing[3] = std::max(existing[3], rect[3]);
            return;
        }
    }
    if (bounds.count < kBatchRects) {
        memcpy(bounds.rects[bounds.count++], rect, sizeof(float) * 4);
        return;
    }
    uint32_t best = 0;
    float bestGrowth = INFINITY;
    for (uint32_t index = 0; index < bounds.count; ++index) {
        const float* existing = bounds.rects[index];
        float width = std::max(existing[2], rect[2]) - std::min(existing[0], rect[0]);
        float height = std::max(existing[3], rect[3]) - std::min(existing[1], rect[1]);
        float growth = width * height - (existing[2] - existing[0]) * (existing[3] - existing[1]);
        if (growth < bestGrowth) {
            bestGrowth = growth;
            best = index;
        }
    }
    float* chosen = bounds.rects[best];
    chosen[0] = std::min(chosen[0], rect[0]);
    chosen[1] = std::min(chosen[1], rect[1]);
    chosen[2] = std::max(chosen[2], rect[2]);
    chosen[3] = std::max(chosen[3], rect[3]);
}

static void noteBatchRect(ExpandState& state, double left, double top, double right, double bottom) {
    MetalRenderer* renderer = state.renderer;
    if (state.batchSink != &renderer->batches || renderer->batches.empty()) {
        return;
    }
    size_t index = renderer->batches.size() - 1;
    std::vector<BatchRects>& rects = renderer->batchRects;
    if (rects.size() <= index) {
        ensureVectorCapacity(renderer, rects, index + 1);
        rects.resize(index + 1, BatchRects{{}, 0});
    }
    float rect[4] = {
        static_cast<float>(floor(left)),
        static_cast<float>(floor(top)),
        static_cast<float>(ceil(right)),
        static_cast<float>(ceil(bottom)),
    };
    includeBatchRect(rects[index], rect);
}

static void emitInstance(
    ExpandState& state,
    uint32_t pipeline,
    __unsafe_unretained id<MTLTexture> texture,
    const Instance& instance,
    uint32_t shader = 0
) {
    MTLScissorRect scissor = clipToScissor(state.renderer, state.clip);
    uint32_t blend = pipeline == PipelineClear ? 0 : pipeline == PipelineBackdrop ? kBlendCopy : drawBlend(state);
    if (!scissor.width || !scissor.height || blend == kBlendSkip) {
        return;
    }
    std::vector<Instance>& instances = state.renderer->instances;
    std::vector<Batch>& batches = *state.batchSink;
    if (state.layerDepth) {
        double bounds[4];
        instanceBounds(instance, bounds);
        includeLayerBounds(
            state,
            std::max<double>(bounds[0], scissor.x),
            std::max<double>(bounds[1], scissor.y),
            std::min<double>(bounds[2], scissor.x + scissor.width),
            std::min<double>(bounds[3], scissor.y + scissor.height)
        );
    }
    if (batches.empty() || batches.back().pipeline != pipeline ||
        batches.back().texture != texture || !scissorsEqual(batches.back().scissor, scissor) ||
        batches.back().blend != (blend != 0) || batches.back().shader != shader ||
        batches.back().start + batches.back().count != instances.size()) {
        ensureVectorCapacity(state.renderer, batches, batches.size() + 1);
        Batch batch = {pipeline, texture, scissor, static_cast<uint32_t>(instances.size()), 0, blend != 0, shader};
        batches.push_back(batch);
    }
    ensureVectorCapacity(state.renderer, instances, instances.size() + 1);
    Instance clipped = instance;
    clipped.roundedClip = state.roundedClip;
    clipped.coverageClip = state.coverageClip;
    clipped.blend = blend;
    instances.push_back(clipped);
    ++batches.back().count;
    if (state.batchSink == &state.renderer->batches) {
        double bounds[4];
        instanceBounds(clipped, bounds);
        uint32_t kind = clipped.kind & 0xffu;
        double margin = !(clipped.kind & kInstanceAffine) && (kind == 0u || kind == 4u || kind == 5u) ? 1.0 : 0.0;
        noteBatchRect(
            state,
            std::max<double>(bounds[0] + margin, scissor.x),
            std::max<double>(bounds[1] + margin, scissor.y),
            std::min<double>(bounds[2] - margin, scissor.x + scissor.width),
            std::min<double>(bounds[3] - margin, scissor.y + scissor.height)
        );
    }
}

static void emitQuads(ExpandState& state, const uint8_t* records, uint32_t count) {
    MetalRenderer* renderer = state.renderer;
    MTLScissorRect scissor = clipToScissor(renderer, state.clip);
    uint32_t blend = drawBlend(state);
    if (!count || !scissor.width || !scissor.height || blend == kBlendSkip) {
        return;
    }
    std::vector<Instance>& instances = renderer->instances;
    std::vector<Batch>& batches = *state.batchSink;
    if (batches.empty() || batches.back().pipeline != PipelineTier1 ||
        batches.back().texture != nil || !scissorsEqual(batches.back().scissor, scissor) ||
        batches.back().blend != (blend != 0) || batches.back().shader != 0 ||
        batches.back().start + batches.back().count != instances.size()) {
        ensureVectorCapacity(renderer, batches, batches.size() + 1);
        Batch batch = {PipelineTier1, nil, scissor, static_cast<uint32_t>(instances.size()), 0, blend != 0, 0};
        batches.push_back(batch);
    }
    Instance base = {};
    setFullContentMask(renderer, base);
    base.opacity = 1;
    base.roundedClip = state.roundedClip;
    base.coverageClip = state.coverageClip;
    base.blend = blend;
    const float scaleX = static_cast<float>(state.matrix[0] * renderer->scale);
    const float scaleY = static_cast<float>(state.matrix[3] * renderer->scale);
    const float shiftX = static_cast<float>(state.matrix[4] * renderer->scale);
    const float shiftY = static_cast<float>(state.matrix[5] * renderer->scale);
    size_t start = instances.size();
    ensureVectorCapacity(renderer, instances, start + count);
    instances.resize(start + count, base);
    Instance* out = instances.data() + start;
    float left = INFINITY, top = INFINITY, right = -INFINITY, bottom = -INFINITY;
    bool tracked = state.batchSink == &renderer->batches;
    for (uint32_t index = 0; index < count; ++index) {
        QuadOp op;
        memcpy(&op, records + static_cast<size_t>(index) * sizeof(QuadOp), sizeof(QuadOp));
        Instance& instance = out[index];
        instance.rect[0] = op.x * scaleX + shiftX;
        instance.rect[1] = op.y * scaleY + shiftY;
        instance.rect[2] = op.width * scaleX;
        instance.rect[3] = op.height * scaleY;
        instance.params[0] = op.cornerRadius * scaleX;
        instance.color = op.color;
        left = std::min(left, instance.rect[0]);
        top = std::min(top, instance.rect[1]);
        right = std::max(right, instance.rect[0] + instance.rect[2]);
        bottom = std::max(bottom, instance.rect[1] + instance.rect[3]);
        if (tracked) {
            noteBatchRect(
                state,
                std::max<double>(instance.rect[0], scissor.x),
                std::max<double>(instance.rect[1], scissor.y),
                std::min<double>(instance.rect[0] + instance.rect[2], scissor.x + scissor.width),
                std::min<double>(instance.rect[1] + instance.rect[3], scissor.y + scissor.height)
            );
        }
    }
    batches.back().count += count;
    if (state.layerDepth) {
        includeLayerBounds(
            state,
            std::max<double>(left - 1.0, scissor.x),
            std::max<double>(top - 1.0, scissor.y),
            std::min<double>(right + 1.0, scissor.x + scissor.width),
            std::min<double>(bottom + 1.0, scissor.y + scissor.height)
        );
    }
}

template <typename Value>
static bool readPacketValue(const uint8_t*& cursor, const uint8_t* end, Value& value) {
    if (static_cast<size_t>(end - cursor) < sizeof(Value)) {
        cursor = end;
        return false;
    }
    memcpy(&value, cursor, sizeof(Value));
    cursor += sizeof(Value);
    return true;
}

static uint64_t mixResourceKey(uint64_t hash, uint64_t value) {
    hash ^= value;
    return hash * 1099511628211ull;
}

static uint64_t glyphKey(CTFontRef font, CGGlyph glyph, int subpixel, double scale) {
    uint64_t key = 1469598103934665603ull;
    key = mixResourceKey(key, reinterpret_cast<uintptr_t>(font));
    key = mixResourceKey(key, glyph);
    key = mixResourceKey(key, static_cast<uint64_t>(subpixel));
    uint64_t scaleBits;
    memcpy(&scaleBits, &scale, sizeof(scaleBits));
    return mixResourceKey(key, scaleBits);
}

static uint64_t glyphProxyKey(CTFontRef font, CGGlyph glyph) {
    uint64_t key = 1469598103934665603ull;
    key = mixResourceKey(key, reinterpret_cast<uintptr_t>(font));
    return mixResourceKey(key, glyph);
}

static bool uploadFits(MetalRenderer* renderer, uint64_t bytes) {
    return renderer->frameUploadBytes == 0 || renderer->frameUploadBytes + bytes <= kFrameUploadCap;
}

static void noteCompletedFrame(MetalRenderer* renderer, int64_t frame) {
    int64_t completed = renderer->completedFrame.load();
    while (completed < frame && !renderer->completedFrame.compare_exchange_weak(completed, frame)) {
    }
}

static void awaitFrame(MetalRenderer* renderer, int64_t frame) {
    if (frame <= renderer->completedFrame.load()) {
        return;
    }
    id<MTLCommandBuffer> latest = nil;
    int64_t latestFrame = -1;
    for (size_t index = 0; index < renderer->committedFrames.size(); ++index) {
        int64_t committed = renderer->committedFrames[index];
        if (committed <= frame && committed > latestFrame) {
            latestFrame = committed;
            latest = renderer->committedBuffers[index];
        }
    }
    [latest waitUntilCompleted];
}

static uint64_t imageKey(
    uint32_t index,
    uint32_t generation,
    int32_t width,
    int32_t height,
    bool templateAlpha
) {
    uint64_t key = 1469598103934665603ull;
    key = mixResourceKey(key, 2);
    key = mixResourceKey(key, index);
    key = mixResourceKey(key, generation);
    key = mixResourceKey(key, static_cast<uint32_t>(width));
    key = mixResourceKey(key, static_cast<uint32_t>(height));
    return mixResourceKey(key, templateAlpha ? 1 : 0);
}

static uint64_t canvasImageKey(uint64_t serial, int32_t width, int32_t height, bool templateAlpha) {
    uint64_t key = 1469598103934665603ull;
    key = mixResourceKey(key, 3);
    key = mixResourceKey(key, static_cast<uint32_t>(serial));
    key = mixResourceKey(key, static_cast<uint32_t>(serial >> 32));
    key = mixResourceKey(key, static_cast<uint32_t>(width));
    key = mixResourceKey(key, static_cast<uint32_t>(height));
    return mixResourceKey(key, templateAlpha ? 1 : 0);
}

static bool allocateAtlasEntry(
    MetalRenderer* renderer,
    MaskAtlas& atlas,
    std::unordered_map<uint64_t, AtlasEntry>& entries,
    int width,
    int height,
    uint16_t& pageIndex,
    uint16_t& x,
    uint16_t& y
) {
    if (width <= 0 || height <= 0 || width > kAtlasSize || height > kAtlasSize) {
        return false;
    }
    for (int attempt = 0; attempt < 2; ++attempt) {
        for (size_t index = 0; index < atlas.pages.size(); ++index) {
            AtlasPage& page = atlas.pages[index];
            if (page.shelfX + width > kAtlasSize) {
                int nextY = page.shelfY + page.shelfHeight;
                if (nextY + height > kAtlasSize) {
                    continue;
                }
                page.shelfX = 0;
                page.shelfY = nextY;
                page.shelfHeight = 0;
            }
            if (page.shelfY + height > kAtlasSize) {
                continue;
            }
            pageIndex = static_cast<uint16_t>(index);
            x = static_cast<uint16_t>(page.shelfX);
            y = static_cast<uint16_t>(page.shelfY);
            page.shelfX += width;
            page.shelfHeight = std::max(page.shelfHeight, height);
            return true;
        }
        if (atlas.pages.size() < atlas.maxPages) {
            MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:atlas.pixelFormat
                                             width:kAtlasSize
                                            height:kAtlasSize
                                         mipmapped:NO];
            descriptor.storageMode = MTLStorageModeShared;
            AtlasPage page;
            page.texture = [renderer->device newTextureWithDescriptor:descriptor];
            if (!page.texture) {
                return false;
            }
            ensureVectorCapacity(renderer, atlas.pages, atlas.pages.size() + 1);
            atlas.pages.push_back(page);
            countAllocation(renderer);
            continue;
        }

        int64_t building = renderer->frameCount - 1;
        size_t victim = atlas.pages.size();
        for (size_t index = 0; index < atlas.pages.size(); ++index) {
            int64_t lastUsed = atlas.pages[index].lastUsedFrame;
            if (lastUsed >= building) {
                continue;
            }
            if (victim == atlas.pages.size() || lastUsed < atlas.pages[victim].lastUsedFrame) {
                victim = index;
            }
        }
        if (victim == atlas.pages.size()) {
            return false;
        }
        AtlasPage& page = atlas.pages[victim];
        awaitFrame(renderer, page.lastUsedFrame);
        page.shelfX = 0;
        page.shelfY = 0;
        page.shelfHeight = 0;
        page.lastUsedFrame = renderer->frameCount;
        ++atlas.evictions;
        for (auto entry = entries.begin(); entry != entries.end();) {
            if (entry->second.page == victim) {
                entry = entries.erase(entry);
            } else {
                ++entry;
            }
        }
    }
    return false;
}

struct GlyphBitmap {
    int width = 0;
    int height = 0;
    int left = 0;
    int top = 0;
};

static bool rasterizeGlyph(
    MetalRenderer* renderer,
    CTFontRef font,
    CGGlyph glyph,
    int subpixel,
    double scale,
    GlyphBitmap& bitmap
) {
    CGRect bounds = CGRectZero;
    CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationDefault, &glyph, &bounds, 1);
    if (bounds.size.width <= 0 || bounds.size.height <= 0) {
        return false;
    }
    const int padding = 1;
    double fraction = subpixel / 4.0;
    bitmap.left = static_cast<int>(floor(bounds.origin.x * scale + fraction)) - padding;
    int bottom = static_cast<int>(floor(bounds.origin.y * scale)) - padding;
    int right = static_cast<int>(ceil(
        (bounds.origin.x + bounds.size.width) * scale + fraction
    )) + padding;
    bitmap.top = static_cast<int>(ceil(
        (bounds.origin.y + bounds.size.height) * scale
    )) + padding;
    bitmap.width = right - bitmap.left;
    bitmap.height = bitmap.top - bottom;
    if (bitmap.width <= 0 || bitmap.height <= 0) {
        return false;
    }
    size_t bytes = static_cast<size_t>(bitmap.width) * bitmap.height;
    ensureVectorCapacity(renderer, renderer->rasterScratch, bytes);
    renderer->rasterScratch.resize(bytes);
    memset(renderer->rasterScratch.data(), 0, bytes);
    CGContextRef context = CGBitmapContextCreate(
        renderer->rasterScratch.data(),
        bitmap.width,
        bitmap.height,
        8,
        bitmap.width,
        nullptr,
        kCGImageAlphaOnly
    );
    if (!context) {
        return false;
    }
    CGContextSetAllowsFontSmoothing(context, false);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextSetAllowsAntialiasing(context, true);
    CGContextSetShouldAntialias(context, true);
    CGContextScaleCTM(context, scale, scale);
    CGPoint position = CGPointMake(
        (-bitmap.left + fraction) / scale,
        (-bottom) / scale
    );
    CGContextSetGrayFillColor(context, 1, 1);
    CTFontDrawGlyphs(font, &glyph, &position, 1, context);
    CGContextRelease(context);
    return true;
}

static void touchGlyphEntry(MetalRenderer* renderer, AtlasEntry& entry) {
    entry.lastUsedFrame = renderer->frameCount - 1;
    if (entry.valid) {
        renderer->glyphAtlas.pages[entry.page].lastUsedFrame = renderer->frameCount - 1;
    }
}

static AtlasEntry* ensureGlyphEntry(
    MetalRenderer* renderer,
    CTFontRef font,
    CGGlyph glyph,
    int subpixel,
    double scale,
    FrameStats& stats,
    bool& deferred
) {
    uint64_t key = glyphKey(font, glyph, subpixel, scale);
    auto found = renderer->glyphEntries.find(key);
    if (found != renderer->glyphEntries.end()) {
        touchGlyphEntry(renderer, found->second);
        return &found->second;
    }
    if (renderer->glyphUploadsDeferred) {
        deferred = true;
        return nullptr;
    }
    if (renderer->glyphAtlasFull) {
        return nullptr;
    }

    GlyphBitmap bitmap;
    if (!rasterizeGlyph(renderer, font, glyph, subpixel, scale, bitmap)) {
        AtlasEntry empty;
        auto inserted = renderer->glyphEntries.emplace(key, empty);
        return &inserted.first->second;
    }
    if (bitmap.width > kAtlasSize || bitmap.height > kAtlasSize) {
        return nullptr;
    }
    uint64_t bytes = static_cast<uint64_t>(bitmap.width) * bitmap.height;
    if (!uploadFits(renderer, bytes)) {
        renderer->glyphUploadsDeferred = true;
        deferred = true;
        return nullptr;
    }
    uint16_t page = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    if (!allocateAtlasEntry(
        renderer,
        renderer->glyphAtlas,
        renderer->glyphEntries,
        bitmap.width,
        bitmap.height,
        page,
        x,
        y
    )) {
        renderer->glyphAtlasFull = true;
        return nullptr;
    }
    [renderer->glyphAtlas.pages[page].texture
        replaceRegion:MTLRegionMake2D(x, y, bitmap.width, bitmap.height)
        mipmapLevel:0
        withBytes:renderer->rasterScratch.data()
        bytesPerRow:bitmap.width];
    renderer->frameUploadBytes += bytes;
    ++stats.glyphUploads;
    stats.glyphUploadBytes += bytes;

    AtlasEntry entry;
    entry.page = page;
    entry.x = x;
    entry.y = y;
    entry.width = static_cast<uint16_t>(bitmap.width);
    entry.height = static_cast<uint16_t>(bitmap.height);
    entry.offsetX = static_cast<int16_t>(bitmap.left);
    entry.offsetTopY = static_cast<int16_t>(bitmap.top);
    entry.scale = static_cast<float>(scale);
    entry.lastUsedFrame = renderer->frameCount - 1;
    entry.valid = true;
    renderer->glyphAtlas.pages[page].lastUsedFrame = renderer->frameCount - 1;
    auto inserted = renderer->glyphEntries.emplace(key, entry);
    renderer->glyphProxies[glyphProxyKey(font, glyph)] = key;
    return &inserted.first->second;
}

static AtlasEntry* findGlyphProxy(MetalRenderer* renderer, CTFontRef font, CGGlyph glyph) {
    auto proxy = renderer->glyphProxies.find(glyphProxyKey(font, glyph));
    if (proxy == renderer->glyphProxies.end()) {
        return nullptr;
    }
    auto found = renderer->glyphEntries.find(proxy->second);
    if (found == renderer->glyphEntries.end() || !found->second.valid || !(found->second.scale > 0)) {
        return nullptr;
    }
    touchGlyphEntry(renderer, found->second);
    return &found->second;
}

static void applyContentMask(
    MetalRenderer* renderer,
    const float contentMask[4],
    const ExpandState& state,
    Instance& instance
) {
    if (contentMask[2] > contentMask[0] && contentMask[3] > contentMask[1]) {
        instance.contentMask[0] = static_cast<float>(
            (contentMask[0] * state.matrix[0] + state.matrix[4]) * renderer->scale
        );
        instance.contentMask[1] = static_cast<float>(
            (contentMask[1] * state.matrix[3] + state.matrix[5]) * renderer->scale
        );
        instance.contentMask[2] = static_cast<float>(
            (contentMask[2] * state.matrix[0] + state.matrix[4]) * renderer->scale
        );
        instance.contentMask[3] = static_cast<float>(
            (contentMask[3] * state.matrix[3] + state.matrix[5]) * renderer->scale
        );
    } else {
        setFullContentMask(renderer, instance);
    }
}

static void emitGlyphInstance(
    ExpandState& state,
    const GlyphRunOp& op,
    const AtlasEntry& entry,
    __unsafe_unretained id<MTLTexture> texture,
    float textureWidth,
    float textureHeight,
    double penFloor,
    double baseline,
    double glyphY,
    const RunGlyph& glyph,
    const GlyphPaint& paint,
    double ratio = 1
) {
    MetalRenderer* renderer = state.renderer;
    Instance instance = {};
    applyContentMask(renderer, op.contentMask, state, instance);
    instance.rect[0] = static_cast<float>(penFloor + entry.offsetX * ratio);
    instance.rect[1] = static_cast<float>(
        baseline - glyphY * state.matrix[3] * renderer->scale - entry.offsetTopY * ratio
    );
    instance.rect[2] = static_cast<float>(entry.width * ratio);
    instance.rect[3] = static_cast<float>(entry.height * ratio);
    instance.params[0] = entry.x / textureWidth;
    instance.params[1] = entry.y / textureHeight;
    instance.params[2] = entry.width / textureWidth;
    instance.params[3] = entry.height / textureHeight;
    instance.color = glyph.hasColor ? glyph.color : op.color;
    instance.kind = ratio == 1 ? 2 : 2 | kInstanceLinear;
    instance.opacity = 1;
    instance.padding[0] = paint.record;
    emitInstance(state, PipelineMask, texture, instance, paint.shader);
}

static bool emitFallbackGlyph(
    ExpandState& state,
    const GlyphRunOp& op,
    CTFontRef font,
    const RunGlyph& glyph,
    int subpixel,
    double penFloor,
    double baseline,
    const GlyphPaint& paint,
    FrameStats& stats
) {
    MetalRenderer* renderer = state.renderer;
    GlyphBitmap bitmap;
    if (!rasterizeGlyph(renderer, font, glyph.glyph, subpixel, renderer->scale * state.matrix[0], bitmap)) {
        return false;
    }
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatA8Unorm
                                     width:bitmap.width
                                    height:bitmap.height
                                 mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    id<MTLTexture> texture = [renderer->device newTextureWithDescriptor:descriptor];
    if (!texture) {
        return false;
    }
    [texture replaceRegion:MTLRegionMake2D(0, 0, bitmap.width, bitmap.height)
               mipmapLevel:0
                 withBytes:renderer->rasterScratch.data()
               bytesPerRow:bitmap.width];
    std::vector<id<MTLTexture>>& textures = renderer->fallbackTextures[renderer->packetIndex];
    ensureVectorCapacity(renderer, textures, textures.size() + 1);
    textures.push_back(texture);
    countAllocation(renderer);

    AtlasEntry entry;
    entry.width = static_cast<uint16_t>(bitmap.width);
    entry.height = static_cast<uint16_t>(bitmap.height);
    entry.offsetX = static_cast<int16_t>(bitmap.left);
    entry.offsetTopY = static_cast<int16_t>(bitmap.top);
    entry.valid = true;
    emitGlyphInstance(
        state,
        op,
        entry,
        texture,
        static_cast<float>(bitmap.width),
        static_cast<float>(bitmap.height),
        penFloor,
        baseline,
        glyph.y,
        glyph,
        paint
    );
    ++stats.fallbackRuns;
    return true;
}

static void fillMultiply(const double* left, const double* right, double* out);
static uint32_t appendGradientPaint(
    MetalRenderer* renderer,
    const ChunkPathSet& set,
    uint32_t index,
    const double* device,
    const double* paper,
    float alpha,
    uint32_t& shader,
    bool text = false
);

static GlyphPaint glyphRunPaint(ExpandState& state, const GlyphRunOp& op) {
    GlyphPaint paint;
    MetalRenderer* renderer = state.renderer;
    if (!op.paint || op.pathSlot >= renderer->chunkPaths.size()) {
        return paint;
    }
    const ChunkPathSet& set = renderer->chunkPaths[op.pathSlot];
    if (!op.pathSerial || set.serial != op.pathSerial) {
        return paint;
    }
    double device[6] = {renderer->scale, 0, 0, renderer->scale, 0, 0};
    fillMultiply(device, state.matrix, device);
    uint32_t shader = 0;
    uint32_t record = appendGradientPaint(renderer, set, op.paint, device, state.paper, 1, shader, true);
    if (!record || !shader) {
        return paint;
    }
    paint.record = record;
    paint.shader = shader;
    return paint;
}

static const double kGreekEmPixels = 4.0;
static const double kGlyphLadderSteps = 4.0;

static const double kGlyphHalfPixelEm = 32.0;
static const double kGlyphWholePixelEm = 64.0;

static double glyphLadderScale(double scale) {
    return exp2(ceil(log2(scale) * kGlyphLadderSteps - 1e-6) / kGlyphLadderSteps);
}

static double glyphPenSnap(double pen) {
    return round(pen * kGlyphPenSteps) / kGlyphPenSteps;
}

static int glyphSubpixel(double fraction, double emPixels) {
    int steps = emPixels < kGlyphHalfPixelEm ? 4 : emPixels < kGlyphWholePixelEm ? 2 : 1;
    return (static_cast<int>(floor(fraction * steps)) * (4 / steps)) & 3;
}
static const float kGreekCoverage = 0.37f;
static const double kQuadCornerInk = (M_PI + 4) / 24;

static void emitGreekedRun(ExpandState& state, const GlyphRunOp& op, const RunRecord& run, double zoom) {
    MetalRenderer* renderer = state.renderer;
    double scale = renderer->scale;
    double blockX = op.x * zoom + state.matrix[4];
    double blockY = op.y * zoom + state.matrix[5];
    double offsetY = (op.height - run.height) / 2.0;
    double height = run.xHeight * zoom * scale;
    for (const RunLine& line : run.lines) {
        if (!line.glyphCount) {
            continue;
        }
        double offsetX = 0;
        if (op.alignment == 1) {
            offsetX = (op.width - line.width) / 2.0;
        } else if (op.alignment == 2) {
            offsetX = op.width - line.width;
        }
        double baseline = (blockY + (offsetY + line.baseline) * zoom) * scale;
        double left = (blockX + (offsetX + run.glyphs[line.glyphStart].x) * zoom) * scale;
        double right = (blockX + (offsetX + line.width) * zoom) * scale;
        if (!(right > left) || !(height > 0)) {
            continue;
        }
        double width = right - left;
        double barWidth = std::max(width, 1.0);
        double barHeight = std::max(height, 1.0);
        Instance instance = {};
        applyContentMask(renderer, op.contentMask, state, instance);
        instance.rect[0] = static_cast<float>((left + right - barWidth) * 0.5);
        instance.rect[1] = static_cast<float>(baseline - (height + barHeight) * 0.5);
        instance.rect[2] = static_cast<float>(barWidth);
        instance.rect[3] = static_cast<float>(barHeight);
        instance.color = op.color;
        instance.kind = 0;
        instance.opacity = static_cast<float>(kGreekCoverage * width * height / (barWidth * barHeight + kQuadCornerInk));
        emitInstance(state, PipelineTier1, nil, instance);
    }
}

static void expandGlyphRun(
    ExpandState& state,
    const GlyphRunOp& op,
    FrameStats& stats
) {
    MetalRenderer* renderer = state.renderer;
    if (op.index >= renderer->runs.size()) {
        ++stats.skippedOps;
        return;
    }
    RunRecord& run = renderer->runs[op.index];
    if (!run.alive || run.generation != op.generation) {
        ++stats.skippedOps;
        return;
    }
    GlyphPaint paint = glyphRunPaint(state, op);
    double zoom = state.matrix[0];
    double glyphScale = renderer->scale * zoom;
    if (zoom > 0 && state.matrix[1] == 0 && state.matrix[2] == 0 &&
            run.emSize * glyphScale < kGreekEmPixels && !paint.shader && !run.tinted) {
        emitGreekedRun(state, op, run, zoom);
        return;
    }
    int64_t frame = renderer->frameCount - 1;
    bool tileFrame = state.tileFrame;
    if (!tileFrame && run.drawnFrame != frame) {
        bool changed = run.drawnScale > 0 && run.drawnScale != glyphScale;
        run.moving = changed && run.scaleChanging;
        run.scaleChanging = changed;
        run.drawnScale = glyphScale;
        run.drawnFrame = frame;
    }
    double rasterScale = run.moving && !tileFrame ? glyphLadderScale(glyphScale) : glyphScale;
    double ratio = glyphScale / rasterScale;
    size_t slots = run.glyphs.size() * 4;
    if (run.entryScale != rasterScale || run.entryEpoch != renderer->glyphAtlas.evictions ||
            run.entries.size() != slots) {
        run.entries.assign(slots, nullptr);
        run.entryScale = rasterScale;
        run.entryEpoch = renderer->glyphAtlas.evictions;
    }
    bool stand = ratio != 1;
    const float atlasSize = static_cast<float>(kAtlasSize);
    double blockX = op.x * zoom + state.matrix[4];
    double blockY = op.y * zoom + state.matrix[5];
    double offsetY = (op.height - run.height) / 2.0;
    for (const RunLine& line : run.lines) {
        double offsetX = 0;
        if (op.alignment == 1) {
            offsetX = (op.width - line.width) / 2.0;
        } else if (op.alignment == 2) {
            offsetX = op.width - line.width;
        }
        double baseline = (blockY + (offsetY + line.baseline) * zoom) * renderer->scale;
        for (uint32_t offset = 0; offset < line.glyphCount; ++offset) {
            const RunGlyph& glyph = run.glyphs[line.glyphStart + offset];
            CTFontRef font = run.fonts[glyph.fontIndex];
            double pen = glyphPenSnap((blockX + (offsetX + glyph.x) * zoom) * renderer->scale);
            double penFloor = floor(pen);
            int subpixel = glyphSubpixel(pen - penFloor, CTFontGetSize(font) * rasterScale);
            size_t slot = static_cast<size_t>(line.glyphStart + offset) * 4 + static_cast<size_t>(subpixel);
            AtlasEntry* entry = run.entries[slot];
            bool deferred = false;
            if (entry) {
                touchGlyphEntry(renderer, *entry);
            } else {
                entry = ensureGlyphEntry(renderer, font, glyph.glyph, subpixel, rasterScale, stats, deferred);
                if (run.entryEpoch != renderer->glyphAtlas.evictions) {
                    run.entries.assign(slots, nullptr);
                    run.entryEpoch = renderer->glyphAtlas.evictions;
                }
                run.entries[slot] = entry;
            }
            AtlasEntry* proxy = deferred && !tileFrame ? findGlyphProxy(renderer, font, glyph.glyph) : nullptr;
            if (!entry && tileFrame) {
                if (deferred) {
                    renderer->tileGlyphsDeferred = true;
                } else {
                    renderer->tileGlyphsMissing = true;
                }
            } else if (proxy) {
                stand = true;
                emitGlyphInstance(
                    state,
                    op,
                    *proxy,
                    renderer->glyphAtlas.pages[proxy->page].texture,
                    atlasSize,
                    atlasSize,
                    penFloor,
                    baseline,
                    glyph.y,
                    glyph,
                    paint,
                    glyphScale / proxy->scale
                );
            } else if (!entry) {
                emitFallbackGlyph(
                    state,
                    op,
                    font,
                    glyph,
                    subpixel,
                    penFloor,
                    baseline,
                    paint,
                    stats
                );
            } else if (entry->valid) {
                __unsafe_unretained id<MTLTexture> texture = renderer->glyphAtlas.pages[entry->page].texture;
                emitGlyphInstance(
                    state,
                    op,
                    *entry,
                    texture,
                    atlasSize,
                    atlasSize,
                    penFloor,
                    baseline,
                    glyph.y,
                    glyph,
                    paint,
                    ratio
                );
            }
        }
    }
    if (stand) {
        renderer->glyphRedraw.store(true);
    }
}

static bool expandImagePaint(ExpandState& state, const ImageOp& op);

static void emitImageQuad(
    ExpandState& state,
    const ImageOp& op,
    id<MTLTexture> texture,
    const MTLScissorRect& scissor,
    const double* device,
    const double* uv,
    uint32_t kind
) {
    double left = std::max<double>(device[0], scissor.x);
    double top = std::max<double>(device[1], scissor.y);
    double right = std::min<double>(device[2], scissor.x + scissor.width);
    double bottom = std::min<double>(device[3], scissor.y + scissor.height);
    if (right <= left || bottom <= top || device[2] <= device[0] || device[3] <= device[1]) {
        return;
    }
    double du = (uv[2] - uv[0]) / (device[2] - device[0]);
    double dv = (uv[3] - uv[1]) / (device[3] - device[1]);
    Instance instance = {};
    applyContentMask(state.renderer, op.contentMask, state, instance);
    instance.rect[0] = static_cast<float>(left);
    instance.rect[1] = static_cast<float>(top);
    instance.rect[2] = static_cast<float>(right - left);
    instance.rect[3] = static_cast<float>(bottom - top);
    instance.params[0] = static_cast<float>(uv[0] + (left - device[0]) * du);
    instance.params[1] = static_cast<float>(uv[1] + (top - device[1]) * dv);
    instance.params[2] = static_cast<float>((right - left) * du);
    instance.params[3] = static_cast<float>((bottom - top) * dv);
    instance.color = op.color;
    instance.kind = imageOpKind(op, kind);
    instance.opacity = 1;
    emitInstance(state, PipelineRGBA, texture, instance);
}

} // namespace

static ImageTile* ensureImageTile(
    MetalRenderer* renderer,
    ImageTexture& image,
    uint32_t level,
    uint32_t column,
    uint32_t row,
    bool& pending
);
static bool ensureImagePreview(MetalRenderer* renderer, ImageTexture& image);
static ImageTile* findImageTile(ImageTexture& image, uint32_t level, uint32_t column, uint32_t row);
static uint32_t imageLevelExtent(uint32_t size, uint32_t level);

static void sweepBrushDabs(MetalRenderer* renderer);
static void releaseBrushDabs(std::vector<FillSpanRecord>& spans);

namespace {

struct ImageFrame {
    double origin[2];
    double axisX[2];
    double axisY[2];
    bool aligned;
};

static ImageFrame imageFrame(ExpandState& state, const ImageOp& op, double width, double height) {
    const double* matrix = state.matrix;
    double scale = state.renderer->scale;
    ImageFrame frame;
    frame.origin[0] = (op.x * matrix[0] + op.y * matrix[2] + matrix[4]) * scale;
    frame.origin[1] = (op.x * matrix[1] + op.y * matrix[3] + matrix[5]) * scale;
    frame.axisX[0] = matrix[0] * scale * op.width / width;
    frame.axisX[1] = matrix[1] * scale * op.width / width;
    frame.axisY[0] = matrix[2] * scale * op.height / height;
    frame.axisY[1] = matrix[3] * scale * op.height / height;
    frame.aligned = matrix[1] == 0 && matrix[2] == 0 && matrix[0] > 0 && matrix[3] > 0;
    return frame;
}

static void imageFramePoint(const ImageFrame& frame, double x, double y, double* point) {
    point[0] = frame.origin[0] + x * frame.axisX[0] + y * frame.axisY[0];
    point[1] = frame.origin[1] + x * frame.axisX[1] + y * frame.axisY[1];
}

static void imageFrameBounds(const ImageFrame& frame, const double* region, double* bounds) {
    bounds[0] = INFINITY;
    bounds[1] = INFINITY;
    bounds[2] = -INFINITY;
    bounds[3] = -INFINITY;
    const double corners[4][2] = {
        {region[0], region[1]}, {region[2], region[1]}, {region[0], region[3]}, {region[2], region[3]},
    };
    for (const auto& corner : corners) {
        double point[2];
        imageFramePoint(frame, corner[0], corner[1], point);
        bounds[0] = std::min(bounds[0], point[0]);
        bounds[1] = std::min(bounds[1], point[1]);
        bounds[2] = std::max(bounds[2], point[0]);
        bounds[3] = std::max(bounds[3], point[1]);
    }
}

static bool imageFrameInverse(const ImageFrame& frame, double x, double y, double* point) {
    double determinant = frame.axisX[0] * frame.axisY[1] - frame.axisX[1] * frame.axisY[0];
    if (!(std::fabs(determinant) > 1e-12)) {
        return false;
    }
    double dx = x - frame.origin[0];
    double dy = y - frame.origin[1];
    point[0] = (dx * frame.axisY[1] - dy * frame.axisY[0]) / determinant;
    point[1] = (dy * frame.axisX[0] - dx * frame.axisX[1]) / determinant;
    return true;
}

static void emitImageRegion(
    ExpandState& state,
    const ImageOp& op,
    id<MTLTexture> texture,
    const MTLScissorRect& scissor,
    const ImageFrame& frame,
    const double* region,
    const double* uv,
    uint32_t kind
) {
    if (frame.aligned) {
        const double device[4] = {
            frame.origin[0] + region[0] * frame.axisX[0],
            frame.origin[1] + region[1] * frame.axisY[1],
            frame.origin[0] + region[2] * frame.axisX[0],
            frame.origin[1] + region[3] * frame.axisY[1],
        };
        emitImageQuad(state, op, texture, scissor, device, uv, kind);
        return;
    }
    double origin[2];
    imageFramePoint(frame, region[0], region[1], origin);
    double spanX = region[2] - region[0];
    double spanY = region[3] - region[1];
    double axisU[2] = {frame.axisX[0] * spanX, frame.axisX[1] * spanX};
    double axisV[2] = {frame.axisY[0] * spanY, frame.axisY[1] * spanY};
    if (!(std::fabs(axisU[0] * axisV[1] - axisU[1] * axisV[0]) > 1e-9)) {
        return;
    }
    Instance instance = {};
    applyContentMask(state.renderer, op.contentMask, state, instance);
    instance.rect[0] = static_cast<float>(origin[0]);
    instance.rect[1] = static_cast<float>(origin[1]);
    instance.rect[2] = static_cast<float>(axisU[0]);
    instance.rect[3] = static_cast<float>(axisU[1]);
    instance.params[0] = static_cast<float>(axisV[0]);
    instance.params[1] = static_cast<float>(axisV[1]);
    instance.params[2] = static_cast<float>(uv[0]);
    instance.params[3] = static_cast<float>(uv[1]);
    float uvSize[2] = {static_cast<float>(uv[2] - uv[0]), static_cast<float>(uv[3] - uv[1])};
    memcpy(instance.padding, uvSize, sizeof(uvSize));
    instance.color = op.color;
    instance.kind = imageOpKind(op, kind | kInstanceAffine);
    instance.opacity = 1;
    emitInstance(state, PipelineRGBA, texture, instance);
}

static MTLScissorRect imageDrawScissor(MetalRenderer* renderer, const ClipState& clip) {
    MTLScissorRect scissor = clipToScissor(renderer, clip);
    if (renderer->encodeMode != 1 || renderer->encodeDamage[2] <= 0 || renderer->encodeDamage[3] <= 0) {
        return scissor;
    }
    double left = std::max<double>(scissor.x, floor(renderer->encodeDamage[0] * renderer->scale));
    double top = std::max<double>(scissor.y, floor(renderer->encodeDamage[1] * renderer->scale));
    double right = std::min<double>(
        scissor.x + scissor.width,
        ceil((renderer->encodeDamage[0] + renderer->encodeDamage[2]) * renderer->scale)
    );
    double bottom = std::min<double>(
        scissor.y + scissor.height,
        ceil((renderer->encodeDamage[1] + renderer->encodeDamage[3]) * renderer->scale)
    );
    if (right <= left || bottom <= top) {
        return MTLScissorRect{0, 0, 0, 0};
    }
    return MTLScissorRect{
        static_cast<NSUInteger>(left),
        static_cast<NSUInteger>(top),
        static_cast<NSUInteger>(right - left),
        static_cast<NSUInteger>(bottom - top),
    };
}

static void addTileDamage(MetalRenderer* renderer, const MTLScissorRect& scissor, const double* bounds) {
    double left = std::max<double>(scissor.x, floor(bounds[0]));
    double top = std::max<double>(scissor.y, floor(bounds[1]));
    double right = std::min<double>(scissor.x + scissor.width, ceil(bounds[2]));
    double bottom = std::min<double>(scissor.y + scissor.height, ceil(bounds[3]));
    if (right <= left || bottom <= top) {
        return;
    }
    if (renderer->hasTileDamage) {
        left = std::min(left, renderer->tileDamage[0]);
        top = std::min(top, renderer->tileDamage[1]);
        right = std::max(right, renderer->tileDamage[2]);
        bottom = std::max(bottom, renderer->tileDamage[3]);
    }
    renderer->tileDamage[0] = left;
    renderer->tileDamage[1] = top;
    renderer->tileDamage[2] = right;
    renderer->tileDamage[3] = bottom;
    renderer->hasTileDamage = true;
}

static void imageTileBounds(
    const ImageTexture& image,
    uint32_t level,
    uint32_t column,
    uint32_t row,
    double* bounds
) {
    double span = static_cast<double>(kImageTileContent) * (1u << level);
    bounds[0] = column * span;
    bounds[1] = row * span;
    bounds[2] = std::min<double>((column + 1) * span, image.width);
    bounds[3] = std::min<double>((row + 1) * span, image.height);
}

static uint32_t imageKind(const ImageTexture& image, uint32_t kind) {
    return kind | (static_cast<uint32_t>(image.space) << kInstanceSpaceShift);
}

static uint32_t imageRegionEdges(const ImageTexture& image, const double* region) {
    return (region[0] == 0 ? 1u : 0u) | (region[1] == 0 ? 2u : 0u) |
        (region[2] == image.width ? 4u : 0u) | (region[3] == image.height ? 8u : 0u);
}

static void emitImageTileRegion(
    ExpandState& state,
    const ImageOp& op,
    const ImageTexture& image,
    uint32_t level,
    uint32_t column,
    uint32_t row,
    const ImageTile& tile,
    const MTLScissorRect& scissor,
    const ImageFrame& frame,
    const double* region
) {
    double factor = static_cast<double>(1u << level);
    double size = static_cast<double>(kImageTileSize);
    double originX = static_cast<double>(column) * kImageTileContent - kImageTileGutter;
    double originY = static_cast<double>(row) * kImageTileContent - kImageTileGutter;
    const double uv[4] = {
        (region[0] / factor - originX) / size,
        (region[1] / factor - originY) / size,
        (region[2] / factor - originX) / size,
        (region[3] / factor - originY) / size,
    };
    uint32_t kind = imageKind(image, (op.sampling ? 8u : 7u) | (imageRegionEdges(image, region) << 8));
    emitImageRegion(state, op, tile.surface.texture, scissor, frame, region, uv, kind);
}

static void drawImageTileFallback(
    ExpandState& state,
    const ImageOp& op,
    ImageTexture& image,
    uint32_t level,
    uint32_t column,
    uint32_t row,
    const MTLScissorRect& scissor,
    const ImageFrame& frame,
    const double* region
) {
    MetalRenderer* renderer = state.renderer;
    for (uint32_t parent = level + 1; parent < image.levels; ++parent) {
        uint32_t shift = parent - level;
        ImageTile* tile = findImageTile(image, parent, column >> shift, row >> shift);
        if (tile && tile->ready) {
            tile->lastUsedFrame = renderer->frameCount;
            emitImageTileRegion(state, op, image, parent, column >> shift, row >> shift, *tile, scissor, frame, region);
            return;
        }
    }
    if (ensureImagePreview(renderer, image)) {
        const double uv[4] = {
            region[0] / image.width,
            region[1] / image.height,
            region[2] / image.width,
            region[3] / image.height,
        };
        uint32_t kind = imageKind(image, (op.sampling ? 8u : 7u) | (imageRegionEdges(image, region) << 8));
        emitImageRegion(state, op, image.preview.texture, scissor, frame, region, uv, kind);
    }
}

static void expandImageTiles(ExpandState& state, const ImageOp& op, ImageTexture& image) {
    MetalRenderer* renderer = state.renderer;
    image.lastUsedFrame = renderer->frameCount;
    MTLScissorRect scissor = imageDrawScissor(renderer, state.clip);
    if (!scissor.width || !scissor.height) {
        return;
    }
    ImageFrame frame = imageFrame(state, op, image.width, image.height);
    double spanX = hypot(frame.axisX[0], frame.axisX[1]);
    double spanY = hypot(frame.axisY[0], frame.axisY[1]);
    double visible[4] = {INFINITY, INFINITY, -INFINITY, -INFINITY};
    const double corners[4][2] = {
        {static_cast<double>(scissor.x), static_cast<double>(scissor.y)},
        {static_cast<double>(scissor.x + scissor.width), static_cast<double>(scissor.y)},
        {static_cast<double>(scissor.x), static_cast<double>(scissor.y + scissor.height)},
        {static_cast<double>(scissor.x + scissor.width), static_cast<double>(scissor.y + scissor.height)},
    };
    for (const auto& corner : corners) {
        double point[2];
        if (!imageFrameInverse(frame, corner[0], corner[1], point)) {
            return;
        }
        visible[0] = std::min(visible[0], point[0]);
        visible[1] = std::min(visible[1], point[1]);
        visible[2] = std::max(visible[2], point[0]);
        visible[3] = std::max(visible[3], point[1]);
    }
    visible[0] = std::max(0.0, visible[0]);
    visible[1] = std::max(0.0, visible[1]);
    visible[2] = std::min<double>(image.width, visible[2]);
    visible[3] = std::min<double>(image.height, visible[3]);
    if (visible[2] <= visible[0] || visible[3] <= visible[1] || !(spanX > 0) || !(spanY > 0)) {
        return;
    }
    double lod = -log2(std::min(spanX, spanY));
    uint32_t level = lod >= 1
        ? static_cast<uint32_t>(std::min<double>(image.levels - 1, floor(lod)))
        : 0;
    double span = static_cast<double>(kImageTileContent) * (1u << level);
    uint32_t columns = (imageLevelExtent(image.width, level) + kImageTileContent - 1) / kImageTileContent;
    uint32_t rows = (imageLevelExtent(image.height, level) + kImageTileContent - 1) / kImageTileContent;
    uint32_t firstColumn = std::min<uint32_t>(columns, static_cast<uint32_t>(floor(visible[0] / span)));
    uint32_t lastColumn = std::min<uint32_t>(columns, static_cast<uint32_t>(ceil(visible[2] / span)));
    uint32_t firstRow = std::min<uint32_t>(rows, static_cast<uint32_t>(floor(visible[1] / span)));
    uint32_t lastRow = std::min<uint32_t>(rows, static_cast<uint32_t>(ceil(visible[3] / span)));
    for (uint32_t row = firstRow; row < lastRow; ++row) {
        for (uint32_t column = firstColumn; column < lastColumn; ++column) {
            double region[4];
            imageTileBounds(image, level, column, row, region);
            double bounds[4];
            imageFrameBounds(frame, region, bounds);
            if (bounds[2] <= scissor.x || bounds[0] >= scissor.x + scissor.width ||
                bounds[3] <= scissor.y || bounds[1] >= scissor.y + scissor.height) {
                continue;
            }
            bool pending = false;
            ImageTile* tile = ensureImageTile(renderer, image, level, column, row, pending);
            if (tile) {
                emitImageTileRegion(state, op, image, level, column, row, *tile, scissor, frame, region);
            } else {
                drawImageTileFallback(state, op, image, level, column, row, scissor, frame, region);
            }
            if (pending) {
                addTileDamage(renderer, scissor, bounds);
            }
        }
    }
}

static void expandImageTexture(ExpandState& state, const ImageOp& op, ImageTexture& image) {
    MetalRenderer* renderer = state.renderer;
    image.lastUsedFrame = renderer->frameCount;
    MTLScissorRect scissor = clipToScissor(renderer, state.clip);
    if (!scissor.width || !scissor.height || !image.single.texture) {
        return;
    }
    ImageFrame frame = imageFrame(state, op, image.width, image.height);
    const double region[4] = {0, 0, static_cast<double>(image.width), static_cast<double>(image.height)};
    const double uv[4] = {0, 0, 1, 1};
    emitImageRegion(state, op, image.single.texture, scissor, frame, region, uv, imageKind(image, (op.sampling ? 8u : 7u) | (0xfu << 8)));
}

static void expandImagePattern(ExpandState& state, const ImageOp& op, ImageTexture& image) {
    MetalRenderer* renderer = state.renderer;
    image.lastUsedFrame = renderer->frameCount;
    MTLScissorRect scissor = clipToScissor(renderer, state.clip);
    if (!scissor.width || !scissor.height || op.tile[2] <= 0 || op.tile[3] <= 0 || !image.single.texture) {
        return;
    }
    double u = (static_cast<double>(op.x) - op.tile[0]) / op.tile[2];
    double v = (static_cast<double>(op.y) - op.tile[1]) / op.tile[3];
    const double uv[4] = {
        u - floor(u),
        v - floor(v),
        u - floor(u) + op.width / op.tile[2],
        v - floor(v) + op.height / op.tile[3],
    };
    ImageFrame frame = imageFrame(state, op, 1, 1);
    const double region[4] = {0, 0, 1, 1};
    if (frame.aligned) {
        double device[4];
        imageFramePoint(frame, 0, 0, device);
        imageFramePoint(frame, 1, 1, device + 2);
        Instance instance = {};
        applyContentMask(renderer, op.contentMask, state, instance);
        instance.rect[0] = static_cast<float>(device[0]);
        instance.rect[1] = static_cast<float>(device[1]);
        instance.rect[2] = static_cast<float>(device[2] - device[0]);
        instance.rect[3] = static_cast<float>(device[3] - device[1]);
        instance.params[0] = static_cast<float>(uv[0]);
        instance.params[1] = static_cast<float>(uv[1]);
        instance.params[2] = static_cast<float>(uv[2] - uv[0]);
        instance.params[3] = static_cast<float>(uv[3] - uv[1]);
        instance.color = op.color;
        instance.kind = imageOpKind(op, imageKind(image, op.sampling ? 10u : 9u));
        instance.opacity = 1;
        emitInstance(state, PipelineRGBA, image.single.texture, instance);
        return;
    }
    emitImageRegion(state, op, image.single.texture, scissor, frame, region, uv, imageKind(image, (op.sampling ? 10u : 9u) | (0xfu << 8)));
}

static void expandImage(ExpandState& state, const ImageOp& op, FrameStats& stats) {
    MetalRenderer* renderer = state.renderer;
    const double* matrix = state.matrix;
    bool exact = matrix[1] == 0 && matrix[2] == 0 && matrix[0] > 0 && matrix[3] > 0;
    if (op.pattern) {
        auto texture = renderer->imageTextures.find(op.identity);
        if (texture == renderer->imageTextures.end() || texture->second.tiled) {
            ++stats.skippedOps;
            return;
        }
        expandImagePattern(state, op, texture->second);
        return;
    }
    if (op.canvas) {
        auto texture = renderer->imageTextures.find(op.identity);
        if (texture != renderer->imageTextures.end()) {
            if (texture->second.tiled) {
                expandImageTiles(state, op, texture->second);
            } else {
                expandImageTexture(state, op, texture->second);
            }
            return;
        }
    }
    if (!exact) {
        if (!expandImagePaint(state, op)) {
            ++stats.skippedOps;
        }
        return;
    }
    int32_t width = static_cast<int32_t>(llround(op.width * matrix[0] * renderer->scale));
    int32_t height = static_cast<int32_t>(llround(op.height * matrix[3] * renderer->scale));
    uint64_t key = op.canvas
        ? canvasImageKey(
            static_cast<uint64_t>(op.index) | (static_cast<uint64_t>(op.generation) << 32),
            width,
            height,
            op.templateAlpha != 0
        )
        : imageKey(op.index, op.generation, width, height, op.templateAlpha != 0);
    auto found = renderer->imageEntries.find(key);
    if (found == renderer->imageEntries.end() || !found->second.valid) {
        if (!expandImagePaint(state, op)) {
            ++stats.skippedOps;
        }
        return;
    }
    AtlasEntry& entry = found->second;
    entry.lastUsedFrame = renderer->frameCount;
    renderer->imageAtlas.pages[entry.page].lastUsedFrame = renderer->frameCount;
    id<MTLTexture> texture = renderer->imageAtlas.pages[entry.page].texture;
    Instance instance = {};
    applyContentMask(renderer, op.contentMask, state, instance);
    instance.rect[0] = static_cast<float>(floor(
        (op.x * matrix[0] + matrix[4]) * renderer->scale
    ));
    instance.rect[1] = static_cast<float>(floor(
        (op.y * matrix[3] + matrix[5]) * renderer->scale
    ));
    instance.rect[2] = entry.width;
    instance.rect[3] = entry.height;
    instance.params[0] = entry.x / static_cast<float>(kAtlasSize);
    instance.params[1] = entry.y / static_cast<float>(kAtlasSize);
    instance.params[2] = entry.width / static_cast<float>(kAtlasSize);
    instance.params[3] = entry.height / static_cast<float>(kAtlasSize);
    instance.color = op.color;
    instance.kind = op.templateAlpha ? 2 : 3;
    instance.opacity = 1;
    emitInstance(
        state,
        op.templateAlpha ? PipelineMask : PipelineRGBA,
        texture,
        instance
    );
}

static void fillMultiply(const double* left, const double* right, double* out) {
    double result[6] = {
        left[0] * right[0] + left[2] * right[1],
        left[1] * right[0] + left[3] * right[1],
        left[0] * right[2] + left[2] * right[3],
        left[1] * right[2] + left[3] * right[3],
        left[0] * right[4] + left[2] * right[5] + left[4],
        left[1] * right[4] + left[3] * right[5] + left[5],
    };
    memcpy(out, result, sizeof(result));
}

static void layerComposite(const double* parent, const double* local, double* out) {
    double determinant = parent[0] * parent[3] - parent[1] * parent[2];
    if (!(std::fabs(determinant) > 1e-12)) {
        memcpy(out, local, sizeof(double) * 6);
        return;
    }
    double inverse[6] = {
        parent[3] / determinant,
        -parent[1] / determinant,
        -parent[2] / determinant,
        parent[0] / determinant,
        (parent[2] * parent[5] - parent[3] * parent[4]) / determinant,
        (parent[1] * parent[4] - parent[0] * parent[5]) / determinant,
    };
    double step[6];
    fillMultiply(parent, local, step);
    fillMultiply(step, inverse, out);
}

static FillGeometryRecord* fillGeometryFor(
    MetalRenderer* renderer,
    uint32_t index,
    uint32_t generation
) {
    if (index >= renderer->fillGeometries.size()) {
        return nullptr;
    }
    FillGeometryRecord& record = renderer->fillGeometries[index];
    return record.resident && record.generation == generation ? &record : nullptr;
}

static bool fillDrawExtent(const FrameFillDraw& source, const MTLScissorRect& scissor, double* out, double margin = 1) {
    const double* device = source.device;
    double origin[6] = {
        device[0],
        device[1],
        device[2],
        device[3],
        device[0] * source.originX + device[2] * source.originY + device[4],
        device[1] * source.originX + device[3] * source.originY + device[5],
    };
    double bounds[4];
    transformBounds(origin, source.bounds, bounds);
    out[0] = std::max({bounds[0] - margin, static_cast<double>(scissor.x),
        static_cast<double>(source.mask[0])});
    out[1] = std::max({bounds[1] - margin, static_cast<double>(scissor.y),
        static_cast<double>(source.mask[1])});
    out[2] = std::min({bounds[2] + margin, static_cast<double>(scissor.x + scissor.width),
        static_cast<double>(source.mask[2])});
    out[3] = std::min({bounds[3] + margin, static_cast<double>(scissor.y + scissor.height),
        static_cast<double>(source.mask[3])});
    return out[0] < out[2] && out[1] < out[3];
}

static void emitFillDraw(ExpandState& state, FrameFillDraw& draw, const MTLScissorRect& scissor) {
    MetalRenderer* renderer = state.renderer;
    bool source = (draw.clip & 3u) == FillClipSource;
    draw.blend = source ? 0 : drawBlend(state);
    if (draw.blend == kBlendSkip) {
        return;
    }
    if (state.layerDepth && !source) {
        double extent[4];
        if (fillDrawExtent(draw, scissor, extent)) {
            includeLayerBounds(state, extent[0], extent[1], extent[2], extent[3]);
        }
    }
    std::vector<Batch>& batches = *state.batchSink;
    uint32_t drawIndex = static_cast<uint32_t>(renderer->fillDraws.size());
    bool extend = !batches.empty() && batches.back().pipeline == PipelineFill &&
        scissorsEqual(batches.back().scissor, scissor) &&
        batches.back().blend == (draw.blend != 0) &&
        renderer->fillBatches[batches.back().start].drawEnd == drawIndex &&
        renderer->fillBatches[batches.back().start].shader == draw.shader;
    if (!extend) {
        ensureVectorCapacity(renderer, renderer->fillBatches, renderer->fillBatches.size() + 1);
        ensureVectorCapacity(renderer, batches, batches.size() + 1);
        uint32_t created = static_cast<uint32_t>(renderer->fillBatches.size());
        renderer->fillBatches.push_back(
            FillBatchInfo{scissor, drawIndex, drawIndex, false, draw.blend != 0, draw.shader}
        );
        Batch batch = {PipelineFill, nil, scissor, created, 1, draw.blend != 0, draw.shader};
        batches.push_back(batch);
    }
    uint32_t fillBatch = batches.back().start;
    draw.batch = fillBatch;
    draw.roundedClip = state.roundedClip;
    ensureVectorCapacity(renderer, renderer->fillDraws, renderer->fillDraws.size() + 1);
    renderer->fillDraws.push_back(draw);
    renderer->fillBatches[fillBatch].drawEnd = drawIndex + 1;
    renderer->fillBatches[fillBatch].paints |= !source;
    double extent[4];
    if (state.batchSink == &renderer->batches && fillDrawExtent(draw, scissor, extent, 0)) {
        noteBatchRect(state, extent[0], extent[1], extent[2], extent[3]);
    }
}

static uint32_t appendCoverage(MetalRenderer* renderer, uint32_t draw, uint32_t parent, uint32_t mode) {
    ensureVectorCapacity(renderer, renderer->coverageClips, renderer->coverageClips.size() + 1);
    renderer->coverageClips.push_back(CoverageClipRecord{draw, parent, mode, 0});
    return static_cast<uint32_t>(renderer->coverageClips.size());
}

static uint32_t emitClipSource(ExpandState& state, FrameFillDraw& draw, const MTLScissorRect& scissor) {
    memset(draw.color, 0, sizeof(draw.color));
    draw.paint = 0;
    draw.shader = 0;
    draw.halfWidth = 0;
    draw.strokeStyle = 0;
    draw.dash = 0;
    draw.clip = FillClipSource;
    draw.coverage = 0;
    uint32_t index = static_cast<uint32_t>(state.renderer->fillDraws.size());
    emitFillDraw(state, draw, scissor);
    return index;
}

static void prepareFillDraw(
    ExpandState& state,
    const double* transform,
    const float* contentMask,
    FrameFillDraw& draw
) {
    MetalRenderer* renderer = state.renderer;
    double scale = renderer->scale;
    double base[6] = {scale, 0, 0, scale, 0, 0};
    fillMultiply(base, state.matrix, base);
    fillMultiply(base, transform, draw.device);
    if (contentMask[2] > contentMask[0] && contentMask[3] > contentMask[1]) {
        double local[4] = {contentMask[0], contentMask[1], contentMask[2], contentMask[3]};
        double device[4];
        transformBounds(base, local, device);
        for (int side = 0; side < 4; ++side) {
            draw.mask[side] = static_cast<float>(device[side]);
        }
    } else {
        draw.mask[0] = 0;
        draw.mask[1] = 0;
        draw.mask[2] = static_cast<float>(renderer->pixelWidth);
        draw.mask[3] = static_cast<float>(renderer->pixelHeight);
    }
}

static double deviceDash(const FrameFillDraw& draw, double dash) {
    double deviceScale = std::sqrt(std::fabs(
        draw.device[0] * draw.device[3] - draw.device[1] * draw.device[2]
    ));
    return dash > 0 && std::isfinite(dash) && dash * 2 * deviceScale >= 1 ? dash : 0;
}

static id<MTLLibrary> newShaderLibrary(id<MTLDevice> device, const std::string& source, const char* kind, uint32_t shader) {
    NSError* error = nil;
    id<MTLLibrary> library = [device
        newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                     options:nil
                       error:&error];
    if (!library) {
        fprintf(
            stderr,
            "brix metal: %s shader %u failed: %s\n",
            kind,
            shader,
            error.localizedDescription.UTF8String
        );
    }
    return library;
}

struct SharedShaderLibrary {
    bool done = false;
    id<MTLLibrary> library = nil;
};

struct SharedShaderLibraries {
    std::mutex mutex;
    std::condition_variable built;
    std::unordered_map<std::string, SharedShaderLibrary> entries;
};

static SharedShaderLibraries& sharedShaderLibraries() {
    static SharedShaderLibraries* libraries = new SharedShaderLibraries();
    return *libraries;
}

static id<MTLLibrary> sharedShaderLibrary(id<MTLDevice> device, const std::string& source, const char* kind, uint32_t shader) {
    SharedShaderLibraries& libraries = sharedShaderLibraries();
    SharedShaderLibrary* entry = nullptr;
    {
        std::unique_lock<std::mutex> lock(libraries.mutex);
        auto found = libraries.entries.find(source);
        if (found == libraries.entries.end()) {
            entry = &libraries.entries[source];
        } else {
            entry = &found->second;
            libraries.built.wait(lock, [entry] { return entry->done; });
            if (!entry->library || entry->library.device == device) {
                return entry->library;
            }
            lock.unlock();
            return newShaderLibrary(device, source, kind, shader);
        }
    }
    id<MTLLibrary> library = newShaderLibrary(device, source, kind, shader);
    {
        std::lock_guard<std::mutex> lock(libraries.mutex);
        entry->library = library;
        entry->done = true;
    }
    libraries.built.notify_all();
    return library;
}

static const FormatPipeline* shaderVariant(
    id<MTLDevice> device,
    ShaderPipelines& pipelines,
    MTLPixelFormat format,
    NSString* vertexName,
    NSString* fragmentName,
    bool blend
) {
    FormatPipeline* variant = nullptr;
    for (FormatPipeline& existing : pipelines.variants) {
        if (existing.format == format) {
            variant = &existing;
            break;
        }
    }
    if (!variant) {
        FormatPipeline made = {format, nil, nil};
        @autoreleasepool {
            id<MTLFunction> vertex = [pipelines.library newFunctionWithName:vertexName];
            made.render = buildRenderPipeline(
                device, pipelines.library, vertex, fragmentName, newSurfaceConstants(format, false), true,
                MTLColorWriteMaskAll, format
            );
        }
        pipelines.variants.push_back(made);
        variant = &pipelines.variants.back();
    }
    if (!variant->render) {
        return nullptr;
    }
    if (blend && !variant->blend) {
        @autoreleasepool {
            id<MTLFunction> vertex = [pipelines.library newFunctionWithName:vertexName];
            variant->blend = buildBlendDrawPipeline(
                device, pipelines.library, vertex, fragmentName, newSurfaceConstants(format, false),
                newSurfaceConstants(format, true), format
            );
        }
    }
    return blend && !variant->blend ? nullptr : variant;
}

static const FormatPipeline* fillShaderVariant(
    id<MTLDevice> device,
    ShaderPipelines& pipelines,
    MTLPixelFormat format,
    bool blend
) {
    return shaderVariant(device, pipelines, format, @"brixGuiMetalFillVertex", @"brixGuiMetalFillFragment", blend);
}

static std::string userShaderCore() {
    return std::string(commonShaderSource) + rendererColorSource() + brixGraphicsShaderInputTypes() +
        fillTypesSource + fillCoverageSource + fillDrawSource;
}

static const FormatPipeline* textShaderVariant(
    id<MTLDevice> device,
    ShaderPipelines& pipelines,
    MTLPixelFormat format,
    bool blend
) {
    return shaderVariant(device, pipelines, format, @"brixGuiMetalVertex", @"brixGuiMetalMaskFragment", blend);
}

static const FormatPipeline* dabShaderVariant(
    id<MTLDevice> device,
    ShaderPipelines& pipelines,
    MTLPixelFormat format,
    bool blend
) {
    return shaderVariant(device, pipelines, format, @"brixGuiMetalDabVertex", @"brixGuiMetalDabFragment", blend);
}

static id<MTLRenderPipelineState> filterShaderVariant(
    id<MTLDevice> device,
    ShaderPipelines& pipelines,
    MTLPixelFormat format
) {
    for (const FormatPipeline& variant : pipelines.variants) {
        if (variant.format == format) {
            return variant.render;
        }
    }
    FormatPipeline variant = {format, nil, nil};
    @autoreleasepool {
        MTLFunctionConstantValues* values = [MTLFunctionConstantValues new];
        bool extended = format != MTLPixelFormatBGRA8Unorm;
        [values setConstantValue:&extended type:MTLDataTypeBool atIndex:1];
        id<MTLFunction> vertex = [pipelines.library newFunctionWithName:@"brixFilterVertex"];
        variant.render = buildRenderPipeline(
            device,
            pipelines.library,
            vertex,
            @"brixFilterFragment",
            values,
            false,
            MTLColorWriteMaskAll,
            format,
            MTLPixelFormatInvalid
        );
    }
    pipelines.variants.push_back(variant);
    return variant.render;
}

static std::unordered_map<uint32_t, ShaderPipelines>& shaderCache(MetalRenderer* renderer, ShaderRole role) {
    switch (role) {
        case ShaderRoleText:
            return renderer->textShaders;
        case ShaderRoleDab:
            return renderer->dabShaders;
        case ShaderRoleFilter:
            return renderer->filterShaders;
        case ShaderRoleFill:
            break;
    }
    return renderer->fillShaders;
}

static const char* shaderRoleName(ShaderRole role) {
    switch (role) {
        case ShaderRoleText:
            return "text";
        case ShaderRoleDab:
            return "brush";
        case ShaderRoleFilter:
            return "filter";
        case ShaderRoleFill:
            break;
    }
    return "fill";
}

static std::string shaderLibrarySource(ShaderRole role, const char* user) {
    if (!user) {
        return std::string();
    }
    bool painting = std::strstr(user, "paint(PaintInput") != nullptr;
    switch (role) {
        case ShaderRoleText:
            if (painting) {
                return std::string();
            }
            return std::string("#define BRIX_USER_SHADER 1\n") + userShaderCore() + instanceFragmentSource +
                brixGraphicsShaderTypes() + "\n#line 1\n" + user;
        case ShaderRoleDab:
            return std::string("#define BRIX_USER_SHADER 1\n#define BRIX_USER_DAB 1\n") + userShaderCore() +
                brixGraphicsShaderDabSource() + dabShaderSource + brixGraphicsShaderTypes() + "\n#line 1\n" + user;
        case ShaderRoleFilter:
            return std::string(filterShaderSource) + rendererColorSource() + brixGraphicsShaderTypes() +
                brixGraphicsShaderArgsType() + filterPassSource + "\n#line 1\n" + user;
        case ShaderRoleFill:
            break;
    }
    return std::string("#define BRIX_USER_SHADER 1\n") + (painting ? "#define BRIX_USER_PAINT 1\n" : "") +
        userShaderCore() + brixGraphicsShaderTypes() + "\n#line 1\n" + user;
}

static MTLPixelFormat shaderFormat(MetalRenderer* renderer, ShaderRole role) {
    return role == ShaderRoleFilter ? renderer->layerFormat : renderer->outputFormat;
}

static void buildShaderPipelines(
    id<MTLDevice> device,
    MTLPixelFormat format,
    ShaderRole role,
    uint32_t shader,
    const std::string& source,
    ShaderPipelines& pipelines
) {
    pipelines.failed = true;
    if (source.empty()) {
        return;
    }
    pipelines.library = sharedShaderLibrary(device, source, shaderRoleName(role), shader);
    if (!pipelines.library) {
        return;
    }
    switch (role) {
        case ShaderRoleFill:
            pipelines.failed = !fillShaderVariant(device, pipelines, format, false);
            break;
        case ShaderRoleText:
            pipelines.failed = !textShaderVariant(device, pipelines, format, false);
            break;
        case ShaderRoleDab:
            pipelines.failed = !dabShaderVariant(device, pipelines, format, false);
            break;
        case ShaderRoleFilter:
            pipelines.failed = !filterShaderVariant(device, pipelines, format);
            break;
    }
}

static ShaderJob shaderJob(ShaderRole role, uint32_t shader, MTLPixelFormat format) {
    return {
        role,
        shader,
        format,
        shaderLibrarySource(role, brixGraphicsShaderSource(shader)),
        ShaderPipelines(),
    };
}

static bool warmShaderRole(const char* source, ShaderRole& role) {
    if (std::strstr(source, "paint(PaintInput") || std::strstr(source, "shade(float2 position, float2 size")) {
        role = ShaderRoleFill;
        return true;
    }
    if (std::strstr(source, "dab(DabInput")) {
        role = ShaderRoleDab;
        return true;
    }
    if (std::strstr(source, "ShaderLayer layer")) {
        role = ShaderRoleFilter;
        return true;
    }
    return false;
}

static void warmUserShader(uint32_t shader, const char* source) {
    ShaderRole role = ShaderRoleFill;
    if (!source || !warmShaderRole(source, role)) {
        return;
    }
    std::string composed = shaderLibrarySource(role, source);
    if (composed.empty()) {
        return;
    }
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        @autoreleasepool {
            id<MTLDevice> device = (__bridge id<MTLDevice>)brixGraphicsMetalDevice();
            if (device) {
                sharedShaderLibrary(device, composed, shaderRoleName(role), shader);
            }
        }
    });
}

static void startShaderJobs(MetalRenderer* renderer, std::vector<ShaderJob>& jobs) {
    id<MTLDevice> device = renderer->device;
    if (renderer->presentationLayer) {
        std::shared_ptr<ShaderCompiles> compiles = renderer->shaderCompiles;
        compiles->pending.fetch_add(static_cast<int64_t>(jobs.size()));
        dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);
        for (ShaderJob& job : jobs) {
            shaderCache(renderer, job.role)[job.shader].pending = true;
            std::shared_ptr<ShaderJob> work = std::make_shared<ShaderJob>(std::move(job));
            dispatch_async(queue, ^{
                @autoreleasepool {
                    buildShaderPipelines(device, work->format, work->role, work->shader, work->source, work->pipelines);
                }
                {
                    std::lock_guard<std::mutex> lock(compiles->mutex);
                    compiles->finished.push_back(std::move(*work));
                    compiles->landed.store(true);
                }
                compiles->pending.fetch_sub(1);
            });
        }
        return;
    }
    ShaderJob* work = jobs.data();
    dispatch_apply(jobs.size(), DISPATCH_APPLY_AUTO, ^(size_t index) {
        @autoreleasepool {
            ShaderJob& job = work[index];
            buildShaderPipelines(device, job.format, job.role, job.shader, job.source, job.pipelines);
        }
    });
    for (ShaderJob& job : jobs) {
        shaderCache(renderer, job.role)[job.shader] = std::move(job.pipelines);
    }
}

static void installCompiledShaders(MetalRenderer* renderer) {
    std::vector<ShaderJob> finished;
    {
        std::lock_guard<std::mutex> lock(renderer->shaderCompiles->mutex);
        finished.swap(renderer->shaderCompiles->finished);
    }
    for (ShaderJob& job : finished) {
        shaderCache(renderer, job.role)[job.shader] = std::move(job.pipelines);
    }
}

static ShaderPipelines* ensureShader(MetalRenderer* renderer, ShaderRole role, uint32_t shader, MTLPixelFormat format) {
    std::unordered_map<uint32_t, ShaderPipelines>& cache = shaderCache(renderer, role);
    auto found = cache.find(shader);
    if (found == cache.end()) {
        std::vector<ShaderJob> jobs = {shaderJob(role, shader, format)};
        startShaderJobs(renderer, jobs);
        found = cache.find(shader);
    }
    if (found->second.pending) {
        renderer->skippedPendingShader = true;
        return nullptr;
    }
    return found->second.failed ? nullptr : &found->second;
}

static ShaderPipelines* ensureShader(MetalRenderer* renderer, ShaderRole role, uint32_t shader) {
    return ensureShader(renderer, role, shader, shaderFormat(renderer, role));
}

static void requestShader(MetalRenderer* renderer, ShaderRole role, uint32_t shader, MTLPixelFormat format) {
    if (shader && !shaderCache(renderer, role).count(shader)) {
        renderer->shaderRequests.push_back({role, shader, format});
    }
}

static void requestShader(MetalRenderer* renderer, ShaderRole role, uint32_t shader) {
    requestShader(renderer, role, shader, shaderFormat(renderer, role));
}

static void compileRequestedShaders(MetalRenderer* renderer) {
    installCompiledShaders(renderer);
    std::vector<ShaderJob> jobs;
    for (const ShaderRequest& request : renderer->shaderRequests) {
        if (shaderCache(renderer, request.role).count(request.shader)) {
            continue;
        }
        bool queued = false;
        for (const ShaderJob& job : jobs) {
            queued = queued || (job.role == request.role && job.shader == request.shader);
        }
        if (!queued) {
            jobs.push_back(shaderJob(request.role, request.shader, request.format));
        }
    }
    renderer->shaderRequests.clear();
    if (!jobs.empty()) {
        startShaderJobs(renderer, jobs);
    }
}

static void releaseShaderPipelines(ShaderPipelines& pipelines) {
    pipelines.variants.clear();
    pipelines.library = nil;
}

static uint32_t appendShaderPaint(
    MetalRenderer* renderer,
    const ChunkPathSet& set,
    const BxGuiMetalGradient& gradient,
    FillPaintGPU& paint
) {
    if (gradient.kind == 5) {
        paint.kind = 7;
        paint.p0 = static_cast<float>(gradient.start[0]);
        paint.p1 = static_cast<float>(gradient.start[1]);
        paint.p2 = static_cast<float>(gradient.end[0]);
        paint.p3 = static_cast<float>(gradient.end[1]);
        float paper = renderer->scale > 0 ? static_cast<float>(1.0 / renderer->scale) : 1.0f;
        std::memcpy(&paint.padding, &paper, sizeof(paper));
    } else {
        paint.kind = 6;
        paint.p0 = static_cast<float>(gradient.end[0]);
        paint.p1 = static_cast<float>(gradient.end[1]);
        paint.p2 = gradient.endRadius >= 0
            ? static_cast<float>(std::max(0.0, renderer->fillTime - gradient.endRadius))
            : 0.0f;
    }
    paint.stopStart = static_cast<uint32_t>(renderer->fillStops.size());
    paint.stopCount = gradient.stopCount;
    uint32_t groups = (gradient.stopCount + 3) / 4;
    ensureVectorCapacity(renderer, renderer->fillStops, renderer->fillStops.size() + groups);
    for (uint32_t group = 0; group < groups; ++group) {
        FillStopGPU stop = {};
        for (uint32_t lane = 0; lane < 4 && group * 4 + lane < gradient.stopCount; ++lane) {
            stop.color[lane] = set.shaderArgs[gradient.stopStart + group * 4 + lane];
        }
        renderer->fillStops.push_back(stop);
    }
    ensureVectorCapacity(renderer, renderer->fillPaints, renderer->fillPaints.size() + 1);
    renderer->fillPaints.push_back(paint);
    return static_cast<uint32_t>(renderer->fillPaints.size()) | FillPaintTranslucent;
}

static uint32_t appendGradientPaint(
    MetalRenderer* renderer,
    const ChunkPathSet& set,
    uint32_t index,
    const double* device,
    const double* paper,
    float alpha,
    uint32_t& shader,
    bool text
) {
    shader = 0;
    if (!index || index > set.gradients.size()) {
        return 0;
    }
    const BxGuiMetalGradient& gradient = set.gradients[index - 1];
    bool custom = gradient.kind == 4 || gradient.kind == 5;
    bool invalid = custom
        ? gradient.stopStart > set.shaderArgs.size() ||
            gradient.stopCount > set.shaderArgs.size() - gradient.stopStart ||
            !ensureShader(renderer, text ? ShaderRoleText : ShaderRoleFill, gradient.shader)
        : !gradient.stopCount || gradient.stopStart > set.stops.size() ||
            gradient.stopCount > set.stops.size() - gradient.stopStart;
    if (invalid) {
        return 0;
    }
    double determinant = device[0] * device[3] - device[1] * device[2];
    if (!(std::fabs(determinant) > 0) || !std::isfinite(determinant)) {
        return 0;
    }
    double inverse[6] = {
        device[3] / determinant,
        -device[1] / determinant,
        -device[2] / determinant,
        device[0] / determinant,
        (device[2] * device[5] - device[3] * device[4]) / determinant,
        (device[1] * device[4] - device[0] * device[5]) / determinant,
    };
    if (gradient.kind != 5) {
        inverse[4] -= gradient.start[0];
        inverse[5] -= gradient.start[1];
    }
    FillPaintGPU paint = {};
    paint.a = static_cast<float>(inverse[0]);
    paint.b = static_cast<float>(inverse[1]);
    paint.c = static_cast<float>(inverse[2]);
    paint.d = static_cast<float>(inverse[3]);
    paint.tx = static_cast<float>(inverse[4]);
    paint.ty = static_cast<float>(inverse[5]);
    paint.paperX = static_cast<float>(paper[0]);
    paint.paperY = static_cast<float>(paper[1]);
    paint.kind = gradient.kind;
    if (custom) {
        shader = gradient.shader;
        return appendShaderPaint(renderer, set, gradient, paint);
    }
    bool translucent = alpha < 1;
    if (gradient.kind == 1) {
        double dx = gradient.end[0] - gradient.start[0];
        double dy = gradient.end[1] - gradient.start[1];
        double length = dx * dx + dy * dy;
        if (!(length > 0)) {
            return 0;
        }
        paint.p0 = static_cast<float>(dx / length);
        paint.p1 = static_cast<float>(dy / length);
    } else if (gradient.kind == 2) {
        paint.p0 = static_cast<float>(gradient.end[0] - gradient.start[0]);
        paint.p1 = static_cast<float>(gradient.end[1] - gradient.start[1]);
        paint.p2 = static_cast<float>(gradient.startRadius);
        paint.p3 = static_cast<float>(gradient.endRadius - gradient.startRadius);
        translucent = true;
    } else if (gradient.kind == 3) {
        paint.p0 = static_cast<float>(gradient.startRadius);
    } else {
        return 0;
    }
    paint.stopStart = static_cast<uint32_t>(renderer->fillStops.size());
    paint.stopCount = gradient.stopCount;
    ensureVectorCapacity(renderer, renderer->fillStops, renderer->fillStops.size() + gradient.stopCount);
    for (uint32_t stop = 0; stop < gradient.stopCount; ++stop) {
        const FillStopGPU& source = set.stops[gradient.stopStart + stop];
        translucent = translucent || source.color[3] < 1;
        renderer->fillStops.push_back(source);
    }
    ensureVectorCapacity(renderer, renderer->fillPaints, renderer->fillPaints.size() + 1);
    renderer->fillPaints.push_back(paint);
    uint32_t record = static_cast<uint32_t>(renderer->fillPaints.size());
    return translucent ? record | FillPaintTranslucent : record;
}

static const double kTinyPathPixels = 3.0;
static const double kTinyOutlinePixels = 2.0;

static bool tinyDeviceBounds(const FrameFillDraw& draw, double limit, double* bounds) {
    const double* device = draw.device;
    if (device[1] != 0 || device[2] != 0 ||
            !((draw.bounds[2] - draw.bounds[0]) * std::fabs(device[0]) <= limit) ||
            !((draw.bounds[3] - draw.bounds[1]) * std::fabs(device[3]) <= limit)) {
        return false;
    }
    double origin[6] = {
        device[0],
        0,
        0,
        device[3],
        device[0] * draw.originX + device[4],
        device[3] * draw.originY + device[5],
    };
    transformBounds(origin, draw.bounds, bounds);
    return true;
}

static void emitCoverageQuad(
    ExpandState& state,
    const FrameFillDraw& draw,
    const double* bounds,
    double area,
    double radius
) {
    double quadWidth = std::max(bounds[2] - bounds[0], 1.0);
    double quadHeight = std::max(bounds[3] - bounds[1], 1.0);
    double corner = std::min({radius, quadWidth * 0.5, quadHeight * 0.5});
    double ink = quadWidth * quadHeight - (4 - M_PI) * corner * corner + kQuadCornerInk;
    double coverage = area * std::fabs(draw.device[0] * draw.device[3]) / ink;
    Instance instance = {};
    instance.params[0] = static_cast<float>(corner);
    memcpy(instance.contentMask, draw.mask, sizeof(instance.contentMask));
    instance.rect[0] = static_cast<float>((bounds[0] + bounds[2] - quadWidth) * 0.5);
    instance.rect[1] = static_cast<float>((bounds[1] + bounds[3] - quadHeight) * 0.5);
    instance.rect[2] = static_cast<float>(quadWidth);
    instance.rect[3] = static_cast<float>(quadHeight);
    instance.color = packColor(draw.color[0], draw.color[1], draw.color[2], draw.color[3]);
    instance.kind = 0;
    instance.opacity = static_cast<float>(std::min(1.0, coverage));
    emitInstance(state, PipelineTier1, nil, instance);
}

static bool emitTinyFill(ExpandState& state, const FrameFillDraw& draw, const FillPathRecord& path) {
    double bounds[4];
    if (!(path.area > 0) || draw.paint || draw.shader || draw.fillRule ||
            !tinyDeviceBounds(draw, kTinyPathPixels, bounds)) {
        return false;
    }
    emitCoverageQuad(state, draw, bounds, path.area, 0);
    return true;
}

static bool emitTinyStroke(ExpandState& state, const FrameFillDraw& draw, const FillPathRecord& path, double half) {
    double bounds[4];
    bool blob = path.width <= half * 2 && path.height <= half * 2;
    if (draw.dash > 0 || !(path.length > 0) ||
            !tinyDeviceBounds(draw, blob ? kTinyPathPixels : kTinyOutlinePixels, bounds)) {
        return false;
    }
    const FillSegmentGPU* segments = state.renderer->fillSegments.data() + path.segmentStart;
    uint32_t subpaths = 0;
    for (uint32_t index = 0; index < path.segmentCount; ++index) {
        subpaths += (segments[index].kind & FillSegmentStart) ? 1u : 0u;
    }
    double box = (path.width + half * 2) * (path.height + half * 2) - (4 - M_PI) * half * half;
    double sweep = path.length * half * 2 + subpaths * M_PI * half * half;
    double radius = half * std::min(std::fabs(draw.device[0]), std::fabs(draw.device[3]));
    emitCoverageQuad(state, draw, bounds, std::min(box, sweep), radius);
    return true;
}

static void emitSpanDraws(
    ExpandState& state,
    FrameFillDraw& draw,
    const FillGeometryRecord& geometry,
    const FillSpanRecord& span,
    const ChunkPathSet* paints,
    const uint8_t* paletteBytes,
    uint32_t explicitCount,
    int64_t inheritedFill,
    int64_t inheritedStroke,
    double defaultWidth,
    uint32_t defaultStyle,
    double dash,
    const MTLScissorRect& scissor
) {
    if (span.path >= geometry.paths.size()) {
        return;
    }
    const FillPathRecord& path = geometry.paths[span.path];
    if (!path.segmentCount) {
        return;
    }
    draw.segmentStart = path.segmentStart;
    draw.segmentCount = path.segmentCount;
    draw.originX = path.originX;
    draw.originY = path.originY;
    draw.clip = FillClipNone;
    draw.coverage = state.coverageClip;
    draw.dash = 0;
    draw.paint = 0;
    draw.shader = 0;
    int64_t fillColor = -1;
    if (span.role == 3) {
        fillColor = span.paletteIndex < explicitCount ? static_cast<int64_t>(span.paletteIndex) : -1;
    } else if (span.role == 0) {
        fillColor = inheritedFill;
    } else if (span.role == 1) {
        fillColor = inheritedStroke;
    }
    if (fillColor >= 0) {
        memcpy(draw.color, paletteBytes + fillColor * sizeof(draw.color), sizeof(draw.color));
        if (span.paint && draw.color[3] > 0) {
            draw.paint = paints
                ? appendGradientPaint(state.renderer, *paints, span.paint, draw.device, state.paper, draw.color[3], draw.shader)
                : 0;
            if (!draw.paint) {
                draw.color[3] = 0;
            }
        }
        if (draw.color[3] > 0) {
            draw.bounds[0] = 0;
            draw.bounds[1] = 0;
            draw.bounds[2] = path.width;
            draw.bounds[3] = path.height;
            draw.fillRule = span.fillRule;
            draw.halfWidth = 0;
            draw.strokeStyle = 0;
            if (span.brush || !emitTinyFill(state, draw, path)) {
                emitFillDraw(state, draw, scissor);
            }
        }
    }
    int64_t strokeColor = -1;
    if (span.strokeRole == 0) {
        strokeColor = inheritedStroke;
    } else if (span.strokeRole != 2) {
        strokeColor = span.strokePaletteIndex < explicitCount
            ? static_cast<int64_t>(span.strokePaletteIndex)
            : -1;
    }
    if (strokeColor < 0) {
        return;
    }
    draw.paint = 0;
    draw.shader = 0;
    memcpy(draw.color, paletteBytes + strokeColor * sizeof(draw.color), sizeof(draw.color));
    if (draw.color[3] <= 0) {
        return;
    }
    double width = span.strokeWidth >= 0 ? span.strokeWidth : defaultWidth;
    if (!(width > 0) || !std::isfinite(width)) {
        return;
    }
    uint32_t cap = span.strokeCap != FillStrokeInherit ? span.strokeCap : (defaultStyle & 15u);
    uint32_t join = span.strokeJoin != FillStrokeInherit
        ? span.strokeJoin
        : ((defaultStyle >> 4) & 15u);
    double half = span.strokeAlignment == FillAlignCenter ? width * 0.5 : width;
    double outset = half * std::max(
        cap == FillStrokeCapSquare ? M_SQRT2 : 1.0,
        join == FillStrokeJoinMiter ? path.miterScale : 1.0
    );
    draw.bounds[0] = -outset;
    draw.bounds[1] = -outset;
    draw.bounds[2] = path.width + outset;
    draw.bounds[3] = path.height + outset;
    if (span.strokeAlignment != FillAlignCenter) {
        bool inside = span.strokeAlignment == FillAlignInside;
        uint32_t rule = inside ? span.fillRule : 1u;
        FrameFillDraw source = draw;
        source.fillRule = rule;
        uint32_t sourceIndex = emitClipSource(state, source, scissor);
        draw.coverage = appendCoverage(
            state.renderer,
            sourceIndex,
            state.coverageClip,
            (inside ? FillClipInside : FillClipOutside) | (rule ? FillClipEvenOdd : 0u)
        );
    }
    draw.fillRule = 0;
    draw.halfWidth = static_cast<float>(half);
    draw.strokeStyle = cap | (join << 4);
    if (dash > 0 && path.length <= dash * 2 * fillDashLimit) {
        draw.dash = static_cast<float>(dash);
    } else if (dash > 0) {
        double extension = cap == FillStrokeCapButt ? 0 : half * 2;
        draw.color[3] *= static_cast<float>(std::min(1.0, (dash + extension) / (dash * 2)));
    }
    bool eligible = !span.brush && span.strokeAlignment == FillAlignCenter && cap == FillStrokeCapRound &&
        join == FillStrokeJoinRound;
    if (!eligible || !emitTinyStroke(state, draw, path, half)) {
        emitFillDraw(state, draw, scissor);
    }
}

static void emitBrushDraw(
    ExpandState& state,
    const ChunkPathSet& set,
    const FillSpanRecord& span,
    const FrameFillDraw& fill,
    const MTLScissorRect& scissor
) {
    MetalRenderer* renderer = state.renderer;
    if (!span.dabs || span.dabs->dabs.empty() || !span.brush || span.brush > set.gradients.size() ||
            span.path >= set.geometry.paths.size()) {
        return;
    }
    const BxGuiMetalGradient& brush = set.gradients[span.brush - 1];
    if (brush.stopCount < kBrushParams || brush.stopStart > set.shaderArgs.size() ||
            brush.stopCount > set.shaderArgs.size() - brush.stopStart) {
        return;
    }
    const float* params = set.shaderArgs.data() + brush.stopStart;
    float reveal = params[10];
    if (!(brush.end[1] > 0) || !(reveal > 0) || !(span.strokeWidth > 0)) {
        return;
    }
    uint32_t blend = drawBlend(state);
    if (blend == kBlendSkip || !ensureShader(renderer, ShaderRoleDab, brush.shader)) {
        return;
    }
    const BxGraphicsDab* dabs = span.dabs->dabs.data();
    uint32_t count = static_cast<uint32_t>(span.dabs->dabs.size());
    if (reveal < 1) {
        float limit = static_cast<float>(reveal * span.dabs->total);
        count = static_cast<uint32_t>(std::upper_bound(
            dabs,
            dabs + count,
            limit,
            [](float value, const BxGraphicsDab& dab) { return value < dab.along; }
        ) - dabs);
        if (!count) {
            return;
        }
    }
    double width = span.strokeWidth;
    const FillPathRecord& path = set.geometry.paths[span.path];
    double outset = width * (0.5 * (1 + std::fabs(params[2])) + 1.2 * std::fabs(params[1])) + 1;
    double local[4] = {
        path.originX - outset,
        path.originY - outset,
        path.originX + path.width + outset,
        path.originY + path.height + outset,
    };
    double bounds[4];
    transformBounds(fill.device, local, bounds);
    double extent[4] = {
        std::max({bounds[0], static_cast<double>(scissor.x), static_cast<double>(fill.mask[0])}),
        std::max({bounds[1], static_cast<double>(scissor.y), static_cast<double>(fill.mask[1])}),
        std::min({bounds[2], static_cast<double>(scissor.x + scissor.width), static_cast<double>(fill.mask[2])}),
        std::min({bounds[3], static_cast<double>(scissor.y + scissor.height), static_cast<double>(fill.mask[3])}),
    };
    if (!(extent[0] < extent[2]) || !(extent[1] < extent[3])) {
        return;
    }
    if (state.layerDepth) {
        includeLayerBounds(state, extent[0], extent[1], extent[2], extent[3]);
    }
    DabDrawGPU draw = {};
    draw.color[0] = static_cast<float>(brush.start[0]);
    draw.color[1] = static_cast<float>(brush.start[1]);
    draw.color[2] = static_cast<float>(brush.end[0]);
    draw.color[3] = static_cast<float>(brush.end[1]);
    memcpy(draw.mask, fill.mask, sizeof(draw.mask));
    draw.a = static_cast<float>(fill.device[0]);
    draw.b = static_cast<float>(fill.device[1]);
    draw.c = static_cast<float>(fill.device[2]);
    draw.d = static_cast<float>(fill.device[3]);
    draw.tx = static_cast<float>(fill.device[4]);
    draw.ty = static_cast<float>(fill.device[5]);
    draw.style.width = static_cast<float>(width);
    draw.style.scatter = params[1];
    draw.style.sizeJitter = params[2];
    draw.style.angleJitter = params[3];
    draw.style.roundness = params[4];
    draw.style.angle = params[5];
    draw.style.fixedAngle = params[6] != 0 ? 1 : 0;
    draw.style.flow = params[7];
    draw.style.taperStart = params[8];
    draw.style.taperEnd = params[9];
    draw.style.seed = params[11];
    draw.style.total = static_cast<float>(span.dabs->total);
    draw.paper = renderer->scale > 0 ? static_cast<float>(1.0 / renderer->scale) : 1.0f;
    draw.paperX = static_cast<float>(state.paper[0]);
    draw.paperY = static_cast<float>(state.paper[1]);
    draw.roundedClip = state.roundedClip;
    draw.coverage = state.coverageClip;
    draw.blend = blend;
    draw.argStart = static_cast<uint32_t>(renderer->fillStops.size());
    draw.argCount = brush.stopCount - kBrushParams;
    uint32_t groups = (draw.argCount + 3) / 4;
    ensureVectorCapacity(renderer, renderer->fillStops, renderer->fillStops.size() + groups);
    for (uint32_t group = 0; group < groups; ++group) {
        FillStopGPU stop = {};
        for (uint32_t lane = 0; lane < 4 && group * 4 + lane < draw.argCount; ++lane) {
            stop.color[lane] = params[kBrushParams + group * 4 + lane];
        }
        renderer->fillStops.push_back(stop);
    }
    uint32_t drawIndex = static_cast<uint32_t>(renderer->brushDraws.size());
    ensureVectorCapacity(renderer, renderer->brushDraws, renderer->brushDraws.size() + 1);
    renderer->brushDraws.push_back(draw);
    uint32_t first = static_cast<uint32_t>(renderer->brushDabs.size());
    ensureVectorCapacity(renderer, renderer->brushDabs, renderer->brushDabs.size() + count);
    renderer->brushDabs.resize(first + count);
    DabGPU* written = renderer->brushDabs.data() + first;
    for (uint32_t index = 0; index < count; ++index) {
        const BxGraphicsDab& dab = dabs[index];
        written[index] = DabGPU{dab.x, dab.y, dab.angle, dab.distance, dab.length, dab.along, drawIndex, index};
    }
    std::vector<Batch>& batches = *state.batchSink;
    if (!batches.empty() && batches.back().pipeline == PipelineDab &&
            batches.back().shader == brush.shader &&
            batches.back().blend == (blend != 0) &&
            scissorsEqual(batches.back().scissor, scissor) &&
            batches.back().start + batches.back().count == first) {
        batches.back().count += count;
        return;
    }
    ensureVectorCapacity(renderer, batches, batches.size() + 1);
    batches.push_back(Batch{PipelineDab, nil, scissor, first, count, blend != 0, brush.shader});
}

static const FillPathRecord& ensureUnitRect(MetalRenderer* renderer) {
    FillGeometryRecord& geometry = renderer->unitRect;
    if (!geometry.resident) {
        const float corners[5][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 0}};
        FillPathRecord path;
        path.segmentStart = static_cast<uint32_t>(renderer->fillSegments.size());
        path.segmentCount = 4;
        path.width = 1;
        path.height = 1;
        path.length = 4;
        ensureVectorCapacity(renderer, renderer->fillSegments, renderer->fillSegments.size() + 4);
        for (uint32_t side = 0; side < 4; ++side) {
            FillSegmentGPU segment = {};
            segment.p[0] = corners[side][0];
            segment.p[1] = corners[side][1];
            segment.p[6] = corners[side + 1][0];
            segment.p[7] = corners[side + 1][1];
            segment.arcStart = static_cast<float>(side);
            segment.arcEnd = static_cast<float>(side + 1);
            renderer->fillSegments.push_back(segment);
        }
        renderer->fillSegments[path.segmentStart].kind = FillSegmentStart | FillSegmentClosed;
        renderer->fillSegments[path.segmentStart + 3].kind = FillSegmentEnd | FillSegmentClosed;
        renderer->fillSegments[path.segmentStart + 3].link = 3;
        geometry.paths.assign(1, path);
        geometry.resident = true;
    }
    return geometry.paths[0];
}

static void pushPathCoverage(
    ExpandState& state,
    const FillPathRecord& path,
    const double* transform,
    uint32_t fillRule,
    const MTLScissorRect& scissor
) {
    const float none[4] = {0, 0, 0, 0};
    FrameFillDraw draw = {};
    prepareFillDraw(state, transform, none, draw);
    draw.segmentStart = path.segmentStart;
    draw.segmentCount = path.segmentCount;
    draw.originX = path.originX;
    draw.originY = path.originY;
    draw.bounds[2] = path.width;
    draw.bounds[3] = path.height;
    draw.fillRule = fillRule;
    uint32_t source = emitClipSource(state, draw, scissor);
    state.coverageClip = appendCoverage(
        state.renderer,
        source,
        state.coverageClip,
        FillClipInside | (fillRule ? FillClipEvenOdd : 0u)
    );
}

static void pushRectCoverage(ExpandState& state, const double* rect) {
    MTLScissorRect scissor = clipToScissor(state.renderer, state.clip);
    if (!scissor.width || !scissor.height) {
        return;
    }
    double transform[6] = {rect[2] - rect[0], 0, 0, rect[3] - rect[1], rect[0], rect[1]};
    pushPathCoverage(state, ensureUnitRect(state.renderer), transform, 0, scissor);
}

static bool matrixRotates(const double* matrix) {
    return matrix[1] != 0 || matrix[2] != 0;
}

static void pushMaskCoverage(ExpandState& state, const float* contentMask) {
    if (!matrixRotates(state.matrix) ||
        !(contentMask[2] > contentMask[0] && contentMask[3] > contentMask[1])) {
        return;
    }
    double rect[4] = {contentMask[0], contentMask[1], contentMask[2], contentMask[3]};
    pushRectCoverage(state, rect);
}

static void expandFill(
    ExpandState& state,
    const FillOp& op,
    const uint8_t* paletteBytes,
    FrameStats& stats
) {
    MetalRenderer* renderer = state.renderer;
    FillGeometryRecord* geometry = fillGeometryFor(renderer, op.index, op.generation);
    if (!geometry) {
        ++stats.skippedOps;
        return;
    }
    MTLScissorRect scissor = clipToScissor(renderer, state.clip);
    if (!scissor.width || !scissor.height) {
        return;
    }
    uint32_t coverage = state.coverageClip;
    pushMaskCoverage(state, op.contentMask);
    FrameFillDraw draw = {};
    prepareFillDraw(state, op.transform, op.contentMask, draw);
    bool inheritsFill = (op.flags & 8u) != 0;
    bool inheritsStroke = (op.flags & 16u) != 0;
    uint32_t inherited = (inheritsFill ? 1u : 0u) + (inheritsStroke ? 1u : 0u);
    uint32_t explicitCount = op.paletteCount >= inherited ? op.paletteCount - inherited : 0;
    int64_t inheritedFill = inheritsFill ? static_cast<int64_t>(explicitCount) : -1;
    int64_t inheritedStroke = inheritsStroke
        ? static_cast<int64_t>(explicitCount) + (inheritsFill ? 1 : 0)
        : -1;
    double dash = deviceDash(draw, op.dash);
    for (const FillSpanRecord& span : geometry->spans) {
        emitSpanDraws(
            state,
            draw,
            *geometry,
            span,
            nullptr,
            paletteBytes,
            explicitCount,
            inheritedFill,
            inheritedStroke,
            op.strokeWidth,
            op.strokeStyle,
            dash,
            scissor
        );
    }
    state.coverageClip = coverage;
}

static bool placeShelf(
    MetalRenderer* renderer,
    uint32_t* shelf,
    uint32_t width,
    uint32_t height,
    uint32_t* origin
) {
    uint32_t limitX = static_cast<uint32_t>(std::max(renderer->pixelWidth, 0));
    uint32_t limitY = static_cast<uint32_t>(std::max(renderer->pixelHeight, 0));
    if (shelf[0] + width > limitX) {
        shelf[0] = 0;
        shelf[1] += shelf[2];
        shelf[2] = 0;
    }
    if (width > limitX || shelf[1] + height > limitY) {
        return false;
    }
    origin[0] = shelf[0];
    origin[1] = shelf[1];
    shelf[0] += (width + 15u) & ~15u;
    shelf[2] = std::max(shelf[2], (height + 15u) & ~15u);
    return true;
}

static int32_t placeShadowResult(MetalRenderer* renderer, uint32_t width, uint32_t height, uint32_t* origin) {
    for (uint32_t pass = 0; pass < 2; ++pass) {
        for (uint32_t layer = 0; layer < kShadowLayers; ++layer) {
            uint32_t page = layer / 4;
            if (renderer->maskPageModes[page] != (pass == 0 ? 2 : 0)) {
                continue;
            }
            if (!placeShelf(renderer, renderer->shadowPageShelves[layer], width + 2, height + 2, origin)) {
                continue;
            }
            renderer->maskPageModes[page] = 2;
            ensureVectorCapacity(renderer, renderer->maskPlaced[page], renderer->maskPlaced[page].size() + 1);
            renderer->maskPlaced[page].push_back({origin[0], origin[1], origin[0] + width, origin[1] + height});
            return static_cast<int32_t>(layer);
        }
    }
    return -1;
}

static bool placeShadowSource(MetalRenderer* renderer, uint32_t width, uint32_t height, uint32_t* origin) {
    if (!renderer->shadowGenerations ||
            !placeShelf(renderer, renderer->shadowShelf, width, height, origin)) {
        memset(renderer->shadowShelf, 0, sizeof(renderer->shadowShelf));
        if (!placeShelf(renderer, renderer->shadowShelf, width, height, origin)) {
            return false;
        }
        ++renderer->shadowGenerations;
        if (renderer->shadowBatches.size() < renderer->shadowGenerations) {
            renderer->shadowBatches.emplace_back();
        }
    }
    return true;
}

static bool expandShadowPiece(
    ExpandState& state,
    const FrameFillDraw& draw,
    const double* shift,
    uint32_t radius,
    const uint32_t* output,
    uint32_t parent,
    const float* color
) {
    MetalRenderer* renderer = state.renderer;
    uint32_t width = output[2] - output[0];
    uint32_t height = output[3] - output[1];
    uint32_t result[2];
    int32_t layer = placeShadowResult(renderer, width, height, result);
    uint32_t region[2];
    if (layer < 0 || !placeShadowSource(renderer, width + radius * 2, height + radius * 2, region)) {
        return false;
    }
    for (uint32_t depth = 0; depth < state.layerDepth; ++depth) {
        LayerPass& owner = *renderer->layers[state.layerStack[depth]];
        if (owner.mask) {
            owner.sampledPages |= 1u << (layer / 4);
        }
    }

    FrameFillDraw source = draw;
    source.device[4] += shift[0] + region[0] + radius - output[0];
    source.device[5] += shift[1] + region[1] + radius - output[1];
    source.mask[0] = static_cast<float>(region[0]);
    source.mask[1] = static_cast<float>(region[1]);
    source.mask[2] = static_cast<float>(region[0] + width + radius * 2);
    source.mask[3] = static_cast<float>(region[1] + height + radius * 2);
    for (int channel = 0; channel < 4; ++channel) {
        source.color[channel] = 1;
    }
    source.fillRule = 0;
    source.clip = FillClipNone;
    source.coverage = 0;
    std::vector<Batch>* sink = state.batchSink;
    uint32_t layerDepth = state.layerDepth;
    uint32_t blend = state.blend;
    uint32_t roundedClip = state.roundedClip;
    state.batchSink = &renderer->shadowBatches[renderer->shadowGenerations - 1];
    state.layerDepth = 0;
    state.blend = 0;
    state.roundedClip = 0;
    MTLScissorRect full = {
        0,
        0,
        static_cast<NSUInteger>(renderer->pixelWidth),
        static_cast<NSUInteger>(renderer->pixelHeight),
    };
    emitFillDraw(state, source, full);
    state.batchSink = sink;
    state.layerDepth = layerDepth;
    state.blend = blend;
    state.roundedClip = roundedClip;

    ShadowRecord record = {};
    record.horizontal = {
        {
            static_cast<float>(region[0] + radius),
            static_cast<float>(region[1]),
            static_cast<float>(width),
            static_cast<float>(height + radius * 2),
        },
        {static_cast<float>(region[0] + radius), static_cast<float>(region[1])},
        radius,
        0,
    };
    record.vertical = {
        {
            static_cast<float>(result[0]),
            static_cast<float>(result[1]),
            static_cast<float>(width),
            static_cast<float>(height),
        },
        {static_cast<float>(region[0] + radius), static_cast<float>(region[1] + radius)},
        radius,
        1,
    };
    record.layer = static_cast<uint32_t>(layer);
    record.generation = renderer->shadowGenerations - 1;
    ensureVectorCapacity(renderer, renderer->shadowRecords, renderer->shadowRecords.size() + 1);
    renderer->shadowRecords.push_back(record);

    ensureVectorCapacity(renderer, renderer->coverageClips, renderer->coverageClips.size() + 1);
    renderer->coverageClips.push_back(CoverageClipRecord{
        result[0] | (result[1] << 16),
        parent,
        FillClipShadow | (static_cast<uint32_t>(layer) << 8),
        output[0] | (output[1] << 16),
    });
    uint32_t coverage = state.coverageClip;
    state.coverageClip = static_cast<uint32_t>(renderer->coverageClips.size());
    Instance instance = {};
    setFullContentMask(renderer, instance);
    instance.rect[0] = static_cast<float>(output[0]);
    instance.rect[1] = static_cast<float>(output[1]);
    instance.rect[2] = static_cast<float>(width);
    instance.rect[3] = static_cast<float>(height);
    instance.color = packColor(color[0], color[1], color[2], color[3]);
    instance.kind = 0;
    instance.opacity = 1;
    emitInstance(state, PipelineTier1, nil, instance);
    state.coverageClip = coverage;
    return true;
}

static void expandShadow(
    ExpandState& state,
    const PathOp& op,
    const FillPathRecord& path,
    const FillSpanRecord& span,
    const float* color,
    FrameStats& stats
) {
    MetalRenderer* renderer = state.renderer;
    MTLScissorRect scissor = clipToScissor(renderer, state.clip);
    if (!(color[3] > 0) || !path.segmentCount || !scissor.width || !scissor.height) {
        return;
    }
    const double identity[6] = {1, 0, 0, 1, 0, 0};
    uint32_t coverage = state.coverageClip;
    pushMaskCoverage(state, op.contentMask);
    FrameFillDraw draw = {};
    prepareFillDraw(state, identity, op.contentMask, draw);
    draw.segmentStart = path.segmentStart;
    draw.segmentCount = path.segmentCount;
    draw.originX = path.originX;
    draw.originY = path.originY;
    draw.bounds[2] = path.width;
    draw.bounds[3] = path.height;
    const double* device = draw.device;
    double deviceScale = std::sqrt(std::fabs(device[0] * device[3] - device[1] * device[2]));
    double blur = std::max(0.0, span.shadowBlur);
    double deviceBlur = blur * deviceScale;
    uint32_t radius = deviceBlur > 1e-9 ? static_cast<uint32_t>(ceil(deviceBlur - 1e-9)) : 0;
    double shift[2] = {
        floor(device[0] * span.shadowOffset[0] + device[2] * span.shadowOffset[1] + 0.5),
        std::round(device[1] * span.shadowOffset[0] + device[3] * span.shadowOffset[1]),
    };
    double local[4] = {path.originX, path.originY, path.originX + path.width, path.originY + path.height};
    double bounds[4];
    transformBounds(device, local, bounds);
    double left = std::max({
        floor(bounds[0] + shift[0]) - radius,
        static_cast<double>(scissor.x),
        floor(static_cast<double>(draw.mask[0])),
        0.0,
    });
    double top = std::max({
        floor(bounds[1] + shift[1]) - radius,
        static_cast<double>(scissor.y),
        floor(static_cast<double>(draw.mask[1])),
        0.0,
    });
    double end = std::min({
        ceil(bounds[2] + shift[0]) + radius,
        static_cast<double>(scissor.x + scissor.width),
        ceil(static_cast<double>(draw.mask[2])),
        static_cast<double>(std::min(renderer->pixelWidth, 65535)),
    });
    double bottom = std::min({
        ceil(bounds[3] + shift[1]) + radius,
        static_cast<double>(scissor.y + scissor.height),
        ceil(static_cast<double>(draw.mask[3])),
        static_cast<double>(std::min(renderer->pixelHeight, 65535)),
    });
    if (!(left < end) || !(top < bottom)) {
        state.coverageClip = coverage;
        return;
    }
    FrameFillDraw exclusion = draw;
    exclusion.fillRule = 1;
    uint32_t exclusionIndex = emitClipSource(state, exclusion, scissor);
    uint32_t parent = appendCoverage(
        renderer,
        exclusionIndex,
        state.coverageClip,
        FillClipOutside | FillClipEvenOdd
    );
    uint32_t across = static_cast<uint32_t>(end - left);
    uint32_t down = static_cast<uint32_t>(bottom - top);
    int64_t limitX = static_cast<int64_t>(renderer->pixelWidth) - radius * 2 - 2;
    int64_t limitY = static_cast<int64_t>(renderer->pixelHeight) - radius * 2 - 2;
    if (limitX <= 0 || limitY <= 0) {
        ++stats.skippedOps;
        state.coverageClip = coverage;
        return;
    }
    uint32_t columns = static_cast<uint32_t>((across + limitX - 1) / limitX);
    uint32_t rows = static_cast<uint32_t>((down + limitY - 1) / limitY);
    uint32_t pieceWidth = (across + columns - 1) / columns;
    uint32_t pieceHeight = (down + rows - 1) / rows;
    bool skipped = false;
    for (uint32_t y = static_cast<uint32_t>(top); y < static_cast<uint32_t>(bottom); y += pieceHeight) {
        for (uint32_t x = static_cast<uint32_t>(left); x < static_cast<uint32_t>(end); x += pieceWidth) {
            uint32_t output[4] = {
                x,
                y,
                std::min(x + pieceWidth, static_cast<uint32_t>(end)),
                std::min(y + pieceHeight, static_cast<uint32_t>(bottom)),
            };
            if (!expandShadowPiece(state, draw, shift, radius, output, parent, color)) {
                skipped = true;
            }
        }
    }
    if (skipped) {
        ++stats.skippedOps;
    }
    state.coverageClip = coverage;
}

static void expandPath(ExpandState& state, const PathOp& op, FrameStats& stats) {
    MetalRenderer* renderer = state.renderer;
    if (op.slot >= renderer->chunkPaths.size()) {
        ++stats.skippedOps;
        return;
    }
    const ChunkPathSet& set = renderer->chunkPaths[op.slot];
    if (!set.geometry.resident || set.serial != op.serial || op.span >= set.geometry.spans.size()) {
        ++stats.skippedOps;
        return;
    }
    const FillSpanRecord& span = set.geometry.spans[op.span];
    if (span.shadow) {
        size_t colorIndex = static_cast<size_t>(span.paletteIndex) * 4;
        if (span.path < set.geometry.paths.size() && colorIndex + 4 <= set.palette.size()) {
            expandShadow(state, op, set.geometry.paths[span.path], span, &set.palette[colorIndex], stats);
        }
        return;
    }
    MTLScissorRect scissor = clipToScissor(renderer, state.clip);
    if (!scissor.width || !scissor.height) {
        return;
    }
    const double identity[6] = {1, 0, 0, 1, 0, 0};
    uint32_t coverage = state.coverageClip;
    pushMaskCoverage(state, op.contentMask);
    FrameFillDraw draw = {};
    prepareFillDraw(state, identity, op.contentMask, draw);
    emitSpanDraws(
        state,
        draw,
        set.geometry,
        span,
        &set,
        reinterpret_cast<const uint8_t*>(set.palette.data()),
        static_cast<uint32_t>(set.palette.size() / 4),
        -1,
        -1,
        0,
        0,
        deviceDash(draw, span.dash),
        scissor
    );
    if (span.brush) {
        emitBrushDraw(state, set, span, draw, scissor);
    }
    state.coverageClip = coverage;
}

static uint32_t appendImagePaint(
    MetalRenderer* renderer,
    const AtlasEntry& entry,
    bool templateAlpha,
    const double* device
) {
    double determinant = device[0] * device[3] - device[1] * device[2];
    if (!(std::fabs(determinant) > 0) || !std::isfinite(determinant)) {
        return 0;
    }
    FillPaintGPU paint = {};
    paint.a = static_cast<float>(device[3] / determinant);
    paint.b = static_cast<float>(-device[1] / determinant);
    paint.c = static_cast<float>(-device[2] / determinant);
    paint.d = static_cast<float>(device[0] / determinant);
    paint.tx = static_cast<float>((device[2] * device[5] - device[3] * device[4]) / determinant);
    paint.ty = static_cast<float>((device[1] * device[4] - device[0] * device[5]) / determinant);
    paint.p0 = entry.x / static_cast<float>(kAtlasSize);
    paint.p1 = entry.y / static_cast<float>(kAtlasSize);
    paint.p2 = entry.width / static_cast<float>(kAtlasSize);
    paint.p3 = entry.height / static_cast<float>(kAtlasSize);
    paint.kind = templateAlpha ? 5 : 4;
    paint.padding = entry.page;
    ensureVectorCapacity(renderer, renderer->fillPaints, renderer->fillPaints.size() + 1);
    renderer->fillPaints.push_back(paint);
    return static_cast<uint32_t>(renderer->fillPaints.size()) | FillPaintTranslucent;
}

static bool expandImagePaint(ExpandState& state, const ImageOp& op) {
    MetalRenderer* renderer = state.renderer;
    bool templateAlpha = op.templateAlpha != 0;
    uint64_t key = op.canvas
        ? canvasImageKey(
            static_cast<uint64_t>(op.index) | (static_cast<uint64_t>(op.generation) << 32),
            0,
            0,
            templateAlpha
        )
        : imageKey(op.index, op.generation, 0, 0, templateAlpha);
    auto found = renderer->imageEntries.find(key);
    if (found == renderer->imageEntries.end() || !found->second.valid) {
        return false;
    }
    AtlasEntry& entry = found->second;
    entry.lastUsedFrame = renderer->frameCount;
    renderer->imageAtlas.pages[entry.page].lastUsedFrame = renderer->frameCount;
    MTLScissorRect scissor = clipToScissor(renderer, state.clip);
    if (!scissor.width || !scissor.height) {
        return true;
    }
    uint32_t coverage = state.coverageClip;
    pushMaskCoverage(state, op.contentMask);
    const FillPathRecord& path = ensureUnitRect(renderer);
    const double transform[6] = {op.width, 0, 0, op.height, op.x, op.y};
    FrameFillDraw draw = {};
    prepareFillDraw(state, transform, op.contentMask, draw);
    uint32_t paint = appendImagePaint(renderer, entry, templateAlpha, draw.device);
    if (!templateAlpha && paint) {
        const FillPaintGPU& record = renderer->fillPaints.back();
        double spreadX = std::fabs(record.a) + std::fabs(record.c);
        double spreadY = std::fabs(record.b) + std::fabs(record.d);
        const double spread[6] = {
            op.width * (1 + 2 * spreadX),
            0,
            0,
            op.height * (1 + 2 * spreadY),
            op.x - op.width * spreadX,
            op.y - op.height * spreadY,
        };
        prepareFillDraw(state, spread, op.contentMask, draw);
    }
    draw.segmentStart = path.segmentStart;
    draw.segmentCount = path.segmentCount;
    draw.originX = path.originX;
    draw.originY = path.originY;
    draw.bounds[2] = path.width;
    draw.bounds[3] = path.height;
    draw.clip = FillClipNone;
    draw.coverage = state.coverageClip;
    uint32_t color = templateAlpha ? op.color : 0xffffffffu;
    for (int channel = 0; channel < 4; ++channel) {
        draw.color[channel] = static_cast<float>((color >> (channel * 8)) & 0xffu) / 255.0f;
    }
    draw.paint = paint;
    if (draw.paint && draw.color[3] > 0) {
        emitFillDraw(state, draw, scissor);
    }
    state.coverageClip = coverage;
    return true;
}

static bool ensureLayerTarget(MetalRenderer* renderer, LayerPass& layer, uint32_t width, uint32_t height) {
    if (layer.texture && layer.texture.width >= width && layer.texture.height >= height) {
        return true;
    }
    auto rounded = [](uint32_t value, int32_t limit) {
        uint32_t step = ((value + 255u) / 256u) * 256u;
        return std::max<NSUInteger>(value, std::min<NSUInteger>(step, static_cast<NSUInteger>(limit)));
    };
    NSUInteger targetWidth = std::max<NSUInteger>(
        layer.texture ? layer.texture.width : 0,
        rounded(width, renderer->pixelWidth)
    );
    NSUInteger targetHeight = std::max<NSUInteger>(
        layer.texture ? layer.texture.height : 0,
        rounded(height, renderer->pixelHeight)
    );
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:renderer->layerFormat
                                     width:targetWidth
                                    height:targetHeight
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModePrivate;
    id<MTLTexture> texture = [renderer->device newTextureWithDescriptor:descriptor];
    MTLTextureDescriptor* depthDescriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                     width:targetWidth
                                    height:targetHeight
                                 mipmapped:NO];
    depthDescriptor.usage = MTLTextureUsageRenderTarget;
    depthDescriptor.storageMode = MTLStorageModeMemoryless;
    id<MTLTexture> depth = [renderer->device newTextureWithDescriptor:depthDescriptor];
    if (!texture || !depth) {
        return false;
    }
    layer.texture = texture;
    layer.depth = depth;
    countAllocation(renderer);
    countAllocation(renderer);
    return true;
}

static void finishMaskLayer(ExpandState& state, LayerPass& layer, uint32_t index) {
    MetalRenderer* renderer = state.renderer;
    std::vector<MaskRegion>& regions = renderer->maskRegions;
    if (layer.maskId >= regions.size()) {
        ensureVectorCapacity(renderer, regions, static_cast<size_t>(layer.maskId) + 1);
        regions.resize(static_cast<size_t>(layer.maskId) + 1);
    }
    MaskRegion& region = regions[layer.maskId];
    region = MaskRegion{};
    if (layer.empty || layer.batches.empty()) {
        return;
    }
    uint32_t left = static_cast<uint32_t>(std::max(0.0, floor(layer.bounds[0])));
    uint32_t top = static_cast<uint32_t>(std::max(0.0, floor(layer.bounds[1])));
    uint32_t right = static_cast<uint32_t>(std::max(0.0, std::min(
        static_cast<double>(std::min(renderer->pixelWidth, 65535)),
        ceil(layer.bounds[2])
    )));
    uint32_t bottom = static_cast<uint32_t>(std::max(0.0, std::min(
        static_cast<double>(std::min(renderer->pixelHeight, 65535)),
        ceil(layer.bounds[3])
    )));
    if (left >= right || top >= bottom) {
        return;
    }
    for (uint32_t page = 0; page < kMaskPages; ++page) {
        if ((layer.sampledPages & (1u << page)) || renderer->maskPageModes[page] == 2) {
            continue;
        }
        bool overlaps = false;
        for (const std::array<uint32_t, 4>& placed : renderer->maskPlaced[page]) {
            if (left < placed[2] && placed[0] < right && top < placed[3] && placed[1] < bottom) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) {
            continue;
        }
        ensureVectorCapacity(renderer, renderer->maskPlaced[page], renderer->maskPlaced[page].size() + 1);
        renderer->maskPlaced[page].push_back({left, top, right, bottom});
        renderer->maskPageModes[page] = 1;
        region.valid = true;
        region.page = page;
        region.bounds[0] = left;
        region.bounds[1] = top;
        region.bounds[2] = right;
        region.bounds[3] = bottom;
        layer.page = page;
        layer.origin[0] = left;
        layer.origin[1] = top;
        layer.size[0] = right - left;
        layer.size[1] = bottom - top;
        ensureVectorCapacity(renderer, renderer->layerOrder, renderer->layerOrder.size() + 1);
        renderer->layerOrder.push_back(index);
        return;
    }
    ++state.skippedMasks;
}

static void finishLayer(ExpandState& state, uint32_t index) {
    MetalRenderer* renderer = state.renderer;
    LayerPass& layer = *renderer->layers[index];
    if (layer.mask) {
        finishMaskLayer(state, layer, index);
        return;
    }
    bool unbounded = blendUnbounded(layer.blend);
    bool empty = layer.empty || layer.batches.empty();
    if (layer.transformed) {
        const double* m = layer.composite;
        if ((empty && !layer.filter) || !(std::fabs(m[0] * m[3] - m[1] * m[2]) > 1e-12)) {
            return;
        }
        unbounded = false;
    }
    if (layer.filter) {
        memcpy(layer.bounds, layer.filterRegion, sizeof(layer.bounds));
        empty = false;
    }
    if (empty && !unbounded) {
        return;
    }
    if (unbounded) {
        MTLScissorRect scissor = clipToScissor(renderer, state.clip);
        layer.bounds[0] = scissor.x;
        layer.bounds[1] = scissor.y;
        layer.bounds[2] = scissor.x + scissor.width;
        layer.bounds[3] = scissor.y + scissor.height;
    }
    double grow = static_cast<double>(layer.blur);
    double left = std::max(0.0, floor(layer.bounds[0]) - grow);
    double top = std::max(0.0, floor(layer.bounds[1]) - grow);
    double right = std::min(static_cast<double>(renderer->pixelWidth), ceil(layer.bounds[2]) + grow);
    double bottom = std::min(static_cast<double>(renderer->pixelHeight), ceil(layer.bounds[3]) + grow);
    if (!(left < right) || !(top < bottom)) {
        return;
    }
    uint32_t width = static_cast<uint32_t>(right - left);
    uint32_t height = static_cast<uint32_t>(bottom - top);
    if (empty) {
        Instance instance = {};
        setFullContentMask(renderer, instance);
        instance.rect[0] = static_cast<float>(left);
        instance.rect[1] = static_cast<float>(top);
        instance.rect[2] = static_cast<float>(width);
        instance.rect[3] = static_cast<float>(height);
        instance.params[2] = 1;
        instance.params[3] = 1;
        instance.color = 0xffffffffu;
        instance.kind = 3;
        instance.opacity = layer.alpha;
        emitInstance(state, PipelineRGBA, renderer->blankTexture, instance);
        return;
    }
    if (!ensureLayerTarget(renderer, layer, width, height)) {
        return;
    }
    layer.origin[0] = static_cast<uint32_t>(left);
    layer.origin[1] = static_cast<uint32_t>(top);
    layer.size[0] = width;
    layer.size[1] = height;
    if (layer.deferred) {
        std::vector<Batch>& parent = *state.batchSink;
        ensureVectorCapacity(renderer, parent, parent.size() + 1);
        parent.push_back(Batch{PipelineDeferredLayer, nil, MTLScissorRect{0, 0, 0, 0}, index, 1, false});
        ensureVectorCapacity(renderer, renderer->deferredLayers, renderer->deferredLayers.size() + 1);
        renderer->deferredLayers.push_back(index);
    } else {
        ensureVectorCapacity(renderer, renderer->layerOrder, renderer->layerOrder.size() + 1);
        renderer->layerOrder.push_back(index);
    }
    if (layer.transformed) {
        const double* m = layer.composite;
        double scale = renderer->scale;
        Instance instance = {};
        setFullContentMask(renderer, instance);
        instance.rect[0] = static_cast<float>(m[0] * left + m[2] * top + m[4] * scale);
        instance.rect[1] = static_cast<float>(m[1] * left + m[3] * top + m[5] * scale);
        instance.rect[2] = static_cast<float>(m[0] * width);
        instance.rect[3] = static_cast<float>(m[1] * width);
        instance.params[0] = static_cast<float>(m[2] * height);
        instance.params[1] = static_cast<float>(m[3] * height);
        instance.params[2] = 0;
        instance.params[3] = 0;
        float uvSize[2] = {
            static_cast<float>(width) / static_cast<float>(layer.texture.width),
            static_cast<float>(height) / static_cast<float>(layer.texture.height),
        };
        memcpy(instance.padding, uvSize, sizeof(uvSize));
        instance.color = 0xffffffffu;
        instance.kind = 7u | (0xfu << 8) | kInstanceAffine;
        instance.opacity = layer.alpha;
        emitInstance(state, PipelineRGBA, layer.texture, instance);
        return;
    }
    Instance instance = {};
    setFullContentMask(renderer, instance);
    instance.rect[0] = static_cast<float>(left);
    instance.rect[1] = static_cast<float>(top);
    instance.rect[2] = static_cast<float>(width);
    instance.rect[3] = static_cast<float>(height);
    instance.params[2] = static_cast<float>(width) / static_cast<float>(layer.texture.width);
    instance.params[3] = static_cast<float>(height) / static_cast<float>(layer.texture.height);
    instance.color = 0xffffffffu;
    instance.kind = 3;
    instance.opacity = layer.alpha;
    if (!layer.clipOverflow || unbounded || layer.blurred || layer.filter) {
        emitInstance(state, PipelineRGBA, layer.texture, instance);
        return;
    }
    uint32_t roundedClip = state.roundedClip;
    uint32_t coverageClip = state.coverageClip;
    state.roundedClip = 0;
    state.coverageClip = 0;
    emitInstance(state, PipelineRGBA, layer.texture, instance);
    state.roundedClip = roundedClip;
    state.coverageClip = coverageClip;
}

static id<MTLTexture> acquireBackdropTexture(MetalRenderer* renderer, uint32_t width, uint32_t height) {
    BackdropTexture* best = nullptr;
    for (BackdropTexture& entry : renderer->backdropTextures) {
        if (entry.used || entry.texture.width < width || entry.texture.height < height) {
            continue;
        }
        if (!best || entry.texture.width * entry.texture.height < best->texture.width * best->texture.height) {
            best = &entry;
        }
    }
    if (!best) {
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:kBackdropFormat
                                         width:((width + 63u) / 64u) * 64u
                                        height:((height + 63u) / 64u) * 64u
                                     mipmapped:NO];
        descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        descriptor.storageMode = MTLStorageModePrivate;
        id<MTLTexture> texture = [renderer->device newTextureWithDescriptor:descriptor];
        if (!texture) {
            return nil;
        }
        countAllocation(renderer);
        ensureVectorCapacity(renderer, renderer->backdropTextures, renderer->backdropTextures.size() + 1);
        renderer->backdropTextures.emplace_back();
        best = &renderer->backdropTextures.back();
        best->texture = texture;
    }
    best->used = true;
    best->lastUsedFrame = renderer->frameCount;
    return best->texture;
}

static void resetBackdrops(MetalRenderer* renderer) {
    renderer->backdrops.clear();
    renderer->backdropArgs.clear();
    std::vector<BackdropTexture>& textures = renderer->backdropTextures;
    size_t kept = 0;
    for (BackdropTexture& entry : textures) {
        if (entry.lastUsedFrame + 240 < renderer->frameCount) {
            continue;
        }
        entry.used = false;
        textures[kept++] = entry;
    }
    textures.resize(kept);
}

static void planBackdrop(
    BackdropRecord& record,
    uint32_t blur,
    int64_t left,
    int64_t top,
    int64_t width,
    int64_t height
) {
    const double sigma = static_cast<double>(blur) * 0.5;
    record.level = 0;
    while (sigma / static_cast<double>(1 << record.level) > 4) {
        ++record.level;
    }
    const double factor = static_cast<double>(1 << record.level);
    const double sigmaLevel = sigma / factor;
    const int32_t radius = sigmaLevel > 0 ? static_cast<int32_t>(std::ceil(3 * sigmaLevel)) : 0;
    const int64_t minX = static_cast<int64_t>(std::floor((static_cast<double>(left) + 0.5) / factor - 0.5));
    const int64_t maxX = static_cast<int64_t>(
        std::floor((static_cast<double>(left + width - 1) + 0.5) / factor - 0.5)
    ) + 1;
    const int64_t minY = static_cast<int64_t>(std::floor((static_cast<double>(top) + 0.5) / factor - 0.5));
    const int64_t maxY = static_cast<int64_t>(
        std::floor((static_cast<double>(top + height - 1) + 0.5) / factor - 0.5)
    ) + 1;
    record.firstX = minX - radius;
    record.firstY = minY - radius;
    record.columns = static_cast<uint32_t>(maxX - minX + 1 + 2 * radius);
    record.rows = static_cast<uint32_t>(maxY - minY + 1 + 2 * radius);
    float weights[13] = {};
    float total = 0;
    for (int32_t tap = -radius; tap <= radius; ++tap) {
        const float weight = radius
            ? std::exp(-static_cast<float>(tap * tap) / static_cast<float>(2 * sigmaLevel * sigmaLevel))
            : 1.0f;
        if (tap >= 0) {
            weights[tap] = weight;
        }
        total += weight;
    }
    record.blur = BackdropBlurGPU{};
    record.blur.center = weights[0] / total;
    for (int32_t tap = 1; tap <= radius; tap += 2) {
        const float near = weights[tap] / total;
        const float far = tap + 1 <= radius ? weights[tap + 1] / total : 0.0f;
        const float pair = near + far;
        record.blur.weights[record.blur.pairs] = pair;
        record.blur.offsets[record.blur.pairs] = static_cast<float>(tap) + (pair > 0 ? far / pair : 0.0f);
        ++record.blur.pairs;
    }
}

static bool sinkPaints(MetalRenderer* renderer, const std::vector<Batch>& batches) {
    for (const Batch& batch : batches) {
        if (batch.pipeline != PipelineFill || renderer->fillBatches[batch.start].paints) {
            return true;
        }
    }
    return false;
}

static MTLScissorRect intersectScissors(
    const MTLScissorRect& left,
    const MTLScissorRect& right
);

static bool scissorTouches(const MTLScissorRect& scissor, const double* bounds) {
    return scissor.width && scissor.height &&
        bounds[0] < static_cast<double>(scissor.x + scissor.width) &&
        bounds[2] > static_cast<double>(scissor.x) &&
        bounds[1] < static_cast<double>(scissor.y + scissor.height) &&
        bounds[3] > static_cast<double>(scissor.y);
}

static bool backdropMissesDamage(const ExpandState& state, const double* region) {
    MetalRenderer* renderer = state.renderer;
    if (renderer->encodeMode != 1 || renderer->encodeDamage[2] <= 0 || renderer->encodeDamage[3] <= 0) {
        return false;
    }
    double corners[8] = {
        region[0], region[1],
        region[2], region[1],
        region[0], region[3],
        region[2], region[3],
    };
    for (uint32_t depth = state.layerDepth; depth > 0; --depth) {
        const LayerPass& layer = *renderer->layers[state.layerStack[depth - 1]];
        if (layer.blurred || layer.filter) {
            return false;
        }
        if (!layer.transformed) {
            continue;
        }
        const double* m = layer.composite;
        for (uint32_t corner = 0; corner < 4; ++corner) {
            const double x = corners[corner * 2];
            const double y = corners[corner * 2 + 1];
            corners[corner * 2] = m[0] * x + m[2] * y + m[4] * renderer->scale;
            corners[corner * 2 + 1] = m[1] * x + m[3] * y + m[5] * renderer->scale;
        }
    }
    double placed[4] = {corners[0], corners[1], corners[0], corners[1]};
    for (uint32_t corner = 1; corner < 4; ++corner) {
        placed[0] = std::min(placed[0], corners[corner * 2]);
        placed[1] = std::min(placed[1], corners[corner * 2 + 1]);
        placed[2] = std::max(placed[2], corners[corner * 2]);
        placed[3] = std::max(placed[3], corners[corner * 2 + 1]);
    }
    const double* damage = renderer->encodeDamage;
    const MTLScissorRect bounds = clipToScissor(
        renderer,
        ClipState{damage[0], damage[1], damage[0] + damage[2], damage[1] + damage[3]}
    );
    const std::vector<double>& values = renderer->encodeDamageAreas;
    MTLScissorRect areas[kMaxDamageAreas];
    size_t count = 0;
    for (size_t index = 0; index + 3 < values.size() && count < kMaxDamageAreas; index += 4) {
        const MTLScissorRect area = intersectScissors(
            clipToScissor(
                renderer,
                ClipState{values[index], values[index + 1], values[index] + values[index + 2], values[index + 1] + values[index + 3]}
            ),
            bounds
        );
        if (area.width && area.height) {
            areas[count++] = area;
        }
    }
    if (count < 2) {
        return !scissorTouches(bounds, placed);
    }
    for (size_t index = 0; index < count; ++index) {
        if (scissorTouches(areas[index], placed)) {
            return false;
        }
    }
    return true;
}

static bool backdropRegion(
    ExpandState& state,
    float x,
    float y,
    float width,
    float height,
    double* bounds,
    double* region
) {
    MetalRenderer* renderer = state.renderer;
    if (state.layerOverflow || !(width > 0) || !(height > 0)) {
        return false;
    }
    for (uint32_t depth = 0; depth < state.layerDepth; ++depth) {
        if (renderer->layers[state.layerStack[depth]]->mask) {
            return false;
        }
    }
    const double scale = renderer->scale;
    double local[4] = {x, y, x + width, y + height};
    transformBounds(state.matrix, local, bounds);
    region[0] = std::max(0.0, std::floor(std::max(bounds[0], state.clip.left) * scale));
    region[1] = std::max(0.0, std::floor(std::max(bounds[1], state.clip.top) * scale));
    region[2] = std::min(
        static_cast<double>(renderer->pixelWidth),
        std::ceil(std::min(bounds[2], state.clip.right) * scale)
    );
    region[3] = std::min(
        static_cast<double>(renderer->pixelHeight),
        std::ceil(std::min(bounds[3], state.clip.bottom) * scale)
    );
    return region[0] < region[2] && region[1] < region[3] && !backdropMissesDamage(state, region);
}

static void commitBackdrop(ExpandState& state, BackdropRecord record, id<MTLTexture> result) {
    MetalRenderer* renderer = state.renderer;
    uint32_t sourceDepth = state.layerDepth;
    while (sourceDepth > 0 && !sinkPaints(renderer, renderer->layers[state.layerStack[sourceDepth - 1]]->batches)) {
        --sourceDepth;
    }
    for (uint32_t depth = sourceDepth; depth < state.layerDepth; ++depth) {
        LayerPass& layer = *renderer->layers[state.layerStack[depth]];
        layer.deferred = true;
        if (layer.transformed) {
            const double* m = layer.composite;
            const double pixels[6] = {m[0], m[1], m[2], m[3], m[4] * renderer->scale, m[5] * renderer->scale};
            fillMultiply(record.sourceTransform, pixels, record.sourceTransform);
            record.transformed = true;
        }
    }
    std::vector<Batch>& sourceSink = sourceDepth == state.layerDepth
        ? *state.batchSink
        : *state.layerSinks[sourceDepth];
    const uint32_t index = static_cast<uint32_t>(renderer->backdrops.size());
    ensureVectorCapacity(renderer, renderer->backdrops, renderer->backdrops.size() + 1);
    renderer->backdrops.push_back(record);
    ensureVectorCapacity(renderer, sourceSink, sourceSink.size() + 1);
    sourceSink.push_back(Batch{PipelineBackdropBlur, nil, MTLScissorRect{0, 0, 0, 0}, index, 1, false});
    const double left = record.region[0];
    const double top = record.region[1];
    const double width = record.region[2];
    const double height = record.region[3];
    const double factor = record.shader ? 1.0 : static_cast<double>(1 << record.level);
    const double originX = record.shader ? left : static_cast<double>(record.firstX);
    const double originY = record.shader ? top : static_cast<double>(record.firstY);
    const double textureWidth = static_cast<double>(result.width);
    const double textureHeight = static_cast<double>(result.height);
    Instance instance = {};
    setFullContentMask(renderer, instance);
    instance.rect[0] = static_cast<float>(left);
    instance.rect[1] = static_cast<float>(top);
    instance.rect[2] = static_cast<float>(width);
    instance.rect[3] = static_cast<float>(height);
    instance.params[0] = static_cast<float>((left / factor - originX) / textureWidth);
    instance.params[1] = static_cast<float>((top / factor - originY) / textureHeight);
    instance.params[2] = static_cast<float>(width / factor / textureWidth);
    instance.params[3] = static_cast<float>(height / factor / textureHeight);
    instance.color = 0xffffffffu;
    instance.kind = 3;
    instance.opacity = 1;
    emitInstance(state, PipelineBackdrop, result, instance);
}

static void expandBackdrop(ExpandState& state, const BackdropOp& op, FrameStats& stats) {
    MetalRenderer* renderer = state.renderer;
    double bounds[4];
    double region[4];
    if (!backdropRegion(state, op.x, op.y, op.width, op.height, bounds, region)) {
        return;
    }
    const double left = region[0];
    const double top = region[1];
    const double right = region[2];
    const double bottom = region[3];
    const double determinant = state.matrix[0] * state.matrix[3] - state.matrix[1] * state.matrix[2];
    const double spread = std::isfinite(op.radius) && op.radius > 0
        ? std::ceil(op.radius * std::sqrt(std::fabs(determinant)) * renderer->scale - 1e-6)
        : 0;
    BackdropRecord record;
    record.region[0] = static_cast<int32_t>(left);
    record.region[1] = static_cast<int32_t>(top);
    record.region[2] = static_cast<int32_t>(right - left);
    record.region[3] = static_cast<int32_t>(bottom - top);
    planBackdrop(
        record,
        static_cast<uint32_t>(std::min<double>(spread, kMaxBackdropBlur)),
        static_cast<int64_t>(left),
        static_cast<int64_t>(top),
        static_cast<int64_t>(right - left),
        static_cast<int64_t>(bottom - top)
    );
    for (int32_t level = record.level > 0 ? 1 : 0; level <= record.level; ++level) {
        const uint32_t expand = 1u << (record.level - level);
        record.levels[level] = acquireBackdropTexture(renderer, record.columns * expand, record.rows * expand);
        if (!record.levels[level]) {
            ++stats.skippedOps;
            return;
        }
    }
    record.scratch = acquireBackdropTexture(renderer, record.columns, record.rows);
    if (!record.scratch) {
        ++stats.skippedOps;
        return;
    }
    commitBackdrop(state, record, record.levels[record.level]);
}

static void expandBackdropShader(ExpandState& state, const BackdropShaderOp& op, FrameStats& stats) {
    MetalRenderer* renderer = state.renderer;
    if (op.slot >= renderer->chunkPaths.size()) {
        return;
    }
    const ChunkPathSet& set = renderer->chunkPaths[op.slot];
    if (set.serial != op.serial || !op.gradient || op.gradient > set.gradients.size()) {
        return;
    }
    const BxGuiMetalGradient& gradient = set.gradients[op.gradient - 1];
    if (gradient.kind != 4 || gradient.stopStart > set.shaderArgs.size() ||
            gradient.stopCount > set.shaderArgs.size() - gradient.stopStart) {
        return;
    }
    double bounds[4];
    double region[4];
    if (!backdropRegion(state, op.x, op.y, op.width, op.height, bounds, region) ||
            !ensureShader(renderer, ShaderRoleFilter, gradient.shader, kBackdropFormat)) {
        return;
    }
    const double scale = renderer->scale;
    const double determinant = state.matrix[0] * state.matrix[3] - state.matrix[1] * state.matrix[2];
    const double spread = std::isfinite(op.radius) && op.radius > 0
        ? std::ceil(op.radius * std::sqrt(std::fabs(determinant)) * scale - 1e-6)
        : 0;
    BackdropRecord record;
    record.region[0] = static_cast<int32_t>(region[0]);
    record.region[1] = static_cast<int32_t>(region[1]);
    record.region[2] = static_cast<int32_t>(region[2] - region[0]);
    record.region[3] = static_cast<int32_t>(region[3] - region[1]);
    record.firstX = record.region[0];
    record.firstY = record.region[1];
    record.columns = static_cast<uint32_t>(record.region[2]);
    record.rows = static_cast<uint32_t>(record.region[3]);
    record.blurred = spread > 0;
    if (record.blurred) {
        planBackdrop(
            record,
            static_cast<uint32_t>(std::min<double>(spread, kMaxBackdropBlur)),
            record.region[0],
            record.region[1],
            record.region[2],
            record.region[3]
        );
    }
    record.shader = gradient.shader;
    record.argStart = static_cast<uint32_t>(renderer->backdropArgs.size());
    record.argCount = gradient.stopCount;
    record.time = gradient.endRadius >= 0
        ? static_cast<float>(std::max(0.0, renderer->fillTime - gradient.endRadius))
        : 0.0f;
    record.shaderOrigin[0] = static_cast<float>(bounds[0] * scale - region[0]);
    record.shaderOrigin[1] = static_cast<float>(bounds[1] * scale - region[1]);
    record.shaderSize[0] = op.width;
    record.shaderSize[1] = op.height;
    record.shaderScale = static_cast<float>(std::sqrt(std::fabs(determinant)) * scale);
    for (int32_t level = record.level > 0 ? 1 : 0; level <= record.level; ++level) {
        const uint32_t expand = 1u << (record.level - level);
        record.levels[level] = acquireBackdropTexture(renderer, record.columns * expand, record.rows * expand);
        if (!record.levels[level]) {
            ++stats.skippedOps;
            return;
        }
    }
    record.scratch = record.blurred ? acquireBackdropTexture(renderer, record.columns, record.rows) : nil;
    record.shaded = acquireBackdropTexture(
        renderer,
        static_cast<uint32_t>(record.region[2]),
        static_cast<uint32_t>(record.region[3])
    );
    if (!record.shaded || (record.blurred && !record.scratch)) {
        ++stats.skippedOps;
        return;
    }
    ensureVectorCapacity(renderer, renderer->backdropArgs, renderer->backdropArgs.size() + record.argCount);
    renderer->backdropArgs.insert(
        renderer->backdropArgs.end(),
        set.shaderArgs.begin() + gradient.stopStart,
        set.shaderArgs.begin() + gradient.stopStart + gradient.stopCount
    );
    commitBackdrop(state, record, record.shaded);
}

static const FillPathRecord* chunkClipPath(MetalRenderer* renderer, const PathClipOp& op) {
    if (op.slot >= renderer->chunkPaths.size()) {
        return nullptr;
    }
    const ChunkPathSet& set = renderer->chunkPaths[op.slot];
    if (!set.geometry.resident || set.serial != op.serial || op.path >= set.geometry.paths.size()) {
        return nullptr;
    }
    return &set.geometry.paths[op.path];
}

static void pushExpandMatrix(ExpandState& state) {
    memcpy(state.matrixStack[state.matrixDepth], state.matrix, sizeof(state.matrix));
    memcpy(state.paperStack[state.matrixDepth], state.paper, sizeof(state.paper));
    ++state.matrixDepth;
}

static void popExpandMatrix(ExpandState& state) {
    --state.matrixDepth;
    memcpy(state.matrix, state.matrixStack[state.matrixDepth], sizeof(state.matrix));
    memcpy(state.paper, state.paperStack[state.matrixDepth], sizeof(state.paper));
}

static bool pushExpandClip(ExpandState& state) {
    if (state.clipDepth >= kExpandDepth) {
        ++state.clipOverflow;
        return false;
    }
    state.clipStack[state.clipDepth] = state.clip;
    state.roundedClipStack[state.clipDepth] = state.roundedClip;
    state.coverageClipStack[state.clipDepth] = state.coverageClip;
    ++state.clipDepth;
    return true;
}

static void popLayerClip(ExpandState& state, const LayerPass& layer) {
    if (layer.mask) {
        return;
    }
    if (layer.clipOverflow) {
        if (state.clipOverflow) {
            --state.clipOverflow;
        }
        return;
    }
    if (state.clipDepth) {
        --state.clipDepth;
        state.clip = state.clipStack[state.clipDepth];
        state.roundedClip = state.roundedClipStack[state.clipDepth];
        state.coverageClip = state.coverageClipStack[state.clipDepth];
    }
}

static void intersectExpandClip(ExpandState& state, const double* local) {
    double bounds[4];
    transformBounds(state.matrix, local, bounds);
    state.clip.left = std::max(state.clip.left, bounds[0]);
    state.clip.top = std::max(state.clip.top, bounds[1]);
    state.clip.right = std::min(state.clip.right, bounds[2]);
    state.clip.bottom = std::min(state.clip.bottom, bounds[3]);
}

static bool inlinesTransformLayer(const MetalRenderer* renderer, const uint8_t* cursor, const uint8_t* end) {
    uint32_t transforms = 0;
    while (cursor < end) {
        PacketOpKind kind = static_cast<PacketOpKind>(*cursor++);
        if (kind == PacketOpPopLayer) {
            return transforms == 0;
        } else if (kind == PacketOpFill) {
            FillOp op;
            if (!readPacketValue(cursor, end, op)) return false;
            size_t paletteBytes = static_cast<size_t>(op.paletteCount) * 16;
            if (static_cast<size_t>(end - cursor) < paletteBytes) return false;
            cursor += paletteBytes;
        } else if (kind == PacketOpPath) {
            PathOp op;
            if (!readPacketValue(cursor, end, op)) return false;
            if (op.slot >= renderer->chunkPaths.size()) return false;
            const FillGeometryRecord& geometry = renderer->chunkPaths[op.slot].geometry;
            if (op.span >= geometry.spans.size()) return false;
            const FillSpanRecord& span = geometry.spans[op.span];
            if (span.shadow || span.brush) return false;
        } else if (kind == PacketOpPushTranslate) {
            TranslateOp op;
            if (!readPacketValue(cursor, end, op)) return false;
            ++transforms;
        } else if (kind == PacketOpPushTransform || kind == PacketOpPushContentTransform) {
            TransformOp op;
            if (!readPacketValue(cursor, end, op)) return false;
            ++transforms;
        } else if (kind == PacketOpPopTransform) {
            if (!transforms) return false;
            --transforms;
        } else {
            return false;
        }
    }
    return false;
}

static const size_t kMergeLookback = 32;

static bool batchMergeable(const Batch& batch) {
    return batch.pipeline == PipelineTier1 || batch.pipeline == PipelineMask ||
        batch.pipeline == PipelineRGBA || batch.pipeline == PipelineFill;
}

static bool batchesMatch(const Batch& left, const Batch& right) {
    return left.pipeline == right.pipeline && left.texture == right.texture &&
        scissorsEqual(left.scissor, right.scissor) && left.blend == right.blend &&
        left.shader == right.shader;
}

static bool batchRectsOverlap(const BatchRects& left, const BatchRects& right) {
    for (uint32_t outer = 0; outer < left.count; ++outer) {
        const float* a = left.rects[outer];
        for (uint32_t inner = 0; inner < right.count; ++inner) {
            const float* b = right.rects[inner];
            if (a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3]) {
                return true;
            }
        }
    }
    return false;
}

static void mergeBatches(MetalRenderer* renderer) {
    std::vector<Batch>& batches = renderer->batches;
    std::vector<BatchGroup>& groups = renderer->batchGroups;
    std::vector<uint32_t>& next = renderer->batchNext;
    groups.clear();
    ensureVectorCapacity(renderer, next, batches.size());
    next.assign(batches.size(), UINT32_MAX);
    size_t barrier = 0;
    bool merged = false;
    for (uint32_t index = 0; index < batches.size(); ++index) {
        const Batch& batch = batches[index];
        if (!batch.count) {
            continue;
        }
        if (!batchMergeable(batch)) {
            barrier = groups.size();
            continue;
        }
        BatchRects bounds = index < renderer->batchRects.size() ? renderer->batchRects[index] : BatchRects{{}, 0};
        size_t target = SIZE_MAX;
        size_t limit = std::max(barrier, groups.size() > kMergeLookback ? groups.size() - kMergeLookback : 0);
        for (size_t group = groups.size(); group > limit; --group) {
            BatchGroup& candidate = groups[group - 1];
            if (batchesMatch(batches[candidate.head], batch)) {
                target = group - 1;
                break;
            }
            if (batchRectsOverlap(candidate.bounds, bounds)) {
                break;
            }
        }
        if (target == SIZE_MAX) {
            ensureVectorCapacity(renderer, groups, groups.size() + 1);
            groups.push_back(BatchGroup{index, index, 1, bounds});
            continue;
        }
        BatchGroup& group = groups[target];
        next[group.tail] = index;
        group.tail = index;
        ++group.members;
        for (uint32_t rect = 0; rect < bounds.count; ++rect) {
            includeBatchRect(group.bounds, bounds.rects[rect]);
        }
        merged = true;
    }
    if (!merged) {
        return;
    }
    std::vector<uint32_t>& heads = renderer->fillGroupHeads;
    ensureVectorCapacity(renderer, heads, renderer->fillBatches.size());
    heads.assign(renderer->fillBatches.size(), UINT32_MAX);
    uint32_t base = static_cast<uint32_t>(renderer->instances.size());
    for (const BatchGroup& group : groups) {
        if (group.members < 2) {
            continue;
        }
        Batch& head = batches[group.head];
        if (head.pipeline == PipelineFill) {
            uint32_t representative = head.start;
            FillBatchInfo& info = renderer->fillBatches[representative];
            heads[representative] = group.head;
            for (uint32_t member = next[group.head]; member != UINT32_MAX; member = next[member]) {
                uint32_t fillBatch = batches[member].start;
                const FillBatchInfo& absorbed = renderer->fillBatches[fillBatch];
                for (uint32_t draw = absorbed.drawStart; draw < absorbed.drawEnd; ++draw) {
                    renderer->fillDraws[draw].batch = representative;
                }
                info.paints = info.paints || absorbed.paints;
                heads[fillBatch] = UINT32_MAX - 1;
                batches[member].count = 0;
            }
            continue;
        }
        uint32_t start = base + renderer->instanceGatherCount;
        uint32_t total = 0;
        for (uint32_t member = group.head; member != UINT32_MAX; member = next[member]) {
            ensureVectorCapacity(renderer, renderer->instanceGather, renderer->instanceGather.size() + 2);
            renderer->instanceGather.push_back(batches[member].start);
            renderer->instanceGather.push_back(batches[member].count);
            total += batches[member].count;
            if (member != group.head) {
                batches[member].count = 0;
            }
        }
        head.start = start;
        head.count = total;
        renderer->instanceGatherCount += total;
    }
    std::vector<uint32_t>& order = renderer->fillOrder;
    ensureVectorCapacity(renderer, order, renderer->fillDraws.size());
    order.clear();
    for (uint32_t fillBatch = 0; fillBatch < renderer->fillBatches.size(); ++fillBatch) {
        uint32_t head = heads[fillBatch];
        if (head == UINT32_MAX - 1) {
            continue;
        }
        if (head == UINT32_MAX) {
            const FillBatchInfo& info = renderer->fillBatches[fillBatch];
            for (uint32_t draw = info.drawStart; draw < info.drawEnd; ++draw) {
                order.push_back(draw);
            }
            continue;
        }
        for (uint32_t member = head; member != UINT32_MAX; member = next[member]) {
            const FillBatchInfo& info = renderer->fillBatches[batches[member].start];
            for (uint32_t draw = info.drawStart; draw < info.drawEnd; ++draw) {
                order.push_back(draw);
            }
        }
    }
}

static void expandPacket(MetalRenderer* renderer, FramePacket* packet, FrameStats& stats, bool tiles) {
    renderer->fillOrder.clear();
    renderer->batchRects.clear();
    renderer->instanceGather.clear();
    renderer->instanceGatherCount = 0;
    renderer->roundedClips.clear();
    renderer->coverageClips.clear();
    renderer->instances.clear();
    renderer->batches.clear();
    renderer->fillDraws.clear();
    renderer->fillPaints.clear();
    renderer->fillStops.clear();
    renderer->fillBatches.clear();
    renderer->brushDabs.clear();
    renderer->brushDraws.clear();
    sweepBrushDabs(renderer);
    renderer->layerCount = 0;
    renderer->layerOrder.clear();
    renderer->deferredLayers.clear();
    renderer->maskRegions.clear();
    for (std::vector<std::array<uint32_t, 4>>& placed : renderer->maskPlaced) {
        placed.clear();
    }
    for (std::vector<Batch>& batches : renderer->shadowBatches) {
        batches.clear();
    }
    renderer->shadowGenerations = 0;
    renderer->shadowRecords.clear();
    resetBackdrops(renderer);
    memset(renderer->shadowShelf, 0, sizeof(renderer->shadowShelf));
    memset(renderer->shadowPageShelves, 0, sizeof(renderer->shadowPageShelves));
    memset(renderer->maskPageModes, 0, sizeof(renderer->maskPageModes));

    ExpandState state;
    state.renderer = renderer;
    state.clip = {0, 0, static_cast<double>(renderer->width),
                  static_cast<double>(renderer->height)};
    state.batchSink = &renderer->batches;
    state.tileFrame = tiles;

    const uint8_t* cursor = packet->bytes.data();
    const uint8_t* end = cursor + packet->bytes.size();
    while (cursor < end) {
        PacketOpKind kind = static_cast<PacketOpKind>(*cursor++);
        if (kind == PacketOpQuad) {
            QuadOp op;
            if (!readPacketValue(cursor, end, op)) break;
            Instance instance = {};
            setFullContentMask(renderer, instance);
            instance.rect[0] = static_cast<float>((op.x * state.matrix[0] + state.matrix[4]) * renderer->scale);
            instance.rect[1] = static_cast<float>((op.y * state.matrix[3] + state.matrix[5]) * renderer->scale);
            instance.rect[2] = static_cast<float>(op.width * state.matrix[0] * renderer->scale);
            instance.rect[3] = static_cast<float>(op.height * state.matrix[3] * renderer->scale);
            instance.params[0] = static_cast<float>(op.cornerRadius * state.matrix[0] * renderer->scale);
            instance.color = op.color;
            instance.kind = 0;
            instance.opacity = 1;
            emitInstance(state, PipelineTier1, nil, instance);
        } else if (kind == PacketOpQuads) {
            uint32_t count;
            if (!readPacketValue(cursor, end, count)) break;
            size_t bytes = static_cast<size_t>(count) * sizeof(QuadOp);
            if (static_cast<size_t>(end - cursor) < bytes) break;
            emitQuads(state, cursor, count);
            cursor += bytes;
        } else if (kind == PacketOpBorder) {
            BorderOp op;
            if (!readPacketValue(cursor, end, op)) break;
            Instance instance = {};
            setFullContentMask(renderer, instance);
            instance.rect[0] = static_cast<float>((op.x * state.matrix[0] + state.matrix[4]) * renderer->scale);
            instance.rect[1] = static_cast<float>((op.y * state.matrix[3] + state.matrix[5]) * renderer->scale);
            instance.rect[2] = static_cast<float>(op.width * state.matrix[0] * renderer->scale);
            instance.rect[3] = static_cast<float>(op.height * state.matrix[3] * renderer->scale);
            instance.params[0] = static_cast<float>(op.cornerRadius * state.matrix[0] * renderer->scale);
            instance.params[1] = static_cast<float>(op.borderWidth * state.matrix[0] * renderer->scale);
            instance.color = op.color;
            instance.kind = 1;
            instance.opacity = 1;
            emitInstance(state, PipelineTier1, nil, instance);
        } else if (kind == PacketOpShape) {
            ShapeOp op;
            if (!readPacketValue(cursor, end, op)) break;
            const float scale = static_cast<float>(renderer->scale * state.matrix[0]);
            Instance instance = {};
            instance.rect[0] = static_cast<float>((op.x * state.matrix[0] + state.matrix[4]) * renderer->scale);
            instance.rect[1] = static_cast<float>((op.y * state.matrix[3] + state.matrix[5]) * renderer->scale);
            instance.rect[2] = op.width * scale;
            instance.rect[3] = op.height * scale;
            instance.params[0] = op.radius * scale;
            instance.params[1] = op.kind == 6 ? op.blur * 0.5f * scale : op.strokeWidth * scale;
            instance.params[2] = op.offset * scale;
            instance.params[3] = op.side >= 0 ? op.size * scale : 0;
            instance.contentMask[0] = op.shadowX * scale;
            instance.contentMask[1] = op.shadowY * scale;
            instance.contentMask[2] = op.spread * scale;
            instance.contentMask[3] = 0;
            uint32_t side = op.side >= 0 && op.size > 0 ? static_cast<uint32_t>(op.side) + 1u : 0u;
            instance.color = op.color;
            instance.kind = op.kind | (side << 8u);
            instance.opacity = 1;
            emitInstance(state, PipelineTier1, nil, instance);
        } else if (kind == PacketOpTile) {
            TileOp op;
            if (!readPacketValue(cursor, end, op)) break;
            if (op.handle >= renderer->tiles.size() ||
                renderer->tiles[static_cast<size_t>(op.handle)].texture == nil) {
                ++stats.skippedOps;
                continue;
            }
            TileRecord& tile = renderer->tiles[static_cast<size_t>(op.handle)];
            Instance instance = {};
            setFullContentMask(renderer, instance);
            instance.rect[0] = static_cast<float>(round((op.x + state.matrix[4]) * renderer->scale));
            instance.rect[1] = static_cast<float>(round((op.y + state.matrix[5]) * renderer->scale));
            instance.rect[2] = static_cast<float>(op.width * renderer->scale);
            instance.rect[3] = static_cast<float>(op.height * renderer->scale);
            instance.params[2] = 1;
            instance.params[3] = 1;
            instance.color = 0xffffffffu;
            instance.kind = 3;
            instance.opacity = op.opacity;
            emitInstance(state, PipelineRGBA, tile.texture, instance);
        } else if (kind == PacketOpRasterTile) {
            RasterTileOp op;
            if (!readPacketValue(cursor, end, op)) break;
            if (op.handle >= renderer->tiles.size() ||
                renderer->tiles[static_cast<size_t>(op.handle)].texture == nil) {
                ++stats.skippedOps;
                continue;
            }
            TileRecord& tile = renderer->tiles[static_cast<size_t>(op.handle)];
            double scale = renderer->scale;
            double left = (op.x * state.matrix[0] + state.matrix[4]) * scale;
            double top = (op.y * state.matrix[3] + state.matrix[5]) * scale;
            double width = op.width * state.matrix[0] * scale;
            double height = op.height * state.matrix[3] * scale;
            if (op.exact) {
                left = round(left);
                top = round(top);
                width = round(width);
                height = round(height);
            }
            double textureWidth = static_cast<double>(tile.texture.width);
            double textureHeight = static_cast<double>(tile.texture.height);
            Instance instance = {};
            setFullContentMask(renderer, instance);
            instance.rect[0] = static_cast<float>(left);
            instance.rect[1] = static_cast<float>(top);
            instance.rect[2] = static_cast<float>(width);
            instance.rect[3] = static_cast<float>(height);
            instance.params[0] = static_cast<float>(op.u / textureWidth);
            instance.params[1] = static_cast<float>(op.v / textureHeight);
            instance.params[2] = static_cast<float>(op.uWidth / textureWidth);
            instance.params[3] = static_cast<float>(op.vHeight / textureHeight);
            instance.color = 0xffffffffu;
            instance.kind = op.exact ? 3u : 7u;
            instance.opacity = 1;
            emitInstance(state, PipelineRGBA, tile.texture, instance);
        } else if (kind == PacketOpSprite) {
            SpriteOp op;
            if (!readPacketValue(cursor, end, op)) break;
            auto found = renderer->tileByKey.find(op.key);
            if (found == renderer->tileByKey.end()) {
                ++stats.skippedOps;
                continue;
            }
            TileRecord& tile = renderer->tiles[found->second];
            if (tile.texture == nil || tile.generation != op.generation || tile.renderedFrame < 0) {
                ++stats.skippedOps;
                continue;
            }
            tile.lastUsedFrame = renderer->frameCount;
            const double* m = state.matrix;
            double scale = renderer->scale;
            double width = tile.pointsWidth;
            double height = tile.pointsHeight;
            double axisU[2] = {m[0] * width * scale, m[1] * width * scale};
            double axisV[2] = {m[2] * height * scale, m[3] * height * scale};
            if (!(std::fabs(axisU[0] * axisV[1] - axisU[1] * axisV[0]) > 1e-9)) {
                continue;
            }
            double originX = (m[0] * op.x + m[2] * op.y + m[4]) * scale;
            double originY = (m[1] * op.x + m[3] * op.y + m[5]) * scale;
            if (std::fabs(axisU[1]) < 1e-6 && std::fabs(axisV[0]) < 1e-6 &&
                    std::fabs(axisU[0] - tile.pixelWidth) < 1e-3 &&
                    std::fabs(axisV[1] - tile.pixelHeight) < 1e-3) {
                originX = round(originX);
                originY = round(originY);
                axisU[0] = tile.pixelWidth;
                axisV[1] = tile.pixelHeight;
            }
            Instance instance = {};
            setFullContentMask(renderer, instance);
            instance.rect[0] = static_cast<float>(originX);
            instance.rect[1] = static_cast<float>(originY);
            instance.rect[2] = static_cast<float>(axisU[0]);
            instance.rect[3] = static_cast<float>(axisU[1]);
            instance.params[0] = static_cast<float>(axisV[0]);
            instance.params[1] = static_cast<float>(axisV[1]);
            instance.params[2] = 0;
            instance.params[3] = 0;
            float uvSize[2] = {1, 1};
            memcpy(instance.padding, uvSize, sizeof(uvSize));
            instance.color = 0xffffffffu;
            instance.kind = (op.sampling ? 8u : 7u) | (0xfu << 8) | kInstanceAffine;
            instance.opacity = op.opacity;
            emitInstance(state, PipelineRGBA, tile.texture, instance);
        } else if (kind == PacketOpGlyphRun) {
            GlyphRunOp op;
            if (!readPacketValue(cursor, end, op)) break;
            expandGlyphRun(state, op, stats);
        } else if (kind == PacketOpImage) {
            ImageOp op;
            if (!readPacketValue(cursor, end, op)) break;
            expandImage(state, op, stats);
        } else if (kind == PacketOpFill) {
            FillOp op;
            if (!readPacketValue(cursor, end, op)) break;
            size_t paletteBytes = static_cast<size_t>(op.paletteCount) * 16;
            if (static_cast<size_t>(end - cursor) < paletteBytes) {
                break;
            }
            const uint8_t* palette = cursor;
            cursor += paletteBytes;
            expandFill(state, op, palette, stats);
        } else if (kind == PacketOpPath) {
            PathOp op;
            if (!readPacketValue(cursor, end, op)) break;
            expandPath(state, op, stats);
        } else if (kind == PacketOpPushClip || kind == PacketOpPushRoundedClip) {
            ClipOp op;
            RoundedClipOp rounded = {};
            if (kind == PacketOpPushRoundedClip) {
                if (!readPacketValue(cursor, end, rounded)) break;
                op = rounded.bounds;
            } else if (!readPacketValue(cursor, end, op)) break;
            if (!pushExpandClip(state)) {
                continue;
            }
            double local[4] = {op.x, op.y, op.x + op.width, op.y + op.height};
            double bounds[4];
            transformBounds(state.matrix, local, bounds);
            intersectExpandClip(state, local);
            if (kind == PacketOpPushClip && matrixRotates(state.matrix)) {
                pushRectCoverage(state, local);
            }
            if (kind == PacketOpPushRoundedClip) {
                RoundedClip clip = {};
                for (int side = 0; side < 4; ++side) {
                    clip.bounds[side] = static_cast<float>(bounds[side] * renderer->scale);
                    double axis = side % 2 ? state.matrix[3] : state.matrix[0];
                    clip.radii[side] = static_cast<float>(
                        rounded.radii[side] * std::fabs(axis) * renderer->scale
                    );
                }
                clip.parent = state.roundedClip;
                renderer->roundedClips.push_back(clip);
                state.roundedClip = static_cast<uint32_t>(renderer->roundedClips.size());
            }
        } else if (kind == PacketOpPushPathClip) {
            PathClipOp op;
            if (!readPacketValue(cursor, end, op)) break;
            if (!pushExpandClip(state)) {
                continue;
            }
            const FillPathRecord* path = chunkClipPath(renderer, op);
            if (!path) {
                ++stats.skippedOps;
                state.clip = {0, 0, 0, 0};
                continue;
            }
            double local[4] = {
                path->originX,
                path->originY,
                path->originX + path->width,
                path->originY + path->height,
            };
            intersectExpandClip(state, local);
            MTLScissorRect scissor = clipToScissor(renderer, state.clip);
            if (scissor.width && scissor.height) {
                const double identity[6] = {1, 0, 0, 1, 0, 0};
                pushPathCoverage(state, *path, identity, op.fillRule ? 1u : 0u, scissor);
            }
        } else if (kind == PacketOpPushMaskClip) {
            MaskClipOp op;
            if (!readPacketValue(cursor, end, op)) break;
            if (!pushExpandClip(state)) {
                continue;
            }
            double local[4] = {op.x, op.y, op.x + op.width, op.y + op.height};
            double bounds[4];
            transformBounds(state.matrix, local, bounds);
            double outset = 1 / renderer->scale;
            state.clip.left = std::max(state.clip.left, bounds[0] - outset);
            state.clip.top = std::max(state.clip.top, bounds[1] - outset);
            state.clip.right = std::min(state.clip.right, bounds[2] + outset);
            state.clip.bottom = std::min(state.clip.bottom, bounds[3] + outset);
            auto found = renderer->imageEntries.find(canvasImageKey(op.serial, 0, 0, true));
            if (found == renderer->imageEntries.end() || !found->second.valid) {
                ++stats.skippedOps;
                state.clip = {0, 0, 0, 0};
                continue;
            }
            AtlasEntry& entry = found->second;
            entry.lastUsedFrame = renderer->frameCount;
            renderer->imageAtlas.pages[entry.page].lastUsedFrame = renderer->frameCount;
            double device[6] = {renderer->scale, 0, 0, renderer->scale, 0, 0};
            const double placement[6] = {op.width, 0, 0, op.height, op.x, op.y};
            fillMultiply(device, state.matrix, device);
            fillMultiply(device, placement, device);
            uint32_t paint = appendImagePaint(renderer, entry, true, device);
            if (!paint) {
                state.clip = {0, 0, 0, 0};
                continue;
            }
            state.coverageClip = appendCoverage(
                renderer,
                paint & ~FillPaintTranslucent,
                state.coverageClip,
                FillClipMask
            );
        } else if (kind == PacketOpPushLayerMaskClip) {
            MaskOp op;
            if (!readPacketValue(cursor, end, op)) break;
            if (!pushExpandClip(state)) {
                continue;
            }
            const MaskRegion* region = op.id < renderer->maskRegions.size()
                ? &renderer->maskRegions[op.id]
                : nullptr;
            if (!region || !region->valid) {
                state.clip = {0, 0, 0, 0};
                continue;
            }
            double scale = renderer->scale;
            state.clip.left = std::max(state.clip.left, region->bounds[0] / scale);
            state.clip.top = std::max(state.clip.top, region->bounds[1] / scale);
            state.clip.right = std::min(state.clip.right, region->bounds[2] / scale);
            state.clip.bottom = std::min(state.clip.bottom, region->bounds[3] / scale);
            for (uint32_t depth = 0; depth < state.layerDepth; ++depth) {
                LayerPass& owner = *renderer->layers[state.layerStack[depth]];
                if (owner.mask) {
                    owner.sampledPages |= 1u << region->page;
                }
            }
            ensureVectorCapacity(renderer, renderer->coverageClips, renderer->coverageClips.size() + 1);
            renderer->coverageClips.push_back(CoverageClipRecord{
                region->bounds[0] | (region->bounds[1] << 16),
                state.coverageClip,
                FillClipLayerMask | (region->page << 8),
                region->bounds[2] | (region->bounds[3] << 16),
            });
            state.coverageClip = static_cast<uint32_t>(renderer->coverageClips.size());
        } else if (kind == PacketOpPopClip) {
            if (state.clipOverflow) {
                --state.clipOverflow;
            } else if (state.clipDepth) {
                --state.clipDepth;
                state.clip = state.clipStack[state.clipDepth];
                state.roundedClip = state.roundedClipStack[state.clipDepth];
                state.coverageClip = state.coverageClipStack[state.clipDepth];
            }
        } else if (kind == PacketOpPushTranslate || kind == PacketOpPushTransform ||
                   kind == PacketOpPushContentTransform) {
            double matrix[6] = {1, 0, 0, 1, 0, 0};
            if (kind == PacketOpPushTranslate) {
                TranslateOp op;
                if (!readPacketValue(cursor, end, op)) break;
                matrix[4] = op.x;
                matrix[5] = op.y;
            } else {
                TransformOp op;
                if (!readPacketValue(cursor, end, op)) break;
                memcpy(matrix, op.matrix, sizeof(matrix));
            }
            if (state.matrixDepth >= kExpandDepth) {
                ++state.matrixOverflow;
                continue;
            }
            pushExpandMatrix(state);
            fillMultiply(state.matrix, matrix, state.matrix);
            if (kind == PacketOpPushContentTransform) {
                state.paper[0] = std::round(state.matrix[4] * state.renderer->scale);
                state.paper[1] = std::round(state.matrix[5] * state.renderer->scale);
            }
        } else if (kind == PacketOpPopTransform) {
            if (state.matrixOverflow) {
                --state.matrixOverflow;
            } else if (state.matrixDepth) {
                popExpandMatrix(state);
            }
        } else if (kind == PacketOpPushLayer || kind == PacketOpPushMaskLayer || kind == PacketOpPushBlurLayer ||
                   kind == PacketOpPushTransformLayer) {
            LayerOp op = {1};
            MaskOp mask = {0};
            BlurLayerOp blur = {0};
            TransformLayerOp turn = {{1, 0, 0, 1, 0, 0}, 1};
            if (kind == PacketOpPushLayer) {
                if (!readPacketValue(cursor, end, op)) break;
            } else if (kind == PacketOpPushMaskLayer) {
                if (!readPacketValue(cursor, end, mask)) break;
            } else if (kind == PacketOpPushTransformLayer) {
                if (!readPacketValue(cursor, end, turn)) break;
                op.alpha = turn.alpha;
                if (turn.alpha >= 1 && !state.blend && state.matrixDepth < kExpandDepth &&
                        inlinesTransformLayer(renderer, cursor, end)) {
                    pushExpandMatrix(state);
                    fillMultiply(state.matrix, turn.matrix, state.matrix);
                    state.inlineLayer = true;
                    continue;
                }
            } else {
                if (!readPacketValue(cursor, end, blur)) break;
            }
            if (state.layerDepth >= kExpandDepth) {
                ++state.layerOverflow;
                continue;
            }
            uint32_t index = renderer->layerCount++;
            if (index >= renderer->layers.size()) {
                renderer->layers.push_back(std::unique_ptr<LayerPass>(new LayerPass()));
                countAllocation(renderer);
            }
            LayerPass& layer = *renderer->layers[index];
            layer.batches.clear();
            layer.empty = true;
            layer.alpha = std::isfinite(op.alpha) ? std::min(std::max(op.alpha, 0.0f), 1.0f) : 1.0f;
            layer.mask = kind == PacketOpPushMaskLayer;
            layer.maskId = mask.id;
            layer.sampledPages = 0;
            layer.blend = state.blend;
            layer.blur = 0;
            layer.blurred = kind == PacketOpPushBlurLayer;
            layer.clipOverflow = false;
            layer.deferred = false;
            layer.filter = 0;
            layer.filterArgs.clear();
            layer.filterImages.clear();
            layer.transformed = kind == PacketOpPushTransformLayer;
            if (layer.transformed) {
                layerComposite(state.matrix, turn.matrix, layer.composite);
                if (pushExpandClip(state)) {
                    state.clip = {0, 0, static_cast<double>(renderer->width), static_cast<double>(renderer->height)};
                    state.roundedClip = 0;
                    state.coverageClip = 0;
                } else {
                    layer.clipOverflow = true;
                }
            } else if (layer.blurred) {
                double determinant = state.matrix[0] * state.matrix[3] - state.matrix[1] * state.matrix[2];
                double spread = std::isfinite(blur.radius) && blur.radius > 0
                    ? ceil(blur.radius * std::sqrt(std::fabs(determinant)) * renderer->scale)
                    : 0;
                layer.blur = static_cast<uint32_t>(std::min<double>(spread, kMaxLayerBlur));
                if (pushExpandClip(state)) {
                    state.clip = {0, 0, static_cast<double>(renderer->width), static_cast<double>(renderer->height)};
                    state.roundedClip = 0;
                    state.coverageClip = 0;
                } else {
                    layer.clipOverflow = true;
                }
            } else if (!layer.mask) {
                if (pushExpandClip(state)) {
                    state.roundedClip = 0;
                    state.coverageClip = 0;
                } else {
                    layer.clipOverflow = true;
                }
            }
            state.blend = 0;
            state.layerSinks[state.layerDepth] = state.batchSink;
            state.layerStack[state.layerDepth++] = index;
            state.batchSink = &layer.batches;
        } else if (kind == PacketOpPushFilterLayer) {
            FilterLayerOp op;
            if (!readPacketValue(cursor, end, op)) break;
            size_t argBytes = static_cast<size_t>(op.argCount) * sizeof(float);
            if (static_cast<size_t>(end - cursor) < argBytes) {
                cursor = end;
                break;
            }
            const uint8_t* argData = cursor;
            cursor += argBytes;
            size_t frameBytes = static_cast<size_t>(op.imageCount) * 4 * sizeof(float);
            if (static_cast<size_t>(end - cursor) < frameBytes) {
                cursor = end;
                break;
            }
            const uint8_t* frameData = cursor;
            cursor += frameBytes;
            if (state.layerDepth >= kExpandDepth) {
                ++state.layerOverflow;
                continue;
            }
            uint32_t index = renderer->layerCount++;
            if (index >= renderer->layers.size()) {
                renderer->layers.push_back(std::unique_ptr<LayerPass>(new LayerPass()));
                countAllocation(renderer);
            }
            LayerPass& layer = *renderer->layers[index];
            layer.batches.clear();
            layer.empty = true;
            layer.alpha = std::isfinite(op.alpha) ? std::min(std::max(op.alpha, 0.0f), 1.0f) : 1.0f;
            layer.mask = false;
            layer.maskId = 0;
            layer.sampledPages = 0;
            layer.blend = state.blend;
            layer.blur = 0;
            layer.blurred = false;
            layer.clipOverflow = false;
            layer.deferred = false;
            layer.transformed = false;
            layer.filter = ensureShader(renderer, ShaderRoleFilter, op.shader) ? op.shader : 0;
            layer.filterTime = std::isfinite(op.time) ? op.time : 0.0f;
            layer.filterArgs.resize(op.argCount);
            if (op.argCount) {
                memcpy(layer.filterArgs.data(), argData, argBytes);
            }
            layer.filterImages.clear();
            for (uint32_t image = 0; image < op.imageCount; ++image) {
                size_t slot = static_cast<size_t>(op.imageStart) + image;
                layer.filterImages.push_back(slot < packet->images.size() ? packet->images[slot] : nil);
            }
            layer.filterFrames.resize(static_cast<size_t>(op.imageCount) * 4);
            if (op.imageCount) {
                memcpy(layer.filterFrames.data(), frameData, frameBytes);
            }
            const double local[4] = {op.x, op.y, op.x + std::max(op.width, 0.0f), op.y + std::max(op.height, 0.0f)};
            double bounds[4];
            transformBounds(state.matrix, local, bounds);
            double scale = renderer->scale;
            layer.filterRegion[0] = bounds[0] * scale;
            layer.filterRegion[1] = bounds[1] * scale;
            layer.filterRegion[2] = bounds[2] * scale;
            layer.filterRegion[3] = bounds[3] * scale;
            double determinant = state.matrix[0] * state.matrix[3] - state.matrix[1] * state.matrix[2];
            layer.filterScale = scale * std::sqrt(std::fabs(determinant));
            for (int axis = 0; axis < 2; ++axis) {
                float resolution = op.resolution[axis];
                bool reduces = std::isfinite(resolution) && resolution > 0 && resolution < 1;
                layer.filterResolution[axis] = reduces ? resolution : 1;
            }
            layer.filterKeepCount = std::min(op.keepCount, kMaxFilterKeeps);
            memcpy(layer.filterKeeps, op.keeps, sizeof(layer.filterKeeps));
            if (pushExpandClip(state)) {
                state.clip = {bounds[0], bounds[1], bounds[2], bounds[3]};
                state.roundedClip = 0;
                state.coverageClip = 0;
            } else {
                layer.clipOverflow = true;
            }
            state.blend = 0;
            state.layerSinks[state.layerDepth] = state.batchSink;
            state.layerStack[state.layerDepth++] = index;
            state.batchSink = &layer.batches;
        } else if (kind == PacketOpPopLayer) {
            if (state.inlineLayer) {
                state.inlineLayer = false;
                popExpandMatrix(state);
            } else if (state.layerOverflow) {
                --state.layerOverflow;
            } else if (state.layerDepth) {
                --state.layerDepth;
                state.batchSink = state.layerSinks[state.layerDepth];
                popLayerClip(state, *renderer->layers[state.layerStack[state.layerDepth]]);
                state.blend = renderer->layers[state.layerStack[state.layerDepth]]->blend;
                finishLayer(state, state.layerStack[state.layerDepth]);
            }
        } else if (kind == PacketOpPushBlend) {
            BlendOp op;
            if (!readPacketValue(cursor, end, op)) break;
            if (state.blendDepth >= kExpandDepth) {
                ++state.blendOverflow;
                continue;
            }
            state.blendStack[state.blendDepth++] = state.blend;
            state.blend = op.mode <= 27 ? op.mode : 0;
        } else if (kind == PacketOpPopBlend) {
            if (state.blendOverflow) {
                --state.blendOverflow;
            } else if (state.blendDepth) {
                state.blend = state.blendStack[--state.blendDepth];
            }
        } else if (kind == PacketOpBackdrop) {
            BackdropOp op;
            if (!readPacketValue(cursor, end, op)) break;
            expandBackdrop(state, op, stats);
        } else if (kind == PacketOpBackdropShader) {
            BackdropShaderOp op;
            if (!readPacketValue(cursor, end, op)) break;
            expandBackdropShader(state, op, stats);
        } else if (kind == PacketOpUnsupported) {
            UnsupportedOp op;
            if (!readPacketValue(cursor, end, op)) break;
            (void)op;
            ++stats.skippedOps;
        } else {
            cursor = end;
        }
    }
    while (state.layerDepth) {
        --state.layerDepth;
        state.batchSink = state.layerSinks[state.layerDepth];
        popLayerClip(state, *renderer->layers[state.layerStack[state.layerDepth]]);
        state.blend = renderer->layers[state.layerStack[state.layerDepth]]->blend;
        finishLayer(state, state.layerStack[state.layerDepth]);
    }
    mergeBatches(renderer);
    stats.layerPasses = static_cast<uint32_t>(renderer->layerOrder.size());
    stats.coverageClips = static_cast<uint32_t>(renderer->coverageClips.size());
    stats.skippedOps += state.skippedMasks;
}

static id<MTLBuffer> ensureInstanceBuffer(
    MetalRenderer* renderer,
    __strong id<MTLBuffer>* slot,
    size_t required
) {
    if (*slot != nil && (*slot).length >= required) {
        return *slot;
    }
    size_t capacity = 65536;
    while (capacity < required) {
        capacity *= 2;
    }
    id<MTLBuffer> replacement = [renderer->device newBufferWithLength:capacity
                                                               options:MTLResourceStorageModeShared];
    if (!replacement) {
        return nil;
    }
    *slot = replacement;
    countAllocation(renderer);
    return *slot;
}

static bool fillEnsureBuffer(
    MetalRenderer* renderer,
    __strong id<MTLBuffer>& buffer,
    size_t bytes,
    MTLResourceOptions options
) {
    bytes = std::max<size_t>(bytes, 16);
    if (buffer && buffer.length >= bytes) {
        return true;
    }
    size_t capacity = buffer ? buffer.length : 4096;
    while (capacity < bytes) {
        capacity *= 2;
    }
    id<MTLBuffer> replacement = [renderer->device newBufferWithLength:capacity options:options];
    if (!replacement) {
        return false;
    }
    buffer = replacement;
    countAllocation(renderer);
    return true;
}

static void fillCompactSegments(MetalRenderer* renderer) {
    if (renderer->fillDeadSegments < 65536 ||
        renderer->fillDeadSegments * 2 < renderer->fillSegments.size()) {
        return;
    }
    std::vector<FillSegmentGPU> compacted;
    compacted.reserve(renderer->fillSegments.size() - renderer->fillDeadSegments);
    auto keep = [&](FillGeometryRecord& geometry) {
        if (!geometry.resident) {
            return;
        }
        for (FillPathRecord& path : geometry.paths) {
            uint32_t start = static_cast<uint32_t>(compacted.size());
            compacted.insert(
                compacted.end(),
                renderer->fillSegments.begin() + path.segmentStart,
                renderer->fillSegments.begin() + path.segmentStart + path.segmentCount
            );
            path.segmentStart = start;
        }
    };
    for (FillGeometryRecord& geometry : renderer->fillGeometries) {
        keep(geometry);
    }
    for (ChunkPathSet& set : renderer->chunkPaths) {
        keep(set.geometry);
    }
    keep(renderer->unitRect);
    renderer->fillSegments.swap(compacted);
    renderer->fillDeadSegments = 0;
    renderer->fillSegmentBuffer = nil;
    renderer->fillSegmentsUploaded = 0;
}

static bool fillUploadSegments(MetalRenderer* renderer) {
    size_t count = renderer->fillSegments.size();
    if (renderer->fillSegmentBuffer && renderer->fillSegmentsUploaded == count) {
        return true;
    }
    size_t bytes = std::max<size_t>(count, 1) * sizeof(FillSegmentGPU);
    if (!renderer->fillSegmentBuffer || renderer->fillSegmentBuffer.length < bytes) {
        size_t capacity = renderer->fillSegmentBuffer ? renderer->fillSegmentBuffer.length : 65536;
        while (capacity < bytes) {
            capacity *= 2;
        }
        id<MTLBuffer> replacement = [renderer->device newBufferWithLength:capacity
                                                                   options:MTLResourceStorageModeShared];
        if (!replacement) {
            return false;
        }
        memcpy(replacement.contents, renderer->fillSegments.data(), count * sizeof(FillSegmentGPU));
        renderer->fillSegmentBuffer = replacement;
        countAllocation(renderer);
    } else {
        size_t uploaded = renderer->fillSegmentsUploaded;
        memcpy(
            static_cast<FillSegmentGPU*>(renderer->fillSegmentBuffer.contents) + uploaded,
            renderer->fillSegments.data() + uploaded,
            (count - uploaded) * sizeof(FillSegmentGPU)
        );
    }
    renderer->fillSegmentsUploaded = count;
    return true;
}

static bool fillEnsureDepth(MetalRenderer* renderer, NSUInteger width, NSUInteger height) {
    if (renderer->depthTexture && renderer->depthTexture.width == width &&
        renderer->depthTexture.height == height) {
        return true;
    }
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                     width:width
                                    height:height
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModeMemoryless;
    id<MTLTexture> replacement = [renderer->device newTextureWithDescriptor:descriptor];
    if (!replacement) {
        return false;
    }
    renderer->depthTexture = replacement;
    countAllocation(renderer);
    return true;
}

static bool fillUploadCoverage(MetalRenderer* renderer, FillSlot& slot) {
    const MTLResourceOptions shared = MTLResourceStorageModeShared;
    const std::vector<uint32_t>& drawSlots = renderer->fillDrawSlots;
    size_t clipCount = renderer->coverageClips.size();
    if (clipCount) {
        if (!fillEnsureBuffer(
                renderer,
                slot.coverageClips,
                clipCount * sizeof(CoverageClipRecord),
                shared
            )) {
            return false;
        }
        CoverageClipRecord* records = static_cast<CoverageClipRecord*>(slot.coverageClips.contents);
        for (size_t index = 0; index < clipCount; ++index) {
            records[index] = renderer->coverageClips[index];
            if (records[index].mode & (FillClipMask | FillClipLayerMask | FillClipShadow)) {
                continue;
            }
            uint32_t frameDraw = records[index].draw;
            records[index].draw = frameDraw < drawSlots.size() ? drawSlots[frameDraw] : UINT32_MAX;
        }
    }
    if (!renderer->fillPaints.empty()) {
        size_t paintBytes = renderer->fillPaints.size() * sizeof(FillPaintGPU);
        if (!fillEnsureBuffer(renderer, slot.paints, paintBytes, shared)) {
            return false;
        }
        memcpy(slot.paints.contents, renderer->fillPaints.data(), paintBytes);
    }
    if (!renderer->fillStops.empty()) {
        size_t stopBytes = renderer->fillStops.size() * sizeof(FillStopGPU);
        if (!fillEnsureBuffer(renderer, slot.stops, stopBytes, shared)) {
            return false;
        }
        memcpy(slot.stops.contents, renderer->fillStops.data(), stopBytes);
    }
    if (!renderer->brushDraws.empty()) {
        size_t dabBytes = renderer->brushDabs.size() * sizeof(DabGPU);
        size_t drawBytes = renderer->brushDraws.size() * sizeof(DabDrawGPU);
        if (!fillEnsureBuffer(renderer, slot.dabs, dabBytes, shared) ||
                !fillEnsureBuffer(renderer, slot.dabDraws, drawBytes, shared)) {
            return false;
        }
        memcpy(slot.dabs.contents, renderer->brushDabs.data(), dabBytes);
        memcpy(slot.dabDraws.contents, renderer->brushDraws.data(), drawBytes);
    }
    return true;
}

static bool fillPrepare(MetalRenderer* renderer, FrameStats& stats) {
    renderer->fillActive = false;
    size_t drawCapacity = renderer->fillDraws.size();
    FillSlot& slot = renderer->fillSlots[renderer->packetIndex];
    if (!drawCapacity) {
        renderer->fillDrawSlots.clear();
        return fillUploadCoverage(renderer, slot);
    }
    size_t batchCount = renderer->fillBatches.size();
    const MTLResourceOptions shared = MTLResourceStorageModeShared;
    if (!fillUploadSegments(renderer) ||
        !fillEnsureBuffer(renderer, slot.draws, drawCapacity * sizeof(FillDrawGPU), shared) ||
        !fillEnsureBuffer(renderer, slot.segmentBases, (drawCapacity + 1) * 4, shared) ||
        !fillEnsureBuffer(renderer, slot.tileBases, (drawCapacity + 1) * 4, shared) ||
        !fillEnsureBuffer(renderer, slot.rowBases, (drawCapacity + 1) * 4, shared) ||
        !fillEnsureBuffer(renderer, slot.ranges, batchCount * 16, shared) ||
        !fillEnsureBuffer(renderer, slot.stats, 64, shared)) {
        return false;
    }
    FillDrawGPU* draws = static_cast<FillDrawGPU*>(slot.draws.contents);
    uint32_t* segmentBases = static_cast<uint32_t*>(slot.segmentBases.contents);
    uint32_t* tileBases = static_cast<uint32_t*>(slot.tileBases.contents);
    uint32_t* rowBases = static_cast<uint32_t*>(slot.rowBases.contents);
    uint32_t visible = 0;
    uint64_t segmentTotal = 0;
    uint64_t tileTotal = 0;
    uint64_t rowTotal = 0;
    std::vector<uint32_t>& drawSlots = renderer->fillDrawSlots;
    ensureVectorCapacity(renderer, drawSlots, drawCapacity);
    drawSlots.assign(drawCapacity, UINT32_MAX);
    const std::vector<uint32_t>& order = renderer->fillOrder;
    bool ordered = order.size() == drawCapacity;
    for (size_t position = 0; position < drawCapacity; ++position) {
        size_t frameDraw = ordered ? order[position] : position;
        const FrameFillDraw& source = renderer->fillDraws[frameDraw];
        const MTLScissorRect& scissor = renderer->fillBatches[source.batch].scissor;
        if (!scissor.width || !scissor.height) {
            continue;
        }
        double extent[4];
        if (!fillDrawExtent(source, scissor, extent)) {
            continue;
        }
        const double* device = source.device;
        double originX = device[0] * source.originX + device[2] * source.originY + device[4];
        double originY = device[1] * source.originX + device[3] * source.originY + device[5];
        int32_t tileX = static_cast<int32_t>(floor(extent[0] / 16));
        int32_t tileY = static_cast<int32_t>(floor(extent[1] / 16));
        uint32_t tileW = static_cast<uint32_t>(static_cast<int32_t>(ceil(extent[2] / 16)) - tileX);
        uint32_t tileH = static_cast<uint32_t>(static_cast<int32_t>(ceil(extent[3] / 16)) - tileY);
        FillDrawGPU& draw = draws[visible];
        memcpy(draw.color, source.color, sizeof(draw.color));
        memcpy(draw.mask, source.mask, sizeof(draw.mask));
        if (source.blend) {
            draw.extent[0] = static_cast<float>(tileX * 16);
            draw.extent[1] = static_cast<float>(tileY * 16);
            draw.extent[2] = static_cast<float>((tileX + static_cast<int32_t>(tileW)) * 16);
            draw.extent[3] = static_cast<float>((tileY + static_cast<int32_t>(tileH)) * 16);
        } else {
            draw.extent[0] = static_cast<float>(floor(extent[0]));
            draw.extent[1] = static_cast<float>(floor(extent[1]));
            draw.extent[2] = static_cast<float>(ceil(extent[2]));
            draw.extent[3] = static_cast<float>(ceil(extent[3]));
        }
        draw.a = static_cast<float>(device[0]);
        draw.b = static_cast<float>(device[1]);
        draw.c = static_cast<float>(device[2]);
        draw.d = static_cast<float>(device[3]);
        draw.tx = static_cast<float>(originX);
        draw.ty = static_cast<float>(originY);
        draw.segmentStart = source.segmentStart;
        draw.segmentCount = source.segmentCount;
        draw.segmentBase = static_cast<uint32_t>(segmentTotal);
        draw.fillRule = source.fillRule | (source.blend << 8);
        draw.tileX = tileX;
        draw.tileY = tileY;
        draw.tileW = tileW;
        draw.tileH = tileH;
        draw.tileBase = static_cast<uint32_t>(tileTotal);
        draw.rowBase = static_cast<uint32_t>(rowTotal);
        draw.batch = source.batch;
        draw.roundedClip = source.roundedClip;
        draw.halfWidth = source.halfWidth;
        draw.strokeStyle = source.strokeStyle;
        draw.clip = source.clip;
        draw.coverage = source.coverage;
        draw.dash = source.dash;
        draw.paint = source.paint;
        segmentBases[visible] = static_cast<uint32_t>(segmentTotal);
        tileBases[visible] = static_cast<uint32_t>(tileTotal);
        rowBases[visible] = static_cast<uint32_t>(rowTotal);
        segmentTotal += source.segmentCount;
        tileTotal += static_cast<uint64_t>(tileW) * tileH;
        rowTotal += tileH;
        drawSlots[frameDraw] = visible;
        ++visible;
    }
    if (!fillUploadCoverage(renderer, slot)) {
        return false;
    }
    segmentBases[visible] = static_cast<uint32_t>(segmentTotal);
    tileBases[visible] = static_cast<uint32_t>(tileTotal);
    rowBases[visible] = static_cast<uint32_t>(rowTotal);
    stats.fillDraws = visible;
    stats.fillSegments = static_cast<int64_t>(segmentTotal);
    stats.fillTiles = static_cast<int64_t>(tileTotal);
    if (!tileTotal || tileTotal > 0xffffffffull) {
        return true;
    }
    uint64_t groups = (tileTotal + 255) / 256;
    uint32_t gridWidth = static_cast<uint32_t>(std::max(renderer->pixelWidth, 0) + 15) / 16;
    uint32_t gridHeight = static_cast<uint32_t>(std::max(renderer->pixelHeight, 0) + 15) / 16;
    uint64_t cells = static_cast<uint64_t>(gridWidth) * gridHeight;
    uint64_t estimate = std::max<uint64_t>({
        renderer->fillPieceHigh.load() * 5 / 4,
        segmentTotal * 48,
        65536,
    });
    estimate = std::min<uint64_t>(estimate, 1ull << 26);
    renderer->fillPieceCapacity = std::max<uint32_t>(
        renderer->fillPieceCapacity,
        static_cast<uint32_t>(estimate)
    );
    const MTLResourceOptions gpu = MTLResourceStorageModePrivate;
    size_t tileBytes = static_cast<size_t>(tileTotal) * 4;
    if (!fillEnsureBuffer(renderer, renderer->fillTileDraws, tileBytes, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillCounts, tileBytes, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillBackdrops, tileBytes, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillStarts, tileBytes, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillCursors, tileBytes, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillVisibleTiles, tileBytes, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillOpaqueTiles, tileBytes, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillGroupSums, groups * 4, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillGroupOpaque, groups * 4, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillTileKinds, static_cast<size_t>(tileTotal), gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillOccluders, static_cast<size_t>(cells) * 4, gpu) ||
        !fillEnsureBuffer(renderer, renderer->fillArgs, batchCount * 32, gpu) ||
        !fillEnsureBuffer(
            renderer,
            renderer->fillPieces,
            static_cast<size_t>(renderer->fillPieceCapacity) * 16,
            gpu
        )) {
        return false;
    }
    memset(slot.ranges.contents, 0, batchCount * 16);
    memset(slot.stats.contents, 0, 64);
    FillParamsGPU params = {};
    params.drawCount = visible;
    params.tileCount = static_cast<uint32_t>(tileTotal);
    params.segmentCount = static_cast<uint32_t>(segmentTotal);
    params.rowCount = static_cast<uint32_t>(rowTotal);
    params.pieceCapacity = static_cast<uint32_t>(std::min<NSUInteger>(
        renderer->fillPieces.length / 16,
        0xffffffffu
    ));
    params.batchCount = static_cast<uint32_t>(batchCount);
    params.tolerance = 0.05f;
    params.miterLimit = static_cast<float>(fillMiterLimit);
    params.viewportWidth = static_cast<float>(renderer->pixelWidth);
    params.viewportHeight = static_cast<float>(renderer->pixelHeight);
    params.gridWidth = gridWidth;
    params.gridHeight = gridHeight;
    renderer->fillParams = params;
    renderer->fillActive = true;
    return true;
}

static void fillDispatch(
    id<MTLComputeCommandEncoder> encoder,
    id<MTLComputePipelineState> pipeline,
    uint32_t threads
) {
    if (!threads) {
        return;
    }
    [encoder setComputePipelineState:pipeline];
    [encoder dispatchThreads:MTLSizeMake(threads, 1, 1)
        threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];
}

static id<MTLBuffer> coverageBuffer(MetalRenderer* renderer) {
    return renderer->coverageClips.empty()
        ? renderer->emptyBuffer
        : renderer->fillSlots[renderer->packetIndex].coverageClips;
}

static void fillEncodeCompute(MetalRenderer* renderer, id<MTLCommandBuffer> commandBuffer) {
    const FillParamsGPU& params = renderer->fillParams;
    FillSlot& slot = renderer->fillSlots[renderer->packetIndex];
    __strong id<MTLComputePipelineState>* kernels = renderer->shared->fillKernels;
    uint32_t groupCount = (params.tileCount + 255) / 256;
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    [encoder setBytes:&params length:sizeof(params) atIndex:0];

    [encoder setBuffer:slot.tileBases offset:0 atIndex:1];
    [encoder setBuffer:renderer->fillTileDraws offset:0 atIndex:2];
    [encoder setBuffer:renderer->fillCounts offset:0 atIndex:3];
    [encoder setBuffer:renderer->fillBackdrops offset:0 atIndex:4];
    [encoder setBuffer:renderer->fillCursors offset:0 atIndex:5];
    [encoder setBuffer:renderer->fillOccluders offset:0 atIndex:6];
    fillDispatch(
        encoder,
        kernels[FillKernelInit],
        std::max(params.tileCount, params.gridWidth * params.gridHeight)
    );

    [encoder setBuffer:slot.draws offset:0 atIndex:1];
    [encoder setBuffer:slot.segmentBases offset:0 atIndex:2];
    [encoder setBuffer:renderer->fillSegmentBuffer offset:0 atIndex:3];
    [encoder setBuffer:renderer->fillCounts offset:0 atIndex:4];
    [encoder setBuffer:renderer->fillBackdrops offset:0 atIndex:5];
    [encoder setBuffer:slot.stats offset:0 atIndex:6];
    fillDispatch(encoder, kernels[FillKernelCount], params.segmentCount);

    [encoder setBuffer:renderer->fillCounts offset:0 atIndex:1];
    [encoder setBuffer:renderer->fillStarts offset:0 atIndex:2];
    [encoder setBuffer:slot.stats offset:0 atIndex:3];
    [encoder setComputePipelineState:kernels[FillKernelAllocate]];
    [encoder dispatchThreadgroups:MTLSizeMake(groupCount, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];

    [encoder setBuffer:slot.draws offset:0 atIndex:1];
    [encoder setBuffer:slot.segmentBases offset:0 atIndex:2];
    [encoder setBuffer:renderer->fillSegmentBuffer offset:0 atIndex:3];
    [encoder setBuffer:renderer->fillStarts offset:0 atIndex:4];
    [encoder setBuffer:renderer->fillCursors offset:0 atIndex:5];
    [encoder setBuffer:renderer->fillPieces offset:0 atIndex:6];
    [encoder setBuffer:slot.stats offset:0 atIndex:7];
    fillDispatch(encoder, kernels[FillKernelWrite], params.segmentCount);

    [encoder setBuffer:slot.draws offset:0 atIndex:1];
    [encoder setBuffer:slot.rowBases offset:0 atIndex:2];
    [encoder setBuffer:renderer->fillBackdrops offset:0 atIndex:3];
    fillDispatch(encoder, kernels[FillKernelBackdrop], params.rowCount);

    [encoder setBuffer:slot.draws offset:0 atIndex:1];
    [encoder setBuffer:renderer->fillTileDraws offset:0 atIndex:2];
    [encoder setBuffer:renderer->fillCounts offset:0 atIndex:3];
    [encoder setBuffer:renderer->fillBackdrops offset:0 atIndex:4];
    [encoder setBuffer:renderer->fillTileKinds offset:0 atIndex:5];
    [encoder setBuffer:renderer->fillOccluders offset:0 atIndex:6];
    [encoder setBuffer:coverageBuffer(renderer) offset:0 atIndex:9];
    fillDispatch(encoder, kernels[FillKernelOcclude], params.tileCount);

    [encoder setBuffer:renderer->fillTileKinds offset:0 atIndex:3];
    [encoder setBuffer:renderer->fillOccluders offset:0 atIndex:4];
    [encoder setBuffer:renderer->fillGroupSums offset:0 atIndex:5];
    [encoder setBuffer:slot.stats offset:0 atIndex:6];
    [encoder setBuffer:renderer->fillGroupOpaque offset:0 atIndex:7];
    [encoder setComputePipelineState:kernels[FillKernelMark]];
    [encoder dispatchThreadgroups:MTLSizeMake(groupCount, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];

    [encoder setComputePipelineState:kernels[FillKernelScanGroups]];
    [encoder setBytes:&groupCount length:sizeof(groupCount) atIndex:3];
    [encoder setBuffer:renderer->fillGroupSums offset:0 atIndex:1];
    [encoder setBuffer:slot.stats offset:24 atIndex:2];
    [encoder dispatchThreadgroups:MTLSizeMake(1, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];
    [encoder setBuffer:renderer->fillGroupOpaque offset:0 atIndex:1];
    [encoder setBuffer:slot.stats offset:28 atIndex:2];
    [encoder dispatchThreadgroups:MTLSizeMake(1, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];

    [encoder setBuffer:slot.draws offset:0 atIndex:1];
    [encoder setBuffer:renderer->fillTileDraws offset:0 atIndex:2];
    [encoder setBuffer:renderer->fillTileKinds offset:0 atIndex:3];
    [encoder setBuffer:renderer->fillOccluders offset:0 atIndex:4];
    [encoder setBuffer:renderer->fillGroupSums offset:0 atIndex:5];
    [encoder setBuffer:renderer->fillVisibleTiles offset:0 atIndex:6];
    [encoder setBuffer:renderer->fillGroupOpaque offset:0 atIndex:7];
    [encoder setBuffer:renderer->fillOpaqueTiles offset:0 atIndex:8];
    [encoder setComputePipelineState:kernels[FillKernelScatter]];
    [encoder dispatchThreadgroups:MTLSizeMake(groupCount, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];

    [encoder setBuffer:slot.draws offset:0 atIndex:1];
    [encoder setBuffer:renderer->fillTileDraws offset:0 atIndex:2];
    [encoder setBuffer:renderer->fillVisibleTiles offset:0 atIndex:3];
    [encoder setBuffer:renderer->fillOpaqueTiles offset:0 atIndex:4];
    [encoder setBuffer:slot.stats offset:0 atIndex:5];
    [encoder setBuffer:slot.ranges offset:0 atIndex:6];
    fillDispatch(encoder, kernels[FillKernelBatchBounds], params.tileCount);

    [encoder setBuffer:slot.ranges offset:0 atIndex:1];
    [encoder setBuffer:slot.stats offset:0 atIndex:2];
    [encoder setBuffer:renderer->fillArgs offset:0 atIndex:3];
    fillDispatch(encoder, kernels[FillKernelBatchArgs], params.batchCount);
    [encoder endEncoding];
}

struct PassTarget {
    MTLScissorRect area;
    float viewport[4];
    int32_t maskPage;
    MTLPixelFormat format;
    const MTLScissorRect* areas;
    size_t areaCount;
};

static FillParamsGPU passFillParams(MetalRenderer* renderer, const PassTarget& target) {
    FillParamsGPU params = renderer->fillActive ? renderer->fillParams : FillParamsGPU{};
    params.originX = target.viewport[0];
    params.originY = target.viewport[1];
    params.viewportWidth = target.viewport[2];
    params.viewportHeight = target.viewport[3];
    return params;
}

static void bindCoverageFragments(
    MetalRenderer* renderer,
    id<MTLRenderCommandEncoder> encoder,
    const PassTarget& target
) {
    FillSlot& slot = renderer->fillSlots[renderer->packetIndex];
    bool active = renderer->fillActive;
    id<MTLBuffer> empty = renderer->emptyBuffer;
    FillParamsGPU params = passFillParams(renderer, target);
    [encoder setFragmentBuffer:active ? renderer->fillPieces : empty offset:0 atIndex:1];
    [encoder setFragmentBuffer:active ? renderer->fillStarts : empty offset:0 atIndex:2];
    [encoder setFragmentBuffer:active ? renderer->fillCounts : empty offset:0 atIndex:3];
    [encoder setFragmentBuffer:active ? renderer->fillBackdrops : empty offset:0 atIndex:4];
    [encoder setFragmentBytes:&params length:sizeof(params) atIndex:5];
    [encoder setFragmentBuffer:active ? slot.draws : empty offset:0 atIndex:6];
    [encoder setFragmentBuffer:coverageBuffer(renderer) offset:0 atIndex:7];
    [encoder setFragmentBuffer:active ? slot.stats : empty offset:0 atIndex:8];
    bool paints = !renderer->fillPaints.empty();
    bool stops = !renderer->fillStops.empty();
    [encoder setFragmentBuffer:paints && slot.paints ? slot.paints : empty offset:0 atIndex:9];
    [encoder setFragmentBuffer:stops && slot.stops ? slot.stops : empty offset:0 atIndex:10];
    const std::vector<AtlasPage>& pages = renderer->imageAtlas.pages;
    for (NSUInteger page = 0; page < 2; ++page) {
        id<MTLTexture> texture = page < pages.size() && pages[page].texture
            ? pages[page].texture
            : renderer->blankTexture;
        [encoder setFragmentTexture:texture atIndex:page + 1];
    }
    for (uint32_t page = 0; page < kMaskPages; ++page) {
        id<MTLTexture> texture = renderer->maskPages[page] && static_cast<int32_t>(page) != target.maskPage
            ? renderer->maskPages[page]
            : renderer->blankTexture;
        [encoder setFragmentTexture:texture atIndex:3 + page];
    }
}

static void encodeFillBatch(
    MetalRenderer* renderer,
    id<MTLRenderCommandEncoder> encoder,
    uint32_t batch,
    const PassTarget& target
) {
    const SurfacePipelines* surface = surfacePipelinesFor(renderer, target.format);
    if (!surface) {
        return;
    }
    FillParamsGPU params = passFillParams(renderer, target);
    FillSlot& slot = renderer->fillSlots[renderer->packetIndex];
    uint32_t selection[2] = {batch, 1};
    [encoder setVertexBytes:&params length:sizeof(params) atIndex:0];
    [encoder setVertexBuffer:slot.draws offset:0 atIndex:1];
    [encoder setVertexBuffer:renderer->fillTileDraws offset:0 atIndex:2];
    [encoder setVertexBuffer:slot.ranges offset:0 atIndex:4];
    const FillBatchInfo& info = renderer->fillBatches[batch];
    const FormatPipeline* custom = nullptr;
    if (info.shader) {
        auto found = renderer->fillShaders.find(info.shader);
        custom = found != renderer->fillShaders.end() && !found->second.failed
            ? fillShaderVariant(renderer->device, found->second, target.format, info.blend)
            : nullptr;
    }
    if (info.blend) {
        selection[1] = 0;
        [encoder setRenderPipelineState:custom ? custom->blend : surface->fillBlend];
        [encoder setDepthStencilState:renderer->shared->depthTest];
        [encoder setVertexBuffer:renderer->fillVisibleTiles offset:0 atIndex:3];
        [encoder setVertexBytes:selection length:sizeof(selection) atIndex:5];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                 indirectBuffer:renderer->fillArgs
           indirectBufferOffset:static_cast<NSUInteger>(batch) * 32];
        [encoder setDepthStencilState:renderer->shared->depthNone];
        return;
    }
    [encoder setRenderPipelineState:surface->fillSolid];
    [encoder setDepthStencilState:renderer->shared->depthWrite];
    [encoder setVertexBuffer:renderer->fillOpaqueTiles offset:0 atIndex:3];
    [encoder setVertexBytes:selection length:sizeof(selection) atIndex:5];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
             indirectBuffer:renderer->fillArgs
       indirectBufferOffset:static_cast<NSUInteger>(batch) * 32 + 16];
    selection[1] = 0;
    [encoder setRenderPipelineState:custom ? custom->render : surface->fillRender];
    [encoder setDepthStencilState:renderer->shared->depthTest];
    [encoder setVertexBuffer:renderer->fillVisibleTiles offset:0 atIndex:3];
    [encoder setVertexBytes:selection length:sizeof(selection) atIndex:5];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
             indirectBuffer:renderer->fillArgs
       indirectBufferOffset:static_cast<NSUInteger>(batch) * 32];
    [encoder setDepthStencilState:renderer->shared->depthNone];
}

static bool encodeDabBatch(
    MetalRenderer* renderer,
    id<MTLRenderCommandEncoder> encoder,
    const Batch& batch,
    const PassTarget& target
) {
    auto found = renderer->dabShaders.find(batch.shader);
    if (found == renderer->dabShaders.end() || found->second.failed) {
        return false;
    }
    const FormatPipeline* variant = dabShaderVariant(renderer->device, found->second, target.format, batch.blend);
    FillSlot& slot = renderer->fillSlots[renderer->packetIndex];
    if (!variant || !slot.dabs || !slot.dabDraws) {
        return false;
    }
    [encoder setRenderPipelineState:batch.blend ? variant->blend : variant->render];
    [encoder setVertexBuffer:slot.dabs offset:0 atIndex:1];
    [encoder setVertexBuffer:slot.dabDraws offset:0 atIndex:2];
    [encoder setFragmentBuffer:slot.dabDraws offset:0 atIndex:11];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                vertexStart:0
                vertexCount:4
              instanceCount:batch.count
               baseInstance:batch.start];
    return true;
}

static void assignFillScissors(
    MetalRenderer* renderer,
    const std::vector<Batch>& batches,
    const MTLScissorRect& area
) {
    for (const Batch& batch : batches) {
        if (batch.pipeline == PipelineFill) {
            renderer->fillBatches[batch.start].scissor = intersectScissors(batch.scissor, area);
        }
    }
}

static void encodeBatches(
    MetalRenderer* renderer,
    id<MTLRenderCommandEncoder> encoder,
    const Batch* batches,
    size_t batchCount,
    id<MTLBuffer> instanceBuffer,
    const PassTarget& target,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches,
    bool binds = true
) {
    const SurfacePipelines* surface = surfacePipelinesFor(renderer, target.format);
    if (!surface) {
        return;
    }
    if (binds) {
        [encoder setFragmentBuffer:renderer->roundedClipBuffers[renderer->packetIndex] offset:0 atIndex:0];
        bindCoverageFragments(renderer, encoder, target);
        [encoder setVertexBytes:target.viewport length:sizeof(target.viewport) atIndex:0];
        [encoder setVertexBuffer:instanceBuffer offset:0 atIndex:1];
        [encoder setDepthStencilState:renderer->shared->depthNone];
    }
    NSUInteger originX = static_cast<NSUInteger>(target.viewport[0]);
    NSUInteger originY = static_cast<NSUInteger>(target.viewport[1]);
    const MTLScissorRect* areas = target.areaCount ? target.areas : &target.area;
    size_t areaCount = target.areaCount ? std::min(target.areaCount, kMaxDamageAreas) : 1;
    uint32_t previousPipeline = UINT32_MAX;
    for (size_t index = 0; index < batchCount; ++index) {
        const Batch& batch = batches[index];
        if (!batch.count) {
            continue;
        }
        MTLScissorRect scissors[kMaxDamageAreas];
        size_t scissorCount = 0;
        for (size_t area = 0; area < areaCount; ++area) {
            MTLScissorRect scissor = intersectScissors(batch.scissor, areas[area]);
            if (!scissor.width || !scissor.height) {
                continue;
            }
            scissor.x -= originX;
            scissor.y -= originY;
            scissors[scissorCount++] = scissor;
        }
        if (!scissorCount) {
            continue;
        }
        if (batch.pipeline == PipelineFill) {
            if (renderer->fillActive && renderer->fillBatches[batch.start].paints) {
                for (size_t area = 0; area < scissorCount; ++area) {
                    [encoder setScissorRect:scissors[area]];
                    encodeFillBatch(renderer, encoder, batch.start, target);
                    drawCalls += 2;
                    pipelineSwitches += 2;
                }
                previousPipeline = UINT32_MAX;
                [encoder setVertexBytes:target.viewport length:sizeof(target.viewport) atIndex:0];
                [encoder setVertexBuffer:instanceBuffer offset:0 atIndex:1];
            }
            continue;
        }
        if (batch.pipeline == PipelineDab) {
            bool drew = false;
            for (size_t area = 0; area < scissorCount; ++area) {
                [encoder setScissorRect:scissors[area]];
                if (encodeDabBatch(renderer, encoder, batch, target)) {
                    ++drawCalls;
                    ++pipelineSwitches;
                    drew = true;
                }
            }
            if (drew) {
                previousPipeline = UINT32_MAX;
                [encoder setVertexBuffer:instanceBuffer offset:0 atIndex:1];
            }
            continue;
        }
        if (batch.shader) {
            auto found = renderer->textShaders.find(batch.shader);
            const FormatPipeline* variant = found != renderer->textShaders.end() && !found->second.failed
                ? textShaderVariant(renderer->device, found->second, target.format, batch.blend)
                : nullptr;
            if (!variant) {
                continue;
            }
            [encoder setRenderPipelineState:batch.blend ? variant->blend : variant->render];
            previousPipeline = UINT32_MAX;
            ++pipelineSwitches;
        } else {
            uint32_t pipeline = batch.pipeline | (batch.blend ? 0x100u : 0u);
            if (pipeline != previousPipeline) {
                [encoder setRenderPipelineState:batch.blend
                    ? surface->blendPipelines[batch.pipeline]
                    : surface->pipelines[batch.pipeline]];
                previousPipeline = pipeline;
                ++pipelineSwitches;
            }
        }
        if (batch.texture) {
            [encoder setFragmentTexture:batch.texture atIndex:0];
        }
        [encoder setVertexBufferOffset:batch.start * sizeof(Instance) atIndex:1];
        for (size_t area = 0; area < scissorCount; ++area) {
            [encoder setScissorRect:scissors[area]];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                        vertexStart:0
                        vertexCount:4
                      instanceCount:batch.count];
            ++drawCalls;
        }
    }
}

static MTLScissorRect intersectScissors(
    const MTLScissorRect& left,
    const MTLScissorRect& right
) {
    NSUInteger x0 = std::max(left.x, right.x);
    NSUInteger y0 = std::max(left.y, right.y);
    NSUInteger x1 = std::min(left.x + left.width, right.x + right.width);
    NSUInteger y1 = std::min(left.y + left.height, right.y + right.height);
    if (x1 <= x0 || y1 <= y0) {
        return MTLScissorRect{0, 0, 0, 0};
    }
    return MTLScissorRect{x0, y0, x1 - x0, y1 - y0};
}

static MTLScissorRect uniteScissors(const MTLScissorRect& left, const MTLScissorRect& right) {
    if (!left.width || !left.height) {
        return right;
    }
    if (!right.width || !right.height) {
        return left;
    }
    NSUInteger x0 = std::min(left.x, right.x);
    NSUInteger y0 = std::min(left.y, right.y);
    NSUInteger x1 = std::max(left.x + left.width, right.x + right.width);
    NSUInteger y1 = std::max(left.y + left.height, right.y + right.height);
    return MTLScissorRect{x0, y0, x1 - x0, y1 - y0};
}

static int64_t recordPresentDamage(
    PresentDamage& damage,
    const MTLScissorRect* areas,
    size_t count
) {
    std::lock_guard<std::mutex> lock(damage.mutex);
    ++damage.serial;
    size_t slot = static_cast<size_t>(damage.serial) % kPresentDamageHistory;
    size_t kept = std::min(count, kMaxDamageAreas);
    for (size_t index = 0; index < kept; ++index) {
        damage.areas[slot * kMaxDamageAreas + index] = areas[index];
    }
    damage.areaCounts[slot] = kept;
    return damage.serial;
}

static void addPresentCopy(std::vector<MTLScissorRect>& copies, const MTLScissorRect& area) {
    if (!area.width || !area.height) {
        return;
    }
    for (MTLScissorRect& copy : copies) {
        MTLScissorRect overlap = intersectScissors(copy, area);
        if (overlap.width && overlap.height) {
            copy = uniteScissors(copy, area);
            return;
        }
    }
    copies.push_back(area);
}

static void forgetPresentBuffers(PresentDamage& damage) {
    std::lock_guard<std::mutex> lock(damage.mutex);
    damage.buffers = {};
}

static std::vector<MTLScissorRect> presentCopyAreas(
    PresentDamage& damage,
    id<MTLTexture> texture,
    int64_t serial,
    NSUInteger width,
    NSUInteger height
) {
    IOSurfaceRef surface = texture.iosurface;
    uint32_t identity = surface ? IOSurfaceGetID(surface) : 0;
    uint64_t now = nowNanos();
    MTLScissorRect full = {0, 0, width, height};
    std::vector<MTLScissorRect> copies;
    std::lock_guard<std::mutex> lock(damage.mutex);
    PresentBuffer* buffer = nullptr;
    PresentBuffer* oldest = &damage.buffers[0];
    for (PresentBuffer& candidate : damage.buffers) {
        if (identity && candidate.surface == identity) {
            buffer = &candidate;
        }
        if (candidate.usedNanos < oldest->usedNanos) {
            oldest = &candidate;
        }
    }
    bool isCurrent = buffer && buffer->serial >= 0 && buffer->serial <= serial &&
        damage.serial - buffer->serial < static_cast<int64_t>(kPresentDamageHistory) &&
        now - buffer->usedNanos < kPresentBufferStaleNanos;
    if (isCurrent) {
        for (int64_t step = buffer->serial + 1; step <= serial; ++step) {
            size_t slot = static_cast<size_t>(step) % kPresentDamageHistory;
            for (size_t index = 0; index < damage.areaCounts[slot]; ++index) {
                addPresentCopy(
                    copies,
                    intersectScissors(damage.areas[slot * kMaxDamageAreas + index], full)
                );
            }
        }
        if (copies.size() > kMaxPresentCopies) {
            MTLScissorRect bounds = {0, 0, 0, 0};
            for (const MTLScissorRect& copy : copies) {
                bounds = uniteScissors(bounds, copy);
            }
            copies.assign(1, bounds);
        }
    } else {
        copies.push_back(full);
    }
    if (!buffer) {
        buffer = oldest;
    }
    buffer->surface = identity;
    buffer->serial = serial;
    buffer->usedNanos = now;
    uint64_t pixels = 0;
    for (const MTLScissorRect& copy : copies) {
        pixels += static_cast<uint64_t>(copy.width) * copy.height;
    }
    damage.copiedPixels.fetch_add(pixels);
    return copies;
}

static void copyPresentAreas(
    id<MTLCommandBuffer> commandBuffer,
    id<MTLTexture> source,
    id<MTLTexture> destination,
    const std::vector<MTLScissorRect>& areas
) {
    if (areas.empty()) {
        return;
    }
    id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
    for (const MTLScissorRect& area : areas) {
        [blit copyFromTexture:source
                  sourceSlice:0
                  sourceLevel:0
                 sourceOrigin:MTLOriginMake(area.x, area.y, 0)
                   sourceSize:MTLSizeMake(area.width, area.height, 1)
                    toTexture:destination
             destinationSlice:0
             destinationLevel:0
            destinationOrigin:MTLOriginMake(area.x, area.y, 0)];
    }
    [blit endEncoding];
}

static id<MTLTexture> newMainTarget(MetalRenderer* renderer, int32_t width, int32_t height, MTLPixelFormat format) {
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:format
                                     width:static_cast<NSUInteger>(width)
                                    height:static_cast<NSUInteger>(height)
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModePrivate;
    return [renderer->device newTextureWithDescriptor:descriptor];
}

static bool readTargetColors(
    MetalRenderer* renderer,
    uint32_t left,
    uint32_t top,
    uint32_t width,
    uint32_t height,
    std::vector<float>& colors
) {
    brixGuiMetalRendererFinish(renderer);
    restoreTarget(renderer);
    id<MTLBuffer> buffer = [renderer->device newBufferWithLength:static_cast<NSUInteger>(width) * height * 4 * sizeof(float)
                                                         options:MTLResourceStorageModeShared];
    if (!buffer) {
        return false;
    }
    const uint32_t region[4] = {left, top, width, height};
    id<MTLCommandBuffer> commandBuffer = rendererCommandBuffer();
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    [encoder setComputePipelineState:renderer->shared->readbackKernel];
    [encoder setTexture:renderer->target atIndex:0];
    [encoder setBuffer:buffer offset:0 atIndex:0];
    [encoder setBytes:region length:sizeof(region) atIndex:1];
    [encoder dispatchThreadgroups:MTLSizeMake((width + 15) / 16, (height + 15) / 16, 1)
            threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    const float* values = static_cast<const float*>(buffer.contents);
    colors.assign(values, values + static_cast<size_t>(width) * height * 4);
    return true;
}

static bool readTarget(MetalRenderer* renderer, std::vector<uint8_t>& pixels) {
    if (renderer->target.pixelFormat != MTLPixelFormatBGRA8Unorm) {
        std::vector<float> colors;
        uint32_t width = static_cast<uint32_t>(renderer->pixelWidth);
        uint32_t height = static_cast<uint32_t>(renderer->pixelHeight);
        if (!readTargetColors(renderer, 0, 0, width, height, colors)) {
            return false;
        }
        pixels.resize(static_cast<size_t>(width) * height * 4);
        for (size_t pixel = 0; pixel < static_cast<size_t>(width) * height; ++pixel) {
            for (int channel = 0; channel < 4; ++channel) {
                float value = std::min(std::max(colors[pixel * 4 + (channel == 3 ? 3 : 2 - channel)], 0.0f), 1.0f);
                pixels[pixel * 4 + channel] = static_cast<uint8_t>(lroundf(value * 255.0f));
            }
        }
        return true;
    }
    brixGuiMetalRendererFinish(renderer);
    restoreTarget(renderer);
    pixels.resize(static_cast<size_t>(renderer->pixelWidth) * renderer->pixelHeight * 4);
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:renderer->pixelWidth
                                    height:renderer->pixelHeight
                                 mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    id<MTLTexture> staging = [renderer->device newTextureWithDescriptor:descriptor];
    id<MTLCommandBuffer> commandBuffer = rendererCommandBuffer();
    id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
    [blit copyFromTexture:renderer->target
              sourceSlice:0
              sourceLevel:0
             sourceOrigin:MTLOriginMake(0, 0, 0)
               sourceSize:MTLSizeMake(renderer->pixelWidth, renderer->pixelHeight, 1)
                toTexture:staging
         destinationSlice:0
         destinationLevel:0
        destinationOrigin:MTLOriginMake(0, 0, 0)];
    [blit endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    [staging getBytes:pixels.data()
          bytesPerRow:static_cast<NSUInteger>(renderer->pixelWidth) * 4
           fromRegion:MTLRegionMake2D(0, 0, renderer->pixelWidth, renderer->pixelHeight)
          mipmapLevel:0];
    return true;
}

static void releaseLayerTargets(MetalRenderer* renderer) {
    for (std::unique_ptr<LayerPass>& layer : renderer->layers) {
        layer->texture = nil;
        layer->depth = nil;
    }
    for (auto& page : renderer->maskPages) {
        page = nil;
    }
    renderer->maskDepth = nil;
    renderer->shadowSource = nil;
    renderer->shadowBlur = nil;
}

static bool ensureShadowTarget(MetalRenderer* renderer, __strong id<MTLTexture>& texture) {
    NSUInteger width = static_cast<NSUInteger>(std::max(renderer->pixelWidth, 1));
    NSUInteger height = static_cast<NSUInteger>(std::max(renderer->pixelHeight, 1));
    if (texture && texture.width == width && texture.height == height) {
        return true;
    }
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:renderer->layerFormat
                                     width:width
                                    height:height
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModePrivate;
    id<MTLTexture> replacement = [renderer->device newTextureWithDescriptor:descriptor];
    if (!replacement) {
        return false;
    }
    texture = replacement;
    countAllocation(renderer);
    return true;
}

static bool ensureMaskPages(MetalRenderer* renderer) {
    NSUInteger width = static_cast<NSUInteger>(std::max(renderer->pixelWidth, 1));
    NSUInteger height = static_cast<NSUInteger>(std::max(renderer->pixelHeight, 1));
    for (uint32_t page = 0; page < kMaskPages; ++page) {
        __strong id<MTLTexture>& texture = renderer->maskPages[page];
        if (renderer->maskPlaced[page].empty() ||
                (texture && texture.width == width && texture.height == height)) {
            continue;
        }
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:renderer->layerFormat
                                         width:width
                                        height:height
                                     mipmapped:NO];
        descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        descriptor.storageMode = MTLStorageModePrivate;
        id<MTLTexture> replacement = [renderer->device newTextureWithDescriptor:descriptor];
        if (!replacement) {
            return false;
        }
        texture = replacement;
        countAllocation(renderer);
    }
    bool used = false;
    for (const std::vector<std::array<uint32_t, 4>>& placed : renderer->maskPlaced) {
        used = used || !placed.empty();
    }
    if (!used || (renderer->maskDepth && renderer->maskDepth.width == width &&
            renderer->maskDepth.height == height)) {
        return true;
    }
    MTLTextureDescriptor* depthDescriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                     width:width
                                    height:height
                                 mipmapped:NO];
    depthDescriptor.usage = MTLTextureUsageRenderTarget;
    depthDescriptor.storageMode = MTLStorageModeMemoryless;
    id<MTLTexture> depth = [renderer->device newTextureWithDescriptor:depthDescriptor];
    if (!depth) {
        return false;
    }
    renderer->maskDepth = depth;
    countAllocation(renderer);
    return true;
}

static bool encodeLayerBlur(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    const LayerPass& layer,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    const SurfacePipelines* surface = surfacePipelinesFor(renderer, layer.texture.pixelFormat);
    if (!surface || !ensureShadowTarget(renderer, renderer->shadowBlur)) {
        return false;
    }
    float width = static_cast<float>(layer.size[0]);
    float height = static_cast<float>(layer.size[1]);
    id<MTLTexture> targets[2] = {renderer->shadowBlur, layer.texture};
    id<MTLTexture> sources[2] = {layer.texture, renderer->shadowBlur};
    id<MTLTexture> depths[2] = {renderer->maskDepth, layer.depth};
    for (uint32_t vertical = 0; vertical < 2; ++vertical) {
        LayerBlurGPU blur = {
            {0, 0, width, height},
            {0, 0},
            layer.blur,
            vertical,
            {0, 0, width, height},
        };
        float viewport[2] = {
            static_cast<float>(targets[vertical].width),
            static_cast<float>(targets[vertical].height),
        };
        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = targets[vertical];
        pass.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.depthAttachment.texture = depths[vertical];
        pass.depthAttachment.loadAction = MTLLoadActionDontCare;
        pass.depthAttachment.storeAction = MTLStoreActionDontCare;
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
        [encoder setRenderPipelineState:surface->layerBlurPipeline];
        [encoder setDepthStencilState:renderer->shared->depthNone];
        [encoder setVertexBytes:&blur length:sizeof(blur) atIndex:0];
        [encoder setVertexBytes:viewport length:sizeof(viewport) atIndex:1];
        [encoder setFragmentBytes:&blur length:sizeof(blur) atIndex:0];
        [encoder setFragmentTexture:sources[vertical] atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [encoder endEncoding];
        ++drawCalls;
        ++pipelineSwitches;
    }
    return true;
}

static bool encodeLayerFilter(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    const LayerPass& layer,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    auto found = renderer->filterShaders.find(layer.filter);
    if (found == renderer->filterShaders.end() || found->second.failed) {
        return true;
    }
    id<MTLRenderPipelineState> pipeline = filterShaderVariant(renderer->device, found->second, layer.texture.pixelFormat);
    if (!pipeline) {
        return true;
    }
    if (!ensureShadowTarget(renderer, renderer->shadowBlur)) {
        return false;
    }
    id<MTLTexture> target = renderer->shadowBlur;
    uint32_t width = std::min<uint32_t>(layer.size[0], static_cast<uint32_t>(target.width));
    uint32_t height = std::min<uint32_t>(layer.size[1], static_cast<uint32_t>(target.height));
    if (!width || !height) {
        return true;
    }
    bool reduces = layer.filterResolution[0] < 1 || layer.filterResolution[1] < 1;
    const SurfacePipelines* surface = reduces ? surfacePipelinesFor(renderer, layer.texture.pixelFormat) : nullptr;
    if (!surface || !surface->filterComposePipeline) {
        reduces = false;
    }
    float resolution[2] = {
        reduces ? layer.filterResolution[0] : 1.0f,
        reduces ? layer.filterResolution[1] : 1.0f,
    };
    uint32_t passWidth = std::max<uint32_t>(1, static_cast<uint32_t>(std::ceil(width * resolution[0])));
    uint32_t passHeight = std::max<uint32_t>(1, static_cast<uint32_t>(std::ceil(height * resolution[1])));
    double scale = layer.filterScale > 0 ? layer.filterScale : 1;
    FilterParamsGPU params = {};
    params.target[0] = -1;
    params.target[1] = 1;
    params.target[2] = 1;
    params.target[3] = -1;
    params.resolution[0] = resolution[0];
    params.resolution[1] = resolution[1];
    params.origin[0] = static_cast<float>(layer.filterRegion[0] - layer.origin[0]);
    params.origin[1] = static_cast<float>(layer.filterRegion[1] - layer.origin[1]);
    params.size[0] = static_cast<float>((layer.filterRegion[2] - layer.filterRegion[0]) / scale);
    params.size[1] = static_cast<float>((layer.filterRegion[3] - layer.filterRegion[1]) / scale);
    params.scale = static_cast<float>(scale);
    params.sourceOrigin[0] = params.origin[0];
    params.sourceOrigin[1] = params.origin[1];
    params.sourceScale = params.scale;
    params.argCount = static_cast<uint32_t>(std::min<size_t>(layer.filterArgs.size(), kMaxFilterArgs));
    float args[kMaxFilterArgs + 1] = {};
    if (params.argCount) {
        memcpy(args, layer.filterArgs.data(), params.argCount * sizeof(float));
    }
    params.imageCount = static_cast<uint32_t>(std::min<size_t>(layer.filterImages.size(), kMaxFilterImages));
    params.time = layer.filterTime;
    id<MTLTexture> images[kMaxFilterImages];
    float frames[kMaxFilterImages * 4] = {};
    if (params.imageCount) {
        memcpy(frames, layer.filterFrames.data(), params.imageCount * 4 * sizeof(float));
    }
    for (uint32_t slot = 0; slot < kMaxFilterImages; ++slot) {
        id<MTLTexture> image = slot < params.imageCount ? layer.filterImages[slot] : nil;
        images[slot] = image ? image : renderer->blankTexture;
    }
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target;
    pass.colorAttachments[0].loadAction = MTLLoadActionDontCare;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.renderTargetWidth = passWidth;
    pass.renderTargetHeight = passHeight;
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
    [encoder setRenderPipelineState:pipeline];
    [encoder setViewport:MTLViewport{0, 0, static_cast<double>(passWidth), static_cast<double>(passHeight), 0, 1}];
    [encoder setVertexBytes:&params length:sizeof(params) atIndex:0];
    [encoder setFragmentBytes:&params length:sizeof(params) atIndex:0];
    [encoder setFragmentBytes:args length:(params.argCount + 1) * sizeof(float) atIndex:1];
    [encoder setFragmentBytes:frames length:sizeof(frames) atIndex:2];
    [encoder setFragmentTexture:layer.texture atIndex:0];
    [encoder setFragmentTextures:images withRange:NSMakeRange(1, kMaxFilterImages)];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];
    ++drawCalls;
    ++pipelineSwitches;
    if (reduces) {
        FilterComposeGPU compose = {};
        compose.target[2] = static_cast<float>(width);
        compose.target[3] = static_cast<float>(height);
        compose.origin[0] = params.origin[0];
        compose.origin[1] = params.origin[1];
        compose.resolution[0] = resolution[0];
        compose.resolution[1] = resolution[1];
        compose.reduced[0] = static_cast<float>(passWidth);
        compose.reduced[1] = static_cast<float>(passHeight);
        compose.scale = params.scale;
        compose.keepCount = layer.filterKeepCount;
        memcpy(compose.keeps, layer.filterKeeps, sizeof(compose.keeps));
        float viewport[2] = {static_cast<float>(layer.texture.width), static_cast<float>(layer.texture.height)};
        MTLRenderPassDescriptor* composing = [MTLRenderPassDescriptor renderPassDescriptor];
        composing.colorAttachments[0].texture = layer.texture;
        composing.colorAttachments[0].loadAction = MTLLoadActionLoad;
        composing.colorAttachments[0].storeAction = MTLStoreActionStore;
        composing.depthAttachment.texture = layer.depth;
        composing.depthAttachment.loadAction = MTLLoadActionDontCare;
        composing.depthAttachment.storeAction = MTLStoreActionDontCare;
        id<MTLRenderCommandEncoder> composer = [commandBuffer renderCommandEncoderWithDescriptor:composing];
        [composer setRenderPipelineState:surface->filterComposePipeline];
        [composer setDepthStencilState:renderer->shared->depthNone];
        [composer setVertexBytes:&compose length:sizeof(compose) atIndex:0];
        [composer setVertexBytes:viewport length:sizeof(viewport) atIndex:1];
        [composer setFragmentBytes:&compose length:sizeof(compose) atIndex:0];
        [composer setFragmentTexture:target atIndex:0];
        [composer drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [composer endEncoding];
        ++drawCalls;
        ++pipelineSwitches;
        return true;
    }
    id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
    [blit copyFromTexture:target
              sourceSlice:0
              sourceLevel:0
             sourceOrigin:MTLOriginMake(0, 0, 0)
               sourceSize:MTLSizeMake(width, height, 1)
                toTexture:layer.texture
         destinationSlice:0
         destinationLevel:0
        destinationOrigin:MTLOriginMake(0, 0, 0)];
    [blit endEncoding];
    return true;
}

struct SinkTarget {
    id<MTLTexture> texture;
    id<MTLTexture> depth;
    PassTarget pass;
    int32_t origin[2];
    int32_t size[2];
};

static void encodeBackdropPass(
    id<MTLCommandBuffer> commandBuffer,
    id<MTLRenderPipelineState> pipeline,
    id<MTLTexture> target,
    uint32_t width,
    uint32_t height,
    id<MTLTexture> source,
    const void* bytes,
    size_t length,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches,
    MTLLoadAction load = MTLLoadActionDontCare
) {
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target;
    pass.colorAttachments[0].loadAction = load;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
    float region[4] = {0, 0, static_cast<float>(width), static_cast<float>(height)};
    float viewport[2] = {static_cast<float>(target.width), static_cast<float>(target.height)};
    [encoder setRenderPipelineState:pipeline];
    [encoder setVertexBytes:region length:sizeof(region) atIndex:0];
    [encoder setVertexBytes:viewport length:sizeof(viewport) atIndex:1];
    if (bytes) {
        [encoder setFragmentBytes:bytes length:length atIndex:0];
    }
    [encoder setFragmentTexture:source atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];
    ++drawCalls;
    ++pipelineSwitches;
}

static BackdropGatherGPU backdropGather(
    const BackdropRecord& record,
    const SinkTarget& sink,
    uint32_t expand,
    int32_t factor
) {
    const double* m = record.sourceTransform;
    return BackdropGatherGPU{
        {static_cast<int32_t>(record.firstX * expand), static_cast<int32_t>(record.firstY * expand)},
        {sink.origin[0], sink.origin[1]},
        {sink.size[0], sink.size[1]},
        {record.region[0], record.region[1]},
        {record.region[2], record.region[3]},
        factor,
        record.transformed ? 1 : 0,
        {static_cast<float>(m[0]), static_cast<float>(m[1])},
        {static_cast<float>(m[2]), static_cast<float>(m[3])},
        {static_cast<float>(m[4]), static_cast<float>(m[5])},
    };
}

static void encodeBackdropBlur(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    const BackdropRecord& record,
    const SinkTarget& sink,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    const int32_t gatherLevel = record.level > 0 ? 1 : 0;
    const uint32_t expand = 1u << (record.level - gatherLevel);
    uint32_t width = record.columns * expand;
    uint32_t height = record.rows * expand;
    BackdropGatherGPU gather = backdropGather(record, sink, expand, record.level > 0 ? 2 : 1);
    encodeBackdropPass(
        commandBuffer,
        renderer->shared->backdropGatherPipeline,
        record.levels[gatherLevel],
        width,
        height,
        sink.texture,
        &gather,
        sizeof(gather),
        drawCalls,
        pipelineSwitches
    );
    for (int32_t level = gatherLevel + 1; level <= record.level; ++level) {
        width /= 2;
        height /= 2;
        encodeBackdropPass(
            commandBuffer,
            renderer->shared->backdropDownPipeline,
            record.levels[level],
            width,
            height,
            record.levels[level - 1],
            nullptr,
            0,
            drawCalls,
            pipelineSwitches
        );
    }
    BackdropBlurGPU blur = record.blur;
    blur.vertical = 0;
    encodeBackdropPass(
        commandBuffer,
        renderer->shared->backdropBlurPipeline,
        record.scratch,
        record.columns,
        record.rows,
        record.levels[record.level],
        &blur,
        sizeof(blur),
        drawCalls,
        pipelineSwitches
    );
    blur.vertical = 1;
    encodeBackdropPass(
        commandBuffer,
        renderer->shared->backdropBlurPipeline,
        record.levels[record.level],
        record.columns,
        record.rows,
        record.scratch,
        &blur,
        sizeof(blur),
        drawCalls,
        pipelineSwitches
    );
}

static void encodeBackdropCopy(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    const BackdropRecord& record,
    const SinkTarget& sink,
    id<MTLTexture> target,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    BackdropGatherGPU gather = backdropGather(record, sink, 1, 1);
    gather.origin[0] = record.region[0];
    gather.origin[1] = record.region[1];
    encodeBackdropPass(
        commandBuffer,
        renderer->shared->backdropGatherPipeline,
        target,
        static_cast<uint32_t>(record.region[2]),
        static_cast<uint32_t>(record.region[3]),
        sink.texture,
        &gather,
        sizeof(gather),
        drawCalls,
        pipelineSwitches,
        MTLLoadActionClear
    );
}

static void encodeBackdropShader(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    const BackdropRecord& record,
    const SinkTarget& sink,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    auto found = renderer->filterShaders.find(record.shader);
    id<MTLRenderPipelineState> pipeline = found == renderer->filterShaders.end() || found->second.failed
        ? nil
        : filterShaderVariant(renderer->device, found->second, record.shaded.pixelFormat);
    if (!pipeline) {
        encodeBackdropCopy(renderer, commandBuffer, record, sink, record.shaded, drawCalls, pipelineSwitches);
        return;
    }
    if (record.blurred) {
        encodeBackdropBlur(renderer, commandBuffer, record, sink, drawCalls, pipelineSwitches);
    } else {
        encodeBackdropCopy(renderer, commandBuffer, record, sink, record.levels[0], drawCalls, pipelineSwitches);
    }
    const double factor = static_cast<double>(1 << record.level);
    const uint32_t width = static_cast<uint32_t>(record.region[2]);
    const uint32_t height = static_cast<uint32_t>(record.region[3]);
    FilterParamsGPU params = {};
    params.target[0] = -1;
    params.target[1] = 1;
    params.target[2] = 1;
    params.target[3] = -1;
    params.resolution[0] = 1;
    params.resolution[1] = 1;
    params.origin[0] = record.shaderOrigin[0];
    params.origin[1] = record.shaderOrigin[1];
    params.size[0] = record.shaderSize[0];
    params.size[1] = record.shaderSize[1];
    params.scale = record.shaderScale;
    params.sourceOrigin[0] = static_cast<float>(
        (record.region[0] + record.shaderOrigin[0]) / factor - static_cast<double>(record.firstX)
    );
    params.sourceOrigin[1] = static_cast<float>(
        (record.region[1] + record.shaderOrigin[1]) / factor - static_cast<double>(record.firstY)
    );
    params.sourceScale = static_cast<float>(record.shaderScale / factor);
    params.argCount = std::min(record.argCount, kMaxFilterArgs);
    params.time = record.time;
    float args[kMaxFilterArgs + 1] = {};
    if (params.argCount) {
        memcpy(args, renderer->backdropArgs.data() + record.argStart, params.argCount * sizeof(float));
    }
    float frames[kMaxFilterImages * 4] = {};
    id<MTLTexture> images[kMaxFilterImages];
    for (uint32_t slot = 0; slot < kMaxFilterImages; ++slot) {
        images[slot] = renderer->blankTexture;
    }
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = record.shaded;
    pass.colorAttachments[0].loadAction = MTLLoadActionDontCare;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.renderTargetWidth = width;
    pass.renderTargetHeight = height;
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
    [encoder setRenderPipelineState:pipeline];
    [encoder setViewport:MTLViewport{0, 0, static_cast<double>(width), static_cast<double>(height), 0, 1}];
    [encoder setVertexBytes:&params length:sizeof(params) atIndex:0];
    [encoder setFragmentBytes:&params length:sizeof(params) atIndex:0];
    [encoder setFragmentBytes:args length:(params.argCount + 1) * sizeof(float) atIndex:1];
    [encoder setFragmentBytes:frames length:sizeof(frames) atIndex:2];
    [encoder setFragmentTexture:record.levels[record.level] atIndex:0];
    [encoder setFragmentTextures:images withRange:NSMakeRange(1, kMaxFilterImages)];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    [encoder endEncoding];
    ++drawCalls;
    ++pipelineSwitches;
}

static void encodeBackdrop(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    const BackdropRecord& record,
    const SinkTarget& sink,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    if (record.shader) {
        encodeBackdropShader(renderer, commandBuffer, record, sink, drawCalls, pipelineSwitches);
        return;
    }
    encodeBackdropBlur(renderer, commandBuffer, record, sink, drawCalls, pipelineSwitches);
}

static bool encodeLayerPass(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    uint32_t index,
    id<MTLBuffer> instanceBuffer,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
);

static bool encodeSink(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    const std::vector<Batch>& batches,
    const SinkTarget& sink,
    MTLLoadAction firstLoad,
    MTLClearColor clearColor,
    const Batch* prelude,
    id<MTLBuffer> instanceBuffer,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    size_t start = 0;
    bool first = true;
    for (size_t index = 0; index <= batches.size(); ++index) {
        const bool done = index == batches.size();
        if (!done && batches[index].pipeline != PipelineBackdropBlur &&
                batches[index].pipeline != PipelineDeferredLayer) {
            continue;
        }
        if (first || index > start) {
            MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
            pass.colorAttachments[0].texture = sink.texture;
            pass.colorAttachments[0].loadAction = first ? firstLoad : MTLLoadActionLoad;
            pass.colorAttachments[0].storeAction = MTLStoreActionStore;
            pass.colorAttachments[0].clearColor = clearColor;
            pass.depthAttachment.texture = sink.depth;
            pass.depthAttachment.loadAction = MTLLoadActionClear;
            pass.depthAttachment.storeAction = MTLStoreActionDontCare;
            pass.depthAttachment.clearDepth = 1.0;
            id<MTLRenderCommandEncoder> encoder =
                [commandBuffer renderCommandEncoderWithDescriptor:pass];
            if (first && prelude) {
                encodeBatches(
                    renderer,
                    encoder,
                    prelude,
                    1,
                    instanceBuffer,
                    sink.pass,
                    drawCalls,
                    pipelineSwitches
                );
            }
            encodeBatches(
                renderer,
                encoder,
                batches.data() + start,
                index - start,
                instanceBuffer,
                sink.pass,
                drawCalls,
                pipelineSwitches
            );
            [encoder endEncoding];
            first = false;
        }
        if (done) {
            break;
        }
        const Batch& marker = batches[index];
        if (marker.pipeline == PipelineBackdropBlur) {
            encodeBackdrop(
                renderer,
                commandBuffer,
                renderer->backdrops[marker.start],
                sink,
                drawCalls,
                pipelineSwitches
            );
        } else if (!encodeLayerPass(
                renderer,
                commandBuffer,
                marker.start,
                instanceBuffer,
                drawCalls,
                pipelineSwitches
            )) {
            return false;
        }
        start = index + 1;
    }
    return true;
}

static bool encodeLayerPass(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    uint32_t index,
    id<MTLBuffer> instanceBuffer,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    const LayerPass& layer = *renderer->layers[index];
    SinkTarget sink = {
        layer.texture,
        layer.depth,
        {
            {layer.origin[0], layer.origin[1], layer.size[0], layer.size[1]},
            {
                static_cast<float>(layer.origin[0]),
                static_cast<float>(layer.origin[1]),
                static_cast<float>(layer.texture.width),
                static_cast<float>(layer.texture.height),
            },
            -1,
            layer.texture.pixelFormat,
        },
        {static_cast<int32_t>(layer.origin[0]), static_cast<int32_t>(layer.origin[1])},
        {static_cast<int32_t>(layer.size[0]), static_cast<int32_t>(layer.size[1])},
    };
    if (!encodeSink(
            renderer,
            commandBuffer,
            layer.batches,
            sink,
            MTLLoadActionClear,
            MTLClearColorMake(0, 0, 0, 0),
            nullptr,
            instanceBuffer,
            drawCalls,
            pipelineSwitches
        )) {
        return false;
    }
    if (layer.blur && !encodeLayerBlur(renderer, commandBuffer, layer, drawCalls, pipelineSwitches)) {
        return false;
    }
    return !layer.filter || encodeLayerFilter(renderer, commandBuffer, layer, drawCalls, pipelineSwitches);
}

static void encodeShadowBlurs(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    id<MTLTexture> target,
    id<MTLTexture> source,
    bool clears,
    uint32_t channel,
    id<MTLBuffer> blurBuffer,
    uint32_t start,
    uint32_t count,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    float viewport[2] = {
        static_cast<float>(renderer->pixelWidth),
        static_cast<float>(renderer->pixelHeight),
    };
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target;
    pass.colorAttachments[0].loadAction = clears ? MTLLoadActionClear : MTLLoadActionLoad;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    pass.depthAttachment.texture = renderer->maskDepth;
    pass.depthAttachment.loadAction = MTLLoadActionDontCare;
    pass.depthAttachment.storeAction = MTLStoreActionDontCare;
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
    [encoder setRenderPipelineState:surfacePipelinesFor(renderer, target.pixelFormat)->shadowBlurPipelines[channel]];
    [encoder setDepthStencilState:renderer->shared->depthNone];
    [encoder setVertexBuffer:blurBuffer offset:0 atIndex:0];
    [encoder setVertexBytes:viewport length:sizeof(viewport) atIndex:1];
    [encoder setFragmentTexture:source atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                vertexStart:0
                vertexCount:4
              instanceCount:count
               baseInstance:start];
    [encoder endEncoding];
    ++drawCalls;
    ++pipelineSwitches;
}

static bool encodeShadows(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    id<MTLBuffer> instanceBuffer,
    const MTLScissorRect& fullArea,
    uint32_t& clearedMaskPages,
    uint32_t& drawCalls,
    uint32_t& pipelineSwitches
) {
    const std::vector<ShadowRecord>& records = renderer->shadowRecords;
    if (!ensureShadowTarget(renderer, renderer->shadowSource) ||
            !ensureShadowTarget(renderer, renderer->shadowBlur)) {
        return false;
    }
    uint32_t count = static_cast<uint32_t>(records.size());
    id<MTLBuffer> blurBuffer = ensureInstanceBuffer(
        renderer,
        &renderer->shadowBlurBuffers[renderer->packetIndex],
        static_cast<size_t>(count) * 2 * sizeof(ShadowBlurGPU)
    );
    if (!blurBuffer) {
        return false;
    }
    ShadowBlurGPU* blurs = static_cast<ShadowBlurGPU*>(blurBuffer.contents);
    PassTarget target = {
        fullArea,
        {0, 0, static_cast<float>(renderer->pixelWidth), static_cast<float>(renderer->pixelHeight)},
        -1,
        renderer->shadowSource.pixelFormat,
    };
    uint32_t first = 0;
    for (uint32_t generation = 0; generation < renderer->shadowGenerations; ++generation) {
        uint32_t last = first;
        uint32_t layerStarts[kShadowLayers + 1] = {};
        while (last < count && records[last].generation == generation) {
            blurs[last] = records[last].horizontal;
            ++layerStarts[records[last].layer + 1];
            ++last;
        }
        for (uint32_t layer = 0; layer < kShadowLayers; ++layer) {
            layerStarts[layer + 1] += layerStarts[layer];
        }
        uint32_t cursors[kShadowLayers];
        memcpy(cursors, layerStarts, sizeof(cursors));
        for (uint32_t index = first; index < last; ++index) {
            blurs[count + first + cursors[records[index].layer]++] = records[index].vertical;
        }

        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = renderer->shadowSource;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
        pass.depthAttachment.texture = renderer->maskDepth;
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.storeAction = MTLStoreActionDontCare;
        pass.depthAttachment.clearDepth = 1.0;
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
        const std::vector<Batch>& batches = renderer->shadowBatches[generation];
        encodeBatches(
            renderer,
            encoder,
            batches.data(),
            batches.size(),
            instanceBuffer,
            target,
            drawCalls,
            pipelineSwitches
        );
        [encoder endEncoding];

        encodeShadowBlurs(
            renderer,
            commandBuffer,
            renderer->shadowBlur,
            renderer->shadowSource,
            true,
            4,
            blurBuffer,
            first,
            last - first,
            drawCalls,
            pipelineSwitches
        );
        for (uint32_t layer = 0; layer < kShadowLayers; ++layer) {
            uint32_t layerCount = layerStarts[layer + 1] - layerStarts[layer];
            if (!layerCount) {
                continue;
            }
            uint32_t page = layer / 4;
            bool clears = !(clearedMaskPages & (1u << page));
            clearedMaskPages |= 1u << page;
            encodeShadowBlurs(
                renderer,
                commandBuffer,
                renderer->maskPages[page],
                renderer->shadowBlur,
                clears,
                layer % 4,
                blurBuffer,
                count + first + layerStarts[layer],
                layerCount,
                drawCalls,
                pipelineSwitches
            );
        }
        first = last;
    }
    return true;
}

static void detachPresentGate(MetalRenderer* renderer) {
    std::shared_ptr<PresentGate> gate = renderer->presentGate;
    if (!gate) {
        return;
    }
    if (gate->outstanding && gate->window && gate->setPending) {
        gate->setPending(gate->window, false);
    }
    gate->window = nullptr;
    gate->setPending = nullptr;
}

static void waitForPresenter(MetalRenderer* renderer) {
    if (renderer->presentGate && renderer->presentGate->outstanding > 0) {
        dispatch_sync(renderer->presentQueue, ^{
        });
    }
}

static void releasePresentQueue(MetalRenderer* renderer) {
    waitForPresenter(renderer);
    detachPresentGate(renderer);
    renderer->presentGate.reset();
    renderer->presentQueue = nullptr;
}

static void watchPresentation(
    const std::shared_ptr<PresentationState>& state,
    id<CAMetalDrawable> drawable,
    int64_t frameId
) {
#if !TARGET_OS_SIMULATOR
    if (@available(macOS 10.15.4, *)) {
        std::shared_ptr<PresentationState> presentedState = state;
        presentedState->pending.fetch_add(1);
        [drawable addPresentedHandler:^(id<MTLDrawable> presented) {
            presentedState->pending.fetch_sub(1);
            std::lock_guard<std::mutex> lock(presentedState->mutex);
            PresentationRecord& record = presentedState->records[
                static_cast<size_t>(frameId) % presentedState->records.size()
            ];
            if (record.frame == frameId) {
                record.presentedTimeNanos = static_cast<uint64_t>(
                    llround(presented.presentedTime * 1e9)
                );
            }
        }];
    }
#endif
}

static void enqueuePresent(MetalRenderer* renderer, int64_t frameId, int64_t serial) {
    std::shared_ptr<PresentGate> gate = renderer->presentGate;
    std::shared_ptr<PresentationState> state = renderer->presentationState;
    std::shared_ptr<PresentDamage> damage = renderer->presentDamage;
    CAMetalLayer* layer = renderer->presentationLayer;
    id<MTLTexture> source = renderer->target;
    if (++gate->outstanding == 1 && gate->window && gate->setPending) {
        gate->setPending(gate->window, true);
    }
    dispatch_async(renderer->presentQueue, ^{
        @autoreleasepool {
            uint64_t waitStarted = nowNanos();
            id<CAMetalDrawable> drawable = [layer nextDrawable];
            uint64_t waited = nowNanos() - waitStarted;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                PresentationRecord& record = state->records[
                    static_cast<size_t>(frameId) % state->records.size()
                ];
                if (record.frame == frameId) {
                    record.presenterWaitNanos = waited;
                }
            }
            id<MTLTexture> texture = drawable.texture;
            if (texture && texture.width == source.width && texture.height == source.height &&
                    texture.pixelFormat == source.pixelFormat) {
                id<MTLCommandBuffer> commandBuffer = rendererCommandBuffer();
                copyPresentAreas(
                    commandBuffer,
                    source,
                    texture,
                    presentCopyAreas(*damage, texture, serial, source.width, source.height)
                );
                watchPresentation(state, drawable, frameId);
                [commandBuffer presentDrawable:drawable];
                [commandBuffer commit];
            }
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            if (--gate->outstanding == 0 && gate->window && gate->setPending) {
                gate->setPending(gate->window, false);
            }
        });
    });
}

static void releaseRendererResources(MetalRenderer* renderer) {
    for (TileRecord& tile : renderer->tiles) {
        tile.texture = nil;
    }
    renderer->retiredTiles.clear();
    renderer->tileStaging = nil;
    for (RunRecord& run : renderer->runs) {
        for (CTFontRef font : run.fonts) {
            if (font) CFRelease(font);
        }
    }
    for (auto& entry : renderer->imageTextures) {
        for (auto& tile : entry.second.tiles) {
            releaseImageTextureTile(tile.second.surface);
        }
        releaseImageTextureTile(entry.second.single);
        releaseImageTextureTile(entry.second.preview);
    }
    for (ImageTextureTile& surface : renderer->imageTilePool) {
        releaseImageTextureTile(surface);
    }
    for (FramePacket& packet : renderer->packets) {
        packet.images.clear();
    }
    releaseLayerTargets(renderer);
    renderer->outputTexture = nil;
    if (renderer->outputSurface) {
        CFRelease(renderer->outputSurface);
        renderer->outputSurface = nullptr;
    }
    for (auto& entry : renderer->dabShaders) {
        releaseShaderPipelines(entry.second);
    }
    for (auto& entry : renderer->fillShaders) {
        releaseShaderPipelines(entry.second);
    }
    for (auto& entry : renderer->textShaders) {
        releaseShaderPipelines(entry.second);
    }
    for (auto& entry : renderer->filterShaders) {
        releaseShaderPipelines(entry.second);
    }
    renderer->backdropTextures.clear();
    releasePresentQueue(renderer);
    renderer->presentationLayer = nil;
    renderer->presentationWindow = nullptr;
    renderer->inflight = nullptr;
}

} // namespace

extern "C" bool brixGuiMetalRendererIsAvailable(void) {
    return MTLCreateSystemDefaultDevice() != nil;
}

extern "C" void* brixGuiMetalCanvasCreateCGImage(void* canvas) {
    return brixGraphicsCanvasCreateCGImage(canvas);
}

static void watchCanvasReleases(MetalRenderer* renderer);
static void unwatchCanvasReleases(MetalRenderer* renderer);

static int32_t rendererPixelSpan(double extent, double scale) {
    return static_cast<int32_t>(ceil(extent * scale - 1e-6));
}

extern "C" void brixGuiMetalWarmUp(void) {
    static std::once_flag once;
    std::call_once(once, [] {
        brixGraphicsShaderSetRegisterHook(warmUserShader);
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            @autoreleasepool {
                id<MTLDevice> device = (__bridge id<MTLDevice>)brixGraphicsMetalDevice();
                if (!device) {
                    return;
                }
                MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
                    texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                 width:64
                                                height:64
                                             mipmapped:NO];
                descriptor.usage = MTLTextureUsageRenderTarget;
                descriptor.storageMode = MTLStorageModePrivate;
                MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
                pass.colorAttachments[0].texture = [device newTextureWithDescriptor:descriptor];
                pass.colorAttachments[0].loadAction = MTLLoadActionClear;
                pass.colorAttachments[0].storeAction = MTLStoreActionStore;
                id<MTLCommandBuffer> buffer = rendererCommandBuffer();
                [[buffer renderCommandEncoderWithDescriptor:pass] endEncoding];
                [buffer commit];
                sharedPipelines();
            }
        });
    });
}

static std::mutex gRenderersMutex;
static std::vector<MetalRenderer*> gRenderers;

extern "C" void brixGuiMetalReleaseIdle(void) {
    if (!brixGraphicsMetalReleaseIdleQueue()) {
        return;
    }
    std::lock_guard<std::mutex> lock(gRenderersMutex);
    for (MetalRenderer* renderer : gRenderers) {
        if (renderer->presentationLayer && renderer->target && !renderer->targetVolatile &&
                (!renderer->presentGate || renderer->presentGate->outstanding == 0)) {
            [renderer->target setPurgeableState:MTLPurgeableStateVolatile];
            renderer->targetVolatile = true;
        }
    }
}

extern "C" void* brixGuiMetalRendererCreate(
    double width,
    double height,
    double scale,
    double red,
    double green,
    double blue,
    double alpha
) {
    if (!(width > 0) || !(height > 0) || !std::isfinite(scale) || scale <= 0 ||
            rendererPixelSpan(width, scale) <= 0 || rendererPixelSpan(height, scale) <= 0) {
        return nullptr;
    }
    watchMemoryPressure();
    MetalRenderer* renderer = new MetalRenderer();
    renderer->memoryPressureSeen = memoryPressureCount.load(std::memory_order_acquire);
    renderer->device = (__bridge id<MTLDevice>)brixGraphicsMetalDevice();
    if (!renderer->device) {
        delete renderer;
        return nullptr;
    }
    renderer->emptyBuffer = [renderer->device newBufferWithLength:256
                                                          options:MTLResourceStorageModeShared];
    MTLTextureDescriptor* blank = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:1
                                    height:1
                                 mipmapped:NO];
    renderer->blankTexture = [renderer->device newTextureWithDescriptor:blank];
    renderer->shared = sharedPipelines();
    if (!renderer->emptyBuffer || !renderer->blankTexture || !renderer->shared) {
        releaseRendererResources(renderer);
        delete renderer;
        return nullptr;
    }
    memset(renderer->emptyBuffer.contents, 0, 256);
    renderer->width = width;
    renderer->height = height;
    renderer->scale = scale;
    renderer->pixelWidth = rendererPixelSpan(width, scale);
    renderer->pixelHeight = rendererPixelSpan(height, scale);
    renderer->background[0] = static_cast<float>(red);
    renderer->background[1] = static_cast<float>(green);
    renderer->background[2] = static_cast<float>(blue);
    renderer->background[3] = static_cast<float>(alpha);
    strncpy(renderer->deviceName, renderer->device.name.UTF8String, 255);

    renderer->target = newMainTarget(renderer, renderer->pixelWidth, renderer->pixelHeight, renderer->outputFormat);
    if (!renderer->target) {
        releaseRendererResources(renderer);
        delete renderer;
        return nullptr;
    }
    renderer->inflight = dispatch_semaphore_create(3);
    renderer->glyphAtlas.maxPages = kGlyphAtlasPages;
    renderer->glyphAtlas.pixelFormat = MTLPixelFormatA8Unorm;
    renderer->glyphAtlas.bytesPerPixel = 1;
    renderer->imageAtlas.maxPages = kImageAtlasPages;
    renderer->imageAtlas.pixelFormat = MTLPixelFormatBGRA8Unorm;
    renderer->imageAtlas.bytesPerPixel = 4;
    renderer->presentationState = std::make_shared<PresentationState>();
    renderer->presentationState->records.resize(2048);
    watchCanvasReleases(renderer);
    {
        std::lock_guard<std::mutex> lock(gRenderersMutex);
        gRenderers.push_back(renderer);
    }
    return renderer;
}

static bool encodeFrame(
    MetalRenderer* renderer,
    int64_t frameId,
    int32_t frameSlot,
    int32_t mode,
    double damageX,
    double damageY,
    double damageWidth,
    double damageHeight,
    uint64_t encodeStarted,
    bool tiles
);

extern "C" void brixGuiMetalRendererFinish(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return;
    }
    waitForPresenter(renderer);
    id<MTLCommandBuffer> commandBuffer = rendererCommandBuffer();
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    noteCompletedFrame(renderer, renderer->frameCount - 1);
    for (int attempt = 0; attempt < 4 && renderer->canReencode &&
            renderer->fillRedraw.exchange(false); ++attempt) {
        dispatch_semaphore_wait(renderer->inflight, DISPATCH_TIME_FOREVER);
        int64_t frameId = renderer->frameCount++;
        int32_t frameSlot = renderer->packetIndex;
        renderer->lastFrame = frameSlot;
        bool encoded = false;
        @autoreleasepool {
            encoded = encodeFrame(
                renderer,
                frameId,
                frameSlot,
                renderer->encodeMode,
                renderer->encodeDamage[0],
                renderer->encodeDamage[1],
                renderer->encodeDamage[2],
                renderer->encodeDamage[3],
                nowNanos(),
                false
            );
        }
        if (!encoded) {
            renderer->canReencode = false;
            break;
        }
        id<MTLCommandBuffer> fence = rendererCommandBuffer();
        [fence commit];
        [fence waitUntilCompleted];
        noteCompletedFrame(renderer, frameId);
    }
}

extern "C" bool brixGuiMetalRendererResize(
    void* pointer,
    double width,
    double height,
    double scale,
    double red,
    double green,
    double blue,
    double alpha
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !(width > 0) || !(height > 0) || !std::isfinite(scale) || scale <= 0) {
        return false;
    }

    int32_t pixelWidth = rendererPixelSpan(width, scale);
    int32_t pixelHeight = rendererPixelSpan(height, scale);
    if (pixelWidth <= 0 || pixelHeight <= 0) {
        return false;
    }
    bool targetChanged = pixelWidth != renderer->pixelWidth || pixelHeight != renderer->pixelHeight;
    id<MTLTexture> replacement = nil;
    if (targetChanged) {
        replacement = newMainTarget(renderer, pixelWidth, pixelHeight, renderer->outputFormat);
        if (!replacement) {
            return false;
        }
    }

    renderer->width = width;
    renderer->height = height;
    renderer->scale = scale;
    renderer->pixelWidth = pixelWidth;
    renderer->pixelHeight = pixelHeight;
    renderer->background[0] = static_cast<float>(red);
    renderer->background[1] = static_cast<float>(green);
    renderer->background[2] = static_cast<float>(blue);
    renderer->background[3] = static_cast<float>(alpha);

    if (targetChanged) {
        renderer->target = replacement;
        renderer->targetVolatile = false;
        forgetPresentBuffers(*renderer->presentDamage);
        releaseLayerTargets(renderer);
        countAllocation(renderer);
    }
    if (renderer->presentationLayer) {
        renderer->presentationLayer.contentsScale = scale;
        renderer->presentationLayer.drawableSize = CGSizeMake(pixelWidth, pixelHeight);
    }
    return true;
}

extern "C" void brixGuiMetalRendererRelease(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return;
    }
    if (renderer->currentPacket) {
        renderer->currentPacket = nullptr;
        dispatch_semaphore_signal(renderer->inflight);
    }
    unwatchCanvasReleases(renderer);
    {
        std::lock_guard<std::mutex> lock(gRenderersMutex);
        gRenderers.erase(std::remove(gRenderers.begin(), gRenderers.end(), renderer), gRenderers.end());
    }
    brixGuiMetalRendererFinish(renderer);
    releaseRendererResources(renderer);
    delete renderer;
}

extern "C" const char* brixGuiMetalRendererDeviceName(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->deviceName : "";
}

static bool presentationIsHeld(MetalRenderer* renderer) {
    if (!renderer->presentationVisibility) {
        return false;
    }
    int32_t visibility = renderer->presentationVisibility(renderer->presentationWindow);
    if (visibility == BxGuiPresentationUnoccluded) {
        renderer->presentationWasUnoccluded = true;
        return false;
    }
    if (visibility == BxGuiPresentationHidden) {
        renderer->presentationWasUnoccluded = false;
    } else if (renderer->presentationWasUnoccluded) {
        return true;
    }
    int32_t held = static_cast<int32_t>(renderer->presentationLayer.maximumDrawableCount) - 1;
    return renderer->presentationState->pending.load() >= held;
}

static void applyPresentationFormat(MetalRenderer* renderer, CAMetalLayer* layer) {
    bool extended = renderer->outputFormat != MTLPixelFormatBGRA8Unorm;
    layer.pixelFormat = renderer->outputFormat;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(extended ? kCGColorSpaceExtendedSRGB : kCGColorSpaceSRGB);
    layer.colorspace = colorSpace;
    CGColorSpaceRelease(colorSpace);
}

static int32_t attachPresentation(
    MetalRenderer* renderer,
    void* target,
    CAMetalLayer* layer,
    int32_t framesPerSecond,
    bool (*setLayer)(void*, void*),
    void (*resetLayer)(void*),
    int32_t (*visibility)(void*),
    void (*setPending)(void*, bool)
) {
    if (!renderer) {
        return 0;
    }
    if (!target) {
        renderer->attachStatus = -1;
        return renderer->attachStatus;
    }
    if (renderer->presentationLayer) {
        bool isSameTarget = renderer->presentationWindow == target;
        renderer->attachStatus = isSameTarget ? 1 : -2;
        return renderer->attachStatus;
    }
    if (!layer) {
        renderer->attachStatus = -3;
        return renderer->attachStatus;
    }
    layer.device = renderer->device;
    applyPresentationFormat(renderer, layer);
    layer.framebufferOnly = NO;
    layer.contentsScale = renderer->scale;
    layer.drawableSize = CGSizeMake(renderer->pixelWidth, renderer->pixelHeight);
    layer.maximumDrawableCount = 3;
#if TARGET_OS_OSX
    layer.displaySyncEnabled = YES;
#endif
    layer.allowsNextDrawableTimeout = YES;
    if (!setLayer(target, (__bridge void*)layer)) {
        renderer->attachStatus = -4;
        resetLayer(target);
        return renderer->attachStatus;
    }
    renderer->presentationLayer = layer;
    renderer->presentationWindow = target;
    renderer->resetPresentationLayer = resetLayer;
    renderer->presentationVisibility = visibility;
    renderer->presentationWasUnoccluded = false;
    if (setPending) {
        if (!renderer->presentQueue) {
            renderer->presentQueue = dispatch_queue_create(
                "brix.metal.present",
                dispatch_queue_attr_make_with_qos_class(
                    DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INTERACTIVE, 0
                )
            );
            renderer->presentGate = std::make_shared<PresentGate>();
        }
        renderer->presentGate->window = target;
        renderer->presentGate->setPending = setPending;
    }
    renderer->attachStatus = 1;
    renderer->maximumFramesPerSecond = framesPerSecond;
    return renderer->attachStatus;
}

extern "C" int32_t brixGuiMetalRendererAttachWindow(void* pointer, void* window) {
    return attachPresentation(
        static_cast<MetalRenderer*>(pointer),
        window,
        window ? (__bridge CAMetalLayer*)brixGuiWindowGetMetalLayer(window) : nil,
        window ? brixGuiWindowGetMaximumFramesPerSecond(window) : 0,
        brixGuiWindowSetCanvasLayer,
        brixGuiWindowResetCanvasLayer,
        brixGuiWindowPresentationVisibility,
        brixGuiWindowSetPresentationPending
    );
}

extern "C" int32_t brixGuiMetalRendererAttachPopup(void* pointer, void* popup) {
    void* window = popup ? brixGuiPopupGetWindow(popup) : nullptr;
    return attachPresentation(
        static_cast<MetalRenderer*>(pointer),
        window ? popup : nullptr,
        window ? (__bridge CAMetalLayer*)brixGuiPopupGetMetalLayer(popup) : nil,
        window ? brixGuiWindowGetMaximumFramesPerSecond(window) : 0,
        brixGuiPopupSetCanvasLayer,
        brixGuiPopupResetCanvasLayer,
        brixGuiPopupPresentationVisibility,
        brixGuiPopupSetPresentationPending
    );
}

extern "C" bool brixGuiMetalRendererSetOutputGamut(void* pointer, bool wide, bool opaque) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || renderer->currentPacket) {
        return false;
    }
    MTLPixelFormat output = MTLPixelFormatBGRA8Unorm;
    if (wide && [renderer->device supportsFamily:MTLGPUFamilyApple3]) {
        output = opaque ? MTLPixelFormatBGR10_XR : MTLPixelFormatBGRA10_XR;
    }
    if (output == renderer->outputFormat) {
        return false;
    }
    MTLPixelFormat layer = output == MTLPixelFormatBGRA8Unorm ? MTLPixelFormatBGRA8Unorm : MTLPixelFormatBGRA10_XR;
    if (!surfacePipelinesFor(renderer, output) || !surfacePipelinesFor(renderer, layer)) {
        return false;
    }
    id<MTLTexture> target = newMainTarget(renderer, renderer->pixelWidth, renderer->pixelHeight, output);
    if (!target) {
        return false;
    }
    renderer->target = target;
    renderer->targetVolatile = false;
    forgetPresentBuffers(*renderer->presentDamage);
    releaseLayerTargets(renderer);
    countAllocation(renderer);
    renderer->outputFormat = output;
    renderer->layerFormat = layer;
    if (renderer->presentationLayer) {
        applyPresentationFormat(renderer, renderer->presentationLayer);
    }
    return true;
}

extern "C" bool brixGuiMetalRendererSetOutputSurface(void* pointer, void* surfacePointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    IOSurfaceRef surface = static_cast<IOSurfaceRef>(surfacePointer);
    if (!renderer) {
        return false;
    }
    if (surface == renderer->outputSurface) {
        return surface == nullptr || renderer->outputTexture != nil;
    }
    renderer->outputTexture = nil;
    if (renderer->outputSurface) {
        CFRelease(renderer->outputSurface);
        renderer->outputSurface = nullptr;
    }
    if (!surface) {
        return true;
    }
    if (IOSurfaceGetPixelFormat(surface) != 'BGRA' || renderer->target.pixelFormat != MTLPixelFormatBGRA8Unorm) {
        return false;
    }
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:IOSurfaceGetWidth(surface)
                                    height:IOSurfaceGetHeight(surface)
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
#if TARGET_OS_OSX
    descriptor.storageMode = renderer->device.hasUnifiedMemory ? MTLStorageModeShared : MTLStorageModeManaged;
#else
    descriptor.storageMode = MTLStorageModeShared;
#endif
    renderer->outputTexture = [renderer->device newTextureWithDescriptor:descriptor iosurface:surface plane:0];
    if (!renderer->outputTexture) {
        return false;
    }
    CFRetain(surface);
    renderer->outputSurface = surface;
    return true;
}

extern "C" int64_t brixGuiMetalRendererCompletedFrame(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->completedFrame.load() : -1;
}

extern "C" bool brixGuiMetalRendererIsWideGamut(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer && renderer->outputFormat != MTLPixelFormatBGRA8Unorm;
}

extern "C" bool brixGuiMetalRendererReadColor(void* pointer, int32_t x, int32_t y, double* color) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !color || x < 0 || y < 0 || x >= renderer->pixelWidth || y >= renderer->pixelHeight) {
        return false;
    }
    std::vector<float> values;
    if (!readTargetColors(renderer, static_cast<uint32_t>(x), static_cast<uint32_t>(y), 1, 1, values)) {
        return false;
    }
    float alpha = values[3];
    for (int channel = 0; channel < 3; ++channel) {
        color[channel] = alpha > 0 ? values[channel] / alpha : 0;
    }
    color[3] = alpha;
    return true;
}

extern "C" int32_t brixGuiMetalRendererAttachStatus(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->attachStatus : 0;
}

extern "C" uint64_t brixGuiMetalRendererTakePresentCopiedPixels(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->presentDamage->copiedPixels.exchange(0) : 0;
}

extern "C" void brixGuiMetalRendererDetachWindow(void* pointer, void* window) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->presentationLayer ||
        renderer->presentationWindow != window) {
        return;
    }
    if (renderer->resetPresentationLayer) {
        renderer->resetPresentationLayer(window);
    }
    renderer->resetPresentationLayer = nullptr;
    renderer->presentationVisibility = nullptr;
    renderer->presentationWasUnoccluded = false;
    detachPresentGate(renderer);
    renderer->presentationLayer = nil;
    renderer->presentationWindow = nullptr;
    renderer->attachStatus = 0;
    renderer->maximumFramesPerSecond = 0;
}

static uint16_t internRunFont(RunRecord& run, CTFontRef font) {
    for (size_t index = 0; index < run.fonts.size(); ++index) {
        if (run.fonts[index] == font || CFEqual(run.fonts[index], font)) {
            return static_cast<uint16_t>(index);
        }
    }
    CFRetain(font);
    run.fonts.push_back(font);
    return static_cast<uint16_t>(run.fonts.size() - 1);
}

extern "C" int64_t brixGuiMetalRunRegister(void* pointer, void* textLayout) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !textLayout) {
        return -1;
    }
    const uint64_t layoutSerial = brixGraphicsTextLayoutGetSerial(textLayout);
    auto found = renderer->runByLayout.find(layoutSerial);
    if (found != renderer->runByLayout.end()) {
        RunRecord& run = renderer->runs[found->second];
        if (run.alive) {
            ++run.references;
            return static_cast<int64_t>(
                (static_cast<uint64_t>(run.generation) << 32) | found->second
            );
        }
    }

    uint32_t index;
    if (!renderer->freeRuns.empty()) {
        index = renderer->freeRuns.back();
        renderer->freeRuns.pop_back();
    } else {
        ensureVectorCapacity(renderer, renderer->runs, renderer->runs.size() + 1);
        index = static_cast<uint32_t>(renderer->runs.size());
        renderer->runs.push_back(RunRecord());
    }
    RunRecord& run = renderer->runs[index];
    run.layoutSerial = layoutSerial;
    run.references = 1;
    run.alive = true;
    run.height = static_cast<float>(brixGraphicsTextLayoutGetHeight(textLayout));
    run.naturalWidth = static_cast<float>(
        brixGraphicsTextLayoutGetNaturalWidth(textLayout)
    );
    run.maximumWidthDelta = 0;
    run.glyphs.clear();
    run.lines.clear();
    run.fonts.clear();
    run.entries.clear();
    run.drawnScale = 0;
    run.drawnFrame = -1;
    run.scaleChanging = false;
    run.moving = false;
    run.emSize = 0;
    run.xHeight = 0;
    run.tinted = false;

    int64_t lineCount = brixGraphicsTextLayoutGetLineCount(textLayout);
    int64_t runCount = brixGraphicsTextLayoutGetRunCount(textLayout);
    for (int64_t lineIndex = 0; lineIndex < lineCount; ++lineIndex) {
        RunLine line;
        line.width = static_cast<float>(
            brixGraphicsTextLayoutGetLineWidth(textLayout, lineIndex)
        );
        line.baseline = static_cast<float>(
            brixGraphicsTextLayoutGetLineBaseline(textLayout, lineIndex)
        );
        line.glyphStart = static_cast<uint32_t>(run.glyphs.size());
        for (int64_t runIndex = 0; runIndex < runCount; ++runIndex) {
            if (brixGraphicsTextLayoutGetRunLine(textLayout, runIndex) != lineIndex) {
                continue;
            }
            int64_t count = brixGraphicsTextLayoutGetRunGlyphCount(textLayout, runIndex);
            if (count <= 0) {
                continue;
            }
            CTFontRef font = static_cast<CTFontRef>(
                brixGraphicsTextLayoutGetRunFont(textLayout, runIndex)
            );
            if (!font) {
                continue;
            }
            if (brixGraphicsTextLayoutIsRunObject(textLayout, runIndex)) {
                continue;
            }
            uint32_t runColor = 0;
            const bool hasRunColor = brixGraphicsTextLayoutGetRunColor(
                textLayout,
                runIndex,
                &runColor
            );
            uint16_t fontIndex = internRunFont(run, font);
            run.emSize = std::max(run.emSize, static_cast<float>(CTFontGetSize(font)));
            run.xHeight = std::max(run.xHeight, static_cast<float>(CTFontGetXHeight(font)));
            run.tinted = run.tinted || hasRunColor;
            std::vector<uint16_t> glyphs(static_cast<size_t>(count));
            std::vector<double> positions(static_cast<size_t>(count) * 2);
            brixGraphicsTextLayoutGetRunGlyphs(
                textLayout,
                runIndex,
                glyphs.data(),
                count
            );
            brixGraphicsTextLayoutGetRunPositions(
                textLayout,
                runIndex,
                positions.data(),
                count
            );
            ensureVectorCapacity(
                renderer,
                run.glyphs,
                run.glyphs.size() + static_cast<size_t>(count)
            );
            for (int64_t glyphIndex = 0; glyphIndex < count; ++glyphIndex) {
                RunGlyph glyph;
                glyph.glyph = glyphs[static_cast<size_t>(glyphIndex)];
                glyph.x = static_cast<float>(positions[static_cast<size_t>(glyphIndex) * 2]);
                glyph.y = static_cast<float>(
                    positions[static_cast<size_t>(glyphIndex) * 2 + 1]
                );
                glyph.fontIndex = fontIndex;
                glyph.hasColor = hasRunColor;
                glyph.color = runColor;
                run.glyphs.push_back(glyph);
            }
        }
        line.glyphCount = static_cast<uint32_t>(run.glyphs.size()) - line.glyphStart;
        run.lines.push_back(line);
    }
    renderer->runByLayout[layoutSerial] = index;
    renderer->maximumRunWidthDelta = std::max(
        renderer->maximumRunWidthDelta,
        run.maximumWidthDelta
    );
    countAllocation(renderer);
    return static_cast<int64_t>(
        (static_cast<uint64_t>(run.generation) << 32) | index
    );
}

extern "C" void brixGuiMetalRunRelease(
    void* pointer,
    uint32_t index,
    uint32_t generation
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || index >= renderer->runs.size()) {
        return;
    }
    RunRecord& run = renderer->runs[index];
    if (!run.alive || run.generation != generation || !run.references) {
        return;
    }
    if (--run.references) {
        return;
    }
    renderer->runByLayout.erase(run.layoutSerial);
    for (CTFontRef font : run.fonts) {
        CFRelease(font);
    }
    run.fonts.clear();
    run.glyphs.clear();
    run.lines.clear();
    run.entries.clear();
    run.layoutSerial = 0;
    run.alive = false;
    ++run.generation;
    if (!run.generation) {
        run.generation = 1;
    }
    renderer->freeRuns.push_back(index);
}

extern "C" double brixGuiMetalRunMaximumWidthDelta(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->maximumRunWidthDelta : -1;
}

extern "C" bool brixGuiMetalImageLookup(
    void* pointer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return false;
    }
    auto found = renderer->imageEntries.find(
        imageKey(index, generation, pixelWidth, pixelHeight, templateAlpha)
    );
    return found != renderer->imageEntries.end() && found->second.valid;
}

static bool upsertImage(
    MetalRenderer* renderer,
    uint64_t key,
    int32_t pixelWidth,
    int32_t pixelHeight,
    CGImageRef image
) {
    if (!renderer || !image || pixelWidth <= 0 || pixelHeight <= 0) {
        if (image) CGImageRelease(image);
        return false;
    }
    auto existing = renderer->imageEntries.find(key);
    if (existing != renderer->imageEntries.end() && existing->second.valid) {
        CGImageRelease(image);
        return true;
    }
    uint64_t bytes = static_cast<uint64_t>(pixelWidth) * pixelHeight * 4;
    if (!uploadFits(renderer, bytes)) {
        CGImageRelease(image);
        return false;
    }
    ensureVectorCapacity(renderer, renderer->rasterScratch, bytes);
    renderer->rasterScratch.resize(static_cast<size_t>(bytes));
    memset(renderer->rasterScratch.data(), 0, static_cast<size_t>(bytes));
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        renderer->rasterScratch.data(),
        pixelWidth,
        pixelHeight,
        8,
        pixelWidth * 4,
        colorSpace,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
    );
    CGColorSpaceRelease(colorSpace);
    if (!context) {
        CGImageRelease(image);
        return false;
    }
    CGContextDrawImage(context, CGRectMake(0, 0, pixelWidth, pixelHeight), image);
    CGContextRelease(context);
    CGImageRelease(image);

    uint16_t page = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    if (!allocateAtlasEntry(
        renderer,
        renderer->imageAtlas,
        renderer->imageEntries,
        pixelWidth,
        pixelHeight,
        page,
        x,
        y
    )) {
        return false;
    }
    [renderer->imageAtlas.pages[page].texture
        replaceRegion:MTLRegionMake2D(x, y, pixelWidth, pixelHeight)
        mipmapLevel:0
        withBytes:renderer->rasterScratch.data()
        bytesPerRow:pixelWidth * 4];
    renderer->frameUploadBytes += bytes;

    AtlasEntry entry;
    entry.page = page;
    entry.x = x;
    entry.y = y;
    entry.width = static_cast<uint16_t>(pixelWidth);
    entry.height = static_cast<uint16_t>(pixelHeight);
    entry.lastUsedFrame = renderer->frameCount;
    entry.valid = true;
    renderer->imageAtlas.pages[page].lastUsedFrame = renderer->frameCount;
    renderer->imageEntries[key] = entry;
    return true;
}

extern "C" bool brixGuiMetalImageUpsert(
    void* pointer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
) {
    return upsertImage(
        static_cast<MetalRenderer*>(pointer),
        imageKey(index, generation, pixelWidth, pixelHeight, templateAlpha),
        pixelWidth,
        pixelHeight,
        static_cast<CGImageRef>(cgImage)
    );
}

extern "C" bool brixGuiMetalImageUpsertSource(
    void* pointer,
    uint32_t index,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
) {
    return upsertImage(
        static_cast<MetalRenderer*>(pointer),
        imageKey(index, generation, 0, 0, templateAlpha),
        pixelWidth,
        pixelHeight,
        static_cast<CGImageRef>(cgImage)
    );
}

extern "C" bool brixGuiMetalCanvasImageUpsertSource(
    void* pointer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
) {
    return upsertImage(
        static_cast<MetalRenderer*>(pointer),
        canvasImageKey(serial, 0, 0, templateAlpha),
        pixelWidth,
        pixelHeight,
        static_cast<CGImageRef>(cgImage)
    );
}

extern "C" bool brixGuiMetalCanvasImageLookup(
    void* pointer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return false;
    }
    auto found = renderer->imageEntries.find(
        canvasImageKey(serial, pixelWidth, pixelHeight, templateAlpha)
    );
    return found != renderer->imageEntries.end() && found->second.valid;
}

extern "C" bool brixGuiMetalCanvasImageUpsert(
    void* pointer,
    uint64_t serial,
    int32_t pixelWidth,
    int32_t pixelHeight,
    bool templateAlpha,
    void* cgImage
) {
    return upsertImage(
        static_cast<MetalRenderer*>(pointer),
        canvasImageKey(serial, pixelWidth, pixelHeight, templateAlpha),
        pixelWidth,
        pixelHeight,
        static_cast<CGImageRef>(cgImage)
    );
}

static MTLPixelFormat imagePixelFormat(int32_t format) {
    return format != 0 ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
}

static uint32_t imagePixelBytes(int32_t format) {
    return format != 0 ? 8 : 4;
}

static uint64_t mipmappedTextureBytes(id<MTLTexture> texture) {
    uint64_t texelBytes = texture.pixelFormat == MTLPixelFormatRGBA16Float ? 8 : 4;
    uint64_t bytes = 0;
    for (NSUInteger level = 0; level < texture.mipmapLevelCount; ++level) {
        bytes += static_cast<uint64_t>(std::max<NSUInteger>(texture.width >> level, 1)) *
            std::max<NSUInteger>(texture.height >> level, 1) * texelBytes;
    }
    return bytes;
}

static id<MTLTexture> newImageTexture(MetalRenderer* renderer, uint32_t width, uint32_t height, int32_t format) {
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:imagePixelFormat(format)
                                     width:width
                                    height:height
                                 mipmapped:YES];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    id<MTLTexture> texture = [renderer->device newTextureWithDescriptor:descriptor];
    if (texture) {
        countAllocation(renderer);
    }
    return texture;
}

static bool createImageSurface(
    MetalRenderer* renderer,
    ImageTextureTile& surface,
    uint32_t width,
    uint32_t height,
    int32_t format
) {
    surface.texture = newImageTexture(renderer, width, height, format);
    if (!surface.texture) {
        return false;
    }
    surface.width = width;
    surface.height = height;
    surface.bytes = mipmappedTextureBytes(surface.texture);
    renderer->imageTextureBytes += surface.bytes;
    return true;
}

static void releaseImageSurface(MetalRenderer* renderer, ImageTextureTile& surface) {
    renderer->imageTextureBytes -= std::min(renderer->imageTextureBytes, surface.bytes);
    releaseImageTextureTile(surface);
    surface.bytes = 0;
}

static bool acquireImageTileSurface(MetalRenderer* renderer, ImageTextureTile& surface, int32_t format) {
    std::vector<ImageTextureTile>& pool = renderer->imageTilePool;
    auto found = std::find_if(pool.rbegin(), pool.rend(), [format](const ImageTextureTile& tile) {
        return tile.texture.pixelFormat == imagePixelFormat(format);
    });
    if (found != pool.rend()) {
        surface = std::move(*found);
        pool.erase(std::next(found).base());
        return true;
    }
    return createImageSurface(renderer, surface, kImageTileSize, kImageTileSize, format);
}

static void retireImageTileSurface(MetalRenderer* renderer, ImageTextureTile& surface) {
    if (surface.texture && renderer->imageTilePool.size() < kImageTilePoolLimit) {
        renderer->imageTilePool.push_back(std::move(surface));
        surface = ImageTextureTile();
        return;
    }
    releaseImageSurface(renderer, surface);
}

static void releaseImageTexture(MetalRenderer* renderer, ImageTexture& image) {
    releaseImageSurface(renderer, image.single);
    releaseImageSurface(renderer, image.preview);
    for (auto& entry : image.tiles) {
        retireImageTileSurface(renderer, entry.second.surface);
    }
    image.tiles.clear();
    image.source = nil;
}

struct ImageTileRegion {
    ImageTextureTile* tile;
    uint32_t bounds[4];
};

static id<MTLTexture> imageTileLevel(ImageTextureTile& tile, uint32_t level) {
    if (tile.levels.empty()) {
        tile.levels.resize(tile.texture.mipmapLevelCount, nil);
    }
    if (!tile.levels[level]) {
        tile.levels[level] = [tile.texture newTextureViewWithPixelFormat:tile.texture.pixelFormat
                                                             textureType:MTLTextureType2D
                                                                  levels:NSMakeRange(level, 1)
                                                                  slices:NSMakeRange(0, 1)];
    }
    return tile.levels[level];
}

static void encodeImageMips(
    MetalRenderer* renderer,
    id<MTLCommandBuffer> commandBuffer,
    std::vector<ImageTileRegion>& regions
) {
    NSUInteger levels = 0;
    for (const ImageTileRegion& region : regions) {
        levels = std::max(levels, region.tile->texture.mipmapLevelCount);
    }
    id<MTLComputeCommandEncoder> encoder = nil;
    for (uint32_t level = 1; level < levels; ++level) {
        bool encoded = false;
        for (ImageTileRegion& region : regions) {
            ImageTextureTile& tile = *region.tile;
            if (level >= tile.texture.mipmapLevelCount) {
                continue;
            }
            double sourceWidth = std::max<uint32_t>(tile.width >> (level - 1), 1);
            double sourceHeight = std::max<uint32_t>(tile.height >> (level - 1), 1);
            uint32_t targetWidth = std::max<uint32_t>(tile.width >> level, 1);
            uint32_t targetHeight = std::max<uint32_t>(tile.height >> level, 1);
            double ratioX = sourceWidth / targetWidth;
            double ratioY = sourceHeight / targetHeight;
            uint32_t* bounds = region.bounds;
            bounds[0] = static_cast<uint32_t>(floor(bounds[0] / ratioX));
            bounds[1] = static_cast<uint32_t>(floor(bounds[1] / ratioY));
            bounds[2] = std::min(targetWidth, static_cast<uint32_t>(ceil(bounds[2] / ratioX)));
            bounds[3] = std::min(targetHeight, static_cast<uint32_t>(ceil(bounds[3] / ratioY)));
            if (bounds[0] >= bounds[2] || bounds[1] >= bounds[3]) {
                continue;
            }
            if (!encoder) {
                encoder = [commandBuffer computeCommandEncoder];
                [encoder setComputePipelineState:renderer->shared->imageMipKernel];
            } else if (!encoded) {
                [encoder memoryBarrierWithScope:MTLBarrierScopeTextures];
            }
            encoded = true;
            [encoder setTexture:imageTileLevel(tile, level - 1) atIndex:0];
            [encoder setTexture:imageTileLevel(tile, level) atIndex:1];
            [encoder setBytes:bounds length:sizeof(region.bounds) atIndex:0];
            [encoder dispatchThreadgroups:MTLSizeMake((bounds[2] - bounds[0] + 15) / 16, (bounds[3] - bounds[1] + 15) / 16, 1)
                    threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
        }
    }
    if (encoder) {
        [encoder endEncoding];
    }
}

static void generateImageMipmaps(MetalRenderer* renderer, ImageTextureTile& surface) {
    if (surface.texture.mipmapLevelCount < 2) {
        return;
    }
    std::vector<ImageTileRegion> regions = {{&surface, {0, 0, surface.width, surface.height}}};
    id<MTLCommandBuffer> commandBuffer = rendererCommandBuffer();
    encodeImageMips(renderer, commandBuffer, regions);
    [commandBuffer commit];
}

static void flushImageTileJobs(MetalRenderer* renderer) {
    if (renderer->imageTileJobs.empty()) {
        return;
    }
    id<MTLCommandBuffer> commandBuffer = rendererCommandBuffer();
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    [encoder setComputePipelineState:renderer->shared->imageTileKernel];
    std::vector<ImageTileRegion> regions;
    for (ImageTileJob& job : renderer->imageTileJobs) {
        const ImageTileParams& params = job.params;
        [encoder setBuffer:job.source offset:0 atIndex:0];
        [encoder setBytes:&params length:sizeof(params) atIndex:1];
        [encoder setTexture:imageTileLevel(*job.surface, 0) atIndex:0];
        [encoder dispatchThreadgroups:MTLSizeMake(
                (params.regionRight - params.regionLeft + 15) / 16,
                (params.regionBottom - params.regionTop + 15) / 16,
                1
            )
                threadsPerThreadgroup:MTLSizeMake(16, 16, 1)];
        regions.push_back({
            job.surface,
            {params.regionLeft, params.regionTop, params.regionRight, params.regionBottom},
        });
    }
    [encoder endEncoding];
    encodeImageMips(renderer, commandBuffer, regions);
    [commandBuffer commit];
    renderer->imageTileJobs.clear();
}

static uint32_t imageLevelExtent(uint32_t size, uint32_t level) {
    return static_cast<uint32_t>((static_cast<uint64_t>(size) + (1ull << level) - 1) >> level);
}

static uint32_t imageTileLevels(uint32_t width, uint32_t height) {
    uint32_t level = 0;
    while (imageLevelExtent(width, level) > kImageTileContent || imageLevelExtent(height, level) > kImageTileContent) {
        ++level;
    }
    return level + 1;
}

static uint64_t imageTileKey(uint32_t level, uint32_t column, uint32_t row) {
    return (static_cast<uint64_t>(level) << 56) | (static_cast<uint64_t>(row) << 28) | column;
}

static ImageTile* findImageTile(ImageTexture& image, uint32_t level, uint32_t column, uint32_t row) {
    auto found = image.tiles.find(imageTileKey(level, column, row));
    return found == image.tiles.end() ? nullptr : &found->second;
}

static bool reserveImageTileBytes(MetalRenderer* renderer, uint64_t bytes) {
    if (renderer->frameTileBytes > 0 && renderer->frameTileBytes + bytes > kImageTileFrameBytes) {
        return false;
    }
    renderer->frameTileBytes += bytes;
    renderer->imageTileSourceBytes += bytes;
    return true;
}

static uint64_t imageTileSourceBytes(const ImageTexture& image, uint32_t level, const uint32_t* region) {
    uint64_t texels = static_cast<uint64_t>(region[2] - region[0]) * (region[3] - region[1]);
    return (texels << (level * 2)) * imagePixelBytes(image.format);
}

static void queueImageTileJob(
    MetalRenderer* renderer,
    ImageTexture& image,
    ImageTextureTile& surface,
    uint32_t level,
    uint32_t column,
    uint32_t row,
    const uint32_t* region
) {
    ImageTileParams params = {};
    params.originX = static_cast<int32_t>(column * kImageTileContent) - static_cast<int32_t>(kImageTileGutter);
    params.originY = static_cast<int32_t>(row * kImageTileContent) - static_cast<int32_t>(kImageTileGutter);
    params.regionLeft = region[0];
    params.regionTop = region[1];
    params.regionRight = region[2];
    params.regionBottom = region[3];
    params.width = image.width;
    params.height = image.height;
    params.level = level;
    params.row = image.sourceRow;
    params.format = static_cast<uint32_t>(image.format);
    renderer->imageTileJobs.push_back({&surface, image.source, params});
}

static void evictImageTextures(
    MetalRenderer* renderer,
    uint64_t budget,
    int64_t before,
    uint64_t protectedIdentity,
    bool includeRecords
) {
    while (renderer->imageTextureBytes > budget && !renderer->imageTilePool.empty()) {
        releaseImageSurface(renderer, renderer->imageTilePool.back());
        renderer->imageTilePool.pop_back();
    }
    while (renderer->imageTextureBytes > budget) {
        int64_t oldest = before;
        ImageTexture* record = nullptr;
        uint64_t recordKey = 0;
        ImageTexture* owner = nullptr;
        uint64_t tileKey = 0;
        bool preview = false;
        for (auto& entry : renderer->imageTextures) {
            ImageTexture& image = entry.second;
            if (!image.tiled) {
                if (includeRecords && !image.external && entry.first != protectedIdentity &&
                        image.lastUsedFrame < oldest) {
                    oldest = image.lastUsedFrame;
                    record = &image;
                    recordKey = entry.first;
                    owner = nullptr;
                }
                continue;
            }
            if (image.preview.texture && image.previewUsedFrame < oldest) {
                oldest = image.previewUsedFrame;
                owner = &image;
                preview = true;
                record = nullptr;
            }
            for (auto& tile : image.tiles) {
                if (tile.second.lastUsedFrame < oldest) {
                    oldest = tile.second.lastUsedFrame;
                    owner = &image;
                    tileKey = tile.first;
                    preview = false;
                    record = nullptr;
                }
            }
        }
        if (record) {
            releaseImageTexture(renderer, *record);
            renderer->imageTextures.erase(recordKey);
            ++renderer->imageTextureEvictions;
        } else if (owner && preview) {
            releaseImageSurface(renderer, owner->preview);
            owner->previewReady = false;
        } else if (owner) {
            auto tile = owner->tiles.find(tileKey);
            releaseImageSurface(renderer, tile->second.surface);
            owner->tiles.erase(tile);
        } else {
            return;
        }
    }
}

static ImageTile* ensureImageTile(
    MetalRenderer* renderer,
    ImageTexture& image,
    uint32_t level,
    uint32_t column,
    uint32_t row,
    bool& pending
) {
    const uint32_t full[4] = {0, 0, kImageTileSize, kImageTileSize};
    ImageTile* existing = findImageTile(image, level, column, row);
    if (existing) {
        existing->lastUsedFrame = renderer->frameCount;
        if (existing->dirty) {
            if (reserveImageTileBytes(renderer, imageTileSourceBytes(image, level, existing->dirtyRect))) {
                queueImageTileJob(renderer, image, existing->surface, level, column, row, existing->dirtyRect);
                existing->dirty = false;
            } else {
                pending = true;
            }
        }
        return existing;
    }
    ImageTile tile;
    if (!reserveImageTileBytes(renderer, imageTileSourceBytes(image, level, full)) ||
        !acquireImageTileSurface(renderer, tile.surface, image.format)) {
        pending = true;
        return nullptr;
    }
    tile.lastUsedFrame = renderer->frameCount;
    tile.ready = true;
    ImageTile& inserted = image.tiles.emplace(imageTileKey(level, column, row), std::move(tile)).first->second;
    queueImageTileJob(renderer, image, inserted.surface, level, column, row, full);
    evictImageTextures(renderer, imageTextureBudgetBytes(), renderer->frameCount, 0, false);
    return &inserted;
}

static bool ensureImagePreview(MetalRenderer* renderer, ImageTexture& image) {
    image.previewUsedFrame = renderer->frameCount;
    if (!image.preview.texture) {
        double ratio = std::max(image.width, image.height) / static_cast<double>(kImageTilePreviewSize);
        uint32_t width = std::max<uint32_t>(1, static_cast<uint32_t>(llround(image.width / ratio)));
        uint32_t height = std::max<uint32_t>(1, static_cast<uint32_t>(llround(image.height / ratio)));
        if (!createImageSurface(renderer, image.preview, width, height, image.format)) {
            return false;
        }
        image.previewReady = false;
    }
    if (!image.previewReady) {
        ImageTileParams params = {};
        params.regionRight = image.preview.width;
        params.regionBottom = image.preview.height;
        params.width = image.width;
        params.height = image.height;
        params.row = image.sourceRow;
        params.stepX = static_cast<float>(static_cast<double>(image.width) / image.preview.width);
        params.stepY = static_cast<float>(static_cast<double>(image.height) / image.preview.height);
        params.preview = 1;
        params.format = static_cast<uint32_t>(image.format);
        renderer->imageTileJobs.push_back({&image.preview, image.source, params});
        image.previewReady = true;
    }
    return true;
}

static void markImageTilesChanged(ImageTexture& image, const int32_t* region) {
    image.previewReady = false;
    for (auto& entry : image.tiles) {
        uint32_t level = static_cast<uint32_t>(entry.first >> 56);
        uint32_t row = static_cast<uint32_t>((entry.first >> 28) & 0xfffffffu);
        uint32_t column = static_cast<uint32_t>(entry.first & 0xfffffffu);
        int64_t factor = 1ll << level;
        int64_t extentX = imageLevelExtent(image.width, level);
        int64_t extentY = imageLevelExtent(image.height, level);
        int64_t left = std::max<int64_t>(region[0], 0) / factor;
        int64_t top = std::max<int64_t>(region[1], 0) / factor;
        int64_t right = (static_cast<int64_t>(region[2]) + factor - 1) / factor;
        int64_t bottom = (static_cast<int64_t>(region[3]) + factor - 1) / factor;
        if (left == 0) {
            left = INT32_MIN;
        }
        if (top == 0) {
            top = INT32_MIN;
        }
        if (right >= extentX) {
            right = INT32_MAX;
        }
        if (bottom >= extentY) {
            bottom = INT32_MAX;
        }
        int64_t originX = static_cast<int64_t>(column) * kImageTileContent - kImageTileGutter;
        int64_t originY = static_cast<int64_t>(row) * kImageTileContent - kImageTileGutter;
        int64_t x0 = std::max<int64_t>(0, left - originX);
        int64_t y0 = std::max<int64_t>(0, top - originY);
        int64_t x1 = std::min<int64_t>(kImageTileSize, right - originX);
        int64_t y1 = std::min<int64_t>(kImageTileSize, bottom - originY);
        if (x0 >= x1 || y0 >= y1) {
            continue;
        }
        ImageTile& tile = entry.second;
        if (tile.dirty) {
            x0 = std::min<int64_t>(x0, tile.dirtyRect[0]);
            y0 = std::min<int64_t>(y0, tile.dirtyRect[1]);
            x1 = std::max<int64_t>(x1, tile.dirtyRect[2]);
            y1 = std::max<int64_t>(y1, tile.dirtyRect[3]);
        }
        tile.dirtyRect[0] = static_cast<uint32_t>(x0);
        tile.dirtyRect[1] = static_cast<uint32_t>(y0);
        tile.dirtyRect[2] = static_cast<uint32_t>(x1);
        tile.dirtyRect[3] = static_cast<uint32_t>(y1);
        tile.dirty = true;
    }
}

static void markImageTextureChanged(ImageTexture& image, const int32_t* region) {
    uint32_t left = static_cast<uint32_t>(std::max(region[0], 0));
    uint32_t top = static_cast<uint32_t>(std::max(region[1], 0));
    uint32_t right = std::min(static_cast<uint32_t>(std::max(region[2], 0)), image.width);
    uint32_t bottom = std::min(static_cast<uint32_t>(std::max(region[3], 0)), image.height);
    if (left >= right || top >= bottom) {
        return;
    }
    if (image.updating) {
        left = std::min(left, image.update[0]);
        top = std::min(top, image.update[1]);
        right = std::max(right, image.update[2]);
        bottom = std::max(bottom, image.update[3]);
    }
    image.update[0] = left;
    image.update[1] = top;
    image.update[2] = right;
    image.update[3] = bottom;
    image.updating = true;
}

static void uploadImageTextureUpdate(
    MetalRenderer* renderer,
    ImageTexture& image,
    const uint8_t* pixels,
    size_t bytesPerRow
) {
    uint32_t left = image.update[0];
    uint32_t top = image.update[1];
    uint32_t right = image.update[2];
    uint32_t bottom = image.update[3];
    uint32_t pixelBytes = imagePixelBytes(image.format);
    uint64_t rowBytes = static_cast<uint64_t>(right - left) * pixelBytes;
    uint64_t used = renderer->frameTextureUploadBytes;
    uint64_t remaining = used < kImageTextureUploadFrameBytes ? kImageTextureUploadFrameBytes - used : 0;
    uint32_t end = static_cast<uint32_t>(std::min<uint64_t>(bottom, top + remaining / rowBytes));
    if (end <= top) {
        return;
    }
    id<MTLBuffer> staging = [renderer->device newBufferWithLength:rowBytes * (end - top)
                                                          options:MTLResourceStorageModeShared];
    if (!staging) {
        return;
    }
    countAllocation(renderer);
    uint8_t* bytes = static_cast<uint8_t*>(staging.contents);
    for (uint32_t row = top; row < end; ++row) {
        memcpy(
            bytes + static_cast<size_t>(row - top) * rowBytes,
            pixels + static_cast<size_t>(row) * bytesPerRow + static_cast<size_t>(left) * pixelBytes,
            rowBytes
        );
    }
    id<MTLCommandBuffer> commandBuffer = rendererCommandBuffer();
    id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
    [blit copyFromBuffer:staging
             sourceOffset:0
        sourceBytesPerRow:rowBytes
      sourceBytesPerImage:0
               sourceSize:MTLSizeMake(right - left, end - top, 1)
                toTexture:image.single.texture
         destinationSlice:0
         destinationLevel:0
        destinationOrigin:MTLOriginMake(left, top, 0)];
    [blit endEncoding];
    std::vector<ImageTileRegion> regions = {{&image.single, {left, top, right, end}}};
    encodeImageMips(renderer, commandBuffer, regions);
    [commandBuffer commit];
    uint64_t uploaded = rowBytes * (end - top);
    renderer->frameTextureUploadBytes += uploaded;
    renderer->imageTextureUploadBytes += uploaded;
    image.update[1] = end;
    image.updating = end < bottom;
}

static std::mutex gCanvasRenderersMutex;
static std::vector<MetalRenderer*> gCanvasRenderers;

static void noteCanvasReleased(uint64_t identity) {
    std::lock_guard<std::mutex> lock(gCanvasRenderersMutex);
    for (MetalRenderer* renderer : gCanvasRenderers) {
        renderer->releasedCanvases.push_back(identity);
    }
}

static void watchCanvasReleases(MetalRenderer* renderer) {
    std::lock_guard<std::mutex> lock(gCanvasRenderersMutex);
    brixGraphicsSetCanvasReleaseObserver(noteCanvasReleased);
    gCanvasRenderers.push_back(renderer);
}

static void unwatchCanvasReleases(MetalRenderer* renderer) {
    std::lock_guard<std::mutex> lock(gCanvasRenderersMutex);
    gCanvasRenderers.erase(
        std::remove(gCanvasRenderers.begin(), gCanvasRenderers.end(), renderer),
        gCanvasRenderers.end()
    );
}

static void releaseCanvasTextures(MetalRenderer* renderer) {
    std::vector<uint64_t> released;
    {
        std::lock_guard<std::mutex> lock(gCanvasRenderersMutex);
        released.swap(renderer->releasedCanvases);
    }
    for (uint64_t identity : released) {
        auto found = renderer->imageTextures.find(identity);
        if (found != renderer->imageTextures.end()) {
            releaseImageTexture(renderer, found->second);
            renderer->imageTextures.erase(found);
        }
    }
    if (released.empty() || renderer->imageTilePool.empty()) {
        return;
    }
    for (const auto& entry : renderer->imageTextures) {
        if (entry.second.tiled) {
            return;
        }
    }
    for (ImageTextureTile& surface : renderer->imageTilePool) {
        releaseImageSurface(renderer, surface);
    }
    renderer->imageTilePool.clear();
}

static id<MTLBuffer> newImageSourceBuffer(MetalRenderer* renderer, void* canvas) {
    void* store = brixGraphicsCanvasRetainPixelStore(canvas);
    if (!store) {
        return nil;
    }
    uint64_t length = brixGraphicsPixelStoreLength(store);
    if (length > renderer->device.maxBufferLength) {
        brixGraphicsPixelStoreRelease(store);
        return nil;
    }
    id<MTLBuffer> buffer = [renderer->device newBufferWithBytesNoCopy:brixGraphicsPixelStoreData(store)
                                                               length:length
                                                              options:MTLResourceStorageModeShared
                                                          deallocator:^(void*, NSUInteger) {
                                                              brixGraphicsPixelStoreRelease(store);
                                                          }];
    if (!buffer) {
        brixGraphicsPixelStoreRelease(store);
    }
    return buffer;
}

static bool createImageTexture(
    MetalRenderer* renderer,
    ImageTexture& image,
    void* canvas,
    const uint8_t* pixels,
    size_t bytesPerRow
) {
    if (std::max(image.width, image.height) > kImageTextureSingleLimit) {
        image.source = newImageSourceBuffer(renderer, canvas);
        if (image.source) {
            image.tiled = true;
            image.sourceRow = static_cast<uint32_t>(bytesPerRow / imagePixelBytes(image.format));
            image.levels = imageTileLevels(image.width, image.height);
            return true;
        }
    }
    if (std::max(image.width, image.height) > kImageTextureSizeLimit ||
        !createImageSurface(renderer, image.single, image.width, image.height, image.format)) {
        return false;
    }
    [image.single.texture replaceRegion:MTLRegionMake2D(0, 0, image.width, image.height)
                            mipmapLevel:0
                              withBytes:pixels
                            bytesPerRow:bytesPerRow];
    uint64_t bytes = static_cast<uint64_t>(image.width) * image.height * imagePixelBytes(image.format);
    renderer->frameTextureUploadBytes += bytes;
    renderer->imageTextureUploadBytes += bytes;
    generateImageMipmaps(renderer, image.single);
    return true;
}

static int32_t upsertExternalImageTexture(MetalRenderer* renderer, void* canvas) {
    id<MTLTexture> texture = (__bridge id<MTLTexture>)brixGraphicsCanvasGPUTexture(canvas, true);
    if (!texture) {
        return CanvasTextureMissing;
    }
    ImageTexture& image = renderer->imageTextures[brixGraphicsCanvasIdentity(canvas)];
    if (!image.external) {
        releaseImageTexture(renderer, image);
        image = ImageTexture();
        image.external = true;
    }
    if (image.single.texture != texture) {
        image.single.texture = texture;
    }
    image.width = static_cast<uint32_t>(texture.width);
    image.height = static_cast<uint32_t>(texture.height);
    image.space = brixGraphicsCanvasGetColorSpaceKind(canvas);
    image.format = brixGraphicsCanvasGetPixelFormat(canvas);
    image.serial = brixGraphicsCanvasContentSerial(canvas);
    image.lastUsedFrame = renderer->frameCount;
    return CanvasTextureReady;
}

extern "C" int32_t brixGuiMetalCanvasTextureUpsert(void* pointer, void* canvas) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !canvas) {
        return CanvasTextureMissing;
    }
    if (brixGraphicsCanvasIsGPUResident(canvas)) {
        return upsertExternalImageTexture(renderer, canvas);
    }
    const uint8_t* pixels = static_cast<const uint8_t*>(brixGraphicsCanvasGetPixelData(canvas));
    uint32_t width = static_cast<uint32_t>(brixGraphicsCanvasGetPixelWidth(canvas));
    uint32_t height = static_cast<uint32_t>(brixGraphicsCanvasGetPixelHeight(canvas));
    size_t bytesPerRow = static_cast<size_t>(brixGraphicsCanvasGetBytesPerRow(canvas));
    int32_t format = brixGraphicsCanvasGetPixelFormat(canvas);
    if (!pixels || !width || !height || brixGraphicsCanvasGetBytesPerPixel(canvas) != imagePixelBytes(format)) {
        return CanvasTextureMissing;
    }
    uint64_t identity = brixGraphicsCanvasIdentity(canvas);
    auto found = renderer->imageTextures.find(identity);
    if (found == renderer->imageTextures.end()) {
        brixGraphicsCanvasObserveChanges(canvas);
        ImageTexture image;
        image.width = width;
        image.height = height;
        image.space = brixGraphicsCanvasGetColorSpaceKind(canvas);
        image.format = format;
        image.serial = brixGraphicsCanvasContentSerial(canvas);
        image.lastUsedFrame = renderer->frameCount;
        if (!createImageTexture(renderer, image, canvas, pixels, bytesPerRow)) {
            releaseImageTexture(renderer, image);
            return CanvasTextureMissing;
        }
        found = renderer->imageTextures.emplace(identity, std::move(image)).first;
        evictImageTextures(renderer, imageTextureBudgetBytes(), renderer->frameCount, identity, true);
    }
    ImageTexture& image = found->second;
    image.lastUsedFrame = renderer->frameCount;
    uint64_t serial = brixGraphicsCanvasContentSerial(canvas);
    if (serial != image.serial) {
        int32_t region[4];
        if (brixGraphicsCanvasChangedRegion(canvas, image.serial, region)) {
            if (image.tiled) {
                markImageTilesChanged(image, region);
            } else {
                markImageTextureChanged(image, region);
            }
        }
        image.serial = serial;
    }
    if (image.updating) {
        uploadImageTextureUpdate(renderer, image, pixels, bytesPerRow);
    }
    return image.updating ? CanvasTexturePending : CanvasTextureReady;
}

extern "C" bool brixGuiMetalRendererTakeTileDamage(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->hasTileDamage) {
        return false;
    }
    renderer->takenTileDamage[0] = renderer->tileDamage[0];
    renderer->takenTileDamage[1] = renderer->tileDamage[1];
    renderer->takenTileDamage[2] = renderer->tileDamage[2] - renderer->tileDamage[0];
    renderer->takenTileDamage[3] = renderer->tileDamage[3] - renderer->tileDamage[1];
    renderer->hasTileDamage = false;
    return true;
}

extern "C" double brixGuiMetalRendererTileDamageValue(void* pointer, int32_t index) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer && index >= 0 && index < 4 ? renderer->takenTileDamage[index] : 0;
}

static void evictTiles(MetalRenderer* renderer, uint64_t budget, int64_t before, uint32_t protectedHandle);
static void settleTileOverflows(MetalRenderer* renderer);

extern "C" void brixGuiMetalPacketBegin(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || renderer->currentPacket) {
        return;
    }
    waitForPresenter(renderer);
    dispatch_semaphore_wait(renderer->inflight, DISPATCH_TIME_FOREVER);
    if (restoreTarget(renderer)) {
        renderer->fillRedraw.store(true);
    }
    settleTileOverflows(renderer);
    uint64_t pressure = memoryPressureCount.load(std::memory_order_acquire);
    if (pressure != renderer->memoryPressureSeen) {
        renderer->memoryPressureSeen = pressure;
        evictTiles(renderer, 0, renderer->frameCount - 1, UINT32_MAX);
        evictImageTextures(renderer, 0, renderer->frameCount - 1, 0, true);
    }
    renderer->packetIndex = (renderer->packetIndex + 1) % 3;
    renderer->fallbackTextures[renderer->packetIndex].clear();
    renderer->frameUploadBytes = 0;
    renderer->glyphUploadsDeferred = false;
    renderer->glyphAtlasFull = false;
    renderer->frameTextureUploadBytes = 0;
    renderer->frameTileBytes = 0;
    releaseCanvasTextures(renderer);
    renderer->maskSequence = 0;
    renderer->currentPacket = &renderer->packets[renderer->packetIndex];
    renderer->currentPacket->bytes.clear();
    renderer->currentPacket->opCount = 0;
    renderer->currentPacket->images.clear();
    renderer->packetBeginNanos = nowNanos();
}

extern "C" void brixGuiMetalPacketPushClip(
    void* pointer,
    double x,
    double y,
    double width,
    double height
) {
    ClipOp op = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height),
    };
    appendPacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPushClip, op);
}

extern "C" void brixGuiMetalPacketPushRoundedClip(
    void* pointer,
    double x, double y, double width, double height,
    double left, double top, double right, double bottom
) {
    RoundedClipOp op = {
        {static_cast<float>(x), static_cast<float>(y),
         static_cast<float>(width), static_cast<float>(height)},
        {static_cast<float>(left), static_cast<float>(top),
         static_cast<float>(right), static_cast<float>(bottom)},
    };
    appendPacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPushRoundedClip, op);
}

extern "C" void brixGuiMetalPacketPopClip(void* pointer) {
    appendBarePacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPopClip);
}

extern "C" void brixGuiMetalPacketPushTranslate(void* pointer, double x, double y) {
    TranslateOp op = {static_cast<float>(x), static_cast<float>(y)};
    appendPacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPushTranslate, op);
}

extern "C" void brixGuiMetalPacketPopTranslate(void* pointer) {
    appendBarePacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPopTransform);
}

extern "C" void brixGuiMetalPacketPushTransform(
    void* pointer,
    double a,
    double b,
    double c,
    double d,
    double tx,
    double ty
) {
    TransformOp op = {{a, b, c, d, tx, ty}};
    appendPacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPushTransform, op);
}

extern "C" void brixGuiMetalPacketPushTransformLayer(
    void* pointer,
    double a,
    double b,
    double c,
    double d,
    double tx,
    double ty,
    double alpha
) {
    TransformLayerOp op = {{a, b, c, d, tx, ty}, static_cast<float>(alpha)};
    appendPacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPushTransformLayer, op);
}

extern "C" void brixGuiMetalPacketPushLayer(void* pointer, double alpha) {
    LayerOp op = {static_cast<float>(alpha)};
    appendPacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPushLayer, op);
}

extern "C" void brixGuiMetalPacketPushFilterLayer(
    void* pointer,
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
    double alpha,
    double resolutionX,
    double resolutionY,
    const double* keeps,
    uint64_t keepCount
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    FramePacket* packet = renderer ? renderer->currentPacket : nullptr;
    if (!packet) {
        return;
    }
    uint32_t count = static_cast<uint32_t>(std::min<uint64_t>(args ? argCount : 0, kMaxFilterArgs));
    uint32_t inputs = static_cast<uint32_t>(
        std::min<uint64_t>(images && imageScales ? imageCount : 0, kMaxFilterImages)
    );
    requestShader(renderer, ShaderRoleFilter, shader);
    FilterLayerOp op = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height),
        static_cast<float>(alpha),
        static_cast<float>(time),
        shader,
        count,
        static_cast<uint32_t>(packet->images.size()),
        inputs,
        {static_cast<float>(resolutionX), static_cast<float>(resolutionY)},
        static_cast<uint32_t>(std::min<uint64_t>(keeps ? keepCount : 0, kMaxFilterKeeps)),
        {},
    };
    for (uint32_t index = 0; index < op.keepCount * 4; ++index) {
        op.keeps[index] = static_cast<float>(keeps[index]);
    }
    size_t offset = packet->bytes.size();
    size_t payload = offset + 1 + sizeof(op);
    size_t required = payload + count * sizeof(float) + inputs * 4 * sizeof(float);
    ensureVectorCapacity(renderer, packet->bytes, required);
    packet->bytes.resize(required);
    packet->bytes[offset] = static_cast<uint8_t>(PacketOpPushFilterLayer);
    memcpy(packet->bytes.data() + offset + 1, &op, sizeof(op));
    for (uint32_t index = 0; index < count; ++index) {
        float value = static_cast<float>(args[index]);
        memcpy(packet->bytes.data() + payload + index * sizeof(float), &value, sizeof(float));
    }
    uint8_t* frames = packet->bytes.data() + payload + count * sizeof(float);
    for (uint32_t index = 0; index < inputs; ++index) {
        id<MTLTexture> texture = (__bridge id<MTLTexture>)brixGraphicsCanvasGPUTexture(images[index], false);
        packet->images.push_back(texture);
        double scale = imageScales[index] > 0 ? imageScales[index] : 1;
        const float frame[4] = {
            static_cast<float>(brixGraphicsCanvasGetPixelWidth(images[index]) / scale),
            static_cast<float>(brixGraphicsCanvasGetPixelHeight(images[index]) / scale),
            static_cast<float>(scale),
            static_cast<float>(brixGraphicsCanvasGetColorSpaceKind(images[index])),
        };
        memcpy(frames + index * sizeof(frame), frame, sizeof(frame));
    }
    ++packet->opCount;
}

extern "C" void brixGuiMetalPacketPopLayer(void* pointer) {
    appendBarePacketOp(static_cast<MetalRenderer*>(pointer), PacketOpPopLayer);
}

static bool appendFillItem(
    MetalRenderer* renderer,
    const BxGuiMetalDisplayItem& item,
    uint64_t index,
    const BxGuiMetalRect* clips,
    uint64_t clipCount,
    const uint32_t* palette,
    uint64_t paletteCount,
    const double* transforms,
    uint64_t transformCount,
    const uint32_t* itemTransforms,
    const double* colors,
    uint64_t colorCount,
    const uint32_t* itemStrokes,
    const float* itemDashes
) {
    FillGeometryRecord* geometry = fillGeometryFor(renderer, item.data0, item.data1);
    FramePacket* packet = renderer->currentPacket;
    if (!geometry || !packet) {
        return false;
    }
    FillOp op = {};
    op.index = item.data0;
    op.generation = item.data1;
    op.transform[0] = 1;
    op.transform[3] = 1;
    op.strokeWidth = item.value;
    op.strokeStyle = itemStrokes ? itemStrokes[index] : 0;
    op.dash = itemDashes ? itemDashes[index] : 0;
    if (itemTransforms && transforms) {
        uint32_t slot = itemTransforms[index];
        if (slot < transformCount) {
            memcpy(op.transform, transforms + static_cast<size_t>(slot) * 6, sizeof(op.transform));
        }
    }
    if ((item.flags & 1u) && clips && item.aux < clipCount) {
        const BxGuiMetalRect& clip = clips[item.aux];
        op.contentMask[0] = static_cast<float>(clip.x);
        op.contentMask[1] = static_cast<float>(clip.y);
        op.contentMask[2] = static_cast<float>(clip.x + clip.width);
        op.contentMask[3] = static_cast<float>(clip.y + clip.height);
    }
    uint32_t inherited = ((item.flags & 8u) ? 1u : 0u) + ((item.flags & 16u) ? 1u : 0u);
    op.paletteCount = item.data3 + inherited;
    op.flags = item.flags;
    appendPacketOp(renderer, PacketOpFill, op);
    size_t offset = packet->bytes.size();
    size_t bytes = static_cast<size_t>(op.paletteCount) * 16;
    ensureVectorCapacity(renderer, packet->bytes, offset + bytes);
    packet->bytes.resize(offset + bytes);
    for (uint32_t entry = 0; entry < op.paletteCount; ++entry) {
        uint64_t source = static_cast<uint64_t>(item.data2) + entry;
        float color[4] = {0, 0, 0, 0};
        if (colors && source < colorCount) {
            for (int channel = 0; channel < 4; ++channel) {
                color[channel] = static_cast<float>(colors[source * 4 + channel]);
            }
        } else if (palette && source < paletteCount) {
            uint32_t packed = palette[source];
            for (int channel = 0; channel < 4; ++channel) {
                color[channel] = static_cast<float>((packed >> (8 * channel)) & 0xffu) / 255.0f;
            }
        }
        memcpy(packet->bytes.data() + offset + entry * 16, color, sizeof(color));
    }
    return true;
}

enum : uint32_t {
    StateRoot = 0,
    StateTransform = 1,
    StateContent = 2,
    StateClip = 3,
    StateClipOutline = 4,
    StateClipPath = 5,
    StateLayer = 6,
    StateClipMask = 7,
    StateMaskLayer = 8,
    StateClipLayerMask = 9,
    StateBlend = 10,
    StateBlur = 11,
};

static bool stateClips(uint32_t kind) {
    return kind == StateClip || kind == StateClipOutline || kind == StateClipPath ||
        kind == StateClipMask || kind == StateClipLayerMask;
}

static uint32_t chunkStatePath(const ChunkPathSet* pathSet, uint64_t state) {
    return pathSet && state < pathSet->statePaths.size() ? pathSet->statePaths[state] : UINT32_MAX;
}

extern "C" void brixGuiMetalPacketPushPathClip(void* pointer, int64_t pathSet, uint32_t state, bool evenOdd) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    uint32_t slot = static_cast<uint32_t>(static_cast<uint64_t>(pathSet) & 0xffffffffu);
    uint32_t serial = static_cast<uint32_t>(static_cast<uint64_t>(pathSet) >> 32);
    const ChunkPathSet* set = pathSet >= 0 && slot < renderer->chunkPaths.size()
        ? &renderer->chunkPaths[slot]
        : nullptr;
    uint32_t path = set && set->serial == serial ? chunkStatePath(set, state) : UINT32_MAX;
    if (path == UINT32_MAX) {
        ClipOp op = {-1.0e7f, -1.0e7f, 2.0e7f, 2.0e7f};
        appendPacketOp(renderer, PacketOpPushClip, op);
        return;
    }
    PathClipOp op = {slot, serial, path, evenOdd ? 1u : 0u};
    appendPacketOp(renderer, PacketOpPushPathClip, op);
}

enum : uint8_t { StateShapeUniform = 1 };

static uint8_t matrixShape(const double* matrix) {
    if (matrix[1] != 0 || matrix[2] != 0) {
        return 0;
    }
    return matrix[0] == matrix[3] && matrix[0] > 0 && std::isfinite(matrix[0])
        ? StateShapeUniform
        : 0;
}

enum : uint32_t { RequireKinds = 1, RequirePathSet = 2 };

static bool itemKindUniform(const BxGuiMetalDisplayItem& item, uint32_t& bits) {
    if (item.kind == 6) {
        return false;
    }
    if (item.kind == 3 || item.kind == 8 || item.kind == 13) {
        if (item.kind != 3) {
            bits |= RequirePathSet;
        }
        return false;
    }
    if (item.kind == 2 && !(item.flags & 2u)) {
        bits &= ~RequireKinds;
        return false;
    }
    if (item.kind != 0 && item.kind != 1 && item.kind != 2 && item.kind != 4 && item.kind != 7 &&
            item.kind != 9 && item.kind != 10 && item.kind != 11 && item.kind != 12 && item.kind != 14) {
        bits &= ~RequireKinds;
        return false;
    }
    return !(item.kind == 4 || item.kind == 9 || item.kind == 10 || item.kind == 11 || item.kind == 12);
}

extern "C" uint32_t brixGuiMetalItemRequirements(
    const void* scalarItemBytes,
    uint64_t count,
    const uint32_t* itemStates,
    uint64_t stateCount,
    uint8_t* requirements
) {
    const BxGuiMetalDisplayItem* items = static_cast<const BxGuiMetalDisplayItem*>(scalarItemBytes);
    uint64_t total = itemStates ? std::max<uint64_t>(stateCount, 1) : 1;
    if (!requirements || (!items && count)) {
        return 0;
    }
    memset(requirements, 0, static_cast<size_t>(total));
    uint32_t bits = RequireKinds;
    for (uint64_t index = 0; index < count; ++index) {
        uint32_t state = total > 1 ? itemStates[index] : 0;
        if (state >= total) {
            bits &= ~RequireKinds;
            continue;
        }
        if (itemKindUniform(items[index], bits)) {
            requirements[state] = 1;
        }
    }
    return bits;
}

static bool itemsDrawable(
    MetalRenderer* renderer,
    const BxGuiMetalDisplayItem* items,
    uint64_t count,
    const BxGuiMetalState* states,
    uint64_t stateCount,
    const uint32_t* itemStates,
    const ChunkPathSet* pathSet,
    const uint8_t* requirements,
    uint32_t requirementBits
) {
    uint64_t total = states && itemStates ? std::max<uint64_t>(stateCount, 1) : 1;
    std::vector<double>& matrices = renderer->stateMatrices;
    std::vector<uint8_t>& shapes = renderer->stateShapes;
    ensureVectorCapacity(renderer, matrices, static_cast<size_t>(total) * 6);
    ensureVectorCapacity(renderer, shapes, static_cast<size_t>(total));
    matrices.resize(static_cast<size_t>(total) * 6);
    shapes.resize(static_cast<size_t>(total));
    const double identity[6] = {1, 0, 0, 1, 0, 0};
    memcpy(matrices.data(), identity, sizeof(identity));
    shapes[0] = StateShapeUniform;
    for (uint64_t index = 1; index < total; ++index) {
        const BxGuiMetalState& state = states[index];
        if (state.parent >= index) {
            return false;
        }
        const double* parent = matrices.data() + static_cast<size_t>(state.parent) * 6;
        double* full = matrices.data() + static_cast<size_t>(index) * 6;
        if (state.kind == StateTransform || state.kind == StateContent) {
            fillMultiply(parent, state.matrix, full);
        } else if (state.kind == StateClipOutline || state.kind == StateClipPath) {
            if (chunkStatePath(pathSet, index) == UINT32_MAX) {
                return false;
            }
            memcpy(full, parent, sizeof(identity));
        } else if (state.kind == StateClip || state.kind == StateLayer || state.kind == StateClipMask ||
                state.kind == StateMaskLayer || state.kind == StateClipLayerMask || state.kind == StateBlend ||
                state.kind == StateBlur) {
            memcpy(full, parent, sizeof(identity));
        } else {
            return false;
        }
        shapes[index] = matrixShape(full);
    }
    if (requirements) {
        if (!(requirementBits & RequireKinds) || ((requirementBits & RequirePathSet) && !pathSet)) {
            return false;
        }
        for (uint64_t index = 0; index < total; ++index) {
            if (requirements[index] && !(shapes[index] & StateShapeUniform)) {
                return false;
            }
        }
        return true;
    }
    for (uint64_t index = 0; index < count; ++index) {
        uint32_t state = total > 1 ? itemStates[index] : 0;
        if (state >= total) {
            return false;
        }
        uint32_t bits = RequireKinds;
        bool uniform = itemKindUniform(items[index], bits);
        if (!(bits & RequireKinds) || ((bits & RequirePathSet) && !pathSet)) {
            return false;
        }
        if (uniform && !(shapes[state] & StateShapeUniform)) {
            return false;
        }
    }
    return true;
}

static void popItemState(MetalRenderer* renderer, const BxGuiMetalState* states) {
    uint32_t index = renderer->stateStack.back();
    renderer->stateStack.pop_back();
    uint32_t kind = states[index].kind;
    if (stateClips(kind)) {
        appendBarePacketOp(renderer, PacketOpPopClip);
    } else if (kind == StateLayer || kind == StateMaskLayer || kind == StateBlur) {
        appendBarePacketOp(renderer, PacketOpPopLayer);
    } else if (kind == StateBlend) {
        appendBarePacketOp(renderer, PacketOpPopBlend);
    } else {
        appendBarePacketOp(renderer, PacketOpPopTransform);
    }
}

static void syncItemState(
    MetalRenderer* renderer,
    const BxGuiMetalState* states,
    uint32_t target,
    const ChunkPathSet* pathSet,
    uint32_t pathSlot
) {
    std::vector<uint32_t>& stack = renderer->stateStack;
    if (stack.empty() ? target == 0 : stack.back() == target) {
        return;
    }
    std::vector<uint32_t>& path = renderer->statePath;
    path.clear();
    for (uint32_t cursor = target; cursor != 0; cursor = states[cursor].parent) {
        ensureVectorCapacity(renderer, path, path.size() + 1);
        path.push_back(cursor);
    }
    size_t depth = 0;
    while (depth < stack.size() && depth < path.size() &&
            stack[depth] == path[path.size() - 1 - depth]) {
        ++depth;
    }
    while (stack.size() > depth) {
        popItemState(renderer, states);
    }
    while (stack.size() < path.size()) {
        uint32_t next = path[path.size() - 1 - stack.size()];
        const BxGuiMetalState& state = states[next];
        if (state.kind == StateClip) {
            ClipOp op = {
                static_cast<float>(state.rect[0]),
                static_cast<float>(state.rect[1]),
                static_cast<float>(state.rect[2]),
                static_cast<float>(state.rect[3]),
            };
            appendPacketOp(renderer, PacketOpPushClip, op);
        } else if (state.kind == StateClipOutline || state.kind == StateClipPath) {
            PathClipOp op = {
                pathSlot,
                pathSet->serial,
                chunkStatePath(pathSet, next),
                state.kind == StateClipPath && state.fillRule ? 1u : 0u,
            };
            appendPacketOp(renderer, PacketOpPushPathClip, op);
        } else if (state.kind == StateClipMask) {
            MaskClipOp op = {
                state.serial,
                static_cast<float>(state.rect[0]),
                static_cast<float>(state.rect[1]),
                static_cast<float>(state.rect[2]),
                static_cast<float>(state.rect[3]),
            };
            appendPacketOp(renderer, PacketOpPushMaskClip, op);
        } else if (state.kind == StateMaskLayer) {
            std::vector<uint32_t>& ids = renderer->stateMaskIds;
            if (next >= ids.size()) {
                ensureVectorCapacity(renderer, ids, static_cast<size_t>(next) + 1);
                ids.resize(static_cast<size_t>(next) + 1, 0);
            }
            MaskOp op = {++renderer->maskSequence};
            ids[next] = op.id;
            appendPacketOp(renderer, PacketOpPushMaskLayer, op);
        } else if (state.kind == StateClipLayerMask) {
            const std::vector<uint32_t>& ids = renderer->stateMaskIds;
            MaskOp op = {state.source < ids.size() ? ids[state.source] : 0u};
            appendPacketOp(renderer, PacketOpPushLayerMaskClip, op);
        } else if (state.kind == StateLayer) {
            LayerOp op = {static_cast<float>(state.opacity)};
            appendPacketOp(renderer, PacketOpPushLayer, op);
        } else if (state.kind == StateBlur) {
            BlurLayerOp op = {static_cast<float>(state.radius)};
            appendPacketOp(renderer, PacketOpPushBlurLayer, op);
        } else if (state.kind == StateBlend) {
            BlendOp op = {static_cast<uint32_t>(std::max(state.blend, 0))};
            appendPacketOp(renderer, PacketOpPushBlend, op);
        } else {
            TransformOp op;
            memcpy(op.matrix, state.matrix, sizeof(op.matrix));
            appendPacketOp(renderer, state.kind == StateContent ? PacketOpPushContentTransform : PacketOpPushTransform, op);
        }
        ensureVectorCapacity(renderer, stack, stack.size() + 1);
        stack.push_back(next);
    }
}

extern "C" bool brixGuiMetalPacketAppendItems(
    void* pointer,
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
    int64_t pathSet,
    const void* quadBytes,
    uint64_t quadCount,
    const uint32_t* order,
    uint64_t orderCount,
    const uint8_t* requirements,
    uint32_t requirementBits
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    const QuadOp* quads = static_cast<const QuadOp*>(quadBytes);
    const BxGuiMetalDisplayItem* items =
        static_cast<const BxGuiMetalDisplayItem*>(scalarItemBytes);
    const BxGuiMetalRect* clips = static_cast<const BxGuiMetalRect*>(clipBytes);
    const BxGuiMetalShape* shapes = static_cast<const BxGuiMetalShape*>(shapeBytes);
    const BxGuiMetalState* states = static_cast<const BxGuiMetalState*>(stateBytes);
    if (!renderer || !renderer->currentPacket || (!items && count)) {
        return false;
    }
    uint32_t pathSlot = static_cast<uint32_t>(static_cast<uint64_t>(pathSet) & 0xffffffffu);
    uint32_t pathSerial = static_cast<uint32_t>(static_cast<uint64_t>(pathSet) >> 32);
    const ChunkPathSet* chunkPathSet =
        pathSet >= 0 && pathSlot < renderer->chunkPaths.size() && pathSerial &&
            renderer->chunkPaths[pathSlot].serial == pathSerial
        ? &renderer->chunkPaths[pathSlot]
        : nullptr;
    if (order && !requirements) {
        return false;
    }
    if (!itemsDrawable(
        renderer,
        items,
        count,
        states,
        stateCount,
        itemStates,
        chunkPathSet,
        requirements,
        requirementBits
    )) {
        return false;
    }
    bool hasStates = states && itemStates && stateCount > 1;
    uint32_t pathSpan = 0;
    renderer->stateStack.clear();
    renderer->stateMaskIds.clear();
    uint64_t visible = order ? orderCount : count;
    for (uint64_t position = 0; position < visible; ++position) {
        uint64_t index = order ? order[position] : position;
        if (index >= count) {
            continue;
        }
        const BxGuiMetalDisplayItem& item = items[index];
        if (hasStates) {
            syncItemState(renderer, states, itemStates[index], chunkPathSet, pathSlot);
        }
        const bool hasShape = (item.flags & 64u) && shapes && item.data1 < shapeCount;
        if (hasShape && (item.kind == 0 || item.kind == 1 || item.kind == 7)) {
            const BxGuiMetalShape& shape = shapes[item.data1];
            if (item.kind == 0 && shape.side < 0) {
                QuadOp op = {item.x, item.y, item.width, item.height, shape.radius, item.data0};
                appendPacketOp(renderer, PacketOpQuad, op);
                continue;
            }
            ShapeOp op = {};
            op.x = item.x;
            op.y = item.y;
            op.width = item.width;
            op.height = item.height;
            op.radius = shape.radius;
            op.strokeWidth = item.kind == 1 ? item.value : 0;
            op.side = shape.side;
            op.offset = shape.offset;
            op.size = shape.size;
            op.blur = shape.blur;
            op.shadowX = shape.x;
            op.shadowY = shape.y;
            op.spread = shape.spread;
            op.color = item.data0;
            op.kind = item.kind == 0 ? 4u : item.kind == 1 ? 5u : 6u;
            appendPacketOp(renderer, PacketOpShape, op);
        } else if (item.kind == 0) {
            QuadOp op = {
                item.x,
                item.y,
                item.width,
                item.height,
                item.value,
                item.data0,
            };
            appendPacketOp(renderer, PacketOpQuad, op);
        } else if (item.kind == 14) {
            if (quads && item.data1 <= quadCount && item.data2 <= quadCount - item.data1) {
                appendQuadsPacketOp(renderer, quads + item.data1, item.data2);
            }
        } else if (item.kind == 1) {
            BorderOp op = {
                item.x,
                item.y,
                item.width,
                item.height,
                0,
                item.value,
                item.data0,
            };
            appendPacketOp(renderer, PacketOpBorder, op);
        } else if (item.kind == 2) {
            GlyphRunOp op = {};
            op.index = item.data0;
            op.generation = item.data1;
            op.x = item.x;
            op.y = item.y;
            op.width = item.width;
            op.height = item.height;
            op.alignment = item.aux;
            op.color = item.data2;
            if ((item.flags & 1u) && clips && item.data3 < clipCount) {
                const BxGuiMetalRect& clip = clips[item.data3];
                op.contentMask[0] = static_cast<float>(clip.x);
                op.contentMask[1] = static_cast<float>(clip.y);
                op.contentMask[2] = static_cast<float>(clip.x + clip.width);
                op.contentMask[3] = static_cast<float>(clip.y + clip.height);
            }
            if (chunkPathSet && item.value >= 1) {
                op.paint = static_cast<uint32_t>(item.value);
                op.pathSlot = pathSlot;
                op.pathSerial = pathSerial;
            }
            appendPacketOp(renderer, PacketOpGlyphRun, op);
        } else if (item.kind == 3) {
            if (!appendFillItem(
                renderer,
                item,
                index,
                clips,
                clipCount,
                palette,
                paletteCount,
                transforms,
                transformCount,
                itemTransforms,
                colors,
                colorCount,
                itemStrokes,
                itemDashes
            )) {
                UnsupportedOp op = {item.kind};
                appendPacketOp(renderer, PacketOpUnsupported, op);
            }
        } else if (item.kind == 4) {
            ImageOp op = {};
            op.index = item.data0;
            op.generation = item.data1;
            op.x = item.x;
            op.y = item.y;
            op.width = item.width;
            op.height = item.height;
            op.color = (item.flags & 4u) ? item.data2 : 0xffffffffu;
            op.templateAlpha = (item.flags & 32u) ? 1u : 0u;
            if ((item.flags & 1u) && clips && item.data3 < clipCount) {
                const BxGuiMetalRect& clip = clips[item.data3];
                op.contentMask[0] = static_cast<float>(clip.x);
                op.contentMask[1] = static_cast<float>(clip.y);
                op.contentMask[2] = static_cast<float>(clip.x + clip.width);
                op.contentMask[3] = static_cast<float>(clip.y + clip.height);
            }
            appendPacketOp(renderer, PacketOpImage, op);
        } else if (item.kind == 9) {
            ImageOp op = {};
            op.identity = imageIdentities && item.aux < imageCount ? imageIdentities[item.aux] : 0;
            op.index = item.data0;
            op.generation = item.data1;
            op.canvas = 1;
            op.sampling = item.value > 0 ? 1u : 0u;
            op.x = item.x;
            op.y = item.y;
            op.width = item.width;
            op.height = item.height;
            op.color = (item.flags & 4u) ? item.data2 : 0xffffffffu;
            op.templateAlpha = (item.flags & 32u) ? 1u : 0u;
            if ((item.flags & 1u) && clips && item.data3 < clipCount) {
                const BxGuiMetalRect& clip = clips[item.data3];
                op.contentMask[0] = static_cast<float>(clip.x);
                op.contentMask[1] = static_cast<float>(clip.y);
                op.contentMask[2] = static_cast<float>(clip.x + clip.width);
                op.contentMask[3] = static_cast<float>(clip.y + clip.height);
            }
            appendPacketOp(renderer, PacketOpImage, op);
        } else if (item.kind == 11 && clips && item.data2 < clipCount) {
            const BxGuiMetalRect& tile = clips[item.data2];
            ImageOp op = {};
            op.identity = imageIdentities && item.aux < imageCount ? imageIdentities[item.aux] : 0;
            op.index = item.data0;
            op.generation = item.data1;
            op.canvas = 1;
            op.pattern = 1;
            op.sampling = item.value > 0 ? 1u : 0u;
            op.x = item.x;
            op.y = item.y;
            op.width = item.width;
            op.height = item.height;
            op.tile[0] = static_cast<float>(tile.x);
            op.tile[1] = static_cast<float>(tile.y);
            op.tile[2] = static_cast<float>(tile.width);
            op.tile[3] = static_cast<float>(tile.height);
            op.color = 0xffffffffu;
            if ((item.flags & 1u) && item.data3 < clipCount) {
                const BxGuiMetalRect& clip = clips[item.data3];
                op.contentMask[0] = static_cast<float>(clip.x);
                op.contentMask[1] = static_cast<float>(clip.y);
                op.contentMask[2] = static_cast<float>(clip.x + clip.width);
                op.contentMask[3] = static_cast<float>(clip.y + clip.height);
            }
            appendPacketOp(renderer, PacketOpImage, op);
        } else if (item.kind == 8) {
            PathOp op = {};
            op.slot = pathSlot;
            op.serial = pathSerial;
            if (order) {
                const std::vector<uint32_t>& spanItems = chunkPathSet->spanItems;
                auto found = std::lower_bound(spanItems.begin(), spanItems.end(), static_cast<uint32_t>(index));
                op.span = found != spanItems.end() && *found == index
                    ? static_cast<uint32_t>(found - spanItems.begin())
                    : UINT32_MAX;
            } else {
                op.span = pathSpan++;
            }
            if ((item.flags & 1u) && clips && item.aux < clipCount) {
                const BxGuiMetalRect& clip = clips[item.aux];
                op.contentMask[0] = static_cast<float>(clip.x);
                op.contentMask[1] = static_cast<float>(clip.y);
                op.contentMask[2] = static_cast<float>(clip.x + clip.width);
                op.contentMask[3] = static_cast<float>(clip.y + clip.height);
            }
            appendPacketOp(renderer, PacketOpPath, op);
        } else if (item.kind == 10) {
            BackdropOp op = {item.x, item.y, item.width, item.height, item.value};
            appendPacketOp(renderer, PacketOpBackdrop, op);
        } else if (item.kind == 13) {
            BackdropShaderOp op = {item.x, item.y, item.width, item.height, pathSlot, pathSerial, item.data1, item.value};
            appendPacketOp(renderer, PacketOpBackdropShader, op);
        } else if (item.kind == 12) {
            SpriteOp op = {};
            op.key = static_cast<uint64_t>(item.data0) | (static_cast<uint64_t>(item.data1) << 32);
            op.generation = item.data2;
            op.sampling = item.aux ? 1u : 0u;
            op.x = item.x;
            op.y = item.y;
            op.opacity = 1;
            appendPacketOp(renderer, PacketOpSprite, op);
        } else if (item.kind != 6) {
            UnsupportedOp op = {item.kind};
            appendPacketOp(renderer, PacketOpUnsupported, op);
        }
    }
    while (!renderer->stateStack.empty()) {
        popItemState(renderer, states);
    }
    return true;
}

extern "C" void brixGuiMetalPacketAppendTile(
    void* pointer,
    uint64_t tile,
    double x,
    double y,
    double width,
    double height,
    double opacity
) {
    TileOp op = {
        tile,
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height),
        static_cast<float>(opacity),
    };
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (renderer && tile < renderer->tiles.size()) {
        renderer->tiles[static_cast<size_t>(tile)].lastUsedFrame = renderer->frameCount;
    }
    appendPacketOp(renderer, PacketOpTile, op);
}

extern "C" void brixGuiMetalPacketAppendRasterTile(
    void* pointer,
    uint64_t tile,
    double x,
    double y,
    double width,
    double height,
    double u,
    double v,
    double uWidth,
    double vHeight,
    bool exact
) {
    RasterTileOp op = {
        tile,
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height),
        static_cast<float>(u),
        static_cast<float>(v),
        static_cast<float>(uWidth),
        static_cast<float>(vHeight),
        exact ? 1u : 0u,
    };
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (renderer && tile < renderer->tiles.size()) {
        renderer->tiles[static_cast<size_t>(tile)].lastUsedFrame = renderer->frameCount;
    }
    appendPacketOp(renderer, PacketOpRasterTile, op);
}

extern "C" uint64_t brixGuiMetalTileLookup(
    void* pointer,
    uint64_t retainedKey,
    uint32_t generation
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return 0;
    }
    auto found = renderer->tileByKey.find(retainedKey);
    if (found == renderer->tileByKey.end()) {
        return 0;
    }
    TileRecord& tile = renderer->tiles[found->second];
    return tile.generation == generation && tile.texture != nil ? found->second + 1 : 0;
}

static uint64_t tileTextureBytes(id<MTLTexture> texture) {
    if (!texture) {
        return 0;
    }
    uint64_t bytes = texture.pixelFormat == MTLPixelFormatBGRA8Unorm ? 4 : 8;
    return static_cast<uint64_t>(texture.width) * texture.height * bytes;
}

static bool isTileFrameComplete(MetalRenderer* renderer, int64_t frame) {
    return frame <= renderer->completedFrame.load();
}

static void retireTileTexture(MetalRenderer* renderer, id<MTLTexture> texture, int64_t lastUsedFrame) {
    if (!texture) {
        return;
    }
    if (renderer->retiredTiles.size() >= kRetiredTileLimit) {
        renderer->retiredTiles.erase(renderer->retiredTiles.begin());
    }
    RetiredTexture retired;
    retired.texture = texture;
    retired.lastUsedFrame = lastUsedFrame;
    renderer->retiredTiles.push_back(retired);
}

static id<MTLTexture> takeRetiredTileTexture(
    MetalRenderer* renderer,
    size_t width,
    size_t height,
    MTLPixelFormat format
) {
    for (size_t index = 0; index < renderer->retiredTiles.size(); ++index) {
        RetiredTexture& retired = renderer->retiredTiles[index];
        if (retired.texture.width == width && retired.texture.height == height &&
            retired.texture.pixelFormat == format &&
            isTileFrameComplete(renderer, retired.lastUsedFrame)) {
            id<MTLTexture> texture = retired.texture;
            renderer->retiredTiles.erase(renderer->retiredTiles.begin() + index);
            return texture;
        }
    }
    return nil;
}

static uint64_t formatTileBytes(size_t width, size_t height, MTLPixelFormat format) {
    uint64_t bytes = format == MTLPixelFormatBGRA8Unorm ? 4 : 8;
    return static_cast<uint64_t>(width) * height * bytes;
}

static void untrackTileBytes(MetalRenderer* renderer, const TileRecord& tile) {
    uint64_t bytes = tileTextureBytes(tile.texture);
    renderer->tileBytes -= std::min(renderer->tileBytes, bytes);
    if (tile.pyramid) {
        renderer->pyramidBytes -= std::min(renderer->pyramidBytes, bytes);
    }
}

static void trackTileBytes(MetalRenderer* renderer, const TileRecord& tile) {
    uint64_t bytes = tileTextureBytes(tile.texture);
    renderer->tileBytes += bytes;
    if (tile.pyramid) {
        renderer->pyramidBytes += bytes;
    }
}

static void releaseTileRecord(MetalRenderer* renderer, uint32_t handle) {
    TileRecord& tile = renderer->tiles[handle];
    untrackTileBytes(renderer, tile);
    renderer->tileByKey.erase(tile.key);
    tile = TileRecord();
    renderer->freeTiles.push_back(handle);
}

static int64_t pyramidKeepFrame(MetalRenderer* renderer, int64_t before) {
    return renderer->lastWindowFrame >= 0 ? std::min(before, renderer->lastWindowFrame) : before;
}

static uint32_t oldestTile(
    MetalRenderer* renderer,
    bool pyramid,
    bool settleOnly,
    int64_t before,
    uint32_t protectedHandle,
    int64_t reservedBefore = INT64_MAX
) {
    uint32_t victim = UINT32_MAX;
    int64_t oldest = INT64_MAX;
    for (size_t index = 0; index < renderer->tiles.size(); ++index) {
        const TileRecord& tile = renderer->tiles[index];
        if (!tile.texture || index == protectedHandle || tile.pyramid != pyramid ||
            (settleOnly && !tile.settle) || tile.lastUsedFrame >= before ||
            tile.reservedFrame >= reservedBefore) {
            continue;
        }
        if (tile.lastUsedFrame < oldest) {
            oldest = tile.lastUsedFrame;
            victim = static_cast<uint32_t>(index);
        }
    }
    return victim;
}

static uint32_t oldestPyramidTile(
    MetalRenderer* renderer,
    int64_t before,
    uint32_t protectedHandle,
    int64_t reservedBefore = INT64_MAX
) {
    uint32_t victim = oldestTile(renderer, true, true, before, protectedHandle, reservedBefore);
    return victim != UINT32_MAX ? victim :
        oldestTile(renderer, true, false, before, protectedHandle, reservedBefore);
}

static void evictTiles(MetalRenderer* renderer, uint64_t budget, int64_t before, uint32_t protectedHandle) {
    while (renderer->tileBytes > budget) {
        uint32_t victim = oldestPyramidTile(renderer, pyramidKeepFrame(renderer, before), protectedHandle);
        if (victim == UINT32_MAX) {
            victim = oldestTile(renderer, false, false, before, protectedHandle);
        }
        if (victim == UINT32_MAX) {
            return;
        }
        releaseTileRecord(renderer, victim);
    }
}

static bool evictPyramidTiles(
    MetalRenderer* renderer,
    uint64_t budget,
    uint32_t protectedHandle,
    int64_t reservedBefore
) {
    int64_t before = pyramidKeepFrame(renderer, renderer->frameCount);
    while (renderer->pyramidBytes > budget) {
        uint32_t victim = oldestPyramidTile(renderer, before, protectedHandle, reservedBefore);
        if (victim == UINT32_MAX) {
            return false;
        }
        releaseTileRecord(renderer, victim);
    }
    return true;
}

static bool isOverflowFrame(MetalRenderer* renderer, int64_t frame) {
    return frame >= 0 && renderer->tileOverflowFrames[frame & 3].load() == frame;
}

static void rejectPyramidTiles(MetalRenderer* renderer, const std::vector<uint32_t>& handles) {
    if (renderer->tileRejections.size() >= kTileRejectionLimit) {
        renderer->tileRejections.erase(renderer->tileRejections.begin());
    }
    const TileRecord& first = renderer->tiles[handles[0]];
    TileRejection rejection;
    rejection.key = first.key;
    rejection.generation = first.generation;
    rejection.tiles = static_cast<uint32_t>(handles.size());
    renderer->tileRejections.push_back(rejection);
}

static void settleTileOverflows(MetalRenderer* renderer) {
    for (size_t slot = 0; slot < 4; ++slot) {
        int64_t frame = renderer->tileOverflowFrames[slot].load();
        if (frame < 0 || frame == renderer->tileOverflowSeen[slot]) {
            continue;
        }
        renderer->tileOverflowSeen[slot] = frame;
        std::vector<uint32_t> rejected;
        for (size_t index = 0; index < renderer->tiles.size(); ++index) {
            TileRecord& tile = renderer->tiles[index];
            if (tile.renderedFrame != frame) {
                continue;
            }
            tile.renderedFrame = -1;
            if (tile.pyramid) {
                rejected.push_back(static_cast<uint32_t>(index));
            }
        }
        if (!rejected.empty()) {
            rejectPyramidTiles(renderer, rejected);
        }
    }
}

static uint32_t tileHandleFor(MetalRenderer* renderer, uint64_t retainedKey) {
    auto found = renderer->tileByKey.find(retainedKey);
    if (found != renderer->tileByKey.end()) {
        return found->second;
    }
    uint32_t handle;
    if (!renderer->freeTiles.empty()) {
        handle = renderer->freeTiles.back();
        renderer->freeTiles.pop_back();
    } else {
        ensureVectorCapacity(renderer, renderer->tiles, renderer->tiles.size() + 1);
        handle = static_cast<uint32_t>(renderer->tiles.size());
        renderer->tiles.push_back(TileRecord());
    }
    renderer->tileByKey[retainedKey] = handle;
    countAllocation(renderer);
    return handle;
}

extern "C" uint64_t brixGuiMetalTileUpsertCanvas(
    void* pointer,
    uint64_t retainedKey,
    uint32_t generation,
    void* canvas,
    double pointsWidth,
    double pointsHeight
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    const uint8_t* pixels = static_cast<const uint8_t*>(brixGraphicsCanvasGetPixelData(canvas));
    if (!renderer || !pixels) {
        return 0;
    }
    size_t width = static_cast<size_t>(brixGraphicsCanvasGetPixelWidth(canvas));
    size_t height = static_cast<size_t>(brixGraphicsCanvasGetPixelHeight(canvas));
    size_t bytesPerRow = static_cast<size_t>(brixGraphicsCanvasGetBytesPerRow(canvas));
    if (!width || !height) {
        return 0;
    }

    uint32_t handle = tileHandleFor(renderer, retainedKey);
    TileRecord& tile = renderer->tiles[handle];
    id<MTLTexture> texture = nil;
    if (tile.texture && tile.texture.width == width && tile.texture.height == height &&
        tile.texture.storageMode == MTLStorageModeShared &&
        isTileFrameComplete(renderer, tile.lastUsedFrame)) {
        texture = tile.texture;
    } else {
        untrackTileBytes(renderer, tile);
        retireTileTexture(renderer, tile.texture, tile.lastUsedFrame);
        tile.texture = nil;
        texture = takeRetiredTileTexture(renderer, width, height, MTLPixelFormatBGRA8Unorm);
        if (texture && texture.storageMode != MTLStorageModeShared) {
            retireTileTexture(renderer, texture, -1);
            texture = nil;
        }
        if (!texture) {
            MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                             width:width
                                            height:height
                                         mipmapped:NO];
            descriptor.storageMode = MTLStorageModeShared;
            texture = [renderer->device newTextureWithDescriptor:descriptor];
            countAllocation(renderer);
        }
        if (!texture) {
            renderer->tileByKey.erase(retainedKey);
            tile = TileRecord();
            renderer->freeTiles.push_back(handle);
            return 0;
        }
        tile.texture = texture;
        trackTileBytes(renderer, tile);
    }
    [texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
               mipmapLevel:0
                 withBytes:pixels
               bytesPerRow:bytesPerRow];

    tile.texture = texture;
    tile.key = retainedKey;
    tile.generation = generation;
    tile.pointsWidth = static_cast<float>(pointsWidth);
    tile.pointsHeight = static_cast<float>(pointsHeight);
    tile.pixelWidth = static_cast<uint32_t>(width);
    tile.pixelHeight = static_cast<uint32_t>(height);
    tile.lastUsedFrame = -1;
    evictTiles(renderer, tileBudgetBytes(), renderer->frameCount, handle);
    return handle + 1;
}

extern "C" uint64_t brixGuiMetalTileLookupRendered(
    void* pointer,
    uint64_t retainedKey,
    uint32_t generation
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return 0;
    }
    auto found = renderer->tileByKey.find(retainedKey);
    if (found == renderer->tileByKey.end()) {
        return 0;
    }
    const TileRecord& tile = renderer->tiles[found->second];
    if (tile.generation != generation || tile.texture == nil ||
            tile.texture.pixelFormat != renderer->layerFormat ||
            tile.renderedFrame < 0 || isOverflowFrame(renderer, tile.renderedFrame)) {
        return 0;
    }
    return found->second + 1;
}


extern "C" bool brixGuiMetalRendererPyramidFull(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer && renderer->pyramidFull;
}

extern "C" bool brixGuiMetalRendererTakeTileRejection(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || renderer->tileRejections.empty()) {
        return false;
    }
    renderer->takenRejection = renderer->tileRejections.front();
    renderer->tileRejections.erase(renderer->tileRejections.begin());
    return true;
}

extern "C" uint64_t brixGuiMetalRendererTileRejectionKey(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->takenRejection.key : 0;
}

extern "C" uint32_t brixGuiMetalRendererTileRejectionGeneration(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->takenRejection.generation : 0;
}

extern "C" uint32_t brixGuiMetalRendererTileRejectionTiles(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->takenRejection.tiles : 0;
}

extern "C" int64_t brixGuiMetalRendererFrameCount(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer ? renderer->frameCount : 0;
}

extern "C" int64_t brixGuiMetalRendererPyramidTileLimit(void* pointer, int32_t pixels) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || pixels <= 0) {
        return 0;
    }
    size_t side = static_cast<size_t>(pixels);
    return static_cast<int64_t>(pyramidBudgetBytes() / formatTileBytes(side, side, renderer->layerFormat));
}

extern "C" uint64_t brixGuiMetalTileReserve(
    void* pointer,
    uint64_t retainedKey,
    uint32_t generation,
    int32_t pixelWidth,
    int32_t pixelHeight,
    double pointsWidth,
    double pointsHeight,
    int32_t slotX,
    int32_t slotY,
    int32_t pool,
    bool settle,
    int64_t keepSince
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->currentPacket || pixelWidth <= 0 || pixelHeight <= 0 ||
            slotX < 0 || slotY < 0 || slotX + pixelWidth > renderer->pixelWidth ||
            slotY + pixelHeight > renderer->pixelHeight) {
        return 0;
    }
    size_t width = static_cast<size_t>(pixelWidth);
    size_t height = static_cast<size_t>(pixelHeight);
    MTLPixelFormat format = renderer->layerFormat;
    bool pyramid = pool != BxGuiMetalTilePoolGeneral;
    if (pyramid) {
        renderer->pyramidFull = false;
        auto existing = renderer->tileByKey.find(retainedKey);
        uint32_t held = existing != renderer->tileByKey.end() ? existing->second : UINT32_MAX;
        uint64_t holding = held != UINT32_MAX ? tileTextureBytes(renderer->tiles[held].texture) : 0;
        uint64_t needed = formatTileBytes(width, height, format);
        uint64_t budget = pyramidBudgetBytes();
        uint64_t room = budget + holding > needed ? budget + holding - needed : 0;
        bool fits = renderer->pyramidBytes <= room ||
            evictPyramidTiles(renderer, room, held, keepSince);
        if (!fits) {
            renderer->pyramidFull = true;
            return 0;
        }
    }
    uint32_t handle = tileHandleFor(renderer, retainedKey);
    TileRecord& tile = renderer->tiles[handle];
    if (tile.pyramid != pyramid) {
        untrackTileBytes(renderer, tile);
        tile.pyramid = pyramid;
        trackTileBytes(renderer, tile);
    }
    tile.settle = pyramid && settle;
    id<MTLTexture> texture = nil;
    if (tile.texture && tile.texture.width == width && tile.texture.height == height &&
        tile.texture.pixelFormat == format && tile.texture.storageMode == MTLStorageModePrivate &&
        isTileFrameComplete(renderer, tile.lastUsedFrame)) {
        texture = tile.texture;
    } else {
        untrackTileBytes(renderer, tile);
        retireTileTexture(renderer, tile.texture, tile.lastUsedFrame);
        tile.texture = nil;
        texture = takeRetiredTileTexture(renderer, width, height, format);
        if (texture && texture.storageMode != MTLStorageModePrivate) {
            retireTileTexture(renderer, texture, -1);
            texture = nil;
        }
        if (!texture) {
            MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:format
                                             width:width
                                            height:height
                                         mipmapped:NO];
            descriptor.usage = MTLTextureUsageShaderRead;
            descriptor.storageMode = MTLStorageModePrivate;
            texture = [renderer->device newTextureWithDescriptor:descriptor];
            countAllocation(renderer);
        }
        if (!texture) {
            renderer->tileByKey.erase(retainedKey);
            tile = TileRecord();
            renderer->freeTiles.push_back(handle);
            return 0;
        }
        tile.texture = texture;
        trackTileBytes(renderer, tile);
    }
    tile.texture = texture;
    tile.key = retainedKey;
    tile.generation = generation;
    tile.pointsWidth = static_cast<float>(pointsWidth);
    tile.pointsHeight = static_cast<float>(pointsHeight);
    tile.pixelWidth = static_cast<uint32_t>(width);
    tile.pixelHeight = static_cast<uint32_t>(height);
    tile.lastUsedFrame = renderer->frameCount;
    tile.renderedFrame = renderer->frameCount;
    tile.reservedFrame = renderer->frameCount;
    ensureVectorCapacity(renderer, renderer->tileCopies, renderer->tileCopies.size() + 1);
    renderer->tileCopies.push_back(TileCopy{
        handle,
        static_cast<uint32_t>(slotX),
        static_cast<uint32_t>(slotY),
    });
    evictTiles(renderer, tileBudgetBytes(), renderer->frameCount, handle);
    return handle + 1;
}

static bool ensureTileStaging(MetalRenderer* renderer) {
    id<MTLTexture> staging = renderer->tileStaging;
    if (staging && staging.width == static_cast<NSUInteger>(renderer->pixelWidth) &&
            staging.height == static_cast<NSUInteger>(renderer->pixelHeight) &&
            staging.pixelFormat == renderer->layerFormat) {
        return true;
    }
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:renderer->layerFormat
                                     width:static_cast<NSUInteger>(renderer->pixelWidth)
                                    height:static_cast<NSUInteger>(renderer->pixelHeight)
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModePrivate;
    id<MTLTexture> replacement = [renderer->device newTextureWithDescriptor:descriptor];
    if (!replacement) {
        return false;
    }
    renderer->tileStaging = replacement;
    countAllocation(renderer);
    return true;
}

static void abandonTileCopies(MetalRenderer* renderer, bool counted) {
    std::vector<uint32_t> rejected;
    for (const TileCopy& copy : renderer->tileCopies) {
        TileRecord& tile = renderer->tiles[copy.handle];
        if (counted && tile.pyramid) {
            rejected.push_back(copy.handle);
        }
    }
    if (!rejected.empty()) {
        rejectPyramidTiles(renderer, rejected);
    }
    for (const TileCopy& copy : renderer->tileCopies) {
        TileRecord& tile = renderer->tiles[copy.handle];
        tile.generation = 0;
        tile.renderedFrame = -1;
    }
    renderer->tileCopies.clear();
}

static void releaseChunkPathSet(MetalRenderer* renderer, uint32_t slot);

extern "C" void brixGuiMetalChunkRelease(void* pointer, uint64_t retainedKey) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return;
    }
    auto tile = renderer->tileByKey.find(retainedKey);
    if (tile != renderer->tileByKey.end()) {
        releaseTileRecord(renderer, tile->second);
    }
    auto paths = renderer->chunkPathByKey.find(retainedKey);
    if (paths != renderer->chunkPathByKey.end()) {
        releaseChunkPathSet(renderer, paths->second);
    }
}

extern "C" void brixGuiMetalChunkReleaseAll(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return;
    }
    for (size_t index = 0; index < renderer->tiles.size(); ++index) {
        if (renderer->tiles[index].texture) {
            releaseTileRecord(renderer, static_cast<uint32_t>(index));
        }
    }
    for (size_t index = 0; index < renderer->chunkPaths.size(); ++index) {
        if (renderer->chunkPaths[index].serial) {
            releaseChunkPathSet(renderer, static_cast<uint32_t>(index));
        }
    }
}

static bool encodeFrame(
    MetalRenderer* renderer,
    int64_t frameId,
    int32_t frameSlot,
    int32_t mode,
    double damageX,
    double damageY,
    double damageWidth,
    double damageHeight,
    uint64_t encodeStarted,
    bool tiles
) {
    FrameStats& stats = renderer->frames[frameSlot];
    bool partial = mode == 1 && damageWidth > 0 && damageHeight > 0;
    std::vector<MTLScissorRect> damageAreas;
    if (partial) {
        const MTLScissorRect bounds = clipToScissor(
            renderer,
            ClipState{damageX, damageY, damageX + damageWidth, damageY + damageHeight}
        );
        const std::vector<double>& values = renderer->encodeDamageAreas;
        for (size_t index = 0; index + 3 < values.size() && damageAreas.size() < kMaxDamageAreas; index += 4) {
            MTLScissorRect area = intersectScissors(
                clipToScissor(
                    renderer,
                    ClipState{
                        values[index],
                        values[index + 1],
                        values[index] + values[index + 2],
                        values[index + 1] + values[index + 3],
                    }
                ),
                bounds
            );
            if (area.width && area.height) {
                damageAreas.push_back(area);
            }
        }
        if (damageAreas.empty()) {
            damageAreas.push_back(bounds);
        }
    }
    size_t mainCount = renderer->instances.size();
    size_t gatherCount = renderer->instanceGatherCount;
    size_t totalCount = mainCount + gatherCount + damageAreas.size();
    id<MTLBuffer> instanceBuffer = ensureInstanceBuffer(
        renderer,
        &renderer->instanceBuffers[renderer->packetIndex],
        std::max<size_t>(1, totalCount) * sizeof(Instance)
    );
    if (!instanceBuffer) {
        dispatch_semaphore_signal(renderer->inflight);
        return false;
    }
    id<MTLBuffer> clipBuffer = ensureInstanceBuffer(
        renderer,
        &renderer->roundedClipBuffers[renderer->packetIndex],
        std::max<size_t>(1, renderer->roundedClips.size()) * sizeof(RoundedClip)
    );
    if (!clipBuffer) {
        dispatch_semaphore_signal(renderer->inflight);
        return false;
    }
    if (!renderer->roundedClips.empty()) {
        memcpy(clipBuffer.contents, renderer->roundedClips.data(),
               renderer->roundedClips.size() * sizeof(RoundedClip));
    }
    Instance* destination = static_cast<Instance*>(instanceBuffer.contents);
    if (mainCount) {
        memcpy(destination, renderer->instances.data(), mainCount * sizeof(Instance));
    }
    Instance* tail = destination + mainCount;
    const std::vector<uint32_t>& gather = renderer->instanceGather;
    for (size_t index = 0; index + 1 < gather.size(); index += 2) {
        memcpy(tail, renderer->instances.data() + gather[index], gather[index + 1] * sizeof(Instance));
        tail += gather[index + 1];
    }
    for (size_t index = 0; index < damageAreas.size(); ++index) {
        const MTLScissorRect& area = damageAreas[index];
        Instance background = {};
        setFullContentMask(renderer, background);
        background.rect[0] = static_cast<float>(area.x);
        background.rect[1] = static_cast<float>(area.y);
        background.rect[2] = static_cast<float>(area.width);
        background.rect[3] = static_cast<float>(area.height);
        background.color = packColor(
            renderer->background[0],
            renderer->background[1],
            renderer->background[2],
            renderer->background[3]
        );
        background.kind = 0;
        background.opacity = 1;
        destination[mainCount + gatherCount + index] = background;
    }

    id<MTLCommandBuffer> commandBuffer = rendererCommandBuffer();
    uint32_t drawCalls = 0;
    uint32_t pipelineSwitches = 0;
    id<CAMetalDrawable> drawable = nil;
    uint64_t drawableWaitNanos = 0;

    MTLScissorRect fullArea = {
        0,
        0,
        static_cast<NSUInteger>(renderer->pixelWidth),
        static_cast<NSUInteger>(renderer->pixelHeight),
    };
    ClipState damageClip = {
        damageX,
        damageY,
        damageX + damageWidth,
        damageY + damageHeight,
    };
    MTLScissorRect mainArea = partial ? clipToScissor(renderer, damageClip) : fullArea;
    for (FillBatchInfo& info : renderer->fillBatches) {
        info.scissor = MTLScissorRect{0, 0, 0, 0};
    }
    for (const std::vector<uint32_t>* indices : {&renderer->layerOrder, &renderer->deferredLayers}) {
        for (uint32_t index : *indices) {
            const LayerPass& layer = *renderer->layers[index];
            MTLScissorRect area = {layer.origin[0], layer.origin[1], layer.size[0], layer.size[1]};
            assignFillScissors(renderer, layer.batches, area);
        }
    }
    for (uint32_t generation = 0; generation < renderer->shadowGenerations; ++generation) {
        assignFillScissors(renderer, renderer->shadowBatches[generation], fullArea);
    }
    assignFillScissors(renderer, renderer->batches, mainArea);

    if (!fillPrepare(renderer, stats)) {
        dispatch_semaphore_signal(renderer->inflight);
        return false;
    }
    if (renderer->fillActive) {
        fillEncodeCompute(renderer, commandBuffer);
    }

    if (!ensureMaskPages(renderer)) {
        dispatch_semaphore_signal(renderer->inflight);
        return false;
    }
    uint32_t clearedMaskPages = 0;
    if (!renderer->shadowRecords.empty() && !encodeShadows(
            renderer,
            commandBuffer,
            instanceBuffer,
            fullArea,
            clearedMaskPages,
            drawCalls,
            pipelineSwitches
        )) {
        dispatch_semaphore_signal(renderer->inflight);
        return false;
    }
    const std::vector<uint32_t>& order = renderer->layerOrder;
    for (size_t position = 0; position < order.size();) {
        const LayerPass& first = *renderer->layers[order[position]];
        if (!first.mask) {
            if (!encodeLayerPass(
                    renderer,
                    commandBuffer,
                    order[position],
                    instanceBuffer,
                    drawCalls,
                    pipelineSwitches
                )) {
                dispatch_semaphore_signal(renderer->inflight);
                return false;
            }
            ++position;
            continue;
        }
        id<MTLTexture> texture = renderer->maskPages[first.page];
        size_t runEnd = position + 1;
        while (runEnd < order.size() && renderer->layers[order[runEnd]]->mask &&
                renderer->layers[order[runEnd]]->page == first.page) {
            ++runEnd;
        }
        bool clears = !(clearedMaskPages & (1u << first.page));
        clearedMaskPages |= 1u << first.page;
        MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = texture;
        pass.colorAttachments[0].loadAction = clears ? MTLLoadActionClear : MTLLoadActionLoad;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
        pass.depthAttachment.texture = renderer->maskDepth;
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.storeAction = MTLStoreActionDontCare;
        pass.depthAttachment.clearDepth = 1.0;
        id<MTLRenderCommandEncoder> encoder =
            [commandBuffer renderCommandEncoderWithDescriptor:pass];
        size_t runStart = position;
        for (; position < runEnd; ++position) {
            const LayerPass& layer = *renderer->layers[order[position]];
            PassTarget target = {
                {layer.origin[0], layer.origin[1], layer.size[0], layer.size[1]},
                {0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                static_cast<int32_t>(layer.page),
                texture.pixelFormat,
            };
            encodeBatches(
                renderer,
                encoder,
                layer.batches.data(),
                layer.batches.size(),
                instanceBuffer,
                target,
                drawCalls,
                pipelineSwitches,
                position == runStart
            );
        }
        [encoder endEncoding];
    }

    bool canPresent = !tiles && renderer->presentationLayer && !presentationIsHeld(renderer);
    bool defersPresent = canPresent && renderer->presentQueue &&
        !renderer->presentationLayer.presentsWithTransaction;
    bool acquiresDrawable = canPresent && !defersPresent;
    id<MTLTexture> mainTarget = tiles ? renderer->tileStaging : renderer->target;
    if (acquiresDrawable && !partial) {
        uint64_t waitStarted = nowNanos();
        drawable = [renderer->presentationLayer nextDrawable];
        drawableWaitNanos += nowNanos() - waitStarted;
        if (!drawable) {
            stats.drawableWaitNanos = drawableWaitNanos;
            uint64_t elapsed = nowNanos() - encodeStarted;
            stats.encodeNanos = elapsed > drawableWaitNanos
                ? elapsed - drawableWaitNanos
                : 0;
            dispatch_semaphore_signal(renderer->inflight);
            return false;
        }
        mainTarget = drawable.texture;
    }
    if (!fillEnsureDepth(renderer, mainTarget.width, mainTarget.height)) {
        dispatch_semaphore_signal(renderer->inflight);
        return false;
    }

    SinkTarget mainSink = {
        mainTarget,
        renderer->depthTexture,
        {
            mainArea,
            {
                0,
                0,
                static_cast<float>(mainTarget.width),
                static_cast<float>(mainTarget.height),
            },
            -1,
            mainTarget.pixelFormat,
        },
        {0, 0},
        {renderer->pixelWidth, renderer->pixelHeight},
    };
    if (damageAreas.size() > 1) {
        mainSink.pass.areas = damageAreas.data();
        mainSink.pass.areaCount = damageAreas.size();
    }
    Batch background = {
        PipelineClear,
        nil,
        mainArea,
        static_cast<uint32_t>(mainCount + gatherCount),
        static_cast<uint32_t>(damageAreas.size()),
        false,
    };
    if (!encodeSink(
            renderer,
            commandBuffer,
            renderer->batches,
            mainSink,
            partial ? MTLLoadActionLoad : MTLLoadActionClear,
            tiles ? MTLClearColorMake(0, 0, 0, 0) : MTLClearColorMake(
                renderer->background[0] * renderer->background[3],
                renderer->background[1] * renderer->background[3],
                renderer->background[2] * renderer->background[3],
                renderer->background[3]
            ),
            partial && mainArea.width && mainArea.height ? &background : nullptr,
            instanceBuffer,
            drawCalls,
            pipelineSwitches
        )) {
        dispatch_semaphore_signal(renderer->inflight);
        return false;
    }

    int64_t damageSerial = tiles
        ? 0
        : recordPresentDamage(
            *renderer->presentDamage,
            partial ? damageAreas.data() : &fullArea,
            partial ? damageAreas.size() : 1
        );
    if (acquiresDrawable) {
        if (partial) {
            uint64_t waitStarted = nowNanos();
            drawable = [renderer->presentationLayer nextDrawable];
            drawableWaitNanos += nowNanos() - waitStarted;
            if (!drawable) {
                stats.drawableWaitNanos = drawableWaitNanos;
                uint64_t elapsed = nowNanos() - encodeStarted;
                stats.encodeNanos = elapsed > drawableWaitNanos
                    ? elapsed - drawableWaitNanos
                    : 0;
                dispatch_semaphore_signal(renderer->inflight);
                return false;
            }
        }
        std::vector<MTLScissorRect> copies = presentCopyAreas(
            *renderer->presentDamage,
            drawable.texture,
            damageSerial,
            fullArea.width,
            fullArea.height
        );
        if (partial) {
            copyPresentAreas(commandBuffer, renderer->target, drawable.texture, copies);
        } else {
            copyPresentAreas(commandBuffer, drawable.texture, renderer->target, {fullArea});
        }
    }

    if (tiles && !renderer->tileCopies.empty()) {
        id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
        for (const TileCopy& copy : renderer->tileCopies) {
            const TileRecord& tile = renderer->tiles[copy.handle];
            [blit copyFromTexture:mainTarget
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake(copy.x, copy.y, 0)
                       sourceSize:MTLSizeMake(tile.pixelWidth, tile.pixelHeight, 1)
                        toTexture:tile.texture
                 destinationSlice:0
                 destinationLevel:0
                destinationOrigin:MTLOriginMake(0, 0, 0)];
        }
        [blit endEncoding];
    }

    id<MTLTexture> output = renderer->outputTexture;
    if (!tiles && output && output.pixelFormat == renderer->target.pixelFormat) {
        copyPresentAreas(
            commandBuffer,
            renderer->target,
            output,
            presentCopyAreas(
                *renderer->presentDamage,
                output,
                damageSerial,
                std::min<NSUInteger>(renderer->target.width, output.width),
                std::min<NSUInteger>(renderer->target.height, output.height)
            )
        );
    }

    stats.drawCalls = drawCalls;
    stats.pipelineSwitches = pipelineSwitches;
    stats.instances = static_cast<uint32_t>(totalCount);
    stats.drawableWaitNanos = drawableWaitNanos;
    bool isPresenting = drawable != nil;
    bool presentsWithTransaction =
        isPresenting && renderer->presentationLayer.presentsWithTransaction;
    bool recordsPresentation = isPresenting || defersPresent;
    if (recordsPresentation) {
        std::shared_ptr<PresentationState> presentedState = renderer->presentationState;
        std::lock_guard<std::mutex> lock(presentedState->mutex);
        PresentationRecord& presentation = presentedState->records[
            static_cast<size_t>(frameId) % presentedState->records.size()
        ];
        presentation = PresentationRecord();
        presentation.frame = frameId;
        presentation.drawableWaitNanos = drawableWaitNanos;
    }
    if (isPresenting) {
        watchPresentation(renderer->presentationState, drawable, frameId);
        if (!presentsWithTransaction) {
            [commandBuffer presentDrawable:drawable];
        }
    }
    if (recordsPresentation) {
        ++renderer->presentCount;
    }
    bool hadFill = renderer->fillActive;
    id<MTLBuffer> fillStats = hadFill ? renderer->fillSlots[renderer->packetIndex].stats : nil;
    if (hadFill) {
        renderer->fillFramesPending.fetch_add(1);
    }
    dispatch_semaphore_t inflight = renderer->inflight;
    PresentationState* presentationState = renderer->presentationState.get();
    bool tileFrame = tiles;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> completed) {
        double start = completed.GPUStartTime;
        double end = completed.GPUEndTime;
        int64_t gpuNanos = end >= start
            ? static_cast<int64_t>((end - start) * 1e9)
            : -1;
        renderer->frames[frameSlot].gpuNanos = gpuNanos;
        if (tileFrame && gpuNanos > 0) {
            renderer->tileGpuNanos.fetch_add(gpuNanos);
        }
        noteCompletedFrame(renderer, frameId);
        if (recordsPresentation) {
            std::lock_guard<std::mutex> lock(presentationState->mutex);
            PresentationRecord& record = presentationState->records[
                static_cast<size_t>(frameId) % presentationState->records.size()
            ];
            if (record.frame == frameId) {
                record.gpuNanos = gpuNanos;
            }
        }
        if (hadFill) {
            const uint32_t* values = static_cast<const uint32_t*>(fillStats.contents);
            FrameStats& frame = renderer->frames[frameSlot];
            frame.fillPieces = values[0];
            frame.fillLines = values[2];
            frame.fillOpaqueTiles = values[5];
            uint64_t pieces = values[0];
            uint64_t high = renderer->fillPieceHigh.load();
            while (pieces > high && !renderer->fillPieceHigh.compare_exchange_weak(high, pieces)) {
            }
            if (values[1]) {
                renderer->fillOverflows.fetch_add(1);
                renderer->fillRedraw.store(true);
                if (tileFrame) {
                    renderer->tileOverflowFrames[frameId & 3].store(frameId);
                }
            }
            renderer->fillFramesPending.fetch_sub(1);
        }
        dispatch_semaphore_signal(inflight);
    }];
    [commandBuffer commit];
    renderer->committedBuffers[static_cast<size_t>(frameId) & 3] = commandBuffer;
    renderer->committedFrames[static_cast<size_t>(frameId) & 3] = frameId;
    if (defersPresent) {
        enqueuePresent(renderer, frameId, damageSerial);
        return true;
    }
    if (presentsWithTransaction) {
        [commandBuffer waitUntilScheduled];
        [drawable present];
    }
    return true;
}

static int64_t commitPacket(
    MetalRenderer* renderer,
    int32_t mode,
    double damageX,
    double damageY,
    double damageWidth,
    double damageHeight,
    bool tiles
) {
    FramePacket* packet = renderer->currentPacket;
    renderer->currentPacket = nullptr;
    int64_t frameId = renderer->frameCount++;
    int32_t frameSlot = renderer->packetIndex;
    if (!tiles) {
        renderer->lastFrame = frameSlot;
        renderer->lastWindowFrame = frameId;
    }
    FrameStats& stats = renderer->frames[frameSlot];
    stats = FrameStats();
    stats.walkNanos = nowNanos() - renderer->packetBeginNanos;
    stats.packetBytes = packet->bytes.size();
    uint64_t encodeStarted = nowNanos();
    renderer->encodeMode = mode;
    renderer->encodeDamage[0] = damageX;
    renderer->encodeDamage[1] = damageY;
    renderer->encodeDamage[2] = damageWidth;
    renderer->encodeDamage[3] = damageHeight;
    renderer->encodeDamageAreas.swap(renderer->pendingDamageAreas);
    renderer->pendingDamageAreas.clear();
    if (tiles) {
        renderer->encodeDamageAreas.clear();
    }
    bool encoded = false;
    renderer->skippedPendingShader = false;
    renderer->tileGlyphsDeferred = false;
    renderer->tileGlyphsMissing = false;
    @autoreleasepool {
        fillCompactSegments(renderer);
        compileRequestedShaders(renderer);
        expandPacket(renderer, packet, stats, tiles);
        flushImageTileJobs(renderer);
        encoded = encodeFrame(
            renderer,
            frameId,
            frameSlot,
            mode,
            damageX,
            damageY,
            damageWidth,
            damageHeight,
            encodeStarted,
            tiles
        );
    }
    renderer->canReencode = !tiles && encoded && !renderer->presentationLayer;
    if (!encoded) {
        return -1;
    }
    uint64_t encodeElapsed = nowNanos() - encodeStarted;
    stats.encodeNanos = encodeElapsed > stats.drawableWaitNanos
        ? encodeElapsed - stats.drawableWaitNanos
        : 0;
    return frameId;
}

extern "C" int64_t brixGuiMetalPacketCommit(
    void* pointer,
    int32_t mode,
    double damageX,
    double damageY,
    double damageWidth,
    double damageHeight
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->currentPacket) {
        return -1;
    }
    return commitPacket(renderer, mode, damageX, damageY, damageWidth, damageHeight, false);
}

extern "C" void brixGuiMetalPacketSetDamageAreas(void* pointer, const double* values, int64_t count) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return;
    }
    size_t kept = values && count > 0 ? std::min(static_cast<size_t>(count), kMaxDamageAreas) : 0;
    renderer->pendingDamageAreas.assign(values, values + kept * 4);
}

extern "C" int64_t brixGuiMetalPacketCommitTiles(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->currentPacket) {
        return -1;
    }
    if (renderer->tileCopies.empty() || !ensureTileStaging(renderer)) {
        abandonTileCopies(renderer, false);
        renderer->currentPacket = nullptr;
        dispatch_semaphore_signal(renderer->inflight);
        return -1;
    }
    int64_t frameId = commitPacket(renderer, 0, 0, 0, 0, 0, true);
    if (frameId < 0) {
        abandonTileCopies(renderer, false);
        return -1;
    }
    bool waits = renderer->skippedPendingShader || renderer->tileGlyphsDeferred;
    if (waits || renderer->tileGlyphsMissing) {
        abandonTileCopies(renderer, !waits);
        return frameId;
    }
    renderer->tilesRendered += static_cast<int64_t>(renderer->tileCopies.size());
    renderer->tileCopies.clear();
    return frameId;
}

extern "C" void brixGuiMetalRendererStatMark(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (renderer) {
        renderer->markedRendererAllocations = renderer->rendererAllocations;
    }
}

extern "C" int64_t brixGuiMetalRendererStatValue(void* pointer, int32_t field) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return -1;
    }
    if (field == BxGuiMetalStatRendererAllocations) {
        return static_cast<int64_t>(
            renderer->rendererAllocations - renderer->markedRendererAllocations
        );
    }
    if (field == BxGuiMetalStatCommits) {
        return renderer->frameCount;
    }
    if (field == BxGuiMetalStatPresents) {
        return static_cast<int64_t>(renderer->presentCount);
    }
    if (field == BxGuiMetalStatMaximumFramesPerSecond) {
        return renderer->maximumFramesPerSecond;
    }
    if (field == BxGuiMetalStatTileGpuNanos) {
        return renderer->tileGpuNanos.load();
    }
    if (field == BxGuiMetalStatTilesRendered) {
        return renderer->tilesRendered;
    }
    if (field == BxGuiMetalStatPacketCapacityBytes) {
        uint64_t capacity = 0;
        for (const FramePacket& packet : renderer->packets) {
            capacity += packet.bytes.capacity();
        }
        for (id<MTLBuffer> buffer : renderer->instanceBuffers) {
            capacity += buffer ? buffer.length : 0;
        }
        for (id<MTLBuffer> buffer : renderer->roundedClipBuffers) {
            capacity += buffer ? buffer.length : 0;
        }
        return static_cast<int64_t>(capacity);
    }
    if (field == BxGuiMetalStatGlyphAtlasBytes) {
        return static_cast<int64_t>(renderer->glyphAtlas.pages.size()) *
            kAtlasSize * kAtlasSize;
    }
    if (field == BxGuiMetalStatGlyphAtlasEvictions) {
        return static_cast<int64_t>(renderer->glyphAtlas.evictions);
    }
    if (field == BxGuiMetalStatImageAtlasBytes) {
        return static_cast<int64_t>(renderer->imageAtlas.pages.size()) *
            renderer->imageAtlas.bytesPerPixel * kAtlasSize * kAtlasSize;
    }
    if (field == BxGuiMetalStatImageAtlasEvictions) {
        return static_cast<int64_t>(renderer->imageAtlas.evictions);
    }
    if (field == BxGuiMetalStatImageTextureBytes) {
        return static_cast<int64_t>(renderer->imageTextureBytes);
    }
    if (field == BxGuiMetalStatImageTextureEvictions) {
        return static_cast<int64_t>(renderer->imageTextureEvictions);
    }
    if (field == BxGuiMetalStatImageTextureUploadBytes) {
        return static_cast<int64_t>(renderer->imageTextureUploadBytes);
    }
    if (field == BxGuiMetalStatImageTileSourceBytes) {
        return static_cast<int64_t>(renderer->imageTileSourceBytes);
    }
    if (field == BxGuiMetalStatImageTileCount) {
        int64_t count = 0;
        for (const auto& entry : renderer->imageTextures) {
            count += static_cast<int64_t>(entry.second.tiles.size());
        }
        return count;
    }
    if (field == BxGuiMetalStatRunCount) {
        int64_t count = 0;
        for (const RunRecord& run : renderer->runs) {
            if (run.alive) ++count;
        }
        return count;
    }
    if (field == BxGuiMetalStatTileBytes) {
        return static_cast<int64_t>(renderer->tileBytes);
    }
    if (field == BxGuiMetalStatPyramidTileBytes) {
        return static_cast<int64_t>(renderer->pyramidBytes);
    }
    if (field == BxGuiMetalStatPyramidCapBytes) {
        return static_cast<int64_t>(pyramidBudgetBytes());
    }
    if (field == BxGuiMetalStatFillOverflows) {
        return static_cast<int64_t>(renderer->fillOverflows.load());
    }
    if (field == BxGuiMetalStatFillRedrawPending) {
        return renderer->fillRedraw.load() || renderer->glyphRedraw.load() ||
            renderer->shaderCompiles->landed.load() ? 1 : 0;
    }
    if (field == BxGuiMetalStatFillFramesPending) {
        return renderer->fillFramesPending.load() + renderer->shaderCompiles->pending.load();
    }
    if (renderer->lastFrame < 0) {
        return field == BxGuiMetalStatGpuNanos ? -1 : 0;
    }
    const FrameStats& stats = renderer->frames[static_cast<size_t>(renderer->lastFrame)];
    switch (field) {
        case BxGuiMetalStatEncodeNanos: return static_cast<int64_t>(stats.encodeNanos);
        case BxGuiMetalStatWalkNanos: return static_cast<int64_t>(stats.walkNanos);
        case BxGuiMetalStatGpuNanos: return stats.gpuNanos;
        case BxGuiMetalStatDrawCalls: return stats.drawCalls;
        case BxGuiMetalStatPipelineSwitches: return stats.pipelineSwitches;
        case BxGuiMetalStatPacketBytes: return static_cast<int64_t>(stats.packetBytes);
        case BxGuiMetalStatSkippedOps: return stats.skippedOps;
        case BxGuiMetalStatInstances: return stats.instances;
        case BxGuiMetalStatGlyphAtlasUploads: return stats.glyphUploads;
        case BxGuiMetalStatGlyphAtlasUploadBytes:
            return static_cast<int64_t>(stats.glyphUploadBytes);
        case BxGuiMetalStatFallbackRuns: return stats.fallbackRuns;
        case BxGuiMetalStatDrawableWaitNanos:
            return static_cast<int64_t>(stats.drawableWaitNanos);
        case BxGuiMetalStatFillDraws: return stats.fillDraws;
        case BxGuiMetalStatFillSegments: return stats.fillSegments;
        case BxGuiMetalStatFillTiles: return stats.fillTiles;
        case BxGuiMetalStatFillPieces: return stats.fillPieces;
        case BxGuiMetalStatFillLines: return stats.fillLines;
        case BxGuiMetalStatFillOpaqueTiles: return stats.fillOpaqueTiles;
        case BxGuiMetalStatLayerPasses: return stats.layerPasses;
        case BxGuiMetalStatCoverageClips: return stats.coverageClips;
        default: return -1;
    }
}

extern "C" int64_t brixGuiMetalRendererFrameStatValue(
    void* pointer,
    int64_t frame,
    int32_t field
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || frame < 0 || !renderer->presentationState) {
        return -1;
    }
    std::shared_ptr<PresentationState> state = renderer->presentationState;
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->records.empty()) {
        return -1;
    }
    const PresentationRecord& record = state->records[
        static_cast<size_t>(frame) % state->records.size()
    ];
    if (record.frame != frame) {
        return -1;
    }
    switch (field) {
        case 0: return record.gpuNanos;
        case 1: return static_cast<int64_t>(record.drawableWaitNanos);
        case 2: return static_cast<int64_t>(record.presentedTimeNanos);
        case 3: return static_cast<int64_t>(record.presenterWaitNanos);
        default: return -1;
    }
}

extern "C" void brixGuiMetalRendererDiffWithCanvas(void* pointer, void* canvas, int64_t tolerance) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return;
    }
    renderer->diffValues[0] = -1;
    renderer->diffValues[1] = -1;
    renderer->diffValues[2] = 0;
    renderer->diffValues[3] = -1;
    std::vector<uint8_t> metalPixels;
    if (!readTarget(renderer, metalPixels)) {
        return;
    }
    CGImageRef image = static_cast<CGImageRef>(brixGraphicsCanvasCreateCGImage(canvas));
    int32_t width = brixGraphicsCanvasGetPixelWidth(canvas);
    int32_t height = brixGraphicsCanvasGetPixelHeight(canvas);
    if (!image || width != renderer->pixelWidth || height != renderer->pixelHeight) {
        if (image) CGImageRelease(image);
        return;
    }
    std::vector<uint8_t> canvasPixels(static_cast<size_t>(width) * height * 4);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        canvasPixels.data(),
        width,
        height,
        8,
        static_cast<size_t>(width) * 4,
        colorSpace,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
    );
    CGColorSpaceRelease(colorSpace);
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
    CGContextRelease(context);
    CGImageRelease(image);

    int64_t differentPixels = 0;
    int64_t maximumDelta = 0;
    int64_t totalDelta = 0;
    size_t pixelCount = static_cast<size_t>(width) * height;
    for (size_t pixel = 0; pixel < pixelCount; ++pixel) {
        int64_t pixelDelta = 0;
        for (size_t channel = 0; channel < 4; ++channel) {
            size_t offset = pixel * 4 + channel;
            int64_t delta = llabs(
                static_cast<int64_t>(metalPixels[offset]) - canvasPixels[offset]
            );
            pixelDelta = std::max(pixelDelta, delta);
        }
        if (pixelDelta > tolerance) ++differentPixels;
        maximumDelta = std::max(maximumDelta, pixelDelta);
        totalDelta += pixelDelta;
    }
    renderer->diffValues[0] = differentPixels;
    renderer->diffValues[1] = maximumDelta;
    renderer->diffValues[2] = static_cast<int64_t>(pixelCount);
    renderer->diffValues[3] = totalDelta;
}

extern "C" int64_t brixGuiMetalRendererDiffValue(void* pointer, int32_t field) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer && field >= 0 && field < 4 ? renderer->diffValues[field] : -1;
}

extern "C" bool brixGuiMetalRendererReadIntoCanvas(void* pointer, void* canvas) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !canvas) {
        return false;
    }
    int32_t width = brixGraphicsCanvasGetPixelWidth(canvas);
    int32_t height = brixGraphicsCanvasGetPixelHeight(canvas);
    if (width != renderer->pixelWidth || height != renderer->pixelHeight) {
        return false;
    }
    if (brixGraphicsCanvasGetBytesPerPixel(canvas) != 4) {
        return false;
    }
    std::vector<uint8_t> pixels;
    if (!readTarget(renderer, pixels)) {
        return false;
    }
    uint8_t* data = static_cast<uint8_t*>(brixGraphicsCanvasGetPixelData(canvas));
    size_t rowBytes = static_cast<size_t>(brixGraphicsCanvasGetBytesPerRow(canvas));
    size_t span = static_cast<size_t>(width) * 4;
    if (!data || rowBytes < span) {
        return false;
    }
    for (int32_t row = 0; row < height; ++row) {
        memcpy(data + row * rowBytes, pixels.data() + row * span, span);
    }
    brixGraphicsCanvasPixelsChanged(canvas);
    return true;
}

extern "C" bool brixGuiMetalRendererWritePNG(void* pointer, const char* path) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !path) {
        return false;
    }
    std::vector<uint8_t> pixels;
    if (!readTarget(renderer, pixels)) {
        return false;
    }
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        pixels.data(),
        renderer->pixelWidth,
        renderer->pixelHeight,
        8,
        static_cast<size_t>(renderer->pixelWidth) * 4,
        colorSpace,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
    );
    CGColorSpaceRelease(colorSpace);
    CGImageRef image = context ? CGBitmapContextCreateImage(context) : nullptr;
    if (context) CGContextRelease(context);
    if (!image) {
        return false;
    }
    CFStringRef pathString = CFStringCreateWithCString(
        kCFAllocatorDefault,
        path,
        kCFStringEncodingUTF8
    );
    CFURLRef url = CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault,
        pathString,
        kCFURLPOSIXPathStyle,
        false
    );
    CFRelease(pathString);
    CGImageDestinationRef destination =
        CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
    CFRelease(url);
    if (!destination) {
        CGImageRelease(image);
        return false;
    }
    CGImageDestinationAddImage(destination, image, nullptr);
    bool result = CGImageDestinationFinalize(destination);
    CFRelease(destination);
    CGImageRelease(image);
    return result;
}

namespace {

static bool fillSegmentIsPoint(const double* points, uint32_t kind) {
    for (int point = 1; point < 4; ++point) {
        if (kind == 0 && point < 3) {
            continue;
        }
        if (points[point * 2] != points[0] || points[point * 2 + 1] != points[1]) {
            return false;
        }
    }
    return true;
}

static void fillBeginSubpath(MetalRenderer* renderer) {
    renderer->fillSubpath[0] = renderer->fillCurrent[0];
    renderer->fillSubpath[1] = renderer->fillCurrent[1];
    renderer->fillSubpathFirst = renderer->fillPending.size();
    renderer->fillHasSubpath = true;
}

static void fillPushSegment(MetalRenderer* renderer, uint32_t kind, const double* points) {
    if (!renderer->fillHasSubpath) {
        fillBeginSubpath(renderer);
    }
    if (fillSegmentIsPoint(points, kind & FillSegmentKindMask)) {
        return;
    }
    FillPendingSegment segment;
    memcpy(segment.p, points, sizeof(segment.p));
    segment.kind = kind;
    segment.link = 0;
    renderer->fillPending.push_back(segment);
}

static void fillFinishSubpath(MetalRenderer* renderer, bool closed) {
    if (!renderer->fillHasSubpath) {
        return;
    }
    double* current = renderer->fillCurrent;
    double* subpath = renderer->fillSubpath;
    std::vector<FillPendingSegment>& pending = renderer->fillPending;
    size_t first = renderer->fillSubpathFirst;
    if (current[0] != subpath[0] || current[1] != subpath[1]) {
        double reach = fillClosingSnap * std::max({1.0, std::fabs(subpath[0]), std::fabs(subpath[1])});
        if (pending.size() > first &&
            std::fabs(current[0] - subpath[0]) <= reach &&
            std::fabs(current[1] - subpath[1]) <= reach) {
            pending.back().p[6] = subpath[0];
            pending.back().p[7] = subpath[1];
        } else {
            double points[8] = {current[0], current[1], 0, 0, 0, 0, subpath[0], subpath[1]};
            fillPushSegment(renderer, closed ? 0u : static_cast<uint32_t>(FillSegmentImplicit), points);
        }
    }
    size_t last = pending.size();
    while (last > first && (pending[last - 1].kind & FillSegmentImplicit)) {
        --last;
    }
    if (last > first) {
        pending[first].kind |= FillSegmentStart;
        pending[last - 1].kind |= FillSegmentEnd;
        if (closed) {
            pending[first].kind |= FillSegmentClosed;
            pending[last - 1].kind |= FillSegmentClosed;
            pending[last - 1].link = static_cast<uint32_t>(last - 1 - first);
        }
    }
    current[0] = subpath[0];
    current[1] = subpath[1];
    renderer->fillHasSubpath = false;
}

static void fillAbandonBuild(MetalRenderer* renderer) {
    releaseBrushDabs(renderer->fillBuild.spans);
    for (const FillPathRecord& path : renderer->fillBuild.paths) {
        renderer->fillDeadSegments += path.segmentCount;
    }
    renderer->fillBuild = FillGeometryRecord();
    renderer->fillPending.clear();
    renderer->fillHasSubpath = false;
    renderer->fillBuilding = false;
}

static bool fillDirection(double x, double y, double* out) {
    double length = std::sqrt(x * x + y * y);
    if (!(length > 1e-12)) {
        return false;
    }
    out[0] = x / length;
    out[1] = y / length;
    return true;
}

static void fillStartTangent(const FillPendingSegment& segment, double* out) {
    const double* p = segment.p;
    if ((segment.kind & FillSegmentKindMask) != 0) {
        for (int point = 1; point < 4; ++point) {
            if (fillDirection(p[point * 2] - p[0], p[point * 2 + 1] - p[1], out)) {
                return;
            }
        }
    }
    if (!fillDirection(p[6] - p[0], p[7] - p[1], out)) {
        out[0] = 1;
        out[1] = 0;
    }
}

static void fillEndTangent(const FillPendingSegment& segment, double* out) {
    const double* p = segment.p;
    if ((segment.kind & FillSegmentKindMask) != 0) {
        for (int point = 2; point >= 0; --point) {
            if (fillDirection(p[6] - p[point * 2], p[7] - p[point * 2 + 1], out)) {
                return;
            }
        }
    }
    if (!fillDirection(p[6] - p[0], p[7] - p[1], out)) {
        out[0] = 1;
        out[1] = 0;
    }
}

static inline double fillDistance(double dx, double dy) {
    return std::sqrt(dx * dx + dy * dy);
}

static double fillCubicLength(const double* p, int depth) {
    double chord = fillDistance(p[6] - p[0], p[7] - p[1]);
    double polygon = fillDistance(p[2] - p[0], p[3] - p[1]) +
        fillDistance(p[4] - p[2], p[5] - p[3]) +
        fillDistance(p[6] - p[4], p[7] - p[5]);
    if (depth >= 10 || polygon - chord <= 1e-2 * polygon) {
        return (chord + polygon) * 0.5;
    }
    double left[8];
    double right[8];
    for (int axis = 0; axis < 2; ++axis) {
        double a = (p[axis] + p[2 + axis]) * 0.5;
        double b = (p[2 + axis] + p[4 + axis]) * 0.5;
        double c = (p[4 + axis] + p[6 + axis]) * 0.5;
        double d = (a + b) * 0.5;
        double e = (b + c) * 0.5;
        double middle = (d + e) * 0.5;
        left[axis] = p[axis];
        left[2 + axis] = a;
        left[4 + axis] = d;
        left[6 + axis] = middle;
        right[axis] = middle;
        right[2 + axis] = e;
        right[4 + axis] = c;
        right[6 + axis] = p[6 + axis];
    }
    return fillCubicLength(left, depth + 1) + fillCubicLength(right, depth + 1);
}

static double fillSegmentLength(const FillPendingSegment& segment) {
    if ((segment.kind & FillSegmentKindMask) == 0) {
        return fillDistance(segment.p[6] - segment.p[0], segment.p[7] - segment.p[1]);
    }
    return fillCubicLength(segment.p, 0);
}

static double fillMiterScale(const std::vector<FillPendingSegment>& pending) {
    double scale = 1;
    for (size_t index = 0; index < pending.size(); ++index) {
        uint32_t kind = pending[index].kind;
        if (kind & FillSegmentImplicit) {
            continue;
        }
        size_t next = index + 1;
        if (kind & FillSegmentEnd) {
            if (!(kind & FillSegmentClosed)) {
                continue;
            }
            next = index - pending[index].link;
        }
        if (next >= pending.size()) {
            continue;
        }
        double incoming[2];
        double outgoing[2];
        fillEndTangent(pending[index], incoming);
        fillStartTangent(pending[next], outgoing);
        double half = (1 + incoming[0] * outgoing[0] + incoming[1] * outgoing[1]) * 0.5;
        if (half * fillMiterLimit * fillMiterLimit >= 1) {
            scale = std::max(scale, 1 / std::sqrt(std::max(half, 1e-12)));
        }
    }
    return scale;
}

} // namespace

static void fillAppendVerbs(
    MetalRenderer* renderer,
    const uint8_t* verbs,
    int64_t count,
    const double* coords
) {
    double* current = renderer->fillCurrent;
    size_t coord = 0;
    for (int64_t index = 0; index < count; ++index) {
        switch (verbs[index]) {
        case 0:
            fillFinishSubpath(renderer, false);
            current[0] = coords[coord];
            current[1] = coords[coord + 1];
            fillBeginSubpath(renderer);
            coord += 2;
            break;
        case 1: {
            double points[8] = {
                current[0], current[1], 0, 0, 0, 0, coords[coord], coords[coord + 1],
            };
            fillPushSegment(renderer, 0, points);
            current[0] = coords[coord];
            current[1] = coords[coord + 1];
            coord += 2;
            break;
        }
        case 2: {
            double points[8] = {
                current[0], current[1],
                coords[coord], coords[coord + 1],
                coords[coord + 2], coords[coord + 3],
                coords[coord + 4], coords[coord + 5],
            };
            fillPushSegment(renderer, 1, points);
            current[0] = coords[coord + 4];
            current[1] = coords[coord + 5];
            coord += 6;
            break;
        }
        case 3:
            fillFinishSubpath(renderer, true);
            break;
        default:
            break;
        }
    }
}

static uint32_t fillEndPath(MetalRenderer* renderer) {
    fillFinishSubpath(renderer, false);
    FillPathRecord path;
    path.segmentStart = static_cast<uint32_t>(renderer->fillSegments.size());
    path.segmentCount = static_cast<uint32_t>(renderer->fillPending.size());
    path.miterScale = fillMiterScale(renderer->fillPending);
    double minX = INFINITY, minY = INFINITY, maxX = -INFINITY, maxY = -INFINITY;
    double area = 0;
    double subpathArea = 0;
    bool mixed = false;
    for (const FillPendingSegment& segment : renderer->fillPending) {
        if (segment.kind & FillSegmentStart) {
            mixed = mixed || subpathArea * area < 0;
            area += subpathArea;
            subpathArea = 0;
        }
        const double* p = segment.p;
        auto cross = [p](int first, int second) {
            return p[first * 2] * p[second * 2 + 1] - p[second * 2] * p[first * 2 + 1];
        };
        if ((segment.kind & FillSegmentKindMask) == 0) {
            subpathArea += cross(0, 3) * 0.5;
        } else {
            subpathArea += (12 * cross(0, 1) + 6 * cross(0, 2) + 2 * cross(0, 3) +
                6 * cross(1, 2) + 6 * cross(1, 3) + 12 * cross(2, 3)) / 40;
        }
        for (int point = 0; point < 4; ++point) {
            if ((segment.kind & FillSegmentKindMask) == 0 && (point == 1 || point == 2)) {
                continue;
            }
            minX = std::min(minX, segment.p[point * 2]);
            minY = std::min(minY, segment.p[point * 2 + 1]);
            maxX = std::max(maxX, segment.p[point * 2]);
            maxY = std::max(maxY, segment.p[point * 2 + 1]);
        }
    }
    if (renderer->fillPending.empty()) {
        minX = minY = maxX = maxY = 0;
    }
    mixed = mixed || subpathArea * area < 0;
    area += subpathArea;
    path.originX = minX;
    path.originY = minY;
    path.width = maxX - minX;
    path.height = maxY - minY;
    path.area = mixed ? 0 : std::fabs(area);
    ensureVectorCapacity(
        renderer,
        renderer->fillSegments,
        renderer->fillSegments.size() + renderer->fillPending.size()
    );
    double arc = 0;
    for (const FillPendingSegment& pending : renderer->fillPending) {
        FillSegmentGPU segment = {};
        for (int point = 0; point < 4; ++point) {
            segment.p[point * 2] = static_cast<float>(pending.p[point * 2] - minX);
            segment.p[point * 2 + 1] = static_cast<float>(pending.p[point * 2 + 1] - minY);
        }
        segment.kind = pending.kind;
        segment.link = pending.link;
        if (pending.kind & FillSegmentStart) {
            arc = 0;
        }
        double length = (pending.kind & FillSegmentImplicit) ? 0 : fillSegmentLength(pending);
        segment.arcStart = static_cast<float>(arc);
        arc += length;
        segment.arcEnd = static_cast<float>(arc);
        path.length += length;
        renderer->fillSegments.push_back(segment);
    }
    renderer->fillPending.clear();
    renderer->fillHasSubpath = false;
    renderer->fillBuild.paths.push_back(path);
    return static_cast<uint32_t>(renderer->fillBuild.paths.size() - 1);
}

extern "C" bool brixGuiMetalPathLookup(void* pointer, uint32_t index, uint32_t generation) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    return renderer && fillGeometryFor(renderer, index, generation) != nullptr;
}

extern "C" void brixGuiMetalPathBegin(void* pointer, uint32_t index, uint32_t generation) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return;
    }
    if (renderer->fillBuilding) {
        fillAbandonBuild(renderer);
    }
    renderer->fillBuild.generation = generation;
    renderer->fillBuildIndex = index;
    renderer->fillBuilding = true;
}

extern "C" void brixGuiMetalPathAppend(
    void* pointer,
    const uint8_t* verbs,
    int64_t count,
    const double* coords
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->fillBuilding || !verbs || count <= 0) {
        return;
    }
    fillAppendVerbs(renderer, verbs, count, coords);
}

extern "C" uint32_t brixGuiMetalPathEndPath(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->fillBuilding) {
        return UINT32_MAX;
    }
    return fillEndPath(renderer);
}

extern "C" void brixGuiMetalPathAddSpan(
    void* pointer,
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
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->fillBuilding || path >= renderer->fillBuild.paths.size()) {
        return;
    }
    FillSpanRecord span;
    span.path = path;
    span.role = fillRole;
    span.paletteIndex = fillIndex;
    span.fillRule = fillRule == 0 ? 0 : 1;
    span.strokeRole = strokeRole;
    span.strokePaletteIndex = strokeIndex;
    span.strokeWidth = strokeWidth;
    span.strokeCap = strokeCap;
    span.strokeJoin = strokeJoin;
    span.strokeAlignment = strokeAlignment <= FillAlignOutside ? strokeAlignment : FillAlignCenter;
    renderer->fillBuild.spans.push_back(span);
}

extern "C" bool brixGuiMetalPathEnd(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer || !renderer->fillBuilding) {
        return false;
    }
    uint32_t index = renderer->fillBuildIndex;
    if (index >= renderer->fillGeometries.size()) {
        renderer->fillGeometries.resize(static_cast<size_t>(index) + 1);
    }
    FillGeometryRecord& record = renderer->fillGeometries[index];
    if (record.resident) {
        for (const FillPathRecord& path : record.paths) {
            renderer->fillDeadSegments += path.segmentCount;
        }
    }
    renderer->fillBuild.resident = true;
    record = std::move(renderer->fillBuild);
    renderer->fillBuild = FillGeometryRecord();
    renderer->fillBuilding = false;
    return true;
}

static uint64_t chunkVerbCoords(const uint8_t* verbs, uint64_t count) {
    uint64_t coords = 0;
    for (uint64_t index = 0; index < count; ++index) {
        uint8_t verb = verbs[index];
        coords += verb == 0 || verb == 1 ? 2 : verb == 2 ? 6 : 0;
    }
    return coords;
}

static void chunkShapeVerbs(MetalRenderer* renderer, const BxGuiMetalDisplayItem& item, bool oval) {
    std::vector<uint8_t>& verbs = renderer->chunkPathVerbs;
    std::vector<double>& coords = renderer->chunkPathCoords;
    verbs.clear();
    coords.clear();
    ensureVectorCapacity(renderer, verbs, 6);
    ensureVectorCapacity(renderer, coords, 26);
    double left = std::min<double>(item.x, item.x + item.width);
    double top = std::min<double>(item.y, item.y + item.height);
    double right = std::max<double>(item.x, item.x + item.width);
    double bottom = std::max<double>(item.y, item.y + item.height);
    if (!oval) {
        verbs.insert(verbs.end(), {0, 1, 1, 1, 3});
        coords.insert(coords.end(), {left, top, right, top, right, bottom, left, bottom});
        return;
    }
    const double kappa = 0.5522847498307936;
    double centerX = (left + right) * 0.5;
    double centerY = (top + bottom) * 0.5;
    double radiusX = (right - left) * 0.5;
    double radiusY = (bottom - top) * 0.5;
    double handleX = radiusX * kappa;
    double handleY = radiusY * kappa;
    verbs.insert(verbs.end(), {0, 2, 2, 2, 2, 3});
    coords.insert(coords.end(), {
        right, centerY,
        right, centerY + handleY, centerX + handleX, bottom, centerX, bottom,
        centerX - handleX, bottom, left, centerY + handleY, left, centerY,
        left, centerY - handleY, centerX - handleX, top, centerX, top,
        centerX + handleX, top, right, centerY - handleY, right, centerY,
    });
}

static void chunkOutlineVerbs(MetalRenderer* renderer, const BxGuiMetalState& state) {
    std::vector<uint8_t>& verbs = renderer->chunkPathVerbs;
    std::vector<double>& coords = renderer->chunkPathCoords;
    verbs.clear();
    coords.clear();
    const double x = state.rect[0];
    const double y = state.rect[1];
    const double width = state.rect[2];
    const double height = state.rect[3];
    if (!(width > 0) || !(height > 0)) {
        return;
    }
    ensureVectorCapacity(renderer, verbs, 22);
    ensureVectorCapacity(renderer, coords, 64);
    const double right = x + width;
    const double bottom = y + height;
    const double r = std::max(0.0, std::min(state.radius, std::min(width, height) / 2));
    const int32_t side = static_cast<int32_t>(state.side);
    const double size = state.size;
    const double span = side <= 1 ? width : height;
    double half = size;
    double tip = state.offset;
    if (span - r - half < r + half) {
        half = std::max(0.0, span / 2 - r);
        tip = span / 2;
    } else {
        tip = std::min(std::max(tip, r + half), span - r - half);
    }
    const bool arrow = side >= 0 && side <= 3 && size > 0 && half > 0;
    const double kappa = 0.5522847498307936;
    double current[2] = {x + r, y};
    auto line = [&](double px, double py) {
        verbs.push_back(1);
        coords.insert(coords.end(), {px, py});
        current[0] = px;
        current[1] = py;
    };
    auto corner = [&](double cx, double cy, double ex, double ey) {
        if (r <= 0) {
            return;
        }
        verbs.push_back(2);
        coords.insert(coords.end(), {
            current[0] + (cx - current[0]) * kappa,
            current[1] + (cy - current[1]) * kappa,
            ex + (cx - ex) * kappa,
            ey + (cy - ey) * kappa,
            ex,
            ey,
        });
        current[0] = ex;
        current[1] = ey;
    };
    verbs.push_back(0);
    coords.insert(coords.end(), {x + r, y});
    if (arrow && side == 0) {
        line(x + tip - half, y);
        line(x + tip, y - size);
        line(x + tip + half, y);
    }
    line(right - r, y);
    corner(right, y, right, y + r);
    if (arrow && side == 3) {
        line(right, y + tip - half);
        line(right + size, y + tip);
        line(right, y + tip + half);
    }
    line(right, bottom - r);
    corner(right, bottom, right - r, bottom);
    if (arrow && side == 1) {
        line(x + tip + half, bottom);
        line(x + tip, bottom + size);
        line(x + tip - half, bottom);
    }
    line(x + r, bottom);
    corner(x, bottom, x, bottom - r);
    if (arrow && side == 2) {
        line(x, y + tip + half);
        line(x - size, y + tip);
        line(x, y + tip - half);
    }
    line(x, y + r);
    corner(x, y, x + r, y);
    verbs.push_back(3);
}

static uint64_t dabKeyMix(uint64_t hash, uint64_t value) {
    hash ^= value * 0x9e3779b97f4a7c15ull;
    hash = (hash << 27) | (hash >> 37);
    return hash * 0xc2b2ae3d27d4eb4full + 0x165667b19e3779f9ull;
}

static uint64_t dabKeyDouble(uint64_t hash, double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return dabKeyMix(hash, bits);
}

static uint64_t brushDabKey(
    const uint8_t* verbs,
    int64_t verbCount,
    const double* coords,
    bool close,
    double width,
    double spacing
) {
    uint64_t hash = dabKeyMix(0x27d4eb2f165667c5ull, static_cast<uint64_t>(verbCount) * 2 + (close ? 1 : 0));
    hash = dabKeyDouble(hash, width);
    hash = dabKeyDouble(hash, spacing);
    uint64_t coordCount = chunkVerbCoords(verbs, static_cast<uint64_t>(verbCount));
    int64_t index = 0;
    for (; index + 8 <= verbCount; index += 8) {
        uint64_t packed;
        memcpy(&packed, verbs + index, sizeof(packed));
        hash = dabKeyMix(hash, packed);
    }
    uint64_t tail = 0;
    for (; index < verbCount; ++index) {
        tail = (tail << 8) | verbs[index];
    }
    hash = dabKeyMix(hash, tail);
    for (uint64_t coord = 0; coord < coordCount; ++coord) {
        hash = dabKeyDouble(hash, coords[coord]);
    }
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdull;
    hash ^= hash >> 33;
    return hash ? hash : 1;
}

static void acquireBrushDabs(
    MetalRenderer* renderer,
    FillSpanRecord& span,
    const uint8_t* verbs,
    int64_t verbCount,
    const double* coords,
    bool close,
    double width,
    double spacing
) {
    uint64_t key = brushDabKey(verbs, verbCount, coords, close, width, spacing);
    auto existing = renderer->dabCache.find(key);
    bool created = existing == renderer->dabCache.end();
    DabCacheEntry& entry = created ? renderer->dabCache[key] : existing->second;
    if (created) {
        countAllocation(renderer);
        entry.dabs.resize(256);
        int64_t needed = brixGraphicsBrushDabs(
            verbs, verbCount, coords, close, width, spacing, entry.dabs.data(),
            static_cast<int64_t>(entry.dabs.size()), &entry.total
        );
        if (needed > static_cast<int64_t>(entry.dabs.size())) {
            entry.dabs.resize(static_cast<size_t>(needed));
            needed = brixGraphicsBrushDabs(
                verbs, verbCount, coords, close, width, spacing, entry.dabs.data(), needed, &entry.total
            );
        }
        entry.dabs.resize(static_cast<size_t>(std::max<int64_t>(needed, 0)));
        entry.dabs.shrink_to_fit();
    }
    ++entry.refs;
    entry.lastUsed = renderer->frameCount;
    span.dabKey = key;
    span.dabs = &entry;
}

static void releaseBrushDabs(std::vector<FillSpanRecord>& spans) {
    for (FillSpanRecord& span : spans) {
        if (span.dabs && span.dabs->refs) {
            --span.dabs->refs;
        }
        span.dabs = nullptr;
    }
}

static void sweepBrushDabs(MetalRenderer* renderer) {
    for (auto entry = renderer->dabCache.begin(); entry != renderer->dabCache.end();) {
        if (!entry->second.refs && entry->second.lastUsed + 2 < renderer->frameCount) {
            entry = renderer->dabCache.erase(entry);
        } else {
            ++entry;
        }
    }
}

static void releaseChunkPathSet(MetalRenderer* renderer, uint32_t slot) {
    ChunkPathSet& set = renderer->chunkPaths[slot];
    releaseBrushDabs(set.geometry.spans);
    for (const FillPathRecord& path : set.geometry.paths) {
        renderer->fillDeadSegments += path.segmentCount;
    }
    renderer->chunkPathByKey.erase(set.key);
    set = ChunkPathSet();
    renderer->freeChunkPaths.push_back(slot);
}

static bool isShaderPaint(const BxGuiMetalGradient* gradients, uint64_t gradientCount, uint64_t index) {
    return index && index <= gradientCount && (gradients[index - 1].kind == 4 || gradients[index - 1].kind == 5);
}

static void requestChunkShaders(
    MetalRenderer* renderer,
    const BxGuiMetalDisplayItem* items,
    uint64_t count,
    const BxGuiMetalPathStyle* styles,
    uint64_t styleCount,
    const BxGuiMetalGradient* gradients,
    uint64_t gradientCount
) {
    for (uint64_t index = 0; index < styleCount; ++index) {
        const BxGuiMetalPathStyle& style = styles[index];
        if (isShaderPaint(gradients, gradientCount, style.gradient)) {
            requestShader(renderer, ShaderRoleFill, gradients[style.gradient - 1].shader);
        }
        if (style.brush && style.brush <= gradientCount && gradients[style.brush - 1].kind == 6) {
            requestShader(renderer, ShaderRoleDab, gradients[style.brush - 1].shader);
        }
    }
    for (uint64_t index = 0; index < count; ++index) {
        const BxGuiMetalDisplayItem& item = items[index];
        if (item.kind == 13 && item.data1 && item.data1 <= gradientCount && gradients[item.data1 - 1].kind == 4) {
            requestShader(renderer, ShaderRoleFilter, gradients[item.data1 - 1].shader, kBackdropFormat);
            continue;
        }
        if (item.kind != 2 || !(item.value >= 1) || item.value > static_cast<float>(gradientCount)) {
            continue;
        }
        uint64_t paint = static_cast<uint64_t>(item.value);
        if (isShaderPaint(gradients, gradientCount, paint)) {
            requestShader(renderer, ShaderRoleText, gradients[paint - 1].shader);
        }
    }
}

extern "C" int64_t brixGuiMetalChunkPaths(
    void* pointer,
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
) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    const BxGuiMetalDisplayItem* items =
        static_cast<const BxGuiMetalDisplayItem*>(scalarItemBytes);
    const BxGuiMetalPathStyle* styles = static_cast<const BxGuiMetalPathStyle*>(styleBytes);
    const BxGuiMetalGradient* gradients = static_cast<const BxGuiMetalGradient*>(gradientBytes);
    const BxGuiMetalGradientStop* stops = static_cast<const BxGuiMetalGradientStop*>(stopBytes);
    const BxGuiMetalState* states = static_cast<const BxGuiMetalState*>(stateBytes);
    if (!renderer || (!items && count) || (!styles && styleCount) ||
            (!gradients && gradientCount) || (!stops && stopCount) ||
            (!shaderArgs && shaderArgCount) || (!states && stateCount)) {
        return -1;
    }
    uint32_t slot;
    auto found = renderer->chunkPathByKey.find(key);
    if (found != renderer->chunkPathByKey.end()) {
        slot = found->second;
        const ChunkPathSet& set = renderer->chunkPaths[slot];
        if (set.generation == generation) {
            return static_cast<int64_t>((static_cast<uint64_t>(set.serial) << 32) | slot);
        }
    } else if (!renderer->freeChunkPaths.empty()) {
        slot = renderer->freeChunkPaths.back();
        renderer->freeChunkPaths.pop_back();
        renderer->chunkPathByKey[key] = slot;
    } else {
        ensureVectorCapacity(renderer, renderer->chunkPaths, renderer->chunkPaths.size() + 1);
        slot = static_cast<uint32_t>(renderer->chunkPaths.size());
        renderer->chunkPaths.emplace_back();
        renderer->chunkPathByKey[key] = slot;
    }
    ChunkPathSet& set = renderer->chunkPaths[slot];
    for (const FillPathRecord& path : set.geometry.paths) {
        renderer->fillDeadSegments += path.segmentCount;
    }
    releaseBrushDabs(set.geometry.spans);
    if (renderer->fillBuilding) {
        fillAbandonBuild(renderer);
    }
    std::swap(renderer->fillBuild, set.geometry);
    renderer->fillBuild.paths.clear();
    renderer->fillBuild.spans.clear();
    renderer->fillBuild.resident = false;
    renderer->fillBuilding = true;
    set.key = key;
    set.palette.clear();
    set.gradients.assign(gradients, gradients + gradientCount);
    requestChunkShaders(renderer, items, count, styles, styleCount, gradients, gradientCount);
    set.stops.resize(static_cast<size_t>(stopCount));
    for (uint64_t index = 0; index < stopCount; ++index) {
        FillStopGPU& stop = set.stops[index];
        stop = {};
        for (int channel = 0; channel < 4; ++channel) {
            stop.color[channel] = static_cast<float>(stops[index].color[channel]);
        }
        stop.offset = static_cast<float>(stops[index].offset);
    }
    set.shaderArgs.resize(static_cast<size_t>(shaderArgCount));
    for (uint64_t index = 0; index < shaderArgCount; ++index) {
        set.shaderArgs[index] = static_cast<float>(shaderArgs[index]);
    }
    set.statePaths.clear();
    set.spanItems.clear();
    uint64_t styled = 0;
    for (uint64_t index = 0; index < count; ++index) {
        const BxGuiMetalDisplayItem& item = items[index];
        if (item.kind != 8) {
            continue;
        }
        bool valid = styled < styleCount && styles[styled].gradient <= gradientCount &&
            styles[styled].brush <= gradientCount;
        uint32_t shape = static_cast<uint32_t>(item.value);
        const uint8_t* itemVerbs = nullptr;
        int64_t itemVerbCount = 0;
        const double* itemCoords = nullptr;
        bool itemClose = false;
        if (valid && shape == 2) {
            uint64_t start = item.data0;
            uint64_t length = item.data1;
            uint64_t coordStart = item.data2;
            valid = verbs && coords && start <= verbCount && length <= verbCount - start &&
                coordStart <= coordCount &&
                chunkVerbCoords(verbs + start, length) <= coordCount - coordStart;
            if (valid) {
                itemVerbs = verbs + start;
                itemVerbCount = static_cast<int64_t>(length);
                itemCoords = coords + coordStart;
                itemClose = (styles[styled].paint & 4u) != 0;
                fillAppendVerbs(renderer, itemVerbs, itemVerbCount, itemCoords);
                if (itemClose) {
                    const uint8_t close = 3;
                    fillAppendVerbs(renderer, &close, 1, nullptr);
                }
            }
        } else if (valid) {
            chunkShapeVerbs(renderer, item, shape == 1);
            itemVerbs = renderer->chunkPathVerbs.data();
            itemVerbCount = static_cast<int64_t>(renderer->chunkPathVerbs.size());
            itemCoords = renderer->chunkPathCoords.data();
            fillAppendVerbs(renderer, itemVerbs, itemVerbCount, itemCoords);
        }
        if (!valid) {
            fillAbandonBuild(renderer);
            releaseChunkPathSet(renderer, slot);
            return -1;
        }
        const BxGuiMetalPathStyle& style = styles[styled];
        FillSpanRecord span;
        span.path = fillEndPath(renderer);
        span.role = (style.paint & 1u) ? 3 : 2;
        span.paletteIndex = static_cast<uint32_t>(styled * 2);
        span.fillRule = (style.paint & 8u) ? 1 : 0;
        span.strokeRole = (style.paint & 2u) ? 3 : 2;
        span.strokePaletteIndex = static_cast<uint32_t>(styled * 2 + 1);
        span.paint = style.gradient;
        span.strokeWidth = style.strokeWidth;
        span.strokeCap = static_cast<uint8_t>(style.strokeStyle & 15u);
        span.strokeJoin = static_cast<uint8_t>((style.strokeStyle >> 4) & 15u);
        span.strokeAlignment = FillAlignCenter;
        span.dash = style.dash;
        span.shadow = (style.paint & 16u) != 0;
        span.shadowOffset[0] = style.shadow[0];
        span.shadowOffset[1] = style.shadow[1];
        span.shadowBlur = style.shadow[2];
        if (style.brush && gradients[style.brush - 1].kind == 6) {
            const BxGuiMetalGradient& brush = gradients[style.brush - 1];
            if (brush.stopCount >= kBrushParams && brush.stopStart <= shaderArgCount &&
                    brush.stopCount <= shaderArgCount - brush.stopStart) {
                span.brush = style.brush;
                acquireBrushDabs(
                    renderer,
                    span,
                    itemVerbs,
                    itemVerbCount,
                    itemCoords,
                    itemClose,
                    style.strokeWidth,
                    shaderArgs[brush.stopStart]
                );
            }
        }
        renderer->fillBuild.spans.push_back(span);
        set.spanItems.push_back(static_cast<uint32_t>(index));
        for (int channel = 0; channel < 4; ++channel) {
            set.palette.push_back(static_cast<float>(style.fill[channel]));
        }
        for (int channel = 0; channel < 4; ++channel) {
            set.palette.push_back(static_cast<float>(style.stroke[channel]));
        }
        ++styled;
    }
    set.statePaths.assign(static_cast<size_t>(stateCount), UINT32_MAX);
    for (uint64_t index = 1; index < stateCount; ++index) {
        const BxGuiMetalState& state = states[index];
        if (state.kind == StateClipOutline) {
            chunkOutlineVerbs(renderer, state);
            fillAppendVerbs(
                renderer,
                renderer->chunkPathVerbs.data(),
                static_cast<int64_t>(renderer->chunkPathVerbs.size()),
                renderer->chunkPathCoords.data()
            );
            set.statePaths[index] = fillEndPath(renderer);
        } else if (state.kind == StateClipPath) {
            uint64_t start = state.pathStart;
            uint64_t length = state.pathCount;
            uint64_t coordStart = state.coordStart;
            if (verbs && coords && start <= verbCount && length <= verbCount - start &&
                    coordStart <= coordCount &&
                    chunkVerbCoords(verbs + start, length) <= coordCount - coordStart) {
                fillAppendVerbs(renderer, verbs + start, static_cast<int64_t>(length), coords + coordStart);
                set.statePaths[index] = fillEndPath(renderer);
            }
        }
    }
    renderer->fillBuild.resident = true;
    renderer->fillBuild.generation = generation;
    std::swap(renderer->fillBuild, set.geometry);
    renderer->fillBuild.paths.clear();
    renderer->fillBuild.spans.clear();
    renderer->fillBuild.resident = false;
    renderer->fillBuilding = false;
    uint32_t serial = ++renderer->chunkPathSerial;
    if (!serial) {
        serial = ++renderer->chunkPathSerial;
    }
    set.generation = generation;
    set.serial = serial;
    return static_cast<int64_t>((static_cast<uint64_t>(serial) << 32) | slot);
}

extern "C" void brixGuiMetalRendererSetFillTime(void* pointer, double seconds) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (renderer) {
        renderer->fillTime = seconds;
    }
}

extern "C" bool brixGuiMetalRendererTakeFillRedraw(void* pointer) {
    MetalRenderer* renderer = static_cast<MetalRenderer*>(pointer);
    if (!renderer) {
        return false;
    }
    bool landed = renderer->shaderCompiles->landed.exchange(false);
    bool lost = restoreTarget(renderer);
    bool glyphs = renderer->glyphRedraw.exchange(false);
    return renderer->fillRedraw.exchange(false) || glyphs || landed || lost;
}
