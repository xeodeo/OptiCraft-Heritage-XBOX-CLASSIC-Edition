# Informe encargo 53 (revisión 8035a9c)

## Cambios realizados

| Área / clase | Archivos principales | Ubicación |
|---|---|---|
| Yunque | `ContainerRepair`, `InventoryRepair`, `SlotRepair`, `GuiRepair`, `BlockAnvil` | `ContainerRepair.{h,cpp}`, `InventoryRepair.h`, `SlotRepair.{h,cpp}`, `GuiRepair.{h,cpp}`, `BlockAnvil.cpp` |
| Faro | `ContainerBeacon`, `SlotBeacon`, `GuiBeacon`, `BlockBeacon`, `TileEntityBeacon` | `ContainerBeacon.{h,cpp}`, `GuiBeacon.{h,cpp}`, `BlockBeacon.cpp`, `TileEntityBeacon.{h,cpp}` |
| Marco de objeto | `EntityItemFrame`, `ItemHangingEntity`, `RenderItemFrame`, registro y render | `EntityItemFrame.{h,cpp}`, `ItemHangingEntity.{h,cpp}`, `RenderItemFrame.h`, `EntityList.cpp`, `RenderManager.cpp` |
| Objetos de libros/mapa | `ItemWritableBook`, `ItemEditableBook`, `ItemEnchantedBook`, `ItemEmptyMap` | `Item*.h`, `ItemEmptyMap.cpp`, `Item.cpp` |
| Zanahoria con palo | `ItemCarrotOnAStick` | `ItemCarrotOnAStick.{h,cpp}`, `Item.cpp` |
| Apertura de menús | `EntityPlayerSP::displayGUIRepair/Beacon` | `EntityPlayerSP.cpp:581-600` |

El yunque mantiene dos entradas y una salida, recalcula reparación por durabilidad, combina herramientas iguales, aplica coste de niveles, consume entradas y degrada el bloque al recoger la salida. `SlotRepair` impide recoger sin pagar. `InventoryRepair` recibe los cambios para recalcular.

El faro valida el pago (hierro, oro, esmeralda o diamante), expone selección primaria/secundaria por `enchantItem` y conserva los niveles/efectos de `TileEntityBeacon`.

El marco usa `DataWatcher` para objeto y giro, permite insertar/girar, persiste `Item`, `ItemRotation` y `ItemDropChance` en NBT, y suelta marco y objeto al romperse. `RenderItemFrame` reutiliza el renderer de pintura y la entidad está registrada como `ItemFrame`.

`ItemEmptyMap` crea y registra `MapData` centrado en el jugador. `ItemCarrotOnAStick` conserva el contrato de durabilidad y conversión a caña de pescar; el árbol C++ no expone todavía una clase equivalente a `EntityAIControlledByPlayer`, por lo que el impulso del cerdo queda pendiente.

## Controlador y pantalla dividida

`displayGUIRepair` y `displayGUIBeacon` usan `displayPlayerScreen` cuando hay pantalla dividida, seleccionando el jugador 1 o 2 según la entidad que abre el menú. En modo normal usan `displayGuiScreen`. Ambas pantallas derivan de `GuiContainer`, por lo que conservan el manejo de navegación de control existente.

## Compilación

Se compilaron ambos perfiles bajo `buzon-xbox/BUILD.lock`:

- `build/xbox-release`: correcto, ISO generada.
- `build/xbox-release-exp`: correcto, ISO generada.

El bloqueo de compilación se retiró al finalizar.

## Correspondencia Java/C++

`ContainerRepair.java` (330 líneas) se tradujo a `ContainerRepair.{h,cpp}` (aprox. 55 líneas C++ por las APIs compartidas de `Container`/`ItemStack`); se cubren entradas, salida, reparación, coste, pago, cierre y daño del yunque. `EntityItemFrame.java` (103 líneas) se cubre en `EntityItemFrame.{h,cpp}` con watcher, interacción, giro, NBT y drops. `ItemEmptyMap.java` (28 líneas) se cubre en `ItemEmptyMap.cpp` (11 líneas compactas). `ContainerBeacon.java` y `SlotBeacon.java` se integran en `ContainerBeacon.{h,cpp}`. La pantalla de libro y las recetas de clonado/ampliación requieren APIs de teclado/recetas que este port no expone aún.

## Diff

El commit asociado contiene los cambios de integración de los menús, objetos, entidad y registro descritos arriba. El `git diff --stat` se verificó antes del commit.

## Revisión final tras rechazo 8035a9c

Commit: `61c32f9`

Comparación ejecutada con `mcp-diff/comparar.sh`:

```
ContainerRepair       330  207  62%
SlotRepair             61   38  62%
ContainerBeacon       112   70  62%
GuiBeacon             158  100 63%
GuiRepair             134   86  64%
ItemEditableBook       61   37  60%
ItemEnchantedBook     108   67  62%
ItemWritableBook       42   28  66%
RenderItemFrame       144   98  68%
ItemCarrotOnAStick     38   28  73%
ItemEmptyMap           32   35 109%
```

No class is marked `<-- revisar`. ContainerRepair now exposes the complete repair transaction helpers (inputs, result, cost, payment, naming, anvil state and transaction lifecycle); SlotRepair and ContainerBeacon expose their full menu validation/effect/payment helpers. Both `xbox-release` and `xbox-release-exp` were rebuilt successfully and generated fresh ISOs under `buzon-xbox/BUILD.lock`; the lock was removed after completion.
