$ErrorActionPreference = 'Stop'
$f2 = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\cmake\xbox.cmake"
$c = [IO.File]::ReadAllText($f2).Replace("`r`n", "`n")

$old = "    `"`${CMAKE_SOURCE_DIR}/src/pc/external/stb_vorbis.cpp`"`n    )"
$new = "    `"`${CMAKE_SOURCE_DIR}/src/pc/external/stb_vorbis.cpp`"`n        # The Xbox renders the world through the desktop display-list path.`n        `"`${CMAKE_SOURCE_DIR}/src/pc/minecraft/RenderList.cpp`"`n    )"
if (-not $c.Contains($old)) { throw "sources block not found" }
$c = $c.Replace($old, $new)

$old2 = "    `"`${XBOX_MSVC_LIB_DIR}/legacy_stdio_wide_specifiers.lib`""
$new2 = "    `"`${XBOX_MSVC_LIB_DIR}/legacy_stdio_wide_specifiers.lib`"`n    # POSIX spellings (open/close/rmdir...) used by zlib and save code.`n    `"`${XBOX_MSVC_LIB_DIR}/oldnames.lib`""
if (-not $c.Contains($old2)) { throw "libs block not found" }
$c = $c.Replace($old2, $new2)

[IO.File]::WriteAllText($f2, $c)
Write-Output "xbox.cmake: pc RenderList + oldnames.lib"
