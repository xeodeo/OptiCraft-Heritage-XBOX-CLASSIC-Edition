// RenderAPI_RSX_PS3.cpp -- platform/RenderAPI.h on the PS3's RSX (PSL1GHT librsx).
//
// RenderAPI is an OpenGL 1.x-shaped API. Like RenderAPI_D3D8_XBOX.cpp (which this
// file follows closely) it emulates the GL state machine, but the RSX has no
// fixed-function pipeline, so one vertex/fragment program pair stands in for it
// (src/ps3/shaders/opticraft.vcg/.fcg):
//
//  * Matrices are kept in GL layout (column-major float[16], M = M * op) so
//    renderGetMatrix() returns what the frustum culler expects. The vertex
//    program gets ModelViewProjection and ModelView as uniforms.
//  * Texture unit 0 is the bound texture, unit 1 the lightmap (Minecraft's sky
//    and block light, a 16x16 texture), both multiplied with the vertex color
//    (GL_MODULATE). A unit the game leaves disabled samples a 1x1 white texture.
//  * Fixed-function lighting stays off, as on the Xbox: the game's light is the
//    lightmap. Fog is computed in the fragment program from uniforms.
//  * Alpha test, blending, depth, culling and color mask are RSX hardware state.
//  * Immediate draws convert the game's interleaved vertices into one 32-byte
//    layout in the per-frame transient ring (main memory the RSX reads).
//  * Display lists record commands (GL_COMPILE: not executed while recording);
//    draws inside a list are converted once into RSX local memory.
//  * Textures are linear A8R8G8B8 in RSX local memory (big-endian: bytes A, R,
//    G, B), written by the CPU. The game hands over RGBA bytes.
#ifdef PS3_PLATFORM

#include "platform/RenderAPI.h"
#include "platform/Log.h"
#include "ps3/render/Ps3Rsx.h"

#include "opticraft_vpo.h"
#include "opticraft_fpo.h"

#include <rsx/rsx.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace
{

// ---------------------------------------------------------------------------
// Matrices (GL layout: m[column * 4 + row])
// ---------------------------------------------------------------------------
struct Mat4
{
    float m[16];
};

Mat4 identity()
{
    Mat4 r = {};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 multiply(const Mat4& a, const Mat4& b)
{
    Mat4 r;
    for (int c = 0; c < 4; ++c)
    {
        for (int row = 0; row < 4; ++row)
        {
            r.m[c * 4 + row] = a.m[0 * 4 + row] * b.m[c * 4 + 0] + a.m[1 * 4 + row] * b.m[c * 4 + 1] +
                               a.m[2 * 4 + row] * b.m[c * 4 + 2] + a.m[3 * 4 + row] * b.m[c * 4 + 3];
        }
    }
    return r;
}

Mat4 s_modelView = identity();
Mat4 s_projection = identity();
Mat4 s_textureMatrix = identity();
std::vector<Mat4> s_modelViewStack;
std::vector<Mat4> s_projectionStack;
std::vector<Mat4> s_textureStack;
// GL keeps one texture matrix per texture unit. Minecraft loads the lightmap's
// (scale 1/256, +8 texel centre) while unit 1 is active; that one must not touch
// unit 0's, or every terrain UV is divided by 256 and all of it samples the
// atlas' top-left texel (flat-coloured blocks). The lightmap coordinate is
// computed in convertVertices, so unit 1's matrix is only kept, never applied.
Mat4 s_unit1TextureMatrix = identity();
std::vector<Mat4> s_unit1TextureStack;
RenderMatrixMode s_matrixMode = RenderMatrixMode::ModelView;
bool s_matricesDirty = true;
// GL texture unit selected by renderSetActiveTextureUnit (0x84C0 = unit 0).
// Unit 1 is the lightmap: its enable/disable/bind calls must not touch unit 0.
int s_activeTextureUnit = 0;

Mat4& currentMatrix()
{
    switch (s_matrixMode)
    {
        case RenderMatrixMode::Projection: return s_projection;
        case RenderMatrixMode::Texture: return s_activeTextureUnit == 0 ? s_textureMatrix : s_unit1TextureMatrix;
        default: return s_modelView;
    }
}

std::vector<Mat4>& currentStack()
{
    switch (s_matrixMode)
    {
        case RenderMatrixMode::Projection: return s_projectionStack;
        case RenderMatrixMode::Texture: return s_activeTextureUnit == 0 ? s_textureStack : s_unit1TextureStack;
        default: return s_modelViewStack;
    }
}

void postMultiply(const Mat4& op)
{
    Mat4& m = currentMatrix();
    m = multiply(m, op);
    s_matricesDirty = true;
}

// ---------------------------------------------------------------------------
// Programs
// ---------------------------------------------------------------------------
rsxVertexProgram* s_vertexProgram = nullptr;
rsxFragmentProgram* s_fragmentProgram = nullptr;
void* s_vertexUcode = nullptr;
std::uint32_t s_fragmentOffset = 0;
rsxProgramConst* s_uMvp = nullptr;
rsxProgramConst* s_uModelView = nullptr;
rsxProgramConst* s_uTextureMatrix = nullptr;
rsxProgramConst* s_uLightConst = nullptr;
rsxProgramConst* s_uFogColor = nullptr;
rsxProgramConst* s_uFogParams = nullptr;
rsxProgramConst* s_uFogWeights = nullptr;
rsxProgramConst* s_uAlphaTest = nullptr;
bool s_programsReady = false;
// Texture units the fragment program reads the texture and the lightmap from.
// cgcomp assigns them; TEXUNITn in the source is not guaranteed to survive.
std::uint8_t s_textureUnit = 0;
std::uint8_t s_lightmapUnit = 1;
std::uint32_t s_programFrame = 0;  // frame the programs were last loaded in

// ---------------------------------------------------------------------------
// Render state mirrored from GL
// ---------------------------------------------------------------------------
bool s_texture2D = false;
int s_boundTexture = 0;
int s_boundOtherUnit = 0;   // texture bound while a unit other than 0 was active
bool s_lightmapOn = false;  // GL_TEXTURE_2D enabled on unit 1
// Unit 1 coordinate set by renderSetMultiTextureCoord (OpenGlHelper::
// setLightmapTextureCoords): raw 0..240 block/sky light. Full light by default.
float s_lightU = 240.0f;
float s_lightV = 240.0f;

std::uint8_t s_currentColor[4] = {255, 255, 255, 255};  // R, G, B, A
std::uint32_t s_clearColor = 0xFF000000u;                // A, R, G, B
float s_clearDepth = 1.0f;
int s_viewport[4] = {0, 0, 1280, 720};

bool s_fogEnabled = false;
float s_fogStart = 0.0f, s_fogEnd = 1.0f, s_fogDensity = 1.0f;
RenderFogMode s_fogMode = RenderFogMode::Exp;
bool s_fogRadial = false;
float s_fogColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
bool s_fogDirty = true;

// GL_ALPHA_TEST, done in the fragment program (see opticraft.fcg).
bool s_alphaTestEnabled = false;
float s_alphaRef = 0.0f;
bool s_alphaTestDirty = true;

bool s_cullEnabled = false;
RenderFace s_cullFace = RenderFace::Back;

// GL's counter-clockwise front faces stay CCW for the RSX even with the viewport's
// y flip (the RSX resolves winding with the same orientation the viewport uses).
// Measured in RPCS3: CW culled the tops of every full block (holes in the ground).
constexpr std::uint32_t kGlFrontFace = GCM_FRONTFACE_CCW;

std::uint32_t uploadTarget()
{
    return static_cast<std::uint32_t>(s_activeTextureUnit == 0 ? s_boundTexture : s_boundOtherUnit);
}

gcmContextData* ctx()
{
    return Ps3Rsx::context();
}

std::uint8_t clampByte(float v)
{
    if (v <= 0.0f) return 0;
    if (v >= 1.0f) return 255;
    return static_cast<std::uint8_t>(v * 255.0f + 0.5f);
}

std::uint32_t toGcmCompare(RenderCompare c)
{
    switch (c)
    {
        case RenderCompare::Never: return GCM_NEVER;
        case RenderCompare::Less: return GCM_LESS;
        case RenderCompare::Equal: return GCM_EQUAL;
        case RenderCompare::LessEqual: return GCM_LEQUAL;
        case RenderCompare::Greater: return GCM_GREATER;
        case RenderCompare::NotEqual: return GCM_NOTEQUAL;
        case RenderCompare::GreaterEqual: return GCM_GEQUAL;
        default: return GCM_ALWAYS;
    }
}

std::uint16_t toGcmBlend(RenderBlendFactor f)
{
    switch (f)
    {
        case RenderBlendFactor::Zero: return GCM_ZERO;
        case RenderBlendFactor::One: return GCM_ONE;
        case RenderBlendFactor::SrcColor: return GCM_SRC_COLOR;
        case RenderBlendFactor::OneMinusSrcColor: return GCM_ONE_MINUS_SRC_COLOR;
        case RenderBlendFactor::SrcAlpha: return GCM_SRC_ALPHA;
        case RenderBlendFactor::OneMinusSrcAlpha: return GCM_ONE_MINUS_SRC_ALPHA;
        case RenderBlendFactor::DstAlpha: return GCM_DST_ALPHA;
        case RenderBlendFactor::OneMinusDstAlpha: return GCM_ONE_MINUS_DST_ALPHA;
        case RenderBlendFactor::DstColor: return GCM_DST_COLOR;
        case RenderBlendFactor::OneMinusDstColor: return GCM_ONE_MINUS_DST_COLOR;
        default: return GCM_ONE;
    }
}

std::uint32_t toGcmPrimitive(RenderPrimitive p)
{
    switch (p)
    {
        case RenderPrimitive::Points: return GCM_TYPE_POINTS;
        case RenderPrimitive::Lines: return GCM_TYPE_LINES;
        case RenderPrimitive::LineLoop: return GCM_TYPE_LINE_LOOP;
        case RenderPrimitive::LineStrip: return GCM_TYPE_LINE_STRIP;
        case RenderPrimitive::TriangleStrip: return GCM_TYPE_TRIANGLE_STRIP;
        case RenderPrimitive::TriangleFan: return GCM_TYPE_TRIANGLE_FAN;
        case RenderPrimitive::Quads: return GCM_TYPE_QUADS;
        default: return GCM_TYPE_TRIANGLES;
    }
}

// Live renderer experiments from the test builds' remote control ("DEBUG <flag>").
bool s_debugNoCull = false;
bool s_debugFlipFront = false;

void applyCull()
{
    if (!Ps3Rsx::ready()) return;
    if (!s_cullEnabled || s_cullFace == RenderFace::FrontAndBack || s_debugNoCull)
    {
        rsxSetCullFaceEnable(ctx(), GCM_FALSE);
        return;
    }
    rsxSetFrontFace(ctx(), s_debugFlipFront ? (kGlFrontFace == GCM_FRONTFACE_CW ? GCM_FRONTFACE_CCW : GCM_FRONTFACE_CW)
                                            : kGlFrontFace);
    rsxSetCullFace(ctx(), s_cullFace == RenderFace::Back ? GCM_CULL_BACK : GCM_CULL_FRONT);
    rsxSetCullFaceEnable(ctx(), GCM_TRUE);
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------
struct Texture
{
    void* memory = nullptr;  // RSX local memory, linear A8R8G8B8
    std::uint32_t offset = 0;
    int width = 0;
    int height = 0;
    std::uint32_t pitch = 0;
    bool blur = false;
    bool clamp = false;
};

std::unordered_map<int, Texture> s_textures;
int s_nextTexture = 1;
bool s_textureCacheDirty = false;

void ensureStorage(Texture& t, int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    if (t.memory != nullptr && t.width == width && t.height == height)
        return;
    if (t.memory != nullptr)
        Ps3Rsx::freeLocal(t.memory);
    t.width = width;
    t.height = height;
    t.pitch = static_cast<std::uint32_t>(width) * 4u;
    t.memory = Ps3Rsx::allocLocal(t.pitch * static_cast<std::uint32_t>(height), 128, &t.offset);
    if (t.memory != nullptr)
        std::memset(t.memory, 0, t.pitch * static_cast<std::uint32_t>(height));
}

// RGBA bytes -> A8R8G8B8 big-endian (bytes A, R, G, B).
void writeRows(Texture& t, int x, int y, int width, int height, const unsigned char* src)
{
    if (t.memory == nullptr || src == nullptr)
        return;
    for (int row = 0; row < height; ++row)
    {
        const int ty = y + row;
        if (ty < 0 || ty >= t.height) continue;
        const int cols = (x + width > t.width) ? t.width - x : width;
        if (cols <= 0 || x < 0) continue;
        const unsigned char* in = src + static_cast<std::size_t>(row) * width * 4;
        std::uint32_t* out = reinterpret_cast<std::uint32_t*>(static_cast<unsigned char*>(t.memory) +
                                                              static_cast<std::size_t>(ty) * t.pitch) + x;
        for (int i = 0; i < cols; ++i)
        {
            const unsigned char* p = in + i * 4;
            out[i] = (static_cast<std::uint32_t>(p[3]) << 24) | (static_cast<std::uint32_t>(p[0]) << 16) |
                     (static_cast<std::uint32_t>(p[1]) << 8) | static_cast<std::uint32_t>(p[2]);
        }
    }
    s_textureCacheDirty = true;
}

void bindUnit(std::uint8_t unit, std::uint32_t offset, int width, int height, std::uint32_t pitch, bool blur,
              bool clamp)
{
    gcmTexture texture;
    std::memset(&texture, 0, sizeof(texture));
    texture.format = GCM_TEXTURE_FORMAT_A8R8G8B8 | GCM_TEXTURE_FORMAT_LIN;
    texture.mipmap = 1;
    texture.dimension = GCM_TEXTURE_DIMS_2D;
    texture.cubemap = GCM_FALSE;
    texture.remap = (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_B_SHIFT) |
                    (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_G_SHIFT) |
                    (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_R_SHIFT) |
                    (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_A_SHIFT) |
                    (GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_B_SHIFT) |
                    (GCM_TEXTURE_REMAP_COLOR_G << GCM_TEXTURE_REMAP_COLOR_G_SHIFT) |
                    (GCM_TEXTURE_REMAP_COLOR_R << GCM_TEXTURE_REMAP_COLOR_R_SHIFT) |
                    (GCM_TEXTURE_REMAP_COLOR_A << GCM_TEXTURE_REMAP_COLOR_A_SHIFT);
    texture.width = static_cast<u16>(width);
    texture.height = static_cast<u16>(height);
    texture.depth = 1;
    texture.location = GCM_LOCATION_RSX;
    texture.pitch = pitch;
    texture.offset = offset;
    rsxLoadTexture(ctx(), unit, &texture);
    rsxTextureControl(ctx(), unit, GCM_TRUE, 0 << 8, 12 << 8, GCM_TEXTURE_MAX_ANISO_1);
    const std::uint8_t filter = blur ? GCM_TEXTURE_LINEAR : GCM_TEXTURE_NEAREST;
    rsxTextureFilter(ctx(), unit, 0, filter, filter, GCM_TEXTURE_CONVOLUTION_QUINCUNX);
    const std::uint8_t wrap = clamp ? GCM_TEXTURE_CLAMP_TO_EDGE : GCM_TEXTURE_REPEAT;
    rsxTextureWrapMode(ctx(), unit, wrap, wrap, GCM_TEXTURE_CLAMP_TO_EDGE, 0, GCM_TEXTURE_ZFUNC_NEVER, 0);
}

void bindWhite(std::uint8_t unit)
{
    bindUnit(unit, Ps3Rsx::whiteTextureOffset(), 1, 1, 128, false, true);
}

// What is currently bound on each unit, to skip redundant texture commands.
struct UnitSignature
{
    std::uint32_t offset = 0xFFFFFFFFu;
    int width = 0, height = 0;
    bool blur = false, clamp = false;
};
UnitSignature s_unitSent[2];
std::uint32_t s_unitFrame = 0;

void applyUnit(std::uint8_t unit, bool enabled, int textureId, bool lightmap)
{
    UnitSignature wanted;
    const Texture* tex = nullptr;
    if (enabled && textureId > 0)
    {
        auto it = s_textures.find(textureId);
        if (it != s_textures.end() && it->second.memory != nullptr)
            tex = &it->second;
    }
    if (tex != nullptr)
    {
        wanted.offset = tex->offset;
        wanted.width = tex->width;
        wanted.height = tex->height;
        // The lightmap is always filtered and clamped, as GL's unit 1 setup does.
        wanted.blur = lightmap ? true : tex->blur;
        wanted.clamp = lightmap ? true : tex->clamp;
    }
    else
    {
        wanted.offset = Ps3Rsx::whiteTextureOffset();
        wanted.width = wanted.height = 1;
        wanted.clamp = true;
    }
    const std::uint8_t hardwareUnit = lightmap ? s_lightmapUnit : s_textureUnit;
    UnitSignature& sent = s_unitSent[unit];
    if (s_unitFrame == Ps3Rsx::frameNumber() && sent.offset == wanted.offset && sent.width == wanted.width &&
        sent.height == wanted.height && sent.blur == wanted.blur && sent.clamp == wanted.clamp)
        return;
    sent = wanted;
    if (tex != nullptr)
        bindUnit(hardwareUnit, tex->offset, tex->width, tex->height, tex->pitch, wanted.blur, wanted.clamp);
    else
        bindWhite(hardwareUnit);
}

// ---------------------------------------------------------------------------
// Vertex layout
// ---------------------------------------------------------------------------
// lu/lv: lightmap coordinate (texture unit 1), already in 0..1.
struct Ps3Vertex
{
    float x, y, z;
    std::uint8_t r, g, b, a;
    float u, v;
    float lu, lv;
};
static_assert(sizeof(Ps3Vertex) == 32, "Ps3Vertex must stay 32 bytes");

// Lightmap coordinate of a raw 0..240 light value, as GL's unit-1 texture
// matrix makes it (EntityRenderer::enableLightmap): (value + 8) / 256.
inline float lightmapCoord(float value)
{
    return (value + 8.0f) * (1.0f / 256.0f);
}

void convertVertices(const RenderInterleavedMesh& mesh, Ps3Vertex* out, bool colorWhite)
{
    const unsigned char* base = static_cast<const unsigned char*>(mesh.data) +
                                static_cast<std::size_t>(mesh.first) * mesh.stride;
    const float constU = lightmapCoord(s_lightU);
    const float constV = lightmapCoord(s_lightV);
    for (int i = 0; i < mesh.count; ++i)
    {
        const unsigned char* src = base + static_cast<std::size_t>(i) * mesh.stride;
        Ps3Vertex& v = out[i];
        if (mesh.positionShort)
        {
            const short* p = reinterpret_cast<const short*>(src);
            v.x = p[0];
            v.y = p[1];
            v.z = p[2];
        }
        else
        {
            const float* p = reinterpret_cast<const float*>(src);
            v.x = p[0];
            v.y = p[1];
            v.z = p[2];
        }
        if (mesh.hasColor)
        {
            // The Tessellator keeps the bytes in R, G, B, A order on both endians.
            const unsigned char* c = src + mesh.colorOffset;
            v.r = c[0];
            v.g = c[1];
            v.b = c[2];
            v.a = c[3];
        }
        else if (colorWhite)
        {
            v.r = v.g = v.b = v.a = 255;
        }
        else
        {
            v.r = s_currentColor[0];
            v.g = s_currentColor[1];
            v.b = s_currentColor[2];
            v.a = s_currentColor[3];
        }
        if (mesh.hasTexture)
        {
            const float* uv = reinterpret_cast<const float*>(src + mesh.texCoordOffset);
            v.u = uv[0];
            v.v = uv[1];
        }
        else
        {
            v.u = v.v = 0.0f;
        }
        if (mesh.hasBrightness)
        {
            // A native int: sky light in the high 16 bits, block light in the low 16
            // (GL reads them as the two shorts of the unit-1 coordinate).
            std::uint32_t packed;
            std::memcpy(&packed, src + mesh.brightnessOffset, sizeof(packed));
            v.lu = lightmapCoord(static_cast<short>(packed & 0xFFFFu));
            v.lv = lightmapCoord(static_cast<short>(packed >> 16));
        }
        else
        {
            v.lu = constU;
            v.lv = constV;
        }
    }
}

std::vector<Ps3Vertex> s_scratch;

// Diagnostics for the first frames: how many draws, and where the first vertex of
// the frame lands in clip space with the current matrices.
int s_diagFrame = 0;
int s_diagDraws = 0;
bool s_diagLoggedVertex = false;

void diagnoseDraw(const Ps3Vertex* v, int count, RenderPrimitive primitive)
{
    ++s_diagDraws;
    if (s_diagFrame > 3 || s_diagLoggedVertex || count <= 0)
        return;
    s_diagLoggedVertex = true;
    const Mat4 mvp = multiply(s_projection, s_modelView);
    float clip[4];
    for (int r = 0; r < 4; ++r)
        clip[r] = mvp.m[0 * 4 + r] * v->x + mvp.m[1 * 4 + r] * v->y + mvp.m[2 * 4 + r] * v->z + mvp.m[3 * 4 + r];
    MC_LOG_INFO("ps3.render", "frame %d first draw: prim=%d n=%d pos=(%.1f %.1f %.1f) rgba=%02x%02x%02x%02x uv=(%.3f %.3f) "
                "clip=(%.2f %.2f %.2f w=%.2f) tex2D=%d tex=%d lightmap=%d\n",
                s_diagFrame, static_cast<int>(primitive), count, v->x, v->y, v->z, v->r, v->g, v->b, v->a, v->u, v->v,
                clip[0], clip[1], clip[2], clip[3], s_texture2D ? 1 : 0, s_boundTexture, s_lightmapOn ? 1 : 0);
}

// ---------------------------------------------------------------------------
// Program state
// ---------------------------------------------------------------------------
bool loadPrograms()
{
    if (s_programsReady)
        return true;
    if (!Ps3Rsx::ready())
        return false;
    s_vertexProgram = reinterpret_cast<rsxVertexProgram*>(const_cast<unsigned char*>(opticraft_vpo));
    s_fragmentProgram = reinterpret_cast<rsxFragmentProgram*>(const_cast<unsigned char*>(opticraft_fpo));
    u32 vertexSize = 0;
    rsxVertexProgramGetUCode(s_vertexProgram, &s_vertexUcode, &vertexSize);
    void* fragmentUcode = nullptr;
    u32 fragmentSize = 0;
    rsxFragmentProgramGetUCode(s_fragmentProgram, &fragmentUcode, &fragmentSize);
    void* fragmentMemory = Ps3Rsx::allocLocal(fragmentSize, 64, &s_fragmentOffset);
    if (fragmentMemory == nullptr)
        return false;
    std::memcpy(fragmentMemory, fragmentUcode, fragmentSize);

    s_uMvp = rsxVertexProgramGetConst(s_vertexProgram, "modelViewProjection");
    s_uModelView = rsxVertexProgramGetConst(s_vertexProgram, "modelView");
    s_uTextureMatrix = rsxVertexProgramGetConst(s_vertexProgram, "textureMatrix");
    s_uLightConst = rsxVertexProgramGetConst(s_vertexProgram, "lightConst");
    s_uFogColor = rsxVertexProgramGetConst(s_vertexProgram, "fogColor");
    s_uFogParams = rsxVertexProgramGetConst(s_vertexProgram, "fogParams");
    s_uFogWeights = rsxVertexProgramGetConst(s_vertexProgram, "fogWeights");
    s_uAlphaTest = rsxVertexProgramGetConst(s_vertexProgram, "alphaTest");
    if (rsxProgramAttrib* texture = rsxFragmentProgramGetAttrib(s_fragmentProgram, "texture0"))
        s_textureUnit = static_cast<std::uint8_t>(texture->index);
    if (rsxProgramAttrib* lightmap = rsxFragmentProgramGetAttrib(s_fragmentProgram, "lightmap"))
        s_lightmapUnit = static_cast<std::uint8_t>(lightmap->index);
    MC_LOG_INFO("ps3.render", "samplers: texture0=unit %u lightmap=unit %u\n", s_textureUnit, s_lightmapUnit);
    MC_LOG_INFO("ps3.render", "programs: vp=%u fp=%u bytes, uniforms mvp=%d mv=%d tm=%d lc=%d fc=%d fp=%d fw=%d\n",
                vertexSize, fragmentSize, s_uMvp != nullptr, s_uModelView != nullptr,
                s_uTextureMatrix != nullptr, s_uLightConst != nullptr, s_uFogColor != nullptr,
                s_uFogParams != nullptr, s_uFogWeights != nullptr);
    s_programsReady = true;
    return true;
}

void setVertexUniform(rsxProgramConst* uniform, const float* values)
{
    if (uniform != nullptr)
        rsxSetVertexProgramParameter(ctx(), s_vertexProgram, uniform, values);
}

void applyFog()
{
    float params[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    float weights[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // fog off: factor 1
    if (s_fogEnabled)
    {
        weights[3] = 0.0f;
        const float range = s_fogEnd - s_fogStart;
        if (s_fogMode == RenderFogMode::Linear || s_fogMode == RenderFogMode::EyeRadial)
        {
            if (std::fabs(range) > 1e-6f)
            {
                params[0] = -1.0f / range;
                params[1] = s_fogEnd / range;
            }
            weights[0] = 1.0f;
        }
        else if (s_fogMode == RenderFogMode::Exp2)
        {
            weights[2] = 1.0f;
        }
        else
        {
            weights[1] = 1.0f;
        }
        params[2] = s_fogDensity;
    }
    // Vertex constants: see opticraft.vcg for why fog is not a fragment uniform.
    setVertexUniform(s_uFogColor, s_fogColor);
    setVertexUniform(s_uFogParams, params);
    setVertexUniform(s_uFogWeights, weights);
}

float s_sentLightConst[4] = {-1.0f, -1.0f, -1.0f, -1.0f};

void setLightConst(bool useConstant)
{
    const float values[4] = {lightmapCoord(s_lightU), lightmapCoord(s_lightV), useConstant ? 1.0f : 0.0f,
                             s_fogRadial ? 1.0f : 0.0f};
    if (std::memcmp(values, s_sentLightConst, sizeof(values)) == 0)
        return;
    std::memcpy(s_sentLightConst, values, sizeof(values));
    setVertexUniform(s_uLightConst, values);
}

// Everything a draw depends on, sent only when it changed.
bool applyDrawState(bool constantLight)
{
    if (!loadPrograms())
        return false;
    if (s_programFrame != Ps3Rsx::frameNumber() || s_programFrame == 0)
    {
        // New frame: re-establish the program and the uniforms once.
        rsxLoadVertexProgram(ctx(), s_vertexProgram, s_vertexUcode);
        rsxLoadFragmentProgramLocation(ctx(), s_fragmentProgram, s_fragmentOffset, GCM_LOCATION_RSX);
        s_programFrame = Ps3Rsx::frameNumber();
        s_matricesDirty = true;
        s_fogDirty = true;
        s_alphaTestDirty = true;
        std::memset(s_sentLightConst, 0xFF, sizeof(s_sentLightConst));
    }
    if (s_alphaTestDirty)
    {
        const float values[4] = {s_alphaTestEnabled ? 1.0f : 0.0f, s_alphaRef, 0.0f, 0.0f};
        setVertexUniform(s_uAlphaTest, values);
        s_alphaTestDirty = false;
    }
    if (s_matricesDirty)
    {
        const Mat4 mvp = multiply(s_projection, s_modelView);
        setVertexUniform(s_uMvp, mvp.m);
        setVertexUniform(s_uModelView, s_modelView.m);
        setVertexUniform(s_uTextureMatrix, s_textureMatrix.m);
        s_matricesDirty = false;
    }
    if (s_fogDirty)
    {
        applyFog();
        s_fogDirty = false;
    }
    setLightConst(constantLight);
    if (s_textureCacheDirty)
    {
        rsxInvalidateTextureCache(ctx(), GCM_INVALIDATE_TEXTURE);
        s_textureCacheDirty = false;
    }
    if (s_unitFrame != Ps3Rsx::frameNumber())
    {
        s_unitSent[0] = UnitSignature();
        s_unitSent[1] = UnitSignature();
    }
    applyUnit(0, s_texture2D, s_boundTexture, false);
    applyUnit(1, s_lightmapOn, s_boundOtherUnit, true);
    s_unitFrame = Ps3Rsx::frameNumber();
    return true;
}

void bindVertexArrays(std::uint32_t offset, std::uint32_t location)
{
    rsxBindVertexArrayAttrib(ctx(), GCM_VERTEX_ATTRIB_POS, 0, offset + 0, sizeof(Ps3Vertex), 3,
                             GCM_VERTEX_DATA_TYPE_F32, location);
    rsxBindVertexArrayAttrib(ctx(), GCM_VERTEX_ATTRIB_COLOR0, 0, offset + 12, sizeof(Ps3Vertex), 4,
                             GCM_VERTEX_DATA_TYPE_U8, location);
    rsxBindVertexArrayAttrib(ctx(), GCM_VERTEX_ATTRIB_TEX0, 0, offset + 16, sizeof(Ps3Vertex), 2,
                             GCM_VERTEX_DATA_TYPE_F32, location);
    rsxBindVertexArrayAttrib(ctx(), GCM_VERTEX_ATTRIB_TEX1, 0, offset + 24, sizeof(Ps3Vertex), 2,
                             GCM_VERTEX_DATA_TYPE_F32, location);
}

int usableCount(RenderPrimitive primitive, int count)
{
    switch (primitive)
    {
        case RenderPrimitive::Quads: return count - count % 4;
        case RenderPrimitive::Triangles: return count - count % 3;
        case RenderPrimitive::Lines: return count - count % 2;
        case RenderPrimitive::TriangleFan:
        case RenderPrimitive::TriangleStrip: return count < 3 ? 0 : count;
        default: return count;
    }
}

bool drawNow(const RenderInterleavedMesh& mesh)
{
    if (!Ps3Rsx::ready()) return true;
    if (mesh.data == nullptr || mesh.stride <= 0 || mesh.count <= 0) return false;
    const int count = usableCount(mesh.primitive, mesh.count);
    if (count <= 0) return true;
    std::uint32_t offset = 0;
    Ps3Vertex* out = static_cast<Ps3Vertex*>(
        Ps3Rsx::allocTransient(static_cast<std::uint32_t>(mesh.count) * sizeof(Ps3Vertex), 128, &offset));
    if (out == nullptr) return true;  // ring full this frame: drop the draw (logged once)
    convertVertices(mesh, out, false);
    diagnoseDraw(out, count, mesh.primitive);
    if (!applyDrawState(false)) return true;
    bindVertexArrays(offset, Ps3Rsx::kTransientLocation);
    rsxInvalidateVertexCache(ctx());
    rsxDrawVertexArray(ctx(), toGcmPrimitive(mesh.primitive), 0, static_cast<u32>(count));
    return true;
}

// ---------------------------------------------------------------------------
// Display lists
// ---------------------------------------------------------------------------
std::unordered_map<int, std::vector<std::function<void()>>> s_lists;
std::vector<std::function<void()>>* s_recording = nullptr;
int s_nextList = 1;
long s_recordedBytes = 0;

// GL_COMPILE: while a list is being recorded, recordable calls are stored
// instead of executed. Returns true when the caller must return immediately.
template <typename F>
bool recordIfCompiling(F&& command)
{
    if (!s_recording) return false;
    s_recording->push_back(std::forward<F>(command));
    return true;
}

// Geometry recorded into a display list, converted once into RSX local memory.
// A mesh without per-vertex color is stored white and takes the current color
// at playback (GL semantics) by being replayed through the transient ring.
struct RecordedMesh
{
    ~RecordedMesh()
    {
        s_recordedBytes -= bytes;
        Ps3Rsx::freeLocal(memory);
    }
    void* memory = nullptr;
    std::uint32_t offset = 0;
    // Meshes that take the current color are re-sent through the transient ring
    // each draw. They are read from this main-memory copy: CPU reads from RSX
    // local memory crawl, and re-reading the sky and every entity model from
    // there each frame cost over half of the frame.
    std::vector<Ps3Vertex> cpuCopy;
    int vertexCount = 0;
    long bytes = 0;
    RenderPrimitive primitive = RenderPrimitive::Triangles;
    bool usesCurrentColor = false;
    bool hasLight = false;  // per-vertex lightmap coordinate; else the current one
};

void drawRecorded(const RecordedMesh& mesh)
{
    if (!Ps3Rsx::ready() || mesh.memory == nullptr) return;
    const int count = usableCount(mesh.primitive, mesh.vertexCount);
    if (count <= 0) return;
    std::uint32_t offset = mesh.offset;
    std::uint32_t location = GCM_LOCATION_RSX;
    if (mesh.usesCurrentColor)
    {
        // Copy with the current color patched in (models are small).
        Ps3Vertex* out = static_cast<Ps3Vertex*>(
            Ps3Rsx::allocTransient(static_cast<std::uint32_t>(mesh.vertexCount) * sizeof(Ps3Vertex), 128, &offset));
        if (out == nullptr) return;
        const Ps3Vertex* in = mesh.cpuCopy.data();
        for (int i = 0; i < mesh.vertexCount; ++i)
        {
            out[i] = in[i];
            out[i].r = s_currentColor[0];
            out[i].g = s_currentColor[1];
            out[i].b = s_currentColor[2];
            out[i].a = s_currentColor[3];
        }
        location = Ps3Rsx::kTransientLocation;
    }
    static int s_recordedLogged = 0;
    if (s_recordedLogged < 12 && mesh.vertexCount >= 64)
    {
        ++s_recordedLogged;
        auto describe = [](int id, char* out, std::size_t size) {
            auto it = s_textures.find(id);
            if (it == s_textures.end())
                std::snprintf(out, size, "%d:MISSING", id);
            else
                std::snprintf(out, size, "%d:%dx%d mem=%p off=%x", id, it->second.width, it->second.height,
                              it->second.memory, it->second.offset);
        };
        char unit0[96], unit1[96];
        describe(s_boundTexture, unit0, sizeof(unit0));
        describe(s_boundOtherUnit, unit1, sizeof(unit1));
        MC_LOG_INFO("ps3.render", "list draw textures: unit0 %s | unit1 %s | sent0 off=%x %dx%d | sent1 off=%x %dx%d\n",
                    unit0, unit1, s_unitSent[0].offset, s_unitSent[0].width, s_unitSent[0].height,
                    s_unitSent[1].offset, s_unitSent[1].width, s_unitSent[1].height);
        const Ps3Vertex* v = static_cast<const Ps3Vertex*>(mesh.memory);
        MC_LOG_INFO("ps3.render", "list draw: prim=%d n=%d uv0=(%.4f %.4f) uv1=(%.4f %.4f) light=(%.3f %.3f) "
                    "tex2D=%d tex=%d unit=%d lightmapOn=%d other=%d colorless=%d\n",
                    static_cast<int>(mesh.primitive), mesh.vertexCount, v[0].u, v[0].v, v[1].u, v[1].v, v[0].lu,
                    v[0].lv, s_texture2D ? 1 : 0, s_boundTexture, s_activeTextureUnit, s_lightmapOn ? 1 : 0,
                    s_boundOtherUnit, mesh.usesCurrentColor ? 1 : 0);
    }
    if (!applyDrawState(!mesh.hasLight)) return;
    bindVertexArrays(offset, location);
    rsxInvalidateVertexCache(ctx());
    rsxDrawVertexArray(ctx(), toGcmPrimitive(mesh.primitive), 0, static_cast<u32>(count));
}

} // namespace

// ===========================================================================
// State
// ===========================================================================
void renderEnable(RenderCapability capability)
{
    if (recordIfCompiling([capability] { renderEnable(capability); })) return;
    if (!Ps3Rsx::ready()) return;
    switch (capability)
    {
        case RenderCapability::Texture2D:
            if (s_activeTextureUnit == 0) s_texture2D = true;
            else s_lightmapOn = true;
            break;
        case RenderCapability::AlphaTest: s_alphaTestEnabled = true; s_alphaTestDirty = true; break;
        case RenderCapability::Blend: rsxSetBlendEnable(ctx(), GCM_TRUE); break;
        case RenderCapability::CullFace: s_cullEnabled = true; applyCull(); break;
        case RenderCapability::DepthTest: rsxSetDepthTestEnable(ctx(), GCM_TRUE); break;
        case RenderCapability::Fog: s_fogEnabled = true; s_fogDirty = true; break;
        case RenderCapability::PolygonOffsetFill: rsxSetPolygonOffsetFillEnable(ctx(), GCM_TRUE); break;
        default: break;  // lighting/normalize/color material: see file comment
    }
}

void renderDisable(RenderCapability capability)
{
    if (recordIfCompiling([capability] { renderDisable(capability); })) return;
    if (!Ps3Rsx::ready()) return;
    switch (capability)
    {
        case RenderCapability::Texture2D:
            if (s_activeTextureUnit == 0) s_texture2D = false;
            else s_lightmapOn = false;
            break;
        case RenderCapability::AlphaTest: s_alphaTestEnabled = false; s_alphaTestDirty = true; break;
        case RenderCapability::Blend: rsxSetBlendEnable(ctx(), GCM_FALSE); break;
        case RenderCapability::CullFace: s_cullEnabled = false; applyCull(); break;
        case RenderCapability::DepthTest: rsxSetDepthTestEnable(ctx(), GCM_FALSE); break;
        case RenderCapability::Fog: s_fogEnabled = false; s_fogDirty = true; break;
        case RenderCapability::PolygonOffsetFill: rsxSetPolygonOffsetFillEnable(ctx(), GCM_FALSE); break;
        default: break;
    }
}

void renderBlendFunc(RenderBlendFactor source, RenderBlendFactor destination)
{
    if (recordIfCompiling([source, destination] { renderBlendFunc(source, destination); })) return;
    if (!Ps3Rsx::ready()) return;
    const std::uint16_t s = toGcmBlend(source);
    const std::uint16_t d = toGcmBlend(destination);
    rsxSetBlendFunc(ctx(), s, d, s, d);
    rsxSetBlendEquation(ctx(), GCM_FUNC_ADD, GCM_FUNC_ADD);
}

void renderDepthMask(bool enabled)
{
    if (recordIfCompiling([enabled] { renderDepthMask(enabled); })) return;
    if (Ps3Rsx::ready()) rsxSetDepthWriteEnable(ctx(), enabled ? GCM_TRUE : GCM_FALSE);
}

void renderDepthFunc(RenderCompare function)
{
    if (Ps3Rsx::ready()) rsxSetDepthFunc(ctx(), toGcmCompare(function));
}

void renderAlphaFunc(RenderCompare function, float reference)
{
    // Minecraft only uses GL_GREATER; the fragment program implements that one.
    (void)function;
    s_alphaRef = reference < 0.0f ? 0.0f : (reference > 1.0f ? 1.0f : reference);
    s_alphaTestDirty = true;
}

void renderCullFace(RenderFace face)
{
    s_cullFace = face;
    applyCull();
}

void renderColorMask(bool red, bool green, bool blue, bool alpha)
{
    if (!Ps3Rsx::ready()) return;
    std::uint32_t mask = 0;
    if (red) mask |= GCM_COLOR_MASK_R;
    if (green) mask |= GCM_COLOR_MASK_G;
    if (blue) mask |= GCM_COLOR_MASK_B;
    if (alpha) mask |= GCM_COLOR_MASK_A;
    rsxSetColorMask(ctx(), mask);
}

void renderShadeModel(RenderShadeModel model)
{
    if (Ps3Rsx::ready())
        rsxSetShadeModel(ctx(), model == RenderShadeModel::Flat ? GCM_SHADE_MODEL_FLAT : GCM_SHADE_MODEL_SMOOTH);
}

void renderColor4f(float r, float g, float b, float a)
{
    if (recordIfCompiling([r, g, b, a] { renderColor4f(r, g, b, a); })) return;
    s_currentColor[0] = clampByte(r);
    s_currentColor[1] = clampByte(g);
    s_currentColor[2] = clampByte(b);
    s_currentColor[3] = clampByte(a);
}

void renderColor3f(float r, float g, float b)
{
    renderColor4f(r, g, b, 1.0f);
}

void renderNormal3f(float, float, float) {}

// Unit 0 is the texture, unit 1 the lightmap.
void renderSetActiveTextureUnit(int textureUnit)
{
    if (recordIfCompiling([textureUnit] { renderSetActiveTextureUnit(textureUnit); })) return;
    s_activeTextureUnit = textureUnit >= 0x84C0 ? textureUnit - 0x84C0 : textureUnit;
}
void renderSetClientActiveTextureUnit(int) {}
void renderSetMultiTextureCoord(int textureUnit, float u, float v)
{
    if (recordIfCompiling([textureUnit, u, v] { renderSetMultiTextureCoord(textureUnit, u, v); })) return;
    const int unit = textureUnit >= 0x84C0 ? textureUnit - 0x84C0 : textureUnit;
    if (unit != 1) return;
    s_lightU = u;
    s_lightV = v;
}
void renderSetLightmapColors(const std::uint32_t*, int) {}

// ===========================================================================
// Clear / viewport / frame
// ===========================================================================
void renderClearColor(float r, float g, float b, float a)
{
    s_clearColor = (static_cast<std::uint32_t>(clampByte(a)) << 24) | (static_cast<std::uint32_t>(clampByte(r)) << 16) |
                   (static_cast<std::uint32_t>(clampByte(g)) << 8) | static_cast<std::uint32_t>(clampByte(b));
}

void renderClearDepth(double depth)
{
    s_clearDepth = static_cast<float>(depth);
}

void renderClear(unsigned int mask)
{
    if (!Ps3Rsx::ready()) return;
    std::uint32_t flags = 0;
    if (mask & RenderClearMask::Color)
    {
        rsxSetClearColor(ctx(), s_clearColor);
        flags |= GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A;
    }
    if (mask & RenderClearMask::Depth)
    {
        const float d = s_clearDepth < 0.0f ? 0.0f : (s_clearDepth > 1.0f ? 1.0f : s_clearDepth);
        rsxSetClearDepthStencil(ctx(), static_cast<std::uint32_t>(d * 16777215.0f) << 8);
        flags |= GCM_CLEAR_Z | GCM_CLEAR_S;
    }
    if (flags)
        rsxClearSurface(ctx(), flags);
}

void renderViewport(int x, int y, int width, int height)
{
    s_viewport[0] = x;
    s_viewport[1] = y;
    s_viewport[2] = width;
    s_viewport[3] = height;
    if (!Ps3Rsx::ready() || width <= 0 || height <= 0) return;
    // GL's viewport origin is bottom-left, the surface's is top-left.
    const int top = Ps3Rsx::height() - (y + height);
    const float scale[4] = {width * 0.5f, height * -0.5f, 0.5f, 0.0f};
    const float offset[4] = {x + width * 0.5f, top + height * 0.5f, 0.5f, 0.0f};
    rsxSetViewport(ctx(), static_cast<u16>(x < 0 ? 0 : x), static_cast<u16>(top < 0 ? 0 : top),
                   static_cast<u16>(width), static_cast<u16>(height), 0.0f, 1.0f, scale, offset);
    rsxSetScissor(ctx(), 0, 0, static_cast<u16>(Ps3Rsx::width()), static_cast<u16>(Ps3Rsx::height()));
}

void renderGetViewport(int* values)
{
    if (!values) return;
    for (int i = 0; i < 4; ++i) values[i] = s_viewport[i];
}

// Ps3Rsx::endFrame() already keeps at most one frame in flight.
void renderFinishGpu() {}
void renderSubmitFrame() {}

void renderPolygonOffset(float factor, float units)
{
    if (Ps3Rsx::ready()) rsxSetPolygonOffset(ctx(), factor, units);
}

void renderLineWidth(float) {}

// ===========================================================================
// Matrices
// ===========================================================================
void renderMatrixMode(RenderMatrixMode mode)
{
    if (recordIfCompiling([mode] { renderMatrixMode(mode); })) return;
    s_matrixMode = mode;
}

void renderLoadIdentity()
{
    if (recordIfCompiling([] { renderLoadIdentity(); })) return;
    currentMatrix() = identity();
    s_matricesDirty = true;
}

void renderPushMatrix()
{
    if (recordIfCompiling([] { renderPushMatrix(); })) return;
    currentStack().push_back(currentMatrix());
}

void renderPopMatrix()
{
    if (recordIfCompiling([] { renderPopMatrix(); })) return;
    std::vector<Mat4>& stack = currentStack();
    if (stack.empty()) return;
    currentMatrix() = stack.back();
    stack.pop_back();
    s_matricesDirty = true;
}

void renderTranslate(float x, float y, float z)
{
    if (recordIfCompiling([x, y, z] { renderTranslate(x, y, z); })) return;
    Mat4 t = identity();
    t.m[12] = x;
    t.m[13] = y;
    t.m[14] = z;
    postMultiply(t);
}

void renderRotate(float angle, float x, float y, float z)
{
    if (recordIfCompiling([angle, x, y, z] { renderRotate(angle, x, y, z); })) return;
    const float length = std::sqrt(x * x + y * y + z * z);
    if (length <= 0.0f) return;
    x /= length;
    y /= length;
    z /= length;
    const float radians = angle * 3.14159265358979f / 180.0f;
    const float c = std::cos(radians), s = std::sin(radians), t = 1.0f - c;
    Mat4 r = identity();
    r.m[0] = x * x * t + c;
    r.m[1] = y * x * t + z * s;
    r.m[2] = x * z * t - y * s;
    r.m[4] = x * y * t - z * s;
    r.m[5] = y * y * t + c;
    r.m[6] = y * z * t + x * s;
    r.m[8] = x * z * t + y * s;
    r.m[9] = y * z * t - x * s;
    r.m[10] = z * z * t + c;
    postMultiply(r);
}

void renderScale(float x, float y, float z)
{
    if (recordIfCompiling([x, y, z] { renderScale(x, y, z); })) return;
    Mat4 s = identity();
    s.m[0] = x;
    s.m[5] = y;
    s.m[10] = z;
    postMultiply(s);
}

void renderScaleDouble(double x, double y, double z)
{
    renderScale(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

void renderFrustum(double left, double right, double bottom, double top, double nearValue, double farValue)
{
    Mat4 f = {};
    f.m[0] = static_cast<float>(2.0 * nearValue / (right - left));
    f.m[5] = static_cast<float>(2.0 * nearValue / (top - bottom));
    f.m[8] = static_cast<float>((right + left) / (right - left));
    f.m[9] = static_cast<float>((top + bottom) / (top - bottom));
    f.m[10] = static_cast<float>(-(farValue + nearValue) / (farValue - nearValue));
    f.m[11] = -1.0f;
    f.m[14] = static_cast<float>(-2.0 * farValue * nearValue / (farValue - nearValue));
    postMultiply(f);
}

void renderOrtho(double left, double right, double bottom, double top, double nearValue, double farValue)
{
    Mat4 o = identity();
    o.m[0] = static_cast<float>(2.0 / (right - left));
    o.m[5] = static_cast<float>(2.0 / (top - bottom));
    o.m[10] = static_cast<float>(-2.0 / (farValue - nearValue));
    o.m[12] = static_cast<float>(-(right + left) / (right - left));
    o.m[13] = static_cast<float>(-(top + bottom) / (top - bottom));
    o.m[14] = static_cast<float>(-(farValue + nearValue) / (farValue - nearValue));
    postMultiply(o);
}

void renderGetMatrix(RenderMatrixQuery query, float* values)
{
    if (!values) return;
    const Mat4& m = query == RenderMatrixQuery::Projection ? s_projection
                  : query == RenderMatrixQuery::Texture ? s_textureMatrix
                  : s_modelView;
    std::memcpy(values, m.m, sizeof(m.m));
}

// ===========================================================================
// Textures
// ===========================================================================
void renderGenerateTextures(int count, int* textures)
{
    if (count <= 0 || !textures) return;
    for (int i = 0; i < count; ++i)
    {
        textures[i] = s_nextTexture++;
        s_textures[textures[i]];
    }
}

void renderDeleteTextures(int count, const int* textures)
{
    if (count <= 0 || !textures) return;
    for (int i = 0; i < count; ++i)
    {
        auto it = s_textures.find(textures[i]);
        if (it == s_textures.end()) continue;
        Ps3Rsx::freeLocal(it->second.memory);
        s_textures.erase(it);
        s_unitSent[0] = UnitSignature();
        s_unitSent[1] = UnitSignature();
    }
}

void renderBindTexture(int texture)
{
    if (recordIfCompiling([texture] { renderBindTexture(texture); })) return;
    if (s_activeTextureUnit != 0)
    {
        s_boundOtherUnit = texture;
        return;
    }
    s_boundTexture = texture;
}

bool renderTextureBeginUpload(int texture, int width, int height, int, bool blur, bool clamp, bool, bool)
{
    Texture& t = s_textures[texture];
    t.blur = blur;
    t.clamp = clamp;
    ensureStorage(t, width, height);
    return t.memory != nullptr;
}

void renderTextureImageRgba(int level, int width, int height, const void* pixels)
{
    const std::uint32_t target = uploadTarget();
    if (level != 0 || target == 0) return;  // one level: no mipmaps yet
    Texture& t = s_textures[static_cast<int>(target)];
    ensureStorage(t, width, height);
    if (pixels)
        writeRows(t, 0, 0, width, height, static_cast<const unsigned char*>(pixels));
}

void renderTextureSubImageRgba(int level, int x, int y, int width, int height, const void* pixels)
{
    const std::uint32_t target = uploadTarget();
    if (level != 0 || target == 0 || !pixels) return;
    auto it = s_textures.find(static_cast<int>(target));
    if (it == s_textures.end()) return;
    writeRows(it->second, x, y, width, height, static_cast<const unsigned char*>(pixels));
}

void renderTextureParameters(bool blur, bool, bool clamp)
{
    if (uploadTarget() == 0) return;
    Texture& t = s_textures[static_cast<int>(uploadTarget())];
    t.blur = blur;
    t.clamp = clamp;
}

void renderApplyTextureQuality(bool blur, int, bool, int)
{
    if (uploadTarget() == 0) return;
    s_textures[static_cast<int>(uploadTarget())].blur = blur;
}

int renderGetMaxAnisotropy() { return 1; }
int renderGetMaxSamples() { return 0; }

bool renderTextureIsValid(int texture)
{
    return texture > 0;
}

void renderResetResources() {}

// ===========================================================================
// Fog / lighting
// ===========================================================================
void renderFogf(RenderFogParameter parameter, float value)
{
    switch (parameter)
    {
        case RenderFogParameter::Density: s_fogDensity = value; break;
        case RenderFogParameter::Start: s_fogStart = value; break;
        case RenderFogParameter::End: s_fogEnd = value; break;
        default: return;
    }
    s_fogDirty = true;
}

void renderFogi(RenderFogParameter parameter, RenderFogMode value)
{
    if (parameter == RenderFogParameter::Mode)
    {
        s_fogMode = value;
        s_fogDirty = true;
    }
    else if (parameter == RenderFogParameter::DistanceMode)
    {
        s_fogRadial = value == RenderFogMode::EyeRadial;
    }
}

void renderFogColor(const float* values)
{
    if (!values) return;
    for (int i = 0; i < 4; ++i)
        s_fogColor[i] = values[i];
    s_fogDirty = true;
}

void renderFogHint(RenderHintMode) {}
void renderLightfv(int, RenderLightParameter, const float*) {}
void renderLightModelAmbient(const float*) {}
void renderColorMaterial(RenderFace, RenderColorMaterialMode) {}

// ===========================================================================
// Queries
// ===========================================================================
const unsigned char* renderGetString(RenderStringQuery query)
{
    switch (query)
    {
        case RenderStringQuery::Vendor: return reinterpret_cast<const unsigned char*>("NVIDIA");
        case RenderStringQuery::Renderer: return reinterpret_cast<const unsigned char*>("PS3 RSX (PSL1GHT)");
        case RenderStringQuery::Version: return reinterpret_cast<const unsigned char*>("1.1");
        default: return reinterpret_cast<const unsigned char*>("");
    }
}

bool renderSupportsFeature(RenderFeature) { return false; }
unsigned int renderGetError() { return 0; }

// ===========================================================================
// Geometry
// ===========================================================================
bool renderDrawInterleaved(const RenderInterleavedMesh& mesh)
{
    if (s_recording)
    {
        // Keep a private copy: the caller's buffer is reused right after.
        if (mesh.data == nullptr || mesh.stride <= 0 || mesh.count <= 0) return false;
        auto recorded = std::make_shared<RecordedMesh>();
        recorded->primitive = mesh.primitive;
        recorded->usesCurrentColor = !mesh.hasColor;
        recorded->hasLight = mesh.hasBrightness;
        recorded->vertexCount = mesh.count;
        const std::uint32_t bytes = static_cast<std::uint32_t>(mesh.count) * sizeof(Ps3Vertex);
        recorded->memory = Ps3Rsx::allocLocal(bytes, 128, &recorded->offset);
        if (recorded->memory != nullptr)
        {
            if (static_cast<int>(s_scratch.size()) < mesh.count)
                s_scratch.resize(static_cast<std::size_t>(mesh.count));
            convertVertices(mesh, s_scratch.data(), true);
            // Written once, straight into RSX local memory.
            std::memcpy(recorded->memory, s_scratch.data(), bytes);
            if (recorded->usesCurrentColor)
                recorded->cpuCopy.assign(s_scratch.begin(), s_scratch.begin() + mesh.count);
            recorded->bytes = static_cast<long>(bytes);
            s_recordedBytes += recorded->bytes;
        }
        s_recording->push_back([recorded] { drawRecorded(*recorded); });
        return true;
    }
    return drawNow(mesh);
}

bool renderCaptureInterleaved(const RenderInterleavedMesh& mesh, RenderCapturedMesh& out, bool append)
{
    if (mesh.data == nullptr || mesh.stride <= 0 || mesh.count <= 0) return false;
    if (!append) out.clear();
    if (!out.empty() && (out.stride != mesh.stride || out.primitive != mesh.primitive ||
                         out.positionShort != mesh.positionShort || out.hasTexture != mesh.hasTexture ||
                         (mesh.hasTexture && out.texCoordOffset != mesh.texCoordOffset) ||
                         out.hasColor != mesh.hasColor || (mesh.hasColor && out.colorOffset != mesh.colorOffset)))
        return false;
    if (out.empty())
    {
        out.stride = mesh.stride;
        out.primitive = mesh.primitive;
        out.positionShort = mesh.positionShort;
        out.hasTexture = mesh.hasTexture;
        out.texCoordOffset = mesh.texCoordOffset;
        out.hasColor = mesh.hasColor;
        out.colorOffset = mesh.colorOffset;
        out.hasNormals = mesh.hasNormals;
        out.normalOffset = mesh.normalOffset;
        out.hasBrightness = mesh.hasBrightness;
        out.brightnessOffset = mesh.brightnessOffset;
    }
    const unsigned char* src = static_cast<const unsigned char*>(mesh.data) +
                               static_cast<std::size_t>(mesh.first) * mesh.stride;
    const std::size_t bytes = static_cast<std::size_t>(mesh.count) * mesh.stride;
    const std::size_t old = out.raw.size();
    out.raw.resize(old + (bytes + 3u) / 4u);
    std::memcpy(reinterpret_cast<unsigned char*>(out.raw.data()) + old * 4u, src, bytes);
    out.vertexCount += mesh.count;
    return true;
}

bool renderDrawCaptured(const RenderCapturedMesh& mesh)
{
    if (mesh.empty()) return false;
    RenderInterleavedMesh view;
    view.data = mesh.raw.data();
    view.stride = mesh.stride;
    view.count = mesh.vertexCount;
    view.primitive = mesh.primitive;
    view.positionShort = mesh.positionShort;
    view.hasTexture = mesh.hasTexture;
    view.texCoordOffset = mesh.texCoordOffset;
    view.hasColor = mesh.hasColor;
    view.colorOffset = mesh.colorOffset;
    view.hasBrightness = mesh.hasBrightness;
    view.brightnessOffset = mesh.brightnessOffset;
    return renderDrawInterleaved(view);
}

// ===========================================================================
// Display lists (desktop 1.2.5 renderer path, used by the Xbox and the PS3)
// ===========================================================================
int renderGenerateDisplayLists(int count)
{
    if (count <= 0) return 0;
    const int first = s_nextList;
    s_nextList += count;
    return first;
}

void renderDeleteDisplayLists(int first, int count)
{
    for (int i = 0; i < count; ++i)
        s_lists.erase(first + i);
}

void renderBeginDisplayList(int list)
{
    std::vector<std::function<void()>>& commands = s_lists[list];
    commands.clear();
    s_recording = &commands;
}

void renderEndDisplayList()
{
    if (s_recording)
        s_recording->shrink_to_fit();
    s_recording = nullptr;
}

void renderCallDisplayList(int list)
{
    auto it = s_lists.find(list);
    if (it == s_lists.end()) return;
    // A list may be called from inside another list's recording.
    if (recordIfCompiling([list] { renderCallDisplayList(list); })) return;
    for (const auto& command : it->second)
        command();
}

void renderCallDisplayLists(int count, const int* lists)
{
    if (!lists) return;
    for (int i = 0; i < count; ++i)
        renderCallDisplayList(lists[i]);
}

// No occlusion queries on this backend (renderSupportsFeature says so).
void renderGenerateOcclusionQueries(int count, int* queries)
{
    if (!queries) return;
    for (int i = 0; i < count; ++i) queries[i] = 0;
}
void renderBeginOcclusionQuery(int) {}
void renderEndOcclusionQuery() {}
bool renderOcclusionQueryResultAvailable(int) { return true; }
unsigned int renderOcclusionQueryResult(int) { return 1; }

// ===========================================================================
// Framebuffer
// ===========================================================================
bool renderReadPixelsRgb(int, int, int, int, void*) { return false; }
bool renderCopyFramebufferToBoundTexture(int, int, int, int) { return false; }
// Legacy Look's presentation gamma: not wired yet on the PS3 (the RSX can do it
// with a gamma-corrected scan-out; to be measured against the Xbox picture).
void renderSetLegacyPresentationGamma(bool) {}

// Called by Display_ps3.cpp after each flip: per-frame GPU state the next frame
// must re-send (vertex program, uniforms, texture units).
#if PS3_SCREENSHOTS
// Test builds: every texture of 128x128 or more as it sits in RSX memory
// (A8R8G8B8 bytes), to /dev_hdd0/tmp/opticraft_tex_<id>_<w>x<h>.raw.
static void dumpTextures()
{
    for (const auto& entry : s_textures)
    {
        const Texture& t = entry.second;
        if (t.memory == nullptr || t.width < 128 || t.height < 128)
            continue;
        char path[96];
        std::snprintf(path, sizeof(path), "/dev_hdd0/tmp/opticraft_tex_%d_%dx%d.raw", entry.first, t.width, t.height);
        if (FILE* f = std::fopen(path, "wb"))
        {
            std::fwrite(t.memory, 1, t.pitch * static_cast<std::uint32_t>(t.height), f);
            std::fclose(f);
        }
    }
    MC_LOG_INFO("ps3.render", "textures dumped\n");
}
#endif

void ps3RenderBeginFrame()
{
#if PS3_SCREENSHOTS
    if (s_diagFrame == 3000)
        dumpTextures();
#endif
    if (s_diagFrame <= 3)
        MC_LOG_INFO("ps3.render", "frame %d done: %d draws, clear=%08x viewport=%d,%d %dx%d\n", s_diagFrame,
                    s_diagDraws, s_clearColor, s_viewport[0], s_viewport[1], s_viewport[2], s_viewport[3]);
    ++s_diagFrame;
    s_diagDraws = 0;
    s_diagLoggedVertex = false;
    s_unitFrame = 0;
    if (Ps3Rsx::ready())
    {
        int vp[4];
        renderGetViewport(vp);
        renderViewport(vp[0], vp[1], vp[2], vp[3]);
        applyCull();
    }
}

// Test builds' remote control: "DEBUG nocull" / "DEBUG cull" / "DEBUG flipfront".
void ps3RenderDebugCommand(const char* flag)
{
    if (std::strcmp(flag, "nocull") == 0)
        s_debugNoCull = true;
    else if (std::strcmp(flag, "cull") == 0)
        s_debugNoCull = false;
    else if (std::strcmp(flag, "flipfront") == 0)
        s_debugFlipFront = !s_debugFlipFront;
    MC_LOG_INFO("ps3.render", "debug %s (nocull=%d flipfront=%d)\n", flag, s_debugNoCull ? 1 : 0,
                s_debugFlipFront ? 1 : 0);
    applyCull();
}

long ps3RenderRecordedBytes()
{
    return s_recordedBytes;
}

#endif // PS3_PLATFORM
