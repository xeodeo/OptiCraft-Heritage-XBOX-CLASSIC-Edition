# Informe Encargo 81 — Selector de Skins Estilo LCE (`GuiSkinSelector`)

## 1. Resumen de Cambios
Se rediseñó la pantalla de selección de skins (`GuiSkinSelector`) en `wt-152-rend` (rama `port152-skins-lce`) para adoptar el estilo auténtico de **Minecraft Legacy Console Edition (LCE)** (`UIScene_SkinSelectMenu` / `CScene_SkinSelect` + `UIControl_PlayerSkinPreview`) cuando `gameSettings->legacyUI` está activado, preservando intacta la interfaz original cuando `legacyUI` está desactivado.

### Archivos modificados
- `src/net/minecraft/src/skin/GuiSkinSelector.h`
- `src/net/minecraft/src/skin/GuiSkinSelector.cpp`

## 2. Diseño Visual y Funcional en Modo LCE (`legacyUI == true`)
1. **Fondo y Escenario LCE (`BackgroundTint` + `TabBar`)**:
   - Desde el menú principal (`mc->theWorld == nullptr`), dibuja el panorama Legacy (`legacyDrawPanorama` + gradiente suave `0x18000000`..`0x50000000`).
   - Desde pausa en partida (`mc->theWorld != nullptr`), dibuja el velo translúcido oscuro de pausa (`0xC0101010`..`0xD0101010`).
   - En la franja central, dibuja la banda horizontal característica de LCE (`BackgroundTint`) enmarcada arriba y abajo por barras divisorias metálicas biseladas (`TabBar`).
2. **Cabecera de Packs en 3 Ranuras (`PackGroup`: `Left`, `Center`, `Right`)**:
   - Pestaña central destacada con caja biselada LCE (`legacyUiTheme()`) mostrando el pack activo (`Default Skins` / `Skins Originales` o `Custom Skins (N)` / `Personalizado (N)`).
   - Pestañas laterales (`< ...` y `... >`) mostrando el pack alternativo con atenuación visual, clicables con ratón y conmutables con cruceta/stick arriba-abajo, `LT`/`RT` (`L2`/`R2`) o `Tab`.
3. **Carrusel 3D de Personajes (`ModelBiped`)**:
   - Renderizado 3D real con `ModelBiped` (soportando texturas `64x32` y `64x64`), iluminación estándar (`RenderHelper::enableStandardItemLighting()`), sombras elípticas en el suelo (`drawFeetShadow`) y prueba de profundidad (`DepthTest`).
   - **Personaje central (`k = 0`)**: mayor escala, mirando al frente con rotación automática suave (`-42°` a `+42°`) o rotación manual libre de `360°` con el **Right Stick** (`rightX`, o teclas `Q`/`E`), inclinación vertical del stick derecho (`rightY`) para alternar poses (caminando / agachado `isSneak` / golpeando `onGround`), y `R3` para restablecer la pose/rotación.
   - **Personajes laterales (`k = -4..-1` y `k = 1..4`)**: dispuestos a izquierda y derecha con escala progresivamente menor (`0.79^|t|`), orientados ligeramente hacia el centro (`-45°` en la izquierda y `+45°` en la derecha) e interpolando suavemente su posición, escala y ángulo (`scrollOffset`) al desplazarse por el carrusel.
   - **Gestión segura de texturas y estado GL/D3D8**: reutiliza los IDs cacheados por `RenderEngine::getTexture` (con fallback automático a `SkinManager::getDefaultSkinTexture()` = `"/mob/char.png"` si un recurso falta o falla) y restaura estrictamente el estado 2D (`disableStandardItemLighting`, `DepthTest` off, `RescaleNormal` off, `ColorMaterial` off, `Texture2D` on, `renderColor4f(1,1,1,1)`) al terminar el pase 3D.
4. **Insignia `"Selected"` / `"Seleccionado"` y Placa `SkinDetails`**:
   - Muestra la píldora biselada LCE `"Selected"` / `"Seleccionado"` sobre la placa inferior cuando el skin enfocado coincide con el skin equipado actualmente para el jugador activo (P1 o P2).
   - Placa inferior centrada (`SkinDetails`) con dos líneas nítidas: nombre del skin (`SkinName`) y nombre del pack + contador `(i/N)` + indicador `[P2]`/`[J2]` cuando se edita el segundo jugador.
5. **Controles de Mando y Prompts LCE (`LegacyButtonPrompt`)**:
   - **A / Cross / Enter**: Seleccionar, guardar en `GameSettings` (`selectedSkin` / `selectedSkinP2`), aplicar textura a `mc->thePlayer` y volver.
   - **B / Circle / Esc**: Cancelar y volver a `parentScreen`.
   - **D-Pad / Left Stick Izquierda/Derecha (y White/Black / L1/R1)**: Anterior / siguiente skin (con auto-repeat al mantener pulsado).
   - **D-Pad / Left Stick Arriba/Abajo o LT/RT / L2/R2 / Tab**: Cambiar entre `Default Skins` y `Custom Skins`.
   - **Right Stick**: Rotar modelo 3D central (`rightX`) y cambiar pose (`rightY`); `R3` reinicia rotación.
   - **Y / Triangle (`L`)**: Abrir `GuiLoadSkinsMenu` (`Load Skins`).
   - **X / Square (`P` / `Del`)**: Alternar selector de Skin de Jugador 2 (o eliminar skin personalizado cuando se está en el pack `Custom`, manteniendo `Back`/`Select` para alternar Jugador 2).

## 3. Escalado y Resoluciones
- Layout adaptable verificado para **640x480** (`320x240` lógicos a escala 2x) y **1280x720** (`640x360` lógicos a escala 2x).

## 4. Compilación
- `cmake --build build/xbox-release` (64 MB / 480p): OK (`bin/xbox/OptiCraft.iso`).
- `cmake --build build/xbox-release-exp` (128 MB / 720p): OK (`bin/xbox-exp/OptiCraft.iso`).
- No se copiaron binarios al launcher.
