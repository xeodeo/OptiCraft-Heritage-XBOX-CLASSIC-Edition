# ENCARGO para agy: en Xbox, volver a NUESTRA pantalla partida y a NUESTRO menú de crafteo

Si algo no está en tu lista, para y pregunta. Si crees que me equivoco, dilo.

## Lo que dice el usuario (probado en la consola real)
- La pantalla partida de OptiJuegos adaptada a Xbox (tu Parte B) funciona mal:
  - el J2 entra **sin mano**;
  - el J2 **no puede mover la cámara**;
  - el J2 **estropea al J1**;
  - **no se puede pausar** con el J2 dentro.
- La NUESTRA ("la mía ya funcionaba mejor") no tenía esos fallos.
- Tampoco le gusta el menú de crafteo de OptiJuegos: quiere el suyo.

## Qué queremos
En la carpeta `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraft-prueba-opti` (rama `prueba-opti`, merge sin commit):
- **En Xbox:** nuestra pantalla partida y nuestro menú de crafteo.
- **Todo lo demás de OptiJuegos se queda:**
  - inventario creativo;
  - mundo isla;
  - dragón (C1);
  - mods (C6);
  - arreglos varios.
- **En PS2 y Wii:** el código de ellos intacto. Separa con `#if PLATFORM_XBOX ... #else ... #endif`.

## Dónde está lo NUESTRO
Commit `0a9dadf` (rama `xbox-port`):
```
git -C "<carpeta>" show 0a9dadf:<ruta>
git -C "<carpeta>" diff 965efdb 0a9dadf -- src/client src/net src/platform src/xbox     # todo lo que hicimos para la pantalla partida
```
Notas del diseño en `buzon-xbox\08-pantalla-dividida.md` (léelo: explica el "contexto de jugador").

Ficheros de nuestra versión:
- `src/client/XboxSplitScreen.cpp/.h`: en la carpeta de prueba se borró el `.cpp` y el `.h` quedó como stubs `#if 0`. **Recupera los dos** de `0a9dadf`.
- `src/client/Minecraft.cpp/.h`:
  - `enterPlayer2Context`, `leavePlayer2Context`, `inPlayer2Context`, `player1Screen`, `player2Screen`;
  - `discardPlayer2Screen`, `guiScreenResolution`, `refreshScreenResolutions`, `tickClickCounters`, `useItemReady`;
  - `player2Mining`, `otherPlayerScreen`, `otherOwnedGuiScreens`, `screenPlayer`, `contextSavedPlayer`, `otherLeftClickCounter`, `otherRightClickDelayTimer`;
  - su uso en `displayGuiScreen`, `runTick` (`XboxSplitScreen::tick/postTick`), el cambio de mundo y `clickMouse` (`player2Mining`).
  - **OJO:** en la carpeta de prueba hay STUBS provisionales en `Minecraft.h`: `inPlayer2Context()` devuelve false, `discardPlayer2Screen() {}`, `player2Mining` y `guiScreenResolution`. Sustitúyelos por lo real.
- `src/net/minecraft/src/EntityRenderer.cpp`: `renderSplitScreen` (dibuja la pantalla de cada jugador en su mitad, con `enterPlayer2Context` para la del J2) y `XboxSplitScreen::turnCamera(mc)`, que se llama antes del bloque de cámara del J1.
- `GuiScreen.cpp` (`handleInput`: en contexto J2 no vaciar las colas de ratón y teclado), `ContainerSlotNavigator.cpp` (un navegador por jugador), `EntityPlayerSP.cpp` (`closeScreen` del J2), `MovementInputFromOptions.cpp` (rama Xbox del J2), `GuiIngameMenu.cpp` (pausa en pantalla partida, `XboxSplitScreen::requestLeave`) y `ItemRenderer.cpp` (mano).
- `src/xbox/input/XboxPad.*` y `src/platform/InputBackend_XBOX.cpp`: comprueba que siguen como en `0a9dadf`. Pulsaciones por jugador y `platformMenuPad()` = `menuPlayer()`.
- `LegacyHeritageOptions.cpp/.h`: nombre del J2 ("Player 2 Name", `playerName2`), si falta.

## Deshacer tu Parte B SOLO en Xbox
En Xbox no debe correr `Ps2SplitScreen`: `Ps2SplitScreen.cpp/.h` vuelven a `#ifdef PS2_PLATFORM`, y en `Minecraft.cpp`/`EntityRenderer.cpp` Xbox vuelve a llamar a `XboxSplitScreen::*`.

Donde el código común de OptiJuegos use su diseño (`displayPlayerScreen(i, ...)`, `getPlayerScreen(i)`, `isPlayerScreenActive(i)`, `closePlayerScreen(i)`, `playerScreens[]`), haz que **en Xbox** esas cuatro funciones usen nuestro contexto, dentro de `Minecraft.cpp`:
- `displayPlayerScreen(1, s)`: si no estás en contexto J2, `enterPlayer2Context(); displayGuiScreen(s); leavePlayer2Context();`. Y al revés para el 0. Si ya estás en el contexto bueno, `displayGuiScreen(s)`.
- `getPlayerScreen(i)` = `i==1 ? player2Screen() : player1Screen()`.
- `isPlayerScreenActive(i)` = `getPlayerScreen(i) != nullptr`.

En Xbox, `playerScreens[]` se queda siempre vacío.

## Menú de crafteo nuestro
OpenCode está creando **a la vez** `src/net/minecraft/src/legacy/XboxCraftingScreen.cpp/.h` (nuestro crafteo renombrado; encargo `buzon-xbox\13-...`). **No toques esos dos ficheros.** Espera a que exista `buzon-xbox\13-INFORME-opencode-crafteo.md` con `ESTADO: TERMINADO` antes de compilar con ellos.

Después, en Xbox, abre `XboxCraftingScreen` en lugar del de OptiJuegos:
- tecla **X** en `Minecraft.cpp` (`XBOX_KEY_X`): `new XboxCraftingScreen(thePlayer)`;
- mesa de trabajo en `EntityPlayerSP.cpp` (`displayWorkbenchGUI`): `new XboxCraftingScreen(this, worldObj, i, j, k)` si `legacyCrafting`;
- X del J2 en `XboxSplitScreen.cpp`;
- inventario (Y): en Xbox, como en `0a9dadf`, el inventario normal (`GuiInventory` o el creativo), NO el `LegacyCraftingScreen` de OptiJuegos. Mira `keyBindInventory` en `Minecraft.cpp` y `GuiInventory.cpp`.

## Aceptación
- Compila: `powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\build_prueba.ps1"` → `sucessfully created OptiCraft.iso`.
- `git -C "<carpeta>" diff 0a9dadf -- src/client/XboxSplitScreen.cpp src/client/XboxSplitScreen.h` sale vacío, o solo con cambios explicados en el informe.
- **No romper** lo que ya se arregló:
  - `GuiScreen.cpp` tiene `|| PLATFORM_XBOX` en los bloques del teclado en pantalla (líneas ~46, 67, 289, 303, 679);
  - `Minecraft.cpp` llama a `VirtualKeyboard::instance().releaseFocus()` en Xbox en `displayGuiScreen`;
  - el C1 del dragón.

## Tuyos / no tuyos
- **TUYOS:** `...\OptiCraft-prueba-opti\src\` salvo los dos `XboxCraftingScreen.*`.
- **NO TUYOS:**
  - `XboxCraftingScreen.*` (de OpenCode);
  - la carpeta `OptiCraftHeritageEdition`;
  - `cmake/`, `scripts/`, `CMakePresets.json`;
  - git que cambie estado (ni commit, reset, checkout de ramas ni stash). `git show`/`diff` sí.

## Trampas
- CRLF en muchos ficheros: consérvalo.
- `src/xbox/` compila con la STL del XDK: no metas objetos `std::` que crucen esa frontera.
- Sin SSE2: nada de `std::pow` nuevo en Xbox.
- No lances dos compilaciones a la vez.

## Informe
Escríbelo en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\12-INFORME-agy-pantalla-partida-nuestra.md`.
- **Primera línea literal:** `ESTADO: TERMINADO` o `ESTADO: BLOQUEADO`.
- **Después:**
  - los ficheros tocados, con una frase cada uno;
  - la última línea de la compilación;
  - lo que no pudiste hacer.

---

## PARTE 2 (después de lo anterior y compilado): dos tipos de mundo limitado
El usuario: "mundo isla no me gusta, mantengamos los dos: el reducido anterior y la isla".

- Hoy "World Size" tiene dos valores: Infinite y Limited. Desde el commit de OptiJuegos `1f6d04d`
  ("adapt Old 256x256 world generation to MCPE 0.6.0 island..."), **Limited genera una isla**.
  Mira qué cambió: `git -C "<carpeta>" show 1f6d04d`. Toca, entre otros, `ChunkProviderGenerate.cpp`,
  `ChunkProviderGenerateLite.cpp`, `ChunkProviderGeneratePopulateIncremental.cpp`, `WorldChunkManager*.cpp`,
  `BiomeDecorator.cpp`, `BiomeGenBase.cpp`, `MapGenStronghold.cpp`, `StructureStrongholdStart.cpp`, `World.cpp` y
  `WorldGenFlowers/TallGrass.cpp`.
- **Queremos tres valores en "World Size":**
  - `Infinite`;
  - `Limited`: 256x256 con el **terreno de antes** de `1f6d04d`, es decir, el borde a ±128 y el
    generador normal;
  - `Island`: el de OptiJuegos, tal como está ahora.
- Guárdalo en el mundo **sin romper los mundos existentes**: `WorldInfo` ya guarda `limitedWorld`
  (bool). Añade otro campo NBT, por ejemplo `limitedWorldType` o `islandWorld`. Si falta, un mundo
  Limited ya creado se considera **Island**, porque se creó con el generador de isla.
- Todas las comprobaciones de la isla en la generación (`isLimitedWorld()` dentro de los ficheros
  de `1f6d04d`) pasan a comprobar "es isla". Las del borde, la retención y la pantalla partida
  (`isLimitedWorld()` en `Entity.cpp`, `ChunkProvider*.cpp`, `XboxSplitScreen`, etc.) siguen valiendo
  para los dos.
- Botón en la pantalla Legacy de crear mundo (`legacy/LegacyCreateWorldScreen.cpp`) y en
  `GuiCreateWorld.cpp`: rotar Infinite → Limited → Island. Textos en español e inglés, como
  los que ya hay ("isEs").
- Esto NO es solo de Xbox: vale para todas las plataformas. No hace falta `#if`.

**Aceptación Parte 2:** compila, y en el informe explica en qué ficheros va "es isla" y en cuáles
"es limitado".
