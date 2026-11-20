# 87. Encargo agy: traer de erluza los arreglos y las optimizaciones que sirven (1.5.2 y 1.2.5) (19-11)

## Qué es erluza
https://bolha.dev/erluza/OptiCraftHeritage es otro fork del mismo OptiCraftHeritageEdition de OptiJuegos (el GitHub de
OptiJuegos lo tumbó una DMCA). Desde nuestra base común (`ca93c04`, 29-09) lleva 144 commits: PSP, 3DS, Wii, PC,
servidores 1.8.9, etc. Ya está descargado en el repo como **`erluza/main`** (rama remota local; `git show <hash>` funciona
desde cualquier worktree). No hay que fusionarlo entero: son 2763 ficheros y movieron/borraron mucho.

Pedido del usuario: «lo de la 1.8.9 no de momento, lo que de verdad nos sirva y las optimizaciones de PS2 etc, todo eso
sí lo quiero, 1.2.5 y 1.5.2».

## Dónde
- **1.5.2**: worktree `wt-152-rend`, rama `port152-erluza` (creada por Claude desde `port152-aldeanos` `ce80a3b`).
- **1.2.5**: worktree `wt-opti30`, rama `opti30-erluza` (creada por Claude desde `opti30-cruceta` `fd2d3a0`).
- Primero 1.5.2 entera, compilada y probada; luego lo mismo en 1.2.5.
- NO toques otras carpetas ni ramas. Sin `reset --hard`. Sin push.

## Cómo
Por cada commit de la lista: `git show <hash>`, compara con nuestro código y decide:
- **ya estaba** (mucho de PS2 PR #59 ya entró por la fusión de OptiJuegos: p. ej. `Ps2OptimizationValidation.cpp` y
  `Ps2WeatherMath.h` ya existen),
- **aplicar** (a mano o `git cherry-pick -n` + arreglar), solo la parte genérica o de Xbox,
- **descartar** (explica por qué).
Un commit nuestro por cada commit de erluza aplicado, con el mensaje terminado en `(de erluza <hash>)`.
Lo que en erluza sea solo de PS2 (`src/ps2/`, `_PS2.cpp`, `PLATFORM_PS2`) pero sirva en Xbox: llévalo a la parte genérica
o a la de Xbox con su guarda `#ifdef` de Xbox, sin cambiar el comportamiento de PC/PS2/Wii.

### A. Arreglos de cuelgues y estabilidad (prioridad 1)
| hash | qué |
|---|---|
| `fe87a9d` | ChunkProvider: borrar la caché `lastChunk` antes de liberar el chunk (SIGSEGV) + `getBlockId` con chunk nulo |
| `1c58d9b` | cuelgue, autosalto en losas, borde del mundo (la parte genérica; nada de `_3DS`) |
| `b134181` | **solo** «persist player edits on unload» (Chunk / ChunkProvider / LoadOrGenerate); la pantalla de multijugador NO |
| `c2e3c1b` | liberar al jugador después del cambio de dimensión, no durante (Minecraft.cpp, World.cpp) |
| `92f557e` | no congelarse en los bordes de chunks al cargar (Entity.cpp, EntityRenderer.cpp) |
| `9344c39` | **solo** `WorldClient.cpp`: cuelgue al dormir y al reaparecer |
| `a1772d5`, `423bb61`, `f7666dd` | (PSP) End: movimiento y mano, sitio de aparición, recrear al jugador 1 al cambiar de dimensión. Solo lo genérico, si nos pasa lo mismo |

### B. Rendimiento (prioridad 2)
| hash | qué |
|---|---|
| `c2b468f` | lag enorme al explotar TNT (EffectRenderer, Explosion, World). Mira bien lo de EntityLiving (340 líneas) y EntityArrow: separa lo que sea del arco (va en C) |
| `10bccb6` | `EntityOcclusionCache` (nuevo), caché de FontRenderer, RenderGlobal, RenderBlocks; tuning `_3DS`/`_WII` no |
| `5019d11` | RenderBlocks y WorldRenderer (lo de 3DS/QR no) |
| `80563df` | la parte de «performance regression»: RenderGlobal, Chunk, BlockRedstoneWire, BlockPistonBase, EntityRenderer, World.h |
| `42aecc9` | TextureCompassFX, TextureWatchFX, RenderEngine, EntityRenderer |
| `031c115` | BiomeDecorator, ChunkProviderGenerate, PopulateIncremental, sliders. **CUIDADO: ver trampa 1** |
| `ec3921d` | simulación del mundo y ritmo de frames (World, WorldRenderer); los valores `_PSP` no |
| `047e42c` | precarga de chunks |
| `07a80e1` | NetClientHandler (si es genérico y barato) |
| `9024ff6` | opción «hielo opaco». Opción nueva = en las DOS interfaces (Java/OptiFine y Legacy `legacyUI`) |
| `6790c69` → `221cfce` (revert) → `f1ad348` | «Pocket Optimizations» de PS2: se puso, se quitó y se volvió a poner. Mira cómo quedó en `erluza/main` y trae lo que sirva |

### C. Jugabilidad (prioridad 3; solo si nuestro juego falla igual)
Arco: `c889ee7`, `8f5048f`, `d6c7d71`, `7be0217`, `94d8924`, `d31a4aa`. Dicen que reimplementan el arco «de MCPE PS2».
**Nosotros queremos el comportamiento de Java** (1.2.5 / 1.5.2 según la rama). Compara con `mcp-diff` (Java) y trae solo
lo que corrija un fallo real de puntería u origen de la flecha que nosotros también tengamos. Si nuestro arco ya va
como Java, descártalo.

### D. NO traer
1.8.9 y proxy, login de Microsoft, mbedtls, CUDA, multi-GPU, Discord, render por software, PSP, 3DS, Wii, PC, QR,
mods compilados, explorador de ficheros, Server Utils, skins online, gestor de texturas, pantalla partida de PS2/Wii,
mandos de Xbox de PC, menú de vídeo rediseñado. **El creativo NO se toca.**

### E. Solo apuntar en el informe (el usuario decidirá)
Panorama nocturno en Legacy UI (`4cbab3a`), reproductor de música (`6277a95`), coordenadas estilo Bedrock (`48a4a97`),
FPS con fondo (`720f795`), menú de pausa con puntuación (`b71d0e8`).

## Trampas
1. **Nunca cambiar el terreno de una semilla.** Nada que toque números de la generación (BiomeDecorator, ChunkProvider*,
   MapGen*, ruido). Si un cambio de `031c115` u otro altera qué se genera, descártalo. Prueba: misma semilla
   (1621073931) antes y después, mismo spawn y mismas estructuras, en foto. Nada de `/fp:fast`.
2. **No perder los ~60 fps** de la 1.5.2 (netlog del 09-11: mediana 59,9, mínimo 55, HUD 0,2-0,4 ms). Mide en xemu
   antes y después (`xbox.perf`, `xbox.spike`).
3. Xbox clásica, 64 MB: mira `KB libres` antes y después. Una caché nueva (p. ej. `EntityOcclusionCache`) cuesta memoria.
4. Compilador del XDK (MSVC viejo): erluza usa C++17 en PC; nada de `std::optional`, `if constexpr`, `auto` en lambdas,
   etc. Si no compila, adáptalo. Revisa los avisos «SSE2 left in» y que no salgan `nan` ni `[X87]` en el netlog.
5. Nuestras ramas tienen cosas que erluza no tiene (ticks por chunk `XBOX_CHUNK_TICK_SCHEDULER`, niebla y cielo de
   Xbox, aldeanos, Wither...). Al aplicar, no pises eso.
6. `BUILD.lock` antes de compilar o abrir xemu (`buzon-xbox\REGLA-BUILD-LOCK.md`).
   1.5.2: `cmake --build build/xbox-release` y `build/xbox-release-exp` en `wt-152-rend` (si hay objetos de otra rama,
   reconstruye limpio). 1.2.5: `powershell -File buzon-xbox\build_opti30.ps1`.
   Si configuras desde cero: `"-DXBOX_DATA_DIR=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data"`.
   NO copies nada al lanzador (eso lo hace Claude).

## Aceptación
- Las dos ramas compilan (480 y 720p) y arrancan en xemu: crear mundo, andar, romper y poner bloques, TNT, ir al Nether
  y al End y volver, guardar y cargar.
- TNT: medir el tirón en el netlog antes y después (`xbox.spike`).
- Misma semilla = mismo terreno (fotos).
- Fps y memoria iguales o mejores (números).

## Informe
`buzon-xbox\87-INFORME-agy-erluza.md`: una tabla con cada hash y qué pasó (ya estaba / aplicado en <commit> / descartado
y por qué), en las dos ramas; mediciones antes/después; qué tiene que probar el usuario en la consola real.
Señal de fin en tu terminal: `ENCARGO 87 LISTO` seguido de los dos commits finales (1.5.2 y 1.2.5).
