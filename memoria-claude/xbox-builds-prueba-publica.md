---
name: xbox-builds-prueba-publica
description: "How the four Xbox builds (normal/experimental x test/public) and the PS2 one are configured and built, and where each is copied"
metadata:
  node_type: memory
  type: project
  originSessionId: 7b5211e6-8642-449a-b2fa-52a40e1b3a14
  modified: 2026-10-01T19:52:50.354Z
---

Build everything on the live branch with `powershell -File "buzon-xbox\build_opti30.ps1"` (Xbox normal + experimental + PS2, takes the BUILD.lock and releases it). Full table in `buzon-xbox\00-CONTEXTO.md` §3.

- **Experimental** = the same sources with one CMake switch, `XBOX_ASYNC_CHUNK_IO=ON` (chunk generation and region loads on a background thread). Default OFF in `src/platform/PlatformConfig.h`, so PC/PS2/Wii never get it; only the `-exp` presets turn it on.
- Presets → output: `xbox-release` → `bin/xbox/`, `xbox-release-exp` → `bin/xbox-exp/`, `xbox-public` / `xbox-public-exp` → `bin/xbox-public*/`, `ps2-release` → `bin/ps2/OptiCraft.elf`.
- Copying to the loader is manual: exp goes as `OptiCraft_exp.xbe` + `OptiCraft_exp_720p.xbe` (the same `default.xbe`; only "720" in the name selects 720p). Back up the previous ones first.
- A **new build folder starts with no log**: configure it once with `"-DMC_LOG_LEVEL=1" "-DXBOX_NETLOG_HOST=192.168.0.18"` — quoted, or PowerShell splits the IP at the dots and CMake fails with `Could not read presets from ...\.168.0.17`. That shipped an exp build with no log once.
- `xbox-spike\build_preset.ps1` is the old script: it points at the main folder, which lags behind.

Releases live in `Desktop\release\v<N>\`; tag `xbox-v<N>` on the `xbox` remote. `origin` is OptiJuegos — never push there. Related: [[estado-actual]], [[xbox-loader-copy]], [[una-compilacion-a-la-vez]].

Since 2026-10-08 `default.xbe` in the loader and the release is the **launcher** (preset `xbox-launcher`, `buzon-xbox\35-lanzador.md`); the game goes as `OptiCraft.xbe` / `_720p` / `_exp` / `_exp_720p`. Assemble the release with `buzon-xbox\montar_release_lanzador.ps1`. The user wants it to always open on the normal build.
