#include "platform/LegacyControlPromptBackend.h"

#ifdef WII_PLATFORM
#include <wiiuse/wpad.h>

#include "net/minecraft/src/GameSettings.h"
#include "wii/input/WiiButtonBindings.h"
#include "wii/input/WiiInputDebug.h"
#include "wii/input/WiiPadKeyCodes.h"

namespace
{
WiiPadFamily activeFamily()
{
    const WiiInputDebugState &debug = wiiInputDebugState();
    if (debug.gcConnected)
        return WiiPadFamily::GameCube;
    if (debug.wmConnected && debug.expType == WPAD_EXP_CLASSIC)
        return WiiPadFamily::Classic;
    return WiiPadFamily::Wiimote;
}

const WiiButtonBindings::FamilySnapshot &familyBindings(
    const WiiButtonBindings::Snapshot &snapshot, WiiPadFamily family)
{
    switch (family)
    {
    case WiiPadFamily::GameCube: return snapshot.gameCube;
    case WiiPadFamily::Classic: return snapshot.classic;
    case WiiPadFamily::Wiimote: return snapshot.wiimote;
    }
    return snapshot.wiimote;
}

unsigned actionMask(const WiiButtonBindings::FamilySnapshot &bindings, LegacyControlAction action)
{
    switch (action)
    {
    case LegacyControlAction::Inventory: return bindings.inventory;
    case LegacyControlAction::Crafting: return bindings.inventory;
    case LegacyControlAction::Drop: return bindings.drop;
    case LegacyControlAction::Jump: return bindings.jump;
    case LegacyControlAction::Attack: return bindings.attack;
    case LegacyControlAction::Use: return bindings.use;
    }
    return 0;
}
}

std::string legacyControlPromptLabel(const GameSettings &settings, LegacyControlAction action)
{
    if (action == LegacyControlAction::Crafting && !settings.legacyCrafting)
        return std::string();
    const WiiButtonBindings::Snapshot &snapshot = WiiButtonBindings::get();
    const WiiPadFamily family = activeFamily();
    const WiiButtonBindings::FamilySnapshot &bindings = familyBindings(snapshot, family);
    const char *name = wiiPadButtonName(family, actionMask(bindings, action));
    return name != nullptr ? std::string(name) : std::string();
}
#endif
