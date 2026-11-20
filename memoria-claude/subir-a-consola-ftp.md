---
name: subir-a-consola-ftp
description: "Subir builds a la Xbox real por FTP: 192.168.0.3 xbox:xbox, juego en F:\\Games\\OptiCraft\\<versión>"
metadata:
  node_type: memory
  type: reference
  originSessionId: 5081e937-c767-46da-b247-a4e0f6c8024e
  modified: 2026-11-19T18:12:42.886Z
---

«Súbelas a la consola» = FTP a la Xbox: `curl --user xbox:xbox -T <fichero> ftp://192.168.0.3/F/Games/OptiCraft/<1.2.5|1.5.2>/<fichero>`.
Misma estructura que `OptiCraft_extracted\OptiCraft\` del lanzador ([[xbox-loader-copy]]). Comparar tamaños antes:
el 19-11 la consola tenía también el `assets.pak` viejo, no solo los XBE. Comprobar tamaños después de subir.
El FTP solo responde con el dashboard abierto (al abrir el juego se cierra).
