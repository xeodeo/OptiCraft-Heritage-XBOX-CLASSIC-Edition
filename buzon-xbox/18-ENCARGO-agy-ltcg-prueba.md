# ENCARGO 18 (agy): XBE con LTCG para probar en la consola real

Sigue del encargo 17 (tu informe `17-INFORME-agy.md`). El usuario quiere probar la configuración C (enlazador moderno + LTCG) en su Xbox, como XBE aparte.

## Qué hacer, en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-ltcg`
1. **Arregla el `LNK1257` de C.** `src\xbox\runtime\XboxP3Math.c` define sustitutos de `floor`, `ceil` y otros, que no pueden compilarse con `/GL` (`C2268`). Sácalo del `/GL`. Idea: compílalo en una `add_library(... OBJECT ...)` propia sin `/GL` (con su `/arch:IA32` de siempre) y enlázala al target. Revisa que ningún otro archivo que defina auxiliares del CRT reciba `/GL`.
2. **Compila C** (`XBOX_MODERN_LINKER=ON`, `XBOX_LTCG=ON`) **con candado** (`buzon-xbox\REGLA-BUILD-LOCK.md`). Usa los mismos comandos del encargo 17, con `--fresh`.
3. **Comprueba:**
   - enlaza;
   - `patch_pe_for_imagebld` e `imagebld` pasan;
   - tamaño del XBE;
   - avisos LITERALES de `patch_sse2_moves.ps1`;
   - xemu llega al menú (con candado). Copia la captura a `buzon-xbox\18-shot-C.png`.
4. **Investiga, sin tocar código del juego,** si el SSE2 nuevo de B/C (`XmmCopy*`, `Palign*`, `LargeRangeBytes`: el `memcpy`/`memmove` de vcruntime) solo se ejecuta si la CPU tiene SSE2. Busca en el `.map` (`wt-ltcg\bin\xbox\OptiCraft.exe.map`) y en el desensamblado si esas rutas dependen de `__isa_available` / `__favor`, y dónde se inicializa eso (`__isa_available_init`). Escribe qué viste; si no lo puedes ver, pon "no verificado".

## Tuyos / no tuyos
- **Tuyos:** `wt-ltcg\cmake\*`.
- **No tuyos:** `src\`, el repo principal, `wt-async\`, ClassicXboxLoader. Claude copia el XBE a la consola.
- No hagas commit.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\18-INFORME-agy.md`. Primera línea literal: `# INFORME 18 agy`. Incluye:
- el resultado de C;
- la ruta exacta del `default.xbe` generado;
- los avisos SSE2 literales;
- lo que encontraste de `__isa_available`;
- el diff de `cmake`.
