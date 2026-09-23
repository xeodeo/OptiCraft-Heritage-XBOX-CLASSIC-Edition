# OptiCraft Heritage

OptiCraft Heritage is a heavily modified, clean-room C++ implementation of classic Minecraft-era gameplay designed around portability, low-end hardware, and console-specific optimization.

This repository is not intended to be a line-for-line source translation. The runtime, platform layers, rendering paths, input backends, storage systems, user interface, asset loading, memory policies, and console support have been extensively reworked for the needs of this project.

## Project goals

- Keep the implementation portable across desktop PC, PlayStation 2, and Nintendo Wii.
- Preserve the intended classic gameplay and visual behavior where practical while allowing platform-specific adaptations.
- Run on constrained hardware through aggressive memory, rendering, chunk, and asset-loading optimizations.
- Keep platform code isolated behind explicit backends instead of scattering host-specific logic through the game code.
- Maintain a debuggable and production-oriented C++17 codebase.

## Clean-room implementation

OptiCraft Heritage is developed as a clean-room implementation. The project code is independently implemented in C/C++ and is heavily modified around its own runtime and platform architecture.

The project does not rely on original proprietary game source code as part of its implementation. Compatibility-oriented behavior may be reproduced from observable behavior, documented formats, protocol behavior, and independently developed interfaces.

This project is not affiliated with, endorsed by, or sponsored by Mojang Studios or Microsoft.

## Supported targets

### PC

The desktop build uses SDL2, OpenGL, and the shared platform abstraction layer. A dedicated 32-bit legacy profile is available for older SSE2-class CPUs and legacy OpenGL hardware.

### PlayStation 2

The PS2 build uses a native platform backend with PS2SDK support, GS-specific rendering, console-aware memory policies, asynchronous asset loading, platform storage, controller input, and optional VU-assisted terrain paths.

The expected USB application directory is:

```text
mass:/OptiCraftHeritage/
```

### Nintendo Wii

The Wii build uses devkitPPC/libogc and a native GX rendering path. The Homebrew Channel layout remains:

```text
apps/OptiCraft/
```

### Original Xbox

The Xbox build targets the retail console (733 MHz Pentium III, 64 MB, NV2A) and runs both on hardware and in [xemu](https://xemu.app). It is compiled with Visual Studio 2022 (C++17, `/arch:SSE`) and linked against the Xbox Development Kit (XDK 5849).

It provides:

- a Direct3D 8 fixed-function renderer;
- XInput controller input;
- DirectSound audio on the MCPX APU (stereo, or Dolby Digital 5.1 from the options);
- saves on the title drive `T:` (`E:\TDATA\FFFF4F43`).

Launch `default.xbe` from a folder on the hard disk, with the `data/` folder next to it, or boot `OptiCraft.iso` in xemu.

## Source layout

```text
src/
  client/       Client-side shared code
  java/         Java compatibility/runtime helpers
  net/          Game implementation
  platform/     Shared platform interfaces and backend selection
  pc/           Desktop-specific implementation
  ps2/          PlayStation 2 implementation
  wii/          Nintendo Wii implementation
  util/         Shared utility code

cmake/          Toolchains, source selection, and platform build logic
external/       Third-party dependencies
```

Platform targets deliberately select one implementation for each public backend. This keeps PC, PS2, and Wii implementations from accidentally entering the same link target.

## Building

CMake 3.21 or newer is required. Presets are defined in `CMakePresets.json`.

### Desktop

```text
cmake --preset gcc-debug
cmake --build --preset gcc-debug
```

For a normal optimized build:

```text
cmake --preset gcc-release
cmake --build --preset gcc-release
```

### 32-bit / legacy PC

The CMake presets do not hardcode an MSYS2 installation path. On Windows, use:

```text
build_gcc32.bat legacy
```

The batch file owns the local MSYS2 installation path instead of exposing it through CMake. To use another installation without editing the project:

```bat
set OPTICRAFT_MSYS2_ROOT=D:\Tools\msys64
build_gcc32.bat legacy
```

The accepted modes are `debug`, `release`, and `legacy`.

### PlayStation 2

```text
cmake --preset ps2-release
cmake --build --preset ps2-release
```

Use `ps2-debug` for a debug build. Asset staging remains a separate step so large runtime data is not recopied after every link.

### Nintendo Wii

```text
cmake --preset wii-release
cmake --build --preset wii-release
```

Use `wii-debug` for a debug build and `wii-bringup` for the minimal hardware/toolchain bring-up target.

### Original Xbox

The Xbox build needs the Microsoft Xbox Development Kit. It is **not** included in this repository and must never be committed to it. The build was developed with **XDK 5849**, which is preserved in the Internet Archive collection <https://archive.org/details/xbox-sdks> (archive `2005-03 - 5849.6, 5849.17 - RecoveryEXE, SDK.7z`).

The XDK does not have to be installed; its installer expects Visual Studio .NET 2003 and Windows XP. The build only uses the XDK's headers, libraries and tools, taken straight from the installer's payload:

1. Download the archive above and unpack it with 7-Zip. It contains `XDKSetup5849.17.exe`.
2. Unpack the installer itself with 7-Zip into a folder of your choice:

   ```text
   7z x XDKSetup5849.17.exe -o<xdk>\sdk
   ```

   The XDK proper ends up in `<xdk>\sdk\XDK\xbox` (`bin\`, `include\`, `lib\`). That folder is all the build uses:
   - the XDK linker (`bin\vc71\Link.Exe`);
   - `imagebld.exe`;
   - the XAPI, Direct3D 8, DirectSound and XNet libraries.
3. Point `XBOX_XDK_ROOT` at that folder.

Tested with: Windows 10 Pro 22H2, **Visual Studio 2022 Community 17.14** (MSVC 14.44, workload *Desktop development with C++* with the x86 build tools), **Windows SDK 10.0.26100**, CMake 3.31 (bundled with VS2022), Ninja 1.13 (in the repository), XDK 5849, extract-xiso 2.7.1, xemu 0.8.136 and a softmodded retail console. The full list of versions and of the technologies used is in [docs/XBOX_PORT.md](docs/XBOX_PORT.md#tested-environment).

Other requirements:

- **Visual Studio 2022** with the C++ x86 tools and the Windows 10 SDK (for the static UCRT).
- **[extract-xiso](https://github.com/XboxDev/extract-xiso)** to pack the ISO. Put it on `PATH` or in `XBOX_TOOLS`, or pass `-DXBOX_EXTRACT_XISO=...`.
- **Game data** (`assets/`, `resources/`). It is not in the repository either. Copy it into `data/` at the repository root, which is ignored by git; the build stages it into the ISO automatically. A different location can be passed as `-DXBOX_DATA_DIR=...`.

```text
set XBOX_XDK_ROOT=C:\path\to\xdk\5849\sdk\XDK\xbox
cmake --preset xbox-release
cmake --build --preset xbox-release
```

The output lands in `bin/xbox/`:

- `iso/` holds `default.xbe` plus `data/`. Copy that folder to the console's hard disk.
- `OptiCraft.iso` holds the same content as an Xbox ISO, for xemu.

Optional cache variables:

| Variable | Meaning |
|----------|---------|
| `XBOX_DEPLOY_DIR` | Copies `OptiCraft.iso` there after every build (for example, an emulator ROM folder). |
| `MC_LOG_LEVEL` | `1` writes a log to `T:\debug.log` and to an in-memory ring readable from xemu's gdbstub. |
| `XBOX_NETLOG_HOST` | A PC's IPv4 address. Every log line is also sent over UDP (port 9999) to `scripts/xbox/tools/escuchar_log.py`, so a hang on the console can be followed live. |
| `XBOX_ENABLE_SOUND` | Enables DirectSound audio (on in the preset). |

`xbox-bringup` builds a small hardware/toolchain smoke test instead of the game.

The full write-up (requirements, setup, toolchain internals, every adaptation made to the shared code, debugging and known issues) is in [docs/XBOX_PORT.md](docs/XBOX_PORT.md).

#### How the hybrid toolchain works

The XDK's own compiler is Visual C++ 7.1 (2003), which cannot build C++17. So the build combines two toolchains:

- **Compile:** `cmake/xbox_toolchain.cmake` compiles with the VS2022 `cl.exe` for 32-bit x86 with `/arch:SSE`. The Pentium III has SSE but no SSE2.
- **Link:** the XDK's linker links against the modern static CRT (`libcmt`, `libcpmt`, `libucrt`), followed by the XDK libraries.

These pieces make that combination boot:

- `src/xbox/runtime/XboxEntry.cpp` does what the XDK's `xapi0` startup does: TLS setup, the main thread and XAPI init. It also runs the XDK libraries' static initializers, which the modern CRT's tables do not reach.
- `src/xbox/runtime/XboxCrtShim.cpp` implements, on top of XAPI and the kernel, the Win32 imports the modern CRT expects. `XboxImports.txt` generates the import thunks.
- `scripts/xbox/patch_pe_for_imagebld.ps1` marks the PE as an Xbox image and moves the CRT's TLS slot from `fs:[2Ch]` to the XDK's `fs:[04h]`. `imagebld` then produces `default.xbe`.
- The VS2022 CRT is prebuilt for SSE2 and, in a few places, uses it with no CPU check. xemu executes SSE2 anyway, but a real console faults on the first such instruction. Two fixes handle this:
  - `scripts/xbox/patch_sse2_moves.ps1` runs after linking. It rewrites the SSE2 data moves to SSE1 and replaces the one SSE2 conversion with an x87 stub. The build prints any SSE2 it leaves behind.
  - `src/xbox/runtime/XboxP3Math.c` supplies x87 versions of the SSE2-only math routines: `ldexp`, `scalbn`, `ceil`, `floor` and the float formatter's `log10`.

The `scripts/xbox/tools` folder has debugging helpers for xemu's gdbstub and for the network log; see its README.

## Development notes

OptiCraft Heritage contains substantial platform-specific changes compared with the behavior it reproduces. Examples include custom render backends, legacy UI work, low-memory chunk policies, console input layers, asset streaming, platform storage, audio backends, profiling, and console-specific performance tuning.

When changing shared systems, keep the platform abstraction boundary intact and avoid introducing PC-only assumptions into common code. Likewise, console-specific optimizations should remain behind platform policies or dedicated backends whenever possible.

## Third-party software

Third-party libraries are kept under `external/` and retain their respective licenses and notices. Review those licenses independently before redistributing binaries.
