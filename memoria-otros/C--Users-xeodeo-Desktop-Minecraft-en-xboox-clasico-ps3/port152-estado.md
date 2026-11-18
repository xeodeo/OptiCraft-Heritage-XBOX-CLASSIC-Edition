---
name: port152-estado
description: Port de OptiCraft a Minecraft 1.5.2 en marcha; empezar por buzon-xbox/48-ESTADO-port152-21-10-tarde.md
metadata:
  type: project
---

Desde el 21-10-2026 se porta OptiCraft (Java 1.2.5, ver [[version-java-125]]) a 1.5.2, directo y por grupos
(1 bloques, 2 redstone/raíles, 3 mobs/comercio, 4 mundo, 5 resto). Fuente Java descompilada en `mcp-diff\src125` y `src152`.
Ramas `port152-base` (wt-152-codex) y `port152-inventario` (wt-152-agy). El estado y lo pendiente están en
`buzon-xbox\48-ESTADO-port152-21-10-tarde.md` (y la nota general `buzon-xbox\SALTO-1.5.2-21-10.md`).

**Why:** el usuario quiere la 1.5.2 estable en su Xbox; la optimización de biomas/cuevas quedó para después.
**How to apply:** el terreno de 1.5.2 cambia en colinas/montañas (no prometer mismas semillas); cada versión va en su
carpeta del lanzador ([[xbox-loader-copy]]); pak y XBE se copian juntos si cambia el atlas.

**26-10:** guía completa de descompilar (MCP) y de revisar encargos en `buzon-xbox/55-GUIA-mcp-descompilar-y-revisar.md`
(scripts `mcp-diff/rehacer.sh` y `mcp-diff/comparar.sh <wt> --commit <hash>`, que marca cascarones con menos del 60 % del Java).
Build en la Xbox: wt-152-codex `6c7d7ce` (mobs con ensureEntityInit). Codex 53 rechazado 2 veces (`56-RECHAZO-codex-53.md`).

**26-10 tarde (fin de sesión):** rama buena `port152-claude` en `wt-152-codex`, último `0327d35` (en la Xbox: sello
"1.5.2 26-10 13:26"): mundo (agy 54), mobs, pistón 1 tick, tolva, pociones (Codex 59), objetos que se juntan/orbes/portal
(Codex 61), yunque/faro/marco/libros/mapas (agy 57). WIP sin compilar: agy 58 creativo+cofre trampa `adbf194` en
wt-152-agy; aldeanos (Codex 62 -> agy2 63) en wt-152-pociones `c746c8f` (sobre `af44b2e`). Lista de lo que falta:
`buzon-xbox/60-LO-QUE-FALTA-152.md`. Codex y agy sin cuota hasta el 27-10. 1.2.5: pistón arreglado `d07650b`.
