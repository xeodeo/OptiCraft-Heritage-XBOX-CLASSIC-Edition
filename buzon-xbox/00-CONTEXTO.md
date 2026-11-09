# Traspaso: port de OptiCraft Heritage a la Xbox clásica

Documento para cualquier IA o persona que continúe el trabajo: otro chat de Claude, agy/Gemini, etc. Léelo entero antes de tocar nada.

> **09-11-2026:** desde el 21-10 el proyecto tiene DOS versiones: la 1.2.5 (`xbox-opti-30`, `wt-opti30`) y el port a
> **Minecraft 1.5.2** (ramas `port152-*`, carpetas `wt-152-*`), que es donde va casi todo el trabajo. Cada versión va en
> su carpeta del lanzador (`OptiCraft\1.2.5\`, `OptiCraft\1.5.2\`). Estado, ramas, builds y decisiones del usuario:
> `ESTADO-ACTUAL.md`. Plan de la 1.5.2: `69-PLAN-COMPLETO-152.md`. Cómo se descompila y revisa: `55-GUIA-...`.
> Fuentes reutilizables: LCE en `minecraft-legacy original\`, OptiFine 1.5.2 en `mcp-diff\srcOF152\`.
> Lo de abajo es el contexto de fondo de la 1.2.5 (toolchain, consola real, rendimiento) y sigue valiendo.

Actualizado el 01-10-2026. **Empieza por `ESTADO-ACTUAL.md`**, que dice en qué punto está todo hoy; este archivo es el contexto de fondo, que cambia poco.

La v1.1 está publicada. Desde entonces: pantalla dividida, `assets.pak`, rendimiento (async), la fusión con lo último de OptiJuegos, el crafteo de consola también en PS2, y dos versiones, normal y experimental. **La rama buena es `xbox-opti-30`, en la carpeta `wt-opti30`.** La versión anterior de este archivo es `00-CONTEXTO-antiguo-2026-09-23.md` y está obsoleta.

---

## 1. Qué es y en qué estado está
- **OptiCraft Heritage Edition** (OptiJuegos) es un Minecraft de la época Beta/1.2 reescrito en C++17. Ya existía para PC, PS2 y Wii.
- Lo portamos a la **Xbox original de 2001**:
  - Pentium III a 733 MHz, con SSE1 y **sin SSE2**;
  - 64 MB de RAM compartida;
  - GPU NV2A.
- **Funciona en consola real y en xemu.** El juego está completo:
  - menús Legacy, mundos, Nether y End, skins;
  - sonido DirectSound, estéreo o Dolby 5.1;
  - 720p opcional;
  - multijugador experimental contra servidores Minecraft 1.2.5 (`online-mode=false`);
  - iconos de botones de Xbox y crafteo estilo consola (X = 2x2, mesa = 3x3; opción *Console Crafting*). Y sigue siendo el inventario normal, como en la 360;
  - pantalla dividida estilo Legacy (J2 con sus propias pantallas, nombre y skin);
  - lo de OptiJuegos: cama, creativo por pestañas, World Size (Infinito / Limitado / Isla) y los mods Stronghold Locator y Rei Minimap con waypoints, que se activan o desactivan en Mods.
- Rendimiento en consola real a 2 chunks: ~40-60 FPS, 25-35 MB libres, con algún tirón corto al generar o cargar terreno.

## 2. Dónde está cada cosa
Base: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\`.

| Qué | Ruta |
|---|---|
| Repositorio | La rama buena es **`xbox-opti-30`**, en la carpeta **`wt-opti30`**. La carpeta principal `OptiCraftHeritageEdition\` está en `xbox-unificada`, una versión atrás. |
| Worktrees | `wt-async` (agy), `wt-ltcg` (cerrado), `wt-prev` (`3b9c8a8`), `OptiCraft-prueba-opti` (rama `prueba-opti`) |
| Documentación técnica completa del port | `OptiCraftHeritageEdition\docs\XBOX_PORT.md` (**léela**), y `README.md` para compilar |
| XDK 5849 (no se sube nunca a git) | `xdk\5849\sdk\XDK\xbox` |
| Scripts de trabajo (build, xemu, logs, gdb) | `xbox-spike\` |
| Backups de versiones buenas | `OptiCraft funcional N\` (la más reciente: 15) y `OptiCraft jugable estable\` |
| Releases empaquetadas | `C:\Users\xeodeo\Desktop\release\v1.0\`, `v1.1\` (ISO, zip, RELEASE.md, REDDIT.md) y `media\` |
| Servidor 1.2.5 para probar el online | `servidor-1.2.5\` |
| Planes e investigación | `buzon-xbox\06-plan-optimizacion.md`, `07-investigacion.md`, `13-plan-rendimiento.md` (registro del rendimiento), `28-unificada.md` (la rama unificada) |
| ISO para xemu | `C:\RetroXeo\roms\xbox\OptiCraft.iso`; xemu en `C:\RetroXeo\emulators\xemu\xemu.exe` |
| Carpeta que el usuario manda a la Xbox | `C:\Users\xeodeo\Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\OptiCraft` |

Git:
- Remote **`xbox`** = `https://github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition`. Se sube con `git push xbox xbox-port:main`.
- Remote `origin` es el upstream de OptiJuegos: **nunca hacer push ahí**. Para traer sus novedades: `git fetch origin` y un merge revisado.
- Tags de versión: `xbox-v1.0` y `xbox-v1.1`.

## 3. Compilamos CUATRO versiones del mismo código

Normal o experimental, y cada una de prueba (con escucha) o pública:

| | Prueba (con escucha) | Pública |
|---|---|---|
| Normal (chunks como la 1.1) | `xbox-release` → `bin\xbox\` | `xbox-public` → `bin\xbox-public\` |
| Experimental de rendimiento (`XBOX_ASYNC_CHUNK_IO=ON`: genera y carga chunks en otro hilo) | `xbox-release-exp` → `bin\xbox-exp\` | `xbox-public-exp` → `bin\xbox-public-exp\` |

### Cómo se compila hoy (rama `xbox-opti-30`, carpeta `wt-opti30`)

**Lo normal es lanzar el script**, que hace las tres de golpe (Xbox normal, Xbox exp y PS2), pone el candado y lo quita:

```
powershell -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\build_opti30.ps1"
```

Tarda unos 2-4 minutos. Si dice `BLOQUEADO`, es que otro agente está compilando: hay que esperar.

**Qué es la "exp".** Es el MISMO código con un solo interruptor de compilación, `XBOX_ASYNC_CHUNK_IO=ON`: genera y carga los chunks en un hilo de fondo, en vez de hacerlo el hilo principal. No hay `#ifdef` sueltos repartidos por ahí; el interruptor está en `cmake\xbox.cmake` y por defecto va en OFF (`src\platform\PlatformConfig.h`), así que PC, PS2 y Wii nunca lo llevan. Los presets `-exp` son los únicos que lo encienden.

**A mano**, si solo quieres una:

```
$env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
cd "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30"
& $cm --build --preset xbox-release-exp
```

**Dónde sale y cómo se copia a la consola** (esto NO lo hace el script, se hace a mano):

| Preset | Sale en | Se copia al loader como |
|---|---|---|
| `xbox-release` | `wt-opti30\bin\xbox\iso\` | `default.xbe` y `OptiCraft_720p.xbe` |
| `xbox-release-exp` | `wt-opti30\bin\xbox-exp\iso\` | `OptiCraft_exp.xbe` y `OptiCraft_exp_720p.xbe` |
| `ps2-release` | `wt-opti30\bin\ps2\OptiCraft.elf` | a `Desktop\OptiCraft PS2 prueba\OptiCraftHeritage\` |

El `default.xbe` de `bin\xbox-exp\iso\` vale para los dos nombres de la exp: el "720" del nombre es lo único que selecciona 720p. Antes de sobrescribir, **haz respaldo** en `OptiCraft_respaldo_<fecha>`.

**Avisos:**
- **Candado:** antes de compilar o abrir xemu, mirar `buzon-xbox\BUILD.lock` y crearlo; al terminar, borrarlo (`REGLA-BUILD-LOCK.md`). El script ya lo hace.
- Cada carpeta de build **nueva** empieza sin log. Hay que configurarla una vez con `"-DMC_LOG_LEVEL=1" "-DXBOX_NETLOG_HOST=192.168.0.17"`, **entre comillas**: sin ellas PowerShell parte la IP en los puntos y CMake falla con `Could not read presets from ...\.168.0.17`. Ya pasó una vez y la exp salió sin log.
- `xbox-spike\build_preset.ps1` es el script VIEJO: apunta a la carpeta principal `OptiCraftHeritageEdition`, que está una versión atrás. Para la rama buena, usa `build_opti30.ps1`.

Detalle de la normal:

| | Versión de PRUEBA (con escucha) | Versión PÚBLICA |
|---|---|---|
| Preset | `xbox-release` | `xbox-public` |
| Comando | `powershell -File xbox-spike\build_preset.ps1 -Preset xbox-release` | `powershell -File xbox-spike\build_preset.ps1 -Preset xbox-public -Fresh` |
| Sale en | `OptiCraftHeritageEdition\bin\xbox\` (`iso\` = carpeta para la consola, `OptiCraft.iso` = xemu) | `OptiCraftHeritageEdition\bin\xbox-public\` |
| Log | Cada línea se envía por UDP al PC 192.168.0.17 (`XBOX_NETLOG_HOST` en su caché) con `MC_LOG_LEVEL 1` | Sin log por red (`MC_LOG_LEVEL 0`) |
| Tras compilar | El script copia solo los XBE (y `data` si cambió) a la carpeta de ClassicXboxLoader | Nada automático |
| Para qué | Probar en la consola del usuario y ver el log en vivo | Lo que se publica (ISO/zip de las releases) |

Las dos llevan sonido y multijugador (`XBOX_ENABLE_SOUND`, `XBOX_ENABLE_NETWORK` en `CMakePresets.json`). El script ya define `XBOX_XDK_ROOT`.

Si xemu tiene abierta la ISO, el build enlaza y copia los XBE pero falla al rehacer `OptiCraft.iso` ("File can't be removed"): cerrar xemu y volver a compilar.

**Escuchar el log de la consola:**
- `cd xbox-spike` y `python -u escuchar_log.py`.
- Se guarda en `xbox-spike\netlog.txt`.
- Líneas útiles: `xbox.perf` (cada 5 s), `xbox.spike` (cada frame de más de 45 ms, con su causa), `xbox.mem`, `xbox.draw` y `xbox.mesh`.

**Probar en xemu sin el usuario:**
- `xbox-spike\autotest.ps1 -Seconds 90` compila `bin\xbox-autotest`, entra solo a un mundo, hace una captura (`autotest.png`) y vuelca el log (`autotest_log.txt`). Con `-Script <archivo>` se le da otra secuencia de botones (formato de `autopilot.txt`: `ms BOTON`, `-` suelta). Ojo: cierra cualquier xemu abierto.
- Para abrir una ISO: `xbox-spike\run-xemu.ps1 -Iso <ruta> -Seconds 30`.
- Ojo: xemu **no** detecta los fallos de SSE2 ni los tirones reales; su host es mucho más rápido. La prueba buena es la consola.

## 4. Cómo se hace una release
1. Compilar la versión pública (`-Preset xbox-public -Fresh`) y comprobarla en xemu.
2. Crear `Desktop\release\vX.Y\` con:
   - `OptiCraft-Xbox-vX.Y.iso` (copia de `bin\xbox-public\OptiCraft.iso`);
   - `OptiCraft-Xbox-vX.Y.zip`, que contiene la carpeta `OptiCraft\` con el contenido de `bin\xbox-public\iso\`;
   - `RELEASE.md` (notas en español) y `REDDIT.md` (español/inglés).
3. Commit y `git push xbox xbox-port:main`. Después, el tag `xbox-vX.Y` y `git push xbox refs/tags/xbox-vX.Y`.
4. La página de la release en GitHub la crea el usuario.

## 5. Reglas del usuario (importantes)
- **Todo lo que se le escribe va en español, corto (2-5 líneas), sin jerga en inglés**, incluidos los mensajes de progreso. Lo ha pedido varias veces.
- **No subir nunca a git** el XDK ni binarios enlazados con él (`.xbe`, `.iso`, `.exe`, `.lib`).
- Commits con `git -c user.name=xeodeo -c user.email=kelopez@seedem.co commit ...`. Hacer commit o push solo cuando lo pida (suele pedirlo).
- Tras cada versión que el usuario confirma como buena: backup `OptiCraft funcional N` con `carpeta-xbox`, ISO, `.exe` y `.map`, y copia del ISO a `C:\RetroXeo\roms\xbox\OptiCraft funcional N.iso`.
- **Siempre dos XBE:** `default.xbe` (480p) y `OptiCraft_720p.xbe`. Es el mismo binario: si el nombre contiene "720", usa 1280×720 (`src/xbox/system/XboxVideoMode.cpp`). El usuario juega en 720p con cable por componentes de 360. Se mide a 480, pero hay que comprobar que 720p sigue funcionando (gasta ~7 MB más).
- **Sin pérdida de calidad visual.** Nada de greedy meshing ni cambios que se vean distintos.
- Prefiere **interruptores** (`#define X 0`) antes que borrar código. Ejemplos:
  - `XBOX_SMOOTH_FPS_WAITS_FOR_GPU`;
  - `XBOX_DISK_LOG`;
  - `XBOX_MESH_DIAGNOSTICS`.
- **No tocar el sonido** sin su permiso.
- **Código de Legacy Console Edition** (`Downloads\minecraft`): el 24-09-2026 el usuario cambió la regla. Se pueden tomar recursos y usarlo como guía (lo de "clean-room" es del proyecto original de OptiJuegos, no de este port). **Nunca modificar** esa carpeta.
  - Ya tomado, siempre con scripts en `scripts/xbox/`:
    - iconos de botones Xbox (`X360ControllerIcons`) → `make_button_atlas.py` genera `data/assets/gui/buttons_xbox.png`, `legacy/XboxButtonAtlasData.h` (coordenadas) y `legacy/XboxButtonAtlasPng.h` (copia dentro del XBE, por si la consola tiene un `data` viejo). Blanco/Negro se dibujan aparte.
    - pestañas y tipos de objeto del crafteo → `make_crafting_groups.py` genera `legacy/LegacyCraftingGroups.h`.
    - iconos de pestaña y flecha del crafteo → `make_crafting_icons.py` genera `data/assets/legacy/crafting_icons.png` y `legacy/LegacyCraftingIconsPng.h`.
  - El diseño de `legacy/LegacyCraftingScreen.cpp` usa las medidas y colores de la escena de crafteo de Legacy a 1280x720, dibujado con rectángulos.
- Cuando diga "no cambies nada, solo dime": solo analizar.
- Los arreglos de rendimiento que retrasan contenido **se probaron y se rechazaron**: `PLATFORM_DEFERRED_POPULATE`, `PLATFORM_INCREMENTAL_POPULATE` y las cargas de disco espaciadas hacían que los árboles, los mobs y la nieve tardaran en salir. El usuario prefiere tirones ocasionales a contenido tarde. Mejor abaratar el trabajo que aplazarlo, y validar siempre en la consola real.
- **agy (Gemini 3.1 Pro) y OpenCode (solo modelos GRATIS)** hacen el análisis y el código grande para ahorrar créditos de Claude. Se les dan encargos en `buzon-xbox\NN-ENCARGO-*.md`, cada uno en su propio worktree, y hay que verificar siempre lo que dicen con `git diff`: se han inventado cambios.
- No dejar contenido tarde (árboles, mobs, nieve), no romper el Ender Dragon, y mantener la cantidad de mobs y su IA.

## 6. Cosas técnicas que no son obvias
- **SSE2:** el CRT de VS2022 trae SSE2 sin comprobar la CPU. xemu lo ejecuta, pero la Xbox real se cuelga.
  - Arreglos: `src/xbox/runtime/XboxP3Math.c` (sustitutos x87) y `scripts/xbox/patch_sse2_moves.ps1` (reescribe SSE2→SSE1 después de enlazar).
  - El build avisa del SSE2 que queda (`frexp`, `powhlp`, `handle_exc`, `num_put`); se supone que esas rutas no se usan. Si la consola se cuelga y xemu no, sospecha primero de esto.
- **Render:** `src/platform/RenderAPI_D3D8_XBOX.cpp` emula GL sobre D3D8: display lists, pools de vertex buffers, vértices compactos de 16/12 bytes y quads nativos.
- **Chunks:** usa el constructor incremental de PC Legacy (`PLATFORM_INCREMENTAL_TERRAIN_BUILD`) y la generación por pasos de PS2. Los ajustes están en `src/xbox/XboxTuning.h`.
- **Red:** `src/xbox/system/XboxNetwork.*` (XNet, con tipos C planos, porque la STL del XDK no es compatible con la de VS2022) y `src/java/JavaNetworkXbox.cpp` (sockets TCP del multijugador).
  - Conectar tiene un límite de 4 s, y un socket sin datos durante 45 s se da por muerto.
- **Captador de fallos** (`src/xbox/system/XboxCrashLog.cpp`):
  - Un acceso inválido en cualquier hilo, o una excepción que escapa de un hilo de trabajo, congelaba la consola **sin avisar**. El audio se quedaba repitiendo su último trozo; ese "ruido feo" es síntoma, no causa.
  - Ahora sale en el log `crash: unhandled exception ... on MAIN/worker thread ... (map address X)`, con registros y pila. Las direcciones se buscan en el `.map` de ESA compilación.
  - Así se encontró el cuelgue del multijugador: la mano dibujaba un `ItemStack` ya borrado por el servidor. Arreglo: `isHeldByInventory` en `ItemRenderer.cpp`.
- **Trampa al unir con OptiJuegos cogiendo "su versión":** sus `#if PLATFORM_PS2 || PLATFORM_WII` no incluyen Xbox. Al quedarse con lo de ellos se perdieron nuestros `|| PLATFORM_XBOX` (el teclado en pantalla dejó de recibir botones al crear mundo). Después de cada unión, busca las líneas con `XBOX` que se hayan borrado: `git diff <antes> -- src | grep "^-.*XBOX"`.
- **Crafteo estilo consola:** solo es una interfaz. A coloca la receta en la cuadrícula real del contenedor con clics de ventana y saca el resultado con shift-clic, así que sirve igual en servidores. Blanco/Negro llegan a los menús como `PLATFORM_TEXT_TAB_LEFT/RIGHT`.
- **Mundos:** se guardan en `T:\` = `E:\TDATA\FFFF4F43\` de la consola.
- **Consola del usuario:** IP 192.168.0.3, FTP `xbox:xbox`. Al abrir el juego se cierra el FTP del dashboard; por eso existe el log por red. El usuario lanza la carpeta desde el disco duro, no la ISO.
- **Pendiente conocido:** a veces un mob en el borde de la zona cargada queda un poco hundido. Es por el poco radio de chunks cargados; el usuario dijo que lo dejemos así.
- **Async (solo en la exp):**
  - `ChunkGenerationScheduler` genera terreno y cuevas; el hilo principal adopta, puebla y enciende la luz;
  - el trabajador tiene su propio `BiomeSource` y hueco de `IntCache`;
  - el cerrojo SRW de `XboxCrtShim.cpp` duerme tras girar, porque había inversión de prioridad;
  - el bucle principal cede 1 ms si hay trabajos pendientes;
  - traza `xbox.async` cada 5 s.
- **`chunkUnload`** en `xbox.spike` es todo el mantenimiento de chunks del tick: adoptar, poblar, pedidos, guardar/descargar y el recorrido del mapa. Siguiente paso: partirlo en subfases para ver cuál pesa.
- **Arreglos del 29-09:**
  - crash por sección borrada (en Xbox ya no se borra la sección vacía);
  - dragón con 3x3 chunks retenidos;
  - carrera en el guardado (`AnvilChunkLoader` no da un chunk por guardado hasta escribirlo);
  - Blanco/Negro en el creativo (`XboxPad::consumePagePressed`);
  - waypoints con Back + cruceta;
  - stick derecho en los logros.
- **Vigilante** (solo en builds con log): `xbox.watchdog main stuck ... phase=...`. Un crash deja `map address`: se busca en el `.map` de ESA build.

## 7. Siguiente trabajo pendiente
- **La lista de pendientes de hoy está en `ESTADO-ACTUAL.md`.** Aquí queda solo lo de fondo.
- Después: versiones públicas normal + exp, y release/push **solo cuando lo pida**.
- Rendimiento pendiente: partir `chunkUnload` en subfases; medir las entidades por tipo (`entTick` ~34 ms) sin cambiar la cantidad de mobs ni su IA.
- **Música:** en Xbox y PS2 va como el Minecraft 1.2.5 original (nada en el menú, primera canción tras una espera al azar dentro del mundo, 10-20 min entre canciones). OptiJuegos la cambió y lo revertimos con `#if !(PLATFORM_XBOX || PLATFORM_PS2)`. El usuario quiere que se quede así.
- Hecho: pantalla dividida estilo Legacy (`buzon-xbox\08-pantalla-dividida.md`).
- **Hecho (25-09-2026, commit 965efdb):** botones Xbox y crafteo estilo consola. La release `Desktop\release\v1.1` se reemplazó con esta versión y el tag `xbox-v1.1` se movió a 965efdb. Backup de la primera versión del crafteo: `OptiCraft crafteo consola v1\`.
- **1080i** (`OptiCraft_1080i.xbe`): lo hizo otra sesión y está sin commit en el working tree. El usuario lo dejó fuera de la release hasta probarlo en consola. Copia: `buzon-xbox\1080i-otra-sesion.patch`.
- **Optimización** (micro-tirones): ver `buzon-xbox\06-plan-optimizacion.md` y `07-investigacion.md`. Todo debe medirse con `xbox.spike` en la consola.
- **Ideas de Legacy para optimizar** (analizadas, sin empezar), por orden:
  1. día/noche sin reconstruir chunks (hoy `World.cpp` llama a `updateAllRenderers` cuando cambia la luz del cielo; comprobar que se ve igual);
  2. caché de luz 16³ y topes a la propagación;
  3. tope a los ticks aleatorios de bloques;
  4. saltar al mallar los bloques totalmente rodeados y las secciones vacías;
  5. luz guardada por planos (planos todo 0 / todo 15 no ocupan memoria);
  6. guardar bloques y luz sin NBT (cuidar los mundos existentes);
  7. hilo de baja prioridad en los ~10 ms libres que deja la espera de vsync.
- Aplazado: zona pregenerada al crear el mundo.
- Los ocelotes ya existen (salen en jungla, no en Pacífico).

## 8. PS2 (añadido el 01-10-2026)
- Desde la unión del 30-09 el port también toca PS2: nuestro menú de crafteo (`XboxCraftingScreen`) funciona en PS2, y sus botones siguen la distribución de la Xbox (Cruz saltar, Círculo soltar, Triángulo inventario, Cuadrado crafteo, R2 atacar, L2 usar, L1/R1 objetos, L3 agacharse, R3 cámara).
- Una sola casilla **Console Crafting** (clave `xboxStyleCrafting`) en Xbox y PS2; la de OptiJuegos (`legacyCrafting`) está oculta en esas dos plataformas y sigue viva en Wii y PC.
- **Compilar PS2 en Windows** (recomendado antes de proponer nada a OptiJuegos: detecta guardas de plataforma que faltan):
  - compilador ya descargado en `OptiCraftHeritageEdition\psdevwindows` (de `ps2dev-windows-latest.tar.gz`), con las DLL de MinGW de 32 bits copiadas en **todas** las carpetas con `.exe`, no solo `ee/bin`;
  - `cmake --preset ps2-release` y `cmake --build --preset ps2-release`; sale `bin/ps2/OptiCraft.elf` y `bin/ps2/usb/MCBETA`;
  - en otra carpeta de trabajo, enlaza el compilador con `New-Item -ItemType Junction -Path "<carpeta>\psdevwindows" -Target "...\OptiCraftHeritageEdition\psdevwindows"`.
- Para probar: copiar a la raíz del USB la carpeta `OptiCraftHeritage` con `OptiCraft.elf`, `data\irx\` y `assets.pak`. Copia lista en `Desktop\OptiCraft PS2 prueba\`.
- El `assets.pak` necesita `assets/gui/buttons_ps2.png`, que OptiJuegos guarda suelto en la raíz del repo; sin él no se ven los iconos de botones. Se añade con `scripts/unpack_pak.py` + `scripts/make_pak.py`.

## 9. Si eres Claude Code
Tu memoria persistente de este proyecto está en `C:\Users\xeodeo\.claude\projects\C--Users-xeodeo-Desktop-Minecraft-en-xboox-clasico-OptiCraftHeritageEdition\memory\` (índice `MEMORY.md`). Este documento resume lo mismo para las IAs que no la leen. Si cambia algo importante, **actualiza los dos**.
