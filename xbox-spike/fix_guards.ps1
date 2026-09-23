$files = @(
    "src/net/minecraft/src/WorldRenderer.h",
    "src/net/minecraft/src/WorldRenderer.cpp",
    "src/net/minecraft/src/ModelRenderer.cpp",
    "src/net/minecraft/src/GLAllocation.h",
    "src/net/minecraft/src/GLAllocation.cpp",
    "src/net/minecraft/src/RenderGlobal.cpp"
)

foreach ($file in $files) {
    $content = Get-Content $file -Raw
    
    # 1. WorldRenderer.h & WorldRenderer.cpp - isFullyInFrustum
    $content = $content -replace '#if PLATFORM_PC \|\| PLATFORM_PS2', '#if PLATFORM_PC || PLATFORM_PS2 || defined(XBOX_PLATFORM)'
    
    # 2. WorldRenderer.h & WorldRenderer.cpp & RenderGlobal.cpp - GL specific things
    # Let's replace `#if PLATFORM_PC` with `#if PLATFORM_PC || defined(XBOX_PLATFORM)` except where it is already handled
    # It's safer to use regex replacement on exactly `#if PLATFORM_PC`
    $content = $content -replace '(?m)^#if PLATFORM_PC\s*$', '#if PLATFORM_PC || defined(XBOX_PLATFORM)'
    
    # 3. WorldRenderer.cpp updateRenderer exclusion: remove XBOX_PLATFORM
    $content = $content -replace '#if !defined\(PS2_PLATFORM\) && !defined\(WII_PLATFORM\) && !defined\(XBOX_PLATFORM\) && !PLATFORM_PC_LEGACY', '#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !PLATFORM_PC_LEGACY'

    # 4. RenderGlobal.cpp renderer selection exclusion:
    # "Un cambio mecánico previo añadió || defined(XBOX_PLATFORM) a guardas "PS2||WII" y ahora hay mezcla inconsistente"
    $content = $content -replace '(?m)^#if defined\(PS2_PLATFORM\) \|\| defined\(WII_PLATFORM\) \|\| defined\(XBOX_PLATFORM\)\s*$', '#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)'
    $content = $content -replace '(?m)^#if defined\(WII_PLATFORM\) \|\| defined\(PS2_PLATFORM\) \|\| defined\(XBOX_PLATFORM\)\s*$', '#if defined(WII_PLATFORM) || defined(PS2_PLATFORM)'

    # 5. But wait, skyMesh logic is inside #if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM) in RenderGlobal.cpp
    # Does XBOX support skyMesh? The task says: "No cambies PS2/Wii/PC." "Para Xbox elige el camino PC/GL (display lists vía RenderAPI), revirtiendo la parte Xbox de esas guardas donde fuerce el camino nativo de consola."
    # We should let XBOX use GL lists for sky.
    
    Set-Content -Path $file -Value $content
}
