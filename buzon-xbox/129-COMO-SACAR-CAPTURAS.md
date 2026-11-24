# Cómo sacar capturas de OptiCraft (xemu + autopilot) — 24-11-2026

Método real para capturas CON EL JUEGO MOVIÉNDOSE SOLO. Esto antes no estaba anotado.

## Piezas (ya existen)
- **xemu**: `C:\RetroXeo\emulators\xemu\xemu.exe` (arrancar con `-dvd_path "<ISO>"`).
- **`OptiCraftHeritageEdition\xbox-spike\captura.ps1 -Out <png>`**: captura la ventana de xemu que esté abierta
  (PrintWindow, solo la ventana del juego, no el escritorio). Necesita xemu ya corriendo. **Usar ESTE, no reinventarlo.**
- **`OptiCraftHeritageEdition\xbox-spike\run-xemu.ps1 -Iso <ISO> -Seconds <n> -Shot <png> -KeepOpen`**: arranca xemu,
  espera y captura una vez. Para una sola foto.
- **Autopilot** (el juego se mueve solo): build con `-DXBOX_AUTOPILOT=ON` y un fichero **`autopilot.txt`** dentro del ISO
  (`bin\<preset>\iso\autopilot.txt`), que el juego lee de `D:\autopilot.txt`.
- **`buzon-xbox\122-trabajo\xemu122.ps1`**: pipeline COMPLETO ya hecho — compila el preset `xbox-autotest` con
  AUTOPILOT, copia `autopilot.txt`, arranca xemu con GDB, y cada 5 s llama a `captura.ps1` → `shot_NNN.png` en
  `buzon-xbox\122-trabajo\xemu-<wt>-<tag>\`. Coge el `BUILD.lock`. Uso:
  `powershell -ExecutionPolicy Bypass -File xemu122.ps1 -Wt wt-build-152 -Tag capturas -Seconds 180`.

## Formato de autopilot.txt (de `src\xbox\input\XboxPad.cpp`, bloque `#if XBOX_AUTOPILOT`)
```
<start_ms> <buttons|-> <lx> <ly> <rx> <ry>   [<buttons2> para jugador 2 / split]
```
- `buttons`: `A B X Y START BACK UP DOWN LEFT RIGHT LT RT WHITE BLACK LS RS` unidos con `+`; `-` = ninguno.
- Sticks **-1..1** (en ly/ry, **-1 = arriba**). Cada paso MANTIENE el estado hasta el `start_ms` del paso siguiente.
- Máximo **128 pasos** (`s_steps[128]`).
- Ejemplos de MOVIMIENTO (para capturas bonitas, no menús):
  ```
  0      - 0 0 0 0            # quieto mientras carga el mundo
  8000   - 0 -1 0 0           # andar hacia delante
  14000  - 0 -1 0.6 0         # andar y girar la cámara a la derecha
  20000  - 0 0 0 0            # parar y mirar
  26000  - 0 -1 -0.6 0        # andar girando a la izquierda
  32000  - 0 0 0 -0.4         # mirar un poco hacia arriba (cielo/árboles)
  ```
  (el 122 original solo hacía `BACK+X` para abrir el menú admin; eso NO mueve, es para probar menús.)

## Receta para la web
1. Hacer un `autopilot.txt` con movimiento (como arriba), dejarlo donde lo coja el pipeline.
2. Correr `xemu122.ps1 -Wt wt-build-152 -Tag web -Seconds 180` (coge el candado, compila autotest+autopilot, captura
   `shot_NNN.png`). O a mano: arrancar la ISO autotest con xemu y llamar `captura.ps1` en bucle.
3. Elegir las 3-4 `shot_NNN.png` más bonitas (mundo, sin menú, sin cartel tapando, no negras) y copiarlas a
   `buzon-xbox\web\img\` como `3.png`..`6.png`.
4. **Redeploy** de la web (ver `128-WEB-Y-KOFI.md`).

## Trampas
- `captura.ps1`/`run-xemu.ps1` ya capturan SOLO la ventana de xemu — no hace falta GDB ni PrintWindow a mano.
- Es un BUILD (preset `xbox-autotest`): respeta `BUILD.lock` (una compilación a la vez). `xemu122.ps1` ya lo coge.
- Descartar fotos negras o con el HUD de controles/cartel de bienvenida tapando.

## Jugar EN VIVO desde Claude/agy (mando en vivo por GDB) — añadido 24-11-2026
Para mover el juego paso a paso (mirar captura → decidir → mover), sin guion fijo y SIN tocar el teclado del usuario.
- **Código:** `g_livePad` en `src\xbox\input\XboxPad.cpp` (solo builds `-DXBOX_AUTOPILOT=ON`). Si `magic == 'LIVE'`
  (0x4556494C) manda sobre el mando 1; con `off` vuelve el guion/mando real. Hecho en `wt-build-152` (sin commit).
- **Arrancar:** `powershell -ExecutionPolicy Bypass -File buzon-xbox\122-trabajo\xemu-vivo.ps1 [-NoMinimap]`
  → compila `xbox-autotest`, abre xemu con `-gdb tcp:127.0.0.1:1235` y lo deja abierto. Coge `BUILD.lock`:
  **borrarlo al cerrar xemu.** Usa `122-trabajo\autopilot.txt` (para vivo basta `0 - 0 0 0 0`).
- **Mover:** `python OptiCraftHeritageEdition\xbox-spike\livepad.py <wt>\bin\xbox-autotest\OptiCraft.exe.map <cmd>`
  - `tap A` (pulsar), `tap DOWN 150`
  - `hold 2000 - 0 -1 0 0` (andar 2 s), `hold 600 - 0 0 0.6 0` (girar cámara), `hold 1500 A 0 -1 0 0` (andar saltando)
  - `set ...` (mantener hasta el siguiente comando), `off` (devolver el mando)
- **Ver:** `xbox-spike\captura.ps1 -Out <png>` entre paso y paso.
- **Ocultar minimapa** (solo pruebas): `-NoMinimap` mete `D:\nominimap.txt` en el ISO (`ReiMinimapMod.cpp`).
- Ruta al mundo tutorial desde el menú: `tap A` (Un jugador) → `tap DOWN` → `tap A` (Jugar tutorial), carga ~40 s.
- Trampa: el gdbstub admite UN cliente; no usar a la vez `xemu122.ps1` (su bucle `gdblog`) y `livepad.py`.
