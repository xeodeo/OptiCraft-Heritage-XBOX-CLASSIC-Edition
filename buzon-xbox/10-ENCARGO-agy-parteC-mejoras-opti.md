# ENCARGO para agy, PARTE C: activar en Xbox las mejoras de OptiJuegos que solo funcionan en PS2/Wii

Mismo sitio y mismas reglas que el encargo 09. Carpeta `...\OptiCraft-prueba-opti`, compilar con `buzon-xbox\build_prueba.ps1`, sin commits. Tuyos: `...\OptiCraft-prueba-opti\src\`. Empieza solo cuando la Parte A del 09 compile. Si algo no está en tu lista, para y pregunta. Si crees que me equivoco, dilo.

Lo he revisado yo en el código de `origin/main`. Cada punto dice fichero, qué hay y qué falta para Xbox. Hazlos en este orden y compila después de cada 2-3.

## C1. El dragón se congela al salir de la zona cargada (PRIORIDAD)
- **Síntoma** (lo vio el usuario en la consola): en el End, el dragón vuela fuera de los ~3 chunks que la Xbox mantiene cargados y se queda quieto en el aire.
- **Solución de ellos:** `PLATFORM_ENTITY_CHUNK_RETENTION` + `Entity::getChunkRetentionRadius()` (`EntityDragon` devuelve 1) + `World::isChunkRetainedByEntity`. En `src/platform/PlatformConfig.h` ya incluye `PLATFORM_XBOX`.
- **Lo que falla:** `src/net/minecraft/src/ChunkProviderLoadOrGenerate.cpp`, que es el que usa la Xbox, lo comprueba con `#if PLATFORM_PS2` en dos sitios: `canChunkExist` (~línea 171) e `isOutsideUnloadRadius` (~185).
- **Qué hacer:** cambia esos dos `#if PLATFORM_PS2` por `#if PLATFORM_ENTITY_CHUNK_RETENTION`, como hace `ChunkProvider.cpp`. Comprueba que los usos en `World.cpp` bajo ese macro compilan en Xbox.

## C2. Su menú de crafteo (`legacy/LegacyCraftingScreen.cpp`) con mando de Xbox
En Xbox usa la ruta genérica (`platformTextInputSnapshot`, ~línea 2166). Falta:
- **Cambiar de categoría:** la ruta genérica no lo hace. Añade `PLATFORM_TEXT_PREV_PAGE` → `changeCategory(-1)` y `PLATFORM_TEXT_NEXT_PAGE` → `changeCategory(1)`. En Xbox son los botones White y Black (`mapTextButtons` en `src/platform/InputBackend_XBOX.cpp`; `PLATFORM_TEXT_TAB_LEFT/RIGHT` son alias de PREV/NEXT_PAGE en `Input.h`).
- **Pistas de botones:** tiene `#if PLATFORM_PS2 {"L1/R1","D-Pad","Cross","Circle"}` / `#elif PLATFORM_WII`. Añade `#elif PLATFORM_XBOX` con `{"White/Black","D-Pad","A","B"}`. `drawControlHintRow` ya dibuja los iconos de Xbox con esos nombres (`ControlIcon.cpp`, rama `PLATFORM_XBOX`).
- **Textos de pestañas:** "L1"/"R1" (`#if PLATFORM_PS2`) → en Xbox, "White"/"Black" o sus iconos (`controlIconTexture(mc, "White")` + `drawControlIcon`).
- **"Ready [Cross]":** en Xbox, "Ready [A]".

Botones Xbox en menús (ya mapeados): A = `PLATFORM_TEXT_TYPE`, B = CLOSE, X = BACK, Y = SHIFT, START = ENTER, White/Black = PREV/NEXT_PAGE.

## C3. Su inventario creativo (`GuiContainerCreative.cpp`) con mando de Xbox
- **Pestañas:** tiene un bloque `#if PLATFORM_PS2` que lee `ps2PadGetSnapshot` para cambiar de pestaña (L1/R1). Añade la rama Xbox con `platformTextInputSnapshot(platformMenuPad())` y PREV/NEXT_PAGE.
- **Textos:** "L1"/"R1" dibujados con `#if PLATFORM_PS2` → White/Black en Xbox.

## C4. Coger objetos con el mando (`GuiContainer.cpp`)
Hay `#if PLATFORM_PS2 || PLATFORM_WII` → `if (controllerSlot != nullptr) slot = controllerSlot;`. Es su arreglo de "coger objetos" con navegador de ranuras. La Xbox usa el mismo `ContainerSlotNavigator`: añade `|| PLATFORM_XBOX`, salvo que veas que choca con algo (entonces dilo en el informe).

## C5. Cerrar pantalla sin arrastrar teclas (`client/Minecraft.cpp`)
En `closePlayerScreen` hay `#if PLATFORM_PS2 || PLATFORM_WII lwjgl::Keyboard::clearEvents();`. Añade Xbox si `lwjgl::Keyboard::clearEvents` existe en `src/xbox/lwjgl/`; si no existe, sáltalo y dilo.

## C6. Mods nuevos con mando de Xbox
- **`src/mods/strongholdlocator/StrongholdLocatorMod.cpp`:** se activa con L3+R3 en PS2 y F7 en PC. En Xbox, clic de los dos sticks: `XBOX_PAD_LEFT_THUMB` + `XBOX_PAD_RIGHT_THUMB` con `XboxPad::snapshot()` (`src/xbox/input/XboxPad.h`). Ojo: ese header no incluye XDK, se puede usar desde código común.
- **`src/mods/reiminimap/GuiWaypointManager.cpp`:** navegación con mando solo en PS2 (`ps2PadGetSnapshot`). Añade la rama Xbox con `platformTextInputSnapshot(platformMenuPad())`: D-Pad navegar, A elegir, X borrar, B atrás. Pistas en Xbox: `{"D-Pad","A","X","B"}`.
- Comprueba que los `#if !PLATFORM_PS2 && !PLATFORM_WII` que incluyen el teclado de PC no rompen la compilación de Xbox. Si rompen, añade `&& !PLATFORM_XBOX` donde toque.

## C7. Opción de pantalla partida horizontal/vertical (`legacy/LegacyHeritageOptions.cpp`)
El botón `BUTTON_SPLITSCREEN_LAYOUT` (`EnumOptions::SPLITSCREEN_LAYOUT`) está solo bajo `#if PLATFORM_PS2` (también el `++rowCount`). Si terminaste la Parte B (pantalla partida en Xbox), actívalo también para Xbox. Si no, déjalo.

## NO hacer (son de PS2 y no los queremos en Xbox)
- Luz suave al 25% (`GameSettings.cpp`, `LegacyOptionState.cpp`).
- Tono de cielo de PS2 (`BiomeGenBase.cpp`).
- `ps2_draw_2d_flush_pending` (`GuiIngame.cpp`).
- Tarjeta de memoria.
- `ThreadPollServers` (en Xbox los hilos funcionan; déjalo).

## Aceptación
- Compila: `sucessfully created OptiCraft.iso`, sin `error`.
- Cada punto C1-C7: hecho, o explicado por qué no.

## Informe
Añádelo al MISMO fichero `buzon-xbox\09-INFORME-agy-prueba-opti.md`, en una sección "## Parte C". La primera línea del fichero pasa a ser `ESTADO: TERMINADO A+B+C` (o lo que corresponda). Por cada punto: ficheros tocados y una frase.
