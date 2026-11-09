# 74. OptiFine 1.5.2: qué nos sirve para la Xbox (09-11-2026)

Solo investigación: no se ha tocado código del juego ni ninguna rama.

## Dónde está
- **Jar:** `mcp-diff\optifine152\OptiFine_1.5.2_HD_U_D5.zip` (377.138 bytes, sha1 `35ede1a266c2fd4d0001b960a822e8473eb10746`).
  - Viene del espejo `github.com/SynArchive/OptiFine-Archive` (`1.5.2/`).
  - El blob de git coincide (`5864d5f2...`).
  - optifine.net ya solo ofrece de la 1.7.2 en adelante.
- **Mezcla con la vanilla y remapeo:**
  - `of152-merged.jar` es el jar vanilla de 1.5.2 con las 126 entradas de OptiFine encima.
  - Ojo: la mezcla se hace de zip a zip con Python, porque hay una clase `aux.class` y Windows no la puede extraer a disco.
  - `of152-named.jar` sale del remapeo `official → named` con `v152\conf\mappings.tiny`, igual que en `rehacer.sh`.
- **Descompilado:** `mcp-diff\srcOF152\` (Vineflower, 1.665 `.java`). Las clases propias de OptiFine están en la raíz: `Config`, `WrUpdaterSmooth`…
- **Clases de 1.5.2 que toca OptiFine:** `mcp-diff\optifine152\clases-tocadas.txt` (27, con el nº de líneas distintas). Por tamaño:
  - RenderBlocks 7016, RenderGlobal 2061, GameSettings 1943, EntityRenderer 1164, EntityLiving 977;
  - Texture 588, Tessellator 559, FontRenderer 527, RenderEngine 340, TextureMap 324, WorldRenderer 323;
  - PlayerManager 215, LongHashMap 95, Frustrum 30…
  - EntityLiving son casi todo ganchos de Forge (`Reflector`): no sirven.

## Lo que YA tenemos
OptiCraft ya trae el OptiFine de la época 1.2.5, conectado de verdad (`Config.cpp`, 622 líneas):
- **Clases:** ConnectedTextures, CustomColorizer, NaturalTextures, RandomMobs y las 5 pantallas `Gui*SettingsOF`.
- **Opciones:**
  - niebla rápida/bonita/apagada y su inicio;
  - nubes, árboles, césped, lluvia, agua, mejor césped y mejor nieve;
  - chunk updates (con modo dinámico), smooth FPS/input, carga lejana y precarga;
  - oclusión, mipmaps y AA/AF;
  - todas las animaciones por separado (agua, lava, fuego, portal, terreno, objetos, redstone, humo…);
  - partículas (vacío, agua, salpicado de lluvia, portal, goteo);
  - cielo, sol/luna, estrellas, hora fija, agua clara, colores de pantano, biomas suaves, autoguardado.

**Animaciones apagadas:** OptiFine 1.5.2 (`TextureMap.updateAnimations` → `isTerrainAnimationActive`) no llama a `updateAnimation()`: ni calcula ni sube nada. Nuestro `RenderEngine::updateDynamicTextures` hace lo mismo:
- con la animación apagada no hay `onTick` ni subida;
- fuego y portal se suben una sola vez (`updateStaticProceduralTextureFx`).

Así que, **con las animaciones apagadas, los ~5 ms de `textureStage` NO deberían venir de las animaciones**. Hay que medirlo (ver 68):
- El netlog de esa cifra es de una build de 1.2.5 de octubre: puede que tuviera las animaciones encendidas.
- Si se siguen pagando con todo apagado, el gasto está en el propio `applyTextureStage` (`s_textures.find`, estados) y no en el swizzle.

Con las animaciones **encendidas**, OptiFine sube solo el tile que cambia (`glTexSubImage2D`). Nosotros, en la Xbox, re-swizzleamos el atlas entero de 256 KB y esperamos a la gráfica. Ese es el punto 1 del informe 68 y sigue siendo lo que hay que hacer.

## Tabla 1: rendimiento (lo que importa; el cuello es la CPU)
| # | Qué hace | Clase(s) OptiFine | ¿Lo tenemos? | Ganancia en Xbox | Esfuerzo |
|---|---|---|---|---|---|
| 1 | **Ticks programados por chunk.** Repartidos en un `LongHashMap` por chunk, así `getPendingBlockUpdates(chunk)` solo mira los de ese chunk. | `NextTickHashSet`, `WorldServerOF.getPendingBlockUpdates` | **Parcial.** PC-legacy tiene `pcLegacyTickScheduler` (`PC_LEGACY_TICK_SCHEDULER`, `pc/tuning/PcLegacyTuning.h:89`). La Xbox va por la rama vieja (`World.cpp:3383`): copia la lista entera de ticks a un vector y la recorre en cada guardado o descarga de chunk. | **Alta en tirones:** `worldTick>chunkUnload` es la 2ª causa (567 tirones en el 68). Probablemente basta con activar el planificador que ya existe. | **Baja** |
| 2 | **Reconstrucción de chunks con tiempo límite adaptativo.** Mide cuánto tarda y ajusta el presupuesto cada frame (10 ms de inicio; baja el doble de lo que se pasa, sube lo que sobra). Si estás quieto, dinámico. `WorldRendererSmooth` parte un chunk en varias pasadas. | `WrUpdaterSmooth`, `WorldRendererSmooth`, `WrUpdates` | **Parecido.** Tenemos generación asíncrona, cola de mallas incremental, «carril urgente» y sección en curso. No sé si el presupuesto es adaptativo como este. | Media: menos tirones al moverse. | Media |
| 3 | **Objetos tirados en modo rápido.** Se dibujan como sprite plano (2D) en lugar de 3D. | `Config.isDroppedItemsFancy` (`ofDroppedItems`), `RenderItem` | **No** | Media con muchos objetos en el suelo (granjas, explosiones). | Baja |
| 4 | **Frustum «entero dentro».** Si la caja grande está entera dentro, no se prueban sus partes. | `ClippingHelper.isBoxInFrustumFully`, `Frustrum` | **No** (no hay `InFrustumFully`) | Baja-media: `visibility=2,6 ms` en el 68. | Baja |
| 5 | **`LongHashMap` más grande desde el inicio** (1.024 huecos en vez de 16) y con otro hash. | `LongHashMap` | **No** (el nuestro es el de 1.2.5/1.5.2) | Baja: menos rehash al cargar el mundo. **Ojo:** si algo itera el mapa en orden de hash, puede cambiar el orden de procesar chunks (no la semilla). | Muy baja |
| 6 | **Partículas de pociones apagables.** | `Config.isPotionParticles` | **No** | Baja-media en combate o con faros. | Muy baja |
| 7 | **Subir solo el tile animado.** | `TextureStitched.updateAnimation`, `TextureUtils` | **No en Xbox** (punto 1 del 68) | Alta **solo** con animaciones encendidas. | Baja-media |
| 8 | Smooth World (prioridad del hilo servidor en un núcleo, `Thread.yield`) | `Config.isSmoothWorld` | No aplica: no tenemos servidor integrado en otro hilo. | — | — |
| 9 | Hilo de chunks (`WrUpdaterThreaded`) | — | No sirve: un solo núcleo (68, «Hilos»). | — | — |

## Tabla 2: visuales (no dan rendimiento; algunas lo cuestan)
| Qué | Clase(s) OptiFine | ¿Lo tenemos? | Nota |
|---|---|---|---|
| Cielo personalizado (paquetes de texturas) | `CustomSky`, `CustomSkyLayer` | No | Solo con paquetes de texturas que lo traigan. Cuesta dibujar capas. |
| Animaciones personalizadas al estilo 1.5.2 | `TextureAnimation(s)` | Parcial (`CustomAnimationFrame`, del 1.2.5) | El formato 1.5.2 va por icono, no por tile. |
| Texturas conectadas, colores, naturales, mobs aleatorios | `ConnectedTextures`, `CustomColorizer`, `NaturalTextures`, `RandomMobs` | **Sí** (versión 1.2.5) | Habría que actualizar los nombres de icono 1.5.2 si se usan paquetes 1.5. |
| Niebla «Fancy» por distancia radial | `EntityRenderer` (`isFogFancy`) | Sí en las opciones | En la Xbox la niebla no se veía por la proyección (arreglado el 09-11, `227432b`). Falta probar si «Fancy» cambia algo con D3D (rango: `D3DRS_RANGEFOGENABLE`). |
| Capas de jugador, zoom | `isShowCapes`, `zoomMode` | No / zoom sí | Sin interés en consola (capas: online). |

## Tabla 3: opciones de menú que añade OptiFine 1.5.2 y no tenemos
`Dropped Items` (Fast/Fancy), `Potion Particles`, `Smooth World`, `Lazy Chunk Loading`, `Show Capes`.
Para la Xbox solo valen las dos primeras.

## Orden recomendado
1. Tick planificado por chunk en Xbox (activar el planificador de PC-legacy). Medir `chunkUnload` antes y después.
2. Medir `textureStage` con TODAS las animaciones apagadas, para saber si los 5 ms son reales.
3. Dropped Items rápido y Potion Particles (opciones nuevas, baratas).
4. Frustum «entero dentro» y la capacidad inicial de `LongHashMap`.
5. Comparar nuestro presupuesto de mallas con el adaptativo de `WrUpdaterSmooth`.

## Nota legal
OptiFine es de código cerrado (sp614x). El descompilado sirve como **guía de técnicas**: el código no se copia tal cual en builds públicas. Lo que se haga se escribe nuestro, a partir de la idea, como ya se hizo con el OptiFine 1.2.5 que trae OptiCraft.
