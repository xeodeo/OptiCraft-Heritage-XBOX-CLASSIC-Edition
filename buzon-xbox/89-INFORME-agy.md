# 89. Informe: reasignar el stick tras «Restablecer» en Controles y cofre en la mano

- **Rama 1.5.2**: `port152-erluza` en `wt-152-rend` (sobre `7b69e69` → **`501b40d`**, 2 commits):
  1. `e6d2065` — `fix(xbox): stick rebind capture, embedded button atlas priority, and Controls reset`
  2. `501b40d` — `fix(render): isolate chest renderType 22 matrix and align chest minecart`
- **Rama 1.2.5**: `opti30-erluza` en `wt-opti30` (sobre `0b46eac` → **`410957a`**, 2 commits):
  1. `af2f850` — `fix(xbox): stick rebind capture, embedded button atlas priority, and Controls reset`
  2. `410957a` — `fix(render): center chest item (renderType 22) in first-person hand, GUI, and minecart`
- **Reglas respetadas**: sin `reset`, sin `push`, sin copiar al lanzador, sin tocar `splashes.txt`, sin tocar Creativo ni worldgen, `BUILD.lock` respetado y borrado, sonda temporal de verificación retirada antes de las compilaciones finales (`git status` limpio y 0 rastros en los XBE finales), 0 avisos nuevos de SSE2, PS2 compilado limpio.

---

## 1. Stick sin icono y no reasignable tras «Restablecer» en Controles

### Causas raíz (`archivo:línea`)
Se detectaron **cinco causas encadenadas** (idénticas en 1.5.2 y 1.2.5):

1. **Atlas `buttons_xbox.png` antiguo en disco / `assets.pak` pisaba el atlas embebido** (`src/net/minecraft/src/TexturePackDefault.cpp:89-99`, antes): `TexturePackDefault::getResourceAsStream` abría primero `GameResources::open(s)` y solo usaba `s_xboxButtonAtlasPngData` como respaldo. Si en la consola había un `buttons_xbox.png` anterior al encargo 84 (sin los iconos `LStickUp/Down/Left/Right`, índices 17–20), los códigos `238..241` quedaban sin icono visible aunque el stick siguiese moviendo al jugador.
2. **La captura de reasignación ignoraba el stick izquierdo** (`src/xbox/input/XboxInput.cpp:225-248`, antes): cuando `platformPadRebindExclusive()` estaba activo, `updateMenu()` solo recorría `kRemappableButtons` (botones digitales, gatillos, pulsación de stick y cruceta), sin leer `p.leftX`/`p.leftY` ni emitir `XBOX_KEY_LSTICK_UP/DOWN/LEFT/RIGHT` (`238..241`). Tampoco existía cancelación con `Start`/`Back` → `KEY_ESCAPE`.
3. **Clic de ratón sintético (`-100`) no bloqueado durante la captura en Xbox**:
   - `src/net/minecraft/src/legacy/LegacyControlsScreen.cpp:367` solo excluía `#if !PLATFORM_PS2 && !PLATFORM_WII`, faltando `&& !PLATFORM_XBOX`.
   - `src/net/minecraft/src/GuiControls.cpp:168-174` no bloqueaba `mouseClicked` durante la captura en consolas, permitiendo que un clic sintético del puntero guardara `-100` (código de ratón PC, sin icono de mando).
4. **Migración incompleta de `options.txt`** (`src/platform/GameSettingsBackend_XBOX.cpp:100-103`, antes): `platformGameSettingsFinalizeLoad` solo migraba los 4 ejes de movimiento si tenían códigos de cruceta o exactamente WASD (`17, 30, 31, 32`). Si un fallo previo había dejado `-100` o `0` (`KEY_NONE`) en `options.txt`, no se auto-reparaba al reiniciar.
5. **Interfaz Java `GuiControls` incompleta en consola** (`src/net/minecraft/src/GuiControls.cpp:35-145`): faltaba el botón «Restablecer por defecto» (`id = 203`), el ajuste de altura para que quepan todas las filas en 480p (`320×240`), la limpieza de `platformSetPadRebindExclusive(false)` en `onGuiClosed()`, y `KEY_NONE` (0, ej. Chat sin asignar) se marcaba como conflicto duplicado (`GuiControls.cpp:171` y `src/net/minecraft/src/legacy/LegacyControlsBinding.cpp:164-165`).

### Arreglo aplicado (`archivo:línea`, idéntico en 1.5.2 y 1.2.5)
- `src/net/minecraft/src/TexturePackBase.cpp:38-46` y `TexturePackDefault.cpp:83`: en `#ifdef XBOX_PLATFORM`, `buttons_xbox.png` (`35883` bytes) y `crafting_icons.png` se sirven desde el recurso compilado **antes** que `data/` o `assets.pak`, garantizando siempre correspondencia exacta con `XboxButtonAtlasData.h`.
- `src/xbox/input/XboxInput.cpp:50, 202-276`: añadido `s_rebindStickArmed` + `rebindStickKeyCode()`. Al entrar en captura exige que el stick pase primero por zona neutra (evitando disparos accidentales por el movimiento previo), y al inclinarlo emite `XBOX_KEY_LSTICK_UP` (`238`), `DOWN` (`239`), `LEFT` (`240`) o `RIGHT` (`241`), reteniendo `s_menuAnalogDirection`/`s_menuAnalogRepeat` para que no salte inmediatamente a la fila siguiente. `Start`/`Back` emiten `KEY_ESCAPE` para cancelar captura.
- `src/net/minecraft/src/legacy/LegacyControlsScreen.cpp:367`: añadido `&& !PLATFORM_XBOX` para bloquear `mouseClicked` durante la captura.
- `src/net/minecraft/src/legacy/LegacyControlsBinding.cpp:164-165`: `KEY_NONE` (`0`) no cuenta como conflicto.
- `src/net/minecraft/src/GuiControls.h:16, 38-40` y `GuiControls.cpp:43-216`: layout adaptable 480p/720p (`480pMaxY = 234/240`, `720pMaxY = 334/360`), botón `Reset to Defaults` (`id = 203`) que llama a `options->resetControlBindingsToDefaults()`, cancelación con `KEY_ESCAPE`, bloqueo de `mouseClicked` en consola, limpieza en `onGuiClosed()`, e ignorar `KEY_NONE` en duplicados.
- `src/platform/GameSettingsBackend_XBOX.cpp:99-103`: `platformGameSettingsFinalizeLoad` migra cualquier código `< lwjgl::Keyboard::KEY_MAX` (teclado, ratón `-100` o `0`) o cruceta antigua en Forward/Left/Back/Right a `XBOX_KEY_LSTICK_UP/LEFT/DOWN/RIGHT`.

### Verificación en xemu 64 MB (1.5.2 y 1.2.5)
Resultados idénticos en ambas versiones (`[e89]`):
- **Atlas embebido**: `ATLAS bytes=35883 expected=35883 ok=1`.
- **Captura de stick en `XboxInput`**: `INPUT_STICK armedGuard=1 keys=(238,239,240,241) esc=1 ok=1` (guarda de reposo inicial + 4 direcciones del stick + `Start` → `Escape`).
- **Reset + iconos + Chat sin conflicto**: `RESET keys=(238,240,239,241) icons=(17,19,18,20) chat=0 chatConf=0 ok=1`.
- **Persistencia en `options.txt` tras recarga**: `RELOAD keys=(238,240,239,241) ok=1`.
- **Auto-reparación de `options.txt` corrupto** (`-100, 0, 31, DPAD_RIGHT`): `MIGRATE keys=(238,240,239,241) ok=1`.
- **Java `GuiControls`**: `hasReset=1 480pMaxY=234/240(1) 720pMaxY=334/360(1) excl=1 mouseBlk=1 stickBind=1 esc=1 resetBtn=1 closeExcl=0`.
- **Legacy `LegacyControlsScreen`**: `excl=1 mouseBlk=1 stickBind=1 resetBtn=1`.

---

## 2. Cofre en la mano (y cofre de ender, cofre trampa, yunque y vagoneta con cofre)

### Causa raíz y comparativa con Java (`archivo:línea`)
- **Java 1.2.5 y Java 1.5.2** (`mcp-diff/src125` y `mcp-diff/src152`, `RenderBlocks.java:6728-6732`, `renderBlockAsItem`, `renderType == 22`):
  ```java
  } else if (var14 == 22) {
      GL11.glRotatef(90.0F, 0.0F, 1.0F, 0.0F);
      GL11.glTranslatef(-0.5F, -0.5F, -0.5F);
      ChestItemRenderHelper.instance.renderChest(var1, var2, var3);
      GL11.glEnable(GL12.GL_RESCALE_NORMAL);
  }
  ```
- **En 1.2.5 (`wt-opti30`) antes de `410957a`** (`src/net/minecraft/src/RenderBlocks.cpp:4897-4901`): faltaban por completo `renderRotate(90.0f, 0.0f, 1.0f, 0.0f)` y `renderTranslate(-0.5f, -0.5f, -0.5f)`. Como `TileEntityChestRenderer` dibuja el cofre en `[0, 1]³` (`[1/16..15/16, 0..14/16, 1/16..15/16]`), sin esa traslación quedaba descentrado en `(+0.5, +0.4375, +0.5)` en vez de centrado en el origen `[-0.5, +0.5]³`. En primera persona (`ItemRenderer::renderItemInFirstPerson`: `T(0.56, -0.52, -0.72) * R(45°, Y) * S(0.4)`), ese desplazamiento empujaba el cofre muy hacia la derecha y arriba, cortándolo por el borde derecho de la pantalla.
- **En 1.5.2 (`wt-152-rend`) antes de `501b40d`** (`src/net/minecraft/src/RenderBlocks.cpp:4947-4954`): sí tenía `renderRotate(90°)` y `renderTranslate(-0.5, -0.5, -0.5)`, pero **sin `renderPushMatrix()`/`renderPopMatrix()`**, por lo que dejaba la matriz ModelView contaminada para cualquier llamador que no aislara la llamada (como `RenderMinecart`). Además, `RenderMinecart.cpp:88-93` en **ambas ramas** tenía un parche viejo (`renderTranslate(-0.5, 0, 0.5); renderRotate(90°)`) heredado de cuando `k == 22` no centraba el cofre, lo que desalineaba el cofre en la vagoneta con cofre.
- **Otros bloques especiales revisados** (1.5.2 `RenderBlocks.cpp:4947-5037`):
  - **Cofre de ender (`Block::enderChest`)** y **Cofre trampa (`Block::chestTrapped`)**: usan el mismo `renderType == 22` → `ChestItemRenderHelper::renderChest`, quedando cubiertos por el mismo arreglo y con `TileEntityEnderChestRenderer` / `TileEntityChestRenderer` centrados.
  - **Yunque (`renderType == 35`)**, **Faro (`34`)**, **Muro (`32`)**, **Tolva (`38`)**: todos envuelven su dibujo con `renderTranslate(-0.5, -0.5, -0.5)` … `renderTranslate(0.5, 0.5, 0.5)` (verificado en xemu `anvil rt=35 mvMaxDiff=0.000000 ok=1`).

### Arreglo aplicado (`archivo:línea`)
- **1.2.5** (`410957a`):
  - `src/net/minecraft/src/RenderBlocks.cpp:4897-4905`: añade `renderPushMatrix()`, `renderRotate(90.0f, 0.0f, 1.0f, 0.0f)`, `renderTranslate(-0.5f, -0.5f, -0.5f)` antes de `ChestItemRenderHelper::instance.renderChest(...)` y `renderPopMatrix()` después.
  - `src/net/minecraft/src/RenderMinecart.cpp:85-92`: sustituye el parche antiguo por `renderTranslate(0.0f, 8.0f / 16.0f, 0.0f)` … `renderTranslate(0.0f, -(8.0f / 16.0f), 0.0f)` (igual que Java).
  - `src/net/minecraft/src/TileEntityChestRenderer.cpp:83-85`: alinea el orden de dibujo (`renderStaticPart` antes que `renderDynamicPart`) con 1.5.2.
- **1.5.2** (`501b40d`):
  - `src/net/minecraft/src/RenderBlocks.cpp:4949-4953`: aísla `k == 22` con `renderPushMatrix()` / `renderPopMatrix()`.
  - `src/net/minecraft/src/RenderMinecart.cpp:85-92`: misma corrección de altura/centrado de la vagoneta con cofre (`±8/16` en Y).

### Comprobación de matriz, 480p, 720p y pantalla partida (xemu 64 MB)
- **Aislamiento de matriz ModelView** tras `renderBlockOnInventory(...)`:
  - 1.5.2: `chest` (`rt=22`), `enderChest` (`rt=22`), `trappedChest` (`rt=22`), `anvil` (`rt=35`) → **`mvMaxDiff = 0.000000` (`ok=1`)**.
  - 1.2.5: `chest` (`rt=22`) → **`mvMaxDiff = 0.000000` (`ok=1`)**.
- **¿Depende de 720p o pantalla partida?** No: el fallo estaba en espacio de objeto/vista (`renderType 22`), por lo que afectaba a **todas** las resoluciones y proporciones (más severo en 4:3/480p por menor campo horizontal, y también recortado en 720p 16:9). Proyección NDC en primera persona medida en xemu (`fov=70°`):

| Caso | Aspecto | `ndcX` (`[-1..+1]` visible) | `ndcY` | Recorte derecho (`> 1.0`) |
|---|---|---|---|---|
| Referencia bloque normal (`planks`, `rt=0`) 480p | `1.333` (4:3) | `[0.412 .. 1.372]` | `[-2.352 .. -0.456]` | Borde normal de mano Java |
| Cofre 1.2.5 **ANTES** (480p) | `1.333` (4:3) | **`[0.886 .. 1.911]`** | `[-1.572 .. -0.251]` | **Sí (~90 % fuera por la derecha)** |
| Cofre 1.2.5 **ANTES** (720p) | `1.778` (16:9) | **`[0.664 .. 1.433]`** | `[-1.572 .. -0.251]` | **Sí (>50 % cortado a la derecha)** |
| Cofre **DESPUÉS** (480p) | `1.333` (4:3) | **`[0.465 .. 1.269]`** | `[-2.176 .. -0.546]` | Igual que `planks` Java |
| Cofre **DESPUÉS** (720p) | `1.778` (16:9) | **`[0.349 .. 0.952]`** | `[-2.176 .. -0.546]` | **No (`clipR=0`, 100 % dentro)** |
| Cofre **DESPUÉS** (pantalla partida 480p) | `2.667` (8:3) | **`[0.232 .. 0.635]`** | `[-2.176 .. -0.546]` | **No (`clipR=0`)** |
| Cofre **DESPUÉS** (pantalla partida 720p) | `3.556` (32:9) | **`[0.174 .. 0.476]`** | `[-2.176 .. -0.546]` | **No (`clipR=0`)** |

---

## 3. Capturas reales de xemu (volcado del backbuffer D3D8 de Xbox)

Todas guardadas en `buzon-xbox\89-capturas\`:

| Archivo | Qué demuestra |
|---|---|
| `125_controls_legacy_reset.png` | 1.2.5 Legacy Controles tras «Restablecer»: los 4 iconos del stick izquierdo visibles en Forward/Left/Back/Right |
| `152_controls_legacy_reset.png` | 1.5.2 Legacy Controles tras «Restablecer»: mismos 4 iconos del stick izquierdo visibles |
| `125_chest_1st_person_before.png` | 1.2.5 **ANTES**: cofre en mano desplazado fuera del borde derecho y cofre en vagoneta flotando arriba |
| `125_chest_1st_person_after.png` | 1.2.5 **DESPUÉS**: cofre en mano completo en la esquina inferior derecha + icono correcto en hotbar |
| `125_chest_3rd_person_hotbar.png` | 1.2.5 tercera persona con cofre en mano + hotbar intacta |
| `152_chest_1st_person.png` | 1.5.2 primera persona con cofre en mano + vagoneta con cofre perfectamente encajada + hotbar (cofre, ender, trampa, yunque, tablones) |
| `152_enderchest_1st_person.png` | 1.5.2 primera persona con cofre de ender en mano + vagoneta con cofre |
| `152_chest_3rd_person_hotbar.png` | 1.5.2 tercera persona con cofre en mano + hotbar intacta |

---

## 4. Compilación limpia final y mediciones en xemu 64 MB

Todas las compilaciones finales se hicieron tras retirar completamente la sonda temporal (`git status` limpio y verificación binaria `hasProbe=False` en todos los XBE):

- **1.5.2 (`port152-erluza` `501b40d`)**:
  - `xbox-release` y `xbox-release-exp` compilan con `rc=0` y **0 avisos nuevos de SSE2** (13 conocidos de CRT/STL).
  - Arranque limpio en xemu 64 MB (`rel` y `exp`): `selftest: 0 failure(s)`, `double53=1`, `0 nan`, `0 [X87]`, `0 crashes`.
  - Memoria libre (`freeKB`): `rel` `45824` (f1) → `45560` (f601) → `41068` (f901 en menú, idéntico a la línea base del encargo 88); `exp` `45804` (f1) → `45540` (f601).
  - Partida semilla `1621073931`: spawn idéntico `(-189.50, 65.62, 42.50)`, `hand=0.2–0.3 ms`, `hud=0.5–1.2 ms`, `fps` subiendo hasta `55.6` durante la carga inicial de chunks en xemu (sin impacto en los 60 fps en consola real).
- **1.2.5 (`opti30-erluza` `410957a`)**:
  - `powershell -File buzon-xbox\build_opti30.ps1` compila limpio `xbox-release`, `xbox-release-exp` y **`ps2-release`** (`OptiCraft.elf`, `rc=0`, **0 avisos nuevos de SSE2**).
  - Arranque limpio en xemu 64 MB (`rel` y `exp`): `selftest: 0 failure(s)`, `double53=1`, `0 nan`, `0 [X87]`, `0 crashes`.
  - Memoria libre (`freeKB`): `rel` `46592` (f1) → `46332` (f601) → `42060` (f901); `exp` `46576` (f1) → `46316` (f601).
  - Partida semilla `1621073931`: spawn idéntico `(43.50, 49.62, 55.50)`, `hand=0.2–0.4 ms`, `hud=0.5–0.6 ms`.

---

## 5. Qué probar en la Xbox real (1.2.5 y 1.5.2)

1. **Controles → «Restablecer por defecto» (Legacy y Java)**:
   - Pulsar «Restablecer» en ambas interfaces y confirmar que **Avanzar / Izquierda / Retroceder / Derecha** conservan sus **4 iconos del stick izquierdo** (incluso con `data/` o `assets.pak` antiguo en la consola) y que **Chat** queda sin asignar sin marcarse en rojo como conflicto.
   - Seleccionar cualquiera de esas filas y **mover el stick izquierdo** en cualquier dirección: debe capturar y asignar la dirección del stick (y `Start`/`Back` o `B` en Legacy deben cancelar sin guardar `-100`).
   - Salir, reiniciar el juego y volver a entrar en Controles para confirmar que `options.txt` conserva los 4 iconos y bindings del stick.
2. **Cofre en mano, tercera persona, hotbar y vagoneta con cofre (480p, 720p y pantalla partida)**:
   - En **1.2.5** y **1.5.2**: sostener un **cofre** en primera persona y comprobar que aparece completo y centrado igual que un bloque normal (en 1.5.2 probar también **cofre de ender**, **cofre trampa** y **yunque**).
   - Cambiar a **tercera persona** y abrir el **inventario/hotbar** para confirmar que siguen alineados.
   - Colocar una **vagoneta con cofre** sobre raíles y verificar que el cofre encaja centrado dentro de la vagoneta.
