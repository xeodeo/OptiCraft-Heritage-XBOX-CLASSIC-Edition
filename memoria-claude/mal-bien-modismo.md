---
name: mal-bien-modismo
description: "Cuando el usuario escribe \"mal bien\" o \"X bien\" al final de un bug, suele ser CONFIRMACION de que ya está bien (no un fallo). Ante la duda preguntar antes de cambiar código."
metadata:
  node_type: memory
  type: feedback
  originSessionId: 808de2cd-0176-4508-a594-554d8b6d7609
  modified: 2026-11-27T17:40:52.270Z
---

Cuando el usuario reporta una prueba y termina con "mal bien", "está bien", "ya bien" u otra variante pegada, es muy posible que esté CONFIRMANDO que la cosa YA ESTÁ BIEN, no reportando un fallo. Lo mismo con "ya me funciona bien", "lo veo bien", etc.

**Why:** 27-11-2026 el usuario escribió "Pigman bebé: la espada flota a tamaño de adulto y la caja de golpes está mal bien" y "s" (confirmación). Lo interpreté como FALLO y empecé a cambiar el traductor ProtocolTranslator340 añadiendo EntityKind::Zombie/PigZombie, cuando en realidad el bebé pigman YA FUNCIONABA. Hubo que revertir todo y el usuario dijo "ojo con dañarlo".

**How to apply:** si un reporte del usuario termina ambiguo con "mal bien" o similar, releer el contexto completo (puede ir precedido de "está bien" o seguido de "s" confirmando) y ANTE LA DUDA PREGUNTAR antes de tocar código. El coste de preguntar es bajo, el de romper algo ya funcionando es alto. Especialmente cuando se trata de código que funciona en pruebas previas (ver `_meta.json` del buzón-xbox/estado/ para el estado de las pruebas en consola).
