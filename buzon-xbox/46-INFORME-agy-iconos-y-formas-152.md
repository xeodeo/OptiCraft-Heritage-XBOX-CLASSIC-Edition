# Informe Encargo 46 (Punto 2) — Bloques con forma especial e items asociados 1.5.2

- **Agente**: `agy`
- **Worktree**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-agy`
- **Rama**: `port152-inventario` (sobre `port152-base` `39a854b`, con commit `533f1c5` de Claude)
- **Fuente de verdad**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`

---

## 1. Resumen general

Se ha implementado íntegramente el **Punto 2 del Encargo 46** (antiguo encargo 45 de Codex):
1. **Losas de madera** (`125` `woodDoubleSlab`, `126` `woodSingleSlab`): clase abstracta base `BlockHalfSlab`, `BlockWoodSlab` con 4 tipos de madera, `BlockStep` adaptado como subclase de `BlockHalfSlab`, `ItemSlab` polimórfico de 4 parámetros, inflamabilidad en `BlockFire` (`5, 20`) y recetas en `CraftingManager`.
2. **Muro de adoquines** (`139` `cobblestoneWall`): `BlockWall` (`renderType = 32`), conexión a bloques normales y otros muros, caja de colisión AABB `maxY = 1.5`, bajada de puertas de valla adyacentes (`-0.1875f`) en `RenderBlocks125`, render en 3D para inventario y recetas para adoquín normal y musgoso.
3. **Maceta** (`140` `flowerPot`) + **Ítem maceta** (`390` `flowerPot`): `BlockFlowerPot` (`renderType = 33`), colocación de plantas mediante interacción, soporte para flores, retoños (4 maderas), setas, cactus, arbusto seco y helecho con render escalado e inset de tierra, dropeo de maceta y planta al romperse y receta con ladrillos en `RecipesCrafting`.
4. **Planta de cacao** (`127` `cocoaPlant`): `BlockCocoa` (`renderType = 28`), 3 etapas de crecimiento (`meta >> 2`), anclaje y validación contra troncos de jungla (`Block::wood`, `meta 3`), plantado con tinte marrón (daño 3) y maduración acelerada con polvo de hueso en `ItemDye.cpp`.
5. **Pilar de cuarzo** (`155` `blockNetherQuartz`): `renderType = 39` implementado en `RenderBlocks125::renderBlockQuartz`, orientación según eje (`onBlockPlaced` Y/X/Z: metadatos 2, 3, 4) y renderizado 3D en inventario.
6. **Cabeza / Calavera** (`144` `skull`) + **Ítem cabeza** (`397` `skull`): `BlockSkull` (`renderType = -1`), `TileEntitySkull` serializando tipo, rotación y extraType, `TileEntitySkullRenderer` con `ModelSkeletonHead` (caja 8x8x8 con rotaciones Y y X) texturizado con mobs (/mob/skeleton.png, /mob/skeleton_wither.png, /mob/zombie.png, mob/char.png, /mob/creeper.png), `ItemSkull` con 5 subtipos e iconos de atlas.
7. **Yunque** (`145` `anvil`): `Material::anvil` (inamovible por pistones, no empujable), `StepSoundAnvil` con sonidos (`random.anvil_land`, `random.anvil_use`, `random.anvil_break`), `BlockAnvil` (`renderType = 35`), `ItemAnvilBlock`, física de caída en `BlockSand` y `EntityFallingSand` con daño por caída a entidades y degradación del bloque en el impacto, renderizado por partes y rotaciones en `RenderBlocks125`.
8. **Cofre de Ender** (`130` `enderChest`): `BlockEnderChest` (`renderType = 22`, cosechable con toque de seda `func_50074_q`), `TileEntityEnderChest` con animación de apertura/cierre, sonidos y sincronización de jugadores mediante `playNoteAt`, `TileEntityEnderChestRenderer` con modelo y `/item/enderchest.png`, `InventoryEnderChest` ligado a `EntityPlayer` serializado en NBT tag `"EnderItems"` y clonado al respawnear, render de ítem en 3D en `ChestItemRenderHelper`.
9. **Faro** (`138` `beacon`) + **Estrella del Nether** (`399` `netherStar`): `BlockBeacon` (`renderType = 34`, nivel de luz 15, `getBlockEntity`), `TileEntityBeacon`, `TileEntityBeaconRenderer` con haz de luz vertical animado y rotatorio usando `/misc/beam.png`, ítem `netherStar` con efecto encantado (`hasEffect = true`), render de mundo e inventario (base de obsidiana + núcleo con textura de faro + cubierta de cristal) y receta 1.5.2.
10. **Menús, UI y traducciones**: Entradas añadidas en `ContainerCreative`, `TooManyItems` y cadenas en inglés/español en `StringTranslate.cpp`.

---

## 2. Tabla de bloques, items, renderTypes y archivos

| ID | Nombre interno / Puntero | Clase C++ | `renderType` | Archivos clave | Texturas / Atlas |
|---:|---|---|---:|---|---|
| 125 | `woodDoubleSlab` | `BlockWoodSlab` (`BlockHalfSlab`) | `0` | `BlockHalfSlab.cpp`, `BlockWoodSlab.cpp` | Madera (roble, abeto, abedul, jungla) |
| 126 | `woodSingleSlab` | `BlockWoodSlab` (`BlockHalfSlab`) | `0` | `BlockHalfSlab.cpp`, `BlockWoodSlab.cpp`, `ItemSlab.cpp` | Madera (roble, abeto, abedul, jungla) |
| 127 | `cocoaPlant` | `BlockCocoa` (`BlockDirectional`) | `28` | `BlockCocoa.cpp`, `ItemDye.cpp`, `RenderBlocks125.cpp` | `cocoa_0.png`, `cocoa_1.png`, `cocoa_2.png` |
| 130 | `enderChest` | `BlockEnderChest` (`BlockContainer`) | `22` | `BlockEnderChest.cpp`, `TileEntityEnderChest.cpp`, `TileEntityEnderChestRenderer.cpp`, `InventoryEnderChest.cpp` | `/item/enderchest.png` |
| 138 | `beacon` | `BlockBeacon` (`BlockContainer`) | `34` | `BlockBeacon.cpp`, `TileEntityBeacon.cpp`, `TileEntityBeaconRenderer.cpp`, `RenderBlocks125.cpp` | `beacon.png`, `/misc/beam.png`, `obsidian`, `glass` |
| 139 | `cobblestoneWall` | `BlockWall` | `32` | `BlockWall.cpp`, `RenderBlocks125.cpp`, `RenderBlocks.cpp` | `cobblestone.png`, `stoneMoss.png` |
| 140 | `flowerPot` | `BlockFlowerPot` | `33` | `BlockFlowerPot.cpp`, `RenderBlocks125.cpp` | `flowerPot.png`, tierra y texturas de plantas |
| 144 | `skull` | `BlockSkull` (`BlockContainer`) | `-1` | `BlockSkull.cpp`, `TileEntitySkull.cpp`, `TileEntitySkullRenderer.cpp`, `ModelSkeletonHead.cpp` | Skins de mobs (/mob/*.png) |
| 145 | `anvil` | `BlockAnvil` (`BlockSand`) | `35` | `BlockAnvil.cpp`, `ItemAnvilBlock.cpp`, `EntityFallingSand.cpp`, `RenderBlocks125.cpp` | `anvil_base.png`, `anvil_top.png`, `anvil_top_damaged_*.png` |
| 155 | `blockNetherQuartz` (pilar) | `BlockQuartz` | `39` | `BlockQuartz.cpp`, `RenderBlocks125.cpp`, `RenderBlocks.cpp` | `quartzblock_lines.png`, `quartzblock_lines_top.png` |
| 390 | `flowerPot` (ítem) | `Item` | - | `Item.cpp:1425` | `TEX152_ITEM_ICON("flowerPot.png")` |
| 397 | `skull` (ítem) | `ItemSkull` | - | `ItemSkull.cpp` | `skull_skeleton.png`, `skull_wither.png`, `skull_zombie.png`, `skull_char.png`, `skull_creeper.png` |
| 399 | `netherStar` (ítem) | `Item` (con `hasEffect`) | - | `Item.cpp:1427` | `TEX152_ITEM_ICON("netherStar.png")` |

---

## 3. Estado de compatibilidad de render por plataforma

- **Xbox (D3D8 / PcLegacy incremental)**: 100% implementado y verificado.
  - Render de TESRs (Ender Chest, Calavera, Faro) integrado en `TileEntityRenderer.cpp` y el pipeline gráfico D3D8.
  - Render de bloques con geometrías y subtipos (Muro 32, Maceta 33, Cacao 28, Faro 34, Yunque 35, Cuarzo 39) implementado en `RenderBlocks125.cpp` usando el patrón nativo de la base OptiCraft (`block->setBlockBounds`).
  - Render 3D en interfaz / slots de inventario añadido en `RenderBlocks::renderBlockOnInventory` y `ChestItemRenderHelper`.
- **PS2 / Wii**:
  - Los TESRs y renderers por teselador se integran limpiamente sin dependencias directas de APIs propietarias de PC. La lógica de `RenderBlocks125` emite quads estándar a través de `Tessellator::instance`, compatible con el backend compartido de consolas.

---

## 4. Estado de compilación y verificación

Ambas configuraciones de Xbox han sido compiladas utilizando el candado `BUILD.lock`:

1. **`build\xbox-release`**:
   - `ninja -C build\xbox-release`: **Exit code 0** (Limpio).
   - Generación de `OptiCraft.iso` (5 archivos, 69,689,531 bytes).
2. **`build\xbox-release-exp`**:
   - `ninja -C build\xbox-release-exp`: **Exit code 0** (Limpio).
   - Generación de `OptiCraft.iso` (5 archivos, 69,738,683 bytes).

---

## 5. Salida de `git diff --stat`

```
 src/mods/toomanyitems/TooManyItems.cpp          |  20 +
 src/net/minecraft/src/Block.cpp                 |  58 ++-
 src/net/minecraft/src/Block.h                   |  10 +
 src/net/minecraft/src/BlockFire.cpp             |   4 +
 src/net/minecraft/src/BlockQuartz.cpp           |  31 ++
 src/net/minecraft/src/BlockQuartz.h             |   3 +
 src/net/minecraft/src/BlockSand.cpp             |  18 +-
 src/net/minecraft/src/BlockSand.h               |   4 +
 src/net/minecraft/src/BlockStep.cpp             | 112 +----
 src/net/minecraft/src/BlockStep.h               |  21 +-
 src/net/minecraft/src/ChestItemRenderHelper.cpp |  17 +-
 src/net/minecraft/src/ChestItemRenderHelper.h   |   2 +
 src/net/minecraft/src/ContainerCreative.cpp     |  16 +
 src/net/minecraft/src/CraftingManager.cpp       |   5 +-
 src/net/minecraft/src/EntityFallingSand.cpp     |  50 ++-
 src/net/minecraft/src/EntityFallingSand.h       |   5 +
 src/net/minecraft/src/EntityPlayer.cpp          |  22 +
 src/net/minecraft/src/EntityPlayer.h            |   3 +
 src/net/minecraft/src/Item.cpp                  |  19 +
 src/net/minecraft/src/Item.h                    |   3 +
 src/net/minecraft/src/ItemDye.cpp               |  37 ++
 src/net/minecraft/src/ItemSlab.cpp              |  67 ++-
 src/net/minecraft/src/ItemSlab.h                |   9 +-
 src/net/minecraft/src/Material.cpp              |   3 +
 src/net/minecraft/src/Material.h                |   1 +
 src/net/minecraft/src/RecipesCrafting.cpp       |  12 +
 src/net/minecraft/src/RenderBlocks.cpp          | 113 ++++-
 src/net/minecraft/src/RenderBlocks.h            |  15 +
 src/net/minecraft/src/RenderBlocks125.cpp       | 573 ++++++++++++++++++++++--
 src/net/minecraft/src/RenderFallingSand.cpp     |  19 +-
 src/net/minecraft/src/StepSound.cpp             |  20 +
 src/net/minecraft/src/StepSound.h               |   9 +
 src/net/minecraft/src/StringTranslate.cpp       |  26 +-
 src/net/minecraft/src/TileEntity.cpp            |   6 +
 src/net/minecraft/src/TileEntityRenderer.cpp    |  15 +
 35 files changed, 1152 insertions(+), 194 deletions(-)
```
