---
name: sesiones-cortas-y-sin-vigilantes
description: El usuario anda justo de cupo; sesiones cortas con relevo escrito. Vigilantes de agentes solo cuando él los pide (los pidió el 12-11-2026)
metadata:
  node_type: memory
  type: feedback
  originSessionId: 5db564a0-44d0-4280-8165-6c16f84c666c
  modified: 2026-11-12T21:43:14.895Z
---

El 11-11-2026 el usuario dijo que una sesión larga de RE4 Xbox le gastó el 50 % del cupo "de un momento" y pidió dejar anotado el estado para poder relevar a Claude. El 12-11-2026, con tres agentes trabajando a la vez, pidió lo contrario para ese rato: "abre los escuchadores de agy, no los tenemos, y Codex también".

**Why:** una conversación de días arrastra todo el contexto en cada turno, y cada aviso de un vigilante despierta a Claude con ese contexto entero. Pero cuando hay agentes en marcha y él está delante probando, prefiere enterarse de las entregas sin tener que avisar.

**How to apply:** (interpretación mía) empezar cada bloque en una sesión nueva leyendo solo el "ARRANQUE RÁPIDO" de `buzon/COMO-VAMOS.md` y actualizarlo al cerrar; cuando la sesión ya es muy larga, proponerle abrir otra. Vigilantes: ponerlos si él los pide o si está presente esperando entregas; no dejarlos armados al cerrar. Los avisos `PARADO` y `AVANCE` del escuchador suelen ser falsas alarmas (agy esperando a un subagente, o la línea fija de Codex): contestar en una línea. Con Codex, usar `--informe` y una `--hecho` que no aparezca en el texto del encargo (la frase de señal sale en pantalla al mandarlo). Leer logs e informes con grep, no enteros. Ver [[re4-estado-y-flujo]] y [[codex-en-orca]].
