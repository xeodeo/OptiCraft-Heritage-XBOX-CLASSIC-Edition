#include "platform/ClientPlatformPolicy.h"

#include "net/minecraft/src/GameResources.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/RenderEngine.h"
#include "platform/Log.h"
#include "ps3/system/Ps3Paths.h"
#include "ps3/system/Ps3Video.h"
#include "ps3/render/Ps3Rsx.h"

#include <string>

namespace ClientPlatformPolicy
{
int initialWidth()
{
    return Ps3Video::width();
}

int initialHeight()
{
    return Ps3Video::height();
}

std::string minecraftDirectory()
{
    // Worlds, options and stats next to the game, in the title's USRDIR.
    return std::string(Ps3Paths::writableDir()) + "/.minecraft";
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

void releaseWorldEntryAssets(RenderEngine*)
{
    // 213 MB: the menu textures can stay resident (unlike the Xbox's 64 MB).
}

int panoramaSampleGrid()
{
    return 8;
}

void reportCrash(const std::string& description)
{
    MC_LOG_ERROR("crash", "%s\n", description.c_str());
    // Returning would end main() and drop back to the XMB with the reason
    // gone. Hold on a red screen instead, so the crash is visible and the log
    // (TTY / UDP) keeps the description.
    Ps3Rsx::holdOnCrashScreen();
}
}
