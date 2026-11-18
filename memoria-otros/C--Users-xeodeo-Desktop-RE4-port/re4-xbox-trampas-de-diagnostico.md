---
name: re4-xbox-trampas-de-diagnostico
description: "RE4 Xbox nativo - trampas que costaron horas el 10-11-2026 (recompilar por contenido, cull, biseccion visual)"
metadata:
  node_type: memory
  type: feedback
  originSessionId: c4712196-64a8-4ad0-989d-fccb2f9260ca
  modified: 2026-11-10T18:15:32.686Z
---

Tres lecciones del port nativo de RE4 a Xbox (sesion del 10-11-2026):

1. `xbox-nativo/scripts/recompilar_juego.sh <patron>` busca el patron en el CONTENIDO de los .cpp, no en el
   nombre. `'game/shape.cpp'` no recompila shape.cpp. Usar el nombre de una funcion del fichero y comprobar
   la fecha del `.o` en `build/game_objs` antes de lanzar xemu.
2. Un fallo visual que "parece texturas o huesos" (bandas oscuras en caras, piezas rotas, suelo que falta,
   texto que no sale) puede ser el criterio de cara oculta: `GX_CULL_FRONT`(1) = `D3DCULL_CW`,
   `GX_CULL_BACK`(2) = `D3DCULL_CCW`. Mirar una escena donde el personaje mira de frente a camara.
3. Cuando dos hipotesis seguidas fallan, dejar de leer codigo y bisecar en imagen:
   `xbox-nativo/scripts/biseccion_fotos.py` + bloque `BISECCION-TEMP` de `src/gxd/gx_d3d.cpp` (apaga una clase
   de material por turnos y la marca en pantalla).

**Why:** el usuario vio "igualito" cinco veces mientras se probaban hipotesis por lectura; la biseccion y una
captura suya resolvieron en dos pruebas lo que no salio en diez.

**How to apply:** en fallos de render de este port, primero aislar en imagen que dibujos lo causan; no
afirmar "encontrado" al usuario hasta ver la foto corregida. Relacionado: [[insertar-codigo-sin-heredoc]],
[[re4-ports-de-referencia]].

- **El audio se juzga en la consola real, no en xemu** (11-11-2026): con el AX + DirectSound de agy, en la Xbox del usuario SÍ suena y en su xemu no se oye nada. No "arreglar" el audio por el silencio de xemu. La consola está en 192.168.0.3 (FTP `xbox:xbox`, carpeta `F:\Games\RE4 Nativo`; el FTP se cierra con el juego abierto) y el log llega por UDP 9999 al PC (`RE4_NETLOG=1`, `scripts/subir_xbox.py`, `scripts/comparar_xbox.py`).
