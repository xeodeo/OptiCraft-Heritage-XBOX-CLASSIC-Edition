---
name: capturas-xemu-autopilot
description: Cómo sacar capturas de OptiCraft con el juego moviéndose solo (xemu + autopilot + captura.ps1)
metadata:
  node_type: memory
  type: reference
  originSessionId: 4bac41f4-d0be-425d-bc5c-04da57364737
  modified: 2026-11-24T12:56:43.458Z
---

Para capturar el juego EN MOVIMIENTO (no solo el spawn) hay un método ya hecho; estaba sin anotar hasta el 24-11-2026.
Guía completa: `buzon-xbox\129-COMO-SACAR-CAPTURAS.md`.

- **captura.ps1** (`OptiCraftHeritageEdition\xbox-spike\captura.ps1 -Out <png>`) captura la ventana de xemu abierta
  (PrintWindow, solo el juego). NO reinventar la captura.
- **run-xemu.ps1** = una foto; **xemu122.ps1** (`buzon-xbox\122-trabajo\`) = pipeline completo: compila preset
  `xbox-autotest` con `-DXBOX_AUTOPILOT=ON`, copia `autopilot.txt` al ISO, arranca xemu y captura `shot_NNN.png` cada 5 s.
  Coge el `BUILD.lock`.
- **autopilot** mueve el juego solo: `D:\autopilot.txt` en el ISO. Formato (de `src\xbox\input\XboxPad.cpp`,
  `#if XBOX_AUTOPILOT`): `<start_ms> <buttons|-> <lx> <ly> <rx> <ry>`; botones unidos con `+` (`-`=ninguno); sticks
  -1..1 (ly/ry: -1 = arriba); cada paso se mantiene hasta el siguiente; máx 128 pasos. Para movimiento: `ly -1` anda,
  `rx` gira la cámara.

Luego elegir las 3-4 mejores y copiarlas a `buzon-xbox\web\img\` (3-6.png) y redeployar la web. Ver [[web-y-kofi]].

**Jugar en vivo (24-11):** `xemu-vivo.ps1` (122-trabajo) + `xbox-spike\livepad.py <map> tap A | hold <ms> <bot> lx ly rx ry | off`
escribe `g_livePad` por GDB; mirar con `captura.ps1` entre pasos. Detalle en la guía 129. Las fotos de la web las elige el
usuario (carrusel en `web\img\shots\`); no borrar `img\1.png`/`3.png` sin preguntar.
