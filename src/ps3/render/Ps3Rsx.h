#pragma once

// Ps3Rsx -- owns the RSX for the game: context, frame buffers, depth buffer,
// flips, and the two memory pools the renderer draws from.
//
//  * Transient memory: a ring in main memory mapped for the RSX (the rsxInit
//    host buffer), split in two halves that alternate per frame. Immediate
//    draws copy their vertices here. Before a half is reused the CPU waits for
//    the GPU to finish the frame that last used it (a backend label).
//  * Local memory: RSX VRAM (rsxMemalign) for textures and display-list meshes.
//    Frees are deferred until the GPU has passed the frame that freed them.
//
// RenderAPI_RSX_PS3.cpp builds the GL-shaped API on top of this; Display_ps3.cpp
// drives the frame.

#include <rsx/rsx.h>

#include <cstdint>

namespace Ps3Rsx
{
bool initialize();
bool ready();
gcmContextData* context();

int width();
int height();

// Frame boundaries. beginFrame() targets the back buffer; endFrame() flips it.
void beginFrame();
void endFrame();
std::uint32_t frameNumber();

// Re-applies the back buffer as render target (after anything that changed it).
void bindBackBuffer();

// Transient (this frame only) memory the RSX reads from main memory.
// Returns null when the frame's half of the ring is full.
void* allocTransient(std::uint32_t bytes, std::uint32_t alignment, std::uint32_t* offset);
constexpr std::uint32_t kTransientLocation = GCM_LOCATION_CELL;

// Persistent RSX VRAM. freeLocal() is deferred until the GPU is done with it.
void* allocLocal(std::uint32_t bytes, std::uint32_t alignment, std::uint32_t* offset);
void freeLocal(void* pointer);

// A 1x1 opaque white texture: bound to units the game leaves disabled.
std::uint32_t whiteTextureOffset();

// Blocks until the RSX has executed everything submitted so far.
void waitIdle();

// Test builds (PS3_SCREENSHOTS): save the next finished frame to
// /dev_hdd0/tmp/opticraft_shot.raw. A no-op otherwise.
void requestScreenshot();

// Waits for the RSX to finish everything before the process exits (atexit).
void shutdown();

// Red screen forever, for fatal errors (ClientPlatformPolicy::reportCrash).
[[noreturn]] void holdOnCrashScreen();
}
