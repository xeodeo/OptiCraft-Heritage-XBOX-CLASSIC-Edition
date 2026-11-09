# Informe Encargo 80 — Opciones gráficas avanzadas (OptiFine) en la interfaz Legacy

## 1. Resumen de cambios

1. **Botón «Avanzado...» en `LegacyVideoOptions`** (`src/net/minecraft/src/legacy/LegacyVideoOptions.cpp`):
   - Se añadió `BUTTON_ADVANCED` (`uiText("Advanced...")`) justo antes de `BUTTON_DONE` (`uiText("Done")`).
   - Al pulsarlo se abre `LegacyAdvancedVideoOptions(this, settings, backgroundMode)`.
   - Al volver desde `LegacyAdvancedVideoOptions` a `LegacyVideoOptions`, `setWorldAndResolution` / `initGui` reconstruye y sincroniza todas las casillas y barras (`Render Clouds`, `Fog`, `Smooth Lighting`, etc.) con los valores actualizados en `GameSettings`.

2. **Nueva pantalla `LegacyAdvancedVideoOptions`** (`src/net/minecraft/src/legacy/LegacyAdvancedVideoOptions.h` y `.cpp`):
   - Pantalla principal (`Category::Hub`, 6 filas en preset `Compact`) con sub-botones para las 5 secciones de OptiFine y botón `Done`:
     - **Detalles...** (`Details...` -> `Category::Details`)
     - **Animaciones...** (`Animations...` -> `Category::Animations`)
     - **Calidad...** (`Quality...` -> `Category::Quality`)
     - **Rendimiento...** (`Performance...` -> `Category::Performance`)
     - **Otros...** (`Other...` -> `Category::Other`)
     - **Listo** (`Done`)
   - En cada subsección:
     - Las opciones booleanas usan `LegacyEnumCheckbox` (derivada de `LegacyOptionCheckbox`), que alterna con **A** y también con **Izquierda/Derecha** en cruceta o stick (`adjustKeyboard`).
     - Las opciones de barra continua (`CLOUD_HEIGHT`, `AO_LEVEL`) usan `LegacyOptionSlider`, que ajusta de 5 % en 5 % con **Izquierda/Derecha** (`adjustKeyboard`) y hace snap exacto a `0.0f` (`OFF`) y `1.0f` (`100%`).
     - Las opciones discretas multivalor usan `LegacyEnumCycleButton` (derivada de `LegacyGuiButton`), que avanza con **A** o **Derecha** y retrocede al valor anterior con **Izquierda**, delegando siempre el cambio final a `GameSettings::setOptionValue` para ejecutar una sola vez las invalidaciones de malla/texturas y el guardado en `options.txt`.
     - Cuando una subsección supera el alto disponible (`Details` con 12 ítems y `Animations` con 19 ítems), se activa paginación dinámica (`Previous` / `Next` en fila compartida más `Done` debajo, con soporte de **Izquierda/Derecha** para mover el foco entre `Previous` y `Next`).

3. **Ajuste de alto de fila en `LegacyOptionsLayout`** (`src/net/minecraft/src/legacy/LegacyOptionsLayout.cpp`):
   - Al pasar `LegacyVideoOptions` de 11 a 12 filas en Xbox, en **1280x720** (`640x360` lógicos) el alto disponible entre el logo (`availableTop = 113`) y la barra inferior de iconos (`footerTop = 331`) es de `218 px`. Con `rowHeight = 16` las 12 filas medían `230 px`.
   - En `legacyOptionsLayout`, cuando `profile.scaleRows` está activo y `panelHeight > footerTop - availableTop`, `rowHeight` se reduce automáticamente (hasta el mínimo de `14 px`), quedando en `15 px` para 12 filas en 720p (`panelHeight = 218 px`), sin tocar el logo superior ni la fila de iconos inferior y sin alterar ninguna pantalla de `<= 11` filas.

4. **Localización en `StringTranslate`** (`src/net/minecraft/src/StringTranslate.cpp`):
   - Se añadieron las traducciones al español (`es_*`) en `StringTranslate::translateUi` para `"Advanced"`, `"Details"`, `"Animations"`, `"Quality"`, `"Performance"`, `"Other"`, `"All ON"`, `"All OFF"` y todas las etiquetas y valores de OptiFine, manteniendo el fallback en inglés para otros idiomas si no están en `/lang/ui/<lang>.lang`.

---

## 2. Opciones incluidas por categoría y conexión con `GameSettings`

### Detalles (`Category::Details` — 12 ítems, 2 páginas de 8 + 4)
| Opción | `EnumOptions` | Campo en `GameSettings` | Control Legacy |
|---|---|---|---|
| Clouds | `EnumOptions::CLOUDS` | `ofClouds` (0..3: Default/Fast/Fancy/OFF) | `LegacyEnumCycleButton` |
| Cloud Height | `EnumOptions::CLOUD_HEIGHT` | `ofCloudsHeight` (0.0..1.0) | `LegacyOptionSlider` |
| Trees | `EnumOptions::TREES` | `ofTrees` (0..2: Default/Fast/Fancy) | `LegacyEnumCycleButton` |
| Grass | `EnumOptions::GRASS` | `ofGrass` (0..2: Default/Fast/Fancy) | `LegacyEnumCycleButton` |
| Water | `EnumOptions::WATER` | `ofWater` (0..2: Default/Fast/Fancy) | `LegacyEnumCycleButton` |
| Rain & Snow | `EnumOptions::RAIN` | `ofRain` (0..3: Default/Fast/Fancy/OFF) | `LegacyEnumCycleButton` |
| Sky | `EnumOptions::SKY` | `ofSky` (`bool`) | `LegacyEnumCheckbox` |
| Stars | `EnumOptions::STARS` | `ofStars` (`bool`) | `LegacyEnumCheckbox` |
| Sun & Moon | `EnumOptions::SUN_MOON` | `ofSunMoon` (`bool`) | `LegacyEnumCheckbox` |
| Depth Fog | `EnumOptions::DEPTH_FOG` | `ofDepthFog` (`bool`) | `LegacyEnumCheckbox` |
| Fog | `EnumOptions::FOG_FANCY` | `ofFogFancy` / `ofFogOff` | `LegacyEnumCycleButton` |
| Fog Start | `EnumOptions::FOG_START` | `ofFogStart` (0.2 / 0.4 / 0.6 / 0.8) | `LegacyEnumCycleButton` |

### Animaciones (`Category::Animations` — 19 ítems, 3 páginas de 8 + 8 + 3)
| Opción | `EnumOptions` / Acción | Campo en `GameSettings` | Control Legacy |
|---|---|---|---|
| Water Animated | `EnumOptions::ANIMATED_WATER` | `ofAnimatedWater` (0=ON, 1=Dynamic, 2=OFF) | `LegacyEnumCycleButton` |
| Lava Animated | `EnumOptions::ANIMATED_LAVA` | `ofAnimatedLava` (0=ON, 1=Dynamic, 2=OFF) | `LegacyEnumCycleButton` |
| Fire Animated | `EnumOptions::ANIMATED_FIRE` | `ofAnimatedFire` (`bool`) | `LegacyEnumCheckbox` |
| Portal Animated | `EnumOptions::ANIMATED_PORTAL` | `ofAnimatedPortal` (`bool`) | `LegacyEnumCheckbox` |
| Redstone Animated | `EnumOptions::ANIMATED_REDSTONE` | `ofAnimatedRedstone` (`bool`) | `LegacyEnumCheckbox` |
| Explosion Animated | `EnumOptions::ANIMATED_EXPLOSION` | `ofAnimatedExplosion` (`bool`) | `LegacyEnumCheckbox` |
| Flame Animated | `EnumOptions::ANIMATED_FLAME` | `ofAnimatedFlame` (`bool`) | `LegacyEnumCheckbox` |
| Smoke Animated | `EnumOptions::ANIMATED_SMOKE` | `ofAnimatedSmoke` (`bool`) | `LegacyEnumCheckbox` |
| Void Particles | `EnumOptions::VOID_PARTICLES` | `ofVoidParticles` (`bool`) | `LegacyEnumCheckbox` |
| Water Particles | `EnumOptions::WATER_PARTICLES` | `ofWaterParticles` (`bool`) | `LegacyEnumCheckbox` |
| Rain Splash | `EnumOptions::RAIN_SPLASH` | `ofRainSplash` (`bool`) | `LegacyEnumCheckbox` |
| Portal Particles | `EnumOptions::PORTAL_PARTICLES` | `ofPortalParticles` (`bool`) | `LegacyEnumCheckbox` |
| Dripping Water/Lava | `EnumOptions::DRIPPING_WATER_LAVA` | `ofDrippingWaterLava` (`bool`) | `LegacyEnumCheckbox` |
| Terrain Animated | `EnumOptions::ANIMATED_TERRAIN` | `ofAnimatedTerrain` (`bool`) | `LegacyEnumCheckbox` |
| Items Animated | `EnumOptions::ANIMATED_ITEMS` | `ofAnimatedItems` (`bool`) | `LegacyEnumCheckbox` |
| Textures Animated | `EnumOptions::ANIMATED_TEXTURES` | `ofAnimatedTextures` (`bool`) | `LegacyEnumCheckbox` |
| Particles | `EnumOptions::PARTICLES` | `particleSetting` (0=All, 1=Decreased, 2=Minimal) | `LegacyEnumCycleButton` |
| All ON | `BUTTON_ALL_ON` | `settings->setAllAnimations(true)` | `LegacyGuiButton` |
| All OFF | `BUTTON_ALL_OFF` | `settings->setAllAnimations(false)` | `LegacyGuiButton` |

### Calidad (`Category::Quality` — 9 ítems en Xbox, 1 sola vista sin paginar)
| Opción | `EnumOptions` | Campo en `GameSettings` | Control Legacy |
|---|---|---|---|
| Smooth Lighting | `EnumOptions::AO_LEVEL` | `ofAoLevel` (0.0..1.0) + `ambientOcclusion` | `LegacyOptionSlider` |
| Clear Water | `EnumOptions::CLEAR_WATER` | `ofClearWater` (`bool`) | `LegacyEnumCheckbox` |
| Better Grass | `EnumOptions::BETTER_GRASS` | `ofBetterGrass` (1=Fast, 2=Fancy, 3=OFF) | `LegacyEnumCycleButton` |
| Better Snow | `EnumOptions::BETTER_SNOW` | `ofBetterSnow` (`bool`) | `LegacyEnumCheckbox` |
| Custom Colors | `EnumOptions::CUSTOM_COLORS` | `ofCustomColors` (`bool`) | `LegacyEnumCheckbox` |
| Swamp Colors | `EnumOptions::SWAMP_COLORS` | `ofSwampColors` (`bool`) | `LegacyEnumCheckbox` |
| Smooth Biomes | `EnumOptions::SMOOTH_BIOMES` | `ofSmoothBiomes` (`bool`) | `LegacyEnumCheckbox` |
| Connected Textures | `EnumOptions::CONNECTED_TEXTURES` | `ofConnectedTextures` (1=Fast, 2=Fancy, 3=OFF) | `LegacyEnumCycleButton` |
| Natural Textures | `EnumOptions::NATURAL_TEXTURES` | `ofNaturalTextures` (`bool`) | `LegacyEnumCheckbox` |

### Rendimiento (`Category::Performance` — 7 ítems en Xbox, 1 sola vista sin paginar)
| Opción | `EnumOptions` | Campo en `GameSettings` | Control Legacy |
|---|---|---|---|
| Smooth FPS | `EnumOptions::SMOOTH_FPS` | `ofSmoothFps` (`bool`) | `LegacyEnumCheckbox` |
| Smooth Input | `EnumOptions::SMOOTH_INPUT` | `ofSmoothInput` (`bool`) | `LegacyEnumCheckbox` |
| Load Far | `EnumOptions::LOAD_FAR` | `ofLoadFar` (`bool`) | `LegacyEnumCheckbox` |
| Preloaded Chunks | `EnumOptions::PRELOADED_CHUNKS` | `ofPreloadedChunks` (0, 2, 4, 6, 8) | `LegacyEnumCycleButton` |
| Chunk Updates | `EnumOptions::CHUNK_UPDATES` | `ofChunkUpdates` (1..5) | `LegacyEnumCycleButton` |
| Dynamic Updates | `EnumOptions::CHUNK_UPDATES_DYNAMIC` | `ofChunkUpdatesDynamic` (`bool`) | `LegacyEnumCheckbox` |
| Far View | `EnumOptions::FAR_VIEW` | `ofFarView` (`bool`) | `LegacyEnumCheckbox` |

### Otros (`Category::Other` — 6 ítems en Xbox, 1 sola vista sin paginar)
| Opción | `EnumOptions` | Campo en `GameSettings` | Control Legacy |
|---|---|---|---|
| Fast Debug Info | `EnumOptions::FAST_DEBUG_INFO` | `ofFastDebugInfo` (`bool`) | `LegacyEnumCheckbox` |
| Debug Profiler | `EnumOptions::PROFILER` | `ofProfiler` (`bool`) | `LegacyEnumCheckbox` |
| Weather | `EnumOptions::WEATHER` | `ofWeather` (`bool`) | `LegacyEnumCheckbox` |
| Time | `EnumOptions::TIME` | `ofTime` (0=Default, 1=Day Only, 2=Night Only) | `LegacyEnumCycleButton` |
| Autosave | `EnumOptions::AUTOSAVE_TICKS` | `ofAutoSaveTicks` (40 / 400 / 4000 / 40000) | `LegacyEnumCycleButton` |
| 3D Anaglyph | `EnumOptions::ANAGLYPH` | `anaglyph` (`bool`) | `LegacyEnumCheckbox` |

---

## 3. Opciones omitidas en Xbox y motivo

Se protegieron con las mismas guardas de plataforma/backend que en las pantallas Java de OptiFine (`GuiQualitySettingsOF`, `GuiVideoSettings`, `GuiOtherSettingsOF`):

1. **`MIPMAP_LEVEL` y `MIPMAP_TYPE`** (`renderSupportsFeature(RenderFeature::Mipmaps)`):
   - En `RenderAPI_D3D8_XBOX.cpp`, `renderSupportsFeature` devuelve `false` y `renderTextureImageRgba` sube únicamente el nivel 0 (`if (level != 0 || target <= 0) return; // one level: no mipmaps yet`).
2. **`AF_LEVEL` (Anisotropic Filtering)** (`renderSupportsFeature(RenderFeature::AnisotropicFiltering)`):
   - Desactivado en el backend Xbox (`renderGetMaxAnisotropy() == 1`, `renderSupportsFeature == false`).
3. **`AA_LEVEL` (Antialiasing)** (`renderSupportsFeature(RenderFeature::MultisampleAntialiasing)`):
   - Desactivado en el backend Xbox (`renderGetMaxSamples() == 0`, `renderSupportsFeature == false`).
4. **`RANDOM_MOBS`** (`#if PLATFORM_OPTIFINE_RANDOM_MOBS`):
   - Desactivado en consolas (`PlatformConfig.h`: `PLATFORM_OPTIFINE_RANDOM_MOBS == 0` en PS2/Wii/Xbox para evitar barridos de archivos inexistentes en disco).
5. **`CUSTOM_FONTS`** (`#if PLATFORM_OPTIFINE_CUSTOM_FONTS`):
   - Desactivado en consolas (`PlatformConfig.h`: `PLATFORM_OPTIFINE_CUSTOM_FONTS == 0` en PS2/Wii/Xbox).
6. **`ADVANCED_OPENGL` (Occlusion Culling)** (`Config::isOcclusionAvailable()`):
   - El backend D3D8 de Xbox no expone consultas de oclusión hardware (`renderSupportsFeature(RenderFeature::OcclusionQuery) == false`).
7. **`RENDER_BACKEND`** (`#if PLATFORM_PC_LEGACY && defined(MC_WIN32)`):
   - Selector exclusivo de escritorio Windows (OpenGL vs Direct3D 9).
8. **Fullscreen / resoluciones de monitor PC**:
   - No existen en `EnumOptions` ni aplican en Xbox (el modo de vídeo lo gobierna el EEPROM/dashboard de la consola).

---

## 4. Verificación en 640x480 y 1280x720 y compilación

- **640x480 (`320x240` lógicos con `XBOX_LEGACY_GUI_SCALE = 2`)**:
  - `LegacyVideoOptions` (12 filas): `rowHeight = 14`, `rowSpacing = 2`, `panelHeight = 206`, `panelY = 6`, `panelBottom = 212` (13 px por encima de `hintRowY = 225`).
  - `LegacyAdvancedVideoOptions` (máx. 10 filas por vista): `panelHeight <= 174`, centrado con holgura sobre la barra de botones inferior.
- **1280x720 (`640x360` lógicos con `XBOX_LEGACY_GUI_SCALE = 2`)**:
  - `LegacyVideoOptions` (12 filas): `rowHeight = 15`, `rowSpacing = 2`, `panelHeight = 218`, `panelY = 113` (9 px debajo del logo), `panelBottom = 331` (14 px por encima de `hintRowY = 345`).
  - `LegacyAdvancedVideoOptions` (máx. 10 filas por vista): `rowHeight = 16`, `panelHeight <= 194`, `panelY >= 113`, `panelBottom <= 307`.
- **Compilación**:
  - `build/xbox-release` y `build/xbox-release-exp` compilados sin errores y generando `OptiCraft.xbe` y `OptiCraft.iso` (con `buzon-xbox\BUILD.lock`, sin copiar al lanzador).
  - Nota sobre capturas: `xemu` no estaba abierto en la sesión y no se lanzó para no alterar binarios ni el entorno del usuario.
