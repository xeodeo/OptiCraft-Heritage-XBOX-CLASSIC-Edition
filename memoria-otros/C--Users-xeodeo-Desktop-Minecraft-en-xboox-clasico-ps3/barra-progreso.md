---
name: barra-progreso
description: "En toda tarea larga (compilar, subir, construir toolchain) mostrar siempre una barra de progreso al usuario"
metadata:
  node_type: memory
  type: feedback
  originSessionId: ab1d4717-6399-4290-b485-70c441461bca
  modified: 2026-10-29T14:57:36.340Z
---

Siempre que algo tarde (compilaciones, toolchain, subidas por FTP, builds del juego), mostrar una barra de progreso `[#####.....] 50% fase` y actualizarla.

**Why:** el usuario lo pidió el 29-10-2026 ("agrega barra de progreso siempre") mientras esperaba la construcción de gcc 13 + PSL1GHT sin saber cuánto faltaba.

**How to apply:** usar un Monitor con un script que imprima una línea con la barra cuando cambie la fase o el %. Ejemplo: `ps3\tools\progreso_toolchain.sh`. Se calcula por los ficheros que van apareciendo o por los objetos compilados sobre el total. Ver [[respuestas-cortas-espanol]].
