// Xbox version launcher (XBOX_LAUNCHER=ON). A small default.xbe that looks like
// the old Java Minecraft launcher: news panel on top, a dirt bar at the bottom
// with the version drop-down and a big Play button. Picking a version starts
// the matching game XBE next to it:
//
//   OptiCraft.xbe              normal, 640x480
//   OptiCraft_720p.xbe         normal, 1280x720
//   OptiCraft_exp.xbe          experimental (chunks on a background thread)
//   OptiCraft_exp_720p.xbe     experimental, 1280x720
//
// The game picks 720p from "720" in its own file name (XboxVideoMode.cpp), so
// the launcher only has to start the right file. It reads its art from the
// game's data folder (D:\data\assets) and draws plain colors if it is missing.
// It always opens on the normal build.

#include "XboxXtl.h"

#include <xgraphics.h>
#include <mmintrin.h>

#include <cstdio>
#include <cstring>
#include <vector>

#define STBI_NO_THREAD_LOCALS
#define STBI_NO_SIMD        // Pentium III: no SSE2
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

const int kWidth = 640;
const int kHeight = 480;

LPDIRECT3D8 g_d3d;
LPDIRECT3DDEVICE8 g_dev;

// --- versions -----------------------------------------------------------------

struct Version {
    const char* name;
    const char* file;
    const char* text;
};

const Version kVersions[] = {
    {"OptiCraft", "D:\\OptiCraft.xbe",
     "The stable build at 640x480. Works on every console and every cable."},
    {"OptiCraft 720p", "D:\\OptiCraft_720p.xbe",
     "The stable build at 1280x720. Needs a component (HD) cable and 720p turned on in the dashboard."},
    {"Experimental", "D:\\OptiCraft_exp.xbe",
     "Chunks are generated and loaded on a background thread: fewer stutters while you walk. "
     "Still being tested, so keep a copy of your worlds."},
    {"Experimental 720p", "D:\\OptiCraft_exp_720p.xbe",
     "The experimental build at 1280x720. Needs a component (HD) cable and 720p turned on in the dashboard."},
};
const int kVersionCount = sizeof(kVersions) / sizeof(kVersions[0]);

bool g_present[kVersionCount];
bool g_720pAllowed;

bool versionIsHd(int i) { return strstr(kVersions[i].file, "720") != nullptr; }

bool fileExists(const char* path)
{
    return GetFileAttributesA(path) != (DWORD)-1;
}

// --- textures -----------------------------------------------------------------

struct Image {
    LPDIRECT3DTEXTURE8 tex = nullptr;
    int width = 0, height = 0;          // the picture
    int texWidth = 0, texHeight = 0;    // its power-of-two texture
    std::vector<unsigned char> rgba;    // kept for the font's glyph widths
};

int nextPow2(int v)
{
    int p = 1;
    while (p < v) p <<= 1;
    return p;
}

bool readFile(const char* path, std::vector<unsigned char>& out)
{
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return false;
    const DWORD size = GetFileSize(f, NULL);
    out.resize(size);
    DWORD got = 0;
    const BOOL ok = ReadFile(f, out.data(), size, &got, NULL);
    CloseHandle(f);
    return ok && got == size;
}

// The picture goes into the top-left corner of a power-of-two texture (unused
// texels transparent); UVs address only the picture.
bool loadImage(const char* path, Image& img, bool keepPixels = false)
{
    std::vector<unsigned char> file;
    if (!readFile(path, file)) return false;
    int w = 0, h = 0, n = 0;
    unsigned char* px = stbi_load_from_memory(file.data(), static_cast<int>(file.size()), &w, &h, &n, 4);
    if (!px) return false;
    img.width = w;
    img.height = h;
    img.texWidth = nextPow2(w);
    img.texHeight = nextPow2(h);
    std::vector<DWORD> argb(static_cast<size_t>(img.texWidth) * img.texHeight, 0);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const unsigned char* p = px + (static_cast<size_t>(y) * w + x) * 4;
            argb[static_cast<size_t>(y) * img.texWidth + x] =
                (DWORD(p[3]) << 24) | (DWORD(p[0]) << 16) | (DWORD(p[1]) << 8) | DWORD(p[2]);
        }
    if (keepPixels) img.rgba.assign(px, px + static_cast<size_t>(w) * h * 4);
    stbi_image_free(px);

    if (FAILED(g_dev->CreateTexture(img.texWidth, img.texHeight, 1, 0, D3DFMT_A8R8G8B8, 0, &img.tex)))
        return false;
    D3DLOCKED_RECT locked;
    if (FAILED(img.tex->LockRect(0, &locked, NULL, 0))) return false;
    XGSwizzleRect(argb.data(), img.texWidth * 4, NULL, locked.pBits, img.texWidth, img.texHeight, NULL, 4);
    // The XDK swizzler is MMX code; leave the x87 stack usable for the floats below.
    _mm_empty();
    img.tex->UnlockRect(0);
    return true;
}

Image g_font, g_dirt, g_gui, g_panorama, g_logo;
int g_glyphWidth[256];

// Minecraft 1.2.5 FontRenderer widths: last non-empty column + 2, space = 4.
void measureFont()
{
    for (int c = 0; c < 256; ++c) {
        int last = -1;
        if (!g_font.rgba.empty()) {
            const int gx = (c % 16) * 8, gy = (c / 16) * 8;
            for (int x = 0; x < 8; ++x)
                for (int y = 0; y < 8; ++y)
                    if (g_font.rgba[((gy + y) * g_font.width + gx + x) * 4 + 3] > 0) last = x > last ? x : last;
        }
        g_glyphWidth[c] = last < 0 ? 4 : last + 2;
    }
    g_glyphWidth[' '] = 4;
}

// --- drawing --------------------------------------------------------------------

struct Vertex {
    float x, y, z, rhw;
    D3DCOLOR color;
    float u, v;
};
const DWORD kFvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

void bind(const Image* img, bool smooth, bool wrap)
{
    g_dev->SetTexture(0, img && img->tex ? img->tex : NULL);
    const DWORD filter = smooth ? D3DTEXF_LINEAR : D3DTEXF_POINT;
    g_dev->SetTextureStageState(0, D3DTSS_MAGFILTER, filter);
    g_dev->SetTextureStageState(0, D3DTSS_MINFILTER, filter);
    const DWORD address = wrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
    g_dev->SetTextureStageState(0, D3DTSS_ADDRESSU, address);
    g_dev->SetTextureStageState(0, D3DTSS_ADDRESSV, address);
    const bool textured = img && img->tex;
    g_dev->SetTextureStageState(0, D3DTSS_COLOROP, textured ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    g_dev->SetTextureStageState(0, D3DTSS_ALPHAOP, textured ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
}

// Quad in screen pixels; u/v in texels of the image.
void quad(const Image* img, float x, float y, float w, float h, float u0, float v0, float u1, float v1, D3DCOLOR c)
{
    float su = 1.0f, sv = 1.0f;
    if (img && img->tex) {
        su = 1.0f / img->texWidth;
        sv = 1.0f / img->texHeight;
    }
    // -0.5: D3D8 pixel centers, so texels land on pixels.
    const float l = x - 0.5f, t = y - 0.5f, r = x + w - 0.5f, b = y + h - 0.5f;
    Vertex v[4] = {
        {l, t, 0, 1, c, u0 * su, v0 * sv},
        {r, t, 0, 1, c, u1 * su, v0 * sv},
        {r, b, 0, 1, c, u1 * su, v1 * sv},
        {l, b, 0, 1, c, u0 * su, v1 * sv},
    };
    g_dev->DrawPrimitiveUP(D3DPT_QUADLIST, 1, v, sizeof(Vertex));
}

void fill(float x, float y, float w, float h, D3DCOLOR c)
{
    bind(nullptr, false, false);
    quad(nullptr, x, y, w, h, 0, 0, 0, 0, c);
}

void frame(float x, float y, float w, float h, D3DCOLOR c)
{
    fill(x, y, w, 2, c);
    fill(x, y + h - 2, w, 2, c);
    fill(x, y, 2, h, c);
    fill(x + w - 2, y, 2, h, c);
}

int textWidth(const char* s, int scale)
{
    int w = 0;
    for (; *s; ++s) w += g_glyphWidth[static_cast<unsigned char>(*s)];
    return w * scale;
}

void textRaw(const char* s, float x, float y, int scale, D3DCOLOR c)
{
    for (; *s; ++s) {
        const unsigned char ch = static_cast<unsigned char>(*s);
        if (ch != ' ')
            quad(&g_font, x, y, 8.0f * scale, 8.0f * scale, float((ch % 16) * 8), float((ch / 16) * 8),
                 float((ch % 16) * 8 + 8), float((ch / 16) * 8 + 8), c);
        x += g_glyphWidth[ch] * scale;
    }
}

// Text with the Minecraft drop shadow (a quarter of the color, one pixel down-right).
void text(const char* s, float x, float y, int scale, D3DCOLOR c)
{
    if (!g_font.tex) return;
    bind(&g_font, false, false);
    const D3DCOLOR shadow = (c & 0xFF000000) | ((c & 0x00FCFCFC) >> 2);
    textRaw(s, x + scale, y + scale, scale, shadow);
    textRaw(s, x, y, scale, c);
}

void textCentered(const char* s, float cx, float y, int scale, D3DCOLOR c)
{
    text(s, cx - textWidth(s, scale) / 2.0f, y, scale, c);
}

// Word-wrapped paragraph; returns the y below it.
float paragraph(const char* s, float x, float y, float maxWidth, int scale, D3DCOLOR c)
{
    char line[160];
    while (*s) {
        int len = 0, lastSpace = -1;
        while (s[len] && len < 159) {
            if (s[len] == ' ') lastSpace = len;
            line[len] = s[len];
            line[len + 1] = 0;
            if (textWidth(line, scale) > maxWidth) break;
            ++len;
        }
        if (s[len] && lastSpace > 0) len = lastSpace;
        memcpy(line, s, len);
        line[len] = 0;
        text(line, x, y, scale, c);
        y += 10.0f * scale;
        s += len;
        while (*s == ' ') ++s;
    }
    return y;
}

// gui.png button (200x20 at 0,66; hovered at 0,86), drawn at 2x and split in
// halves so any width up to 400 keeps both rounded ends.
void button(float x, float y, float w, const char* label, bool hovered)
{
    const float h = 40.0f;
    const float v = hovered ? 86.0f : 66.0f;
    if (g_gui.tex) {
        bind(&g_gui, false, false);
        const float half = w / 2.0f, srcHalf = half / 2.0f;
        quad(&g_gui, x, y, half, h, 0, v, srcHalf, v + 20, 0xFFFFFFFF);
        quad(&g_gui, x + half, y, half, h, 200 - srcHalf, v, 200, v + 20, 0xFFFFFFFF);
    } else {
        fill(x, y, w, h, hovered ? 0xFF7080C0 : 0xFF707070);
    }
    textCentered(label, x + w / 2.0f, y + (h - 16.0f) / 2.0f, 2, hovered ? 0xFFFFFFA0 : 0xFFE0E0E0);
}

void present()
{
    g_dev->Present(NULL, NULL, NULL, NULL);
}

void initVideo()
{
    g_d3d = Direct3DCreate8(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = kWidth;
    pp.BackBufferHeight = kHeight;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    g_d3d->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_dev);

    g_dev->SetVertexShader(kFvf);
    g_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    g_dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    g_dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

// --- input ----------------------------------------------------------------------

enum : unsigned {
    kUp = 1, kDown = 2, kLeft = 4, kRight = 8, kA = 16, kB = 32, kStart = 64,
};

HANDLE g_pads[4];

// `plugged` is set when a controller appeared this frame: whatever it holds
// then is not a press (buttons kept down from the dashboard, or a pad that
// reports a button on its first read).
unsigned readButtons(bool* plugged = nullptr)
{
    DWORD insertions = 0, removals = 0;
    if (XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals)) {
        if (plugged && insertions) *plugged = true;
        for (int p = 0; p < 4; ++p) {
            if ((removals & (1u << p)) && g_pads[p]) {
                XInputClose(g_pads[p]);
                g_pads[p] = NULL;
            }
            if ((insertions & (1u << p)) && !g_pads[p])
                g_pads[p] = XInputOpen(XDEVICE_TYPE_GAMEPAD, p, XDEVICE_NO_SLOT, NULL);
        }
    }
    unsigned bits = 0;
    for (int p = 0; p < 4; ++p) {
        XINPUT_STATE s;
        if (!g_pads[p] || XInputGetState(g_pads[p], &s) != ERROR_SUCCESS) continue;
        const XINPUT_GAMEPAD& g = s.Gamepad;
        if ((g.wButtons & XINPUT_GAMEPAD_DPAD_UP) || g.sThumbLY > 16000) bits |= kUp;
        if ((g.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) || g.sThumbLY < -16000) bits |= kDown;
        if ((g.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) || g.sThumbLX < -16000) bits |= kLeft;
        if ((g.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) || g.sThumbLX > 16000) bits |= kRight;
        if (g.wButtons & XINPUT_GAMEPAD_START) bits |= kStart;
        if (g.bAnalogButtons[XINPUT_GAMEPAD_A] > 30) bits |= kA;
        if (g.bAnalogButtons[XINPUT_GAMEPAD_B] > 30) bits |= kB;
    }
    return bits;
}

void initInput()
{
    XInitDevices(0, NULL);
    const DWORD present = XGetDevices(XDEVICE_TYPE_GAMEPAD);
    for (int p = 0; p < 4; ++p)
        if (present & (1u << p)) g_pads[p] = XInputOpen(XDEVICE_TYPE_GAMEPAD, p, XDEVICE_NO_SLOT, NULL);
}

// --- screen -----------------------------------------------------------------------

const float kBarTop = 376.0f;

struct State {
    int selected = 0;
    bool listOpen = false;
    bool focusPlay = true;
    float scroll = 0.0f;
    const char* error = nullptr;
};

void drawBackground(const State& st)
{
    // Panorama across the top, scrolling slowly, darkened like the launcher's news page.
    if (g_panorama.tex) {
        bind(&g_panorama, true, false);
        const float h = kBarTop;
        const float w = g_panorama.width * h / g_panorama.height;
        float x = -st.scroll;
        while (x < kWidth) {
            quad(&g_panorama, x, 0, w, h, 0, 0, float(g_panorama.width), float(g_panorama.height), 0xFF909090);
            x += w;
        }
    } else {
        fill(0, 0, kWidth, kBarTop, 0xFF2A3A55);
    }
    // Dirt bar along the bottom, 2x texels, dark.
    if (g_dirt.tex) {
        bind(&g_dirt, false, true);
        const float tiles = (kHeight - kBarTop) / 32.0f;
        quad(&g_dirt, 0, kBarTop, kWidth, kHeight - kBarTop, 0, 0, 16.0f * kWidth / 32.0f, 16.0f * tiles,
             0xFF505050);
    } else {
        fill(0, kBarTop, kWidth, kHeight - kBarTop, 0xFF3A2A1E);
    }
    fill(0, kBarTop, kWidth, 2, 0xFF000000);
}

void drawNews(const State& st)
{
    const float x = 48, y = 150, w = kWidth - 96, h = kBarTop - y - 14;
    fill(x, y, w, h, 0xB0000000);
    frame(x, y, w, h, 0x60FFFFFF);

    const int sel = st.selected;
    text(kVersions[sel].name, x + 14, y + 12, 3, 0xFFFFFF55);
    float ty = paragraph(kVersions[sel].text, x + 14, y + 46, w - 28, 2, 0xFFE0E0E0);
    if (!g_present[sel]) {
        paragraph("This file is not in the game folder.", x + 14, ty + 6, w - 28, 2, 0xFFFF6060);
    } else if (versionIsHd(sel) && !g_720pAllowed) {
        paragraph("720p is off in the dashboard: it will start at 640x480.", x + 14, ty + 6, w - 28, 2,
                  0xFFFFB040);
    }
    if (st.error) paragraph(st.error, x + 14, y + h - 52, w - 28, 2, 0xFFFF6060);
    text("A choose   B back   START play", x + 14, y + h - 22, 2, 0xFF909090);
}

void drawLogo()
{
    if (g_logo.tex) {
        bind(&g_logo, true, false);
        const float w = 480, h = w * g_logo.height / g_logo.width;
        quad(&g_logo, (kWidth - w) / 2, 26, w, h, 0, 0, float(g_logo.width), float(g_logo.height), 0xFFFFFFFF);
    } else {
        textCentered("OPTICRAFT", kWidth / 2.0f, 50, 6, 0xFFFFFFFF);
    }
}

const float kDropX = 36, kDropY = 410, kDropW = 212, kDropH = 30;
const float kPlayX = 262, kPlayY = 400, kPlayW = 150;

void drawBar(const State& st)
{
    text("Version:", kDropX, 388, 2, 0xFFFFFFFF);
    const bool dropFocus = !st.focusPlay || st.listOpen;
    fill(kDropX, kDropY, kDropW, kDropH, 0xFF000000);
    frame(kDropX, kDropY, kDropW, kDropH, dropFocus ? 0xFFFFFFFF : 0xFF808080);
    text(kVersions[st.selected].name, kDropX + 8, kDropY + 7, 2, 0xFFFFFFFF);
    text(st.listOpen ? "^" : "v", kDropX + kDropW - 20, kDropY + 7, 2, 0xFFC0C0C0);

    button(kPlayX, kPlayY, kPlayW, "Play", st.focusPlay && !st.listOpen);

    text("Ready to play", 426, 394, 2, 0xFFFFFFFF);
    text("Minecraft 1.2.5", 426, 416, 2, 0xFFC0C0C0);

    if (st.listOpen) {
        const float rowH = 28;
        const float top = kDropY - rowH * kVersionCount - 2;
        fill(kDropX, top, kDropW, rowH * kVersionCount + 2, 0xF0101010);
        frame(kDropX, top, kDropW, rowH * kVersionCount + 2, 0xFFFFFFFF);
        for (int i = 0; i < kVersionCount; ++i) {
            const float ry = top + 1 + i * rowH;
            if (i == st.selected) fill(kDropX + 2, ry, kDropW - 4, rowH, 0xFF3060B0);
            text(kVersions[i].name, kDropX + 8, ry + 6, 2, g_present[i] ? 0xFFFFFFFF : 0xFF707070);
        }
    }
}

void draw(const State& st, const char* overlay = nullptr)
{
    g_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
    g_dev->BeginScene();
    drawBackground(st);
    drawLogo();
    drawNews(st);
    drawBar(st);
    if (overlay) {
        fill(0, 0, kWidth, kHeight, 0xC0000000);
        textCentered(overlay, kWidth / 2.0f, kHeight / 2.0f - 12, 3, 0xFFFFFFFF);
    }
    g_dev->EndScene();
    present();
}

void launch(State& st)
{
    const int i = st.selected;
    if (!g_present[i]) {
        st.error = "That version is missing: copy its .xbe next to default.xbe.";
        return;
    }
    char overlay[96];
    std::snprintf(overlay, sizeof(overlay), "Starting %s...", kVersions[i].name);
    draw(st, overlay);
    // Keep that frame on screen while the game loads.
    g_dev->PersistDisplay();
    XLaunchNewImage(kVersions[i].file, NULL);
    st.error = "Could not start that version.";   // only reached if the launch failed
}

} // namespace

void __cdecl main()
{
    initVideo();
    initInput();

    loadImage("D:\\data\\assets\\font\\default.png", g_font, true);
    measureFont();
    loadImage("D:\\data\\assets\\gui\\background.png", g_dirt);
    loadImage("D:\\data\\assets\\gui\\gui.png", g_gui);
    loadImage("D:\\data\\assets\\legacy\\panorama.png", g_panorama);
    loadImage("D:\\data\\assets\\legacy\\title.png", g_logo);

    for (int i = 0; i < kVersionCount; ++i) g_present[i] = fileExists(kVersions[i].file);
    g_720pAllowed = (XGetVideoFlags() & XC_VIDEO_FLAGS_HDTV_720p) != 0;

    State st;   // opens on the normal build

    unsigned previous = readButtons();   // ignore buttons still held from the dashboard
    const DWORD startTick = GetTickCount();
    DWORD lastTick = startTick;
    DWORD repeatAt = 0;
    for (;;) {
        const DWORD now = GetTickCount();
        const float dt = (now - lastTick) / 1000.0f;
        lastTick = now;
        st.scroll += dt * 12.0f;
        if (g_panorama.tex) {
            const float w = g_panorama.width * kBarTop / g_panorama.height;
            while (st.scroll >= w) st.scroll -= w;
        }

        bool plugged = false;
        const unsigned held = readButtons(&plugged);
        if (plugged || now - startTick < 500) {
            previous = held;
            draw(st);
            continue;
        }
        unsigned pressed = held & ~previous;
        // Held up/down repeats, so the list can be scrolled by holding.
        const unsigned arrows = held & (kUp | kDown);
        if (arrows && (held & ~previous & (kUp | kDown))) {
            repeatAt = now + 400;
        } else if (arrows && now >= repeatAt) {
            pressed |= arrows;
            repeatAt = now + 150;
        }
        previous = held;

        if (pressed & (kUp | kDown)) {
            st.listOpen = true;
            st.error = nullptr;
            st.selected = (st.selected + ((pressed & kDown) ? 1 : kVersionCount - 1)) % kVersionCount;
        }
        if (pressed & (kLeft | kRight)) {
            st.listOpen = false;
            st.focusPlay = (pressed & kRight) != 0;
        }
        if (pressed & kB) st.listOpen = false;
        if (pressed & kStart) {
            st.listOpen = false;
            launch(st);
        } else if (pressed & kA) {
            if (st.listOpen) {
                st.listOpen = false;
                st.focusPlay = true;
            } else if (!st.focusPlay) {
                st.listOpen = true;
            } else {
                launch(st);
            }
        }
        draw(st);
    }
}
