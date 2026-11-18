---
name: ps3-sdk-sony-filtrado
description: Al usuario no le importa usar el SDK de Sony filtrado en el port PS3 si con eso se ganan fps
metadata:
  node_type: memory
  type: user
  originSessionId: ab1d4717-6399-4290-b485-70c441461bca
  modified: 2026-10-30T03:33:37.879Z
---

El usuario dijo (29-10-2026) que no le importa usar el SDK de Sony filtrado (libs, SPURS, cabeceras) en el port de PS3 si así se ganan fps.

**Why:** su objetivo es exprimir la PS3 (60 fps estables, "como God of War 3"); el origen del SDK no le preocupa.

**How to apply:** se puede considerar para las SPU (SPURS/libspurs de Sony) donde dé ventaja real. Dos frenos técnicos ya conocidos, ver [[ps3-sdk]]:
- el gcc/SNC de Sony no entiende C++11;
- su ABI usa punteros de 4 bytes y no enlaza con el gcc 13 + PSL1GHT del juego.

Lo que se pueda hacer con PSL1GHT (spu-gcc 9.5, sysSpu*, DMA) va por ahí. Sony solo donde PSL1GHT no llegue y enlace de verdad.
