---
name: una-compilacion-a-la-vez
description: "Only one build/xemu at a time across Claude, agy and OpenCode — use buzon-xbox/BUILD.lock"
metadata:
  node_type: memory
  type: feedback
  originSessionId: b9320646-46b1-4ea9-beec-04aa1c771a88
  modified: 2026-09-28T20:07:08.163Z
---

Only one Xbox build (and one xemu) may run at a time across all agents. Before building or launching xemu, check `buzon-xbox\BUILD.lock`; if it exists wait, else create it (`<who> <what> <time>`) and delete it when done (even on failure). Rule file: `buzon-xbox\REGLA-BUILD-LOCK.md`.

**Why:** 2026-09-28 the user noticed two builds running at once (agy re-verifying in wt-async while OpenCode built unity in wt-ltcg and Claude built the main repo): "que antes de buildear se necesite confirmar antes de buildear para que no pase eso".

**How to apply:** Claude also follows the lock for `xbox-spike\build_preset.ps1` and autotest runs. Tell agents to stop extra verification builds once their task is integrated. Related: [[anotar-en-buzon]].
