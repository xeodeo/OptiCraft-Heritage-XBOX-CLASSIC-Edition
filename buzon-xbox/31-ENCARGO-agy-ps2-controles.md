# ENCARGO 31 (agy): dos fallos de PS2 que en Xbox ya están arreglados

Si algo no está en tu lista, para y pregunta. Si crees que me equivoco, dilo.

## Dónde
- Carpeta **`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30`**, rama `xbox-opti-30`, commit `6e05847`, que es la unión de ayer ya guardada.
- Solo trabajas ahí. No hagas commit ni push. Yo reviso y hago el commit.
- **La Xbox no debe cambiar**: los dos arreglos son para PS2.

## Fallo 1: al cerrar el inventario con Círculo se suelta el objeto
- **Síntoma:** en PS2, Círculo cierra el inventario y también es "soltar objeto" en el juego (`keyBindDrop` = `PS2_KEY_CIRCLE`). Al cerrar el menú, el mismo botón suelta lo que tienes en la mano.
- **El arreglo de Xbox** (commit `128b852`, en `src/xbox/input/XboxInput.cpp`, busca `s_suppressedButtons`):
  - al pasar de menú a juego, los botones que siguen pulsados se guardan en `s_suppressedButtons`;
  - el juego no los ve hasta que se sueltan (`s_suppressedButtons &= pad.held`).

  Míralo con `git show 128b852 -- src/xbox/input/XboxInput.cpp`.
- **Qué hacer:** lo mismo en la capa de mando de PS2 (`src/ps2/input/Ps2InputMapper.cpp` o donde se pase de estado de menú a juego). Cuando un menú se cierra, los botones aún pulsados se ignoran en el juego hasta que se suelten.
- Que sirva también para el J2 de la pantalla dividida de PS2 (`Ps2SplitScreen.cpp` usa `ps2PadGetSnapshot(1)`).
- En el informe, explica dónde lo pusiste y por qué ahí, con `archivo:línea`.

## Fallo 2: la pantalla de Controles de PS2 no funciona
Es `src/net/minecraft/src/legacy/LegacyControlsScreen.cpp`.
- **Qué pasa:**
  - en PS2 usa el diseño de OptiJuegos de **dos columnas** con dibujo central (`LAYOUT_ART`, que en PS2 es el del teclado de PC). No se ven los iconos;
  - `actionPerformed` tiene `#if !PLATFORM_PS2` (~líneas 294 y 301) que **desactivan** en PS2 capturar botón, página anterior/siguiente y restablecer. Por eso "no anda".
- **En Xbox ya está arreglado:** una sola columna (`#if PLATFORM_XBOX` en `initGui`, ~línea 84), sin dibujo central (`#if !PLATFORM_XBOX` alrededor de `drawLayoutArtwork()`, ~376) e iconos del mando dibujados sobre cada fila (`#if PLATFORM_XBOX`, ~381).
- **Qué hacer en PS2:**
  1. Que use el diseño de **una columna** de Xbox (extiende esos `#if PLATFORM_XBOX` a `|| PLATFORM_PS2`), **sin dibujo central**.
  2. Que se vean los **iconos de PS2** (Cruz, Círculo, Cuadrado, Triángulo, L1/R1, L2/R2, L3/R3, Start, Select). El dibujo de iconos de Xbox usa `LegacyButtonPrompt` y `ControlIcon`. En PS2, `LegacyButtonPrompt` ya usa el atlas `/gui/buttons_ps2.png`, que ya está en el `assets.pak` de PS2. Adapta el mapeo de nombres de botón a icono de PS2.
  3. Que **funcione**:
     - elegir una fila y pulsar un botón para reasignarla;
     - página anterior/siguiente;
     - "Restablecer" y "Atrás".

     Averigua por qué OptiJuegos lo desactivó en PS2 con `#if !PLATFORM_PS2`: quizá la PS2 reasigna en otra pantalla, `LegacyControllerLayoutScreen`. Si hay una razón técnica, dila en el informe antes de cambiarlo.
  4. **No cambies Wii ni PC.**

## Compilar (candado `buzon-xbox\BUILD.lock`)
- `ps2-release` en `wt-opti30`: el enlace `psdevwindows` ya existe. Debe acabar en `Linking CXX executable ...OptiCraft.elf`.
- **También `xbox-release`**, para confirmar que la Xbox sigue compilando igual.
- **Copia la última línea de cada compilación en el informe.** La última vez dijiste que PS2 compilaba y no era verdad (faltaba `#include "ps2/input/Ps2PadKeyCodes.h"`). Esta vez pega la salida real.

## Informe
Escríbelo en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\31-INFORME-agy.md`, con primera línea literal `# INFORME 31 agy`. Incluye:
- las causas y los arreglos, con `archivo:línea`;
- `git diff --stat`;
- las últimas líneas reales de las dos compilaciones.

---

## AÑADIDO (el usuario): mando de PS2 IGUAL que el de Xbox al jugar, y UNA sola opción de crafteo

### A. "Sigue teniendo los controles raros, en el juego"
En PS2 **todo** el juego debe usar la misma distribución que la Xbox (`src/xbox/input/XboxInput.cpp` `updateGameplay` y `GameSettingsBackend_XBOX.cpp`). Los mandos son iguales, salvo que la Xbox tiene Blanco/Negro donde la PS2 tiene L1/R1:

| Acción en el juego | Xbox | PS2 |
|---|---|---|
| Moverse | stick izquierdo / cruceta | stick izquierdo / cruceta |
| Cámara | stick derecho | stick derecho |
| Saltar | A | Cruz |
| Soltar objeto | B | Círculo |
| Inventario (el normal) | Y | Triángulo |
| Nuestro crafteo | X | Cuadrado |
| Atacar / picar | RT | R2 |
| Usar / colocar | LT | L2 |
| Objeto anterior / siguiente | Blanco / Negro | L1 / R1 |
| Agacharse | clic stick izquierdo | L3 |
| Cambiar cámara (F5) | clic stick derecho | R3 |
| Pausa | START | START |
| Lo que haga Back en Xbox | BACK | SELECT |

1. Mira cómo lo hace hoy PS2: `src/ps2/input/Ps2InputMapper.cpp` y `GameSettingsBackend_PS2.cpp`.
2. Pon en el informe una tabla con **lo que había** y **lo que queda**.
3. Si algo de PS2 lo hace una ruta distinta (por ejemplo `PLATFORM_DIRECT_ANALOG_MOVEMENT` para moverse y mirar), no lo rompas: solo cambia qué botón hace qué.
4. Que valga también con un `options.txt` de PS2 ya guardado: migra las teclas viejas, como hiciste en el 30.
5. Las pistas de botones en pantalla en PS2 deben mostrar esos mismos botones.

### B. Opciones de crafteo: iguales en Xbox y PS2, UNA sola
- Hoy en Xbox y PS2 salen **dos** casillas en Opciones → Heritage: "Xbox-Style Crafting" y "Legacy Crafting". El usuario no quiere eso.
- **En Xbox y PS2:** una **sola** casilla que activa **nuestro** menú (`xboxStyleCrafting`), con el nombre que tenía en Xbox en la v1.1: **"Console Crafting"**.
- En Xbox y PS2, **oculta** la casilla "Legacy Crafting" de ellos, con `#if !(PLATFORM_XBOX || PLATFORM_PS2)`. Busca también `GuiBetaOptions.cpp` y `GuiOptiCraftOptions.cpp`, que tienen su "Legacy Crafting: ON/OFF".
- En Xbox y PS2, que el menú de ellos no pueda abrirse en su lugar: si "Console Crafting" está desactivado, sale el crafteo clásico de Minecraft.
- Wii y PC: sin cambios.
- La clave de `options.txt` (`xboxStyleCrafting`) se queda igual.
