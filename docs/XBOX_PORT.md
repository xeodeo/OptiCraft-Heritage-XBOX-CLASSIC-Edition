# OptiCraft Heritage on the original Xbox

This document describes the original Xbox port: what it runs on, how to build it, how the toolchain is put together, what was adapted in the shared code, and how to debug it on an emulator and on a real console.

The short build recipe is also in the main [README](../README.md#original-xbox). [Section 10](#10-how-the-port-was-done-step-by-step) tells how the port was done, problem by problem.

---

## 1. Status

- It boots and plays on a **real retail console** (softmodded, 64 MB, launched from a hard-disk folder) and in **[xemu](https://xemu.app)**.
- It is the full game: menus in the Legacy console UI, world generation, saving, the tutorial world, the skin selector and options.
- **Rendering:** Direct3D 8 fixed-function on the NV2A (textures, fog, blending, culling).
- **Performance:** 40-60 FPS at 2 chunks render distance on real hardware, with 20-35 MB free memory in a world.
- **Video:** `default.xbe` runs at 640x480; `OptiCraft_720p.xbe` (the same program under another name) runs at 1280x720 progressive when 720p is enabled in the dashboard (component cable).
- **Sound:** DirectSound on the MCPX audio processor. Music streams; effects are pitched and attenuated by distance. Output is stereo, or Dolby Digital 5.1 from the options.
- **Controller:** XInput. The first connected pad is player 1, on any port.
- **Saves** go to the title drive `T:`, which is `E:\TDATA\FFFF4F43` on the console.

Known limits are listed in [section 9](#9-known-issues-and-next-steps).

---

## 2. What you need

| Item | Where | Notes |
|------|-------|-------|
| **XDK 5849** (Microsoft Xbox Development Kit) | Internet Archive collection <https://archive.org/details/xbox-sdks>, archive `2005-03 - 5849.6, 5849.17 - RecoveryEXE, SDK.7z` | **Not in this repository, never commit it.** It is used without installing it (see below). |
| **Visual Studio 2022** | Microsoft | Workload "Desktop development with C++" with the **MSVC x86 tools** and a **Windows 10/11 SDK** (the static UCRT comes from it). The toolchain finds both automatically through `vswhere`. Developed with MSVC 14.44 and SDK 10.0.26100. |
| **CMake** + **Ninja** | CMake ships with VS2022; `ninja.exe` is in the repository root | The presets use `${sourceDir}/ninja.exe`. |
| **7-Zip** | 7-zip.org | Unpacks the XDK archive and its installer. |
| **extract-xiso** | <https://github.com/XboxDev/extract-xiso> | Packs the ISO. Put it on `PATH`, in `XBOX_TOOLS`, in `../xbox-tools/artifacts/`, or pass `-DXBOX_EXTRACT_XISO=<path>`. |
| **Game data** | Your own copy (`assets/` and `resources/`) | Not in the repository. Copy it to `<repo>/data/` (ignored by git), or pass `-DXBOX_DATA_DIR=...`. Each build copies whatever changed into the ISO tree. |
| xemu (optional) | <https://xemu.app> | Needs its usual BIOS/MCPX/HDD images. |
| Python 3 (optional) | python.org | For the debugging tools in `scripts/xbox/tools`. |

### Tested environment

The port was built and tested with exactly this setup. Other versions of Visual Studio 2022 and of the Windows SDK should work, but these are the ones known to work:

| Tool | Version used | Needed for |
|------|--------------|------------|
| Windows | 10 Pro 22H2 (x64) | Host OS |
| **Visual Studio 2022 Community** | 17.14 (build 17.14.37614) | Compiler and CRT |
| MSVC toolset | 14.44.35207 (v143) | `cl.exe`, `ml.exe`, `lib.exe` and `dumpbin.exe` (x86) |
| **Windows SDK** | 10.0.26100.0 | Static UCRT (`libucrt.lib`) and UCRT headers; `rc.exe` and `mt.exe` |
| CMake | 3.31.6 (the one bundled with VS2022) | Configure and build; needs 3.26 or newer for `copy_directory_if_different` |
| Ninja | 1.13.2 (`ninja.exe` in the repository) | Build tool used by the presets |
| **XDK** | 5849 (`XDKSetup5849.17.exe`, extracted, not installed) | Linker, `imagebld`, Xbox headers and libraries |
| 7-Zip | 26.02 | Extracting the XDK |
| extract-xiso | 2.7.1 | Packing `OptiCraft.iso` |
| Windows PowerShell | 5.1 (built into Windows) | Post-build scripts in `scripts/xbox/*.ps1` |
| xemu | 0.8.136 | Emulator tests |
| Python | 3.14 (optional) | Debugging tools in `scripts/xbox/tools` |
| Console | Retail original Xbox, softmodded, 64 MB | Hardware tests (game launched from a hard-disk folder) |

**Visual Studio Installer components** to select (workload *Desktop development with C++*):

- *MSVC v143 – VS 2022 C++ x64/x86 build tools (latest)*. This provides the 32-bit x86 compiler, MASM and `dumpbin`.
- *Windows 11 SDK (10.0.26100)*, or another Windows 10/11 SDK. The toolchain picks the newest installed one.
- *C++ CMake tools for Windows*. This provides CMake; a separate CMake 3.26+ also works.

No Visual Studio IDE project is used and no Developer Command Prompt is needed. `cmake/xbox_toolchain.cmake` finds VS2022 through `vswhere`, and the Windows SDK in `Program Files (x86)\Windows Kits\10`. Both can be overridden with `-DXBOX_MSVC_ROOT=...`, `-DXBOX_WINSDK_ROOT=...` and `-DXBOX_WINSDK_VERSION=...`.

### Technologies used

- **Compilation:**
  - C++17 with the VS2022 compiler (`cl.exe` x86, `/arch:SSE`).
  - The modern static C/C++ runtime (UCRT, `libcpmt`, `libvcruntime`).
  - MASM (`ml.exe`) for the generated import thunks.
- **Linking and image:** the XDK 5849 linker (`Link.Exe`, VC 7.1 era) and `imagebld` (PE to XBE).
- **XDK libraries:**
  - `xapilib`: Win32-like system API, threads, files, XInput.
  - `d3d8`, `d3dx8` and `xgraphics`: Direct3D 8 and texture swizzling.
  - `dsound`: DirectSound on the MCPX APU, including Dolby Digital.
  - `xnet`: sockets, only for the debug network log.
  - `xboxkrnl`: kernel.
- **Libraries already in the project** (`external/`, `src/java/fdlibm`):
  - `stb_vorbis` (Ogg Vorbis decoding) and `stb_image` (PNG);
  - zlib and minizip (saves, packs);
  - fdlibm (Java `StrictMath`).
- **Build scripts:**
  - CMake presets and toolchain file.
  - PowerShell scripts for the post-link steps: PE patch, SSE2 patch (uses `dumpbin` from the MSVC toolset), import generation.
- **Debugging:**
  - xemu's gdbstub with small Python clients.
  - UDP log receiver.
  - `T:\debug.log` read over FTP from a softmodded console.

### Getting the XDK without installing it

The XDK installer expects Windows XP and Visual Studio .NET 2003. The build needs neither: it only uses the XDK's headers, libraries and a few tools, and those sit inside the installer as plain files.

1. Download `2005-03 - 5849.6, 5849.17 - RecoveryEXE, SDK.7z` from the collection above and unpack it with 7-Zip. Inside is `XDKSetup5849.17.exe`.
2. Unpack that `.exe` with 7-Zip too. 7-Zip opens the installer's payload directly:

   ```text
   7z x XDKSetup5849.17.exe -oC:\xdk\5849\sdk
   ```

3. The folder that matters is `C:\xdk\5849\sdk\XDK\xbox`:

   ```text
   XDK\xbox\bin\vc71\Link.Exe   XDK linker (used for the final link)
   XDK\xbox\bin\imagebld.exe    turns the linked PE into default.xbe
   XDK\xbox\include\            xtl.h, d3d8.h, dsound.h, winsockx.h, ...
   XDK\xbox\lib\                xapilib, d3d8, d3dx8, xgraphics, dsound, xnet, xboxkrnl
   ```

   The other folders the installer carries (`VS71*`, `VS7*`, `NMDIR`, `SYSTEM_DIR`) are not used.
4. Tell the build where it is:

   ```text
   set XBOX_XDK_ROOT=C:\xdk\5849\sdk\XDK\xbox
   ```

---

## 3. Building

```text
set XBOX_XDK_ROOT=C:\xdk\5849\sdk\XDK\xbox
cmake --preset xbox-release
cmake --build --preset xbox-release
```

Run these from a normal command prompt. The toolchain file sets the compilers itself, so a Developer Prompt is not required.

**Output** (in `bin/xbox/`):

| Path | What |
|------|------|
| `iso/default.xbe` + `iso/data/` | The game as a folder: this is what goes on the console's hard disk. |
| `OptiCraft.iso` | The same content as an Xbox ISO (for xemu or ISO loaders). |
| `OptiCraft.exe`, `OptiCraft.exe.map` | The linked PE and linker map (for debugging; the map resolves crash addresses). |

**Cache variables** (`-DNAME=value` at configure time):

| Variable | Default | Meaning |
|----------|---------|---------|
| `XBOX_XDK_ROOT` | env `XBOX_XDK_ROOT` | The XDK `xbox` folder. |
| `XBOX_DATA_DIR` | `<repo>/data` | Folder with `assets/` and `resources/`. Staged into the ISO on every build (changed files only); `--target xbox-data` forces a full copy. |
| `XBOX_DEPLOY_DIR` | empty | Also copies `OptiCraft.iso` there after each build (for example an emulator ROM folder). |
| `XBOX_ENABLE_SOUND` | `ON` in the preset | DirectSound audio. `OFF` builds the silent stub. |
| `MC_LOG_LEVEL` | `0` | `1` enables the log: an in-memory ring, `T:\debug.log`, and the network log if set. |
| `XBOX_NETLOG_HOST` | empty | IPv4 address of a PC. Every log line is sent to it over UDP port 9999 (see [section 8](#8-debugging)). Links the devkit XNet library. |
| `XBOX_LIMIT_MEMORY` | `ON` | Limits the title to the retail 64 MB even on 128 MB kits and xemu. |
| `XBOX_AUTOPILOT` | `OFF` | Test builds only: replays a scripted controller from `D:\autopilot.txt`. Never deploy these. |
| `XBOX_TITLE_ID` | `0xFFFF4F43` | Test title ID in the XBE. It decides the `E:\TDATA\<id>` save folder. |

`cmake --preset xbox-bringup` builds `src/xbox/tools/XboxBringup.cpp` instead of the game. It is a color-staged smoke test of the runtime: C++17, exceptions, RTTI, threads, `thread_local` and D3D8.

---

## 4. Running

**xemu:** load `bin/xbox/OptiCraft.iso` as the DVD. For debugging, start it with `-gdb tcp:127.0.0.1:1235`.

**Real console** (softmodded, with FTP access):

1. Copy the **whole** `bin/xbox/iso` folder to the hard disk, for example `E:\Games\OptiCraft\`, so that `default.xbe` and `data\` sit side by side. The game reads its assets from `D:\data`, and `D:` is the folder the XBE was launched from.
2. Launch `default.xbe` from the dashboard.
3. Saves, `options.txt` and `debug.log` are written to `E:\TDATA\FFFF4F43\`. XAPI maps `T:` there for this title. If `T:` is not writable, the game mounts its utility drive `Z:` and uses that instead.

Launching a game unloads the dashboard, and its FTP server goes with it. To read logs while the game runs, use the network log.

**Settings:**
- **Video:** Includes "30 FPS Limit" (waits for an extra vblank, applied instantly) and "Show Coordinates" (displays X/Y/Z and orientation within the TV safe zone).
- **Music/Sound:** Includes a "Dolby Digital" checkbox to enable 5.1 surround sound.

**Controller:**

| In game | In menus |
|---------|----------|
| Left stick / D-pad: move | D-pad: navigate |
| Right stick: look | A: select |
| A: jump | B / Y: back |
| Y: inventory | X: delete the highlighted world (in the world list) |
| B: drop | Left/right: sliders, or switch between side-by-side buttons (Yes/Cancel) |
| Left stick click: sneak | |

---

## 5. How the toolchain works

The XDK's own compiler is Visual C++ 7.1 (2003). It cannot compile this C++17 code base, so the build is a **hybrid**:

```text
VS2022 cl.exe (C++17, /arch:SSE, x86)  -->  .obj
XDK 5849 Link.Exe  +  VS2022 static CRT  +  XDK libraries  -->  OptiCraft.exe (PE)
patch_pe_for_imagebld.ps1   subsystem -> XBOX, TLS slot fs:[2Ch] -> fs:[04h]
patch_sse2_moves.ps1        SSE2 -> SSE1 / x87 in the prebuilt CRT
imagebld.exe                PE -> default.xbe
extract-xiso                folder -> OptiCraft.iso
```

### 5.1 Compiler and headers (`cmake/xbox_toolchain.cmake`, `cmake/xbox.cmake`)

- **Compiler flags:** `cl.exe` from VS2022 (`Hostx64/x86`) with `/std:c++17 /arch:SSE /O2 /GS- /Gy /utf-8 /EHsc /GR /d2FH4- /Zc:__cplusplus`.
  - `/arch:SSE`: the Pentium III has SSE, not SSE2.
  - `/GS-`: there is no fast-fail handler.
  - `/d2FH4-`: keeps the classic exception tables.
  - **No `/bigobj`:** the 2003 linker cannot read that object format.
- **Include order:**
  1. project folders;
  2. the **VS2022 STL and UCRT** headers;
  3. the XDK or Win32 `um/shared` headers.

  This keeps the XDK's 2003-era STL from shadowing the modern one. The dirs are passed as `/I` compile options.
- **XDK headers are included only through `src/xbox/XboxXtl.h`**, never `<xtl.h>` or `<windows.h>` directly. It renames the XDK's `_Interlocked*` declarations so they do not clash with the modern intrinsics.
- **Defines:**
  - `_XBOX`, `XBOX_PLATFORM`, `NDEBUG`, `NOMINMAX`, `NO_NETWORK`.
  - `_M_FP_FAST`: with `/arch:SSE` the UCRT would make `float_t` a `long double`, which clashes with the Java type aliases. This define only changes those typedefs.
  - `_USE_STD_VECTOR_ALGORITHMS=0`: keeps the STL scalar.
- **fdlibm** (`src/java/fdlibm`) and `StrictMathCompat.cpp` build with `/fp:strict`, which keeps the Java `StrictMath` results exact.

### 5.2 Link

- **Link order:** `libcmt`, `libcpmt`, `libvcruntime`, `libucrt`, `legacy_stdio_*`, `oldnames`, then `xapilib`, `d3d8`, `d3dx8`, `xgraphics`, `dsound`, `xnet` (only with the network log), and `xboxkrnl`.
- **Linker options:** `/NODEFAULTLIB /ENTRY:XboxEntry /SUBSYSTEM:WINDOWS /MAP`.
- The image links as WINDOWS because the modern CRT has 64-byte aligned sections, which the XBOX subsystem rejects. The subsystem is patched afterwards.
- **imagebld options:** `/TESTID:0xFFFF4F43 /TESTREGION:0x80000007 /TESTMEDIATYPES:0x400003FF /STACK:0x40000 /LIMITMEM`.

### 5.3 Runtime glue (`src/xbox/runtime/`)

- **`XboxEntry.cpp`** does what the XDK's `xapi0` startup would do:
  - computes the TLS layout from `_tls_used`;
  - creates the main thread;
  - runs `XapiApplyKernelPatches` and `XapiInitProcess`, then the XAPI initializers (`.CRT$RIA..RIZ`);
  - calls the modern `mainCRTStartup`.

  It also runs the **XDK libraries' C++ static initializers**. DirectSound's memory counters, for example, live in *read-write* `.CRT$XCU` contributions. The modern CRT's table is read-only data, so the linker put the XDK entries in a separate `.CRT` section the CRT never walks. Without them, DirectSound crashes inside `DirectSoundCreate`. Two read-write markers (`.CRT$XCT_XDK`, `.CRT$XCV_XDK`) bracket that section, and the entry point calls every pointer between them.
- **`XboxCrtShim.cpp`** implements the Win32 API the modern CRT imports, on top of XAPI and the kernel:
  - critical sections, SRW locks, `InitOnce`, condition variables;
  - fibers-local storage mapped to TLS;
  - heaps (`Rtl*Heap`), time, wide-char file functions (converted to ANSI, `/` to `\`), locale and code-page stubs.

  It also contains a few overrides for things the Xbox does not have:
  - The SEH validation functions (`_ValidateEH3RN`, `__except_validate_*`) check the TIB stack bounds. On the Xbox, `fs:[4]` is not the stack base, and the check turned exceptions into double faults.
  - `__acrt_is_secure_process` and `__acrt_app_verifier_enabled` read the Win32 PEB.
- **`XboxImports.txt`** lists those functions. `scripts/xbox/gen_imports.ps1` generates `XboxImports.asm` from it, which publishes the `__imp__Name@N` pointers the CRT links against.
- **`XboxP3Math.c`** holds the x87 math replacements (see 5.5).

### 5.4 PE fixups (`scripts/xbox/patch_pe_for_imagebld.ps1`)

- Sets the subsystem to `IMAGE_SUBSYSTEM_XBOX` (14), which `imagebld` requires.
- Rewrites every `mov r32, fs:[2Ch]` to `fs:[04h]`. The modern compiler hard-codes the Win32 TEB slot for `thread_local` and thread-safe statics; XAPI keeps the TLS array at `fs:[04h]` (the XDK defines `__tls_array = 4`).

### 5.5 Pentium III vs. the VS2022 CRT (the "real console" problem)

The static CRT that ships with VS2022 is **prebuilt for SSE2**. Most of its SSE2 paths check the CPU first:

- the `*_pentium4` libm entry points;
- `memmove`, `memset`, `strchr`;
- the vectorized STL algorithms.

A handful use SSE2 without any check. **xemu executes SSE2 anyway** (it reports a Pentium III but does not enforce it). A **real Xbox faults on the first such instruction**: the title shows a black screen, and before the fixes the log stopped at the first `printf("%f")`.

Two fixes remove them:

- **`scripts/xbox/patch_sse2_moves.ps1`** runs after linking. It disassembles the image with `dumpbin` and uses the linker map to skip functions that dispatch on the CPU.
  - Every SSE2 *data move* (`movsd`/`movq`/`movlpd`/`movapd`/`movupd`/`movd`/`xorpd`...) becomes its same-length SSE1 twin: the prefix byte becomes a NOP, and the move becomes `movlps`/`movaps`/`movups`/`movss`.
  - The float formatter's `cvttsd2si r32,[ebp+disp]` becomes a `call` into an x87 stub. The script writes that stub into a code cave (`XboxSse2Cave`, 512 bytes of `int3` reserved in `XboxP3Math.c`).
  - The build prints every SSE2 instruction it leaves behind (see 9).
- **`src/xbox/runtime/XboxP3Math.c`** (built `/arch:IA32`) replaces CRT routines whose bodies do SSE2 arithmetic. These are linked ahead of `libucrt`, so the library objects are never pulled in:

  | Routine | Why |
  |---------|-----|
  | `ldexp`, `scalbn` | SSE2 bodies; `fdlibm` calls `scalbn`. |
  | `ceil`, `floor` | Without SSE2, the UCRT dispatches to `__ceil_default` / `__floor_default`, which are SSE2 too. |
  | `_libm_sse2_log10_precise` | `__acrt_fltout` (every `%f`/`%g`) calls it directly. |
  | `_87except` | x87 math error hook with an SSE2 body. |

- Three game call sites used `std::pow` (fog, the Legacy colour-grade LUT). On Xbox they use `JavaMath::pow` (fdlibm) instead, because the UCRT's `pow` helper is SSE2.

`src/xbox/main_xbox.cpp` also pins the CRT's CPU dispatch to "no SSE2" (`_set_SSE2_enable(0)`, `__isa_available`), so xemu and the console run the same code paths.

---

## 6. Xbox platform code

Everything Xbox-specific sits behind `PLATFORM_XBOX` / `XBOX_PLATFORM`. It lives in `src/xbox/` or in `src/platform/*_XBOX.cpp`, following the same backend pattern as PS2 and Wii (`mcbeta_select_platform_backends`).

| Area | Files | Notes |
|------|-------|-------|
| Entry, bootstrap | `src/xbox/main_xbox.cpp`, `system/XboxBootstrap.*` | CPU pinning, log file, numeric self-test, display, hand-off to `Minecraft::start`. |
| Renderer | `src/platform/RenderAPI_D3D8_XBOX.cpp`, `render/XboxD3D.*`, `lwjgl/Display_xbox.cpp` | Details below the table. |
| Input | `input/XboxPad.*`, `XboxInput.*`, `XboxPadKeyCodes.*`, `platform/InputBackend_XBOX.cpp`, `GameSettingsBackend_XBOX.cpp` | XInput on 4 ports; menu and gameplay mapping ported from the PS2 input mapper; default key bindings. |
| Storage | `storage/XboxFileSystem.cpp`, `platform/storage/StorageBackend_XBOX.cpp`, `java/File_xbox.cpp`, `system/XboxWritableRoot.*` | XAPI file I/O; assets read from `D:\data`; writable root `T:` with fallback `Z:`. |
| Audio | `platform/audio/SoundManager_XBOX.cpp` | Details below the table. |
| Settings/UI policy | `platform/ClientPlatformPolicy_XBOX.cpp`, `PlatformUserSettings_XBOX.cpp`, `LegacyControlPromptBackend_XBOX.cpp` | Minecraft folder on the writable root; releases the menu textures when a world starts; red crash screen instead of a reboot. |
| Tuning | `src/xbox/XboxTuning.h` | World and memory policy: bounded world, render distance 2, chunk cache radius 5, 40 live mobs, autosave every 1200 ticks, GUI scale 2. |
| Diagnostics | `system/XboxLogRing.cpp`, `XboxNetLog.cpp`, `XboxSelfTest.cpp`, `platform/Diagnostics_XBOX.cpp` | See section 8. |
| Stubs | `java/JavaNetwork_xbox.cpp`, `Runtime_xbox.cpp`, profiler/screenshot backends | No online play. |

**Renderer details:**

- The GL-1.x-style `RenderAPI` is emulated on Direct3D 8 fixed function.
- Matrices are kept in GL layout, with a z-range fix in the projection.
- Textures are resampled to powers of two, swizzled with `XGSwizzleRect` and uploaded lazily.
- Display lists are recorded as pre-converted 24-byte vertices.
- Geometry goes through `DrawVerticesUP`, batched at 1020 vertices; strips and fans are split safely, because oversized inline pushes break the NV2A command stream.
- Two texture units are tracked.

**Audio details:**

- Vorbis is decoded with `stb_vorbis`.
- Effects play from a 1 MB least-recently-used PCM cache through `SetBufferData`, with no second copy. There are 16 voices.
- Music streams through a 64 KB ring on a worker thread.
- Stereo, or Dolby Digital 5.1 through `DirectSoundOverrideSpeakerConfig`. DirectSound is re-created when the option changes.

### Performance work

These optimizations were implemented to make the game playable on a retail 64 MB console (data measured on real console, from the `xbox.perf` log):

| Change | Measured effect |
|---|---|
| Audio SFX in contiguous physical memory (`XPhysicalAlloc`, write-combine) | fixes noisy crash on real hardware |
| Redundant D3D state filtering | lower draw cost |
| Display lists in static vertex buffers (shared pools, fences) | 17 → ~43 FPS, opaque pass 19 → 6.5 ms |
| Static textures without RAM copy | ~6.7 → ~10.5 MB free |
| Chunk cache bounded by render distance | ~10.5 → ~25 MB free, ~50-60 FPS |
| Bounded pathfinding re-enabled (300 nodes, 3 per tick) | cheaper mob AI |
| Free chunk geometry when exiting to menu | ~31 MB free in the menu after playing |
| 512 KB vertex pools | less fragmentation |

- **Audio SFX in contiguous physical memory:** DirectSound effects previously crashed or produced loud noise on the real console because regular allocations were used. They are now allocated with `XPhysicalAlloc` as contiguous, write-combined memory, which the MCPX audio processor requires. (`src/platform/audio/SoundManager_XBOX.cpp`)
- **Redundant D3D state filtering:** The D3D8 fixed-function pipeline received many redundant state changes per frame. A software state filter now tracks the current render states and texture binds, dropping duplicate calls before they reach the GPU. (`src/platform/RenderAPI_D3D8_XBOX.cpp`)
- **Display lists in static vertex buffers:** Pushing vertices every frame via `DrawVerticesUP` was too slow. Display lists are now compiled into static vertex buffers, managed via shared memory pools and GPU fences, significantly reducing CPU overhead per frame. (`src/platform/RenderAPI_D3D8_XBOX.cpp`, `src/net/minecraft/src/RenderGlobal.cpp`)
- **Static textures without RAM copy:** The game used to keep a CPU copy of texture data after uploading it to the GPU. This copy is now freed for static textures, saving several megabytes of main memory. (`src/platform/RenderAPI_D3D8_XBOX.cpp`)
- **Chunk cache bounded by render distance:** Chunks outside the visible area were kept loaded, wasting memory. The cache is now tightly constrained to the active render distance, freeing up significant memory and allowing the game to run at ~50-60 FPS at a distance of 2. (`src/net/minecraft/src/ChunkProvider*.cpp`, `src/xbox/XboxTuning.h`)
- **Bounded pathfinding re-enabled:** Unbounded pathfinding could freeze the game by generating huge node vectors. It was modified to evaluate a maximum of 300 nodes, processed across multiple ticks (3 per tick), keeping mob AI functional without stalling the main thread. (`src/xbox/XboxTuning.h`)
- **Free chunk geometry when exiting to menu:** Returning to the main menu left chunk geometry in memory. A cleanup step now explicitly frees these vertex buffers when leaving a world, ensuring a stable memory baseline before the next session.
- **512 KB vertex pools:** Small, individual allocations for vertex buffers caused severe memory fragmentation over time. They are now sub-allocated from large 512 KB pools, which keeps the memory layout clean and predictable.

Second round (all behaviour-exact, measured with the `xbox.perf`, `xbox.draw` and `xbox.mesh` log lines):

| Change | Effect |
|---|---|
| Compact vertices: `SHORT4` position at 1/1024 block, `D3DCOLOR`, `NORMSHORT2` UV (16 bytes instead of 24; 12 without colour) | chunk geometry ~15 → ~6-10 MB |
| Entity models and items in pooled vertex buffers, colour from `D3DRS_TEXTUREFACTOR` | entity pass 4.2 → 2-3 ms |
| Menu textures released in game, no CPU copy of static non-power-of-two textures | textures in game 7.4 → 2.3 MB |
| Incremental, time-sliced chunk builder of the low-end PC profile (`PLATFORM_INCREMENTAL_TERRAIN_BUILD`) plus section visibility culling | meshing spikes 20 → ~8 ms |
| Tessellator keeps quads (`D3DPT_QUADLIST`), staging buffer 8 MB → 256 KB | a third fewer terrain vertices, ~38 MB free in xemu |
| Animated tiles re-swizzle the atlas once per frame, not once per tile | fewer full-atlas locks per tick |
| Hash set for `World::isLoadedEntityPointer`, fast/early block collisions, cached entity and chunk queries | cheaper entity ticking |

Third round (hitches):

| Change | Effect |
|---|---|
| OptiFine "Smooth FPS" no longer calls `BlockUntilIdle` (`XBOX_SMOOTH_FPS_WAITS_FOR_GPU 0`) | opaque pass 10.5 → 1.6 ms; CPU and GPU overlap again |
| Incremental chunk generation (the PS2 generator), only the 3x3 columns around the player generate on demand | no more 25-40 ms generation frames while exploring |
| `T:\debug.log` off (`XBOX_DISK_LOG 0`): McLog reopened the file after every line | fewer hard-disk writes during play; the network log carries the same lines |
| `xbox.spike` log line: every frame over 45 ms with its breakdown | finds the cause of each hitch |

---

## 7. Changes to shared code

About 95 shared files were touched. Most changes add `PLATFORM_XBOX` to existing console guards (`PS2 || WII` becomes `PS2 || WII || XBOX`). The ones with behaviour changes:

| Change | Files | Why |
|--------|-------|-----|
| `PLATFORM_XBOX` in the platform config and in console UI, input, storage and render policies | `platform/PlatformConfig.h`, `PlatformCompat.h`, `ConsoleInputClock.h`, `tuning/*.h`, many `Gui*` / `legacy/*` | The Xbox uses the console (Legacy) UI and controller paths. |
| **fdlibm little-endian detection** also recognises `_M_IX86` / `_M_X64` | `java/fdlibm/fdlibm.h` | MSVC x86 was treated as big-endian: `StrictMath` sin/sqrt returned garbage (NaN/-inf). That caused falling through the floor, spawning at bedrock and hangs. |
| **Lighting queue capped** at 16384 jobs on Xbox | `tuning/PlatformGameTuning.h` | The unbounded queue kept doubling its vector until one allocation failed mid-generation: `bad_alloc`, then back to the menu. |
| **Virtual keyboard releases focus on every screen change** | `VirtualKeyboard.*`, `client/Minecraft.cpp` | Screens are freed late, so a field left focused kept the pad "owned" by an invisible keyboard. |
| **Create-world screen ignores the stale pointer** on Xbox, as on PS2 | `legacy/LegacyCreateWorldScreen.cpp` | After leaving a world the old cursor position sat on a button. That closed the keyboard and blocked the D-pad. |
| **Delete world** with X in the Legacy world list, with confirmation | `legacy/LegacyPlayGameScreen.*`, `GuiSelectWorld.*` (new `promptDeleteWorld`) | The console world list had no delete option on any platform. |
| **Left/right step between buttons** when nothing adjustable is selected (Xbox) | `GuiScreen.cpp` | Needed to reach "Cancel" in Yes/No dialogs. |
| **Skin fallback** to `/mob/char.png` when the selected skin image is not in the data | `skin/SkinManager.cpp` | The skin images are packed by `scripts/update_skins_assets.py`; data sets without them rendered no player. |
| **Dolby Digital option** (`dolbyDigital` in `options.txt`, Xbox-only checkbox in the Music/Sound screen) | `GameSettings.*`, `GameSettingsPersistence.cpp`, `legacy/LegacyViewOptions.*` | Not every TV or receiver decodes AC-3. |
| `pow` through fdlibm on Xbox | `EntityRenderer.cpp`, `legacy/LegacyLook.cpp`, `legacy/LegacyColorGradePolicy.h` | See 5.5. |
| Display lists and GL allocation also compiled for Xbox | `RenderList.h`, `GLAllocation.*`, `WorldRenderer.*`, `RenderGlobal.*`, `ModelRenderer.cpp`, `RenderAPI.h` | The Xbox renderer uses the desktop display-list path. |
| Log hook for the ring and UDP copies | `platform/Log.cpp` | Diagnostics. |

---

## 8. Debugging

- **In-memory log** (`MC_LOG_LEVEL=1`):
  - Every log line also goes to `g_xboxLogRing` (64 KB).
  - With xemu started with `-gdb tcp:127.0.0.1:1235`, `python scripts/xbox/tools/gdblog.py bin/xbox/OptiCraft.exe.map` prints it.
  - If the CPU sits in a kernel bug check, the script decodes it: `eax=1E`, `ecx`=exception code, `edx`=faulting address.
  - **XBE address = map address − `0x3F0000`.**
- **`T:\debug.log`:** the same log written to the console's hard disk. It is committed line by line, so a hang still names its last step. Read it over FTP from `E:\TDATA\FFFF4F43\debug.log` after powering off.
- **Network log (live, on real hardware):**
  - Configure with `-DXBOX_NETLOG_HOST=<PC IPv4>`.
  - Run `python scripts/xbox/tools/escuchar_log.py` on that PC. Allow Python through the Windows firewall on private networks.
  - The console takes its IP from the dashboard's network settings. Every line arrives as a UDP datagram and is also saved to `netlog.txt`.
  - This is how the real-console boot failures were found. The log stopped at the first `%f`, and later at `ceil`.
- **Crash screen:** an unexpected C++ exception logs `crash: <what()>` and holds a red screen. Without it the title would exit and the Xbox would reboot it, wiping the in-memory log.
- **Performance reporting:** Every 5 seconds, an `xbox.perf` report logs FPS, present time, render phases, ticks, lighting, and chunk load/save times. An `xbox.mem` report logs free memory, display lists, `vbPools` size, and texture memory.
- **Out of memory** logs `out of memory: free=<KB>` and returns to the menu.
- **Other gdb tools** in `scripts/xbox/tools`:
  - `gdbthrow.py` catches every C++ throw.
  - `gdbstack.py` resolves the call chain at a bug check.
  - `gdbstep.py` and `gdbring.py` break at addresses.
  - `autotest.ps1` + `autopilot.txt` run a scripted controller in xemu.
- **Self-test:** `XboxSelfTest.cpp` logs known values at boot: `java.util.Random` sequences, 64-bit math, `floor`, `sqrt`, `sin`, `MathHelper`, and float formatting step by step. A toolchain or CPU problem shows up there first.

---

## 9. Known issues and next steps

- **Memory** is no longer the limit at 2 chunks (20-35 MB free). Remaining candidates:
  - keep fewer chunks resident and lean on the hard disk (`T:` or the `Z:` utility drive) for evicted chunks;
  - DXT-compressed textures.
- **Remaining hitches** (about one frame over 45 ms per second on the console), in the order they will be tackled:
  1. Name the tick phases in the Xbox profiler (nested phases are currently summed twice).
  2. Chunk streaming: load saved chunks through a budgeted queue instead of synchronously; defer/slice population (the real cost of the `gen` spikes); split the remaining atomic generation steps (base terrain noise, chunk build, skylight).
  3. World tick: the low-end PC tick scheduler, fewer allocations in entity queries, cheaper mob-spawn attempts.
  4. HUD and render: the font looks up every glyph by rebuilding a 220-character table; status bars, model faces and the sky dome are one draw each; animated tiles re-swizzle the whole atlas.
  5. Hardware: triple buffering, `_mm_prefetch` in meshing/generation loops.
- **Leftover SSE2** reported by the build, in paths not expected to run:
  - wide/money `num_put`;
  - `frexp` (iostream float output);
  - `__powhlp` (special cases of the CRT `pow`, which the game no longer calls);
  - `__handle_exc` (math error reporting).

  If a real-console crash points at one of them, replace it like the others.
- **Skins:** the skin images are not part of every data set; missing ones fall back to Steve.
- **No online or multiplayer.**

---

## 10. How the port was done, step by step

This is the bring-up log in chronological order. Each item gives the symptom that was seen, the cause behind it, and the fix that went into the code. The scripts, logs and screenshots of those sessions are kept in [`xbox-spike/`](../xbox-spike).

### Phase 1: can a modern C++17 toolchain produce a bootable XBE?

The first attempt was to use the XDK's own compiler, Visual C++ 7.1. It cannot compile the C++17 engine, and rewriting the engine for C++03 was not an option. The goal became to compile with VS2022 and borrow only the XDK's linker, libraries and `imagebld`.

The small spikes in `xbox-spike/step1` and `step2` settled this before touching the game. The problems, in the order they came up:

1. **Linker choice.** The modern `link.exe` refuses `/SUBSYSTEM:XBOX`, so the XDK linker (`bin\vc71\Link.Exe`) is used.
2. **Section alignment.** The modern CRT has 64-byte aligned sections, which the XBOX subsystem rejects.
   - Fix: link as `/SUBSYSTEM:WINDOWS`.
   - Then patch the subsystem field to 14 before `imagebld`, which only accepts XBOX images. This became `patch_pe_for_imagebld.ps1`.
3. **Missing Win32 imports.** The modern CRT imports Win32 functions the Xbox does not have.
   - `XboxCrtShim.cpp` implements them on top of XAPI and the kernel.
   - An import list (`XboxImports.txt`) plus `gen_imports.ps1` generates the `__imp__` pointers.
   - Functions XAPI already provides (`RaiseException`, `GetCurrentThreadId`, `VirtualQuery`...) were removed from the shim after they caused duplicate symbols.
4. **Build tooling.** An environment variable named `ML` broke MASM detection, so it was renamed to `MASM32`.
5. **Crash on start.** The modern CRT's `mainCRTStartup` expects a Win32 process. `XboxEntry.cpp` was written to do what the XDK's `xapi0` does:
   - computes the TLS size;
   - creates the main thread;
   - runs `XapiApplyKernelPatches` and `XapiInitProcess`, and the XAPI initializers `.CRT$RIA..RIZ`;
   - only then calls `mainCRTStartup`.
6. **`thread_local` and thread-safe statics crashed.** The compiler hard-codes the Win32 TLS slot `fs:[2Ch]`, but the XDK keeps it at `fs:[04h]` (`__tls_array = 4`).
   - The patch script rewrites every `mov r32, fs:[2Ch]`.
   - All 34 hits were checked to land on real instructions.
7. **C++ exceptions hung, then double-faulted** (bug check `0x7F`). The CRT's SEH validation checks the handler against the TIB stack bounds; on the Xbox `fs:[4]` is not the stack base, and the failure path (`int 29h`) is fatal.
   - Fix: `_ValidateEH3RN`, `__except_validate_context_record` and `__except_validate_jump_buffer` are overridden in the shim.
8. **Access violation reading the PEB.** `__acrt_is_secure_process` and `__acrt_app_verifier_enabled` read the Win32 PEB (`fs:[18h]+30h`), which does not exist here. Both are overridden.
9. **Header clashes.** The XDK ships a 2003 STL that shadowed the modern one.
   - Fix: include order is project, then modern STL/UCRT, then XDK.
   - The XDK's `_Interlocked*` declarations clash with the modern intrinsics; `XboxXtl.h` renames them.
10. **More linker limits.**
    - `/bigobj` objects cannot be read by the 2003 linker.
    - `extract-xiso` mangles forward-slash paths.

`xbox-spike/step2` ended as a working C++17 program on xemu: exceptions, RTTI, threads, `thread_local`, streams and D3D8. `src/xbox/tools/XboxBringup.cpp` is its color-staged successor. Each stage paints the screen a different color, which is how the first boots were followed before any log existed.

### Phase 2: compiling the whole game

`cmake/xbox_toolchain.cmake` and `cmake/xbox.cmake` were written. The Wii port was used as the template for the platform layer, since it has the same backend structure as PS2 and Wii (`*_XBOX.cpp` files, `src/xbox/` tree). Parts were written in parallel by several agents, each with a task file and a shared rule set; the coordination notes are the `buzon-xbox` mailbox, not committed.

Shared-code build breaks and their fixes:

| Break | Fix |
|-------|-----|
| UCRT `float_t` clashed with the engine's Java aliases | `_M_FP_FAST` |
| Missing `zconf.h` | Generated with `configure_file` |
| fdlibm's `HUGE`/`DOMAIN` clashed with the UCRT names | `_CRT_DECLARE_NONSTDC_NAMES=0` on those files |
| Mod sources could not find their headers | Include paths added |
| Code guarded as "PS2 or Wii" or "PC-only" | `PLATFORM_XBOX` added to those guards |

### Phase 3: first screens in xemu

| Symptom | Cause | Fix |
|---------|-------|-----|
| Menu black | First renderer draft | `RenderAPI_D3D8_XBOX.cpp` rewritten as a full GL-state emulation on D3D8 fixed function. |
| Logo missing | Non-power-of-two texture | Resample textures to powers of two before `XGSwizzleRect`. |
| Controller not detected | `Display::update()` never called `processMessages()` | Call it. |
| Only the D-pad worked, buttons did nothing | Menu input code guarded as PS2-only | Xbox added to those Legacy UI guards. |
| "Failed to write session lock" | Saves pointed at `D:` (read-only disc/launch folder) | Save to `T:` (`ClientPlatformPolicy_XBOX::minecraftDirectory`). |
| xemu aborted with *"Reserved pb command"* | Whole chunk sections sent in one `DrawVerticesUP` overflowed the NV2A push buffer | Batch at 1020 vertices; create D3D textures lazily. |
| World untextured | The lightmap bind on texture unit 1 overwrote the unit-0 texture | Track the active texture unit. |
| World seen from inside, no ground | Cull mode inverted | `GL_BACK` maps to `D3DCULL_CW`. |
| Borrowed Wii tuning broke rendering | Wii-only vertical window and mesh knobs | `XboxTuning.h` reduced to a minimal, Xbox-specific set. |

### Phase 4: in-game crashes, falling through the floor, spawning at bedrock

Gameplay showed NaNs everywhere:

- `Path::sortForward` crashed on null heap slots from NaN distances;
- `EntityLiving::onUpdate` looped forever normalizing infinite rotations;
- players walked through blocks.

The FPU/MMX state was the first suspect. The log showed it clean.

A boot-time numeric self-test (`XboxSelfTest.cpp`) was added: known `java.util.Random` values, 64-bit math, `floor`, `sqrt`, `sin`, and `MathHelper`. It printed:

```text
FAIL MathHelper::sin(1) got=-0.1097 expected=0.8415
FAIL MathHelper::sqrt_double(16) got=-inf expected=4
```

**Cause:** `fdlibm.h` picks the word order of a `double` from the compiler's macros. It knew `__i386__` but not MSVC's `_M_IX86`, so on the Xbox it assumed big-endian. Every `StrictMath` call (`sin`, `sqrt`, `atan2`, ...) was reading the two halves of each double swapped.

**Fix:** recognise `_M_IX86` / `_M_X64`. That one line fixed the NaNs, the collisions and the bedrock spawns.

Two memory fixes followed, found through a memory log and by catching the throw with `gdbthrow.py`:

- **Out of memory while walking** (back to the menu with `bad allocation` and 10 MB still free). The world's lighting-update queue grew without limit, doubling its vector until one contiguous allocation failed. On Xbox it is now capped (`PLATFORM_LIGHTING_QUEUE_HARD_CAP`).
- **Chunk display lists** are stored as pre-converted 24-byte vertices instead of 32-byte tessellator data. That saves memory and the per-frame conversion.

### Phase 5: the real console would not boot

The same ISO that ran in xemu showed only a black screen on a softmodded Xbox. The kernel cannot be the difference, because xemu runs the real BIOS/kernel image; only the CPU and GPU emulation differ.

A scan of the executable found about 1800 SSE2 instructions, all inside the VS2022 CRT. The Pentium III has SSE, not SSE2, and xemu executes SSE2 anyway.

Two tools made the real hardware debuggable:

- **`T:\debug.log`,** written line by line from the first instruction of `main` and read over FTP after power-off.
- **A UDP network log** (`XBOX_NETLOG_HOST` + `escuchar_log.py`) so the console could be watched live. This was needed because launching a game unloads the dashboard and its FTP server.

Then it was bisected on the console, one boot at a time:

1. **The log stopped right after the CPU line,** at the first self-test `snprintf("%.3f")`.
   - Most CRT SSE2 is plain 8-byte moves (`xorps xmm0,xmm0` + `movlpd [mem],xmm0`, `movq`/`movsd` pairs). `patch_sse2_moves.ps1` rewrites them to their SSE1 twins after linking (218 sites).
   - SSE2 arithmetic in math routines was replaced with x87 C code: `ldexp`, `scalbn`, `_87except`.
   - The game's three `std::pow` calls were moved to fdlibm.
2. **Still stopped at the same line.**
   - The script also missed `cvttsd2si ecx,[ebp+disp]` in `__acrt_fltout`: a memory operand, so no `xmm` in the disassembly text. It now calls an x87 stub written into a reserved code cave.
   - The instrumented self-test showed that `%.0f` of `0.0` worked but `2.0` did not.
3. **`__acrt_fltout` calls `__libm_sse2_log10_precise` directly,** a pure SSE2 log10 with no CPU check. The script had skipped it because "sse2" in a function name was taken to mean "dispatched". It was replaced by an x87 version, and the name exemption was narrowed.
4. **The next boot stopped at the `ceil` step.** Without SSE2, `ceil`/`floor` dispatch to `__ceil_default` / `__floor_default`, which are also SSE2 code. They were replaced by x87 `frndint` versions.
5. **The next boot reached the main menu on the real console.**

`scripts/xbox/patch_sse2_moves.ps1` now prints any SSE2 it leaves in the image on every build.

### Phase 6: polish after it ran on hardware

- **Menu textures released on world entry** (panorama, logo), as the PS2 port does.
- **Delete world** with X in the world list. The confirmation's Yes/Cancel is reachable with left/right.
- **Creating a second world after leaving one:** the keyboard did not open and the D-pad did nothing. The network log, with screen and keyboard-focus events added, showed the name field gaining focus and losing it in the same instant.
  - Cause: the create-world screen still computed mouse hover from the stale cursor position on Xbox (the guard excluded only PS2). That hover closed the keyboard and blocked navigation.
  - Related fix: the virtual keyboard now also releases focus on every screen change.
- **Player skin invisible.** The selected skin's image is not in the data set, so the game now falls back to `/mob/char.png`.
- **Sound** (`SoundManager_XBOX.cpp`). DirectSound crashed inside `DirectSoundCreate` with a null memory counter.
  - Cause: the XDK libraries' own C++ static initializers had never run. They sit in read-write `.CRT$XCU` data, which the linker grouped apart from the modern CRT's read-only initializer table.
  - `XboxEntry.cpp` now runs them, between two read-write markers.
  - A stereo / Dolby Digital option was added to the options.
- **Red crash screen.** An unexpected exception now logs the error and holds a red screen, instead of exiting and letting the console reboot the title.

### Phase 7: optimization

With the game booting on real hardware, it ran at around 17 FPS at 2 chunks, and memory was constantly nearing the 64 MB limit. The unbounded pathfinding caused `vector too long` out-of-memory crashes (desktop limits were infinite), and audio often crashed the console entirely.

A comprehensive profiling and optimization pass (using the new `xbox.perf` and `xbox.mem` logs) solved the main bottlenecks:
- **Audio:** SFX data was moved to contiguous physical memory (`XPhysicalAlloc` with write-combine) to fix the hardware crashes.
- **Memory:** Dropping CPU-side texture copies, freeing geometry on menu exit, and restricting the chunk cache to the render distance reclaimed over 15 MB.
- **Performance:** Rendering was overhauled by filtering redundant D3D states and moving display lists to static vertex buffers backed by 512 KB shared pools.
- **Stability:** Pathfinding was bounded to 300 nodes and time-sliced (3 per tick), preventing the `vector too long` out-of-memory crashes.

These changes stabilized memory and boosted performance to ~50-60 FPS on real hardware at a 2-chunk render distance.

---

## 11. Legal

The XDK is Microsoft's and is **not** distributed with this project. Do not commit it or any file derived from it; the repository only contains original source and build scripts. A built XBE links XDK libraries, so do not redistribute binaries without considering that. Game assets are not included either.
