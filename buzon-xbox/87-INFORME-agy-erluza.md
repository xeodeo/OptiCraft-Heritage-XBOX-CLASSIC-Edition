# 87. Informe: port selectivo de arreglos y optimizaciones desde `erluza/main` (1.5.2 y 1.2.5)

- **Rama 1.5.2**: `port152-erluza` en `wt-152-rend` (base `ce80a3b` → final **`1eef258`**, 18 commits).
- **Rama 1.2.5**: `opti30-erluza` en `wt-opti30` (base `fd2d3a0` → final **`e728cf6`**, 18 commits).
- **Reglas respetadas**: sin `reset --hard`, sin `push`, sin tocar otras ramas/carpetas ni el lanzador, sin tocar modo Creativo ni ninguna generación de terreno/semillas (`BiomeDecorator`, `ChunkProviderGenerate*`, `MapGen*`, ruido = 0 cambios), preservando `XBOX_CHUNK_TICK_SCHEDULER`, niebla/cielo de Xbox, aldeanos, Wither y `PLATFORM_RELEASE_OLD_WORLD_BEFORE_PORTAL`, y sin alterar PC/PS2/Wii.

---

## 1. Tabla completa de auditoría (Grupos A, B y C)

### Grupo A — Arreglos de cuelgues y estabilidad

| Hash `erluza` | 1.5.2 (`port152-erluza`) | 1.2.5 (`opti30-erluza`) | Detalle / motivo |
|---|---|---|---|
| `fe87a9d` | **Aplicado** (`0c1ca9e`) | **Aplicado** (`c6fbbca`) | Limpia `ChunkProvider::lastChunk` de nuevo tras `chunk->onChunkUnload()` antes de `delete chunk` (evita UAF/SIGSEGV cuando `onChunkUnload` invalida tile entities y repuebla la caché) + guarda `chunk != nullptr` en `World::getBlockId`. |
| `1c58d9b` | **Aplicado** (`59cff9e`) | **Aplicado** (`8573a70`) | Solo la parte genérica del borde de mundo limitado: `ChunkProvider::chunkExistsForPopulate`, `World::chunkExists` fuera de límites en Overworld limitado, y resorte/deslizamiento suave en `Entity::moveEntity` para evitar bloqueos diagonales en el borde. Descartado lo exclusivo de 3DS (`shutdownFinalize`, auto-salto `CTR_PLATFORM`). |
| `b134181` | **Aplicado** (`4b8fb59`) | **Aplicado** (`621e75d`) | **Solo** persistencia de ediciones al descargar chunks: marca `isModified = true` (y `runtimeSaveRequired` bajo `PLATFORM_SAVE_RUNTIME_CHUNK_EDITS_ON_UNLOAD`) en `Chunk::setChunkBlockTileEntity` / `removeChunkBlockTileEntity` / `setChunkModified`, y guarda chunks modificados al descargar en `ChunkProvider` y `ChunkProviderLoadOrGenerate`. GUI multijugador descartada. |
| `c2e3c1b` | **Aplicado** (`87fe61a`) | **Aplicado** (`32d1a57`) | Libera `EntityPlayerSP` de forma diferida (`Minecraft::playersToDelete` + `flushDeferredPlayers()` en `~Minecraft`, `shutdownMinecraftApplet` y `runTick`; `thePlayer2` diferido en `changeWorld`) y protege `World::updateEntities` contra índice obsoleto (`l >= loadedEntityList.size() \|\| loadedEntityList[l] != entity`) tras portales. |
| `92f557e` | **Aplicado** (`4c584fb`) | **Aplicado** (`393f4df`) | Desactiva la barrera horizontal de pegado en bordes de chunks en carga en `#if !PLATFORM_PS2 && !PLATFORM_XBOX` (manteniendo la protección vertical contra caer al vacío) y preserva `entryViewEntity` en `EntityRenderer::renderSplitScreen`. |
| `9344c39` | **Aplicado** (`206d93d`) | **Aplicado** (`06c5796`) | **Solo** `WorldClient.cpp`: corrige el contrato de propiedad de `WorldClient::entityJoinedWorld` cuando falta la columna (no dejar punteros colgantes en `knownEntities`/`entitySpawnQueue` para entidades efímeras sin dueño). |
| `a1772d5` | **Aplicado** (`483981c`) | **Aplicado** (`c1440ee`) | Parte genérica en `Minecraft::usePortal`: reinicia interpolación del jugador (`prevPos*`, `lastTickPos*`, `prevRotation*`), balanceo de brazo (`prevRenderArmPitch/Yaw`), `timeInPortal`/`prevTimeInPortal`/`timeUntilPortal`, velocidad y `fallDistance` tras cambiar de dimensión. (En 1.2.5 se mantiene `Teleporter().placeInPortal` en lugar de `portalTeleporter`). |
| `423bb61` | **Aplicado** (`21be5a1`) | **Aplicado** (`d5c5711`) | Restringe el clamp de mundo limitado en `Entity::moveEntity` solo al Overworld (`worldType == 0`) y centra la aparición en la plataforma de obsidiana del End en `Teleporter::placeInPortal` (`100.5, 50 + yOffset, 0.5`, `fallDistance = 0`). |
| `f7666dd` | **Aplicado** (`e669445`) | **Aplicado** (`f87fa8e`) | Cierra pantallas activas al usar portal, limpia `playerTag` obsoleto del destino, devuelve al jugador del End al spawn del Overworld con `preparePlayerToSpawn()`, permite chunks del Nether/End en mundos limitados (`ChunkProvider::canChunkExist`/`provideChunk`), y protege `World::spawnPlayerWithLoadedChunks` ante NBT de otra dimensión. **Se preservó `#if PLATFORM_RELEASE_OLD_WORLD_BEFORE_PORTAL`** (crítico en Xbox 64 MB). |

### Grupo B — Rendimiento y nuevas opciones

| Hash `erluza` | 1.5.2 (`port152-erluza`) | 1.2.5 (`opti30-erluza`) | Detalle / motivo |
|---|---|---|---|
| `c2b468f` | **Aplicado** (`8045e2d`) | **Aplicado** (`066f5c2`) | Solo la parte de lag por TNT: expulsión en anillo O(1) (`fxEvictCursor[4]`) y borrado O(1) swap-and-pop de partículas muertas en `EffectRenderer`, elimina reserva inútil `blockList` en `Explosion::doExplosionA`, y en `World::entityJoinedWorld` usa `isLoadedEntityPointer()`. **Bug extra corregido al portar**: `trackLoadedEntityPointer`/`untrackLoadedEntityPointer` no actualizaban `loadedEntityPointerSetListSize`, causando reconstrucciones O(N²) del set en cada spawn. (Las partes de arco/`EntityLiving`/`EntityArrow` se auditaron aparte en el Grupo C). |
| `10bccb6` | **Aplicado** (`9ffb62a`) | **Aplicado** (`cc80397`) | Añade `src/platform/world/EntityOcclusionCache.h/.cpp` (adaptado a 8 bits de Y para soportar Y=0..255 de 1.5.2, `kNever = 1.0e30` finito, exención de pistones y en 1.5.2 también `Block::beacon`, más guarda `entity1 != renderViewEntity && !ignoreFrustumCheck`) y `PLATFORM_FANCY_LEAVES_FACE_CULLING` en `RenderBlocks::shouldRenderFace`. **Bug de erluza corregido al portar**: en erluza el culling de hojas Fancy iba después de `usesDefaultFaceCullingLookup` (código muerto porque `BlockLeavesBase` devuelve `false`); se colocó antes para que surta efecto. Activado solo en `XboxTuning.h` (`PlatformGameTuning.h` queda a `0` en PC/PS2/Wii). Descartados caché de display lists de `FontRenderer` de PC, throttle unfocused y tuning 3DS/Wii. |
| `5019d11` | **Ya estaba / Descartado** | **Ya estaba / Descartado** | Era una copia 3DS-only del fast-path de cubos simples que Xbox ya tiene activo mediante `PLATFORM_INCREMENTAL_TERRAIN_BUILD` (`WorldRendererPcLegacy.cpp`). Partes 3DS/QR descartadas. |
| `80563df` | **Aplicado** (`bea39a8`) | **Aplicado** (`b7cae2f`) | Coalescencia de marcas de malla en cascadas de pistones (`World::PistonMarkScope`, `BlockPistonBase::playBlock`, `RenderGlobal::markRenderersInRange`) y omisión de recálculo de luz solar no-op en `Chunk::setBlockIDWithMetadata` si `lightOpacity` no cambió. En **1.2.5** además se aplicó el arreglo de doble marcado en `BlockRedstoneWire.cpp` (`setBlockMetadata` en lugar de `setBlockMetadataWithNotify`); en **1.5.2** ya usaba `setBlockMetadataWithNotify152(..., 2)`. Hacks de cielo/niebla de 3DS descartados. |
| `42aecc9` | **Aplicado** (`243bd6d`) | **Aplicado** (`119531d`) | Parte útil de `EntityRenderer::renderRainSnow`: activa `Ps2WeatherMath.h` en `PLATFORM_XBOX` y protege `rainXCoords`/`rainYCoords` contra `0.0f / 0.0f = NaN` en la columna de la cámara (`length > 0.0f`). En 1.2.5 se aseguró `#if PLATFORM_FLOAT_VERTEX_MATH` para no introducir `double` en PS2. Flips verticales 3DS de brújula/reloj/RenderEngine descartados. |
| `031c115` | **Aplicado solo sliders** (`57f3397`) | **Aplicado solo sliders** (`3873985`) | Solo el filtro del centinela `-10000` en `GuiSlider` y `LegacyOptionSlider`. **Todo lo de generación de terreno (`BiomeDecorator`, `ChunkProviderGenerate*`, `PopulateIncremental`) fue descartado** por Trampa 1 (cambiaba terreno/semilla). Creativo y `DsInput` descartados. |
| `ec3921d` | **Ya estaba / Descartado** | **Ya estaba / Descartado** | Xbox ya cuenta con `PLATFORM_MESH_BUDGET`, `PLATFORM_THROTTLE_ENTITY_AI`, `PLATFORM_CACHE_NEAREST_PLAYER`, `PLATFORM_RANDOM_SPLIT_MULTIPLY`, `PLATFORM_FAST_REGION_COMPRESSION`, `PLATFORM_ENTITY_CHUNK_RETENTION` y `loadedEntityPointerSet`. Los recortes de radio/populate eran exclusivos de PSP y se descartaron. |
| `047e42c` | **Aplicado** (`bbda1e5`) | **Aplicado** (`08a0294`) | Respeta `Config::getPreloadedChunks() * 250L` como presupuesto de calentamiento en `Minecraft::changeWorld` (inactivo con el valor por defecto `0`). |
| `07a80e1` | **Aplicado** (`de52c95`) | **Aplicado** (`e865e9e`) | Acota `invalidateBlockReceiveRegion` y `markBlocksDirty` en `NetClientHandler::handleMapChunk` al rango vertical real del bitmask `packet->yChMin & 0xffff`. |
| `9024ff6` | **Aplicado** (`dcb2213`) | **Aplicado** (`828d720`) | Opción **Hielo opaco / Opaque Ice** (`ofOpaqueIce`, OFF por defecto) en **ambas interfaces**: Java (`GuiPerformanceSettingsOF`) y Legacy (`LegacyAdvancedVideoOptions` en 1.5.2; `LegacyVideoOptions` con paginación automática si supera `legacyOptionsMaxRows` en 1.2.5). **Bug extra corregido al portar**: `PcLegacyBlockRenderInfo` cacheaba `renderPass`/`opaqueCube` una sola vez al arrancar; se añadió `pcLegacyInvalidateBlockRenderInfo()` llamado desde `GameSettings::invalidateChunkMeshes()`. |
| `6790c69` → `221cfce` → `f1ad348` | **Aplicado** (`1eef258`) | **Aplicado** (`e728cf6`) | Perfil opcional **Optimizaciones Pocket / Pocket Optimizations** (`PocketOptimizations.h/.cpp`, **OFF por defecto en Xbox** para no alterar la calidad base salvo que el usuario lo active), accesible en **ambas interfaces** (Java `GuiPerformanceSettingsOF` y Legacy `LegacyAdvancedVideoOptions` / `LegacyVideoOptions`). Conecta límites de partículas/lluvia, rango de entidades/tile entities (eximiendo `Block::beacon` en 1.5.2), `lightmapUpdateInterval`, `randomDisplayProbes`, `GuiParticle` y streaming de chunks. |

### Grupo C — Jugabilidad (Arco)

| Hashes `erluza` | 1.5.2 (`port152-erluza`) | 1.2.5 (`opti30-erluza`) | Motivo |
|---|---|---|---|
| `c889ee7`, `8f5048f`, `d6c7d71`, `7be0217`, `94d8924`, `d31a4aa` | **Descartado** | **Descartado** | Comparado línea por línea con `mcp-diff/src152` y `mcp-diff/src125`: nuestros `ItemBow.cpp`, `EntityArrow.cpp`, `EntityPlayer::getEyeHeight()` (`0.12f`) y `Minecraft::runTick` ya coinciden con Java 1.5.2 y 1.2.5. Además Xbox usa la tabla de senos completa de 65536 entradas (`PLATFORM_SIN_TABLE_BITS == 16`), sin el error de cuantización de 64 entradas que motivó esos parches en PS2. |

---

## 2. Grupo E — Elementos solo apuntados para decisión del usuario

Quedan anotados sin aplicar para que el usuario decida si los quiere en un futuro encargo:
- `4cbab3a` — Panorama nocturno en Legacy UI.
- `6277a95` — Reproductor de música.
- `48a4a97` — Coordenadas estilo Bedrock.
- `720f795` — Contador de FPS con fondo.
- `b71d0e8` — Menú de pausa OG con puntuación.

---

## 3. Compilación y mediciones antes / después (xemu 64 MB, semilla `1621073931`)

### Estado de compilación (480p y 720p)
- **1.5.2 (`wt-152-rend`, `port152-erluza` `1eef258`)**:
  - `xbox-release`: compila limpio (`default.xbe`, `OptiCraft.xbe`, `OptiCraft_720p.xbe`, `OptiCraft.iso`). **0 avisos nuevos de `SSE2 left in`**.
  - `xbox-release-exp`: compila limpio (`default.xbe`, `OptiCraft_exp.xbe`, `OptiCraft_exp_720p.xbe`, `OptiCraft.iso`). **0 avisos nuevos de `SSE2 left in`**.
- **1.2.5 (`wt-opti30`, `opti30-erluza` `e728cf6`)** mediante `buzon-xbox\build_opti30.ps1`:
  - `xbox-release`: compila limpio (480p + 720p + `OptiCraft.iso`). **0 avisos nuevos de `SSE2 left in`**.
  - `xbox-release-exp`: compila limpio (480p + 720p + `OptiCraft.iso`). **0 avisos nuevos de `SSE2 left in`**.
  - `ps2-release`: compila limpio (`bin/ps2/OptiCraft.elf`), verificando que no se rompió PS2.

### Arranques limpios en xemu 64 MB (`xemu64.toml`)
Todas las ISOs arrancaron limpias con `xbox.selftest: done: 0 failure(s)`, `double53=1`, **0 `nan`** y **0 `[X87]`**:
- **1.5.2 `xbox-release` (`1eef258`)**: menú `frame=901` → `freeKB=41140` (vs `41128` en `ce80a3b`, **+12 KB**).
- **1.5.2 `xbox-release-exp` (`1eef258`)**: menú `frame=901` → `freeKB=41068`.
- **1.2.5 `xbox-release` (`e728cf6`)**: menú `frame=901` → `freeKB=42140`.
- **1.2.5 `xbox-release-exp` (`e728cf6`)**: menú `frame=901` → `freeKB=42124`.

### Prueba en partida antes vs después (xemu 64 MB, semilla `1621073931`)
El driver temporal (eliminado antes de la compilación final limpia) creó el mundo `erluza_test` con semilla `1621073931`, comprobó spawn y terreno, colocó y rompió un bloque, caminó 60 ticks, detonó 8 TNT simultáneas, viajó Overworld → Nether → Overworld → End → Overworld, guardó y recargó el mundo:

#### 1.5.2 (`ce80a3b` antes vs `1eef258` después)
| Métrica | Antes (`ce80a3b`) | Después (`1eef258`) | Observación |
|---|---|---|---|
| Spawn semilla `1621073931` | `(-189.50, 65.62, 42.50)`, chunk `(-12, 2)` | `(-189.50, 65.62, 42.50)`, chunk `(-12, 2)` | Idéntico; 0 archivos de worldgen tocados |
| Overworld inicial (`xbox.perf`) | `41.4 fps` (`24.2 ms`), `ent=2.6 ms`, `opaque=6.7 ms`, `hud=0.1 ms` | **`46.6 fps`** (`21.5 ms`), **`ent=0.1 ms`**, **`opaque=4.9 ms`**, `hud=0.1 ms` | Mejora clara en entidades (`EntityOcclusionCache`) y opacos (hojas Fancy) |
| Overworld tras recargar (`xbox.perf`) | `33.9–41.9 fps` (`23.9–29.5 ms`), `ent=0.3–1.3 ms` | **`55.0–57.4 fps`** (`17.4–18.2 ms`), **`ent=0.6–1.0 ms`**, `hud=0.1–0.2 ms` | Rendimiento superior, HUD 0.1–0.2 ms |
| Memoria (`freeKB`, 64 MB) | Menú `41128`, post-portales `35536`, post-recarga `34764` | Menú **`41140`**, post-portales **`36536`**, post-recarga **`35172`** | Igual o mejor (+408 a +1000 KB libres en partida) |
| 8× TNT simultáneas (`xbox.ent` / `25 ticks`) | `entTick=568.0 ms` (8 TNT `556.3 ms`), `2141 ms`, `ent=4.2 ms` | `entTick=582.0 ms` (8 TNT `570.3 ms`), `2212 ms`, **`ent=1.5 ms`** | Con 8 TNT (~800 partículas < límite 4000) el pico de `doExplosionA` es equivalente y el render de entidades cae de `4.2` a `1.5 ms`; el beneficio grande de `c2b468f` (anillo O(1) + swap-and-pop) evita bloqueos O(N²) en cadenas masivas >4000 partículas |
| Entrada al End | `(100.00, 53.62, 0.00)` (esquina) | **`(100.50, 51.62, 0.50)`**, `minY=50.00`, `belowId=49` (obsidiana) | Centrado en plataforma de obsidiana |
| **Vuelta del End al Overworld** | **ROTO**: `(100.00, 52.45, 0.00)` (caía en coords del End en Overworld hasta `49.75`) | **ARREGLADO**: `(-189.50, 65.62, 42.50)` (spawn real del Overworld) | Arreglo grave verificado en vivo |
| Guardar y recargar | OK (en coords rotas `100, 49.75, 0`) | **OK** (`dim=0 pos=(-189.50, 65.62, 42.50)`) | Sin `nan`, sin `[X87]`, `double53=1` |

#### 1.2.5 (`fd2d3a0` antes vs `e728cf6` después)
| Métrica | Antes (`fd2d3a0`) | Después (`e728cf6`) | Observación |
|---|---|---|---|
| Spawn y terreno T0 (`1621073931`) | `(43.50, 49.62, 55.50)`, chunk `(2, 3)`, **hash FNV-1a 3×3 chunks = `b8dbea09`** | `(43.50, 49.62, 55.50)`, chunk `(2, 3)`, **hash FNV-1a 3×3 chunks = `b8dbea09`** | **Terreno idéntico bit a bit** |
| Overworld inicial (`xbox.perf`) | `25.4–29.7 fps`, `opaque=4.8–5.4 ms`, `hud=0.1 ms` | `25.2–27.5 fps`, `opaque=2.8–5.0 ms`, `hud=0.1 ms` | Equivalente en el arranque de chunks |
| Overworld tras recargar (`xbox.perf`) | `40.3–44.5 fps` (enterrado en coords rotas del End `100, 49.85, 0`, `entities=6.7–7.7 ms`) | **`46.5 fps`** pico (`21.5 ms`, `ent=0.0 ms`) y `35.6–37.1 fps` en superficie real de spawn (`entities=9.2–10.5 ms`, `ent=0.1 ms`, `hud=0.1 ms`) | En `antes` el jugador estaba atrapado bajo tierra en `(100, 50, 0)` con menos entidades; `después` vuelve al spawn real con `ent=0.1 ms` |
| Memoria (`freeKB`, 64 MB) | `frame=901`: `36980`, `frame=1501`: `40312`, `frame=2401`: `30040` | `frame=901`: `36604`, `frame=1501`: `39120`, `frame=2401`: **`31724`** | **+1684 KB libres** al estabilizarse tras recarga |
| 8× TNT simultáneas (`xbox.ent` / `25 ticks`) | `entTick=173.3 ms` (8 TNT `172.5 ms`), `1586 ms` | `entTick=179.7 ms` (8 TNT `178.8 ms`), `1656 ms` | Equivalente por debajo de 4000 partículas; evita cuelgue O(N²) en cadenas masivas de TNT |
| Entrada al End | `(100.00, 53.62, 0.00)` (esquina) | **`(100.50, 51.62, 0.50)`**, `minY=50.00`, `belowId=49` | Centrado en plataforma de obsidiana |
| **Vuelta del End al Overworld** | **ROTO**: `(100.00, 52.45, 0.00)` → recarga en `(100.00, 49.85, 0.00)` | **ARREGLADO**: `(43.50, 49.62, 55.50)` → recarga en `(43.50, 49.62, 55.50)` | Arreglo grave verificado en vivo |
| Estabilidad FPU / NaN | `double53=1`, 0 `nan`, 0 `[X87]` | `double53=1`, 0 `nan`, 0 `[X87]` | Limpio |

> **Nota sobre capturas de pantalla en esta sesión**: el entorno headless donde corre el agente devuelve `Win32Exception` en `CopyFromScreen` y cuadros en negro con `PrintWindow` sobre ventanas OpenGL de xemu (y el QMP de esta build de xemu no incluye `screendump`). Por ello la paridad de terreno se verificó mediante **0 cambios en ficheros de worldgen**, **coordenadas de spawn exactas** en ambas ramas y **hash FNV-1a idéntico bit a bit (`b8dbea09`)** de los 3×3 chunks alrededor del spawn al tick 0 en 1.2.5.

---

## 4. Qué probar en la Xbox real

1. **Verificación visual de semilla `1621073931` (1.5.2 y 1.2.5)**: crear mundo con semilla `1621073931` y confirmar visualmente que el paisaje, árboles y spawn coinciden con la versión anterior.
2. **Hojas en gráficos Detallados (Fancy)**: caminar por un bosque denso y confirmar que los árboles se ven bien (sin agujeros raros en bordes de hojas) y con mejor fluidez por el culling interior.
3. **Oclusión de entidades y tile entities (`EntityOcclusionCache`)**: comprobar en zonas con muchos cofres/carteles/mobs detrás de paredes (y en tercera persona / con jefes o balizas en 1.5.2) que nada visible parpadee ni desaparezca por error.
4. **Opciones nuevas en las DOS interfaces (480p y 720p)**:
   - **Hielo opaco (`Opaque Ice`)**: activar/desactivar en Java (`Ajustes de rendimiento`) y en Legacy UI (`Vídeo avanzado` en 1.5.2; `Vídeo` —con paginación automática en 480p— en 1.2.5). Comprobar que el hielo cambia entre translúcido y opaco al instante sin salir del mundo y se guarda al reiniciar.
   - **Optimizaciones Pocket (`Pocket Optimizations`)**: comprobar que viene **desactivada por defecto**, que se puede activar/desactivar desde Java y Legacy UI, y que al activarla mejora FPS en zonas cargadas o con lluvia.
5. **Portales Nether y End (en mundos infinitos y limitados)**: entrar al Nether y volver; entrar al End (verificar aparición centrada sobre la plataforma de obsidiana), volver al Overworld (verificar regreso al spawn del Overworld, no a `(100, 50, 0)`), guardar y recargar.
6. **Cadena grande de TNT** (p. ej. 20–40 TNT encadenadas): confirmar que ya no hay congelación larga por limpieza de partículas y que la consola de 64 MB no se queda sin RAM.
7. **Mundos limitados y persistencia**: caminar contra el borde del mundo (incluso en diagonal) para verificar que desliza sin congelarse; abrir/modificar cofres/hornos, alejarse para descargar el chunk y volver (o guardar/recargar) para confirmar que los cambios se conservaron.
8. **Redstone y pistones (especialmente en 1.2.5)** y **lluvia/nieve**: probar un reloj de redstone o pistones en cadena y activar lluvia para verificar fluidez y ausencia de triángulos corruptos (`NaN`) sobre la cámara.
