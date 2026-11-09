# Informe de Encargo 52: Grupo 2, Parte C — Redstone 1.5.2 (Comparador, Tolva, Soltador, Repetidor y Vagoneta Tolva)

**Rama:** `port152-inventario`  
**Commit:** `cf6876d871fa01221dc6531c70c7d0ecfb9974d2`  
**Fecha:** 25 de Octubre de 2026  
**Estado:** COMPLETADO CON ÉXITO — Ambas compilaciones (`xbox-release` y `xbox-release-exp`) limpias con código de salida 0.

---

## 1. Resumen de Implementación

Se ha completado el port fiel de la infraestructura de redstone avanzada de Minecraft 1.5.2 (src152) a C++ para la Xbox clásica, abarcando:
1. **Lógica base de Redstone (`BlockRedstoneLogic`):** Base unificada para repetidores y comparadores con soporte de prioridades de scheduling de ticks (`func_82740_a`), orientación direccional y detección de bloqueo lateral (`func_94476_e`).
2. **Repetidor 1.5.2 (`BlockRedstoneRepeater`):** Reescrito sobre `BlockRedstoneLogic` preservando la lógica de delay/ticks, pero añadiendo el bloqueo lateral introducido en 1.4+, incluyendo renderizado de la barra de bedrock en `RenderBlocks` cuando está bloqueado por otro repetidor o comparador.
3. **Comparador de Redstone (`BlockComparator`, `TileEntityComparator`, Ítem 404):** Bloques 149 (apagado) y 150 (encendido) con alternancia entre modo Comparación y modo Resta (sustracción) mediante clic derecho, lectura de señal directa o a través de 1 bloque sólido opaco, y cálculo de señal para inventarios (`Container::calcRedstoneFromInventory`). Actualización de `Block::isComparatorId` (reconociendo 149 y 150) y propagación mediante `World::func_96440_m`.
4. **Tolva (`BlockHopper`, `TileEntityHopper`, Ítem 154):** Capacidad de 5 ranuras, ciclo de transferencia con cooldown de 8 ticks (fiel a 1.5.2), extracción e inserción hacia contenedores enfrentados con soporte para `ISidedInventory` (p. ej. hornos), absorción de ítems flotantes (`EntityItem`) por encima, y bloqueo de redstone mediante el bit 8 de metadatos. RenderType 38 completo (mundo e inventario) en `RenderBlocks`.
5. **Vagoneta con Tolva (`EntityMinecartHopper`, Ítem 408):** Tipo de vagoneta 5 con 5 ranuras, absorción continua de ítems sobre vías y bloqueo automático al pasar sobre un raíl activador energizado (`onActivatorRailPass`).
6. **Soltador (`BlockDropper`, `TileEntityDropper`, Bloque 158):** Dispensación como `EntityItem` al mundo cuando apunta a aire/bloque no contenedor, o inserción atómica directa en el inventario contenedor adyacente según orientación.

---

## 2. Tabla Bloque / Ítem -> Archivos y Líneas

| Bloque / Ítem | ID Bloque / Ítem | Archivos Principales | Líneas / Métodos Clave |
|---|---|---|---|
| **Base Lógica Redstone** | N/A (Abstracto) | `src/net/minecraft/src/BlockRedstoneLogic.h`<br>`src/net/minecraft/src/BlockRedstoneLogic.cpp` | Lógica de scheduling prioritario, `func_94476_e` (bloqueo), `func_94482_f` (potencia lateral). |
| **Repetidor de Redstone** | 93 (idle), 94 (active) | `src/net/minecraft/src/BlockRedstoneRepeater.h`<br>`src/net/minecraft/src/BlockRedstoneRepeater.cpp` | Herencia de `BlockRedstoneLogic`, re-implementación de bloqueo lateral, `isPowered[4]`. |
| **Comparador de Redstone** | 149 (idle), 150 (active)<br>Ítem 404 (ID 148) | `src/net/minecraft/src/BlockComparator.h`<br>`src/net/minecraft/src/BlockComparator.cpp`<br>`src/net/minecraft/src/TileEntityComparator.h`<br>`src/net/minecraft/src/TileEntityComparator.cpp` | `blockActivated` (conmuta modo), `calculateInputStrength` (directo y a través de bloque sólido), NBT `OutputSignal`. |
| **Tolva (Hopper)** | Bloque 154<br>Ítem 154 | `src/net/minecraft/src/BlockHopper.h`<br>`src/net/minecraft/src/BlockHopper.cpp`<br>`src/net/minecraft/src/TileEntityHopper.h`<br>`src/net/minecraft/src/TileEntityHopper.cpp`<br>`src/net/minecraft/src/ContainerHopper.h`<br>`src/net/minecraft/src/GuiHopper.h` | 5 slots, cooldown de 8 ticks, `suckItemsIntoHopper`, `insertItemToInventory`, bit 8 de redstone lock, RenderType 38. |
| **Vagoneta con Tolva** | Entidad 46 (tipo 5)<br>Ítem 408 (ID 152) | `src/net/minecraft/src/EntityMinecartHopper.h`<br>`src/net/minecraft/src/EntityMinecartHopper.cpp`<br>`src/net/minecraft/src/RenderMinecart.cpp` | `onActivatorRailPass` (bloqueo/desbloqueo), succión de ítems y entidades, integración de `Hopper.h`. |
| **Soltador (Dropper)** | Bloque 158 | `src/net/minecraft/src/BlockDropper.h`<br>`src/net/minecraft/src/BlockDropper.cpp`<br>`src/net/minecraft/src/TileEntityDropper.h`<br>`src/net/minecraft/src/TileEntityDropper.cpp` | `dispense` (inserción en inventario frontal o spawn de `EntityItem`), `container.dropper`. |
| **Interfaz Hopper** | N/A | `src/net/minecraft/src/Hopper.h` | `getWorldObj`, `getXPos`, `getYPos`, `getZPos`, `getHopperInventory`. |
| **Interfaz Sided Inventory** | N/A | `src/net/minecraft/src/ISidedInventory.h` | `getAccessibleSlotsFromSide`, `canInsertItem`, `canExtractItem`. Implementado en `TileEntityFurnace`. |

---

## 3. Contenedores y Señal al Comparador

El comparador evalúa la plenitud de los contenedores mediante `Container::calcRedstoneFromInventory(IInventory *inv)`:
$$\text{señal} = \left\lfloor \frac{\sum \frac{\text{stackSize}}{\min(\text{maxStack}, \text{limit})}}{\text{tamañoInventario}} \times 14 \right\rfloor + 1$$
(con señal 0 si el contenedor está completamente vacío).

Los bloques adaptados con `hasComparatorInputOverride()` y `getComparatorInputOverride()`:
- **`BlockChest`:** Evalúa cofre simple o cofre doble (`InventoryLargeChest`). Notifica a comparadores (`func_96440_m`) al destruirse o cambiar inventario.
- **`BlockDispenser`:** Evalúa sus 9 ranuras.
- **`BlockDropper`:** Evalúa sus 9 ranuras (compartiendo tile entity dispenser).
- **`BlockFurnace`:** Evalúa las 3 ranuras (combustible, entrada, salida).
- **`BlockBrewingStand`:** Evalúa las 4 ranuras del soporte de pociones.
- **`BlockJukeBox`:** Emite señal 15 fija cuando tiene un disco insertado.
- **`BlockDetectorRail`:** Evalúa el contenido de cualquier vagoneta con inventario sobre el raíl (p. ej. cofre o tolva).
- **`BlockHopper`:** Evalúa sus 5 ranuras de tolva.
- **Actualización de comparadores:** `Block::isComparatorId(id)` ahora devuelve `true` para 149 y 150. `World::func_96440_m` y `TileEntity::onInventoryChanged` propagan notificaciones a los comparadores vecinos (y a través de bloques sólidos vecinos) sin demoras.

---

## 4. Análisis de Rendimiento y Coste por Tick de la Tolva

Para salvaguardar la CPU de la Xbox clásica (Pentium III Coppermine 733 MHz) y la memoria de 64 MB:
1. **Zero heap allocation por tick:**
   - No se utiliza `new` en la rutina de actualización de la tolva (`TileEntityHopper::updateEntity` ni `EntityMinecartHopper::onUpdate`).
   - Los arreglos de ranuras accesibles de `ISidedInventory` en `TileEntityFurnace` son estáticos `const std::vector<int_t> &` preasignados (arriba, laterales, abajo), eliminando cualquier reserva de memoria dinámica durante transferencias.
2. **Transfer Cooldown de 8 ticks:**
   - La comprobación pesada de inventarios adyacentes y succión de ítems solo ocurre 1 vez cada 8 ticks por tolva activa. Si la tolva transfiere un ítem o está vacía/esperando, el contador de cooldown previene la ejecución de consultas de inventario en los ticks intermedios.
3. **Optimización de AABB para recogida de ítems:**
   - La búsqueda de `EntityItem` en el mundo solo se realiza si **no** hay un contenedor arriba (`getInventoryAboveHopper == nullptr`). Si hay un cofre u otra tolva encima, nunca se efectúa consulta AABB espacial de entidades.
4. **Metadatos e Inactividad con Redstone:**
   - Cuando una tolva recibe corriente de redstone (metadato con bit 8 = 0 en lógica de 1.5.2), el procesamiento de inserción y succión se desactiva inmediatamente sin explorar inventarios.

---

## 5. Integración con el Sistema y UI de Consola

1. **Recetas de Crafteo (`CraftingManager.cpp`):**
   - Comparador: 3 antorchas de redstone, 1 cuarzo del Nether, 3 piedras lisas.
   - Soltador: 7 adoquines, 1 polvo de redstone.
   - Tolva: 5 lingotes de hierro, 1 cofre.
   - Vagoneta con Tolva: 1 tolva + 1 vagoneta.
2. **Menú de Crafteo de Consola Xbox:**
   - Añadidas entradas a `scripts/xbox/make_crafting_groups.py` en `EXTRA_ENTRIES`: 154 (Hopper), 158 (Dropper), 404 (Comparator) en Mecanismos (`M`), y 408 (Hopper Minecart) en Transporte (`V`). Regenerado `LegacyCraftingGroups.h` asegurando preservación de bloques de cuarzo y finales de línea normalizados.
3. **Modo Creativo y TooManyItems:**
   - Registrados en `ContainerCreative.cpp` dentro de la pestaña de Redstone y Transporte (`CREATIVE_TAB_REDSTONE_TRANSPORT`).
   - Soportados nativamente en TooManyItems mediante la iteración de items/blocks registrados.
4. **Texturas e Iconos:**
   - Renderizado 2D de bloques en inventario para `BlockHopper` usando `Texture152Atlas` (`TEX152_BLOCK_ICON`).
   - Texturas de cara y de interior (`hopper_inside.png`, `hopper_top.png`, `hopper.png`).
5. **Localización (`StringTranslate.cpp`):**
   - Nombres oficiales en inglés y español para todos los bloques, ítems y contenedores (`tile.hopper.name`, `container.hopper`, `tile.dropper.name`, `container.dropper`, `item.comparator.name`, `item.minecartHopper.name`).

---

## 6. Verificación de Compilaciones

- `ninja -C build\xbox-release`: **Exit code 0**, ISO generada: `OptiCraft.iso` (77,095,035 bytes).
- `ninja -C build\xbox-release-exp`: **Exit code 0**, ISO generada: `OptiCraft.iso` (77,144,187 bytes).
- Candado `buzon-xbox\BUILD.lock` respetado y liberado.
