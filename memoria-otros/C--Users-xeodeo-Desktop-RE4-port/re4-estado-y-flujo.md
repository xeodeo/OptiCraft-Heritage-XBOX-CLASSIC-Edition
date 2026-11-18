---
name: re4-estado-y-flujo
description: "Estado del port de RE4 al 2026-11-12 (se juega en la Xbox real, dos versiones instaladas) y como trabaja el usuario: el prueba en la consola, Claude compila y sube"
metadata:
  node_type: memory
  type: project
  originSessionId: 5db564a0-44d0-4280-8165-6c16f84c666c
  modified: 2026-11-12T19:18:07.990Z
---

Al 2026-11-12 el trabajo vivo es la rama nativa de Xbox clásica (`xbox-nativo` + `re4`, ramas `xbox-formatos`): RE4 se juega entero en la consola real con audio. El estado, lo pendiente de probar y los fallos abiertos están en el RELEVO de arriba de `buzon/COMO-VAMOS.md`; el cómo (compilar, subir, medir) está en la skill `re4-xbox`. La rama de recompilación para PC (`nwiirecomp`, `RE4Recomp.exe`, `buzon/TRASPASO-recomp.md`) quedó parada desde octubre.

En la consola hay dos versiones instaladas: `RE4 Nativo` (final, sin depuración) y `RE4 Nativo Debug` (textos, barras, log por red).

**Flujo con el usuario:** él prueba jugando en la consola y cuenta lo que ve; Claude compila, sube por FTP y lee el log. El FTP se cierra con un juego abierto: hay que pedirle que salga al dashboard. Suele pedir "deja todo anotado e hiberna el PC" al cerrar.

**Why:** lo pidió así ("yo hago las pruebas") y va justo de cupo, ver [[sesiones-cortas-y-sin-vigilantes]].

**How to apply:** empezar siempre por el RELEVO de `COMO-VAMOS.md`; no abrir ventanas encima mientras juega; para un fallo visto en la versión final, pedirle que lo repita en Debug (la final casi no manda log). Preferencias de optimización en [[re4-fps-sin-perder-calidad]].

**RE4 GC no tiene delta de tiempo**: el juego va a 2 refrescos por fotograma (30 fps de tope); si bajan los fps se ralentiza, y forzar 60 acelera la lógica x2.

Referencia externa: Ayano hizo un port nativo de RE4 GC partiendo de NWiiRecomp (post en r/decomps); otros ports en [[re4-ports-de-referencia]].
