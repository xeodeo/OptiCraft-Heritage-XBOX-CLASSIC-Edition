# 76. Rendimiento 1.5.2: ticks por chunk, texturas animadas y HUD Legacy (09-11-2026)

Rama `port152-rendimiento` en `wt-152-rend` (sobre `port152-aldeanos` 09e64cb). Compilan las 4 builds; NO copiadas al
lanzador ni probadas en consola/xemu. XBE en `wt-152-rend\bin\xbox\iso\` y `wt-152-rend\bin\xbox-exp\iso\`.
**Estas builds llevan netlog** (`MC_LOG_LEVEL=1`, `XBOX_NETLOG_HOST=192.168.0.18`, como la 1.2.5). Las de
`wt-152-pociones` no lo tenían (`MC_LOG_LEVEL=0`).

## Commits
| hash | qué |
|---|---|
| `6f1fcfa` | Ticks programados por chunk en Xbox (`XBOX_CHUNK_TICK_SCHEDULER` en `xbox/XboxTuning.h`): se usa el `PcLegacyTickScheduler` de PC. |
| `973d483` | Texturas animadas: no se sube un tile igual al anterior; swizzle solo del rectángulo cambiado (8x4); `LockRect` con `D3DLOCK_NOOVERWRITE` en las subidas parciales; netlog `xbox.tex`. |
| `2a6976a` | HUD Legacy: los iconos de la fila de botones en un solo draw y su texto teselado una vez (la caché de PS2, ahora también Xbox). |
| `0e2c0b8` | Arreglo de compilación (declaración adelantada en `World.h`). |

## 1. Ticks por chunk
- Antes, guardar/descargar un chunk copiaba y recorría TODOS los ticks pendientes del mundo (`World::getPendingBlockUpdates`).
  Ahora cada chunk tiene su lista (idea de `NextTickHashSet` de OptiFine).
- Mismo comportamiento: mismo orden (tiempo, prioridad, id de entrada), misma regla de duplicados, mismo consumo de ids.
- El planificador de PC estaba atrasado respecto a la 1.5.2: `func_82740_a` (prioridades de repetidor y comparador) e
  `isBlockTickScheduled` iban a los contenedores viejos y con el planificador activo **no se habrían ejecutado nunca**.
  Ahora pasan por el planificador (esto arregla también el PC-legacy, que no he compilado).
- Única diferencia: los ticks de un chunk se guardan ordenados por tiempo (como el TreeSet de Java), no en orden de hash.
  Al recargar, los empates se ejecutan en el mismo orden que antes de guardar.
- Prueba: redstone con repetidores y comparadores, agua y lava fluyendo, guardar y cargar con circuitos en marcha.

## 2. Texturas animadas
- `RenderEngine::uploadTextureFxTile` guarda por efecto el último tile subido (1 KB) y se salta el `memcmp` igual.
  Se borra al re-preparar cualquier atlas (`setupTexture`), al cambiar la lista de efectos o si el tile lo escribe otro
  camino (animación personalizada).
- Corrige además algo que el informe 75 no vio: con la animación apagada, el **fuego y el portal también se subían cada
  tick** (`updateStaticProceduralTextureFx` devuelve `true` siempre). Brújula y reloj igual.
- D3D (`RenderAPI_D3D8_XBOX.cpp`): `renderTextureSubImageRgba` guarda el rectángulo sucio; `swizzleUpload` hace
  `XGSwizzleRect` parcial (alineado a 8x4) y bloquea con `D3DLOCK_NOOVERWRITE`. Riesgo: un tile a medio cambiar durante
  1 frame (invisible en animaciones). Texturas no potencia de 2 o recién creadas siguen con la subida entera.

## 3. HUD Legacy
- Los ~5 MB del menú Legacy (panorama, título, logo) **ya se liberaban** al empezar a jugar
  (`ClientPlatformPolicy_XBOX.cpp: releaseWorldEntryAssets`, llamado en `Minecraft.cpp` al crear el mundo y otra vez al
  cerrar la última pantalla). El informe 75 se equivocó ahí. Las pantallas Legacy dentro de la partida usan degradado,
  no el panorama. No he cambiado nada de eso.
- Iconos de la fila: `drawControlIcons` (nuevo, `ControlIcon.cpp`) agrupa los del mismo atlas en un draw.
- Texto de la fila: se tesela una vez y se repite (`LEGACY_CACHE_HINT_TEXT`); en Xbox la ruta de mallas capturadas ya
  se usaba para el terreno.

## 4. No hecho (informe 74, puntos 2-5)
- Objetos tirados planos (`ofDroppedItems`) y partículas de poción apagables: necesitan opción nueva en el menú,
  guardado y textos en 58 idiomas. No es "barato y seguro" para meterlo a ciegas.
- Frustum "entero dentro": nuestra visibilidad va por `PcLegacySectionVisibility`; hay que medir antes (`visibility=`).
- `LongHashMap` más grande: puede cambiar el orden en que se recorren chunks. No tocado.
- Presupuesto adaptativo de mallas (`WrUpdaterSmooth`): falta comparar con `PcLegacyMeshScheduler`.

## Cómo medir (netlog)
Receptor: `python -u xbox-spike\escuchar_log.py` (UDP 9999 → `xbox-spike\netlog.txt`); el PC debe tener la IP
192.168.0.18. Un minuto quieto y un minuto andando por cada caso:
1. **Animaciones apagadas / encendidas**: `xbox.tex uploads per s=... (partial ...) | per frame lock=... swizzle=...` y
   `xbox.draw ... textureStage=`. Apagadas debería quedar casi en 0 subidas por segundo (solo al girar la brújula).
2. **Legacy UI encendido / apagado**: `xbox.perf ... hud=` y `freeKB`.
3. **Ticks**: contar `xbox.spike` con `worldTick>chunkUnload` y `autosave` andando por terreno nuevo, frente a la build
   anterior.

## Revisión
Como fork no podía lanzar un subagente de auditoría; revisé yo: dueño de las entradas de tick (el guardado pide
`remove=false`), punteros del planificador (se crea al primer tick y se borra en `~World`), invalidación de la caché de
tiles y el camino parcial de swizzle (solo con copia CPU sin re-muestreo y textura ya creada). Conviene una auditoría
aparte antes de pasarlo a la consola.

## Decisión del usuario (09-11)
Objetos tirados planos (ofDroppedItems) y apagar partículas de pociones: NO se hacen. Se quedan como están.

## 09-11 agy

Rama `port152-rendimiento` en `wt-152-rend` hasta `bf9ac4e` (6 commits sobre `ff8a92f`):
- `4a6a27c`: `Separate WorldInfo totalTime from worldTime for scheduled ticks`
- `0af4150`: `Match Java 1.5.2 scheduled tick batching, radius 0, and associated block IDs`
- `c8a37ad`: `Persist TileTicks priority, 2-block chunk border margin, unload cleanup, and O(1) chunk detach`
- `b092688`: `Keep up to 4 separate dirty rectangles for partial texture uploads on Xbox`
- `cd67261`: `Do not purge or force-save pending ticks on chunk unload`
- `bf9ac4e`: `Implement BlockFlowing and BlockFire func_82506_l and use range 0 in func_82740_a`

### Cambios por punto (Encargo 78 + Revisión 78b)
1. **`isBlockTickScheduled` sobre el lote en ejecución (`field_94579_S`)**:
   - `src/net/minecraft/src/World.h:522` y `src/net/minecraft/src/World.cpp:277-279,309,356,421,490`: añadido `field_94579_S` (`std::vector<NextTickListEntry *>`) en `World` y su limpieza en constructores y `~World()`.
   - `src/net/minecraft/src/World.cpp:3266-3275`: `World::isBlockTickScheduled` comprueba únicamente `field_94579_S` mediante `entry->equals(&probe)` (con `Block::isAssociatedBlockID`), igual que `WorldServer.java:267-270`.
2. **Drenado previo del lote del tick (`cleaning`) antes de ejecutar (`ticking`)**:
   - `src/net/minecraft/src/World.cpp:5631-5698`: `World::TickUpdates` drena primero hasta 1000 entradas vencidas (`flag || entry->scheduledTime <= worldInfo->getWorldTotalTime()`) a `field_94579_S` (tanto en la ruta `WORLD_CHUNK_TICK_SCHEDULER` como en `#else`) y después recorre `field_94579_S`, poniendo `field_94579_S[i] = nullptr` justo antes de ejecutar cada entrada para que un tick con retardo 0 programado durante `updateTick` no se ejecute en el mismo tick ni `isBlockTickScheduled` vea entradas ya consumidas.
3. **Chunk no cargado con radio 0 y rama inmediata (`func_82506_l`)**:
   - `src/net/minecraft/src/World.cpp:5676-5695`: en `World::TickUpdates`, `RANGE = 0` (`checkChunksExist(x, y, z, x, y, z)`). Si el chunk no está cargado se llama a `scheduleBlockUpdate(entry->xCoord, entry->yCoord, entry->zCoord, entry->blockID, 0)` como en `WorldServer.java:394` (en la práctica, mientras el chunk siga descargado, `func_82740_a` vuelve a comprobar radio 0 y también lo descarta, igual que en Java).
   - `src/net/minecraft/src/World.cpp:3261-3301`: `scheduleBlockUpdate` delega en `func_82740_a(..., 0)` con `RANGE = 0` tanto en la rama inmediata como en la normal (`WorldServer.java:280,296`).
   - `src/net/minecraft/src/BlockFlowing.h:11`, `src/net/minecraft/src/BlockFlowing.cpp:214-217`, `src/net/minecraft/src/BlockFire.h:25` y `src/net/minecraft/src/BlockFire.cpp:374-377`: añadidos los overrides `func_82506_l()` que devuelven `false` (como en `BlockFlowing.java:280` y `BlockFire.java:157`), haciendo que los manantiales durante la generación (`scheduledUpdatesAreImmediate`) se programen con `delay = 1` en lugar de ejecutarse de inmediato.
4. **Comparación con `Block::isAssociatedBlockID`**:
   - `src/net/minecraft/src/World.cpp:5685`: `World::TickUpdates` compara `Block::isAssociatedBlockID(blockId, entry->blockID)` en lugar de `blockId == entry->blockID`.
   - `src/net/minecraft/src/BlockRedstoneTorch.h:22` y `src/net/minecraft/src/BlockRedstoneTorch.cpp:160-163`: añadido `BlockRedstoneTorch::isAssociatedBlockID` (`torchRedstoneIdle` y `torchRedstoneActive`), como en `BlockRedstoneTorch.java:199-201`.
5. **`getWorldTotalTime` / `getTotalWorldTime` frente a `getWorldTime`**:
   - `src/net/minecraft/src/WorldInfo.h:36,45,89` y `src/net/minecraft/src/WorldInfo.cpp:34-38,81,116,141,224-225,270,279`: separado `totalTime` (NBT `"Time"`, `getWorldTotalTime()`, `incrementTotalWorldTime()`) de `worldTime` (NBT `"DayTime"`, con fallback a `totalTime` en mundos viejos sin `"DayTime"`).
   - `src/net/minecraft/src/World.h:447-449` y `src/net/minecraft/src/World.cpp:5070,5110-5131,6387-6400`: `World::tick()` incrementa `totalTime` y `worldTime` por separado (`WorldClient.cpp:128` también). Dormir, `/time set`, TMI, el salto de tiempo debug y los modos Día/Noche de OptiFine solo modifican `worldTime`.
   - Ticks programados y marcas monotónicas pasados a `getWorldTotalTime()` / `getTotalWorldTime()`: `src/net/minecraft/src/World.cpp:3304,3337,5650,5658`, `src/net/minecraft/src/AnvilChunkLoader.cpp:304,406`, `src/net/minecraft/src/ChunkLoader.cpp:403`, `src/net/minecraft/src/Chunk.cpp:1525,1528`, `src/net/minecraft/src/ChunkProvider.cpp:347,1059,1091`, `src/net/minecraft/src/ChunkProviderLoadOrGenerate.cpp:180,367,396`, `src/net/minecraft/src/BlockRedstoneTorch.cpp:25,91`, `src/net/minecraft/src/TileEntityBeacon.cpp:39,175-176`, `src/net/minecraft/src/TileEntityBeaconRenderer.cpp:43` y `src/net/minecraft/src/TileEntityDaylightDetector.cpp:7`.
6. **Guardado y carga de prioridad `"p"` en `TileTicks`**:
   - `src/net/minecraft/src/AnvilChunkLoader.cpp:418,553`: guarda `tick->setInteger("p", entry->priority)` y carga `tag->getInteger("p")` (devuelve `0` si la clave `"p"` no existe en mundos viejos).
   - `src/net/minecraft/src/World.h:330` y `src/net/minecraft/src/World.cpp:3335-3360`: `World::scheduleBlockUpdateFromLoad` recibe `priority = 0` y lo pasa tanto a `pcLegacyTickScheduler->schedule` como a `entry->func_82753_a(priority)`.
7. **Margen de 2 bloques fuera del borde del chunk (`[-2, +18)`)**:
   - `src/pc/world/PcLegacyTickScheduler.cpp:52-92`: `PcLegacyTickScheduler::getPendingForChunk` calcula `minX = (chunkX << 4) - 2`, `maxX = minX + 16 + 2` (igual para Z) e inspecciona las 9 cubetas vecinas 3x3 `(chunkX + dx, chunkZ + dz)`.
   - `src/net/minecraft/src/World.cpp:3362-3415`: `World::getPendingBlockUpdates` usa el mismo rango `[-2, +18)` en `#else` (iterando `scheduledTickTreeSet` en orden) y además recorre `field_94579_S` en ambas rutas, como `WorldServer.java:405-442`.
8. **`remove = true` en `getPendingBlockUpdates` y descarga de chunks como Java (`cd67261`)**:
   - `src/pc/world/PcLegacyTickScheduler.cpp:83-90` y `src/net/minecraft/src/World.cpp:3386-3411`: `getPendingBlockUpdates(chunk, true)` extrae las entradas de los índices y pone `field_94579_S[i] = nullptr` sin dejar punteros colgantes.
   - En `cd67261` se quitó el borrado de ticks en `Chunk::onChunkUnload()` y el guardado forzado en `ChunkProvider::unloadChunk` / `ChunkProviderLoadOrGenerate::unloadChunk`: Java 1.5.2 no borra ticks al descargar un chunk (deja que caduquen al llegar a `TickUpdates`), evitando que el margen `[-2, +18)` borre ticks de un chunk vecino aún cargado o fuerce escrituras a disco al descargar chunks con fuego en el Nether.
9. **`detachFromChunk` en $O(1)$ sin búsqueda lineal**:
   - `src/net/minecraft/src/NextTickListEntry.h:29` y `src/pc/world/PcLegacyTickScheduler.cpp:126-152`: cada `NextTickListEntry` guarda `chunkBucketIndex`; `attachToChunk` asigna el índice al insertar y `detachFromChunk` hace swap-and-pop en $O(1)$ actualizando `last->chunkBucketIndex`.
10. **Rectángulos sucios separados en `RenderAPI_D3D8_XBOX.cpp`**:
    - `src/platform/RenderAPI_D3D8_XBOX.cpp:339-346,452-468,1764-1814`: `Texture` mantiene hasta `MAX_DIRTY_RECTS = 4` rectángulos sucios independientes (uniendo los que se tocan/solapan, como tiles contiguos de agua o lava, sin unir el portal en la fila 0 con la lava en la fila 14) y los sube con `XGSwizzleRect` dentro de un único `LockRect`/`UnlockRect`.

### Qué se probó y qué no
- **Probado**: compilación limpia solo de Xbox — las 4 builds XBE de `build/xbox-release` (`OptiCraft.xbe` y `OptiCraft_720p.xbe` en `wt-152-rend\bin\xbox\iso\`) y `build/xbox-release-exp` (`OptiCraft.xbe` y `OptiCraft_720p.xbe` en `wt-152-rend\bin\xbox-exp\iso\`) con `buzon-xbox\BUILD.lock`; revisión estática frente a `WorldServer.java`, `NextTickListEntry.java`, `AnvilChunkLoader.java`, `BlockRedstoneTorch.java`, `BlockFlowing.java` y `BlockFire.java` de MCP 1.5.2.
- **No probado**: ejecución en xemu/consola real (no se copió nada al lanzador) ni compilación de PS2, Wii o PC-legacy.
