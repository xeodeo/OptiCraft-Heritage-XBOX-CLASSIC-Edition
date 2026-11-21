# 90. Informe: partículas del cofre y fuego en packs LCE, cursor en Logros, niebla del Nether y documentación de ocultación de entidades

- **Rama 1.5.2**: `port152-erluza` en `wt-152-rend` (sobre `1c120bb` → **`5b6f332`**, 3 commits):
  1. `b5227f8` — `fix(textures): static procedural fire/portal/fluid fallback for LCE packs and ender chest break particles`
  2. `27b9311` — `fix(ui): draw Achievements cursor on top of tooltips and enable pointer in Legacy UI`
  3. `5b6f332` — `docs(xbox): document EntityOcclusionCache 6-block vertical threshold and disable macro`
- **Rama 1.2.5**: `opti30-erluza` en `wt-opti30` (sobre `a6642bb` → **`e1f6bb7`**, 3 commits):
  1. `88f853f` — `fix(textures): oak planks chest break particles and static fire/portal/fluid fallback for LCE packs`
  2. `b3c8a37` — `fix(ui): draw Achievements cursor on top of tooltips and enable pointer in Legacy UI`
  3. `e1f6bb7` — `docs(xbox): document EntityOcclusionCache 6-block vertical threshold and disable macro`
- **Reglas respetadas**: sin `reset`, sin `push`, sin copiar al lanzador, sin tocar hojas (punto 4 respetado tal cual), sin tocar Creativo ni worldgen, sin opción nueva para la niebla del Nether ni cambios en 1.2.5 (según corrección 20-11), respetando `BUILD.lock`, 0 avisos nuevos de SSE2, `ps2-release` compilado y sonda temporal eliminada antes de las compilaciones finales (`hasProbe=False`).

---

## 1. Partículas verdes del cofre y fuego/portal mal con paquetes de texturas LCE

### Causa raíz (`archivo:línea`)
Se inspeccionaron los **11 paquetes `.pak` LCE** (`Legacy Console.pak`, `City.pak`, `Fantasy.pak`, `Natural.pak`, `Plastic.pak`, `Candy.pak`, `Cartoon.pak`, `Festive.pak`, `Halloween.pak`, `Steampunk.pak`, `Mass Effect.pak`) frente al atlas Java 1.2.5 / 1.5.2:

1. **Partículas verdes del cofre (1.2.5)**:
   - `wt-opti30/src/net/minecraft/src/BlockChest.cpp:25,382-463` y `BlockLockedChest.cpp:15-31`: en 1.2.5 el cofre inicializaba `blockIndexInTexture = 26` y `getBlockTextureFromSide(0)` devolvía la casilla **`25`** (antiguo cofre de Beta).
   - En `terrain.png` de los packs LCE (basados en TU19 / MC 1.6.4), las casillas `25`, `26` y `27` ya no son del cofre sino **Bloque de esmeralda** (`25`, verde RGB `(81, 217, 117)` en `Legacy Console.pak` y verde en 9 de 11 packs), **Bloque de redstone** (`26`) y **Yunque/Cuarzo** (`27`).
   - Al romper un cofre, `EffectRenderer::addBlockDestroyEffects` / `EntityDiggingFX` toma `block->getBlockTextureFromSideAndMetadata(0, meta)` → casilla `25` → partículas **verde esmeralda**. En Java 1.5.2 (y en nuestra 1.5.2) el cofre usa la casilla **`4`** (tablones de roble), que es madera marrón en el pack por defecto y en los 11 packs LCE.
2. **Partículas del Cofre de Ender (1.5.2)**:
   - `wt-152-rend/src/net/minecraft/src/BlockEnderChest.cpp:13-22`: no asignaba `blockIndexInTexture`, heredando `0` (parte superior de hierba) en lugar de **`37`** (obsidiana, como Java 1.5.2).
   - Auditoría del resto de bloques con `renderType != 0` / modelo especial en ambas ramas (`BlockBed`, `BlockSign`, `BlockDoor`, `BlockTrapDoor`, `BlockFence`, `BlockFenceGate`, `BlockBrewingStand`, `BlockCauldron`, `BlockEnchantmentTable`, `BlockAnvil`, `BlockBeacon`, `BlockFlowerPot`, `BlockSkull`, `BlockHopper`, `BlockTrappedChest`, `BlockDragonEgg`): todos devuelven índices válidos (el cofre trampa hereda de `BlockChest` → casilla `4`).
3. **Fuego, portal y fluidos vacíos en los packs LCE (1.5.2 y 1.2.5)**:
   - En los 11 packs LCE, `terrain.png` contiene **marcadores de posición** en las casillas de fuego `31`/`47` (rojo `(255, 0, 0)` con el texto `"FIRE TEX HERE"` en 7 packs y transparente `(0, 0, 0, 0)` en 4) y en la casilla de portal `14` (azul `(0, 42, 255)` `"PORTAL TEX HERE"` en 6 packs y transparente en 5), y ninguno incluye tiras de animación custom (`custom_fire_*.png`, `custom_portal.png`). Además `City.pak`, `Fantasy.pak` y `Natural.pak` tienen casillas 100 % transparentes en agua (`205`/`206`) y lava (`237`/`238`).
   - En Xbox, `ofAnimatedFire` y `ofAnimatedPortal` vienen en **`false`** por defecto (`GameSettings.cpp:288-289`).
   - `RenderEngine::updateStaticProceduralTextureFx` (`wt-152-rend/src/net/minecraft/src/RenderEngine.cpp:1132` y `wt-opti30/src/net/minecraft/src/RenderEngine.cpp:1129`) tenía la guarda `if (!isDefaultTexturePack()) return;`, por lo que al activar cualquier pack LCE con animaciones apagadas **nunca** sustituía los marcadores de fuego/portal ni las casillas transparentes de fluidos.

### Arreglo
- **1.2.5** (`88f853f`):
  - `src/net/minecraft/src/BlockChest.cpp`: `blockIndexInTexture = 4` (tablones de roble, igual que Java 1.5.2 y `wt-152-rend`) y simplificación de `getBlockTexture` / `getBlockTextureFromSide` para devolver `blockIndexInTexture`.
  - `src/net/minecraft/src/BlockLockedChest.cpp`: `blockIndexInTexture = 4` y getters alineados con 1.5.2.
- **1.5.2** (`b5227f8`):
  - `src/net/minecraft/src/BlockEnderChest.cpp`: `blockIndexInTexture = 37` (obsidiana).
- **Ambas versiones** (`b5227f8` / `88f853f`):
  - `src/net/minecraft/src/RenderEngine.cpp`:
    - En `updateDefaultTerrainTextureFx`: si un pack no por defecto tiene una casilla de agua/lava 100 % transparente (`alpha == 0` en todos sus píxeles), devuelve `SpecialTextureFxResult::NotHandled` para que actúe el respaldo procedural.
    - En `updateStaticProceduralTextureFx`: se elimina la exclusión `!isDefaultTexturePack()` para sintetizar una única vez el fotograma estático procedural de fuego (`31`/`47`), portal (`14`) y (si la casilla está vacía) agua/lava cuando las animaciones están desactivadas y no hay tira custom. El pack por defecto y los packs con texturas/tiras reales no se ven alterados.

---

## 2. Cursor invisible sobre logros / tooltips en `GuiAchievements` (1.5.2 y 1.2.5)

### Causa raíz (`archivo:línea`)
1. `GuiAchievements::drawAchievementsPanel` (`GuiAchievements.cpp:542-596` en ambas ramas) llamaba a `GuiScreen::drawScreen(i, j, f)` (que dibujaba el cursor de consola) **antes** de pintar el tooltip del logro bajo el cursor y **antes** de `drawTitle()`. Además, `renderitem.renderItemIntoGUI` dejaba `RenderCapability::CullFace` activo al salir del bucle de iconos.
2. `GuiAchievements` (`GuiAchievements.h`) no sobrescribía `allowsPlatformPointerInput() const override { return true; }`, por lo que en **Legacy UI** (`legacyUI == true`) `GuiScreen::drawScreen` suprimía el cursor (`menuCursorSuppressed`).
3. En `GuiScreen.cpp:747-761`, la supresión por `ContainerSlotNavigator` no comprobaba `dynamic_cast<const GuiContainer *>(target) != nullptr`.

### Arreglo (`27b9311` / `b3c8a37`)
- `src/net/minecraft/src/GuiScreen.h` y `GuiScreen.cpp`: se divide `drawScreen` en `drawScreenControls(mouseX, mouseY)` y `drawPlatformCursor(mouseX, mouseY, zOffset = 100.0f)`, y se restringe la supresión de `ContainerSlotNavigator` a pantallas `GuiContainer`.
- `src/net/minecraft/src/GuiAchievements.h`: se añade `bool allowsPlatformPointerInput() const override { return true; }`.
- `src/net/minecraft/src/GuiAchievements.cpp`: se desactiva `CullFace` tras el bucle de iconos, se llama a `drawScreenControls(i, j)` dentro de `drawAchievementsPanel` y se invoca `drawPlatformCursor(i, j, 300.0f)` al final de `GuiAchievements::drawScreen`, después del tooltip y de `drawTitle()` (y fuera de cualquier scissor del panel).

---

## 3. Niebla del Nether (1.5.2 vs 1.2.5)

### Auditoría y mediciones en vivo en `xemu` 64 MB (`dim=-1`)
Se compararon archivo a archivo `EntityRenderer.cpp`, `WorldProviderHell.cpp`, `WorldProvider.cpp`, `World.cpp`, `RenderGlobal.cpp`, `RenderAPI_D3D8_XBOX.cpp`, `GameSettings.cpp`, `Config.cpp` y `XboxTuning.h` entre `port152-erluza` y `opti30-erluza`: **no existe ninguna diferencia de código** en el cálculo de niebla, color de niebla ni estado D3D8 entre 1.5.2 y 1.2.5 (las únicas diferencias en `EntityRenderer.cpp` son `bossColorModifier` del Wither y `Potion::nightVision`, ambos `0` en condiciones normales).

Medición directa en vivo en `xemu` 64 MB (`dim=-1`, misma semilla `1621073931`, ajustes por defecto de Xbox `renderDistance=2 / 64`, `ofFogStart=0.80`, `ofFogType=1` Rápido):

| Versión | `dim` | `y` | `rd` / `fine` | `far` / `effFar` | `ofFogStart` | `fogStart` | `fogEnd` | `fogColor (R, G, B)` | `fogBrightness` | `fogOff` |
|---|---|---|---|---|---|---|---|---|---|---|
| **1.5.2** (`port152-erluza`) | `-1` | `71.62` | `2` / `64` | `64.0` / `64.0` | `0.80` | **`23.04`** | **`64.00`** | **`(0.1079, 0.0162, 0.0162)`** | `0.6232` | `0` |
| **1.2.5** (`opti30-erluza`) | `-1` | `71.62` | `2` / `64` | `64.0` / `64.0` | `0.80` | **`23.04`** | **`64.00`** | **`(0.1081, 0.0162, 0.0162)`** | `0.6207` | `0` |

- Con `Niebla: NO` (`ofFogType=3`), ambas aplican `fogStart = 256.0`, `fogEnd = 512.0` (a `64`). Con `Distancia: Normal (128)`, ambas aplican `fogStart = 46.08`, `fogEnd = 128.00`.
- **Resultado**: no se tocó 1.2.5 ni se añadió ninguna opción nueva (según corrección del usuario). Las capturas `152_nether_fog.png` y `125_nether_fog.png` confirman un render **idéntico píxel a píxel** con la misma configuración.
- **Explicación probable de la diferencia percibida en consola real**: si en el `options.txt` guardado de 1.2.5 en la Xbox estaba puesta `Niebla: NO` (`ofFogType:3`), `Distancia: Normal` (`128`) o `Inicio de niebla: 0.8` y en el `options.txt` de 1.5.2 otro valor (p. ej. `Niebla: Rápido` o `Inicio de niebla: 0.2`), 1.2.5 mostrará mucha menos niebla que 1.5.2 aun ejecutando exactamente el mismo código.

---

## 4. Hojas
No tocado (optimización Fancy de hojas de erluza sigue activa tal cual tras el revert de Claude en ambas ramas).

---

## 5. Documentación de ocultación de entidades (`5b6f332` / `e1f6bb7`)
Añadida en `docs/XBOX_PORT.md` de ambas ramas la subsección **Entity occlusion culling (vertical threshold)** dentro de la sección de rendimiento, documentando `src/platform/world/EntityOcclusionCache.cpp`, el umbral `kMinBelowCameraBlocks = 6.0`, el motivo (evitar que mobs en superficie tras una colina desaparezcan), los macros de ajuste (`PLATFORM_ENTITY_OCCLUSION_RAYCASTS_PER_FRAME`, `PLATFORM_ENTITY_OCCLUSION_RECHECK_FRAMES`) y cómo desactivarlo (`#define PLATFORM_ENTITY_OCCLUSION_CULLING 0` en `src/xbox/XboxTuning.h`).

---

## Capturas reales del backbuffer D3D8 (`buzon-xbox\90-capturas\`)

| Archivo | Qué demuestra |
|---|---|
| `152_lce_chest_fire_portal.png` | 1.5.2 con `Legacy Console.pak` (`isDefault=0`, `animFire=0`, `animPortal=0`): partículas de cofre de madera marrón, fuego naranja correcto y portal morado correcto. |
| `125_lce_chest_fire_portal.png` | 1.2.5 con `Legacy Console.pak` (`isDefault=0`, `animFire=0`, `animPortal=0`): partículas de cofre de madera marrón (ya NO verde esmeralda), fuego naranja y portal morado correctos. |
| `152_achievements_cursor.png` | 1.5.2 en `Legacy UI`: cursor blanco visible **encima** del icono *Taking Inventory* y de su tooltip. |
| `125_achievements_cursor.png` | 1.2.5 en `Legacy UI`: cursor blanco visible **encima** del icono *Taking Inventory* y de su tooltip. |
| `152_nether_fog.png` | 1.5.2 en el Nether (`dim=-1`, `start=23.04`, `end=64.00`). |
| `125_nether_fog.png` | 1.2.5 en el Nether (`dim=-1`, `start=23.04`, `end=64.00`), idéntica a 1.5.2. |

---

## Compilación final limpia y verificación en `xemu` 64 MB

Sonda temporal eliminada (`git status` limpio en ambos repos, `hasProbe=False` verificado en los 4 XBE, `BUILD.lock` liberado):

- **1.5.2 (`port152-erluza` `5b6f332`)**:
  - `xbox-release`: `rc=0`, **0 avisos nuevos de SSE2**, XBE `4,796,416 B` (`hasProbe=False`), arranque limpio `selftest: 0 failure(s)`, `double53=1`, `freeKB=45556`, `0 nan`, `0 [X87]`.
  - `xbox-release-exp`: `rc=0`, **0 avisos nuevos de SSE2**, XBE `4,808,704 B` (`hasProbe=False`), arranque limpio `selftest: 0 failure(s)`, `double53=1`, `freeKB=45808`, `0 nan`, `0 [X87]`.
- **1.2.5 (`opti30-erluza` `e1f6bb7`)**:
  - `xbox-release`: `rc=0`, **0 avisos nuevos de SSE2**, XBE `4,354,048 B` (`hasProbe=False`), arranque limpio `selftest: 0 failure(s)`, `double53=1`, `freeKB=46372`, `0 nan`, `0 [X87]`.
  - `xbox-release-exp`: `rc=0`, **0 avisos nuevos de SSE2**, XBE `4,370,432 B` (`hasProbe=False`), arranque limpio `selftest: 0 failure(s)`, `double53=1`, `freeKB=46356`, `0 nan`, `0 [X87]`.
  - `ps2-release`: `rc=0`.

---

## Qué probar en la Xbox real

1. **Pack LCE (`Legacy Console.pak` u otro `.pak` LCE)** en 1.2.5 y 1.5.2: romper un cofre (y un cofre de ender en 1.5.2) y comprobar partículas de madera (y obsidiana en el de ender), encender fuego y abrir un portal al Nether con animaciones apagadas por defecto para comprobar que ya se ven bien y sin el marcador rojo/azul. Comprobar también con el pack por defecto que nada ha cambiado.
2. **Logros (`GuiAchievements`)** en 1.2.5 y 1.5.2 (con **Legacy UI** y con UI Java): pasar el cursor por encima de varios logros y comprobar que el puntero siempre queda por delante tanto de los iconos como del cuadro de descripción.
3. **Niebla del Nether**: comprobar en Opciones de Vídeo que 1.5.2 y 1.2.5 tengan el mismo ajuste de `Niebla`, `Inicio de niebla` y `Distancia de visión` (p. ej. `Niebla: NO` si es como la tenías en 1.2.5) y verificar que se ven idénticas.
