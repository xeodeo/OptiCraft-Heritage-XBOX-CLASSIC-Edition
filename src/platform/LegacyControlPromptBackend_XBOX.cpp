#include "platform/LegacyControlPromptBackend.h"

#ifdef XBOX_PLATFORM
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/KeyBinding.h"

std::string legacyControlPromptLabel(const GameSettings &, LegacyControlAction action)
{
    switch (action)
    {
    case LegacyControlAction::Inventory: return "Y";
    case LegacyControlAction::Drop: return "B";
    case LegacyControlAction::Jump: return "A";
    case LegacyControlAction::Attack: return "RT";
    case LegacyControlAction::Use: return "LT";
    case LegacyControlAction::Crafting: return "X";
    }
    return std::string();
}
#endif
