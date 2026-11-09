---
name: port152-estado
description: Port de OptiCraft a Minecraft 1.5.2 en marcha; empezar por buzon-xbox/69-PLAN-COMPLETO-152.md (03-11)
metadata:
  node_type: memory
  type: project
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-09T15:59:50.533Z
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


**03-11:** the whole plan (done, being tested, in progress, missing in order, performance) is in `buzon-xbox9-PLAN-COMPLETO-152.md`. The good branch is `port152-claude` (wt-152-codex).
- The user wants a COMPLETE 1.5.2 ("versión muy completa"): commands, villagers, Wither, etc.
- Lesson: AI deliveries (Codex/agy) arrived with crashes: a pooled AABB deleted, nulls, recursion. Audit every delivery with subagents before handing it to the console.
- TEMP diagnostics `3b71bea` (item frame and anvil) are still in the branch; revert them once the cause is known.

**06-11:** the latest handoff is `buzon-xbox\71-TRASPASO-06-11.md`: state, what the user has tested, next steps and lessons.
- Agreed next steps: port the 1.5.2 fixes to 1.2.5 (configurable D-pad, `getHeldItem`, chat), and agy resumes the villagers.
- The diagnostic commit `3b71bea` was already reverted (`a195ade`).

**09-11:** see `buzon-xbox\ESTADO-ACTUAL.md` (rewritten): test branch `port152-aldeanos` `f909e5f` + docs `7493c3e`.

**06-11 tarde:** villagers done in `port152-aldeanos` (`wt-152-pociones`, `1e9fb58`), audited, XBEs in the loader; waiting
for the user's console test, then merge into `port152-claude`. The user does NOT want the 1.5.2 fixes ported to 1.2.5 now.
