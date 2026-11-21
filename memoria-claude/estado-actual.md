---
name: estado-actual
description: "Start any session by reading buzon-xbox/ESTADO-ACTUAL.md (21-11): test branches port152-erluza (wt-152-rend) and opti30-erluza (wt-opti30)"
metadata:
  node_type: memory
  type: project
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-20T17:23:29.850Z
---

`Desktop\Minecraft en xboox clasico\buzon-xbox\ESTADO-ACTUAL.md` is the short status file: branches per version,
builds in the launcher, decisions the user made, what to test and what is next. Header rewritten on 2026-11-20.

As of 2026-11-21: the console runs the `*-erluza` test branches (1.5.2 `port152-erluza` `b73c465` in `wt-152-rend`,
1.2.5 `opti30-erluza` `d0bff75` in `wt-opti30`), pushed to `xbox`. Encargos 87-94 done (94 = fireworks 1.5.2); Claude
added the End-return crash fix (1.2.5), LCE lava fix and the LCE autosave icon (both). Most 1.2.5 items confirmed by the
user. agy: encargo 95 (1.2.5 clock not turning + dragon tick spikes) in progress from its UNCOMMITTED tree in
`wt-opti30` — commit there only with `git add <file>`, never `-a`; next encargo 96 (LCE animated character in the HUD).
User decisions: hidden mobs only 6+ blocks below the camera; erluza's leaves culling stays on (no option).
Once the user confirms: fast-forward `port152-claude` and `xbox-opti-30` to the erluza branches.

**Why:** the user wants a resume file for each new chat; several worktrees make "which folder is current" non-obvious.

**How to apply:** read it plus `00-CONTEXTO.md` at the start of a session and update it whenever the state changes.
Keep it short; the technical history goes in `docs/XBOX_PORT.md` (tracked though `docs` is gitignored: `git add -f`),
per-topic findings in numbered `buzon-xbox\NN-*.md`. Related: [[port152-estado]], [[traspaso-contexto-ia]],
[[usar-agy-no-subagentes]], [[subir-a-consola-ftp]].
