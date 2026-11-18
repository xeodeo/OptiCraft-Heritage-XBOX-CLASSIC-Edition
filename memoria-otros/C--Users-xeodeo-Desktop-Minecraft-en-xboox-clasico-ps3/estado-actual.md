---
name: estado-actual
description: Start any new session by reading buzon-xbox/ESTADO-ACTUAL.md; the live branch is xbox-opti-30 in wt-opti30
metadata:
  node_type: memory
  type: project
  originSessionId: 7b5211e6-8642-449a-b2fa-52a40e1b3a14
  modified: 2026-10-02T14:47:00.846Z
---

`Desktop\Minecraft en xboox clasico\buzon-xbox\ESTADO-ACTUAL.md` is the short status file: which branch is current (`xbox-opti-30`, worktree `wt-opti30` — *not* the main folder, which lags on `xbox-unificada`), where the built XBE/ELF are, what was done last and what is pending.

As of 2026-10-02: everything is pushed to `xbox` and `fork`. The experimental 720p build is validated on console and the user likes it. Top pending item is **RAM climbing on death** (detail and a validation plan in `buzon-xbox\30-RAM-al-morir.md`) — unconfirmed, no death was ever captured in a log, and the game never actually crashed. Then the two unsent PRs to OptiJuegos.

**Why:** 2026-10-01 the user asked for a resume file for the next chat, after several sessions and six worktrees made "which folder is current" non-obvious.

**How to apply:** read it plus `00-CONTEXTO.md` at the start of a session, and update it whenever the state changes. Keep it short; the long technical history belongs in `docs/XBOX_PORT.md`, and per-topic findings in numbered `buzon-xbox\NN-*.md` files. Related: [[traspaso-contexto-ia]], [[xbox-builds-prueba-publica]], [[musica-comportamiento-original]].
