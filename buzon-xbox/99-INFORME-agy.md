# Informe Encargo 99 — Dispensador 1.5.2 + Soltador + Bloque de Comandos, Generador de Mobs 1.5.2 + Vagoneta con Generador, Armadura de Cuero Teñida + Caldero 1.5.2 (Solo 1.5.2)

- **Worktree / Rama**: `wt-152-rend` (`port152-erluza`)
- **Commit**: `f310e146` (`f310e146f73e3972d8935c067afc6c8f71e41035`) sobre `a2ec7d1e`
- **Alcance**: Exclusivamente **1.5.2** (sin tocar `wt-opti30`, sin tocar `wt-152-codex`, sin `git push`, sin copiar binarios al launcher).
- **Presets compilados en limpio**: `xbox-release` (`bin\xbox\OptiCraft.iso`) y `xbox-release-exp` (`bin\xbox-exp\OptiCraft.iso`), ambos con 0 errores tras retirar todo código temporal de verificación.
- **Capturas**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\99-capturas\`

---

## 1. Resumen de Implementación

### Bloque 1 — Dispensador 1.5.2 + Soltador + Bloque de Comandos

1. **Dispensador 1.5.2 (`BlockDispenser.cpp`, `TileEntityDispenser.h/.cpp`, `RenderGlobal.cpp`, `ItemBucket.h/.cpp`, `ItemDye.h/.cpp`)**:
   - **Orientación vertical (`0..5`, `DOWN`/`UP`/4 horizontales)**: colocación mediante `BlockPistonBase::determineOrientation`, texturas dedicadas (`221` cara vertical, `46` cara horizontal, `62` laterales cuando apunta arriba/abajo).
   - **Disparo por flanco (bit 8 de metadatos)**: evita disparos repetidos mientras la señal de redstone permanece activa.
   - **Los 16 comportamientos de Java 1.5.2 + suelta por defecto**:
     1. Flechas (`EntityArrow`, `canBePickedUp = 1`, `1002`).
     2. Huevos (`EntityEgg`, `1002`).
     3. Bolas de nieve (`EntitySnowball`, `1002`).
     4. Pociones arrojadizas (`EntityPotion`, `1002`).
     5. Frascos de experiencia (`EntityExpBottle`, `1002`).
     6. Huevos generadores (`ItemMonsterPlacer::spawnCreature`, `1000`).
     7. Cohetes de fuegos artificiales (`EntityFireworkRocket`, `1002`).
     8. Cargas ígneas (`EntitySmallFireball`, `1009`).
     9. Barcos (`EntityBoat` en agua o aire sobre agua, `1000`).
     10. Cubos de agua y lava (`ItemBucket::tryPlaceContainedLiquid` -> cubo vacío, `1000`).
     11. Cubo vacío (recoge fuente de agua/lava adyacente -> llena el cubo o añade al inventario del dispensador, `1000`).
     12. Vagonetas (`EntityMinecart::createMinecart` sobre rieles normales/activadores/detectores/propulsores, `1000`).
     13. Bloque de TNT (`EntityTNTPrimed` encendido con sonido `random.fuse`, `1000`).
     14. Mechero (`flintAndSteel` prende fuego al aire o enciende bloque de TNT y gasta 1 de durabilidad, `1000`/`1001`).
     15. Polvo de hueso (`ItemDye::applyBonemeal` + partículas `2005`, `1000`/`1001`).
     16. Armadura (`ItemArmor` equipa directamente al jugador situado delante si el slot correspondiente está vacío, conservando NBT/color, `1000`).
     - **Por defecto**: expulsa el `EntityItem` hacia `dir` (incluyendo arriba/abajo) + humo direccional `2000` (con soporte vertical `9`/`10` en `RenderGlobal.cpp`) y sonido `1000`/`1001`.
   - **`TileEntityDispenser`**: `getRandomStackFromInventory`, `addItem`, soporte de `CustomName` en NBT (`readFromNBT`/`writeToNBT`).

2. **Soltador (`BlockDropper.cpp`, `TileEntityDropper.cpp`)**:
   - Pulido para orientación 6-vías, flanco de redstone (bit 8), selección aleatoria de slot (`getRandomStackFromInventory`), transferencia a inventarios adyacentes (`IInventory` / `TileEntityHopper`) preservando el stack si el destino está lleno, y suelta con humo `2000` al soltar al aire.

3. **Bloque de Comandos (`BlockCommandBlock.h/.cpp`, `TileEntityCommandBlock.h/.cpp`, `GuiCommandBlock.h/.cpp`, `Block.h/.cpp`, `TileEntity.cpp`, `EntityPlayer.h/.cpp`, `EntityPlayerSP.h/.cpp`, `StringTranslate.cpp`)**:
   - ID `137`, TileEntity `"Control"`, irrompible en supervivencia, **NO** aparece en pestañas de creativo (`setCreativeTab(nullptr)`) y **NO** es crafteable.
   - Redstone activa `trigger()` (con `commandBlockOutput` y `SuccessCount` para salida de comparador `getComparatorInputOverride`).
   - Interfaz `GuiCommandBlock` compatible con **Java UI** y **Legacy UI (`legacyUI`)**, teclado virtual de Xbox (`GuiTextField`), mando, y layout compacto automático para **Pantalla Dividida (J2)** (`height < 170`).
   - Solo editable en creativo (`capabilities.isCreativeMode`).

---

### Bloque 2 — Generador de Mobs 1.5.2 + Vagoneta con Generador

1. **`MobSpawnerBaseLogic.h/.cpp`**:
   - Lógica completa de Java 1.5.2 compartida entre bloque y vagoneta: `EntityId`, `Delay`, `MinSpawnDelay`, `MaxSpawnDelay`, `SpawnCount`, `MaxNearbyEntities`, `RequiredPlayerRange`, `SpawnRange`, `SpawnData` (incluyendo cadena `"Riding"`), `SpawnPotentials` ponderados (`WeightedRandomMinecart`) y entidad en caché para render (`func_98281_h`).
   - Compatibilidad total hacia atrás al cargar NBT legacy (solo `EntityId` + `Delay` conserva defaults 1.5.2: `min=200`, `max=800`, `count=4`, `maxNearby=6`, `reqRange=16`, `spawnRange=4`).

2. **`TileEntityMobSpawner.h/.cpp` y `TileEntityMobSpawnerRenderer.h/.cpp`**:
   - `TileEntityMobSpawner` delega completamente en `TileEntityMobSpawnerLogic`.
   - `TileEntityMobSpawnerRenderer::renderMobSpawnerLogic` compartido con `RenderMinecart`.

3. **`EntityMinecartMobSpawner.h/.cpp`, `EntityList.cpp`, `EntityMinecart.cpp`, `RenderMinecart.cpp`**:
   - Registrada como `"MinecartSpawner"` (ID `47`, `minecartType = 4`), tickea `MobSpawnerBaseLogic`, persiste todo su NBT al guardar/recargar, renderiza la jaula `Block::mobSpawner` + mob girando dentro de la vagoneta.
   - **NO** crafteable y **NO** añadida a pestañas de creativo (solo comando / creativo avanzado / NBT).

---

### Bloque 3 — Armadura de Cuero Teñida + Crafteo de Consola + Caldero 1.5.2

1. **Armadura de cuero teñida (`ItemArmor.h/.cpp`, `RecipesArmorDyes.h/.cpp`, `CraftingManager.cpp`, `RenderLiving.h/.cpp`, `RenderPlayer.h/.cpp`, `ItemRenderer.cpp`, `scripts/port152/build_textures_152.py`)**:
   - Almacenamiento en NBT `display.color` (color por defecto `0xA06540`), `hasColor`, `getColor`, `setColor`, `removeColor`.
   - Render en **2 pasadas** (`func_46058_c`, `func_46057_a`, `getColorFromItemStack`): capa base tintada + capa overlay (`cloth_1_b.png` / `cloth_2_b.png` e iconos overlay en `gui/items.png`) sin tintar (correas, rodilleras y suelas marrones).
   - **Equipado rápido** con clic derecho / LT (`onItemRightClick`), tooltip `"Dyed"` / `"Teñido"` (`addInformation`).
   - **`RecipesArmorDyes`** registrado en `CraftingManager`: mezcla fiel de Java 1.5.2 (uno o varios tintes, re-teñido acumulativo, cualquier pieza de cuero en rejilla 2x2 o 3x3).
   - **Pipeline de texturas (`scripts/port152/build_textures_152.py`)**: eliminada la tintura marrón pre-horneada en `cloth_1.png`, `cloth_2.png` y celdas de cuero de `gui/items.png` para usar las texturas en escala de grises originales de Java 1.5.2 y que el teñido en tiempo real muestre colores vivos y fieles.
   - **Correcciones de render detectadas durante las pruebas en Xbox**:
     - `ItemRenderer.cpp`: bucle de 2 pasadas (`0..1`) aplicando `getColorFromItemStack` en ambas pasadas para que la armadura teñida en mano muestre su color real y su overlay.
     - `RenderLiving.cpp` / `RenderPlayer.cpp`: `ModelBase::isChild` se inicializaba a `true` por defecto y `RenderLiving::doRenderLiving` solo sincronizaba `renderPassModel->isChild` antes del bucle de pasadas, dejando `modelArmorChestplate->isChild == true` (casco y pechera encogidos a tamaño bebé dentro del torso/pierna). Sincronizado `onGround`/`isRiding`/`isChild` dentro del bucle de pasadas y en `RenderPlayer::setArmorModel` — ahora casco, pechera, pantalones y botas renderizan a escala y posición exactas.

2. **Crafteo de Consola (`legacy/XboxCraftingScreen.h/.cpp`)**:
   - Pestaña de **Combate/Armadura** habilitada tanto en **2x2** como en **3x3** con entradas dinámicas de teñido de armadura de cuero (`addDynamicArmorDyeEntries`, `refreshArmorDyePreviews`) según los tintes presentes en el inventario, resolviendo la receta real a través de `InventoryCrafting` -> `RecipesArmorDyes`.

3. **Caldero 1.5.2 (`BlockCauldron.h/.cpp`, `World.cpp`)**:
   - Niveles de agua `0..3`, llenado con cubo de agua, llenado de frascos de cristal (`glassBottle` -> `potion` agua), **lavado de armadura de cuero teñida** (`removeColor` + decremento de nivel de agua), llenado con lluvia (`fillWithRain` enlazado desde el tick de lluvia en `World.cpp`), y salida de comparador `getComparatorInputOverride` (`0..3`).
   - **Caldero con lava**: **NO existe en Java 1.5.2** (fue añadido en versiones muy posteriores/Bedrock) y **NO se ha inventado**, manteniéndose estrictamente fiel a Java 1.5.2.

---

## 2. Resultados de Pruebas en `xemu` (64 MB RAM)

### A. Prueba completa en mundo (`bin\xbox-exp\OptiCraft.iso`, mundo superplano nuevo `probe99_v5`)

| Prueba | Resultado | Evidencia en log |
|---|---|---|
| Texturas verticales y horizontales de Dispensador | **OK** | `DISP_TEX up=221(exp 221) down=221(exp 221) sideWhenUp=62(exp 62) horiz=46(exp 46)` |
| Disparo vertical (UP) cohete y flecha | **OK** | `DISP_FIREWORK_UP=1`, `DISP_ARROW_UP=1` |
| 16 comportamientos del Dispensador + suelta por defecto | **OK (16/16 + default)** | `DISP_BEH egg=1 snow=1 pot=1 exp=1 pig=1 fb=1 boat=1 wPlace=1 bEmpty=1 wScoop=1 bWater=1 lava=1 cart=1 tnt=1 fire=1 fDmg=1 bone=7 armor=1 drop=1 rem=1` |
| Soltador -> Tolva (conserva resto) | **OK** | `DROPPER_TO_HOPPER=1` |
| Bloque de Comandos (fuera de creativo, `/time set 0`, `/give @p 264 3`, comparador) | **OK** | `CMDBLOCK creativeTab=0 timeSet0=1 give3Dia=1 compOut=1` |
| `GuiCommandBlock` en Java UI y Legacy UI + teclado Xbox | **OK** | Capturas `02_cmdblock_gui_java.png` y `03_cmdblock_gui_legacy.png` |
| Generador NBT legacy compatible (defaults 1.5.2) | **OK** | `SPAWNER_LEGACY mob=Zombie delay=42 minD=200 maxD=800 cnt=4 maxNear=6 rng=4` |
| Vagoneta con Generador (`MinecartSpawner`, tipo 4, no creativo) | **OK** | `MINECART_SPAWNER type=4 idStr=MinecartSpawner` |
| 4 generadores activos simultáneos + vagoneta con generador (`SpawnData` oveja roja + `SpawnPotentials`) | **OK** | `SPAWNERS_4_PERF elapsedMs=4915 totalMobs=19 redSheepSpawnData=5`, **51.0 FPS**, `freeKB: 38164 -> 36276` |
| Teñido de cuero (4 piezas, 3 colores + mezcla Amarillo+Rojo, tooltip) | **OK** | `DYED_CRAFT helm=0xcc4c4c chest=0x3366cc legs=0x7fcc19 boots=0xd8983f tip=Dyed` |
| Caldero 1.5.2 (nivel 3, comparador, lavar cuero -> `0xa06540`, botella 2->1, lluvia 1->2) | **OK** | `CAULDRON_INIT level=3 comp=3`; `CAULDRON_WASH before=1 after=0 color=0xa06540 lvl=2 comp=2 afterBottle=1 afterRain=2` |
| Crafteo de Consola (`XboxCraftingScreen` 2x2 pestaña Armadura/Combate) | **OK** | Captura `06_xbox_crafting_armor_dye.png` (casco de cuero + tinte magenta -> casco magenta con correa marrón) |
| Pantalla Dividida (J2) abriendo `GuiCommandBlock` | **OK** | `SPLITSCREEN active=1 p2=1 p2CmdGui=1 freeKB=38260` (sin crash, entrada y salida limpias) |
| Guardado y recarga de mundo (persistencia íntegra de NBT) | **OK** | `RELOAD disp=(VertDisp99,dir=1) hpDia=1 cb=('/give @p 264 3',succ=1) sp=(Sheep,25,40,2) cartSp=1 armor=(0xcc4c4c,0x3366cc) cauld=2 freeKB_final=38260` |

### B. Telemetría, FPU y Arranque Limpio (sin sonda)

- **FPU x87**: `cw=027f double53=1` en todas las muestras (`frame=1, 301, 601, 901, 1201, 1501, 1801`).
- **Errores**: `0` crashes, `0` `[X87]`, `0` `NaN`.
- **Memoria (64 MB Xbox)**:
  - Arranque limpio (ISO final sin sonda): `freeKB = 45588 KB` (`frame=1`, `double53=1`).
  - Durante sesión completa en mundo: inicio `40512 KB` -> 4 spawners activos `36276 KB` -> tras recargar mundo `38260 KB` (sin fugas de memoria).
- **FPS**: `50–56 FPS` en mundo con 4 spawners activos + vagoneta generadora (`~41 FPS` en el pico con 19+ entidades y captura en curso; menú >74 FPS).

---

## 3. Capturas en `buzon-xbox\99-capturas\`

1. `01_dispensers_dropper_cmdblock.png` — Dispensador mirando arriba (`221`) disparando cohete con partículas, Bloque de Comandos (`137`), dispensador horizontal (`46`), Tolva, Soltador y 3 diamantes entregados por `/give @p 264 3`.
2. `02_cmdblock_gui_java.png` — `GuiCommandBlock` en modo **Java UI** con teclado virtual de Xbox.
3. `03_cmdblock_gui_legacy.png` — `GuiCommandBlock` en modo **Legacy UI** (panel 3D clásico) con teclado virtual de Xbox.
4. `04_spawners_and_cart.png` — 4 generadores de mobs activos (incluyendo ovejas rojas desde `SpawnData`/`SpawnPotentials`) + vagoneta con generador sobre rieles con cerdo girando en su interior.
5. `05_dyed_armor_worn_and_hand.png` — Steve en tercera persona llevando gorra de cuero roja (`0xcc4c4c`), túnica azul (`0x3366cc`), pantalones verde lima (`0x7fcc19`) con correas/rodilleras marrones intactas, sosteniendo botas teñidas naranja-dorado (`0xd8983f`), con el caldero y la línea de bloques detrás.
6. `06_xbox_crafting_armor_dye.png` — `XboxCraftingScreen` (2x2) pestaña **Combat** mostrando previsualización y crafteo de gorra de cuero teñida en magenta.

---

## 4. Ficheros Modificados y Creados (51 ficheros)

- **Nuevos (12)**:
  - `src/net/minecraft/src/BlockCommandBlock.h` / `.cpp`
  - `src/net/minecraft/src/TileEntityCommandBlock.h` / `.cpp`
  - `src/net/minecraft/src/GuiCommandBlock.h` / `.cpp`
  - `src/net/minecraft/src/MobSpawnerBaseLogic.h` / `.cpp`
  - `src/net/minecraft/src/EntityMinecartMobSpawner.h` / `.cpp`
  - `src/net/minecraft/src/RecipesArmorDyes.h` / `.cpp`
- **Modificados (39)**:
  - `scripts/port152/build_textures_152.py`
  - `src/net/minecraft/src/Block.h` / `.cpp`
  - `src/net/minecraft/src/BlockCauldron.h` / `.cpp`
  - `src/net/minecraft/src/BlockDispenser.cpp`
  - `src/net/minecraft/src/BlockDropper.cpp`
  - `src/net/minecraft/src/CraftingManager.cpp`
  - `src/net/minecraft/src/EntityList.cpp`
  - `src/net/minecraft/src/EntityMinecart.cpp`
  - `src/net/minecraft/src/EntityPlayer.h` / `.cpp`
  - `src/net/minecraft/src/EntityPlayerSP.h` / `.cpp`
  - `src/net/minecraft/src/ItemArmor.h` / `.cpp`
  - `src/net/minecraft/src/ItemBucket.h` / `.cpp`
  - `src/net/minecraft/src/ItemDye.h` / `.cpp`
  - `src/net/minecraft/src/ItemRenderer.cpp`
  - `src/net/minecraft/src/RenderGlobal.cpp`
  - `src/net/minecraft/src/RenderLiving.h` / `.cpp`
  - `src/net/minecraft/src/RenderMinecart.cpp`
  - `src/net/minecraft/src/RenderPlayer.h` / `.cpp`
  - `src/net/minecraft/src/StringTranslate.cpp`
  - `src/net/minecraft/src/TileEntity.cpp`
  - `src/net/minecraft/src/TileEntityDispenser.h` / `.cpp`
  - `src/net/minecraft/src/TileEntityDropper.cpp`
  - `src/net/minecraft/src/TileEntityMobSpawner.h` / `.cpp`
  - `src/net/minecraft/src/TileEntityMobSpawnerRenderer.h` / `.cpp`
  - `src/net/minecraft/src/World.cpp`
  - `src/net/minecraft/src/legacy/XboxCraftingScreen.h` / `.cpp`
