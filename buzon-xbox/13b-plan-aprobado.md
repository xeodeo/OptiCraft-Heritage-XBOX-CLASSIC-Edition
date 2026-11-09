# Plan: rendimiento Xbox, puntos 1 (carga/guardado en hilo aparte) y 3 (LTCG)

## Contexto
- El juego va a ~60 FPS, pero tiene **tirones**. En el último log de la consola:
  - entidades: 27-38 ms;
  - guardado automático: 63 ms;
  - cargas de chunk: 10-50 ms, alguna de 174 ms.
- El usuario eligió dos mejoras:
  - **3. LTCG**, barata: 5-15 % de CPU sin tocar el juego;
  - **1. Cargar y guardar chunks en un hilo aparte** que use los ~10 ms libres del vsync. Es la que más quita tirones.
- **Regla del usuario:** el análisis y el código los hacen **agy y OpenCode** (modelos gratis). Claude solo reparte, revisa y compila para la consola. Nada de subagentes de Claude.
- **Reglas del proyecto** (`buzon-xbox\00-CONTEXTO.md`):
  - no retrasar contenido: árboles y mobs tarde = rechazado;
  - sin pérdida visual;
  - usar interruptores `#define`;
  - probar siempre en la consola real.

## Paso 0 (Claude, antes de repartir)
1. **Guardar** el texto de la propuesta (puntos 1-5) en `buzon-xbox\13-plan-rendimiento.md`, junto con este plan.
2. **Commit local, sin push**, de lo pendiente en `xbox-port`:
   - pantalla dividida, J2, menús;
   - arreglo del `.pak` (`Resources_XBOX.cpp`);
   - skins y demás.

   Así cada agente parte de un árbol limpio y no se mezcla trabajo.
3. **Una carpeta de trabajo propia por agente**, para que no se pisen compilando:
   - `git worktree add ..\wt-ltcg -b xbox-ltcg` → **OpenCode**;
   - `git worktree add ..\wt-async -b xbox-async-io` → **agy**.
   - Cada uno compila solo en su carpeta, con su propio `build\` y `bin\`. El `data` común se pasa como `-DXBOX_DATA_DIR=<ruta absoluta del repo principal>\data`.
   - **Ninguno usa** `xbox-spike\build_preset.ps1`: ese script apunta al repo principal y copia a ClassicXboxLoader. Se les da el comando `cmake` exacto (preset `xbox-release`, `XBOX_XDK_ROOT` definido).
   - Solo Claude copia a ClassicXboxLoader.

## Reparto (en paralelo, sin tocar lo del otro)
| | OpenCode (Nemotron 3 Ultra Free) | agy (Gemini 3.1 Pro) |
|---|---|---|
| Tarea | 3. LTCG | 1. Carga/guardado en hilo |
| Carpeta | `..\wt-ltcg` | `..\wt-async` |
| Suyos | `cmake\xbox.cmake`, `CMakePresets.json` | `src\net\minecraft\src\ChunkProvider*`, `AnvilChunkLoader*`, `RegionFile*`, `ThreadedFileIOBase*`, `src\xbox\XboxTuning.h` |
| No suyos | todo `src\` | `cmake\`, todo lo de UI, input y pantalla dividida |
| Encargo | `buzon-xbox\14-ENCARGO-opencode-ltcg.md` | `buzon-xbox\15-ENCARGO-agy-async-io.md` |

### Encargo 14: LTCG (OpenCode)
- Añadir la opción `XBOX_LTCG` (ON por defecto en `xbox-release` y `xbox-public`):
  - `/GL` en las opciones de C/C++ (`cmake\xbox.cmake:138`, junto a `/O2`);
  - `/LTCG` en la línea de enlace (`:248`);
  - no ponerlo en `XboxP3Math.c` (`:83`, compilado con `/arch:IA32`).
- Compilar en su worktree.
- **Aceptación:**
  - enlaza sin errores;
  - `patch_sse2_moves.ps1` termina sin avisos **nuevos** de SSE2 (comparar con la lista actual: `frexp`, `powhlp`, `handle_exc`, `num_put`);
  - arranca en xemu (`xbox-spike\run-xemu.ps1 -Iso <su iso> -Seconds 30`, captura con el menú);
  - informe con el tamaño del XBE antes y después.
- Informe en `buzon-xbox\14-INFORME-opencode.md`.

### Encargo 15: hilo de carga y guardado (agy)
- **Primero analizar** e incluirlo en el informe antes de programar. Por dónde pasa hoy la carga síncrona:
  - `ChunkProvider.cpp:618-630` (`prepareChunkInternal`);
  - `AnvilChunkLoader::loadChunk` (`:130-185`): leer sectores + inflar + NBT;
  - el guardado (`AnvilChunkLoader.cpp:254-294` NBT en el hilo principal; comprimir y escribir en `ThreadedFileIOBase`);
  - el mutex de `RegionFile::write` (`:474-500`);
  - el mesh builder, que pide cargas (`WorldRendererPcLegacy.cpp:332`).
- **Reusar** lo que ya existe:
  - `ThreadedFileIOBase` (hilo de E/S);
  - el patrón de `ChunkGenerationScheduler` (`PLATFORM_ASYNC_CHUNK_GENERATION`, hoy solo Wii y PC legacy);
  - `platform\Thread.cpp` (`SetThreadPriority`).
- **Diseño pedido, detrás de un interruptor `XBOX_ASYNC_CHUNK_IO` en `XboxTuning.h`:**
  - leer disco + inflar + parsear NBT en un hilo de **prioridad baja**;
  - el hilo principal solo adopta el chunk ya decodificado;
  - radio síncrono 1, como hoy, para no retrasar lo que está al lado del jugador;
  - el guardado construye el NBT fuera del hilo principal y el mutex de región no se mantiene durante `compress2`;
  - la población (árboles) sigue igual, inline.
- **Aceptación:**
  - compila en su worktree;
  - arranca en xemu y entra a un mundo (autotest con `-Script`);
  - con el interruptor en 0, el comportamiento es idéntico al actual;
  - informe con los cambios `archivo:línea` y los riesgos (hilos, memoria, mundos existentes).
- Informe en `buzon-xbox\15-INFORME-agy.md`.

### Comunes a los dos encargos (por la guía `Desktop\COMO-USAR-AGY.md`)
- Rutas absolutas; no escribir en el Desktop ni en el repo principal.
- "Si algo no está en tu lista, para y pregunta; si crees que el plan está mal, dilo."
- Informe con primera línea literal y `worker_done` con la dispatch capability.
- agy arranca con `--dangerously-skip-permissions`; comprobar su pantalla con `--screen`.
- OpenCode ya tiene permisos globales. Su terminal está atada a una tarea antigua en Orca, así que el encargo se le manda por `orca terminal send`.

## Lanzar y vigilar (Claude)
- Encargo a fichero + puntero corto por la terminal (`orca terminal send`).
- Vigilar con un `Monitor` sobre `buzon-xbox\1[45]-INFORME-*.md`.

## Verificación y cierre (Claude)
1. Revisar los informes y el diff de cada worktree. No fiarse de las explicaciones (guía §10).
2. Integrar primero **LTCG** en `xbox-port`, compilar la versión de prueba con `build_preset.ps1` → ClassicXboxLoader. El usuario prueba en la consola que arranca.
3. Luego integrar **async IO** y compilar con el interruptor en 1. El usuario juega y trae el `netlog`. Comparar `xbox.spike` (`load`, `save`, `autosave`, `chunkUnload`) antes y después. Comprobar que los árboles y los mobs no salen más tarde.
4. Si algo empeora, interruptor a 0.
5. Anotar el resultado en `buzon-xbox\13-plan-rendimiento.md` y `00-CONTEXTO.md`.
6. Push y release solo cuando el usuario lo pida.
