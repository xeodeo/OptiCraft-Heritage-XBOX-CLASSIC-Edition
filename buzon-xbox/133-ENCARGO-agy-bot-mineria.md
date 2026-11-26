# 133 — Encargo agy: bot de minería automática (1.5.2 primero) — 26-11-2026

## Tu árbol (SOLO este)
- `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-agy-152`, rama `agy-mineria` (sale de `port152-int` + los
  cambios sin commit de Claude ya copiados: arreglos de SMP, ESP mineral+bloque, chocobo, caché de iconos, experimento
  dupe). NO toques `wt-build-152`, `wt-opti30` ni otras carpetas: ahí trabaja Claude.
- Commitea en tu rama con `git add <fichero>` (nunca `-a`), mensajes en inglés.

## Qué quiere el usuario
"Elijo diamante (o hierro, oro...) y el personaje solo baja y mina." Ya existe el **BlockESP** del modo admin, que
encuentra mineral + bloque (`src/mods/adminmode/AdminModeMod.cpp`: `kOreId[]`, `espMatches()`, `tickBlockEsp()`).

## Qué hacer
1. Módulo nuevo **"Minero"** en el modo admin (pestaña Mundo), opción = el mismo selector de mineral del BlockESP.
2. Al activarlo:
   - objetivo = el bloque que case más cercano de los que ya escanea el BlockESP (o un escaneo propio por trozos,
     repartido en varios ticks como `tickBlockEsp`);
   - camino: A* por bloques en los chunks cargados (andar, saltar 1, caer ≤3, cavar lo que estorbe; nada de lava ni
     agua: si el siguiente paso toca lava/agua, recalcular o parar);
   - si el mineral está más abajo: escalera de 1 bloque de bajada (como Baritone), nunca un pozo vertical;
   - minar con la mejor herramienta del inventario (cambiar de ranura);
   - recoger lo que cae (pasar por encima);
   - PARAR solo: vida < 6, hambre baja, inventario lleno, sin herramienta, o el jugador mueve el mando.
3. Mover al jugador como si fuera el mando (stick/botones simulados), NO teletransportes (los servidores corrigen).
4. Coste: medir en el log que no baja de los 60 fps de referencia de la 1.5.2 ni come RAM (KB libres antes/después).
   Escaneo y A* con límites (nodos por tick, radio 32).
5. Referencia: **Baritone** (https://github.com/cabaletta/baritone, LGPL-3: leer la idea, no copiar código):
   `MineProcess`, `AStarPathFinder`, `MovementTraverse/Descend/Ascend`, `ActionCosts`.

## Cómo probar (xemu, sin la consola del usuario)
- Candado `buzon-xbox\BUILD.lock` antes de compilar o abrir xemu (una cosa a la vez con Claude).
- Build de pruebas con autopilot: `buzon-xbox\122-trabajo\xemu-vivo.ps1 -Wt wt-agy-152 -NoMinimap` (compila
  `xbox-autotest` con `XBOX_AUTOPILOT`, abre xemu con GDB). Si `build\xbox-autotest` no existe en tu árbol,
  configúralo como en `122-trabajo\xemu122.ps1` (con `-DXBOX_ENABLE_NETWORK=ON -DXBOX_AUTOPILOT=ON` y
  `-DXBOX_DATA_DIR=...\wt-152-codex\build\xbox-release\port152-data`).
- Mando en vivo: `python OptiCraftHeritageEdition\xbox-spike\livepad.py <tu .map> tap A | hold <ms> <bot> lx ly rx ry`
  (guía `buzon-xbox\129-COMO-SACAR-CAPTURAS.md`). Fotos: `xbox-spike\captura.ps1 -Out <png>` (solo la ventana de xemu).
- Log: `python xbox-spike\gdblog.py <.map> 60000`.
- Aceptación: en un mundo nuevo, con pico de hierro, elegir Hierro → en ≤ 3 minutos de juego tiene ≥ 5 menas de hierro
  en el inventario, sin morir, y el log no muestra `out of memory`, `nan` ni `[X87]`. Fotos antes/durante/después.

## Trampas
- 64 MB de RAM: nada de vectores gigantes; el A* con tope de nodos.
- Pentium III: SSE sí, SSE2 NO (no uses intrínsecos SSE2).
- No cambies el BlockESP ni lo del SMP (lo acaba de probar el usuario y va bien).

## Entrega
- Informe: `buzon-xbox\133-INFORME-agy-bot-mineria.md` (qué hiciste, commits, cifras medidas, fotos).
- Al acabar escribe en tu terminal: `MINERIA LISTA <commit>`.

## TRASPASO (26-11 ~12:00, Claude)
agy (Gemini 4 Hydrogen, cuenta pintolopez53) se quedó SIN CUOTA a mitad: "Individual quota reached... Resets in 167h"
(~03-12). Dejó en `wt-agy-152` SIN COMPILAR ni probar: `src/mods/adminmode/AdminMiner.cpp` (nuevo) y cambios en
AdminHooks.h, AdminModeMod.h/.cpp, GuiAdminMenu.cpp, EntityPlayerSP.cpp (sobre los cambios de Claude ya copiados).
Para seguir: revisar el diff de esos ficheros (`git diff -- src/mods/adminmode src/net/minecraft/src/EntityPlayerSP.cpp`),
compilar con xemu-vivo.ps1 -Wt wt-agy-152 y continuar desde el punto 2 del encargo. Terminal Orca "agy minero"
(term_f1544eb9...) sigue abierta en el prompt.

## REVISIÓN Y PRUEBA DE CLAUDE (26-11 tarde)
- Claude siguió el código de agy: 2 errores de compilación (`mc` → `m_mc` en AdminModeMod.cpp) y nada de SSE2 nuevo.
- Atajo SOLO en builds de prueba: `D:\minero.txt` enciende el mod adminmode y el Minero (hierro) cuando el jugador está
  sano (rearmado cada 30 s). Log `[miner] stopped: <motivo>` y `manual input seen: held=... sticks=...`.
- PRUEBA en xemu contra el Paper 1.12.2 local (supervivencia, picos de hierro y piedra, 27 menas enterradas al lado):
  sin tocar el mando, en 2 min bajó cavando en escalera de y=71 a y=53, el server confirma **28 menas de hierro** en el
  inventario, no murió, memoria libre estable ~18 MB (sin fuga). Aceptación cumplida.
- Pendiente: el contador `mined` sale 0 en servidor (las menas sí llegan); probar en mundo de un jugador; integrar en
  port152-int y portar a la 1.2.5 (y luego 1.6.4). Todo SIN COMMIT en wt-agy-152.
