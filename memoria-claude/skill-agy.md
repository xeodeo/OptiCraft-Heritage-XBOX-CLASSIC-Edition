---
name: skill-agy
description: Para lanzar, escuchar o revisar a agy usar la skill `agy` (~/.claude/skills/agy) con su escuchador.py
metadata:
  type: feedback
---

Todo lo de dirigir a agy está en la skill `agy` (`~/.claude/skills/agy/SKILL.md`, pedida por el usuario el 12-10-2026). La guía larga es `Desktop\COMO-USAR-AGY.md`.

**Why:** el usuario se quejó de que no tenía "escuchador de agy". Yo esperaba con un `until` sobre un handle fijo de Orca, y esos handle caducan.

**How to apply:** al lanzar un encargo a agy, arma enseguida el Monitor con `escuchador.py --hecho <señal> --clave <carpeta>` y rearma al expirar. Ver también [[agy-sin-permisos]].

**03-11 mistake:** I sent encargo 58 to an agy terminal from ANOTHER project (the PS3 Store, cwd `...\ps3`), because it was the only `antigravity` terminal. The user had to pause it.
**Why:** several projects share Orca, so `agentIdentity` alone does not say whose agy a terminal is.
**How to apply:** check the terminal's `cwd`/`worktreePath` (in `orca terminal list --json`) and its screen before sending anything. If no agy is open in our worktree, launch a new one there:
`orca terminal create --worktree active --shell powershell.exe --command 'cd "<wt>"; agy --dangerously-skip-permissions --effort high'`
