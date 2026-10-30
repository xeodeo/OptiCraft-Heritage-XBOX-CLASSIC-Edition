#!/bin/bash
# ps3_spu_build.sh -- builds the SPU task ELF (src/ps3/spu/tasks) with Sony's SPU
# toolchain (spu-lv2-gcc 4.1.1, a Windows .exe) from inside WSL.
#
#   ps3_spu_build.sh <out.elf> <sony sdk cell dir> <work dir under /mnt> <source>...
#
# Every path is a Linux path; the script hands the .exe Windows paths (wslpath -w).
# The driver cannot find its own cc1plus/as/crt when started from WSL, so the
# prefixes (-B), headers (-isystem) and libraries (-L) are given explicitly. The
# work dir must be on a Windows drive (/mnt/...): the .exe writes its objects
# there, then the ELF is copied to <out.elf>.
#
# Flags: -O2, C++03 (what 4J wrote the tasks for), _CONTENT_PACKAGE (no
# spu_printf: there is no PPU printf server), and never -ffast-math. This gcc
# does not contract a*b+c into dfma by itself; the noise code places its fused
# steps explicitly to match the PPU (see PerlinNoise/ImprovedNoise_SPU.cpp).
set -e
OUT="$1"; SDK="$2"; WORK="$3"; shift 3

W_SDK="$(wslpath -w "$SDK" | tr '\\' '/')"
H="$W_SDK/host-win32/spu"
GCC="$SDK/host-win32/spu/bin/spu-lv2-gcc.exe"
[ -x "$GCC" ] || { echo "ps3_spu_build: $GCC not found" >&2; exit 1; }

mkdir -p "$WORK"
cd "$WORK"
SRCS=()
INCS=()
for s in "$@"; do
    SRCS+=("$(wslpath -w "$s" | tr '\\' '/')")
    INCS+=("-I$(wslpath -w "$(dirname "$s")" | tr '\\' '/')")
done

COMMON=(
    -B"$H/libexec/gcc/spu-lv2/4.1.1/" -B"$H/lib/gcc/spu-lv2/4.1.1/" -B"$H/spu-lv2/bin/"
    --sysroot="$W_SDK/target/spu"
    -isystem "$H/lib/gcc/spu-lv2/4.1.1/include"
    -isystem "$W_SDK/target/spu/include"
    -isystem "$W_SDK/target/common/include"
    -L"$H/lib/gcc/spu-lv2/4.1.1" -L"$H/spu-lv2/lib" -L"$W_SDK/target/spu/lib"
)

rm -f spu_tasks.elf
"$GCC" "${COMMON[@]}" "${INCS[@]}" -O2 -Wall -D_CONTENT_PACKAGE -fno-exceptions -fno-rtti \
    "${SRCS[@]}" -o spu_tasks.elf -ldma
cp spu_tasks.elf "$OUT"
