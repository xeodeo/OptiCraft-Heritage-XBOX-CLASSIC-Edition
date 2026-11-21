# Informe 95: Reloj/brújula (Default + LCE, 1.2.5 y 1.5.2) y tirones del dragón del End (1.2.5)

- **Fecha**: 2026-11-21
- **Commits finales**:
  - **1.2.5 (`wt-opti30`, rama `opti30-erluza`)**: `96297410` (sobre `d0bff752`, sin tocar los commits del icono de guardado de Claude; 14 ficheros, `+675/-88`).
  - **1.5.2 (`wt-152-rend`, rama `port152-erluza`)**: `b9336633` (sobre `b73c4655`, sin tocar los commits del icono de guardado de Claude; 9 ficheros, `+273/-33`).
- **Capturas (backbuffer D3D8 640×480)**: `buzon-xbox\95-capturas\` (8 PNG verificados).
- **Reglas respetadas**: sin `git reset`, sin `git push`, sin copiar al lanzador, modo creativo intacto, generación de terreno intacta, `BUILD.lock` retirado, sin sondas temporales en los binarios finales.

---

## 1. Reloj (`Item::pocketSundial`) y brújula (`Item::compass`) — 1.2.5 y 1.5.2

### Causas raíz encontradas
1. **Pack LCE / DLC sin píxeles clave magenta en `/gui/items.png`**:
   - El reloj de Java 1.2.5 (`TextureWatchFX`) busca píxeles clave magenta (`R == B && G == 0`) en el icono `(6,4)` de `/gui/items.png` para sustituirlos por el disco `/misc/dial.png`.
   - En el pack **Legacy Console** (`TitleUpdate/res/items.png`) y en 8 de los 9 packs DLC LCE, el icono `(6,4)` del reloj tiene **0 píxeles magenta** (es un reloj de día estático pintado) y el icono `(6,3)` de la brújula tiene **6 píxeles rojos estáticos** pintados de fábrica, porque LCE/1.5 anima ambos objetos mediante tiras verticales (`/textures/items/clock.png` y `/textures/items/compass.png`), que además `scripts/xbox/make_legacy_texture_packs.py` no copiaba al generar packs.
2. **Recarga ausente al cambiar de paquete de texturas (`RenderEngine::refreshTextures`) — 1.2.5 y 1.5.2**:
   - `RenderEngine::refreshTextures()` nunca llamaba a un recargador en los `TextureFX` registrados ni limpiaba `uploadedTextureFxTiles`.
   - Si el usuario cambiaba entre el pack por defecto y el pack LCE (o cualquier DLC) en caliente, `TextureWatchFX` y `TextureCompassFX` se quedaban congelados con los datos de arranque o con el atlas viejo, y además el mapa de tiles subidos podía saltarse la re-subida al nuevo `/gui/items.png`.
3. **1.5.2 (`wt-152-rend`) tenía fallos reales en brújula y cambio de pack**:
   - En 1.5.2, `TextureWatchFX` ya usaba `/textures/items/clock.png`, pero **no** recargaba la tira al cambiar de pack.
   - `TextureCompassFX` seguía usando el código 1.2.5 leyendo `/gui/items.png` — cuyo icono de brújula en 1.5.2 tiene aguja roja estática pintada (`compass_red=3`) — en lugar de usar la tira de 32 fotogramas `/textures/items/compass.png` que ya trae 1.5.2.
4. **Detección de dimensiones sin ciclo solar (Nether / End)**:
   - En 1.2.5, `TextureWatchFX` y `TextureCompassFX` comprobaban `worldProvider->isNether` en vez de `!worldProvider->func_48217_e()` (que cubre tanto Nether como End).

### Solución aplicada
- **`TextureFX` + `RenderEngine` (1.2.5 y 1.5.2)**:
  - Añadido método virtual `TextureFX::reloadFromTextures(RenderEngine*)` y `RenderEngine::reloadDynamicTextures()` invocado automáticamente al final de `RenderEngine::refreshTextures()`.
  - `RenderEngine::refreshTextures()` limpia ahora `uploadedTextureFxTiles` para garantizar que los tiles dinámicos se vuelvan a subir al nuevo atlas en cuanto se cambia de pack.
  - Verificado que `ofAnimatedItems`, `PLATFORM_DYNAMIC_TEXTURE_INTERVAL_TICKS`, `updateSpecialTextureFx` y `uploadTextureFxTile` (`tileImage == 1`) ya propagaban bien los cambios a la mano, la barra rápida y el inventario en cuanto `TextureWatchFX`/`TextureCompassFX` producían fotogramas distintos.
- **`TextureWatchFX` y `TextureCompassFX` (1.2.5)**:
  - **Modo 1 — Tiras LCE/1.5** (`/textures/items/clock.png` y `/textures/items/compass.png` de cualquier resolución, p. ej. 16×1024 ó 16×512): se cargan y reescalan a 16×16 por fotograma cuando existen en el pack activo. Se verificó la dirección horaria de la tira de brújula (`turns = -watchAngle / (2*PI)` para que 0=Sur, 8=Oeste, 16=Norte, 24=Este coincida exactamente con la aguja 1.2.5).
  - **Modo 2 — Packs LCE/DLC ya instalados en el HDD de la Xbox sin tiras**:
    - En el reloj, si el icono del pack activo no tiene píxeles magenta, se superpone la máscara de 54 píxeles magenta del icono vanilla (`Default`) sobre el bisel del pack activo y se compone el disco giratorio usando `/misc/dial.png` del pack activo. Así los packs LCE ya copiados en la consola real funcionan de inmediato sin necesidad de regenerarlos.
    - En la brújula, si el icono del pack activo trae aguja roja estática pintada, se limpian esos píxeles rojos rellenándolos con el fondo gris circundante antes de dibujar la aguja móvil, evitando la «doble aguja» fantasma.
  - **Modo 3 — Pack por defecto / packs 1.2.5 clásicos**: se mantiene el camino exacto de Java 1.2.5 con clave magenta + `/misc/dial.png` y aguja procedural.
  - Actualizada la condición de giro aleatorio en Nether y End a `!worldProvider->func_48217_e()`.
- **`TextureWatchFX` y `TextureCompassFX` (1.5.2)**:
  - `TextureWatchFX` implementa `reloadFromTextures()` con limpieza segura y fallback al modo dial clásico si un pack carece de tira.
  - `TextureCompassFX` implementa lectura de la tira `/textures/items/compass.png` (32 fotogramas, con misma dirección validada) + fallback con limpieza de aguja estática + `reloadFromTextures()` al cambiar de pack.
- **`scripts/xbox/make_legacy_texture_packs.py` (1.2.5 y 1.5.2)**:
  - Añadidas `textures/items/clock.png` y `textures/items/compass.png` a `DIRECT_FILES` para que los futuros packs LCE/DLC generados incluyan también las tiras nativas de alta fidelidad (como el reloj cuadrado de `Plastic`).

### Evidencia en `xemu` 64 MB (1.2.5)
Hashes FNV-1a de los 1024 bytes RGBA de `imageData` al cambiar packs en caliente (`swDef=1 swLce=1 swPlast=1`):

| Pack | Reloj Día (`t=1000`) | Reloj Noche (`t=18000`) | Brújula `yaw=0` | Brújula `yaw=180` |
|---|---|---|---|---|
| **Default** | `810C4115` | `7AFA13BC` | `CCDB0EF2` | `7BA0D9E9` |
| **Legacy Console** (sin tiras, layout actual HDD) | `23D40E91` | `D653B3A0` | `6F9A3124` | `C0CADD5B` |
| **Plastic** (LCE DLC con tiras `clock.png`/`compass.png`) | `0A695388` | `4732F71D` | *(tira activa)* | *(tira activa)* |

Todos los hashes Día≠Noche y Default≠LCE confirman tanto el giro real como la recarga en caliente sin reiniciar.

### Capturas guardadas (`buzon-xbox\95-capturas\`)
1. `01_clock_default_day.png` — Default, día (`t=1000`): reloj con sol y cielo azul en mano y barra rápida; brújula apuntando abajo-derecha.
2. `02_clock_default_night.png` — Default, noche (`t=18000`): reloj con luna y cielo nocturno en mano y barra rápida; brújula girada 180°.
3. `03_clock_default_inv.png` — Default, noche (`t=18000`): inventario abierto mostrando reloj nocturno y brújula girada.
4. `04_clock_lce_day.png` — Legacy Console (sin tira), día (`t=1000`): bisel LCE + dial solar girando en mano y barra; brújula limpia con una sola aguja (sin aguja roja fantasma).
5. `05_clock_lce_night.png` — Legacy Console (sin tira), noche (`t=18000`): dial nocturno con luna en mano y barra; brújula girada 180°.
6. `06_clock_lce_inv.png` — Legacy Console (sin tira), noche (`t=18000`): inventario abierto con reloj nocturno y brújula girada.
7. `07_clock_lce_plastic_strip_night.png` — LCE DLC `Plastic` (con tiras `clock.png` y `compass.png`), noche (`t=18000`): reloj cuadrado Plastic con luna creciente y brújula cuadrada Plastic en mano y barra.
8. `08_end_dragon_combat.png` — El End durante la prueba de 2 minutos: dragón + barra de jefe + pilares/cristales + 40 enderman + reloj y brújula girando en el End.

---

## 2. Tirones del dragón en el End (1.2.5, `wt-opti30`)

### Diagnóstico (perfilado de sub-fases en `xemu` 64 MB con dragón + 40 enderman)
El perfilado por sub-fases dentro de `EntityDragon::onLivingUpdate()` y `World::updateEntityWithOptionalForce()` reveló las causas exactas de los picos de 107–169 ms en la consola real:

1. **Escaneo masivo de fluidos cada tick en `Entity::onEntityUpdate()` (`World::handleMaterialAcceleration` + `World::isMaterialInBB`)**:
   - El AABB del dragón mide `16 × 8 × 16` (`17 × 9 × 17 = 2.601` bloques). Cada tick se ejecutaban **5.202 llamadas a `World::getBlockId()`** (2.601 para agua + 2.601 para lava) con `z` en el bucle más interno, rompiendo el caché de chunk y atravesando secciones de aire vacías.
   - Además, las **7 partes hijas** (`EntityDragonPart`) heredaban `Entity::onUpdate()` → `Entity::onEntityUpdate()`, añadiendo otras **928 llamadas/tick** a `getBlockId()` para fluidos/fuego/vacío sobre hitboxes fantasma que nunca necesitan física de entidad completa.
   - Sólo esta fase ya costaba **33–42 ms/tick** de forma recurrente (`onUpd=42.2ms`).
2. **Thrashing de guardado/descarga/recarga síncrona de chunks del End (`retPost=34.8ms` en xemu, >100 ms en disco IDE de Xbox)**:
   - `XboxTuning.h` dejaba `PLATFORM_END_RESIDENT_CHUNK_RADIUS = -1`. Con distancia corta en Xbox, los chunks del borde de la isla del End se guardaban y descargaban en cuanto el jugador se movía, y al orbitar el dragón por la isla `World::ensureEntityChunkRetention(dragon)` y `prefetchEntityChunkRetention(dragon)` recargaban síncronamente hasta 3–4 chunks de golpe en el mismo tick (`EntityDragon x1 107-169ms` cada pocos segundos en la consola real).
3. **Búsqueda de cristales del End y destrucción de bloques sin caché de chunk/sección**:
   - `EntityDragon::updateDragonEnderCrystal()` consultaba `getEntitiesWithinAABB` sobre un volumen de `80 × 72 × 80` (36 columnas × ~6 slices = ~216 listas) cada tick, cuando en `loadedEntityList` hay ~50 entidades en total.
   - `EntityDragon::destroyBlocksInAABB()` llamaba ~208 veces/tick a `worldObj->getBlockId()` sin reutilizar puntero de `Chunk` ni saltar secciones vacías (`ExtendedBlockStorage == NULL`).

### Optimizaciones aplicadas (mismo comportamiento exacto de Java 1.2.5)
- **`World.cpp`**:
  - `World::handleMaterialAcceleration()` y `World::isMaterialInBB()` ahora iteran por columnas de chunk `(cx, cz)` obteniendo el `Chunk*` una sola vez por columna, consultando `chunk->getBlockStorageArray()` y saltando secciones vacías (`ebs == NULL`, `ebs->getIsEmpty()`) de golpe, y leyendo el ID directamente con `ebs->getExtBlockID(x & 15, y & 15, z & 15)`.
  - `World::ensureEntityChunkRetention()` y `World::prefetchEntityChunkRetention()` limitan las cargas síncronas forzadas a **1 chunk por tick** (`budget = 1`) en vez de disparar varias cargas de disco en un solo fotograma.
- **`EntityDragonPart.h` / `EntityDragonPart.cpp`**:
  - Override ligero `EntityDragonPart::onUpdate()` que únicamente actualiza `prevPos*`, `prevRotation*`, `ticksExisted` y `hurtResistantTime`, evitando las 928 consultas de fluidos/bloques por tick sobre las 7 hitboxes hijas (sus posiciones las fija el dragón en cada `onLivingUpdate()`).
- **`EntityDragon.cpp`**:
  - `updateDragonEnderCrystal()` busca el `EntityEnderCrystal` más cercano dentro de 32 bloques recorriendo `worldObj->loadedEntityList` (mismo criterio de distancia y elección idéntica).
  - `destroyBlocksInAABB()` agrupa por columna `(cx, cz)` y sección vertical (`y >> 4`), saltando secciones vacías y leyendo el ID directo de `ExtendedBlockStorage` antes de comprobar obsidiana/piedra del End/bedrock y llamar a `setBlockWithNotify(x, y, z, 0)`.
- **`XboxTuning.h`**:
  - `PLATFORM_END_RESIDENT_CHUNK_RADIUS = 6` (radio 6 alrededor de `(0, 0)`, manteniendo residente la isla del End durante el combate para evitar el ciclo constante de guardar → descargar → recargar síncrono del disco duro).

### Resultados antes vs después en `xemu` 64 MB (El End + dragón vivo + 40 enderman + 15 cristales)

| Métrica | Antes (`e95_diag.log`, 1200 ticks) | Después (`e95_verify2.log`, 2400 ticks = 2 min) |
|---|---|---|
| **`EntityDragon` en estado estable** | **33–43 ms/tick** (con picos hasta `148.9 ms` al recargar chunks) | **0,2–0,3 ms/tick** (`avgDrag = 0,48 ms/tick`) |
| **Sub-fases del dragón (`cryst / move / coll / dest`)** | `onUpd=42,2 ms`, `retPost=34,8 ms` | `cryst=0,0 / move=0,0 / coll=0,0 / dest=0,0 ms` |
| **Media `entTick` (dragón + 40 enderman + 15 cristales)** | >15–45 ms con picos de `150,0 ms` (`105` picos >10 ms en 60 s) | **`avgEnt = 4,37 ms/tick`** (`entities=1,4–2,6 ms/frame`) |
| **Recargas síncronas por retención del dragón** | Continuas cada pocos segundos | **`5` en total en los 2 minutos completos** |
| **FPS / TPS en combate** | `49,9 fps` | **`63,0–64,6 fps` (`20,0–20,2 tps`)** |
| **RAM libre (`freeKB`)** | — | **`20.412 KB`** (con 7,3 MB de buffers de captura temporales → ~27,7 MB libres en limpio) |

*Nota sobre xemu*: durante la prueba de 2 minutos en Windows, el planificador del host introdujo pausas puntuales de ~40 ms en el hilo TCG de `xemu` que afectaron por igual a entidades triviales (`EntityEnderCrystal x15 40.7ms`, `EntityEnderman 40.1ms`, `EntityDragon 41.5ms` con todas las sub-fases internas del dragón en `0.0ms`). El trabajo real de CPU del dragón quedó reducido de ~33–169 ms a **0,2–0,48 ms/tick** (~100× menos).

---

## 3. Verificación final de compilación y arranque limpio (`xemu` 64 MB)

Todas las sondas temporales fueron retiradas y verificadas ausentes (`hasProbe=False`, `PROBE_HITS=0`). Los 4 binarios finales compilaron con `rc=0` y **0 warnings nuevos SSE2/propios**, y arrancaron limpiamente en `xemu` 64 MB:

| Rama / Versión | Commit | Preset | `default.xbe` | `hasProbe` | `selftest` | `double53` | `freeKB` | `fps` | Crashes / `[X87]` / `nan` |
|---|---|---|---|---|---|---|---|---|---|
| 1.2.5 (`opti30-erluza`) | `96297410` | `xbox-release` | 4.370.432 B | `False` | `0 failure(s)` | `1` | `42.316` | `73,6` | `0 / 0 / 0` |
| 1.2.5 (`opti30-erluza`) | `96297410` | `xbox-release-exp` | 4.382.720 B | `False` | `0 failure(s)` | `1` | `46.300` | `78,0` | `0 / 0 / 0` |
| 1.5.2 (`port152-erluza`) | `b9336633` | `xbox-release` | 4.964.352 B | `False` | `0 failure(s)` | `1` | `41.192` | `77,0` | `0 / 0 / 0` |
| 1.5.2 (`port152-erluza`) | `b9336633` | `xbox-release-exp` | 4.976.640 B | `False` | `0 failure(s)` | `1` | `41.196` | `74,7` | `0 / 0 / 0` |

## 4. Ficheros modificados

### 1.2.5 (`wt-opti30`, commit `96297410`)
- `scripts/xbox/make_legacy_texture_packs.py`
- `src/net/minecraft/src/TextureFX.h` / `TextureFX.cpp`
- `src/net/minecraft/src/RenderEngine.h` / `RenderEngine.cpp`
- `src/net/minecraft/src/TextureWatchFX.h` / `TextureWatchFX.cpp`
- `src/net/minecraft/src/TextureCompassFX.h` / `TextureCompassFX.cpp`
- `src/net/minecraft/src/EntityDragon.cpp`
- `src/net/minecraft/src/EntityDragonPart.h` / `EntityDragonPart.cpp`
- `src/net/minecraft/src/World.cpp`
- `src/xbox/XboxTuning.h`

### 1.5.2 (`wt-152-rend`, commit `b9336633`)
- `scripts/xbox/make_legacy_texture_packs.py`
- `src/net/minecraft/src/TextureFX.h` / `TextureFX.cpp`
- `src/net/minecraft/src/RenderEngine.h` / `RenderEngine.cpp`
- `src/net/minecraft/src/TextureWatchFX.h` / `TextureWatchFX.cpp`
- `src/net/minecraft/src/TextureCompassFX.h` / `TextureCompassFX.cpp`
