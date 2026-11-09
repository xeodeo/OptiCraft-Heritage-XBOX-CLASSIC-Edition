# Informe Encargo 44 — Bloques simples e items básicos 1.5.2 (Grupo 1, parte agy)

- **Agente**: `agy`
- **Worktree**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-agy`
- **Rama**: `port152-inventario` (`e8728e2`)
- **Fuente de verdad**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`

---

## 1. Bloques portados (12 bloques + 2 variantes de losa)

| ID | Puntero en `Block` | Clase | `blockName` | Dureza / Resist. | Notas de implementación |
|---:|---|---|---|---:|---|
| 128 | `Block::stairsSandStone` | `BlockStairs` | `stairsSandStone` | Heredado (`sandStone`, meta `0`) | Usa nuevo constructor `BlockStairs(int, Block*, int modelMeta)` |
| 129 | `Block::oreEmerald` | `BlockOre` | `oreEmerald` | `3.0f` / `5.0f` | Suelta `Item::emerald`, `3..7` XP con `dropXpOnBlockBreak`, requiere pico de hierro o diamante (`harvestLevel >= 2`) |
| 133 | `Block::blockEmerald` | `BlockOreStorage` | `blockEmerald` | `5.0f` / `10.0f` | Bloque compacto `3x3` de esmeralda, requiere pico `harvestLevel >= 2` |
| 134 | `Block::stairsWoodSpruce` | `BlockStairs` | `stairsWoodSpruce` | Heredado (`planks`, meta `1`) | Textura de abeto (`meta 1`), inflamable en `BlockFire` (`5, 20`) |
| 135 | `Block::stairsWoodBirch` | `BlockStairs` | `stairsWoodBirch` | Heredado (`planks`, meta `2`) | Textura de abedul (`meta 2`), inflamable en `BlockFire` (`5, 20`) |
| 136 | `Block::stairsWoodJungle` | `BlockStairs` | `stairsWoodJungle` | Heredado (`planks`, meta `3`) | Textura de jungla (`meta 3`), inflamable en `BlockFire` (`5, 20`) |
| 141 | `Block::carrot` | `BlockCarrot` | `carrots` | `0.0f` | Extiende `BlockCrops`; `getSeedItem()` y `getCropItem()` devuelven `Item::carrot->shiftedIndex` |
| 142 | `Block::potato` | `BlockPotato` | `potatoes` | `0.0f` | Extiende `BlockCrops`; `dropBlockAsItemWithChance` añade 2% de probabilidad en `meta >= 7` de soltar `Item::poisonousPotato` |
| 152 | `Block::blockRedstone` | `BlockPoweredOre` | `blockRedstone` | `5.0f` / `10.0f` | Extiende `BlockOreStorage`; `canProvidePower() = true`, `isPoweringTo() = true`; `Block::isNormalCube` excluye bloques con `canProvidePower()` y `World::isBlockIndirectlyProvidingPowerTo` usa `Block::isNormalCube` para energizar cables, pistones, lámparas y repetidores |
| 153 | `Block::oreNetherQuartz` | `BlockOre` | `netherquartz` | `3.0f` / `5.0f` | Suelta `Item::netherQuartz`, `2..5` XP con `dropXpOnBlockBreak`, cosechable con cualquier pico |
| 155 | `Block::blockNetherQuartz` | `BlockQuartz` | `quartzBlock` | `0.8f` / `0.0f` | Metadatos `0` (`default`), `1` (`chiseled`), `2..4` (`lines`, pilar orientado en Y/X/Z via `onBlockPlaced` + `getRenderType() == 39`). Registrado con `ItemMultiTextureTile` |
| 156 | `Block::stairsNetherQuartz` | `BlockStairs` | `stairsQuartz` | Heredado (`blockNetherQuartz`, meta `0`) | Escalera de cuarzo, cosechable con pico |
| 43/44 | `Block::stairDouble` / `Block::stairSingle` | `BlockStep` | `stoneSlab` | `2.0f` / `10.0f` | Ampliado `blockStepTypes` de 6 a 8 variantes añadiendo `6 = "netherBrick"` y `7 = "quartz"` en `BlockStep` e `ItemSlab` |

### Ajustes adicionales en bloques existentes
- **`BlockStairs`**:
  - Añadido `modelBlockMetadata` (por defecto `0`) y delegación de `getBlockTextureFromSideAndMetadata(side)` a `modelBlock->getBlockTextureFromSideAndMetadata(side, modelBlockMetadata)`.
  - Eliminados los overrides heredados de Beta 1.7.3 (`idDropped`, `quantityDropped`, `dropBlockAsItemWithChance`) para que todas las escaleras se suelten a sí mismas al romperse como en 1.2.5 / 1.5.2.
- **`BlockCrops`**:
  - Convertidos `getSeedItem()` y `getCropItem()` en métodos `virtual` protegidos.
  - Portada la lógica 1.5.2 de `dropBlockAsItemWithChance` (en `meta >= 7` realiza 3 tiradas con fortuna para soltar `getSeedItem()`, además del `getCropItem()` principal) y actualizado `ItemDye` para aplicar polvo de hueso a cualquier `BlockCrops*` (`crops`, `carrot`, `potato`).
- **`BlockOre` y `Block::dropXpOnBlockBreak`**:
  - Añadido `Block::dropXpOnBlockBreak(World*, int, int, int, int)` que genera `EntityXPOrb` en servidor/singleplayer.
  - Actualizado `BlockOre::dropBlockAsItemWithChance` con los rangos 1.5.2 de experiencia (`coal`: 0..2, `diamond`: 3..7, `emerald`: 3..7, `lapis`: 2..5, `netherQuartz`: 2..5).

---

## 2. Items portados (9 items + renombre de `diamond`)

| ID (`shiftedIndex`) | Offset (`id - 256`) | Puntero en `Item` | Clase | `itemName` | Parámetros 1.5.2 |
|---:|---:|---|---|---|---|
| 264 | 8 | `Item::diamond` | `Item` | `diamond` | Actualizado de `"emerald"` a `"diamond"`; `StringTranslate` remapea `"item.emerald.name"` -> `"item.diamond.name"` cuando carga archivos `.lang` 1.2.5 |
| 388 | 132 | `Item::emerald` | `Item` | `emerald` | Gema de esmeralda |
| 391 | 135 | `Item::carrot` | `ItemSeedFood` | `carrots` | `healAmount = 4`, `saturation = 0.6f`, planta `Block::carrot->blockID` sobre `Block::tilledField->blockID` |
| 392 | 136 | `Item::potato` | `ItemSeedFood` | `potato` | `healAmount = 1`, `saturation = 0.3f`, planta `Block::potato->blockID` sobre `Block::tilledField->blockID` |
| 393 | 137 | `Item::bakedPotato` | `ItemFood` | `potatoBaked` | `healAmount = 6`, `saturation = 0.6f`, `isWolfsFavoriteMeat = false` |
| 394 | 138 | `Item::poisonousPotato` | `ItemFood` | `potatoPoisonous` | `healAmount = 2`, `saturation = 0.3f`, `setPotionEffect(Potion::poison->id, 5, 0, 0.6f)` |
| 396 | 140 | `Item::goldenCarrot` | `ItemFood` | `carrotGolden` | `healAmount = 6`, `saturation = 1.2f`, `setPotionEffect(PotionHelper::goldenCarrotEffect)` (`"-0+1+2-3+13&4-4"`) |
| 400 | 144 | `Item::pumpkinPie` | `ItemFood` | `pumpkinPie` | `healAmount = 8`, `saturation = 0.3f`, `isWolfsFavoriteMeat = false` |
| 405 | 149 | `Item::netherrackBrick` | `Item` | `netherbrick` | Lingote de ladrillo del Nether obtenido fundiendo `Block::netherrack` |
| 406 | 150 | `Item::netherQuartz` | `Item` | `netherquartz` | Cuarzo del Nether |

---

## 3. Recetas, horno, entidades, aldeas y menús

### Recetas de crafteo y horno
- **`RecipesIngots`**:
  - `Block::blockEmerald <-> 9 Item::emerald`
  - `Block::blockRedstone <-> 9 Item::redstone`
- **`RecipesCrafting`**:
  - `2x2 Item::netherrackBrick -> 1 Block::netherBrick`
  - `2x2 Item::netherQuartz -> 1 Block::blockNetherQuartz (meta 0)`
  - `1x2 Block::stairSingle (meta 7) -> 1 Block::blockNetherQuartz (meta 1, cincelado)`
  - `1x2 Block::blockNetherQuartz (meta 0) -> 2 Block::blockNetherQuartz (meta 2, pilar)`
  - `Escalera 3x3 Block::sandStone -> 4 Block::stairsSandStone`
  - `Escalera 3x3 Block::blockNetherQuartz -> 4 Block::stairsNetherQuartz`
- **`CraftingManager`**:
  - Escaleras de madera por metadato de `Block::planks`: `meta 0 -> stairCompactPlanks`, `meta 1 -> stairsWoodSpruce`, `meta 2 -> stairsWoodBirch`, `meta 3 -> stairsWoodJungle`.
  - Losas adicionales (`1x3`): `Block::netherBrick -> 6 Block::stairSingle (meta 6)` y `Block::blockNetherQuartz -> 6 Block::stairSingle (meta 7)`.
- **`RecipesFood`**:
  - `Item::carrot` rodeado de `8 Item::goldNugget` -> `1 Item::goldenCarrot`
  - Shapeless: `Block::pumpkin + Item::sugar + Item::egg -> 1 Item::pumpkinPie`
- **`FurnaceRecipes`**:
  - `Item::potato -> Item::bakedPotato` (`0.35f` XP)
  - `Block::netherrack -> Item::netherrackBrick` (`0.1f` XP)
  - `Block::oreEmerald -> Item::emerald` (`1.0f` XP)
  - `Block::oreNetherQuartz -> Item::netherQuartz` (`0.2f` XP)

### Hooks en entidades, pociones, aldeas y UI
- **`PotionHelper`**: Añadido `PotionHelper::goldenCarrotEffect = "-0+1+2-3+13&4-4"` y las fórmulas `potionRequirements` para `Potion::nightVision` (`"!0 & 1 & !2 & 3 & 1+6"`) y `Potion::invisibility` (`"!0 & 1 & 2 & !3 & 2+6"`), permitiendo elaborar pociones de Visión Nocturna e Invisibilidad con `goldenCarrot`.
- **`EntityZombie`**: `dropRareDrop` actualizado a 1.5.2 (`0: Item::ingotIron`, `1: Item::carrot`, `2: Item::potato`).
- **`EntityPig`**: Añadido `EntityAITempt` para `Item::carrot->shiftedIndex` y override `EntityPig::isWheat(ItemStack*) const` que acepta `Item::carrot` para reproducción.
- **`ComponentVillageField` y `ComponentVillageField2`**: Selección aleatoria de cultivos por parcela (`getRandomCrop`: 20% `carrot`, 20% `potato`, 60% `crops`) persistida en los campos de instancia.
- **`ContainerCreative`, `TooManyItems`, `LegacyCraftingGroups.h` y `StringTranslate`**:
  - Todos los bloques (incluyendo subtipos `0..2` de `blockNetherQuartz` y `0..7` de `stairSingle`) e items aparecen en el inventario creativo, TooManyItems y `XboxCraftingScreen`.
  - `StringTranslate` incluye nombres fallback en inglés y español para las nuevas claves 1.5.2 sin tocar ningún archivo en `OptiCraftHeritageEdition\data`.

---

## 4. Marcadores `// PORT152_ICON:` para el encargo de atlas

| Archivo y línea | Marcador | Índice provisional usado |
|---|---|---:|
| `src/net/minecraft/src/Block.cpp:1080` | `// PORT152_ICON: oreEmerald` | `50` (`oreDiamond` en `terrain.png`) |
| `src/net/minecraft/src/Block.cpp:1081` | `// PORT152_ICON: blockEmerald` | `24` (`blockDiamond` en `terrain.png`) |
| `src/net/minecraft/src/Block.cpp:1087` | `// PORT152_ICON: blockRedstone` | `24` (`blockDiamond` en `terrain.png`) |
| `src/net/minecraft/src/Block.cpp:1088` | `// PORT152_ICON: netherquartz` | `50` (`oreDiamond` en `terrain.png`) |
| `src/net/minecraft/src/BlockCarrot.cpp:5,20` | `// PORT152_ICON: carrots_0` | `88` (`crops_0` en `terrain.png`) |
| `src/net/minecraft/src/BlockCarrot.cpp:22` | `// PORT152_ICON: carrots_1` | `90` (`crops_2` en `terrain.png`) |
| `src/net/minecraft/src/BlockCarrot.cpp:24` | `// PORT152_ICON: carrots_2` | `92` (`crops_4` en `terrain.png`) |
| `src/net/minecraft/src/BlockCarrot.cpp:27` | `// PORT152_ICON: carrots_3` | `95` (`crops_7` en `terrain.png`) |
| `src/net/minecraft/src/BlockPotato.cpp:7,22` | `// PORT152_ICON: potatoes_0` | `88` (`crops_0` en `terrain.png`) |
| `src/net/minecraft/src/BlockPotato.cpp:24` | `// PORT152_ICON: potatoes_1` | `90` (`crops_2` en `terrain.png`) |
| `src/net/minecraft/src/BlockPotato.cpp:26` | `// PORT152_ICON: potatoes_2` | `92` (`crops_4` en `terrain.png`) |
| `src/net/minecraft/src/BlockPotato.cpp:29` | `// PORT152_ICON: potatoes_3` | `95` (`crops_7` en `terrain.png`) |
| `src/net/minecraft/src/BlockQuartz.cpp:12,44` | `// PORT152_ICON: quartzblock_side` | `192` (`sandStone` side en `terrain.png`) |
| `src/net/minecraft/src/BlockQuartz.cpp:22` | `// PORT152_ICON: quartzblock_chiseled_top` | `176` (`sandStone` top en `terrain.png`) |
| `src/net/minecraft/src/BlockQuartz.cpp:24` | `// PORT152_ICON: quartzblock_chiseled` | `229` (`sandStone` chiseled en `terrain.png`) |
| `src/net/minecraft/src/BlockQuartz.cpp:32` | `// PORT152_ICON: quartzblock_lines_top` | `176` (`sandStone` top en `terrain.png`) |
| `src/net/minecraft/src/BlockQuartz.cpp:34` | `// PORT152_ICON: quartzblock_lines` | `230` (`sandStone` smooth en `terrain.png`) |
| `src/net/minecraft/src/BlockQuartz.cpp:38` | `// PORT152_ICON: quartzblock_bottom` | `208` (`sandStone` bottom en `terrain.png`) |
| `src/net/minecraft/src/BlockQuartz.cpp:42` | `// PORT152_ICON: quartzblock_top` | `176` (`sandStone` top en `terrain.png`) |
| `src/net/minecraft/src/BlockStep.cpp:68` | `// PORT152_ICON: quartzblock_top` | `176/192/208` (`sandStone` en `terrain.png`) |
| `src/net/minecraft/src/Item.cpp:614` | `// PORT152_ICON: emerald` | `(7, 3)` (`diamond` en `items.png`) |
| `src/net/minecraft/src/Item.cpp:615` | `// PORT152_ICON: carrots` | `(10, 0)` (`appleRed` en `items.png`) |
| `src/net/minecraft/src/Item.cpp:616` | `// PORT152_ICON: potato` | `(10, 0)` (`appleRed` en `items.png`) |
| `src/net/minecraft/src/Item.cpp:617` | `// PORT152_ICON: potatoBaked` | `(9, 2)` (`bread` en `items.png`) |
| `src/net/minecraft/src/Item.cpp:618` | `// PORT152_ICON: potatoPoisonous` | `(11, 8)` (`spiderEye` en `items.png`) |
| `src/net/minecraft/src/Item.cpp:619` | `// PORT152_ICON: carrotGolden` | `(11, 0)` (`appleGold` en `items.png`) |
| `src/net/minecraft/src/Item.cpp:620` | `// PORT152_ICON: pumpkinPie` | `(13, 1)` (`cake` en `items.png`) |
| `src/net/minecraft/src/Item.cpp:621` | `// PORT152_ICON: netherbrick` | `(6, 1)` (`brick` en `items.png`) |
| `src/net/minecraft/src/Item.cpp:622` | `// PORT152_ICON: netherquartz` | `(7, 3)` (`diamond` en `items.png`) |

---

## 5. Compilación verificada

- **Preset**: `xbox-release` (con protocolo `BUILD.lock` respetado y liberado al terminar).
- **Resultado**: `1188/1188` objetivos compilados, enlazados y empaquetados en `OptiCraft.iso` con **0 errores** y **0 warnings nuevos** en los archivos tocados.
