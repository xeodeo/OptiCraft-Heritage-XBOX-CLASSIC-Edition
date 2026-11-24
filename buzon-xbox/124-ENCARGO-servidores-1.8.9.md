# 124. Encargo: servidores 1.8.9 (protocolo 47) además de 1.12.2 (23-11)

PRIORIDAD: ver sección «PRIMERO» al final.

Tras el 121 (traductor 1.2.5 ↔ 1.12.2, `ProtocolTranslator340`, probado en consola real el 23-11: entra al SMP
minecraft3ds.org en Auto, memoria justa ~10 MB libres, radio de chunks baja a 3). Falta 1.8.9:
- Añadir el traductor 1.8.9 de erluza (`erluza/main`: `ProtocolTranslator189*`, commits 4fd75801 / 65c980c1 y
  posteriores) o un modo 47 dentro del de 340 (comparten mucho: 1.8 ya tiene VarInt, compresión, chunks distintos).
- «Server version»: Auto / 1.2.5 / 1.8.9 / 1.12.2. Auto: elegir por el protocolo que responde el ping (47 → 1.8.9,
  340 → 1.12.2, otros modernos → 340 si el servidor tiene ViaVersion).
- Con «1.2.5» y un nombre sin puerto, si el puerto 25565 no responde, probar el SRV (hoy falla con minecraft3ds.org).
- Memoria: bajar el gasto en el SMP (spawn grande): menos copias, columnas comprimidas, radio mínimo 3.
- Probar con un Paper/Spigot 1.8.9 local offline (`servidor-1.8.9\`) y el SMP.
Reglas como el 121. Informe `124-INFORME.md`; señal `CIENTOVEINTICUATRO-LISTO`.

## PRIMERO (consola real 23-11 tarde): cuelgue al MORIR en el SMP 1.12.2
Log: GuiGameOver → GuiDownloadTerrain → … → GuiDownloadTerrain (dos cambios de dimensión al reaparecer) y luego
`crash c0000005 write 00000000` en `RtlpCoalesceFreeBlocks` ← `HeapFree` ← `~DataWatcher` ← `~Entity` ←
`World::updateEntities` (mapa exp, build 1.2.5 a5ee300). Heap corrompido antes: sospecha doble borrado del jugador
viejo / entidades entre `changeWorld1` seguidos (`NetClientHandler::handleRespawn` crea WorldClient nuevo por cada
cambio de dimensión y `Minecraft::respawn` hace setEntityDead del jugador viejo en el mundo nuevo). Reproducir con el
servidor falso de repetición o el Paper 1.12.2 local (morir, y respawn con cambio de dimensión doble) y arreglar en
las dos versiones (la 1.5.2 comparte el código de respawn MP).
