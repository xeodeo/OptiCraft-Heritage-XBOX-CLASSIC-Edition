---
name: xbox-loader-copy
description: "After every Xbox build, copy it into its version folder under ClassicXboxLoader\\...\\OptiCraft (1.2.5\\, 1.5.2\\)"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 1f1fbea3-50a1-408f-826c-f375a335f8f3
  modified: 2026-10-26T14:11:09.684Z
---

After every compiled Xbox build, replace the files in `C:\Users\xeodeo\Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\OptiCraft`, **in the folder of its version** (since 2026-10-21):
- root: `default.xbe` = the launcher (lists the folders by itself) + `data\assets` with its 5 PNGs only;
- `1.2.5\`: OptiCraft[_exp][_720p].xbe + assets.pak + texturepacks\ (branch xbox-opti-30). No loose data\ since 2026-10-21 (the user asked for only what is needed): xbox-opti-30 builds loose data, so when its data changes regenerate the pak with `python scripts/make_pak.py <data> <pak>` instead of copying data\; the game reads D:\assets.pak first;
- `1.5.2\`: the 1.5.2 port (branch port152-*) with its own assets.pak; version.txt = launcher description.

**Why:** the user sends the game to the console from that folder ("siempre reemplaza los archivos nuevos acá", 2026-09-24) and on 2026-10-21 asked that each version have its own folder ("no se debe quedar acá") and that the launcher detect them by itself.

**ALWAYS put every new build straight into `OptiCraft\` (its version folder): that is the folder the user transfers.**
On 2026-10-26 a diagnostic build was only in OptiCraft_actualizar\ and the user was annoyed ("ponlo en la de pasar").
WARNING: the log line "build <date> <time>" is __DATE__/__TIME__ of main_xbox.cpp only (recompiled only when that file
changes), NOT the XBE build time: never use it to claim the console runs an old build (Claude did, wrongly).

**Only-what-changed folder (asked 2026-10-21, "pasar siempre solo lo necesario"), extra, never instead:** after each copy also rebuild
`OptiCraft_extracted\OptiCraft_actualizar\` from scratch with ONLY the files changed since the previous update, same
subfolders (1.2.5\, 1.5.2\), plus a LEEME.txt with the date and what changed. The user copies it over OptiCraft on the
Xbox. OptiCraft\ stays the complete install. Usually only XBEs; assets.pak only when the atlas/pak changed; texturepacks
and the launcher almost never.

**How to apply:** never drop game files loose in the root again. `xbox-spike\build_preset.ps1` still copies to the old flat root: copy by hand into the right folder. Related: [[xbox-720p-xbe]], [[xbox-builds-prueba-publica]].
