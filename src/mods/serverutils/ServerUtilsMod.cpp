#include "mods/serverutils/ServerUtilsMod.h"
#include "mods/ModManager.h"

bool ServerUtilsMod::isActive()
{
    IMod *mod = ModManager::getInstance().getMod("serverutils");
    return mod != nullptr && mod->isEnabled();
}
