#include "platform/ClientPlatformPolicy.h"

#include "net/minecraft/src/GameResources.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/RenderEngine.h"
#include "platform/Log.h"

#include <string>
#include "xbox/render/XboxD3D.h"
#include "xbox/system/XboxWritableRoot.h"

namespace ClientPlatformPolicy
{
int initialWidth()
{
    return 640;
}

int initialHeight()
{
    return 480;
}

std::string minecraftDirectory()
{
    // The install directory is the game disc (D:, read-only); worlds, options
    // and stats go to the title's writable drive instead (T:, or Z:).
    return std::string(XboxWritableRoot::get()) + "/.minecraft";
}

bool saveConverterUsesSavesSubdirectory()
{
    return true;
}

void applyGameSettingsDefaults(GameSettings*)
{
}

void preloadStartupTextures(RenderEngine*)
{
}

void releaseWorldEntryAssets(RenderEngine* renderEngine)
{
    if (renderEngine == nullptr)
        return;

    // Same as PS2: with 64 MB the menu-only textures (panorama faces, logo)
    // must not stay resident while the world allocates its chunks and
    // meshes. getTexture() reloads them when the main menu comes back.
    for (int face = 0; face < 6; ++face)
        renderEngine->releaseTexture("/title/bg/panorama" + std::to_string(face) + ".png");
    renderEngine->releaseTexture("/legacy/panorama.png");
    renderEngine->releaseTexture("/title/mclogo.png");
    renderEngine->clearDecodedTextureCache();
}

int panoramaSampleGrid()
{
    return 8;
}

void reportCrash(const std::string& description)
{
    MC_LOG_ERROR("crash", "%s\n", description.c_str());
    // Returning ends main(), and the Xbox then reboots the title, wiping the
    // in-memory log. Hold here on a red screen instead so the crash stays
    // visible and the log can still be read.
    for (;;)
    {
        if (g_pD3DDevice)
        {
            g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(160, 0, 0), 1.0f, 0);
            g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
        }
        Sleep(100);
    }
}
}
