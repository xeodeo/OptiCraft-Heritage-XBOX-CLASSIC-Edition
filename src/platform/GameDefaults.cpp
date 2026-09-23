#include "platform/GameDefaults.h"

#include "platform/PlatformConfig.h"
#include "platform/PlatformTuning.h"

const PlatformGameDefaults& platformGameDefaults()
{
    static const PlatformGameDefaults defaults = [] {
        PlatformGameDefaults d;
#if PLATFORM_CONSOLE_LOW || PLATFORM_WII || PLATFORM_PC_LEGACY || PLATFORM_XBOX
        d.usePerformanceProfile = true;
        d.renderDistance = PLATFORM_DEFAULT_RENDER_DISTANCE;
        // Fast graphics reduce terrain vertex count: BlockLeaves reports
        // isOpaqueCube() == !graphicsLevel, so interior leaf faces are culled
        // instead of entering the chunk mesh and the translucent terrain pass.
        // This remains a normal setting toggle; only the profile default changes.
        d.fancyGraphics = false;
        // Smooth lighting off on both consoles: each AO face samples nine
        // light values and blends four corner brightnesses, which is most of
        // the per-face cost of RenderBlocks on a chunk build. The setting stays
        // available in the options.
        d.ambientOcclusion = false;
#if PLATFORM_PC_LEGACY
        d.particleSetting = 2;
#elif PLATFORM_WII || PLATFORM_XBOX
        // Decreased: spawnParticle drops a third of the requests. A block
        // break alone is 4x4x4 EntityFX, each a live entity with its own
        // collision sweep every tick; PS2 skips particles outright.
        d.particleSetting = 1;
#else
        d.particleSetting = 0;
#endif
#if PLATFORM_WII || PLATFORM_XBOX
        // Balanced: the GX/D3D swap already waits for vsync, so the
        // Power saver sleep before the swap only pushes frames to the next
        // vblank. Chunk updates stay bounded by the per-frame limit either way.
        d.limitFramerate = 1;
#else
        d.limitFramerate = 2;
#endif
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
        d.viewBobbing = true;
#else
        d.viewBobbing = false;
#endif
        d.fogOff = PLATFORM_PS2 != 0;
        d.brightness = (PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX) ? 1.0f : 0.0f;
        d.aoLevel = 0.0f;
#if PLATFORM_PC_LEGACY
        d.smoothFps = false;
#else
        d.smoothFps = true;
#endif
        d.autoSaveTicks = 40000;
        d.weather = false;
#if PLATFORM_PC_LEGACY
        d.sky = false;
        d.sunMoon = false;
        d.clouds = 3;
#else
        d.clouds = 1;
#endif
        d.stars = false;
        d.chunkUpdates = PLATFORM_MIN_RENDERER_UPDATES_PER_FRAME;
        d.chunkUpdatesDynamic = false;
        d.mipmapLevel = PLATFORM_DEFAULT_MIPMAP_LEVEL;
#endif
        return d;
    }();
    return defaults;
}
