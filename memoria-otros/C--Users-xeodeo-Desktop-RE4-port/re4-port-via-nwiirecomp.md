---
name: re4-port-via-nwiirecomp
description: "El port de RE4 a PC usa NWiiRecomp (recompilacion estatica del .dol), no la descompilacion de adonis-singh/re4"
metadata:
  node_type: memory
  type: project
  originSessionId: 6b41f0d9-721b-4fb7-a4d2-78d8b7f5f5c7
  modified: 2026-10-01T22:02:51.292Z
---

Desde el 2026-10-01, el port de Resident Evil 4 (GameCube, `G4BE08`) a PC en
`~/Desktop/RE4 port` se hace con **NWiiRecomp**, que traduce el `.dol` a C++
nativo. Se abandonó la vía anterior: portar la descompilación de
`adonis-singh/re4` a x86-32 con Aurora para GX.

**Por qué:** la vía de la descompilación exigía arreglar el endian fichero a
fichero, reescribir 8 unidades de ensamblador PowerPC y resolver 644 marcas
`COMPILER-DIFF` — lo que el propio análisis del usuario (`buzon/ANALISIS-OpenCrossing-vs-RE4.md`)
llamaba "el grueso". NWiiRecomp no necesita el código fuente del juego y lo
evita entero. Por lo mismo, el decodificador TPL hecho a mano
(`src/sdk/tpl_decode.cpp`) dejó de estar en el camino crítico: el runtime trae
rasterizador y generador de shaders propios.

**Estado al cerrar ese día:** `RE4Recomp.exe` compila y arranca; llega al bucle
principal del juego y se atasca en un callback de alarma del DVD. El detalle
técnico vive en `buzon/TRASPASO-recomp.md`.

El objetivo declarado del usuario es modesto a propósito: **"al menos que abra
en PC"**, no que sea jugable. Trabajar contra ese listón.

Ver también [[re4-fuentes-y-herramientas]].
