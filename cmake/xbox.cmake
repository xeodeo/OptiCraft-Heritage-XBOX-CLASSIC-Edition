# xbox.cmake — original Xbox build branch for OptiCraft.
#
# Included from the top of CMakeLists.txt when cmake/xbox_toolchain.cmake is
# active (it sets XBOX), which then return()s so none of the desktop
# SDL2/glad/OpenGL configuration runs. Structured to mirror cmake/wii.cmake.
#
#   xbox-release    the full game
#   xbox-bringup    only src/xbox/tools/XboxBringup.cpp: proves the hybrid
#                   toolchain (C++17 runtime, SEH/C++ exceptions, RTTI, threads,
#                   thread_local, D3D8) boots, without pulling game code.
#
# How an Xbox image is produced:
#   1. VS2022 cl.exe compiles everything as x86 C++17 against the modern STL +
#      UCRT headers (see xbox_toolchain.cmake).
#   2. The XDK 7.1 linker links the objects with the VS2022 static CRT
#      (libcmt/libcpmt/libvcruntime/libucrt) *and* the XDK libraries. It links as
#      /SUBSYSTEM:WINDOWS because the modern CRT has 64-byte aligned sections
#      that the XBOX subsystem's 32-byte alignment rejects.
#   3. src/xbox/runtime/XboxCrtShim.cpp implements every Win32 import the modern
#      CRT needs that XAPI lacks; XboxImports.asm (generated from
#      XboxImports.txt) publishes the matching __imp__Name@N pointers.
#      XboxEntry.cpp replicates the XDK startup (TLS sizing, XAPI main thread).
#   4. scripts/xbox/patch_pe_for_imagebld.ps1 marks the PE as XBOX subsystem and
#      rewrites the modern compiler's TLS accesses (fs:[2Ch] -> fs:[04h]).
#   5. imagebld turns the PE into default.xbe; extract-xiso packs an ISO that
#      xemu (or a real console) boots.
#
# Output: bin/xbox/iso/default.xbe (+ data) and bin/xbox/OptiCraft.iso.

cmake_minimum_required(VERSION 3.21)

include(${CMAKE_SOURCE_DIR}/cmake/SourceSelection.cmake)
enable_language(ASM_MASM)

# --- Options ------------------------------------------------------------------
option(XBOX_BRINGUP "Build only the Xbox toolchain smoke test instead of the game" OFF)
option(XBOX_ENABLE_SOUND "Enable the DirectSound backend" OFF)
option(XBOX_ENABLE_NETWORK "Experimental: multiplayer over XNet TCP (Minecraft 1.2.5 servers)" OFF)
option(XBOX_AUTOPILOT "Test build: replay a scripted controller from D:/autopilot.txt (never deployed)" OFF)
option(XBOX_LIMIT_MEMORY "Limit the title to the retail 64 MB even on 128 MB dev kits / xemu" ON)
set(MC_LOG_LEVEL "0" CACHE STRING "Unified diagnostic verbosity: 0=off, 1=info, 2=debug, 3=trace")
set_property(CACHE MC_LOG_LEVEL PROPERTY STRINGS 0 1 2 3)
set(XBOX_TITLE_NAME "OptiCraft by xeodeo" CACHE STRING "Title name embedded in the XBE")
set(XBOX_TITLE_ID "0xFFFF4F43" CACHE STRING "Test title ID embedded in the XBE ('OC')")
set(XBOX_STACK_SIZE "0x40000" CACHE STRING "Main thread stack size")
set(XBOX_NETLOG_HOST "" CACHE STRING
    "Debug: also send every log line over UDP (port 9999) to this PC IPv4 address; empty = off")

find_program(XBOX_EXTRACT_XISO NAMES extract-xiso
    HINTS "$ENV{XBOX_TOOLS}" "${CMAKE_SOURCE_DIR}/../xbox-tools/artifacts")
if(NOT XBOX_EXTRACT_XISO)
    message(WARNING "Xbox build: extract-xiso not found (set XBOX_EXTRACT_XISO); no ISO will be packed.")
endif()

# Test builds (autopilot) go to their own folder and are never deployed.
if(XBOX_AUTOPILOT)
    set(XBOX_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/xbox-autotest")
else()
    set(XBOX_BIN_SUBDIR "xbox" CACHE STRING "Output folder under bin/ (xbox-public for the release preset)")
    set(XBOX_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/${XBOX_BIN_SUBDIR}")
endif()

# --- Runtime glue (always linked) ---------------------------------------------
set(XBOX_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/src/xbox/runtime")
set(XBOX_IMPORTS_ASM "${CMAKE_BINARY_DIR}/XboxImports.asm")
add_custom_command(
    OUTPUT "${XBOX_IMPORTS_ASM}"
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/xbox/gen_imports.ps1"
            -List "${XBOX_RUNTIME_DIR}/XboxImports.txt"
            -Out "${XBOX_IMPORTS_ASM}"
    DEPENDS "${XBOX_RUNTIME_DIR}/XboxImports.txt" "${CMAKE_SOURCE_DIR}/scripts/xbox/gen_imports.ps1"
    COMMENT "Generating Win32 import thunks for the Xbox CRT shim"
    VERBATIM
)
set(XBOX_RUNTIME_SOURCES
    "${XBOX_RUNTIME_DIR}/XboxEntry.cpp"
    "${XBOX_RUNTIME_DIR}/XboxCrtShim.cpp"
    "${XBOX_RUNTIME_DIR}/XboxP3Math.c"
    "${XBOX_IMPORTS_ASM}"
)
# Pentium III replacements for SSE2-only UCRT math: plain x87/integer code.
set_property(SOURCE "${XBOX_RUNTIME_DIR}/XboxP3Math.c" APPEND PROPERTY COMPILE_OPTIONS "/arch:IA32")

# --- Source selection ---------------------------------------------------------
if(XBOX_BRINGUP)
    set(XBOX_SOURCES "${CMAKE_SOURCE_DIR}/src/xbox/tools/XboxBringup.cpp")
    message(STATUS "Xbox build: BRINGUP (toolchain smoke test only)")
else()
    mcbeta_collect_platform_sources(XBOX_SOURCES xbox)
    # The runtime glue is added explicitly below; the smoke test is its own shape.
    mcbeta_exclude_sources(XBOX_SOURCES
        "[/\\]xbox[/\\]runtime[/\\]"
        "[/\\]xbox[/\\]tools[/\\]")
    list(APPEND XBOX_SOURCES
        "${CMAKE_SOURCE_DIR}/external/zlib/contrib/minizip/ioapi.c"
        "${CMAKE_SOURCE_DIR}/external/zlib/contrib/minizip/unzip.c"
        "${CMAKE_SOURCE_DIR}/src/pc/external/stb_vorbis.cpp"
        # The Xbox renders the world through the desktop display-list path.
        "${CMAKE_SOURCE_DIR}/src/pc/minecraft/RenderList.cpp"
        # Incremental terrain builder shared with the low-end PC profile.
        "${CMAKE_SOURCE_DIR}/src/pc/minecraft/WorldRendererPcLegacy.cpp"
        "${CMAKE_SOURCE_DIR}/src/pc/minecraft/RenderGlobalPcLegacyVisibility.cpp"
        "${CMAKE_SOURCE_DIR}/src/pc/render/PcLegacySectionCache.cpp"
        "${CMAKE_SOURCE_DIR}/src/pc/render/PcLegacyTerrainStaging.cpp"
        "${CMAKE_SOURCE_DIR}/src/pc/render/PcLegacyBlockRenderInfo.cpp"
        "${CMAKE_SOURCE_DIR}/src/pc/render/PcLegacyCubeMaterialInfo.cpp"
        "${CMAKE_SOURCE_DIR}/src/pc/render/PcLegacyMeshScheduler.cpp"
        "${CMAKE_SOURCE_DIR}/src/pc/render/PcLegacySectionVisibility.cpp"
        "${CMAKE_SOURCE_DIR}/src/pc/render/PcLegacyStaticTileEntityMesh.cpp"
    )
    file(GLOB XBOX_ZLIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/external/zlib/*.c")
    list(APPEND XBOX_SOURCES ${XBOX_ZLIB_SOURCES})
    # zlib's zconf.h is generated at configure time (the desktop build gets it
    # from external/zlib's own CMakeLists; consoles use a portlib instead).
    configure_file("${CMAKE_SOURCE_DIR}/external/zlib/zconf.h.cmakein"
                   "${CMAKE_BINARY_DIR}/zlib/zconf.h" @ONLY)
    # Stats stay local on consoles; the desktop synchronizer must not enter.
    mcbeta_exclude_remote_stats_sources(XBOX_SOURCES)
    # No network stack on the Xbox port (yet): drop the SDL_net backend.
    mcbeta_exclude_sources(XBOX_SOURCES "[/\\]java[/\\]JavaNetwork\\.cpp$")
    mcbeta_select_platform_backends(XBOX_SOURCES XBOX D3D8_XBOX XBOX)
    message(STATUS "Xbox build: FULL game sources")
endif()

# --- Target -------------------------------------------------------------------
add_executable(OptiCraft ${XBOX_SOURCES} ${XBOX_RUNTIME_SOURCES})
set_target_properties(OptiCraft PROPERTIES
    SUFFIX ".exe"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${XBOX_BIN_DIR}"
)

target_compile_options(OptiCraft PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:/arch:SSE>        # Pentium III: SSE, no SSE2
    $<$<COMPILE_LANGUAGE:C,CXX>:/O2>
    $<$<COMPILE_LANGUAGE:C,CXX>:/GS->             # no /GS cookies (no fastfail handler)
    $<$<COMPILE_LANGUAGE:C,CXX>:/Gy>
    $<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>
    $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
    $<$<COMPILE_LANGUAGE:CXX>:/GR>
    $<$<COMPILE_LANGUAGE:CXX>:/d2FH4->            # classic FH3 unwind tables
    $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
    # No /bigobj: the XDK 7.1 linker cannot read the extended COFF format.
    $<$<COMPILE_LANGUAGE:C,CXX>:/wd4996>
)

target_compile_definitions(OptiCraft PRIVATE
    "_XBOX"
    "XBOX_PLATFORM"
    "NDEBUG"
    "NOMINMAX"
    "_CRT_SECURE_NO_WARNINGS"
    $<$<NOT:$<BOOL:${XBOX_ENABLE_NETWORK}>>:NO_NETWORK>
    MC_LOG_LEVEL=${MC_LOG_LEVEL}
    XBOX_AUTOPILOT=$<BOOL:${XBOX_AUTOPILOT}>
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
    target_compile_definitions(OptiCraft PRIVATE "NO_SOUND")
endif()
if(XBOX_NETLOG_HOST)
    target_compile_definitions(OptiCraft PRIVATE "XBOX_NETLOG_HOST=\"${XBOX_NETLOG_HOST}\"")
endif()

target_include_directories(OptiCraft PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/pc"
    "${CMAKE_SOURCE_DIR}/src/xbox"
    "${CMAKE_SOURCE_DIR}/src/net/minecraft/src"
    "${CMAKE_SOURCE_DIR}/src/mods"
    "${CMAKE_SOURCE_DIR}/external/stb"
    "${CMAKE_SOURCE_DIR}/external/zlib"
    "${CMAKE_SOURCE_DIR}/external/zlib/contrib/minizip"
    "${CMAKE_BINARY_DIR}/zlib"
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
# Passed as /I compile options rather than INCLUDE_DIRECTORIES: CMake places
# source-level include directories *before* the target's, which would let the
# XDK's 2003 STL headers shadow the modern ones. Compile options come after.
set(_xbox_win32_include_flags "")
foreach(_dir IN LISTS XBOX_WIN32_INCLUDES)
    list(APPEND _xbox_win32_include_flags "/I${_dir}")
endforeach()
if(XBOX_XDK_SOURCES)
    set_source_files_properties(${XBOX_XDK_SOURCES} PROPERTIES
        COMPILE_OPTIONS "/I${XBOX_XDK_ROOT}/include")
endif()
if(XBOX_WIN32_SOURCES)
    set_source_files_properties(${XBOX_WIN32_SOURCES} PROPERTIES
        COMPILE_OPTIONS "${_xbox_win32_include_flags}")
endif()

# Java StrictMath/fdlibm must keep the exact operation ordering (see the
# desktop branch in CMakeLists.txt). The UCRT's legacy non-standard names
# (`extern double HUGE`, DOMAIN, ...) collide with fdlibm's own macros.
if(NOT XBOX_BRINGUP)
    set(XBOX_STRICT_MATH_SOURCES ${XBOX_SOURCES})
    list(FILTER XBOX_STRICT_MATH_SOURCES INCLUDE REGEX "([/\\]java[/\\]fdlibm[/\\].*\\.c|[/\\]java[/\\]StrictMathCompat\\.cpp)$")
    set_property(SOURCE ${XBOX_STRICT_MATH_SOURCES} APPEND PROPERTY COMPILE_OPTIONS "/fp:strict")
    set_property(SOURCE ${XBOX_STRICT_MATH_SOURCES} APPEND PROPERTY COMPILE_DEFINITIONS "_CRT_DECLARE_NONSTDC_NAMES=0")
endif()
# --- Link ---------------------------------------------------------------------
# Order matters: the modern CRT resolves first (its mainCRTStartup is the one
# XboxEntry calls), XAPI supplies Win32 services, xboxkrnl comes last.
set(XBOX_LINK_LIBS
    "${XBOX_MSVC_LIB_DIR}/libcmt.lib"
    "${XBOX_MSVC_LIB_DIR}/libcpmt.lib"
    "${XBOX_MSVC_LIB_DIR}/libvcruntime.lib"
    "${XBOX_UCRT_LIB_DIR}/libucrt.lib"
    "${XBOX_MSVC_LIB_DIR}/legacy_stdio_definitions.lib"
    "${XBOX_MSVC_LIB_DIR}/legacy_stdio_wide_specifiers.lib"
    # POSIX spellings (open/close/rmdir...) used by zlib and save code.
    "${XBOX_MSVC_LIB_DIR}/oldnames.lib"
    "${XBOX_XDK_ROOT}/lib/xapilib.lib"
    "${XBOX_XDK_ROOT}/lib/d3d8.lib"
    "${XBOX_XDK_ROOT}/lib/d3dx8.lib"
    "${XBOX_XDK_ROOT}/lib/xgraphics.lib"
    "${XBOX_XDK_ROOT}/lib/dsound.lib"
)
if(XBOX_NETLOG_HOST OR XBOX_ENABLE_NETWORK)
    # Devkit XNet: the only flavour that may talk to an untrusted host (a PC).
    list(APPEND XBOX_LINK_LIBS "${XBOX_XDK_ROOT}/lib/xnet.lib")
endif()
list(APPEND XBOX_LINK_LIBS "${XBOX_XDK_ROOT}/lib/xboxkrnl.lib")
string(REPLACE ";" "\" \"" _xbox_libs_quoted "${XBOX_LINK_LIBS}")
set(CMAKE_CXX_LINK_EXECUTABLE
    "\"${CMAKE_LINKER}\" /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:XboxEntry /INCREMENTAL:NO /OPT:REF /MAP:<TARGET>.map /OUT:<TARGET> <OBJECTS> \"${_xbox_libs_quoted}\"")

# --- Post-link: PE fixups, XBE, ISO ------------------------------------------
get_filename_component(_xbox_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
set(XBOX_DUMPBIN "${_xbox_cl_dir}/dumpbin.exe")
set(XBOX_ISO_DIR "${XBOX_BIN_DIR}/iso")
set(XBOX_XBE "${XBOX_ISO_DIR}/default.xbe")
set(XBOX_IMAGEBLD_FLAGS
    /NOLOGO /STACK:${XBOX_STACK_SIZE} /TESTID:${XBOX_TITLE_ID} /TESTREGION:0x80000007
    /TESTMEDIATYPES:0x400003FF "/TESTNAME:${XBOX_TITLE_NAME}" /NOLIBWARN
)

# Dashboard artwork: scripts/xbox/media/*.bmp (made from the game's own
# textures) packed to XPR by the XDK bundler and embedded by imagebld as the
# title image (launchers, dashboard) and the default save-game image.
set(XBOX_MEDIA_DIR "${CMAKE_SOURCE_DIR}/scripts/xbox/media")
set(XBOX_TITLE_IMAGE "${XBOX_BIN_DIR}/titleimage.xpr")
set(XBOX_SAVE_IMAGE "${XBOX_BIN_DIR}/saveimage.xpr")
if(EXISTS "${XBOX_MEDIA_DIR}/titleimage.rdf")
    add_custom_command(TARGET OptiCraft PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E make_directory "${XBOX_BIN_DIR}"
        COMMAND "${XBOX_XDK_ROOT}/bin/bundler.exe" titleimage.rdf -o "${XBOX_TITLE_IMAGE}" -q
        COMMAND "${XBOX_XDK_ROOT}/bin/bundler.exe" saveimage.rdf -o "${XBOX_SAVE_IMAGE}" -q
        WORKING_DIRECTORY "${XBOX_MEDIA_DIR}"
        COMMENT "bundler: title and save images"
        VERBATIM)
    list(APPEND XBOX_IMAGEBLD_FLAGS "/TITLEIMAGE:${XBOX_TITLE_IMAGE}" "/DEFAULTSAVEIMAGE:${XBOX_SAVE_IMAGE}")
endif()
if(XBOX_LIMIT_MEMORY)
    list(APPEND XBOX_IMAGEBLD_FLAGS /LIMITMEM)
endif()

# Game data (assets/, resources/) is not part of the repository. Put it in
# <repo>/data or pass -DXBOX_DATA_DIR=...; every build stages whatever changed
# into the ISO tree before the ISO is packed.
set(XBOX_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Folder with the game's assets/ and resources/")
set(_xbox_stage_data)
if(EXISTS "${XBOX_DATA_DIR}/assets")
    set(_xbox_stage_data
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                "${XBOX_DATA_DIR}/assets" "${XBOX_ISO_DIR}/data/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                "${XBOX_DATA_DIR}/resources" "${XBOX_ISO_DIR}/data/resources")
else()
    message(WARNING "Xbox build: no game data at XBOX_DATA_DIR='${XBOX_DATA_DIR}' "
                    "(expects assets/ and resources/); the ISO will not have data/.")
endif()

add_custom_command(TARGET OptiCraft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${XBOX_ISO_DIR}"
    ${_xbox_stage_data}
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/xbox/patch_pe_for_imagebld.ps1"
            -Path "$<TARGET_FILE:OptiCraft>"
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/xbox/patch_sse2_moves.ps1"
            -Path "$<TARGET_FILE:OptiCraft>" -Map "$<TARGET_FILE:OptiCraft>.map"
            -Dumpbin "${XBOX_DUMPBIN}"
    COMMAND "${XBOX_XDK_ROOT}/bin/imagebld.exe" ${XBOX_IMAGEBLD_FLAGS}
            "/IN:$<TARGET_FILE:OptiCraft>" "/MAP:$<TARGET_FILE:OptiCraft>.map" "/OUT:${XBOX_XBE}"
    # Same program under a name with "720" in it: runs at 1280x720 when the
    # dashboard allows it (src/xbox/system/XboxVideoMode.h).
    COMMAND ${CMAKE_COMMAND} -E copy "${XBOX_XBE}" "${XBOX_ISO_DIR}/OptiCraft_720p.xbe"
    # Folder thumbnail for XBMC-style dashboards.
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/scripts/xbox/media/cover.png" "${XBOX_ISO_DIR}/default.tbn"
    # Xbox port credit shown after the OptiProjects logo (StartupPresentation).
    COMMAND ${CMAKE_COMMAND} -E make_directory "${XBOX_ISO_DIR}/data/assets/legacy"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/scripts/xbox/media/logo3.png" "${XBOX_ISO_DIR}/data/assets/legacy/logo3.png"
    COMMENT "imagebld: ${XBOX_XBE}"
    VERBATIM
)

if(XBOX_EXTRACT_XISO)
    # extract-xiso mangles forward-slash absolute paths; give it native ones.
    file(TO_NATIVE_PATH "${XBOX_ISO_DIR}" _xbox_iso_dir_native)
    file(TO_NATIVE_PATH "${XBOX_BIN_DIR}/OptiCraft.iso" _xbox_iso_native)
    add_custom_command(TARGET OptiCraft POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -f "${XBOX_BIN_DIR}/OptiCraft.iso"
        COMMAND "${XBOX_EXTRACT_XISO}" -c "${_xbox_iso_dir_native}" "${_xbox_iso_native}"
        COMMENT "extract-xiso: bin/xbox/OptiCraft.iso"
        VERBATIM
    )
endif()

# Optional deploy: copy the finished ISO where the emulator/front end reads it
# (e.g. -DXBOX_DEPLOY_DIR=C:/RetroXeo/roms/xbox). Overwrites the previous copy.
set(XBOX_DEPLOY_DIR "" CACHE PATH "Folder the built OptiCraft.iso is copied to after each build")
if(XBOX_EXTRACT_XISO AND XBOX_DEPLOY_DIR AND NOT XBOX_AUTOPILOT)
    add_custom_command(TARGET OptiCraft POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${XBOX_BIN_DIR}/OptiCraft.iso" "${XBOX_DEPLOY_DIR}/OptiCraft.iso"
        COMMENT "Deploying OptiCraft.iso to ${XBOX_DEPLOY_DIR}"
        VERBATIM
    )
endif()
# Forces a full re-copy of the game data (the normal build only copies what
# changed):  cmake --build build/xbox-release --target xbox-data
add_custom_target(xbox-data
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${XBOX_DATA_DIR}/assets" "${XBOX_ISO_DIR}/data/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${XBOX_DATA_DIR}/resources" "${XBOX_ISO_DIR}/data/resources"
    COMMENT "Staging data/ into ${XBOX_ISO_DIR}/data"
    VERBATIM
)
