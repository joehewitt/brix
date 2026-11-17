#include "BxGraphics/BxGraphics.h"
#include "BxGraphicsColor.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <dispatch/dispatch.h>
#include <IOSurface/IOSurfaceRef.h>
#include <ImageIO/ImageIO.h>
#include <mach/mach.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <zlib.h>

// **********************************************************************************************

struct BxGraphicsColorCacheEntry {
    double red;
    double green;
    double blue;
    double alpha;
    CGColorRef color;
};

static const size_t bxGraphicsColorCacheSize = 64;
static const int32_t bxGraphicsDirtyLogSize = 16;
static const int32_t bxGraphicsDrawStateDepth = 64;
static const double bxGraphicsMiterLimit = 10;

struct BxGraphicsDirtyEntry {
    uint64_t serial;
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};

struct BxGraphicsDrawState {
    double strokeWidth;
    int32_t strokeJoin;
    int32_t blendMode;
    uint64_t compositeSerial;
};

static const size_t bxGraphicsPixelStoreThreshold = 1024 * 1024;

struct BxGraphicsPixelStore {
    void* data;
    size_t length;
    std::atomic<int32_t> references;
};

static const size_t bxGraphicsPaintQueueLimit = 512;
static const size_t bxGraphicsPaintQueueBytes = 64 * 1024 * 1024;

struct BxGraphicsQueuedPaint {
    int64_t job;
    CGPathRef clip;
    bool evenOdd;
    CGRect area;
};

struct BxGraphicsPaintQueue {
    void* batch;
    std::vector<BxGraphicsQueuedPaint> paints;
};

struct BxGraphicsCanvas {
    CGContextRef context;
    CGColorSpaceRef colorSpace;
    void* pixels;
    size_t bytesPerRow;
    size_t bytesPerPixel;
    int32_t width;
    int32_t height;
    int32_t pixelWidth;
    int32_t pixelHeight;
    double scale;
    CGImageRef snapshot;
    CGImageRef maskSnapshot;
    uint64_t contentSerial;
    double maskOrigin[2];
    int32_t blurRadius;
    BxGraphicsCanvas* parent;
    int32_t groupKind;
    int32_t absoluteLeft;
    int32_t absoluteTop;
    bool preservesSRGBColors;
    double filterRegion[4];
    double filterScale;
    BxGraphicsColorCacheEntry colorCache[bxGraphicsColorCacheSize];
    uint64_t identity;
    bool observed;
    BxGraphicsDirtyEntry dirtyLog[bxGraphicsDirtyLogSize];
    int32_t dirtyCount;
    BxGraphicsDrawState drawState;
    BxGraphicsDrawState drawStates[bxGraphicsDrawStateDepth];
    int32_t drawDepth;
    double widestStroke;
    BxGraphicsPixelStore* store;
    int32_t space;
    int32_t format;
    bool namedSpace;
    void* gpu;
    bool gpuNewer;
    uint64_t gpuSerial;
    int32_t gpuDirty[4];
    IOSurfaceRef surface;
    BxGraphicsPaintQueue* paintQueue;
};

enum : int32_t {
    BxGraphicsGroupNone = 0,
    BxGraphicsGroupMask = 1,
    BxGraphicsGroupBlur = 2,
    BxGraphicsGroupOpacity = 3,
    BxGraphicsGroupFilter = 4,
};

enum : int32_t {
    BxGraphicsSamplingSmooth = 0,
    BxGraphicsSamplingPixelated = 1,
};

struct BxGraphicsTextLine {
    CTLineRef line;
    CFIndex base;
    CFIndex start;
    CFIndex end;
    double width;
    double height;
    double ascent;
    double descent;
    double leading;
    double baseline;
    CGRect typographicBounds;
    mutable CGRect inkBounds;
    mutable bool hasInkBounds;
};

struct BxGraphicsTextFragment {
    int64_t tag;
    int64_t line;
    double x;
    double width;
    double ascent;
    double descent;
};

struct BxGraphicsTextPathCommand {
    uint8_t verb;
    double coordinates[6];
};

struct BxGraphicsTextLayout {
    std::vector<BxGraphicsTextLine> lines;
    std::vector<BxGraphicsTextFragment> fragments;
    std::vector<BxGraphicsTextPathCommand> outline;
    bool hasOutline = false;
    double naturalWidth;
    double constrainedWidth;
    double layoutWidth;
    double height;
    double lineSpacing;
    bool truncated;
    uint64_t serial;
};

struct BxGraphicsFontRegistration {
    CFURLRef url;
    std::string family;
};

// **********************************************************************************************

static const int64_t bxGraphicsShaderImageLimit = 8;

static void syncCanvasFromGPU(BxGraphicsCanvas* canvas);
static void flushCanvasPaints(BxGraphicsCanvas* canvas);

static BxGraphicsCanvas*
peekCanvas(void* ptr) {
    return static_cast<BxGraphicsCanvas*>(ptr);
}

static BxGraphicsCanvas*
getStateCanvas(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    if (canvas && canvas->gpuNewer) {
        syncCanvasFromGPU(canvas);
    }
    return canvas;
}

static BxGraphicsCanvas*
getCanvas(void* ptr) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->paintQueue) {
        flushCanvasPaints(canvas);
    }
    return canvas;
}

static void
clearPaintQueue(BxGraphicsPaintQueue* queue) {
    for (BxGraphicsQueuedPaint& paint : queue->paints) {
        if (paint.clip) {
            CGPathRelease(paint.clip);
        }
    }
    queue->paints.clear();
    bxGraphicsShaderBatchReset(queue->batch);
}

static BxGraphicsPixelStore*
createPixelStore(size_t bytes) {
    const size_t page = static_cast<size_t>(getpagesize());
    const size_t length = (bytes + page - 1) / page * page;
    void* data = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (data == MAP_FAILED) {
        return nullptr;
    }
    BxGraphicsPixelStore* store = new BxGraphicsPixelStore();
    store->data = data;
    store->length = length;
    store->references.store(1, std::memory_order_relaxed);
    return store;
}

static void
releasePixelStore(BxGraphicsPixelStore* store) {
    if (store && store->references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        munmap(store->data, store->length);
        delete store;
    }
}

static void
releaseCanvas(BxGraphicsCanvas* canvas) {
    if (!canvas) {
        return;
    }

    if (canvas->snapshot) {
        CGImageRelease(canvas->snapshot);
        canvas->snapshot = nullptr;
    }
    if (canvas->maskSnapshot) {
        CGImageRelease(canvas->maskSnapshot);
        canvas->maskSnapshot = nullptr;
    }
    for (size_t index = 0; index < bxGraphicsColorCacheSize; ++index) {
        if (canvas->colorCache[index].color) {
            CGColorRelease(canvas->colorCache[index].color);
            canvas->colorCache[index].color = nullptr;
        }
    }

    if (canvas->context) {
        CGContextRelease(canvas->context);
        canvas->context = nullptr;
    }
    if (canvas->colorSpace) {
        CGColorSpaceRelease(canvas->colorSpace);
        canvas->colorSpace = nullptr;
    }
    if (canvas->gpu) {
        bxGraphicsGPURelease(canvas->gpu);
        canvas->gpu = nullptr;
        canvas->gpuNewer = false;
    }
    if (canvas->paintQueue) {
        clearPaintQueue(canvas->paintQueue);
        bxGraphicsShaderBatchRelease(canvas->paintQueue->batch);
        delete canvas->paintQueue;
        canvas->paintQueue = nullptr;
    }
    if (canvas->surface) {
        CFRelease(canvas->surface);
        canvas->surface = nullptr;
    } else if (canvas->store) {
        releasePixelStore(canvas->store);
        canvas->store = nullptr;
    } else if (canvas->pixels) {
        free(canvas->pixels);
    }
    canvas->pixels = nullptr;
}

static std::atomic<uint64_t> gCanvasContentSerial{0};

static void
placeGroup(BxGraphicsCanvas* group, BxGraphicsCanvas* parent, CGRect bounds, int32_t kind) {
    group->parent = parent;
    group->groupKind = kind;
    group->absoluteLeft = parent->absoluteLeft + static_cast<int32_t>(bounds.origin.x);
    group->absoluteTop = parent->absoluteTop + parent->pixelHeight -
        static_cast<int32_t>(bounds.origin.y + bounds.size.height);
}

static uint64_t
nextContentSerial() {
    return gCanvasContentSerial.fetch_add(1, std::memory_order_relaxed) + 1;
}

static bool
isUnboundedBlend(int32_t mode) {
    return mode == kCGBlendModeClear
        || mode == kCGBlendModeCopy
        || mode == kCGBlendModeSourceIn
        || mode == kCGBlendModeSourceOut
        || mode == kCGBlendModeDestinationIn
        || mode == kCGBlendModeDestinationAtop;
}

static void (*gCanvasReleaseObserver)(uint64_t identity) = nullptr;

static void
bumpContentSerial(BxGraphicsCanvas* canvas) {
    canvas->contentSerial = nextContentSerial();
    if (canvas->snapshot) {
        CGImageRelease(canvas->snapshot);
        canvas->snapshot = nullptr;
    }
    if (canvas->maskSnapshot) {
        CGImageRelease(canvas->maskSnapshot);
        canvas->maskSnapshot = nullptr;
    }
}

static void
syncCanvasFromGPU(BxGraphicsCanvas* canvas) {
    canvas->gpuNewer = false;
    if (canvas->pixels) {
        bxGraphicsGPUReadback(canvas->gpu, static_cast<uint8_t*>(canvas->pixels), canvas->bytesPerRow, canvas->gpuDirty);
    }
}

static void
logDirtyPixels(BxGraphicsCanvas* canvas, int32_t left, int32_t top, int32_t right, int32_t bottom) {
    left = std::max(left, 0);
    top = std::max(top, 0);
    right = std::min(right, canvas->pixelWidth);
    bottom = std::min(bottom, canvas->pixelHeight);
    if (left >= right || top >= bottom) {
        return;
    }
    bumpContentSerial(canvas);
    if (left == 0 && top == 0 && right == canvas->pixelWidth && bottom == canvas->pixelHeight) {
        canvas->dirtyLog[0] = {canvas->contentSerial, left, top, right, bottom};
        canvas->dirtyCount = 1;
        return;
    }
    if (canvas->dirtyCount > 0) {
        BxGraphicsDirtyEntry& last = canvas->dirtyLog[canvas->dirtyCount - 1];
        if (last.left == left && last.top == top && last.right == right && last.bottom == bottom) {
            last.serial = canvas->contentSerial;
            return;
        }
    }
    if (canvas->dirtyCount == bxGraphicsDirtyLogSize) {
        const BxGraphicsDirtyEntry& first = canvas->dirtyLog[0];
        BxGraphicsDirtyEntry& second = canvas->dirtyLog[1];
        second.left = std::min(first.left, second.left);
        second.top = std::min(first.top, second.top);
        second.right = std::max(first.right, second.right);
        second.bottom = std::max(first.bottom, second.bottom);
        memmove(
            canvas->dirtyLog,
            canvas->dirtyLog + 1,
            sizeof(BxGraphicsDirtyEntry) * (bxGraphicsDirtyLogSize - 1)
        );
        --canvas->dirtyCount;
    }
    canvas->dirtyLog[canvas->dirtyCount++] = {canvas->contentSerial, left, top, right, bottom};
}

static void
invalidateSnapshot(BxGraphicsCanvas* canvas) {
    if (!canvas) {
        return;
    }
    if (!canvas->observed) {
        bumpContentSerial(canvas);
        return;
    }
    logDirtyPixels(canvas, 0, 0, canvas->pixelWidth, canvas->pixelHeight);
}

static void
invalidateDeviceRect(BxGraphicsCanvas* canvas, CGRect device) {
    if (!canvas->observed || !canvas->context) {
        bumpContentSerial(canvas);
        return;
    }
    const CGRect clip = CGRectApplyAffineTransform(
        CGContextGetClipBoundingBox(canvas->context),
        CGContextGetCTM(canvas->context)
    );
    const CGRect area = isUnboundedBlend(canvas->drawState.blendMode)
        ? clip
        : CGRectIntersection(CGRectStandardize(device), clip);
    if (CGRectIsNull(area) || CGRectIsEmpty(area) || !std::isfinite(area.origin.x) ||
        !std::isfinite(area.origin.y) || !std::isfinite(area.size.width) ||
        !std::isfinite(area.size.height)) {
        return;
    }
    const double limit = static_cast<double>(INT32_MAX / 2);
    const auto pixel = [limit](double value) {
        return static_cast<int32_t>(std::max(-limit, std::min(limit, value)));
    };
    logDirtyPixels(
        canvas,
        pixel(std::floor(CGRectGetMinX(area))) - 1,
        canvas->pixelHeight - pixel(std::ceil(CGRectGetMaxY(area))) - 1,
        pixel(std::ceil(CGRectGetMaxX(area))) + 1,
        canvas->pixelHeight - pixel(std::floor(CGRectGetMinY(area))) + 1
    );
}

static void
invalidateUserRect(BxGraphicsCanvas* canvas, CGRect rect, double outset = 0) {
    if (!canvas->observed || !canvas->context) {
        bumpContentSerial(canvas);
        return;
    }
    const CGRect area = outset > 0 ? CGRectInset(CGRectStandardize(rect), -outset, -outset) : rect;
    invalidateDeviceRect(
        canvas,
        CGRectApplyAffineTransform(area, CGContextGetCTM(canvas->context))
    );
}

static double
strokeReach(BxGraphicsCanvas* canvas) {
    const BxGraphicsDrawState& state = canvas->drawState;
    const double half = std::fabs(state.strokeWidth) / 2;
    return half * (state.strokeJoin == kCGLineJoinMiter ? bxGraphicsMiterLimit : 1.5);
}

static void
resetDrawState(BxGraphicsCanvas* canvas) {
    canvas->drawState = {1, kCGLineJoinMiter, kCGBlendModeNormal};
    canvas->drawDepth = 0;
    canvas->widestStroke = 1;
}

static void
releaseMaskBytes(void*, const void* data, size_t) {
    free(const_cast<void*>(data));
}

static CGColorSpaceRef
getDeviceGrayColorSpace() {
    static CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
    return colorSpace;
}

static uint8_t
unitByte(float value) {
    return static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, std::floor(value * 255.0f + 0.5f))));
}

static uint8_t
pixelAlpha(const BxGraphicsCanvas* canvas, const uint8_t* pixel) {
    if (canvas->format == BxGraphicsFormatRGBA16Float) {
        _Float16 alpha;
        memcpy(&alpha, pixel + 6, sizeof(alpha));
        return unitByte(static_cast<float>(alpha));
    }
    return pixel[canvas->bytesPerPixel - 1];
}

static CGImageRef
getMaskSnapshot(BxGraphicsCanvas* canvas) {
    if (!canvas || !canvas->context || !canvas->pixels) {
        return nullptr;
    }
    if (!canvas->maskSnapshot) {
        const size_t width = static_cast<size_t>(canvas->pixelWidth);
        const size_t height = static_cast<size_t>(canvas->pixelHeight);
        uint8_t* alpha = static_cast<uint8_t*>(malloc(width * height));
        if (!alpha) {
            return nullptr;
        }
        const uint8_t* source = static_cast<const uint8_t*>(canvas->pixels);
        const size_t stride = canvas->bytesPerPixel;
        for (size_t y = 0; y < height; ++y) {
            const uint8_t* row = source + y * canvas->bytesPerRow;
            uint8_t* target = alpha + y * width;
            for (size_t x = 0; x < width; ++x) {
                target[x] = pixelAlpha(canvas, row + x * stride);
            }
        }
        CGDataProviderRef provider =
            CGDataProviderCreateWithData(nullptr, alpha, width * height, releaseMaskBytes);
        if (!provider) {
            free(alpha);
            return nullptr;
        }
        canvas->maskSnapshot = CGImageCreate(
            width,
            height,
            8,
            8,
            width,
            getDeviceGrayColorSpace(),
            kCGImageAlphaNone,
            provider,
            nullptr,
            false,
            kCGRenderingIntentDefault
        );
        CGDataProviderRelease(provider);
    }
    return canvas->maskSnapshot;
}

static CGImageRef
getSnapshot(BxGraphicsCanvas* canvas) {
    if (!canvas || !canvas->context) {
        return nullptr;
    }
    if (!canvas->snapshot) {
        canvas->snapshot = CGBitmapContextCreateImage(canvas->context);
    }
    return canvas->snapshot;
}

static CGColorSpaceRef
getSRGBColorSpace() {
    static CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    return colorSpace;
}

static CGColorRef
createSRGBColor(double red, double green, double blue, double alpha) {
    const CGFloat components[] = {
        static_cast<CGFloat>(red),
        static_cast<CGFloat>(green),
        static_cast<CGFloat>(blue),
        static_cast<CGFloat>(alpha)
    };
    return CGColorCreate(getSRGBColorSpace(), components);
}

static CGColorRef
getCachedSRGBColor(BxGraphicsCanvas* canvas, double red, double green, double blue, double alpha) {
    const double values[] = {red, green, blue, alpha};
    uint64_t hash = 1469598103934665603ull;
    for (double value : values) {
        uint64_t bits;
        memcpy(&bits, &value, sizeof(bits));
        hash = (hash ^ bits) * 1099511628211ull;
        hash ^= hash >> 29;
    }
    BxGraphicsColorCacheEntry& entry = canvas->colorCache[hash % bxGraphicsColorCacheSize];
    if (entry.color && memcmp(&entry.red, values, sizeof(values)) == 0) {
        return entry.color;
    }
    CGColorRef color = createSRGBColor(red, green, blue, alpha);
    if (!color) {
        return nullptr;
    }
    if (entry.color) {
        CGColorRelease(entry.color);
    }
    entry.red = red;
    entry.green = green;
    entry.blue = blue;
    entry.alpha = alpha;
    entry.color = color;
    return color;
}

static void
setCanvasFillColor(
    BxGraphicsCanvas* canvas,
    double red,
    double green,
    double blue,
    double alpha) {
    if (!canvas->preservesSRGBColors) {
        CGContextSetRGBFillColor(canvas->context, red, green, blue, alpha);
        return;
    }
    CGColorRef color = getCachedSRGBColor(canvas, red, green, blue, alpha);
    if (color) {
        CGContextSetFillColorWithColor(canvas->context, color);
    }
}

static void
setCanvasStrokeColor(
    BxGraphicsCanvas* canvas,
    double red,
    double green,
    double blue,
    double alpha) {
    if (!canvas->preservesSRGBColors) {
        CGContextSetRGBStrokeColor(canvas->context, red, green, blue, alpha);
        return;
    }
    CGColorRef color = getCachedSRGBColor(canvas, red, green, blue, alpha);
    if (color) {
        CGContextSetStrokeColorWithColor(canvas->context, color);
    }
}

static void
getCanvasClearPixel(
    BxGraphicsCanvas* canvas,
    double red,
    double green,
    double blue,
    double alpha,
    uint8_t pixel[4]) {
    double matchedRed = red;
    double matchedGreen = green;
    double matchedBlue = blue;
    double matchedAlpha = alpha;
    if (canvas->preservesSRGBColors) {
        CGColorRef source = createSRGBColor(red, green, blue, alpha);
        CGColorRef matched = source
            ? CGColorCreateCopyByMatchingToColorSpace(
                canvas->colorSpace,
                kCGRenderingIntentDefault,
                source,
                nullptr)
            : nullptr;
        if (matched) {
            const CGFloat* components = CGColorGetComponents(matched);
            size_t componentCount = CGColorGetNumberOfComponents(matched);
            if (components && componentCount >= 4) {
                matchedRed = components[0];
                matchedGreen = components[1];
                matchedBlue = components[2];
                matchedAlpha = components[3];
            } else if (components && componentCount >= 2) {
                matchedRed = components[0];
                matchedGreen = components[0];
                matchedBlue = components[0];
                matchedAlpha = components[1];
            }
            CGColorRelease(matched);
        }
        if (source) {
            CGColorRelease(source);
        }
    }

    const double safeAlpha = std::min(1.0, std::max(0.0, matchedAlpha));
    pixel[0] = static_cast<uint8_t>(std::lround(
        std::min(1.0, std::max(0.0, matchedBlue)) * safeAlpha * 255));
    pixel[1] = static_cast<uint8_t>(std::lround(
        std::min(1.0, std::max(0.0, matchedGreen)) * safeAlpha * 255));
    pixel[2] = static_cast<uint8_t>(std::lround(
        std::min(1.0, std::max(0.0, matchedRed)) * safeAlpha * 255));
    pixel[3] = static_cast<uint8_t>(std::lround(safeAlpha * 255));
}

static void
drawCurrentPath(BxGraphicsCanvas* canvas, bool fill, bool stroke, bool evenOdd) {
    if (!canvas || !canvas->context) {
        return;
    }
    if (!fill && !stroke) {
        CGContextBeginPath(canvas->context);
        return;
    }
    invalidateUserRect(canvas, CGContextGetPathBoundingBox(canvas->context), stroke ? strokeReach(canvas) : 0);

    if (fill && stroke) {
        CGContextDrawPath(
            canvas->context,
            evenOdd ? kCGPathEOFillStroke : kCGPathFillStroke
        );
    } else if (fill) {
        CGContextDrawPath(canvas->context, evenOdd ? kCGPathEOFill : kCGPathFill);
    } else {
        CGContextDrawPath(canvas->context, kCGPathStroke);
    }
}

// **********************************************************************************************

extern "C" void*
brixGraphicsCanvasCreate(int32_t width, int32_t height) {
    return brixGraphicsCanvasCreateScaled(width, height, 1.0);
}

static CFStringRef
namedColorSpace(int32_t space) {
    switch (space) {
        case BxGraphicsSpaceDisplayP3:
            return kCGColorSpaceDisplayP3;
        case BxGraphicsSpaceLinearSRGB:
            return kCGColorSpaceExtendedLinearSRGB;
        case BxGraphicsSpaceLinearDisplayP3:
            return kCGColorSpaceExtendedLinearDisplayP3;
        case BxGraphicsSpaceLinearRec2020:
            return kCGColorSpaceExtendedLinearITUR_2020;
        default:
            return kCGColorSpaceSRGB;
    }
}

static int32_t
colorSpaceKind(CGColorSpaceRef colorSpace, bool* named) {
    *named = false;
    CFStringRef name = colorSpace ? CGColorSpaceCopyName(colorSpace) : nullptr;
    if (!name) {
        return BxGraphicsSpaceSRGB;
    }
    const struct {
        CFStringRef name;
        int32_t space;
    } kinds[] = {
        {kCGColorSpaceSRGB, BxGraphicsSpaceSRGB},
        {kCGColorSpaceExtendedSRGB, BxGraphicsSpaceSRGB},
        {kCGColorSpaceDisplayP3, BxGraphicsSpaceDisplayP3},
        {kCGColorSpaceExtendedDisplayP3, BxGraphicsSpaceDisplayP3},
        {kCGColorSpaceLinearSRGB, BxGraphicsSpaceLinearSRGB},
        {kCGColorSpaceExtendedLinearSRGB, BxGraphicsSpaceLinearSRGB},
        {kCGColorSpaceLinearDisplayP3, BxGraphicsSpaceLinearDisplayP3},
        {kCGColorSpaceExtendedLinearDisplayP3, BxGraphicsSpaceLinearDisplayP3},
        {kCGColorSpaceLinearITUR_2020, BxGraphicsSpaceLinearRec2020},
        {kCGColorSpaceExtendedLinearITUR_2020, BxGraphicsSpaceLinearRec2020},
    };
    int32_t space = BxGraphicsSpaceSRGB;
    for (const auto& kind : kinds) {
        if (CFEqual(name, kind.name)) {
            space = kind.space;
            *named = true;
            break;
        }
    }
    CFRelease(name);
    return space;
}

static uint32_t
canvasBitmapInfo(int32_t format, bool alphaOnly) {
    if (alphaOnly) {
        return static_cast<uint32_t>(kCGImageAlphaOnly);
    }
    return format == BxGraphicsFormatRGBA16Float
        ? kCGImageAlphaPremultipliedLast | kCGBitmapFloatComponents | kCGBitmapByteOrder16Little
        : kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little;
}

static size_t
canvasBitsPerComponent(int32_t format, bool alphaOnly) {
    return !alphaOnly && format == BxGraphicsFormatRGBA16Float ? 16 : 8;
}

static int32_t
canvasPixelSpan(double extent, double scale) {
    return static_cast<int32_t>(std::ceil(extent * scale - 1e-6));
}

static void*
createScaledCanvas(
    double width,
    double height,
    double scale,
    CGColorSpaceRef requestedColorSpace,
    bool preservesSRGBColors,
    bool alphaOnly,
    int32_t format = BxGraphicsFormatRGBA8) {
    if (!std::isfinite(scale) || scale <= 0) {
        scale = 1.0;
    }
    if (!(width > 0) || !(height > 0) || canvasPixelSpan(width, scale) <= 0 ||
            canvasPixelSpan(height, scale) <= 0) {
        return nullptr;
    }

    BxGraphicsCanvas* canvas = static_cast<BxGraphicsCanvas*>(calloc(1, sizeof(BxGraphicsCanvas)));
    if (!canvas) {
        return nullptr;
    }

    canvas->width = canvasPixelSpan(width, 1.0);
    canvas->height = canvasPixelSpan(height, 1.0);
    canvas->scale = scale;
    canvas->contentSerial = nextContentSerial();
    canvas->identity = canvas->contentSerial;
    resetDrawState(canvas);
    canvas->pixelWidth = canvasPixelSpan(width, scale);
    canvas->pixelHeight = canvasPixelSpan(height, scale);
    canvas->format = alphaOnly ? BxGraphicsFormatRGBA8 : format;
    canvas->bytesPerPixel = alphaOnly ? 1 : bxGraphicsFormatBytesPerPixel(canvas->format);
    canvas->bytesPerRow = static_cast<size_t>(canvas->pixelWidth) * canvas->bytesPerPixel;
    const size_t pixelBytes = static_cast<size_t>(canvas->pixelHeight) * canvas->bytesPerRow;
    if (pixelBytes >= bxGraphicsPixelStoreThreshold) {
        canvas->store = createPixelStore(pixelBytes);
        canvas->pixels = canvas->store ? canvas->store->data : nullptr;
    } else {
        canvas->pixels = calloc(static_cast<size_t>(canvas->pixelHeight), canvas->bytesPerRow);
    }
    if (alphaOnly) {
        canvas->colorSpace = nullptr;
    } else if (requestedColorSpace &&
        CGColorSpaceGetModel(requestedColorSpace) == kCGColorSpaceModelRGB) {
        canvas->colorSpace = CGColorSpaceRetain(requestedColorSpace);
    } else if (preservesSRGBColors) {
        canvas->colorSpace = CGColorSpaceRetain(getSRGBColorSpace());
    } else {
        canvas->colorSpace = CGColorSpaceCreateDeviceRGB();
    }
    canvas->preservesSRGBColors = preservesSRGBColors;
    canvas->space = colorSpaceKind(canvas->colorSpace, &canvas->namedSpace);

    if (!canvas->pixels || (!alphaOnly && !canvas->colorSpace)) {
        releaseCanvas(canvas);
        free(canvas);
        return nullptr;
    }

    canvas->context = CGBitmapContextCreate(
        canvas->pixels,
        static_cast<size_t>(canvas->pixelWidth),
        static_cast<size_t>(canvas->pixelHeight),
        canvasBitsPerComponent(canvas->format, alphaOnly),
        canvas->bytesPerRow,
        canvas->colorSpace,
        canvasBitmapInfo(canvas->format, alphaOnly)
    );

    if (!canvas->context) {
        releaseCanvas(canvas);
        free(canvas);
        return nullptr;
    }

    CGContextSetAllowsAntialiasing(canvas->context, true);
    CGContextSetShouldAntialias(canvas->context, true);
    CGContextTranslateCTM(canvas->context, 0, canvas->pixelHeight);
    CGContextScaleCTM(canvas->context, scale, -scale);

    // The pixel buffer comes from calloc, so it is already transparent-cleared; an explicit
    // transparent clear here would write every pixel a second time.
    CGContextSaveGState(canvas->context);

    return canvas;
}

extern "C" void*
brixGraphicsCanvasCreateScaled(double width, double height, double scale) {
    return createScaledCanvas(width, height, scale, nullptr, false, false);
}

extern "C" void*
brixGraphicsCanvasCreateAlphaOnly(double width, double height, double scale) {
    return createScaledCanvas(width, height, scale, nullptr, false, true);
}

extern "C" void*
brixGraphicsCanvasCreateScaledWithColorSpace(
    double width,
    double height,
    double scale,
    void* colorSpace) {
    return createScaledCanvas(
        width,
        height,
        scale,
        static_cast<CGColorSpaceRef>(colorSpace),
        true,
        false
    );
}

extern "C" void*
brixGraphicsCanvasCreateFormatted(int32_t width, int32_t height, double scale, int32_t space, int32_t format) {
    if (space < 0 || space >= BxGraphicsSpaceCount) {
        space = BxGraphicsSpaceSRGB;
    }
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(namedColorSpace(space));
    void* canvas = createScaledCanvas(
        width,
        height,
        scale,
        colorSpace,
        true,
        false,
        format == BxGraphicsFormatRGBA16Float ? BxGraphicsFormatRGBA16Float : BxGraphicsFormatRGBA8
    );
    CGColorSpaceRelease(colorSpace);
    return canvas;
}

static void
setSurfaceProperty(CFMutableDictionaryRef properties, CFStringRef key, int32_t value) {
    CFNumberRef number = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value);
    CFDictionarySetValue(properties, key, number);
    CFRelease(number);
}

extern "C" void*
brixGraphicsSurfaceCreate(int32_t pixelWidth, int32_t pixelHeight) {
    if (pixelWidth <= 0 || pixelHeight <= 0) {
        return nullptr;
    }
    CFMutableDictionaryRef properties = CFDictionaryCreateMutable(
        nullptr,
        0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    const size_t bytesPerRow = IOSurfaceAlignProperty(kIOSurfaceBytesPerRow, static_cast<size_t>(pixelWidth) * 4);
    setSurfaceProperty(properties, kIOSurfaceWidth, pixelWidth);
    setSurfaceProperty(properties, kIOSurfaceHeight, pixelHeight);
    setSurfaceProperty(properties, kIOSurfaceBytesPerElement, 4);
    setSurfaceProperty(properties, kIOSurfaceBytesPerRow, static_cast<int32_t>(bytesPerRow));
    setSurfaceProperty(properties, kIOSurfacePixelFormat, static_cast<int32_t>('BGRA'));
    IOSurfaceRef surface = IOSurfaceCreate(properties);
    CFRelease(properties);
    if (surface) {
        CFPropertyListRef space = CGColorSpaceCopyPropertyList(getSRGBColorSpace());
        if (space) {
            IOSurfaceSetValue(surface, kIOSurfaceColorSpace, space);
            CFRelease(space);
        }
    }
    return surface;
}

extern "C" void
brixGraphicsSurfaceRelease(void* surface) {
    if (surface) {
        CFRelease(static_cast<IOSurfaceRef>(surface));
    }
}

extern "C" int32_t
brixGraphicsSurfaceGetPixelWidth(void* surface) {
    return surface ? static_cast<int32_t>(IOSurfaceGetWidth(static_cast<IOSurfaceRef>(surface))) : 0;
}

extern "C" int32_t
brixGraphicsSurfaceGetPixelHeight(void* surface) {
    return surface ? static_cast<int32_t>(IOSurfaceGetHeight(static_cast<IOSurfaceRef>(surface))) : 0;
}

extern "C" uint32_t
brixGraphicsSurfaceCreateMachPort(void* surface) {
    return surface ? IOSurfaceCreateMachPort(static_cast<IOSurfaceRef>(surface)) : MACH_PORT_NULL;
}

extern "C" void*
brixGraphicsSurfaceLookUpMachPort(uint32_t port) {
    if (port == MACH_PORT_NULL) {
        return nullptr;
    }
    IOSurfaceRef surface = IOSurfaceLookupFromMachPort(port);
    mach_port_deallocate(mach_task_self(), port);
    return surface;
}

extern "C" void*
brixGraphicsCanvasCreateWithSurface(void* surfacePointer, double scale) {
    IOSurfaceRef surface = static_cast<IOSurfaceRef>(surfacePointer);
    if (!surface || IOSurfaceGetPixelFormat(surface) != 'BGRA' || IOSurfaceGetBytesPerElement(surface) != 4) {
        return nullptr;
    }
    if (!std::isfinite(scale) || scale <= 0) {
        scale = 1.0;
    }
    BxGraphicsCanvas* canvas = static_cast<BxGraphicsCanvas*>(calloc(1, sizeof(BxGraphicsCanvas)));
    if (!canvas) {
        return nullptr;
    }
    canvas->pixelWidth = static_cast<int32_t>(IOSurfaceGetWidth(surface));
    canvas->pixelHeight = static_cast<int32_t>(IOSurfaceGetHeight(surface));
    canvas->width = std::max<int32_t>(1, static_cast<int32_t>(std::lround(canvas->pixelWidth / scale)));
    canvas->height = std::max<int32_t>(1, static_cast<int32_t>(std::lround(canvas->pixelHeight / scale)));
    canvas->scale = scale;
    canvas->contentSerial = nextContentSerial();
    canvas->identity = canvas->contentSerial;
    resetDrawState(canvas);
    canvas->format = BxGraphicsFormatRGBA8;
    canvas->bytesPerPixel = 4;
    canvas->bytesPerRow = IOSurfaceGetBytesPerRow(surface);
    CFRetain(surface);
    canvas->surface = surface;
    canvas->pixels = IOSurfaceGetBaseAddress(surface);
    canvas->colorSpace = CGColorSpaceRetain(getSRGBColorSpace());
    canvas->preservesSRGBColors = true;
    canvas->space = colorSpaceKind(canvas->colorSpace, &canvas->namedSpace);
    canvas->context = CGBitmapContextCreate(
        canvas->pixels,
        static_cast<size_t>(canvas->pixelWidth),
        static_cast<size_t>(canvas->pixelHeight),
        canvasBitsPerComponent(canvas->format, false),
        canvas->bytesPerRow,
        canvas->colorSpace,
        canvasBitmapInfo(canvas->format, false)
    );
    canvas->gpu = canvas->context ? bxGraphicsGPUCreateWithSurface(surface) : nullptr;
    if (!canvas->gpu) {
        releaseCanvas(canvas);
        free(canvas);
        return nullptr;
    }
    CGContextSetAllowsAntialiasing(canvas->context, true);
    CGContextSetShouldAntialias(canvas->context, true);
    CGContextTranslateCTM(canvas->context, 0, canvas->pixelHeight);
    CGContextScaleCTM(canvas->context, scale, -scale);
    CGContextSaveGState(canvas->context);
    canvas->observed = true;
    canvas->gpuSerial = canvas->contentSerial;
    return canvas;
}

extern "C" void
brixGraphicsCanvasSurfaceChanged(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    if (!canvas || !canvas->surface) {
        return;
    }
    invalidateSnapshot(canvas);
    canvas->gpuSerial = canvas->contentSerial;
}

extern "C" int32_t
brixGraphicsCanvasGetColorSpaceKind(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->space : BxGraphicsSpaceSRGB;
}

extern "C" int32_t
brixGraphicsCanvasGetPixelFormat(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->format : BxGraphicsFormatRGBA8;
}

extern "C" void
brixGraphicsCanvasRelease(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    if (canvas && canvas->observed && gCanvasReleaseObserver) {
        gCanvasReleaseObserver(canvas->identity);
    }
    releaseCanvas(canvas);
    free(canvas);
}

extern "C" void
brixGraphicsSetCanvasReleaseObserver(void (*observer)(uint64_t identity)) {
    gCanvasReleaseObserver = observer;
}

extern "C" uint64_t
brixGraphicsCanvasIdentity(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->identity : 0;
}

extern "C" void*
brixGraphicsCanvasRetainPixelStore(void* ptr) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->store) {
        return nullptr;
    }
    canvas->store->references.fetch_add(1, std::memory_order_relaxed);
    return canvas->store;
}

extern "C" void*
brixGraphicsPixelStoreData(void* store) {
    return store ? static_cast<BxGraphicsPixelStore*>(store)->data : nullptr;
}

extern "C" uint64_t
brixGraphicsPixelStoreLength(void* store) {
    return store ? static_cast<BxGraphicsPixelStore*>(store)->length : 0;
}

extern "C" void
brixGraphicsPixelStoreRelease(void* store) {
    releasePixelStore(static_cast<BxGraphicsPixelStore*>(store));
}

extern "C" void
brixGraphicsCanvasObserveChanges(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    if (canvas) {
        canvas->observed = true;
    }
}

extern "C" bool
brixGraphicsCanvasChangedRegion(void* ptr, uint64_t since, int32_t* region) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    if (!canvas || !region || since >= canvas->contentSerial) {
        return false;
    }
    if (!canvas->observed) {
        region[0] = 0;
        region[1] = 0;
        region[2] = canvas->pixelWidth;
        region[3] = canvas->pixelHeight;
        return true;
    }
    int32_t left = canvas->pixelWidth;
    int32_t top = canvas->pixelHeight;
    int32_t right = 0;
    int32_t bottom = 0;
    for (int32_t index = canvas->dirtyCount - 1; index >= 0; --index) {
        const BxGraphicsDirtyEntry& entry = canvas->dirtyLog[index];
        if (entry.serial <= since) {
            break;
        }
        left = std::min(left, entry.left);
        top = std::min(top, entry.top);
        right = std::max(right, entry.right);
        bottom = std::max(bottom, entry.bottom);
    }
    if (left >= right || top >= bottom) {
        return false;
    }
    region[0] = left;
    region[1] = top;
    region[2] = right;
    region[3] = bottom;
    return true;
}

static void*
canvasGPUTexture(BxGraphicsCanvas* canvas, bool mipmaps) {
    if (!canvas || !canvas->context || !canvas->pixels || canvas->bytesPerPixel == 1) {
        return nullptr;
    }
    flushCanvasPaints(canvas);
    if (!canvas->gpu) {
        canvas->gpu = bxGraphicsGPUCreate(canvas->pixelWidth, canvas->pixelHeight, canvas->format);
        if (!canvas->gpu) {
            return nullptr;
        }
        canvas->observed = true;
        const int32_t full[4] = {0, 0, canvas->pixelWidth, canvas->pixelHeight};
        bxGraphicsGPUUpload(canvas->gpu, static_cast<const uint8_t*>(canvas->pixels), canvas->bytesPerRow, full);
        canvas->gpuSerial = canvas->contentSerial;
    } else if (!canvas->gpuNewer && canvas->gpuSerial != canvas->contentSerial) {
        int32_t region[4];
        if (brixGraphicsCanvasChangedRegion(canvas, canvas->gpuSerial, region)) {
            bxGraphicsGPUUpload(canvas->gpu, static_cast<const uint8_t*>(canvas->pixels), canvas->bytesPerRow, region);
        }
        canvas->gpuSerial = canvas->contentSerial;
    }
    return bxGraphicsGPUTexture(canvas->gpu, mipmaps);
}

static bool
canvasGPUInputs(
    void* const* images,
    const double* scales,
    int64_t imageCount,
    void** textures,
    double* frames
) {
    for (int64_t index = 0; index < imageCount; ++index) {
        BxGraphicsCanvas* input = peekCanvas(images[index]);
        void* texture = canvasGPUTexture(input, false);
        if (!texture || !(scales[index] > 0)) {
            return false;
        }
        textures[index] = texture;
        frames[index * 4] = input->pixelWidth / scales[index];
        frames[index * 4 + 1] = input->pixelHeight / scales[index];
        frames[index * 4 + 2] = scales[index];
        frames[index * 4 + 3] = input->space;
    }
    return true;
}

extern "C" void*
brixGraphicsCanvasGPUTexture(void* ptr, bool mipmaps) {
    return canvasGPUTexture(peekCanvas(ptr), mipmaps);
}

extern "C" bool
brixGraphicsCanvasIsGPUResident(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas && canvas->gpu;
}

extern "C" bool
brixGraphicsCanvasRender(
    void* ptr,
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
) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    if (!canvas || !(scale > 0) || imageCount < 0 || imageCount > bxGraphicsShaderImageLimit ||
            (!args && argCount) || ((!images || !imageScales) && imageCount)) {
        return false;
    }
    if (!canvasGPUTexture(canvas, false)) {
        return false;
    }
    void* textures[bxGraphicsShaderImageLimit] = {};
    double frames[bxGraphicsShaderImageLimit * 4] = {};
    if (!canvasGPUInputs(images, imageScales, imageCount, textures, frames)) {
        return false;
    }
    const double limit = static_cast<double>(INT32_MAX / 2);
    const int32_t region[4] = {
        static_cast<int32_t>(std::max(0.0, std::floor(std::max(-limit, x * scale)))),
        static_cast<int32_t>(std::max(0.0, std::floor(std::max(-limit, y * scale)))),
        static_cast<int32_t>(std::min<double>(canvas->pixelWidth, std::ceil(std::min(limit, (x + width) * scale)))),
        static_cast<int32_t>(std::min<double>(canvas->pixelHeight, std::ceil(std::min(limit, (y + height) * scale)))),
    };
    if (region[2] <= region[0] || region[3] <= region[1]) {
        return true;
    }
    const double size[2] = {canvas->pixelWidth / scale, canvas->pixelHeight / scale};
    if (!bxGraphicsGPURender(
            canvas->gpu,
            region,
            scale,
            size,
            canvas->space,
            shader,
            args,
            argCount,
            textures,
            frames,
            static_cast<int32_t>(imageCount))) {
        return false;
    }
    if (canvas->gpuNewer) {
        canvas->gpuDirty[0] = std::min(canvas->gpuDirty[0], region[0]);
        canvas->gpuDirty[1] = std::min(canvas->gpuDirty[1], region[1]);
        canvas->gpuDirty[2] = std::max(canvas->gpuDirty[2], region[2]);
        canvas->gpuDirty[3] = std::max(canvas->gpuDirty[3], region[3]);
    } else {
        std::copy(region, region + 4, canvas->gpuDirty);
    }
    bumpContentSerial(canvas);
    canvas->gpuSerial = canvas->contentSerial;
    canvas->gpuNewer = true;
    return true;
}

extern "C" int32_t
brixGraphicsCanvasGetWidth(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->width : 0;
}

extern "C" int32_t
brixGraphicsCanvasGetHeight(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->height : 0;
}

extern "C" int32_t
brixGraphicsCanvasGetPixelWidth(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->pixelWidth : 0;
}

extern "C" int32_t
brixGraphicsCanvasGetPixelHeight(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->pixelHeight : 0;
}

extern "C" double
brixGraphicsCanvasGetScale(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->scale : 1.0;
}

extern "C" void*
brixGraphicsCanvasCreateCGImage(void* ptr) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return nullptr;
    }

    CGImageRef image = getSnapshot(canvas);
    if (image) {
        CGImageRetain(image);
    }
    return image;
}

extern "C" uint64_t
brixGraphicsCanvasContentSerial(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->contentSerial : 0;
}

extern "C" void*
brixGraphicsCanvasGetPixelData(void* ptr) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    return canvas ? canvas->pixels : nullptr;
}

extern "C" uint64_t
brixGraphicsCanvasGetBytesPerRow(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->bytesPerRow : 0;
}

extern "C" int32_t
brixGraphicsCanvasGetBytesPerPixel(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? static_cast<int32_t>(canvas->bytesPerPixel) : 0;
}

extern "C" void*
brixGraphicsCanvasGetColorSpace(void* ptr) {
    BxGraphicsCanvas* canvas = peekCanvas(ptr);
    return canvas ? canvas->colorSpace : nullptr;
}

extern "C" void
brixGraphicsCanvasPixelsChanged(void* ptr) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (canvas && canvas->context) {
        invalidateSnapshot(canvas);
        CGContextFlush(canvas->context);
    }
}

static uint32_t
pngWord(const uint8_t* bytes) {
    return (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) |
        (static_cast<uint32_t>(bytes[2]) << 8) | static_cast<uint32_t>(bytes[3]);
}

static void
appendPNGWord(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value >> 24));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value));
}

static bool
inflatePNGData(const std::vector<uint8_t>& packed, std::vector<uint8_t>& raw) {
    if (packed.size() > UINT32_MAX) {
        return false;
    }
    z_stream stream = {};
    if (inflateInit(&stream) != Z_OK) {
        return false;
    }
    stream.next_in = const_cast<Bytef*>(packed.data());
    stream.avail_in = static_cast<uInt>(packed.size());
    const size_t step = std::max<size_t>(packed.size() * 4, 65536);
    int status = Z_OK;
    while (status == Z_OK) {
        size_t used = raw.size();
        raw.resize(used + step);
        stream.next_out = raw.data() + used;
        stream.avail_out = static_cast<uInt>(step);
        status = inflate(&stream, Z_NO_FLUSH);
        raw.resize(raw.size() - stream.avail_out);
    }
    inflateEnd(&stream);
    return status == Z_STREAM_END;
}

static bool
deflatePNGData(const std::vector<uint8_t>& raw, std::vector<uint8_t>& packed) {
    if (raw.size() > UINT32_MAX) {
        return false;
    }
    z_stream stream = {};
    if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS, MAX_MEM_LEVEL, Z_FILTERED) != Z_OK) {
        return false;
    }
    packed.resize(deflateBound(&stream, static_cast<uLong>(raw.size())));
    stream.next_in = const_cast<Bytef*>(raw.data());
    stream.avail_in = static_cast<uInt>(raw.size());
    stream.next_out = packed.data();
    stream.avail_out = static_cast<uInt>(packed.size());
    int status = deflate(&stream, Z_FINISH);
    packed.resize(stream.total_out);
    deflateEnd(&stream);
    return status == Z_STREAM_END;
}

static bool
compactPNG(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    static const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    static const uint8_t idat[4] = {'I', 'D', 'A', 'T'};
    if (size < 8 || memcmp(data, signature, 8) != 0) {
        return false;
    }
    std::vector<uint8_t> packed;
    for (size_t offset = 8; offset + 12 <= size;) {
        size_t length = pngWord(data + offset);
        if (length > size - offset - 12) {
            return false;
        }
        if (memcmp(data + offset + 4, idat, 4) == 0) {
            packed.insert(packed.end(), data + offset + 8, data + offset + 8 + length);
        }
        offset += 12 + length;
    }
    std::vector<uint8_t> raw;
    std::vector<uint8_t> squeezed;
    if (packed.empty() || !inflatePNGData(packed, raw) || !deflatePNGData(raw, squeezed) ||
        squeezed.size() >= packed.size()) {
        return false;
    }
    out.assign(data, data + 8);
    bool placed = false;
    for (size_t offset = 8; offset + 12 <= size;) {
        size_t length = pngWord(data + offset);
        if (memcmp(data + offset + 4, idat, 4) != 0) {
            out.insert(out.end(), data + offset, data + offset + 12 + length);
        } else if (!placed) {
            appendPNGWord(out, static_cast<uint32_t>(squeezed.size()));
            size_t start = out.size();
            out.insert(out.end(), idat, idat + 4);
            out.insert(out.end(), squeezed.begin(), squeezed.end());
            appendPNGWord(out, static_cast<uint32_t>(crc32(0, out.data() + start, static_cast<uInt>(squeezed.size() + 4))));
            placed = true;
        }
        offset += 12 + length;
    }
    return true;
}

static CFMutableDataRef
encodeCanvasPNG(BxGraphicsCanvas* canvas, bool compact) {
    CGImageRef image = CGBitmapContextCreateImage(canvas->context);
    if (!image) {
        return nullptr;
    }

    CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault, 0);
    if (!data) {
        CGImageRelease(image);
        return nullptr;
    }

    CGImageDestinationRef destination = CGImageDestinationCreateWithData(
        data,
        CFSTR("public.png"),
        1,
        nullptr
    );
    if (!destination) {
        CFRelease(data);
        CGImageRelease(image);
        return nullptr;
    }

    CGImageDestinationAddImage(destination, image, nullptr);
    bool didEncode = CGImageDestinationFinalize(destination);

    CFRelease(destination);
    CGImageRelease(image);

    if (!didEncode) {
        CFRelease(data);
        return nullptr;
    }
    std::vector<uint8_t> smaller;
    if (compact && compactPNG(CFDataGetBytePtr(data), static_cast<size_t>(CFDataGetLength(data)), smaller)) {
        CFDataReplaceBytes(
            data,
            CFRangeMake(0, CFDataGetLength(data)),
            smaller.data(),
            static_cast<CFIndex>(smaller.size())
        );
    }
    return data;
}

extern "C" bool
brixGraphicsCanvasWritePNG(void* ptr, const char* path, bool compact) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || !path) {
        return false;
    }

    CFMutableDataRef data = encodeCanvasPNG(canvas, compact);
    if (!data) {
        return false;
    }

    const size_t length = static_cast<size_t>(CFDataGetLength(data));
    FILE* file = fopen(path, "wb");
    bool didWrite = file && fwrite(CFDataGetBytePtr(data), 1, length, file) == length;
    if (file && fclose(file) != 0) {
        didWrite = false;
    }
    CFRelease(data);
    return didWrite;
}

extern "C" void*
brixGraphicsCanvasEncodePNG(void* ptr, bool compact) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return nullptr;
    }
    return encodeCanvasPNG(canvas, compact);
}

extern "C" uint64_t
brixGraphicsEncodedLength(void* encoded) {
    return encoded ? static_cast<uint64_t>(CFDataGetLength(static_cast<CFDataRef>(encoded))) : 0;
}

extern "C" void
brixGraphicsEncodedCopy(void* encoded, uint8_t* bytes, uint64_t length) {
    if (!encoded || !bytes) {
        return;
    }
    CFDataRef data = static_cast<CFDataRef>(encoded);
    uint64_t available = static_cast<uint64_t>(CFDataGetLength(data));
    CFDataGetBytes(data, CFRangeMake(0, static_cast<CFIndex>(length < available ? length : available)), bytes);
}

extern "C" void
brixGraphicsEncodedRelease(void* encoded) {
    if (encoded) {
        CFRelease(encoded);
    }
}

extern "C" void*
brixGraphicsCanvasEncodeJPEG(void* ptr, double quality) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return nullptr;
    }

    CGImageRef image = CGBitmapContextCreateImage(canvas->context);
    if (!image) {
        return nullptr;
    }

    CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault, 0);
    if (!data) {
        CGImageRelease(image);
        return nullptr;
    }

    CGImageDestinationRef destination = CGImageDestinationCreateWithData(
        data,
        CFSTR("public.jpeg"),
        1,
        nullptr
    );
    if (!destination) {
        CFRelease(data);
        CGImageRelease(image);
        return nullptr;
    }

    CFNumberRef compression = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &quality);
    const void* keys[] = {kCGImageDestinationLossyCompressionQuality};
    const void* values[] = {compression};
    CFDictionaryRef properties = CFDictionaryCreate(
        kCFAllocatorDefault,
        keys,
        values,
        1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    CFRelease(compression);
    CGImageDestinationAddImage(destination, image, properties);
    CFRelease(properties);
    bool didEncode = CGImageDestinationFinalize(destination);

    CFRelease(destination);
    CGImageRelease(image);

    if (!didEncode) {
        CFRelease(data);
        return nullptr;
    }
    return data;
}

static size_t
imageSourceLongestSide(CGImageSourceRef source) {
    CFDictionaryRef properties = CGImageSourceCopyPropertiesAtIndex(source, 0, nullptr);
    if (!properties) {
        return 0;
    }
    int64_t width = 0;
    int64_t height = 0;
    CFNumberRef value = static_cast<CFNumberRef>(CFDictionaryGetValue(properties, kCGImagePropertyPixelWidth));
    if (value) {
        CFNumberGetValue(value, kCFNumberSInt64Type, &width);
    }
    value = static_cast<CFNumberRef>(CFDictionaryGetValue(properties, kCGImagePropertyPixelHeight));
    if (value) {
        CFNumberGetValue(value, kCFNumberSInt64Type, &height);
    }
    CFRelease(properties);
    return static_cast<size_t>(std::max<int64_t>(std::max(width, height), 0));
}

static CGImageRef
createSourceImage(CGImageSourceRef source, int32_t maxPixelSize, int32_t format, bool fromImage) {
    CFMutableDictionaryRef options = CFDictionaryCreateMutable(
        kCFAllocatorDefault,
        4,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    if (!options) {
        return nullptr;
    }
    CFDictionarySetValue(
        options,
        fromImage ? kCGImageSourceCreateThumbnailFromImageAlways : kCGImageSourceCreateThumbnailFromImageIfAbsent,
        kCFBooleanTrue
    );
    CFDictionarySetValue(options, kCGImageSourceCreateThumbnailWithTransform, kCFBooleanTrue);
    if (format == BxGraphicsFormatRGBA16Float) {
        CFDictionarySetValue(options, kCGImageSourceShouldAllowFloat, kCFBooleanTrue);
    }
    if (maxPixelSize > 0) {
        CFNumberRef limit = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &maxPixelSize);
        if (limit) {
            CFDictionarySetValue(options, kCGImageSourceThumbnailMaxPixelSize, limit);
            CFRelease(limit);
        }
    }
    CGImageRef image = CGImageSourceCreateThumbnailAtIndex(source, 0, options);
    CFRelease(options);
    return image;
}

static void*
decodeImageSource(CGImageSourceRef source, int32_t maxPixelSize, int32_t space, int32_t format, bool prefersEmbedded) {
    if (!source || CGImageSourceGetCount(source) == 0) {
        return nullptr;
    }
    CGImageRef image = nullptr;
    if (prefersEmbedded && maxPixelSize > 0) {
        image = createSourceImage(source, maxPixelSize, format, false);
        size_t full = imageSourceLongestSide(source);
        size_t wanted = full ? std::min<size_t>(full, static_cast<size_t>(maxPixelSize)) : static_cast<size_t>(maxPixelSize);
        if (image && std::max(CGImageGetWidth(image), CGImageGetHeight(image)) * 4 < wanted * 3) {
            CGImageRelease(image);
            image = nullptr;
        }
    }
    if (!image) {
        image = createSourceImage(source, maxPixelSize, format, true);
    }
    if (!image) {
        return nullptr;
    }
    const size_t width = CGImageGetWidth(image);
    const size_t height = CGImageGetHeight(image);
    if (!width || !height || width > INT32_MAX || height > INT32_MAX) {
        CGImageRelease(image);
        return nullptr;
    }
    if (space < 0 || space >= BxGraphicsSpaceCount) {
        CGColorSpaceRef source = CGImageGetColorSpace(image);
        space = source && CGColorSpaceIsWideGamutRGB(source) ? BxGraphicsSpaceDisplayP3 : BxGraphicsSpaceSRGB;
    }
    CGColorSpaceRef colorSpace = space == BxGraphicsSpaceSRGB
        ? CGColorSpaceRetain(getSRGBColorSpace())
        : CGColorSpaceCreateWithName(namedColorSpace(space));
    BxGraphicsCanvas* canvas = static_cast<BxGraphicsCanvas*>(createScaledCanvas(
        static_cast<int32_t>(width),
        static_cast<int32_t>(height),
        1.0,
        colorSpace,
        true,
        false,
        format == BxGraphicsFormatRGBA16Float ? BxGraphicsFormatRGBA16Float : BxGraphicsFormatRGBA8
    ));
    CGColorSpaceRelease(colorSpace);
    if (!canvas) {
        CGImageRelease(image);
        return nullptr;
    }
    CGContextRef context = canvas->context;
    CGContextSaveGState(context);
    CGContextConcatCTM(context, CGAffineTransformInvert(CGContextGetCTM(context)));
    CGContextSetBlendMode(context, kCGBlendModeCopy);
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
    CGContextRestoreGState(context);
    CGImageRelease(image);
    invalidateSnapshot(canvas);
    return canvas;
}

extern "C" void*
brixGraphicsImageDecodeData(
    const uint8_t* bytes,
    uint64_t length,
    int32_t maxPixelSize,
    int32_t space,
    int32_t format,
    bool prefersEmbedded
) {
    if (!bytes || !length) {
        return nullptr;
    }
    CFDataRef data = CFDataCreateWithBytesNoCopy(
        kCFAllocatorDefault,
        bytes,
        static_cast<CFIndex>(length),
        kCFAllocatorNull
    );
    if (!data) {
        return nullptr;
    }
    CGImageSourceRef source = CGImageSourceCreateWithData(data, nullptr);
    void* canvas = decodeImageSource(source, maxPixelSize, space, format, prefersEmbedded);
    if (source) {
        CFRelease(source);
    }
    CFRelease(data);
    return canvas;
}

static double
imagePropertyNumber(CFDictionaryRef properties, CFStringRef key, double fallback) {
    CFNumberRef value = static_cast<CFNumberRef>(CFDictionaryGetValue(properties, key));
    double number = 0;
    if (!value || !CFNumberGetValue(value, kCFNumberDoubleType, &number) || !(number > 0)) {
        return fallback;
    }
    return number;
}

extern "C" bool
brixGraphicsImageMeasureData(const uint8_t* bytes, uint64_t length, double* measures) {
    if (!bytes || !length || !measures) {
        return false;
    }
    CFDataRef data = CFDataCreateWithBytesNoCopy(
        kCFAllocatorDefault,
        bytes,
        static_cast<CFIndex>(length),
        kCFAllocatorNull
    );
    if (!data) {
        return false;
    }
    CGImageSourceRef source = CGImageSourceCreateWithData(data, nullptr);
    CFRelease(data);
    if (!source) {
        return false;
    }
    CFDictionaryRef properties = CGImageSourceGetCount(source) > 0
        ? CGImageSourceCopyPropertiesAtIndex(source, 0, nullptr)
        : nullptr;
    CFRelease(source);
    if (!properties) {
        return false;
    }
    double width = imagePropertyNumber(properties, kCGImagePropertyPixelWidth, 0);
    double height = imagePropertyNumber(properties, kCGImagePropertyPixelHeight, 0);
    double dpiX = imagePropertyNumber(properties, kCGImagePropertyDPIWidth, 72);
    double dpiY = imagePropertyNumber(properties, kCGImagePropertyDPIHeight, dpiX);
    double orientation = imagePropertyNumber(properties, kCGImagePropertyOrientation, 1);
    CFRelease(properties);
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (orientation >= 5 && orientation <= 8) {
        std::swap(width, height);
        std::swap(dpiX, dpiY);
    }
    measures[0] = width;
    measures[1] = height;
    measures[2] = dpiX;
    measures[3] = dpiY;
    return true;
}

extern "C" void*
brixGraphicsImageDecodeFile(
    const char* path,
    int32_t maxPixelSize,
    int32_t space,
    int32_t format,
    bool prefersEmbedded
) {
    if (!path) {
        return nullptr;
    }
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(path),
        static_cast<CFIndex>(strlen(path)),
        false
    );
    if (!url) {
        return nullptr;
    }
    CGImageSourceRef source = CGImageSourceCreateWithURL(url, nullptr);
    CFRelease(url);
    void* canvas = decodeImageSource(source, maxPixelSize, space, format, prefersEmbedded);
    if (source) {
        CFRelease(source);
    }
    return canvas;
}

extern "C" uint64_t
brixGraphicsCanvasCountNonTransparentPixels(
    void* ptr,
    double x,
    double y,
    double width,
    double height
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->pixels || width <= 0 || height <= 0) {
        return 0;
    }

    int32_t left = std::max(0, static_cast<int32_t>(std::floor(x * canvas->scale)));
    int32_t top = std::max(0, static_cast<int32_t>(std::floor(y * canvas->scale)));
    int32_t right = std::min(
        canvas->pixelWidth,
        static_cast<int32_t>(std::ceil((x + width) * canvas->scale))
    );
    int32_t bottom = std::min(
        canvas->pixelHeight,
        static_cast<int32_t>(std::ceil((y + height) * canvas->scale))
    );
    if (right <= left || bottom <= top) {
        return 0;
    }

    const uint8_t* pixels = static_cast<const uint8_t*>(canvas->pixels);
    const size_t stride = canvas->bytesPerPixel;
    uint64_t count = 0;
    for (int32_t py = top; py < bottom; ++py) {
        const uint8_t* row = pixels + static_cast<size_t>(py) * canvas->bytesPerRow;
        for (int32_t px = left; px < right; ++px) {
            if (pixelAlpha(canvas, row + static_cast<size_t>(px) * stride)) {
                ++count;
            }
        }
    }
    return count;
}

extern "C" bool
brixGraphicsCanvasOpaqueBounds(void* ptr, int32_t minimumAlpha, double* bounds) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->pixels || !bounds) {
        return false;
    }

    const uint8_t* pixels = static_cast<const uint8_t*>(canvas->pixels);
    const size_t stride = canvas->bytesPerPixel;
    int32_t left = canvas->pixelWidth;
    int32_t top = canvas->pixelHeight;
    int32_t right = -1;
    int32_t bottom = -1;
    for (int32_t py = 0; py < canvas->pixelHeight; ++py) {
        const uint8_t* row = pixels + static_cast<size_t>(py) * canvas->bytesPerRow;
        for (int32_t px = 0; px < canvas->pixelWidth; ++px) {
            if (pixelAlpha(canvas, row + static_cast<size_t>(px) * stride) > minimumAlpha) {
                left = std::min(left, px);
                right = std::max(right, px);
                top = std::min(top, py);
                bottom = py;
            }
        }
    }
    if (right < left || bottom < top) {
        return false;
    }
    bounds[0] = left;
    bounds[1] = top;
    bounds[2] = right + 1 - left;
    bounds[3] = bottom + 1 - top;
    return true;
}

extern "C" uint32_t
brixGraphicsCanvasGetPixel(void* ptr, double x, double y) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->pixels) {
        return 0;
    }

    int32_t px = static_cast<int32_t>(std::floor(x * canvas->scale));
    int32_t py = static_cast<int32_t>(std::floor(y * canvas->scale));
    if (px < 0 || py < 0 || px >= canvas->pixelWidth || py >= canvas->pixelHeight) {
        return 0;
    }

    const uint8_t* pixel = static_cast<const uint8_t*>(canvas->pixels) +
        static_cast<size_t>(py) * canvas->bytesPerRow +
        static_cast<size_t>(px) * canvas->bytesPerPixel;
    if (canvas->bytesPerPixel == 1) {
        return static_cast<uint32_t>(pixel[0]) << 24;
    }
    if (canvas->format == BxGraphicsFormatRGBA16Float) {
        _Float16 channels[4];
        memcpy(channels, pixel, sizeof(channels));
        return static_cast<uint32_t>(unitByte(static_cast<float>(channels[0]))) |
            (static_cast<uint32_t>(unitByte(static_cast<float>(channels[1]))) << 8) |
            (static_cast<uint32_t>(unitByte(static_cast<float>(channels[2]))) << 16) |
            (static_cast<uint32_t>(unitByte(static_cast<float>(channels[3]))) << 24);
    }
    return static_cast<uint32_t>(pixel[2]) | (static_cast<uint32_t>(pixel[1]) << 8) |
        (static_cast<uint32_t>(pixel[0]) << 16) | (static_cast<uint32_t>(pixel[3]) << 24);
}

extern "C" bool
brixGraphicsCanvasCopyPixels(void* ptr, uint8_t* out, uint64_t size) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->pixels || !out) {
        return false;
    }

    const size_t width = static_cast<size_t>(canvas->pixelWidth);
    const size_t height = static_cast<size_t>(canvas->pixelHeight);
    if (size < width * height * 4) {
        return false;
    }

    const uint8_t* pixels = static_cast<const uint8_t*>(canvas->pixels);
    for (size_t py = 0; py < height; ++py) {
        const uint8_t* row = pixels + py * canvas->bytesPerRow;
        uint8_t* target = out + py * width * 4;
        for (size_t px = 0; px < width; ++px) {
            const uint8_t* pixel = row + px * canvas->bytesPerPixel;
            uint8_t* color = target + px * 4;
            if (canvas->bytesPerPixel == 1) {
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
                color[3] = pixel[0];
            } else if (canvas->format == BxGraphicsFormatRGBA16Float) {
                _Float16 channels[4];
                memcpy(channels, pixel, sizeof(channels));
                for (size_t channel = 0; channel < 4; ++channel) {
                    color[channel] = unitByte(static_cast<float>(channels[channel]));
                }
            } else {
                color[0] = pixel[2];
                color[1] = pixel[1];
                color[2] = pixel[0];
                color[3] = pixel[3];
            }
        }
    }
    return true;
}

extern "C" bool
brixGraphicsCanvasWritePixels(void* ptr, const uint8_t* in, uint64_t size) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->pixels || !in || canvas->bytesPerPixel != 4 ||
        canvas->format == BxGraphicsFormatRGBA16Float) {
        return false;
    }

    const size_t width = static_cast<size_t>(canvas->pixelWidth);
    const size_t height = static_cast<size_t>(canvas->pixelHeight);
    if (size < width * height * 4) {
        return false;
    }

    uint8_t* pixels = static_cast<uint8_t*>(canvas->pixels);
    for (size_t py = 0; py < height; ++py) {
        uint8_t* row = pixels + py * canvas->bytesPerRow;
        const uint8_t* source = in + py * width * 4;
        for (size_t px = 0; px < width; ++px) {
            uint8_t* pixel = row + px * 4;
            const uint8_t* color = source + px * 4;
            pixel[0] = color[2];
            pixel[1] = color[1];
            pixel[2] = color[0];
            pixel[3] = color[3];
        }
    }
    brixGraphicsCanvasPixelsChanged(ptr);
    return true;
}

extern "C" uint64_t
brixGraphicsCanvasCountDifferentPixels(void* leftPtr, void* rightPtr) {
    BxGraphicsCanvas* left = getCanvas(leftPtr);
    BxGraphicsCanvas* right = getCanvas(rightPtr);
    if (!left || !right || !left->pixels || !right->pixels ||
        left->pixelWidth != right->pixelWidth || left->pixelHeight != right->pixelHeight ||
        left->bytesPerPixel != right->bytesPerPixel) {
        return UINT64_MAX;
    }

    const uint8_t* leftPixels = static_cast<const uint8_t*>(left->pixels);
    const uint8_t* rightPixels = static_cast<const uint8_t*>(right->pixels);
    uint64_t count = 0;
    for (int32_t y = 0; y < left->pixelHeight; ++y) {
        const uint8_t* leftRow = leftPixels + static_cast<size_t>(y) * left->bytesPerRow;
        const uint8_t* rightRow = rightPixels + static_cast<size_t>(y) * right->bytesPerRow;
        for (int32_t x = 0; x < left->pixelWidth; ++x) {
            const size_t offset = static_cast<size_t>(x) * left->bytesPerPixel;
            if (std::memcmp(leftRow + offset, rightRow + offset, left->bytesPerPixel) != 0) {
                ++count;
            }
        }
    }
    return count;
}

// **********************************************************************************************

extern "C" void
brixGraphicsSave(void* ptr) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        CGContextSaveGState(canvas->context);
        if (canvas->drawDepth < bxGraphicsDrawStateDepth) {
            canvas->drawStates[canvas->drawDepth] = canvas->drawState;
        }
        ++canvas->drawDepth;
    }
}

extern "C" void
brixGraphicsRestore(void* ptr) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        if (canvas->drawDepth <= 0 || canvas->drawDepth > bxGraphicsDrawStateDepth ||
                canvas->drawStates[canvas->drawDepth - 1].compositeSerial != canvas->drawState.compositeSerial) {
            flushCanvasPaints(canvas);
        }
        CGContextRestoreGState(canvas->context);
        if (canvas->drawDepth <= 0) {
            canvas->drawState = {1, kCGLineJoinMiter, kCGBlendModeNormal};
            return;
        }
        --canvas->drawDepth;
        if (canvas->drawDepth < bxGraphicsDrawStateDepth) {
            canvas->drawState = canvas->drawStates[canvas->drawDepth];
        } else {
            canvas->drawState = {canvas->widestStroke, kCGLineJoinMiter, kCGBlendModeCopy};
        }
    }
}

extern "C" void
brixGraphicsTransform(
    void* ptr,
    double a,
    double c,
    double b,
    double d,
    double tx,
    double ty
) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        CGContextConcatCTM(
            canvas->context,
            CGAffineTransformMake(a, c, b, d, tx, ty)
        );
    }
}

extern "C" void
brixGraphicsTranslate(void* ptr, double x, double y) {
    brixGraphicsTransform(ptr, 1, 0, 0, 1, x, y);
}

extern "C" void
brixGraphicsClipRect(void* ptr, double x, double y, double width, double height) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (canvas && canvas->context) {
        ++canvas->drawState.compositeSerial;
        CGContextClipToRect(canvas->context, CGRectMake(x, y, width, height));
    }
}

extern "C" void
brixGraphicsClipPath(void* ptr, int32_t fillRule) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (canvas && canvas->context) {
        ++canvas->drawState.compositeSerial;
        if (fillRule == 1) {
            CGContextEOClip(canvas->context);
        } else {
            CGContextClip(canvas->context);
        }
    }
}

extern "C" void
brixGraphicsClearRect(void* ptr, double x, double y, double width, double height) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (canvas && canvas->context) {
        invalidateUserRect(canvas, CGRectMake(x, y, width, height));
        CGContextClearRect(canvas->context, CGRectMake(x, y, width, height));
    }
}

extern "C" void
brixGraphicsSetBlendMode(void* ptr, int32_t mode) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (canvas && canvas->context && mode >= kCGBlendModeNormal && mode <= kCGBlendModePlusLighter) {
        CGContextSetBlendMode(canvas->context, static_cast<CGBlendMode>(mode));
        canvas->drawState.blendMode = mode;
        ++canvas->drawState.compositeSerial;
    }
}

// **********************************************************************************************

extern "C" void
brixGraphicsClear(void* ptr, double red, double green, double blue, double alpha) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || !canvas->pixels) {
        return;
    }
    invalidateSnapshot(canvas);

    if (canvas->format == BxGraphicsFormatRGBA16Float) {
        _Float16 pixel[4] = {};
        CGContextRef sample = CGBitmapContextCreate(
            pixel,
            1,
            1,
            16,
            sizeof(pixel),
            canvas->colorSpace,
            canvasBitmapInfo(canvas->format, false)
        );
        if (sample) {
            CGColorRef color = createSRGBColor(red, green, blue, alpha);
            CGContextSetBlendMode(sample, kCGBlendModeCopy);
            CGContextSetFillColorWithColor(sample, color);
            CGContextFillRect(sample, CGRectMake(0, 0, 1, 1));
            CGColorRelease(color);
            CGContextRelease(sample);
        }
        uint8_t* pixels = static_cast<uint8_t*>(canvas->pixels);
        const size_t rowBytes = static_cast<size_t>(canvas->pixelWidth) * sizeof(pixel);
        for (int32_t y = 0; y < canvas->pixelHeight; ++y) {
            memset_pattern8(pixels + static_cast<size_t>(y) * canvas->bytesPerRow, pixel, rowBytes);
        }
        CGContextFlush(canvas->context);
        return;
    }

    uint8_t pixel[4];
    getCanvasClearPixel(canvas, red, green, blue, alpha, pixel);
    uint8_t* pixels = static_cast<uint8_t*>(canvas->pixels);
    if (canvas->bytesPerPixel == 1) {
        for (int32_t y = 0; y < canvas->pixelHeight; ++y) {
            memset(pixels + static_cast<size_t>(y) * canvas->bytesPerRow, pixel[3],
                static_cast<size_t>(canvas->pixelWidth));
        }
        CGContextFlush(canvas->context);
        return;
    }
    const size_t rowBytes = static_cast<size_t>(canvas->pixelWidth) * 4;
    uint32_t pattern;
    memcpy(&pattern, pixel, sizeof(pattern));
    if (canvas->bytesPerRow == rowBytes) {
        const size_t totalBytes = rowBytes * static_cast<size_t>(canvas->pixelHeight);
        if (pattern == 0) {
            memset(pixels, 0, totalBytes);
        } else {
            memset_pattern4(pixels, pixel, totalBytes);
        }
    } else {
        for (int32_t y = 0; y < canvas->pixelHeight; ++y) {
            uint8_t* row = pixels + static_cast<size_t>(y) * canvas->bytesPerRow;
            if (pattern == 0) {
                memset(row, 0, rowBytes);
            } else {
                memset_pattern4(row, pixel, rowBytes);
            }
        }
    }
    CGContextFlush(canvas->context);
}

extern "C" void
brixGraphicsSetFillColor(void* ptr, double red, double green, double blue, double alpha) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        setCanvasFillColor(canvas, red, green, blue, alpha);
    }
}

extern "C" void
brixGraphicsSetStrokeColor(void* ptr, double red, double green, double blue, double alpha) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        setCanvasStrokeColor(canvas, red, green, blue, alpha);
    }
}

extern "C" void
brixGraphicsSetStrokeWidth(void* ptr, double width) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        CGContextSetLineWidth(canvas->context, width);
        canvas->drawState.strokeWidth = width;
        canvas->widestStroke = std::max(canvas->widestStroke, std::fabs(width));
    }
}

extern "C" void
brixGraphicsSetStrokeCap(void* ptr, int32_t cap) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (!canvas || !canvas->context) {
        return;
    }
    CGLineCap nativeCap = kCGLineCapButt;
    if (cap == 1) {
        nativeCap = kCGLineCapRound;
    } else if (cap == 2) {
        nativeCap = kCGLineCapSquare;
    }
    CGContextSetLineCap(canvas->context, nativeCap);
}

extern "C" void
brixGraphicsSetStrokeJoin(void* ptr, int32_t join) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (!canvas || !canvas->context) {
        return;
    }
    CGLineJoin nativeJoin = kCGLineJoinMiter;
    if (join == 1) {
        nativeJoin = kCGLineJoinRound;
    } else if (join == 2) {
        nativeJoin = kCGLineJoinBevel;
    }
    CGContextSetLineJoin(canvas->context, nativeJoin);
    canvas->drawState.strokeJoin = nativeJoin;
}

extern "C" void
brixGraphicsSetStrokeDash(void* ptr, double length) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (!canvas || !canvas->context) {
        return;
    }
    if (length > 0) {
        CGFloat lengths[2] = {length, length};
        CGContextSetLineDash(canvas->context, 0, lengths, 2);
    } else {
        CGContextSetLineDash(canvas->context, 0, NULL, 0);
    }
}

extern "C" void
brixGraphicsFillGradient(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    int32_t kind,
    const double* geometry,
    const double* components,
    const double* locations,
    int64_t stopCount
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || width <= 0 || height <= 0 || !geometry ||
        !components || !locations || stopCount < 2 || kind < 1 || kind > 3) {
        return;
    }

    CGGradientRef gradient = CGGradientCreateWithColorComponents(
        getSRGBColorSpace(),
        components,
        locations,
        static_cast<size_t>(stopCount)
    );
    if (!gradient) {
        return;
    }

    invalidateUserRect(canvas, CGRectMake(x, y, width, height));
    CGContextSaveGState(canvas->context);
    CGContextClipToRect(canvas->context, CGRectMake(x, y, width, height));
    const CGGradientDrawingOptions extend =
        kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation;
    if (kind == 1) {
        CGContextDrawLinearGradient(
            canvas->context,
            gradient,
            CGPointMake(geometry[0], geometry[1]),
            CGPointMake(geometry[3], geometry[4]),
            extend
        );
    } else if (kind == 2) {
        CGContextDrawRadialGradient(
            canvas->context,
            gradient,
            CGPointMake(geometry[0], geometry[1]),
            geometry[2],
            CGPointMake(geometry[3], geometry[4]),
            geometry[5],
            extend
        );
    } else {
        CGContextDrawConicGradient(
            canvas->context,
            gradient,
            CGPointMake(geometry[0], geometry[1]),
            geometry[2]
        );
    }
    CGContextRestoreGState(canvas->context);
    CGGradientRelease(gradient);
}


struct BxGraphicsShaderArea {
    double left;
    double bottom;
    int32_t columns;
    int32_t rows;
    CGAffineTransform device;
    CGAffineTransform inverse;
};

static bool
shaderArea(BxGraphicsCanvas* canvas, CGRect rect, BxGraphicsShaderArea& result) {
    CGContextRef context = canvas->context;
    CGAffineTransform device = CGContextGetUserSpaceToDeviceSpaceTransform(context);
    CGRect area = CGRectIntersection(
        CGRectApplyAffineTransform(rect, device),
        CGRectApplyAffineTransform(CGContextGetClipBoundingBox(context), device)
    );
    area = CGRectIntersection(area, CGRectMake(0, 0, canvas->pixelWidth, canvas->pixelHeight));
    if (CGRectIsNull(area) || CGRectIsEmpty(area)) {
        return false;
    }
    result.left = std::floor(CGRectGetMinX(area));
    result.bottom = std::floor(CGRectGetMinY(area));
    result.columns = static_cast<int32_t>(std::ceil(CGRectGetMaxX(area)) - result.left);
    result.rows = static_cast<int32_t>(std::ceil(CGRectGetMaxY(area)) - result.bottom);
    result.device = device;
    result.inverse = CGAffineTransformInvert(device);
    return result.columns > 0 && result.rows > 0;
}

static CGImageRef
shaderAreaImage(
    BxGraphicsCanvas* canvas,
    const BxGraphicsShaderArea& area,
    CGRect frame,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double time
) {
    CGAffineTransform raster = CGAffineTransformConcat(
        CGAffineTransformConcat(
            CGAffineTransformMake(1, 0, 0, -1, area.left, area.bottom + area.rows),
            area.inverse
        ),
        CGAffineTransformMakeTranslation(-frame.origin.x, -frame.origin.y)
    );
    const double transform[6] = {raster.a, raster.b, raster.c, raster.d, raster.tx, raster.ty};
    return static_cast<CGImageRef>(bxGraphicsShaderRaster(
        shader,
        area.columns,
        area.rows,
        transform,
        frame.size.width,
        frame.size.height,
        args,
        argCount,
        time,
        canvas->format,
        canvas->namedSpace ? canvas->space : BxGraphicsSpaceSRGB,
        canvas->namedSpace ? canvas->colorSpace : getSRGBColorSpace()
    ));
}

extern "C" void
brixGraphicsFillShader(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    uint32_t shader,
    const double* args,
    int64_t argCount
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || width <= 0 || height <= 0 || (!args && argCount)) {
        return;
    }
    CGContextRef context = canvas->context;
    CGRect rect = CGRectMake(x, y, width, height);
    BxGraphicsShaderArea area;
    if (!shaderArea(canvas, rect, area)) {
        return;
    }
    CGImageRef image = shaderAreaImage(canvas, area, rect, shader, args, argCount, 0);
    if (!image) {
        return;
    }
    invalidateUserRect(canvas, rect);
    CGContextSaveGState(context);
    CGContextClipToRect(context, rect);
    CGContextConcatCTM(context, area.inverse);
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextDrawImage(context, CGRectMake(area.left, area.bottom, area.columns, area.rows), image);
    CGContextRestoreGState(context);
    CGImageRelease(image);
}

extern "C" void
brixGraphicsFillPathShader(
    void* ptr,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double time,
    int32_t fillRule
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || (!args && argCount)) {
        return;
    }
    CGContextRef context = canvas->context;
    if (CGContextIsPathEmpty(context)) {
        return;
    }
    CGPathRef path = CGContextCopyPath(context);
    CGRect bounds = CGPathGetPathBoundingBox(path);
    BxGraphicsShaderArea area;
    if (bounds.size.width > 0 && bounds.size.height > 0 && shaderArea(canvas, bounds, area)) {
        CGImageRef image = shaderAreaImage(canvas, area, bounds, shader, args, argCount, time);
        if (image) {
            invalidateUserRect(canvas, bounds);
            CGContextSaveGState(context);
            if (fillRule == 1) {
                CGContextEOClip(context);
            } else {
                CGContextClip(context);
            }
            CGContextConcatCTM(context, area.inverse);
            CGContextSetInterpolationQuality(context, kCGInterpolationNone);
            CGContextDrawImage(context, CGRectMake(area.left, area.bottom, area.columns, area.rows), image);
            CGContextRestoreGState(context);
            CGImageRelease(image);
        }
    }
    CGContextBeginPath(context);
    CGContextAddPath(context, path);
    CGPathRelease(path);
}

// **********************************************************************************************

extern "C" void
brixGraphicsDrawRect(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    bool fill,
    bool stroke
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return;
    }

    CGContextBeginPath(canvas->context);
    CGContextAddRect(canvas->context, CGRectMake(x, y, width, height));
    drawCurrentPath(canvas, fill, stroke, false);
}

extern "C" void
brixGraphicsDrawEllipse(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    bool fill,
    bool stroke
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return;
    }

    CGContextBeginPath(canvas->context);
    CGContextAddEllipseInRect(canvas->context, CGRectMake(x, y, width, height));
    drawCurrentPath(canvas, fill, stroke, false);
}

static void
addOutlinePath(
    CGMutablePathRef path,
    CGRect rect,
    double radius,
    int32_t side,
    double offset,
    double size
) {
    const double x = CGRectGetMinX(rect);
    const double y = CGRectGetMinY(rect);
    const double right = CGRectGetMaxX(rect);
    const double bottom = CGRectGetMaxY(rect);
    const double r = std::max(0.0, std::min(radius, std::min(rect.size.width, rect.size.height) / 2));
    const bool hasArrow = side >= 0 && side <= 3 && size > 0;
    const double span = side <= 1 ? rect.size.width : rect.size.height;
    double half = size;
    double low = r + half;
    double high = span - r - half;
    double tip = offset;
    if (high < low) {
        half = std::max(0.0, span / 2 - r);
        tip = span / 2;
    } else {
        tip = std::min(std::max(offset, low), high);
    }
    const bool drawsArrow = hasArrow && half > 0;
    CGPathMoveToPoint(path, nullptr, x + r, y);
    if (drawsArrow && side == 0) {
        CGPathAddLineToPoint(path, nullptr, x + tip - half, y);
        CGPathAddLineToPoint(path, nullptr, x + tip, y - size);
        CGPathAddLineToPoint(path, nullptr, x + tip + half, y);
    }
    CGPathAddLineToPoint(path, nullptr, right - r, y);
    if (r > 0) {
        CGPathAddArcToPoint(path, nullptr, right, y, right, y + r, r);
    }
    if (drawsArrow && side == 3) {
        CGPathAddLineToPoint(path, nullptr, right, y + tip - half);
        CGPathAddLineToPoint(path, nullptr, right + size, y + tip);
        CGPathAddLineToPoint(path, nullptr, right, y + tip + half);
    }
    CGPathAddLineToPoint(path, nullptr, right, bottom - r);
    if (r > 0) {
        CGPathAddArcToPoint(path, nullptr, right, bottom, right - r, bottom, r);
    }
    if (drawsArrow && side == 1) {
        CGPathAddLineToPoint(path, nullptr, x + tip + half, bottom);
        CGPathAddLineToPoint(path, nullptr, x + tip, bottom + size);
        CGPathAddLineToPoint(path, nullptr, x + tip - half, bottom);
    }
    CGPathAddLineToPoint(path, nullptr, x + r, bottom);
    if (r > 0) {
        CGPathAddArcToPoint(path, nullptr, x, bottom, x, bottom - r, r);
    }
    if (drawsArrow && side == 2) {
        CGPathAddLineToPoint(path, nullptr, x, y + tip + half);
        CGPathAddLineToPoint(path, nullptr, x - size, y + tip);
        CGPathAddLineToPoint(path, nullptr, x, y + tip - half);
    }
    CGPathAddLineToPoint(path, nullptr, x, y + r);
    if (r > 0) {
        CGPathAddArcToPoint(path, nullptr, x, y, x + r, y, r);
    }
    CGPathCloseSubpath(path);
}

static CGColorRef
createCanvasColor(BxGraphicsCanvas* canvas, double red, double green, double blue, double alpha) {
    if (canvas->preservesSRGBColors || !canvas->colorSpace ||
        CGColorSpaceGetNumberOfComponents(canvas->colorSpace) != 3) {
        return createSRGBColor(red, green, blue, alpha);
    }
    const CGFloat components[] = {
        static_cast<CGFloat>(red),
        static_cast<CGFloat>(green),
        static_cast<CGFloat>(blue),
        static_cast<CGFloat>(alpha)
    };
    return CGColorCreate(canvas->colorSpace, components);
}

static void
drawPathShadow(
    BxGraphicsCanvas* canvas,
    CGPathRef shape,
    CGPathRef exclusion,
    double red,
    double green,
    double blue,
    double alpha,
    double blur,
    double offsetX,
    double offsetY
) {
    CGContextRef context = canvas->context;
    const CGRect shapeBounds = CGPathGetBoundingBox(shape);
    if (CGRectIsEmpty(shapeBounds) || CGRectIsNull(shapeBounds) || alpha <= 0) {
        return;
    }
    const double safeBlur = std::max(0.0, blur);
    const double reach = safeBlur * 2 + 2;
    CGRect area = CGRectInset(CGRectOffset(shapeBounds, offsetX, offsetY), -reach, -reach);
    if (exclusion) {
        area = CGRectUnion(area, CGPathGetBoundingBox(exclusion));
    }
    const CGAffineTransform device = CGContextGetUserSpaceToDeviceSpaceTransform(context);
    const double determinant = device.a * device.d - device.b * device.c;
    if (!(std::fabs(determinant) > 0)) {
        return;
    }
    const double deviceScale = std::sqrt(std::fabs(determinant));
    const double distance = std::ceil(
        CGRectGetMaxX(CGRectApplyAffineTransform(shapeBounds, device)) -
        CGRectGetMinX(CGRectApplyAffineTransform(area, device)) + safeBlur * deviceScale * 2 + 16
    );
    const CGSize away = CGSizeMake(-distance * device.d / determinant, distance * device.b / determinant);
    const CGSize deviceOffset = CGSizeApplyAffineTransform(CGSizeMake(offsetX, offsetY), device);
    const CGSize shadowOffset = CGSizeMake(distance + deviceOffset.width, -deviceOffset.height);
    CGColorRef color = createCanvasColor(canvas, red, green, blue, alpha);
    if (!color) {
        return;
    }
    invalidateUserRect(canvas, area);
    CGContextSaveGState(context);
    CGContextBeginPath(context);
    CGContextAddRect(context, area);
    if (exclusion) {
        CGContextAddPath(context, exclusion);
    }
    CGContextEOClip(context);
    CGContextSetShadowWithColor(context, shadowOffset, safeBlur * deviceScale, color);
    CGContextSetRGBFillColor(context, 0, 0, 0, 1);
    CGContextTranslateCTM(context, away.width, away.height);
    CGContextBeginPath(context);
    CGContextAddPath(context, shape);
    CGContextFillPath(context);
    CGContextRestoreGState(context);
    CGColorRelease(color);
}

extern "C" void
brixGraphicsDrawOutline(
    void* ptr,
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
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || width <= 0 || height <= 0) {
        return;
    }
    CGContextBeginPath(canvas->context);
    if (radius <= 0 && (side < 0 || size <= 0)) {
        CGContextAddRect(canvas->context, CGRectMake(x, y, width, height));
    } else {
        CGMutablePathRef path = CGPathCreateMutable();
        addOutlinePath(path, CGRectMake(x, y, width, height), radius, side, offset, size);
        CGContextAddPath(canvas->context, path);
        CGPathRelease(path);
    }
    drawCurrentPath(canvas, fill, stroke, false);
}

extern "C" void
brixGraphicsClipOutline(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    double radius,
    int32_t side,
    double offset,
    double size
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return;
    }
    ++canvas->drawState.compositeSerial;
    if (width <= 0 || height <= 0) {
        CGContextClipToRect(canvas->context, CGRectZero);
        return;
    }
    if (radius <= 0 && (side < 0 || size <= 0)) {
        CGContextClipToRect(canvas->context, CGRectMake(x, y, width, height));
        return;
    }
    CGMutablePathRef path = CGPathCreateMutable();
    addOutlinePath(path, CGRectMake(x, y, width, height), radius, side, offset, size);
    CGContextBeginPath(canvas->context);
    CGContextAddPath(canvas->context, path);
    CGContextClip(canvas->context);
    CGPathRelease(path);
}

extern "C" void
brixGraphicsDrawOutlineShadow(
    void* ptr,
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
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || width <= 0 || height <= 0) {
        return;
    }
    const CGRect bounds = CGRectMake(x, y, width, height);
    CGRect spreadBounds = CGRectInset(bounds, -spread, -spread);
    if (spreadBounds.size.width <= 0 || spreadBounds.size.height <= 0) {
        return;
    }
    const double spreadRadius = radius > 0 ? std::max(0.0, radius + spread) : 0;
    CGMutablePathRef shape = CGPathCreateMutable();
    addOutlinePath(
        shape,
        spreadBounds,
        spreadRadius,
        side,
        offset + spread,
        std::max(0.0, size + spread * 0.41421356237309515)
    );
    CGMutablePathRef exclusion = CGPathCreateMutable();
    addOutlinePath(exclusion, bounds, radius, side, offset, size);
    drawPathShadow(canvas, shape, exclusion, red, green, blue, alpha, blur, offsetX, offsetY);
    CGPathRelease(exclusion);
    CGPathRelease(shape);
}

extern "C" void
brixGraphicsDrawPathShadow(
    void* ptr,
    double red,
    double green,
    double blue,
    double alpha,
    double blur,
    double offsetX,
    double offsetY
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return;
    }
    CGPathRef shape = CGContextCopyPath(canvas->context);
    CGContextBeginPath(canvas->context);
    if (!shape) {
        return;
    }
    drawPathShadow(canvas, shape, shape, red, green, blue, alpha, blur, offsetX, offsetY);
    CGPathRelease(shape);
}

extern "C" void
brixGraphicsDrawImage(
    void* canvasPtr,
    void* imageCanvasPtr,
    double x,
    double y,
    double width,
    double height,
    bool templateAlpha,
    int32_t sampling
) {
    BxGraphicsCanvas* canvas = getCanvas(canvasPtr);
    BxGraphicsCanvas* imageCanvas = getCanvas(imageCanvasPtr);
    if (!canvas || !canvas->context || width <= 0 || height <= 0) {
        return;
    }
    CGImageRef image = templateAlpha ? getMaskSnapshot(imageCanvas) : nullptr;
    if (!image) {
        image = getSnapshot(imageCanvas);
    }
    if (!image) {
        return;
    }

    const bool retainedForSelfDraw = canvas == imageCanvas;
    if (retainedForSelfDraw) {
        CGImageRetain(image);
    }
    invalidateUserRect(canvas, CGRectMake(x, y, width, height));
    CGContextSaveGState(canvas->context);
    CGContextTranslateCTM(canvas->context, x, y + height);
    CGContextScaleCTM(canvas->context, 1, -1);
    if (sampling == BxGraphicsSamplingPixelated) {
        CGContextSetInterpolationQuality(canvas->context, kCGInterpolationNone);
    }
    CGRect rect = CGRectMake(0, 0, width, height);
    if (templateAlpha) {
        CGContextClipToMask(canvas->context, rect, image);
        CGContextFillRect(canvas->context, rect);
    } else {
        CGContextDrawImage(canvas->context, rect, image);
    }
    CGContextRestoreGState(canvas->context);
    if (retainedForSelfDraw) {
        CGImageRelease(image);
    }
}

extern "C" void
brixGraphicsFillImage(
    void* canvasPtr,
    void* imageCanvasPtr,
    double x,
    double y,
    double width,
    double height,
    double tileX,
    double tileY,
    double tileWidth,
    double tileHeight,
    int32_t sampling
) {
    BxGraphicsCanvas* canvas = getCanvas(canvasPtr);
    BxGraphicsCanvas* imageCanvas = getCanvas(imageCanvasPtr);
    if (!canvas || !canvas->context || width <= 0 || height <= 0 || tileWidth <= 0 || tileHeight <= 0) {
        return;
    }
    CGImageRef image = getSnapshot(imageCanvas);
    if (!image) {
        return;
    }

    const bool retainedForSelfDraw = canvas == imageCanvas;
    if (retainedForSelfDraw) {
        CGImageRetain(image);
    }
    invalidateUserRect(canvas, CGRectMake(x, y, width, height));
    CGContextSaveGState(canvas->context);
    CGContextClipToRect(canvas->context, CGRectMake(x, y, width, height));
    CGContextTranslateCTM(canvas->context, tileX, tileY + tileHeight);
    CGContextScaleCTM(canvas->context, 1, -1);
    if (sampling == BxGraphicsSamplingPixelated) {
        CGContextSetInterpolationQuality(canvas->context, kCGInterpolationNone);
    }
    CGContextDrawTiledImage(canvas->context, CGRectMake(0, 0, tileWidth, tileHeight), image);
    CGContextRestoreGState(canvas->context);
    if (retainedForSelfDraw) {
        CGImageRelease(image);
    }
}

extern "C" void
brixGraphicsClipMask(
    void* canvasPtr,
    void* imageCanvasPtr,
    double x,
    double y,
    double width,
    double height
) {
    BxGraphicsCanvas* canvas = getCanvas(canvasPtr);
    if (!canvas || !canvas->context) {
        return;
    }
    ++canvas->drawState.compositeSerial;
    CGImageRef image = getMaskSnapshot(getCanvas(imageCanvasPtr));
    if (!image || width <= 0 || height <= 0) {
        CGContextClipToRect(canvas->context, CGRectZero);
        return;
    }
    CGImageRetain(image);
    const CGAffineTransform flip = CGAffineTransformMake(1, 0, 0, -1, x, y + height);
    CGContextConcatCTM(canvas->context, flip);
    CGContextClipToMask(canvas->context, CGRectMake(0, 0, width, height), image);
    CGContextConcatCTM(canvas->context, CGAffineTransformInvert(flip));
    CGImageRelease(image);
}

extern "C" void*
brixGraphicsBeginMask(void* ptr) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return nullptr;
    }
    const CGAffineTransform transform = CGContextGetCTM(canvas->context);
    CGRect bounds = CGRectIntegral(CGRectApplyAffineTransform(
        CGContextGetClipBoundingBox(canvas->context),
        transform
    ));
    bounds = CGRectIntersection(bounds, CGRectMake(0, 0, canvas->pixelWidth, canvas->pixelHeight));
    if (CGRectIsNull(bounds) || CGRectIsEmpty(bounds)) {
        bounds = CGRectMake(0, 0, 1, 1);
    }
    BxGraphicsCanvas* mask = static_cast<BxGraphicsCanvas*>(calloc(1, sizeof(BxGraphicsCanvas)));
    if (!mask) {
        return nullptr;
    }
    mask->pixelWidth = static_cast<int32_t>(bounds.size.width);
    mask->pixelHeight = static_cast<int32_t>(bounds.size.height);
    mask->width = mask->pixelWidth;
    mask->height = mask->pixelHeight;
    mask->scale = canvas->scale;
    mask->contentSerial = nextContentSerial();
    mask->maskOrigin[0] = bounds.origin.x;
    mask->maskOrigin[1] = bounds.origin.y;
    placeGroup(mask, canvas, bounds, BxGraphicsGroupMask);
    mask->bytesPerPixel = 1;
    mask->bytesPerRow = static_cast<size_t>(mask->pixelWidth);
    mask->pixels = calloc(static_cast<size_t>(mask->pixelHeight), mask->bytesPerRow);
    if (mask->pixels) {
        mask->context = CGBitmapContextCreate(
            mask->pixels,
            static_cast<size_t>(mask->pixelWidth),
            static_cast<size_t>(mask->pixelHeight),
            8,
            mask->bytesPerRow,
            nullptr,
            kCGImageAlphaOnly
        );
    }
    if (!mask->context) {
        releaseCanvas(mask);
        free(mask);
        return nullptr;
    }
    CGContextSetAllowsAntialiasing(mask->context, true);
    CGContextSetShouldAntialias(mask->context, true);
    CGContextConcatCTM(
        mask->context,
        CGAffineTransformConcat(
            transform,
            CGAffineTransformMakeTranslation(-bounds.origin.x, -bounds.origin.y)
        )
    );
    CGContextSaveGState(mask->context);
    return mask;
}

extern "C" void
brixGraphicsEndMask(void* ptr, void* maskPtr) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    BxGraphicsCanvas* mask = getCanvas(maskPtr);
    if (!canvas || !canvas->context) {
        releaseCanvas(mask);
        free(mask);
        return;
    }
    ++canvas->drawState.compositeSerial;
    if (!mask || !mask->context) {
        CGContextClipToRect(canvas->context, CGRectZero);
        releaseCanvas(mask);
        free(mask);
        return;
    }
    CGContextRelease(mask->context);
    mask->context = nullptr;
    const size_t width = static_cast<size_t>(mask->pixelWidth);
    const size_t height = static_cast<size_t>(mask->pixelHeight);
    void* bytes = mask->pixels;
    mask->pixels = nullptr;
    CGDataProviderRef provider =
        CGDataProviderCreateWithData(nullptr, bytes, width * height, releaseMaskBytes);
    CGImageRef image = provider
        ? CGImageCreate(
            width,
            height,
            8,
            8,
            width,
            getDeviceGrayColorSpace(),
            kCGImageAlphaNone,
            provider,
            nullptr,
            false,
            kCGRenderingIntentDefault
        )
        : nullptr;
    if (provider) {
        CGDataProviderRelease(provider);
    } else {
        free(bytes);
    }
    if (image) {
        const CGAffineTransform transform = CGContextGetCTM(canvas->context);
        CGContextConcatCTM(canvas->context, CGAffineTransformInvert(transform));
        CGContextClipToMask(
            canvas->context,
            CGRectMake(mask->maskOrigin[0], mask->maskOrigin[1], width, height),
            image
        );
        CGContextConcatCTM(canvas->context, transform);
        CGImageRelease(image);
    } else {
        CGContextClipToRect(canvas->context, CGRectZero);
    }
    releaseCanvas(mask);
    free(mask);
}

static const int32_t kMaxLayerBlur = 256;

static uint8_t
quantizeBlurChannel(float value) {
    return static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, std::floor(value + 0.5f))));
}

static uint8_t
blurElement(float value, uint8_t*) {
    return quantizeBlurChannel(value);
}

static _Float16
blurElement(float value, _Float16*) {
    return static_cast<_Float16>(value);
}

static const size_t kBlurBandRows = 16;
static const size_t kBlurParallelPixels = 65536;

template <typename Element>
struct BlurLayerPass {
    uint8_t* pixels;
    size_t width;
    size_t height;
    size_t bytesPerRow;
    int32_t radius;
    const float* weights;
    float total;
    Element* scratch;
    size_t scratchRow;
};

template <typename Element, size_t Channels>
static void
blurLayerRows(void* context, size_t band) {
    const BlurLayerPass<Element>& pass = *static_cast<const BlurLayerPass<Element>*>(context);
    const int64_t columns = static_cast<int64_t>(pass.width);
    const size_t first = band * kBlurBandRows;
    const size_t last = std::min(pass.height, first + kBlurBandRows);
    for (size_t y = first; y < last; ++y) {
        const Element* row = reinterpret_cast<const Element*>(pass.pixels + y * pass.bytesPerRow);
        Element* out = pass.scratch + y * pass.scratchRow;
        for (int64_t x = 0; x < columns; ++x) {
            float sum[Channels] = {};
            const int64_t from = std::max<int64_t>(-pass.radius, -x);
            const int64_t to = std::min<int64_t>(pass.radius, columns - 1 - x);
            for (int64_t tap = from; tap <= to; ++tap) {
                const Element* texel = row + static_cast<size_t>(x + tap) * Channels;
                const float weight = pass.weights[static_cast<size_t>(tap + pass.radius)];
                for (size_t channel = 0; channel < Channels; ++channel) {
                    sum[channel] += weight * static_cast<float>(texel[channel]);
                }
            }
            for (size_t channel = 0; channel < Channels; ++channel) {
                out[static_cast<size_t>(x) * Channels + channel] = blurElement(sum[channel] / pass.total, static_cast<Element*>(nullptr));
            }
        }
    }
}

template <typename Element, size_t Channels>
static void
blurLayerColumns(void* context, size_t band) {
    const BlurLayerPass<Element>& pass = *static_cast<const BlurLayerPass<Element>*>(context);
    const int64_t rows = static_cast<int64_t>(pass.height);
    const size_t first = band * kBlurBandRows;
    const size_t last = std::min(pass.height, first + kBlurBandRows);
    for (size_t y = first; y < last; ++y) {
        Element* out = reinterpret_cast<Element*>(pass.pixels + y * pass.bytesPerRow);
        const int64_t from = std::max<int64_t>(-pass.radius, -static_cast<int64_t>(y));
        const int64_t to = std::min<int64_t>(pass.radius, rows - 1 - static_cast<int64_t>(y));
        for (size_t x = 0; x < pass.width; ++x) {
            float sum[Channels] = {};
            for (int64_t tap = from; tap <= to; ++tap) {
                const Element* texel = pass.scratch + static_cast<size_t>(static_cast<int64_t>(y) + tap) * pass.scratchRow +
                    x * Channels;
                const float weight = pass.weights[static_cast<size_t>(tap + pass.radius)];
                for (size_t channel = 0; channel < Channels; ++channel) {
                    sum[channel] += weight * static_cast<float>(texel[channel]);
                }
            }
            for (size_t channel = 0; channel < Channels; ++channel) {
                out[x * Channels + channel] = blurElement(sum[channel] / pass.total, static_cast<Element*>(nullptr));
            }
        }
    }
}

template <typename Element>
static void
runBlurLayerPasses(BlurLayerPass<Element>* pass, void (*rows)(void*, size_t), void (*columns)(void*, size_t)) {
    const size_t bands = (pass->height + kBlurBandRows - 1) / kBlurBandRows;
    if (pass->width * pass->height < kBlurParallelPixels) {
        for (size_t band = 0; band < bands; ++band) {
            rows(pass, band);
        }
        for (size_t band = 0; band < bands; ++band) {
            columns(pass, band);
        }
        return;
    }
    dispatch_apply_f(bands, DISPATCH_APPLY_AUTO, pass, rows);
    dispatch_apply_f(bands, DISPATCH_APPLY_AUTO, pass, columns);
}

template <typename Element>
static void
blurLayerElements(
    uint8_t* pixels,
    size_t width,
    size_t height,
    size_t bytesPerRow,
    size_t channels,
    int32_t radius
) {
    if (radius <= 0 || !width || !height) {
        return;
    }
    const float sigma = std::max(static_cast<float>(radius) * 0.5f, 0.5f);
    const float falloff = -0.5f / (sigma * sigma);
    std::vector<float> weights(static_cast<size_t>(radius) * 2 + 1);
    float total = 0;
    for (int32_t tap = -radius; tap <= radius; ++tap) {
        const float weight = std::exp(static_cast<float>(tap * tap) * falloff);
        weights[static_cast<size_t>(tap + radius)] = weight;
        total += weight;
    }
    const size_t scratchRow = width * channels;
    std::vector<Element> scratch(height * scratchRow);
    BlurLayerPass<Element> pass = {pixels, width, height, bytesPerRow, radius, weights.data(), total, scratch.data(), scratchRow};
    if (channels == 4) {
        runBlurLayerPasses(&pass, blurLayerRows<Element, 4>, blurLayerColumns<Element, 4>);
    } else if (channels == 1) {
        runBlurLayerPasses(&pass, blurLayerRows<Element, 1>, blurLayerColumns<Element, 1>);
    }
}

static void*
beginGroupLayer(BxGraphicsCanvas* canvas, double radius, int32_t kind, const CGRect* region = nullptr) {
    const bool alphaOnly = canvas && canvas->bytesPerPixel == 1;
    if (!canvas || !canvas->context || (!alphaOnly && !canvas->colorSpace)) {
        return nullptr;
    }
    const CGAffineTransform transform = CGContextGetCTM(canvas->context);
    const double determinant = transform.a * transform.d - transform.b * transform.c;
    const double spread = std::isfinite(radius) && radius > 0
        ? std::ceil(radius * std::sqrt(std::fabs(determinant)))
        : 0;
    const int32_t blur = static_cast<int32_t>(std::min<double>(spread, kMaxLayerBlur));
    CGRect bounds = CGRectIntegral(CGRectApplyAffineTransform(
        region ? *region : CGContextGetClipBoundingBox(canvas->context),
        transform
    ));
    bounds = CGRectInset(bounds, -blur, -blur);
    bounds = CGRectIntersection(bounds, CGRectMake(0, 0, canvas->pixelWidth, canvas->pixelHeight));
    if (CGRectIsNull(bounds) || CGRectIsEmpty(bounds)) {
        bounds = CGRectMake(0, 0, 1, 1);
    }
    BxGraphicsCanvas* layer = static_cast<BxGraphicsCanvas*>(calloc(1, sizeof(BxGraphicsCanvas)));
    if (!layer) {
        return nullptr;
    }
    layer->pixelWidth = static_cast<int32_t>(bounds.size.width);
    layer->pixelHeight = static_cast<int32_t>(bounds.size.height);
    layer->width = layer->pixelWidth;
    layer->height = layer->pixelHeight;
    layer->scale = canvas->scale;
    layer->contentSerial = nextContentSerial();
    layer->maskOrigin[0] = bounds.origin.x;
    layer->maskOrigin[1] = bounds.origin.y;
    layer->blurRadius = blur;
    placeGroup(layer, canvas, bounds, kind);
    layer->format = canvas->format;
    layer->space = canvas->space;
    layer->namedSpace = canvas->namedSpace;
    layer->bytesPerPixel = canvas->bytesPerPixel;
    layer->bytesPerRow = static_cast<size_t>(layer->pixelWidth) * layer->bytesPerPixel;
    layer->pixels = calloc(static_cast<size_t>(layer->pixelHeight), layer->bytesPerRow);
    layer->colorSpace = alphaOnly ? nullptr : CGColorSpaceRetain(canvas->colorSpace);
    layer->preservesSRGBColors = canvas->preservesSRGBColors;
    if (layer->pixels) {
        layer->context = CGBitmapContextCreate(
            layer->pixels,
            static_cast<size_t>(layer->pixelWidth),
            static_cast<size_t>(layer->pixelHeight),
            canvasBitsPerComponent(layer->format, alphaOnly),
            layer->bytesPerRow,
            layer->colorSpace,
            canvasBitmapInfo(layer->format, alphaOnly)
        );
    }
    if (!layer->context) {
        releaseCanvas(layer);
        free(layer);
        return nullptr;
    }
    CGContextSetAllowsAntialiasing(layer->context, true);
    CGContextSetShouldAntialias(layer->context, true);
    CGContextConcatCTM(
        layer->context,
        CGAffineTransformConcat(
            transform,
            CGAffineTransformMakeTranslation(-bounds.origin.x, -bounds.origin.y)
        )
    );
    if (region) {
        CGContextClipToRect(layer->context, *region);
    }
    CGContextSaveGState(layer->context);
    return layer;
}

static size_t
firstInkByte(const uint8_t* line, size_t bytes) {
    size_t index = 0;
    while (index + 8 <= bytes) {
        uint64_t word;
        memcpy(&word, line + index, 8);
        if (word) {
            break;
        }
        index += 8;
    }
    while (index < bytes && !line[index]) {
        ++index;
    }
    return index;
}

static bool
groupIsBlank(const BxGraphicsCanvas* group) {
    const uint8_t* pixels = static_cast<const uint8_t*>(group->pixels);
    if (!pixels) {
        return true;
    }
    const size_t bytes = static_cast<size_t>(group->pixelWidth) * group->bytesPerPixel;
    for (int32_t row = 0; row < group->pixelHeight; ++row) {
        if (firstInkByte(pixels + static_cast<size_t>(row) * group->bytesPerRow, bytes) < bytes) {
            return false;
        }
    }
    return true;
}

static void
endGroupLayer(BxGraphicsCanvas* canvas, BxGraphicsCanvas* layer, double alpha, int32_t blendMode) {
    if (!canvas || !canvas->context || !layer || !layer->context) {
        releaseCanvas(layer);
        free(layer);
        return;
    }
    const size_t width = static_cast<size_t>(layer->pixelWidth);
    const size_t height = static_cast<size_t>(layer->pixelHeight);
    const size_t stride = layer->bytesPerPixel;
    uint8_t* pixels = static_cast<uint8_t*>(layer->pixels);
    size_t left = width;
    size_t right = 0;
    size_t top = height;
    size_t bottom = 0;
    for (size_t row = 0; row < height; ++row) {
        const uint8_t* line = pixels + row * layer->bytesPerRow;
        const size_t bytes = width * stride;
        const size_t first = firstInkByte(line, bytes);
        if (first == bytes) {
            continue;
        }
        size_t last = bytes;
        while (last > first && !line[last - 1]) {
            --last;
        }
        left = std::min(left, first / stride);
        right = std::max(right, (last - 1) / stride + 1);
        top = std::min(top, row);
        bottom = row + 1;
    }
    if (left < right && top < bottom) {
        const size_t spread = static_cast<size_t>(std::max(layer->blurRadius, 0));
        left = left > spread ? left - spread : 0;
        top = top > spread ? top - spread : 0;
        right = std::min(width, right + spread);
        bottom = std::min(height, bottom + spread);
        if (layer->format == BxGraphicsFormatRGBA16Float) {
            blurLayerElements<_Float16>(
                pixels + top * layer->bytesPerRow + left * stride,
                right - left,
                bottom - top,
                layer->bytesPerRow,
                4,
                layer->blurRadius
            );
        } else {
            blurLayerElements<uint8_t>(
                pixels + top * layer->bytesPerRow + left * stride,
                right - left,
                bottom - top,
                layer->bytesPerRow,
                stride,
                layer->blurRadius
            );
        }
    }
    if (isUnboundedBlend(blendMode)) {
        left = 0;
        top = 0;
        right = width;
        bottom = height;
    }
    if (left < right && top < bottom) {
        const size_t regionWidth = right - left;
        const size_t regionHeight = bottom - top;
        uint8_t* origin = pixels + top * layer->bytesPerRow + left * stride;
        const size_t regionRow = regionWidth * stride;
        uint8_t* copy = static_cast<uint8_t*>(malloc(regionRow * regionHeight));
        if (copy) {
            for (size_t row = 0; row < regionHeight; ++row) {
                memcpy(copy + row * regionRow, origin + row * layer->bytesPerRow, regionRow);
            }
        }
        CGContextRef region = copy
            ? CGBitmapContextCreate(
                copy,
                regionWidth,
                regionHeight,
                8,
                regionRow,
                layer->colorSpace,
                stride == 1
                    ? static_cast<uint32_t>(kCGImageAlphaOnly)
                    : kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
            )
            : nullptr;
        CGImageRef image = region ? CGBitmapContextCreateImage(region) : nullptr;
        if (image) {
            invalidateDeviceRect(
                canvas,
                CGRectMake(
                    layer->maskOrigin[0] + static_cast<double>(left),
                    layer->maskOrigin[1] + static_cast<double>(height - bottom),
                    static_cast<double>(regionWidth),
                    static_cast<double>(regionHeight)
                )
            );
            CGContextSaveGState(canvas->context);
            CGContextSetAlpha(canvas->context, std::isfinite(alpha) ? alpha : 1);
            CGContextConcatCTM(canvas->context, CGAffineTransformInvert(CGContextGetCTM(canvas->context)));
            CGContextSetInterpolationQuality(canvas->context, kCGInterpolationNone);
            CGContextDrawImage(
                canvas->context,
                CGRectMake(
                    layer->maskOrigin[0] + static_cast<double>(left),
                    layer->maskOrigin[1] + static_cast<double>(height - bottom),
                    static_cast<double>(regionWidth),
                    static_cast<double>(regionHeight)
                ),
                image
            );
            CGContextRestoreGState(canvas->context);
            CGImageRelease(image);
        }
        if (region) {
            CGContextRelease(region);
        }
        free(copy);
    }
    releaseCanvas(layer);
    free(layer);
}

extern "C" void*
brixGraphicsBeginBlur(void* ptr, double radius) {
    return beginGroupLayer(getCanvas(ptr), radius, BxGraphicsGroupBlur);
}

extern "C" void
brixGraphicsEndBlur(void* ptr, void* layerPtr, int32_t blendMode) {
    endGroupLayer(getCanvas(ptr), getCanvas(layerPtr), 1, blendMode);
}


extern "C" void*
brixGraphicsBeginFilter(void* ptr, double x, double y, double width, double height) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context) {
        return nullptr;
    }
    const CGRect region = CGRectMake(x, y, std::max(width, 0.0), std::max(height, 0.0));
    const CGAffineTransform transform = CGContextGetCTM(canvas->context);
    BxGraphicsCanvas* layer = static_cast<BxGraphicsCanvas*>(
        beginGroupLayer(canvas, 0, BxGraphicsGroupFilter, &region)
    );
    if (layer) {
        const CGRect device = CGRectApplyAffineTransform(region, transform);
        layer->filterRegion[0] = CGRectGetMinX(device);
        layer->filterRegion[1] = CGRectGetMinY(device);
        layer->filterRegion[2] = CGRectGetMaxX(device);
        layer->filterRegion[3] = CGRectGetMaxY(device);
        layer->filterScale = std::sqrt(std::fabs(transform.a * transform.d - transform.b * transform.c));
    }
    return layer;
}

extern "C" void
brixGraphicsEndFilter(
    void* ptr,
    void* layerPtr,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    void* const* images,
    const double* imageScales,
    int64_t imageCount,
    double time,
    double opacity,
    int32_t blendMode
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    BxGraphicsCanvas* layer = getCanvas(layerPtr);
    void* textures[bxGraphicsShaderImageLimit] = {};
    double frames[bxGraphicsShaderImageLimit * 4] = {};
    const bool inputs = imageCount >= 0 && imageCount <= bxGraphicsShaderImageLimit &&
        (imageCount == 0 || (images && imageScales)) &&
        canvasGPUInputs(images, imageScales, imageCount, textures, frames);
    if (inputs && layer && layer->context && layer->bytesPerPixel != 1 && layer->filterScale > 0 && (args || !argCount)) {
        CGContextFlush(layer->context);
        const double scale = layer->filterScale;
        const double origin[2] = {
            layer->filterRegion[0] - layer->maskOrigin[0],
            layer->maskOrigin[1] + layer->pixelHeight - layer->filterRegion[3],
        };
        const double size[2] = {
            (layer->filterRegion[2] - layer->filterRegion[0]) / scale,
            (layer->filterRegion[3] - layer->filterRegion[1]) / scale,
        };
        bxGraphicsShaderFilter(
            shader,
            static_cast<uint8_t*>(layer->pixels),
            layer->pixelWidth,
            layer->pixelHeight,
            layer->bytesPerRow,
            layer->format,
            layer->space,
            origin,
            size,
            scale,
            args,
            argCount,
            textures,
            frames,
            static_cast<int32_t>(imageCount),
            time
        );
    }
    endGroupLayer(canvas, layer, opacity, blendMode);
}

extern "C" void*
brixGraphicsBeginOpacity(void* ptr) {
    return beginGroupLayer(getCanvas(ptr), 0, BxGraphicsGroupOpacity);
}

extern "C" void
brixGraphicsEndOpacity(void* ptr, void* layerPtr, double opacity, int32_t blendMode) {
    endGroupLayer(getCanvas(ptr), getCanvas(layerPtr), opacity, blendMode);
}

static const int32_t kMaxBackdropBlur = 512;

struct BackdropPlan {
    int32_t level;
    int32_t factor;
    int32_t radius;
    int64_t firstX;
    int64_t firstY;
    int64_t columns;
    int64_t rows;
    std::vector<float> weights;
};

static BackdropPlan
planBackdrop(int32_t blur, int64_t left, int64_t top, int64_t width, int64_t height) {
    BackdropPlan plan = {};
    const double sigma = static_cast<double>(blur) * 0.5;
    while (sigma / static_cast<double>(1 << plan.level) > 4) {
        ++plan.level;
    }
    plan.factor = 1 << plan.level;
    const double sigmaLevel = sigma / plan.factor;
    plan.radius = sigmaLevel > 0 ? static_cast<int32_t>(std::ceil(3 * sigmaLevel)) : 0;
    const double factor = plan.factor;
    const int64_t minX = static_cast<int64_t>(std::floor((static_cast<double>(left) + 0.5) / factor - 0.5));
    const int64_t maxX = static_cast<int64_t>(
        std::floor((static_cast<double>(left + width - 1) + 0.5) / factor - 0.5)
    ) + 1;
    const int64_t minY = static_cast<int64_t>(std::floor((static_cast<double>(top) + 0.5) / factor - 0.5));
    const int64_t maxY = static_cast<int64_t>(
        std::floor((static_cast<double>(top + height - 1) + 0.5) / factor - 0.5)
    ) + 1;
    plan.firstX = minX - plan.radius;
    plan.firstY = minY - plan.radius;
    plan.columns = maxX - minX + 1 + 2 * plan.radius;
    plan.rows = maxY - minY + 1 + 2 * plan.radius;
    plan.weights.resize(static_cast<size_t>(plan.radius) * 2 + 1);
    float total = 0;
    for (int32_t tap = -plan.radius; tap <= plan.radius; ++tap) {
        const float weight = plan.radius
            ? std::exp(-static_cast<float>(tap * tap) / static_cast<float>(2 * sigmaLevel * sigmaLevel))
            : 1.0f;
        plan.weights[static_cast<size_t>(tap + plan.radius)] = weight;
        total += weight;
    }
    for (float& weight : plan.weights) {
        weight /= total;
    }
    return plan;
}

static int64_t
mirrorBackdropIndex(int64_t value, int64_t start, int64_t length) {
    const int64_t period = length * 2;
    int64_t offset = (value - start) % period;
    if (offset < 0) {
        offset += period;
    }
    return start + (offset < length ? offset : period - 1 - offset);
}

static void
backdropTexel(
    const BxGraphicsCanvas* source,
    int64_t x,
    int64_t y,
    int64_t left,
    int64_t top,
    int64_t columns,
    int64_t rows,
    float* out
) {
    x = mirrorBackdropIndex(x, left, columns);
    y = mirrorBackdropIndex(y, top, rows);
    const int64_t localX = x - source->absoluteLeft;
    const int64_t localY = y - source->absoluteTop;
    if (localX < 0 || localY < 0 || localX >= source->pixelWidth || localY >= source->pixelHeight) {
        out[0] = out[1] = out[2] = out[3] = 0;
        return;
    }
    const uint8_t* texel = static_cast<const uint8_t*>(source->pixels) +
        static_cast<size_t>(localY) * source->bytesPerRow + static_cast<size_t>(localX) * 4;
    for (int channel = 0; channel < 4; ++channel) {
        out[channel] = static_cast<float>(texel[channel]);
    }
}

static uint8_t*
blurBackdropPixels(
    const BxGraphicsCanvas* source,
    int64_t left,
    int64_t top,
    int64_t columns,
    int64_t rows,
    int32_t blur
) {
    const BackdropPlan plan = planBackdrop(blur, left, top, columns, rows);

    const int64_t expand = int64_t(1) << (plan.level > 0 ? plan.level - 1 : 0);
    int64_t levelColumns = plan.columns * expand;
    int64_t levelRows = plan.rows * expand;
    const int64_t originX = plan.firstX * expand;
    const int64_t originY = plan.firstY * expand;
    const int64_t gather = plan.level > 0 ? 2 : 1;
    std::vector<float> level(static_cast<size_t>(levelColumns * levelRows) * 4);
    for (int64_t row = 0; row < levelRows; ++row) {
        for (int64_t column = 0; column < levelColumns; ++column) {
            float sum[4] = {0, 0, 0, 0};
            for (int64_t dy = 0; dy < gather; ++dy) {
                for (int64_t dx = 0; dx < gather; ++dx) {
                    float texel[4];
                    backdropTexel(
                        source,
                        (originX + column) * gather + dx,
                        (originY + row) * gather + dy,
                        left,
                        top,
                        columns,
                        rows,
                        texel
                    );
                    for (int channel = 0; channel < 4; ++channel) {
                        sum[channel] += texel[channel];
                    }
                }
            }
            float* out = level.data() + static_cast<size_t>(row * levelColumns + column) * 4;
            for (int channel = 0; channel < 4; ++channel) {
                out[channel] = sum[channel] / static_cast<float>(gather * gather);
            }
        }
    }
    for (int32_t step = 1; step < plan.level; ++step) {
        const int64_t nextColumns = levelColumns / 2;
        const int64_t nextRows = levelRows / 2;
        std::vector<float> next(static_cast<size_t>(nextColumns * nextRows) * 4);
        for (int64_t row = 0; row < nextRows; ++row) {
            for (int64_t column = 0; column < nextColumns; ++column) {
                float* out = next.data() + static_cast<size_t>(row * nextColumns + column) * 4;
                for (int channel = 0; channel < 4; ++channel) {
                    float sum = 0;
                    for (int64_t dy = 0; dy < 2; ++dy) {
                        for (int64_t dx = 0; dx < 2; ++dx) {
                            sum += level[static_cast<size_t>(
                                (row * 2 + dy) * levelColumns + column * 2 + dx
                            ) * 4 + channel];
                        }
                    }
                    out[channel] = sum * 0.25f;
                }
            }
        }
        level.swap(next);
        levelColumns = nextColumns;
        levelRows = nextRows;
    }

    const int64_t taps = plan.radius;
    std::vector<float> horizontal(level.size());
    for (int64_t row = 0; row < levelRows; ++row) {
        for (int64_t column = taps; column < levelColumns - taps; ++column) {
            float sum[4] = {0, 0, 0, 0};
            for (int64_t tap = -taps; tap <= taps; ++tap) {
                const float weight = plan.weights[static_cast<size_t>(tap + taps)];
                const float* texel = level.data() + static_cast<size_t>(row * levelColumns + column + tap) * 4;
                for (int channel = 0; channel < 4; ++channel) {
                    sum[channel] += weight * texel[channel];
                }
            }
            float* out = horizontal.data() + static_cast<size_t>(row * levelColumns + column) * 4;
            for (int channel = 0; channel < 4; ++channel) {
                out[channel] = sum[channel];
            }
        }
    }
    for (int64_t row = taps; row < levelRows - taps; ++row) {
        for (int64_t column = taps; column < levelColumns - taps; ++column) {
            float sum[4] = {0, 0, 0, 0};
            for (int64_t tap = -taps; tap <= taps; ++tap) {
                const float weight = plan.weights[static_cast<size_t>(tap + taps)];
                const float* texel = horizontal.data() + static_cast<size_t>((row + tap) * levelColumns + column) * 4;
                for (int channel = 0; channel < 4; ++channel) {
                    sum[channel] += weight * texel[channel];
                }
            }
            float* out = level.data() + static_cast<size_t>(row * levelColumns + column) * 4;
            for (int channel = 0; channel < 4; ++channel) {
                out[channel] = sum[channel];
            }
        }
    }

    const size_t outputRow = static_cast<size_t>(columns) * 4;
    uint8_t* output = static_cast<uint8_t*>(malloc(outputRow * static_cast<size_t>(rows)));
    if (!output) {
        return nullptr;
    }
    const double factor = plan.factor;
    for (int64_t row = 0; row < rows; ++row) {
        const double v = (static_cast<double>(top + row) + 0.5) / factor - 0.5 - static_cast<double>(plan.firstY);
        const int64_t y0 = static_cast<int64_t>(std::floor(v));
        const float ty = static_cast<float>(v - static_cast<double>(y0));
        const int64_t y1 = std::min<int64_t>(y0 + 1, levelRows - 1);
        for (int64_t column = 0; column < columns; ++column) {
            const double u = (static_cast<double>(left + column) + 0.5) / factor - 0.5 -
                static_cast<double>(plan.firstX);
            const int64_t x0 = static_cast<int64_t>(std::floor(u));
            const float tx = static_cast<float>(u - static_cast<double>(x0));
            const int64_t x1 = std::min<int64_t>(x0 + 1, levelColumns - 1);
            const float* a = level.data() + static_cast<size_t>(y0 * levelColumns + x0) * 4;
            const float* b = level.data() + static_cast<size_t>(y0 * levelColumns + x1) * 4;
            const float* c = level.data() + static_cast<size_t>(y1 * levelColumns + x0) * 4;
            const float* d = level.data() + static_cast<size_t>(y1 * levelColumns + x1) * 4;
            uint8_t* out = output + static_cast<size_t>(row) * outputRow + static_cast<size_t>(column) * 4;
            for (int channel = 0; channel < 4; ++channel) {
                const float upper = a[channel] + (b[channel] - a[channel]) * tx;
                const float lower = c[channel] + (d[channel] - c[channel]) * tx;
                out[channel] = quantizeBlurChannel(upper + (lower - upper) * ty);
            }
            for (int channel = 0; channel < 3; ++channel) {
                out[channel] = std::min(out[channel], out[3]);
            }
        }
    }
    return output;
}

static int32_t
backdropSpread(const CGAffineTransform& transform, double radius) {
    const double determinant = transform.a * transform.d - transform.b * transform.c;
    const double spread = std::isfinite(radius) && radius > 0
        ? std::ceil(radius * std::sqrt(std::fabs(determinant)) - 1e-6)
        : 0;
    return static_cast<int32_t>(std::min<double>(spread, kMaxBackdropBlur));
}

extern "C" void
brixGraphicsBackdropBlur(void* ptr, double x, double y, double width, double height, double radius) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || canvas->bytesPerPixel != 4 || !(width > 0) || !(height > 0)) {
        return;
    }
    for (const BxGraphicsCanvas* group = canvas; group; group = group->parent) {
        if (group->groupKind == BxGraphicsGroupMask) {
            return;
        }
    }
    const BxGraphicsCanvas* source = canvas;
    while (source->parent && groupIsBlank(source)) {
        source = source->parent;
    }
    if (!source->pixels || source->bytesPerPixel != 4) {
        return;
    }
    const CGAffineTransform transform = CGContextGetCTM(canvas->context);
    CGRect device = CGRectIntersection(
        CGRectApplyAffineTransform(CGRectMake(x, y, width, height), transform),
        CGRectApplyAffineTransform(CGContextGetClipBoundingBox(canvas->context), transform)
    );
    if (CGRectIsNull(device) || CGRectIsEmpty(device)) {
        return;
    }
    device = CGRectIntersection(
        CGRectIntegral(device),
        CGRectMake(0, 0, canvas->pixelWidth, canvas->pixelHeight)
    );
    if (CGRectIsNull(device) || CGRectIsEmpty(device)) {
        return;
    }
    const int64_t columns = static_cast<int64_t>(device.size.width);
    const int64_t rows = static_cast<int64_t>(device.size.height);
    const int64_t left = canvas->absoluteLeft + static_cast<int64_t>(device.origin.x);
    const int64_t top = canvas->absoluteTop + canvas->pixelHeight -
        static_cast<int64_t>(device.origin.y + device.size.height);
    const size_t outputRow = static_cast<size_t>(columns) * 4;
    uint8_t* output = blurBackdropPixels(source, left, top, columns, rows, backdropSpread(transform, radius));
    if (!output) {
        return;
    }
    CGContextRef region = CGBitmapContextCreate(
        output,
        static_cast<size_t>(columns),
        static_cast<size_t>(rows),
        8,
        outputRow,
        canvas->colorSpace,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
    );
    CGImageRef image = region ? CGBitmapContextCreateImage(region) : nullptr;
    if (image) {
        invalidateDeviceRect(canvas, device);
        CGContextSaveGState(canvas->context);
        CGContextConcatCTM(canvas->context, CGAffineTransformInvert(transform));
        CGContextSetInterpolationQuality(canvas->context, kCGInterpolationNone);
        CGContextSetBlendMode(canvas->context, kCGBlendModeCopy);
        CGContextDrawImage(canvas->context, device, image);
        CGContextRestoreGState(canvas->context);
        CGImageRelease(image);
    }
    if (region) {
        CGContextRelease(region);
    }
    free(output);
}

extern "C" void
brixGraphicsBackdropShader(
    void* ptr,
    double x,
    double y,
    double width,
    double height,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double time,
    double radius
) {
    BxGraphicsCanvas* canvas = getCanvas(ptr);
    if (!canvas || !canvas->context || canvas->bytesPerPixel != 4 || !(width > 0) || !(height > 0) ||
        (!args && argCount)) {
        return;
    }
    for (const BxGraphicsCanvas* group = canvas; group; group = group->parent) {
        if (group->groupKind == BxGraphicsGroupMask) {
            return;
        }
    }
    const BxGraphicsCanvas* source = canvas;
    while (source->parent && groupIsBlank(source)) {
        source = source->parent;
    }
    if (!source->pixels || source->bytesPerPixel != 4) {
        return;
    }
    const CGAffineTransform transform = CGContextGetCTM(canvas->context);
    const CGRect box = CGRectApplyAffineTransform(CGRectMake(x, y, width, height), transform);
    CGRect device = CGRectIntersection(
        box,
        CGRectApplyAffineTransform(CGContextGetClipBoundingBox(canvas->context), transform)
    );
    if (CGRectIsNull(device) || CGRectIsEmpty(device)) {
        return;
    }
    device = CGRectIntersection(
        CGRectIntegral(device),
        CGRectMake(0, 0, canvas->pixelWidth, canvas->pixelHeight)
    );
    if (CGRectIsNull(device) || CGRectIsEmpty(device)) {
        return;
    }
    const int64_t columns = static_cast<int64_t>(device.size.width);
    const int64_t rows = static_cast<int64_t>(device.size.height);
    const int64_t left = canvas->absoluteLeft + static_cast<int64_t>(device.origin.x);
    const int64_t top = canvas->absoluteTop + canvas->pixelHeight -
        static_cast<int64_t>(device.origin.y + device.size.height);
    const size_t outputRow = static_cast<size_t>(columns) * 4;
    const int32_t blur = backdropSpread(transform, radius);
    uint8_t* output = blur > 0
        ? blurBackdropPixels(source, left, top, columns, rows, blur)
        : static_cast<uint8_t*>(calloc(outputRow, static_cast<size_t>(rows)));
    if (!output) {
        return;
    }
    if (blur <= 0) {
        for (int64_t row = 0; row < rows; ++row) {
            const int64_t localY = top + row - source->absoluteTop;
            if (localY < 0 || localY >= source->pixelHeight) {
                continue;
            }
            const int64_t firstX = std::max<int64_t>(left, source->absoluteLeft);
            const int64_t lastX = std::min<int64_t>(left + columns, source->absoluteLeft + source->pixelWidth);
            if (firstX >= lastX) {
                continue;
            }
            memcpy(
                output + static_cast<size_t>(row) * outputRow + static_cast<size_t>(firstX - left) * 4,
                static_cast<const uint8_t*>(source->pixels) + static_cast<size_t>(localY) * source->bytesPerRow +
                    static_cast<size_t>(firstX - source->absoluteLeft) * 4,
                static_cast<size_t>(lastX - firstX) * 4
            );
        }
    }
    const double scale = std::sqrt(std::fabs(transform.a * transform.d - transform.b * transform.c));
    const double origin[2] = {
        static_cast<double>(canvas->absoluteLeft) + CGRectGetMinX(box) - static_cast<double>(left),
        static_cast<double>(canvas->absoluteTop + canvas->pixelHeight) - CGRectGetMaxY(box) -
            static_cast<double>(top),
    };
    const double size[2] = {width, height};
    const bool shaded = scale > 0 && bxGraphicsShaderFilter(
        shader,
        output,
        static_cast<int32_t>(columns),
        static_cast<int32_t>(rows),
        outputRow,
        source->format,
        source->space,
        origin,
        size,
        scale,
        args,
        argCount,
        nullptr,
        nullptr,
        0,
        time
    );
    CGContextRef region = shaded ? CGBitmapContextCreate(
        output,
        static_cast<size_t>(columns),
        static_cast<size_t>(rows),
        8,
        outputRow,
        canvas->colorSpace,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
    ) : nullptr;
    CGImageRef image = region ? CGBitmapContextCreateImage(region) : nullptr;
    if (image) {
        invalidateDeviceRect(canvas, device);
        CGContextSaveGState(canvas->context);
        CGContextConcatCTM(canvas->context, CGAffineTransformInvert(transform));
        CGContextSetInterpolationQuality(canvas->context, kCGInterpolationNone);
        CGContextSetBlendMode(canvas->context, kCGBlendModeCopy);
        CGContextDrawImage(canvas->context, device, image);
        CGContextRestoreGState(canvas->context);
        CGImageRelease(image);
    }
    if (region) {
        CGContextRelease(region);
    }
    free(output);
}

// **********************************************************************************************

extern "C" void
brixGraphicsBeginPath(void* ptr) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        CGContextBeginPath(canvas->context);
    }
}

extern "C" bool
brixGraphicsPathContainsPoint(void* ptr, double x, double y, int32_t fillRule) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (!canvas || !canvas->context) {
        return false;
    }
    bool inside = CGContextPathContainsPoint(
        canvas->context,
        CGPointMake(x, y),
        fillRule == 1 ? kCGPathEOFill : kCGPathFill
    );
    CGContextBeginPath(canvas->context);
    return inside;
}

extern "C" void
brixGraphicsMoveTo(void* ptr, double x, double y) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        CGContextMoveToPoint(canvas->context, x, y);
    }
}

extern "C" void
brixGraphicsLineTo(void* ptr, double x, double y) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        CGContextAddLineToPoint(canvas->context, x, y);
    }
}

extern "C" void
brixGraphicsCurveTo(
    void* ptr,
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3
) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        CGContextAddCurveToPoint(canvas->context, x1, y1, x2, y2, x3, y3);
    }
}

extern "C" void
brixGraphicsClosePath(void* ptr) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (canvas && canvas->context) {
        CGContextClosePath(canvas->context);
    }
}

extern "C" void
brixGraphicsAppendPath(void* ptr, const uint8_t* verbs, int64_t verbCount, const double* coords) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (!canvas || !canvas->context || !verbs || verbCount <= 0) {
        return;
    }
    CGContextRef context = canvas->context;
    double currentX = 0;
    double currentY = 0;
    double subpathX = 0;
    double subpathY = 0;
    bool hasCurrent = false;
    size_t coord = 0;
    for (int64_t index = 0; index < verbCount; ++index) {
        switch (verbs[index]) {
        case 0:
            currentX = coords[coord];
            currentY = coords[coord + 1];
            coord += 2;
            subpathX = currentX;
            subpathY = currentY;
            hasCurrent = true;
            CGContextMoveToPoint(context, currentX, currentY);
            break;
        case 1:
            currentX = coords[coord];
            currentY = coords[coord + 1];
            coord += 2;
            CGContextAddLineToPoint(context, currentX, currentY);
            break;
        case 2:
            currentX = coords[coord + 4];
            currentY = coords[coord + 5];
            CGContextAddCurveToPoint(
                context,
                coords[coord],
                coords[coord + 1],
                coords[coord + 2],
                coords[coord + 3],
                currentX,
                currentY
            );
            coord += 6;
            break;
        default:
            if (hasCurrent && (currentX != subpathX || currentY != subpathY)) {
                CGContextAddLineToPoint(context, subpathX, subpathY);
                currentX = subpathX;
                currentY = subpathY;
            }
            CGContextClosePath(context);
            break;
        }
    }
}

// **********************************************************************************************

static CFStringRef
textStringFromUTF8(const char* value) {
    if (!value || !value[0]) {
        return CFStringCreateCopy(kCFAllocatorDefault, CFSTR(""));
    }

    CFStringRef string = CFStringCreateWithCString(
        kCFAllocatorDefault,
        value,
        kCFStringEncodingUTF8
    );
    return string ? string : CFStringCreateCopy(kCFAllocatorDefault, CFSTR(""));
}

struct TextFontCacheState {
    CFMutableDictionaryRef entries;
    std::mutex mutex;
    std::atomic<int64_t> created {0};
    std::atomic<uint64_t> generation {0};

    TextFontCacheState()
        : entries(CFDictionaryCreateMutable(
            kCFAllocatorDefault,
            0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks
        )) {}
};

static TextFontCacheState&
textFontCacheState() {
    static TextFontCacheState* state = new TextFontCacheState();
    return *state;
}

static thread_local int64_t textFontLastCreateMissCount = 0;

struct TextFontMemoEntry {
    std::string family;
    double size = 0;
    double weight = 0;
    bool italic = false;
    uint64_t generation = 0;
    CTFontRef font = nullptr;
    bool hasLineMetrics = false;
    double lineMetrics[4] = {0, 0, 0, 0};
    uint64_t asciiCoverage[2] = {0, 0};
    bool coversEllipsis = false;
};

static constexpr size_t textFontMemoCapacity = 4;

struct TextFontMemo {
    TextFontMemoEntry entries[textFontMemoCapacity];
    size_t next = 0;

    ~TextFontMemo() {
        for (TextFontMemoEntry& entry : entries) {
            if (entry.font) {
                CFRelease(entry.font);
            }
        }
    }
};

static thread_local TextFontMemo textFontMemo;

static TextFontMemoEntry*
findTextFontMemo(const char* family, double size, double weight, bool italic, uint64_t generation) {
    const char* name = family ? family : "";
    for (TextFontMemoEntry& entry : textFontMemo.entries) {
        if (entry.font && entry.generation == generation && entry.size == size &&
                entry.weight == weight && entry.italic == italic && entry.family == name) {
            return &entry;
        }
    }
    return nullptr;
}

static TextFontMemoEntry*
rememberTextFont(
    const char* family,
    double size,
    double weight,
    bool italic,
    uint64_t generation,
    CTFontRef font
) {
    TextFontMemoEntry& entry = textFontMemo.entries[textFontMemo.next];
    textFontMemo.next = (textFontMemo.next + 1) % textFontMemoCapacity;
    CFRetain(font);
    if (entry.font) {
        CFRelease(entry.font);
    }
    entry.family = family ? family : "";
    entry.size = size;
    entry.weight = weight;
    entry.italic = italic;
    entry.generation = generation;
    entry.font = font;
    entry.hasLineMetrics = false;
    return &entry;
}

static void
ensureTextFontLineMetrics(TextFontMemoEntry& entry) {
    if (entry.hasLineMetrics) {
        return;
    }
    const double ascent = static_cast<double>(CTFontGetAscent(entry.font));
    const double descent = static_cast<double>(CTFontGetDescent(entry.font));
    const double leading = static_cast<double>(CTFontGetLeading(entry.font));
    double height = ascent + descent + leading;
    if (!std::isfinite(height) || height < 0) {
        height = 0;
    }
    entry.lineMetrics[0] = ascent;
    entry.lineMetrics[1] = descent;
    entry.lineMetrics[2] = leading;
    entry.lineMetrics[3] = height;

    constexpr CFIndex printableCount = 0x7F - 0x20;
    UniChar characters[printableCount + 1];
    CGGlyph glyphs[printableCount + 1] = {};
    for (CFIndex index = 0; index < printableCount; ++index) {
        characters[index] = static_cast<UniChar>(0x20 + index);
    }
    characters[printableCount] = 0x2026;
    CTFontGetGlyphsForCharacters(entry.font, characters, glyphs, printableCount + 1);
    entry.asciiCoverage[0] = 0;
    entry.asciiCoverage[1] = 0;
    for (CFIndex index = 0; index < printableCount; ++index) {
        if (glyphs[index] != 0) {
            const unsigned code = static_cast<unsigned>(characters[index]);
            entry.asciiCoverage[code >> 6] |= uint64_t{1} << (code & 63);
        }
    }
    entry.coversEllipsis = glyphs[printableCount] != 0;
    entry.hasLineMetrics = true;
}

static double
quantizedTextFontSize(double size) {
    const double safeSize = std::isfinite(size) && size > 0 ? size : 13.0;
    const double scaledSize = safeSize * 4.0;
    return std::isfinite(scaledSize) ? std::round(scaledSize) / 4.0 : safeSize;
}

static CFArrayRef
createTextFontCacheKey(CFStringRef family, double size, double weight, bool italic) {
    CFNumberRef sizeNumber = CFNumberCreate(
        kCFAllocatorDefault,
        kCFNumberDoubleType,
        &size
    );
    CFNumberRef weightNumber = CFNumberCreate(
        kCFAllocatorDefault,
        kCFNumberDoubleType,
        &weight
    );
    const void* values[] = {
        family,
        sizeNumber,
        weightNumber,
        italic ? kCFBooleanTrue : kCFBooleanFalse,
    };
    CFArrayRef key = CFArrayCreate(
        kCFAllocatorDefault,
        values,
        4,
        &kCFTypeArrayCallBacks
    );
    CFRelease(sizeNumber);
    CFRelease(weightNumber);
    return key;
}

static CTFontRef
createWeightedTextFont(CTFontRef font, double size, double weight) {
    CFStringRef family = CTFontCopyFamilyName(font);
    if (!family) {
        return nullptr;
    }
    CFNumberRef weightNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &weight);
    const void* traitKeys[] = {kCTFontWeightTrait};
    const void* traitValues[] = {weightNumber};
    CFDictionaryRef traits = CFDictionaryCreate(
        kCFAllocatorDefault,
        traitKeys,
        traitValues,
        1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    const void* keys[] = {kCTFontFamilyNameAttribute, kCTFontTraitsAttribute};
    const void* values[] = {family, traits};
    CFDictionaryRef attributes = CFDictionaryCreate(
        kCFAllocatorDefault,
        keys,
        values,
        2,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes(attributes);
    CTFontRef weighted = descriptor ? CTFontCreateWithFontDescriptor(descriptor, size, nullptr) : nullptr;
    if (descriptor) {
        CFRelease(descriptor);
    }
    CFRelease(attributes);
    CFRelease(traits);
    CFRelease(weightNumber);
    CFRelease(family);
    return weighted;
}

static void
flushTextFontCacheFamilyLocked(TextFontCacheState& state, const std::string& family) {
    CFStringRef familyName = textStringFromUTF8(family.c_str());
    const CFIndex count = CFDictionaryGetCount(state.entries);
    std::vector<const void*> keys(static_cast<size_t>(count));
    std::vector<const void*> values(static_cast<size_t>(count));
    CFDictionaryGetKeysAndValues(state.entries, keys.data(), values.data());
    for (const void* value : keys) {
        CFArrayRef key = static_cast<CFArrayRef>(value);
        if (CFGetTypeID(key) != CFArrayGetTypeID() || CFArrayGetCount(key) != 4) {
            continue;
        }
        CFTypeRef keyFamily = static_cast<CFTypeRef>(
            const_cast<void*>(CFArrayGetValueAtIndex(key, 0))
        );
        if (CFGetTypeID(keyFamily) == CFStringGetTypeID() && CFEqual(keyFamily, familyName)) {
            CFDictionaryRemoveValue(state.entries, key);
        }
    }
    CFRelease(familyName);
    state.generation.fetch_add(1, std::memory_order_acq_rel);
}

static TextFontMemoEntry*
textFontMemoEntry(const char* family, double size, double weight, bool italic, bool& cacheMiss) {
    cacheMiss = false;
    const double safeSize = quantizedTextFontSize(size);
    const double safeWeight = std::isfinite(weight) ? std::max(-1.0, std::min(weight, 1.0)) : 0.0;
    TextFontCacheState& state = textFontCacheState();
    const uint64_t generation = state.generation.load(std::memory_order_acquire);
    TextFontMemoEntry* remembered = findTextFontMemo(
        family,
        safeSize,
        safeWeight,
        italic,
        generation
    );
    if (remembered) {
        return remembered;
    }
    CFStringRef familyName = textStringFromUTF8(family);
    CFArrayRef key = createTextFontCacheKey(familyName, safeSize, safeWeight, italic);
    std::lock_guard<std::mutex> lock(state.mutex);
    CTFontRef cached = static_cast<CTFontRef>(
        const_cast<void*>(CFDictionaryGetValue(state.entries, key))
    );
    if (cached) {
        CFRelease(key);
        CFRelease(familyName);
        return rememberTextFont(family, safeSize, safeWeight, italic, generation, cached);
    }

    CTFontRef font = nullptr;
    if (family && family[0]) {
        font = CTFontCreateWithName(familyName, safeSize, nullptr);
    } else {
        font = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, safeSize, nullptr);
    }
    if (!font) {
        font = CTFontCreateWithName(CFSTR("Helvetica"), safeSize, nullptr);
    }

    if (safeWeight != 0 && font) {
        CTFontRef weightedFont = createWeightedTextFont(font, safeSize, safeWeight);
        if (weightedFont) {
            CFRelease(font);
            font = weightedFont;
        }
    }

    if (italic && font) {
        CTFontRef italicFont = CTFontCreateCopyWithSymbolicTraits(
            font,
            safeSize,
            nullptr,
            kCTFontItalicTrait,
            kCTFontItalicTrait
        );
        if (!italicFont) {
            const CGAffineTransform oblique = CGAffineTransformMake(
                1,
                0,
                std::tan(12.0 * M_PI / 180.0),
                1,
                0,
                0
            );
            CTFontDescriptorRef descriptor = CTFontCopyFontDescriptor(font);
            italicFont = descriptor
                ? CTFontCreateWithFontDescriptor(descriptor, safeSize, &oblique)
                : nullptr;
            if (descriptor) {
                CFRelease(descriptor);
            }
        }
        if (italicFont) {
            CFRelease(font);
            font = italicFont;
        }
    }

    TextFontMemoEntry* entry = nullptr;
    if (font) {
        CFDictionarySetValue(state.entries, key, font);
        state.created.fetch_add(1, std::memory_order_relaxed);
        cacheMiss = true;
        entry = rememberTextFont(family, safeSize, safeWeight, italic, generation, font);
        CFRelease(font);
    }
    CFRelease(key);
    CFRelease(familyName);
    return entry;
}

static CTFontRef
createTextFont(const char* family, double size, double weight, bool italic, bool& cacheMiss) {
    TextFontMemoEntry* entry = textFontMemoEntry(family, size, weight, italic, cacheMiss);
    if (!entry) {
        return nullptr;
    }
    CFRetain(entry->font);
    return entry->font;
}

static CFAttributedStringRef
createAttributedText(CFStringRef string, CTFontRef font) {
    const void* keys[] = {
        kCTFontAttributeName,
        kCTForegroundColorFromContextAttributeName,
    };
    const void* values[] = {
        font,
        kCFBooleanTrue,
    };
    CFDictionaryRef attributes = CFDictionaryCreate(
        kCFAllocatorDefault,
        keys,
        values,
        2,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    CFAttributedStringRef attributed = CFAttributedStringCreate(
        kCFAllocatorDefault,
        string,
        attributes
    );
    CFRelease(attributes);
    return attributed;
}

static CTLineRef
createTextLine(CFStringRef string, CTFontRef font) {
    CFAttributedStringRef attributed = createAttributedText(string, font);
    CTLineRef line = attributed ? CTLineCreateWithAttributedString(attributed) : nullptr;
    if (attributed) {
        CFRelease(attributed);
    }
    return line;
}

static double
textLineWidth(CTLineRef line) {
    if (!line) {
        return 0;
    }
    double width = CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
    return std::isfinite(width) ? std::max(0.0, width) : 0.0;
}

static CTLineRef
createEmptyTextLine(CTFontRef font) {
    CFStringRef empty = CFStringCreateCopy(kCFAllocatorDefault, CFSTR(""));
    CTLineRef line = createTextLine(empty, font);
    CFRelease(empty);
    return line;
}

static CTLineRef
createTruncatedTextLine(
    CTLineRef sourceLine,
    CTFontRef font,
    double maximumWidth,
    CTLineTruncationType truncationType
) {
    CFStringRef ellipsis = CFStringCreateCopy(kCFAllocatorDefault, CFSTR("\u2026"));
    CTLineRef ellipsisLine = createTextLine(ellipsis, font);
    if (!ellipsisLine) {
        CFRelease(ellipsis);
        return createEmptyTextLine(font);
    }
    const double ellipsisWidth = textLineWidth(ellipsisLine);

    if (maximumWidth <= 0 || ellipsisWidth > maximumWidth) {
        CFRelease(ellipsisLine);
        CFRelease(ellipsis);
        return createEmptyTextLine(font);
    }

    CTLineRef truncated = CTLineCreateTruncatedLine(
        sourceLine,
        maximumWidth,
        truncationType,
        ellipsisLine
    );
    CFRelease(ellipsisLine);
    CFRelease(ellipsis);
    return truncated ? truncated : createEmptyTextLine(font);
}

static CGRect
finiteTextRect(CGRect rect) {
    if (CGRectIsNull(rect) || CGRectIsInfinite(rect)
        || !std::isfinite(rect.origin.x) || !std::isfinite(rect.origin.y)
        || !std::isfinite(rect.size.width) || !std::isfinite(rect.size.height)) {
        return CGRectZero;
    }
    rect.size.width = std::max(0.0, rect.size.width);
    rect.size.height = std::max(0.0, rect.size.height);
    return rect;
}

static std::string
utf8String(CFStringRef string) {
    if (!string) {
        return std::string();
    }
    CFIndex capacity = CFStringGetMaximumSizeForEncoding(
        CFStringGetLength(string),
        kCFStringEncodingUTF8
    ) + 1;
    if (capacity <= 1) {
        return std::string();
    }
    std::vector<char> buffer(static_cast<size_t>(capacity));
    if (!CFStringGetCString(string, buffer.data(), capacity, kCFStringEncodingUTF8)) {
        return std::string();
    }
    return std::string(buffer.data());
}

static std::vector<CFRange>
hardTextLineRanges(CFStringRef string) {
    std::vector<CFRange> ranges;
    const CFIndex length = string ? CFStringGetLength(string) : 0;
    CFIndex lineStart = 0;
    CFIndex index = 0;
    while (index < length) {
        const UniChar character = CFStringGetCharacterAtIndex(string, index);
        CFIndex boundaryLength = 0;
        if (character == '\r') {
            boundaryLength = index + 1 < length &&
                CFStringGetCharacterAtIndex(string, index + 1) == '\n'
                ? 2
                : 1;
        } else if (character == '\n' || character == 0x2028 || character == 0x2029) {
            boundaryLength = 1;
        }
        if (boundaryLength == 0) {
            index += 1;
            continue;
        }
        ranges.push_back(CFRangeMake(lineStart, index - lineStart));
        index += boundaryLength;
        lineStart = index;
    }
    ranges.push_back(CFRangeMake(lineStart, length - lineStart));
    return ranges;
}

static CFIndex
safeComposedBreakCount(CFStringRef string, CFIndex start, CFIndex suggestedCount) {
    const CFIndex length = CFStringGetLength(string);
    if (start >= length) {
        return 0;
    }
    CFIndex end = std::min(length, start + std::max<CFIndex>(0, suggestedCount));
    if (end <= start) {
        CFRange cluster = CFStringGetRangeOfComposedCharactersAtIndex(string, start);
        return std::max<CFIndex>(1, cluster.location + cluster.length - start);
    }
    if (end < length) {
        CFRange cluster = CFStringGetRangeOfComposedCharactersAtIndex(string, end - 1);
        const CFIndex clusterEnd = cluster.location + cluster.length;
        if (end < clusterEnd) {
            end = cluster.location > start ? cluster.location : clusterEnd;
        }
    }
    return std::max<CFIndex>(1, end - start);
}

static CFIndex
trimTextLineTrailingWhitespace(CFStringRef string, CFIndex start, CFIndex count) {
    CFCharacterSetRef whitespace = CFCharacterSetGetPredefined(
        kCFCharacterSetWhitespaceAndNewline
    );
    CFIndex end = start + count;
    while (end > start) {
        const UniChar character = CFStringGetCharacterAtIndex(string, end - 1);
        if (!CFCharacterSetIsCharacterMember(whitespace, character)) {
            break;
        }
        end -= 1;
    }
    return end - start;
}

static bool
appendTextLayoutLine(
    BxGraphicsTextLayout* layout,
    CTLineRef line,
    CTFontRef font,
    CFIndex base,
    CFIndex start,
    CFIndex end
) {
    if (!layout || !line || !font) {
        return false;
    }
    if (!layout->lines.empty()) {
        layout->height += layout->lineSpacing;
    }

    BxGraphicsTextLine measured = {};
    measured.line = line;
    measured.base = base;
    measured.start = start;
    measured.end = end;
    CGFloat lineAscent = 0;
    CGFloat lineDescent = 0;
    CGFloat lineLeading = 0;
    double lineWidth = CTLineGetTypographicBounds(
        line,
        &lineAscent,
        &lineDescent,
        &lineLeading
    );
    measured.width = std::isfinite(lineWidth) ? std::max(0.0, lineWidth) : 0.0;
    measured.ascent = std::max(
        static_cast<double>(CTFontGetAscent(font)),
        static_cast<double>(lineAscent)
    );
    measured.descent = std::max(
        static_cast<double>(CTFontGetDescent(font)),
        static_cast<double>(lineDescent)
    );
    measured.leading = std::max(
        static_cast<double>(CTFontGetLeading(font)),
        static_cast<double>(lineLeading)
    );
    measured.height = measured.ascent + measured.descent + measured.leading;
    if (!std::isfinite(measured.height) || measured.height < 0) {
        measured.height = 0;
    }
    measured.baseline = layout->height + measured.ascent;
    measured.typographicBounds = CGRectMake(
        0,
        layout->height,
        measured.width,
        measured.height
    );
    layout->layoutWidth = std::max(layout->layoutWidth, measured.width);
    layout->height += measured.height;
    layout->lines.push_back(measured);
    return true;
}

static const CGRect&
textLineInkBounds(const BxGraphicsTextLine& measured) {
    if (!measured.hasInkBounds) {
        CGRect nativeInk = finiteTextRect(CTLineGetBoundsWithOptions(
            measured.line,
            kCTLineBoundsUseGlyphPathBounds | kCTLineBoundsExcludeTypographicLeading
        ));
        measured.inkBounds = finiteTextRect(CGRectMake(
            nativeInk.origin.x,
            measured.typographicBounds.origin.y + measured.ascent - CGRectGetMaxY(nativeInk),
            nativeInk.size.width,
            nativeInk.size.height
        ));
        measured.hasInkBounds = true;
    }
    return measured.inkBounds;
}

static void
releaseTextLayout(BxGraphicsTextLayout* layout) {
    if (!layout) {
        return;
    }
    for (BxGraphicsTextLine& line : layout->lines) {
        if (line.line) {
            CFRelease(line.line);
            line.line = nullptr;
        }
    }
    delete layout;
}

static const BxGraphicsTextLine*
getTextLayoutLine(void* ptr, int64_t index) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    if (!layout || index < 0 || index >= static_cast<int64_t>(layout->lines.size())) {
        return nullptr;
    }
    return &layout->lines[static_cast<size_t>(index)];
}

static CTRunRef
getTextLayoutRun(void* ptr, int64_t index, int64_t* lineIndex) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    if (!layout || index < 0) {
        return nullptr;
    }
    int64_t current = 0;
    for (size_t line = 0; line < layout->lines.size(); ++line) {
        CFArrayRef runs = CTLineGetGlyphRuns(layout->lines[line].line);
        const int64_t count = static_cast<int64_t>(CFArrayGetCount(runs));
        if (index < current + count) {
            if (lineIndex) {
                *lineIndex = static_cast<int64_t>(line);
            }
            return static_cast<CTRunRef>(
                const_cast<void*>(CFArrayGetValueAtIndex(runs, index - current))
            );
        }
        current += count;
    }
    return nullptr;
}

static double
finiteNonnegative(double value) {
    return std::isfinite(value) ? std::max(0.0, value) : 0.0;
}

static std::atomic<uint64_t> gTextLayoutSerial{0};

static bool
isHardTextBreak(UniChar character) {
    return character == '\r' || character == '\n' || character == 0x2028 || character == 0x2029;
}

static CTLineRef
createClampedTailLine(
    CFAttributedStringRef text,
    CTFontRef font,
    CFIndex start,
    CFIndex end,
    bool hasMaximumWidth,
    double maximumWidth
) {
    const CFIndex length = CFAttributedStringGetLength(text);
    const CFIndex finish = hasMaximumWidth ? length : end;
    CFAttributedStringRef source = CFAttributedStringCreateWithSubstring(
        kCFAllocatorDefault,
        text,
        CFRangeMake(start, finish - start)
    );
    if (!source) {
        return nullptr;
    }
    CFMutableAttributedStringRef rest = CFAttributedStringCreateMutableCopy(
        kCFAllocatorDefault,
        0,
        source
    );
    CFRelease(source);
    if (!rest) {
        return nullptr;
    }
    CFStringRef string = CFAttributedStringGetString(rest);
    const CFIndex count = CFStringGetLength(string);
    for (CFIndex index = 0; index < count; ++index) {
        if (isHardTextBreak(CFStringGetCharacterAtIndex(string, index))) {
            CFAttributedStringReplaceString(rest, CFRangeMake(index, 1), CFSTR(" "));
        }
    }
    if (!hasMaximumWidth) {
        CFAttributedStringReplaceString(rest, CFRangeMake(count, 0), CFSTR("…"));
        if (count == 0) {
            CFAttributedStringSetAttribute(rest, CFRangeMake(0, 1), kCTFontAttributeName, font);
        }
    }
    CTLineRef line = CTLineCreateWithAttributedString(rest);
    CFRelease(rest);
    if (!line || !hasMaximumWidth || textLineWidth(line) <= maximumWidth) {
        return line;
    }
    CTLineRef truncated = createTruncatedTextLine(line, font, maximumWidth, kCTLineTruncationEnd);
    CFRelease(line);
    return truncated;
}

static bool
clampTextLayoutHeight(
    BxGraphicsTextLayout* layout,
    CFAttributedStringRef text,
    CTFontRef font,
    bool hasMaximumWidth,
    double maximumWidth,
    double maximumHeight
) {
    size_t kept = 1;
    while (kept < layout->lines.size() &&
           CGRectGetMaxY(layout->lines[kept].typographicBounds) <= maximumHeight + 0.01) {
        kept += 1;
    }
    if (kept >= layout->lines.size()) {
        return true;
    }
    std::vector<BxGraphicsTextLine> lines;
    lines.swap(layout->lines);
    const CFIndex start = lines[kept - 1].start;
    CTLineRef tail = createClampedTailLine(
        text,
        font,
        start,
        lines[kept - 1].end,
        hasMaximumWidth,
        maximumWidth
    );
    for (size_t index = kept - 1; index < lines.size(); ++index) {
        CFRelease(lines[index].line);
    }
    layout->height = 0;
    layout->layoutWidth = 0;
    layout->truncated = true;
    for (size_t index = 0; index + 1 < kept; ++index) {
        const BxGraphicsTextLine& line = lines[index];
        appendTextLayoutLine(layout, line.line, font, line.base, line.start, line.end);
    }
    if (!appendTextLayoutLine(layout, tail, font, start, start, CFAttributedStringGetLength(text))) {
        if (tail) {
            CFRelease(tail);
        }
        return false;
    }
    return true;
}

static BxGraphicsTextLayout*
createTextLayoutFromAttributed(
    CFAttributedStringRef text,
    CTFontRef font,
    bool hasMaximumWidth,
    double maximumWidth,
    bool hasMaximumHeight,
    double maximumHeight,
    int32_t truncation,
    bool wordWrap,
    double lineSpacing
) {
    CFStringRef string = CFAttributedStringGetString(text);
    BxGraphicsTextLayout* layout = new BxGraphicsTextLayout();
    layout->serial = gTextLayoutSerial.fetch_add(1, std::memory_order_relaxed) + 1;
    layout->lineSpacing = finiteNonnegative(lineSpacing);
    layout->naturalWidth = 0;
    layout->constrainedWidth = 0;
    layout->layoutWidth = 0;
    layout->height = 0;
    layout->truncated = false;

    const double safeMaximumWidth = std::isfinite(maximumWidth)
        ? std::max(0.0, maximumWidth)
        : 0.0;
    bool valid = true;
    const CFIndex textLength = CFStringGetLength(string);
    for (const CFRange hardRange : hardTextLineRanges(string)) {
        CFAttributedStringRef segment = hardRange.location == 0 && hardRange.length == textLength
            ? static_cast<CFAttributedStringRef>(CFRetain(text))
            : CFAttributedStringCreateWithSubstring(kCFAllocatorDefault, text, hardRange);
        const bool wraps = wordWrap && hasMaximumWidth && safeMaximumWidth > 0;
        CTTypesetterRef typesetter = wraps && segment
            ? CTTypesetterCreateWithAttributedString(segment)
            : nullptr;
        CTLineRef naturalLine = typesetter
            ? CTTypesetterCreateLine(typesetter, CFRangeMake(0, 0))
            : (segment && !wraps ? CTLineCreateWithAttributedString(segment) : nullptr);
        if (!naturalLine) {
            if (typesetter) {
                CFRelease(typesetter);
            }
            if (segment) {
                CFRelease(segment);
            }
            valid = false;
            break;
        }
        const double naturalWidth = textLineWidth(naturalLine);
        layout->naturalWidth = std::max(layout->naturalWidth, naturalWidth);

        if (!wordWrap || !hasMaximumWidth) {
            CFRelease(segment);
            CTLineRef visibleLine = naturalLine;
            if (!wordWrap && truncation != 0 && hasMaximumWidth &&
                naturalWidth > safeMaximumWidth) {
                visibleLine = createTruncatedTextLine(
                    naturalLine,
                    font,
                    safeMaximumWidth,
                    truncation == 2 ? kCTLineTruncationStart : kCTLineTruncationEnd
                );
                CFRelease(naturalLine);
                layout->truncated = true;
            }
            if (!appendTextLayoutLine(
                layout,
                visibleLine,
                font,
                hardRange.location,
                hardRange.location,
                hardRange.location + hardRange.length
            )) {
                if (visibleLine) {
                    CFRelease(visibleLine);
                }
                valid = false;
                break;
            }
            continue;
        }

        if (safeMaximumWidth == 0) {
            CFRelease(naturalLine);
            CFRelease(segment);
            CTLineRef emptyLine = createEmptyTextLine(font);
            if (!appendTextLayoutLine(
                layout,
                emptyLine,
                font,
                hardRange.location,
                hardRange.location,
                hardRange.location + hardRange.length
            )) {
                if (emptyLine) {
                    CFRelease(emptyLine);
                }
                valid = false;
                break;
            }
            layout->truncated = layout->truncated || hardRange.length > 0;
            continue;
        }

        CFStringRef segmentString = CFAttributedStringGetString(segment);
        const CFIndex segmentLength = CFStringGetLength(segmentString);
        if (segmentLength > 0 && naturalWidth <= safeMaximumWidth) {
            valid = appendTextLayoutLine(
                layout,
                naturalLine,
                font,
                hardRange.location,
                hardRange.location,
                hardRange.location + segmentLength
            );
            if (!valid) {
                CFRelease(naturalLine);
            }
        } else if (segmentLength == 0) {
            CFRelease(naturalLine);
            CTLineRef emptyLine = createEmptyTextLine(font);
            valid = appendTextLayoutLine(
                layout,
                emptyLine,
                font,
                hardRange.location,
                hardRange.location,
                hardRange.location
            );
            if (!valid && emptyLine) {
                CFRelease(emptyLine);
            }
        } else {
            CFRelease(naturalLine);
            CFIndex start = 0;
            while (valid && start < segmentLength) {
                CFIndex count = CTTypesetterSuggestLineBreak(
                    typesetter,
                    start,
                    safeMaximumWidth
                );
                if (count <= 0) {
                    count = CTTypesetterSuggestClusterBreak(
                        typesetter,
                        start,
                        safeMaximumWidth
                    );
                }
                count = safeComposedBreakCount(segmentString, start, count);
                count = std::min(count, segmentLength - start);
                CFIndex visibleCount = count;
                if (start + count < segmentLength) {
                    visibleCount = trimTextLineTrailingWhitespace(segmentString, start, count);
                }
                CTLineRef line = visibleCount > 0
                    ? CTTypesetterCreateLine(
                        typesetter,
                        CFRangeMake(start, visibleCount)
                    )
                    : createEmptyTextLine(font);
                valid = appendTextLayoutLine(
                    layout,
                    line,
                    font,
                    hardRange.location,
                    hardRange.location + start,
                    hardRange.location + start + count
                );
                if (!valid && line) {
                    CFRelease(line);
                }
                start += count;
            }
        }
        CFRelease(typesetter);
        CFRelease(segment);
        if (!valid) {
            break;
        }
    }

    if (valid && truncation == 1 && hasMaximumHeight && std::isfinite(maximumHeight)) {
        valid = clampTextLayoutHeight(
            layout,
            text,
            font,
            hasMaximumWidth,
            safeMaximumWidth,
            std::max(0.0, maximumHeight)
        );
    }

    layout->constrainedWidth = hasMaximumWidth
        ? std::min(layout->naturalWidth, safeMaximumWidth)
        : layout->naturalWidth;

    if (!valid || layout->lines.empty()) {
        releaseTextLayout(layout);
        return nullptr;
    }
    return layout;
}

extern "C" void*
brixGraphicsTextLayoutCreate(
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
) {
    bool fontCacheMiss = false;
    CTFontRef font = createTextFont(fontFamily, fontSize, weight, italic, fontCacheMiss);
    textFontLastCreateMissCount = fontCacheMiss ? 1 : 0;
    if (!font) {
        return nullptr;
    }

    CFStringRef string = textStringFromUTF8(text);
    CFAttributedStringRef attributed = createAttributedText(string, font);
    CFRelease(string);
    BxGraphicsTextLayout* layout = attributed
        ? createTextLayoutFromAttributed(
            attributed,
            font,
            hasMaximumWidth,
            maximumWidth,
            hasMaximumHeight,
            maximumHeight,
            truncation,
            wordWrap,
            lineSpacing
        )
        : nullptr;
    if (attributed) {
        CFRelease(attributed);
    }
    CFRelease(font);
    return layout;
}

extern "C" bool
brixGraphicsTextLayoutSimpleMetrics(
    const char* text,
    const char* fontFamily,
    double fontSize,
    double weight,
    bool italic,
    bool hasMaximumWidth,
    int32_t truncation,
    bool wordWrap,
    double* metrics
) {
    textFontLastCreateMissCount = 0;
    if (!metrics || (wordWrap && hasMaximumWidth)) {
        return false;
    }
    bool fontCacheMiss = false;
    TextFontMemoEntry* entry = textFontMemoEntry(fontFamily, fontSize, weight, italic, fontCacheMiss);
    textFontLastCreateMissCount = fontCacheMiss ? 1 : 0;
    if (!entry) {
        return false;
    }
    ensureTextFontLineMetrics(*entry);
    if (!wordWrap && truncation != 0 && hasMaximumWidth && !entry->coversEllipsis) {
        return false;
    }
    if (text) {
        for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text); *cursor; ++cursor) {
            const unsigned code = *cursor;
            if (code < 0x20 || code > 0x7E || !(entry->asciiCoverage[code >> 6] & (uint64_t{1} << (code & 63)))) {
                return false;
            }
        }
    }
    metrics[0] = entry->lineMetrics[0];
    metrics[1] = entry->lineMetrics[1];
    metrics[2] = entry->lineMetrics[2];
    metrics[3] = entry->lineMetrics[3];
    return true;
}

static const CFStringRef styledTextTagAttributeName = CFSTR("BxStyledTextTag");

struct BxGraphicsStyledText {
    CFMutableAttributedStringRef text;
    CTFontRef baseFont;
    int64_t fontCacheMisses;
};

struct BxGraphicsStyledTextObject {
    double width;
    double ascent;
    double descent;
};

static void
styledTextObjectDeallocate(void* refCon) {
    delete static_cast<BxGraphicsStyledTextObject*>(refCon);
}

static CGFloat
styledTextObjectAscent(void* refCon) {
    return static_cast<BxGraphicsStyledTextObject*>(refCon)->ascent;
}

static CGFloat
styledTextObjectDescent(void* refCon) {
    return static_cast<BxGraphicsStyledTextObject*>(refCon)->descent;
}

static CGFloat
styledTextObjectWidth(void* refCon) {
    return static_cast<BxGraphicsStyledTextObject*>(refCon)->width;
}

static void
appendStyledTextRun(
    BxGraphicsStyledText* styled,
    CFStringRef string,
    CFDictionaryRef attributes
) {
    CFAttributedStringRef run = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    if (!run) {
        return;
    }
    CFAttributedStringReplaceAttributedString(
        styled->text,
        CFRangeMake(CFAttributedStringGetLength(styled->text), 0),
        run
    );
    CFRelease(run);
}

extern "C" void*
brixGraphicsStyledTextCreate(
    const char* fontFamily,
    double fontSize,
    double weight,
    bool italic
) {
    bool fontCacheMiss = false;
    CTFontRef font = createTextFont(fontFamily, fontSize, weight, italic, fontCacheMiss);
    if (!font) {
        return nullptr;
    }
    CFMutableAttributedStringRef text = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
    if (!text) {
        CFRelease(font);
        return nullptr;
    }
    CFAttributedStringBeginEditing(text);
    BxGraphicsStyledText* styled = new BxGraphicsStyledText();
    styled->text = text;
    styled->baseFont = font;
    styled->fontCacheMisses = fontCacheMiss ? 1 : 0;
    return styled;
}

extern "C" void
brixGraphicsStyledTextRelease(void* ptr) {
    BxGraphicsStyledText* styled = static_cast<BxGraphicsStyledText*>(ptr);
    if (!styled) {
        return;
    }
    CFRelease(styled->text);
    CFRelease(styled->baseFont);
    delete styled;
}

extern "C" void
brixGraphicsStyledTextAppendText(
    void* ptr,
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
) {
    BxGraphicsStyledText* styled = static_cast<BxGraphicsStyledText*>(ptr);
    if (!styled || !text || !text[0]) {
        return;
    }
    bool fontCacheMiss = false;
    CTFontRef font = createTextFont(fontFamily, fontSize, weight, italic, fontCacheMiss);
    if (!font) {
        return;
    }
    styled->fontCacheMisses += fontCacheMiss ? 1 : 0;
    CFStringRef string = textStringFromUTF8(text);
    CFNumberRef tagNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &tag);
    CGColorRef color = hasColor ? createSRGBColor(red, green, blue, alpha) : nullptr;
    const void* keys[] = {
        kCTFontAttributeName,
        styledTextTagAttributeName,
        color ? kCTForegroundColorAttributeName : kCTForegroundColorFromContextAttributeName,
    };
    const void* values[] = {
        font,
        tagNumber,
        color ? static_cast<const void*>(color) : static_cast<const void*>(kCFBooleanTrue),
    };
    CFDictionaryRef attributes = CFDictionaryCreate(
        kCFAllocatorDefault,
        keys,
        values,
        3,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    appendStyledTextRun(styled, string, attributes);
    CFRelease(attributes);
    if (color) {
        CGColorRelease(color);
    }
    CFRelease(tagNumber);
    CFRelease(string);
    CFRelease(font);
}

extern "C" void
brixGraphicsStyledTextAppendObject(
    void* ptr,
    double width,
    double ascent,
    double descent,
    int64_t tag,
    bool joinBefore,
    bool joinAfter
) {
    BxGraphicsStyledText* styled = static_cast<BxGraphicsStyledText*>(ptr);
    if (!styled) {
        return;
    }
    BxGraphicsStyledTextObject* object = new BxGraphicsStyledTextObject();
    object->width = finiteNonnegative(width);
    object->ascent = finiteNonnegative(ascent);
    object->descent = finiteNonnegative(descent);
    CTRunDelegateCallbacks callbacks = {};
    callbacks.version = kCTRunDelegateCurrentVersion;
    callbacks.dealloc = styledTextObjectDeallocate;
    callbacks.getAscent = styledTextObjectAscent;
    callbacks.getDescent = styledTextObjectDescent;
    callbacks.getWidth = styledTextObjectWidth;
    CTRunDelegateRef delegate = CTRunDelegateCreate(&callbacks, object);
    if (!delegate) {
        delete object;
        return;
    }

    UniChar characters[3];
    CFIndex length = 0;
    if (joinBefore) {
        characters[length++] = 0x2060;
    }
    characters[length++] = 0xFFFC;
    if (joinAfter) {
        characters[length++] = 0x2060;
    }
    CFStringRef string = CFStringCreateWithCharacters(kCFAllocatorDefault, characters, length);
    CFNumberRef tagNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &tag);
    CGColorRef clear = createSRGBColor(0, 0, 0, 0);
    const void* keys[] = {
        kCTFontAttributeName,
        styledTextTagAttributeName,
        kCTForegroundColorAttributeName,
    };
    const void* values[] = {
        styled->baseFont,
        tagNumber,
        clear,
    };
    CFDictionaryRef attributes = CFDictionaryCreate(
        kCFAllocatorDefault,
        keys,
        values,
        3,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    appendStyledTextRun(styled, string, attributes);
    CFAttributedStringSetAttribute(
        styled->text,
        CFRangeMake(CFAttributedStringGetLength(styled->text) - (joinAfter ? 2 : 1), 1),
        kCTRunDelegateAttributeName,
        delegate
    );
    CFRelease(attributes);
    CGColorRelease(clear);
    CFRelease(tagNumber);
    CFRelease(string);
    CFRelease(delegate);
}

static int64_t
styledTextRunTag(CTRunRef run) {
    CFNumberRef number = static_cast<CFNumberRef>(
        CFDictionaryGetValue(CTRunGetAttributes(run), styledTextTagAttributeName)
    );
    int64_t tag = -1;
    if (number && CFGetTypeID(number) == CFNumberGetTypeID()) {
        CFNumberGetValue(number, kCFNumberSInt64Type, &tag);
    }
    return tag;
}

static void
collectTextLayoutFragments(BxGraphicsTextLayout* layout) {
    for (size_t lineIndex = 0; lineIndex < layout->lines.size(); ++lineIndex) {
        CFArrayRef runs = CTLineGetGlyphRuns(layout->lines[lineIndex].line);
        const CFIndex count = CFArrayGetCount(runs);
        for (CFIndex index = 0; index < count; ++index) {
            CTRunRef run = static_cast<CTRunRef>(
                const_cast<void*>(CFArrayGetValueAtIndex(runs, index))
            );
            if (CTRunGetGlyphCount(run) <= 0) {
                continue;
            }
            const int64_t tag = styledTextRunTag(run);
            CGPoint origin = CGPointZero;
            CTRunGetPositions(run, CFRangeMake(0, 1), &origin);
            CGFloat ascent = 0;
            CGFloat descent = 0;
            const double width = finiteNonnegative(
                CTRunGetTypographicBounds(run, CFRangeMake(0, 0), &ascent, &descent, nullptr)
            );
            BxGraphicsTextFragment fragment;
            fragment.tag = tag;
            fragment.line = static_cast<int64_t>(lineIndex);
            fragment.x = std::isfinite(origin.x) ? origin.x : 0;
            fragment.width = width;
            fragment.ascent = finiteNonnegative(ascent);
            fragment.descent = finiteNonnegative(descent);
            if (!layout->fragments.empty()) {
                BxGraphicsTextFragment& previous = layout->fragments.back();
                const bool adjacent = std::fabs(previous.x + previous.width - fragment.x) < 0.01;
                if (previous.tag == tag && previous.line == fragment.line && adjacent) {
                    previous.width += fragment.width;
                    previous.ascent = std::max(previous.ascent, fragment.ascent);
                    previous.descent = std::max(previous.descent, fragment.descent);
                    continue;
                }
            }
            layout->fragments.push_back(fragment);
        }
    }
}

extern "C" void*
brixGraphicsStyledTextLayoutCreate(
    void* ptr,
    bool hasMaximumWidth,
    double maximumWidth,
    bool hasMaximumHeight,
    double maximumHeight,
    int32_t truncation,
    bool wordWrap,
    double lineSpacing
) {
    BxGraphicsStyledText* styled = static_cast<BxGraphicsStyledText*>(ptr);
    if (!styled) {
        return nullptr;
    }
    CFAttributedStringEndEditing(styled->text);
    textFontLastCreateMissCount = styled->fontCacheMisses;
    BxGraphicsTextLayout* layout = createTextLayoutFromAttributed(
        styled->text,
        styled->baseFont,
        hasMaximumWidth,
        maximumWidth,
        hasMaximumHeight,
        maximumHeight,
        truncation,
        wordWrap,
        lineSpacing
    );
    CFAttributedStringBeginEditing(styled->text);
    if (layout) {
        collectTextLayoutFragments(layout);
    }
    return layout;
}

static const BxGraphicsTextFragment*
getTextLayoutFragment(void* ptr, int64_t index) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    if (!layout || index < 0 || index >= static_cast<int64_t>(layout->fragments.size())) {
        return nullptr;
    }
    return &layout->fragments[static_cast<size_t>(index)];
}

extern "C" int64_t
brixGraphicsTextLayoutGetFragmentCount(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout ? static_cast<int64_t>(layout->fragments.size()) : 0;
}

extern "C" int64_t
brixGraphicsTextLayoutGetFragmentTag(void* ptr, int64_t index) {
    const BxGraphicsTextFragment* fragment = getTextLayoutFragment(ptr, index);
    return fragment ? fragment->tag : -1;
}

extern "C" int64_t
brixGraphicsTextLayoutGetFragmentLine(void* ptr, int64_t index) {
    const BxGraphicsTextFragment* fragment = getTextLayoutFragment(ptr, index);
    return fragment ? fragment->line : -1;
}

extern "C" double
brixGraphicsTextLayoutGetFragmentX(void* ptr, int64_t index) {
    const BxGraphicsTextFragment* fragment = getTextLayoutFragment(ptr, index);
    return fragment ? fragment->x : 0;
}

extern "C" double
brixGraphicsTextLayoutGetFragmentWidth(void* ptr, int64_t index) {
    const BxGraphicsTextFragment* fragment = getTextLayoutFragment(ptr, index);
    return fragment ? fragment->width : 0;
}

extern "C" double
brixGraphicsTextLayoutGetFragmentAscent(void* ptr, int64_t index) {
    const BxGraphicsTextFragment* fragment = getTextLayoutFragment(ptr, index);
    return fragment ? fragment->ascent : 0;
}

extern "C" double
brixGraphicsTextLayoutGetFragmentDescent(void* ptr, int64_t index) {
    const BxGraphicsTextFragment* fragment = getTextLayoutFragment(ptr, index);
    return fragment ? fragment->descent : 0;
}

static void
appendTextPathCommand(void* context, const CGPathElement* element) {
    auto& commands = *static_cast<std::vector<BxGraphicsTextPathCommand>*>(context);
    BxGraphicsTextPathCommand command = {};
    size_t pointCount = 0;
    switch (element->type) {
        case kCGPathElementMoveToPoint:
            command.verb = BxGraphicsTextPathMove;
            pointCount = 1;
            break;
        case kCGPathElementAddLineToPoint:
            command.verb = BxGraphicsTextPathLine;
            pointCount = 1;
            break;
        case kCGPathElementAddQuadCurveToPoint:
            command.verb = BxGraphicsTextPathQuadratic;
            pointCount = 2;
            break;
        case kCGPathElementAddCurveToPoint:
            command.verb = BxGraphicsTextPathCubic;
            pointCount = 3;
            break;
        case kCGPathElementCloseSubpath:
            command.verb = BxGraphicsTextPathClose;
            break;
    }
    for (size_t i = 0; i < pointCount; ++i) {
        command.coordinates[i * 2] = element->points[i].x;
        command.coordinates[i * 2 + 1] = element->points[i].y;
    }
    commands.push_back(command);
}

static void
ensureTextLayoutOutline(BxGraphicsTextLayout* layout) {
    if (!layout || layout->hasOutline) {
        return;
    }
    for (const BxGraphicsTextLine& line : layout->lines) {
        const CFArrayRef runs = CTLineGetGlyphRuns(line.line);
        for (CFIndex index = 0; index < CFArrayGetCount(runs); ++index) {
            const auto run = static_cast<CTRunRef>(CFArrayGetValueAtIndex(runs, index));
            const CFDictionaryRef attributes = CTRunGetAttributes(run);
            if (CFDictionaryGetValue(attributes, kCTRunDelegateAttributeName)) {
                continue;
            }
            const auto font = static_cast<CTFontRef>(
                CFDictionaryGetValue(attributes, kCTFontAttributeName)
            );
            const CFIndex count = CTRunGetGlyphCount(run);
            if (!font || count <= 0) {
                continue;
            }
            std::vector<CGGlyph> glyphs(static_cast<size_t>(count));
            std::vector<CGPoint> positions(static_cast<size_t>(count));
            CTRunGetGlyphs(run, CFRangeMake(0, count), glyphs.data());
            CTRunGetPositions(run, CFRangeMake(0, count), positions.data());
            const CGAffineTransform flip = CGAffineTransformMake(1, 0, 0, -1, 0, line.baseline);
            for (CFIndex glyph = 0; glyph < count; ++glyph) {
                CGAffineTransform transform = CTRunGetTextMatrix(run);
                transform.tx = positions[glyph].x;
                transform.ty = positions[glyph].y;
                transform = CGAffineTransformConcat(transform, flip);
                const CGPathRef path = CTFontCreatePathForGlyph(font, glyphs[glyph], &transform);
                if (path) {
                    CGPathApply(path, &layout->outline, appendTextPathCommand);
                    CGPathRelease(path);
                }
            }
        }
    }
    layout->hasOutline = true;
}

extern "C" int64_t
brixGraphicsTextLayoutGetPathCommandCount(void* ptr) {
    auto* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    ensureTextLayoutOutline(layout);
    return layout ? static_cast<int64_t>(layout->outline.size()) : 0;
}

extern "C" void
brixGraphicsTextLayoutGetPathCommands(
    void* ptr,
    uint8_t* verbs,
    double* coordinates,
    int64_t capacity
) {
    auto* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    if (!layout || !verbs || !coordinates || capacity <= 0) {
        return;
    }
    ensureTextLayoutOutline(layout);
    const auto count = std::min(static_cast<size_t>(capacity), layout->outline.size());
    for (size_t index = 0; index < count; ++index) {
        verbs[index] = layout->outline[index].verb;
        std::copy_n(layout->outline[index].coordinates, 6, coordinates + index * 6);
    }
}

extern "C" void
brixGraphicsTextLayoutRelease(void* ptr) {
    releaseTextLayout(static_cast<BxGraphicsTextLayout*>(ptr));
}

extern "C" int64_t
brixGraphicsTextFontCreatedCount() {
    return textFontCacheState().created.load(std::memory_order_relaxed);
}

extern "C" int64_t
brixGraphicsTextLayoutGetLastFontCreateMissCount() {
    return textFontLastCreateMissCount;
}

extern "C" double
brixGraphicsTextLayoutGetWidth(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout ? layout->layoutWidth : 0;
}

extern "C" double
brixGraphicsTextLayoutGetNaturalWidth(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout ? layout->naturalWidth : 0;
}

extern "C" double
brixGraphicsTextLayoutGetConstrainedWidth(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout ? layout->constrainedWidth : 0;
}

extern "C" double
brixGraphicsTextLayoutGetHeight(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout ? layout->height : 0;
}

extern "C" uint64_t
brixGraphicsTextLayoutGetSerial(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout ? layout->serial : 0;
}

extern "C" double
brixGraphicsTextLayoutGetAscent(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? line->ascent : 0;
}

extern "C" double
brixGraphicsTextLayoutGetDescent(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? line->descent : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLeading(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? line->leading : 0;
}

extern "C" double
brixGraphicsTextLayoutGetBaseline(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? line->baseline : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLastBaseline(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout && !layout->lines.empty() ? layout->lines.back().baseline : 0;
}

extern "C" int64_t
brixGraphicsTextLayoutGetLineCount(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout ? static_cast<int64_t>(layout->lines.size()) : 0;
}

extern "C" bool
brixGraphicsTextLayoutIsTruncated(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    return layout ? layout->truncated : false;
}

extern "C" double
brixGraphicsTextLayoutGetTypographicX(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? line->typographicBounds.origin.x : 0;
}

extern "C" double
brixGraphicsTextLayoutGetTypographicY(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? line->typographicBounds.origin.y : 0;
}

extern "C" double
brixGraphicsTextLayoutGetInkX(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? textLineInkBounds(*line).origin.x : 0;
}

extern "C" double
brixGraphicsTextLayoutGetInkY(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? textLineInkBounds(*line).origin.y : 0;
}

extern "C" double
brixGraphicsTextLayoutGetInkWidth(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? textLineInkBounds(*line).size.width : 0;
}

extern "C" double
brixGraphicsTextLayoutGetInkHeight(void* ptr) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, 0);
    return line ? textLineInkBounds(*line).size.height : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineTypographicX(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? line->typographicBounds.origin.x : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineTypographicY(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? line->typographicBounds.origin.y : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineWidth(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? line->width : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineHeight(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? line->height : 0;
}

extern "C" int64_t
brixGraphicsTextLayoutGetLineStart(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? static_cast<int64_t>(line->start) : 0;
}

extern "C" int64_t
brixGraphicsTextLayoutGetLineEnd(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? static_cast<int64_t>(line->end) : 0;
}

extern "C" int64_t
brixGraphicsTextLayoutGetLineForIndex(void* ptr, int64_t index) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    if (!layout || layout->lines.empty()) {
        return 0;
    }
    int64_t found = 0;
    for (size_t at = 0; at < layout->lines.size(); at += 1) {
        if (static_cast<int64_t>(layout->lines[at].start) <= index) {
            found = static_cast<int64_t>(at);
        }
    }
    return found;
}

extern "C" double
brixGraphicsTextLayoutGetXForIndex(void* ptr, int64_t index) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    if (!layout || layout->lines.empty()) {
        return 0;
    }
    const BxGraphicsTextLine* line = getTextLayoutLine(
        ptr,
        brixGraphicsTextLayoutGetLineForIndex(ptr, index)
    );
    if (!line || !line->line || line->end <= line->start) {
        return 0;
    }
    const CFIndex clamped = std::min(
        std::max(static_cast<CFIndex>(index), line->start),
        line->end
    );
    const double x = CTLineGetOffsetForStringIndex(line->line, clamped - line->base, nullptr);
    return std::isfinite(x) ? x : 0;
}

extern "C" int64_t
brixGraphicsTextLayoutGetIndexAtPoint(void* ptr, double x, double y) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    if (!layout || layout->lines.empty()) {
        return 0;
    }
    const BxGraphicsTextLine* line = &layout->lines.front();
    for (const BxGraphicsTextLine& candidate : layout->lines) {
        if (candidate.typographicBounds.origin.y <= y) {
            line = &candidate;
        }
    }
    if (!line->line || line->end <= line->start) {
        return static_cast<int64_t>(line->start);
    }
    const CFIndex local = CTLineGetStringIndexForPosition(
        line->line,
        CGPointMake(std::isfinite(x) ? x : 0, 0)
    );
    if (local == kCFNotFound) {
        return static_cast<int64_t>(line->start);
    }
    const CFIndex global = std::min(std::max(line->base + local, line->start), line->end);
    if (global == line->end && line != &layout->lines.back()) {
        const BxGraphicsTextLine* next = line + 1;
        if (next->start == line->end && global > line->start) {
            return static_cast<int64_t>(global - 1);
        }
    }
    return static_cast<int64_t>(global);
}

extern "C" double
brixGraphicsTextLayoutGetLineInkX(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? textLineInkBounds(*line).origin.x : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineInkY(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? textLineInkBounds(*line).origin.y : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineInkWidth(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? textLineInkBounds(*line).size.width : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineInkHeight(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? textLineInkBounds(*line).size.height : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineBaseline(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? line->baseline : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineAscent(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? line->ascent : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineDescent(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? line->descent : 0;
}

extern "C" double
brixGraphicsTextLayoutGetLineLeading(void* ptr, int64_t index) {
    const BxGraphicsTextLine* line = getTextLayoutLine(ptr, index);
    return line ? line->leading : 0;
}

extern "C" int64_t
brixGraphicsTextLayoutGetRunCount(void* ptr) {
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(ptr);
    if (!layout) {
        return 0;
    }
    int64_t count = 0;
    for (const BxGraphicsTextLine& line : layout->lines) {
        count += static_cast<int64_t>(CFArrayGetCount(CTLineGetGlyphRuns(line.line)));
    }
    return count;
}

extern "C" int64_t
brixGraphicsTextLayoutGetRunGlyphCount(void* ptr, int64_t runIndex) {
    CTRunRef run = getTextLayoutRun(ptr, runIndex, nullptr);
    return run ? static_cast<int64_t>(CTRunGetGlyphCount(run)) : 0;
}

extern "C" int64_t
brixGraphicsTextLayoutGetRunLine(void* ptr, int64_t runIndex) {
    int64_t lineIndex = -1;
    return getTextLayoutRun(ptr, runIndex, &lineIndex) ? lineIndex : -1;
}

extern "C" void
brixGraphicsTextLayoutGetRunGlyphs(
    void* ptr,
    int64_t runIndex,
    uint16_t* glyphs,
    int64_t capacity
) {
    CTRunRef run = getTextLayoutRun(ptr, runIndex, nullptr);
    if (!run || !glyphs || capacity <= 0) {
        return;
    }
    const CFIndex count = std::min<CFIndex>(CTRunGetGlyphCount(run), capacity);
    CTRunGetGlyphs(run, CFRangeMake(0, count), reinterpret_cast<CGGlyph*>(glyphs));
}

extern "C" void
brixGraphicsTextLayoutGetRunPositions(
    void* ptr,
    int64_t runIndex,
    double* xyPairs,
    int64_t capacity
) {
    CTRunRef run = getTextLayoutRun(ptr, runIndex, nullptr);
    if (!run || !xyPairs || capacity <= 0) {
        return;
    }
    const CFIndex count = std::min<CFIndex>(CTRunGetGlyphCount(run), capacity);
    static_assert(sizeof(CGPoint) == sizeof(double) * 2, "CGPoint export layout changed");
    CTRunGetPositions(
        run,
        CFRangeMake(0, count),
        reinterpret_cast<CGPoint*>(xyPairs)
    );
}

extern "C" void*
brixGraphicsTextLayoutGetRunFont(void* ptr, int64_t runIndex) {
    CTRunRef run = getTextLayoutRun(ptr, runIndex, nullptr);
    if (!run) {
        return nullptr;
    }
    CFDictionaryRef attributes = CTRunGetAttributes(run);
    return const_cast<void*>(CFDictionaryGetValue(attributes, kCTFontAttributeName));
}

extern "C" bool
brixGraphicsTextLayoutIsRunObject(void* ptr, int64_t runIndex) {
    CTRunRef run = getTextLayoutRun(ptr, runIndex, nullptr);
    return run && CFDictionaryGetValue(CTRunGetAttributes(run), kCTRunDelegateAttributeName);
}

extern "C" bool
brixGraphicsTextLayoutGetRunColor(void* ptr, int64_t runIndex, uint32_t* rgba8) {
    CTRunRef run = getTextLayoutRun(ptr, runIndex, nullptr);
    if (!run || !rgba8) {
        return false;
    }
    CGColorRef color = static_cast<CGColorRef>(const_cast<void*>(
        CFDictionaryGetValue(CTRunGetAttributes(run), kCTForegroundColorAttributeName)
    ));
    if (!color) {
        return false;
    }
    CGColorRef matched = CGColorCreateCopyByMatchingToColorSpace(
        getSRGBColorSpace(),
        kCGRenderingIntentDefault,
        color,
        nullptr
    );
    if (!matched) {
        return false;
    }
    uint32_t packed = 0;
    const size_t count = CGColorGetNumberOfComponents(matched);
    const CGFloat* components = CGColorGetComponents(matched);
    if (count >= 4 && components) {
        for (size_t channel = 0; channel < 4; ++channel) {
            const double value = std::min(1.0, std::max(0.0, static_cast<double>(components[channel])));
            packed |= static_cast<uint32_t>(std::lround(value * 255.0)) << (channel * 8);
        }
    }
    CGColorRelease(matched);
    *rgba8 = packed;
    return count >= 4;
}

static void
drawTextLines(
    CGContextRef context,
    const BxGraphicsTextLayout* layout,
    double x,
    double y,
    double width,
    double height,
    int32_t alignment
) {
    double offsetY = (height - layout->height) / 2.0;
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    CGContextTranslateCTM(context, x, y + height);
    CGContextScaleCTM(context, 1, -1);
    for (const BxGraphicsTextLine& line : layout->lines) {
        double offsetX = 0;
        if (alignment == 1) {
            offsetX = (width - line.width) / 2.0;
        } else if (alignment == 2) {
            offsetX = width - line.width;
        }
        const double baselineFromBottom = height - offsetY - line.baseline;
        CGContextSetTextPosition(context, offsetX, baselineFromBottom);
        CTLineDraw(line.line, context);
    }
}

extern "C" void
brixGraphicsDrawTextLayout(
    void* canvasPtr,
    void* layoutPtr,
    double x,
    double y,
    double width,
    double height,
    int32_t alignment,
    double red,
    double green,
    double blue,
    double alpha
) {
    BxGraphicsCanvas* canvas = getCanvas(canvasPtr);
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(layoutPtr);
    if (!canvas || !canvas->context || !layout || layout->lines.empty() ||
        width <= 0 || height <= 0) {
        return;
    }
    invalidateUserRect(canvas, CGRectMake(x, y, width, height));

    CGContextSaveGState(canvas->context);
    CGContextClipToRect(canvas->context, CGRectMake(x, y, width, height));
    setCanvasFillColor(canvas, red, green, blue, alpha);
    drawTextLines(canvas->context, layout, x, y, width, height, alignment);
    CGContextRestoreGState(canvas->context);
}

extern "C" void
brixGraphicsDrawTextLayoutShader(
    void* canvasPtr,
    void* layoutPtr,
    double x,
    double y,
    double width,
    double height,
    int32_t alignment,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double time
) {
    BxGraphicsCanvas* canvas = getCanvas(canvasPtr);
    BxGraphicsTextLayout* layout = static_cast<BxGraphicsTextLayout*>(layoutPtr);
    if (!canvas || !canvas->context || !layout || layout->lines.empty() ||
        width <= 0 || height <= 0 || (!args && argCount)) {
        return;
    }
    CGContextRef context = canvas->context;
    CGRect rect = CGRectMake(x, y, width, height);
    BxGraphicsShaderArea area;
    if (!shaderArea(canvas, rect, area)) {
        return;
    }
    const size_t columns = static_cast<size_t>(area.columns);
    const size_t rows = static_cast<size_t>(area.rows);
    void* coverage = calloc(columns * rows, 1);
    if (!coverage) {
        return;
    }
    CGContextRef mask = CGBitmapContextCreate(coverage, columns, rows, 8, columns, nullptr, kCGImageAlphaOnly);
    if (!mask) {
        free(coverage);
        return;
    }
    CGContextSetAllowsFontSmoothing(mask, false);
    CGContextSetShouldSmoothFonts(mask, false);
    CGContextConcatCTM(
        mask,
        CGAffineTransformConcat(area.device, CGAffineTransformMakeTranslation(-area.left, -area.bottom))
    );
    CGContextSetRGBFillColor(mask, 1, 1, 1, 1);
    drawTextLines(mask, layout, x, y, width, height, alignment);
    CGContextRelease(mask);
    CGDataProviderRef provider = CGDataProviderCreateWithData(nullptr, coverage, columns * rows, releaseMaskBytes);
    if (!provider) {
        free(coverage);
        return;
    }
    CGImageRef clip = CGImageCreate(
        columns,
        rows,
        8,
        8,
        columns,
        getDeviceGrayColorSpace(),
        kCGImageAlphaNone,
        provider,
        nullptr,
        false,
        kCGRenderingIntentDefault
    );
    CGDataProviderRelease(provider);
    if (!clip) {
        return;
    }
    const double frameWidth = layout->layoutWidth;
    const double frameHeight = layout->height;
    double frameX = x;
    if (alignment == 1) {
        frameX = x + (width - frameWidth) / 2.0;
    } else if (alignment == 2) {
        frameX = x + width - frameWidth;
    }
    CGRect frame = CGRectMake(frameX, y + (height - frameHeight) / 2.0, frameWidth, frameHeight);
    CGImageRef image = shaderAreaImage(canvas, area, frame, shader, args, argCount, time);
    if (!image) {
        CGImageRelease(clip);
        return;
    }
    invalidateUserRect(canvas, rect);
    CGRect pixels = CGRectMake(area.left, area.bottom, area.columns, area.rows);
    CGContextSaveGState(context);
    CGContextClipToRect(context, rect);
    CGContextConcatCTM(context, area.inverse);
    CGContextClipToMask(context, pixels, clip);
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextDrawImage(context, pixels, image);
    CGContextRestoreGState(context);
    CGImageRelease(image);
    CGImageRelease(clip);
}

extern "C" void*
brixGraphicsFontRegister(const char* path) {
    if (!path || !path[0]) {
        return nullptr;
    }
    struct stat fileInfo;
    if (stat(path, &fileInfo) != 0 || !S_ISREG(fileInfo.st_mode)) {
        return nullptr;
    }
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(path),
        static_cast<CFIndex>(strlen(path)),
        false
    );
    if (!url) {
        return nullptr;
    }

    CFErrorRef error = nullptr;
    if (!CFURLResourceIsReachable(url, &error)) {
        if (error) {
            CFRelease(error);
        }
        CFRelease(url);
        return nullptr;
    }
    if (!CTFontManagerRegisterFontsForURL(url, kCTFontManagerScopeProcess, &error)) {
        if (error) {
            CFRelease(error);
        }
        CFRelease(url);
        return nullptr;
    }

    std::string family;
    CFArrayRef descriptors = CTFontManagerCreateFontDescriptorsFromURL(url);
    if (descriptors && CFArrayGetCount(descriptors) > 0) {
        CTFontDescriptorRef descriptor = static_cast<CTFontDescriptorRef>(
            const_cast<void*>(CFArrayGetValueAtIndex(descriptors, 0))
        );
        CFTypeRef value = CTFontDescriptorCopyAttribute(
            descriptor,
            kCTFontFamilyNameAttribute
        );
        if (value && CFGetTypeID(value) == CFStringGetTypeID()) {
            family = utf8String(static_cast<CFStringRef>(value));
        }
        if (value) {
            CFRelease(value);
        }
    }
    if (descriptors) {
        CFRelease(descriptors);
    }
    if (family.empty()) {
        CTFontManagerUnregisterFontsForURL(url, kCTFontManagerScopeProcess, nullptr);
        CFRelease(url);
        return nullptr;
    }

    BxGraphicsFontRegistration* registration = new BxGraphicsFontRegistration();
    registration->url = url;
    registration->family = family;
    return registration;
}

extern "C" void
brixGraphicsFontRegistrationRelease(void* ptr) {
    BxGraphicsFontRegistration* registration = static_cast<BxGraphicsFontRegistration*>(ptr);
    if (!registration) {
        return;
    }
    TextFontCacheState& cache = textFontCacheState();
    std::lock_guard<std::mutex> lock(cache.mutex);
    flushTextFontCacheFamilyLocked(cache, registration->family);
    CFErrorRef error = nullptr;
    CTFontManagerUnregisterFontsForURL(
        registration->url,
        kCTFontManagerScopeProcess,
        &error
    );
    if (error) {
        CFRelease(error);
    }
    CFRelease(registration->url);
    delete registration;
}

extern "C" bool
brixGraphicsFontRegistrationIsValid(void* ptr) {
    return ptr != nullptr;
}

extern "C" const char*
brixGraphicsFontRegistrationGetFamily(void* ptr) {
    BxGraphicsFontRegistration* registration = static_cast<BxGraphicsFontRegistration*>(ptr);
    return registration ? registration->family.c_str() : "";
}

extern "C" void
brixGraphicsDrawPath(void* ptr, bool fill, bool stroke, int32_t fillRule) {
    drawCurrentPath(fill || stroke ? getCanvas(ptr) : getStateCanvas(ptr), fill, stroke, fillRule == 1);
}

static BxGraphicsPaintQueue*
getPaintQueue(BxGraphicsCanvas* canvas) {
    if (!canvas->paintQueue) {
        void* batch = bxGraphicsShaderBatchCreate();
        if (!batch) {
            return nullptr;
        }
        canvas->paintQueue = new BxGraphicsPaintQueue{batch, {}};
    }
    return canvas->paintQueue;
}

static void
flushCanvasPaints(BxGraphicsCanvas* canvas) {
    BxGraphicsPaintQueue* queue = canvas->paintQueue;
    if (!queue || queue->paints.empty()) {
        return;
    }
    CGContextRef context = canvas->context;
    if (context && bxGraphicsShaderBatchFinish(queue->batch)) {
        const bool clipped = std::any_of(
            queue->paints.begin(),
            queue->paints.end(),
            [](const BxGraphicsQueuedPaint& paint) { return paint.clip != nullptr; }
        );
        CGPathRef path = clipped && !CGContextIsPathEmpty(context) ? CGContextCopyPath(context) : nullptr;
        CGAffineTransform inverse = CGAffineTransformInvert(CGContextGetUserSpaceToDeviceSpaceTransform(context));
        CGColorSpaceRef colorSpace = canvas->namedSpace ? canvas->colorSpace : getSRGBColorSpace();
        for (const BxGraphicsQueuedPaint& paint : queue->paints) {
            CGImageRef image = static_cast<CGImageRef>(bxGraphicsShaderBatchImage(queue->batch, paint.job, colorSpace));
            if (!image) {
                continue;
            }
            CGContextSaveGState(context);
            CGContextConcatCTM(context, inverse);
            if (paint.clip) {
                CGContextBeginPath(context);
                CGContextAddPath(context, paint.clip);
                if (paint.evenOdd) {
                    CGContextEOClip(context);
                } else {
                    CGContextClip(context);
                }
            }
            CGContextSetInterpolationQuality(context, kCGInterpolationNone);
            CGContextDrawImage(context, paint.area, image);
            CGContextRestoreGState(context);
            CGImageRelease(image);
        }
        if (path) {
            CGContextBeginPath(context);
            CGContextAddPath(context, path);
            CGPathRelease(path);
        }
    }
    clearPaintQueue(queue);
}

static void
queuePaint(BxGraphicsCanvas* canvas, int64_t job, CGPathRef clip, bool evenOdd, CGRect area) {
    BxGraphicsPaintQueue* queue = canvas->paintQueue;
    queue->paints.push_back(BxGraphicsQueuedPaint{job, clip, evenOdd, area});
    if (canvas->surface || queue->paints.size() >= bxGraphicsPaintQueueLimit ||
            bxGraphicsShaderBatchBytes(queue->batch) >= bxGraphicsPaintQueueBytes) {
        flushCanvasPaints(canvas);
    }
}

static bool
pathRaster(
    BxGraphicsCanvas* canvas,
    CGRect bounds,
    double& left,
    double& bottom,
    int32_t& columns,
    int32_t& rows,
    double* local,
    double* paper,
    double& pixel
) {
    CGContextRef context = canvas->context;
    CGAffineTransform device = CGContextGetUserSpaceToDeviceSpaceTransform(context);
    CGRect area = CGRectIntersection(
        CGRectApplyAffineTransform(bounds, device),
        CGRectApplyAffineTransform(CGContextGetClipBoundingBox(context), device)
    );
    area = CGRectIntersection(area, CGRectMake(0, 0, canvas->pixelWidth, canvas->pixelHeight));
    if (CGRectIsNull(area) || CGRectIsEmpty(area)) {
        return false;
    }
    left = std::floor(CGRectGetMinX(area));
    bottom = std::floor(CGRectGetMinY(area));
    double right = std::ceil(CGRectGetMaxX(area));
    double top = std::ceil(CGRectGetMaxY(area));
    columns = static_cast<int32_t>(right - left);
    rows = static_cast<int32_t>(top - bottom);
    CGAffineTransform raster = CGAffineTransformConcat(
        CGAffineTransformMake(1, 0, 0, -1, left, top),
        CGAffineTransformInvert(device)
    );
    local[0] = raster.a;
    local[1] = raster.b;
    local[2] = raster.c;
    local[3] = raster.d;
    local[4] = raster.tx;
    local[5] = raster.ty;
    double scale = canvas->scale > 0 ? canvas->scale : 1;
    paper[0] = 1 / scale;
    paper[1] = -1 / scale;
    paper[2] = (canvas->absoluteLeft + left) / scale;
    paper[3] = (canvas->absoluteTop + top) / scale;
    pixel = std::sqrt(std::fabs(raster.a * raster.d - raster.b * raster.c));
    return true;
}

extern "C" void
brixGraphicsFillPathPaint(
    void* ptr,
    uint32_t shader,
    const double* args,
    int64_t argCount,
    double red,
    double green,
    double blue,
    double alpha,
    int32_t fillRule
) {
    BxGraphicsCanvas* canvas = getStateCanvas(ptr);
    if (!canvas || !canvas->context || (!args && argCount)) {
        return;
    }
    CGContextRef context = canvas->context;
    if (CGContextIsPathEmpty(context)) {
        return;
    }
    CGPathRef path = CGContextCopyPath(context);
    double left = 0;
    double bottom = 0;
    int32_t columns = 0;
    int32_t rows = 0;
    double local[6];
    double paper[4];
    double pixel = 1;
    CGRect bounds = CGPathGetPathBoundingBox(path);
    BxGraphicsPaintQueue* queue = getPaintQueue(canvas);
    if (queue && pathRaster(canvas, bounds, left, bottom, columns, rows, local, paper, pixel)) {
        const double color[4] = {red, green, blue, alpha};
        int64_t job = bxGraphicsShaderBatchPaint(
            queue->batch,
            shader,
            columns,
            rows,
            local,
            paper,
            pixel,
            color,
            args,
            argCount,
            canvas->format,
            canvas->namedSpace ? canvas->space : BxGraphicsSpaceSRGB
        );
        if (job >= 0) {
            invalidateUserRect(canvas, bounds);
            CGAffineTransform device = CGContextGetUserSpaceToDeviceSpaceTransform(context);
            queuePaint(
                canvas,
                job,
                CGPathCreateCopyByTransformingPath(path, &device),
                fillRule == 1,
                CGRectMake(left, bottom, columns, rows)
            );
        }
    }
    CGContextBeginPath(context);
    CGContextAddPath(context, path);
    CGPathRelease(path);
}

static void
brushFlattenCubic(std::vector<double>& points, const double* p, double tolerance) {
    double ax = p[0] - 2 * p[2] + p[4];
    double ay = p[1] - 2 * p[3] + p[5];
    double bx = p[2] - 2 * p[4] + p[6];
    double by = p[3] - 2 * p[5] + p[7];
    double bend = std::sqrt(std::max(ax * ax + ay * ay, bx * bx + by * by));
    int steps = static_cast<int>(std::ceil(std::sqrt(0.75 * bend / tolerance)));
    steps = std::max(1, std::min(steps, 512));
    for (int step = 1; step <= steps; ++step) {
        double t = static_cast<double>(step) / steps;
        double u = 1 - t;
        double w0 = u * u * u;
        double w1 = 3 * u * u * t;
        double w2 = 3 * u * t * t;
        double w3 = t * t * t;
        points.push_back(w0 * p[0] + w1 * p[2] + w2 * p[4] + w3 * p[6]);
        points.push_back(w0 * p[1] + w1 * p[3] + w2 * p[5] + w3 * p[7]);
    }
}

static void
brushEmitSubpath(
    const std::vector<double>& points,
    double step,
    double& along,
    BxGraphicsDab* dabs,
    int64_t capacity,
    int64_t& count
) {
    size_t vertices = points.size() / 2;
    if (vertices < 2) {
        return;
    }
    const double* p = points.data();
    double length = 0;
    for (size_t index = 1; index < vertices; ++index) {
        double dx = p[index * 2] - p[index * 2 - 2];
        double dy = p[index * 2 + 1] - p[index * 2 - 1];
        length += std::sqrt(dx * dx + dy * dy);
    }
    float subpath = static_cast<float>(length);
    auto emit = [&](double x, double y, double angle, double distance) {
        if (count < capacity) {
            dabs[count] = BxGraphicsDab{
                static_cast<float>(x),
                static_cast<float>(y),
                static_cast<float>(angle),
                static_cast<float>(distance),
                subpath,
                static_cast<float>(along + distance),
            };
        }
        ++count;
    };
    if (!(length > 0)) {
        emit(p[0], p[1], 0, 0);
        return;
    }
    double next = 0;
    double walked = 0;
    double last = 0;
    double lastDx = 1;
    double lastDy = 0;
    for (size_t index = 1; index < vertices; ++index) {
        double x0 = p[index * 2 - 2];
        double y0 = p[index * 2 - 1];
        double dx = p[index * 2] - x0;
        double dy = p[index * 2 + 1] - y0;
        double segment = std::sqrt(dx * dx + dy * dy);
        if (!(segment > 0)) {
            continue;
        }
        lastDx = dx;
        lastDy = dy;
        if (next <= walked + segment) {
            double angle = std::atan2(dy, dx);
            double scale = 1 / segment;
            do {
                double t = (next - walked) * scale;
                emit(x0 + dx * t, y0 + dy * t, angle, next);
                last = next;
                next += step;
            } while (next <= walked + segment);
        }
        walked += segment;
    }
    if (length - last > step * 0.5) {
        emit(p[vertices * 2 - 2], p[vertices * 2 - 1], std::atan2(lastDy, lastDx), length);
    }
    along += length;
}

struct BrushPathVerbs {
    std::vector<uint8_t> verbs;
    std::vector<double> coords;
    double current[2] = {0, 0};
};

static void
brushPathElement(void* info, const CGPathElement* element) {
    BrushPathVerbs& path = *static_cast<BrushPathVerbs*>(info);
    const CGPoint* points = element->points;
    switch (element->type) {
    case kCGPathElementMoveToPoint:
        path.verbs.push_back(0);
        path.coords.insert(path.coords.end(), {points[0].x, points[0].y});
        path.current[0] = points[0].x;
        path.current[1] = points[0].y;
        break;
    case kCGPathElementAddLineToPoint:
        path.verbs.push_back(1);
        path.coords.insert(path.coords.end(), {points[0].x, points[0].y});
        path.current[0] = points[0].x;
        path.current[1] = points[0].y;
        break;
    case kCGPathElementAddQuadCurveToPoint:
        path.verbs.push_back(2);
        path.coords.insert(path.coords.end(), {
            path.current[0] + (points[0].x - path.current[0]) * 2 / 3,
            path.current[1] + (points[0].y - path.current[1]) * 2 / 3,
            points[1].x + (points[0].x - points[1].x) * 2 / 3,
            points[1].y + (points[0].y - points[1].y) * 2 / 3,
            points[1].x,
            points[1].y,
        });
        path.current[0] = points[1].x;
        path.current[1] = points[1].y;
        break;
    case kCGPathElementAddCurveToPoint:
        path.verbs.push_back(2);
        path.coords.insert(path.coords.end(), {
            points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y,
        });
        path.current[0] = points[2].x;
        path.current[1] = points[2].y;
        break;
    case kCGPathElementCloseSubpath:
        path.verbs.push_back(3);
        break;
    }
}

extern "C" int64_t
brixGraphicsBrushDabs(
    const uint8_t* verbs,
    int64_t verbCount,
    const double* coords,
    bool close,
    double width,
    double spacing,
    BxGraphicsDab* dabs,
    int64_t capacity,
    double* total
) {
    const int64_t limit = 1 << 21;
    double size = std::max(std::fabs(width), 1e-3);
    double step = std::max(spacing, 0.01) * size;
    double tolerance = std::max(std::min(size * 0.05, 0.25), 1e-3);
    double reach = 0;
    size_t coord = 0;
    double previous[2] = {0, 0};
    for (int64_t index = 0; index < verbCount; ++index) {
        int points = verbs[index] == 0 || verbs[index] == 1 ? 1 : verbs[index] == 2 ? 3 : 0;
        for (int point = 0; point < points; ++point) {
            if (verbs[index] != 0 || point) {
                double dx = coords[coord] - previous[0];
                double dy = coords[coord + 1] - previous[1];
                reach += std::sqrt(dx * dx + dy * dy);
            }
            previous[0] = coords[coord];
            previous[1] = coords[coord + 1];
            coord += 2;
        }
    }
    step = std::max(step, reach / limit);
    std::vector<double> points;
    double start[2] = {0, 0};
    double current[2] = {0, 0};
    double along = 0;
    int64_t count = 0;
    coord = 0;
    for (int64_t index = 0; index < verbCount; ++index) {
        switch (verbs[index]) {
        case 0:
            brushEmitSubpath(points, step, along, dabs, capacity, count);
            points.clear();
            start[0] = current[0] = coords[coord];
            start[1] = current[1] = coords[coord + 1];
            points.push_back(current[0]);
            points.push_back(current[1]);
            coord += 2;
            break;
        case 1:
            if (points.empty()) {
                points.push_back(current[0]);
                points.push_back(current[1]);
            }
            current[0] = coords[coord];
            current[1] = coords[coord + 1];
            points.push_back(current[0]);
            points.push_back(current[1]);
            coord += 2;
            break;
        case 2: {
            if (points.empty()) {
                points.push_back(current[0]);
                points.push_back(current[1]);
            }
            const double curve[8] = {
                current[0], current[1],
                coords[coord], coords[coord + 1],
                coords[coord + 2], coords[coord + 3],
                coords[coord + 4], coords[coord + 5],
            };
            brushFlattenCubic(points, curve, tolerance);
            current[0] = coords[coord + 4];
            current[1] = coords[coord + 5];
            coord += 6;
            break;
        }
        case 3:
            if (!points.empty()) {
                points.push_back(start[0]);
                points.push_back(start[1]);
            }
            brushEmitSubpath(points, step, along, dabs, capacity, count);
            points.clear();
            current[0] = start[0];
            current[1] = start[1];
            break;
        default:
            break;
        }
    }
    if (close && !points.empty()) {
        points.push_back(start[0]);
        points.push_back(start[1]);
    }
    brushEmitSubpath(points, step, along, dabs, capacity, count);
    if (total) {
        *total = along;
    }
    return count;
}

extern "C" void
brixGraphicsStrokePathBrush(
    void* ptr,
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
) {
    const int64_t params = 12;
    BxGraphicsCanvas* canvas = fill ? getCanvas(ptr) : getStateCanvas(ptr);
    if (!canvas || !canvas->context || (!args && argCount)) {
        return;
    }
    CGContextRef context = canvas->context;
    if (CGContextIsPathEmpty(context)) {
        return;
    }
    CGPathRef path = CGContextCopyPath(context);
    drawCurrentPath(canvas, fill, false, fillRule == 1);
    CGContextBeginPath(context);
    if (argCount >= params && width > 0 && alpha > 0 && args[10] > 0) {
        BrushPathVerbs verbs;
        CGPathApply(path, &verbs, brushPathElement);
        std::vector<BxGraphicsDab> dabs(256);
        double total = 0;
        int64_t count = brixGraphicsBrushDabs(
            verbs.verbs.data(), static_cast<int64_t>(verbs.verbs.size()), verbs.coords.data(),
            false, width, args[0], dabs.data(), static_cast<int64_t>(dabs.size()), &total
        );
        if (count > static_cast<int64_t>(dabs.size())) {
            dabs.resize(static_cast<size_t>(count));
            count = brixGraphicsBrushDabs(
                verbs.verbs.data(), static_cast<int64_t>(verbs.verbs.size()), verbs.coords.data(),
                false, width, args[0], dabs.data(), count, &total
            );
        }
        if (args[10] < 1) {
            float limit = static_cast<float>(args[10] * total);
            count = std::upper_bound(
                dabs.begin(),
                dabs.begin() + count,
                limit,
                [](float value, const BxGraphicsDab& dab) { return value < dab.along; }
            ) - dabs.begin();
        }
        double outset = width * (0.5 * (1 + std::fabs(args[2])) + 1.2 * std::fabs(args[1])) + 1;
        CGRect bounds = CGRectInset(CGPathGetPathBoundingBox(path), -outset, -outset);
        double left = 0;
        double bottom = 0;
        int32_t columns = 0;
        int32_t rows = 0;
        double local[6];
        double paper[4];
        double pixel = 1;
        BxGraphicsPaintQueue* queue = count > 0 ? getPaintQueue(canvas) : nullptr;
        if (queue && pathRaster(canvas, bounds, left, bottom, columns, rows, local, paper, pixel)) {
            CGAffineTransform toRaster = CGAffineTransformInvert(
                CGAffineTransformMake(local[0], local[1], local[2], local[3], local[4], local[5])
            );
            const double raster[6] = {toRaster.a, toRaster.b, toRaster.c, toRaster.d, toRaster.tx, toRaster.ty};
            const double color[4] = {red, green, blue, alpha};
            int64_t job = bxGraphicsShaderBatchDabs(
                queue->batch,
                shader,
                columns,
                rows,
                raster,
                paper,
                color,
                args,
                width,
                total,
                dabs.data(),
                count,
                args + params,
                argCount - params,
                canvas->format,
                canvas->namedSpace ? canvas->space : BxGraphicsSpaceSRGB
            );
            if (job >= 0) {
                invalidateUserRect(canvas, bounds);
                queuePaint(canvas, job, nullptr, false, CGRectMake(left, bottom, columns, rows));
            }
        }
    }
    CGPathRelease(path);
}
