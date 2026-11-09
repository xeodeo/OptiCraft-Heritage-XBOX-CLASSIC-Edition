---
name: anyxboxclasico
description: "AnyXboxClasico (Desktop\\AnyXboxClasico) — herramienta universal y AUTOMÁTICA para jugar cualquier juego de Xbox clásica en PC sin emulador, sobre xboxrecomp"
metadata:
  node_type: memory
  type: project
  originSessionId: 61393428-185c-4352-893f-714f761ccd9c
  modified: 2026-10-16T01:05:23.721Z
---

Proyecto AnyXboxClasico en `Desktop\AnyXboxClasico` (empezado 15-10-2026): convertir juegos de Xbox clásica en .exe de PC sin emulador, usando el toolkit xboxrecomp (clonado dentro) + nuestras herramientas (`anyx.py` con barras de progreso, `tools/lib_match.py` que nombra funciones con los .lib del XDK filtrado 5849). Juego de prueba: The Suffering (`Desktop\ojala`). Estado y pasos en su README.md.

Hito 15-10-2026: The Suffering llega al menú principal en PC (vídeo + menú en español) con puentes D3D por nombre XDK (runtime/anyx_*.c, tools/gen_puentes.py) y XbSymbolDatabase (MIT, Cxbx) como segunda fuente de símbolos. Cxbx-Reloaded es GPL: solo como referencia.

**Why:** el usuario quiere una herramienta UNIVERSAL: que después, con cualquier juego, funcione sin intervención humana.

**How to apply:** nunca arreglar con parches por dirección de un juego concreto; hacer reglas por NOMBRE de función XDK (las identifica lib_match) que se apliquen solas a cualquier XBE, y meter todo paso nuevo en `anyx.py`. The Suffering es solo el banco de pruebas.

**Cambio de motor 06-11-2026:** el usuario eligió XboxXBERecomp (github TGP482, MIT: D3D12, XAudio2, SDL3, kernel completo) en vez de xboxrecomp + nuestro xdk_hle. Proyecto `AnyXboxClasico\suffering12` (compilar.bat, clang de LLVM por winget). XDK 5788 = copia de su xdk/5849 (layout comprobado). Llega al menú. Detalle en `buzon\04-PLAN-xbexrecomp.md`. lib_match se equivoca con envoltorios DSOUND casi iguales: sus firmas mandan.
