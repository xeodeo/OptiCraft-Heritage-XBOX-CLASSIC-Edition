# ps3_toolchain.cmake — PSL1GHT / ps3toolchain (gcc 13 PPU) cross toolchain for the PlayStation 3.
#
# Why not the official Sony SDK: its PPU compilers (gcc 4.1.1 and SNC 360) predate C++11, and its
# ppu-lv2 ABI uses 4-byte pointers inside ELF64, which no modern gcc/clang can emit, so a modern
# compiler cannot be linked against Sony's static libraries the way the Xbox build links MSVC
# objects against the XDK. PSL1GHT reaches the same firmware libraries (the sprx modules in
# dev_flash) from an ordinary powerpc64 gcc. See ps3\buzon\01-PLAN-port-ps3.md.
#
# The toolchain runs on Linux (WSL). Resolution order for the SDK root:
#   1. -DPS3DEV=<path> on the CMake command line
#   2. $ENV{PS3DEV}
#   3. $ENV{HOME}/ps3dev, then /usr/local/ps3dev
# PSL1GHT itself is installed into the same prefix by ps3toolchain.

# CMake re-includes the toolchain file for every try_compile; keep it idempotent.
if(PS3_TOOLCHAIN_INCLUDED)
    return()
endif()
set(PS3_TOOLCHAIN_INCLUDED YES)

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_VERSION   1)
set(CMAKE_SYSTEM_PROCESSOR ppc64)

# Marks the build for CMakeLists.txt, which dispatches to cmake/ps3.cmake.
set(PLAYSTATION3 YES)

# --- SDK root -----------------------------------------------------------------
set(_ps3dev_candidates "")
if(DEFINED PS3DEV)
    list(APPEND _ps3dev_candidates "${PS3DEV}")
endif()
if(DEFINED ENV{PS3DEV})
    list(APPEND _ps3dev_candidates "$ENV{PS3DEV}")
endif()
list(APPEND _ps3dev_candidates "$ENV{HOME}/ps3dev" "/usr/local/ps3dev")
set(PS3DEV "")
foreach(_candidate IN LISTS _ps3dev_candidates)
    if(IS_DIRECTORY "${_candidate}/ppu/bin")
        set(PS3DEV "${_candidate}")
        break()
    endif()
endforeach()
if(PS3DEV STREQUAL "")
    message(FATAL_ERROR "PS3 toolchain not found. Build it with ps3\\tools\\construir_toolchain.sh "
                        "or pass -DPS3DEV=<path to ps3dev>.")
endif()
set(PS3DEV "${PS3DEV}" CACHE PATH "ps3toolchain install prefix (PPU/SPU gcc + PSL1GHT)" FORCE)
set(PSL1GHT "${PS3DEV}" CACHE PATH "PSL1GHT install prefix" FORCE)

# --- Compilers ----------------------------------------------------------------
set(CMAKE_C_COMPILER   "${PS3DEV}/ppu/bin/powerpc64-ps3-elf-gcc")
set(CMAKE_CXX_COMPILER "${PS3DEV}/ppu/bin/powerpc64-ps3-elf-g++")
set(CMAKE_ASM_COMPILER "${PS3DEV}/ppu/bin/powerpc64-ps3-elf-gcc")
set(CMAKE_AR           "${PS3DEV}/ppu/bin/powerpc64-ps3-elf-ar"     CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB       "${PS3DEV}/ppu/bin/powerpc64-ps3-elf-ranlib" CACHE FILEPATH "" FORCE)
set(CMAKE_STRIP        "${PS3DEV}/ppu/bin/powerpc64-ps3-elf-strip"  CACHE FILEPATH "" FORCE)

# The cross gcc cannot link a hosted executable without PSL1GHT's crt and libs.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Same machine flags as PSL1GHT's ppu_rules (MACHDEP). Never -ffast-math: it changes
# the terrain a seed generates.
set(CMAKE_C_FLAGS_INIT   "-mcpu=cell -mhard-float -fmodulo-sched -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cell -mhard-float -fmodulo-sched -ffunction-sections -fdata-sections -D_GLIBCXX11_USE_C99_STDIO")

set(CMAKE_FIND_ROOT_PATH "${PS3DEV}/ppu" "${PS3DEV}/portlibs/ppu")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
