# ENCARGO para OpenCode: recuperar el menú de crafteo NUESTRO como `XboxCraftingScreen`

Si algo no está en tu lista, para y pregunta. Si crees que me equivoco, dilo.

## Contexto
- El usuario prefiere su menú de crafteo de consola (el que hicimos para Xbox) al de OptiJuegos: "ese menú de crafteo está horrible, volvamos al mío".
- Carpeta de trabajo: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraft-prueba-opti` (rama `prueba-opti`, merge sin commit). Usa **rutas absolutas**.
- Allí, `src/net/minecraft/src/legacy/LegacyCraftingScreen.cpp/.h` es AHORA el de OptiJuegos. **No lo borres ni lo toques**: se queda para PS2 y Wii.
- El NUESTRO está guardado en git, commit `0a9dadf` (rama `xbox-port`), con el mismo nombre de fichero:
  ```
  git -C "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraft-prueba-opti" show 0a9dadf:src/net/minecraft/src/legacy/LegacyCraftingScreen.cpp
  git -C "..." show 0a9dadf:src/net/minecraft/src/legacy/LegacyCraftingScreen.h
  ```
  Usa también `LegacyCraftingGroups.h` y `LegacyCraftingIconsPng.h`, que ya están en la carpeta sin cambios.

## Tarea (solo ficheros NUEVOS; otro agente, agy, conecta las llamadas y compila)
1. Crea `src/net/minecraft/src/legacy/XboxCraftingScreen.h` y `.cpp` con el contenido de esos dos ficheros de `0a9dadf`, renombrando la clase `LegacyCraftingScreen` → `XboxCraftingScreen` en todas partes: include guard, constructores, destructor, métodos y el `#include` de su propio `.h`.
2. Guárdalos en **UTF-8 con finales de línea CRLF**, como los originales.
3. Comprueba que nada de esos ficheros choca con el `LegacyCraftingScreen` de OptiJuegos:
   - ningún símbolo global o `static` con el mismo nombre en los dos `.cpp`;
   - que ninguna constante del namespace anónimo se exporte.

   Si hay algo con el mismo nombre fuera de un namespace anónimo, renómbralo con prefijo `xbox`.
4. **NO compiles** (lo hará agy; dos compilaciones a la vez se pisan). **NO toques ningún otro fichero.**

## Tuyos / no tuyos
- **TUYOS:** solo `src/net/minecraft/src/legacy/XboxCraftingScreen.cpp` y `XboxCraftingScreen.h` (nuevos).
- **NO TUYOS:** todo lo demás, incluida la carpeta `OptiCraftHeritageEdition`. Nada de git que cambie estado (ni add, commit, checkout, stash ni reset).

## Informe
Escríbelo en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\13-INFORME-opencode-crafteo.md`.
- **Primera línea literal:** `ESTADO: TERMINADO` o `ESTADO: BLOQUEADO`.
- **Después:** la lista de símbolos renombrados y cualquier conflicto encontrado.
