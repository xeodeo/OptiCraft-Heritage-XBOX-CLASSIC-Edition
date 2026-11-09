# ENCARGO 16 (OpenCode): "unity build" en el build de Xbox

## Contexto (medido)
- El LTCG del encargo 14 no es viable: el enlazador es el `Link.Exe` VC 7.1 del XDK (`cmake\xbox_toolchain.cmake:65`). Buen trabajo detectándolo.
- Alternativa con el mismo enlazador: **unity build**. CMake junta varios `.cpp` en uno, así el compilador puede hacer inlining entre archivos.
- **TU carpeta (worktree `wt-ltcg`, rama `xbox-ltcg`):** `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg`
- **agy trabaja AL MISMO TIEMPO** en `wt-async` sobre estos archivos: `ChunkProvider*`, `AnvilChunkLoader*`, `RegionFile*`, `ThreadedFileIOBase*`, `SpawnerAnimals*`, `ChunkGenerationScheduler*`, `src\platform\PlatformConfig.h`, `src\xbox\XboxTuning.h`. **No los toques** en `wt-ltcg`, o luego no se podrán juntar los dos trabajos.

## Qué hacer
1. En `wt-ltcg\cmake\xbox.cmake`:
   - deja `XBOX_LTCG` en **OFF por defecto** (no lo borres);
   - añade la opción `XBOX_UNITY_BUILD` (ON por defecto) que active `set_target_properties(OptiCraft PROPERTIES UNITY_BUILD ON UNITY_BUILD_BATCH_SIZE 16)` para el target `OptiCraft`.
2. **Excluye** de la unidad, con `set_source_files_properties(... PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)`:
   - `src\xbox\runtime\XboxP3Math.c` (compilado con `/arch:IA32`);
   - todo `src\xbox\**` y `src\java\JavaNetworkXbox.cpp` (usan las cabeceras del XDK y su STL vieja);
   - todo archivo con opciones de compilación propias.
3. Compila (PowerShell):
   ```powershell
   $env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
   $cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
   cd "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg"
   & $cm --preset xbox-release --fresh "-DXBOX_DATA_DIR=C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition/data"
   & $cm --build --preset xbox-release
   ```
4. **Si hay choques de nombres** (funciones `static`, `namespace {}` o `#define` repetidos entre archivos):
   - la PRIMERA opción es excluir ese `.cpp` con `SKIP_UNITY_BUILD_INCLUSION`;
   - solo si es un archivo que NO está en la lista de agy, puedes renombrar el símbolo local (cambio mínimo).
   - Anota cada exclusión o cambio en el informe.
5. Prueba en xemu que llega al menú. Antes mira que no haya un xemu abierto (`Get-Process xemu`): agy lo comparte.
   ```powershell
   powershell -ExecutionPolicy Bypass -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\run-xemu.ps1" -Iso "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg\bin\xbox\OptiCraft.iso" -Seconds 30
   ```

## Tuyos / no tuyos
- **Tuyos:**
  - `wt-ltcg\cmake\xbox.cmake`;
  - `wt-ltcg\CMakePresets.json`;
  - renombres mínimos en `wt-ltcg\src\` fuera de la lista de agy.
- **No tuyos:**
  - los archivos de agy (lista de arriba);
  - la carpeta `wt-async\`;
  - el repo principal `OptiCraftHeritageEdition\`;
  - `xbox-spike\build_preset.ps1`;
  - ClassicXboxLoader.
- No hagas commit ni push. Si algo no está en tu lista, para y pregunta. Si crees que el plan está mal, dilo.

## Aceptación
- Compila y enlaza con `XBOX_UNITY_BUILD` ON.
- `patch_sse2_moves.ps1` solo avisa de `frexp`, `powhlp`, `handle_exc` y `num_put`, nada nuevo.
- xemu llega al menú.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\16-INFORME-opencode.md`. Primera línea literal: `# INFORME 16 opencode`. Incluye:
- el diff (`git -C wt-ltcg diff`);
- la lista de archivos excluidos y el motivo;
- el tamaño de `default.xbe` (antes: 4.182.016 bytes sin unity);
- el tiempo de compilación aproximado;
- los avisos SSE2;
- el resultado de xemu y cualquier error exacto.
