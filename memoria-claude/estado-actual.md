---
name: estado-actual
description: "Start any session by reading buzon-xbox/PENDIENTES-TODO.md + ESTADO-ACTUAL.md (22-11): test branches port152-erluza (wt-152-rend) and opti30-erluza (wt-opti30)"
metadata:
  node_type: memory
  type: project
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-20T17:23:29.850Z
---

`Desktop\Minecraft en xboox clasico\buzon-xbox\ESTADO-ACTUAL.md` is the short status file: branches per version,
builds in the launcher, decisions the user made, what to test and what is next. Header rewritten on 2026-11-20.

As of 2026-11-22 17:55: **full pending list in `buzon-xbox/PENDIENTES-TODO.md`** (read it first). On the console:
1.5.2 `2ff4078` (branch `port152-int` = everything + mods + 3D skins + HTTPS) and 1.2.5 `a0addaf` (`opti30-erluza`).
agy-1 encargo 105 half done uncommitted in `wt-152-rend` (no reset); agy-2 encargo 108 on `port152-blq` not started;
109 (packs from console) queued; 110 HTTPS done by Claude (`XboxTls`, mbedTLS from the Classic Xbox Store, relaxed).
Merge 105/108 into `port152-int`. Per-console work: [[plataformas-que-hacer]]. Commit in agy trees only with
`git add <file>` ([[commit-sin-a-con-agy]]). Decisions: hidden mobs 6+ below; erluza leaves culling stays.

**Why:** the user wants a resume file for each new chat; several worktrees make "which folder is current" non-obvious.

**How to apply:** read it plus `00-CONTEXTO.md` at the start of a session and update it whenever the state changes.
Keep it short; the technical history goes in `docs/XBOX_PORT.md` (tracked though `docs` is gitignored: `git add -f`),
per-topic findings in numbered `buzon-xbox\NN-*.md`. Related: [[port152-estado]], [[traspaso-contexto-ia]],
[[usar-agy-no-subagentes]], [[subir-a-consola-ftp]].
