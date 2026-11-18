import sys

stub_h = """
#include "platform/RenderAPI.h"
#include "xbox/render/XboxD3D.h"
#include <vector>
#include <cstring>
#include <cstdio>

// Globals for simple matrix stack
static RenderMatrixMode g_matrixMode = RenderMatrixMode::ModelView;
static std::vector<D3DMATRIX> g_modelViewStack;
static std::vector<D3DMATRIX> g_projectionStack;
static std::vector<D3DMATRIX> g_textureStack;
static D3DMATRIX g_modelView;
static D3DMATRIX g_projection;
static D3DMATRIX g_texture;

static DWORD g_currentColor = 0xFFFFFFFF;

static void InitMatrix(D3DMATRIX& m) {
    memset(&m, 0, sizeof(m));
    m._11 = m._22 = m._33 = m._44 = 1.0f;
}

static D3DMATRIX* GetCurrentMatrix() {
    switch (g_matrixMode) {
        case RenderMatrixMode::ModelView: return &g_modelView;
        case RenderMatrixMode::Projection: return &g_projection;
        case RenderMatrixMode::Texture: return &g_texture;
    }
    return &g_modelView;
}

static void UpdateD3DMatrix() {
    if (!g_pD3DDevice) return;
    switch (g_matrixMode) {
        case RenderMatrixMode::ModelView: g_pD3DDevice->SetTransform(D3DTS_VIEW, &g_modelView); break;
        case RenderMatrixMode::Projection: g_pD3DDevice->SetTransform(D3DTS_PROJECTION, &g_projection); break;
        case RenderMatrixMode::Texture: g_pD3DDevice->SetTransform(D3DTS_TEXTURE0, &g_texture); break;
    }
}

void renderEnable(RenderCapability capability) {
    if (!g_pD3DDevice) return;
    switch (capability) {
        case RenderCapability::AlphaTest: g_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); break;
        case RenderCapability::Blend: g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); break;
        case RenderCapability::CullFace: g_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); break;
        case RenderCapability::DepthTest: g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE); break;
        case RenderCapability::Fog: g_pD3DDevice->SetRenderState(D3DRS_FOGENABLE, TRUE); break;
        case RenderCapability::Lighting: g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE); break;
        case RenderCapability::Texture2D: break; // D3D handles it per stage, or via FVF
        default: break;
    }
}

void renderDisable(RenderCapability capability) {
    if (!g_pD3DDevice) return;
    switch (capability) {
        case RenderCapability::AlphaTest: g_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); break;
        case RenderCapability::Blend: g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); break;
        case RenderCapability::CullFace: g_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); break;
        case RenderCapability::DepthTest: g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE); break;
        case RenderCapability::Fog: g_pD3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE); break;
        case RenderCapability::Lighting: g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE); break;
        case RenderCapability::Texture2D: break;
        default: break;
    }
}

void renderClear(unsigned int mask) {
    if (!g_pD3DDevice) return;
    DWORD flags = 0;
    if (mask & RenderClearMask::Color) flags |= D3DCLEAR_TARGET;
    if (mask & RenderClearMask::Depth) flags |= D3DCLEAR_ZBUFFER;
    if (flags) {
        g_pD3DDevice->Clear(0, NULL, flags, 0, 1.0f, 0);
    }
}

void renderClearColor(float r, float g, float b, float a) { /* D3D clear color is passed in Clear() */ }
void renderClearDepth(double depth) { /* D3D clear depth is passed in Clear() */ }
void renderViewport(int x, int y, int width, int height) {
    if (!g_pD3DDevice) return;
    D3DVIEWPORT8 vp;
    vp.X = x;
    vp.Y = y;
    vp.Width = width;
    vp.Height = height;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    g_pD3DDevice->SetViewport(&vp);
}

void renderMatrixMode(RenderMatrixMode mode) { g_matrixMode = mode; }

void renderLoadIdentity() {
    D3DMATRIX* m = GetCurrentMatrix();
    InitMatrix(*m);
    UpdateD3DMatrix();
}

void renderPushMatrix() {
    switch (g_matrixMode) {
        case RenderMatrixMode::ModelView: g_modelViewStack.push_back(g_modelView); break;
        case RenderMatrixMode::Projection: g_projectionStack.push_back(g_projection); break;
        case RenderMatrixMode::Texture: g_textureStack.push_back(g_texture); break;
    }
}

void renderPopMatrix() {
    switch (g_matrixMode) {
        case RenderMatrixMode::ModelView:
            if (!g_modelViewStack.empty()) { g_modelView = g_modelViewStack.back(); g_modelViewStack.pop_back(); UpdateD3DMatrix(); }
            break;
        case RenderMatrixMode::Projection:
            if (!g_projectionStack.empty()) { g_projection = g_projectionStack.back(); g_projectionStack.pop_back(); UpdateD3DMatrix(); }
            break;
        case RenderMatrixMode::Texture:
            if (!g_textureStack.empty()) { g_texture = g_textureStack.back(); g_textureStack.pop_back(); UpdateD3DMatrix(); }
            break;
    }
}

void renderTranslate(float x, float y, float z) {
    D3DMATRIX* m = GetCurrentMatrix();
    D3DMATRIX t;
    InitMatrix(t);
    t._41 = x;
    t._42 = y;
    t._43 = z;
    // Simple multiply (m = m * t) for OpenGL-style matrix operations
    // Actually, OpenGL post-multiplies: M' = M * T. D3D uses row-major.
    D3DMATRIX res;
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
            res.m[i][j] = t.m[i][0]*m->m[0][j] + t.m[i][1]*m->m[1][j] + t.m[i][2]*m->m[2][j] + t.m[i][3]*m->m[3][j];
        }
    }
    *m = res;
    UpdateD3DMatrix();
}
void renderRotate(float angle, float x, float y, float z) {
    // Stub for now, can implement later if menus use it heavily (usually just ortho/translate)
}
void renderScale(float x, float y, float z) {
    D3DMATRIX* m = GetCurrentMatrix();
    D3DMATRIX s;
    InitMatrix(s);
    s._11 = x;
    s._22 = y;
    s._33 = z;
    D3DMATRIX res;
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
            res.m[i][j] = s.m[i][0]*m->m[0][j] + s.m[i][1]*m->m[1][j] + s.m[i][2]*m->m[2][j] + s.m[i][3]*m->m[3][j];
        }
    }
    *m = res;
    UpdateD3DMatrix();
}
void renderScaleDouble(double x, double y, double z) {
    renderScale((float)x, (float)y, (float)z);
}
void renderFrustum(double left, double right, double bottom, double top, double nearValue, double farValue) {
    // Stub
}
void renderOrtho(double left, double right, double bottom, double top, double nearValue, double farValue) {
    if (!g_pD3DDevice) return;
    D3DMATRIX& m = g_projection;
    InitMatrix(m);
    m._11 = 2.0f / (right - left);
    m._22 = 2.0f / (top - bottom);
    m._33 = 1.0f / (nearValue - farValue); // D3D depth is 0 to 1
    m._41 = (left + right) / (left - right);
    m._42 = (top + bottom) / (bottom - top);
    m._43 = nearValue / (nearValue - farValue);
    g_pD3DDevice->SetTransform(D3DTS_PROJECTION, &m);
}

void renderColor4f(float r, float g, float b, float a) { 
    g_currentColor = D3DCOLOR_COLORVALUE(r, g, b, a);
}
void renderColor3f(float r, float g, float b) { 
    g_currentColor = D3DCOLOR_COLORVALUE(r, g, b, 1.0f);
}

static int g_currentTexture = 0;
void renderBindTexture(int texture) { 
    g_currentTexture = texture;
    // In a real implementation we would look up the IDirect3DTexture8* from the ID and SetTexture(0, tex)
}

void renderGenerateTextures(int count, int *textures) {
    static int nextId = 1;
    for(int i=0; i<count; ++i) textures[i] = nextId++;
}

struct MyVertex {
    float x, y, z;
    DWORD color;
    float u, v;
};
#define D3DFVF_MYVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

bool renderDrawInterleaved(const RenderInterleavedMesh& mesh) {
    if (!g_pD3DDevice) return true;
    
    D3DPRIMITIVETYPE pt;
    switch (mesh.primitive) {
        case RenderPrimitive::Points: pt = D3DPT_POINTLIST; break;
        case RenderPrimitive::Lines: pt = D3DPT_LINELIST; break;
        case RenderPrimitive::LineLoop: pt = D3DPT_LINELIST; break; // not quite correct, but stub
        case RenderPrimitive::LineStrip: pt = D3DPT_LINESTRIP; break;
        case RenderPrimitive::Triangles: pt = D3DPT_TRIANGLELIST; break;
        case RenderPrimitive::TriangleStrip: pt = D3DPT_TRIANGLESTRIP; break;
        case RenderPrimitive::TriangleFan: pt = D3DPT_TRIANGLEFAN; break;
        case RenderPrimitive::Quads: pt = D3DPT_QUADLIST; break;
        default: return false;
    }

    int primCount = 0;
    switch (mesh.primitive) {
        case RenderPrimitive::Triangles: primCount = mesh.count / 3; break;
        case RenderPrimitive::Quads: primCount = mesh.count / 4; break;
        default: primCount = mesh.count / 3; break; // fallback
    }

    if (mesh.positionShort) return false;

    std::vector<MyVertex> vertices(mesh.count);
    const char* src = (const char*)mesh.data + (mesh.first * mesh.stride);
    for(int i = 0; i < mesh.count; i++) {
        const char* vsrc = src + (i * mesh.stride);
        MyVertex& v = vertices[i];
        
        float* pos = (float*)vsrc;
        v.x = pos[0]; v.y = pos[1]; v.z = pos[2];
        
        if (mesh.hasColor) {
            unsigned char* c = (unsigned char*)(vsrc + mesh.colorOffset);
            v.color = D3DCOLOR_RGBA(c[0], c[1], c[2], c[3]);
        } else {
            v.color = g_currentColor;
        }

        if (mesh.hasTexture) {
            float* uv = (float*)(vsrc + mesh.texCoordOffset);
            v.u = uv[0]; v.v = uv[1];
        } else {
            v.u = 0.0f; v.v = 0.0f;
        }
    }

    g_pD3DDevice->SetVertexShader(D3DFVF_MYVERTEX);
    g_pD3DDevice->DrawPrimitiveUP(pt, primCount, vertices.data(), sizeof(MyVertex));
    
    return true;
}

// ---------------- STUBS -----------------
void renderBlendFunc(RenderBlendFactor source, RenderBlendFactor destination) {}
void renderDepthMask(bool enabled) {}
void renderDepthFunc(RenderCompare function) {}
void renderAlphaFunc(RenderCompare function, float reference) {}
void renderCullFace(RenderFace face) {}
void renderColorMask(bool red, bool green, bool blue, bool alpha) {}
void renderSetActiveTextureUnit(int textureUnit) {}
void renderSetClientActiveTextureUnit(int textureUnit) {}
void renderSetMultiTextureCoord(int textureUnit, float u, float v) {}
void renderSetLightmapColors(const std::uint32_t* colors, int count) {}
void renderNormal3f(float x, float y, float z) {}
void renderDeleteTextures(int count, const int *textures) {}
void renderTextureSubImageRgba(int level, int x, int y, int width, int height, const void *pixels) {}
void renderTextureImageRgba(int level, int width, int height, const void *pixels) {}
void renderTextureParameters(bool blur, bool mipmaps, bool clamp) {}
void renderApplyTextureQuality(bool blur, int mipmapLevel, bool mipmapLinear, int anisotropy) {}
int renderGetMaxAnisotropy() { return 1; }
int renderGetMaxSamples() { return 1; }
bool renderTextureBeginUpload(int texture, int width, int height, int maxLevel, bool blur, bool clamp, bool tileAtlas, bool highPrecision) { return true; }
bool renderTextureIsValid(int texture) { return true; }
void renderResetResources() {}
void renderFogf(RenderFogParameter parameter, float value) {}
void renderFogi(RenderFogParameter parameter, RenderFogMode value) {}
void renderFogColor(const float* values) {}
void renderLightfv(int lightIndex, RenderLightParameter parameter, const float* values) {}
void renderLightModelAmbient(const float* values) {}
void renderColorMaterial(RenderFace face, RenderColorMaterialMode mode) {}
void renderShadeModel(RenderShadeModel model) {}
void renderFinishGpu() {}
void renderSubmitFrame() {}
void renderPolygonOffset(float factor, float units) {}
void renderLineWidth(float width) {}
void renderGetViewport(int* values) {}
void renderGetMatrix(RenderMatrixQuery query, float* values) {}
const unsigned char* renderGetString(RenderStringQuery query) { return (const unsigned char*)"Xbox D3D8"; }
bool renderSupportsFeature(RenderFeature feature) { return false; }
unsigned int renderGetError() { return 0; }
void renderFogHint(RenderHintMode mode) {}

bool renderCaptureInterleaved(const RenderInterleavedMesh& mesh, RenderCapturedMesh& out, bool append) { return false; }
bool renderDrawCaptured(const RenderCapturedMesh& mesh) { return false; }

void renderStaticMeshCreate(RenderStaticMesh& mesh) {}
void renderStaticMeshDestroy(RenderStaticMesh& mesh) {}
bool renderStaticMeshCompile(RenderStaticMesh& mesh, const RenderInterleavedMesh& source) { return false; }
bool renderStaticMeshDraw(const RenderStaticMesh& mesh) { return false; }

int renderGenerateDisplayLists(int count) { return 0; }
void renderDeleteDisplayLists(int first, int count) {}
void renderBeginDisplayList(int list) {}
void renderEndDisplayList() {}
void renderCallDisplayList(int list) {}
void renderCallDisplayLists(int count, const int* lists) {}
void renderGenerateOcclusionQueries(int count, int* queries) {}
void renderBeginOcclusionQuery(int query) {}
void renderEndOcclusionQuery() {}
bool renderOcclusionQueryResultAvailable(int query) { return false; }
unsigned int renderOcclusionQueryResult(int query) { return 0; }

bool renderReadPixelsRgb(int x, int y, int width, int height, void* pixels) { return false; }
bool renderCopyFramebufferToBoundTexture(int x, int y, int width, int height) { return false; }
void renderSetLegacyPresentationGamma(bool enabled) {}

"""

with open(r"C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\src\platform\RenderAPI_D3D8_XBOX.cpp", "w", encoding="utf-8") as f:
    f.write(stub_h)
