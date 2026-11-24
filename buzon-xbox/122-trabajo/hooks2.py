import sys, os
sys.path.insert(0, os.path.dirname(__file__))
root = sys.argv[1]

def edit(rel, pairs):
    p = os.path.join(root, rel)
    s = open(p, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    for old, new in pairs:
        o = old.replace('\n', nl); n = new.replace('\n', nl)
        c = s.count(o)
        if c != 1:
            print('FAIL', rel, c, repr(old[:80])); sys.exit(1)
        s = s.replace(o, n, 1)
    open(p, 'w', encoding='utf-8', newline='').write(s)
    print('ok', rel)

N = 'net/minecraft/src/'
edit(N+'XRayMode.h', [
 ('bool comboEnabled();\n',
  'bool comboEnabled();\n// Admin mode CaveFinder (Nodus): draw only stone so caves stand out.\nvoid setCaveMode(bool on);\nbool caveMode();\n'),
])
edit(N+'XRayMode.cpp', [
 ('bool s_comboEnabled = false;\n}', 'bool s_comboEnabled = false;\nbool s_cave = false;\n}'),
 ('bool active()\n{\n    return s_active && s_comboEnabled;\n}',
  'bool active()\n{\n    return (s_active && s_comboEnabled) || s_cave;\n}'),
 ('bool isVisibleBlock(int blockID)\n{\n',
  'bool isVisibleBlock(int blockID)\n{\n    if (s_cave && !(s_active && s_comboEnabled))\n        return blockID == 1; // stone\n'),
 ('bool comboEnabled()\n{',
  'void setCaveMode(bool on)\n{\n    if (s_cave == on)\n        return;\n    s_cave = on;\n    Minecraft *mc = Minecraft::getMinecraft();\n    if (mc != nullptr && mc->renderGlobal != nullptr)\n        mc->renderGlobal->markAllRenderersDirty();\n}\n\nbool caveMode()\n{\n    return s_cave;\n}\n\nbool comboEnabled()\n{'),
])
edit(N+'EntityFish.h', [
 ('\tint_t shake;\nprivate:', '\tint_t shake;\n\tint getTicksCatchable() const { return ticksCatchable; } // admin mode AutoFish\nprivate:'),
])
edit(N+'GuiEditSign.h', [
 ('\tvoid drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;\n',
  '\tvoid drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;\n\tTileEntitySign *getSign() const { return entitySign; } // admin mode AutoSign\n'),
])
edit('client/Minecraft.h', [
 ('private:\n    friend void LegacyStartup::run(Minecraft* minecraft);\n',
  'private:\n    friend void LegacyStartup::run(Minecraft* minecraft);\n    friend class AdminModeMod; // FastPlace / Timer modules\n'),
])
edit('xbox/input/XboxInput.h', [
 ('bool consumeVillageLocatorRequest();\n',
  'bool consumeVillageLocatorRequest();\n// Back + X combo per player: admin mode menu (encargo 122).\nbool consumeAdminMenuRequest(int playerIndex);\nvoid setAdminMenuRequest(int playerIndex);\n'),
])
edit('xbox/input/XboxInput.cpp', [
 ('bool s_villageLocatorRequest = false;\n', 'bool s_villageLocatorRequest = false;\nbool s_adminMenuRequest[2] = { false, false };\n'),
 ('\tconst bool villageEnabled = ModManager::getInstance().isModEnabled("villagelocator");\n',
  '\tconst bool villageEnabled = ModManager::getInstance().isModEnabled("villagelocator");\n\tconst bool adminEnabled = ModManager::getInstance().isModEnabled("adminmode");\n'),
 ('\telse if (villageEnabled && backHeld && (p.pressed & XBOX_PAD_Y))\n\t{\n\t\ts_villageLocatorRequest = true;\n\t\ts_backCombo = true;\n\t}\n',
  '\telse if (villageEnabled && backHeld && (p.pressed & XBOX_PAD_Y))\n\t{\n\t\ts_villageLocatorRequest = true;\n\t\ts_backCombo = true;\n\t}\n'
  '\telse if (adminEnabled && backHeld && (p.pressed & XBOX_PAD_X))\n\t{\n\t\ts_adminMenuRequest[0] = true;\n\t\ts_backCombo = true;\n\t}\n'),
 ('\tsetKey(XBOX_KEY_X, (p.held & XBOX_PAD_X) != 0);\n',
  '\t// Back + X is the admin menu combo: X must not also open the crafting screen.\n\tsetKey(XBOX_KEY_X, (p.held & XBOX_PAD_X) != 0 && !(adminEnabled && backHeld));\n'),
 ('bool consumeVillageLocatorRequest()\n{',
  'bool consumeAdminMenuRequest(int playerIndex)\n{\n\tconst int idx = (playerIndex == 1) ? 1 : 0;\n\tconst bool req = s_adminMenuRequest[idx];\n\ts_adminMenuRequest[idx] = false;\n\treturn req;\n}\n\nvoid setAdminMenuRequest(int playerIndex)\n{\n\ts_adminMenuRequest[(playerIndex == 1) ? 1 : 0] = true;\n}\n\nbool consumeVillageLocatorRequest()\n{'),
])
edit('client/XboxSplitScreen.cpp', [
 ('        else if (pressed & XBOX_PAD_DPAD_RIGHT) { XboxInput::setWaypointRequest(1, 4); s_p2BackCombo = true; }\n    }\n',
  '        else if (pressed & XBOX_PAD_DPAD_RIGHT) { XboxInput::setWaypointRequest(1, 4); s_p2BackCombo = true; }\n'
  '        else if ((pressed & XBOX_PAD_X) && ModManager::getInstance().isModEnabled("adminmode"))\n'
  '        {\n            XboxInput::setAdminMenuRequest(1);\n            s_p2BackCombo = true;\n        }\n    }\n'),
 ('    if ((pressed & XBOX_PAD_X) && mc->gameSettings->xboxStyleCrafting && mc->playerController != nullptr &&',
  '    if ((pressed & XBOX_PAD_X) && !backHeld && mc->gameSettings->xboxStyleCrafting && mc->playerController != nullptr &&'),
])
edit('mods/ModManager.cpp', [
 ('#include "serverutils/ServerUtilsMod.h"\n', '#include "serverutils/ServerUtilsMod.h"\n#include "adminmode/AdminModeMod.h"\n'),
 ('    if (getMod("serverutils") == nullptr)\n        registerMod(std::make_unique<ServerUtilsMod>());\n',
  '    if (getMod("serverutils") == nullptr)\n        registerMod(std::make_unique<ServerUtilsMod>());\n    if (getMod("adminmode") == nullptr)\n        registerMod(std::make_unique<AdminModeMod>());\n'),
])
