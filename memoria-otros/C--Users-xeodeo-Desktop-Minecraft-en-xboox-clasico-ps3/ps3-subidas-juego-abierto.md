---
name: ps3-subidas-juego-abierto
description: Nunca subir por FTP ni mandar ordenes a webMAN (install/mount) con un juego o app abiertos en la PS3
metadata:
  node_type: memory
  type: feedback
  originSessionId: ab1d4717-6399-4290-b485-70c441461bca
  modified: 2026-10-29T21:24:51.510Z
---

No subir por FTP `assets.pak` (ni EBOOT) a `/dev_hdd0/game/OPTC00001` mientras el usuario está jugando: el juego lee el pak en vivo y el 29-10-2026 se colgó la consola al sobrescribirlo ("dañaste el juego mientras jugaba").

**Why:** el usuario estaba probando y se le colgó la PS3 a mitad.
**How to apply:** antes de subir, pedir que cierre el juego (o esperar a que lo diga). La PS3 1.5.2 usa el pak de la Xbox 1.5.2 (`ClassicXboxLoader\...\OptiCraft\1.5.2\assets.pak`), no `OptiCraftHeritageEdition\data` (que es 1.2.5). Ver [[ps3-sdk]].

31-10-2026: tambien con XeoStore. Mande `/install.ps3` (instalar un .pkg) a webMAN mientras el usuario jugaba Adventure Time y la consola se colgo al cerrar el juego ("me crasho el ps3... ojo con eso"). Regla ampliada: nada de FTP ni ordenes a webMAN (install, mount, refresh) con un juego o app abiertos. Comprobar antes y, si hay duda, preguntar.

18-11-2026: subi el EBOOT de XeoStore CON LA TIENDA ABIERTA (descargando). cpursx.ps3 devolvio VACIO (webMAN no
contesto) y lo lei como "libre". Comprobacion correcta: exigir HTTP 200 de cpursx Y que ademas el usuario diga que
esta en el XMB; si cpursx falla o viene vacio = NO SUBIR. Mirar tambien la hora del xeostore_log.txt (si es de hace
segundos, la tienda esta abierta).
