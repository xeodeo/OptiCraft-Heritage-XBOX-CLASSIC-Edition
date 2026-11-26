---
name: estado-actual
description: "26-11: consola = builds 24-11 12:38 (port152-int 69b94bc5 / opti30-erluza 4d28ed75 + arreglo redstone, ESP, dupe SIN COMMIT) + lanzador Update. Start any session by reading buzon-xbox/PENDIENTES-TODO.md (22-11 20:45): good branches port152-int 6121e6c and opti30-erluza a20d430, in launcher but NOT uploaded"
metadata:
  node_type: memory
  type: project
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-20T17:23:29.850Z
---

`Desktop\Minecraft en xboox clasico\buzon-xbox\ESTADO-ACTUAL.md` is the short status file: branches per version,
builds in the launcher, decisions the user made, what to test and what is next. Header rewritten on 2026-11-20.

As of 2026-11-22 20:45: launcher has 1.5.2 `6121e6c` / 1.2.5 `a20d430` (113 chocobos, 114 minimap, 111.5 save hitch, 115 mods off + per-world detector, 116 Modrinth) NOT uploaded (console was off); agy out of quota until ~2026-11-28; next: upload, test, then 108. Older note — as of 17:55: **full pending list in `buzon-xbox/PENDIENTES-TODO.md`** (read it first). On the console (18:40):
1.5.2 `ee55f32` (branch `port152-int` = everything + mods + 3D skins + HTTPS + UI fixes) and 1.2.5 `e4af03f` (`opti30-erluza`).
agy-1 encargo 105 half done uncommitted in `wt-152-rend` (no reset); agy-2 encargo 108 on `port152-blq` not started;
agy queue 105→108→111→113→114→115; 110 HTTPS done by Claude (`XboxTls`, mbedTLS from the Classic Xbox Store, relaxed).
Merge 105/108 into `port152-int`. Per-console work: [[plataformas-que-hacer]]. Commit in agy trees only with
`git add <file>` ([[commit-sin-a-con-agy]]). Decisions: hidden mobs 6+ below; erluza leaves culling stays.

**Why:** the user wants a resume file for each new chat; several worktrees make "which folder is current" non-obvious.

**How to apply:** read it plus `00-CONTEXTO.md` at the start of a session and update it whenever the state changes.
Keep it short; the technical history goes in `docs/XBOX_PORT.md` (tracked though `docs` is gitignored: `git add -f`),
per-topic findings in numbered `buzon-xbox\NN-*.md`. Related: [[port152-estado]], [[traspaso-contexto-ia]],
[[usar-agy-no-subagentes]], [[subir-a-consola-ftp]].
