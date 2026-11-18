$ErrorActionPreference = 'Stop'
$f = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\cmake\xbox_toolchain.cmake"
$c = [IO.File]::ReadAllText($f).Replace("`r`n", "`n")
$old = @'
# Headers: modern STL + UCRT first, then the Win32 "um/shared" headers for the
# handful of shared files that include <windows.h> (they only use the Win32
# subset XAPI or XboxCrtShim.cpp provide). XDK headers are added only to the
# Xbox backend sources that need them (see cmake/xbox.cmake).
set(XBOX_SYSTEM_INCLUDES
    "${XBOX_MSVC_ROOT}/include"
    "${XBOX_WINSDK_ROOT}/Include/${XBOX_WINSDK_VERSION}/ucrt"
    "${XBOX_WINSDK_ROOT}/Include/${XBOX_WINSDK_VERSION}/um"
    "${XBOX_WINSDK_ROOT}/Include/${XBOX_WINSDK_VERSION}/shared"
)
set(XBOX_UCRT_LIB_DIR "${XBOX_WINSDK_ROOT}/Lib/${XBOX_WINSDK_VERSION}/ucrt/x86")
set(XBOX_MSVC_LIB_DIR "${XBOX_MSVC_ROOT}/lib/x86")
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${XBOX_SYSTEM_INCLUDES})
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${XBOX_SYSTEM_INCLUDES})
'@.Replace("`r`n", "`n")
$new = @'
# Header search order is set per source in cmake/xbox.cmake:
#   every TU       : project dirs, then the modern STL + UCRT (XBOX_STL_INCLUDES)
#   Xbox backends  : + the XDK include dir, *after* the STL, because the XDK
#                    also ships its own 2003 STL/CRT headers that must not win
#   everything else: + the Win32 "um/shared" headers, for the few shared files
#                    that include <windows.h> (they only use the Win32 subset
#                    XAPI or XboxCrtShim.cpp provide). XTL.h and <windows.h>
#                    conflict, so no TU ever sees both.
set(XBOX_STL_INCLUDES
    "${XBOX_MSVC_ROOT}/include"
    "${XBOX_WINSDK_ROOT}/Include/${XBOX_WINSDK_VERSION}/ucrt"
)
set(XBOX_WIN32_INCLUDES
    "${XBOX_WINSDK_ROOT}/Include/${XBOX_WINSDK_VERSION}/um"
    "${XBOX_WINSDK_ROOT}/Include/${XBOX_WINSDK_VERSION}/shared"
)
set(XBOX_UCRT_LIB_DIR "${XBOX_WINSDK_ROOT}/Lib/${XBOX_WINSDK_VERSION}/ucrt/x86")
set(XBOX_MSVC_LIB_DIR "${XBOX_MSVC_ROOT}/lib/x86")
'@.Replace("`r`n", "`n")
if (-not $c.Contains($old)) { throw "toolchain block not found" }
[IO.File]::WriteAllText($f, $c.Replace($old, $new))

$f2 = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\cmake\xbox.cmake"
$c = [IO.File]::ReadAllText($f2).Replace("`r`n", "`n")
$old2 = @'
    "${CMAKE_SOURCE_DIR}/external/zlib/contrib/minizip"
)

# XDK headers (XTL.h, D3D8.h, DSound.h, XInput via XTL) conflict with the
# Win32 SDK's <windows.h>, so only Xbox backend sources get them. A backend TU
# must include <xtl.h> and must not include <windows.h>.
file(GLOB_RECURSE XBOX_XDK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/xbox/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/platform/*_XBOX.cpp"
    "${CMAKE_SOURCE_DIR}/src/platform/*/*_XBOX.cpp"
)
list(FILTER XBOX_XDK_SOURCES EXCLUDE REGEX "[/\\]xbox[/\\]runtime[/\\]")
if(XBOX_XDK_SOURCES)
    set_source_files_properties(${XBOX_XDK_SOURCES} PROPERTIES
        INCLUDE_DIRECTORIES "${XBOX_XDK_ROOT}/include")
endif()
'@.Replace("`r`n", "`n")
$new2 = @'
    "${CMAKE_SOURCE_DIR}/external/zlib/contrib/minizip"
    ${XBOX_STL_INCLUDES}
)

# XDK headers (XTL.h, D3D8.h, DSound.h, XInput via XTL) conflict with the
# Win32 SDK's <windows.h>, so only Xbox backend sources get them, appended
# after the modern STL (see xbox_toolchain.cmake). A backend TU includes
# <xtl.h> and never <windows.h>; every other TU gets the Win32 um/shared dirs.
set(XBOX_XDK_SOURCES ${XBOX_SOURCES})
list(FILTER XBOX_XDK_SOURCES INCLUDE REGEX "([/\\]src[/\\]xbox[/\\]|_XBOX\\.cpp$)")
list(FILTER XBOX_XDK_SOURCES EXCLUDE REGEX "[/\\]xbox[/\\]runtime[/\\]")
set(XBOX_WIN32_SOURCES ${XBOX_SOURCES})
list(FILTER XBOX_WIN32_SOURCES EXCLUDE REGEX "([/\\]src[/\\]xbox[/\\]|_XBOX\\.cpp$)")
if(XBOX_XDK_SOURCES)
    set_source_files_properties(${XBOX_XDK_SOURCES} PROPERTIES
        INCLUDE_DIRECTORIES "${XBOX_XDK_ROOT}/include")
endif()
if(XBOX_WIN32_SOURCES)
    set_source_files_properties(${XBOX_WIN32_SOURCES} PROPERTIES
        INCLUDE_DIRECTORIES "${XBOX_WIN32_INCLUDES}")
endif()
'@.Replace("`r`n", "`n")
if (-not $c.Contains($old2)) { throw "xbox.cmake block not found" }
[IO.File]::WriteAllText($f2, $c.Replace($old2, $new2))
Write-Output "include ordering updated"
