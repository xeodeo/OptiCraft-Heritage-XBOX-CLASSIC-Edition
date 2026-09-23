import os
import re

def process_file(path, replacements):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    new_content = content
    for pattern, repl in replacements:
        new_content = re.sub(pattern, repl, new_content, flags=re.MULTILINE)
        
    if new_content != content:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Modified {path}")
    else:
        print(f"No changes for {path}")

base = "src/net/minecraft/src/"

# 1. WorldRenderer.h & WorldRenderer.cpp
# isFullyInFrustum is under `#if PLATFORM_PC || PLATFORM_PS2`
# GL specific stuff is under `#if PLATFORM_PC`
# updateRenderer logic uses `!defined(XBOX_PLATFORM)` - wait, updateRenderer is in `WorldRenderer.cpp`.
# We want XBOX to act like PC.
# So `#if PLATFORM_PC` -> `#if PLATFORM_PC || defined(XBOX_PLATFORM)`
# And `#if PLATFORM_PC || PLATFORM_PS2` -> `#if PLATFORM_PC || PLATFORM_PS2 || defined(XBOX_PLATFORM)`

process_file(base + "WorldRenderer.h", [
    (r'^#if PLATFORM_PC$', '#if PLATFORM_PC || defined(XBOX_PLATFORM)'),
    (r'^#if PLATFORM_PC \|\| PLATFORM_PS2$', '#if PLATFORM_PC || PLATFORM_PS2 || defined(XBOX_PLATFORM)'),
    (r'^#if defined\(WII_PLATFORM\) \|\| defined\(PS2_PLATFORM\) \|\| defined\(XBOX_PLATFORM\)$', '#if defined(WII_PLATFORM) || defined(PS2_PLATFORM)'),
    (r'^#if defined\(PS2_PLATFORM\) \|\| defined\(WII_PLATFORM\) \|\| defined\(XBOX_PLATFORM\)$', '#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)'),
])

process_file(base + "WorldRenderer.cpp", [
    (r'^#if PLATFORM_PC$', '#if PLATFORM_PC || defined(XBOX_PLATFORM)'),
    (r'^#if PLATFORM_PC \|\| PLATFORM_PS2$', '#if PLATFORM_PC || PLATFORM_PS2 || defined(XBOX_PLATFORM)'),
    (r'^#if !defined\(PS2_PLATFORM\) && !defined\(WII_PLATFORM\) && !defined\(XBOX_PLATFORM\) && !PLATFORM_PC_LEGACY$', '#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !PLATFORM_PC_LEGACY'),
])

# 2. ModelRenderer.cpp
process_file(base + "ModelRenderer.cpp", [
    (r'^#if PLATFORM_PC$', '#if PLATFORM_PC || defined(XBOX_PLATFORM)'),
    (r'^#elif PLATFORM_PC$', '#elif PLATFORM_PC || defined(XBOX_PLATFORM)'),
])

# 3. GLAllocation.h & GLAllocation.cpp
process_file(base + "GLAllocation.h", [
    (r'^#if PLATFORM_PC$', '#if PLATFORM_PC || defined(XBOX_PLATFORM)'),
])
process_file(base + "GLAllocation.cpp", [
    (r'^#if PLATFORM_PC$', '#if PLATFORM_PC || defined(XBOX_PLATFORM)'),
])

# 4. RenderGlobal.h & RenderGlobal.cpp
# In RenderGlobal.h, we remove XBOX from console guards
process_file(base + "RenderGlobal.h", [
    (r'^#if defined\(PS2_PLATFORM\) \|\| defined\(WII_PLATFORM\) \|\| defined\(XBOX_PLATFORM\)$', '#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)'),
    (r'^#if defined\(WII_PLATFORM\) \|\| defined\(PS2_PLATFORM\) \|\| defined\(XBOX_PLATFORM\)$', '#if defined(WII_PLATFORM) || defined(PS2_PLATFORM)'),
])

process_file(base + "RenderGlobal.cpp", [
    (r'^#if PLATFORM_PC$', '#if PLATFORM_PC || defined(XBOX_PLATFORM)'),
    (r'^#if defined\(PS2_PLATFORM\) \|\| defined\(WII_PLATFORM\) \|\| defined\(XBOX_PLATFORM\)$', '#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)'),
    (r'^#if defined\(WII_PLATFORM\) \|\| defined\(PS2_PLATFORM\) \|\| defined\(XBOX_PLATFORM\)$', '#if defined(WII_PLATFORM) || defined(PS2_PLATFORM)'),
    (r'^#if !defined\(PS2_PLATFORM\) && !defined\(WII_PLATFORM\) && !defined\(XBOX_PLATFORM\)$', '#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM)'),
    (r'^#if !defined\(WII_PLATFORM\) && !defined\(PS2_PLATFORM\) && !defined\(XBOX_PLATFORM\)$', '#if !defined(WII_PLATFORM) && !defined(PS2_PLATFORM)'),
])
