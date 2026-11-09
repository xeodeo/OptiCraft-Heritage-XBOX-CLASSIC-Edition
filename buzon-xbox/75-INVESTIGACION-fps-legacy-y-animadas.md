# 75. Por qué el Legacy UI y las texturas animadas cuestan fps (09-11-2026)

Solo lectura de código (`wt-152-pociones`, igual en `wt-opti30`) y del netlog viejo
(`OptiCraftHeritageEdition\xbox-spike\netlog.txt`, 1.2.5 exp). No se ha tocado ni compilado nada.

## Resumen
- **Texturas animadas:** con las animaciones apagadas, agua, lava, fuego y portal ya **no** se suben (bien hecho).
  Lo que sigue gastando es **la brújula y el reloj**: se suben siempre, cada tick, aunque nadie los lleve. Cada subida
  re-swizzlea el atlas de objetos entero (256x256, 256 KB) con un `LockRect` que espera a la GPU.
  Con las animaciones encendidas (vienen así por defecto) además se re-swizzlea el atlas de terreno entero cada tick.
  De ahí los picos de `textureStage` del netlog: mediana 1,5 ms y p90 5,3 ms por frame, con máximo de 12 ms.
- **Legacy UI en partida:** cuesta poco en CPU. Todo el HUD mide 0,6 ms de mediana en el netlog. Lo que añade es la
  fila de botones de abajo (5 iconos dibujados uno a uno y unos 35 caracteres con sombra, cada frame) y el panel de
  consejos cuando hay uno.
  Lo más gordo es la **memoria**: el título y el panorama del menú Legacy (2 + 2 + 1 MB) se quedan cargados durante
  toda la partida. Son 5 MB de los 64, y la 1.5.2 va con solo 9-16 MB libres.

## B. Texturas animadas (ordenado por ganancia/esfuerzo)

### Cómo funciona hoy
1. Los efectos se actualizan en `RenderEngine::updateDynamicTextures` (`RenderEngine.cpp:1768`), una vez por tick.
2. Con la animación apagada:
   - `updateStaticProceduralTextureFx` (`:1631`) sube un único fotograma y no vuelve a subir.
   - `updateCustomAnimations` (`:1061`) se salta las animaciones apagadas.
   - `updateDefaultTerrainTextureFx` devuelve `HandledNoUpload`.
3. `TextureCompassFX` y `TextureWatchFX` no redefinen `isAnimationEnabled()`, y la versión base
   (`TextureFX.cpp:21`) devuelve `true`. Por eso **cada tick** pasan por `uploadTextureFxTile` (`RenderEngine.cpp:1435`),
   haya cambiado algo o no.
4. `renderTextureSubImageRgba` (`RenderAPI_D3D8_XBOX.cpp:1678`) copia el tile en la copia de CPU y marca `t.dirty`
   (`:1701`). En el siguiente bind, `applyTextureStage` (`:516-520`) llama a `swizzleUpload` (`:391`).
5. `swizzleUpload` hace:
   - `LockRect(0, NULL, 0)`, que **bloquea** hasta que la GPU deja de usar la textura. El atlas de objetos se usa en
     cada frame (barra rápida, objeto en la mano), así que casi siempre espera.
   - `XGSwizzleRect` del atlas **entero** (256 KB).
   - Esto se repite 20 veces por segundo por cada atlas sucio. `textureStage` incluye esa espera y ese swizzle.

### Arreglos
| # | Qué | Dónde | Ganancia | Esfuerzo |
|---|---|---|---|---|
| 1 | **No subir un tile igual al anterior.** Guardar los 1 KB del último `imageData` subido por cada efecto y hacer `memcmp` antes de subir. La brújula solo cambia al girar y el reloj cada pocos segundos. | `RenderEngine.cpp:1435` (`uploadTextureFxTile`) y `:1798` | Quita casi todas las subidas del atlas de objetos con las animaciones apagadas: los picos de 1-5 ms. | Muy baja |
| 2 | **Swizzle parcial.** Guardar el rectángulo sucio (unión de los tiles) en `renderTextureSubImageRgba` y swizzlear solo ese rectángulo con `XGSwizzleRect(src, pitch, &rect, bits, W, H, &point, 4)`, alineado a 8x4. Si la textura está re-muestreada (no potencia de 2), seguir con la subida entera. | `RenderAPI_D3D8_XBOX.cpp:1678-1701` (guardar el rectángulo) y `:391-428` (usarlo) | Con las animaciones encendidas: de 2 swizzles de 256 KB por tick a unos 9 tiles de 1 KB. | Baja-media |
| 3 | **Lock sin espera:** `LockRect(0, &locked, NULL, D3DLOCK_NOOVERWRITE)`. Según la XDK no bloquea aunque la GPU use la textura. El riesgo es ver un tile a medio cambiar durante 1 frame, invisible en agua y lava. Alternativa sin riesgo: dos texturas D3D por atlas animado que se alternan (+256 KB). | `RenderAPI_D3D8_XBOX.cpp:420` | Quita la espera a la GPU, que probablemente es la mayor parte de `textureStage`. | Baja |
| 4 | **Medir:** separar en `xbox.draw` el tiempo de `LockRect`, el de `XGSwizzleRect` y el número de swizzles por segundo. | `RenderAPI_D3D8_XBOX.cpp:391-428` y `Profiler_XBOX.cpp:219` | Confirma 1-3 antes y después. | Muy baja |
| 5 | (Opcional) Brújula y reloj solo si hay alguno visible (en la mano, en la barra, en el inventario abierto o en un marco). | `RenderEngine.cpp:1787` | Con el punto 1 ya casi no hace falta. | Media |

Otros casos que hacen lo mismo y no son de animación: un mapa en la mano sube su textura de 128x128 entera cada
frame (por el mismo `t.dirty`). Con el punto 2 se arregla también.

## A. Legacy UI durante la partida

Qué hace `legacyUI` en partida (fuera de los menús):

| Qué | Dónde | Coste estimado en la Xbox |
|---|---|---|
| Fila de botones de abajo (Y Inventario, X Crafteo, A, RT, LT). Cada frame genera 5 etiquetas con `std::string`, `iconFromName`, 5 `getTexture("/gui/buttons_xbox.png")` (dos búsquedas en mapas con string cada uno) y 5 iconos **en 5 draws separados**, cada uno con su bind y sus estados (`ControlIcon.cpp:105`). Después tesela unos 35 caracteres con sombra (unos 70 quads) en un lote. La fila de texto **no se cachea en Xbox**: la caché `captureTextBatch` solo está activada para PS2. | `legacy/LegacyControlTooltipHud.cpp:242-268`, `:199-238` | 0,2-0,5 ms por frame |
| Panel de consejos (nine-slice de 9 quads, más el texto escalado) mientras hay un consejo en cola, como el de bienvenida | `legacy/LegacyTipHud.cpp:140-203` | 0,1-0,3 ms, solo cuando sale |
| Escala de GUI grande | `ScaledResolution.cpp:40` | Solo relleno de GPU, que no es el cuello |
| Texturas del menú Legacy cargadas toda la partida: panorama 640x480 (guardado 1024x512, 2 MB), título 1230x216 (2048x256, 2 MB), 995x205 (1 MB) | `legacy/LegacyUiAssets.cpp:19-49` (caché del título, nunca se libera), y el panorama del menú | **5 MB de RAM.** Los fps no suben directamente, pero la 1.5.2 va justa (9-16 MB libres) y con poca memoria hay más tirones del heap y de la descarga de chunks |
| `legacyLook` (otra opción): en Xbox es la rampa de gamma por hardware, gratis. La viñeta se apaga, lo que ahorra. | `RenderAPI_D3D8_XBOX.cpp:1970`, `GuiIngame.cpp:814` | ~0 |

### Arreglos (mantienen el aspecto)
1. **Liberar las texturas del menú al entrar al mundo** (panorama, título y la de 995x205) y volver a cargarlas al
   salir al menú. O guardarlas a potencia de 2 sin desperdicio: 1024x256 en vez de 2048x256, o una textura lineal.
   Dónde: `LegacyUiAssets.cpp:42-49` (deleteTexture y `texture=-1` al cargar mundo) y `RenderAPI_D3D8_XBOX.cpp:404-416`.
   **+5 MB libres en partida.** Esfuerzo bajo. Ya estaba en el informe 68, punto 9.
2. **Iconos de la fila en un solo draw:** bindear el atlas una vez y meter los 5 quads en un solo
   `startDrawingQuads`/`draw` (copiar el bucle de `drawControlIcon`, `ControlIcon.cpp:105-131`, dentro de
   `LegacyControlTooltipHud.cpp:263`). Guardar el `getTexture` del atlas en la fila. Esfuerzo muy bajo.
3. **Cachear el texto de la fila en Xbox:** activar el camino `captureTextBatch`/`drawCapturedText` (ya existe:
   `renderCaptureInterleaved` y `renderDrawCaptured` en `RenderAPI_D3D8_XBOX.cpp:1845`/`:1878`) también para
   `PLATFORM_XBOX` en `LegacyControlTooltipHud.cpp:199`, `:231` y `:260`. El texto se tesela una vez y se repite tal cual.
   Esfuerzo bajo.
4. **En Xbox las etiquetas son fijas** (`LegacyControlPromptBackend_XBOX.cpp:7`): `refreshRowKey` puede saltarse
   generar las 5 etiquetas y buscar los iconos si el idioma y el tamaño no cambiaron. Esfuerzo muy bajo.

### Cómo comprobarlo
El netlog ya da `xbox.perf ... hud=` por frame. Basta medir 1 minuto quieto con Legacy UI encendido y otro apagado,
mirando `hud=` y `freeKB`. Si `hud` baja menos de 0,5 ms, lo que notó el usuario es sobre todo memoria o ruido.
La regla de Melee-X dice que repitamos cualquier mejora de menos del 3 %.
