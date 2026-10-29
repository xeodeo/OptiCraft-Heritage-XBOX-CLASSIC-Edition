# ps3.cmake — PlayStation 3 (Cell PPU / RSX, PSL1GHT) build branch for OptiCraft.
#
# Included from the top of CMakeLists.txt when cmake/ps3_toolchain.cmake is active
# (it sets PLAYSTATION3), which then return()s so none of the desktop configuration
# runs. Structured like cmake/wii.cmake.
#
#   ps3-bringup
#       Builds only the RSX smoke test (src/ps3/tools/Ps3Bringup.cpp): a shader
#       triangle for five seconds, every step logged to /dev_hdd0/tmp. Proves the
#       compiler, the shaders, the signing and the packaging on the real console.
#
# Output: bin/ps3/<title id>/ shaped like /dev_hdd0/game/<title id>/ on the console
# (PARAM.SFO, ICON0.PNG, USRDIR/EBOOT.BIN). It runs under HEN: EBOOT.BIN is an NPDRM
# "free" SELF signed with scetool (the Sony SDK's fself is rejected with 80010009).
# Copy the folder over FTP, or point RPCS3 at USRDIR/EBOOT.BIN.

cmake_minimum_required(VERSION 3.21)

include(${CMAKE_SOURCE_DIR}/cmake/SourceSelection.cmake)

# --- PS3 options ----------------------------------------------------------------
option(PS3_BRINGUP "Build only the PS3 RSX smoke test instead of the game" OFF)
set(PS3_TITLE_ID "OPTC00001" CACHE STRING "9-character title id (folder under /dev_hdd0/game)")
set(PS3_TITLE "OptiCraft" CACHE STRING "Title shown in the XMB")
set(PS3_SCETOOL_DIR "$ENV{HOME}/ps3tools/scetool" CACHE PATH
    "Folder holding scetool and its data/ keys (see ps3\\tools\\firmar_npdrm.sh)")
set(MC_LOG_LEVEL "0" CACHE STRING "Unified diagnostic verbosity: 0=off, 1=info, 2=debug, 3=trace")
set_property(CACHE MC_LOG_LEVEL PROPERTY STRINGS 0 1 2 3)

set(PS3_CONTENT_ID "UP0001-${PS3_TITLE_ID}_00-0000000000000000")
set(PS3_PKG_ROOT "${CMAKE_SOURCE_DIR}/bin/ps3/${PS3_TITLE_ID}")

# --- Shaders ----------------------------------------------------------------------
# Compiles a Cg shader with PSL1GHT's cgcomp and embeds the result as a C array
# named <name>_vpo / <name>_fpo, with a header of the same name.
find_program(PS3_CGCOMP NAMES cgcomp HINTS "${PS3DEV}/bin" REQUIRED)
set(PS3_SHADER_OUT "${CMAKE_BINARY_DIR}/ps3_shaders")
file(MAKE_DIRECTORY "${PS3_SHADER_OUT}")

function(ps3_add_shader out_sources shader_path)
    get_filename_component(_name "${shader_path}" NAME_WE)
    get_filename_component(_ext "${shader_path}" EXT)
    if(_ext STREQUAL ".vcg")
        set(_kind "-v")
        set(_symbol "${_name}_vpo")
    elseif(_ext STREQUAL ".fcg")
        set(_kind "-f")
        set(_symbol "${_name}_fpo")
    else()
        message(FATAL_ERROR "ps3_add_shader: ${shader_path} is neither .vcg nor .fcg")
    endif()
    set(_bin "${PS3_SHADER_OUT}/${_symbol}.bin")
    set(_c   "${PS3_SHADER_OUT}/${_symbol}.c")
    set(_h   "${PS3_SHADER_OUT}/${_symbol}.h")
    add_custom_command(
        OUTPUT "${_c}" "${_h}"
        COMMAND "${PS3_CGCOMP}" ${_kind} "${shader_path}" "${_bin}"
        COMMAND ${CMAKE_COMMAND} -DINPUT=${_bin} -DSYMBOL=${_symbol} -DOUT_C=${_c} -DOUT_H=${_h}
                -P "${CMAKE_SOURCE_DIR}/cmake/ps3_embed_binary.cmake"
        DEPENDS "${shader_path}" "${CMAKE_SOURCE_DIR}/cmake/ps3_embed_binary.cmake"
        COMMENT "cgcomp ${_name}${_ext}"
        VERBATIM
    )
    set(${out_sources} ${${out_sources}} "${_c}" PARENT_SCOPE)
endfunction()

# --- Source selection ---------------------------------------------------------
if(PS3_BRINGUP)
    set(PS3_SOURCES "${CMAKE_SOURCE_DIR}/src/ps3/tools/Ps3Bringup.cpp")
    ps3_add_shader(PS3_SOURCES "${CMAKE_SOURCE_DIR}/src/ps3/shaders/bringup_color.vcg")
    ps3_add_shader(PS3_SOURCES "${CMAKE_SOURCE_DIR}/src/ps3/shaders/bringup_color.fcg")
    set(PS3_LIBS rsx gcm_sys sysutil rt lv2 m)
    message(STATUS "PS3 build: BRINGUP (RSX smoke test only)")
elseif(PS3_COMMON_PROBE)
    # Porting aid: compile only the platform-independent sources (no PS3 backends,
    # nothing linked) to see what the shared code needs from a new platform.
    mcbeta_collect_common_sources(PS3_PROBE_SOURCES)
    mcbeta_exclude_remote_stats_sources(PS3_PROBE_SOURCES)
    list(FILTER PS3_PROBE_SOURCES EXCLUDE REGEX
        "[/\\\\]platform[/\\\\][A-Za-z/]*_(PC|GL|GX_WII|GS_PS2|D3D8_XBOX|WII|PS2|XBOX)\\.cpp$")
    list(FILTER PS3_PROBE_SOURCES EXCLUDE REGEX "[/\\\\]java[/\\\\]JavaNetwork\\.cpp$")
    add_library(OptiCraftCommonProbe OBJECT ${PS3_PROBE_SOURCES})
    set_target_properties(OptiCraftCommonProbe PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS YES)
    target_compile_options(OptiCraftCommonProbe PRIVATE -O0 -fno-math-errno -fno-trapping-math)
    target_compile_definitions(OptiCraftCommonProbe PRIVATE "PS3_PLATFORM" MC_LOG_LEVEL=${MC_LOG_LEVEL})
    target_include_directories(OptiCraftCommonProbe PRIVATE
        "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/pc" "${CMAKE_SOURCE_DIR}/src/ps3"
        "${CMAKE_SOURCE_DIR}/external/stb" "${CMAKE_SOURCE_DIR}/external/zlib/contrib/minizip"
        "${PSL1GHT}/ppu/include" "${PS3DEV}/portlibs/ppu/include")
    message(STATUS "PS3 build: COMMON PROBE (${CMAKE_CURRENT_LIST_LINE}: shared sources only, no link)")
    return()
else()
    message(FATAL_ERROR "PS3 build: only -DPS3_BRINGUP=ON exists so far (ps3\\buzon\\01-PLAN-port-ps3.md, step 4)")
endif()

# --- Target -------------------------------------------------------------------
add_executable(OptiCraft ${PS3_SOURCES})
set_target_properties(OptiCraft PROPERTIES
    SUFFIX ".elf"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    # gnu++17, not c++17: PSL1GHT's newlib 1.20 hides snprintf/vsnprintf/vsscanf
    # under __STRICT_ANSI__, and libstdc++'s <cstdio> then fails to compile.
    CXX_EXTENSIONS YES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/ps3"
)

target_compile_options(OptiCraft PRIVATE
    $<$<CONFIG:Release>:-O2>
    # Same numerically-neutral relaxations as the Wii/PS2 builds; see cmake/wii.cmake.
    -fno-math-errno
    -fno-trapping-math
)

target_compile_definitions(OptiCraft PRIVATE
    "PS3_PLATFORM"
    MC_LOG_LEVEL=${MC_LOG_LEVEL}
)

target_include_directories(OptiCraft PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/ps3"
    "${PS3_SHADER_OUT}"
    "${PSL1GHT}/ppu/include"
    "${PSL1GHT}/ppu/include/simdmath"
)
target_link_directories(OptiCraft PRIVATE "${PSL1GHT}/ppu/lib" "${PS3DEV}/portlibs/ppu/lib")
target_link_libraries(OptiCraft ${PS3_LIBS})
target_link_options(OptiCraft PRIVATE
    -mcpu=cell -mhard-float
    "-Wl,-Map,${CMAKE_BINARY_DIR}/OptiCraft.map"
)

# --- Packaging ----------------------------------------------------------------
# Same steps as PSL1GHT's ppu_rules: strip, then sprxlinker (binds the firmware sprx
# imports -- signing the ELF without this step gives a black screen and an exit),
# then sign as an NPDRM SELF for HEN.
find_program(PS3_SPRXLINKER NAMES sprxlinker HINTS "${PS3DEV}/bin" REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(_stripped "${CMAKE_BINARY_DIR}/OptiCraft.stripped.elf")

add_custom_command(TARGET OptiCraft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PS3_PKG_ROOT}/USRDIR"
    COMMAND "${CMAKE_STRIP}" "$<TARGET_FILE:OptiCraft>" -o "${_stripped}"
    COMMAND "${PS3_SPRXLINKER}" "${_stripped}"
    COMMAND ./scetool --sce-type=SELF --compress-data=TRUE --skip-sections=FALSE --key-revision=04
            --self-auth-id=1010000001000003 --self-add-shdrs=TRUE --self-vendor-id=01000002
            --self-app-version=0001000000000000 --self-type=NPDRM --self-fw-version=0003004000000000
            --np-license-type=FREE --np-content-id=${PS3_CONTENT_ID} --np-app-type=EXEC
            --np-real-fname=EBOOT.BIN --encrypt "${_stripped}" "${PS3_PKG_ROOT}/USRDIR/EBOOT.BIN"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/ps3/make_sfo.py"
            "${PS3_PKG_ROOT}/PARAM.SFO" "${PS3_TITLE_ID}" "${PS3_TITLE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/scripts/ps3/ICON0.PNG"
            "${PS3_PKG_ROOT}/ICON0.PNG"
    WORKING_DIRECTORY "${PS3_SCETOOL_DIR}"
    COMMENT "Packaging ${PS3_PKG_ROOT} (sprxlinker + scetool NPDRM)"
    VERBATIM
)
