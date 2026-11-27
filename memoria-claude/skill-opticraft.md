---
name: skill-opticraft
description: "Skill suprema `opticraft` con todo el inventario del proyecto (repos, ramas, worktrees, buzón, consolas, scripts, mods, SDKs, reglas del usuario). Cargarla al inicio de cualquier sesión del port."
metadata:
  node_type: memory
  type: reference
  originSessionId: 808de2cd-0176-4508-a594-554d8b6d7609
  modified: 2026-11-27T22:33:08.365Z
---

Skill en `~/.claude/skills/opticraft/SKILL.md` (copia pública en `buzon-xbox/SKILL-OPTICRAFT.md`, se sube a la rama `xbox/contexto` con `sh contexto-respaldo/actualizar-contexto.sh` — el bucle del script ya la incluye desde 27-11-2026).

Contenido: directorios raíz, 3 remotes (xbox / fork / origin), todas las ramas port152-* y opti30-*, 25 worktrees, buzón de 269 entradas numeradas, consolas (Xbox 192.168.0.3, UDP log 192.168.0.18:9999, PS3, PS2 real/pcsx2), script `compilar-y-subir.ps1`, reglas de compilado (nunca /fp:fast, 4 XBE por versión), repos LCE (lce-tu19 TU19, lce-wiiu-reborn 1.13, DMCA smartcmd, GRAnimated, etc.), mods (Herobrine iLoveYou, ChocoCraft 1.7.10, ChocoCraft4, OptiFine 1.5.2, Nodus 1.5.2), SDKs (24 XDK en xbox-sdks), servidores Java locales, agentes (agy/OpenCode/subagentes), modismos del usuario ("mal bien" = confirmación), estado al día.

Cargarla:

- Al abrir conversación sobre Minecraft / OptiCraft / Xbox / PS2.
- Antes de tocar `wt-*`, `OptiCraftHeritageEdition`, `buzon-xbox`, `mcp-diff`, `lce-*`, `contexto-respaldo`.
- Antes de compilar, subir, probar, portar.
- Al cerrar sesión (para actualizar contexto).

Reemplaza en gran parte lecturas repetidas de [[estado-actual]], [[lce-repos-referencia]], [[xbox-plan-optimizacion]], [[subir-a-consola-ftp]], etc.
