# Encargo 65 (agy): revisar TODOS los idiomas de la 1.5.2, con los mismos archivos que Minecraft

## Tu sitio
- Worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-idiomas`, rama `port152-idiomas`
  (sale de `port152-claude` 706705a). Solo aquí.
- NO toques `wt-152-agy` (otro agy hace el menú creativo), ni `wt-152-codex`, ni `wt-152-pociones`.
- Guía de trabajo y reglas: `buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`.

## Material
- Idiomas de Java 1.5.2: `mcp-diff\src152\lang\*.lang` (66) y `mcp-diff\src152\lang\languages.txt` si existe.
- Idiomas de Java 1.2.5: `mcp-diff\src125\lang\`.
- Los que lleva OptiCraft: `data\assets\lang\` del repo (59). Se mezclan al empaquetar en
  `scripts\port152\build_textures_152.py` → `merge_languages()`. Resultado: `build\xbox-release\port152-data\assets\lang\`.
- Código: `src\net\minecraft\src\StringTranslate.cpp`, `StatCollector`, y todo lo que llame `translateKey`,
  `translateNamedKey`, `translateToLocal`, `translateToLocalFormatted`, `getItemDisplayName`, `getTranslatedName`...

## Qué hacer
1. Que el juego use los textos igual que Minecraft 1.5.2: misma clave, mismo archivo y el mismo respaldo a `en_US`
   cuando falta una clave (`StringTranslate.java`). Compara `StringTranslate.cpp` con el Java.
2. Lista de TODAS las claves que usa el C++ (grep de las llamadas de arriba, de `setItemName`/`setBlockName`
   → `item.X.name`/`tile.X.name`, de `potion.*`, `enchantment.*`, `itemGroup.*`, `container.*`, `death.*`,
   `entity.*`, `stat.*`, `achievement.*`, `gui.*`, `options.*`...). Comprueba que cada una existe en `en_US` de 1.5.2
   y en cada idioma empaquetado. Las que falten en un idioma pero estén en el Java de ese idioma: que entren.
3. Textos escritos a mano en inglés en el C++ donde el Java usa una clave (p. ej. nombres de pestañas, «Enchanted Book»,
   nombres de pociones, efectos del faro, yunque «Repair & Name», «Too Expensive!», libros): pásalos a la clave.
   OJO: el usuario ve «libros encantados» mal escrito/sin nivel ni encantamiento; revisa `ItemEnchantedBook`
   (`getItemDisplayName`/información de encantamiento) contra el Java.
4. `merge_languages`: que no pierda claves ni deje textos de 1.2.5 viejos donde 1.5.2 los cambió, y que respete los textos
   propios de OptiCraft (menús Legacy de consola). Si hay idiomas del Java que OptiCraft no lleva, dilo (no los añadas sin
   preguntar: el menú de idiomas de consola tiene su lista).
5. Caracteres: que las fuentes muestren tildes/ñ/ç y lo que no se pueda (cirílico, CJK) dilo con la lista de idiomas
   afectados (`font.txt`/`glyph_sizes.bin`); no cambies fuentes.
6. Reconstruye el pak (el script) y compila `xbox-release` y `xbox-release-exp` con el candado `buzon-xbox\BUILD.lock`.

## Reglas
Nunca /fp:fast, no push, no ClassicXboxLoader, no xemu. Nada de relleno. Commits pequeños por tema.

## Al acabar
Informe `buzon-xbox\65-INFORME-agy-idiomas-152.md`: tabla corta por idioma (claves que faltaban → arregladas), lista de
textos a mano pasados a clave, y lo que no se puede (fuentes). Commit y en tu terminal: `ENCARGO 65 LISTO <commit>`.

## Compilar (Git Bash; en PowerShell el -D con 1.5.2 se rompe)
```
export XBOX_XDK_ROOT="C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/xdk/5849/sdk/XDK/xbox"
C=$(ls "/c/Program Files/Microsoft Visual Studio/2022/"*/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe | head -1)
"$C" --preset xbox-release "-DXBOX_VERSION_LABEL=1.5.2" "-DXBOX_DATA_DIR=<este wt>/build/xbox-release/port152-data"
"$C" --build build/xbox-release --target OptiCraft --parallel 4      (igual con xbox-release-exp)
```
Sin `XBOX_DATA_DIR` el juego sale sin datos. El `port152-data` lo genera el script de texturas (paso 6); para empezar
puedes copiar `wt-152-codex\build\xbox-release\port152-data` a tu build.

## Nota de Claude (03-11 tarde)
`port152-claude` avanzó (9edcfe5, da5b0a4: creativo nuevo, `StringTranslate::translateKeyFormat` ya acepta %d/%N$d,
tooltip con display.Name). Antes de tocar `StringTranslate.cpp`/`ItemStack.cpp`: `git cherry-pick 9edcfe5 cc5f988 da5b0a4`
(si hay conflicto en ItemAppleGold, quédate con la versión de port152-claude).
