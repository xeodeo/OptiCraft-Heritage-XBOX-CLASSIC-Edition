---
name: bot-minero
description: Bot de minería "Minero" del modo admin (AdminMiner): dónde está, cómo se prueba y qué falta (poner bloques)
metadata:
  type: project
---

Desde 26-11-2026 el modo admin tiene **Minero** (`src/mods/adminmode/AdminMiner.cpp`, agy lo empezó en el encargo 133 y
se quedó sin cuota; Claude lo terminó y probó). Integrado SIN COMMIT en `wt-build-152`; copia en `wt-agy-152`.
Probado en xemu contra el Paper 1.12.2 local con builds de prueba: `D:\minero.txt` enciende mod y Minero (hierro);
28 y 39 menas en 2 min. Pedidos del usuario ya hechos: comer solo, pico más barato primero, salir de corrientes de
agua, no quedarse mirando menas con lava. Siguiente: poner bloques (tapar lava/agua, scaffold) con `sendPlaceBlock` del
módulo Build; plan y lo leído de Baritone en `buzon-xbox\134-PLAN-minero-poner-bloques.md`.

**How to apply:** al probar en xemu, el jugador de pruebas puede estar muerto → solo A para reaparecer (NO arriba+A,
que sale al menú); no teletransportar a una altura fija (se asfixia): usar `spreadplayers`. Ver [[repos-3ds-referencia]].
