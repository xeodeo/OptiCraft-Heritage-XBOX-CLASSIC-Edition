// Ps3Bringup.cpp -- PS3 toolchain/RSX smoke test (preset ps3-bringup).
//
// Draws a shader triangle for five seconds and returns to the XMB. Every step is
// appended to /dev_hdd0/tmp/opticraft_bringup.txt, so a hang on the real console
// still says how far it got (read it back over FTP). It mirrors ps3\prueba\rsx-tri,
// the first RSX program confirmed on hardware; PSL1GHT's own rsxtest sample hangs
// the real console even though RPCS3 runs it, so it is not a reference.

#include <cstdio>
#include <cstring>
#include <malloc.h>
#include <unistd.h>
#include <sys/process.h>
#include <sysutil/video.h>
#include <rsx/rsx.h>

#include "bringup_color_vpo.h"
#include "bringup_color_fpo.h"

SYS_PROCESS_PARAM(1001, 0x100000);

namespace
{
constexpr u32 kCommandBufferSize = 0x80000;
constexpr u32 kHostSize = 1024 * 1024;
constexpr u32 kLabelIndex = 255;
constexpr const char* kLogPath = "/dev_hdd0/tmp/opticraft_bringup.txt";

struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

gcmContextData* g_context = nullptr;
u32 g_labelValue = 1;

void logStep(const char* text)
{
    if (FILE* f = std::fopen(kLogPath, "a"))
    {
        std::fprintf(f, "%s\n", text);
        std::fclose(f);
    }
}

void waitIdle()
{
    rsxSetWriteBackendLabel(g_context, kLabelIndex, g_labelValue);
    rsxSetWaitLabel(g_context, kLabelIndex, g_labelValue);
    ++g_labelValue;
    rsxSetWriteBackendLabel(g_context, kLabelIndex, g_labelValue);
    rsxFlushBuffer(g_context);
    while (*(vu32*)gcmGetLabelAddress(kLabelIndex) != g_labelValue)
        usleep(30);
    ++g_labelValue;
}

void setColorSurface(u32 offset, u32 pitch, u32 width, u32 height)
{
    gcmSurface sf;
    std::memset(&sf, 0, sizeof(sf));
    sf.colorFormat = GCM_SURFACE_X8R8G8B8;
    sf.colorTarget = GCM_SURFACE_TARGET_0;
    for (int i = 0; i < GCM_MAX_MRT_COUNT; ++i)
    {
        sf.colorLocation[i] = GCM_LOCATION_RSX;
        sf.colorPitch[i] = 64;
    }
    sf.colorOffset[0] = offset;
    sf.colorPitch[0] = pitch;
    sf.depthFormat = GCM_SURFACE_ZETA_Z16;
    sf.depthLocation = GCM_LOCATION_RSX;
    sf.depthPitch = 64;
    sf.type = GCM_SURFACE_TYPE_LINEAR;
    sf.antiAlias = GCM_SURFACE_CENTER_1;
    sf.width = width;
    sf.height = height;
    rsxSetSurface(g_context, &sf);
}
} // namespace

int main()
{
    char line[160];
    std::remove(kLogPath);
    logStep("1 start (cmake ps3-bringup)");

    void* host = memalign(1024 * 1024, kHostSize);
    rsxInit(&g_context, kCommandBufferSize, kHostSize, host);
    std::snprintf(line, sizeof(line), "2 rsxInit ctx=%p", (void*)g_context);
    logStep(line);
    if (!g_context)
        return 1;

    // Keep the resolution the console is already in. Forcing another mode
    // (rsxtest tries 960x1080 first) is not safe on a real TV.
    videoState state;
    videoGetState(0, 0, &state);
    videoResolution resolution;
    videoGetResolution(state.displayMode.resolution, &resolution);
    videoConfiguration config = {state.displayMode.resolution, VIDEO_BUFFER_FORMAT_XRGB,
                                 VIDEO_ASPECT_AUTO, {0, 0, 0, 0, 0, 0, 0, 0, 0},
                                 (u32)resolution.width * 4};
    videoConfigure(0, &config, nullptr, 0);
    waitIdle();
    gcmSetFlipMode(GCM_FLIP_VSYNC);
    std::snprintf(line, sizeof(line), "3 video %dx%d", resolution.width, resolution.height);
    logStep(line);

    const u32 width = resolution.width;
    const u32 height = resolution.height;
    const u32 pitch = width * 4;
    u32 frameOffset[2];
    for (int i = 0; i < 2; ++i)
    {
        void* buffer = rsxMemalign(64, height * pitch);
        rsxAddressToOffset(buffer, &frameOffset[i]);
        gcmSetDisplayBuffer(i, frameOffset[i], pitch, width, height);
    }
    logStep("4 framebuffers");

    rsxVertexProgram* vertexProgram = (rsxVertexProgram*)bringup_color_vpo;
    rsxFragmentProgram* fragmentProgram = (rsxFragmentProgram*)bringup_color_fpo;
    void* vertexUcode;
    u32 vertexSize;
    void* fragmentUcode;
    u32 fragmentSize;
    rsxVertexProgramGetUCode(vertexProgram, &vertexUcode, &vertexSize);
    rsxFragmentProgramGetUCode(fragmentProgram, &fragmentUcode, &fragmentSize);
    void* fragmentMemory = rsxMemalign(64, fragmentSize);
    std::memcpy(fragmentMemory, fragmentUcode, fragmentSize);
    u32 fragmentOffset;
    rsxAddressToOffset(fragmentMemory, &fragmentOffset);
    std::snprintf(line, sizeof(line), "5 shaders vp=%u fp=%u bytes", vertexSize, fragmentSize);
    logStep(line);

    Vertex* vertices = (Vertex*)rsxMemalign(128, 3 * sizeof(Vertex));
    const Vertex triangle[3] = {
        {0.0f, 0.7f, 0.0f, 1, 0, 0, 1},
        {-0.7f, -0.6f, 0.0f, 0, 1, 0, 1},
        {0.7f, -0.6f, 0.0f, 0, 0, 1, 1},
    };
    std::memcpy(vertices, triangle, sizeof(triangle));
    u32 positionOffset;
    u32 colorOffset;
    rsxAddressToOffset(&vertices[0].x, &positionOffset);
    rsxAddressToOffset(&vertices[0].r, &colorOffset);
    logStep("6 vertices");

    gcmResetFlipStatus();
    for (int frame = 0; frame < 300; ++frame)
    {
        const int buffer = frame & 1;
        setColorSurface(frameOffset[buffer], pitch, width, height);

        float scale[4] = {width * 0.5f, height * -0.5f, 0.5f, 0.0f};
        float offset[4] = {width * 0.5f, height * 0.5f, 0.5f, 0.0f};
        rsxSetViewport(g_context, 0, 0, width, height, 0.0f, 1.0f, scale, offset);
        rsxSetScissor(g_context, 0, 0, width, height);
        rsxSetColorMask(g_context, GCM_COLOR_MASK_R | GCM_COLOR_MASK_G | GCM_COLOR_MASK_B | GCM_COLOR_MASK_A);
        rsxSetColorMaskMrt(g_context, 0);
        rsxSetDepthTestEnable(g_context, GCM_FALSE);
        rsxSetCullFaceEnable(g_context, GCM_FALSE);
        rsxSetClearColor(g_context, 0x00402020 + ((frame & 0x7F) << 8));
        rsxClearSurface(g_context, GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A);

        rsxBindVertexArrayAttrib(g_context, GCM_VERTEX_ATTRIB_POS, 0, positionOffset, sizeof(Vertex), 3,
                                 GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
        rsxBindVertexArrayAttrib(g_context, GCM_VERTEX_ATTRIB_COLOR0, 0, colorOffset, sizeof(Vertex), 4,
                                 GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
        rsxLoadVertexProgram(g_context, vertexProgram, vertexUcode);
        rsxLoadFragmentProgramLocation(g_context, fragmentProgram, fragmentOffset, GCM_LOCATION_RSX);
        rsxDrawVertexArray(g_context, GCM_TYPE_TRIANGLES, 0, 3);

        if (frame < 2)
        {
            std::snprintf(line, sizeof(line), "7 draw sent frame %d", frame);
            logStep(line);
            waitIdle();
            logStep("8 rsx finished the draw");
        }

        gcmSetFlip(g_context, buffer);
        rsxFlushBuffer(g_context);
        gcmSetWaitFlip(g_context);
        while (gcmGetFlipStatus() != 0)
            usleep(200);
        gcmResetFlipStatus();
        if (frame < 2)
            logStep("9 flip ok");
    }
    logStep("10 done, 300 frames");
    waitIdle();
    rsxFinish(g_context, 1);
    logStep("11 clean exit");
    return 0;
}
