ESTADO: TERMINADO A+B+C

## Resumen de cambios

### Parte A (Arreglar compilación y restos de la pantalla partida de Xbox)
- En `src/net/minecraft/src/EntityPlayerSP.cpp` se eliminó el bloque inicial que usaba `enterPlayer2Context/leavePlayer2Context` en `closeScreen()`, devolviendo la estructura a como estaba en `origin/main`.
- En `src/net/minecraft/src/ContainerSlotNavigator.cpp` se quitó la llamada `steersMenuPointer()` que generaba un error por no estar en `origin/main`.
- En `src/net/minecraft/src/EntityRenderer.cpp` se eliminó el macro huérfano `XBOX_FRAME_SLOT_END(3, xboxScreen)` en la función de dibujado de GUI, ya que en el código de origen de la rama fusionada no existe este profiling de frame específico allí.
- En `src/net/minecraft/src/GuiContainer.cpp` se corrigieron los bloques `#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)` eliminando `XBOX_PLATFORM`, ya que en origen la variable `controllerSlot` se declara en un bloque que no incluye a Xbox.
- En `src/net/minecraft/src/MovementInputFromOptions.cpp` se actualizó la lógica de control del jugador 2 en Xbox, reemplazando la llamada al inexistente `owner->player2Screen()` por `owner->isPlayerScreenActive(1)`.
- En `src/net/minecraft/src/legacy/LegacyHeritageOptions.cpp` se agregaron las variables enumeradoras de botones específicas de Xbox (`BUTTON_EDIT_PLAYER_NAME2`, `BUTTON_LEGACY_CRAFTING`) que se habían perdido en el namespace durante el merge.
- En `src/platform/Input.h` se añadieron las constantes `PLATFORM_TEXT_TAB_LEFT` y `PLATFORM_TEXT_TAB_RIGHT` para que el backend de teclado de Xbox pueda seguir mapeando correctamente los botones L1/R1 (White/Black) en menús textuales.

### Parte B (Adaptar pantalla partida PS2 a Xbox)
- En `src/client/Minecraft.cpp` se reemplazaron las llamadas vacías a `XboxSplitScreen::tick` y `postTick` para que Xbox invoque ahora a `Ps2SplitScreen::tick(this)` y `postTick(this)`.
- En `src/client/Ps2SplitScreen.h` se incluyó `XBOX_PLATFORM` en la definición de la guarda del header para exponer los métodos en ambas consolas.
- En `src/client/Ps2SplitScreen.cpp`:
  - Se extendió la directiva de compilación (`#if defined(PS2_PLATFORM) || defined(XBOX_PLATFORM)`) para que compile en ambas plataformas.
  - Se añadió una capa de compatibilidad para mapear las entradas y botones de Xbox (ej. `XBOX_PAD_A`, `XBOX_PAD_B`, `XBOX_PAD_WHITE`, etc.) hacia las constantes hardcodeadas nativas de PS2 (`PS2_PAD_CROSS`, `PS2_PAD_CIRCLE`, `PS2_PAD_L1`, etc.) evitando así la reescritura de toda la lógica.
  - Se creó un wrapper `ps2PadGetSnapshot(int port)` local en la capa de abstracción para Xbox, que internamente pide el `XboxPad::playerSnapshot(port)` y lo devuelve con el layout nativo para que `tick()` funcione transparente.
  - Se sustituyó el manejo de pad de menús (`ps2SetMenuPad` / `ps2SetMenuOwnerPad`) por unas funciones puente (`platformSetMenuPad` y `platformClearMenuPad`), ruteando de manera adecuada en Xbox hacia `XboxPad::setMenuPlayer`.
  - Se adaptaron los mensajes de chat enviados en `joinPlayer2` para que, cuando compila en Xbox, indique los botones reales del mando de Microsoft (A, Click Derecho, X, en lugar de X, R3, Cuadrado).
- En `src/net/minecraft/src/EntityRenderer.cpp` se extendió el bloque de guardas del jugador 2 (`#if defined(PS2_PLATFORM)`) para que Xbox también lo corra, logrando así que el player 2 en Xbox pueda mover la vista libremente con el joystick derecho.

Se verificó el resultado mediante el script `build_prueba.ps1` y se compiló la ISO `OptiCraft.iso` exitosamente.

### Parte C (Mejoras de OptiJuegos)
- **C1:** En `src/net/minecraft/src/ChunkProviderLoadOrGenerate.cpp` se cambió `#if PLATFORM_PS2` por `#if PLATFORM_ENTITY_CHUNK_RETENTION` en `canChunkExist` e `isOutsideUnloadRadius` para arreglar el dragón congelado en Xbox.
- **C2:** En `src/net/minecraft/src/legacy/LegacyCraftingScreen.cpp` se añadieron las pulsaciones `PLATFORM_TEXT_PREV_PAGE` y `PLATFORM_TEXT_NEXT_PAGE` al manejo del mando de Xbox para cambiar de pestaña, se añadió el hint visual de Xbox (`{"White/Black", "D-Pad", "A", "B"}`), se usaron los íconos de "White"/"Black" para las pestañas y se adaptó "Ready [Cross]" a "Ready [A]".
- **C3:** En `src/net/minecraft/src/GuiContainerCreative.cpp` se incluyeron `ControlIcon.h` e `Input.h`, se añadió la rama `PLATFORM_XBOX` para manejar el pase de página con `PLATFORM_TEXT_PREV_PAGE / NEXT_PAGE`, borrar ítemes con `PLATFORM_TEXT_SHIFT` y se agregaron los íconos de White/Black como hints visuales.
- **C4:** En `src/net/minecraft/src/GuiContainer.cpp` se habilitó el uso de `controllerSlot` en Xbox añadiendo `|| defined(XBOX_PLATFORM)` en todos los macros relacionados al Slot Navigator para que se puedan coger objetos con el mando al igual que en PS2 y Wii.
- **C5:** Saltado. No se encontró ninguna función `lwjgl::Keyboard::clearEvents()` en `src/xbox/lwjgl/`.
- **C6:** En `src/mods/strongholdlocator/StrongholdLocatorMod.cpp` se agregó `#elif PLATFORM_XBOX` (leyendo clics de ambos sticks mediante `XboxPad::snapshot()`). En `src/mods/reiminimap/GuiWaypointManager.cpp` se agregó `#elif PLATFORM_XBOX` para capturar botones mediante `platformTextInputSnapshot`, así como las pistas visuales adecuadas de los botones de Xbox. Además, se adaptó el macro `#if !PLATFORM_PS2 && !PLATFORM_WII` para ignorar los llamados de teclado en Xbox (`&& !PLATFORM_XBOX`).
- **C7:** En `src/net/minecraft/src/legacy/LegacyHeritageOptions.cpp` se habilitó la opción `BUTTON_SPLITSCREEN_LAYOUT` (horizontal/vertical) en Xbox cambiando el macro de exclusión a `#if PLATFORM_PS2 || PLATFORM_XBOX`.
