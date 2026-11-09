# Estado del port 1.2.5 -> 1.5.2 (21-10, 13:25) — empezar por aquí

## Ramas
- `port152-base` (worktree `wt-152-codex`, último `Textures: 1.5.2 language texts in the pak`):
  setBlock 1.5.2 (40), hooks de Block (42), texturas+pak (43), lanzador por carpetas, D: = carpeta del XBE,
  atlas arreglado (todas las casillas 1.2.5 con su icono; cofre = tablones; TODO cabe en 256), idiomas 1.5.2.
- `port152-inventario` (worktree `wt-152-agy`, encima de base `222d1ab` + `a233860`):
  docs de inventario (41), grupo 1 sencillos (44), iconos reales (Claude), grupo 1 con forma (46), fix cofre de Ender.
  OJO: los idiomas (último commit de base) aún NO están rebasados en esta rama; no hace falta para compilar.
- `xbox-opti-30` (wt-opti30) también tiene el lanzador por carpetas y el arreglo de D: (cherry-pick).

## Regenerar el pak (desde wt-152-codex)
`python -I scripts/port152/build_textures_152.py --repo . --base-data "...\OptiCraftHeritageEdition\data" --source "...\mcp-diff\src152" --output-data build/xbox-release/port152-data --output-pak build/xbox-release/port152-output/assets.pak --report-json build/xbox-release/port152-output/report.json --preview buzon-xbox/47-atlas.png`
Si cambia `Texture152Atlas.h`, hay que recompilar los XBE y copiar pak+XBE JUNTOS (si no, texturas cruzadas).

## En la consola (ClassicXboxLoader\...\OptiCraft\)
- `1.2.5\`: rama buena, solo assets.pak (la data suelta está en OptiCraft_quitado_21-10).
- `1.5.2\`: XBE de wt-152-agy (13:17, con fix del cofre de Ender) + pak con idiomas (13:19).

## Pendiente
1. Cofre de Ender: SÍ tiene textura (Claude lo entendió mal). Solo fallaba al abrirlo: cuelgue (arreglado, `a233860`)
   y título "container.enderchest" (arreglado con los idiomas 1.5.2 del pak). Falta que el usuario lo confirme.
2. Parón de ~7 s (watchdog main stuck frameBegin) y bajón con mucho fuego: medir con el log.
3. Grupo 2 (redstone/raíles) para agy (modelo Gemini 3.8 Flash; Argon sin cuota hasta ~28-10; Codex hasta 19-11).
4. El escuchador del log está APAGADO (Claude Code lo mató por memoria baja): arrancarlo cuando el usuario juegue.

## Actualización 21-10 ~17:00 — redstone/pistones
- Log de consola (reloj+pistones): 60 fps, 20 tps, redstone <3 ms; el fallo era el DIBUJADO: 30 secciones en cola y
  1 publicada en 5 s. Arreglos en xbox-opti-30 y port152-base:
  `6a1de35`/`d8855bf` markDirty ya no reinicia la sección en montaje (coalesce, como markDirtyFromLighting);
  `93a3498` pistón en movimiento a RenderGlobal al instante (onMovingBlockTileEntity) + carril urgente Xbox
  (16 bloques, 6 ms/frame, quita el staging a una build de fondo con abandonTerrainBuild). Después: 58 secciones/5 s.
- 1.5.2: el cable de agy avisaba a los vecinos al cambiar fuerza (quitó el editingBlocks de 1.2.5); ahora flags 2
  como 1.5.2 (`ce3de78`, también la placa de presión). Sin probar en consola.
- PENDIENTE: en 1.2.5 el pistón pegajoso solo se ve bien, pero el BLOQUE empujado/recogido no se ve mientras se mueve
  y "se queda duro". wt-opti30 tiene SIN COMMIT unos logs `xbox.piston` (add/drop/done) en World.cpp, RenderGlobal.cpp
  y TileEntityPiston.cpp; esa build está en OptiCraft_actualizar\1.2.5. Falta que el usuario pruebe y leer el log
  (filtrar `xbox.piston`). Al acabar, quitar esos logs (git checkout) o dejarlos como MC_LOG_DEBUG.
- `OptiCraft_actualizar\` = solo lo cambiado (ver memoria xbox-loader-copy).
