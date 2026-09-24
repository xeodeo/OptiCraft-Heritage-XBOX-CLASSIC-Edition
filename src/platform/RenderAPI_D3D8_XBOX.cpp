// RenderAPI_D3D8_XBOX.cpp — platform/RenderAPI.h on the Xbox's Direct3D 8
// fixed-function pipeline (NV2A).
//
// RenderAPI is an OpenGL 1.x-shaped API, so this file emulates the GL state
// machine on top of D3D8, the way RenderAPI_GL.cpp forwards it on desktop:
//
//  * Matrices are kept in GL layout (column-major float[16], M = M * op) so
//    renderGetMatrix() returns exactly what the frustum culler expects. A GL
//    column-major matrix read as a D3DMATRIX is its transpose, which is what
//    D3D's row-vector convention wants, so ModelView uploads as-is (as the
//    VIEW transform, WORLD stays identity). Projection is pre-multiplied by a
//    z remap from GL's [-w,w] clip range to D3D's [0,w].
//  * Textures are swizzled A8R8G8B8 (Xbox linear textures would need texel
//    coordinates). A CPU copy of level 0 is kept so sub-image updates (the
//    animated water/lava/fire tiles) can re-swizzle.
//  * Immediate draws convert the game's interleaved vertices (float3/short3
//    position, float2 UV, RGBA8 color) to one FVF and use DrawVerticesUP.
//  * Display lists record commands (GL_COMPILE semantics: not executed while
//    recording) and replay them; draws inside a list keep a copy of the data.
//  * Fixed-function lighting is left off: the terrain and GUI bake brightness
//    into vertex colors; entity shading is a later refinement.
#ifdef XBOX_PLATFORM

#include "platform/RenderAPI.h"
#include "xbox/render/XboxD3D.h"
#include "platform/Log.h"
#include "xbox/system/XboxVideoMode.h"

#include <xgraphics.h>

#include <intrin.h>
#include <mmintrin.h>

#include <cmath>
#include <cstring>
#include <functional>
#include <iterator>
#include <map>
#include <unordered_map>
#include <memory>
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
RenderMatrixMode s_matrixMode = RenderMatrixMode::ModelView;
bool s_matricesDirty = true;

Mat4& currentMatrix()
{
    switch (s_matrixMode)
    {
        case RenderMatrixMode::Projection: return s_projection;
        case RenderMatrixMode::Texture: return s_textureMatrix;
        default: return s_modelView;
    }
}

std::vector<Mat4>& currentStack()
{
    switch (s_matrixMode)
    {
        case RenderMatrixMode::Projection: return s_projectionStack;
        case RenderMatrixMode::Texture: return s_textureStack;
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
// Render state mirrored from GL
// ---------------------------------------------------------------------------
bool s_defaultsApplied = false;
bool s_texture2D = false;
int s_boundTexture = 0;
// GL texture unit selected by renderSetActiveTextureUnit (0x84C0 = unit 0).
// Only unit 0 exists here; unit 1 is the desktop lightmap, which the Xbox
// path does not use, so its enable/disable/bind calls must not touch unit 0.
int s_activeTextureUnit = 0;
int s_boundOtherUnit = 0;  // texture bound while a unit other than 0 was active

// Texture that uploads/parameters apply to: the one bound on the active unit.
int uploadTarget()
{
    return s_activeTextureUnit == 0 ? s_boundTexture : s_boundOtherUnit;
}
DWORD s_currentColor = 0xFFFFFFFF;
DWORD s_clearColor = 0xFF000000;
float s_clearDepth = 1.0f;
int s_viewport[4] = {0, 0, 640, 480};
bool s_fogEnabled = false;
float s_fogStart = 0.0f, s_fogEnd = 1.0f, s_fogDensity = 1.0f;
RenderFogMode s_fogMode = RenderFogMode::Exp;
bool s_cullEnabled = false;
RenderFace s_cullFace = RenderFace::Back;

// Redundant-state filter. The game sets the same states over and over (every
// draw re-applies the texture stage); each Set* call costs CPU time in the
// D3D runtime and push-buffer space, so only changes reach the device.
DWORD s_rsValue[D3DRS_MAX];
bool s_rsKnown[D3DRS_MAX] = {};
DWORD s_tssValue[D3DTSS_MAXSTAGES][D3DTSS_MAX];
bool s_tssKnown[D3DTSS_MAXSTAGES][D3DTSS_MAX] = {};
IDirect3DBaseTexture8* s_stageTexture[D3DTSS_MAXSTAGES] = {};
bool s_stageTextureKnown[D3DTSS_MAXSTAGES] = {};
DWORD s_vertexShader = 0;
bool s_vertexShaderKnown = false;

void setRs(IDirect3DDevice8* d, D3DRENDERSTATETYPE state, DWORD value)
{
    const unsigned index = static_cast<unsigned>(state);
    if (index < D3DRS_MAX)
    {
        if (s_rsKnown[index] && s_rsValue[index] == value)
            return;
        s_rsKnown[index] = true;
        s_rsValue[index] = value;
    }
    d->SetRenderState(state, value);
}

void setTss(IDirect3DDevice8* d, DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
    const unsigned index = static_cast<unsigned>(type);
    if (stage < D3DTSS_MAXSTAGES && index < D3DTSS_MAX)
    {
        if (s_tssKnown[stage][index] && s_tssValue[stage][index] == value)
            return;
        s_tssKnown[stage][index] = true;
        s_tssValue[stage][index] = value;
    }
    d->SetTextureStageState(stage, type, value);
}

void setTex(IDirect3DDevice8* d, DWORD stage, IDirect3DBaseTexture8* texture)
{
    if (stage < D3DTSS_MAXSTAGES)
    {
        if (s_stageTextureKnown[stage] && s_stageTexture[stage] == texture)
            return;
        s_stageTextureKnown[stage] = true;
        s_stageTexture[stage] = texture;
    }
    d->SetTexture(stage, texture);
}

void setVs(IDirect3DDevice8* d, DWORD shader)
{
    if (s_vertexShaderKnown && s_vertexShader == shader)
        return;
    s_vertexShaderKnown = true;
    s_vertexShader = shader;
    d->SetVertexShader(shader);
}

IDirect3DDevice8* device()
{
    IDirect3DDevice8* d = g_pD3DDevice;
    if (d && !s_defaultsApplied)
    {
        s_defaultsApplied = true;
        // GL defaults. D3D8 starts with lighting ON, which would black out
        // every FVF vertex without normals.
        setRs(d, D3DRS_LIGHTING, FALSE);
        setRs(d, D3DRS_CULLMODE, D3DCULL_NONE);
        setRs(d, D3DRS_ZENABLE, D3DZB_FALSE);
        setRs(d, D3DRS_ZWRITEENABLE, TRUE);
        setRs(d, D3DRS_ZFUNC, D3DCMP_LESS);
        setRs(d, D3DRS_ALPHABLENDENABLE, FALSE);
        setRs(d, D3DRS_SRCBLEND, D3DBLEND_ONE);
        setRs(d, D3DRS_DESTBLEND, D3DBLEND_ZERO);
        setRs(d, D3DRS_ALPHATESTENABLE, FALSE);
        setRs(d, D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
        setRs(d, D3DRS_ALPHAREF, 0);
        setRs(d, D3DRS_FOGENABLE, FALSE);
        setRs(d, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
        const D3DMATRIX* id = reinterpret_cast<const D3DMATRIX*>(identity().m);
        d->SetTransform(D3DTS_WORLD, id);
        setTss(d, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        setTss(d, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }
    return d;
}

DWORD toD3DColor(float r, float g, float b, float a)
{
    auto clamp255 = [](float v) -> DWORD {
        if (v <= 0.0f) return 0;
        if (v >= 1.0f) return 255;
        return static_cast<DWORD>(v * 255.0f + 0.5f);
    };
    return (clamp255(a) << 24) | (clamp255(r) << 16) | (clamp255(g) << 8) | clamp255(b);
}

D3DCMPFUNC toD3DCompare(RenderCompare c)
{
    switch (c)
    {
        case RenderCompare::Never: return D3DCMP_NEVER;
        case RenderCompare::Less: return D3DCMP_LESS;
        case RenderCompare::Equal: return D3DCMP_EQUAL;
        case RenderCompare::LessEqual: return D3DCMP_LESSEQUAL;
        case RenderCompare::Greater: return D3DCMP_GREATER;
        case RenderCompare::NotEqual: return D3DCMP_NOTEQUAL;
        case RenderCompare::GreaterEqual: return D3DCMP_GREATEREQUAL;
        default: return D3DCMP_ALWAYS;
    }
}

D3DBLEND toD3DBlend(RenderBlendFactor f)
{
    switch (f)
    {
        case RenderBlendFactor::Zero: return D3DBLEND_ZERO;
        case RenderBlendFactor::One: return D3DBLEND_ONE;
        case RenderBlendFactor::SrcColor: return D3DBLEND_SRCCOLOR;
        case RenderBlendFactor::OneMinusSrcColor: return D3DBLEND_INVSRCCOLOR;
        case RenderBlendFactor::SrcAlpha: return D3DBLEND_SRCALPHA;
        case RenderBlendFactor::OneMinusSrcAlpha: return D3DBLEND_INVSRCALPHA;
        case RenderBlendFactor::DstAlpha: return D3DBLEND_DESTALPHA;
        case RenderBlendFactor::OneMinusDstAlpha: return D3DBLEND_INVDESTALPHA;
        case RenderBlendFactor::DstColor: return D3DBLEND_DESTCOLOR;
        case RenderBlendFactor::OneMinusDstColor: return D3DBLEND_INVDESTCOLOR;
        default: return D3DBLEND_ONE;
    }
}

void applyCull()
{
    IDirect3DDevice8* d = device();
    if (!d) return;
    if (!s_cullEnabled || s_cullFace == RenderFace::FrontAndBack)
    {
        setRs(d, D3DRS_CULLMODE, D3DCULL_NONE);
        return;
    }
    // GL fronts are counter-clockwise as seen on screen, and D3D's CULLMODE is
    // also defined by the on-screen winding, so GL back faces are the ones
    // that appear clockwise: cull CW for GL_BACK, CCW for GL_FRONT.
    setRs(d, D3DRS_CULLMODE, s_cullFace == RenderFace::Back ? D3DCULL_CW : D3DCULL_CCW);
}

void applyFog()
{
    IDirect3DDevice8* d = device();
    if (!d) return;
    setRs(d, D3DRS_FOGENABLE, s_fogEnabled ? TRUE : FALSE);
    if (!s_fogEnabled) return;
    switch (s_fogMode)
    {
        case RenderFogMode::Linear:
        case RenderFogMode::EyeRadial:
            setRs(d, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
            break;
        case RenderFogMode::Exp2:
            setRs(d, D3DRS_FOGTABLEMODE, D3DFOG_EXP2);
            break;
        default:
            setRs(d, D3DRS_FOGTABLEMODE, D3DFOG_EXP);
            break;
    }
    setRs(d, D3DRS_FOGSTART, *reinterpret_cast<const DWORD*>(&s_fogStart));
    setRs(d, D3DRS_FOGEND, *reinterpret_cast<const DWORD*>(&s_fogEnd));
    setRs(d, D3DRS_FOGDENSITY, *reinterpret_cast<const DWORD*>(&s_fogDensity));
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------
struct Texture
{
    IDirect3DTexture8* d3d = nullptr;
    int width = 0;           // image size as the game uploads it
    int height = 0;
    int storageWidth = 0;    // power-of-two size of the swizzled D3D texture
    int storageHeight = 0;
    // Level 0, linear A8R8G8B8, image size. Only kept while needed: until the
    // D3D texture exists, for non-power-of-two images (every upload resamples
    // it), and for dynamic textures that receive sub-image updates (animated
    // terrain/items tiles). Static power-of-two textures drop it after upload.
    std::vector<DWORD> pixels;
    bool dynamic = false;
    bool dirty = false;         // pixels not yet in the D3D texture
    bool blur = false;
    bool clamp = false;
};

std::unordered_map<int, Texture> s_textures;

// Per-frame draw statistics for the periodic profile report.
struct DrawStats
{
    unsigned long long drawCycles = 0;   // CPU time inside D3D draw calls (push-buffer stalls land here)
    long vbDraws = 0;
    long upDraws = 0;
    long upVertices = 0;
    long viewSets = 0;
    unsigned long long transformCycles = 0;   // SetTransform (view + projection)
    unsigned long long listCycles = 0;        // top-level display-list replay, everything included
    unsigned long long stageCycles = 0;       // applyTextureStage
    long listCalls = 0;
};
int s_listDepth = 0;
DrawStats s_drawStats;
int s_nextTexture = 1;

bool isPowerOfTwo(int v)
{
    return v > 0 && (v & (v - 1)) == 0;
}

int nextPowerOfTwo(int v)
{
    int p = 1;
    while (p < v) p <<= 1;
    return p;
}

std::vector<DWORD> s_resampled;

bool ensureD3DTexture(Texture& t)
{
    if (t.d3d) return true;
    IDirect3DDevice8* d = g_pD3DDevice;
    if (!d || t.storageWidth <= 0 || t.storageHeight <= 0) return false;
    return SUCCEEDED(d->CreateTexture(t.storageWidth, t.storageHeight, 1, 0, D3DFMT_A8R8G8B8, 0, &t.d3d));
}

void convertRgba(const unsigned char* src, DWORD* dst, int count)
{
    for (int i = 0; i < count; ++i)
    {
        dst[i] = (static_cast<DWORD>(src[3]) << 24) | (static_cast<DWORD>(src[0]) << 16) |
                 (static_cast<DWORD>(src[1]) << 8) | static_cast<DWORD>(src[2]);
        src += 4;
    }
}

void swizzleUpload(Texture& t)
{
    // Textures can be uploaded before Display::create() made the device; the
    // pixels are kept and the D3D texture is created on the first upload or
    // bind that happens with a device.
    if (t.pixels.empty()) return;
    if (!ensureD3DTexture(t))
    {
        t.dirty = true;
        return;
    }
    // Swizzled textures must be power-of-two. UVs are normalized, so a
    // nearest-neighbour resample to the storage size keeps every texel where
    // the game expects it (exact for the power-of-two atlases, which skip it).
    const DWORD* source = t.pixels.data();
    if (t.storageWidth != t.width || t.storageHeight != t.height)
    {
        s_resampled.resize(static_cast<size_t>(t.storageWidth) * t.storageHeight);
        for (int y = 0; y < t.storageHeight; ++y)
        {
            const int sy = y * t.height / t.storageHeight;
            const DWORD* row = t.pixels.data() + static_cast<size_t>(sy) * t.width;
            DWORD* out = s_resampled.data() + static_cast<size_t>(y) * t.storageWidth;
            for (int x = 0; x < t.storageWidth; ++x)
                out[x] = row[x * t.width / t.storageWidth];
        }
        source = s_resampled.data();
    }
    D3DLOCKED_RECT locked;
    if (FAILED(t.d3d->LockRect(0, &locked, NULL, 0))) return;
    XGSwizzleRect(source, t.storageWidth * 4, NULL, locked.pBits, t.storageWidth, t.storageHeight, NULL, 4);
    // The XDK swizzler is MMX code; MMX aliases the x87 register stack, and
    // without EMMS every following x87 operation (all double math: world gen,
    // physics, pathfinding) produces NaN. Clear the MMX state unconditionally.
    _mm_empty();
    t.d3d->UnlockRect(0);
    t.dirty = false;
    if (!t.dynamic)
        std::vector<DWORD>().swap(t.pixels);   // the GPU copy is the only one needed
}

// Brings back the CPU copy of a texture from its swizzled D3D texture, the
// first time it receives a sub-image update. A non-power-of-two image was
// only ever enlarged by the nearest-neighbour resample, so every image texel
// is still present in the storage and comes back exactly.
bool restorePixels(Texture& t)
{
    if (!t.d3d || t.width <= 0 || t.height <= 0) return false;
    const bool resampled = t.storageWidth != t.width || t.storageHeight != t.height;
    D3DLOCKED_RECT locked;
    if (FAILED(t.d3d->LockRect(0, &locked, NULL, 0))) return false;
    t.pixels.assign(static_cast<size_t>(t.width) * t.height, 0);
    if (!resampled)
    {
        XGUnswizzleRect(locked.pBits, t.storageWidth, t.storageHeight, NULL, t.pixels.data(), t.width * 4, NULL, 4);
    }
    else
    {
        s_resampled.resize(static_cast<size_t>(t.storageWidth) * t.storageHeight);
        XGUnswizzleRect(locked.pBits, t.storageWidth, t.storageHeight, NULL, s_resampled.data(), t.storageWidth * 4, NULL, 4);
        // Storage texel s holds image texel s * size / storage; the first
        // storage texel showing image texel i is ceil(i * storage / size).
        for (int y = 0; y < t.height; ++y)
        {
            const int sy = (y * t.storageHeight + t.height - 1) / t.height;
            const DWORD* row = s_resampled.data() + static_cast<size_t>(sy) * t.storageWidth;
            DWORD* out = t.pixels.data() + static_cast<size_t>(y) * t.width;
            for (int x = 0; x < t.width; ++x)
                out[x] = row[(x * t.storageWidth + t.width - 1) / t.width];
        }
    }
    _mm_empty();   // see swizzleUpload
    t.d3d->UnlockRect(0);
    return true;
}

void ensureStorage(Texture& t, int width, int height)
{
    if (t.d3d && t.width == width && t.height == height) return;
    if (t.d3d)
    {
        t.d3d->Release();
        t.d3d = nullptr;
    }
    t.width = width;
    t.height = height;
    t.storageWidth = nextPowerOfTwo(width);
    t.storageHeight = nextPowerOfTwo(height);
    t.pixels.assign(static_cast<size_t>(width) * height, 0);
    t.dirty = false;
    if (width > 0 && height > 0)
        ensureD3DTexture(t);
}


// Stage-0 setup last sent by applyTextureStage. Terrain replays hundreds of
// draws with the same texture and colour source; when nothing changed the
// eleven filtered state writes below are skipped. Anything else that touches
// stage 0 (texture deletion) clears s_stageValid.
struct StageSignature
{
    IDirect3DTexture8* texture;
    DWORD filter;
    DWORD address;
    DWORD colorArg;
};
StageSignature s_stageSent = {};
bool s_stageValid = false;

// colorFromFactor: the vertices carry no colour and the current colour comes
// from D3DRS_TEXTUREFACTOR instead (display lists without per-vertex colour).
void applyTextureStage(bool colorFromFactor = false)
{
    const unsigned long long stageStart = __rdtsc();
    struct StageTimer
    {
        unsigned long long start;
        ~StageTimer() { s_drawStats.stageCycles += __rdtsc() - start; }
    } stageTimer{stageStart};
    const DWORD colorArg = colorFromFactor ? D3DTA_TFACTOR : D3DTA_DIFFUSE;
    IDirect3DDevice8* d = device();
    if (!d) return;
    Texture* tex = nullptr;
    if (s_texture2D && s_boundTexture > 0)
    {
        auto it = s_textures.find(s_boundTexture);
        if (it != s_textures.end())
        {
            if (it->second.dirty || (!it->second.d3d && !it->second.pixels.empty()))
                swizzleUpload(it->second);
            if (it->second.d3d)
                tex = &it->second;
        }
    }
    StageSignature wanted = {};
    wanted.colorArg = colorArg;
    if (tex)
    {
        wanted.texture = tex->d3d;
        wanted.filter = tex->blur ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        wanted.address = tex->clamp ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
    }
    if (s_stageValid && wanted.texture == s_stageSent.texture && wanted.filter == s_stageSent.filter &&
        wanted.address == s_stageSent.address && wanted.colorArg == s_stageSent.colorArg)
        return;
    s_stageSent = wanted;
    s_stageValid = true;
    if (tex)
    {
        setTex(d, 0, tex->d3d);
        const DWORD filter = tex->blur ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        const DWORD address = tex->clamp ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
        setTss(d, 0, D3DTSS_MINFILTER, filter);
        setTss(d, 0, D3DTSS_MAGFILTER, filter);
        setTss(d, 0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
        setTss(d, 0, D3DTSS_ADDRESSU, address);
        setTss(d, 0, D3DTSS_ADDRESSV, address);
        setTss(d, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        setTss(d, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        setTss(d, 0, D3DTSS_COLORARG2, colorArg);
        setTss(d, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        setTss(d, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        setTss(d, 0, D3DTSS_ALPHAARG2, colorArg);
    }
    else
    {
        setTex(d, 0, NULL);
        setTss(d, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        setTss(d, 0, D3DTSS_COLORARG1, colorArg);
        setTss(d, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        setTss(d, 0, D3DTSS_ALPHAARG1, colorArg);
    }
}

void applyMatrices()
{
    if (!s_matricesDirty) return;
    IDirect3DDevice8* d = device();
    if (!d) return;
    s_matricesDirty = false;
    // Every chunk section moves the modelview; the projection stays the same
    // for the whole pass. Each SetTransform makes the driver rebuild the
    // composite matrix, so only send what actually changed.
    static Mat4 s_sentView, s_sentProjection;
    static bool s_sentValid = false;
    if (!s_sentValid || std::memcmp(s_sentView.m, s_modelView.m, sizeof(s_modelView.m)) != 0)
    {
        s_sentView = s_modelView;
        ++s_drawStats.viewSets;
        const unsigned long long start = __rdtsc();
        d->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(s_modelView.m));
        s_drawStats.transformCycles += __rdtsc() - start;
    }
    if (!s_sentValid || std::memcmp(s_sentProjection.m, s_projection.m, sizeof(s_projection.m)) != 0)
    {
        s_sentProjection = s_projection;
        // GL clip z is [-w, w]; D3D wants [0, w]: z' = 0.5 z + 0.5 w.
        Mat4 zFix = identity();
        zFix.m[10] = 0.5f;
        zFix.m[14] = 0.5f;
        const Mat4 projection = multiply(zFix, s_projection);
        d->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(projection.m));
    }
    s_sentValid = true;
}

// ---------------------------------------------------------------------------
// Display lists
// ---------------------------------------------------------------------------
std::unordered_map<int, std::vector<std::function<void()>>> s_lists;
std::vector<std::function<void()>>* s_recording = nullptr;
int s_nextList = 1;

// GL_COMPILE: while a list is being recorded, recordable calls are stored
// instead of executed. Returns true when the caller must return immediately.
template <typename F>
bool recordIfCompiling(F&& command)
{
    if (!s_recording) return false;
    s_recording->push_back(std::forward<F>(command));
    return true;
}

// ---------------------------------------------------------------------------
// Vertex conversion
// ---------------------------------------------------------------------------
struct XboxVertex
{
    float x, y, z;
    DWORD color;
    float u, v;
};
const DWORD kXboxVertexFvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

// Compact layout for display lists kept in vertex buffers (all terrain):
// 16 bytes instead of 24. Chunk sections are built in section-local
// coordinates, so positions fit a short at 1/1024 of a block (error below
// 1/2000 block, far under a texel) and atlas UVs a normalized short (error
// 1/65000, well inside the atlas' texel inset). The world transform scales
// positions back. Meshes whose values do not fit keep the 24-byte layout.
struct XboxCompactVertex
{
    short x, y, z, w;   // w = 1
    DWORD color;
    short u, v;
};
// Same without colour (entity models, items): the colour comes from the
// texture factor at playback, as GL takes the current colour.
struct XboxCompactVertexNoColor
{
    short x, y, z, w;   // w = 1
    short u, v;
};
constexpr float kCompactPositionScale = 1024.0f;
constexpr float kCompactPositionLimit = 32767.0f / kCompactPositionScale;
DWORD s_compactShader = 0;
bool s_compactShaderTried = false;
DWORD s_compactNoColorShader = 0;
bool s_worldCompact = false;   // D3DTS_WORLD holds the compact scale

DWORD compactShader(IDirect3DDevice8* d)
{
    if (!s_compactShaderTried)
    {
        s_compactShaderTried = true;
        const DWORD decl[] = {
            D3DVSD_STREAM(0),
            D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_SHORT4),
            D3DVSD_REG(D3DVSDE_DIFFUSE, D3DVSDT_D3DCOLOR),
            D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_NORMSHORT2),
            D3DVSD_END()
        };
        DWORD handle = 0;
        if (SUCCEEDED(d->CreateVertexShader(decl, NULL, &handle, 0)))
            s_compactShader = handle;
        const DWORD declNoColor[] = {
            D3DVSD_STREAM(0),
            D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_SHORT4),
            D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_NORMSHORT2),
            D3DVSD_END()
        };
        handle = 0;
        if (SUCCEEDED(d->CreateVertexShader(declNoColor, NULL, &handle, 0)))
            s_compactNoColorShader = handle;
        MC_LOG_INFO("xbox.render", "compact vertex shaders %s/%s\n", s_compactShader ? "ready" : "FAILED",
                    s_compactNoColorShader ? "ready" : "FAILED");
    }
    return s_compactShader;
}

void setWorldCompact(IDirect3DDevice8* d, bool compact)
{
    if (s_worldCompact == compact) return;
    s_worldCompact = compact;
    D3DMATRIX m;
    std::memset(&m, 0, sizeof(m));
    const float scale = compact ? 1.0f / kCompactPositionScale : 1.0f;
    m._11 = m._22 = m._33 = scale;
    m._44 = 1.0f;
    d->SetTransform(D3DTS_WORLD, &m);
}

bool fitsCompact(const XboxVertex* v, int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (!(v[i].x >= -kCompactPositionLimit && v[i].x <= kCompactPositionLimit &&
              v[i].y >= -kCompactPositionLimit && v[i].y <= kCompactPositionLimit &&
              v[i].z >= -kCompactPositionLimit && v[i].z <= kCompactPositionLimit &&
              v[i].u >= -1.0f && v[i].u <= 1.0f && v[i].v >= -1.0f && v[i].v <= 1.0f))
            return false;
    }
    return true;
}

short quantize(float value, float scale)
{
    const float q = value * scale;
    return static_cast<short>(q >= 0.0f ? q + 0.5f : q - 0.5f);
}

void writeCompact(const XboxVertex* in, int count, XboxCompactVertex* out)
{
    for (int i = 0; i < count; ++i)
    {
        XboxCompactVertex c;
        c.x = quantize(in[i].x, kCompactPositionScale);
        c.y = quantize(in[i].y, kCompactPositionScale);
        c.z = quantize(in[i].z, kCompactPositionScale);
        c.w = 1;
        c.color = in[i].color;
        c.u = quantize(in[i].u, 32767.0f);
        c.v = quantize(in[i].v, 32767.0f);
        out[i] = c;   // one 16-byte store run into write-combined memory
    }
}

void writeCompactNoColor(const XboxVertex* in, int count, XboxCompactVertexNoColor* out)
{
    for (int i = 0; i < count; ++i)
    {
        XboxCompactVertexNoColor c;
        c.x = quantize(in[i].x, kCompactPositionScale);
        c.y = quantize(in[i].y, kCompactPositionScale);
        c.z = quantize(in[i].z, kCompactPositionScale);
        c.w = 1;
        c.u = quantize(in[i].u, 32767.0f);
        c.v = quantize(in[i].v, 32767.0f);
        out[i] = c;
    }
}

std::vector<XboxVertex> s_scratch;

D3DPRIMITIVETYPE toD3DPrimitive(RenderPrimitive p)
{
    switch (p)
    {
        case RenderPrimitive::Points: return D3DPT_POINTLIST;
        case RenderPrimitive::Lines: return D3DPT_LINELIST;
        case RenderPrimitive::LineLoop: return D3DPT_LINESTRIP;
        case RenderPrimitive::LineStrip: return D3DPT_LINESTRIP;
        case RenderPrimitive::TriangleStrip: return D3DPT_TRIANGLESTRIP;
        case RenderPrimitive::TriangleFan: return D3DPT_TRIANGLEFAN;
        case RenderPrimitive::Quads: return D3DPT_QUADLIST;
        default: return D3DPT_TRIANGLELIST;
    }
}

// Converts a GL-style interleaved mesh into the fixed Xbox vertex layout.
// Vertices without per-vertex color take the current color.
void convertVertices(const RenderInterleavedMesh& mesh, XboxVertex* out)
{
    const unsigned char* base = static_cast<const unsigned char*>(mesh.data) +
                                static_cast<size_t>(mesh.first) * mesh.stride;
    for (int i = 0; i < mesh.count; ++i)
    {
        const unsigned char* src = base + static_cast<size_t>(i) * mesh.stride;
        XboxVertex& v = out[i];
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
            const unsigned char* c = src + mesh.colorOffset;
            v.color = (static_cast<DWORD>(c[3]) << 24) | (static_cast<DWORD>(c[0]) << 16) |
                      (static_cast<DWORD>(c[1]) << 8) | static_cast<DWORD>(c[2]);
        }
        else
        {
            v.color = s_currentColor;
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
    }
}

void submitVertices(RenderPrimitive primitive, const XboxVertex* vertices, int count)
{
    IDirect3DDevice8* d = device();
    if (!d || count <= 0) return;

    applyMatrices();
    applyTextureStage();
    setWorldCompact(d, false);
    setVs(d, kXboxVertexFvf);

    // DrawVerticesUP copies the vertices inline into the push buffer; a whole
    // chunk section in one call overflows what one inline submission may hold
    // and the NV2A rejects the stream ("reserved pb command"). Independent
    // primitive lists split cleanly on primitive boundaries; strips, fans and
    // loops are small in this game and go through whole.
    const D3DPRIMITIVETYPE type = toD3DPrimitive(primitive);
    int perPrimitive = 0;
    switch (primitive)
    {
        case RenderPrimitive::Quads: perPrimitive = 4; break;
        case RenderPrimitive::Triangles: perPrimitive = 3; break;
        case RenderPrimitive::Lines: perPrimitive = 2; break;
        case RenderPrimitive::Points: perPrimitive = 1; break;
        default: break;
    }
    const int kMaxBatchVertices = 1020;  // multiple of 1, 2, 3 and 4
    // A trailing partial primitive is an invalid push-buffer draw on the NV2A.
    if (perPrimitive > 0)
        count -= count % perPrimitive;
    if (count <= 0 || (primitive == RenderPrimitive::TriangleFan && count < 3))
        return;
    ++s_drawStats.upDraws;
    s_drawStats.upVertices += count;
    if (count <= kMaxBatchVertices)
    {
        d->DrawVerticesUP(type, static_cast<UINT>(count), vertices, sizeof(XboxVertex));
        return;
    }
    if (primitive == RenderPrimitive::TriangleFan)
    {
        // Each piece is its own fan around the shared first vertex.
        static XboxVertex fan[kMaxBatchVertices];
        fan[0] = vertices[0];
        for (int start = 1; start + 1 < count; start += kMaxBatchVertices - 2)
        {
            int n = count - start;
            if (n > kMaxBatchVertices - 1) n = kMaxBatchVertices - 1;
            std::memcpy(fan + 1, vertices + start, static_cast<size_t>(n) * sizeof(XboxVertex));
            d->DrawVerticesUP(type, static_cast<UINT>(n + 1), fan, sizeof(XboxVertex));
        }
        return;
    }
    if (perPrimitive == 0)
    {
        // Strips: consecutive pieces overlap by the vertices a primitive
        // shares (2 for triangles, 1 for lines); an even step keeps the
        // triangle winding of every piece.
        const int overlap = (primitive == RenderPrimitive::TriangleStrip) ? 2 : 1;
        const int step = kMaxBatchVertices - 4;
        for (int start = 0; start + overlap < count; start += step)
        {
            int n = count - start;
            if (n > step + overlap) n = step + overlap;
            d->DrawVerticesUP(type, static_cast<UINT>(n), vertices + start, sizeof(XboxVertex));
        }
        return;
    }
    const int usable = count - (count % perPrimitive);
    for (int start = 0; start < usable; start += kMaxBatchVertices)
    {
        const int n = (usable - start < kMaxBatchVertices) ? usable - start : kMaxBatchVertices;
        d->DrawVerticesUP(type, static_cast<UINT>(n), vertices + start, sizeof(XboxVertex));
    }
}

bool drawNow(const RenderInterleavedMesh& mesh)
{
    if (!device()) return true;
    if (mesh.data == nullptr || mesh.stride <= 0 || mesh.count <= 0) return false;

    const bool loop = mesh.primitive == RenderPrimitive::LineLoop;
    const int count = mesh.count + (loop ? 1 : 0);
    if (static_cast<int>(s_scratch.size()) < count)
        s_scratch.resize(static_cast<size_t>(count));
    convertVertices(mesh, s_scratch.data());
    if (loop)
        s_scratch[static_cast<size_t>(mesh.count)] = s_scratch[0];
    submitVertices(mesh.primitive, s_scratch.data(), count);
    return true;
}

// Geometry recorded into a display list, already in the Xbox layout (24
// bytes a vertex instead of the tessellator's 32) so playback submits it
// directly. Meshes without per-vertex color pick up the current color when
// played back, as GL would.
long s_recordedBytes = 0;

// ---- Static vertex memory for display lists --------------------------------
// One vertex buffer per mesh would round every chunk section up to whole 4 KB
// pages of contiguous memory (thousands of small meshes = megabytes wasted).
// Meshes instead take ranges of shared 512 KB vertex buffers (small enough
// that pools drain and get released as the world changes); a mesh larger than
// that gets a pool of its own. A freed range is tagged with a GPU fence and
// only reused once the GPU has passed it.
constexpr UINT kVertexPoolBytes = 512 * 1024;
constexpr UINT kVertexGranule = 96;   // multiple of both vertex sizes (24, 16), 32-byte aligned

struct VertexPool
{
    IDirect3DVertexBuffer8* vb = nullptr;
    BYTE* base = nullptr;                    // CPU address of the buffer memory
    UINT size = 0;                           // bytes
    std::map<UINT, UINT> freeRanges;         // offset -> size
};

struct PendingRange
{
    int pool;
    UINT offset;
    UINT size;
    DWORD fence;
};

std::vector<VertexPool> s_vertexPools;
std::vector<PendingRange> s_pendingRanges;

UINT roundToGranule(UINT bytes)
{
    return (bytes + kVertexGranule - 1) / kVertexGranule * kVertexGranule;
}

void releaseRangeNow(int pool, UINT offset, UINT size)
{
    std::map<UINT, UINT>& ranges = s_vertexPools[static_cast<size_t>(pool)].freeRanges;
    auto next = ranges.lower_bound(offset);
    if (next != ranges.begin())
    {
        auto prev = std::prev(next);
        if (prev->first + prev->second == offset)
        {
            offset = prev->first;
            size += prev->second;
            ranges.erase(prev);
        }
    }
    if (next != ranges.end() && offset + size == next->first)
    {
        size += next->second;
        ranges.erase(next);
    }
    ranges[offset] = size;
}

void reapVertexRanges()
{
    IDirect3DDevice8* d = g_pD3DDevice;
    if (!d) return;
    size_t kept = 0;
    for (const PendingRange& r : s_pendingRanges)
    {
        if (d->IsFencePending(r.fence))
            s_pendingRanges[kept++] = r;
        else
            releaseRangeNow(r.pool, r.offset, r.size);
    }
    s_pendingRanges.resize(kept);

    // Give fully free pools back to the system (e.g. after leaving a world).
    // A released pool keeps its slot so pool indices of live meshes stay valid.
    if (!s_pendingRanges.empty())
        return;
    for (VertexPool& pool : s_vertexPools)
    {
        if (pool.vb != nullptr && pool.freeRanges.size() == 1 &&
            pool.freeRanges.begin()->first == 0 && pool.freeRanges.begin()->second == pool.size &&
            !pool.vb->IsBusy())
        {
            pool.vb->Release();
            pool.vb = nullptr;
            pool.base = nullptr;
            pool.freeRanges.clear();
        }
    }
}

bool allocateVertexRange(UINT bytes, int* pool, UINT* offset)
{
    bytes = roundToGranule(bytes);
    // Best fit over every pool (ties go to the lower pool): keeps large holes
    // for large meshes and lets the last pools drain so they can be released.
    int emptySlot = -1;
    int bestPool = -1;
    UINT bestOffset = 0;
    UINT bestSize = 0xFFFFFFFFu;
    for (size_t p = 0; p < s_vertexPools.size(); ++p)
    {
        if (s_vertexPools[p].vb == nullptr)
        {
            if (emptySlot < 0) emptySlot = static_cast<int>(p);
            continue;
        }
        for (const auto& range : s_vertexPools[p].freeRanges)
        {
            if (range.second >= bytes && range.second < bestSize)
            {
                bestPool = static_cast<int>(p);
                bestOffset = range.first;
                bestSize = range.second;
                if (bestSize == bytes) break;
            }
        }
        if (bestSize == bytes) break;
    }
    if (bestPool >= 0)
    {
        std::map<UINT, UINT>& ranges = s_vertexPools[static_cast<size_t>(bestPool)].freeRanges;
        ranges.erase(bestOffset);
        if (bestSize > bytes) ranges[bestOffset + bytes] = bestSize - bytes;
        *pool = bestPool;
        *offset = bestOffset;
        return true;
    }
    IDirect3DDevice8* d = g_pD3DDevice;
    VertexPool fresh;
    fresh.size = bytes > kVertexPoolBytes ? bytes : kVertexPoolBytes;
    if (!d || FAILED(d->CreateVertexBuffer(fresh.size, 0, 0, D3DPOOL_DEFAULT, &fresh.vb)))
        return false;
    if (FAILED(fresh.vb->Lock(0, 0, &fresh.base, 0)))
    {
        fresh.vb->Release();
        return false;
    }
    fresh.vb->Unlock();   // Xbox buffers stay CPU-addressable; the pointer remains valid
    if (fresh.size > bytes)
        fresh.freeRanges[bytes] = fresh.size - bytes;
    if (emptySlot >= 0)
    {
        s_vertexPools[static_cast<size_t>(emptySlot)] = fresh;
        *pool = emptySlot;
    }
    else
    {
        s_vertexPools.push_back(fresh);
        *pool = static_cast<int>(s_vertexPools.size()) - 1;
    }
    *offset = 0;
    return true;
}

void freeVertexRange(int pool, UINT offset, UINT bytes)
{
    bytes = roundToGranule(bytes);
    IDirect3DDevice8* d = g_pD3DDevice;
    if (d)
        s_pendingRanges.push_back({pool, offset, bytes, d->InsertFence()});
    else
        releaseRangeNow(pool, offset, bytes);
}

// Geometry of a display list. Meshes with per-vertex color (all terrain) are
// converted once into a static vertex buffer the GPU fetches by itself, so
// replaying a chunk costs a few commands instead of the CPU copying every
// vertex into the push buffer each frame (DrawVerticesUP). Meshes that take
// the current color at playback keep a CPU copy and go through the UP path.
struct RecordedMesh
{
    ~RecordedMesh()
    {
        s_recordedBytes -= bytes;
        if (pool >= 0)
            freeVertexRange(pool, offset, static_cast<UINT>(vertexCount) * stride);
    }
    std::vector<XboxVertex> vertices;
    int pool = -1;          // shared vertex buffer holding this mesh, or -1
    UINT offset = 0;        // byte offset of the mesh inside it
    int vertexCount = 0;
    UINT stride = sizeof(XboxVertex);   // sizeof(XboxCompactVertex) when compact
    long bytes = 0;
    RenderPrimitive primitive = RenderPrimitive::Triangles;
    bool usesCurrentColor = false;
};

void drawRecorded(const RecordedMesh& mesh)
{
    IDirect3DDevice8* d = device();
    if (!d) return;
    if (mesh.pool >= 0)
    {
        int count = mesh.vertexCount;
        if (mesh.primitive == RenderPrimitive::Quads) count -= count % 4;
        else if (mesh.primitive == RenderPrimitive::Triangles) count -= count % 3;
        if (count <= 0) return;
        applyMatrices();
        applyTextureStage(mesh.usesCurrentColor);
        if (mesh.usesCurrentColor)
            setRs(d, D3DRS_TEXTUREFACTOR, s_currentColor);
        const bool compact = mesh.stride != sizeof(XboxVertex);
        setWorldCompact(d, compact);
        setVs(d, !compact ? kXboxVertexFvf : mesh.usesCurrentColor ? s_compactNoColorShader : s_compactShader);
        d->SetStreamSource(0, s_vertexPools[static_cast<size_t>(mesh.pool)].vb, mesh.stride);
        const unsigned long long start = __rdtsc();
        d->DrawVertices(toD3DPrimitive(mesh.primitive), mesh.offset / mesh.stride, static_cast<UINT>(count));
        s_drawStats.drawCycles += __rdtsc() - start;
        ++s_drawStats.vbDraws;
        return;
    }
    const int count = static_cast<int>(mesh.vertices.size());
    if (!mesh.usesCurrentColor)
    {
        submitVertices(mesh.primitive, mesh.vertices.data(), count);
        return;
    }
    if (static_cast<int>(s_scratch.size()) < count)
        s_scratch.resize(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        s_scratch[static_cast<size_t>(i)] = mesh.vertices[static_cast<size_t>(i)];
        s_scratch[static_cast<size_t>(i)].color = s_currentColor;
    }
    submitVertices(mesh.primitive, s_scratch.data(), count);
}

} // namespace

// ===========================================================================
// State
// ===========================================================================
void renderEnable(RenderCapability capability)
{
    if (recordIfCompiling([capability] { renderEnable(capability); })) return;
    IDirect3DDevice8* d = device();
    if (!d) return;
    switch (capability)
    {
        case RenderCapability::Texture2D: if (s_activeTextureUnit == 0) s_texture2D = true; break;
        case RenderCapability::AlphaTest: setRs(d, D3DRS_ALPHATESTENABLE, TRUE); break;
        case RenderCapability::Blend: setRs(d, D3DRS_ALPHABLENDENABLE, TRUE); break;
        case RenderCapability::CullFace: s_cullEnabled = true; applyCull(); break;
        case RenderCapability::DepthTest: setRs(d, D3DRS_ZENABLE, D3DZB_TRUE); break;
        case RenderCapability::Fog: s_fogEnabled = true; applyFog(); break;
        default: break;  // lighting/normalize/color material: see file comment
    }
}

void renderDisable(RenderCapability capability)
{
    if (recordIfCompiling([capability] { renderDisable(capability); })) return;
    IDirect3DDevice8* d = device();
    if (!d) return;
    switch (capability)
    {
        case RenderCapability::Texture2D: if (s_activeTextureUnit == 0) s_texture2D = false; break;
        case RenderCapability::AlphaTest: setRs(d, D3DRS_ALPHATESTENABLE, FALSE); break;
        case RenderCapability::Blend: setRs(d, D3DRS_ALPHABLENDENABLE, FALSE); break;
        case RenderCapability::CullFace: s_cullEnabled = false; applyCull(); break;
        case RenderCapability::DepthTest: setRs(d, D3DRS_ZENABLE, D3DZB_FALSE); break;
        case RenderCapability::Fog: s_fogEnabled = false; applyFog(); break;
        default: break;
    }
}

void renderBlendFunc(RenderBlendFactor source, RenderBlendFactor destination)
{
    if (recordIfCompiling([source, destination] { renderBlendFunc(source, destination); })) return;
    IDirect3DDevice8* d = device();
    if (!d) return;
    setRs(d, D3DRS_SRCBLEND, toD3DBlend(source));
    setRs(d, D3DRS_DESTBLEND, toD3DBlend(destination));
}

void renderDepthMask(bool enabled)
{
    if (recordIfCompiling([enabled] { renderDepthMask(enabled); })) return;
    if (IDirect3DDevice8* d = device()) setRs(d, D3DRS_ZWRITEENABLE, enabled ? TRUE : FALSE);
}

void renderDepthFunc(RenderCompare function)
{
    if (IDirect3DDevice8* d = device()) setRs(d, D3DRS_ZFUNC, toD3DCompare(function));
}

void renderAlphaFunc(RenderCompare function, float reference)
{
    IDirect3DDevice8* d = device();
    if (!d) return;
    setRs(d, D3DRS_ALPHAFUNC, toD3DCompare(function));
    const float clamped = reference < 0.0f ? 0.0f : (reference > 1.0f ? 1.0f : reference);
    setRs(d, D3DRS_ALPHAREF, static_cast<DWORD>(clamped * 255.0f + 0.5f));
}

void renderCullFace(RenderFace face)
{
    s_cullFace = face;
    applyCull();
}

void renderColorMask(bool red, bool green, bool blue, bool alpha)
{
    IDirect3DDevice8* d = device();
    if (!d) return;
    DWORD mask = 0;
    if (red) mask |= D3DCOLORWRITEENABLE_RED;
    if (green) mask |= D3DCOLORWRITEENABLE_GREEN;
    if (blue) mask |= D3DCOLORWRITEENABLE_BLUE;
    if (alpha) mask |= D3DCOLORWRITEENABLE_ALPHA;
    setRs(d, D3DRS_COLORWRITEENABLE, mask);
}

void renderShadeModel(RenderShadeModel model)
{
    if (IDirect3DDevice8* d = device())
        setRs(d, D3DRS_SHADEMODE, model == RenderShadeModel::Flat ? D3DSHADE_FLAT : D3DSHADE_GOURAUD);
}

void renderColor4f(float r, float g, float b, float a)
{
    if (recordIfCompiling([r, g, b, a] { renderColor4f(r, g, b, a); })) return;
    s_currentColor = toD3DColor(r, g, b, a);
}

void renderColor3f(float r, float g, float b)
{
    renderColor4f(r, g, b, 1.0f);
}

void renderNormal3f(float, float, float) {}

// Single texture unit on this backend; the lightmap unit is unused by Beta.
void renderSetActiveTextureUnit(int textureUnit)
{
    if (recordIfCompiling([textureUnit] { renderSetActiveTextureUnit(textureUnit); })) return;
    s_activeTextureUnit = textureUnit >= 0x84C0 ? textureUnit - 0x84C0 : textureUnit;
}
void renderSetClientActiveTextureUnit(int) {}
void renderSetMultiTextureCoord(int, float, float) {}
void renderSetLightmapColors(const std::uint32_t*, int) {}

// ===========================================================================
// Clear / viewport / frame
// ===========================================================================
void renderClearColor(float r, float g, float b, float a)
{
    s_clearColor = toD3DColor(r, g, b, a);
}

void renderClearDepth(double depth)
{
    s_clearDepth = static_cast<float>(depth);
}

void renderClear(unsigned int mask)
{
    IDirect3DDevice8* d = device();
    if (!d) return;
    DWORD flags = 0;
    if (mask & RenderClearMask::Color) flags |= D3DCLEAR_TARGET;
    if (mask & RenderClearMask::Depth) flags |= D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
    if (flags) d->Clear(0, NULL, flags, s_clearColor, s_clearDepth, 0);
}

void renderViewport(int x, int y, int width, int height)
{
    s_viewport[0] = x;
    s_viewport[1] = y;
    s_viewport[2] = width;
    s_viewport[3] = height;
    IDirect3DDevice8* d = device();
    if (!d || width <= 0 || height <= 0) return;
    // GL's viewport origin is bottom-left, D3D's is top-left of the back buffer.
    const int screenHeight = XboxVideoMode::height();
    D3DVIEWPORT8 vp;
    vp.X = static_cast<DWORD>(x < 0 ? 0 : x);
    const int top = screenHeight - (y + height);
    vp.Y = static_cast<DWORD>(top < 0 ? 0 : top);
    vp.Width = static_cast<DWORD>(width);
    vp.Height = static_cast<DWORD>(height);
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    d->SetViewport(&vp);
}

void renderGetViewport(int* values)
{
    if (!values) return;
    for (int i = 0; i < 4; ++i) values[i] = s_viewport[i];
}

// OptiFine "Smooth FPS" calls renderFinishGpu at the start of every terrain
// pass. BlockUntilIdle stalls the CPU until the GPU has drained the whole
// previous frame, so CPU and GPU never overlap; on this CPU-bound console
// that stall showed up as most of the opaque pass (10.5 -> 1.6 ms without
// it). Present() already paces the frame. Set to 1 to bring the wait back.
#ifndef XBOX_SMOOTH_FPS_WAITS_FOR_GPU
#define XBOX_SMOOTH_FPS_WAITS_FOR_GPU 0
#endif

void renderFinishGpu()
{
#if XBOX_SMOOTH_FPS_WAITS_FOR_GPU
    if (IDirect3DDevice8* d = device()) d->BlockUntilIdle();
#endif
}

void renderSubmitFrame() {}
void renderPolygonOffset(float, float) {}
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
        if (it->second.d3d)
        {
            if (IDirect3DDevice8* d = device())
            {
                if (s_boundTexture == textures[i]) setTex(d, 0, NULL);
                s_stageValid = false;
            }
            it->second.d3d->Release();
        }
        s_textures.erase(it);
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
    return true;
}

void renderTextureImageRgba(int level, int width, int height, const void* pixels)
{
    const int target = uploadTarget();
    if (level != 0 || target <= 0) return;  // one level: no mipmaps yet
    Texture& t = s_textures[target];
    ensureStorage(t, width, height);
    if (t.pixels.size() != static_cast<size_t>(width) * height)
        t.pixels.assign(static_cast<size_t>(width) * height, 0);   // CPU copy was dropped
    if (pixels)
        convertRgba(static_cast<const unsigned char*>(pixels), t.pixels.data(), width * height);
    swizzleUpload(t);
}

void renderTextureSubImageRgba(int level, int x, int y, int width, int height, const void* pixels)
{
    const int target = uploadTarget();
    if (level != 0 || target <= 0 || !pixels) return;
    auto it = s_textures.find(target);
    if (it == s_textures.end()) return;
    Texture& t = it->second;
    const unsigned char* src = static_cast<const unsigned char*>(pixels);
    if (t.pixels.empty() && !restorePixels(t))
        return;
    t.dynamic = true;   // keeps its CPU copy from now on
    for (int row = 0; row < height; ++row)
    {
        const int ty = y + row;
        if (ty < 0 || ty >= t.height) continue;
        const int cols = (x + width > t.width) ? t.width - x : width;
        if (cols <= 0 || x < 0) continue;
        convertRgba(src + static_cast<size_t>(row) * width * 4,
                    t.pixels.data() + static_cast<size_t>(ty) * t.width + x, cols);
    }
    // Animated tiles (water, lava, fire, portal...) arrive a dozen per tick,
    // each re-swizzling and locking the whole atlas. Mark it instead: the next
    // bind (applyTextureStage) uploads it once with every tile applied.
    t.dirty = true;
}

void renderTextureParameters(bool blur, bool, bool clamp)
{
    if (uploadTarget() <= 0) return;
    Texture& t = s_textures[uploadTarget()];
    t.blur = blur;
    t.clamp = clamp;
}

void renderApplyTextureQuality(bool blur, int, bool, int)
{
    if (uploadTarget() <= 0) return;
    s_textures[uploadTarget()].blur = blur;
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
    applyFog();
}

void renderFogi(RenderFogParameter parameter, RenderFogMode value)
{
    if (parameter == RenderFogParameter::Mode || parameter == RenderFogParameter::DistanceMode)
    {
        s_fogMode = value;
        applyFog();
    }
}

void renderFogColor(const float* values)
{
    if (!values) return;
    if (IDirect3DDevice8* d = device())
        setRs(d, D3DRS_FOGCOLOR, toD3DColor(values[0], values[1], values[2], values[3]));
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
        case RenderStringQuery::Renderer: return reinterpret_cast<const unsigned char*>("Xbox NV2A (Direct3D 8)");
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
        const bool loop = mesh.primitive == RenderPrimitive::LineLoop;
        auto recorded = std::make_shared<RecordedMesh>();
        recorded->primitive = mesh.primitive;
        recorded->usesCurrentColor = !mesh.hasColor;
        const int count = mesh.count + (loop ? 1 : 0);
        int pool = -1;
        UINT offset = 0;
        IDirect3DDevice8* d = device();
        if (d != nullptr)
        {
            if (static_cast<int>(s_scratch.size()) < count)
                s_scratch.resize(static_cast<size_t>(count));
            convertVertices(mesh, s_scratch.data());
            if (loop)
                s_scratch[static_cast<size_t>(count - 1)] = s_scratch[0];
            compactShader(d);
            const bool colorless = recorded->usesCurrentColor;
            const bool compact = (colorless ? s_compactNoColorShader : s_compactShader) != 0 &&
                                 fitsCompact(s_scratch.data(), count);
            // A colourless mesh that does not fit the compact layout keeps
            // the CPU copy below (its colour is patched in at playback).
            const UINT stride = colorless ? sizeof(XboxCompactVertexNoColor)
                              : compact ? sizeof(XboxCompactVertex) : sizeof(XboxVertex);
            if ((compact || !colorless) && allocateVertexRange(static_cast<UINT>(count) * stride, &pool, &offset))
            {
                // Written once, straight into the (write-combined) buffer memory.
                BYTE* out = s_vertexPools[static_cast<size_t>(pool)].base + offset;
                if (colorless)
                    writeCompactNoColor(s_scratch.data(), count, reinterpret_cast<XboxCompactVertexNoColor*>(out));
                else if (compact)
                    writeCompact(s_scratch.data(), count, reinterpret_cast<XboxCompactVertex*>(out));
                else
                    std::memcpy(out, s_scratch.data(), static_cast<size_t>(count) * sizeof(XboxVertex));
                recorded->pool = pool;
                recorded->offset = offset;
                recorded->vertexCount = count;
                recorded->stride = stride;
            }
        }
        if (pool < 0)
        {
            recorded->vertices.resize(static_cast<size_t>(count));
            convertVertices(mesh, recorded->vertices.data());
            if (loop)
                recorded->vertices.back() = recorded->vertices.front();
        }
        recorded->bytes = static_cast<long>(count) * static_cast<long>(recorded->stride);
        s_recordedBytes += recorded->bytes;
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
                               static_cast<size_t>(mesh.first) * mesh.stride;
    const size_t bytes = static_cast<size_t>(mesh.count) * mesh.stride;
    const size_t old = out.raw.size();
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
    return renderDrawInterleaved(view);
}

// ===========================================================================
// Display lists (desktop 1.2.5 renderer path, used by the Xbox)
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
    const unsigned long long start = __rdtsc();
    ++s_listDepth;
    for (const auto& command : it->second)
        command();
    if (--s_listDepth == 0)
    {
        s_drawStats.listCycles += __rdtsc() - start;
        ++s_drawStats.listCalls;
    }
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
void renderSetLegacyPresentationGamma(bool) {}

#endif // XBOX_PLATFORM

#ifdef XBOX_PLATFORM
// Called once per frame after Present: releases vertex buffers of deleted
// display lists the GPU no longer reads.
void xboxRenderEndFrame()
{
    reapVertexRanges();
}

// Number of 1 MB vertex pools currently allocated, for the memory log.
long xboxVertexPoolCount()
{
    long pools = 0;
    for (const VertexPool& pool : s_vertexPools)
        if (pool.vb != nullptr) ++pools;
    return pools;
}

// Draw statistics since the last call, for the profile report.
void xboxRenderTakeListStats(double* transformMs, double* listMs, double* stageMs, long* listCalls)
{
    *transformMs = static_cast<double>(s_drawStats.transformCycles) / 733333.0;
    *listMs = static_cast<double>(s_drawStats.listCycles) / 733333.0;
    *stageMs = static_cast<double>(s_drawStats.stageCycles) / 733333.0;
    *listCalls = s_drawStats.listCalls;
}

void xboxRenderTakeDrawStats(double* drawMs, long* vbDraws, long* upDraws, long* upVertices, long* viewSets)
{
    *drawMs = static_cast<double>(s_drawStats.drawCycles) / 733333.0;
    *vbDraws = s_drawStats.vbDraws;
    *upDraws = s_drawStats.upDraws;
    *upVertices = s_drawStats.upVertices;
    *viewSets = s_drawStats.viewSets;
    s_drawStats = DrawStats();
}

// Memory held by the emulated GL objects, for the periodic memory log.
void xboxRenderMemoryStats(long* listKB, long* lists, long* textureKB, long* textures)
{
    *listKB = s_recordedBytes / 1024;
    *lists = static_cast<long>(s_lists.size());
    long bytes = 0;
    for (const auto& entry : s_textures)
        bytes += static_cast<long>(entry.second.pixels.capacity() * sizeof(DWORD)) +
                 static_cast<long>(entry.second.storageWidth) * entry.second.storageHeight * 4;
    *textureKB = bytes / 1024;
    *textures = static_cast<long>(s_textures.size());

    // Which textures hold the memory, whenever the set changes.
    static size_t s_loggedCount = 0;
    if (s_textures.size() != s_loggedCount)
    {
        s_loggedCount = s_textures.size();
        for (const auto& entry : s_textures)
        {
            const Texture& t = entry.second;
            const long kb = static_cast<long>(t.pixels.capacity() * sizeof(DWORD) +
                                              static_cast<size_t>(t.storageWidth) * t.storageHeight * 4) / 1024;
            if (kb >= 128)
                MC_LOG_INFO("xbox.tex", "id=%d %dx%d storage=%dx%d cpuKB=%ld totalKB=%ld dynamic=%d\n",
                            entry.first, t.width, t.height, t.storageWidth, t.storageHeight,
                            static_cast<long>(t.pixels.capacity() * sizeof(DWORD) / 1024), kb, t.dynamic ? 1 : 0);
        }
    }
}
#endif

