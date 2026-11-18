---
name: skill-agy
description: Para lanzar, escuchar o revisar a agy usar la skill `agy` (~/.claude/skills/agy) con su escuchador.py
metadata:
  type: feedback
---

Todo lo de dirigir a agy está en la skill `agy` (`~/.claude/skills/agy/SKILL.md`, pedida por el usuario el 12-10-2026). La guía larga es `Desktop\COMO-USAR-AGY.md`.

**Why:** el usuario se quejó de que no tenía "escuchador de agy". Yo esperaba con un `until` sobre un handle fijo de Orca, y esos handle caducan.

**How to apply:** al lanzar un encargo a agy, arma enseguida el Monitor con `escuchador.py --hecho <señal> --clave <carpeta>` y rearma al expirar. Ver también [[agy-sin-permisos]].
