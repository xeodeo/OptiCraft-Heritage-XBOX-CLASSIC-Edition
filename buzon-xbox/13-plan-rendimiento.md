# Plan de rendimiento Xbox (28-09-2026)

## Propuesta que eligió el usuario (texto original)
El juego ya va a 60 FPS la mayor parte del tiempo. Cada fotograma tarda 16,7 ms y de esos pasa ~10 ms esperando al televisor sin hacer nada. El problema no son los FPS medios, son los tirones al cargar terreno.

**Para quitar tirones (lo más importante):**
1. Cargar y guardar chunks en un hilo aparte que trabaje en esos ~10 ms libres. Hoy leer el disco, descomprimir y leer el formato de guardado bloquean el juego de 10 a 50 ms, y alguna vez 174. Es lo que más ganaría. Esfuerzo: medio-alto.
2. Guardar sin NBT, como Legacy: los chunks cargan y guardan mucho más rápido. Esfuerzo: medio. Hay que cuidar los mundos que ya tienes.
3. LTCG: 10-15 % más de CPU en todo, sin tocar código. Esfuerzo: bajo.

**Para ver más lejos (más chunks):**
4. Luz y bloques comprimidos, como Legacy: cada chunk pasa de ~80 KB a ~15-20 KB. Con eso cabrían más chunks en los 64 MB. Esfuerzo: alto.
5. Texturas comprimidas y audio por hardware: liberan varios MB. Esfuerzo: medio.

Recomendación: primero la 3, que es barata y se nota en todo, y después la 1, que es la que de verdad elimina los tirones.

## Plan aprobado
Está en `13b-plan-aprobado.md`, copia de `C:\Users\xeodeo\.claude\plans\pasted-content-id-d218-primero-un-joyful-flute.md`.

Resumen del reparto:
- OpenCode hace la **3 (LTCG)** en el worktree `..\wt-ltcg`, con el encargo 14.
- agy hace la **1 (carga y guardado en hilo)** en `..\wt-async`, con el encargo 15.
- Claude solo reparte, revisa y compila para la consola.

## Estado
- [ ] Paso 0: commit local de lo pendiente y worktrees.
- [ ] Encargo 14 (LTCG) entregado y verificado.
- [ ] Encargo 15 (async IO) entregado y verificado.
- [ ] Probado en consola y comparado con `xbox.spike`.

## Resultado del encargo 14: LTCG (28-09)
- **No es viable con la toolchain actual.** Se enlaza con el `Link.Exe` VC 7.1 del XDK (`cmake/xbox_toolchain.cmake:65`), y `/GL` de VS2022 da `LNK1262` (versiones distintas de `c2.dll` y del enlazador). Verificado por Claude.
- Única vía: enlazar con el `link.exe` de VS2022 y adaptar el PE para `imagebld` (`scripts/xbox/patch_pe_for_imagebld.ps1`). Es un experimento de riesgo medio; queda pendiente y solo si el usuario lo pide.
- El worktree `wt-ltcg` (rama `xbox-ltcg`) NO se integra: deja `XBOX_LTCG` en ON por defecto y rompería el build.
- Sin LTCG, OpenCode confirmó el build, que no hay SSE2 nuevo y que xemu llega al menú.
- **Descubierto en el log (14:46):** la causa n.º 1 de tirones es `chunkUnload`, que guarda al descargar (141 de 254 tirones, 25 ms cada uno), y `mobSpawn` dispara una carga de disco (178 ms). Todo se pasó a agy (`15b-CORRECCIONES-agy.md`), con la condición del usuario de no romper el Ender Dragon.

## Encargo 15 (agy) integrado, 28-09 15:05
- Rama `xbox-async-io`, commit `8ca8c87`, fusionada en `xbox-port`. Compilada la versión de prueba y copiada a ClassicXboxLoader.
- Qué hace:
  - `XBOX_ASYNC_CHUNK_IO=1` (`PlatformConfig.h`) activa `PLATFORM_ASYNC_CHUNK_GENERATION` en Xbox: carga y generación en `ChunkGenerationScheduler`, con prioridad por debajo de lo normal;
  - la serialización NBT del guardado va en el hilo de E/S;
  - `compress2` fuera del mutex de región;
  - `SpawnerAnimals` usa `getChunkIfExists`.
- Según agy, la población (y el Ender Dragon, `BiomeEndDecorator.cpp:53`) sigue en el hilo principal (`drainPendingPopulate`). **Falta verificarlo en la consola.**
- Para volver atrás: `#define XBOX_ASYNC_CHUNK_IO 0`.
- Pendiente: la prueba del usuario en consola con netlog, y comparar `chunkUnload`/`save`/`load`/`mobSpawn` con la partida de las 14:46 (254 tirones).

## Encargo 16: unity build (OpenCode), 28-09 15:20
- **No es viable sin refactor.** CMake desactiva `UNITY_BUILD` porque `CMAKE_CXX_LINK_EXECUTABLE` es propio. A mano salen unos 60 choques: nombres en `namespace {}` repetidos (`BUTTON_DONE`, `clampInt`, `PI_F`, `containsBiome`…), `stb_image`, y `windows.h` frente a `xtl.h`. Resultado real, documentado en `16-INFORME-opencode.md`.
- OpenCode también cambió, sin pedírselo, `wt-ltcg/cmake/xbox_toolchain.cmake`: comentó el `Link.Exe` del XDK y usa el `link.exe` moderno. Su XBE (15:16) llegaría al menú en xemu. Es una pista para el LTCG con enlazador moderno, pero **no está verificado en la consola** y NO se integra.
- La rama `xbox-ltcg` NO se integra.
- **Versión de prueba final (15:20):** `xbox-port` en `3b9c8a8` (async IO de agy + la protección contra nulos), en ClassicXboxLoader junto con el `assets.pak`. Pendiente de probar en la consola.

## Encargo 17: LTCG con link.exe moderno (agy), 28-09 15:45. CERRADO, no compensa
- A (XDK, sin LTCG): OK, como siempre.
- B (link.exe moderno, sin LTCG): enlaza, XBE de 4.079.616 bytes, llega al menú en xemu. PERO `patch_sse2_moves.ps1` avisa de mucho SSE2 NUEVO del CRT (`XmmCopy*`, `Palign*`, `LargeRangeBytes` de `memcpy`, `money_put`/`num_put`, `get_encoding`). La Xbox real no tiene SSE2: riesgo de cuelgue que xemu no detecta.
- C (moderno + LTCG): `LNK1257` / `C2268`, porque `floor` es auxiliar predefinido y `XboxP3Math.c` no debe llevar `/GL`. Tiene arreglo, pero arrastra el SSE2 de B.
- Decisión recomendada: NO seguir con LTCG (5-10 % de CPU frente a riesgo de cuelgues). Las ramas `xbox-ltcg` y `wt-ltcg` quedan como referencia, sin integrar.
- Siguiente candidata: día/noche sin reconstruir chunks (`World.cpp:4735`, `updateAllRenderers` al cambiar la luz del cielo).

## Encargos 19 y 20 (agy), integrados en `4f29550` (28-09 ~16:50)
- Medido a las 16:24: `chunkUnload` en 153 tirones (28 ms de media), con `gen` casi igual y `save` de 2,8 ms. El guardado ya estaba arreglado.
- Diagnóstico correcto de agy: `PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT` valía 0 en Xbox (`PlatformAsyncTuning.h`), así que TODA la generación seguía en el hilo principal (`drainPendingGeneration` dentro de `unload100OldestChunks`). Claude se había equivocado al pensar que era una espera al hilo.
- 19: no se descargan chunks con un vecino en generación (`isWorkingOn` del scheduler).
- 20: cola asíncrona 4, `ISOLATED_BIOME_SOURCE` 1, `ASYNC_CHUNK_DECODE` 1. La población sigue en el hilo principal.
- Versión de prueba en ClassicXboxLoader. PENDIENTE: prueba del usuario, netlog, y vigilar la memoria libre (`xbox.mem` / `freeKB`).

## 28-09 17:02: `4f29550` va PEOR en la consola
- 466 tirones frente a 377; `chunkUnload`+`gen` 242 veces. Encargo 21 a agy, con trazas `xbox.async`.
- Mientras tanto, ClassicXboxLoader tiene OTRA VEZ `3b9c8a8`, compilado en el worktree `wt-prev` (detached) con `-DMC_LOG_LEVEL=1`; XBE de 4.218.880 bytes.
- **Trampa:** un worktree nuevo compila con `MC_LOG_LEVEL=0` (sin netlog de tirones). El `1` solo está en la caché del repo principal. En los worktrees, pasar siempre `-DMC_LOG_LEVEL=1` si el XBE va a la consola.

## Encargo 21 (agy) integrado en `7e10e5c`, 28-09 17:10
- Causa (agy): en `provideChunk`, el bloque `PLATFORM_ASYNC_CHUNK_GENERATION` iba DESPUÉS del bloque incremental síncrono, que se quedaba con casi todo. Además, `QueueFull` caía a la generación síncrona.
- Arreglo: primero el bloque async; con `QueueFull` devuelve `blankChunk` y reintenta en el siguiente tick. Trazas `[xbox.async] sync= async= queueFull= popNeighbour= adopted=` cada 5 s.
- En ClassicXboxLoader, lado a lado:
  - `default.xbe` y `OptiCraft_720p.xbe` = `3b9c8a8` (la anterior);
  - `OptiCraft_async.xbe` y `OptiCraft_async_720p.xbe` = `7e10e5c` (la nueva).
- Riesgo a vigilar en la consola: `blankChunk` temporal (¿huecos o caídas al vacío fuera del radio 1?).

## 28-09 ~18:30: estado
- La estable `3b9c8a8` (`default.xbe`): 73 tirones en 5,5 min (13/min), FPS mínimo 51. Lo que queda son 52 tirones de `chunkUnload`+`gen` síncrono (~23 ms).
- La async `36531e0` (`OptiCraft_async*.xbe`): más fluida (`sync=0`, ~5 chunks/s), PERO sin árboles (los chunks adoptados no se pueblan) y se congela al caer (probable `blankChunk`). Es el encargo 23.
- agy se quedó SIN CUOTA con Claude Opus 4.6 al empezar el 23 (se reinicia en ~4h23m desde las 18:30). El usuario puede cambiar a Gemini 3.1 Pro con `/model`. El worktree `wt-async` quedó limpio.

## 28-09 21:50 — Claude: causa de los chunks corruptos y los cuelgues de la async
- En Xbox, los `#if PLATFORM_PC_LEGACY || PLATFORM_WII` dejaban fuera a la Xbox. Por eso el worker hacía `provideChunk()` COMPLETO: estructuras, decoración y luz, con el `WorldChunkManager` compartido y sin fuente de biomas aislada. Además, `IntCache` era un `static` compartido (sin ranura propia del worker). Eran carreras de datos: biomas y chunks corruptos, y corrupción del heap (cuelgues, worker muerto).
- Arreglo (sin commit): se añade `|| PLATFORM_XBOX` en `ChunkProvider.cpp:121` y `ChunkGenerationScheduler.cpp:266/375/424`, y en `IntCache.cpp`/`.h` (ranura del worker, igual que en la Wii).
- Build: `OptiCraft_async.xbe` (+`_720p`) en ClassicXboxLoader. Falta probarla en la consola.

## 28-09 21:55 — Claude: cuelgue en "render" = inversión de prioridad
- El log muestra que el worker ya trabaja (`workerMs=4749`, `adopted=12`), pero el principal se congela en `render` para siempre (y la música sigue).
- Causa: en `XboxCrtShim.cpp`, `std::mutex` (SRW) era un spin lock con `NtYieldExecution`, y la condvar hacía polling con el mismo yield. Ese yield NO cede la CPU a hilos de menor prioridad. Si el worker (`BELOW_NORMAL`) tiene un mutex y es desalojado, el principal gira sin fin.
- Arreglo: 32 yields y después `Sleep(1)` en `AcquireSRWLockExclusive`; `Sleep(1)` en el polling de `SleepConditionVariableSRW`.

## 28-09 22:11 — prueba en la consola real con los dos arreglos (IntCache/Xbox + Sleep(1) en el mutex)
- Unos 13 min SIN cuelgues. El worker trabaja siempre (`workerMs` entre 1.100 y 4.200 por ventana), `sync=0`, `qFull=0`, `popN=0`, `gen` casi 0.
- FPS 50-60. Tirones ≥80 ms: 59 (~4,6/min; la estable: 13/min). Tirones ≥50 ms: 390, y casi todos son `chunkUnload` (mediana 29 ms, máx. 94). `save` hasta 58 ms.
- Siguiente paso: repartir o abaratar `chunkUnload` (`unload100OldestChunks` y el guardado de cada chunk descargado).

## 28-09 22:30 — Claude: revisión de todo lo de agy (3b9c8a8..HEAD), rama xbox-optimizacion
- `db75cab`: el vigilante colgaba al Salir (join del hilo estático en `exit()`). Ahora se detiene al cerrar.
- `e3b4077`:
  - el mallado vuelve a ESPERAR las columnas vecinas, como la estable (agy lo quitó, y los bordes quedaban con aire y luz mala sin rehacerse). El planificador ya da un solo intento por frame, así que no frena;
  - `asyncRequestedChunks` podía dejar un chunk en blanco para siempre: se reintenta si el worker ya no lo tiene;
  - el `try_lock` fallido ya no cuenta como "en cola";
  - `delay(1)` solo en Xbox (antes también en Wii y PC);
  - el vigilante ya no rompe la compilación de `xbox-public`, PC ni Wii (antes, con `MC_LOG_LEVEL=0`, no compilaba).
- Las dos compilan: `xbox-release` y `xbox-public`.
