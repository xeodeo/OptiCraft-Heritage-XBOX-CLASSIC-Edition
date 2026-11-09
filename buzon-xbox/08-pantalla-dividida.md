# Pantalla dividida (2 jugadores) en Xbox: estado y plan

Actualizado el 26-09-2026. Léelo antes de tocar la pantalla dividida. Si la sesión se corta, sigue desde "Plan" marcando lo hecho.

## De dónde viene
- Upstream (OptiJuegos) añadió pantalla dividida para **PS2**: commit `90700cc`, fusionado en `121448d` (rama `xbox-port`). Código: `src/client/Ps2SplitScreen.cpp`, render en `EntityRenderer::renderSplitScreen`.
- El usuario (con otra sesión) hizo la versión **Xbox**, todavía **sin commit** en el working tree:
  - `src/client/XboxSplitScreen.cpp/.h`: unirse o salir el J2, su tick, el giro de su cámara y la correa de 32 bloques;
  - `xbox/input/XboxPad.*`: `playerSnapshot(1)` y `setMenuPlayer`;
  - `platform/InputBackend_XBOX.cpp`;
  - `client/Minecraft.cpp`, `EntityRenderer.cpp`, `ItemRenderer.cpp`, `MovementInputFromOptions.cpp`, `legacy/LegacyControlsScreen.cpp`, `xbox/main_xbox.cpp`;
  - `xbox/system/XboxCrashLog.*` (nuevo).
- El J2 es un segundo `EntityPlayerSP` en el mismo mundo (`mc->thePlayer2`; `mc->thePlayerOne` = J1). Entra con START en el mando 2 y solo en mundos Limited (256x256). Cada tick se cambia `mc->thePlayer` al jugador que actúa.

## Problemas que pidió arreglar el usuario (26-09)
1. **El J2 queda inmortal después de morir.** HECHO, falta compilar.
   - Causa: `EntityPlayer::onDeath` hace `setSize(0.2,0.2)`. La reaparición de la pantalla dividida resucita al mismo objeto sin `preparePlayerToSpawn()`, así que la caja de golpe se queda de 0,2 y los golpes del J1 no le dan.
   - Arreglo: `p2->preparePlayerToSpawn(); hurtTime=0; heartsLife=0;` en `XboxSplitScreen.cpp` y también en `Ps2SplitScreen.cpp` (tenía el mismo fallo).
2. **El J2 no tiene skins.** HECHO, falta compilar.
   - Causa: `data/assets` de Xbox no tenía la carpeta `skins/`. El J1 funcionaba por el respaldo a `char.png`; el J2 no tenía respaldo.
   - Arreglo:
     - Se sacaron las 64 PNG de `assets/skins/` del `assets.pak` que venía en `Downloads\ASSETS.7z`, a `OptiCraftHeritageEdition\data\assets\skins\`. `data/` no está en git; la copia temporal está en el scratchpad.
     - `SkinManager::getPlayer2SkinTexture` ahora tiene el mismo respaldo que el J1.
3. **Que los dos puedan abrir inventario o crafteo a la vez, como en Legacy.** CÓDIGO ESCRITO (26-09), falta compilar y probar. Ver "Diseño elegido".
4. **La cámara del J2 no giraba en Xbox.** HECHO, falta compilar. `XboxSplitScreen::turnCamera` estaba dentro de un bloque solo de PS2 (`PLATFORM_DIRECT_ANALOG_MOVEMENT`) en `EntityRenderer.cpp`. Ahora se llama antes del bloque de cámara del J1, sin depender de su foco.

5. **El J2 entraba "como espectador"** (26-09, el usuario lo probó en consola): no golpeaba, no rompía y no interactuaba. HECHO y compilado, falta probar.
   - Causa: el J2 solo llamaba a `clickMouse(0, true)` (mantener pulsado) y nunca a `clickMouse(0)` (el golpe o clic inicial). Además compartía `leftClickCounter` con el J1, que vale 10000 mientras el J1 tiene un menú abierto.
   - Arreglo:
     - `leftClickCounter` y `rightClickDelayTimer` se intercambian con el contexto del J2 (`otherLeftClickCounter`…) y `tickClickCounters()` los descuenta.
     - El J2 hace como el J1: RT pulsar → golpe o clic, RT mantener → minar, LT → usar y repetir, soltar LT → `onStoppedUsingItem` (comer, arco).
   - Límite conocido: los dos comparten `playerController`, así que si minan a la vez se estorban.
6. **Nombre propio del J2:** HECHO y compilado. Ajuste `playerName2` en `options.txt` y campo "Player 2 Name" en Opciones → Heritage (solo Xbox). Se aplica al unirse (`s_sessionP2.username`).

## Cómo lo hace Legacy (resumen del análisis de `Downloads\minecraft`)
- **Jugadores:** hay un jugador local por mando (`localplayers[]`, `localgameModes[]`). `Minecraft::setLocalPlayerIdx(i)` cambia los globales (`player`, `gameMode`, `cameraTarget`…) y el tick y el render se ejecutan una vez por jugador.
- **UI por mando:** hay un grupo por mando (`UIController m_groups[iPad+1]`) y un grupo de pantalla completa.
  - `NavigateToScene(iPad, …)` y `CloseUIScenes(iPad)` solo afectan a ese mando.
  - La entrada de un mando solo llega a su grupo. Solo ese mando pasa a "modo menú"; el otro sigue jugando.
  - Si ese mando ya tiene un menú abierto, se ignoran nuevas peticiones.
- **Dibujo:** cada escena se dibuja dentro del trozo de pantalla de su jugador (`getRenderDimensions` y `setupRenderPosition`), con versiones "Split" o "_small" de cada escena.
- **Contenedores:** cada jugador tiene su propio `containerMenu` o `inventoryMenu` y su propio contador de ventana.
- **Pausa:** el menú de pausa en pantalla dividida no pausa al otro jugador.
- **Muerte del J2:** hay pantalla de muerte por mando. El servidor crea una entidad nueva, que se registra de nuevo en el mundo con su id nuevo.
- **Skins:** hay una skin por mando (`GameSettingsA[iPad]->dwSelectedSkin`). Las de fábrica son `char.png`…`char7.png`.

## Estado actual del port (antes del cambio)
- Hay un solo `mc->currentScreen` y un indicador `screenOwnedByPlayer2`. Si uno abre menú, el otro recibe "Menu in use by player N".
- La pantalla se dibuja completa encima de las dos mitades (`EntityRenderer.cpp` ~1464-1481). El J1 y el J2 se congelan con cualquier pantalla abierta.
- Hay una sola ruta de mando para menús (`XboxPad s_menuPlayer`) y un solo `ContainerSlotNavigator` (singleton).
- Hay una sola "pulsación retenida" (`s_latched`) compartida.

## Plan (hacer en este orden, compilar y probar en cada paso)
1. [x] **Pulsaciones por jugador:** en `XboxPad`, un latch por jugador (`consumePressed(player)`). `platformTextInputSnapshot(port)` debe usar el puerto, y `platformMenuPad()` debe devolver el jugador del "contexto actual".
2. [x] **Pantalla propia del J2:** añadir `mc->screenP2` y un "contexto de jugador" (0/1).
   - Con contexto J2, `displayGuiScreen()` escribe en `screenP2`. Así el menú de pausa → opciones, etc. funcionan sin tocarlos.
   - `EntityPlayerSP::closeScreen()` del J2 cierra solo la suya.
3. [x] **Ámbito de contexto** (como `setLocalPlayerIdx`). Se guarda y se restaura:
   - `thePlayer`, `currentScreen` ↔ `screenP2`, `objectMouseOver`;
   - el jugador de menú de `XboxPad`;
   - el navegador de ranuras (una instancia por jugador).
4. [x] **Tick de la pantalla del J2** en contexto J2: `handleInput()` sin vaciar las colas de ratón y teclado del J1, y luego `updateScreen()`. B/Y del mando 2 → `keyTyped(ESC)` en pantallas que no son "specialized".
5. [x] **Dibujo por mitad:** en `renderSplitScreen`, después del HUD de cada mitad, se dibuja la pantalla de ese jugador con `ScaledResolution` de media altura. Las pantallas se crean y redimensionan con ese tamaño, y también cuando entra o sale el J2.
6. [x] **Congelar solo al que tiene menú:**
   - El J2 mira `screenP2`, no `currentScreen` (`XboxSplitScreen::tick`, `turnCamera`, `MovementInputFromOptions`).
   - El J1 sigue como está.
   - Con el J2 unido, la pausa no pausa el mundo.
7. [ ] **Probar:** en xemu, con autopilot, el 7.º campo son los botones del mando 2 (ya soportado en `XboxPad.cpp`). La prueba final es en la consola real con 2 mandos.

## Diseño elegido (detalle técnico)
- **`Minecraft`:** `otherPlayerScreen` y `otherOwnedGuiScreens` guardan la pantalla del jugador que NO está en contexto. `screenPlayer` vale 0 o 1.
  - `enterPlayer2Context()` intercambia: `currentScreen` ↔ `otherPlayerScreen`, `ownedGuiScreens` ↔ `otherOwnedGuiScreens`, `thePlayer` → `thePlayer2`, `objectMouseOver` ↔ `objectMouseOver2`, y hace `XboxPad::setMenuPlayer(1)`.
  - `leavePlayer2Context()` lo deshace.
  - Así TODO el código de pantallas existente (inventario, crafteo, pausa, mesa, cofre…) funciona para el J2 sin cambios.
- **`displayGuiScreen` en contexto J2:** no toca el foco del ratón del J1 y no crea `GuiGameOver`.
- **Resolución:** con J2 unido, las pantallas usan `ScaledResolution(displayWidth, displayHeight/2)`. Se redimensionan al entrar o salir el J2.
- **`XboxPad`:**
  - `snapshot()` es siempre el mando 1.
  - Hay un latch por jugador. `consumePressed()` y `clearLatchedPressed()` usan el del jugador de contexto.
  - `platformTextInputSnapshot` lee el mando del jugador de contexto.
  - `platformMenuPad()` devuelve `menuPlayer()`.
  - Puntero activo, cursor visible y `setMenuCursor` solo con `menuPlayer()==0`: el J2 navega con cruceta, sin puntero.
- **`ContainerSlotNavigator::instance()`:** una instancia por jugador en Xbox.
- **`GuiScreen::handleInput`:** en contexto J2 no vacía las colas de ratón y teclado, que son del J1.
- **`XboxSplitScreen::tick`:**
  - Si el J2 tiene pantalla: contexto J2, stick → cruceta, B/Y → ESC si la pantalla no es "specialized", `handleInput` y `updateScreen`.
  - Si no la tiene: limpiar su latch; START = pausa, Y = inventario y X = crafteo, todo en contexto J2. Atacar y usar también en contexto J2, así la mesa o el cofre se abren en su mitad.
- **`EntityPlayerSP::closeScreen`:** si el jugador es el J2 y no está en contexto, entra en contexto para cerrar su pantalla.
- **Pausa:** `isGamePaused` es falso con J2 unido.
- **Render (`EntityRenderer::renderSplitScreen`):**
  - Tras el HUD de cada mitad se dibuja la pantalla de ese jugador; la del J2 en contexto J2.
  - La pantalla del J1 ya no se dibuja completa al final si la pantalla dividida está activa.

## Cuidado
- No subir nada sin que el usuario lo pida; lo de pantalla dividida del usuario aún no tiene commit.
- Hay otra sesión de Claude ("Opticvraft para Xbox clásico") que también toca este repo. Mirar `git status` antes de commitear.
- El 1080i de esa sesión ya no aparece en el working tree (revisar si se descartó o se integró).

7. **El J2 no podia romper bloques** (26-09): el tick del J1 llamaba resetBlockRemoving() sobre el playerController compartido cada tick que el J1 no atacaba. Arreglo: flag mc->player2Mining (RT del J2) y Minecraft::clickMouse no resetea en contexto J1 si el J2 mina. Compilado, falta probar.
8. **Pendiente** (pedido 26-09): OptiCraft Options queda cortado abajo con un jugador; en pantalla dividida, quitar el titulo Heritage del menu de pausa para ganar espacio.
9. **Hecho 26-09 (compilado):** Heritage Options vuelve a su altura original: los nombres J1 y J2 van lado a lado en la misma fila (lo cortaban las 2 filas extra). Pausa en pantalla dividida: sin titulo Heritage y botones centrados (GuiIngameMenu). Aviso: un parche mio dejo CR dobles (\r\r\n) en LegacyHeritageOptions.*; ya corregido.
