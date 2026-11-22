---
name: estado-actual
description: "Start any session by reading buzon-xbox/ESTADO-ACTUAL.md (22-11): test branches port152-erluza (wt-152-rend) and opti30-erluza (wt-opti30)"
metadata:
  node_type: memory
  type: project
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-20T17:23:29.850Z
---

`Desktop\Minecraft en xboox clasico\buzon-xbox\ESTADO-ACTUAL.md` is the short status file: branches per version,
builds in the launcher, decisions the user made, what to test and what is next. Header rewritten on 2026-11-20.

As of 2026-11-22: test branches `port152-erluza` `a2ec7d1` (wt-152-rend) and `opti30-erluza` `f1bf66a` (wt-opti30),
pushed to `xbox`, built into the launcher but NOT yet uploaded to the console (FTP was down). Encargos 87-98 done:
95 clock/compass + dragon, 96 animated HUD character (Claude), 97 firework crash/console crafting (LCE TU19
categories)/beacon/pigman/nether ice, 98 fixes from fork ramide1 (bow, display lists, MP, "Pistas en pantalla").
Claude also did: End-return crash, LCE lava, save icon, key table after load (chat button), options.txt dedupe.
Open: watch free RAM when returning to loaded areas (24->11 MB in agy's test); End portal 4-layers-per-pass idea
not approved yet; plan after 1.5.2: 1.6.4 then 1.7.10 ([[lce-repos-referencia]]). Commit in agy worktrees only with
`git add <file>` ([[commit-sin-a-con-agy]]). Decisions: hidden mobs 6+ below camera; erluza leaves culling stays.
Once the user confirms: fast-forward `port152-claude` and `xbox-opti-30` to the erluza branches.

**Why:** the user wants a resume file for each new chat; several worktrees make "which folder is current" non-obvious.

**How to apply:** read it plus `00-CONTEXTO.md` at the start of a session and update it whenever the state changes.
Keep it short; the technical history goes in `docs/XBOX_PORT.md` (tracked though `docs` is gitignored: `git add -f`),
per-topic findings in numbered `buzon-xbox\NN-*.md`. Related: [[port152-estado]], [[traspaso-contexto-ia]],
[[usar-agy-no-subagentes]], [[subir-a-consola-ftp]].
