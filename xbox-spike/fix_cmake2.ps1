$ErrorActionPreference = 'Stop'
$f2 = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\cmake\xbox.cmake"
$c = [IO.File]::ReadAllText($f2).Replace("`r`n", "`n")

$old = "    MC_LOG_LEVEL=`${MC_LOG_LEVEL}`n)`nif(NOT XBOX_ENABLE_SOUND)"
$new = @'
    MC_LOG_LEVEL=${MC_LOG_LEVEL}
    # With /arch:SSE (no SSE2) the UCRT evaluates floats in x87 precision and
    # typedefs float_t/double_t as long double, which clashes with the Java
    # primitive aliases in src/java/Type.h. _M_FP_FAST selects the plain
    # float/double typedefs; it changes no code generation (the /fp model is
    # still precise), only those typedefs, FLT_EVAL_METHOD and the float
    # min/max vectorization switch below.
    "_M_FP_FAST"
    # libcpmt's vectorized algorithms are SSE2+; keep the STL scalar on the P3.
    "_USE_STD_VECTOR_ALGORITHMS=0"
)
if(NOT XBOX_ENABLE_SOUND)
'@.Replace("`r`n", "`n")
if (-not $c.Contains($old)) { throw "definitions block not found" }
$c = $c.Replace($old, $new)

$old2 = "    file(GLOB XBOX_ZLIB_SOURCES CONFIGURE_DEPENDS `"`${CMAKE_SOURCE_DIR}/external/zlib/*.c`")`n    list(APPEND XBOX_SOURCES `${XBOX_ZLIB_SOURCES})"
$new2 = @'
    file(GLOB XBOX_ZLIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/external/zlib/*.c")
    list(APPEND XBOX_SOURCES ${XBOX_ZLIB_SOURCES})
    # zlib's zconf.h is generated at configure time (the desktop build gets it
    # from external/zlib's own CMakeLists; consoles use a portlib instead).
    configure_file("${CMAKE_SOURCE_DIR}/external/zlib/zconf.h.cmakein"
                   "${CMAKE_BINARY_DIR}/zlib/zconf.h" @ONLY)
'@.Replace("`r`n", "`n")
if (-not $c.Contains($old2)) { throw "zlib block not found" }
$c = $c.Replace($old2, $new2)

$old3 = "    `"`${CMAKE_SOURCE_DIR}/external/zlib/contrib/minizip`"`n    `${XBOX_STL_INCLUDES}"
$new3 = "    `"`${CMAKE_SOURCE_DIR}/external/zlib/contrib/minizip`"`n    `"`${CMAKE_BINARY_DIR}/zlib`"`n    `${XBOX_STL_INCLUDES}"
if (-not $c.Contains($old3)) { throw "include block not found" }
$c = $c.Replace($old3, $new3)

[IO.File]::WriteAllText($f2, $c)
Write-Output "xbox.cmake: _M_FP_FAST, scalar STL algorithms, zconf.h"
