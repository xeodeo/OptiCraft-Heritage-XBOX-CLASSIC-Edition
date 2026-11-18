---
name: anotar-en-buzon
description: "For long tasks, write findings and a step-by-step plan into buzon-xbox as you go, so work survives a cut session"
metadata:
  node_type: memory
  type: feedback
  originSessionId: b9320646-46b1-4ea9-beec-04aa1c771a88
  modified: 2026-09-26T02:20:37.059Z
---

On long tasks, write what was discovered, what was fixed and the remaining plan into a `buzon-xbox\NN-tema.md` file early, and tick it off as work progresses; link it from 00-CONTEXTO.md §7.

**Why:** the user asked (2026-09-26, split screen work): "deja todo anotado, si vas a morir o algo, deja lo que descubriste etc." Earlier chats had collapsed and lost context.

**How to apply:** create the note before big code changes, update it after each step (mark [x]), and mention file:line causes. Current one: buzon-xbox/08-pantalla-dividida.md. Related: [[traspaso-contexto-ia]].

Watch out when writing Windows paths through the Bash tool heredocs: backslash sequences like `\0`, `\r`, `\v` got turned into control bytes in 00-CONTEXTO.md twice. Use the Write/Edit tools for Markdown with backslash paths.
