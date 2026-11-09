# Encargo 52 (agy): grupo 2, parte C — comparador, tolva, soltador, repetidor 1.5

Claude ACEPTÓ el 51 (vagoneta TNT y raíles: fórmula, fall, TNTFuse coinciden con el Java). Rama `port152-inventario`
en `08cfebf`. No hagas reset ni rebase. Codex sigue parado.

## Qué portar (traducción del Java 1.5.2, src152)
1. `BlockRedstoneLogic` (base común de 1.5) y `BlockRedstoneRepeater` reescrito encima (con bloqueo lateral de 1.4,
   `func_94476_e` etc.). Que los circuitos con repetidores sigan igual.
2. `BlockComparator` + `TileEntityComparator` (149 apagado, 150 encendido): modo comparar/restar (clic), lectura de
   `getComparatorInputOverride` de cofres, dispensadores, hornos, tolvas, jukebox, pastel, marco… (lo que 1.5.2 tenga),
   lectura a través de un bloque sólido. `Block::isComparatorId` (lo dejó Codex devolviendo false) debe pasar a
   reconocer 149/150, y `World::func_96440_m` ya avisa a los comparadores: compruébalo.
   `hasComparatorInputOverride`/`getComparatorInputOverride` en BlockChest, BlockDispenser, BlockFurnace,
   BlockBrewingStand, BlockJukeBox, BlockCake, BlockEndPortalFrame, BlockDetectorRail… como en 1.5.2
   (`Container.calcRedstoneFromInventory`).
3. `BlockHopper` + `TileEntityHopper` (+ `ContainerHopper`, `GuiHopper`, `Hopper`/`ISidedInventory` lo que haga
   falta): recoger ítems de arriba, pasar a un contenedor, bloqueo con redstone, render (renderType 38).
   Y `EntityMinecartHopper` (vagoneta con tolva, 408) + raíl activador que la bloquea.
4. `BlockDropper` (158) + `TileEntityDropper`: suelta ítems o los mete en un contenedor delante.
5. Recetas, creativo, TooManyItems, crafteo de consola (`EXTRA_ENTRIES` si LCE no las trae), iconos TEX152_*.

## OJO (aprendido en este port)
- RENDIMIENTO: la tolva hace trabajo cada tick (transfer cooldown 8 ticks en 1.5.2). Respeta el cooldown, nada de buscar
  entidades por AABB cada tick si el Java no lo hace, ningún `new` por tick. Una granja de tolvas no debe hundir la Xbox.
- `setBlockMetadataWithNotify152(..., 2)` donde el Java usa flags 2; la versión vieja avisa a vecinos.
- `removeBlockTileEntity` fuera del tick de tile entities BORRA el objeto (commit `3eee9da`).
- Bloques con cambio de metadato: `setRequiresSelfNotify()`.
- Candado BUILD.lock; xbox-release y xbox-release-exp; nunca /fp:fast; no push; no copies a ClassicXboxLoader.

## Al acabar
Informe `buzon-xbox\52-INFORME-agy-redstone-C-152.md` (tabla bloque -> archivos:línea, qué contenedores dan señal al
comparador, coste por tick de la tolva), commits, y en tu terminal: `REDSTONE C LISTO <commit>`
