---
name: estado-actual
description: "Start any session by reading buzon-xbox/ESTADO-ACTUAL.md (rewritten 09-11): 1.2.5 in wt-opti30, 1.5.2 test branch port152-aldeanos in wt-152-pociones"
metadata:
  node_type: memory
  type: project
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-09T15:59:46.511Z
---

`Desktop\Minecraft en xboox clasico\buzon-xbox\ESTADO-ACTUAL.md` is the short status file: branches per version,
builds in the launcher, decisions the user made, what to test and what is next. Rewritten on 2026-11-09.

As of 2026-11-09: 1.2.5 = `xbox-opti-30` (`wt-opti30`, fog + sky fixed). 1.5.2 test build = `port152-aldeanos`
(`wt-152-pociones`): villagers, entity events, sounds, Java spawn walk, fog/sky, perf; waiting for the user's console test,
then merge into `port152-claude` (`wt-152-codex`). agy works on encargo 78 (tick parity with Java) in `wt-152-rend`.

**Why:** the user wants a resume file for each new chat; several worktrees make "which folder is current" non-obvious.

**How to apply:** read it plus `00-CONTEXTO.md` at the start of a session and update it whenever the state changes.
Keep it short; the technical history goes in `docs/XBOX_PORT.md` (tracked though `docs` is gitignored: `git add -f`),
per-topic findings in numbered `buzon-xbox\NN-*.md`. Related: [[port152-estado]], [[traspaso-contexto-ia]], [[usar-agy-no-subagentes]].
