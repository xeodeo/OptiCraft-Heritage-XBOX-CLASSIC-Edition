# ENCARGO 12 (OpenCode): generar el assets.pak de Xbox para probar

## Contexto (medido)
- El juego de Xbox ya sabe leer un `assets.pak`: `src\platform\Resources_XBOX.cpp:29` monta `D:\assets.pak`. `D:` es la carpeta donde está `default.xbe`. Si el pak existe, se usa antes que los archivos sueltos de `data\`.
- Las claves dentro del pak van relativas a `data\`: `assets/terrain.png`, `assets/gui/buttons_xbox.png`, `resources/newmusic/hal4.ogg`…
- El formato es MCPK. Se genera con `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\scripts\make_pak.py` (función `build(carpeta, salida)`; lee el script para ver cómo se llama).
- Datos de Xbox: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data\` (contiene `assets\` y `resources\`).

## Tuyos / no tuyos
- **Tuyos:** crear `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\pak-prueba\assets.pak` y copiarlo a `C:\Users\xeodeo\Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\OptiCraft\assets.pak`, junto a `default.xbe`.
- **NO tuyos:** no toques nada dentro de `OptiCraftHeritageEdition\` (ni `src`, ni `data`, ni `cmake`, ni `bin`). No uses el `assets.pak` de Descargas: es el de PS2.

## Pasos
1. Genera el pak a partir de `OptiCraftHeritageEdition\data\`, usando el script en modo lectura, en `...\pak-prueba\assets.pak`.
2. Comprueba con un script Python que lea la cabecera MCPK (formato en `scripts\update_button_assets.py`, función `unpack_pak`):
   - cuántas entradas tiene y cuánto ocupa;
   - que existen `assets/terrain.png`, `assets/gui/buttons_xbox.png`, `assets/legacy/crafting_icons.png`, `assets/skins/LegacySteve.png` y al menos un `resources/newmusic/*.ogg`.
3. Cópialo a la carpeta de ClassicXboxLoader indicada arriba.

## Aceptación
- El pak existe, con **más de 700 entradas**, y contiene las 5 claves del paso 2.
- Está copiado junto a `default.xbe` en la carpeta de ClassicXboxLoader.

## Informe
- En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\12-INFORME-opencode.md`. Primera línea literal: `# INFORME 12 opencode`.
- Incluye: comando usado, número de entradas, tamaño, las 5 claves encontradas (sí/no) y la ruta de la copia.
- Si algo falla, escribe el error exacto; no inventes. Si crees que el plan está mal, dilo.
