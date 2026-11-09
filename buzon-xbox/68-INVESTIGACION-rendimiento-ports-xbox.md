# 68 - Técnicas de rendimiento de otros ports a Xbox clásica, aplicadas a OptiCraft (03-11-2026)

Solo he investigado: no he tocado código ni compilado.
- **Netlog usado:** `OptiCraftHeritageEdition\xbox-spike\netlog.txt` (exp 1.2.5, 01-10; 72.900 líneas).
  El de `wt-152-codex\xbox-spike\netlog.txt` es viejo y casi no tiene líneas de rendimiento.
- **Código mirado:** `wt-152-codex`.
- **Fuentes principales:**
  - la ayuda de la XDK 5849 que ya tenemos (`xdk\5849\sdk\XDK\doc\XboxSDK.chm`; la extraje en un temporal, se cita por el nombre de la página);
  - Melee-X (`Desktop\RE4 port\referencias\Melee-X\docs`);
  - los informes 52 y 55 de BT3 (`Desktop\port bodokaii 3\buzon`);
  - el backend de Xbox de ClassiCube.

Solo pongo cifras que vienen de una fuente o de nuestro netlog. Si algo no está comprobado, lo digo.

---

## 0. Lo que dice el netlog (consola real)

| campo | valor típico | lectura |
|---|---|---|
| `xbox.perf fps/frame/present` | 59,9 fps, 16,7 ms, `present=1.3ms` | La CPU va casi al límite: ~15 ms de trabajo por frame. |
| `loop render` | 14,5 ms | La mayor parte del frame es render en la **CPU**. |
| `xbox.draw vbDraws=186 (in D3D 0.20ms) upDraws=26 upVerts=2310` | | Dibujar cuesta poco: **la API D3D no es el cuello**. |
| `xbox.draw replay=2.1ms setTransform=0.02ms textureStage=5.0-5.5ms` | | **`textureStage` (5 ms por frame) es lo más gordo del render.** |
| `xbox.terrain visibility=2.6ms submit=2.1ms` | | |
| tirones (`xbox.spike`, 4.230) | | Causa principal: `entities>entTick` 1.936, `worldTick>chunkUnload` 567, `stats>mouseOver` 444, `worldTick>randomBlocks` 344 (hasta 170 ms), `worldTick>autosave` 111 (24-49 ms). |
| `xbox.ent slowest` | `EntityPlayerSP` 81 veces | Mediana **43 ms**, máximo **354 ms**. Lo siguen PigZombie (70), Dragon (35), Zombie (34) y Blaze (32). |
| `freeKB` | 15,7 MB en juego | Mínimo 2,7 MB, con un `out of memory` en el frame 3301 (`lists=26951KB`, versión vieja). |
| `xbox.tex` | 3 texturas de 1230x216 guardadas como 2048x256 | **2 MB cada una** (menús). |

**Conclusión:** la GPU y la API van sobradas. El tiempo se va en la CPU, en tres sitios:
- la subida de texturas animadas;
- la reproducción de display lists;
- la lógica de juego: jugador, autosave y random ticks.

Melee-X llegó a lo mismo: las ganancias vinieron de **"saltarse trabajo" y del compilador**. Los arreglos "estructurales" de esperas (prefetch planificado, páginas de 4 MB, back-end diferido) no dieron nada o fueron a peor (`Melee-X\docs\fps-plan.md`, "Lessons").

---

## 1. Tabla ordenada (ganancia para nosotros ÷ esfuerzo)

| # | técnica | ganancia esperada en OptiCraft | esfuerzo | fuente / evidencia |
|---|---|---|---|---|
| 1 | **Texturas animadas sin bloqueo:** `LockRect` con `D3DLOCK_NOOVERWRITE` (o dos texturas alternas) y re-swizzle **solo del tile 16x16 que cambia** (`XGSwizzleRect` con `pRect`/`pPoint`, alineado a 8x4 en 32 bpp). | **Alta**: ataca la parte de `textureStage=5 ms/frame` que sea espera o swizzle. Hoy se re-swizzlean 256 KB enteros con `LockRect(...,0)`, que **espera a que la GPU suelte la textura**. Hay que partir el contador para saber cuánto es. | Baja-media | XDK, página `IDirect3DTexture8::LockRect`: "D3DLOCK_NOOVERWRITE keeps the lock from blocking if the texture is in use by the GPU". XDK, `XGSwizzleRect`: rectángulos parciales, "optimized for write-combined memory". ClassiCube `Gfx_UpdateTexture` escribe subrectángulos swizzleados ([Graphics_Xbox.c](https://github.com/ClassiCube/ClassiCube/blob/master/src/xbox/Graphics_Xbox.c)). |
| 2 | **Build de perfil con `d3d8i.lib` + `D3DPERF_GetStatistics` + `D3D__NullHardware`.** | Es medida, no ganancia: dice cuánto hay de `OBJECTLOCK_WAITS` (el punto 1), `PUSHBUFFER_WAITS` y `PRESENT_WAITS`, y si una GPU infinita sube los fps. | Baja (BT3 ya lo tiene hecho, se copia) | XDK `prog_wp_graphicsperf.htm` ("Xbox Graphics Performance Tuning Strategies"). BT3 `buzon\55-INFORME-claude-xdk.md` §1 (`d3dperf_frame`, `-D3DPerf`). |
| 3 | **Tick de `EntityPlayerSP` (43 ms de mediana en las ventanas lentas, 354 ms de máximo):** partirlo en subfases (`moveEntity`/colisión, `pushOutOfBlocks`, recoger objetos, `onLivingUpdate`) y saltarse el trabajo que no hace falta. Ejemplos: rechazo por caja antes de la prueba fina, `reserve` del `std::vector<AxisAlignedBB*>` de colisiones. | **Alta en tirones:** es la causa nº 1. Sin medida de qué subfase es. | Media | Melee-X `fps-plan.md` D: el rechazo por caja en `mplib.c` se salta el 93 % de las pruebas de líneas, con el mismo resultado bit a bit. Puntos de entrada: `Block::getCollidingBoundingBoxes` (`Block.cpp:538`) y `World::getCollidingBoundingBoxes`. |
| 4 | **Autosave repartido o en el hilo de fondo:** serializar NBT y comprimir fuera del tick, como ya hace `XBOX_ASYNC_CHUNK_IO` al descargar, o un tope de chunks por tick. | **Media-alta en tirones** (111 tirones de 24-49 ms). | Media | XDK `prog_wp_maxperf.htm`: E/S en un hilo propio que espera con eventos; escritura sin búfer (`FILE_FLAG_NO_BUFFERING`); fijar el tamaño del fichero antes de escribir (`SetFilePointer`+`SetEndOfFile`) para que no se fragmente; la caché de ficheros por defecto es de 64 KB (`XSetFileCacheSize`). |
| 5 | **`D3DPRESENT_INTERVAL_ONE_OR_IMMEDIATE`** en lugar de `ONE`. | **Media en sensación:** un frame de 17-30 ms se muestra en cuanto está, sin esperar al siguiente vblank (pasar a 33 ms). A cambio puede verse un corte (tearing) en esos frames. No sube los fps medios. | **Muy baja** (una línea) | XDK `D3DPRESENT_PARAMETERS`: "flip immediately if the rendering has gone over a frame". Hoy está en `Display_xbox.cpp:171`. |
| 6 | **Display lists sin `std::function`:** un array plano de comandos POD (enum + datos) en vez de `std::vector<std::function<void()>>`. | Media: `replay=2.1 ms` en 91 listas. Además, menos asignaciones pequeñas al grabar. | Media | Patrón de Melee-X (`renderer.md`, "Display-list cache": planes de decodificación y plantillas en memoria plana). Nuestro código: `RenderAPI_D3D8_XBOX.cpp:600` (`s_lists`). |
| 7 | **Medir la RAM de verdad:** `GlobalMemoryStatus` solo cuenta páginas libres; no ve el hueco libre ni la fragmentación **dentro** del heap del CRT. Añadir un desglose: `XMemAlloc`/`XMemFree`/`XMemSize` propios (ven todo lo que piden D3D, DSound y XNet), ocupación de los `vbPools` y un contador de `new`/`delete` por frame. | Evita sustos: el 1.5.2 va con 9-16 MB libres. Así Melee-X encontró una fuga de 64 KB en 64 KB (la cola de eventos de SDL). | Baja-media | XDK `XMemAlloc.htm` (se puede sustituir; tiene que estar en un .obj del ejecutable, no en una .lib). Melee-X `decisions.md` ("Memory breakdown", `[MEMB]`). Nuestro heap: `XboxCrtShim.cpp:326` (`HeapAlloc` → `RtlAllocateHeap`). |
| 8 | **Menos RAM de vídeo a 720p:** back buffer `R5G6B5` en vez de `X8R8G8B8`. | **~3,5 MB** a 720p (frente+trasero de 1280x720 pasan de 4 a 2 B/px). Riesgo visual: **bandas en el cielo y la niebla**, así que lo tiene que aprobar el usuario. Solo si el 1.5.2 se queda sin RAM. | Baja | Melee-X `architecture.md`: "720p: 1280x720, R5G6B5 + Z24S8 (32-bit colour at 720p does not fit, OpenCrossing's measurement)". Nuestro `Display_xbox.cpp:167`. |
| 9 | **Texturas que no son potencia de 2** (las de 1230x216 guardadas en 2048x256, 2 MB cada una): reescalarlas a 1024x256 o usar una textura lineal `D3DFMT_LIN_*` (pitch múltiplo de 64, ~1 MB). | ~1 MB por textura en menús. Baja el pico al entrar al mundo. | Baja-media | XDK `XGSwizzleRect` (pitch de las texturas lineales). Ojo: una textura lineal usa coordenadas en texels, lo avisa la cabecera de `RenderAPI_D3D8_XBOX.cpp`. |
| 10 | **Orden de funciones en el enlace** (`/ORDER` con `/Gy`, que ya está), sacando la lista de las funciones calientes del perfilador. | Baja: Melee-X midió **+0,9 % fps sin PGO** (0 con PGO). | Media | Melee-X `fps-plan.md` A2. **Sin comprobar** que el `Link.Exe` 7.1 de la XDK acepte `/ORDER` con objetos de VS2022 (es probable, porque ya acepta `/OPT:REF`). |
| 11 | **SSE1 y prefetch en bucles de flotantes** (cuantizar los vértices del mesh, mezclar el audio), solo cuando el resultado sea **idéntico bit a bit**. | Baja-media. Casi toda la lógica es `double` en x87 (53 bits), donde no se puede usar SSE1. | Media | XDK, muestras FastCPU y FastVSConstants: el prefetch es "~90 %" del +68 % de ese bucle de dibujo. Pero en Melee-X el prefetch planificado (B3) **quedó en ruido**, y el mezclador en SSE1 (E) **sí** ganó. |
| 12 | Push buffers grabados (`BeginPushBuffer`/`RunPushBuffer`), `BeginPush`, `XGCompileDrawIndexedVertices`. | **Casi nada para nosotros:** los chunks ya van con `DrawVertices` sin índices y en `QUADLIST`, y D3D se lleva 0,2 ms. Solo serviría con índices, que la XDK copia por CPU. | Alta | XDK `_dx_pushbuffer_resources_graphics.htm` y `prog_wp_graphicsperf.htm` ("Index Buffers are not native GPU resources ... copied into the push-buffer"). |
| 13 | Regiones de tile del framebuffer (+11 % en Melee-X). | **Nada:** el D3D de la XDK ya pone en tile el framebuffer por defecto (en pbkit/nxdk venía mal), y nuestra GPU no es el cuello. | — | Melee-X `renderer.md` "Tile regions". XDK `LockRect`: "default frame buffer surfaces ... reside in tiled memory". |
| 14 | PGO / LTCG / unity build. | — | **Imposible** | La PGO de MSVC necesita `/GL` + `/LTCG:PGI`, y el enlazador de la XDK no acepta LTCG (LNK1262). Melee-X saca su +13 % con clang + lld (ThinLTO + PGO), que es otra cadena de compilación. |
| 15 | `XFreeSection` de secciones de la XDK. | **Ínfima.** En nuestro `.map`: D3D 72 KB, DSOUND 31 KB, XNET 57 KB, DOLBY 28 KB, XGRPH 7 KB. | Baja | `bin\xbox-exp\OptiCraft.exe.map`. XDK `x_jsk_xfreesection.htm` (solo para secciones cargadas con `XLoadSection`). |
| 16 | Flush-to-zero o denormales. | Nada, y **prohibido**: cambia resultados, y la regla es no tocar la semilla. Melee-X midió `FP_ASSIST` por debajo del 1 %. | — | Melee-X `fps-plan.md` B1. |

### Lo que ya tenemos bien (no tocar)
- Los chunks están en VB estáticos (pool con *best fit* y liberación por *fence*), con vértice compacto de 16 B (short3 + byte2 + color + short2) y `D3DPT_QUADLIST` sin índices.
  ClassiCube hace lo mismo con un vértice más grande, y la XDK dice que los índices se copian por CPU.
  Nuestros VB son del D3D de la XDK, así que ya están en memoria contigua write-combined.
  **Ojo:** leer de ellos desde la CPU es lento (XDK `_dx_memory_and_graphics_graphics.htm`).
- Filtro de estado repetido (`StageSignature`, `setRs`/`setTss`) y matrices que solo se mandan si cambian (`setTransform=0.02ms`).
- Iluminación fija apagada (el lightmap va en la etapa 1).
- `BlockUntilIdle` quitado (`XBOX_SMOOTH_FPS_WAITS_FOR_GPU 0`, de 10,5 a 1,6 ms).
- Flags de cl: `/arch:SSE /O2 /GS- /Gy /Gw`. Ya está todo lo razonable.
- `ChunkProvider::lastChunk`, hilo de fondo por debajo de la prioridad normal (`Thread.cpp:211`) y `_controlfp_s(_PC_53)`.

### Hilos (un solo núcleo)
- **Ayudan:** E/S que espera al disco (la XDK lo recomienda: un hilo que duerme en un evento casi no gasta CPU).
- **No ayudan:** lo que es CPU pura. En un solo núcleo solo cambia *cuándo* se paga, más el coste del cambio de contexto y de los candados.
  Cada hilo cuesta al menos 12 KB de pila + 13 KB de objeto (XDK `prog_wp_maxperf.htm`).
- Nuestro `sync=0` en 51 de 73 ventanas confirma que el diseño actual funciona.

---

## 2. Qué medir primero (con nuestros campos del netlog)

1. **Partir `textureStage`** (`s_drawStats.stageCycles`, `RenderAPI_D3D8_XBOX.cpp:504`) en tres contadores:
   - el `LockRect`, que si es grande es espera a la GPU;
   - el `XGSwizzleRect`;
   - el resto (`s_textures.find` y estados).

   Contar también **cuántos `swizzleUpload` hay por segundo**. Si el `LockRect` se lleva más de 1 ms, el punto 1 es seguro.
2. **Build `-D3DPerf`** (punto 2): líneas `OBJECTLOCK_WAITS`, `PRESENT_WAITS`, `PUSHBUFFER_WAITS`; después, `D3D__NullHardware`.
   Si con una GPU infinita no sube nada, **todo es CPU**. Es lo esperado con `present=1.3ms`.
3. **`xbox.ent` de `EntityPlayerSP` por subfases.** Las 81 ventanas lentas tienen `chunkChanges=0`, así que no es cargar chunks.
4. **`worldTick>randomBlocks`** (hasta 170 ms): registrar **qué bloque** se llevó el `updateTick` más lento.
   Sospecha **sin comprobar**: el decaimiento de hojas (recorre 9x9x9) o el fuego.
5. **`freeKB` junto a la memoria interna del heap** (punto 7) y la ocupación de `vbPools` (hoy 15 pools de 512 KB).
6. Regla de Melee-X para decidir: 1 ms ahorrado por tick son 20 ms por segundo (a 20 tps).
   Repetir cualquier medida que mejore menos de un 3 %: el ruido en la consola es de ~1,5 %.

---

## 3. Dónde tocar en nuestro código (los puntos de arriba)

| punto | fichero:línea (`wt-152-codex\src`) |
|---|---|
| 1 Texturas animadas | `platform/RenderAPI_D3D8_XBOX.cpp:391-428` (`swizzleUpload`: `LockRect(0,..,NULL,0)` + `XGSwizzleRect` entero); `:1675-1689` (sub-image: hoy solo marca `t.dirty`; hay que guardar también **el rectángulo sucio**); `:516-519` (subida dentro de `applyTextureStage`). Mantener `_mm_empty()` después de cada `XGSwizzleRect`. |
| 2 D3DPERF | `xbox/lwjgl/Display_xbox.cpp:99` (después de `Present`), `cmake/xbox.cmake:254-276` (cambiar `d3d8.lib` por `d3d8i.lib` en una variante). Copiar de BT3 `gs_d3d8.cpp:2155-2220`. |
| 3 Jugador | `net/minecraft/src/Block.cpp:538`, `World.cpp` (`getCollidingBoundingBoxes`, `getEntitiesWithinAABBExcludingEntity`), `EntityPlayerSP`/`EntityLiving::onLivingUpdate`. Hay 122 `dynamic_cast` en Entity*/World. Algunos ya se cambiaron por etiquetas (`EntityLiving.cpp:266`). Medir antes de seguir con eso. |
| 4 Autosave | El camino `XBOX_ASYNC_CHUNK_IO` (`cmake/xbox.cmake:44`, `platform/chunks/`). La fase `autosave` del spike sale de `worldTick`. |
| 5 Presentación | `xbox/lwjgl/Display_xbox.cpp:171` (`FullScreen_PresentationInterval`). La espera de 30 fps de `:84-96` sigue valiendo igual. |
| 6 Display lists | `platform/RenderAPI_D3D8_XBOX.cpp:600-611` (grabar), `:1898-1920` (reproducir/borrar), `RecordedMesh` en `:1219`. |
| 7 Memoria | `xbox/runtime/XboxCrtShim.cpp:324-330` (heap), `platform/Diagnostics_XBOX.cpp:14-36` y `xbox/java/Runtime_xbox.cpp:24` (`GlobalMemoryStatus`), pools en `RenderAPI_D3D8_XBOX.cpp:1056` (`kVertexPoolBytes`). |
| 8 Back buffer 720p | `xbox/lwjgl/Display_xbox.cpp:167` (`BackBufferFormat`). |
| 9 Texturas NPOT | `RenderAPI_D3D8_XBOX.cpp:404-416` (re-muestreo a potencia de 2) y `ensureStorage` (`:465`). |
| Mesh de chunks (informativo) | `platform/RenderTerrainAPI_D3D8_XBOX.cpp` está vacío a propósito: los chunks van por las display lists → `RecordedMesh` → VB compacto (`:618-720`). |

---

## 4. Notas sobre las fuentes

- **ClassiCube Xbox** (nxdk/pbkit; es lo más parecido como juego de vóxeles):
  - VB en `MmAllocateContiguousMemoryEx(..., PAGE_WRITECOMBINE|PAGE_READWRITE)`;
  - `NV2A_DrawArrays` con QUADS y sin índices;
  - texturas swizzleadas a mano;
  - 6 vertex programs precargados;
  - espera al vblank cada frame, y un `pb_busy()` girando en vacío.

  Su página de descarga dice que es **"early alpha ... may perform poorly"** ([classicube.net/download/xbox](https://www.classicube.net/download/xbox)), así que no da cifras. Nosotros ya estamos por delante en formato de vértice y en filtrar estados.
- **Melee-X:** es la mejor referencia con medidas en consola, pero va sobre nxdk/pbkit, no sobre el D3D de la XDK. Lo de las tiles, las combiners y el vertex cache no se puede trasladar tal cual.
- **No he encontrado** ports con nxdk de Quake 2, Half-Life o DOOM con notas de rendimiento públicas. La búsqueda no dio nada útil y no las cito.
- No he podido comprobar si en el D3D de la XDK `D3DUSAGE_WRITEONLY` cambia algo. La XDK dice que **todo** VB va en memoria contigua write-combined, así que probablemente da igual.

## 09-11: CORRECCIÓN del usuario
El agua/lava/fuego quietos NO es un fallo: son las opciones tipo OptiFine con las animaciones desactivadas.
Falta medir si `textureStage` (~5 ms) se sigue pagando con las animaciones apagadas (si sí, eso sí es un fallo).

## Nota de Claude (03-11 noche): lo que dice el usuario
- AHORA MISMO el agua, la lava y el fuego NO se animan en la consola (se ven quietos). Aun así el netlog cuenta
  ~5-5,5 ms/frame en `textureStage` (swizzle de animadas). O sea: se paga la subida y NO se ve. Revisar primero por
  qué no animan (¿1.5.2 guarda agua/lava/fuego como tiras de animación y `TextureWaterFX/LavaFX/FlameFX` escriben en
  celdas que el atlas ya no usa? ver `BLOCK_SLOT_OVERRIDES` 205/206/237/238 y 31/47 en `build_textures_152.py`;
  CONFIRMADO por el usuario: también pasa en 1.2.5, así que NO es el atlas de 1.5.2: es el camino Xbox de las
  texturas animadas (TextureFX → swizzleUpload / bindTexture del atlas) desde antes del port) y DESPUÉS medir el coste.
- El usuario pidió: por ahora SOLO MEDIR y dejar anotado. Nada de cambios de rendimiento todavía.
- Primer paso cuando toque: build de medición (d3d8i / D3DPERF_GetStatistics / partir `textureStage` y el tick de
  `EntityPlayerSP` en subfases), como BT3 informe 55.
