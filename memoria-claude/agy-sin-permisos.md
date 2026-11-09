---
name: agy-sin-permisos
description: Always launch agy (Antigravity CLI) with --dangerously-skip-permissions; the user authorized it
metadata:
  node_type: memory
  type: feedback
  originSessionId: 7b5211e6-8642-449a-b2fa-52a40e1b3a14
  modified: 2026-09-28T13:48:24.660Z
---

Launch agy with `agy --dangerously-skip-permissions` (add `--conversation=<id>` / `-c` to keep a conversation), from the folder it must work in. Full procedure in `C:\Users\xeodeo\Desktop\COMO-USAR-AGY.md` section 4-A.

**Why:** 2026-09-28 agy sat for minutes on a hidden "Requesting permission for: ... Run this command?" menu; the user said "bypass los permisos todo ... pa que no pregunte cada rato" and asked to record it in the guide.

**How to apply:** before handing agy a task, check its screen with `orca terminal read --screen --json` (plain read hides the permission menu); if it was started without the flag, exit with two Ctrl+C and relaunch with it, then tell it to resume. Related: [[traspaso-contexto-ia]].

OpenCode (Windows, npm `opencode-ai`, logged in to OpenCode Console): permissions skipped globally in `~/.config/opencode/opencode.json` (`"permission": {"*":"allow",...}`, default model `opencode/gemini-3.1-pro`; `opencode reload` after edits). Launch as an Orca worker with `--agent opencode --worktree current` so its tab shows next to agy's (a `path:` worktree puts it in another sidebar group). Guide section 13.
