import os

gui_files = [
    'src/net/minecraft/src/GuiBetaOptions.cpp',
    'src/net/minecraft/src/GuiControls.cpp',
    'src/net/minecraft/src/GuiErrorScreen.cpp',
    'src/net/minecraft/src/GuiIngameMenu.cpp',
    'src/net/minecraft/src/GuiMainMenu.cpp',
    'src/net/minecraft/src/GuiOptiCraftOptions.cpp',
    'src/net/minecraft/src/GuiScreen.cpp',
    'src/net/minecraft/src/legacy/LegacyCreateWorldScreen.cpp',
    'src/net/minecraft/src/legacy/LegacyHeritageOptions.cpp',
    'src/net/minecraft/src/legacy/LegacyMenuHints.cpp',
    'src/net/minecraft/src/legacy/LegacyMenuNavigation.cpp',
    'src/net/minecraft/src/legacy/LegacyOptionsScreen.cpp',
    'src/net/minecraft/src/legacy/LegacyPlayGameScreen.cpp',
    'src/net/minecraft/src/legacy/LegacyVideoOptions.cpp',
    'src/net/minecraft/src/skin/GuiSkinSelector.cpp',
]

for gf in gui_files:
    if os.path.exists(gf):
        with open(gf, "r", encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
        for i, l in enumerate(lines, 1):
            if 'WII_PLATFORM' in l or 'PLATFORM_WII' in l:
                print(f"{gf}:{i}: {l.strip()}")
                # print 2 lines before and after
                start = max(0, i-3)
                end = min(len(lines), i+2)
                for j in range(start, end):
                    print(f"   {j+1}: {lines[j].rstrip()}")
