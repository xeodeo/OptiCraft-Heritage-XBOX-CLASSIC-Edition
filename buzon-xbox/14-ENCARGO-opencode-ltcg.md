# ENCARGO 14 (OpenCode): activar LTCG en el build de Xbox

## Contexto (medido)
- Juego: OptiCraft (Minecraft 1.2.5 en C++) para la **Xbox original** (Pentium III con SSE1, **sin SSE2**). Compila con el MSVC de VS2022 más las librerías del XDK 5849.
- **TU carpeta de trabajo (worktree git, rama `xbox-ltcg`):** `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg`
- Flags actuales en `wt-ltcg\cmake\xbox.cmake`:
  - `:137` `/arch:SSE`, `:138` `/O2` (opciones de C/C++ del target);
  - `:83` `XboxP3Math.c` se compila aparte con `/arch:IA32`;
  - `:248` la línea de enlace es `"${CMAKE_LINKER}" /nologo /MACHINE:I386 ... /OPT:REF /MAP:... /OUT:<TARGET> <OBJECTS> ...`.
- Después de enlazar, `scripts\xbox\patch_sse2_moves.ps1` reescribe el SSE2 que queda. Hoy avisa SOLO de: `frexp`, `powhlp`, `handle_exc`, `num_put`.

## Qué hacer
1. Añadir en `cmake\xbox.cmake` una opción `XBOX_LTCG` (ON por defecto):
   - si está ON: `/GL` en las opciones de C/C++ (junto a `/O2`), pero **no** en `XboxP3Math.c`;
   - y `/LTCG` en la línea de enlace (`:248`).
2. Compilar en TU carpeta, en PowerShell:
   ```powershell
   $env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
   $cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
   cd "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg"
   & $cm --preset xbox-release "-DXBOX_DATA_DIR=C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition/data"
   & $cm --build --preset xbox-release
   ```
   Sale `wt-ltcg\bin\xbox\OptiCraft.iso` y `wt-ltcg\bin\xbox\iso\default.xbe`.
3. Anotar el tamaño de `default.xbe` con y sin LTCG: compila una vez con `-DXBOX_LTCG=OFF` y otra con ON.
4. Probar que arranca en xemu. Cierra xemu antes y después:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\run-xemu.ps1" -Iso "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg\bin\xbox\OptiCraft.iso" -Seconds 30
   ```
   Genera `xbox-spike\shot.png`. Descríbelo o copia esa imagen a `buzon-xbox\14-shot.png`.

## Tuyos / no tuyos
- **Tuyos:** SOLO `wt-ltcg\cmake\xbox.cmake` (y `wt-ltcg\CMakePresets.json` si hace falta).
- **NO tuyos:**
  - todo `src\`;
  - la carpeta `OptiCraftHeritageEdition\` (el repo principal);
  - `wt-async\` (es de agy);
  - `xbox-spike\build_preset.ps1` (no lo uses: compila el repo principal);
  - ClassicXboxLoader.
- No hagas commit ni push. Si algo no está en tu lista, para y pregunta. Si crees que el plan está mal, dilo.

## Aceptación
- Enlaza sin errores con LTCG ON.
- `patch_sse2_moves.ps1` no avisa de NINGÚN SSE2 nuevo, aparte de los 4 de arriba. Copia sus líneas de aviso literales.
- xemu llega al menú principal.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\14-INFORME-opencode.md`. Primera línea literal: `# INFORME 14 opencode`. Incluye:
- el diff de `cmake\xbox.cmake` (`git -C wt-ltcg diff`);
- los tamaños del XBE;
- los avisos SSE2;
- el resultado de xemu;
- cualquier error exacto.
