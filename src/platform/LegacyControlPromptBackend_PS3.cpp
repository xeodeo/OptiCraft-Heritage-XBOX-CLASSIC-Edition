#include "platform/LegacyControlPromptBackend.h"

#ifdef PS3_PLATFORM
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/KeyBinding.h"

// DualShock 3 names for the same slots the Xbox build uses (Y/B/A/RT/LT/X).
std::string legacyControlPromptLabel(const GameSettings &, LegacyControlAction action)
{
    switch (action)
    {
    case LegacyControlAction::Inventory: return "Triangle";
    case LegacyControlAction::Drop: return "Circle";
    case LegacyControlAction::Jump: return "Cross";
    case LegacyControlAction::Attack: return "R2";
    case LegacyControlAction::Use: return "L2";
    case LegacyControlAction::Crafting: return "Square";
    }
    return std::string();
}
#endif
