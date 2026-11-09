# Informe Encargo 67: Detalles del creativo, cruceta separada, vídeo en 480p e iconos de controles

- **Rama**: `port152-inventario` (base `port152-claude` tras cherry-picks)
- **Commit final**: `13c718b`
- **Compilaciones**: `build/xbox-release` (OK) y `build/xbox-release-exp` (OK) - ISOs generadas limpiamente.

---

## 1. Menú Creativo

### a. Pestaña «Inventario de supervivencia»: tapa del cofre en 3D
- **Problema**: El icono del cofre en la pestaña se dibujaba sin la tapa o con la tapa ocluida/recortada.
- **Causa y Solución**:
  1. En `RenderBlocks::renderBlockOnInventory` (case 22): Java MCP aplica `renderRotate(90.0F, 0.0F, 1.0F, 0.0F)` y `renderTranslate(-0.5F, -0.5F, -0.5F)` antes de delegar a `ChestItemRenderHelper.instance.renderChest`. Se implementó esta rotación y traslación exacta.
  2. En `TileEntityChestRenderer::renderTileEntityChestAt`: Se reordenó la llamada dibujando primero la parte estática (`renderStaticPart`) y posteriormente la dinámica (`renderDynamicPart`), evitando z-fighting y garantizando que la tapa se dibuje correctamente orientada.
  3. En `GuiContainerCreative::renderCreativeTab`: Se aseguró `glEnable(GL_DEPTH_TEST)` antes de dibujar el icono (`itemRenderer.renderItemIntoGUI`) para permitir que la geometría 3D del cofre no sea ocluida por el fondo de la pestaña.
- **Commit**: `7f405bd`

### b. Auditoría de bloques y pestañas del creativo
- **Problema**: Aparecían bloques técnicos en el menú creativo (ej. bloque de cultivo de zanahoria 141 con icono de brote en la pestaña Decoración).
- **Causa y Solución**: Se auditó `CreativeTabs::initTabsForBlocksAndItems()` contra los constructores de Java MCP 1.5.2. En Java, los bloques técnicos definen explícitamente `setCreativeTab(null)` o no tienen pestaña asignada en su jerarquía. Se eliminó la asignación de pestaña a 9 bloques técnicos que no deben estar en el creativo:
  - `Block::stoneDoubleSlab` (43)
  - `Block::crops` (59, trigo plantado)
  - `Block::torchRedstoneIdle` (75, antorcha de redstone inactiva)
  - `Block::pumpkinStem` (104, tallo de calabaza)
  - `Block::melonStem` (105, tallo de sandía)
  - `Block::netherStalk` (115, verruga del Nether plantada)
  - `Block::woodDoubleSlab` (125)
  - `Block::carrot` (141, cultivo de zanahoria)
  - `Block::potato` (142, cultivo de patata)
- **Commit**: `8672ff7`

### c. Marco del portal del End en 3D (RenderType 26) y revisión de bloques especiales
- **Problema**: El bloque 120 (`BlockEndPortalFrame`, renderType 26) se dibujaba plano (una cara 2D) en los inventarios y pestañas creativas.
- **Solución**:
  - En `RenderBlocks::renderItemIn3d`: Se agregó el `case 26` para retornar `true`.
  - En `RenderBlocks::renderBlockOnInventory`: Se añadió el `case 26` renderizando las 6 caras del bloque con sus bounds calculados y el teselador configurado idéntico a Java MCP 1.5.2.
- **Revisión de otros bloques con renderType especial en creativo**:
  - `renderType 0` (Cubos estándar): Render 3D completo.
  - `renderType 1` (Flores, hierbas, retoños): Render 2D cruzado (correcto según Java).
  - `renderType 2` (Antorchas): Render estándar con inclinación.
  - `renderType 4` (Líquidos): No en creativo directamente.
  - `renderType 10` (Escaleras): Render 3D completo.
  - `renderType 11` (Vallas): Render 3D completo.
  - `renderType 13` (Cactus): Render 3D completo.
  - `renderType 14` (Cama): Render 3D / item 2D según corresponda.
  - `renderType 16` (Pistón base): Render 3D completo.
  - `renderType 17` (Pistón extensión): Técnico (no en creativo).
  - `renderType 21` (Valla de redstone / Nether brick fence): Render 3D completo.
  - `renderType 22` (Cofre): Render 3D vía `ChestItemRenderHelper`.
  - `renderType 26` (End Portal Frame): **Ahora en 3D**.
  - `renderType 31` (Panel de cristal / barrotes): Render plano / cruzado según Java.
  - `renderType 38` (Yunque): Render 3D completo.
  - `renderType 39` (Tolva / Hopper): Render 3D completo.
  - `renderType 40` (Comparador): Render 3D de base con antorchas.
  - `renderType 41` (Dropper / Dispenser): Render 3D completo.
- **Commit**: `986cf34`

### d. Buscador y Teclado Virtual (VirtualKeyboard)
- **Problema**:
  1. Al cambiar a la pestaña de búsqueda (`tabAllSearch`), el teclado no se abría automáticamente.
  2. Al cerrar el teclado con B y pulsar de nuevo en el campo de texto, no volvía a abrirse porque el campo ya tenía el foco y `canLoseFocus` impedía reactivarlo.
  3. El consumo de botones del pad en `GuiContainerCreative::updateScreen` interfería con el teclado virtual.
  4. En pantalla partida no se dibujaba el teclado en todas las vistas.
- **Solución**:
  - En `GuiContainerCreative::setCurrentCreativeTab`: Se añade llamada automática a `VirtualKeyboard::open(&searchField, ...)` al entrar en la pestaña de búsqueda.
  - En `GuiTextField::mouseClicked`: Si el clic recae sobre las coordenadas del campo, se asegura que el teclado virtual se abra aunque el campo ya estuviese enfocado.
  - En `GuiContainerCreative::updateScreen`: Se protege el polling de botones del pad con `!platformTextInputExclusive()`, impidiendo que los botones destinados al teclado se consuman por el menú creativo.
  - En `EntityRenderer.cpp`: Se integró `VirtualKeyboard::instance().render()` en el ciclo de pantalla partida.
  - Se incluyó `VirtualKeyboard.h` en `GuiContainerCreative.cpp`.
- **Commits**: `a75f743`, `13c718b`

---

## 2. Cruceta (D-pad) separada del Joystick (Legacy Console)

- **Comportamiento general**:
  - **Stick izquierdo**: Maneja el movimiento analógico del jugador (`PLATFORM_DIRECT_ANALOG_MOVEMENT`).
  - **Cruceta en juego**: Ya NO mueve al jugador (desacoplada de WASD y flechas en `XboxInput.cpp`, `MovementInputFromOptions.cpp`, `Ps2InputMapper.cpp` y `GameSettingsBackend_PS2.cpp`).
- **Asignación de la Cruceta en juego (coincidente con Minecraft Xbox 360 / PS3 Edition)**:
  - **ARRIBA**: Cambiar perspectiva de cámara (1ª persona, 3ª persona trasera, 3ª persona frontal) enviando `KEY_F5`.
  - **ABAJO**: Abrir la pantalla de chat (`KEY_T`), permitiendo chatear y enviar comandos.
  - **IZQUIERDA**: Ciclar slot de la barra de acceso rápido hacia la izquierda (`mouse wheel -1`).
  - **DERECHA**: Ciclar slot de la barra de acceso rápido hacia la derecha (`mouse wheel +1`).
- **Menús e Inventarios**:
  - Dentro de cualquier `GuiContainer`, la cruceta conserva intacta la navegación por casillas y celdas mediante `ContainerSlotNavigator`, sin ninguna interferencia con el stick ni con los atajos de juego.
- **Pantalla partida**:
  - Cada mando gestiona su cruceta de forma independiente. En `XboxSplitScreen.cpp`, la cruceta del Jugador 2 cicla su propia barra de acceso rápido (izquierda/derecha) y no mueve su avatar.
- **Apertura de chat en un jugador**:
  - En `Minecraft.cpp`, se permite abrir `GuiChat` mediante `keyBindChat` tanto en modo un jugador como en multijugador.
- **Commit**: `bfd66a5`

---

## 3. Pantallas de Vídeo y Opciones a 480p

- **Problema**: En resolución 640x480, las listas de opciones se salían verticalmente de la pantalla debido al espacio ocupado por el banner del título "Minecraft" (~75 píxeles) y al centrado rígido.
- **Solución**:
  - En `LegacyOptionsScreen.cpp` y `LegacyCreateWorldScreen.cpp`: Se omite el dibujo del título de Minecraft (`drawMinecraftTitle`) cuando `mc->displayHeight <= 480`.
  - En `LegacyOptionsLayout.cpp`:
    - Para `displayHeight <= 480`, `legacyCenteredPanelY` utiliza un margen superior de 6 píxeles en lugar de `scene.contentTop`, aprovechando toda la altura disponible.
    - Se recalcula la altura visible y el número de filas máximas (`legacyOptionsMaxRows`) para que todas las opciones quepan perfectamente centradas y legibles sin recortarse.
  - En 720p y resoluciones superiores, el diseño y la visualización del título permanecen intactos.
- **Commit**: `913d234`

---

## 4. Iconos de Controles desde el Inicio

- **Problema**: Al entrar por primera vez a Opciones > Controles, las acciones de atacar y usar ítem mostraban etiquetas genéricas como "Botón 1" / "Botón 2" en lugar de los glifos gráficos ("RT", "LT").
- **Solución**:
  - En `GameSettingsBackend_XBOX.cpp` y `GameSettingsBackend_PS2.cpp`, se definen por defecto los códigos canónicos de mando (`XBOX_KEY_RIGHT_TRIGGER`, `XBOX_KEY_LEFT_TRIGGER`, `PS2_KEY_R2`, `PS2_KEY_L2`) para `keyBindAttack` y `keyBindUseItem`.
  - En `platformGameSettingsFinalizeLoad`, se añade migración automática que mapea enlaces antiguos (teclas 1 y 2) a los gatillos correspondientes.
  - Al abrir `LegacyControlsScreen`, `LegacyButtonPrompt::iconFromName` localiza de inmediato el icono correspondiente ("RT", "LT", "A", "B", etc.), mostrando los glifos gráficos desde el primer inicio sin requerir reasignación manual.
- **Commit**: `25f08fb`

---

## Cherry-picks aplicados desde `port152-claude`
- `3b71bea`: fix(lang): allow fallback translations for missing keys in StringTranslate
- `c78f54a`: diag(input): log input events to buzon-xbox for troubleshooting
- `23350b7`: fix(item): ensure ItemStack tags match between client and server
- `41ab79b`: fix(gui): sync selected creative tab state on reconnect

---

## Resumen de Commits del Encargo
- `7f405bd`: fix(gui): correct chest tab icon orientation and depth sorting in creative inventory
- `8672ff7`: fix(creative): remove technical crop, stem, and double slab blocks from creative tabs
- `986cf34`: feat(render): render end portal frame in 3D in inventory and creative tabs
- `a75f743`: fix(gui): auto-open and properly reopen virtual keyboard in creative search tab and split-screen
- `bfd66a5`: feat(input): decouple D-pad from walking; bind camera, chat, and hotbar cycling
- `25f08fb`: fix(gui): show controller button icons from the start in controls menu
- `913d234`: fix(gui): adjust layout and centering of video and options screens for 480p
- `13c718b`: fix(gui): include VirtualKeyboard.h in GuiContainerCreative.cpp
