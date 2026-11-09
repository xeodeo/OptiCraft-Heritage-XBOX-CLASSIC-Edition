# Informe Encargo 70: Cruceta configurable como la Legacy e iconos del joystick con dirección

- **Rama**: `port152-inventario` (iniciado desde `port152-claude` tras `git reset --hard`)
- **Commit final**: `8b3fde8`
- **Compilaciones**: `build/xbox-release` (OK) y `build/xbox-release-exp` (OK) - ISOs generadas exitosamente con `extract-xiso`.
- **Candado**: Adquirido y liberado según protocolo en `buzon-xbox\BUILD.lock`.

---

## 1. Tabla de Acciones y Asignación de Botones por Defecto (con CAMBIO DEL USUARIO)

| Acción (Español) | Action (English) | KeyBinding | Botón por defecto Xbox | Botón por defecto PS2 | Notas de comportamiento |
|---|---|---|---|---|---|
| **Adelante** | Forward | `keyBindForward` | **Stick Izquierdo Arriba** | **Stick Izquierdo Arriba** | Andar hacia adelante (icono stick con flecha arriba) |
| **Atrás** | Back | `keyBindBack` | **Stick Izquierdo Abajo** | **Stick Izquierdo Abajo** | Andar hacia atrás (icono stick con flecha abajo) |
| **Izquierda** | Left | `keyBindLeft` | **Stick Izquierdo Izq.** | **Stick Izquierdo Izq.** | Andar lateral izq. (icono stick con flecha izq.) |
| **Derecha** | Right | `keyBindRight` | **Stick Izquierdo Der.** | **Stick Izquierdo Der.** | Andar lateral der. (icono stick con flecha der.) |
| **Saltar** | Jump | `keyBindJump` | **A** | **Cruz (Cross)** | Saltar; doble toque rápido entra en vuelo creativo |
| **Agacharse** | Sneak | `keyBindSneak` | **LS** (Click Stick Izq) | **L3** | Sigilo / agacharse en suelo |
| **Atacar / Destruir** | Attack | `keyBindAttack` | **RT** (Gatillo Der) | **R2** | Minar / atacar |
| **Usar / Colocar** | Use | `keyBindUseItem` | **LT** (Gatillo Izq) | **L2** | Colocar bloques / interactuar / comer |
| **Soltar** | Drop | `keyBindDrop` | **B** | **Círculo (Circle)** | Soltar el ítem seleccionado de la barra |
| **Inventario** | Inventory | `keyBindInventory` | **Y** | **Triángulo (Triangle)** | Abre el inventario del jugador o panel creativo |
| **Fabricar** | Crafting | `keyBindCrafting` | **X** | **Cuadrado (Square)** | Interfaz de crafteo rápido Legacy |
| **Objeto anterior** | Previous Item | `keyBindPrevItem` | **Blanco / LB (White)** | **L1** | Cicla la barra hacia la izquierda (anterior) |
| **Objeto siguiente** | Next Item | `keyBindNextItem` | **Negro / RB (Black)** | **R1** | Cicla la barra hacia la derecha (siguiente) |
| **Cambiar perspectiva** | Change Perspective | `keyBindTogglePerspective` | **RS** (Click Stick Der) | **R3** | Cicla perspectiva: 1ª persona, 3ª trasera, 3ª frontal |
| **Subir (volar)** | Fly Up | `keyBindFlyUp` | **Cruceta ARRIBA** | **Cruceta ARRIBA** | Ascenso en vuelo creativo (estilo LCE) |
| **Bajar (volar)** | Fly Down | `keyBindFlyDown` | **Cruceta ABAJO** | **Cruceta ABAJO** | Descenso en vuelo creativo (estilo LCE) |
| **Chat** | Chat | `keyBindChat` | *(Sin asignar / Ninguno)* | *(Sin asignar / Ninguno)* | Acción reasignable en Controles (libre por defecto) |
| *(Cruceta IZQ / DER)* | *(D-Pad Left / Right)* | — | *(Sin asignar / Libre)* | *(Sin asignar / Libre)* | Libres por defecto para que el usuario asigne lo que desee |

---

## 2. Puntos Clave del Cambio del Usuario (Override 18:15)

1. **Cruceta IZQ y DER libres por defecto**:
   - Ya no cambian los objetos de la barra rápida de forma predeterminada.
   - Las 4 flechas de la cruceta hacen **exclusivamente lo que el usuario configure** en *Opciones > Controles*.
   - «Objeto anterior» y «Objeto siguiente» están asignados por defecto a **LB / RB** (**Blanco / Negro** en Xbox, **L1 / R1** en PS2), donde ya estaban habitualmente.

2. **Cruceta ARRIBA y ABAJO para Vuelo Creativo (LCE)**:
   - `keyBindFlyUp` («Subir»): asignado a **Cruceta ARRIBA**. En vuelo creativo aplica ascenso vertical continuo (`motionY += 0.15`).
   - `keyBindFlyDown` («Bajar»): asignado a **Cruceta ABAJO**. En vuelo creativo aplica descenso vertical continuo (`motionY -= 0.15`).
   - Nada más interfiere con estas flechas de forma fija.

3. **Acción Chat**:
   - `keyBindChat` existe como acción reasignable en la lista de Controles.
   - Por defecto en consola está **desvinculada / sin botón asignado** (`KEY_NONE`) para evitar solapamientos con el vuelo.
   - Si el usuario desea usar el chat (por ejemplo en multijugador o con teclado / mando), puede asignarlo a cualquier botón desde *Opciones > Controles*. Además, si el usuario decidiera asignarlo al mismo botón que el vuelo, se protege para que en vuelo creativo mande el descenso y no se abra la ventana de chat.

4. **Reasignación Total desde Opciones > Controles**:
   - `kRemappableButtons` en `XboxInput.cpp` y `Ps2InputMapper.cpp` incluye las 4 direcciones de la cruceta (`XBOX_PAD_DPAD_UP`, `XBOX_PAD_DPAD_DOWN`, `XBOX_PAD_DPAD_LEFT`, `XBOX_PAD_DPAD_RIGHT`).
   - Cualquier acción del juego (perspectiva, chat, objeto ant./sig., soltar, inventario, volar, etc.) se puede vincular a cualquier flecha de la cruceta desde la pantalla de Controles.

5. **Iconos Direccionales del Stick Izquierdo (Estilo LCE)**:
   - Se extrajeron los glifos originales de la LCE (`ButtonLeftStick_Navigate.png`) y se integraron en el atlas de botones de Xbox (`buttons_xbox.png`, `XboxButtonAtlasData.h`, `XboxButtonAtlasPng.h`).
   - En *Opciones > Controles*, las acciones de movimiento (Adelante, Atrás, Izquierda, Derecha) muestran el icono del stick izquierdo con la flecha indicadora correspondiente a cada dirección.

6. **Pantalla Partida (Jugador 2)**:
   - En `XboxSplitScreen.cpp`, el Jugador 2 cicla la barra rápida con Blanco/Negro (LB/RB), coincidiendo con el Jugador 1.
   - En vuelo creativo, P2 vuela hacia arriba con Cruceta ARRIBA y hacia abajo con Cruceta ABAJO.
   - Cruceta IZQ y DER quedan liberadas de la barra para P2.

7. **Inventarios**:
   - La navegación celda por celda en interfaces de cofres, inventario y menús sigue intacta con la cruceta (`ContainerSlotNavigator`, `updateMenu`).

8. **Migración de Partidas Guardadas**:
   - En `platformGameSettingsFinalizeLoad`, las opciones guardadas previas migran `keyBindPrevItem` a Blanco (LB), `keyBindNextItem` a Negro (RB), `keyBindFlyUp` a Cruceta Arriba, `keyBindFlyDown` a Cruceta Abajo, y `keyBindChat` a desvinculado por defecto.
   - Las teclas de movimiento se migran a las teclas sintéticas del stick para evitar que queden atadas a códigos de teclado o cruceta antigua.

---

## 3. Resumen de Commits

1. `32c3f1b`: `feat(gui): add directional left stick icons to atlas and controls display`
2. `ed07da3`: `feat(input): add reassignable hotbar, perspective, fly, and chat keybindings`
3. `baa7477`: `fix: include KeyBinding.h in EntityPlayerSP.cpp`
4. `8b3fde8`: `fix(input): apply user override for D-pad fly controls and bumper hotbar defaults`
