#ifdef PS3_PLATFORM

#include "ps3/render/Ps3Rsx.h"

#include "platform/Log.h"
#include "ps3/system/Ps3Clock.h"
#include "ps3/system/Ps3Stage.h"
#include "ps3/system/Ps3Video.h"

#include <malloc.h>
#include <ppu-asm.h>
#include <sys/thread.h>
#include <sysutil/video.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>

extern "C" void ps3PlatformLogWrite(const char* line);
extern "C" const char* volatile g_ps3Stage = "start";
extern "C" volatile double g_ps3StageValues[6] = {};

namespace
{
// Command buffer at the start of the host buffer; the rest is the transient
// ring. The host buffer must be 1 MB aligned and a multiple of 1 MB.
constexpr std::uint32_t kCommandBufferSize = 8 * 1024 * 1024;
constexpr std::uint32_t kHostSize = 64 * 1024 * 1024;
constexpr std::uint32_t kRingGuard = 1024 * 1024;
constexpr std::uint32_t kIdleLabel = 255;
constexpr std::uint32_t kFrameLabel = 254;

gcmContextData* s_context = nullptr;
bool s_ready = false;
int s_width = 1280;
int s_height = 720;
std::uint32_t s_colorPitch = 0;
std::uint32_t s_colorOffset[2] = {};
void* s_colorBuffer[2] = {};
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
bool s_screenshotRequested = false;  // test builds: see dumpBackBuffer()

// The command buffer is managed here, in two halves that alternate per frame
// (like the vertex ring). The firmware's own wrap -- the default context callback
// jumping back to the start when the buffer fills -- computed a bad target
// (RPCS3: "jump 0x1f800000", then "Dead FIFO"), which is also why PSL1GHT's
// rsxtest sample hung the real console once its buffer wrapped. Frames switch
// halves with an explicit jump; a frame that fills its half wraps inside it
// after the GPU has caught up (overflowCallback).
constexpr std::uint32_t kJumpFlag = 0x20000000u;
constexpr std::uint32_t kHalfMarginWords = 16;  // room for the jump past `end`
u32* s_cbBegin[2] = {};
u32* s_cbEnd[2] = {};
std::uint32_t s_cbOffset[2] = {};
int s_cbHalf = 0;
bool s_cbOverflowLogged = false;

void writeJump(std::uint32_t offset)
{
    // Written by hand: the rsx command helpers would re-enter the callback.
    *s_context->current = offset | kJumpFlag;
    ++s_context->current;
}

// Every CPU wait on the GPU goes through here. A wait that lasts seconds means
// the RSX stopped (bad command or address) -- the console just freezes -- so log
// where and the FIFO state once, for the netlog.
// Time the main thread spent in the waits below, for the heartbeat.
volatile std::uint64_t s_waitMicros = 0;

struct StallWatch
{
    const char* site;
    Ps3StageScope stage;
    std::uint64_t spins = 0;
    bool logged = false;
    std::uint64_t startMicros;
    explicit StallWatch(const char* name) : site(name), stage(name), startMicros(ps3GetMonotonicMicros()) {}
    ~StallWatch() { s_waitMicros += ps3GetMonotonicMicros() - startMicros; }
    void tick(std::uint32_t sleepUs)
    {
        usleep(sleepUs);
        if (logged || ++spins * sleepUs < 3 * 1000 * 1000)
            return;
        logged = true;
        gcmControlRegister* control = gcmGetControlRegister();
        MC_LOG_ERROR("ps3.rsx", "GPU stalled in %s: get=%08x put=%08x ref=%08x frame=%u done=%u idle=%u/%u half=%d\n",
                     site, control->get, control->put, control->ref, s_frame,
                     *reinterpret_cast<volatile std::uint32_t*>(gcmGetLabelAddress(kFrameLabel)),
                     *reinterpret_cast<volatile std::uint32_t*>(gcmGetLabelAddress(kIdleLabel)), s_idleValue,
                     s_cbHalf);
    }
};

// A heartbeat every 5 s from its own high-priority thread: frame counter and FIFO
// state. After a freeze the last beats say whether frames still advance and where
// the GPU is. Written straight to the sink, not through McLog, whose lock the
// stuck thread may hold.
//
// Between beats it samples g_ps3Stage about every millisecond: a poor man's
// profiler. Each beat is followed by a "ps3.prof" line with the share of
// samples per stage (the innermost PS3_STAGE_SCOPE the main thread was in).
struct StageSample
{
    const char* name;
    unsigned count;
};

void writeProfile(StageSample* samples, int used, unsigned total)
{
    if (total == 0)
        return;
    std::sort(samples, samples + used, [](const StageSample& a, const StageSample& b) { return a.count > b.count; });
    char line[512];
    int length = std::snprintf(line, sizeof(line), "[MC][I][ps3.prof]");
    for (int i = 0; i < used && i < 12 && length < static_cast<int>(sizeof(line)) - 40; ++i)
        length += std::snprintf(line + length, sizeof(line) - length, " %s=%u%%", samples[i].name,
                                samples[i].count * 100 / total);
    std::snprintf(line + length, sizeof(line) - length, "\n");
    ps3PlatformLogWrite(line);
}

void frameWatchdog(void*)
{
    std::uint32_t lastFrame = 0;
    constexpr int kMaxStages = 48;
    StageSample samples[kMaxStages];
    for (;;)
    {
        int used = 0;
        unsigned total = 0;
        const std::uint64_t beatEnd = ps3GetMonotonicMicros() + 5 * 1000 * 1000;
        while (ps3GetMonotonicMicros() < beatEnd)
        {
            usleep(1000);
            const char* stage = g_ps3Stage;
            int i = 0;
            while (i < used && samples[i].name != stage)
                ++i;
            if (i == used)
            {
                if (used == kMaxStages)
                    continue;
                samples[used++] = {stage, 0};
            }
            ++samples[i].count;
            ++total;
        }
        gcmControlRegister* control = gcmGetControlRegister();
        const std::uint32_t frame = *const_cast<volatile std::uint32_t*>(&s_frame);
        const unsigned frames = frame - lastFrame;
        const unsigned fps = frames / 5;
        lastFrame = frame;
        const std::uint64_t waited = s_waitMicros;
        s_waitMicros = 0;
        const unsigned frameMs = frames != 0 ? 5000 / frames : 0;
        const unsigned waitMs = frames != 0 ? static_cast<unsigned>(waited / 1000 / frames) : 0;
        char line[320];
        std::snprintf(line, sizeof(line),
                      "[MC][I][ps3.wd] fps=%u frameMs=%u waitMs=%u stage=%s frame=%u done=%u get=%08x put=%08x ref=%08x half=%d\n",
                      fps, frameMs, waitMs, g_ps3Stage, frame,
                      *reinterpret_cast<volatile std::uint32_t*>(gcmGetLabelAddress(kFrameLabel)),
                      control->get, control->put, control->ref, s_cbHalf);
        ps3PlatformLogWrite(line);
        writeProfile(samples, used, total);
    }
}

void waitGpuCaughtUp()
{
    gcmControlRegister* control = gcmGetControlRegister();
    StallWatch watch("waitGpuCaughtUp");
    while (control->get != control->put)
        watch.tick(20);
}

// Points the context at `half` (its begin) and makes the GPU jump there.
void switchToHalf(int half)
{
    writeJump(s_cbOffset[half]);
    s_context->current = s_cbBegin[half];
    s_context->end = s_cbEnd[half];
    s_cbHalf = half;
    rsxFlushBuffer(s_context);
}

s32 overflowCallback(gcmContextData* context, u32 count)
{
    (void)context;
    (void)count;
    if (!s_cbOverflowLogged)
    {
        MC_LOG_WARN("ps3.rsx", "frame filled its command buffer half; wrapping inside it\n");
        s_cbOverflowLogged = true;
    }
    // Jump back to the start of this half, then wait until the GPU has executed
    // everything up to there before the CPU writes over it.
    switchToHalf(s_cbHalf);
    waitGpuCaughtUp();
    return 0;
}

// The context gcm hands back does NOT span the command buffer: in its default
// FIFO mode begin..end is one small segment (~16 KB) that the firmware callback
// rotates. The halves are cut from the whole buffer instead: from the context's
// begin (gcm keeps the first 4 KB of the host buffer) to the end of the
// kCommandBufferSize bytes reserved at the start of the host buffer.
void setupCommandBufferHalves(void* host)
{
    u32* begin = s_context->begin;
    u32* end = reinterpret_cast<u32*>(static_cast<unsigned char*>(host) + kCommandBufferSize) - 1;
    const std::size_t words = static_cast<std::size_t>(end - begin);
    const std::size_t halfWords = words / 2;
    s_cbBegin[0] = begin;
    s_cbBegin[1] = begin + halfWords;
    s_cbEnd[0] = s_cbBegin[1] - kHalfMarginWords;
    s_cbEnd[1] = end - kHalfMarginWords;
    rsxAddressToOffset(s_cbBegin[0], &s_cbOffset[0]);
    rsxAddressToOffset(s_cbBegin[1], &s_cbOffset[1]);
    s_context->callback = reinterpret_cast<gcmContextCallback>(__get_opd32(overflowCallback));
    // Start in half 1 so half 0 (where rsxInit already wrote) is never mixed.
    switchToHalf(1);
    waitGpuCaughtUp();
}

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
    StallWatch watch("waitForFrame");
    while (static_cast<std::int32_t>(finishedFrame() - frame) < 0)
        watch.tick(20);
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
    config.resolution = static_cast<u8>(Ps3Video::resolutionId());
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
        s_colorBuffer[i] = buffer;
        gcmSetDisplayBuffer(i, s_colorOffset[i], s_colorPitch, s_width, s_height);
    }
    s_depthPitch = static_cast<std::uint32_t>(s_width) * 4;
    void* depth = rsxMemalign(64, s_depthPitch * static_cast<std::uint32_t>(s_height));
    if (depth == nullptr)
        return false;
    rsxAddressToOffset(depth, &s_depthOffset);

    // librsx keeps more than kCommandBufferSize at the start of the host buffer (gcm's
    // reserved area sits in front of the command buffer), so starting the ring right at
    // kCommandBufferSize let vertex data overwrite the last commands (RPCS3: "Invalid RSX
    // method" with garbage arguments). Leave a 1 MB guard.
    s_ringBase = static_cast<unsigned char*>(host) + kCommandBufferSize + kRingGuard;
    s_ringHalfSize = (kHostSize - kCommandBufferSize - kRingGuard) / 2;
    s_ringUsed = 0;

    // A8R8G8B8 big-endian: bytes A, R, G, B.
    std::uint32_t* white = static_cast<std::uint32_t*>(rsxMemalign(128, 128));
    for (int i = 0; i < 32; ++i)
        white[i] = 0xFFFFFFFFu;
    rsxAddressToOffset(white, &s_whiteOffset);

    setupCommandBufferHalves(host);
    MC_LOG_INFO("ps3.rsx", "command buffer halves: %u KB each\n",
                static_cast<unsigned>((s_cbEnd[0] - s_cbBegin[0]) * 4 / 1024));

    *frameLabel() = 0;
    // The game leaves through exit(0) (PLATFORM_EXIT_PROCESS_ON_SHUTDOWN).
    std::atexit([] { Ps3Rsx::shutdown(); });
    sys_ppu_thread_t watchdog;
    if (sysThreadCreate(&watchdog, frameWatchdog, nullptr, 200, 16 * 1024, 0, const_cast<char*>("frame watchdog")) != 0)
        MC_LOG_ERROR("ps3.rsx", "watchdog thread not started\n");
    s_ready = true;
    waitIdle();
    s_backBuffer = 0;
    setSurface();
    MC_LOG_INFO("ps3.rsx", "ready %dx%d, host %u MB, ring 2x%u KB\n", s_width, s_height,
                kHostSize >> 20, s_ringHalfSize >> 10);
    return true;
}

void requestScreenshot()
{
#if PS3_SCREENSHOTS
    s_screenshotRequested = true;
#endif
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

#if PS3_SCREENSHOTS
// Test builds: the finished back buffer on request (Ps3Rsx::requestScreenshot,
// the remote control's SHOT command), raw X8R8G8B8 rows (big-endian bytes X, R,
// G, B) to /dev_hdd0/tmp/opticraft_shot.raw, then an empty .done marker.
// ps3\tools\mando.py waits for the marker and turns the shot into a PNG.
void dumpBackBuffer()
{
    if (!s_screenshotRequested)
        return;
    s_screenshotRequested = false;
    waitIdle();
    std::remove("/dev_hdd0/tmp/opticraft_shot.done");
    if (FILE* f = std::fopen("/dev_hdd0/tmp/opticraft_shot.raw", "wb"))
    {
        std::fwrite(s_colorBuffer[s_backBuffer], 1, s_colorPitch * static_cast<std::uint32_t>(s_height), f);
        std::fclose(f);
    }
    if (FILE* done = std::fopen("/dev_hdd0/tmp/opticraft_shot.done", "wb"))
        std::fclose(done);
    MC_LOG_INFO("ps3.rsx", "screenshot taken (%dx%d)\n", s_width, s_height);
}
#endif

void endFrame()
{
    if (!s_ready)
        return;
#if PS3_SCREENSHOTS
    dumpBackBuffer();
#endif
    // The CPU does not wait for the previous flip: gcmSetWaitFlip below already
    // stops the GPU until this flip has happened, so the next frame's drawing
    // (into the other buffer, the one that just left the screen) cannot start
    // early, and beginFrame() keeps the CPU at most one frame ahead. Waiting here
    // as well locked any frame slower than 16.7 ms to 30 FPS.
    s_firstFlip = false;

    gcmSetFlip(s_context, static_cast<u8>(s_backBuffer));
    gcmSetWaitFlip(s_context);
    rsxSetWriteBackendLabel(s_context, kFrameLabel, s_frame);
    // Next frame builds in the other half; beginFrame() waits for the frame that
    // last used it (s_frame - 1 after the increment below) before writing there.
    switchToHalf(s_cbHalf ^ 1);

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
    StallWatch watch("waitIdle");
    while (*reinterpret_cast<volatile std::uint32_t*>(gcmGetLabelAddress(kIdleLabel)) != s_idleValue)
        watch.tick(30);
    ++s_idleValue;
    // Everything submitted is done, including all frames so far.
    *frameLabel() = s_frame - 1;
}

// Leaving the process with the RSX still running (a flip queued, the GPU
// parked on gcmSetWaitFlip) froze the whole console on quit. Drain it first.
void shutdown()
{
    if (!s_ready)
        return;
    MC_LOG_INFO("ps3.rsx", "shutdown: draining the RSX\n");
    waitIdle();
    rsxFinish(s_context, 1);
    s_ready = false;
    MC_LOG_INFO("ps3.rsx", "shutdown: RSX idle\n");
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
