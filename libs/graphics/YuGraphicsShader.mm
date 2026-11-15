#include "YuGraphics/YuGraphics.h"
#include "YuGraphicsColor.h"

#import <Metal/Metal.h>
#import <objc/runtime.h>
#include <CoreGraphics/CoreGraphics.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

static const int32_t kShaderImageLimit = 8;
static const int32_t kGPUTextureSizeLimit = 16384;

struct YuGraphicsShaderKernel {
    id<MTLComputePipelineState> pipeline = nil;
    bool tried = false;
};

struct YuGraphicsShaderEntry {
    std::string source;
    std::string error;
    bool validated = false;
    YuGraphicsShaderKernel raster;
    YuGraphicsShaderKernel filter;
    YuGraphicsShaderKernel render;
    YuGraphicsShaderKernel paint;
    id<MTLRenderPipelineState> dabPipelines[2] = {nil, nil};
    bool dabTried[2] = {false, false};
};

struct YuGraphicsShaderRasterParams {
    float a, b, c, d, tx, ty;
    float width, height;
    uint32_t columns, rows, argCount, imageCount;
    uint32_t format, target, clamped;
    float time;
};

struct YuGraphicsShaderFilterParams {
    float origin[2];
    float size[2];
    float scale;
    uint32_t columns, rows, argCount, imageCount;
    uint32_t format, target, clamped;
    float time;
};

struct YuGraphicsShaderPaintParams {
    float a, b, c, d, tx, ty;
    float paperScaleX, paperScaleY, paperX, paperY;
    float pixel;
    float padding0;
    float color[4];
    uint32_t columns, rows, argCount, format, target, clamped;
    uint32_t padding2, padding3;
};
static_assert(sizeof(YuGraphicsShaderPaintParams) == 96, "Paint params layout");

struct YuGraphicsShaderDabStyle {
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

struct YuGraphicsShaderDabParams {
    float a, b, c, d, tx, ty;
    float paperScaleX, paperScaleY, paperX, paperY;
    float color[4];
    YuGraphicsShaderDabStyle style;
    uint32_t columns, rows, argCount, target, clamped, padding;
};
static_assert(sizeof(YuGraphicsShaderDabParams) == 128, "Dab params layout");

struct YuGraphicsShaderDabRecord {
    float x, y, angle, distance, length, along;
    uint32_t draw, index;
};

struct YuGraphicsShaderRenderParams {
    uint32_t origin[2];
    uint32_t extent[2];
    float size[2];
    float scale;
    uint32_t argCount, imageCount, target, clamped, padding;
};

struct YuGraphicsGPUStore {
    id<MTLTexture> texture = nil;
    size_t bytesPerPixel = 4;
    bool mipsStale = true;
};

static std::mutex yuGraphicsShaderMutex;
static std::vector<YuGraphicsShaderEntry*> yuGraphicsShaders;
static std::unordered_map<std::string, uint32_t> yuGraphicsShaderIds;
static std::once_flag yuGraphicsShaderDeviceOnce;
static id<MTLDevice> yuGraphicsShaderDevice = nil;
static std::mutex yuGraphicsShaderQueueMutex;
static id<MTLCommandQueue> yuGraphicsShaderQueue = nil;
static std::atomic<size_t> yuGraphicsShaderCommandsAlive {0};
static char yuGraphicsShaderCommandTokenKey;
static id<MTLTexture> yuGraphicsShaderBlank = nil;

@interface YuGraphicsShaderCommandToken : NSObject
@end

@implementation YuGraphicsShaderCommandToken
- (void)dealloc {
    yuGraphicsShaderCommandsAlive.fetch_sub(1, std::memory_order_release);
    [super dealloc];
}
@end

static const char* yuGraphicsShaderIncludes = R"METAL(
#include <metal_stdlib>
using namespace metal;
)METAL";

static const char* yuGraphicsShaderColorFunctions = R"METAL(
constant uint kYumiSpacePrimaries[5] = {0u, 1u, 0u, 1u, 2u};
constant uint kYumiSpaceEncoded[5] = {1u, 1u, 0u, 0u, 0u};

float3 yumiDecodeSRGB(float3 value) {
    float3 magnitude = abs(value);
    float3 linear = select(pow((magnitude + 0.055) / 1.055, 2.4), magnitude / 12.92, magnitude <= 0.04045);
    return sign(value) * linear;
}

float3 yumiEncodeSRGB(float3 value) {
    float3 magnitude = abs(value);
    float3 encoded = select(1.055 * pow(magnitude, 1.0 / 2.4) - 0.055, magnitude * 12.92, magnitude <= 0.0031308);
    return sign(value) * encoded;
}

float3 yumiLinearColor(float3 rgb, uint source, uint target) {
    uint from = min(source, 4u);
    uint to = min(target, 4u);
    if (kYumiSpaceEncoded[from] != 0u) {
        rgb = yumiDecodeSRGB(rgb);
    }
    return kYumiPrimaryConversions[kYumiSpacePrimaries[from] * 3u + kYumiSpacePrimaries[to]] * rgb;
}

float3 yumiEncodeColor(float3 rgb, uint target) {
    return kYumiSpaceEncoded[min(target, 4u)] != 0u ? yumiEncodeSRGB(rgb) : rgb;
}

float3 yumiConvertColor(float3 rgb, uint source, uint target) {
    return source == target ? rgb : yumiEncodeColor(yumiLinearColor(rgb, source, target), target);
}
)METAL";

static const char* yuGraphicsShaderTypes = R"METAL(
struct ShaderImage {
    texture2d<float> texture;
    float2 size;
    float scale;
    uint space;
    uint target;

    float4 sample(float2 position) const {
        constexpr sampler pixels(coord::normalized, address::clamp_to_edge, filter::linear);
        float2 extent = float2(texture.get_width(), texture.get_height());
        float4 color = texture.sample(pixels, position * scale / extent, level(0.0));
        if (color.a <= 0.0) {
            return float4(0.0);
        }
        return float4(yumiLinearColor(color.rgb / color.a, space, target), color.a);
    }
};

struct ShaderLayer {
    texture2d<float> texture;
    float2 origin;
    float scale;
    float2 size;
    uint space;

    float4 sample(float2 position) const {
        constexpr sampler pixels(coord::pixel, address::clamp_to_zero, filter::linear);
        float4 color = texture.sample(pixels, origin + position * scale);
        if (color.a <= 0.0) {
            return float4(0.0);
        }
        return float4(yumiLinearColor(color.rgb / color.a, space, space), color.a);
    }
};
)METAL";

static const char* yuGraphicsShaderInputTypes = R"METAL(
struct PaintInput {
    float2 position;
    float2 paper;
    float4 color;
    float pixel;
};

struct DabInput {
    float2 offset;
    float2 position;
    float2 paper;
    float4 color;
    float along;
    float distance;
    float pressure;
    float random;
    float pixel;
};
)METAL";

static const char* yuGraphicsShaderDabSource = R"METAL(
struct DabRecord {
    float x;
    float y;
    float angle;
    float distance;
    float length;
    float along;
    uint draw;
    uint index;
};

struct DabStyle {
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
    uint fixedAngle;
};

struct DabPlacement {
    float2 local;
    float2 offset;
    float pressure;
    float random;
    float along;
    float radius;
};

static float dabHash(uint index, uint seed, uint lane) {
    uint x = index * 0x9e3779b9u ^ seed * 0x85ebca6bu ^ lane * 0xc2b2ae35u;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return float(x >> 8) * (1.0 / 16777216.0);
}

static DabPlacement dabPlace(DabRecord dab, DabStyle style, uint vertexId) {
    uint seed = uint(int(floor(style.seed * 7.0 + 0.5)));
    float size = dabHash(dab.index, seed, 0u) * 2.0 - 1.0;
    float turn = dabHash(dab.index, seed, 1u) * 2.0 - 1.0;
    float across = dabHash(dab.index, seed, 2u) * 2.0 - 1.0;
    float ahead = dabHash(dab.index, seed, 3u) * 2.0 - 1.0;
    float pressure = 1.0;
    if (style.taperStart > 0.0) {
        pressure = min(pressure, dab.distance / (style.taperStart * dab.length));
    }
    if (style.taperEnd > 0.0) {
        pressure = min(pressure, (dab.length - dab.distance) / (style.taperEnd * dab.length));
    }
    pressure = saturate(pressure);
    pressure = pressure * (2.0 - pressure);
    float radius = style.width * 0.5 * max(1.0 + style.sizeJitter * size, 0.05) * mix(0.1, 1.0, pressure);
    float2 tangent = float2(cos(dab.angle), sin(dab.angle));
    float2 normal = float2(-tangent.y, tangent.x);
    float2 center = float2(dab.x, dab.y) + style.width * style.scatter * (across * normal + ahead * 0.5 * tangent);
    float angle = (style.fixedAngle != 0u ? style.angle : dab.angle) + style.angleJitter * turn;
    float2 axis = float2(cos(angle), sin(angle));
    float2 side = float2(-axis.y, axis.x);
    float2 corner = float2(float(vertexId & 1u), float(vertexId >> 1u)) * 2.0 - 1.0;
    float2 reach = float2(radius, radius * clamp(style.roundness, 0.05, 1.0));
    DabPlacement placement;
    placement.local = center + axis * (corner.x * reach.x) + side * (corner.y * reach.y);
    placement.offset = corner;
    placement.pressure = pressure;
    placement.random = dabHash(dab.index, seed, 4u);
    placement.along = style.total > 0.0 ? dab.along / style.total : 0.0;
    placement.radius = min(reach.x, reach.y);
    return placement;
}
)METAL";

static const char* yuGraphicsShaderArgsType = R"METAL(
struct ShaderArgs {
    device const float* values;
    uint count;
    array<texture2d<float>, 8> textures;
    constant float4* frames;
    uint imageCount;
    uint target;
    float time;

    float operator[](uint index) const {
        return index < count ? values[index] : 0.0;
    }

    float4 color(uint index) const {
        float4 rgba = float4(
            index < count ? values[index] : 0.0,
            index + 1u < count ? values[index + 1u] : 0.0,
            index + 2u < count ? values[index + 2u] : 0.0,
            index + 3u < count ? values[index + 3u] : 0.0
        );
        return float4(yumiLinearColor(rgba.rgb, 0u, target), rgba.a);
    }

    ShaderImage image(uint index) const {
        uint slot = min(index, 7u);
        float4 frame = index < imageCount ? frames[slot] : float4(0.0);
        return ShaderImage{textures[slot], frame.xy, frame.z, uint(frame.w + 0.5), target};
    }
};

float4 yumiShaderOutput(float4 color, uint target, uint clamped) {
    float alpha = saturate(color.a);
    float3 rgb = yumiEncodeColor(color.rgb, target);
    if (clamped != 0u) {
        rgb = saturate(rgb);
    }
    return float4(rgb * alpha, alpha);
}
)METAL";

static const char* yuGraphicsShaderRasterKernel = R"METAL(
float4 shade(float2 position, float2 size, ShaderArgs args);

struct YumiShaderRasterParams {
    float a, b, c, d, tx, ty;
    float width, height;
    uint columns, rows, argCount, imageCount;
    uint format, target, clamped;
    float time;
};

kernel void yumiShaderKernel(
    device uchar* pixels [[buffer(0)]],
    constant YumiShaderRasterParams& params [[buffer(1)]],
    device const float* args [[buffer(2)]],
    constant float4* frames [[buffer(3)]],
    array<texture2d<float>, 8> images [[texture(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.columns || gid.y >= params.rows) {
        return;
    }
    float2 pixel = float2(gid) + 0.5;
    float2 position = float2(
        params.a * pixel.x + params.c * pixel.y + params.tx,
        params.b * pixel.x + params.d * pixel.y + params.ty
    );
    ShaderArgs shaderArgs = {args, params.argCount, images, frames, params.imageCount, params.target, params.time};
    float4 color = yumiShaderOutput(
        shade(position, float2(params.width, params.height), shaderArgs),
        params.target,
        params.clamped
    );
    uint index = gid.y * params.columns + gid.x;
    if (params.format != 0u) {
        ((device half4*)pixels)[index] = half4(color);
    } else {
        ((device uint*)pixels)[index] = pack_float_to_unorm4x8(color);
    }
}
)METAL";

static const char* yuGraphicsShaderPaintKernel = R"METAL(
float4 paint(PaintInput input, ShaderArgs args);

struct YumiShaderPaintParams {
    float a, b, c, d, tx, ty;
    float paperScaleX, paperScaleY, paperX, paperY;
    float pixel;
    float padding0;
    float4 color;
    uint columns, rows, argCount, format, target, clamped;
    uint padding2, padding3;
};

kernel void yumiShaderKernel(
    device uchar* pixels [[buffer(0)]],
    constant YumiShaderPaintParams& params [[buffer(1)]],
    device const float* args [[buffer(2)]],
    constant float4* frames [[buffer(3)]],
    array<texture2d<float>, 8> images [[texture(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.columns || gid.y >= params.rows) {
        return;
    }
    float2 pixel = float2(gid) + 0.5;
    PaintInput input;
    input.position = float2(
        params.a * pixel.x + params.c * pixel.y + params.tx,
        params.b * pixel.x + params.d * pixel.y + params.ty
    );
    input.paper = pixel * float2(params.paperScaleX, params.paperScaleY) + float2(params.paperX, params.paperY);
    input.color = float4(yumiLinearColor(params.color.rgb, 0u, params.target), params.color.a);
    input.pixel = params.pixel;
    ShaderArgs shaderArgs = {args, params.argCount, images, frames, 0u, params.target};
    float4 color = yumiShaderOutput(paint(input, shaderArgs), params.target, params.clamped);
    uint index = gid.y * params.columns + gid.x;
    if (params.format != 0u) {
        ((device half4*)pixels)[index] = half4(color);
    } else {
        ((device uint*)pixels)[index] = pack_float_to_unorm4x8(color);
    }
}
)METAL";

static const char* yuGraphicsShaderDabPass = R"METAL(
float4 dab(DabInput input, ShaderArgs args);

struct YumiShaderDabParams {
    float a, b, c, d, tx, ty;
    float paperScaleX, paperScaleY, paperX, paperY;
    float color[4];
    DabStyle style;
    uint columns, rows, argCount, target, clamped, padding;
};

struct YumiShaderDabOut {
    float4 position [[position]];
    float2 offset;
    float2 local;
    float pressure [[flat]];
    float random [[flat]];
    float distance [[flat]];
    float along [[flat]];
    float pixel [[flat]];
};

vertex YumiShaderDabOut yumiShaderDabVertex(
    uint vertexId [[vertex_id]],
    uint instanceId [[instance_id]],
    device const DabRecord* dabs [[buffer(0)]],
    constant YumiShaderDabParams& params [[buffer(1)]]
) {
    DabRecord dab = dabs[instanceId];
    DabPlacement placement = dabPlace(dab, params.style, vertexId);
    float2 pixel = float2(
        params.a * placement.local.x + params.c * placement.local.y + params.tx,
        params.b * placement.local.x + params.d * placement.local.y + params.ty
    );
    float scale = sqrt(abs(params.a * params.d - params.b * params.c));
    YumiShaderDabOut out;
    out.position = float4(
        pixel.x / float(params.columns) * 2.0 - 1.0,
        1.0 - pixel.y / float(params.rows) * 2.0,
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
    return out;
}

fragment float4 yumiShaderDabFragment(
    YumiShaderDabOut in [[stage_in]],
    constant YumiShaderDabParams& params [[buffer(1)]],
    device const float* args [[buffer(2)]],
    constant float4* frames [[buffer(3)]],
    array<texture2d<float>, 8> images [[texture(0)]]
) {
    DabInput input;
    input.offset = in.offset;
    input.position = in.local;
    input.paper = in.position.xy * float2(params.paperScaleX, params.paperScaleY) + float2(params.paperX, params.paperY);
    input.color = float4(
        yumiLinearColor(float3(params.color[0], params.color[1], params.color[2]), 0u, params.target),
        params.color[3]
    );
    input.along = in.along;
    input.distance = in.distance;
    input.pressure = in.pressure;
    input.random = in.random;
    input.pixel = in.pixel;
    ShaderArgs shaderArgs = {args, params.argCount, images, frames, 0u, params.target};
    float4 color = dab(input, shaderArgs);
    return yumiShaderOutput(float4(color.rgb, color.a * params.style.flow), params.target, params.clamped);
}
)METAL";

static const char* yuGraphicsShaderFilterKernel = R"METAL(
float4 shade(float2 position, ShaderLayer layer, ShaderArgs args);

struct YumiShaderFilterParams {
    float2 origin;
    float2 size;
    float scale;
    uint columns, rows, argCount, imageCount;
    uint format, target, clamped;
    float time;
};

kernel void yumiShaderKernel(
    texture2d<float> source [[texture(0)]],
    array<texture2d<float>, 8> images [[texture(1)]],
    device uchar* pixels [[buffer(0)]],
    constant YumiShaderFilterParams& params [[buffer(1)]],
    device const float* args [[buffer(2)]],
    constant float4* frames [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.columns || gid.y >= params.rows) {
        return;
    }
    ShaderLayer layer = {source, params.origin, params.scale, params.size, params.target};
    ShaderArgs shaderArgs = {args, params.argCount, images, frames, params.imageCount, params.target, params.time};
    float2 position = (float2(gid) + 0.5 - params.origin) / params.scale;
    float4 color = yumiShaderOutput(shade(position, layer, shaderArgs), params.target, params.clamped);
    uint index = gid.y * params.columns + gid.x;
    if (params.format != 0u) {
        ((device half4*)pixels)[index] = half4(color);
    } else {
        ((device uint*)pixels)[index] = pack_float_to_unorm4x8(color.bgra);
    }
}
)METAL";

static const char* yuGraphicsShaderRenderKernel = R"METAL(
float4 shade(float2 position, float2 size, ShaderArgs args);

struct YumiShaderRenderParams {
    uint2 origin;
    uint2 extent;
    float2 size;
    float scale;
    uint argCount, imageCount, target, clamped;
};

kernel void yumiShaderKernel(
    texture2d<float, access::write> target [[texture(0)]],
    array<texture2d<float>, 8> images [[texture(1)]],
    constant YumiShaderRenderParams& params [[buffer(0)]],
    device const float* args [[buffer(1)]],
    constant float4* frames [[buffer(2)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.extent.x || gid.y >= params.extent.y) {
        return;
    }
    uint2 pixel = params.origin + gid;
    float2 position = (float2(pixel) + 0.5) / params.scale;
    ShaderArgs shaderArgs = {args, params.argCount, images, frames, params.imageCount, params.target};
    target.write(yumiShaderOutput(shade(position, params.size, shaderArgs), params.target, params.clamped), pixel);
}
)METAL";

static const double kYuGraphicsPrimaries[3][6] = {
    {0.64, 0.33, 0.30, 0.60, 0.15, 0.06},
    {0.680, 0.320, 0.265, 0.690, 0.150, 0.060},
    {0.708, 0.292, 0.170, 0.797, 0.131, 0.046},
};

static const double kYuGraphicsWhite[2] = {0.3127, 0.3290};

static void
yuGraphicsInvert3(const double* m, double* out) {
    const double a = m[4] * m[8] - m[5] * m[7];
    const double b = m[5] * m[6] - m[3] * m[8];
    const double c = m[3] * m[7] - m[4] * m[6];
    const double determinant = m[0] * a + m[1] * b + m[2] * c;
    out[0] = a / determinant;
    out[1] = (m[2] * m[7] - m[1] * m[8]) / determinant;
    out[2] = (m[1] * m[5] - m[2] * m[4]) / determinant;
    out[3] = b / determinant;
    out[4] = (m[0] * m[8] - m[2] * m[6]) / determinant;
    out[5] = (m[2] * m[3] - m[0] * m[5]) / determinant;
    out[6] = c / determinant;
    out[7] = (m[1] * m[6] - m[0] * m[7]) / determinant;
    out[8] = (m[0] * m[4] - m[1] * m[3]) / determinant;
}

static void
yuGraphicsMultiply3(const double* left, const double* right, double* out) {
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            out[row * 3 + column] = left[row * 3] * right[column] + left[row * 3 + 1] * right[3 + column] +
                left[row * 3 + 2] * right[6 + column];
        }
    }
}

static void
yuGraphicsPrimariesToXYZ(const double* primaries, double* out) {
    double matrix[9];
    for (int primary = 0; primary < 3; ++primary) {
        const double x = primaries[primary * 2];
        const double y = primaries[primary * 2 + 1];
        matrix[primary] = x / y;
        matrix[3 + primary] = 1;
        matrix[6 + primary] = (1 - x - y) / y;
    }
    double inverse[9];
    yuGraphicsInvert3(matrix, inverse);
    const double white[3] = {
        kYuGraphicsWhite[0] / kYuGraphicsWhite[1],
        1,
        (1 - kYuGraphicsWhite[0] - kYuGraphicsWhite[1]) / kYuGraphicsWhite[1],
    };
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            const double weight = inverse[column * 3] * white[0] + inverse[column * 3 + 1] * white[1] +
                inverse[column * 3 + 2] * white[2];
            out[row * 3 + column] = matrix[row * 3 + column] * weight;
        }
    }
}

static const std::string&
yuGraphicsShaderColorSource() {
    static const std::string source = [] {
        std::string text = "constant float3x3 kYumiPrimaryConversions[9] = {\n";
        char buffer[512];
        for (int from = 0; from < 3; ++from) {
            for (int to = 0; to < 3; ++to) {
                double conversion[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
                if (from != to) {
                    double source[9];
                    double target[9];
                    double inverse[9];
                    yuGraphicsPrimariesToXYZ(kYuGraphicsPrimaries[from], source);
                    yuGraphicsPrimariesToXYZ(kYuGraphicsPrimaries[to], target);
                    yuGraphicsInvert3(target, inverse);
                    yuGraphicsMultiply3(inverse, source, conversion);
                }
                snprintf(
                    buffer,
                    sizeof(buffer),
                    "    float3x3(float3(%.9g, %.9g, %.9g), float3(%.9g, %.9g, %.9g), float3(%.9g, %.9g, %.9g)),\n",
                    conversion[0], conversion[3], conversion[6],
                    conversion[1], conversion[4], conversion[7],
                    conversion[2], conversion[5], conversion[8]
                );
                text += buffer;
            }
        }
        text += "};\n";
        text += yuGraphicsShaderColorFunctions;
        return text;
    }();
    return source;
}

static bool
yuGraphicsShaderDeviceReady() {
    std::call_once(yuGraphicsShaderDeviceOnce, [] {
        yuGraphicsShaderDevice = MTLCreateSystemDefaultDevice();
        if (!yuGraphicsShaderDevice) {
            return;
        }
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                         width:1
                                        height:1
                                     mipmapped:NO];
        descriptor.usage = MTLTextureUsageShaderRead;
        yuGraphicsShaderBlank = [yuGraphicsShaderDevice newTextureWithDescriptor:descriptor];
        const uint32_t clear = 0;
        [yuGraphicsShaderBlank replaceRegion:MTLRegionMake2D(0, 0, 1, 1)
                                 mipmapLevel:0
                                   withBytes:&clear
                                 bytesPerRow:4];
    });
    return yuGraphicsShaderDevice && yuGraphicsShaderBlank;
}

static id<MTLCommandBuffer>
yuGraphicsShaderCommandBuffer() {
    if (!yuGraphicsShaderDeviceReady()) {
        return nil;
    }
    std::lock_guard<std::mutex> lock(yuGraphicsShaderQueueMutex);
    if (!yuGraphicsShaderQueue) {
        yuGraphicsShaderQueue = [yuGraphicsShaderDevice newCommandQueue];
    }
    id<MTLCommandBuffer> commands = [yuGraphicsShaderQueue commandBuffer];
    if (commands) {
        YuGraphicsShaderCommandToken* token = [YuGraphicsShaderCommandToken new];
        yuGraphicsShaderCommandsAlive.fetch_add(1, std::memory_order_relaxed);
        objc_setAssociatedObject(commands, &yuGraphicsShaderCommandTokenKey, token, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [token release];
    }
    return commands;
}

extern "C" void*
yumiGraphicsMetalDevice() {
    return yuGraphicsShaderDeviceReady() ? (void*)yuGraphicsShaderDevice : nullptr;
}

extern "C" void*
yumiGraphicsMetalCommandBuffer() {
    return (void*)yuGraphicsShaderCommandBuffer();
}

extern "C" bool
yumiGraphicsMetalReleaseIdleQueue() {
    std::lock_guard<std::mutex> lock(yuGraphicsShaderQueueMutex);
    if (yuGraphicsShaderCommandsAlive.load(std::memory_order_acquire) != 0) {
        return false;
    }
    [yuGraphicsShaderQueue release];
    yuGraphicsShaderQueue = nil;
    return true;
}

static id<MTLLibrary>
yuGraphicsShaderLibrary(const std::string& source, std::string& message) {
    NSError* error = nil;
    id<MTLLibrary> library = [yuGraphicsShaderDevice
        newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                     options:nil
                       error:&error];
    if (!library) {
        message = error.localizedDescription.UTF8String ?: "Shader compilation failed";
    }
    return library;
}

static std::string
yuGraphicsShaderCombined(const char* kernel, const std::string& source) {
    return std::string(yuGraphicsShaderIncludes) + yuGraphicsShaderColorSource() + yuGraphicsShaderTypes +
        yuGraphicsShaderInputTypes + yuGraphicsShaderArgsType + (kernel ? kernel : "") + "\n#line 1\n" + source;
}

static void
yuGraphicsShaderValidate(YuGraphicsShaderEntry* entry) {
    if (!yuGraphicsShaderDeviceReady()) {
        entry->error = "Metal is unavailable";
        return;
    }
    @autoreleasepool {
        id<MTLLibrary> library = yuGraphicsShaderLibrary(yuGraphicsShaderCombined(nullptr, entry->source), entry->error);
        [library release];
    }
}

static id<MTLComputePipelineState>
yuGraphicsShaderKernelFor(YuGraphicsShaderEntry* entry, YuGraphicsShaderKernel& kernel, const char* source, const char* role) {
    std::lock_guard<std::mutex> lock(yuGraphicsShaderMutex);
    if (kernel.tried) {
        return kernel.pipeline;
    }
    kernel.tried = true;
    if (!entry->error.empty() || !yuGraphicsShaderDeviceReady()) {
        return nil;
    }
    std::string message;
    @autoreleasepool {
        id<MTLLibrary> library = yuGraphicsShaderLibrary(yuGraphicsShaderCombined(source, entry->source), message);
        id<MTLFunction> function = library ? [library newFunctionWithName:@"yumiShaderKernel"] : nil;
        NSError* error = nil;
        kernel.pipeline = function
            ? [yuGraphicsShaderDevice newComputePipelineStateWithFunction:function error:&error]
            : nil;
        if (function && !kernel.pipeline) {
            message = error ? error.localizedDescription.UTF8String : "Shader function failed";
        }
        [function release];
        [library release];
    }
    if (!kernel.pipeline) {
        fprintf(stderr, "yumi graphics: shader has no usable %s function: %s\n", role, message.c_str());
    }
    return kernel.pipeline;
}

extern "C" uint32_t
yumiGraphicsShaderRegister(const char* source) {
    std::lock_guard<std::mutex> lock(yuGraphicsShaderMutex);
    std::string text = source ? source : "";
    auto found = yuGraphicsShaderIds.find(text);
    if (found != yuGraphicsShaderIds.end()) {
        return found->second;
    }
    YuGraphicsShaderEntry* entry = new YuGraphicsShaderEntry();
    entry->source = text;
    yuGraphicsShaders.push_back(entry);
    uint32_t shader = static_cast<uint32_t>(yuGraphicsShaders.size());
    yuGraphicsShaderIds[text] = shader;
    return shader;
}

static YuGraphicsShaderEntry*
yuGraphicsShaderEntry(uint32_t shader) {
    std::lock_guard<std::mutex> lock(yuGraphicsShaderMutex);
    return shader && shader <= yuGraphicsShaders.size() ? yuGraphicsShaders[shader - 1] : nullptr;
}

extern "C" const char*
yumiGraphicsShaderError(uint32_t shader) {
    YuGraphicsShaderEntry* entry = yuGraphicsShaderEntry(shader);
    if (!entry) {
        return "Unknown shader";
    }
    std::lock_guard<std::mutex> lock(yuGraphicsShaderMutex);
    if (!entry->validated) {
        yuGraphicsShaderValidate(entry);
        entry->validated = true;
    }
    return entry->error.c_str();
}

extern "C" const char*
yumiGraphicsShaderSource(uint32_t shader) {
    YuGraphicsShaderEntry* entry = yuGraphicsShaderEntry(shader);
    return entry && entry->error.empty() ? entry->source.c_str() : nullptr;
}

extern "C" const char*
yumiGraphicsShaderColor() {
    return yuGraphicsShaderColorSource().c_str();
}

extern "C" const char*
yumiGraphicsShaderTypes() {
    return yuGraphicsShaderTypes;
}

extern "C" const char*
yumiGraphicsShaderArgsType() {
    return yuGraphicsShaderArgsType;
}

extern "C" const char*
yumiGraphicsShaderInputTypes() {
    return yuGraphicsShaderInputTypes;
}

extern "C" const char*
yumiGraphicsShaderDabSource() {
    return yuGraphicsShaderDabSource;
}

static void
yuGraphicsShaderReleaseData(void* info, const void* data, size_t size) {
    free(const_cast<void*>(data));
}

static void
yuGraphicsShaderDispatch(
    id<MTLComputeCommandEncoder> encoder,
    id<MTLComputePipelineState> pipeline,
    int32_t columns,
    int32_t rows
) {
    NSUInteger groupWidth = pipeline.threadExecutionWidth;
    NSUInteger groupHeight = std::max<NSUInteger>(1, pipeline.maxTotalThreadsPerThreadgroup / groupWidth);
    [encoder dispatchThreads:MTLSizeMake(static_cast<NSUInteger>(columns), static_cast<NSUInteger>(rows), 1)
       threadsPerThreadgroup:MTLSizeMake(groupWidth, groupHeight, 1)];
}

static std::vector<float>
yuGraphicsShaderArgs(const double* args, int64_t argCount) {
    std::vector<float> values(static_cast<size_t>(std::max<int64_t>(argCount, 0)) + 1, 0.0f);
    for (int64_t index = 0; index < argCount; ++index) {
        values[static_cast<size_t>(index)] = static_cast<float>(args[index]);
    }
    return values;
}

static void
yuGraphicsShaderBindImages(
    id<MTLComputeCommandEncoder> encoder,
    NSUInteger textureIndex,
    NSUInteger frameIndex,
    void* const* images,
    const double* frames,
    int32_t imageCount
) {
    id<MTLTexture> textures[kShaderImageLimit];
    float values[kShaderImageLimit * 4] = {};
    for (int32_t slot = 0; slot < kShaderImageLimit; ++slot) {
        id<MTLTexture> texture = slot < imageCount ? (id<MTLTexture>)images[slot] : nil;
        textures[slot] = texture ? texture : yuGraphicsShaderBlank;
        for (int32_t lane = 0; lane < 4 && slot < imageCount; ++lane) {
            values[slot * 4 + lane] = static_cast<float>(frames[slot * 4 + lane]);
        }
    }
    [encoder setTextures:textures withRange:NSMakeRange(textureIndex, kShaderImageLimit)];
    [encoder setBytes:values length:sizeof(values) atIndex:frameIndex];
}

static MTLPixelFormat
yuGraphicsTextureFormat(int32_t format) {
    return format == YuGraphicsFormatRGBA16Float ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
}

static void*
yuGraphicsShaderImage(CGDataProviderRef provider, int32_t columns, int32_t rows, int32_t format, void* colorSpace);

static CGDataProviderRef
yuGraphicsShaderBytes(void* bytes, size_t byteCount) {
    return bytes ? CGDataProviderCreateWithData(nullptr, bytes, byteCount, yuGraphicsShaderReleaseData) : nullptr;
}

void*
yuGraphicsShaderRaster(
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
) {
    YuGraphicsShaderEntry* entry = yuGraphicsShaderEntry(shader);
    if (!entry || columns <= 0 || rows <= 0 || argCount < 0) {
        return nullptr;
    }
    id<MTLComputePipelineState> pipeline =
        yuGraphicsShaderKernelFor(entry, entry->raster, yuGraphicsShaderRasterKernel, "fill");
    if (!pipeline) {
        return nullptr;
    }
    const size_t bytesPerPixel = yuGraphicsFormatBytesPerPixel(format);
    size_t byteCount = static_cast<size_t>(columns) * static_cast<size_t>(rows) * bytesPerPixel;
    void* bytes = nullptr;
    @autoreleasepool {
        id<MTLBuffer> pixels = [yuGraphicsShaderDevice newBufferWithLength:byteCount
                                                                   options:MTLResourceStorageModeShared];
        std::vector<float> values = yuGraphicsShaderArgs(args, argCount);
        YuGraphicsShaderRasterParams params = {
            static_cast<float>(transform[0]),
            static_cast<float>(transform[1]),
            static_cast<float>(transform[2]),
            static_cast<float>(transform[3]),
            static_cast<float>(transform[4]),
            static_cast<float>(transform[5]),
            static_cast<float>(width),
            static_cast<float>(height),
            static_cast<uint32_t>(columns),
            static_cast<uint32_t>(rows),
            static_cast<uint32_t>(argCount),
            0,
            static_cast<uint32_t>(format),
            static_cast<uint32_t>(space),
            format == YuGraphicsFormatRGBA16Float ? 0u : 1u,
            static_cast<float>(time),
        };
        id<MTLCommandBuffer> commands = yuGraphicsShaderCommandBuffer();
        id<MTLComputeCommandEncoder> encoder = [commands computeCommandEncoder];
        [encoder setComputePipelineState:pipeline];
        [encoder setBuffer:pixels offset:0 atIndex:0];
        [encoder setBytes:&params length:sizeof(params) atIndex:1];
        [encoder setBytes:values.data() length:values.size() * sizeof(float) atIndex:2];
        yuGraphicsShaderBindImages(encoder, 0, 3, nullptr, nullptr, 0);
        yuGraphicsShaderDispatch(encoder, pipeline, columns, rows);
        [encoder endEncoding];
        [commands commit];
        [commands waitUntilCompleted];
        if (commands.status == MTLCommandBufferStatusCompleted) {
            bytes = malloc(byteCount);
            if (bytes) {
                memcpy(bytes, pixels.contents, byteCount);
            }
        }
        [pixels release];
    }
    return yuGraphicsShaderImage(yuGraphicsShaderBytes(bytes, byteCount), columns, rows, format, colorSpace);
}

static void*
yuGraphicsShaderImage(CGDataProviderRef provider, int32_t columns, int32_t rows, int32_t format, void* colorSpace) {
    if (!provider) {
        return nullptr;
    }
    const size_t bytesPerPixel = yuGraphicsFormatBytesPerPixel(format);
    const bool floating = format == YuGraphicsFormatRGBA16Float;
    CGImageRef image = CGImageCreate(
        static_cast<size_t>(columns),
        static_cast<size_t>(rows),
        floating ? 16 : 8,
        floating ? 64 : 32,
        static_cast<size_t>(columns) * bytesPerPixel,
        static_cast<CGColorSpaceRef>(colorSpace),
        floating
            ? kCGImageAlphaPremultipliedLast | kCGBitmapFloatComponents | kCGBitmapByteOrder16Little
            : kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault,
        provider,
        nullptr,
        false,
        kCGRenderingIntentDefault
    );
    CGDataProviderRelease(provider);
    return image;
}

struct YuGraphicsShaderJob {
    id<MTLBuffer> pixels;
    int32_t columns;
    int32_t rows;
    int32_t format;
};

struct YuGraphicsShaderBatch {
    id<MTLCommandBuffer> commands = nil;
    id<MTLComputeCommandEncoder> compute = nil;
    std::vector<YuGraphicsShaderJob> jobs;
    size_t bytes = 0;
    bool completed = false;
};

static void
yuGraphicsShaderBatchEndCompute(YuGraphicsShaderBatch* batch) {
    if (batch->compute) {
        [batch->compute endEncoding];
        [batch->compute release];
        batch->compute = nil;
    }
}

static id<MTLCommandBuffer>
yuGraphicsShaderBatchCommands(YuGraphicsShaderBatch* batch) {
    if (!batch->commands) {
        batch->commands = [yuGraphicsShaderCommandBuffer() retain];
    }
    return batch->commands;
}

static id<MTLComputeCommandEncoder>
yuGraphicsShaderBatchCompute(YuGraphicsShaderBatch* batch) {
    if (!batch->compute) {
        batch->compute = [[yuGraphicsShaderBatchCommands(batch) computeCommandEncoder] retain];
    }
    return batch->compute;
}

static int64_t
yuGraphicsShaderBatchAdd(YuGraphicsShaderBatch* batch, id<MTLBuffer> pixels, int32_t columns, int32_t rows, int32_t format) {
    batch->jobs.push_back(YuGraphicsShaderJob{pixels, columns, rows, format});
    batch->bytes += pixels.length;
    batch->completed = false;
    return static_cast<int64_t>(batch->jobs.size() - 1);
}

static void
yuGraphicsShaderReleaseBuffer(void* info, const void* data, size_t size) {
    [static_cast<id<MTLBuffer>>(info) release];
}

void*
yuGraphicsShaderBatchCreate() {
    return yuGraphicsShaderDeviceReady() ? new YuGraphicsShaderBatch() : nullptr;
}

void
yuGraphicsShaderBatchReset(void* pointer) {
    YuGraphicsShaderBatch* batch = static_cast<YuGraphicsShaderBatch*>(pointer);
    if (!batch) {
        return;
    }
    yuGraphicsShaderBatchEndCompute(batch);
    [batch->commands release];
    batch->commands = nil;
    for (YuGraphicsShaderJob& job : batch->jobs) {
        [job.pixels release];
    }
    batch->jobs.clear();
    batch->bytes = 0;
    batch->completed = false;
}

void
yuGraphicsShaderBatchRelease(void* pointer) {
    yuGraphicsShaderBatchReset(pointer);
    delete static_cast<YuGraphicsShaderBatch*>(pointer);
}

size_t
yuGraphicsShaderBatchBytes(void* pointer) {
    YuGraphicsShaderBatch* batch = static_cast<YuGraphicsShaderBatch*>(pointer);
    return batch ? batch->bytes : 0;
}

bool
yuGraphicsShaderBatchFinish(void* pointer) {
    YuGraphicsShaderBatch* batch = static_cast<YuGraphicsShaderBatch*>(pointer);
    if (!batch) {
        return false;
    }
    if (batch->commands) {
        yuGraphicsShaderBatchEndCompute(batch);
        @autoreleasepool {
            [batch->commands commit];
            [batch->commands waitUntilCompleted];
            batch->completed = batch->commands.status == MTLCommandBufferStatusCompleted;
        }
        [batch->commands release];
        batch->commands = nil;
    }
    return batch->completed;
}

void*
yuGraphicsShaderBatchImage(void* pointer, int64_t job, void* colorSpace) {
    YuGraphicsShaderBatch* batch = static_cast<YuGraphicsShaderBatch*>(pointer);
    if (!batch || !batch->completed || job < 0 || job >= static_cast<int64_t>(batch->jobs.size())) {
        return nullptr;
    }
    const YuGraphicsShaderJob& entry = batch->jobs[static_cast<size_t>(job)];
    const size_t byteCount =
        static_cast<size_t>(entry.columns) * static_cast<size_t>(entry.rows) * yuGraphicsFormatBytesPerPixel(entry.format);
    [entry.pixels retain];
    CGDataProviderRef provider =
        CGDataProviderCreateWithData(entry.pixels, entry.pixels.contents, byteCount, yuGraphicsShaderReleaseBuffer);
    if (!provider) {
        [entry.pixels release];
    }
    return yuGraphicsShaderImage(provider, entry.columns, entry.rows, entry.format, colorSpace);
}

int64_t
yuGraphicsShaderBatchPaint(
    void* pointer,
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
) {
    YuGraphicsShaderBatch* batch = static_cast<YuGraphicsShaderBatch*>(pointer);
    YuGraphicsShaderEntry* entry = yuGraphicsShaderEntry(shader);
    if (!batch || !entry || columns <= 0 || rows <= 0 || argCount < 0) {
        return -1;
    }
    id<MTLComputePipelineState> pipeline =
        yuGraphicsShaderKernelFor(entry, entry->paint, yuGraphicsShaderPaintKernel, "paint");
    if (!pipeline) {
        return -1;
    }
    const size_t bytesPerPixel = yuGraphicsFormatBytesPerPixel(format);
    size_t byteCount = static_cast<size_t>(columns) * static_cast<size_t>(rows) * bytesPerPixel;
    id<MTLBuffer> pixels = [yuGraphicsShaderDevice newBufferWithLength:byteCount
                                                               options:MTLResourceStorageModeShared];
    if (!pixels) {
        return -1;
    }
    @autoreleasepool {
        std::vector<float> values = yuGraphicsShaderArgs(args, argCount);
        YuGraphicsShaderPaintParams params = {};
        params.a = static_cast<float>(local[0]);
        params.b = static_cast<float>(local[1]);
        params.c = static_cast<float>(local[2]);
        params.d = static_cast<float>(local[3]);
        params.tx = static_cast<float>(local[4]);
        params.ty = static_cast<float>(local[5]);
        params.paperScaleX = static_cast<float>(paper[0]);
        params.paperScaleY = static_cast<float>(paper[1]);
        params.paperX = static_cast<float>(paper[2]);
        params.paperY = static_cast<float>(paper[3]);
        params.pixel = static_cast<float>(pixel);
        for (int lane = 0; lane < 4; ++lane) {
            params.color[lane] = static_cast<float>(color[lane]);
        }
        params.columns = static_cast<uint32_t>(columns);
        params.rows = static_cast<uint32_t>(rows);
        params.argCount = static_cast<uint32_t>(argCount);
        params.format = static_cast<uint32_t>(format);
        params.target = static_cast<uint32_t>(space);
        params.clamped = format == YuGraphicsFormatRGBA16Float ? 0u : 1u;
        id<MTLComputeCommandEncoder> encoder = yuGraphicsShaderBatchCompute(batch);
        [encoder setComputePipelineState:pipeline];
        [encoder setBuffer:pixels offset:0 atIndex:0];
        [encoder setBytes:&params length:sizeof(params) atIndex:1];
        [encoder setBytes:values.data() length:values.size() * sizeof(float) atIndex:2];
        yuGraphicsShaderBindImages(encoder, 0, 3, nullptr, nullptr, 0);
        yuGraphicsShaderDispatch(encoder, pipeline, columns, rows);
    }
    return yuGraphicsShaderBatchAdd(batch, pixels, columns, rows, format);
}

static id<MTLRenderPipelineState>
yuGraphicsShaderDabPipeline(YuGraphicsShaderEntry* entry, int32_t format) {
    int slot = format == YuGraphicsFormatRGBA16Float ? 1 : 0;
    std::lock_guard<std::mutex> lock(yuGraphicsShaderMutex);
    if (entry->dabTried[slot]) {
        return entry->dabPipelines[slot];
    }
    entry->dabTried[slot] = true;
    if (!entry->error.empty() || !yuGraphicsShaderDeviceReady()) {
        return nil;
    }
    std::string message;
    @autoreleasepool {
        std::string source = std::string(yuGraphicsShaderDabSource) + yuGraphicsShaderDabPass;
        id<MTLLibrary> library = yuGraphicsShaderLibrary(yuGraphicsShaderCombined(source.c_str(), entry->source), message);
        id<MTLFunction> vertex = library ? [library newFunctionWithName:@"yumiShaderDabVertex"] : nil;
        id<MTLFunction> fragment = library ? [library newFunctionWithName:@"yumiShaderDabFragment"] : nil;
        if (vertex && fragment) {
            MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
            descriptor.vertexFunction = vertex;
            descriptor.fragmentFunction = fragment;
            MTLRenderPipelineColorAttachmentDescriptor* attachment = descriptor.colorAttachments[0];
            attachment.pixelFormat = slot ? MTLPixelFormatRGBA16Float : MTLPixelFormatRGBA8Unorm;
            attachment.blendingEnabled = YES;
            attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
            attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
            attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            NSError* error = nil;
            entry->dabPipelines[slot] = [yuGraphicsShaderDevice newRenderPipelineStateWithDescriptor:descriptor
                                                                                               error:&error];
            if (!entry->dabPipelines[slot]) {
                message = error ? error.localizedDescription.UTF8String : "Shader pipeline failed";
            }
            [descriptor release];
        }
        [vertex release];
        [fragment release];
        [library release];
    }
    if (!entry->dabPipelines[slot]) {
        fprintf(stderr, "yumi graphics: shader has no usable brush function: %s\n", message.c_str());
    }
    return entry->dabPipelines[slot];
}

int64_t
yuGraphicsShaderBatchDabs(
    void* pointer,
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
) {
    YuGraphicsShaderBatch* batch = static_cast<YuGraphicsShaderBatch*>(pointer);
    YuGraphicsShaderEntry* entry = yuGraphicsShaderEntry(shader);
    if (!batch || !entry || columns <= 0 || rows <= 0 || columns > kGPUTextureSizeLimit || rows > kGPUTextureSizeLimit ||
            argCount < 0 || dabCount <= 0) {
        return -1;
    }
    id<MTLRenderPipelineState> pipeline = yuGraphicsShaderDabPipeline(entry, format);
    if (!pipeline) {
        return -1;
    }
    const size_t bytesPerPixel = yuGraphicsFormatBytesPerPixel(format);
    size_t rowBytes = static_cast<size_t>(columns) * bytesPerPixel;
    size_t byteCount = rowBytes * static_cast<size_t>(rows);
    id<MTLBuffer> output = nil;
    @autoreleasepool {
        std::vector<YuGraphicsShaderDabRecord> records(static_cast<size_t>(dabCount));
        for (int64_t index = 0; index < dabCount; ++index) {
            const YuGraphicsDab& dab = dabs[index];
            records[static_cast<size_t>(index)] = YuGraphicsShaderDabRecord{
                dab.x, dab.y, dab.angle, dab.distance, dab.length, dab.along, 0, static_cast<uint32_t>(index),
            };
        }
        YuGraphicsShaderDabParams params = {};
        params.a = static_cast<float>(raster[0]);
        params.b = static_cast<float>(raster[1]);
        params.c = static_cast<float>(raster[2]);
        params.d = static_cast<float>(raster[3]);
        params.tx = static_cast<float>(raster[4]);
        params.ty = static_cast<float>(raster[5]);
        params.paperScaleX = static_cast<float>(paper[0]);
        params.paperScaleY = static_cast<float>(paper[1]);
        params.paperX = static_cast<float>(paper[2]);
        params.paperY = static_cast<float>(paper[3]);
        for (int lane = 0; lane < 4; ++lane) {
            params.color[lane] = static_cast<float>(color[lane]);
        }
        params.style.width = static_cast<float>(width);
        params.style.scatter = static_cast<float>(style[1]);
        params.style.sizeJitter = static_cast<float>(style[2]);
        params.style.angleJitter = static_cast<float>(style[3]);
        params.style.roundness = static_cast<float>(style[4]);
        params.style.angle = static_cast<float>(style[5]);
        params.style.fixedAngle = style[6] != 0 ? 1 : 0;
        params.style.flow = static_cast<float>(style[7]);
        params.style.taperStart = static_cast<float>(style[8]);
        params.style.taperEnd = static_cast<float>(style[9]);
        params.style.seed = static_cast<float>(style[11]);
        params.style.total = static_cast<float>(total);
        params.columns = static_cast<uint32_t>(columns);
        params.rows = static_cast<uint32_t>(rows);
        params.argCount = static_cast<uint32_t>(argCount);
        params.target = static_cast<uint32_t>(space);
        params.clamped = format == YuGraphicsFormatRGBA16Float ? 0u : 1u;
        std::vector<float> values = yuGraphicsShaderArgs(args, argCount);
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:format == YuGraphicsFormatRGBA16Float
                ? MTLPixelFormatRGBA16Float
                : MTLPixelFormatRGBA8Unorm
                                         width:static_cast<NSUInteger>(columns)
                                        height:static_cast<NSUInteger>(rows)
                                     mipmapped:NO];
        descriptor.usage = MTLTextureUsageRenderTarget;
        descriptor.storageMode = MTLStorageModePrivate;
        id<MTLTexture> target = [yuGraphicsShaderDevice newTextureWithDescriptor:descriptor];
        id<MTLBuffer> dabBuffer = [yuGraphicsShaderDevice newBufferWithBytes:records.data()
                                                                      length:records.size() * sizeof(YuGraphicsShaderDabRecord)
                                                                     options:MTLResourceStorageModeShared];
        output = [yuGraphicsShaderDevice newBufferWithLength:byteCount options:MTLResourceStorageModeShared];
        if (target && dabBuffer && output) {
            MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
            pass.colorAttachments[0].texture = target;
            pass.colorAttachments[0].loadAction = MTLLoadActionClear;
            pass.colorAttachments[0].storeAction = MTLStoreActionStore;
            pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
            yuGraphicsShaderBatchEndCompute(batch);
            id<MTLCommandBuffer> commands = yuGraphicsShaderBatchCommands(batch);
            id<MTLRenderCommandEncoder> encoder = [commands renderCommandEncoderWithDescriptor:pass];
            [encoder setRenderPipelineState:pipeline];
            [encoder setVertexBuffer:dabBuffer offset:0 atIndex:0];
            [encoder setVertexBytes:&params length:sizeof(params) atIndex:1];
            [encoder setFragmentBytes:&params length:sizeof(params) atIndex:1];
            [encoder setFragmentBytes:values.data() length:values.size() * sizeof(float) atIndex:2];
            id<MTLTexture> textures[kShaderImageLimit];
            for (int32_t slot = 0; slot < kShaderImageLimit; ++slot) {
                textures[slot] = yuGraphicsShaderBlank;
            }
            float frames[kShaderImageLimit * 4] = {};
            [encoder setFragmentTextures:textures withRange:NSMakeRange(0, kShaderImageLimit)];
            [encoder setFragmentBytes:frames length:sizeof(frames) atIndex:3];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                        vertexStart:0
                        vertexCount:4
                      instanceCount:static_cast<NSUInteger>(dabCount)];
            [encoder endEncoding];
            id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
            [blit copyFromTexture:target
                             sourceSlice:0
                             sourceLevel:0
                            sourceOrigin:MTLOriginMake(0, 0, 0)
                              sourceSize:MTLSizeMake(static_cast<NSUInteger>(columns), static_cast<NSUInteger>(rows), 1)
                                toBuffer:output
                       destinationOffset:0
                  destinationBytesPerRow:rowBytes
                destinationBytesPerImage:byteCount];
            [blit endEncoding];
        } else {
            [output release];
            output = nil;
        }
        [target release];
        [dabBuffer release];
    }
    return output ? yuGraphicsShaderBatchAdd(batch, output, columns, rows, format) : -1;
}

bool
yuGraphicsShaderFilter(
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
) {
    YuGraphicsShaderEntry* entry = yuGraphicsShaderEntry(shader);
    if (!entry || !pixels || columns <= 0 || rows <= 0 || argCount < 0 || !(scale > 0) ||
            imageCount < 0 || imageCount > kShaderImageLimit) {
        return false;
    }
    id<MTLComputePipelineState> pipeline =
        yuGraphicsShaderKernelFor(entry, entry->filter, yuGraphicsShaderFilterKernel, "filter");
    if (!pipeline) {
        return false;
    }
    size_t rowBytes = static_cast<size_t>(columns) * yuGraphicsFormatBytesPerPixel(format);
    size_t byteCount = rowBytes * static_cast<size_t>(rows);
    bool done = false;
    @autoreleasepool {
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:yuGraphicsTextureFormat(format)
                                         width:static_cast<NSUInteger>(columns)
                                        height:static_cast<NSUInteger>(rows)
                                     mipmapped:NO];
        descriptor.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> source = [yuGraphicsShaderDevice newTextureWithDescriptor:descriptor];
        id<MTLBuffer> output = [yuGraphicsShaderDevice newBufferWithLength:byteCount
                                                                   options:MTLResourceStorageModeShared];
        if (source && output) {
            [source replaceRegion:MTLRegionMake2D(0, 0, static_cast<NSUInteger>(columns), static_cast<NSUInteger>(rows))
                      mipmapLevel:0
                        withBytes:pixels
                      bytesPerRow:bytesPerRow];
            std::vector<float> values = yuGraphicsShaderArgs(args, argCount);
            YuGraphicsShaderFilterParams params = {
                {static_cast<float>(origin[0]), static_cast<float>(origin[1])},
                {static_cast<float>(size[0]), static_cast<float>(size[1])},
                static_cast<float>(scale),
                static_cast<uint32_t>(columns),
                static_cast<uint32_t>(rows),
                static_cast<uint32_t>(argCount),
                static_cast<uint32_t>(imageCount),
                static_cast<uint32_t>(format),
                static_cast<uint32_t>(space),
                format == YuGraphicsFormatRGBA16Float ? 0u : 1u,
                static_cast<float>(time),
            };
            id<MTLCommandBuffer> commands = yuGraphicsShaderCommandBuffer();
            id<MTLComputeCommandEncoder> encoder = [commands computeCommandEncoder];
            [encoder setComputePipelineState:pipeline];
            [encoder setTexture:source atIndex:0];
            [encoder setBuffer:output offset:0 atIndex:0];
            [encoder setBytes:&params length:sizeof(params) atIndex:1];
            [encoder setBytes:values.data() length:values.size() * sizeof(float) atIndex:2];
            yuGraphicsShaderBindImages(encoder, 1, 3, images, frames, imageCount);
            yuGraphicsShaderDispatch(encoder, pipeline, columns, rows);
            [encoder endEncoding];
            [commands commit];
            [commands waitUntilCompleted];
            if (commands.status == MTLCommandBufferStatusCompleted) {
                const uint8_t* filtered = static_cast<const uint8_t*>(output.contents);
                for (int32_t row = 0; row < rows; ++row) {
                    memcpy(pixels + static_cast<size_t>(row) * bytesPerRow, filtered + static_cast<size_t>(row) * rowBytes, rowBytes);
                }
                done = true;
            }
        }
        [source release];
        [output release];
    }
    return done;
}

void*
yuGraphicsGPUCreate(int32_t width, int32_t height, int32_t format) {
    if (width <= 0 || height <= 0 || width > kGPUTextureSizeLimit || height > kGPUTextureSizeLimit ||
            !yuGraphicsShaderDeviceReady()) {
        return nullptr;
    }
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:yuGraphicsTextureFormat(format)
                                     width:static_cast<NSUInteger>(width)
                                    height:static_cast<NSUInteger>(height)
                                 mipmapped:YES];
    descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    descriptor.storageMode = MTLStorageModePrivate;
    id<MTLTexture> texture = [yuGraphicsShaderDevice newTextureWithDescriptor:descriptor];
    if (!texture) {
        return nullptr;
    }
    YuGraphicsGPUStore* store = new YuGraphicsGPUStore();
    store->texture = texture;
    store->bytesPerPixel = yuGraphicsFormatBytesPerPixel(format);
    return store;
}

void*
yuGraphicsGPUCreateWithSurface(IOSurfaceRef surface) {
    if (!surface || !yuGraphicsShaderDeviceReady()) {
        return nullptr;
    }
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:IOSurfaceGetWidth(surface)
                                    height:IOSurfaceGetHeight(surface)
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageShaderRead;
#if TARGET_OS_OSX
    descriptor.storageMode = yuGraphicsShaderDevice.hasUnifiedMemory ? MTLStorageModeShared : MTLStorageModeManaged;
#else
    descriptor.storageMode = MTLStorageModeShared;
#endif
    id<MTLTexture> texture = [yuGraphicsShaderDevice newTextureWithDescriptor:descriptor iosurface:surface plane:0];
    if (!texture) {
        return nullptr;
    }
    YuGraphicsGPUStore* store = new YuGraphicsGPUStore();
    store->texture = texture;
    store->bytesPerPixel = 4;
    store->mipsStale = false;
    return store;
}

void
yuGraphicsGPURelease(void* pointer) {
    YuGraphicsGPUStore* store = static_cast<YuGraphicsGPUStore*>(pointer);
    if (!store) {
        return;
    }
    [store->texture release];
    delete store;
}

void*
yuGraphicsGPUTexture(void* pointer, bool mipmaps) {
    YuGraphicsGPUStore* store = static_cast<YuGraphicsGPUStore*>(pointer);
    if (!store) {
        return nullptr;
    }
    if (mipmaps && store->mipsStale && store->texture.mipmapLevelCount > 1) {
        @autoreleasepool {
            id<MTLCommandBuffer> commands = yuGraphicsShaderCommandBuffer();
            id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
            [blit generateMipmapsForTexture:store->texture];
            [blit endEncoding];
            [commands commit];
        }
        store->mipsStale = false;
    }
    return (void*)store->texture;
}

bool
yuGraphicsGPUUpload(void* pointer, const uint8_t* pixels, size_t bytesPerRow, const int32_t* region) {
    YuGraphicsGPUStore* store = static_cast<YuGraphicsGPUStore*>(pointer);
    int32_t width = region[2] - region[0];
    int32_t height = region[3] - region[1];
    if (!store || !pixels || width <= 0 || height <= 0) {
        return false;
    }
    size_t rowBytes = static_cast<size_t>(width) * store->bytesPerPixel;
    @autoreleasepool {
        id<MTLBuffer> staging = [yuGraphicsShaderDevice newBufferWithLength:rowBytes * static_cast<size_t>(height)
                                                                    options:MTLResourceStorageModeShared];
        if (!staging) {
            return false;
        }
        uint8_t* bytes = static_cast<uint8_t*>(staging.contents);
        const uint8_t* source = pixels + static_cast<size_t>(region[1]) * bytesPerRow +
            static_cast<size_t>(region[0]) * store->bytesPerPixel;
        for (int32_t row = 0; row < height; ++row) {
            memcpy(bytes + static_cast<size_t>(row) * rowBytes, source + static_cast<size_t>(row) * bytesPerRow, rowBytes);
        }
        id<MTLCommandBuffer> commands = yuGraphicsShaderCommandBuffer();
        id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
        [blit copyFromBuffer:staging
                 sourceOffset:0
            sourceBytesPerRow:rowBytes
          sourceBytesPerImage:rowBytes * static_cast<size_t>(height)
                   sourceSize:MTLSizeMake(static_cast<NSUInteger>(width), static_cast<NSUInteger>(height), 1)
                    toTexture:store->texture
             destinationSlice:0
             destinationLevel:0
            destinationOrigin:MTLOriginMake(static_cast<NSUInteger>(region[0]), static_cast<NSUInteger>(region[1]), 0)];
        [blit endEncoding];
        [commands commit];
        [staging release];
    }
    store->mipsStale = true;
    return true;
}

bool
yuGraphicsGPUReadback(void* pointer, uint8_t* pixels, size_t bytesPerRow, const int32_t* region) {
    YuGraphicsGPUStore* store = static_cast<YuGraphicsGPUStore*>(pointer);
    int32_t width = region[2] - region[0];
    int32_t height = region[3] - region[1];
    if (!store || !pixels || width <= 0 || height <= 0) {
        return false;
    }
    size_t rowBytes = static_cast<size_t>(width) * store->bytesPerPixel;
    bool done = false;
    @autoreleasepool {
        id<MTLBuffer> staging = [yuGraphicsShaderDevice newBufferWithLength:rowBytes * static_cast<size_t>(height)
                                                                    options:MTLResourceStorageModeShared];
        if (!staging) {
            return false;
        }
        id<MTLCommandBuffer> commands = yuGraphicsShaderCommandBuffer();
        id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
        [blit copyFromTexture:store->texture
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake(static_cast<NSUInteger>(region[0]), static_cast<NSUInteger>(region[1]), 0)
                       sourceSize:MTLSizeMake(static_cast<NSUInteger>(width), static_cast<NSUInteger>(height), 1)
                         toBuffer:staging
                destinationOffset:0
           destinationBytesPerRow:rowBytes
         destinationBytesPerImage:rowBytes * static_cast<size_t>(height)];
        [blit endEncoding];
        [commands commit];
        [commands waitUntilCompleted];
        if (commands.status == MTLCommandBufferStatusCompleted) {
            const uint8_t* bytes = static_cast<const uint8_t*>(staging.contents);
            uint8_t* target = pixels + static_cast<size_t>(region[1]) * bytesPerRow +
                static_cast<size_t>(region[0]) * store->bytesPerPixel;
            for (int32_t row = 0; row < height; ++row) {
                memcpy(target + static_cast<size_t>(row) * bytesPerRow, bytes + static_cast<size_t>(row) * rowBytes, rowBytes);
            }
            done = true;
        }
        [staging release];
    }
    return done;
}

bool
yuGraphicsGPURender(
    void* pointer,
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
) {
    YuGraphicsGPUStore* store = static_cast<YuGraphicsGPUStore*>(pointer);
    YuGraphicsShaderEntry* entry = yuGraphicsShaderEntry(shader);
    int32_t width = region[2] - region[0];
    int32_t height = region[3] - region[1];
    if (!store || !entry || width <= 0 || height <= 0 || argCount < 0 || !(scale > 0) ||
            imageCount < 0 || imageCount > kShaderImageLimit) {
        return false;
    }
    id<MTLComputePipelineState> pipeline =
        yuGraphicsShaderKernelFor(entry, entry->render, yuGraphicsShaderRenderKernel, "render");
    if (!pipeline) {
        return false;
    }
    @autoreleasepool {
        id<MTLCommandBuffer> commands = yuGraphicsShaderCommandBuffer();
        void* inputs[kShaderImageLimit] = {};
        id<MTLTexture> copy = nil;
        for (int32_t slot = 0; slot < imageCount; ++slot) {
            inputs[slot] = images[slot];
            if (images[slot] != (void*)store->texture) {
                continue;
            }
            if (!copy) {
                MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
                    texture2DDescriptorWithPixelFormat:store->texture.pixelFormat
                                                 width:store->texture.width
                                                height:store->texture.height
                                             mipmapped:NO];
                descriptor.usage = MTLTextureUsageShaderRead;
                descriptor.storageMode = MTLStorageModePrivate;
                copy = [yuGraphicsShaderDevice newTextureWithDescriptor:descriptor];
                if (!copy) {
                    return false;
                }
                id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
                [blit copyFromTexture:store->texture
                          sourceSlice:0
                          sourceLevel:0
                         sourceOrigin:MTLOriginMake(0, 0, 0)
                           sourceSize:MTLSizeMake(store->texture.width, store->texture.height, 1)
                            toTexture:copy
                     destinationSlice:0
                     destinationLevel:0
                    destinationOrigin:MTLOriginMake(0, 0, 0)];
                [blit endEncoding];
            }
            inputs[slot] = (void*)copy;
        }
        std::vector<float> values = yuGraphicsShaderArgs(args, argCount);
        YuGraphicsShaderRenderParams params = {
            {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1])},
            {static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
            {static_cast<float>(size[0]), static_cast<float>(size[1])},
            static_cast<float>(scale),
            static_cast<uint32_t>(argCount),
            static_cast<uint32_t>(imageCount),
            static_cast<uint32_t>(space),
            store->texture.pixelFormat == MTLPixelFormatRGBA16Float ? 0u : 1u,
            0,
        };
        id<MTLComputeCommandEncoder> encoder = [commands computeCommandEncoder];
        [encoder setComputePipelineState:pipeline];
        [encoder setTexture:store->texture atIndex:0];
        [encoder setBytes:&params length:sizeof(params) atIndex:0];
        [encoder setBytes:values.data() length:values.size() * sizeof(float) atIndex:1];
        yuGraphicsShaderBindImages(encoder, 1, 2, inputs, frames, imageCount);
        yuGraphicsShaderDispatch(encoder, pipeline, width, height);
        [encoder endEncoding];
        [commands commit];
        [copy release];
    }
    store->mipsStale = true;
    return true;
}
