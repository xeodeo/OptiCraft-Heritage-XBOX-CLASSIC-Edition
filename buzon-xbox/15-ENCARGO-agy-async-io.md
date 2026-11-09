# ENCARGO 15 (agy): cargar y guardar chunks fuera del hilo principal

## Contexto (medido en la consola real)
- Juego: OptiCraft (Minecraft 1.2.5 en C++17) para la **Xbox original**: 1 núcleo Pentium III a 733 MHz y 64 MB de RAM.
- Va a ~60 FPS. Cada fotograma pasa ~10 ms esperando al vsync.
- Tirones medidos (`xbox.spike` en el netlog):
  - cargas de chunk (`load`) de 10-50 ms, alguna de 174 ms;
  - `autosave` de 63 ms;
  - `chunkUnload` de 15 ms.
- Hoy todo esto va en el hilo principal. Solo comprimir y escribir la región va en un hilo aparte (`ThreadedFileIOBase`).
- **TU carpeta de trabajo (worktree git, rama `xbox-async-io`):** `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-async`

## Dónde está hoy (rutas en `wt-async\src\net\minecraft\src\` salvo aviso)
- `ChunkProvider.cpp:618-630` (`prepareChunkInternal`): carga de disco síncrona antes de generar.
- `AnvilChunkLoader.cpp:130-185` (`loadChunk`): lee sectores, infla y parsea NBT.
- `AnvilChunkLoader.cpp:254-294`: el guardado construye el NBT en el hilo principal; `:626-660` es la parte del hilo de E/S.
- `RegionFile.cpp:474-500`: `write` mantiene el mutex de región durante `compress2`. `:350,359`: las lecturas del hilo principal esperan ese mutex.
- `ChunkProvider.cpp:1364-1545` (`unload100OldestChunks`): guardado síncrono al descargar.
- `WorldRendererPcLegacy.cpp:332`: el mesh builder pide columnas y puede provocar una carga de disco.
- **Reusable:**
  - `ThreadedFileIOBase.*` (hilo de E/S);
  - el patrón `ChunkGenerationScheduler` (`PLATFORM_ASYNC_CHUNK_GENERATION`, `src\platform\PlatformConfig.h:91`, hoy solo Wii y PC legacy);
  - `src\platform\Thread.cpp` (`SetThreadPriority`);
  - los ajustes de Xbox en `src\xbox\XboxTuning.h`.

## Reglas del usuario (obligatorias)
- **No retrasar contenido.** Ya se rechazó la decoración diferida porque los árboles y los mobs salían tarde. La población (árboles) sigue inline, como ahora.
- **Sin pérdida visual.**
- **Todo detrás de un interruptor** `XBOX_ASYNC_CHUNK_IO` en `XboxTuning.h`. Con valor 0, el comportamiento debe ser idéntico al actual.
- Radio síncrono 1 (`PLATFORM_GENERATE_SYNC_RADIUS`), como hoy: lo que está pegado al jugador se carga en el acto.

## Qué hacer
1. **Analiza primero.** Escribe en el informe el flujo actual de carga y de guardado, y tu diseño, ANTES de programar.
2. **Implementa:**
   - un hilo de prioridad BAJA lee de disco, infla y parsea NBT;
   - el hilo principal solo adopta el `Chunk` ya decodificado (poco a poco, en el tick);
   - el guardado construye el NBT fuera del hilo principal, o al menos no mantiene el mutex de región durante `compress2`.
3. **Compila** en TU carpeta, en PowerShell:
   ```powershell
   $env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
   $cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
   cd "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-async"
   & $cm --preset xbox-release "-DXBOX_DATA_DIR=C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition/data"
   & $cm --build --preset xbox-release
   ```
   Sale `wt-async\bin\xbox\OptiCraft.iso`.
4. **Prueba en xemu** que arranca y llega al menú. Cierra xemu antes y después:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\run-xemu.ps1" -Iso "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-async\bin\xbox\OptiCraft.iso" -Seconds 30
   ```
   Entrar a un mundo lo prueba Claude después, en la consola real.

## Tuyos / no tuyos
- **Tuyos:** en `wt-async\src\net\minecraft\src\`: `ChunkProvider*`, `AnvilChunkLoader*`, `RegionFile*`, `ThreadedFileIOBase*`, más un archivo nuevo para el hilo si hace falta, y `wt-async\src\xbox\XboxTuning.h`.
- **NO tuyos:**
  - `cmake\`, UI, input, pantalla dividida, render y sonido;
  - la carpeta `OptiCraftHeritageEdition\` (el repo principal);
  - `wt-ltcg\` (es de OpenCode);
  - `xbox-spike\build_preset.ps1` (no lo uses);
  - ClassicXboxLoader.
- No hagas commit ni push. Si algo no está en tu lista, para y pregunta. Si crees que el plan está mal, dilo.

## Aceptación
- Compila y enlaza sin errores con `XBOX_ASYNC_CHUNK_IO 1` y con `0`.
- xemu llega al menú.
- El diseño deja la población inline y el radio síncrono 1.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\15-INFORME-agy.md`. Primera línea literal: `# INFORME 15 agy`. Incluye:
- el análisis;
- el diseño;
- los cambios `archivo:línea`;
- los riesgos (hilos y memoria, mundos existentes, cierre del juego con cargas pendientes);
- el resultado de la compilación y de xemu.
