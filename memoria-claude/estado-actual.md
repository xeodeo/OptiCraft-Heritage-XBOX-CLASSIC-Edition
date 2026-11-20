---
name: estado-actual
description: "Start any session by reading buzon-xbox/ESTADO-ACTUAL.md (20-11): test branches port152-erluza (wt-152-rend) and opti30-erluza (wt-opti30)"
metadata:
  node_type: memory
  type: project
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-20T17:23:29.850Z
---

`Desktop\Minecraft en xboox clasico\buzon-xbox\ESTADO-ACTUAL.md` is the short status file: branches per version,
builds in the launcher, decisions the user made, what to test and what is next. Header rewritten on 2026-11-20.

As of 2026-11-20: the builds on the console are the `*-erluza` test branches (1.5.2 `port152-erluza` in `wt-152-rend`,
1.2.5 `opti30-erluza` in `wt-opti30`): fixes from the erluza fork (encargo 87), Nether fog / HUD safe area / same
options in both versions (88), stick rebind + chest in hand (89). agy is on encargo 90 (LCE texture pack: green chest
particles and fire; cursor over achievements; Nether fog 1.5.2 like 1.2.5, option if it is a matter of taste).
User decisions: hidden mobs only 6+ blocks below the camera; erluza's leaves culling stays on (no option).
Once the user confirms: fast-forward `port152-claude` and `xbox-opti-30` to the erluza branches.

**Why:** the user wants a resume file for each new chat; several worktrees make "which folder is current" non-obvious.

**How to apply:** read it plus `00-CONTEXTO.md` at the start of a session and update it whenever the state changes.
Keep it short; the technical history goes in `docs/XBOX_PORT.md` (tracked though `docs` is gitignored: `git add -f`),
per-topic findings in numbered `buzon-xbox\NN-*.md`. Related: [[port152-estado]], [[traspaso-contexto-ia]],
[[usar-agy-no-subagentes]], [[subir-a-consola-ftp]].
