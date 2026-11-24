---
name: lanzador-update
description: El lanzador se actualiza solo por internet (botón Update) desde la release "ultima" de GitHub; rama wifi-instalar
metadata:
  type: project
---

24-11-2026: el lanzador (default.xbe) tiene botón **Update** junto a Play y mira solo si hay build nueva ("New build!"),
como la Classic Xbox Store. Baja archivos sueltos (no ISO, no zip) de la release `ultima` del repo xbox
(manifest.txt: ruta tamaño sha256 asset), comprueba SHA-256, reemplaza al final, guarda `D:\instalado.txt`.
Sin nada instalado lista las versiones de la release como "<ver> (get)" y Play pasa a Download (probado en xemu). La página del celular está APAGADA (XBOX_LAUNCHER_WEB=0): el usuario no quiere celular. Rama `wifi-instalar` (wt-wifi), subida a xbox. Detalle y pendientes:
`buzon-xbox\130-INSTALAR-POR-WIFI.md`. Al usuario le ENCANTA el Update; el celular es secundario.

**How to apply:** falta crear/subir la release `ultima` (pedir permiso; no hay gh) con `scripts-claude\montar_ultima.py`
y probar en la consola real. Cada build pública nueva → volver a montar y subir `ultima`. Ver [[repos-3ds-referencia]].

Baja todo lo de la carpeta de la versión (4 XBE + assets.pak + texturepacks...), ~100 MB la 1.5.2 la primera vez; Update solo lo que cambió. Pendientes en buzon 130 § Pendientes.
