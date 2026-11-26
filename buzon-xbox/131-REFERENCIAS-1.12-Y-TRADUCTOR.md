# 131 — Referencias de GitHub para ir más allá (1.12.2) y fallo de la tabla del traductor (24-11-2026)

El usuario vio en el SMP de la 3DS (1.12.2) tolvas, pociones, etc. y que en nuestra 1.5.2 "la tolva no sirve".

## OJO (comprobado después en xemu, 24-11): en la 1.5.2 la tolva y el bloque de redstone del server SE DIBUJAN bien,
## así que esa tabla probablemente solo se aplica a la 1.2.5. Falta ver si FUNCIONAN (abrir inventario, mover ítems).

## Sospecha inicial: la tabla del traductor 340 es de la 1.2.5 también en la 1.5.2
`src/net/minecraft/src/ProtocolTranslator340.cpp` (~línea 900-1010, `switch (id)`) baja los bloques de la 1.12.2 a ids
de la **1.2.5** aunque el cliente sea 1.5.2, que YA tiene muchos de ellos. En 1.5.2 deben pasar tal cual (mismo id/meta):
tolva 154 (hoy → piedra), comparador 149/150 (→ repetidor), soltador 158 (→ dispensador), sensor de luz 151 (→ losa),
bloque de redstone 152 (→ mineral de redstone), mineral/bloque/escaleras de cuarzo 153/155/156, raíl activador 157,
cofre trampa 146 (→ cofre), placas de presión con peso 147/148, yunque 145 (→ hierro), faro 138 (→ cristal),
muro de piedra 139, maceta 140, zanahoria/patata 141/142, botón de madera 143, cabeza 144, mineral/bloque de esmeralda
129/133, cofre del End 130, gancho de cable 131/132, escaleras 134-136, bloque de comandos 137, cacao 127,
losas de madera 125/126, escaleras de arenisca 128. Lo mismo para los ÍTEMS en la tabla de ítems.
Falta de verdad en 1.5.2 (sí hay que bajarlos): caballos, alfombra, fardo de heno, arcilla endurecida/teñida (1.6),
cristal tintado (1.7), y 1.8-1.12 (slime, prismarina, observador, shulker, concreto, terracota esmaltada...).
→ Arreglo: en el árbol 1.5.2, `#if` por versión (o tabla aparte) que solo baje lo posterior a 1.5.2.

## Referencias (investigado por subagente 24-11)
1. **Cuberite** https://github.com/cuberite/cuberite (C++, Apache-2.0, activo): servidor 1.8-1.12.2 con la LÓGICA en C++:
   `src/BlockEntities/` (HopperEntity, DropSpenserEntity, BrewingstandEntity, BeaconEntity),
   `src/Simulator/IncrementalRedstoneSimulator/` (comparador, observador, tolva, sensor de luz), `src/Mobs/Horse.cpp`,
   `src/Protocol/` (protocolo 1.12.2). Licencia permisiva: se puede portar.
2. **Tablas para el traductor**: PrismarineJS minecraft-data (MIT) `data/pc/1.12/blocks.json`, `items.json`,
   `entities.json`, `pc/1.12.2/protocol.json` (no hay 1.5.2; lo más viejo `pc/1.7`).
   ViaBackwards / ViaRewind / ViaLegacy (GPL-3: SOLO leer tablas): `item-mappings-1.12.json`, sustituciones 1.8→1.7,
   `r1_5_2tor1_6_1` (cambios de paquetes 1.5.2→1.6). ViaVersion/Mappings `mapping-1.12.json` (id:meta → estado).
   Protocolo: wiki.vg cerró; está en minecraft.wiki (MCW:Projects/wiki.vg_merge), protocolo 340 = oldid 14204.
3. Consolas descompiladas: GRAnimated/MinecraftLCE (Switch, 11 %), MC3DS-Save-Research/code.bin (Ghidra, "useless").
   La 3DS es un fork de Bedrock/Pocket ~1.0-1.1, sin código público útil.
4. Clientes caseros (craftus_reloaded, Craftus-Next, ctraft, 3dscraft, GXCraft, ClassiCube): ninguno tiene tolvas,
   comparadores, destilación ni caballos. Solo referencia de render/memoria.
5. Nuestro `mcp-diff\srcOF152` ya tiene TileEntityHopper, BlockComparator, TileEntityBrewingStand, TileEntityBeacon,
   BlockDropper, ContainerRepair. Caballos: haría falta EntityHorse de MCP 1.6.4.

Orden: (1) arreglar la tabla del traductor en 1.5.2; (2) tablas minecraft-data + Via*; (3) Cuberite para caballos,
observador y lógica 1.8-1.12.

## Edición 3DS y NuMC3DS (24-11, tarde)
- Minecraft: New Nintendo 3DS Edition (Mojang + Other Ocean, 2017): fork de Pocket Edition alpha 0.15.4, por funciones
  ~Pocket 1.1.5; última 1.9.19 (ene-2019); sin online (solo local). Nada que ver con la Legacy Console Edition.
- El .rar del usuario (`Downloads\00040000001B8700.rar`) es el mod **NuMC3DS**: `code.ips` (50 KB, parche del
  ejecutable) + romfs (texturas, sonidos, fuentes, resourcepacks vanilla client/server con JSON de Bedrock) +
  changelog. NO trae code.bin → no se puede descompilar "como MCP". Con un volcado propio (New 3DS + Luma + GodMode9)
  saldría pseudo-C ARM sin nombres (meses/años de trabajo). Lo legible sería libminecraftpe.so de Android 1.1.x
  (publicado con símbolos), de una copia propia.
- Ideas de su changelog: crossplay con MCPE, comandos y bloques de comandos, mapas/localizadores, packs de comportamiento
  y skins desde la SD, mundos infinitos, altura 256, reasignar mando, updater desde GitHub.
- Universal-Updater + UniStore (tienda de mods 3DS): índice JSON en GitHub, lista con iconos, buscar/ordenar, aviso de
  actualización, cola de descargas, herramienta de borrado → modelo para una "tienda de contenido" en nuestro lanzador.
