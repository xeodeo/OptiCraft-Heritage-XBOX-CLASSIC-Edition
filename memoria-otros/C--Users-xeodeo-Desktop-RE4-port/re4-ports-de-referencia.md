---
name: re4-ports-de-referencia
description: CubeEvil4-Android y re4dc son ports nativos de la misma decompilacion de RE4; el de Android trae 218 parches little-endian sobre nuestro mismo commit base
metadata:
  node_type: memory
  type: reference
  originSessionId: c4712196-64a8-4ad0-989d-fccb2f9260ca
  modified: 2026-11-09T20:46:37.031Z
---

Ports nativos de la decompilacion `adonis-singh/re4` que sirven de referencia para la rama nativa de Xbox ([[re4-estado-y-flujo]]):

- **https://github.com/Andiweli/CubeEvil4-Android** (v0.99.6-rc el 09-11-2026, juego entero jugable). Fijado al commit `9b76ff0` del upstream, que es el mismo en que se basa nuestra rama `agy-port-msvc`. Lo util esta en `port/whole/`: `patches/` (218 parches numerados sobre la decomp: byte order, `0021-psq-in-c` con `CalcSk1_x` en C, `0020-gekko-math-in-c`, `0016/0018` uniones little-endian, GX, CRI, tarjeta) y `runtime/` (aram.cpp, os_link.cpp + module_loader.cpp para los REL compilados nativos, yz2_decode.cpp, gx_fifo/gx_decode). Compila tambien para **i686** (`port/tools/whole_build.sh i686`), o sea x86 de 32 bits little-endian como la Xbox. Estado sesion a sesion en `port/docs/whole-program-status.md` (en aleman).
- **https://github.com/lamb2k/re4dc** (rama `dreamcast-port`, Test 27.1 el 08-11-2026): jugable hasta el castillo en 16 MB de RAM. Referencia para memoria y para convertir assets fuera de linea (`port/dreamcast/`).
- **https://github.com/GabeConway/OpenCrossing-Xbox** (clonado en `referencias/OpenCrossing-Xbox`, encontrado por agy el 09-11-2026): Animal Crossing (decompilacion de GameCube) nativo en la **Xbox clasica de 64 MB**. Lo mas parecido a lo nuestro: `xbox/shaders/gx.vsh` (vertex shader de NV2A que hace transformacion, luces de GX, niebla y texgen), `xbox/src/xbox_nv2a.c` (render y formatos de textura), `docs/memory.md`, `docs/perf.md`, `docs/renderer.md`. Es la referencia para pasar nuestro T&L de CPU a GPU. GPL-2.0-or-later.
- Tambien clonados en `referencias/`: `aurora` (GX sobre WebGPU, cache de display lists), `vgmstream` (DSP-ADPCM y ADX en C), `pl_mpeg` (MPEG-1 en un fichero, para Sofdec). De Melee-X faltan por usar `xbox/src/hw/nv2a_vp.c` (vertex programs GX con PNMTXIDX y 8 luces) y `nv2a_fog.c`.
- En CubeEvil4-Android, ademas de los parches: `port/whole/byte_views.json` (85 sitios "palabra sobre bytes" revisados; aplicado con `re4/tools/aplicar_byte_views.py`) y el parche 0016 (uniones de bytes). Antes de arreglar un fallo de orden de bytes a mano, mirar si ya esta en esos inventarios.
- Otros ports nativos a **Xbox clasica** clonados en `referencias/` (encontrados por agy, 10-11-2026): `Perfect-Dark-X` (https://github.com/GTTeancum/Perfect-Dark-X, rama `port`: Perfect Dark desde decompilacion de N64, pipeline NV2A y texturas) y `sm64-xbox` (https://github.com/fgsfdsfgs/sm64-port, rama `xbox`: doble buffer de vertices write-combining, audio a 48 kHz). Informe con ficheros concretos: `buzon/AGY-herramientas-xbox.md` (sus cifras de rendimiento de la NV2A son de especificacion, sin medir por nosotros).
- El upstream `adonis-singh/re4` no tiene commits posteriores al 02-10-2026 (comprobado 09-11-2026).
- No se encontro ningun port publico de RE4 a Xbox clasica (busqueda 09-11-2026); el usuario cree que alguien lo logro, sin enlace.

Licencia: en ambos, la licencia propia cubre solo scripts, herramientas y docs; el codigo derivado del juego no.
