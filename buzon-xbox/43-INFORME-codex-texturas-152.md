# Encargo 43 - atlas y texturas Minecraft 1.5.2

## Decision aplicada

Se generan `terrain.png` y `gui/items.png` a 256x256, 16x16 celdas de 16 px. La comparacion exacta RGBA hallo 235 imagenes distintas en los iconos estaticos de bloques y 200 en los de items; el generador incluye ademas el primer cuadro de los strips animados aptos. Caben en 256 casillas por atlas y mantienen el formato que usa el renderer heredado. El script conserva los indices con coincidencia exacta de arte 1.5.2, sustituye indices heredados activos cuyo arte cambio (bloques 14 portal, 19 gravel y 31 fire; los items se derivan de `Item.cpp`) y rellena casillas libres sin cambiar las dimensiones.

Coste estimado de los dos atlas, solo nivel base:

| Plataforma | Formato observado en el codigo | Por atlas | Ambos |
|---|---|---:|---:|
| Xbox | D3D8 A8R8G8B8, 32 bpp | 256 KiB | 512 KiB (0,78 % de 64 MiB compartidos) |
| PS2 | GS PSMT8, 8 bpp + dos CLUT CT16 de 512 B | ~65 KiB | ~130 KiB (~3,2 % de 4 MiB VRAM) |
| Wii | GX RGB5A3, 16 bpp; RGBA8 solo para alta precision | 128 KiB | 256 KiB base, sin contar mipmaps |
| PC | OpenGL RGBA / D3D9 A8R8G8B8, 32 bpp | 256 KiB | 512 KiB |

Referencias de implementacion: `RenderEngine.cpp:62-66,457-461,804-814,1770-1782` reconoce/carga los dos atlas, obtiene el ancho de icono del ancho de imagen y actualiza `TextureFX`; `RenderBlocks.cpp:859-864` calcula UV con cuadricula de 16x16 y denominador 256. Xbox D3D8 crea A8R8G8B8 en `src/platform/RenderAPI_D3D8_XBOX.cpp:378`; PS2 usa T8 y CLUT en `src/ps2/render/Ps2Texture.cpp:166-169,706-720` (VRAM de 4 MiB: `src/ps2/render/Ps2Graphics.cpp:19`); Wii selecciona RGB5A3/RGBA8 y calcula bytes en `src/wii/render/WiiNativeTexture.cpp:301-309`; PC usa RGBA en `src/pc/render/PcOpenGlRenderBackend.cpp:305-313` y D3D9 A8R8G8B8 en `src/pc/render/d3d9/PcD3D9Textures.cpp:197-199`.

Las texturas 1.5.2 estan tambien copiadas como archivos sueltos en `assets/textures/blocks/` y `assets/textures/items/`, incluidas las animaciones completas, por lo que el `.pak` conserva todos los PNG originales y no depende solo del primer cuadro empaquetado en el atlas.

## Iconos sin coincidencia en 1.2.5

Se compararon por pixel todos los PNG 16x16 contra las 256 celdas del atlas 1.2.5. Hay 54 nombres nuevos de bloque (50 imagenes distintas; algunos estados comparten pixeles) y 64 nombres nuevos de item (64 imagenes distintas). Los strips animados `fire_0.png`, `fire_1.png`, `lava.png`, `lava_flow.png`, `portal.png`, `water.png`, `water_flow.png`, y los items animados `clock.png` y `compass.png`, no entran en este conteo estatico: se copian integros al paquete y el atlas incluye su primer cuadro cuando el ancho permite extraerlo.

**Bloques:** activatorRail.png, activatorRail_powered.png, anvil_base.png, anvil_top.png, anvil_top_damaged_1.png, anvil_top_damaged_2.png, beacon.png, blockEmerald.png, blockRedstone.png, brewingStand.png, carrots_0.png, carrots_1.png, carrots_2.png, carrots_3.png, cocoa_0.png, cocoa_1.png, cocoa_2.png, commandBlock.png, comparator.png, comparator_lit.png, daylightDetector_side.png, daylightDetector_top.png, detectorRail_on.png, dispenser_front_vertical.png, dropper_front.png, dropper_front_vertical.png, enchantment_side.png, endframe_eye.png, endframe_side.png, flowerPot.png, gravel.png, hopper.png, itemframe_back.png, leaves_jungle.png, netherquartz.png, oreEmerald.png, potatoes_0.png, potatoes_1.png, potatoes_2.png, potatoes_3.png, quartzblock_bottom.png, quartzblock_chiseled.png, quartzblock_chiseled_top.png, quartzblock_lines.png, quartzblock_lines_top.png, quartzblock_side.png, quartzblock_top.png, repeater.png, repeater_lit.png, sapling_jungle.png, stem_bent.png, stem_straight.png, tripWire.png, tripWireSource.png

**items:** apple.png, beefCooked.png, beefRaw.png, blazePowder.png, boat.png, bootsCloth.png, bootsCloth_overlay.png, bread.png, carrotGolden.png, carrotOnAStick.png, carrots.png, chestplateCloth.png, chestplateCloth_overlay.png, chickenCooked.png, chickenRaw.png, coal.png, comparator.png, diamond.png, diode.png, dyePowder_brown.png, dyePowder_green.png, dyePowder_red.png, dyePowder_white.png, emerald.png, emptyMap.png, enchantedBook.png, enderPearl.png, fireworks.png, fireworksCharge.png, fireworksCharge_overlay.png, flint.png, flowerPot.png, frame.png, goldNugget.png, helmetCloth.png, helmetCloth_overlay.png, hopper.png, leggingsCloth.png, leggingsCloth_overlay.png, magmaCream.png, minecartChest.png, minecartHopper.png, minecartTnt.png, netherbrick.png, netherquartz.png, netherStalkSeeds.png, netherStar.png, porkchopCooked.png, porkchopRaw.png, potato.png, potatoBaked.png, potatoPoisonous.png, pumpkinPie.png, record_wait.png, rottenFlesh.png, ruby.png, skull_char.png, skull_creeper.png, skull_skeleton.png, skull_wither.png, skull_zombie.png, stick.png, writingBook.png, writtenBook.png

La comparacion de los atlas originales dejo 2 casillas transparentes en `terrain.png` y 73 en `gui/items.png`. Estas son las casillas vacias del atlas anterior; no equivalen al espacio libre del atlas nuevo, porque las celdas antiguas que no se reutilizan exactas se consideran candidatas para las texturas 1.5.2.

El header generado [Texture152Atlas.h](../wt-152-codex/src/net/minecraft/src/Texture152Atlas.h) contiene la tabla `nombre de PNG -> indice` para 248 nombres de bloques y 203 de items. indices reportados por el script: 240 imagenes distintas para bloques y 202 para items, contando los primeros cuadros de animaciones. No se portaron bloques.

### Bloques heredados con indice de atlas fijo

Esta lista sale de las inicializaciones con indice numerico en `Block.cpp:922-1045`; el numero anterior se mantiene y su celda contiene los pixeles 1.5.2 indicados. Bloques con textura por cara, metadato o clase especializada derivan otras celdas con el codigo existente.

| ID | Bloque | indice actual | Imagen 1.5.2 en esa celda |
|---:|---|---:|---|
| 1 | stone | 1 | `stone.png` |
| 3 | dirt | 2 | `dirt.png` |
| 4 | stonebrick | 16 | `stonebrick.png` |
| 6 | sapling | 15 | `sapling.png` |
| 7 | bedrock | 17 | `bedrock.png` |
| 12 | sand | 18 | `sand.png` |
| 13 | gravel | 19 | `gravel.png` |
| 14 | oreGold | 32 | `oreGold.png` |
| 15 | oreIron | 33 | `oreIron.png` |
| 16 | oreCoal | 34 | `oreCoal.png` |
| 20 | glass | 49 | `glass.png` |
| 21 | oreLapis | 160 | `oreLapis.png` |
| 22 | blockLapis | 144 | `blockLapis.png` |
| 27 | goldenRail | 179 | `goldenRail_powered.png` |
| 28 | detectorRail | 195 | `detectorRail.png` |
| 29 | pistonStickyBase | 106 | `piston_top_sticky.png` |
| 30 | web | 11 | `web.png` |
| 33 | pistonBase | 107 | `piston_top.png` |
| 41 | blockGold | 23 | `blockGold.png` |
| 42 | blockIron | 22 | `blockIron.png` |
| 45 | brick | 7 | `brick.png` |
| 46 | tnt | 8 | `tnt_side.png` |
| 47 | bookshelf | 35 | `bookshelf.png` |
| 48 | stoneMoss | 36 | `stoneMoss.png` |
| 49 | obsidian | 37 | `obsidian.png` |
| 50 | torch | 80 | `torch.png` |
| 52 | mobSpawner | 65 | `mobSpawner.png` |
| 55 | redstoneDust | 164 | `redstoneDust_cross.png` |
| 56 | oreDiamond | 50 | `oreDiamond.png` |
| 57 | blockDiamond | 24 | `blockDiamond.png` |
| 59 | crops | 88 | `crops_0.png` |
| 65 | ladder | 83 | `ladder.png` |
| 66 | rail | 128 | `rail.png` |
| 69 | lever | 96 | `lever.png` |
| 73 | oreRedstone | 51 | `oreRedstone.png` |
| 74 | oreRedstone | 51 | `oreRedstone.png` |
| 75 | notGate | 115 | `redtorch.png` |
| 76 | notGate | 99 | `redtorch_lit.png` |
| 78 | snow | 66 | `snow.png` |
| 79 | ice | 67 | `ice.png` |
| 80 | snow | 66 | `snow.png` |
| 81 | cactus | 70 | `cactus_side.png` |
| 82 | clay | 72 | `clay.png` |
| 83 | reeds | 73 | `reeds.png` |
| 84 | jukebox | 74 | `musicBlock.png` |
| 85 | fence | 4 | `wood.png` |
| 86 | pumpkin | 102 | `pumpkin_top.png` |
| 87 | hellrock | 103 | `hellrock.png` |
| 88 | hellsand | 104 | `hellsand.png` |
| 89 | lightgem | 105 | `lightgem.png` |
| 91 | litpumpkin | 102 | `pumpkin_top.png` |
| 92 | cake | 121 | `cake_top.png` |
| 101 | fenceIron | 85 | `fenceIron.png` |
| 102 | thinGlass | 49 | `glass.png` |
| 107 | fenceGate | 4 | `wood.png` |
| 112 | netherBrick | 224 | `netherBrick.png` |
| 113 | netherFence | 224 | `netherBrick.png` |
| 121 | whiteStone | 175 | `whiteStone.png` |
| 122 | dragonEgg | 167 | `dragonEgg.png` |

## Recursos 1.5.2 fuera de los atlas

Se anadieron al pak 39 PNG de rutas nuevas o con pixeles distintos en `src152`, y se dejaron intactos los archivos identicos. Son:

`armor/cloth_1.png`, `armor/cloth_1_b.png`, `armor/cloth_2.png`, `armor/cloth_2_b.png`, `armor/diamond_1.png`, `armor/witherarmor.png`, `art/kz.png`, `environment/moon.png`, `environment/moon_phases.png`, `environment/sun.png`, `gui/beacon.png`, `gui/book.png`, `gui/creative_inv/list_items.png`, `gui/creative_inv/search.png`, `gui/creative_inv/survival_inv.png`, `gui/demo_bg.png`, `gui/hopper.png`, `gui/repair.png`, `gui/trading.png`, `item/chests/trap_large.png`, `item/chests/trap_small.png`, `item/enderchest.png`, `item/largexmaschest.png`, `item/skis.png`, `item/xmaschest.png`, `misc/beacon.png`, `misc/beam.png`, `misc/tunnel.png`, `mob/bat.png`, `mob/pigzombie.png`, `mob/skeleton_wither.png`, `mob/villager/witch.png`, `mob/wither.png`, `mob/wither_invul.png`, `mob/wolf_collar.png`, `mob/wolf_tame.png`, `mob/zombie.png`, `mob/zombie_villager.png`, `particles.png`

Se excluyeron `gui/allitems.png`, `gui/gui.png`, `gui/icons.png` y `gui/inventory.png`: contienen posiciones y composicion de interfaz 1.5.2 que no coinciden con las pantallas de consola actuales. El resto de `gui/`, ademas de `particles.png`, `armor/`, `art/`, `environment/`, `item/`, `misc/` y `mob/`, se incluyo cuando es nuevo o cambia. Como el runtime actual usa sus recursos por ruta pero aun no incorpora todas las pantallas/entidades 1.5.2, su presencia en el pak no implica que esas pantallas o mobs estan portados.

## Texture packs

Siguen cargandose por las mismas rutas `terrain.png` y `gui/items.png` y el tamano de ambos atlas continua en 256x256. `TexturePackList.cpp:120-127` carga `.pak` o carpeta desde `D:\texturepacks`; `scripts/xbox/make_legacy_texture_packs.py:89-90,122-126` conserva esas rutas y adapta atlas Legacy x32 a x16. Packs antiguos siguen siendo validos para las celdas y bloques heredados. Si un pack reemplaza el atlas completo, sus casillas reflejan su propio arte 1.2.5; para conservar el aspecto 1.5.2 en un pack personalizado hay que migrar sus dos atlas desde una base 1.5.2 y mantener las celdas correspondientes.

## Paquete y compilacion

`scripts/make_pak.py:22,37,81-124` empaqueta todos los archivos de `data/` como entradas `assets/...` y `resources/...` en MCPK, sin compresion, con offsets alineados a 64 bytes. El cambio de `cmake/xbox.cmake:292-309,328-336` admite un `XBOX_ASSETS_PAK` opcional y lo copia a la raiz de la salida/ISO, donde `Resources_XBOX.cpp:17-31` busca `D:/assets.pak`. El build usa exclusivamente `build/xbox-release/port152-data`; el `data` real se leyo como base y no se modifico.

Salida generada: `build/xbox-release/port152-output/assets.pak`: 1284 entradas, 56,665,856 bytes (54.0 MiB). Imagen de revision: `buzon-xbox/43-atlas.png` (atlas de bloques a la izquierda, items a la derecha). El header y el generador si van en Git; PNG fuente de Mojang, `.pak`, preview y staging quedan fuera del commit.

## Revision de Claude (dfa11f7): ISO sin duplicar data/

`cmake/xbox.cmake` ahora borra `bin/<perfil>/iso/data/` antes de empaquetar si `XBOX_ASSETS_PAK` existe y copia solo `assets.pak` a la raiz. `xbox-data` aplica la misma regla. El staging del logo suelto tambien se omite en esa modalidad porque el `logo3.png` ya esta incluido en el pak.

La comparacion del MCPK con `build/xbox-release/port152-data` dio 1.284 entradas esperadas y 1.284 presentes, con cero archivos elegibles ausentes o entradas inesperadas. Incluye 299 entradas `resources/` y 280 de musica/sonidos. Las unicas tres exclusiones son `assets/legacy/tutorial/session.lock`, `level.dat_old` y `level.dat_mcr`, tal como documenta `scripts/make_pak.py` para el guardado de solo lectura.

Builds `xbox-release` y `xbox-release-exp` completados con el mismo `assets.pak` (SHA-256 coincidente). Ambas ISO pesan 69.337.088 bytes (66,1 MiB), frente a 127.664.128 bytes antes. Las carpetas `iso/` no contienen `data/`. La release contiene `OptiCraft.xbe` y `OptiCraft_720p.xbe`; la experimental contiene `OptiCraft_exp.xbe` y `OptiCraft_exp_720p.xbe`.

Con `BUILD.lock` y xemu libre, la captura a los 15 s mostro el logo de presentacion del port y a los 30 s el menu principal. Captura: `build/xemu-assets-pak-30s.png`. El build termino sin errores; el enlazador XDK siguio mostrando avisos LNK4229 de `/FAILIFMISMATCH` y el parcheador SSE2 sus avisos conocidos del CRT.
