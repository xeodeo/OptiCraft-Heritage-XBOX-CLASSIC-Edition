# 112 - Menús nuevos con aspecto Legacy (LCE) en vez de Bedrock

Fecha: 22-11. Subagente Claude. Ramas: `port152-ui` (wt-152-ui) commit **e2479f35** y
`opti30-ui` (wt-125-ui) commit **3b1ea166** (cherry-pick limpio, mismos ficheros).
Solo se cambia el modo **Legacy UI** (`gameSettings->legacyUI`); el modo Java queda como estaba.
Controles y acciones del mando son los mismos (Start = texturas en el catálogo sigue).

## Pieza nueva compartida
`src/net/minecraft/src/legacy/LegacyMenuWidgets.{h,cpp}` reutiliza lo que ya usan Opciones / Crear mundo:
- `legacyMenuLayout()`: calcula panel + filas centrado entre el borde superior de la zona segura HUD y la fila
  de ayudas (`legacyHintRowY`), con scroll si no caben. Altura de fila: 16 a 480p, 18 a 720p, 14 en pantalla dividida.
- `legacyDrawMenuBackdrop()`: panorama en movimiento (`legacyDrawPanorama` + `legacyScenePanoramaTimer`) en menús;
  en partida, el mismo velo del menú de pausa (`legacyPauseOverlay*Color`).
- `legacyDrawMenuPanel()` = `LegacyOptionsPanel` (panel gris biselado del tema).
- `legacyDrawMenuButton*()` = mismo dibujo que `LegacyGuiButton` (botón piedra de /gui/gui.png, relleno azul-gris y texto
  amarillo en el seleccionado, `legacyDrawOptionText`).
- `legacyDrawMenuCheckbox()` / `legacyDrawTickBox()` = TickBox de `LegacyOptionCheckbox` (sprites /legacy/tickbox*.png y tick.png, con respaldo procedural).
- `legacyDrawInsetBox()` (hueco hundido; variante oscura = pozo de lista de mundo extraplano), `legacyDrawTextFieldBox()`
  (caja negra como la semilla), `legacyDrawMenuScrollbar()`, títulos/textos gris oscuro sobre el panel,
  `legacyDrawInlineHints()` (iconos de botón + texto sobre el panel).

**Fallo encontrado:** `drawControlHintRow()` no dibuja nada con más de 4 entradas. El catálogo (5) y los waypoints (6) no
mostraban ayudas. Ahora: 4 en la fila de abajo y las demás sobre el panel (Blanco/Negro = pestañas, Start = texturas;
Blanco = nombre, Negro = opciones, cruceta = color). En modo Java los waypoints siguen con 6 (no se ven, sin tocar).

## Pantallas cambiadas
| Pantalla | Antes (Legacy) | Ahora |
|---|---|---|
| Catálogo de mods (`GuiCompiledMods`) | filas planas oscuras, píldoras azules INSTALAR, pestañas planas | cada mod = botón piedra (icono, nombre, versión, descripción), TickBox = activado, pestañas = botones piedra pequeños, búsqueda en caja negra (borde amarillo al escribir), detalles en pozo oscuro (lado derecho a 720p, franja abajo a 480p) |
| ChocoCraft (`GuiChocoboMenu`) | filas planas | 4 botones piedra (desactivados en gris si no se pueden usar), título/estadísticas en gris oscuro, renombrar en panelito LCE |
| Herobrine (`GuiHerobrineMenu`) | filas con SI/OFF de color | 7 TickBox como las Opciones, descripción en 2 líneas (rojo oscuro si es destructivo) |
| Minimapa (`GuiMinimapMenu`) | filas planas | sí/no = TickBox; forma/zoom/cueva = botón con valor (`< valor >` en el seleccionado); gestor de waypoints = botón |
| Waypoints (`GuiWaypointManager`) | tarjeta casi negra | panel LCE, cada waypoint = botón piedra con muestra de color, nombre, coordenadas y TickBox = visible |
| Info del servidor (`GuiServerInfo`) | panel con texto suelto | panel LCE, datos en pozo oscuro, ayuda B abajo |
| Buscar skin (`GuiSearchSkin`, rama Legacy) | botones Java, vista previa azul, ayuda pegada al borde | botones `LegacyGuiButton` (el foco del mando = seleccionado amarillo; el campo de texto conserva el marco amarillo), vista previa y estado en pozo oscuro, fila de ayudas A/Y/X/B en la posición estándar con zona segura |

No tocados: selector de skins (rueda, ya tenía estilo LCE), pantallas de mundo extraplano/plantillas (ya en estilo LCE,
no están en estas ramas), `GuiLoadModsMenu`/`GuiLoadModsList` (.och) y `GuiConfirm*Install` (sin rama Legacy).

## Cuentas de diseño (GUI escala 2 en Legacy)
- **480p** = 320x240 lógicos. Zona segura 4 %: arriba/abajo 10, lados 13. Fila de ayudas `hintY` = 240-15-10 = **215**.
  Hueco para el panel: y 10 .. 209 (199 px), ancho máx. 320-26-8 = 286.
  - Catálogo: cabecera 42 + filas 22(+2) + franja 24 + ayudas panel → 4 filas visibles con scroll. Panel 286 de ancho.
  - Herobrine: cabecera 32 + 7 filas de 16(+2) + descripción 22 → 174 px, cabe entero.
  - Minimapa: 9 filas de 16(+2) + título 22 → cabe entero (la línea "Back + cruceta" solo sale a 720p).
  - Waypoints: filas de 22 → 6 visibles. Chocobo: 120 px. Servidor: hasta 13 líneas.
- **720p** = 640x360. Zona segura: arriba/abajo 14, lados 26. `hintY` = 360-15-14 = **331**. Hueco: y 14 .. 325.
  - Catálogo: panel 470, lista 58 % + tarjeta de detalle; filas 24(+3) → hasta 9 mods sin scroll.
- **Pantalla dividida** (horizontal, 320x120 a 480p): `hintY` = 108, filas de 14, sin líneas de pie;
  chocobo cabe entero (4 filas), minimapa 4 filas con scroll, waypoints 3. Buscar skin: sin fila de ayudas en dividida (como antes).
- Todo centrado horizontal (`(ancho-panel)/2`) y vertical dentro del hueco.

## Compilación
`build-ui.ps1`: las 4 compilaciones (wt-152-ui y wt-125-ui, xbox-release y xbox-release-exp) terminan sin errores, con "SSE2 left in" = 13 en todas.

## Sin probar
- Nada probado en xemu ni en la consola (no se ejecutó xemu). Hay que mirar en 480p y 720p: catálogo, chocobo,
  Herobrine, minimapa, waypoints, info del servidor, buscar skin; y chocobo/minimapa/waypoints en pantalla dividida.
- Que existan los sprites /legacy/tickbox*.png y tick.png en el ISO (si no, sale la casilla procedural).
- El texto de los iconos Blanco/Negro/Start sobre el panel (tamaño/posición de iconos del atlas Xbox).
