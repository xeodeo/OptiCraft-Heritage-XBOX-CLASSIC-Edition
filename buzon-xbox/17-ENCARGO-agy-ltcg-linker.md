# ENCARGO 17 (agy): LTCG enlazando con el link.exe de VS2022

## Contexto (medido)
- Queremos LTCG (`/GL` + `/LTCG`), un 5-15 % más de CPU. Con el `Link.Exe` VC 7.1 del XDK falla: `LNK1262` (versiones distintas de `c2.dll` y del enlazador). Ver `14-INFORME-opencode.md`.
- El unity build no es viable (`16-INFORME-opencode.md`).
- **Pista de OpenCode:** en `wt-ltcg\cmake\xbox_toolchain.cmake:65` comentó `set(CMAKE_LINKER ".../vc71/Link.Exe")`, así que CMake usa el `link.exe` moderno de VS2022. Según él, ese XBE (15:16) llega al menú en xemu. **No está verificado**, y `XBOX_LTCG` quedó en OFF.
- **TU carpeta:** worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg` (rama `xbox-ltcg`). Tiene los cambios de OpenCode en `cmake\xbox.cmake` y `cmake\xbox_toolchain.cmake`.
- Tras enlazar, el build ejecuta `scripts\xbox\patch_pe_for_imagebld.ps1`, luego `imagebld`, y después `scripts\xbox\patch_sse2_moves.ps1`. Hoy este último avisa SOLO de `frexp`, `powhlp`, `handle_exc` y `num_put`.

## Qué hacer
1. **Limpieza:** quita el unity build de `cmake\xbox.cmake` (la opción `XBOX_UNITY_BUILD` y su bloque). No sirve.
2. **Interruptor `XBOX_MODERN_LINKER`** (ON), en `cmake\xbox_toolchain.cmake`: ON usa el `link.exe` de VS2022 (`${XBOX_MSVC_ROOT}/bin/Hostx64/x86/link.exe`); OFF usa el `Link.Exe` del XDK de siempre.
   - `XBOX_LTCG` (ON) solo se permite si `XBOX_MODERN_LINKER` también está en ON.
3. **Compila con candado** (`buzon-xbox\REGLA-BUILD-LOCK.md`), en PowerShell:
   ```powershell
   $env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
   $cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
   cd "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg"
   & $cm --preset xbox-release --fresh "-DXBOX_DATA_DIR=C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition/data"
   & $cm --build --preset xbox-release
   ```
   Hazlo en tres configuraciones y anota cada una:
   - A) enlazador del XDK, sin LTCG (la referencia);
   - B) enlazador moderno, sin LTCG;
   - C) enlazador moderno con LTCG.
4. **Para cada configuración, anota:**
   - ¿enlaza?
   - ¿`patch_pe_for_imagebld.ps1` e `imagebld` pasan?
   - tamaño de `default.xbe`;
   - líneas LITERALES de avisos de `patch_sse2_moves.ps1`, que no aparezca ningún SSE2 nuevo;
   - xemu llega al menú (`xbox-spike\run-xemu.ps1 -Iso ...\wt-ltcg\bin\xbox\OptiCraft.iso -Seconds 30`, **con candado**). Copia la captura a `buzon-xbox\17-shot-<A|B|C>.png`.
5. **Ojo SSE2:** el CRT moderno trae SSE2 y la consola real NO tiene SSE2; xemu no lo detecta. Con LTCG, el código se reordena. Si `patch_sse2_moves.ps1` avisa de algo nuevo, C **no vale**, aunque xemu funcione.

## Tuyos / no tuyos
- **Tuyos:** `wt-ltcg\cmake\xbox.cmake` y `wt-ltcg\cmake\xbox_toolchain.cmake`.
- **No tuyos:**
  - todo `src\`;
  - el repo principal `OptiCraftHeritageEdition\`;
  - `wt-async\`;
  - `xbox-spike\build_preset.ps1`;
  - ClassicXboxLoader.
- No hagas commit ni push. Si algo no está en tu lista, para y pregunta. Si crees que el plan está mal, dilo.
- Escribe solo lo que hayas VISTO. Si no lo viste, pon "no verificado".

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\17-INFORME-agy.md`. Primera línea literal: `# INFORME 17 agy`. Incluye:
- una tabla con las configuraciones A, B y C;
- el diff (`git -C wt-ltcg diff`);
- tu recomendación: cuál probar en la consola real.
