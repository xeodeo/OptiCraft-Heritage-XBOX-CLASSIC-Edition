#include "platform/LegacyControlPromptBackend.h"

#ifdef PS2_PLATFORM
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/KeyBinding.h"
#include "ps2/input/Ps2PadKeyCodes.h"

namespace
{
std::string bindingLabel(const KeyBinding *binding)
{
    if (binding == nullptr)
        return std::string();
    const char *name = ps2PadKeyName(binding->keyCode);
    if (name != nullptr)
        return name;
    return GameSettings::getKeyDisplayString(binding->keyCode);
}
}

std::string legacyControlPromptLabel(const GameSettings &settings, LegacyControlAction action)
{
    switch (action)
    {
    case LegacyControlAction::Inventory: return bindingLabel(settings.keyBindInventory);
    case LegacyControlAction::Crafting: return settings.legacyCrafting ? bindingLabel(settings.keyBindCrafting) : std::string();
    case LegacyControlAction::Drop: return bindingLabel(settings.keyBindDrop);
    case LegacyControlAction::Jump: return bindingLabel(settings.keyBindJump);
    case LegacyControlAction::Attack: return "R2";
    case LegacyControlAction::Use: return "L2";
    }
    return std::string();
}
#endif
