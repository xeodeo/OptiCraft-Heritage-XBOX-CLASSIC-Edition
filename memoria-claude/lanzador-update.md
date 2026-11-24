---
name: lanzador-update
description: El lanzador se actualiza solo por internet (botón Update) desde la release "ultima" de GitHub; rama wifi-instalar
metadata:
  type: project
---

24-11-2026: el lanzador (default.xbe) tiene botón **Update** junto a Play y mira solo si hay build nueva ("New build!"),
como la Classic Xbox Store. Baja archivos sueltos (no ISO, no zip) de la release `ultima` del repo xbox
(manifest.txt: ruta tamaño sha256 asset), comprueba SHA-256, reemplaza al final, guarda `D:\instalado.txt`.
Extra: página web en el puerto 80 de la consola. Rama `wifi-instalar` (wt-wifi), subida a xbox. Detalle y pendientes:
`buzon-xbox\130-INSTALAR-POR-WIFI.md`. Al usuario le ENCANTA el Update; el celular es secundario.

**How to apply:** falta crear/subir la release `ultima` (pedir permiso; no hay gh) con `scripts-claude\montar_ultima.py`
y probar en la consola real. Cada build pública nueva → volver a montar y subir `ultima`. Ver [[repos-3ds-referencia]].
