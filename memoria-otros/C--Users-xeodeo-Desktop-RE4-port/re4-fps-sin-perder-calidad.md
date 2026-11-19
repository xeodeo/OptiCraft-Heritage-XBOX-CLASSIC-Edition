---
name: re4-fps-sin-perder-calidad
description: "En RE4 Xbox el usuario quiere fps sin perder calidad de imagen y que la optimización la haga Claude, no agy"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 5db564a0-44d0-4280-8165-6c16f84c666c
  modified: 2026-11-19T02:54:57.374Z
---

El 12-11-2026 el usuario dijo que no quiere perder calidad para ganar fps: nada de recorte "estilo PS2". La distancia de dibujado corta no es idea suya (sale del repo re4dc); queda como último recurso, conservadora y apagable. También pidió que la optimización la haga Claude directamente si puede hacerlo mejor que agy, midiendo en qué se va el tiempo y mirando otros ports de Xbox como referencia.

**Why:** tras ver la subida de fps con el build final y la ronda 3 de huesos, prefiere seguir por ahí (CPU, caché, depuración fuera) antes que tocar la imagen. Los informes de agy dan cifras estimadas como hechas (interpretación mía, tras revisar la ronda 3).

**How to apply:** proponer primero mejoras que no cambian lo que se ve; si algo recorta imagen, decirlo antes y dejarlo con interruptor. Mantener siempre dos builds de consola: depuración y final (`RE4_FINAL`), ver [[re4-estado-y-flujo]]. El xemu no sirve para medir tiempos: solo la consola ([[re4-xbox-trampas-de-diagnostico]]).

**Actualización 18-11-2026:** el usuario rechazó de nuevo reducir la distancia de dibujo ("si GameCube podría, aquí se
puede") y probar assets de PS2; tampoco quiere que una sala vaya mal al entrar y bien después (lo que libere memoria va
en la carga). Pidió explícitamente usar agy, Codex y subagentes a la vez cuando hay cupo.

**Actualización 13-11-2026:** el usuario sí quiere delegar en agy los trabajos grandes de rendimiento (dijo que agy tiene modelo nuevo por un rato), pero con un encargo muy bien escrito y tras el análisis de Claude; Claude reparte el trabajo y escucha con el escuchador. Antes de lanzar, Claude debe decirle qué falta probar y qué dudas tiene. Interpretación mía: "optimiza Claude, no agy" queda como "Claude diagnostica y decide; agy ejecuta piezas grandes bien acotadas".
