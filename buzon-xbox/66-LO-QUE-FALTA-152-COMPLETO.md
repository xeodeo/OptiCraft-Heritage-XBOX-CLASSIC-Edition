# 66. Lo que falta para una 1.5.2 COMPLETA (03-11, medido sobre `port152-claude` 706705a)

Objetivo del usuario: versión completa, igual que Java 1.5.2 (no "lo justo"). Medido así:
- Clases Java 1.5.2 sin `.h` en C++: **438** → `66-clases-152-que-no-existen.txt`.
- Clases con lógica cambiada 1.2.5→1.5.2 que NO se han tocado desde que empezó el port: **262** de 386
  → `66-clases-logica-sin-tocar.txt` (con nº de líneas cambiadas).

## En marcha
- agy 58: menú creativo 1.5.2 (12 pestañas, todos los libros/pociones/huevos) + cofre trampa.
- agy 65: idiomas.
- WIP parado: aldeanos/comercio (`wt-152-pociones` c746c8f, encargos 62/63).

## Falta — jugabilidad (por orden de importancia)
1. **Comandos** (~40): `CommandHandler`, `CommandBase`, `/gamemode /time /give /tp /weather /gamerule /xp /kill /effect
   /enchant /clear /difficulty /seed /spawnpoint /toggledownfall /defaultgamemode /say /me /help /debug /testfor
   /scoreboard`, `PlayerSelector` (@p @a @r), autocompletar. Con el chat y el teclado en pantalla. Necesita
   `ICommandSender`, `GameRules`, `EnumGameType` (aventura), `Scoreboard`.
2. **Reglas de juego** (`GameRules`: keepInventory, mobGriefing, doFireTick, doMobLoot, doMobSpawning, doTileDrops,
   commandBlockOutput, naturalRegeneration, doDaylightCycle) y **modo aventura**.
3. **Aldeanos completos**: comercio (`EntityVillager` 475 líneas, `GuiMerchant`, `MerchantRecipe(List)`...), aldeas
   del desierto, reputación, `VillageSiege`, `Village` 151.
4. **Wither (jefe)**: `EntityWither`, `EntityWitherSkull`, `ModelWither`, `RenderWither(Skull)`, barra de jefe
   `BossStatus` (también para el dragón; `EntityDragon` 393 líneas sin tocar).
5. **Fuegos artificiales**: cohete, estrella, receta, partículas.
6. **Dispensador 1.5.2**: `DispenserBehavior*` (14 comportamientos), `IBlockSource`.
7. **Bloque de comandos** (`BlockCommandBlock`, `TileEntityCommandBlock`, `GuiCommandBlock`) — va con los comandos.
8. **Generador de mobs 1.5.2** (`MobSpawnerBaseLogic`, datos NBT `SpawnData`) + **vagoneta con generador**.
9. **Cuero teñido** (`RecipesArmorDyes`, `ItemArmor` color, render en 2 pasadas, lavar en caldero `BlockCauldron` 105).
10. **Encantamientos**: Espinas, libros en la mesa (`ContainerEnchantment` 87).
11. **Mapas**: zoom/escalas, `MapData` 144, `MapInfo` 87 (marcadores en marcos).
12. **Superplano personalizable** (`FlatGeneratorInfo`, `GuiCreateFlatWorld`, presets; `ChunkProviderFlat` 173).
13. **Marcador** (`Scoreboard`, objetivos, equipos) — va con /scoreboard.
14. **Combate**: `CombatTracker` (mensajes de muerte 1.5.2: "fue disparado por..."), `EntityArrow` 278 (flechas
    de fuego encienden TNT, pulsan botones), `Explosion` 189, `EntityLargeFireball`.
15. **Bloques sin actualizar**: puertas (`BlockDoor` 192), cama (explota en el Nether, 144), troncos tumbados
    (`BlockLog` 110), hojas (manzanas con fortuna), antorcha de redstone 109, calabaza, valla 1,5, escalera de mano,
    TNT 81, nieve por capas (`ItemSnow`), cacao en jungla (`WorldGenTrees` 120).
16. **Mobs sin actualizar**: lobo (collar de color, 149), Enderman 120, slime (fase lunar en pantanos), ocelote,
    gólem, oveja (mezcla de color), calamar, barca nueva (176), crías con hitbox pequeño.
17. **Inventario**: shift-clic a armadura/horno (`ContainerPlayer`, `ContainerFurnace`), repartir pila arrastrando,
    XP del horno, `InventoryPlayer` 87.
18. **Nether**: cuevas del Nether con la semilla de 1.5.2 (`MapGenCavesHell` 230).
19. **Pantalla**: nombre del objeto al cambiar de casilla, chat nuevo (`GuiNewChat`, `GuiChat` 195), calaveras
    en la cabeza (`RenderPlayer` 303), `FontRenderer` (563: colores/formato §).
20. **Estadísticas/logros 1.5.2** (`StatFileWriter`, `StatList`).

## No hace falta en consola (no se porta)
Servidor dedicado, RCON, LAN/online (Mco*, Tcp*, Packet* nuevos), snooper, CrashReport/Callable*, texture packs
por descarga, `TextureMap`/`Stitcher` (usamos atlas fijo), `Vec3Pool`/`AABBPool` (tenemos el nuestro), applet.

## Mandos (decisión pendiente del usuario)
Separar cruceta del stick (ahora las dos mueven al jugador en Xbox/PS2): proponer cruceta = cámara/chat-comandos/zoom.
