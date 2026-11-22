---
name: re4-melee-x-referencia
description: "Ports hermanos clonados en RE4 port\\referencias (Prime-X, Melee-X, PuebloCTR): mirarlos ANTES de inventar una optimización; el coste real es la traducción GX->D3D por dibujo"
metadata:
  node_type: memory
  type: reference
  originSessionId: 2ceedec2-9572-473e-b537-b76605d14e76
  modified: 2026-11-22T13:52:11.466Z
---

En `C:\Users\xeodeo\Desktop\RE4 port\referencias\`:
- **Prime-X** (TechProGabe, Metroid Prime en Xbox clásica, 40-50 fps a 480i): `docs\performance.md` (caché de listas
  que no cabe, ARAM al disco `aram.swp`, de 4.000 a 400 dibujos, memcpy pequeños, PGO).
- **Melee-X** (mismo autor; ya estaba clonado desde el 04-10, su parte de rendimiento no se aplicó hasta el 18-11):
  `docs\fps-plan.md`, `renderer.md` (grupos sucios de estado, ~31 µs por dibujo en la Xbox), `pgo.md`.
- **PuebloCTR** (RE4 en New 3DS, 25-30 fps, MISMA descompilación vía CubeEvil4): 37 parches de juego
  `overlay\port\whole\patches\03*.patch` casi todos "mismos resultados" (part-cull, matrices, colisiones, trig, GX).

Ya aplicado el 18-11 [[re4-estado-y-flujo]]: caché de listas Prime-X, anillo de VB, ARAM al disco, texturas 512 +
reciclado, caché de programas en D:, PGO con clang SIN LTO (con LTO el pueblo peta). Pendiente: menos dibujos (~1.200/f),
estado por dibujo más barato, parches de PuebloCTR. El juego en sí cuesta < 3 % de la CPU.

Revisión 22-11-2026: PuebloCTR añadió 0338 (oclusión de modelos) y 0339 (sprites de efectos en un dibujo), sin portar
aún. No existen más ports GC -> Xbox clásica que los de TechProGabe; Nightjar (RE4 PC/móvil) no aporta. Melee-X reescribió
su historia. Otras consolas (22-11): Vita (Rinnegatamante `re4-vita`) repo vacío, sin build; DS (kiki-macoto) es un
demake desde la versión Zeebo, no el código; RE4Wii-Android (PotatoRecomp) recompila la versión Wii y gana fps bajando
la resolución, la distancia y los efectos; PSP/PS3/Wii U/Switch: nada. Hacer `git fetch` y leer con `git show @{u}:` antes de hacer pull. Detalle en COMO-VAMOS (22-11).

Ojo (el usuario lo señaló): Melee-X ya estaba clonado y nadie lo usó para rendimiento. Antes de proponer algo "nuevo",
buscar en `referencias\`. XboxXBERecomp (TGP482) es Xbox -> PC: no sirve.
