# 88. Informe: niebla del Nether, zona segura del HUD y mismas opciones en 1.2.5 y 1.5.2

- **Rama 1.5.2**: `port152-erluza` en `wt-152-rend` (sobre `a25a953` → **`7b69e69`**, 3 commits):
  1. `efc205f` — Niebla del Nether/End en Xbox.
  2. `c0e77aa` — Zona segura del HUD (0–10 %, Java + Legacy + vista previa en vivo).
  3. `7b69e69` — Unificación de menús de Opciones/Vídeo (Pocket, Hielo opaco y Zona segura en página 1).
- **Rama 1.2.5**: `opti30-erluza` en `wt-opti30` (sobre `9787bb1` → **`0b46eac`**, 3 commits):
  1. `a829d2c` — Niebla del Nether/End en Xbox.
  2. `9c47796` — Zona segura del HUD (0–10 %, Java + Legacy + vista previa en vivo).
  3. `0b46eac` — Port de `LegacyAdvancedVideoOptions` + unificación total de opciones con 1.5.2.
- **Reglas respetadas**: sin `reset`, sin `push`, sin tocar otras ramas ni el lanzador, sin tocar Creativo ni worldgen, respetando `BUILD.lock`, 0 avisos nuevos de SSE2, PS2 compilado y sin cambios de comportamiento en PC/PS2/Wii.

---

## 1. Niebla del Nether (solo `#if PLATFORM_XBOX`)

### Causa y cambio (`archivo:línea`)
- **1.5.2**: `src/net/minecraft/src/EntityRenderer.cpp:3053-3086` (`EntityRenderer::setupFog`)
- **1.2.5**: `src/net/minecraft/src/EntityRenderer.cpp:2993-3026` (`EntityRenderer::setupFog`)

Antes, en el Nether y el End (`func_48218_b`), Xbox usaba `fogStart = effectiveFar * 0.05f` (que con la distancia de visión de consola `32..128` dejaba el inicio de la niebla a **1,6–6,4 bloques** de la cara) e ignoraba tanto `ofFogStart` como `ofFogOff` (`!isNether`).

Ahora, bajo `#if PLATFORM_XBOX`:
- Para `fogMode >= 0`: `fogEnd = effectiveFar`, `fogStart = std::max(12.0f * (fogStartOpt / 0.8f), fogEnd * (0.45f * fogStartOpt))` limitado a `0.65f * fogEnd` (respetando `Config::getFogStart()` en `0.2..0.8`, por defecto `0.8`).
- Para `Config::isFogOff() && fogMode >= 0`: se aplica también en el Nether/End (`4 * effectiveFar` .. `8 * effectiveFar`), permitiendo desactivar la niebla si el usuario la apaga.

### Comparativa de valores de niebla (bloques)

| Distancia en Xbox (`ofRenderDistanceFine`) | Java PC referencia (Lejos = 256) | Xbox **Antes** (`0.05 * far .. far`) | Xbox **Después** (`Fog Start = 0.8`, defecto) | Xbox **Después** (`Fog Start = 0.2 / 0.4 / 0.6`) | Xbox **`Fog: OFF`** |
|---|---|---|---|---|---|
| Mínima / Tiny (`32`) | — | `1.60 .. 32.00` | **`12.00 .. 32.00`** | `3.00 / 6.00 / 9.00 .. 32.00` | `128 .. 256` |
| Corta / Short (`64`, **defecto Xbox**) | `12.80 .. 96.00` (23 % niebla a 32 bl.) | `3.20 .. 64.00` (47 % niebla a 32 bl.) | **`23.04 .. 64.00`** (22 % niebla a 32 bl., 61 % a 48 bl.) | `5.76 / 11.52 / 17.28 .. 64.00` | `256 .. 512` |
| Normal (`128`, **máx. Xbox**) | `12.80 .. 96.00` | `6.40 .. 128.00` | **`46.08 .. 128.00`** | `11.52 / 23.04 / 34.56 .. 128.00` | `512 .. 1024` |

*(Nota sobre capturas: el entorno headless devuelve `Win32Exception`/negro con `CopyFromScreen`/`PrintWindow` sobre OpenGL de xemu y esta build de xemu no incluye QMP `screendump`; los valores anteriores están medidos en vivo en `dim=-1` desde `XboxLogRing` en xemu 64 MB en ambas versiones).*

---

## 2. Zona segura del HUD (0–10 %, pasos de 1 %, Java + Legacy)

### Archivos y líneas modificadas (idénticos en 1.5.2 y 1.2.5)
- `src/net/minecraft/src/HudSafeArea.h` (nuevo): cálculo unificado `computeHudSafeInsets(mc, sw, sh, applySplitOuterEdgesOnly)` (en pantalla partida aplica el margen solo a los bordes exteriores de la TV, calculado sobre la altura/anchura total de TV para que J2 abajo tenga el mismo porcentaje físico que en 1 jugador) y `drawHudSafeAreaPreview(...)` (marco con escuadras amarillas y banda exterior tenue).
- `src/net/minecraft/src/EnumOptions.h:90` y `EnumOptions.cpp:47,113`: `EnumOptions::HUD_SAFE_AREA` (`"Safe Area"`).
- `src/net/minecraft/src/GameSettings.h:88`, `GameSettings.cpp:107-111,532-535,937-938,998-999` y `GameSettingsPersistence.cpp:152-153,424,485`: campo `hudSafeArea` (defecto **`4`** en `#if PLATFORM_XBOX`, **`0`** en PC/PS2/Wii), guardado en `options.txt` como `hudSafeArea:<0..10>`.
- `src/net/minecraft/src/GuiButton.h:51`, `GuiSlider.cpp:66-74` y `legacy/LegacyOptionSlider.cpp:169-170`: paso de 1 % (`0.1f`) con cruceta/teclado y estado de foco para la vista previa.
- `src/net/minecraft/src/GuiIngame.h:33-35` y `GuiIngame.cpp:43,260-360,446-455,844-847,935-951,996-1002,1026`: reutiliza y unifica el margen de esquinas (`kHudCornerBaseX/Y` + `debugSafe` conservando el piso de 12 px en Wii/PS2), eleva `hudHeight` (barra de objetos, selector, iconos, corazones, comida, armadura, burbujas, barra y nivel de XP, título de disco y chat), mete el chat hacia dentro en X, baja la barra de jefe y la lista de jugadores. Aplica automáticamente a J1 y J2 en pantalla partida.
- `src/net/minecraft/src/GuiChat.cpp:12,55-63,252-261`: caja de entrada de chat ajustada a `safe.left`, `safe.right` y `safe.bottom`.
- `src/net/minecraft/src/GuiAchievement.cpp:10,110-112`: notificación de logros desplazada hacia dentro.
- `src/net/minecraft/src/legacy/LegacyMenuHints.h:12` y `LegacyMenuHints.cpp:7-12`: `legacyHintRowY()` resta `safe.bottom`, moviendo al instante tanto la fila de botones del mando en partida como los consejos de todos los menús Legacy (y ajustando el presupuesto de filas de `LegacyOptionsLayout`).
- `src/net/minecraft/src/legacy/LegacyControlTooltipHud.cpp:10,115-117,138-140,169-193`: aplica `safe.left`/`safe.right`/`safe.bottom` e incluye los 3 valores en la clave de caché de `PromptRow`.
- `src/net/minecraft/src/legacy/LegacyTipHud.cpp:8,172-174`: panel de consejos tutoriales desplazado hacia dentro.
- `src/net/minecraft/src/GuiVideoSettings.cpp:17,45,131-158` y `legacy/LegacyVideoOptions.cpp:16,80,178-181,316-331`: deslizador **Zona segura** en ambas interfaces con vista previa en vivo al seleccionarlo/moverlo.

### Verificación de layout 480p y 720p (xemu 64 MB)

| Resolución lógica (modo) | Zona segura | `safe (L,T,R,B)` | `legacyHintRowY` | `hudHeight` (Legacy) | `LegacyVideoOptions` (`maxRows` / `rpp` / páginas) |
|---|---|---|---|---|---|
| `320×240` (480p Auto) | `0%` | `(0, 0, 0, 0)` | `225` | `212` | `11` / `9` / `2` |
| `320×240` (480p Auto) | **`4%` (defecto)** | **`(13, 10, 13, 10)`** | **`215`** (icono hasta `225`, 6,25 % libre) | **`202`** | `11` / **`9`** / `2` (pág. 1 caben los 8 principales + balanceo) |
| `427×240` (720p Auto ×3) | **`4%` (defecto)** | **`(17, 10, 17, 10)`** | **`215`** (45 px físicos libres abajo) | **`202`** | `11` / **`9`** / `2` |
| `640×360` (720p Normal ×2) | **`4%` (defecto)** | **`(26, 14, 26, 14)`** | **`331`** (38 px físicos libres abajo) | **`318`** | `16` / **`14`** / `1` (todo en 1 página) |
| `320×240` / `427×240` | `10%` (máx.) | `(32/43, 24, 32/43, 24)` | `201` | `188` | `10` / **`8`** / `2` (pág. 1 siguen cabiendo los 8 principales) |

---

## 3. Mismas opciones en 1.2.5 y 1.5.2 (Java + Legacy)

Se portaron a 1.2.5 `LegacyAdvancedVideoOptions.h/.cpp`, las mejoras 480p de `LegacyOptionsLayout.cpp`, el ajuste de extremos de `LegacyOptionSlider.cpp` y las 78 cadenas de traducción al español en `StringTranslate.cpp`. Además, en **ambas ramas** se añadieron `Show Coordinates` y `Fullbright` a `GuiOtherSettingsOF` (para que también existan en la UI Java), se tradujeron los 5 botones de submenús OptiFine en `GuiVideoSettings` y se unificó `LegacyVideoOptions` con paginación automática y **Zona segura**, **Optimizaciones Pocket**, **Hielo opaco** y **Avanzado...** en la página 1.

**Resultado**: los **24 archivos** de pantallas de opciones (`Gui*Options/Settings*.cpp`, `HudSafeArea.h` y todos los `legacy/Legacy*Options*.h/.cpp`) son ahora **100 % idénticos bit a bit** entre 1.5.2 y 1.2.5.

### Tabla completa de opciones (1.5.2 vs 1.2.5)

| Pantalla | Interfaz | Opciones | 1.5.2 | 1.2.5 | Notas |
|---|---|---|---|---|---|
| `GuiOptions` | Java | Música, Sonido, Invertir ratón, Sensibilidad, FOV, Dificultad, Vídeo…, Controles…, Idioma…, Opciones OptiCraft… | Sí | Sí | Idénticas |
| `GuiVideoSettings` | Java | Gráficos, Distancia de visión (fina), Iluminación suave, Límite FPS, Anaglifo, Balanceo, Escala GUI, OpenGL avanzado, Niebla, Inicio niebla, Brillo, **Zona segura**, Animaciones…, Detalles…, Calidad…, Rendimiento…, Paquetes de texturas…, Otros… | Sí | Sí | Idénticas (Zona segura ocupa el hueco 12 sin crecer en altura) |
| `GuiDetailSettingsOF` | Java | Nubes, Altura nubes, Árboles, Hierba, Agua, Lluvia y nieve, Cielo, Estrellas, Sol y luna, Niebla de profundidad | Sí | Sí | Idénticas |
| `GuiQualitySettingsOF` | Java | Agua clara, Criaturas aleatorias*, Hierba mejorada, Nieve mejorada, Colores personalizados, Colores de pantano, Biomas suavizados, Texturas conectadas, Texturas naturales, Mipmap*, Anisotrópico*, Antialiasing*, Fuentes personalizadas* | Sí | Sí | Idénticas (*filtradas por capacidad de backend/plataforma) |
| `GuiAnimationSettingsOF` | Java | Agua, Lava, Fuego, Portal, Redstone, Explosión, Llama, Humo, Partículas vacío/agua/lluvia/portal, Partículas, Goteo agua/lava, Terreno, Objetos, Texturas + Todo SÍ / Todo NO | Sí | Sí | Idénticas |
| `GuiPerformanceSettingsOF` | Java | FPS estables, Entrada fluida, Cargar lejos, Chunks precargados, Actualizaciones de chunks, Actualizaciones dinámicas, **Hielo opaco**, **Optimizaciones Pocket** | Sí | Sí | Idénticas |
| `GuiOtherSettingsOF` | Java | Backend Render (solo PC Win32), Depuración rápida, Perfilador, Clima, Tiempo, Autoguardado, **Mostrar coordenadas** (Xbox), **Brillo máximo** (Xbox) | Sí | Sí | Idénticas |
| `GuiWorldSettingsOF` | Java | Distancia de visión fina, Vista lejana | Sí | Sí | Idénticas |
| `GuiOptiCraftOptions` | Java | Nombre jugador, Legacy UI, Legacy Look, Fabricación consola/Legacy, Creativo Legacy, Relación de aspecto*, Controles alternativos (Wii), Zona muerta* | Sí | Sí | Idénticas |
| `GuiDeadzoneSettings` / `GuiBetaOptions` | Java | Calibración zona muerta / Opciones Beta | Sí | Sí | Idénticas |
| `LegacyHelpOptions` | Legacy | Vídeo, Controles, Idioma, Opciones OptiCraft, Vista, Atrás | Sí | Sí | Idénticas |
| `LegacyVideoOptions` | Legacy | **Pág. 1**: Gráficos detallados, Iluminación suave, Distancia de visión, Brillo, **Zona segura**, **Optimizaciones Pocket**, **Hielo opaco**, **Avanzado…** (+ Balanceo). **Pág. 2** (auto si > `maxRows`): Nubes, Niebla, Límite 30 FPS (Xbox), Mostrar coordenadas (Xbox), Brillo máximo (Xbox) *(Wii: Filtro antiparpadeo)* | Sí | Sí | Idénticas; PS2/Wii siguen en 1 sola página |
| `LegacyAdvancedVideoOptions` | Legacy | **5 secciones** idénticas a OptiFine: **Detalles** (12 opciones), **Animaciones** (17 + Todo SÍ/NO), **Calidad** (hasta 15), **Rendimiento** (10, incluyendo **Hielo opaco** y **Optimizaciones Pocket**), **Otros** (6) | Sí | **Portado** | Idénticas en 1.5.2 y 1.2.5 |
| `LegacyViewOptions` | Legacy | FOV, Sensibilidad, Invertir ratón, Dificultad, Música, Sonido, Dolby Digital (Xbox) | Sí | Sí | Idénticas |
| `LegacyHeritageOptions` | Legacy | Nombre J1 (+ J2 en Xbox), Relación de aspecto*, Pantalla partida (PS2), Legacy UI, Legacy Look, Fabricación consola, Creativo Legacy, Controles alt. (Wii), Zona muerta* | Sí | Sí | Idénticas |
| `LegacyDebugOptions` | Legacy | Mostrar FPS, Info F3 extendida, Hacer de día, Modo de juego, Conservar inventario, Eliminar entidades | Sí | Sí | Idénticas |
| `LegacyLanguageOptions` | Legacy | Selector de idioma | Sí | Sí | Idénticas |

*(No hay opciones excluidas por diferencia entre Java 1.2.5 y 1.5.2: todas las opciones de vídeo, rendimiento, controles y HUD existen y funcionan en ambas versiones).*

### Qué hace cada opción nueva / destacada (en una frase)
- **Zona segura (`Safe Area`)**: mete hacia dentro todo el HUD 2D (hotbar, corazones, iconos del mando, chat, esquinas, logros y pantalla partida) entre 0 % y 10 % para evitar recortes por overscan de la tele sin escalar el mundo 3D.
- **Optimizaciones Pocket (`Pocket Optimizations`)**: perfil opcional (apagado por defecto) que recorta partículas, lluvia, distancia de entidades/tile entities y frecuencia de lightmap/sondeos para ganar FPS en zonas cargadas.
- **Hielo opaco (`Opaque Ice`)**: dibuja el hielo como bloque sólido opaco en el pase 0 en vez de translúcido, ahorrando relleno de píxeles en biomas helados.
- **Avanzado… (`LegacyAdvancedVideoOptions`)**: da acceso desde el menú Legacy a todas las secciones de OptiFine (Detalles, Animaciones, Calidad, Rendimiento y Otros) con paginación adaptada a 480p y 720p.
- **Mostrar coordenadas / Brillo máximo en UI Java (`GuiOtherSettingsOF`)**: permite activar en la interfaz Java las mismas dos opciones exclusivas de Xbox que ya estaban en Legacy Vídeo.

---

## 4. Compilación y mediciones en xemu 64 MB

- **1.5.2 (`port152-erluza` `7b69e69`)**: `xbox-release` y `xbox-release-exp` (480p + 720p) compilan limpio (exit 0, **0 avisos nuevos de SSE2**).
  - Arranque limpio: `selftest: 0 failure(s)`, `double53=1`, `0 nan`, `0 [X87]`. `freeKB`: `45628` (rel f601), `41064` (exp f901; antes `41068`).
  - Partida semilla `1621073931`: spawn idéntico `(-189.50, 65.62, 42.50)` chunk `(-12, 2)`, `hud=0.1 ms`, `freeKB=36120` (f901).
- **1.2.5 (`opti30-erluza` `0b46eac`)**: `build_opti30.ps1` compila limpio `xbox-release` (480p + 720p), `xbox-release-exp` (480p + 720p) y **`ps2-release`** (exit 0, **0 avisos nuevos de SSE2**).
  - Arranque limpio: `selftest: 0 failure(s)`, `double53=1`, `0 nan`, `0 [X87]`. `freeKB`: `46384` (rel f601), `42100` (exp f901; antes `42124`).
  - Partida semilla `1621073931`: spawn idéntico `(43.50, 49.62, 55.50)` chunk `(2, 3)` y **hash de terreno T0 idéntico `b8dbea09`**; Nether estable `fps=47.3` (`21.2 ms`), `ent=0.1 ms`, `hud=0.1–0.8 ms`.

---

## 5. Qué probar en la Xbox real

1. **Niebla del Nether (1.5.2 y 1.2.5)**: entrar al Nether con distancia por defecto (`Corta` / 64) y confirmar que ya se ve con claridad alrededor (niebla densa a media distancia, sin muro rojo en la cara). Probar a cambiar `Inicio de niebla` (`0.2`..`0.8`) y `Niebla: NO` en Avanzado → Detalles para comprobar que responde.
2. **Zona segura en 720p y 480p**: comprobar que con el valor por defecto (`4%`) los iconos de botones del mando abajo en 720p quedan completamente dentro de la tele y que 480p sigue viéndose bien. Entrar en Vídeo (tanto en Legacy como en Java) y mover el deslizador `Zona segura` de `0%` a `10%`: verificar la vista previa en vivo (escuadras amarillas + movimiento inmediato de los botones de abajo), salir al mundo (y probar pantalla partida con J2) y reiniciar el juego para confirmar que se guarda en `options.txt`.
3. **Opciones unificadas en 1.2.5 y 1.5.2**: en Legacy → Vídeo comprobar que en la **página 1** salen `Zona segura`, `Optimizaciones Pocket`, `Hielo opaco` y `Avanzado…` (y en página 2 a 480p el resto sin solaparse), y entrar en `Avanzado…` en 1.2.5 para verificar las 5 secciones en español igual que en 1.5.2.
