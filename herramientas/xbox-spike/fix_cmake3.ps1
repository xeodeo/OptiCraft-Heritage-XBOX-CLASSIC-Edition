$ErrorActionPreference = 'Stop'
$f2 = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\cmake\xbox.cmake"
$c = [IO.File]::ReadAllText($f2).Replace("`r`n", "`n")

# 1. mods/ and the flat net/minecraft/src include dirs, as the PS2 build does.
$old = "    `"`${CMAKE_SOURCE_DIR}/src/xbox`"`n    `"`${CMAKE_SOURCE_DIR}/external/stb`""
$new = "    `"`${CMAKE_SOURCE_DIR}/src/xbox`"`n    `"`${CMAKE_SOURCE_DIR}/src/net/minecraft/src`"`n    `"`${CMAKE_SOURCE_DIR}/src/mods`"`n    `"`${CMAKE_SOURCE_DIR}/external/stb`""
if (-not $c.Contains($old)) { throw "include block not found" }
$c = $c.Replace($old, $new)

# 2. fdlibm / StrictMathCompat: strict FP like the desktop MSVC build, and no
#    UCRT non-standard names (its `extern double HUGE` collides with fdlibm's macro).
$anchor = "# --- Link ---------------------------------------------------------------------"
$block = @'
# Java StrictMath/fdlibm must keep the exact operation ordering (see the
# desktop branch in CMakeLists.txt). The UCRT's legacy non-standard names
# (`extern double HUGE`, DOMAIN, ...) collide with fdlibm's own macros.
if(NOT XBOX_BRINGUP)
    set(XBOX_STRICT_MATH_SOURCES ${XBOX_SOURCES})
    list(FILTER XBOX_STRICT_MATH_SOURCES INCLUDE REGEX "([/\\]java[/\\]fdlibm[/\\].*\\.c|[/\\]java[/\\]StrictMathCompat\\.cpp)$")
    set_property(SOURCE ${XBOX_STRICT_MATH_SOURCES} APPEND PROPERTY COMPILE_OPTIONS "/fp:strict")
    set_property(SOURCE ${XBOX_STRICT_MATH_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS "_CRT_DECLARE_NONSTDC_NAMES=0")
endif()

'@.Replace("`r`n", "`n")
if (-not $c.Contains($anchor)) { throw "link anchor not found" }
$c = $c.Replace($anchor, $block + $anchor)

[IO.File]::WriteAllText($f2, $c)
Write-Output "xbox.cmake: mods include dirs + strict fdlibm"
