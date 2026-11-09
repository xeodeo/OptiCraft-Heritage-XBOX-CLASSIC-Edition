# ENCARGO 29 (agy): traer las 4 PR abiertas de OptiJuegos a Xbox sin romper nada

## Dónde
- Carpeta **`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-pr`**, rama `xbox-upstream-prs`, que sale de `xbox-unificada` (`65bd2a8`).
- **Solo trabajas ahí.** No toques la carpeta principal `OptiCraftHeritageEdition\`: hay otra sesión trabajando en ella.
- No hagas commit. No hagas push.

## Qué traer
Las PR ya están descargadas como ramas remotas en el repo (se ven desde tu worktree): `origin/pr-57`, `origin/pr-60`, `origin/pr-56`, `origin/pr-59`. Fusiónalas **en este orden**, con `git merge --no-commit --no-ff origin/pr-NN`, y resuelve cada una antes de pasar a la siguiente. Si al final quedan varias fusiones sin commit, dilo: Claude hará los commits.

1. **#57** (crashes del multijugador, protecciones contra nulos). Entra entera. **Además:** en `src/net/minecraft/src/NetworkManager.cpp`, el guard `#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)` (contrapresión cuando la cola de paquetes está llena) debe incluir también `defined(XBOX_PLATFORM)`. La Xbox tiene 64 MB y también se desconectaba con "Incoming packet queue overflow" en ráfagas de chunks.
2. **#60** (semilla en la lista de mundos). Entra entera.
3. **#56** (controles de PS2, GUIs de controles, crafteo legacy de ellos). **Conflictos esperados:**
   - `GameSettings`:
     - ellos añaden un campo `legacyCrafting`, y nosotros ya tenemos uno con ese nombre (opción *Console Crafting*, ON por defecto en Xbox);
     - debe quedar **uno solo**, y en Xbox se conservan nuestro valor por defecto y nuestra clave de guardado en `options.txt`;
     - si la clave de ellos es distinta, lee las dos al cargar.
   - `GameSettingsBackend.h` añade `platformGameSettingsApplyLegacyCrafting(GameSettings&)`: impleméntala en `src\platform\GameSettingsBackend_XBOX.cpp`. Si no tiene sentido en Xbox, vacía, como hace la Wii.
   - `LegacyControlPromptBackend.h` añade la acción `Crafting`: añádela en `src\platform\LegacyControlPromptBackend_XBOX.cpp` con el botón **X**, que es nuestro crafteo.
   - `GuiContainerCreative.cpp`:
     - en el bloque `#elif PLATFORM_XBOX`, las pestañas deben seguir usando `XboxPad::consumePagePressed()` (Blanco/Negro);
     - no vuelvas a `pad.pressed & PLATFORM_TEXT_PREV_PAGE`, porque el navegador de casillas se come esas pulsaciones.
   - `LegacyCraftingScreen.*` es el crafteo **de ellos**. En Xbox se usa `XboxCraftingScreen`, el nuestro. Acepta los cambios de ellos en `LegacyCraftingScreen`, pero **no cambies qué pantalla abre la Xbox** (`EntityPlayerSP::displayWorkbenchGUI`, tecla X en `Minecraft.cpp`, `XboxSplitScreen.cpp`).
   - `Minecraft.cpp`: conserva todo lo de Xbox, es decir los bloques `PLATFORM_XBOX`, `enterPlayer2Context`, `XBOX_WATCHDOG_PHASE`, el `delay(1)` con trabajos async y el respawn 3x3.
   - **Sonido:** no cambies nada del sonido de Xbox (`SoundManager_XBOX.cpp`). `SoundManager.h` puede recibir su `triggerMusicNow()`.
4. **#59** (rendimiento de PS2). Casi todo va con `#if PLATFORM_PS2`: acéptalo tal cual. Lo compartido que sí afecta a Xbox ya lo revisó Claude y **se acepta**:
   - `RenderBlocks.cpp`: el color del agua se calcula solo si hay cara que dibujar;
   - `ItemRenderer.cpp`: el objeto en la mano en 1 dibujado en vez de 6. En Xbox la luz por normales está apagada, así que se ve igual;
   - `RenderGlobal::markRenderersInRange`: los cambios que no vienen del jugador se agrupan en una sola reconstrucción;
   - `RenderEngine.cpp`: la textura del aldeano. Ya teníamos ese arreglo: deja una sola versión.

## Reglas
- Nada de lo nuestro de Xbox puede desaparecer:
  - pantalla dividida con contexto J2;
  - `XboxCraftingScreen`;
  - combo de waypoints Back + cruceta (`XboxInput.cpp`);
  - stick derecho en los logros;
  - `AnvilChunkLoader`: `writingPending` / `writingDone`;
  - `Chunk.cpp`: `#if !PLATFORM_XBOX` alrededor del borrado de secciones;
  - retención 3x3 del dragón en `World.cpp`;
  - todo lo async (`XBOX_ASYNC_CHUNK_IO`).
- Al terminar, **comprueba cada punto anterior con `grep`** e incluye el resultado en el informe.
- No borres código; si algo de ellos choca con Xbox, pon `#if !PLATFORM_XBOX`.

## Compilar (con candado)
1. Mira `buzon-xbox\BUILD.lock`. Si existe, espera. Si no, créalo con `agy PRs <hora>` y bórralo al terminar, aunque falle (`REGLA-BUILD-LOCK.md`).
2. En `wt-pr`, dos builds, con **las comillas** de los `-D` (PowerShell parte la IP si no):
   ```
   $env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
   $cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
   & $cm --preset xbox-release "-DMC_LOG_LEVEL=1" "-DXBOX_NETLOG_HOST=192.168.0.17" "-DXBOX_DATA_DIR=C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition/data"
   & $cm --build --preset xbox-release
   & $cm --preset xbox-release-exp "-DMC_LOG_LEVEL=1" "-DXBOX_NETLOG_HOST=192.168.0.17" "-DXBOX_DATA_DIR=C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition/data"
   & $cm --build --preset xbox-release-exp
   ```
3. **No uses** `xbox-spike\build_preset.ps1`: apunta a la carpeta principal. **No copies nada** al loader.
4. xemu: `xbox-spike\run-xemu.ps1 -Iso "<wt-pr>\bin\xbox-exp\OptiCraft.iso" -Seconds 45` (y lo mismo con `bin\xbox\OptiCraft.iso`). Guarda las capturas del menú y cierra xemu.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\29-INFORME-agy.md`. Primera línea literal: `# INFORME 29 agy`. Incluye:
- cada conflicto y cómo lo resolviste, con `archivo:línea`;
- el `grep` de la lista de reglas;
- `git diff --stat xbox-unificada` de tu carpeta;
- el resultado de las dos compilaciones y las capturas.

**No digas que algo está hecho si no lo está**: Claude revisa el diff línea a línea.

## REVISIÓN DE CLAUDE (29-09 ~15:55)
- Resultado: commit `b26a41f` en `xbox-upstream-prs` (worktree `wt-pr`). Es una fusión con 4 padres más, que registra las PR 57/60/56/59.
- Aprobado lo de agy. Claude corrigió tres cosas:
  1. `LegacyHeritageOptions`: en Xbox la fila de Console Crafting se contaba dos veces (`rowCount 7` más `++` Xbox). Se quitó el `++`.
  2. `GuiContainerCreative`: Blanco/Negro respetan el ajuste nuevo `legacyCreative`.
  3. `Minecraft.cpp`: `keyBindCrafting` en Xbox no usa la ruta PS2 `displayPlayerScreen` en pantalla dividida.
- Observado: la #56 añade música aleatoria en el menú y `triggerMusicNow` al entrar al mundo (lógica compartida, no el backend de Xbox).
- Observado: en Xbox `keyBindCrafting` queda en `KEY_C` (no se usa con el mando; en Controles sale como "C").
- Falta fusionarlo en `xbox-unificada`: la otra sesión tiene cambios sin commit en la carpeta principal.
