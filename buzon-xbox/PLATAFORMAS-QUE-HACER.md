# Qué hay que rehacer en cada consola (22-11-2026)

Todo el juego es el mismo C++ para todas las plataformas. Lo de jugabilidad (1.5.2, mods, arreglos) sirve en todas;
lo que toca **red, mando, dibujado, memoria y almacenamiento** hay que hacerlo para cada consola.
Página de estado: `ESTADO-OPTICRAFT.html`. Ramas: `ESTADO-ACTUAL.md`.

## Xbox clásica (64 MB, 733 MHz, D3D8) — la principal
- **Ramas buenas**: 1.2.5 `opti30-erluza` (`wt-opti30`), 1.5.2 `port152-erluza` (`wt-152-rend`) + integración
  `port152-int` (1.5.2 con los mods). Todo se compila en `wt-build-152` / `wt-build-125` con `build88.ps1`.
- **Hecho y en consola**: 1.5.2 (plan 69 filas 1–9 y casi todo lo demás), mods (catálogo, minimapa Rei+Xaero, X-Ray,
  luces dinámicas, Herobrine de tkelly, ChocoCraft, localizadores, TMI, info de servidores), skins por usuario
  (HTTP plano a minotar + DNS de respaldo 8.8.8.8/1.1.1.1 + HTTPS relajado con mbedTLS, como la Classic Xbox Store), texturas `.zip` y conversor de packs en el PC.
- **En marcha**: superplano/marcador/combate (105), bloques/mobs/inventario/pantalla (108), skins con relieve 3D
  como la 1.8 (107). **Falta**: Nether con cuevas de 1.5.2 (fila 16), portal del End 4 capas por pasada (idea),
  historial de skins (solo con programa en el PC). Después: 1.6.4 (ya descompilada con su OptiFine) y 1.7.10 (techo).

## PS2 (32 MB, EE 294 MHz, GS 4 MB VRAM) — informe completo: `106-INFORME-agy-auditoria-ps2.md`
- **Compila**: las dos versiones enlazan para PS2 con 2 arreglos triviales (ramas `opti30-ps2` `15baefb`,
  `port152-ps2` `59a05b5`). ELF 1.2.5 6,76 MB, 1.5.2 7,54 MB.
- **Viable**: 1.2.5 con margen (~6–8 MB libres jugando); 1.5.2 sí, con recortes (~4,5–6,5 MB libres).
- **Rehacer para PS2** (pasos del informe, en orden):
  1. Arranque limpio 1.2.5: icono de guardado en `data/`, `ps2_draw_2d_flush_pending()` antes del personaje
     animado, `clearEntityReferences` al salir el J2 en `Ps2SplitScreen.cpp`.
  2. Iconos/pistas del DualShock en los menús nuevos (catálogo, minimapa, Herobrine, chocobo) y combos del minimapa.
  3. Minimapa sin el pico de 384 KB (`ps2_palettize_ct16`): sub-imagen / CT16 y refresco cada ≥10 ticks; luces
     dinámicas a 6 fuentes cada 6 ticks.
  4. X-Ray en el mallado PS2 (`Ps2GreedyMesh`, `WorldRendererPs2`).
  5. Texturas `.zip` en USB con tope 16x (256 px), miniaturas pequeñas.
  6. Skins: liberar texturas de la rueda (VRAM, 256 texturas máx.), guardar en USB; red: `httpGetUrl` en
     `JavaNetwork_ps2.cpp` + DNS de respaldo, solo desde el menú principal. HTTPS: mbedTLS ya existe para PS2
     (fork ramide1); traer `XboxTls` como `Ps2Tls` sobre `Ps2Socket` (RAM: ~60 KB por conexión).
  7. 1.5.2: revisar los 27 bloques nuevos en el mallado/VU1 y aplicar recortes de RAM; medir en PCSX2 y PS2 real.
- **No viable en PS2**: 720p, packs 32x o más, autoguardado cada minuto en Memory Card (congela), pantalla partida
  en mundos infinitos, carga de chunks en segundo plano.

## PS3 (256 MB + 256 MB VRAM, Cell) — la lleva otro chat
- SDK 3.70 listo en `Minecraft en xboox clasico\ps3` (memoria `ps3-sdk`, skill `ps3`).
- Por potencia podría ir más allá de la 1.7.10 (1.8–1.12 razonable con el MCP; desde 1.14 Mojang publica sus mapeos).
- **Pendiente**: una auditoría como la de PS2 cuando su port esté al día (red, mando, dibujado RSX, memoria).

## Wii / 3DS / PSP / PC
- Vienen del proyecto original (OptiJuegos/erluza/ramide1); no los tocamos. Lo nuevo compila en PC; en Wii/3DS/PSP
  habría que hacer una auditoría como la de PS2 si algún día se quiere.
