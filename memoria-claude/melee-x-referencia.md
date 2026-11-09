---
name: melee-x-referencia
description: "Melee-X (RE4 port\\referencias\\Melee-X) es la referencia obligada para el port de RE4 a Xbox; revisarla antes de diseñar GX, memoria, ARAM, hilos o endianness"
metadata:
  node_type: memory
  type: reference
  originSessionId: 88790f8e-9eb3-4e3c-98f2-f72e1258341b
  modified: 2026-10-08T18:10:45.393Z
---

`C:\Users\xeodeo\Desktop\RE4 port\referencias\Melee-X`: port nativo y terminado de Melee
(decompilación de GameCube) a la Xbox clásica. El usuario pidió (08-10-2026) revisarlo siempre
"para no olvidar". Empezar por su `CLAUDE.md` y `docs/decisions.md`, `docs/renderer.md`,
`docs/architecture.md` (presupuesto de memoria).

Lo que nos sirve en [[re4-xbox-port]]:
- GX propio sobre la NV2A: `xbox/src/sdk/gx/gx_state.c`, `gx_vtx.c`, `gx_tex.c` (conversión de texturas,
  con tests), `xbox/src/hw/nv2a_rc.c` (TEV → register combiners), `nv2a_vp.c` (vertex programs
  generados: iluminación GX, skinning por índice de matriz, texgen), `nv2a_fog.c`.
- Endianness: igual que nosotros (structs de disco big-endian en memoria, `DISC_STRUCT`/`DISC_PTR`);
  ellos reescriben accesos con `tools/lower/disc_lower` (clang). Nosotros usamos `BE<T>`.
- `OSDisableInterrupts` como lock recursivo; alarmas en el hilo del juego; frame en `GXCopyDisp` +
  `VIWaitForRetrace`.
- ARAM respaldada por el disco (`xbox/src/sdk/ar.c`), memoria tamaño GameCube comprometida bajo demanda,
  consolas de 128 MB forzadas a 64 MB.
- Menos draws = más rendimiento (juntar lotes de display lists, quads → listas de triángulos).
