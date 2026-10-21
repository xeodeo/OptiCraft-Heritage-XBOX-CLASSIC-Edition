# Inventario de IDs de Bloques e Ítems Añadidos en 1.5.2 y Verificación de Colisiones (`ids.md`)

Análisis comparativo entre **Minecraft 1.2.5** (`mcp-diff/src125/net/minecraft/src/Block.java` e `Item.java`), **Minecraft 1.5.2** (`mcp-diff/src152/net/minecraft/src/Block.java` e `Item.java`) y el código C++ actual de **OptiCraft Heritage** (`wt-152-agy/src/`).

---

## 0. Resumen de Colisiones en OptiCraft

- **Colisiones de IDs de Bloques en OptiCraft**: **0 (NINGUNA)**.
  - En `src/net/minecraft/src/Block.cpp:825-948` y `src/net/minecraft/src/Block.h:212-334`, OptiCraft registra únicamente los bloques vanilla de 1.2.5 desde el ID `1` (`stone`) hasta el ID `124` (`redstoneLampActive` en `src/net/minecraft/src/Block.cpp:948`).
  - Los slots `125..255` (y `256..4095` en PC) de `Block::blocksList` se inicializan a `nullptr` en `src/net/minecraft/src/Block.cpp:130`.
  - Los **34 bloques nuevos de 1.5.2 (IDs 125 a 158)** están **100% libres**.
- **Colisiones de IDs de Ítems en OptiCraft**: **0 (NINGUNA)**.
  - En `src/net/minecraft/src/Item.cpp:474-614` y `src/net/minecraft/src/Item.h:90-230`, OptiCraft registra únicamente los ítems secuenciales vanilla de 1.2.5 desde ID base `0` (`shovelSteel`, ID final `256`) hasta ID base `129` (`fireballCharge`, ID final `385` en `src/net/minecraft/src/Item.cpp:603`), más los 11 discos de música en IDs base `2000..2010` (IDs finales `2256..2266`, `record13` a `record11` en `src/net/minecraft/src/Item.cpp:604-614`).
  - Los **24 ítems nuevos de 1.5.2** —IDs base **130 a 152** (IDs finales **386 a 408**) y el disco `recordWait` en ID base **2011** (ID final **2267**)— están **100% libres** (`nullptr` en `Item::itemsList`, `src/net/minecraft/src/Item.cpp:59`).
- **Mods integrados en OptiCraft (`src/mods/`)**:
  - Verificados `src/mods/ModManager.cpp:1-90`, `src/mods/OchPackReader.cpp:1-120`, `src/mods/StrongholdLocatorMod.cpp:1-80`, `src/mods/reiminimap/ReiMinimap.cpp:1-360` y `src/mods/toomanyitems/TooManyItems.cpp:1-200`. Ninguno registra bloques ni ítems adicionales en `Block::blocksList` (`src/net/minecraft/src/Block.cpp:130`) ni en `Item::itemsList` (`src/net/minecraft/src/Item.cpp:59`).

---

## 1. Bloques Nuevos Añadidos (1.2.5 → 1.5.2): IDs 125 al 158

En 1.2.5 y en OptiCraft (`src/net/minecraft/src/Block.cpp:948`), el último bloque registrado es el **ID 124** (`redstoneLampActive`). Entre 1.3.1, 1.4.2 y 1.5 se añadieron **34 bloques nuevos** (IDs **125 a 158**).

> **Nota sobre API de nombres**: En 1.5.2 el método `.setBlockName(...)` de 1.2.5 (`src/net/minecraft/src/Block.h:358`, `src/net/minecraft/src/Block.cpp:542`) fue renombrado a `.setUnlocalizedName(...)` (`mcp-diff/src152/net/minecraft/src/Block.java:1022`).

| ID | Campo en `Block.java` (1.5.2) | Línea en `src152/Block.java` | Clase Java e instanciación en 1.5.2 | `unlocalizedName` | Versión | Estado en OptiCraft C++ (`archivo:línea`) |
|---:|---|---|---|---|---|---|
| **125** | `woodDoubleSlab` | `Block.java:399-403` | `(BlockHalfSlab)(new BlockWoodSlab(125, true))` | `"woodSlab"` | 1.3.1 | **Libre** (`src/net/minecraft/src/Block.cpp:948` termina en ID 124; `blocksList[125] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **126** | `woodSingleSlab` | `Block.java:404-408` | `(BlockHalfSlab)(new BlockWoodSlab(126, false))` | `"woodSlab"` | 1.3.1 | **Libre** (`blocksList[126] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **127** | `cocoaPlant` | `Block.java:409` | `new BlockCocoa(127)` | `"cocoa"` | 1.3.1 | **Libre** (`blocksList[127] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **128** | `stairsSandStone` | `Block.java:410` | `new BlockStairs(128, sandStone, 0)` | `"stairsSandStone"` | 1.3.1 | **Libre** (`blocksList[128] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **129** | `oreEmerald` | `Block.java:411-415` | `new BlockOre(129)` | `"oreEmerald"` | 1.3.1 | **Libre** (`blocksList[129] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **130** | `enderChest` | `Block.java:416-421` | `new BlockEnderChest(130)` | `"enderChest"` | 1.3.1 | **Libre** (`blocksList[130] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **131** | `tripWireSource` | `Block.java:422` | `(BlockTripWireSource)(new BlockTripWireSource(131))` | `"tripWireSource"` | 1.3.1 | **Libre** (`blocksList[131] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **132** | `tripWire` | `Block.java:423` | `new BlockTripWire(132)` | `"tripWire"` | 1.3.1 | **Libre** (`blocksList[132] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **133** | `blockEmerald` | `Block.java:424-428` | `new BlockOreStorage(133)` | `"blockEmerald"` | 1.3.1 | **Libre** (`blocksList[133] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **134** | `stairsWoodSpruce` | `Block.java:429` | `new BlockStairs(134, planks, 1)` | `"stairsWoodSpruce"` | 1.3.1 | **Libre** (`blocksList[134] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **135** | `stairsWoodBirch` | `Block.java:430` | `new BlockStairs(135, planks, 2)` | `"stairsWoodBirch"` | 1.3.1 | **Libre** (`blocksList[135] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **136** | `stairsWoodJungle` | `Block.java:431` | `new BlockStairs(136, planks, 3)` | `"stairsWoodJungle"` | 1.3.1 | **Libre** (`blocksList[136] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **137** | `commandBlock` | `Block.java:432` | `new BlockCommandBlock(137)` | `"commandBlock"` | 1.4.2 | **Libre** (`blocksList[137] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **138** | `beacon` | `Block.java:433` | `(BlockBeacon)(new BlockBeacon(138))` | `"beacon"` | 1.4.2 | **Libre** (`blocksList[138] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **139** | `cobblestoneWall` | `Block.java:434` | `new BlockWall(139, cobblestone)` | `"cobbleWall"` | 1.4.2 | **Libre** (`blocksList[139] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **140** | `flowerPot` | `Block.java:435` | `new BlockFlowerPot(140)` | `"flowerPot"` | 1.4.2 | **Libre** (`blocksList[140] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **141** | `carrot` | `Block.java:436` | `new BlockCarrot(141)` | `"carrots"` | 1.4.2 | **Libre** (`blocksList[141] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **142** | `potato` | `Block.java:437` | `new BlockPotato(142)` | `"potatoes"` | 1.4.2 | **Libre** (`blocksList[142] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **143** | `woodenButton` | `Block.java:438` | `new BlockButtonWood(143)` | `"button"` | 1.4.2 | **Libre** (`blocksList[143] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **144** | `skull` | `Block.java:439` | `new BlockSkull(144)` | `"skull"` | 1.4.2 | **Libre** (`blocksList[144] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **145** | `anvil` | `Block.java:440` | `new BlockAnvil(145)` | `"anvil"` | 1.4.2 | **Libre** (`blocksList[145] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **146** | `chestTrapped` | `Block.java:441` | `new BlockChest(146, 1)` | `"chestTrap"` | 1.5 | **Libre** (`blocksList[146] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **147** | `pressurePlateGold` | `Block.java:442-445` | `new BlockPressurePlateWeighted(147, "blockGold", Material.iron, 64)` | `"weightedPlate_light"` | 1.5 | **Libre** (`blocksList[147] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **148** | `pressurePlateIron` | `Block.java:446-449` | `new BlockPressurePlateWeighted(148, "blockIron", Material.iron, 640)` | `"weightedPlate_heavy"` | 1.5 | **Libre** (`blocksList[148] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **149** | `redstoneComparatorIdle` | `Block.java:450-454` | `(BlockComparator)(new BlockComparator(149, false))` | `"comparator"` | 1.5 | **Libre** (`blocksList[149] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **150** | `redstoneComparatorActive` | `Block.java:455-460` | `(BlockComparator)(new BlockComparator(150, true))` | `"comparator"` | 1.5 | **Libre** (`blocksList[150] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **151** | `daylightSensor` | `Block.java:461-464` | `(BlockDaylightDetector)(new BlockDaylightDetector(151))` | `"daylightDetector"` | 1.5 | **Libre** (`blocksList[151] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **152** | `blockRedstone` | `Block.java:465-469` | `new BlockPoweredOre(152)` | `"blockRedstone"` | 1.5 | **Libre** (`blocksList[152] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **153** | `oreNetherQuartz` | `Block.java:470-474` | `new BlockOre(153)` | `"netherquartz"` | 1.5 | **Libre** (`blocksList[153] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **154** | `hopperBlock` | `Block.java:475-479` | `(BlockHopper)(new BlockHopper(154))` | `"hopper"` | 1.5 | **Libre** (`blocksList[154] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **155** | `blockNetherQuartz` | `Block.java:480` | `new BlockQuartz(155)` | `"quartzBlock"` | 1.5 | **Libre** (`blocksList[155] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **156** | `stairsNetherQuartz` | `Block.java:481` | `new BlockStairs(156, blockNetherQuartz, 0)` | `"stairsQuartz"` | 1.5 | **Libre** (`blocksList[156] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **157** | `railActivator` | `Block.java:482` | `new BlockRailPowered(157)` | `"activatorRail"` | 1.5 | **Libre** (`blocksList[157] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |
| **158** | `dropper` | `Block.java:483` | `new BlockDropper(158)` | `"dropper"` | 1.5 | **Libre** (`blocksList[158] == nullptr` en `src/net/minecraft/src/Block.cpp:130`) |

### 1.1. Bloques Existentes (IDs 1 a 124) que Cambian de Nombre de Campo o Clase en 1.5.2

| ID | OptiCraft C++ (`archivo:línea`) | `src152/Block.java` | Detalle del cambio |
|---:|---|---|---|
| **25** | `musicBlock` (`src/net/minecraft/src/Block.h:235`, `src/net/minecraft/src/Block.cpp:169,849`) | `music` (`Block.java:86`) | En Java 1.2.5 y 1.5.2 se llama `music`; OptiCraft lo renombró a `musicBlock`. |
| **26** | `blockBed` (`src/net/minecraft/src/Block.h:236`, `src/net/minecraft/src/Block.cpp:170,850`) | `bed` (`Block.java:87`) | En Java 1.2.5 y 1.5.2 se llama `bed`; OptiCraft lo renombró a `blockBed`. |
| **27** | `railPowered` (`BlockRail`, `src/net/minecraft/src/Block.h:237`, `src/net/minecraft/src/Block.cpp:851`) | `railPowered` (`BlockRailPowered`, `Block.java:88`) | Cambia de `new BlockRail(27, 179, true)` a `new BlockRailPowered(27)` (hereda de `BlockRailBase`). |
| **28** | `railDetector` (`BlockDetectorRail`, `src/net/minecraft/src/Block.h:238`, `src/net/minecraft/src/Block.cpp:852`) | `railDetector` (`BlockDetectorRail`, `Block.java:89`) | `BlockDetectorRail` pasa a heredar de `BlockRailBase`. |
| **42** | `blockSteel` (`src/net/minecraft/src/Block.h:252`, `src/net/minecraft/src/Block.cpp:186,866`) | `blockIron` (`Block.java:123`) | Renombrado de `blockSteel` a `blockIron` (mantiene `"blockIron"` como nombre interno). |
| **43** | `stairDouble` (`Block*`, `src/net/minecraft/src/Block.h:253`, `src/net/minecraft/src/Block.cpp:187,867`) | `stoneDoubleSlab` (`BlockHalfSlab`, `Block.java:128`) | Renombrado a `stoneDoubleSlab` y tipado como `BlockHalfSlab*`. |
| **44** | `stairSingle` (`Block*`, `src/net/minecraft/src/Block.h:254`, `src/net/minecraft/src/Block.cpp:188,868`) | `stoneSingleSlab` (`BlockHalfSlab`, `Block.java:133`) | Renombrado a `stoneSingleSlab` y tipado como `BlockHalfSlab*`. |
| **50** | `torchWood` (`src/net/minecraft/src/Block.h:260`, `src/net/minecraft/src/Block.cpp:874`) | `torchWood` (`Block.java:161`) | Pasa a usar `soundWoodFootstep` en vez de `soundStoneFootstep` (`src/net/minecraft/src/Block.cpp:874`). |
| **53** | `stairCompactPlanks` (`src/net/minecraft/src/Block.h:263`, `src/net/minecraft/src/Block.cpp:197,877`) | `stairsWoodOak` (`Block.java:173`) | Renombrado a `stairsWoodOak`; constructor recibe `(53, planks, 0)`. |
| **54** | `chest` (`Block*`, `src/net/minecraft/src/Block.h:264`, `src/net/minecraft/src/Block.cpp:198,878`) | `chest` (`BlockChest`, `Block.java:174`) | Tipado como `BlockChest*` y recibe `chestType` (`0` = normal, `1` = trapped). |
| **61** | `stoneOvenIdle` (`src/net/minecraft/src/Block.h:271`, `src/net/minecraft/src/Block.cpp:205,885`) | `furnaceIdle` (`Block.java:193`) | Renombrado de `stoneOvenIdle` a `furnaceIdle`. |
| **62** | `stoneOvenActive` (`src/net/minecraft/src/Block.h:272`, `src/net/minecraft/src/Block.cpp:206,886`) | `furnaceBurning` (`Block.java:198`) | Renombrado de `stoneOvenActive` a `furnaceBurning`. |
| **65** | `ladder` (`src/net/minecraft/src/Block.h:275`, `src/net/minecraft/src/Block.cpp:889`) | `ladder` (`Block.java:209`) | Pasa a usar `soundLadderFootstep` (`Block.java:18,211`). |
| **66** | `rail` (`src/net/minecraft/src/Block.h:276`, `src/net/minecraft/src/Block.cpp:890`) | `rail` (`Block.java:214`) | `BlockRail` pasa a heredar de `BlockRailBase`. |
| **67** | `stairCompactCobblestone` (`src/net/minecraft/src/Block.h:277`, `src/net/minecraft/src/Block.cpp:211,891`) | `stairsCobblestone` (`Block.java:215`) | Renombrado a `stairsCobblestone`; constructor `new BlockStairs(67, cobblestone, 0)`. |
| **71** | `doorSteel` (`src/net/minecraft/src/Block.h:281`, `src/net/minecraft/src/Block.cpp:215,895`) | `doorIron` (`Block.java:226`) | Renombrado de `doorSteel` a `doorIron`. |
| **77** | `button` (`src/net/minecraft/src/Block.h:287`, `src/net/minecraft/src/Block.cpp:221,901`) | `stoneButton` (`BlockButtonStone`, `Block.java:257`) | Renombrado a `stoneButton` e instancia `BlockButtonStone` (separado de `woodenButton` ID 143). |
| **78** | `snow` (`src/net/minecraft/src/Block.h:288`, `src/net/minecraft/src/Block.cpp:902`) | `snow` (`Block.java:258`) | Pasa a usar `soundSnowFootstep` (`Block.java:17,260`). |
| **93 / 94** | `redstoneRepeaterIdle` / `Active` (`src/net/minecraft/src/Block.h:303-304`, `src/net/minecraft/src/Block.cpp:917-918`) | `redstoneRepeaterIdle` / `Active` (`Block.java:301-312`) | Tipados como `BlockRedstoneRepeater` (que hereda de `BlockRedstoneLogic`). |
| **97** | `silverfish` (`src/net/minecraft/src/Block.h:307`, `src/net/minecraft/src/Block.cpp:921`) | `silverfish` (`Block.java:315`) | Añade `.setUnlocalizedName("monsterStoneEgg")`. |
| **108 / 109 / 114** | `stairsBrick`, `stairsStoneBrickSmooth`, `stairsNetherBrick` (`src/net/minecraft/src/Block.h:318-319,324`, `src/net/minecraft/src/Block.cpp:932-933,938`) | `stairsBrick`, `stairsStoneBrick`, `stairsNetherBrick` (`Block.java:347-348,360`) | ID 109 renombrado a `stairsStoneBrick`; constructores reciben `(id, modelBlock, 0)`. |
| **116** | `enchantmentTable` (`src/net/minecraft/src/Block.h:326`, `src/net/minecraft/src/Block.cpp:940`) | `enchantmentTable` (`Block.java:362`) | Dureza reducida de `5.0F` (`src/net/minecraft/src/Block.cpp:940`) a `2.0F` (`Block.java:363`). |

---

## 2. Ítems Nuevos Añadidos (1.2.5 → 1.5.2): IDs Base 130–152 y 2011 (IDs Finales 386–408 y 2267)

El constructor `Item(int par1)` suma `256` al ID base (`this->shiftedIndex = 256 + par1` en `src/net/minecraft/src/Item.cpp:210` / `this.itemID = 256 + var1` en `mcp-diff/src152/net/minecraft/src/Item.java:215`).
En 1.2.5 y en OptiCraft (`src/net/minecraft/src/Item.cpp:603-614`), el último ítem regular es `fireballCharge` (ID base **129**, ID final **385**) y el último disco es `record11` (ID base **2010**, ID final **2266**).
Entre 1.3.1 y 1.5 se añadieron **24 ítems nuevos** (**23 regulares** en IDs base `130..152` y **1 disco** en ID base `2011`).

| ID Base (`Item.java`) | ID Final (`+256` en `itemsList`) | Campo en `Item.java` (1.5.2) | Línea en `src152/Item.java` | Clase Java e instanciación en 1.5.2 | `unlocalizedName` | Versión | Estado en OptiCraft C++ (`archivo:línea`) |
|---:|---:|---|---|---|---|---|---|
| **130** | **386** | `writableBook` | `Item.java:169` | `new ItemWritableBook(130)` | `"writingBook"` | 1.3.1 | **Libre** (`src/net/minecraft/src/Item.cpp:603` termina en base 129 / final 385; `itemsList[386] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **131** | **387** | `writtenBook` | `Item.java:170` | `new ItemEditableBook(131)` | `"writtenBook"` | 1.3.1 | **Libre** (`itemsList[387] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **132** | **388** | `emerald` | `Item.java:171` | `new Item(132)` | `"emerald"` | 1.3.1 | **Libre** (`itemsList[388] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **133** | **389** | `itemFrame` | `Item.java:172` | `new ItemHangingEntity(133, EntityItemFrame.class)` | `"frame"` | 1.4.2 | **Libre** (`itemsList[389] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **134** | **390** | `flowerPot` | `Item.java:173` | `new ItemReed(134, Block.flowerPot)` | `"flowerPot"` | 1.4.2 | **Libre** (`itemsList[390] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **135** | **391** | `carrot` | `Item.java:174` | `new ItemSeedFood(135, 4, 0.6F, Block.carrot.blockID, Block.tilledField.blockID)` | `"carrots"` | 1.4.2 | **Libre** (`itemsList[391] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **136** | **392** | `potato` | `Item.java:175` | `new ItemSeedFood(136, 1, 0.3F, Block.potato.blockID, Block.tilledField.blockID)` | `"potato"` | 1.4.2 | **Libre** (`itemsList[392] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **137** | **393** | `bakedPotato` | `Item.java:176` | `new ItemFood(137, 6, 0.6F, false)` | `"potatoBaked"` | 1.4.2 | **Libre** (`itemsList[393] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **138** | **394** | `poisonousPotato` | `Item.java:177` | `(new ItemFood(138, 2, 0.3F, false)).setPotionEffect(Potion.poison.id, 5, 0, 0.6F)` | `"potatoPoisonous"` | 1.4.2 | **Libre** (`itemsList[394] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **139** | **395** | `emptyMap` | `Item.java:178` | `(ItemEmptyMap)(new ItemEmptyMap(139))` | `"emptyMap"` | 1.4.2 | **Libre** (`itemsList[395] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **140** | **396** | `goldenCarrot` | `Item.java:179` | `(new ItemFood(140, 6, 1.2F, false)).setPotionEffect(PotionHelper.goldenCarrotEffect)` | `"carrotGolden"` | 1.4.2 | **Libre** (`itemsList[396] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **141** | **397** | `skull` | `Item.java:180` | `new ItemSkull(141)` | `"skull"` | 1.4.2 | **Libre** (`itemsList[397] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **142** | **398** | `carrotOnAStick` | `Item.java:181` | `new ItemCarrotOnAStick(142)` | `"carrotOnAStick"` | 1.4.2 | **Libre** (`itemsList[398] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **143** | **399** | `netherStar` | `Item.java:182` | `new ItemSimpleFoiled(143)` | `"netherStar"` | 1.4.2 | **Libre** (`itemsList[399] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **144** | **400** | `pumpkinPie` | `Item.java:183` | `new ItemFood(144, 8, 0.3F, false)` | `"pumpkinPie"` | 1.4.2 | **Libre** (`itemsList[400] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **145** | **401** | `firework` | `Item.java:184` | `new ItemFirework(145)` | `"fireworks"` | 1.4.6 | **Libre** (`itemsList[401] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **146** | **402** | `fireworkCharge` | `Item.java:185` | `new ItemFireworkCharge(146)` | `"fireworksCharge"` | 1.4.6 | **Libre** (`itemsList[402] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **147** | **403** | `enchantedBook` | `Item.java:186` | `(ItemEnchantedBook)(new ItemEnchantedBook(147)).setMaxStackSize(1)` | `"enchantedBook"` | 1.4.6 | **Libre** (`itemsList[403] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **148** | **404** | `comparator` | `Item.java:187` | `new ItemReed(148, Block.redstoneComparatorIdle)` | `"comparator"` | 1.5 | **Libre** (`itemsList[404] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **149** | **405** | `netherrackBrick` | `Item.java:188` | `new Item(149)` | `"netherbrick"` | 1.5 | **Libre** (`itemsList[405] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **150** | **406** | `netherQuartz` | `Item.java:189` | `new Item(150)` | `"netherquartz"` | 1.5 | **Libre** (`itemsList[406] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **151** | **407** | `minecartTnt` | `Item.java:190` | `new ItemMinecart(151, 3)` | `"minecartTnt"` | 1.5 | **Libre** (`itemsList[407] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **152** | **408** | `minecartHopper` | `Item.java:191` | `new ItemMinecart(152, 5)` | `"minecartHopper"` | 1.5 | **Libre** (`itemsList[408] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |
| **2011** | **2267** | `recordWait` | `Item.java:203` | `new ItemRecord(2011, "wait")` | `"record"` | 1.4.4 | **Libre** (`src/net/minecraft/src/Item.cpp:614` termina en `record11` = 2010/2266; `itemsList[2267] == nullptr` en `src/net/minecraft/src/Item.cpp:59`) |

### 2.1. Ítems Existentes (IDs Base 0 a 129) que Cambian en 1.5.2

| ID Base | ID Final | OptiCraft C++ (`archivo:línea`) | `src152/Item.java` | Detalle del cambio |
|---:|---:|---|---|---|
| **0, 1, 2, 11, 36** | **256..258, 267, 292** | `shovelSteel`, `pickaxeSteel`, `axeSteel`, `swordSteel`, `hoeSteel` (`src/net/minecraft/src/Item.h:90-92,101,126`, `src/net/minecraft/src/Item.cpp:474-476,485,510`) | `shovelIron`, `pickaxeIron`, `axeIron`, `swordIron`, `hoeIron` (`Item.java:10-12,21,49`) | Renombrados de `*Steel` a `*Iron`. |
| **8** | **264** | `diamond` con `.setItemName("emerald")` (`src/net/minecraft/src/Item.h:98`, `src/net/minecraft/src/Item.cpp:482`) | `diamond` con `.setUnlocalizedName("diamond")` (`Item.java:18`) | En 1.2.5 el diamante usaba la clave `"emerald"`. En 1.5.2 `"diamond"` pasa a `Item.diamond` (ID 264) y `"emerald"` pasa al nuevo `Item.emerald` (ID 388). |
| **31** | **287** | `silk` (`new Item(31)`, `src/net/minecraft/src/Item.h:121`, `src/net/minecraft/src/Item.cpp:505`) | `silk` (`new ItemReed(31, Block.tripWire)`, `Item.java:41`) | Cambia de `Item` genérico a `ItemReed(31, Block.tripWire)` para colocar cuerda trampa (`Block.tripWire`, ID 132). |
| **50..53** | **306..309** | `helmetSteel`, `plateSteel`, `legsSteel`, `bootsSteel` (`src/net/minecraft/src/Item.h:140-143`, `src/net/minecraft/src/Item.cpp:524-527`) | `helmetIron`, `plateIron`, `legsIron`, `bootsIron` (`Item.java:63-66`) | Renombrados de `*Steel` a `*Iron` y tipados como `ItemArmor*`. |
| **65** | **321** | `painting` (`new ItemPainting(65)`, `src/net/minecraft/src/Item.h:155`, `src/net/minecraft/src/Item.cpp:539`) | `painting` (`new ItemHangingEntity(65, EntityPainting.class)`, `Item.java:78`) | `ItemPainting` se reemplaza por `ItemHangingEntity` (compartido con `itemFrame` ID base 133). |
| **74** | **330** | `doorSteel` (`src/net/minecraft/src/Item.h:164`, `src/net/minecraft/src/Item.cpp:548`) | `doorIron` (`Item.java:90`) | Renombrado de `doorSteel` a `doorIron`. |
| **84** | **340** | `book` (`new Item(84)`, `src/net/minecraft/src/Item.h:174`, `src/net/minecraft/src/Item.cpp:558`) | `book` (`new ItemBook(84)`, `Item.java:100`) | Cambia a `ItemBook` para permitir encantar libros en la mesa de encantamientos. |

---

## 3. Subtipos / Metadata Nuevos y `ItemBlock` Especializados

En `src/net/minecraft/src/Block.cpp:950-962`, OptiCraft registra las clases `ItemBlock` de 1.2.5. En **1.5.2** (`mcp-diff/src152/net/minecraft/src/Block.java:1120-1149`) se actualizan y añaden:

1. **`ItemMultiTextureTile`** reemplaza a `ItemMetadata` (`src/net/minecraft/src/ItemMetadata.h:6`) e `ItemSapling` (`src/net/minecraft/src/ItemSapling.h:5`):
   - **ID 17 (`wood`)**, **ID 5 (`planks`)**, **ID 97 (`silverfish`)**, **ID 98 (`stoneBrick`)**, **ID 24 (`sandStone`)**, **ID 155 (`blockNetherQuartz`)**, **ID 6 (`sapling`)** y **ID 139 (`cobblestoneWall`)** (`Block.java:1122-1147`).
2. **`ItemSlab`** (`src/net/minecraft/src/ItemSlab.h:5`) recibe ahora referencias a `(singleSlab, doubleSlab, isDouble)` para:
   - **ID 44 (`stoneSingleSlab`)** e **ID 43 (`stoneDoubleSlab`)** (`Block.java:1132-1135`), con **2 subtipos nuevos**: `meta 6` (`"netherBrick"`) y `meta 7` (`"quartz"`) en `BlockStep.java:7`.
   - **ID 126 (`woodSingleSlab`)** e **ID 125 (`woodDoubleSlab`)** (`Block.java:1136-1137`), con subtipos `0..3` (`oak`, `spruce`, `birch`, `jungle`) en `BlockWoodSlab.java:7`.
3. **`ItemSnow`** (`Block.java:1142`) para **ID 78 (`snow`)**: permite apilar de 1 a 8 capas (`meta 0..7`).
4. **`ItemAnvilBlock`** (`Block.java:1148`) para **ID 145 (`anvil`)**: gestiona los 3 estados de daño (`0` intacto, `1` ligeramente dañado, `2` muy dañado).
5. **Huevos generadores (`ItemMonsterPlacer`, ID 383)** en `src/net/minecraft/src/EntityList.cpp:170-217` vs `mcp-diff/src152/net/minecraft/src/EntityList.java:115-170`:
   - Añade huevo **ID 65** (`EntityBat`) y huevo **ID 66** (`EntityWitch`).
   - Añade IDs de entidad sin huevo: **18** (`EntityItemFrame`), **19** (`EntityWitherSkull`), **22** (`EntityFireworkRocket`), **42..47** (subclases de `EntityMinecart`) y **64** (`EntityWither`).

---

## 4. Límites de Arrays, Máscaras y Puntos en C++ Afectados por Nuevos IDs

1. **Tamaño de registros `Block` e `Item`**:
   - `Block::BLOCK_REGISTRY_SIZE` es `256` en consolas (`PS2_PLATFORM || WII_PLATFORM || XBOX_PLATFORM`) y `4096` en PC (`src/net/minecraft/src/Block.h:191-196`). Como el ID máximo de bloque en 1.5.2 es **158** (`< 256`), **no hace falta ampliar `BLOCK_REGISTRY_SIZE` en consolas**.
   - `Item::ITEM_LIST_SIZE` es `4096` en consolas y `32000` en PC (`src/net/minecraft/src/Item.h:82-87`). Como el ID máximo de ítem en 1.5.2 es **2267** (`< 4096`), **no hace falta ampliar `ITEM_LIST_SIZE`**.
   - En `Item::cleanup()` (`src/net/minecraft/src/Item.cpp:629-739`), al declarar los 24 nuevos punteros estáticos en `Item`, deben anularse también al cerrar.
2. **Almacenamiento de bloques en chunks (`ExtendedBlockStorage`)**:
   - `src/net/minecraft/src/ExtendedBlockStorage.cpp:15-19` verifica `static_assert(Block::BLOCK_REGISTRY_SIZE == 256)` en consolas y guarda IDs en `blockLSBArray` (`uint8_t[4096]`). Todos los bloques `125..158` caben en 1 byte sin necesitar `blockMSBArray`.
3. **Paquetes de red con máscaras/tamaños de ID de bloque**:
   - `src/net/minecraft/src/Packet53BlockChange.cpp:20,29`: lee/escribe `type` como 1 byte (`uint8_t`, 11 bytes totales); en 1.5.2 (`Packet53BlockChange.java:31,40,48`) pasa a `short` (2 bytes, 12 bytes totales).
   - `src/net/minecraft/src/Packet54PlayNoteBlock.cpp:14-31`: en 1.5.2 (`Packet54PlayNoteBlock.java:12,33,43`) añade `blockId` (`readShort() & 4095`).
   - `src/net/minecraft/src/NetClientHandler.cpp:643`: ya decodifica `(blockData & 0x0fff) >> 4`, compatible con 1.5.2.
4. **Estadísticas, Hornos y Mods internos**:
   - `src/net/minecraft/src/StatList.cpp:203-216`: añadir `replaceSimilarBlocks(statArray, Block::woodDoubleSlab->blockID, Block::woodSingleSlab->blockID)` (`StatList.java:149-150`).
   - `src/net/minecraft/src/TileEntityFurnace.cpp:229-247`: dar `150` ticks a `Block::woodSingleSlab->blockID` (`TileEntityFurnace.java:286-288`, ya que el chequeo genérico `blockMaterial == Material::wood` en `src/net/minecraft/src/TileEntityFurnace.cpp:234` le daría 300 ticks) y `200` ticks a herramientas/espadas/azadas de madera (`TileEntityFurnace.java:294-296`).
   - `src/net/minecraft/src/ContainerCreative.cpp:25-256` y `src/mods/toomanyitems/TooManyItems.cpp:80-149`: actualizar listas de bloques/subtipos para exponer los nuevos bloques `125..158`, ítems `386..408`, `2267` y ocultar bloques técnicos (`woodDoubleSlab`, `cocoaPlant`, `tripWire`, `carrot`, `potato`, `skull`, `redstoneComparatorIdle/Active`, `flowerPot`).
   - Tablas por plataforma (`src/pc/render/PcLegacyBlockRenderInfo.cpp:13,34`, `src/ps2/render/Ps2BlockRenderInfo.cpp:13,35`, `src/wii/render/WiiBlockRenderInfo.cpp:13,35`, `src/platform/world/PlatformBlockCollisionInfo.cpp:15-16`): indexan hasta `Block::BLOCK_REGISTRY_SIZE`, por lo que admiten `125..158` sin cambios de tamaño.
