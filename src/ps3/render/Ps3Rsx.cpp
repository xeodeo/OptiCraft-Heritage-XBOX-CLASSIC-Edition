#ifdef PS3_PLATFORM

#include "ps3/render/Ps3Rsx.h"

#include "platform/Log.h"
#include "ps3/system/Ps3Video.h"

#include <malloc.h>
#include <sysutil/video.h>
#include <unistd.h>

#include <cstring>
#include <deque>

namespace
{
// Command buffer at the start of the host buffer; the rest is the transient
// ring. The host buffer must be 1 MB aligned and a multiple of 1 MB.
constexpr std::uint32_t kCommandBufferSize = 2 * 1024 * 1024;
constexpr std::uint32_t kHostSize = 64 * 1024 * 1024;
constexpr std::uint32_t kIdleLabel = 255;
constexpr std::uint32_t kFrameLabel = 254;

gcmContextData* s_context = nullptr;
bool s_ready = false;
int s_width = 1280;
int s_height = 720;
std::uint32_t s_colorPitch = 0;
std::uint32_t s_colorOffset[2] = {};
std::uint32_t s_depthOffset = 0;
std::uint32_t s_depthPitch = 0;
int s_backBuffer = 0;
bool s_firstFlip = true;

std::uint32_t s_idleValue = 1;
std::uint32_t s_frame = 1;  // frame being built; the frame label holds the last finished one

unsigned char* s_ringBase = nullptr;
std::uint32_t s_ringHalfSize = 0;
std::uint32_t s_ringUsed = 0;
bool s_ringFullLogged = false;

std::uint32_t s_whiteOffset = 0;

struct PendingFree
{
    std::uint32_t frame;
    void* pointer;
};
std::deque<PendingFree> s_pendingFrees;

volatile std::uint32_t* frameLabel()
{
    return reinterpret_cast<volatile std::uint32_t*>(gcmGetLabelAddress(kFrameLabel));
}

// Frames the GPU has completely finished.
std::uint32_t finishedFrame()
{
    return *frameLabel();
}

void waitForFrame(std::uint32_t frame)
{
    while (static_cast<std::int32_t>(finishedFrame() - frame) < 0)
        usleep(20);
}

void releaseFinishedFrees()
{
    const std::uint32_t done = finishedFrame();
    while (!s_pendingFrees.empty() && static_cast<std::int32_t>(done - s_pendingFrees.front().frame) >= 0)
    {
        rsxFree(s_pendingFrees.front().pointer);
        s_pendingFrees.pop_front();
    }
}

void setSurface()
{
    gcmSurface surface;
    std::memset(&surface, 0, sizeof(surface));
    surface.colorFormat = GCM_SURFACE_X8R8G8B8;
    surface.colorTarget = GCM_SURFACE_TARGET_0;
    for (int i = 0; i < GCM_MAX_MRT_COUNT; ++i)
    {
        surface.colorLocation[i] = GCM_LOCATION_RSX;
        surface.colorPitch[i] = 64;
    }
    surface.colorOffset[0] = s_colorOffset[s_backBuffer];
    surface.colorPitch[0] = s_colorPitch;
    surface.depthFormat = GCM_SURFACE_ZETA_Z24S8;
    surface.depthLocation = GCM_LOCATION_RSX;
    surface.depthOffset = s_depthOffset;
    surface.depthPitch = s_depthPitch;
    surface.type = GCM_SURFACE_TYPE_LINEAR;
    surface.antiAlias = GCM_SURFACE_CENTER_1;
    surface.width = static_cast<u16>(s_width);
    surface.height = static_cast<u16>(s_height);
    rsxSetSurface(s_context, &surface);
}

bool configureVideo()
{
    // Keep the mode the XMB is in (see Ps3Video.h).
    videoState state;
    if (videoGetState(0, 0, &state) != 0)
        return false;
    s_width = Ps3Video::width();
    s_height = Ps3Video::height();
    videoConfiguration config;
    std::memset(&config, 0, sizeof(config));
    config.resolution = state.displayMode.resolution;
    config.format = VIDEO_BUFFER_FORMAT_XRGB;
    config.aspect = VIDEO_ASPECT_AUTO;
    config.pitch = static_cast<u32>(s_width) * 4;
    return videoConfigure(0, &config, nullptr, 0) == 0;
}
} // namespace

namespace Ps3Rsx
{
bool initialize()
{
    if (s_ready)
        return true;

    void* host = memalign(1024 * 1024, kHostSize);
    if (host == nullptr)
    {
        MC_LOG_ERROR("ps3.rsx", "host buffer allocation failed (%u bytes)\n", kHostSize);
        return false;
    }
    if (rsxInit(&s_context, kCommandBufferSize, kHostSize, host) != 0 || s_context == nullptr)
    {
        MC_LOG_ERROR("ps3.rsx", "rsxInit failed\n");
        return false;
    }
    if (!configureVideo())
    {
        MC_LOG_ERROR("ps3.rsx", "videoConfigure failed\n");
        return false;
    }
    gcmSetFlipMode(GCM_FLIP_VSYNC);

    s_colorPitch = static_cast<std::uint32_t>(s_width) * 4;
    for (int i = 0; i < 2; ++i)
    {
        void* buffer = rsxMemalign(64, s_colorPitch * static_cast<std::uint32_t>(s_height));
        if (buffer == nullptr)
            return false;
        rsxAddressToOffset(buffer, &s_colorOffset[i]);
        gcmSetDisplayBuffer(i, s_colorOffset[i], s_colorPitch, s_width, s_height);
    }
    s_depthPitch = static_cast<std::uint32_t>(s_width) * 4;
    void* depth = rsxMemalign(64, s_depthPitch * static_cast<std::uint32_t>(s_height));
    if (depth == nullptr)
        return false;
    rsxAddressToOffset(depth, &s_depthOffset);

    s_ringBase = static_cast<unsigned char*>(host) + kCommandBufferSize;
    s_ringHalfSize = (kHostSize - kCommandBufferSize) / 2;
    s_ringUsed = 0;

    // A8R8G8B8 big-endian: bytes A, R, G, B.
    std::uint32_t* white = static_cast<std::uint32_t*>(rsxMemalign(128, 128));
    for (int i = 0; i < 32; ++i)
        white[i] = 0xFFFFFFFFu;
    rsxAddressToOffset(white, &s_whiteOffset);

    *frameLabel() = 0;
    s_ready = true;
    waitIdle();
    s_backBuffer = 0;
    setSurface();
    MC_LOG_INFO("ps3.rsx", "ready %dx%d, host %u MB, ring 2x%u KB\n", s_width, s_height,
                kHostSize >> 20, s_ringHalfSize >> 10);
    return true;
}

bool ready() { return s_ready; }
gcmContextData* context() { return s_context; }
int width() { return s_width; }
int height() { return s_height; }
std::uint32_t frameNumber() { return s_frame; }

void bindBackBuffer()
{
    if (s_ready)
        setSurface();
}

void beginFrame()
{
    if (!s_ready)
        return;
    // This frame reuses the ring half of frame s_frame - 2: the GPU must be done with it.
    if (s_frame > 2)
        waitForFrame(s_frame - 2);
    s_ringUsed = 0;
    s_ringFullLogged = false;
    releaseFinishedFrees();
    setSurface();
}

void endFrame()
{
    if (!s_ready)
        return;
    // At most one flip in flight: wait for the previous one before queueing.
    if (!s_firstFlip)
    {
        while (gcmGetFlipStatus() != 0)
            usleep(200);
    }
    gcmResetFlipStatus();
    s_firstFlip = false;

    rsxSetWriteBackendLabel(s_context, kFrameLabel, s_frame);
    gcmSetFlip(s_context, static_cast<u8>(s_backBuffer));
    rsxFlushBuffer(s_context);
    gcmSetWaitFlip(s_context);

    ++s_frame;
    s_backBuffer ^= 1;
    beginFrame();
}

void* allocTransient(std::uint32_t bytes, std::uint32_t alignment, std::uint32_t* offset)
{
    if (!s_ready || bytes == 0)
        return nullptr;
    const std::uint32_t align = alignment < 16 ? 16 : alignment;
    std::uint32_t start = (s_ringUsed + align - 1) & ~(align - 1);
    if (start + bytes > s_ringHalfSize)
    {
        if (!s_ringFullLogged)
        {
            MC_LOG_ERROR("ps3.rsx", "transient ring full (%u KB used, %u requested)\n", s_ringUsed >> 10, bytes);
            s_ringFullLogged = true;
        }
        return nullptr;
    }
    s_ringUsed = start + bytes;
    unsigned char* pointer = s_ringBase + (s_frame & 1) * s_ringHalfSize + start;
    rsxAddressToOffset(pointer, offset);
    return pointer;
}

void* allocLocal(std::uint32_t bytes, std::uint32_t alignment, std::uint32_t* offset)
{
    void* pointer = rsxMemalign(alignment < 128 ? 128 : alignment, bytes);
    if (pointer == nullptr)
    {
        // VRAM is full of memory freed this frame: let the GPU catch up and retry.
        waitIdle();
        releaseFinishedFrees();
        pointer = rsxMemalign(alignment < 128 ? 128 : alignment, bytes);
        if (pointer == nullptr)
        {
            MC_LOG_ERROR("ps3.rsx", "VRAM allocation failed (%u bytes)\n", bytes);
            return nullptr;
        }
    }
    rsxAddressToOffset(pointer, offset);
    return pointer;
}

void freeLocal(void* pointer)
{
    if (pointer != nullptr)
        s_pendingFrees.push_back({s_frame, pointer});
}

std::uint32_t whiteTextureOffset() { return s_whiteOffset; }

void waitIdle()
{
    if (!s_ready)
        return;
    rsxSetWriteBackendLabel(s_context, kIdleLabel, s_idleValue);
    rsxSetWaitLabel(s_context, kIdleLabel, s_idleValue);
    ++s_idleValue;
    rsxSetWriteBackendLabel(s_context, kIdleLabel, s_idleValue);
    rsxFlushBuffer(s_context);
    while (*reinterpret_cast<volatile std::uint32_t*>(gcmGetLabelAddress(kIdleLabel)) != s_idleValue)
        usleep(30);
    ++s_idleValue;
    // Everything submitted is done, including all frames so far.
    *frameLabel() = s_frame - 1;
}

void holdOnCrashScreen()
{
    for (;;)
    {
        if (s_ready)
        {
            setSurface();
            rsxSetColorMask(s_context, GCM_COLOR_MASK_R | GCM_COLOR_MASK_G | GCM_COLOR_MASK_B | GCM_COLOR_MASK_A);
            rsxSetClearColor(s_context, 0x00A00000);
            rsxClearSurface(s_context, GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A);
            endFrame();
        }
        usleep(100 * 1000);
    }
}
} // namespace Ps3Rsx

#endif // PS3_PLATFORM
