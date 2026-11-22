# 106. Informe de Auditoría PS2 — Qué hace falta para llevar lo nuevo de Xbox a PS2 (1.2.5 y 1.5.2)

- **Agente**: `agy-2`
- **Worktrees auditados**:
  - **1.2.5**: `wt-125-ps2` (rama `opti30-ps2`, base `c66352da` → commit con 2 arreglos triviales de compilación/enlace: **`15baefb6`**)
  - **1.5.2**: `wt-152-ps2` (rama `port152-ps2`, base `ac018fef` → commit con 2 arreglos triviales de compilación/enlace: **`59a05b5f`**)
- **Toolchain usada**: `OptiCraftHeritageEdition\psdevwindows` (`mips64r5900el-ps2-elf-g++ 15.2.0`, `dvp-as` VU1/VU0, `ps2sdk`, `gsKit`, LTO + `--gc-sections`, preset equivalente a `ps2-release`).
- **Conclusión ejecutiva**: la disciplina multiplataforma del repositorio ha aguantado sorprendentemente bien. De **1.252 objetivos en 1.2.5** y **1.381 en 1.5.2**, **solo había 2 errores de build** (1 error de compilación de GCC 15 y 1 símbolo sin definir en el enlazador). Sin embargo, **compilar no equivale a funcionar bien en 32 MB / GS 4 MB / EE 294 MHz**: hay **9 adaptaciones funcionales críticas** (X-Ray en `Ps2GreedyMesh`, paletizado `384 KB` del minimapa, tope de `DynamicLights`, HTTP+DNS en `JavaNetwork_ps2`, tope 256 px en `TexturePackZip`, pistas de mando PS2, etc.) que deben hacerse antes de ejecutar en hardware real. **1.2.5 cabe holgadamente con ajustes ligeros; 1.5.2 es viable en PS2**, pero exige recortes explícitos en EE RAM/CPU (detallados en el §3).

---

## 1. Compilación de `ps2-release` en 1.2.5 y 1.5.2 — Errores encontrados

Se configuraron y compilaron ambas ramas completas con `ninja -k 0` (`PLATFORM=PS2`, `PS2_ENABLE_SOUND=ON`, `PS2_ENABLE_VU1_TERRAIN=ON`, `PS2_ENABLE_NETWORK=ON`, `CMAKE_BUILD_TYPE=Release`).

### 1.1 Nota previa de entorno (worktrees)
- `CMakePresets.json:84-86` apunta `PS2DEV` a `${sourceDir}/psdevwindows`. En worktrees nuevos (`wt-125-ps2`, `wt-152-ps2`), `psdevwindows/` no se clona (es untracked en `OptiCraftHeritageEdition\psdevwindows`), por lo que para compilar desde un worktree hay que exportar `PS2DEV`/`PS2SDK` apuntando al `psdevwindows` común (o crear junction simbólico). `cmake/ps2_toolchain.cmake:19-48` lo soporta nativamente y acorta la ruta a 8.3 sin espacios.

### 1.2 Errores reales agrupados por causa (idénticos en 1.2.5 y 1.5.2)

| # | Fase | Archivo y línea | Error exacto (GCC 15.2.0 / ld) | Causa raíz | Arreglo trivial aplicado |
|---|---|---|---|---|---|
| **1** | Compilación (`1/1250` en 1.2.5, `1/1379` en 1.5.2) | `src/net/minecraft/src/skin/GuiSearchSkin.cpp:403` | `error: operands to '?:' have different types 'const jstring' and 'std::string'` | MSVC aceptaba la conversión implícita entre `jstring` y `std::string` en el operador ternario; GCC 15 (`mips64r5900el-ps2-elf-g++`) la rechaza. | Envolver con `std::string(usernameField->getText())`. |
| **2** | Enlace (`ld.exe`) | `src/ps2/JavaNetwork_ps2.cpp:521-527` (llamado desde `GuiSearchSkin.cpp`) | `undefined reference to JavaNetwork::httpGetUrl(const std::string&, std::vector<unsigned char>&, int*, std::size_t, int)` | `cmake/ps2.cmake:98-100` excluye `src/java/JavaNetwork.cpp` cuando `PS2_ENABLE_NETWORK=ON` y usa `JavaNetwork_ps2.cpp`. El encargo de skins online añadió `httpGetUrl()` en `JavaNetwork.h:44`, `JavaNetwork.cpp` y `JavaNetworkXbox.cpp`, pero no en `JavaNetwork_ps2.cpp`. | Stub de 8 líneas en `JavaNetwork_ps2.cpp:523-530` devolviendo `HttpResult::NoConnection` (igual que `readUrl`/`postUrl` justo debajo), permitiendo completar el enlace y medir el ELF real. |

Tras esos dos arreglos triviales (commiteados en **`15baefb6`** y **`59a05b5f`**), **ambos `OptiCraft.elf` compilaron y enlazaron con código de salida 0**, incluyendo los microprogramas `Ps2Vu1Terrain.vsm` y `Ps2Vu0MeshFinalize.vsm`.

---

## 2. Clasificación del código nuevo (desde `5802c26`) para PS2

### 2.1 Común — Sirve igual en PS2 (sin cambios o con ajustes cosméticos mínimos)

| Subsistema | Ficheros principales (`fichero:línea`) | Estado en PS2 |
|---|---|---|
| **Gestor y catálogo de mods** | `src/mods/ModManager.cpp`, `src/mods/IMod.h`, `src/mods/GuiMods.cpp`, `src/mods/GuiCompiledMods.cpp` | Compila y navega con mando vía `platformTextInputSnapshot` (`GuiCompiledMods.cpp:191,490`) y `VirtualKeyboard` (`GuiCompiledMods.cpp:295,357`). Solo falta la barra visual de pistas PS2 (`GuiCompiledMods.cpp:746`). |
| **Herobrine Classic (`iLoveYou`)** | `src/mods/herobrine/HerobrineMod.cpp`, `EntityHerobrine.cpp`, `RenderHerobrine.cpp`, `GuiHerobrineMenu.cpp` | 100 % C++ común. Ojos blancos emisivos (`0xFFFFFFFF` sobre `0x00000000`) son compatibles con alfa de 1 bit (`PSM_CT16`) de la GS. Destructivos/robo ya están en OFF por defecto. Solo falta barra de pistas PS2 (`GuiHerobrineMenu.cpp:342`). |
| **ChocoCraft (clean-room)** | `src/mods/chococraft/ChocoCraftItems.cpp`, `ChocoCraftMod.cpp`, `EntityChocobo.cpp`, `ModelChocobo.cpp`, `RenderChocobo.cpp`, `GuiChocoboMenu.cpp`, `ItemDye.cpp` (1.2.5) | 100 % C++ común. Texturas procedurales `64×32` (6 colores, carga perezosa = ~12 KB VRAM en PSMT8). Respeta `PS2_MAX_LIVE_MOBS` / `PS2_WORLDGEN_ANIMAL_LIVE_MAX`. Solo falta barra de pistas PS2 (`GuiChocoboMenu.cpp:448`). |
| **Village Locator & Stronghold Locator** | `src/mods/villagelocator/VillageLocatorMod.cpp:66-76`, `src/net/minecraft/src/MapGenVillage.cpp`, `src/mods/strongholdlocator/StrongholdLocatorMod.cpp:53-62` | Ya tienen los combos PS2 (`Select+Triangle` y `L3+R3`, ambos soportan P1 y P2). Búsqueda solo lectura sin tocar terreno. |
| **TooManyItems** | `src/mods/toomanyitems/TooManyItems.cpp:23,267-289` | Ya tiene controles DualShock y usa 4 columnas a 640×448 (`width=320` virtuales). |
| **Server Utils** | `src/mods/serverutils/ServerUtilsMod.cpp`, `src/net/minecraft/src/GuiServerInfo.cpp`, `GuiMultiplayer.cpp`, `GuiSlotServer.cpp`, `ThreadPollServers.cpp` | 100 % común sobre `JavaNetwork::createSocket()` (ya soportado por `Ps2Socket`). |
| **Crafteo de consola y comercio Legacy** | `src/net/minecraft/src/legacy/XboxCraftingScreen.cpp:3`, `src/net/minecraft/src/legacy/LegacyTradingScreen.cpp:530` | Ambos ya incluyen `#if defined(PS2_PLATFORM)` y pistas DualShock. |
| **Arreglos de erluza / ramide1 / gameplay** | `EntityDragon.cpp`, `EntityMinecart.cpp`, `Explosion.cpp`, `AnvilChunkLoader.cpp`, `BlockFire.cpp`, `EntityOcclusionCache`, `GuiAchievement.cpp`, `GameSettings.cpp` (`toggleSneak`, `onScreenHints`), `GuiContainer.cpp` (mantener A) | Código común. `EntityOcclusionCache` ya está activo en `RenderGlobal.cpp:635,789,899`. |
| **Skins selector 3D y packs LCE** | `src/net/minecraft/src/skin/GuiSkinSelector.cpp:32-35,112-121,457-460,1862`, `GuiLoadSkinsMenu.cpp`, `GuiLoadSkinsList.cpp:14-18,175-184`, `GuiConfirmSkinInstall.cpp:14,114` | Ya tienen soporte de pad PS2, pistas PS2 y escaneo en `Ps2Assets::installDir()/skins` y `mc0:/OPTCRAFT/skins`. (Ver §3 para el impacto de los 116 PNGs de Halo/Mass Effect/Skyrim en USB/VRAM). |
| **Mecánicas 1.5.2** | Aldeanos, comandos, Wither, fuegos artificiales, dispensador/dropper/tolva/comparador, bloque de comandos, spawner 1.5.2, armadura teñible + caldero, encantar 1.5.2, mapas 1.5.2, faro, yunque, bruja | Toda la lógica es C++ común; sin dependencias D3D8/XDK. (Ver §2.3 y §3 para el coste CPU/RAM en EE). |

---

### 2.2 Necesita equivalente o adaptación en PS2 (lista priorizada)

1. **X-Ray en el renderizador de terreno PS2 (ALTA)**
   - **Problema**: `XRayMod.cpp:74-84` ya lee el combo PS2 (`L1+R1+L2+R2`), pero el terreno en PS2 NO usa `WorldRenderer.cpp` ni `WorldRendererPcLegacy.cpp`; usa **`src/ps2/minecraft/WorldRendererPs2.cpp`** y **`src/ps2/render/Ps2GreedyMesh.cpp`**, donde hay **0 referencias a `XRayMode`**. Hoy X-Ray en PS2 muestra el mensaje `[X-Ray] ON` pero sigue dibujando piedra/tierra/hierba.
   - **Qué hacer**: filtrar `!XRayMode::isVisibleBlock(id)` en `Ps2GreedyMesh.cpp` (`SectionReader::isOpaque` L63, `isOpaqueBlockId` L93 y emisión de caras) y en `WorldRendererPs2.cpp` (ruta por bloque + `markAllRenderersDirty()` + luz a tope `15/15` cuando `XRayMode::active()`).

2. **Minimapa (`ReiMinimap`) — Pico de 384 KB en `ps2_palettize_ct16` y combos faltantes (ALTA)**
   - **Problema 1 (RAM/CPU)**: `ReiMinimap::uploadPlayerTexture()` (`src/mods/reiminimap/ReiMinimap.cpp:735-743`) llama a `RenderEngine::setupTexture()` cada vez que la textura `64×64` cambia. En PS2 eso entra en `ps2_texture_upload_rgba()` → `ps2_palettize_ct16()` (`src/ps2/render/Ps2Texture.cpp:96-105`), que hace `calloc(65536*4) + malloc(65536*2) = 384 KB` de heap temporal y recorre un histograma de 65.536 entradas en la EE (~294 MHz) **en cada actualización** (riesgo grave de fragmentación/OOM y tirones).
   - **Qué hacer**: en `PLATFORM_PS2`, usar `renderTextureSubImageRgba` sobre la textura ya creada (o un modo `CT16` directo sin paletizar de `64×64×2 = 8 KB` VRAM, evitando `ps2_palettize_ct16` por completo) y limitar `m_uploadCooldown` a ≥10 ticks.
   - **Problema 2 (Combos)**: `ReiMinimap.cpp:381-407` solo mapea `Triangle+Up` (WP rápido) y `Triangle+Down` (gestor WP) en PS2; faltan **`Triangle+Left`** (ciclar zoom) y **`Triangle+Right`** (abrir `GuiMinimapMenu`), que sí existen en Xbox (`XboxInput.cpp`).

3. **Luces Dinámicas (`DynamicLights.cpp`) — Tope demasiado alto para EE (ALTA)**
   - **Problema**: `src/net/minecraft/src/DynamicLights.cpp:21-27` pone `kMaxSources=16, kScanIntervalTicks=3` solo bajo `#if PLATFORM_XBOX`; en PS2 cae en el `#else` de PC (**`kMaxSources=64, kScanIntervalTicks=2`**). En PS2, donde `PS2_LIGHTING_BUDGET_US = 2500` (`Ps2WorldTuning.h:598`) y `PS2_CHUNK_BUILD_BUDGET_MS = 4` (`Ps2MeshTuning.h:376`), 64 fuentes llamando a `World::updateLightByType(EnumSkyBlock::Block, ...)` saturarían la cola de luz y remallado.
   - **Qué hacer**: añadir `#elif PLATFORM_PS2` con `kMaxSources = 6` (o `4`: P1, P2 y 2–4 entidades/objetos cercanos) y `kScanIntervalTicks = 5..6`.

4. **Skins online — HTTP + DNS + hilo en `JavaNetwork_ps2.cpp` y `GuiSearchSkin.cpp` (MEDIA-ALTA)**
   - **Problema 1 (HTTP)**: `JavaNetwork_ps2.cpp` no implementa `JavaNetwork::httpGetUrl()` (actualmente stub en `L523`). Necesita un cliente HTTP/1.0-1.1 plano sobre `Ps2Socket` (~160 líneas, análogo a `JavaNetworkXbox.cpp`, respetando `maxBytes = 64 KB`, redirecciones 301/302 y timeout).
   - **Problema 2 (DNS)**: `JavaNetwork_ps2.cpp:137` depende de `gethostbyname()` de `lwIP`/`ps2ip` (DNS del router por DHCP). No tiene el fallback de DNS público (`8.8.8.8`/`1.1.1.1`) añadido a `src/xbox/system/XboxNetwork.cpp` (`6819d617`). Si el DNS local falla en PS2, habrá que portar el pequeño resolver UDP de 100 líneas a `Ps2Network.cpp`.
   - **Problema 3 (Hilo y stack EE)**: `GuiSearchSkin.cpp:40` lanza un `std::thread` que llama a `httpGetUrl` + `stbi_load_from_memory` + `stbi_write_png_to_mem` (ver `SkinManager.cpp:19-21`). En PS2SDK los hilos EE tienen stack pequeño y `lwIP`/`ps2ip` no siempre tolera `gethostbyname` concurrente con `ThreadPollServers`; conviene verificar tamaño de stack del hilo o ejecutar la descarga con un `Ps2Thread` dedicado/acotado (64 KB stack) y mutex alrededor de `ps2ip` DNS.
   - **Problema 4 (Almacenamiento)**: `SkinManager.cpp:583-586` guarda/escanea en `mc0:/OPTCRAFT/skins`. La Memory Card (8 MB, escritura muy lenta) se llenaría con pocas skins descargadas; en PS2 conviene priorizar `Ps2Assets::installDir() + "/skins"` en USB (`mass:`) si es escribible, dejando `mc0:` como respaldo.

5. **Paquetes de texturas `.zip` (`TexturePackZip.cpp` + `TexturePackList.cpp`) (MEDIA)**
   - **Estado**: `cmake/ps2.cmake:77-89` **ya compila `minizip` (`ioapi.c` + `unzip.c` con `USE_FILE32API`)**, pero `TexturePackZip.cpp:5` y `TexturePackList.cpp:12,119` están cerrados a `#if defined(XBOX_PLATFORM)`.
   - **Qué hacer**: abrir ambos a `defined(XBOX_PLATFORM) || defined(PS2_PLATFORM)`, buscar en `Ps2Assets::installDir() + "/texturepacks"` (USB `mass:`, nunca `mc0:`) y **bajar los topes en PS2**:
     - `kMaxAtlasPixels = 256` (solo packs 16×; ver §3 por qué 32×/512 px no cabe en PS2).
     - `kMaxThumbnailPixels = 64` (en vez de 256) y `kMaxFileBytes = 512 KB` (en vez de 4 MB).
     - Liberar `thumbnailTexture` al salir de `GuiTexturePacks` para no agotar `PS2_MAX_TEX = 256` (`Ps2Texture.h:25`) ni los 4 MB de VRAM.

6. **Barras de pistas DualShock en las 4 pantallas nuevas de mods (BAJA — muy rápida)**
   - `GuiCompiledMods.cpp:746`, `GuiMinimapMenu.cpp:387`, `GuiHerobrineMenu.cpp:342` y `GuiChocoboMenu.cpp:448` tienen `#if PLATFORM_XBOX` para `drawControlHintRow(...)`. En PS2 no muestran pistas en la parte inferior.
   - **Qué hacer**: añadir `#elif PLATFORM_PS2` usando los nombres de `ControlIcon.cpp:31-37` (`"D-Pad"`, `"Cross"`, `"Circle"`, `"Square"`, `"Triangle"`, `"L1"`, `"R1"`, `"L2"`, `"R2"`). `GuiWaypointManager.cpp:639` ya lo tiene como ejemplo.

7. **Icono de guardado y Personaje animado en `GuiIngame.cpp` (BAJA)**
   - **Icono de guardado (`GuiIngame.cpp:443-478`)**: funciona en PS2, pero `PS2_DISABLE_RUNTIME_AUTOSAVE` vale `1` (`Ps2CoreTuning.h:117`), por lo que solo aparecerá si `World::lastAutosaveMillis` se actualiza también en el guardado manual de `Ps2SaveStorage` y si `data/assets/gui/save_chest.png` + `save_arrow.png` se copian al árbol `data/` del USB de PS2.
   - **Personaje animado (`GuiIngame.cpp:484-560`)**: funciona, pero conviene: (a) llamar a `ps2_draw_2d_flush_pending()` antes de `renderClear(RenderClearMask::Depth)` (`L521`) para no corromper el batch 2D (`PS2_BATCH_2D_SPRITES=1`, `Ps2CoreTuning.h:254`), y (b) añadir la opción de desactivarlo (en EE un modelo bípedo extra cuesta ~0,8–1,0 ms/frame).

8. **Pantalla dividida (`Ps2SplitScreen.cpp` vs `XboxSplitScreen.cpp`) (BAJA)**
   - PS2 conserva su propio `src/client/Ps2SplitScreen.cpp` (solo mundos Antiguos 256×256 por límite de 32 MB). Conviene portar a `Ps2SplitScreen::leavePlayer2` la limpieza `EntityLiving::clearEntityReferences(mc->thePlayer2)` que corregimos en `XboxSplitScreen.cpp` para evitar punteros colgantes en `EntityLiving::currentTarget` / `cachedNearestPlayer`.

9. **Reconstrucción de tabla de teclas y opciones (`GameSettings.cpp`, `HudSafeArea`) (BAJA)**
   - `GameSettings.cpp:108-110` mantiene `hudSafeArea = 0` en PS2 y `GuiIngame.cpp:269` conserva el piso de 12 px de TV-safe en PS2; `onScreenHints`, `toggleSneak` y el arreglo de teclas/chat (`90faed9f`) ya son comunes, pero falta revisar que las reasignaciones de cruceta/chat en `Ps2InputMapper.cpp` reconstruyan la tabla igual que en `XboxInput.cpp`.

---

### 2.3 No viable (o desaconsejado) en PS2 y por qué

| Funcionalidad | Motivo técnico en PS2 |
|---|---|
| **Packs de texturas 32× (`512×512`) y 64× (`1024×1024`)** | La GS tiene **4 MB de VRAM totales** (framebuffer doble `640×448×16` + Z16 ya consume ~1,7 MB, dejando **~2,3 MB** para todas las texturas). Un atlas `512×512` con mipmaps ocupa ~341 KB en PSMT8 VRAM + 1 MB RGBA al decodificar + 384 KB en `ps2_palettize_ct16`, y al recargar `terrain.png` + `gui/items.png` + `ctm.png` se saturan VRAM y el heap de 32 MB. En PS2 limitar a **16× (`256×256`)**. |
| **Modo 720p (`1280×720`)** | En PS2 `1280×720×16` doble buffer + Z16 requeriría **5,5 MB de VRAM > 4 MB físicos de la GS** (imposible en hardware sin trucos de frame-slicing). PS2 permanece en `640×448` NTSC/PAL (`PS2_NTSC_MODE=ON`). |
| **Generación/carga asíncrona de chunks (`XBOX_ASYNC_CHUNK_IO`)** | Diseñada para 64 MB con hilos Win32/XAPI. En PS2 los 32 MB no toleran dos copias de buffers de generación/NBT en paralelo y el rendimiento está ya acotado con el generador incremental en el hilo principal (`PS2_INCREMENTAL_CHUNK_GENERATION=1`, `Ps2WorldTuning.h:693`). |
| **Autoguardado cada minuto en Memory Card (`mc0:`)** | La Memory Card de PS2 escribe a ~100–150 KB/s y congela el juego varios segundos por guardado. `PS2_DISABLE_RUNTIME_AUTOSAVE=1` (`Ps2CoreTuning.h:117`) **debe seguir activo**; el icono de guardado solo se dispara en guardados explícitos (o en USB si se activa opcionalmente). |
| **Pantalla dividida en mundos infinitos** | En 32 MB el presupuesto de chunks está fijado a radio 2 (`5×5 = 25` columnas, `Ps2CoreTuning.h:62`). Dos jugadores alejándose doblan el conjunto residente y provocan OOM; debe mantenerse el límite de `Ps2SplitScreen.cpp:108` (solo mundos `256×256` + distancia máx. 32 bloques). |
| **Luces dinámicas con 64 fuentes / cada 2 ticks** | Saturaría el presupuesto de luz (`2,5 ms`) y de remallado (`4 ms`) del EE. Viable solo recortado a `4–6` fuentes y `5–6` ticks (y OFF por defecto). |
| **Niebla/cielo W-friendly D3D8 y `XboxVideoMode`** | Específicos de NV2A/D3D8; la GS tiene su propia niebla por vértice (`Ps2RenderLighting.cpp`) y `PS2_SKIP_CLOUDS=1` / `PS2_ENABLE_SKY_STARS=0` (`Ps2MeshTuning.h:518-529`). |

---

## 3. Análisis de Memoria (32 MB EE RAM + 4 MB GS VRAM) y Viabilidad de 1.5.2

### 3.1 Medidas reales del ELF (obtenidas con `mips64r5900el-ps2-elf-size` y `OptiCraft.map`)

| Métrica ELF (RAM fija desde el arranque) | PS2 1.2.5 (`wt-125-ps2`) | PS2 1.5.2 (`wt-152-ps2`) | Diferencia (1.5.2 − 1.2.5) |
|---|---:|---:|---:|
| `.text` + `.vutext` (código EE + VU) | 5 058 KB (4,94 MB) | 5 683 KB (5,55 MB) | **+625 KB** |
| `.rodata` (tablas constantes, textos) | 310 KB | 384 KB | **+74 KB** |
| `.eh_frame` + `.gcc_except_table` | 669 KB | 769 KB | **+100 KB** |
| `.data` + `.sdata` | 20 KB | 20 KB | +0 KB |
| `.bss` (estáticos/globales) | 860 KB | 862 KB | **+2 KB** |
| **Total `load_memory_bytes` (ELF en EE RAM)** | **6 918 KB (6,76 MB)** | **7 719 KB (7,54 MB)** | **+801 KB (+0,78 MB)** |

Desglose exacto dentro del ELF de lo añadido desde `5802c26` (medido en `OptiCraft.map`):
- **Todos los mods juntos (9 mods)**: **295,2 KB** en 1.2.5 / **295,8 KB** en 1.5.2:
  - Minimapa (`ReiMinimap` + `GuiMinimapMenu` + `GuiWaypointManager`): `69,5 KB`
  - Herobrine (`iLoveYou` completo + menú): `53,1 KB`
  - Catálogo + gestor (`GuiCompiledMods` + `GuiMods` + `ModManager`): `50,3 KB`
  - ChocoCraft (modelo, render, IA, menú, ítems): `42,8 KB`
  - Mods erluza (`XRay`, `DynamicLights`, `VillageLocator`, `StrongholdLocator`, `ServerUtils`, `TMI`): `99,7 KB`
- **Sistema de skins (incl. `GuiSearchSkin` + `stb_image_write` + listas Halo/Mass Effect/Skyrim)**: **162,6 KB**
- **`XboxCraftingScreen`**: **27,1 KB** (1.2.5) / **40,8 KB** (1.5.2, tablas TU19 más grandes)
- **`TexturePackZip` + `minizip`**: **11,5 KB** (minizip ya estaba enlazado en PS2 para tutorial saves)
- **Suma de código nuevo común en 1.2.5**: **~496 KB (~0,48 MB)** adicionales en el ELF.

### 3.2 Presupuesto real de los 32 MB de EE RAM en PS2 (32 768 KB)

Partiendo de los datos documentados en `Ps2CoreTuning.h`, `Ps2MeshTuning.h`, `Ps2WorldTuning.h` y `ClientPlatformPolicy_PS2.cpp`:

| Bloque de EE RAM | 1.2.5 base erluza | + Novedades 1.2.5 (mods OFF / ON) | + Port 1.5.2 (con recortes) |
|---|---:|---:|---:|
| Kernel PS2 + stack EE + IOP/RPC + cola GS | ~2,5 MB | ~2,5 MB | ~2,5 MB |
| Imagen ELF cargada (`load_memory_bytes`) | ~6,2 MB | **6,76 MB** (+0,5 MB) | **7,54 MB** (+1,3 MB) |
| Subsistema base (RenderEngine, Tessellator 256 KB, Sound, UI, tablas Block/Item/Recipes) | ~3,5 MB | ~3,6 MB | ~4,2 MB (+0,6 MB: +27 bloques, +13 ítems, +18 entidades, +44 recetas) |
| **Margen libre antes de entrar al mundo** | **~19,8 MB** | **~19,1 MB** | **~17,8 MB** |
| Mundo residente (`5×5` a `7×7` columnas = 25–49 chunks; ~65–90 KB/chunk) | ~2,5–4,5 MB | ~2,5–4,5 MB | ~2,8–4,8 MB (+tile entities 1.5.2: tolvas/comparadores/faros) |
| Caché de mallas terreno (`PS2_MESH_RAM_TARGET=6 MB`, `HARD=7,5 MB`) | ~5,2–6,0 MB | ~5,2–6,0 MB | ~5,2–6,0 MB (acotado por `Ps2MeshMemoryPolicy`) |
| Texturas EE (espejos CPU solo para atlases dinámicos `terrain`/`items`, resto descartado por `TextureResidencyPolicy_PS2`) | ~1,2–1,8 MB | ~1,3–2,0 MB | ~1,4–2,2 MB (**1.5.2 usa `terrain.png`/`items.png` 256×256**, NO atlas gigante!) |
| Cola de luz (`PS2_LIGHTING_QUEUE_HARD_CAP=6144` ≈ 200 KB) + partículas + entidades | ~0,8 MB | ~0,8–1,1 MB | ~1,0–1,4 MB (aldeanos, Wither, fuegos) |
| **Coste dinámico de mods nuevos (cuando se activan)** | — | **+0,15 a +0,55 MB** | **+0,15 a +0,55 MB** |
| **Margen libre estable en partida (objetivo > 4–6 MB anti-fragmentación)** | **~7–9 MB** | **~6–8 MB** | **~4,5–6,5 MB** |

Desglose dinámico de cada novedad nueva en RAM y VRAM (GS = 4 MB):
- **Minimapa (`ReiMinimap`)**: **80 KB EE RAM** (fijo: 2 buffers `64×64×4` + 1 `BufferedImage` + caché radar) + **8 KB VRAM** (2 texturas `64×64` en PSMT8). **PERO** con el bug actual de `ps2_palettize_ct16` genera picos transitorios de **+384 KB** en cada refresco (imprescindible cambiar a CT16/subimage en PS2).
- **ChocoCraft**: ~6 KB EE (entidades) + **2 KB VRAM por color visto** (máx. 12 KB para los 6 colores). Muy ligero.
- **Herobrine (`iLoveYou`)**: ~4 KB EE + **4 KB VRAM** (skin + máscara de ojos emisivos). Muy ligero.
- **X-Ray / VillageLocator / StrongholdLocator / ServerUtils / TMI**: < 16 KB EE en conjunto, 0 KB VRAM extra.
- **DynamicLights**: ~4 KB estáticos (`Source[64]`, o ~0,4 KB bajando a `kMaxSources=6`). Coste real es CPU (cola de luz/remallado), no RAM.
- **Skins LCE (Halo 40 + Mass Effect 36 + Skyrim 40 = 116 skins) + `GuiSearchSkin`**:
  - Metadatos en `SkinManager`: ~25 KB EE.
  - **Peligro en `GuiSkinSelector`**: el carrusel 3D carga varias texturas `64×32` (2 KB VRAM cada una en PSMT8) al deslizar; en 116 skins, **sin descarga/evicción al cambiar de pestaña**, se llenarían los 256 slots de `PS2_MAX_TEX` (`Ps2Texture.h:25`). Al entrar al mundo (`ClientPlatformPolicy_PS2::releaseWorldEntryAssets`), hay que liberar las texturas de previsualización de skins que no estén equipadas.
  - Descarga online (`GuiSearchSkin`): pico temporal de ~140 KB (buffer HTTP 64 KB + PNG decodificado `64×64×4` = 16 KB + `stb_image_write` + stack de hilo ~64 KB). **Solo debe abrirse desde el menú principal** (donde hay ~18–19 MB libres), nunca dentro de una partida cargada!
- **Texture Packs `.zip` (`TexturePackZip`)**: índice del directorio central en RAM ~20–60 KB (para ~500–1400 entradas); 1 fichero descomprimido bajo demanda. Con el tope `256 px` en PS2, un pack 16× consume lo mismo que el default (~64 KB VRAM `terrain.png` + ~64 KB VRAM `items.png`). Con packs 32× (`512 px`) saltaría a ~1 MB temporal + 341 KB VRAM por atlas y rompería PS2.
- **Icono de guardado (`save_chest.png` + `save_arrow.png`)**: ~2 KB VRAM.
- **Datos en disco (`assets.pak` / `data/` en USB)**: 1.2.5 ocupa 54,2 MB, 1.5.2 ocupa 61,5 MB (+7,3 MB, principalmente sonidos de aldeano/bruja/Wither/fuegos y texturas de mobs/ítems nuevos; en USB no impacta en RAM porque se leen bajo demanda).

### 3.3 Opinión razonada sobre Minecraft 1.5.2 en PS2: ¿es viable?

**Sí, es técnicamente viable**, y mucho más cerca de lo que cabría temer, por tres razones medidas en esta auditoría:
1. **El ELF de 1.5.2 ya enlaza entero para PS2 y solo añade `+801 KB` (`+0,78 MB`)** sobre la 1.2.5 (`7,54 MB` vs `6,76 MB`).
2. **El port 1.5.2 mantiene los atlas `256×256` (`terrain.png` y `gui/items.png`)**, por lo que encaja directamente en el pipeline `PSMT8` + `MIPTBP1` + `Ps2GreedyMesh` + `VU1` sin rehacer la GS ni explotar los 4 MB de VRAM.
3. **Los registros de bloque siguen acotados a `BLOCK_REGISTRY_SIZE = 256`** en consolas (`ExtendedBlockStorage.h:100-103`), manteniendo intacto el coste por sección de chunk (`ExtendedBlockStorage` ≈ 10 KB por sección no vacía).

**Pero NO funcionará estable sin estos recortes específicos para PS2 en 1.5.2:**
1. **Recortar memoria de mallas de `6 / 7,5 MB` a `5 / 6,5 MB`** (`PS2_MESH_RAM_TARGET_BYTES` / `HARD_BYTES` en `Ps2MeshTuning.h:229-230`) para recuperar ~1 MB que compensa los `+801 KB` del ELF 1.5.2 + `+0,6 MB` de tablas/recetas/entidades, manteniendo ≥5 MB de margen libre contra fragmentación (`Ps2Heap.cpp:114`).
2. **Limitar entidades y IA nuevas de 1.5.2**:
   - **Aldeanos y asedios**: tope de aldeanos activos por aldea (ej. máx. 6–8) y asedio zombi limitado a `≤8` (en Xbox se propuso `≤20`; en PS2 `PS2_MAX_LIVE_MOBS=4`).
   - **Wither**: limitar destrucción de bloques por tick del Wither (los remallados continuos + luz de explosión saturarían el presupuesto de `4 ms` del EE), desactivar partículas excesivas del escudo y aplicar la optimización del dragón/Wither (un único `updateLight` por explosión).
   - **Tolvas / relojes de redstone / comparadores**: heredar el `ChunkTickScheduler` o acotar `tileEntity` ticks de tolvas fuera del radio visible (`24` bloques).
3. **Residencia del End**: bajar `PS2_END_RESIDENT_CHUNK_RADIUS` de `5` (`11×11 = 121` columnas ≈ 3,5 MB, `Ps2WorldTuning.h:317`) a **`4`** (`9×9 = 81` columnas ≈ 2,3 MB) en 1.5.2 para que el Wither/End sigan dejando margen seguro.
4. **Prohibir `GuiSearchSkin` in-game y limitar `TexturePackZip` a 16× (`256 px`)**, más liberar texturas de previews de skins en `ClientPlatformPolicy_PS2::releaseWorldEntryAssets()`.
5. **Reemplazar `PS2_ENABLE_LTO` / excepciones si faltara margen**: `.eh_frame + .gcc_except_table` pesan **769 KB** en 1.5.2; hoy el código usa `try/catch` para OOM y NBT, así que se mantiene, pero es una reserva conocida si hiciera falta.
6. **Validar bloques 1.5.2 en `Ps2BlockRenderInfo.cpp` y `Ps2GreedyMesh.cpp`**: las tablas de propiedades de render PS2 (yunque, faro, tolva, caldero, marco, cabezas, muros de adoquín, macetas, comparador, sensor solar, trampa) deben revisarse para asegurar que bloques no-cubo nunca entren en el greedy mesher (`simpleOpaqueCube = false`).

---

## 4. Plan por pasos para llevarlo a PS2 (primero 1.2.5, luego 1.5.2)

Cada paso es independiente, compilable y verificable en PCSX2 / PS2 real con `PS2_RENDER_STATS=ON` y el log `[PS2][FRAME]` (`mallocUsed`/`free`/`VRAM`).

| Paso | Alcance | Versión | Tamaño | Riesgo | Detalle técnico |
|---|---|---|---:|---|---|
| **1. Arranque limpio 1.2.5 en PS2** | Integrar `15baefb6` (`GuiSearchSkin.cpp:403` + stub `httpGetUrl`), copiar `save_chest.png`/`save_arrow.png` a `data/assets/gui/`, revisar `ps2_draw_2d_flush_pending()` antes de `renderAnimatedCharacter`, y añadir `clearEntityReferences` a `Ps2SplitScreen.cpp`. | 1.2.5 | Pequeño (0,5 día) | Bajo | Permite arrancar el `OptiCraft.elf` de 1.2.5 en PCSX2/PS2 y medir `freeKB` y `VRAM` reales base antes de activar mods. |
| **2. Pistas de mando PS2 y combos del Minimapa** | Añadir `#elif PLATFORM_PS2` en `GuiCompiledMods.cpp:746`, `GuiMinimapMenu.cpp:387`, `GuiHerobrineMenu.cpp:342`, `GuiChocoboMenu.cpp:448`; añadir `Triangle+Left/Right` en `ReiMinimap.cpp:381-407`; verificar `Ps2InputMapper` para cruceta/chat reasignable. | 1.2.5 | Pequeño (0,5 día) | Bajo | Deja 100 % navegables con DualShock el catálogo y todos los menús nuevos. |
| **3. Minimapa y Luces Dinámicas seguros en 32 MB** | Evitar `ps2_palettize_ct16` (`384 KB`) en `ReiMinimap::uploadPlayerTexture()` usando actualización sub-imagen / CT16 sin paletizar y cooldown ≥10 ticks; limitar `DynamicLights.cpp:21` en `PLATFORM_PS2` a `kMaxSources=6, kScanIntervalTicks=6`. | 1.2.5 | Medio (1 día) | Medio | Elimina el único riesgo real de OOM/fragmentación y caída de FPS de los mods en EE. |
| **4. X-Ray nativo en terreno PS2** | Integrar `XRayMode::active()` / `isVisibleBlock()` + brillo `15/15` en `Ps2GreedyMesh.cpp` y `WorldRendererPs2.cpp`. | 1.2.5 | Pequeño-Medio (0,5–1 día) | Bajo-Medio | Completa paridad de los 9 mods en PS2 (ChocoCraft, Herobrine, TMI, Locators y ServerUtils ya funcionan tal cual). |
| **5. Paquetes `.zip` 16× en USB** | Habilitar `TexturePackZip.cpp` + `TexturePackList.cpp` en `PS2_PLATFORM` apuntando a `Ps2Assets::installDir()/texturepacks`, con topes `kMaxAtlasPixels=256`, `kMaxThumbnailPixels=64`, `kMaxFileBytes=512 KB`, y liberación de miniaturas al cerrar `GuiTexturePacks`. | 1.2.5 | Pequeño-Medio (1 día) | Medio | Aprovecha `minizip` que ya está compilado en `cmake/ps2.cmake:77-89`. Probar con 2–3 packs 16× y rechazar limpiamente 32×+. |
| **6. Skins LCE (gestión VRAM) + Skins online HTTP/DNS** | (a) Evicción de texturas de preview en `GuiSkinSelector` + `releaseWorldEntryAssets` y guardar skins descargadas en USB `mass:` cuando sea escribible; (b) implementar `JavaNetwork::httpGetUrl()` en `JavaNetwork_ps2.cpp` sobre `Ps2Socket` + fallback DNS UDP en `Ps2Network.cpp` + bloqueo in-game en `GuiSearchSkin` y stack/mutex seguro para EE. | 1.2.5 | Grande (2–3 días) | Medio-Alto | Mayor riesgo por `ps2ip`/`lwIP` y concurrencia de hilos en PS2SDK; dividir en 6a (offline/VRAM) y 6b (HTTP/DNS online). |
| **7. Auditoría de bloques 1.5.2 en GS/VU1 + recortes RAM** | Tras estabilizar 1.2.5 en consola real, llevar pasos 1–6 a `port152-ps2`, auditar los 27 bloques nuevos de 1.5.2 en `Ps2BlockRenderInfo.cpp` / `Ps2GreedyMesh.cpp`, aplicar los recortes del §3.3 (`PS2_MESH_RAM_TARGET=5 MB`, `PS2_END_RESIDENT_CHUNK_RADIUS=4`, límites Wither/aldeanos/tolvas) y medir en PCSX2 + PS2 real (`freeKB`, `VRAM`, FPS en aldea, Nether, End y Wither). | 1.5.2 | Grande (3–5 días) | Alto | El ELF ya compila y enlaza hoy (`7,54 MB`), pero las entidades y bloques complejos de 1.5.2 requieren perfilado real en hardware de 32 MB. |

---

## 5. Resumen de cambios hechos en esta auditoría

Solo se aplicaron los **2 arreglos triviales de compilación/enlace** necesarios para ver todos los errores y medir los ELF reales (sin portar subsistemas):
- **`wt-125-ps2` (`opti30-ps2`) → commit `15baefb6`**:
  - `src/net/minecraft/src/skin/GuiSearchSkin.cpp:403`: conversión explícita `std::string(usernameField->getText())` en ternario para GCC 15.
  - `src/ps2/JavaNetwork_ps2.cpp:523-530`: stub `JavaNetwork::httpGetUrl(...)` devolviendo `HttpResult::NoConnection`.
- **`wt-152-ps2` (`port152-ps2`) → commit `59a05b5f`**:
  - Mismos 2 arreglos triviales en `GuiSearchSkin.cpp:403` y `JavaNetwork_ps2.cpp:523-530`.
