# Arquitectura de Renderizado de Bloques por Plataforma y Guía para Nuevos Bloques 1.5.2 (`render.md`)

Documentación técnica con referencias `archivo:línea` sobre dónde y cómo dibuja OptiCraft Heritage los bloques en cada plataforma (**Xbox D3D8**, **PlayStation 2 GS/VU0/VU1**, **Nintendo Wii GX**, **PC OpenGL / D3D9 / PC-Legacy**) y qué archivos y funciones hay que tocar para añadir un tipo de bloque nuevo de Minecraft 1.5.2 (como la tolva `BlockHopper` o el yunque `BlockAnvil`).

---

## 1. Arquitectura Común de Renderizado de Bloques (`src/net/minecraft/src/`)

### 1.1 Registro y Tablas Estáticas (`Block.h` / `Block.cpp`)
- **Tamaño del registro (`Block::BLOCK_REGISTRY_SIZE`)**:
  - `256` cuando `defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)` (`src/net/minecraft/src/Block.h:191-193`).
  - `4096` en PC (`src/net/minecraft/src/Block.h:194-195`).
  - Todos los bloques de 1.5.2 llegan hasta el ID `158` (`BlockDropper`), por lo que caben dentro de `256` en todas las consolas.
- **Tablas estáticas indexadas por `blockID`** (pobladas en `Block::Block(...)` en `src/net/minecraft/src/Block.cpp:275-300` y recalculadas tras construir subclases en `Block::initialize()` en `src/net/minecraft/src/Block.cpp:978-996`):
  - `Block::blocksList[i]` (`src/net/minecraft/src/Block.h:196`, `src/net/minecraft/src/Block.cpp:130,292`).
  - `Block::opaqueCubeLookup[i]` (`src/net/minecraft/src/Block.h:198`, `src/net/minecraft/src/Block.cpp:132,294,982`) — cachea el método virtual `isOpaqueCube()` (`src/net/minecraft/src/Block.h:100`, `src/net/minecraft/src/Block.cpp:483-486`).
  - `Block::staticOpaqueCubeLookupSafe[i]` (`src/net/minecraft/src/Block.h:201`, `src/net/minecraft/src/Block.cpp:135,295,983`) — cachea el método virtual `usesStaticOpaqueCubeLookup()` (`src/net/minecraft/src/Block.h:104`, por defecto `true`; `BlockLeaves` lo sobrescribe a `false` porque cambia entre gráficos Fast y Fancy).
  - `Block::usesDefaultFaceCullingLookup[i]` (`src/net/minecraft/src/Block.h:202`, `src/net/minecraft/src/Block.cpp:136,296,984`) — cachea el método virtual `usesDefaultFaceCulling()` (`src/net/minecraft/src/Block.h:83`, por defecto `true`).
    - **REGLA CRÍTICA**: Si una subclase de `Block` sobrescribe `shouldSideBeRendered(IBlockAccess*, int_t, int_t, int_t, int_t)` (`src/net/minecraft/src/Block.h:76`, `src/net/minecraft/src/Block.cpp:428-437`), **DEBE** sobrescribir también `bool usesDefaultFaceCulling() override { return false; }` (`src/net/minecraft/src/Block.h:83`). De lo contrario, el fast-path `canUseDefaultFaceCullingFastPath` (`src/net/minecraft/src/RenderBlocks.cpp:279-302`) y los clasificadores de consola (`src/pc/render/PcLegacyBlockRenderInfo.cpp:47`, `src/ps2/render/Ps2BlockRenderInfo.cpp:46`, `src/wii/render/WiiBlockRenderInfo.cpp:40`) ignorarán `shouldSideBeRendered`.
  - `Block::vineAttachableLookup[i]` (`src/net/minecraft/src/Block.h:200`, `src/net/minecraft/src/Block.cpp:134,985-986`).
  - `Block::isBlockContainer[i]` (`src/net/minecraft/src/Block.h:203`, `src/net/minecraft/src/Block.cpp:137,299`).
  - `Block::lightOpacity[i]` (`src/net/minecraft/src/Block.h:204`, `src/net/minecraft/src/Block.cpp:138,297,987`).
  - `Block::canBlockGrass[i]` (`src/net/minecraft/src/Block.h:205`, `src/net/minecraft/src/Block.cpp:139,298,988`).
  - `Block::lightValue[i]` (`src/net/minecraft/src/Block.h:207`, `src/net/minecraft/src/Block.cpp:140,324-335,1002`).
  - `Block::useNeighborBrightness[i]` (`src/net/minecraft/src/Block.h:209`, `src/net/minecraft/src/Block.cpp:141,990-994`) — `true` si `getRenderType() == 10` (escaleras), `dynamic_cast<BlockStep*> != nullptr` (losas), `blockID == tilledField->blockID` o `canBlockGrass[i]` (`!isOpaqueCube()`).
- **Otros virtuales de `Block` usados por los fast-paths de plataforma**:
  - `usesDefaultWorldTextureLookup()` (`src/net/minecraft/src/Block.h:90`, por defecto `true`): **DEBE** sobrescribirse a `false` si el bloque sobrescribe `getBlockTexture(IBlockAccess*, int_t, int_t, int_t, int_t)` (`src/net/minecraft/src/Block.h:86`, `src/net/minecraft/src/Block.cpp:449-452`). Si se deja en `true`, `resolveBlockFaceTexture` (`src/net/minecraft/src/RenderBlocks.cpp:240`), `src/pc/render/PcLegacyCubeMaterialInfo.cpp:74`, `src/ps2/render/Ps2BlockRenderInfo.cpp:58` y `src/wii/render/WiiBlockRenderInfo.cpp:51` llamarán únicamente a `getBlockTextureFromSideAndMetadata(side, meta)`.
  - `usesDefaultColorMultiplier()` (`src/net/minecraft/src/Block.h:92`, por defecto `true`): **DEBE** sobrescribirse a `false` si el bloque sobrescribe `colorMultiplier(IBlockAccess*, int_t, int_t, int_t)` (`src/net/minecraft/src/Block.h:156`, `src/net/minecraft/src/Block.cpp:718-721`).

### 1.2 Despacho de Geometría en Mundo (`RenderBlocks.h`, `RenderBlocks.cpp`, `RenderBlocks125.cpp`)
- **Punto de entrada por bloque en sección de chunk**: `RenderBlocks::renderBlockByRenderType(Block *block, int_t i, int_t j, int_t k)` (`src/net/minecraft/src/RenderBlocks.cpp:700-829`).
  - En `#if PLATFORM_INCREMENTAL_TERRAIN_BUILD` (Xbox y PC-Legacy, `src/net/minecraft/src/RenderBlocks.cpp:702-710`), obtiene `renderType` y `opaqueCube` desde `pcLegacyGetBlockRenderInfo(block->blockID)` (`src/pc/render/PcLegacyBlockRenderInfo.cpp:69-77`).
  - En `#else` (PC estándar, PS2, Wii, `src/net/minecraft/src/RenderBlocks.cpp:710-718`), consulta `block->getRenderType()` y `block->isOpaqueCube()`.
  - Si `!opaqueCube`, marca `usedAlphaTestedTexture = true` (`src/net/minecraft/src/RenderBlocks.cpp:708-709,716-717`), bandera leída por Wii (`src/wii/minecraft/WorldRendererWii.cpp:303`) para decidir si la sección opaca puede usar early-Z (`GX_SetZCompLoc(GX_ENABLE)`).
  - Llama a `block->setBlockBoundsBasedOnState(blockAccess, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:719`) y ejecuta el `switch (l)` sobre el `renderType`:
    - `case 0`: `renderStandardBlock(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:723`, impl. `2575-2598`).
    - `case 4`: `renderBlockFluids(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:726`, impl. `2031-2285`).
    - `case 13`: `renderBlockCactus(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:729`, impl. `3601-3766`).
    - `case 1`: `renderBlockReed(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:732`, impl. `1284-1315`, planos cruzados).
    - `case 19`: `renderBlockStem(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:735`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:50-119`).
    - `case 23`: `renderBlockLilyPad(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:738`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:259-292`).
    - `case 6`: `renderBlockCrops(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:741`, impl. `1317-1342`).
    - `case 2`: `renderBlockTorch(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:744`, impl. `831-891`).
    - `case 3`: `renderBlockFire(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:747`, impl. `1034-1219`).
    - `case 5`: `renderBlockRedstoneWire(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:750`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:694-942`).
    - `case 8`: `renderBlockLadder(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:753`, impl. `1221-1282`).
    - `case 7`: `renderBlockDoor(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:756`, impl. `1721-1906`).
    - `case 9`: `renderBlockMinecartTrack(static_cast<BlockRail*>(block), i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:759-763`, impl. `1515-1719`).
    - `case 10`: `renderBlockStairs(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:766`, impl. `3847-3948`).
    - `case 11`: `renderBlockFence(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:769`, impl. `3768-3845`).
    - `case 12`: `renderBlockLever(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:772`, impl. `893-1032`).
    - `case 14`: `renderBlockBed(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:775`, impl. `1908-2029`).
    - `case 15`: `renderBlockRepeater(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:778`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:944-976`).
    - `case 16`: `renderPistonBase(block, i, j, k, false)` (`src/net/minecraft/src/RenderBlocks.cpp:781`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:419-467`).
    - `case 17`: `renderPistonExtension(block, i, j, k, true)` (`src/net/minecraft/src/RenderBlocks.cpp:784`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:545-686`).
    - `case 18`: `renderBlockPane(static_cast<BlockPane*>(block), i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:787-791`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:121-257`).
    - `case 20`: `renderBlockVine(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:794`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:294-376`).
    - `case 21`: `renderBlockFenceGate(static_cast<BlockFenceGate*>(block), i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:797-801`, impl. `src/net/minecraft/src/RenderBlocks125.cpp:378-417`).
    - `case 24`: `renderBlockCauldron(static_cast<BlockCauldron*>(block), i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:804-808`, impl. `src/net/minecraft/src/RenderBlocks.cpp:2287-2374`).
    - `case 25`: `renderBlockBrewingStand(static_cast<BlockBrewingStand*>(block), i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:811-815`, impl. `src/net/minecraft/src/RenderBlocks.cpp:2376-2472`).
    - `case 26`: `renderBlockEndPortalFrame(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:818`, impl. `src/net/minecraft/src/RenderBlocks.cpp:2474-2514`).
    - `case 27`: `renderBlockDragonEgg(block, i, j, k)` (`src/net/minecraft/src/RenderBlocks.cpp:821`, impl. `src/net/minecraft/src/RenderBlocks.cpp:2516-2573`).
    - `case 22`: Cofres (`BlockChest`) caen al `default: return false;` (`src/net/minecraft/src/RenderBlocks.cpp:826`) porque se dibujan vía `TileEntityChestRenderer` (`src/net/minecraft/src/TileEntityRenderer.cpp:72`) o el mallado estático de TileEntities en Xbox/PC-Legacy (`src/pc/render/PcLegacyStaticTileEntityPolicy.h:22-37`).
- **Emisores de caras y rotación UV en `RenderBlocks`**:
  - `renderStandardBlock` (`src/net/minecraft/src/RenderBlocks.cpp:2575-2598`) elige entre `renderStandardBlockWithAmbientOcclusion` (`src/net/minecraft/src/RenderBlocks.cpp:2629-3428`) o `renderStandardBlockWithColorMultiplier` (`src/net/minecraft/src/RenderBlocks.cpp:3438-3599`).
  - Las 6 funciones emisoras de cara usan los límites del bloque y los campos de rotación UV (`bottomFaceRotation`, `topFaceRotation`, `eastFaceRotation`, `westFaceRotation`, `northFaceRotation`, `southFaceRotation`, `flipTexture`, definidos en `src/net/minecraft/src/RenderBlocks.h:202-216`):
    - `renderBottomFace` (cara 0, Y-): `src/net/minecraft/src/RenderBlocks.cpp:3950-4054`
    - `renderTopFace` (cara 1, Y+): `src/net/minecraft/src/RenderBlocks.cpp:4056-4160`
    - `renderEastFace` (cara 2, Z-): `src/net/minecraft/src/RenderBlocks.cpp:4162-4268`
    - `renderWestFace` (cara 3, Z+): `src/net/minecraft/src/RenderBlocks.cpp:4270-4376`
    - `renderNorthFace` (cara 4, X-): `src/net/minecraft/src/RenderBlocks.cpp:4378-4484`
    - `renderSouthFace` (cara 5, X+): `src/net/minecraft/src/RenderBlocks.cpp:4486-4592`

### 1.3 Renderizado de Bloques en Inventario, Mano y `EntityItem`
1. **`RenderBlocks::renderItemIn3d(int_t i)`** (`src/net/minecraft/src/RenderBlocks.cpp:4893-4912`):
   - Decide si un `ItemBlock` se dibuja como bloque 3D isométrico en GUI/mano/suelo (`true`) o como sprite 2D plano (`false`).
   - Actualmente devuelve `true` para `renderType` = `0`, `13` (cactus), `10` (stairs), `11` (fence), `22` (chest), `21` (fence gate), `27` (dragon egg) y `16` (piston base).
2. **`RenderBlocks::renderBlockOnInventory(Block *block, int_t metadata, float brightness)`** (`src/net/minecraft/src/RenderBlocks.cpp:4594-4891`) y su alias inline `renderBlockAsItem` (`src/net/minecraft/src/RenderBlocks.h:167`):
   - Dibuja el bloque centrado en `(-0.5, -0.5, -0.5)` llamando a `Tessellator::startDrawingQuads()`, `setNormal(...)` y `draw()` por cara.
   - Si `j == 22` (chest), delega en `ChestItemRenderHelper::instance->func_35609_a(block, metadata, brightness)` (`src/net/minecraft/src/RenderBlocks.cpp:4648-4653`, `src/net/minecraft/src/ChestItemRenderHelper.cpp:11-21`).
   - Invocado desde:
     - **GUI de inventario**: `RenderItem::drawItemIntoGui` (`src/net/minecraft/src/RenderItem.cpp:213-266`, línea `250`).
     - **Ítems tirados en el suelo (`EntityItem`)**: `RenderItem::doRenderItem` (`src/net/minecraft/src/RenderItem.cpp:131-151`, línea `144`).
     - **Mano en 1ª y 3ª persona**: `ItemRenderer::renderItem` (`src/net/minecraft/src/ItemRenderer.cpp:74-101`, línea `94`).

### 1.4 TileEntities con Renderizador Especial (`TileEntityRenderer` / `TileEntitySpecialRenderer`)
- `TileEntityRenderer::TileEntityRenderer()` (`src/net/minecraft/src/TileEntityRenderer.cpp:43-77`) registra en `specialRendererMap` los renderizadores dinámicos indexados por `getClassId()`:
  - `"TileEntitySign"` -> `new TileEntitySignRenderer()` (`src/net/minecraft/src/TileEntityRenderer.cpp:70`)
  - `"TileEntityMobSpawner"` -> `new TileEntityMobSpawnerRenderer()` (`src/net/minecraft/src/TileEntityRenderer.cpp:71`)
  - `"TileEntityChest"` -> `new TileEntityChestRenderer()` (`src/net/minecraft/src/TileEntityRenderer.cpp:72`)
  - `"TileEntityPiston"` -> `new TileEntityRendererPiston()` (`src/net/minecraft/src/TileEntityRenderer.cpp:73`)
  - `"TileEntityEnchantmentTable"` -> `new RenderEnchantmentTable()` (`src/net/minecraft/src/TileEntityRenderer.cpp:74`)
  - `"TileEntityEndPortal"` -> `new RenderEndPortal()` (`src/net/minecraft/src/TileEntityRenderer.cpp:75`)
- Durante el mallado de cada sección (`src/net/minecraft/src/WorldRenderer.cpp:382-389`, `src/pc/minecraft/WorldRendererPcLegacy.cpp:458-466`, `src/ps2/minecraft/WorldRendererPs2.cpp:945-951`, `src/wii/minecraft/WorldRendererWii.cpp:265-271`), si `Block::isBlockContainer[i3]` es `true`, se comprueba `TileEntityRenderer::instance->hasSpecialRenderer(tileentity)` y se añade a `RenderGlobal::tileEntities`, dibujándose cada frame en `RenderGlobal::renderEntities` (`src/net/minecraft/src/RenderGlobal.cpp:1264-1305`).

### 1.5 Daño de Rotura (`drawBlockBreaking`), Selección y Atlas `/terrain.png`
- **Overlay de rotura de bloque**: `RenderGlobal::drawBlockBreaking` (`src/net/minecraft/src/RenderGlobal.cpp:2850-2928`) llama a `globalRenderBlocks->renderBlockUsingTexture(block, blockX, blockY, blockZ, 240 + (int_t)(damagePartialTime * 10.0f))` (`src/net/minecraft/src/RenderGlobal.cpp:2887`), que fuerza `overrideBlockTexture` (`src/net/minecraft/src/RenderBlocks.cpp:685-698`) y llama a `renderBlockByRenderType`.
- **Caja de selección**: `RenderGlobal::drawSelectionBox` (`src/net/minecraft/src/RenderGlobal.cpp:2930-2959`) llama a `block->setBlockBoundsBasedOnState(worldObj, x, y, z)` y `getSelectedBoundingBoxFromPool`.
- **Atlas de texturas `/terrain.png` vs `IconRegister` de 1.5.2**:
  - OptiCraft usa el atlas fijo de 16×16 celdas (256 slots `0..255`) cargado desde `/terrain.png` (`src/net/minecraft/src/RenderBlocks.cpp:3993-4003`, `src/net/minecraft/src/TerrainTextureManager.cpp:28-68`).
  - Todos los formatos compactos de vértices de consola (Xbox `NORMSHORT2`, PS2 `S16` UV + `Ps2TerrainTileRun` de 256 tiles en `src/ps2/render/Ps2TerrainMesh.cpp:134-165`, Wii `GX_TexCoord2s16` a `1/4096` en `src/wii/render/WiiNativeDraw.cpp:90`) asumen un único atlas `/terrain.png` con índices enteros `0..255`. Por tanto, tal como indica `SALTO-1.5.2-21-10.md`, el atlas debe pre-generarse fuera de la consola y cada textura nueva de 1.5.2 asignarse a un índice libre `0..255` devuelto por `getBlockTextureFromSideAndMetadata` / `getBlockTexture`.

---

## 2. Dónde Dibuja los Bloques Cada Plataforma

### 2.1 Xbox (Direct3D 8 — `PLATFORM_XBOX`)
1. **Flags de plataforma (`src/platform/PlatformConfig.h`)**:
   - `PLATFORM_XBOX` vale `1` (`src/platform/PlatformConfig.h:16-17`), activando `PLATFORM_INCREMENTAL_TERRAIN_BUILD = 1` (`src/platform/PlatformConfig.h:236-238`) y `PLATFORM_COMPACT_TERRAIN_VERTICES = 1` (`src/platform/PlatformConfig.h:246-248`), compartiendo el pipeline incremental `PcLegacy*`.
2. **Clasificación de bloques (`src/pc/render/PcLegacyBlockRenderInfo.cpp` y `src/pc/render/PcLegacyCubeMaterialInfo.cpp`)**:
   - `pcLegacyGetBlockRenderInfo(int_t blockId)` (`src/pc/render/PcLegacyBlockRenderInfo.cpp:29-77`) construye `s_renderInfo[Block::BLOCK_REGISTRY_SIZE]` y marca `simpleOpaqueCube = (renderType == 0 && renderPass == 0 && opaqueCube && normalCube && Block::usesDefaultFaceCullingLookup[id] && isUnitBounds(block))` (`src/pc/render/PcLegacyBlockRenderInfo.cpp:47-49`).
   - `pcLegacyGetCubeMaterialInfo(int_t blockId)` (`src/pc/render/PcLegacyCubeMaterialInfo.cpp:62-102`) cachea `texturesByMetaAndSide[16][6]` y `constantWhiteColor` para bloques `simpleOpaqueCube`.
3. **Snapshot de sección y Worksets (`src/pc/render/PcLegacySectionCache.cpp`)**:
   - `PcLegacySectionCache::prepare` (`src/pc/render/PcLegacySectionCache.cpp:106-195`) copia una rejilla de `18×18×18` IDs de bloques (`preloadBlockIds`, `src/pc/render/PcLegacySectionCache.cpp:53-104`), precalcula `exposedFaceMask` (6 bits) para cada `simpleOpaqueCube` (`src/pc/render/PcLegacySectionCache.cpp:130-153`), y separa bloques en `renderWorksets[0]` y `renderWorksets[1]` (`src/pc/render/PcLegacySectionCache.cpp:155-171`).
4. **Mallado incremental (`src/pc/minecraft/WorldRendererPcLegacy.cpp`)**:
   - `WorldRenderer::updateRenderer()` (`src/pc/minecraft/WorldRendererPcLegacy.cpp:81-92`) ejecuta el mallado por pasos en `pcLegacyBuildRendererStep` (`src/pc/minecraft/WorldRendererPcLegacy.cpp:290-508`), planificado por `src/pc/render/PcLegacyMeshScheduler.cpp:1-250`.
   - **Ruta rápida (`simpleOpaqueCube`)**: En `src/pc/minecraft/WorldRendererPcLegacy.cpp:469-475`, llama a `renderblocks.renderSimpleOpaqueCubeLegacy(block, x, y, z, faceMask)` (`src/net/minecraft/src/RenderBlocks.cpp:498-682`). Sin AO ni Connected Textures (`src/net/minecraft/src/RenderBlocks.cpp:544-550`), emite vértices compactos de 24 bytes (`PcLegacyCompactTerrainVertex` en `src/pc/render/PcLegacyCubeMaterialInfo.cpp:18-31`) en `staging->compactPassMeshes[pass]` vía `pcLegacyEmitCompactUnitCubeFace` (`src/pc/render/PcLegacyCubeMaterialInfo.cpp:104-205`).
   - **Ruta general (bloques con `renderType != 0` como tolva/yunque)**: En `src/pc/minecraft/WorldRendererPcLegacy.cpp:476-479`, llama a `renderblocks.renderBlockByRenderType(block, x, y, z)`, que emite al `Tessellator::instance` (`src/net/minecraft/src/Tessellator.cpp:447-510`) y se captura en `staging->passMeshes[pass]` (`src/pc/minecraft/WorldRendererPcLegacy.cpp:485`).
   - **Mallado estático de TileEntities**: En `src/pc/minecraft/WorldRendererPcLegacy.cpp:458-466`, si `pcLegacyCanMeshTileEntityStatically(tileentity)` (`src/pc/render/PcLegacyStaticTileEntityPolicy.h:22-37`, actualmente `TileEntitySign` y `TileEntityChest`) es `true`, se pre-construye en `staging->staticTileEntities` (`src/pc/render/PcLegacyStaticTileEntityMesh.cpp:1-210`) y se dibuja en `glRenderList + 2` (`src/pc/minecraft/WorldRendererPcLegacy.cpp:193-220`).
5. **Publicación y Dibujo en Hardware D3D8 (`src/platform/RenderAPI_D3D8_XBOX.cpp`)**:
   - `WorldRenderer::pcLegacyPublishBuild` (`src/pc/minecraft/WorldRendererPcLegacy.cpp:142-288`) graba `glRenderList + pass` llamando a `renderDrawCaptured(staging.compactPassMeshes[pass])` (`src/pc/minecraft/WorldRendererPcLegacy.cpp:183`) y `renderDrawCaptured(staging.passMeshes[pass])` (`src/pc/minecraft/WorldRendererPcLegacy.cpp:184`).
   - En `src/platform/RenderAPI_D3D8_XBOX.cpp`:
     - `renderDrawInterleaved` (`src/platform/RenderAPI_D3D8_XBOX.cpp:1772-1831`) convierte los vértices (`convertVertices`, `src/platform/RenderAPI_D3D8_XBOX.cpp:783-841`) y los aloja en pools de Vertex Buffers de 512 KB (`allocateVertexRange`, `src/platform/RenderAPI_D3D8_XBOX.cpp:1056-1199`).
     - El formato hardware de terreno es `XboxCompactVertex` de **16 bytes/vértice** (`src/platform/RenderAPI_D3D8_XBOX.cpp:643-649,664-693,732-747`): posición `D3DVSDT_SHORT3` (`* 1024.0f`), lightmap `D3DVSDT_PBYTE2`, color `D3DVSDT_D3DCOLOR` y UV `D3DVSDT_NORMSHORT2` (`* 32767.0f`).
     - Cada frame, `RenderGlobal::renderSortedRenderers` (`src/net/minecraft/src/RenderGlobal.cpp:1720-1745`) -> `RenderList::render()` (`src/net/minecraft/src/RenderList.h:35-51`) -> `renderCallDisplayLists` (`src/platform/RenderAPI_D3D8_XBOX.cpp:1939-1946`) -> `drawRecorded` (`src/platform/RenderAPI_D3D8_XBOX.cpp:1235-1285`) invoca `g_device->SetStreamSource` + `g_device->DrawVertices` (`src/platform/RenderAPI_D3D8_XBOX.cpp:1255-1257`).

---

### 2.2 PlayStation 2 (GS / VU0 / VU1 — `PLATFORM_PS2`)
1. **Clasificación y Elegibilidad (`src/ps2/render/Ps2BlockRenderInfo.cpp`, `src/ps2/render/Ps2TerrainEligibility.cpp`)**:
   - `ps2GetBlockRenderInfo(int_t blockId)` (`src/ps2/render/Ps2BlockRenderInfo.cpp:30-87`) calcula `simpleOpaqueCube` (`src/ps2/render/Ps2BlockRenderInfo.cpp:46-48`) y `staticTextureBySide` (`src/ps2/render/Ps2BlockRenderInfo.cpp:51-70`).
   - `ps2_block_contributes_to_pass` (`src/ps2/render/Ps2TerrainEligibility.cpp:52-66`) filtra bloques por pass y descarta `renderType < 0` o `renderType == 22`.
2. **Greedy Meshing en Pass 0 (`src/ps2/render/Ps2GreedyMesh.cpp`)**:
   - `ps2_is_greedy_cube(Block *block)` (`src/ps2/render/Ps2GreedyMesh.cpp:482-502`) activa el greedy meshing para cubos `simpleOpaqueCube` en `WorldRenderer::ps2BuildRendererStep` (`src/ps2/minecraft/WorldRendererPs2.cpp:736-864`), ejecutando `ps2_greedy_mesh_face_raw` (`src/ps2/render/Ps2GreedyMesh.cpp:636-774`).
3. **Barrido por bloque (`src/ps2/minecraft/WorldRendererPs2.cpp:881-1001`)**:
   - Los bloques no greedy pero `simpleOpaqueCube` usan `ps2Renderblocks.renderSimpleOpaqueCubePs2` (`src/ps2/minecraft/WorldRendererPs2.cpp:984-985`, `src/net/minecraft/src/RenderBlocks.cpp:350-369`).
   - **Cualquier bloque con `renderType != 0` o no opaco** (como tolva o yunque) llama a `ps2Renderblocks.renderBlockByRenderType(block, x, y, z)` (`src/ps2/minecraft/WorldRendererPs2.cpp:1000`).
   - `renderCaptureInterleaved` (`src/platform/RenderAPI_GS_PS2.cpp:582-632`) captura los quads del `Tessellator` en formato crudo de 24 bytes/vértice aplicando la iluminación del lightmap al RGBA con `ps2_lighting_apply_packed_brightness` (`src/platform/RenderAPI_GS_PS2.cpp:627`).
4. **Ordenamiento, Empaquetado VU0 y Dibujo VU1 / VU0 (`src/ps2/render/Ps2TerrainMesh.cpp`, `src/ps2/render/Ps2TerrainSectionDraw.cpp`, `src/ps2/render/Ps2Vu1Terrain.cpp`)**:
   - `renderTerrainCacheSortFaces` (`src/platform/RenderTerrainAPI_GS_PS2.cpp:145-149`) ordena los quads por orientación de cara (`+X, -X, +Y, -Y, +Z, -Z, Other`) y sub-cluster 2×2×2.
   - `ps2_terrain_mesh_build` (`src/ps2/render/Ps2TerrainMesh.cpp:296-441`) empaqueta con **VU0** (`src/ps2/render/Ps2TerrainMesh.cpp:85-103,283`) a posiciones `S16`, UVs `S16` y runs de tiles `Ps2TerrainTileRun` (`src/ps2/render/Ps2TerrainMesh.cpp:134-165`).
   - En cada frame (`src/ps2/minecraft/WorldRendererPs2.cpp:1423-1480` -> `src/ps2/render/Ps2TerrainSectionDraw.cpp:24-800`), los lotes opacos se envían por **DMA Path1 a VU1** (`src/ps2/render/Ps2Vu1Terrain.cpp:200-344`) o se recortan con **VU0/EE** hacia la cola Path3 del GS.

---

### 2.3 Nintendo Wii (GX — `PLATFORM_WII`)
1. **Clasificación de bloques (`src/wii/render/WiiBlockRenderInfo.cpp`)**:
   - `wiiGetBlockRenderInfo(int_t blockId)` (`src/wii/render/WiiBlockRenderInfo.cpp:30-65`) calcula `simpleOpaqueCube` (`src/wii/render/WiiBlockRenderInfo.cpp:39-48`).
2. **Mallado incremental y Early-Z (`src/wii/minecraft/WorldRendererWii.cpp`)**:
   - `wiiBuildRendererStep` (`src/wii/minecraft/WorldRendererWii.cpp:123-445`) usa `renderblocks.renderSimpleOpaqueCubeWii` (`src/wii/minecraft/WorldRendererWii.cpp:285`, `src/net/minecraft/src/RenderBlocks.cpp:373-392`) para cubos opacos simples y `renderblocks.renderBlockByRenderType(block, x, y, z)` (`src/wii/minecraft/WorldRendererWii.cpp:288`) para el resto.
   - Si cualquier bloque tiene `!isOpaqueCube()`, `renderblocks.usedAlphaTestedTexture` activa `wiiBuildNeedsAlphaTest[wiiBuildPass]` (`src/wii/minecraft/WorldRendererWii.cpp:303`), separando en `RenderList::addTerrainRenderer` (`src/net/minecraft/src/RenderList.h:22-33`) los chunks `opaqueEarlyDepthChunks` (`GX_SetZCompLoc(GX_ENABLE)` en `src/wii/render/WiiNativeDraw.cpp:149-150,791`) de los `alphaTestedChunks` (`GX_SetZCompLoc(GX_DISABLE)`).
3. **Display Lists GX y Backface Culling por Orientación (`src/wii/render/WiiNativeDraw.cpp`)**:
   - `wii_sort_opaque_faces` (`src/wii/render/WiiNativeDraw.cpp:693-747`) agrupa los quads del `pass 0` en 7 buckets de orientación (`+X, -X, +Y, -Y, +Z, -Z, Other`).
   - `emitChunkDisplayLists` (`src/wii/render/WiiNativeDraw.cpp:485-617`) compila hasta 7 display lists nativos GX (`GX_BeginDispList` / `GX_EndDispList`) con vértices compactos de **17 bytes/vértice** (`setupTerrainVertexDescriptor`, `src/wii/render/WiiNativeDraw.cpp:232-243,398-431`) y los invoca con `GX_CallDispList` en `callChunkGeometry` (`src/wii/render/WiiNativeDraw.cpp:623-673`).

---

### 2.4 PC (OpenGL, Direct3D 9 y PC-Legacy — `PLATFORM_PC`)
1. **Despacho de Backend (`src/platform/RenderAPI_PC.cpp:9-24`)**:
   - Redirige cada llamada `render*` a `PcD3D9RenderBackend` si `pcRenderBackendIsDirect3D9()` es `true`, o a `PcOpenGlRenderBackend`.
2. **PC Estándar (`src/net/minecraft/src/WorldRenderer.cpp:277-492`) vs PC-Legacy (`src/pc/minecraft/WorldRendererPcLegacy.cpp:290-508`)**:
   - PC Estándar construye cada sección síncronamente dentro de `renderBeginDisplayList(glRenderList + pass)` (`src/net/minecraft/src/WorldRenderer.cpp:453`) llamando a `renderblocks.renderBlockByRenderType(block, x, y, z)` (`src/net/minecraft/src/WorldRenderer.cpp:400`).
   - PC-Legacy usa el mismo pipeline incremental `WorldRendererPcLegacy.cpp` que Xbox.
3. **OpenGL (`src/pc/render/PcOpenGlRenderBackend.cpp`)**:
   - Graba display lists con `glNewList`/`glEndList` (`src/pc/render/PcOpenGlRenderBackend.cpp:608-616`), emite arrays en `renderDrawInterleaved` (`src/pc/render/PcOpenGlRenderBackend.cpp:679-732`) con `glDrawArrays` (`src/pc/render/PcOpenGlRenderBackend.cpp:715`) y dibuja con `glCallLists` (`src/pc/render/PcOpenGlRenderBackend.cpp:623-626`).
4. **Direct3D 9 (`src/pc/render/d3d9/PcD3D9DisplayLists.cpp`)**:
   - Captura cada display list en un `PcD3D9RetainedMesh` (`src/pc/render/d3d9/PcD3D9DisplayLists.cpp:111-147,529-536`), hornea la traslación estática de la sección (`bakeTerrainStaticTransform`, `src/pc/render/d3d9/PcD3D9DisplayLists.cpp:158-233`) y fusiona múltiples secciones en `mergedTerrainBatch` (`src/pc/render/d3d9/PcD3D9DisplayLists.cpp:421-521`).

---

## 3. Nuevos `renderType` de 1.5.2 en `RenderBlocks`

En `mcp-diff/src152/net/minecraft/src/RenderBlocks.java:175-262` (`renderBlockByRenderType`), `RenderBlocks.java:6880-7119` (`renderBlockAsItem`) y `RenderBlocks.java:7121-7143` (`renderItemIn3d`), Minecraft 1.5.2 añade los siguientes tipos de render (del `28` al `39`, más `-1` para `BlockSkull`):

| `renderType` | Método en `src152/RenderBlocks.java` | Bloques 1.5.2 que lo usan | `renderItemIn3d` (`RenderBlocks.java:7122`) | Dibujo en Inventario / Mano (`renderBlockAsItem`) |
| :---: | :--- | :--- | :---: | :--- |
| **`28`** | `renderBlockCocoa` (`RenderBlocks.java:1872-2003`) | `BlockCocoa` (ID 127) | `false` | Sprite 2D (se planta con `ItemDye` cacao). |
| **`29`** | `renderBlockTripWireSource` (`RenderBlocks.java:5759-5842`) | `BlockTripWireSource` (ID 131) | `false` | Sprite 2D. |
| **`30`** | `renderBlockTripWire` (`RenderBlocks.java:5844-5889`) | `BlockTripWire` (ID 132) | `false` | Sprite 2D (`Item.silk`). |
| **`31`** | `renderBlockLog` (`RenderBlocks.java:1579-1623`) | `BlockLog` (ID 17, troncos orientables `meta & 12`) | `true` | 1 cubo estándar en `RenderBlocks.java:6940-6969`. |
| **`32`** | `renderBlockWall` (`RenderBlocks.java:5570-5676`) | `BlockWall` (ID 139, muros de roca/musgosa) | `true` | 2 cajas en `RenderBlocks.java:6996-7024` (poste central + travesaño). |
| **`33`** | `renderBlockFlowerpot` (`RenderBlocks.java:520-608`) | `BlockFlowerPot` (ID 140, maceta) | `false` | Sprite 2D (`Item.flowerPot`). |
| **`34`** | `renderBlockBeacon` (`RenderBlocks.java:3613-3656`) | `BlockBeacon` (ID 138, faro) | `true` | 3 cubos concéntricos en `RenderBlocks.java:6915-6935` + `TileEntityBeaconRenderer` para el haz en el mundo. |
| **`35`** | `renderBlockAnvil` (`RenderBlocks.java:610-723`) | `BlockAnvil` (ID 145, yunque) | `true` | Llama a `renderBlockAnvilOrient(block, 0, 0, 0, meta << 2, true)` (`RenderBlocks.java:6912-6914`, 4 cajas apiladas). |
| **`36`** | `renderBlockRedstoneLogic` (`RenderBlocks.java:1661-1776`) | `BlockRedstoneRepeater` (IDs 93, 94) | `false` | Sprite 2D (`Item.redstoneRepeater`). |
| **`37`** | `renderBlockComparator` (`RenderBlocks.java:1625-1659`) | `BlockComparator` (IDs 149, 150) | `false` | Sprite 2D (`Item.comparator`). |
| **`38`** | `renderBlockHopper` (`RenderBlocks.java:5891-6062`) | `BlockHopper` (ID 154, tolva) | `false` | Sprite 2D en inventario (`BlockHopper.java:201-203`), aunque `renderBlockAsItem` también incluye rama `var2 == 38` en `RenderBlocks.java:6907-6910`. |
| **`39`** | `renderBlockQuartz` (`RenderBlocks.java:1778-1832`) | `BlockQuartz` (ID 155, pilar de cuarzo orientable) | `true` | 1 cubo estándar en `RenderBlocks.java:6940-6969`. |

---

## 4. Guía Paso a Paso: Qué Archivos y Funciones Tocar para Añadir un Bloque Nuevo (Ej. Tolva `BlockHopper` o Yunque `BlockAnvil`)

### Paso 1: Definir y Registrar la Clase del Bloque (`src/net/minecraft/src/Block.h` y `src/net/minecraft/src/Block.cpp`)
1. Crear `src/net/minecraft/src/BlockAnvil.h`/`.cpp` (`renderType = 35`) o `src/net/minecraft/src/BlockHopper.h`/`.cpp` (`renderType = 38`).
2. Declarar el puntero estático en `src/net/minecraft/src/Block.h:212-335` e instanciarlo en `Block::initialize()` (`src/net/minecraft/src/Block.cpp:780-976`) antes del bucle de tablas de lookup (`src/net/minecraft/src/Block.cpp:978-996`).
3. **Sobrescribir obligatoriamente los virtuales de opacidad y culling** en `BlockAnvil` / `BlockHopper`:
   - `bool isOpaqueCube() override { return false; }` (`src/net/minecraft/src/Block.h:100`):
     - Hace que `Block::opaqueCubeLookup[id]` sea `false` (`src/net/minecraft/src/Block.cpp:294,982`), evitando que bloques adyacentes oculten sus caras.
     - Hace que `simpleOpaqueCube` sea `false` en **Xbox/PC-Legacy** (`src/pc/render/PcLegacyBlockRenderInfo.cpp:47`), **PS2** (`src/ps2/render/Ps2BlockRenderInfo.cpp:46`) y **Wii** (`src/wii/render/WiiBlockRenderInfo.cpp:40`), enviando el bloque a `renderBlockByRenderType`.
     - Activa `usedAlphaTestedTexture = true` (`src/net/minecraft/src/RenderBlocks.cpp:708-709,716-717`) para que **Wii** lo envíe a `alphaTestedChunks` (`src/net/minecraft/src/RenderList.h:27-32`) sin corromper el early-Z.
   - `bool renderAsNormalBlock() override { return false; }` (`src/net/minecraft/src/Block.h:54`).
   - `int_t getRenderType() override { return 35; /* 35 = Anvil, 38 = Hopper */ }` (`src/net/minecraft/src/Block.h:56`).
   - `bool shouldSideBeRendered(IBlockAccess*, int_t, int_t, int_t, int_t) override { return true; }` (`src/net/minecraft/src/Block.h:76`) **junto con** `bool usesDefaultFaceCulling() override { return false; }` (`src/net/minecraft/src/Block.h:83`) para que `RenderBlocks::renderStandardBlock` no descarte las caras retraídas de las sub-cajas del yunque o de la tolva cuando tienen un bloque sólido al lado (`src/net/minecraft/src/RenderBlocks.cpp:279-302`).

### Paso 2: Declarar los Métodos de Dibujo en `src/net/minecraft/src/RenderBlocks.h`
- En `src/net/minecraft/src/RenderBlocks.h:55-136`, declarar:
  - Para **Yunque (`BlockAnvil`)**:
    - `bool renderBlockAnvil(BlockAnvil *block, int_t x, int_t y, int_t z);`
    - `bool renderBlockAnvilMetadata(BlockAnvil *block, int_t x, int_t y, int_t z, int_t meta);`
    - `bool renderBlockAnvilOrient(BlockAnvil *block, int_t x, int_t y, int_t z, int_t meta, bool renderFromInventory);`
    - `float renderBlockAnvilRotate(BlockAnvil *block, int_t x, int_t y, int_t z, int_t part, float yOffset, float sizeX, float sizeY, float sizeZ, bool swapXZ, bool renderFromInventory, int_t meta);`
  - Para **Tolva (`BlockHopper`)**:
    - `bool renderBlockHopper(BlockHopper *block, int_t x, int_t y, int_t z);`
    - `bool renderBlockHopperMetadata(BlockHopper *block, int_t x, int_t y, int_t z, int_t meta, bool renderFromInventory);`
- Los campos de rotación UV (`bottomFaceRotation`, `topFaceRotation`, `eastFaceRotation`, `westFaceRotation`, `northFaceRotation`, `southFaceRotation`) ya existen en `src/net/minecraft/src/RenderBlocks.h:202-207` y ya están soportados en `renderBottomFace`..`renderSouthFace` (`src/net/minecraft/src/RenderBlocks.cpp:3950-4592`).

### Paso 3: Añadir el Case en `RenderBlocks::renderBlockByRenderType` e Implementar la Geometría (`src/net/minecraft/src/RenderBlocks.cpp` y `src/net/minecraft/src/RenderBlocks125.cpp`)
1. En `RenderBlocks::renderBlockByRenderType` (`src/net/minecraft/src/RenderBlocks.cpp:720-828`), añadir al `switch (l)`:
   - `case 35: return renderBlockAnvil(static_cast<BlockAnvil*>(block), i, j, k);`
   - `case 38: return renderBlockHopper(static_cast<BlockHopper*>(block), i, j, k);`
2. Implementar los métodos en `src/net/minecraft/src/RenderBlocks125.cpp:50-976`:
   - **`renderBlockAnvil`** (portado de `mcp-diff/src152/net/minecraft/src/RenderBlocks.java:610-723`): dibuja 4 cajas superpuestas (base, cintura inferior, cuello y parte superior dañada `0..2`) ajustando las rotaciones UV según `meta & 3` y restaurando `setBlockBounds(0, 0, 0, 1, 1, 1)` y las 6 rotaciones UV a `0` al salir. Además, en `src/net/minecraft/src/RenderFallingSand.cpp:29-65` (comparar con `RenderFallingSand.java:20-32`), añadir la rama `block->getRenderType() == 35` que invoca `renderBlockAnvilMetadata` cuando el yunque cae como `EntityFallingSand`.
   - **`renderBlockHopper`** (portado de `mcp-diff/src152/net/minecraft/src/RenderBlocks.java:5891-6062`):
     1. Corona exterior superior `(0, 0.625, 0)-(1, 1, 1)` con `renderStandardBlock`.
     2. 4 caras interiores del embudo desplazadas `0.125f` (`renderSouthFace`, `renderNorthFace`, `renderWestFace`, `renderEastFace`) y fondo interior (`renderTopFace` con textura `hopper_inside`).
     3. Cuerpo central `(0.25, 0.25, 0.25)-(0.75, 0.625, 0.75)` con `renderStandardBlock`.
     4. Boquilla inferior `(0.375, 0, 0.375)-(0.625, 0.25, 0.625)` o lateral según `dir = BlockHopper::getDirectionFromMetadata(meta)` (`0, 2, 3, 4, 5`) con `renderStandardBlock`.
     - **Nota sobre `overrideBlockTexture` en C++**: En `src/net/minecraft/src/RenderBlocks.cpp:685-698`, `overrideBlockTexture` es un `int_t` (`-1` si está inactivo). En `renderBlockHopper`, solo se debe fijar temporalmente `overrideBlockTexture` al tile de `hopper_outside` cuando su valor previo sea `-1`, para no pisar la textura de grietas de rotura (`240..249`) enviada por `RenderGlobal::drawBlockBreaking` (`src/net/minecraft/src/RenderGlobal.cpp:2887`).

### Paso 4: Renderizado en Inventario, Mano y Suelo (`src/net/minecraft/src/RenderBlocks.cpp`)
1. En `RenderBlocks::renderItemIn3d(int_t i)` (`src/net/minecraft/src/RenderBlocks.cpp:4893-4912`):
   - Añadir `i == 35` (yunque), `i == 31` (tronco), `i == 32` (muro), `i == 34` (faro) e `i == 39` (cuarzo) para que devuelvan `true` (`RenderBlocks.java:7122`).
   - La tolva (`i == 38`) devuelve `false` porque en inventario y mano usa su sprite 2D de ítem (`BlockHopper.java:201-203`).
2. En `RenderBlocks::renderBlockOnInventory(Block *block, int_t i, float f)` (`src/net/minecraft/src/RenderBlocks.cpp:4594-4891`):
   - Añadir rama `else if (j == 35)` que traslada `(-0.5f, -0.5f, -0.5f)` y llama a `renderBlockAnvilOrient(static_cast<BlockAnvil*>(block), 0, 0, 0, i << 2, true)` (`RenderBlocks.java:6912-6914`).
   - Añadir rama `else if (j == 38)` que traslada `(-0.5f, -0.5f, -0.5f)` y llama a `renderBlockHopperMetadata(static_cast<BlockHopper*>(block), 0, 0, 0, 0, true)` (`RenderBlocks.java:6907-6910`).

### Paso 5: Bloques con `TileEntitySpecialRenderer` (`BlockEnderChest`, `BlockBeacon`, `BlockSkull`, `BlockChest` Trampa)
Si el bloque nuevo requiere un `TileEntitySpecialRenderer` (`mcp-diff/src152/net/minecraft/src/TileEntityRenderer.java:24-33`):
1. Registrar el nuevo renderer en `TileEntityRenderer::TileEntityRenderer()` (`src/net/minecraft/src/TileEntityRenderer.cpp:69-76`).
2. En `src/pc/render/PcLegacyStaticTileEntityPolicy.h:22-37` y `src/pc/render/PcLegacyStaticTileEntityMesh.cpp:1-210`, evaluar si puede mallarse estáticamente en Xbox / PC-Legacy (p. ej. `TileEntitySkull` es estático y puede ir al display list `glRenderList + 2`, mientras que `TileEntityBeacon` y `TileEntityEnderChest` son animados y deben seguir siendo TESR dinámicos).
3. Para el render en inventario de `BlockEnderChest` (ID 130) y `BlockChest` trampa (ID 146), actualizar `src/net/minecraft/src/RenderBlocks.cpp:4648-4653` y `src/net/minecraft/src/ChestItemRenderHelper.cpp:11-21` (`ChestItemRenderHelper.java:6-18`).

---

## 5. Tabla Resumen de Archivos Clave por Plataforma

| Subsistema / Plataforma | Archivos Clave (`archivo:línea`) | Función Principal |
| :--- | :--- | :--- |
| **Registro y Lookups** | `src/net/minecraft/src/Block.h:54-105,144-210`<br>`src/net/minecraft/src/Block.cpp:275-300,978-996` | Virtuales de bloque y población de `opaqueCubeLookup`, `usesDefaultFaceCullingLookup`, `useNeighborBrightness`. |
| **Despacho `RenderBlocks`** | `src/net/minecraft/src/RenderBlocks.h:55-216`<br>`src/net/minecraft/src/RenderBlocks.cpp:700-829,2575-4912`<br>`src/net/minecraft/src/RenderBlocks125.cpp:50-976` | `renderBlockByRenderType`, `renderStandardBlock`, las 6 caras, `renderBlockOnInventory`, `renderItemIn3d` y bloques especiales. |
| **Ítems, TESR y Rotura** | `src/net/minecraft/src/RenderItem.cpp:131-151,213-266`<br>`src/net/minecraft/src/ItemRenderer.cpp:74-101`<br>`src/net/minecraft/src/TileEntityRenderer.cpp:43-97`<br>`src/net/minecraft/src/RenderGlobal.cpp:2850-2959` | Render de bloques en suelo, GUI y mano, registro de `TileEntitySpecialRenderer`, `drawBlockBreaking` y `drawSelectionBox`. |
| **Xbox (D3D8) y PC-Legacy** | `src/platform/PlatformConfig.h:236-248`<br>`src/pc/render/PcLegacyBlockRenderInfo.cpp:29-77`<br>`src/pc/render/PcLegacyCubeMaterialInfo.cpp:18-31,62-205`<br>`src/pc/render/PcLegacySectionCache.cpp:53-195`<br>`src/pc/minecraft/WorldRendererPcLegacy.cpp:142-508`<br>`src/pc/render/PcLegacyStaticTileEntityPolicy.h:22-37`<br>`src/platform/RenderAPI_D3D8_XBOX.cpp:643-693,783-841,1056-1285` | Clasificación `simpleOpaqueCube`, worksets por sección, mallado incremental por pasos, TESR estáticos y empaquetado en `XboxCompactVertex` (16 B/vértice) dentro de `VertexPool` D3D8. |
| **PS2 (GS / VU0 / VU1)** | `src/ps2/render/Ps2BlockRenderInfo.cpp:30-87`<br>`src/ps2/render/Ps2TerrainEligibility.cpp:52-66`<br>`src/ps2/render/Ps2GreedyMesh.cpp:482-774`<br>`src/ps2/minecraft/WorldRendererPs2.cpp:736-1001,1140-1348`<br>`src/ps2/render/Ps2TerrainMesh.cpp:61-165,296-441`<br>`src/ps2/render/Ps2TerrainSectionDraw.cpp:24-800`<br>`src/ps2/render/Ps2Vu1Terrain.cpp:200-344`<br>`src/platform/RenderAPI_GS_PS2.cpp:582-632` | Clasificación por bloque, greedy meshing en `pass 0`, captura de quads iluminados, empaquetado con microcódigo VU0 a `S16` + `Ps2TerrainTileRun` y dibujo DMA Path1 en VU1. |
| **Wii (GX)** | `src/wii/render/WiiBlockRenderInfo.cpp:30-65`<br>`src/wii/minecraft/WorldRendererWii.cpp:123-445,502-507`<br>`src/wii/render/WiiNativeDraw.cpp:232-243,398-431,485-747`<br>`src/net/minecraft/src/RenderList.h:22-47` | Clasificación `simpleOpaqueCube`, detección de `usedAlphaTestedTexture` para early-Z (`GX_SetZCompLoc`) y compilación de display lists GX (17 B/vértice) en 7 grupos de orientación. |
| **PC (OpenGL / D3D9)** | `src/net/minecraft/src/WorldRenderer.cpp:277-492`<br>`src/platform/RenderAPI_PC.cpp:8-24`<br>`src/pc/render/PcOpenGlRenderBackend.cpp:608-626,679-732`<br>`src/pc/render/d3d9/PcD3D9DisplayLists.cpp:158-233,421-521` | Mallado en display lists, `glNewList`/`glCallLists` en OpenGL y `PcD3D9RetainedMesh` con `MergedTerrainBatch` en Direct3D 9. |
