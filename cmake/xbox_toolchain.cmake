# xbox_toolchain.cmake — original Xbox (Pentium III / NV2A) toolchain.
#
# Hybrid toolchain:
#   * compiler: Visual Studio 2022 cl.exe (x86), so the game stays C++17 and
#     uses the modern MSVC STL + UCRT;
#   * linker, libraries, imagebld: the Xbox XDK (5849), which provides XAPI,
#     D3D8, DirectSound and the XBE image builder.
#
# The XDK is not part of this repository. Point XBOX_XDK_ROOT (cache variable
# or environment variable) at the XDK's "xbox" directory, i.e. the folder that
# contains bin\imagebld.exe, bin\vc71\Link.Exe, include\XTL.h and lib\xapilib.lib.
#
# See cmake/xbox.cmake for how the pieces fit together at link time.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)
set(XBOX TRUE CACHE BOOL "Building for the original Xbox" FORCE)

# --- Locate the XDK ---------------------------------------------------------
if(NOT XBOX_XDK_ROOT AND DEFINED ENV{XBOX_XDK_ROOT})
    file(TO_CMAKE_PATH "$ENV{XBOX_XDK_ROOT}" XBOX_XDK_ROOT)
endif()
set(XBOX_XDK_ROOT "${XBOX_XDK_ROOT}" CACHE PATH "Path to the XDK 'xbox' directory (bin, include, lib)")
if(NOT EXISTS "${XBOX_XDK_ROOT}/bin/imagebld.exe" OR NOT EXISTS "${XBOX_XDK_ROOT}/lib/xapilib.lib")
    message(FATAL_ERROR
        "Xbox build: XBOX_XDK_ROOT='${XBOX_XDK_ROOT}' does not look like an XDK 'xbox' directory.\n"
        "Expected bin/imagebld.exe and lib/xapilib.lib under it. Pass -DXBOX_XDK_ROOT=... or set the env var.")
endif()

# --- Locate Visual Studio 2022's x86 compiler --------------------------------
if(NOT XBOX_MSVC_ROOT)
    file(TO_CMAKE_PATH "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe" _vswhere)
    if(EXISTS "${_vswhere}")
        execute_process(
            COMMAND "${_vswhere}" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
            OUTPUT_VARIABLE _vs_path OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(_vs_path)
            file(STRINGS "${_vs_path}/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" _vc_ver LIMIT_COUNT 1)
            string(STRIP "${_vc_ver}" _vc_ver)
            file(TO_CMAKE_PATH "${_vs_path}/VC/Tools/MSVC/${_vc_ver}" XBOX_MSVC_ROOT)
        endif()
    endif()
endif()
set(XBOX_MSVC_ROOT "${XBOX_MSVC_ROOT}" CACHE PATH "Path to VC/Tools/MSVC/<version> of Visual Studio 2022")
if(NOT EXISTS "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/cl.exe")
    message(FATAL_ERROR "Xbox build: cl.exe not found under XBOX_MSVC_ROOT='${XBOX_MSVC_ROOT}'. Install the VS2022 C++ x86 tools.")
endif()

# --- Locate the Windows SDK UCRT (headers + libucrt.lib) ----------------------
if(NOT XBOX_WINSDK_ROOT)
    file(TO_CMAKE_PATH "$ENV{ProgramFiles\(x86\)}/Windows Kits/10" XBOX_WINSDK_ROOT)
endif()
if(NOT XBOX_WINSDK_VERSION)
    file(GLOB _sdk_versions LIST_DIRECTORIES true "${XBOX_WINSDK_ROOT}/Include/10.*")
    list(SORT _sdk_versions)
    list(GET _sdk_versions -1 _sdk_latest)
    get_filename_component(XBOX_WINSDK_VERSION "${_sdk_latest}" NAME)
endif()
set(XBOX_WINSDK_ROOT "${XBOX_WINSDK_ROOT}" CACHE PATH "Windows 10 SDK root")
set(XBOX_WINSDK_VERSION "${XBOX_WINSDK_VERSION}" CACHE STRING "Windows 10 SDK version used for UCRT/um headers")

set(CMAKE_C_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/cl.exe")
set(CMAKE_CXX_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/cl.exe")
set(CMAKE_ASM_MASM_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/ml.exe")
set(CMAKE_LINKER "${XBOX_XDK_ROOT}/bin/vc71/Link.Exe")
set(CMAKE_AR "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/lib.exe")
# CMake's Windows platform module requires an RC compiler even though no .rc
# file is built; the SDK's rc.exe satisfies it. The manifest tool is unused.
set(CMAKE_RC_COMPILER "${XBOX_WINSDK_ROOT}/bin/${XBOX_WINSDK_VERSION}/x86/rc.exe")
set(CMAKE_MT "${XBOX_WINSDK_ROOT}/bin/${XBOX_WINSDK_VERSION}/x86/mt.exe")

# The XDK linker cannot link CMake's compiler probes against the Win32 SDK;
# only check that sources compile.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

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

# Never link the desktop import libraries (kernel32.lib etc.).
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)

# Static release CRT, matching libcmt/libcpmt/libucrt at link time.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
