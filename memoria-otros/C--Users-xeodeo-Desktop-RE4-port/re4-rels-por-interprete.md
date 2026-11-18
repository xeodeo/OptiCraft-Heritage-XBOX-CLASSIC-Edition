---
name: re4-rels-por-interprete
description: "En RE4 la camara, la luz, el jugador, las armas y los enemigos son RELs que corren por el interprete de nWiiRuntime, no recompilados"
metadata:
  node_type: memory
  type: project
  originSessionId: 4fce6649-f59d-4e48-8a47-967ea8c99d30
  modified: 2026-10-05T03:39:11.327Z
---

Los módulos `t_camera`, `t_light`, `pl*` (jugador), `wep*` (armas), `em*`
(enemigos) y `st*` (fases) de RE4 son RELs. NWiiRecomp solo traduce el `.dol`;
todo eso lo ejecuta `nWiiRuntime/src/hle/interpreter.cpp`. El recompilador
además delega ahí algunas instrucciones del propio `.dol` (`lfsu`, `stfsu`) vía
`micro_interpret`.

**Por qué importa:** síntomas de cámara, iluminación, armas o enemigos apuntan
primero al intérprete, que era mucho más incompleto que el recompilador. El
2026-10-04 le faltaban las operaciones lógicas de CR (`cror` cae en silencio,
y CodeWarrior lo usa para todo `>=`/`<=` entre floats), `lwzux` y familia, y
el llenado de ps1.

**Otro hallazgo de ese día:** RE4 activa la caché bloqueada (`LCEnable` en
`systemStartInit`) y `MakeWeightPalette` escribe la paleta del skinning en
`0xE000xxxx`. La MMU no mapeaba esa zona.

**Cómo aplicarlo:** ante un fallo de lógica de juego, comparar primero el
intérprete con el recompilador para esa instrucción. Ver [[re4-port-via-nwiirecomp]].
