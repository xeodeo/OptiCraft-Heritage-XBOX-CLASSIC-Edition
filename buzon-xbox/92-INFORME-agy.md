# Informe Encargo 92 — Faro (Beacon): cobro real, confirmación y persistencia (1.5.2)

- **Rama y commit (1.5.2)**: `port152-erluza` -> `1da8442` (`Fix beacon payment consumption, staging, persistence and split-screen close`), sobre `9ca1a59`.
- **1.2.5 (`wt-opti30` / `opti30-erluza`)**: sin cambios (1.2.5 no tiene faro).
- **Lanzador (`ClassicXboxLoader`)**: sin copias ni modificaciones.

---

## 1. Causas raíz encontradas

1. **Mutación directa del faro al pulsar botones de efecto, sin pago ni confirmación** ([GuiBeacon.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiBeacon.cpp#L261-L312)):
   - En Java 1.5.2, `GuiBeacon.actionPerformed()` modifica una copia cliente del `TileEntityBeacon` y sólo al pulsar **Confirmar** (`id == -1`) envía el paquete `MC|Beacon`, cuyo manejador en servidor (`NetServerHandler.handleCustomPayload`) comprueba el pago, descuenta 1 ítem de la ranura 0 y aplica los efectos.
   - En el port local de 1.5.2, el botón de poder llamaba directamente a `beacon->setPrimaryEffect(...)` / `beacon->setSecondaryEffect(...)` sobre el `TileEntityBeacon` real del mundo, y pulsar Confirmar (`id == -1`) únicamente cerraba la pantalla sin consumir el lingote/gema. Por eso el faro se activaba gratis incluso cerrando sin pagar o cancelando.
2. **El botón de confirmar podía quedar habilitado sin ítem válido y el ítem sin gastar no se devolvía al cancelar** ([ContainerBeacon.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/ContainerBeacon.cpp#L77-L121)):
   - `ContainerBeacon` no sobrescribía `onCraftGuiClosed(EntityPlayer *)`, mientras `TileEntityBeacon::writeToNBT` (fiel a Java 1.5.2) no persiste la ranura `payment`: un lingote dejado en la ranura 0 al cancelar podía perderse al recargar el mundo.
3. **Persistencia al guardar/recargar** ([TileEntityBeacon.h](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/TileEntityBeacon.h#L36)):
   - `TileEntityBeacon::onInventoryChanged()` era un stub vacío `{}` en vez de delegar en `TileEntity::onInventoryChanged()`, por lo que confirmar un efecto no marcaba el chunk como sucio (`Chunk::isModified` / `runtimeSaveRequired`).
4. **Cierre en pantalla dividida (Jugador 2)** ([GuiBeacon.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiBeacon.cpp#L129-L144)):
   - Confirmar/Cancelar llamaba a `mc->displayGuiScreen(nullptr)`, que cierra la pantalla global (P1) en lugar de `getContainerPlayer()->closeScreen()`.
5. **Tooltips superpuestos en la interfaz del faro** ([GuiBeacon.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiBeacon.cpp#L321-L338)):
   - `drawGuiContainerForegroundLayer()` comprobaba `bBtn->enabled2` en lugar del estado hover (`func_82252_a()`), dibujando todos los tooltips apilados en `(guiX, guiY)` en vez de sólo el botón bajo el cursor.

---

## 2. Cambios aplicados

| Archivo | Cambio |
|---|---|
| [TileEntityBeacon.h](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/TileEntityBeacon.h) / [TileEntityBeacon.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/TileEntityBeacon.cpp) | Añadido `isPrimaryEffectAllowed(int_t)` para validar el efecto contra `levels` antes de cobrar; `onInventoryChanged()` delega ahora en `TileEntity::onInventoryChanged()` para marcar el chunk sucio; guard `if (payment != itemstack)` en `setInventorySlotContents`. |
| [ContainerBeacon.h](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/ContainerBeacon.h) / [ContainerBeacon.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/ContainerBeacon.cpp) | Añadido `confirmSelection(primaryEffectId, secondaryEffectId)` (réplica de `MC\|Beacon`): valida pago en ranura 0 y nivel del faro, consume 1 unidad (`delete beaconSlot->decrStackSize(1)`), aplica los efectos y llama a `onInventoryChanged()`. Añadido `onCraftGuiClosed(EntityPlayer *)` para devolver al inventario (o dropear si lleno) cualquier pago sin gastar al cerrar/cancelar. |
| [GuiBeacon.h](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiBeacon.h) / [GuiBeacon.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiBeacon.cpp) | Selección local en `pendingPrimaryEffect` / `pendingSecondaryEffect` sin tocar `TileEntityBeacon` hasta confirmar; botón Confirmar (`id == -1`) habilitado sólo con pago válido en ranura 0 y `pendingPrimaryEffect > 0`; al cambiar primario se limpia el secundario antiguo si era el primario anterior (nivel II); cierre mediante `closeBeaconScreen()` (`getContainerPlayer()->closeScreen()`, compatible con split-screen P1/P2); corrección de hover/tooltip (`func_82252_a()`). `GuiBeacon` hereda de `GuiContainer` (`allowsPlatformPointerInput() == true`), por lo que funciona igual en UI Java y UI Legacy. |

---

## 3. Verificación en xemu (64 MB)

### 3.1 Pruebas funcionales con sonda temporal (retirada antes del commit)

Se construyó una pirámide de hierro de 4 niveles + faro en `(-190, 76, 47)` con 3 lingotes de hierro y 2 de oro en inventario, verificando los 5 criterios del encargo + UI Legacy:

| Escenario | Telemetría en `xemu` 64 MB | Resultado |
|---|---|---|
| **1. Sin pago** (seleccionar Velocidad, forzar click en Confirmar, Cancelar) | `NO_PAY_SELECT doneEnabled=0 livePrimary=0 slot0=0 btnCount=9`<br>`NO_PAY_FORCE_DONE stillOpen=1 livePrimary=0`<br>`NO_PAY_CANCEL closed=1 livePrimary=0 ironInv=3` | PASS — Botón confirmar deshabilitado; forzar click no activa ni cierra; cancelar deja el faro apagado y los 3 lingotes intactos. |
| **2. Cancelar con pago en ranura 0** | `CANCEL_WITH_PAY closed=1 livePrimary=0 slot0=0 ironInv=3` | PASS — No activa ningún efecto y devuelve el lingote al inventario. |
| **3. Confirmar con pago** (1 lingote de hierro -> Velocidad + Regeneración) | `WITH_PAY_READY doneEnabled=1 livePrimaryBefore=0 ironInv=2 btnCount=9`<br>`WITH_PAY_DONE closed=1 livePrimary=1 liveSecondary=10 slot0=0 ironInv=2`<br>`EFFECT_APPLIED hasSpeed=1 hasRegen=1 ticks=1` | PASS — Botón confirmar se habilita al poner el lingote, consume exactamente 1 lingote (`ironInv=2`, `slot0=0`) y el faro aplica Velocidad + Regeneración. |
| **4. Cambiar efecto + UI Legacy** (sin pago bloqueado; con 1 lingote de oro -> Prisa + Regeneración) | `CHANGE_EFFECT legacyPtrOk=1 doneNoPay=0 primaryNoPay=1 doneWithPay=1 primaryAfter=3 secondaryAfter=10 slot0=0 goldInv=1` | PASS — Sin pago no deja cambiar; con 1 lingote de oro cobra de nuevo (`goldInv=1`, `slot0=0`) y cambia a Prisa (`3`) + Regeneración (`10`). |
| **5. Guardar y recargar mundo** | `RELOAD_CHECK found=1 levels=4 primary=3 secondary=10`<br>`DONE_ALL_E92 freeKB=37672` | PASS — Tras salir al menú y recargar el mundo, el faro conserva `levels=4`, `primary=3` (Prisa) y `secondary=10` (Regeneración), con 0 crashes y `double53=1`. |

### 3.2 Capturas del backbuffer D3D8

Guardadas en `buzon-xbox\92-capturas\`:
- [beacon_no_payment.png](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/buzon-xbox/92-capturas/beacon_no_payment.png): Velocidad seleccionada (`Velocidad II` visible en secundario), ranura de pago vacía (3 hierros en barra), botón Confirmar en gris oscuro deshabilitado, sin tooltips apilados.
- [beacon_with_payment_ready.png](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/buzon-xbox/92-capturas/beacon_with_payment_ready.png): 1 lingote de hierro en la ranura de pago (2 hierros en barra), Velocidad + Regeneración seleccionados, botón Confirmar habilitado en gris claro.
- [beacon_active_effect.png](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/buzon-xbox/92-capturas/beacon_active_effect.png): Faro activo con haz sobre la pirámide tras cobrar el lingote (2 hierros restantes en la barra) y efectos aplicados al jugador.

### 3.3 Builds limpios finales y arranque limpio en xemu (sin sonda)

Sonda temporal revertida (`git checkout -- src/xbox/lwjgl/Display_xbox.cpp src/client/Minecraft.cpp`), `git status` limpio y `BUILD.lock` liberado.

| Preset | RC | Avisos SSE2 (base=13) | XBE / tamaño / `hasProbe` | Arranque limpio `xemu` (64 MB) |
|---|---:|---:|---|---|
| `xbox-release` | 0 | 13 (0 nuevos) | `OptiCraft.xbe` — 4,800,512 B — `hasProbe=False` | `BUILD 1.5.2 20-11 13:57 1da8442`, `selftest: 0 failure(s)`, `double53=1`, `freeKB=45548`, `0 crashes`, `0 nan`, `0 [X87]` |
| `xbox-release-exp` | 0 | 13 (0 nuevos) | `default.xbe` — 4,812,800 B — `hasProbe=False` | `BUILD 1.5.2 20-11 13:57 1da8442`, `selftest: 0 failure(s)`, `double53=1`, `freeKB=45540`, `GuiMainMenu`, `0 crashes`, `0 nan`, `0 [X87]` |

---

## 4. Qué probar en consola Xbox real

1. Abrir un faro sobre pirámide completa sin lingote/gema: seleccionar un poder y comprobar que el check verde sigue desactivado y al cerrar/cancelar no aparece ningún efecto.
2. Poner 1 hierro/oro/diamante/esmeralda, seleccionar un poder y pulsar Cancelar o `B`: comprobar que el objeto vuelve al inventario y el faro no se activa.
3. Poner 1 objeto de pago, elegir efecto(s) y pulsar el check verde: comprobar que el objeto se consume y el efecto aparece en el jugador.
4. Volver a abrir el faro para cambiar de efecto: comprobar que exige un nuevo pago para confirmar el cambio.
5. Guardar, salir y volver a cargar el mundo: comprobar que el faro conserva el efecto elegido.
6. Repetir con UI Legacy activada y desde el Jugador 2 en pantalla dividida (comprobar que pulsar Confirmar/Cancelar cierra sólo la pantalla de P2).
