# Tarea agy-input — Mando de Xbox (XInput) -> controles del juego

Lee primero `00-CONTEXTO.md` (mismo buzón).

## Objetivo
Jugar con el mando de Xbox (Duke / Controller S) igual que en Wii/PS2 con sus mandos.

## Cómo funciona en Wii (tu modelo)
- `src/platform/InputBackend_WII.cpp` + `src/wii/input/*` leen el mando y generan eventos de
  teclado/ratón virtuales para la capa lwjgl (`src/lwjgl/Keyboard.h`, `Mouse.h`), implementada en
  `src/wii/lwjgl/Keyboard_wii.cpp` y `Mouse_wii.cpp`. Mapeo de botones en `WiiPadKeyCodes.cpp` /
  `WiiButtonBindings.cpp`; cursor para menús en `WiiPointer.cpp`.
- PS2 hace lo mismo con su pad analógico (`src/ps2/...`): útil porque tiene **dos sticks**, como Xbox.

## Xbox (XDK)
- Incluye `"XboxXtl.h"`. Una vez al arrancar: `XInitDevices(0, NULL)`. Luego sondea inserción con
  `XGetDevices(XDEVICE_TYPE_GAMEPAD)` / `XGetDeviceChanges`, abre con
  `XInputOpen(XDEVICE_TYPE_GAMEPAD, puerto, XDEVICE_NO_SLOT, NULL)` y lee con `XInputGetState`.
- `XINPUT_GAMEPAD`: `wButtons` (DPAD_UP/DOWN/LEFT/RIGHT, START, BACK, LEFT_THUMB, RIGHT_THUMB),
  `bAnalogButtons[8]` (A, B, X, Y, BLACK, WHITE, LEFT_TRIGGER, RIGHT_TRIGGER; analógicos 0-255,
  considera pulsado > ~30), `sThumbLX/LY/RX/RY` (-32768..32767, aplica zona muerta).
- Mapeo sugerido (estilo Minecraft consola): stick izq = mover, stick der = cámara, A = saltar,
  B = soltar/atrás, X = inventario, Y = crafteo/abrir, RT = romper/atacar, LT = usar/poner,
  WHITE/BLACK = cambiar slot, clic stick izq = correr, clic stick der = agacharse, START = pausa,
  BACK = cambiar vista. En menús: stick izq mueve cursor (como WiiPointer), A = clic.

## Ficheros tuyos
`src/platform/InputBackend_XBOX.cpp`, `src/xbox/input/**`, `src/xbox/lwjgl/Keyboard_xbox.cpp`,
`src/xbox/lwjgl/Mouse_xbox.cpp`. Si `src/pc/lwjgl/Keyboard.h`/`Mouse.h` o `src/lwjgl/*.h` necesitan
una guarda Xbox para no incluir SDL (hoy fallan: `No se puede abrir 'SDL_events.h'`), puedes
tocarlas SOLO para eso (probablemente ya estén; verifica).
**No toques** Display/GLContext (agy-render) ni `PlatformCompat.h` (agy-plataforma).

## Aceptación
`compile_one.ps1` da `OK` en tus ficheros, y también en `src/net/minecraft/src/MouseHelper.cpp` y
`src/net/minecraft/src/GameSettings.cpp` (comprobación de que las cabeceras lwjgl compilan en Xbox;
no los edites, son solo testigos).

ESTADO: PAUSADO
# (1) QUE QUEDO HECHO Y FUNCIONANDO
- Comprobación del entorno de compilación con `compile_one.ps1`:
  - `src\net\minecraft\src\MouseHelper.cpp`: OK
  - `src\net\minecraft\src\GameSettings.cpp`: OK
  - `src\platform\InputBackend_XBOX.cpp` (placeholder existente): OK
- Verificado que las cabeceras lwjgl compartidas ya compilan correctamente para Xbox sin requerir ajustes inmediatos en `MouseHelper.cpp` / `GameSettings.cpp`.
- Ficheros de código modificados: ninguno (se detuvo la ejecución antes de realizar cambios de código).

# (2) QUE QUEDO A MEDIAS
- Análisis preliminar de la estructura de input de Wii (`src/wii/input/`, `src/wii/lwjgl/`) y PS2 para adaptar el modelo a Xbox XInput (`XInitDevices`, `XGetDevices`, `XInputOpen`, `XInputGetState`).
- Estructuración pendiente de `src/xbox/input/`, `src/xbox/lwjgl/Keyboard_xbox.cpp`, `src/xbox/lwjgl/Mouse_xbox.cpp` e implementación de `src/platform/InputBackend_XBOX.cpp`.

# (3) QUE HAS DESCARTADO Y POR QUE
- No se descartó nada técnico; se detuvo la ejecución inmediatamente por orden de pausa del coordinador sin ejecutar comandos adicionales ni modificar fuentes.

# (4) QUE SABES QUE NO ES OBVIO
- Los archivos testigo `src\net\minecraft\src\MouseHelper.cpp` y `src\net\minecraft\src\GameSettings.cpp` ya compilan de forma limpia (`OK`) en `compile_one.ps1`, por lo que las cabeceras `Keyboard.h` y `Mouse.h` ya no arrastran `SDL_events.h` en el entorno actual.

# (5) EL SIGUIENTE PASO CONCRETO
1. Crear `src/xbox/input/` con la capa de sondeo de XInput (manejo de conexión/desconexión con `XGetDeviceChanges`, deadzones de thumbsticks y umbrales analógicos).
2. Implementar `src/xbox/lwjgl/Keyboard_xbox.cpp` y `Mouse_xbox.cpp` emulando eventos para lwjgl según el mapeo de botones/sticks propuesto (y puntero virtual para menús).
3. Implementar `src/platform/InputBackend_XBOX.cpp` conectando el ciclo de bombeo de eventos con el backend de plataforma.
4. Compilar cada archivo con `compile_one.ps1` hasta obtener `OK`.

# (6) QUE NO VERIFICASTE
- Comportamiento en tiempo de ejecución de lectura XInput en xemu / hardware real Xbox (aún no implementado ni enlazado).

