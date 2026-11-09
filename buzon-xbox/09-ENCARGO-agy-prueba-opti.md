# ENCARGO para agy: versión de prueba "prueba-opti" (código de OptiJuegos en Xbox)

Léelo entero antes de empezar. Si algo no está en tu lista de "tuyos", para y pregunta. Si crees que me equivoco, dilo.

## Contexto (ya medido)
- El usuario quiere probar en la Xbox **los cambios nuevos de OptiJuegos tal cual**: su menú de crafteo, su pantalla partida, su inventario creativo, el mundo isla y sus arreglos.
- Nuestro trabajo NO se pierde: está guardado en la rama `xbox-port` (commit `0a9dadf`), en otra carpeta que **no debes tocar**.
- Carpeta de trabajo (un *git worktree*):
  `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraft-prueba-opti`
  - Rama `prueba-opti`, con un **merge en curso** (sin commit) de `origin/main` hecho con `-X theirs`: en cada conflicto ganó la versión de ellos.
  - Ya hecho por mí:
    - `src/client/XboxSplitScreen.cpp` borrado;
    - `XboxSplitScreen.h` convertido en stubs (`#if 0`);
    - stubs en `src/client/Minecraft.h`: `inPlayer2Context()` devuelve false, `guiScreenResolution`, `discardPlayer2Screen`, `player2Mining`;
    - el X de crafteo usa el constructor de ellos.
- Compilar (tarda ~8 min la primera vez):
  ```
  powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\build_prueba.ps1"
  ```
  Solo imprime errores y, si va bien, la línea `sucessfully created OptiCraft.iso`.

## Errores actuales (medidos en la última compilación)
Todos en `src/client/Minecraft.cpp`. Son restos de NUESTRA pantalla partida de Xbox (la del "contexto de jugador"), que en esta rama ya no existe:
- líneas ~3160-3260: `Minecraft::enterPlayer2Context`, `leavePlayer2Context`, `tickClickCounters`, `refreshScreenResolutions`. Usan miembros que no existen: `screenPlayer`, `otherPlayerScreen`, `otherOwnedGuiScreens`, `contextSavedPlayer`, `otherLeftClickCounter`, `otherRightClickDelayTimer`. Además, el compilador no ve `currentScreen` ni `thePlayer` dentro de ellas, probablemente por un `#if`/`#endif` descuadrado alrededor; revísalo.
- Puede haber más usos sueltos de esas funciones en `Minecraft.cpp`, `EntityRenderer.cpp`, `GuiScreen.cpp`, `GuiIngameMenu.cpp`, `EntityPlayerSP.cpp`, `MovementInputFromOptions.cpp` y `ContainerSlotNavigator.cpp`.

## Tarea
### Parte A: que compile
Quita esos restos de nuestra pantalla partida de Xbox, de modo que el comportamiento sea **el de OptiJuegos**. Referencia exacta de su versión: `git show origin/main:<ruta>`.
- **Criterio:** donde dudes, deja el código como está en `origin/main`.
- **Excepciones que SÍ deben quedarse** (son de Xbox, no de la pantalla partida):
  - todo lo que esté bajo `PLATFORM_XBOX` / `XBOX_PLATFORM` y no hable de "player 2 context";
  - `preloadWorld("Respawning", 16)` en Xbox;
  - la tecla X de crafteo;
  - la liberación de assets del menú (`releaseWorldEntryAssets`);
  - `XboxCrashLog`;
  - las comprobaciones de `ItemRenderer.cpp` (`isHeldByInventory`).

**Aceptación A:** la compilación imprime `sucessfully created OptiCraft.iso`, sin líneas `error`.

### Parte B (solo si A está terminada): la pantalla partida de ELLOS en Xbox
La de ellos está en `src/client/Ps2SplitScreen.cpp` y solo se compila en PS2 (`#ifdef PS2_PLATFORM`). Hay que hacer que funcione también en Xbox:
- **Mando 2:** en Xbox ya existe `XboxPad::playerSnapshot(1)` (estado crudo del segundo mando: `held`, `pressed`, sticks) en `src/xbox/input/XboxPad.h`. `XboxPad::setMenuPlayer(int)` y `platformMenuPad()` (que devuelve `XboxPad::menuPlayer()`) son el equivalente de `ps2SetMenuPad`.
- **Botones:** equivalencias PS2 → Xbox:
  - Cross → A, Circle → B, Square → X, Triangle → Y;
  - L1/R1 → White/Black, L2/R2 → LT/RT;
  - R3 → clic del stick derecho, Start → START, Select → BACK.

  Constantes en `XboxPad.h` (`XBOX_PAD_*`).
- **Llamadas:** donde el código común llame a `Ps2SplitScreen::tick/postTick` bajo `PS2_PLATFORM`, que también se llame en Xbox. Mira también `MovementInputFromOptions.cpp` (movimiento del J2) y la cámara del J2 en `EntityRenderer.cpp` (bloque `#if defined(PS2_PLATFORM)` que usa `platformGamepadSnapshot(1)`). En Xbox, `platformGamepadSnapshot(1)` ya devuelve el mando 2.
- **Enfoque:** no reescribas su diseño, **adáptalo**. Lo ideal son `#if PLATFORM_PS2 ... #elif PLATFORM_XBOX ... #endif` pequeños en sus ficheros.

**Aceptación B:** compila (`sucessfully created OptiCraft.iso`) y en `Ps2SplitScreen.cpp` (o un fichero hermano) hay rama Xbox para unirse con START del mando 2. La prueba real la hace el usuario en la consola; tú no puedes verlo.

## Tuyos / no tuyos
- **TUYOS:** cualquier fichero dentro de `...\OptiCraft-prueba-opti\src\`.
- **NO TUYOS, no tocar:**
  - la carpeta `...\OptiCraftHeritageEdition\` (la de trabajo principal), ni ninguna otra carpeta fuera de `OptiCraft-prueba-opti`;
  - nada de git que cambie historia: ni commit, ni reset, ni checkout de otras ramas, ni stash. El merge debe seguir sin commit; yo lo cierro;
  - `CMakePresets.json`, `cmake/`, `scripts/`.
- No crees ficheros sueltos en el Escritorio. Guiones temporales solo en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\tmp-agy\`.

## Trampas conocidas
- Los ficheros usan **CRLF** en muchos casos: al editar, conserva el final de línea que tenga el fichero.
- Hay ficheros bajo `src/xbox/` que se compilan con las cabeceras del XDK (STL de 2003). No metas `std::` de la STL moderna cruzando esa frontera.
- La Xbox (Pentium III) no tiene SSE2: no uses `std::pow` en código nuevo de Xbox (hay `JavaMath::pow`).
- La compilación tarda: no la lances dos veces a la vez.

## Informe
Escribe el informe en:
`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\09-INFORME-agy-prueba-opti.md`

- **Primera línea literal:** `ESTADO: TERMINADO A+B`, `ESTADO: TERMINADO A` o `ESTADO: BLOQUEADO`.
- **Después:**
  - lista de ficheros tocados, con una frase por cada uno;
  - la última línea que imprimió la compilación;
  - lo que no pudiste hacer.

No lo cuentes solo por la terminal: el informe tiene que estar en el fichero.
