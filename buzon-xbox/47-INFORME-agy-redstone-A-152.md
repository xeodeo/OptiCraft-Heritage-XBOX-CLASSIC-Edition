# Informe Encargo 47 (agy): Grupo 2, Parte A — Potencia de Redstone 1.5 + Botones, Placas, Sensor de Luz Diurna, Cable Trampa

- **Agente**: `agy`
- **Worktree**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-agy`
- **Rama**: `port152-inventario` (rebasada sobre `port152-base` `1097f9c`)
- **Fuente de verdad**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`

---

## 1. Resumen de Implementación

Se ha implementado en su totalidad la **Parte A del Grupo 2** (redstone 1.5.2):

1. **API de Potencia Analógica (0..15) e Interoperabilidad 1.2.5**:
   - En `Block.h` y `Block.cpp`:
     - `isProvidingWeakPower(IBlockAccess *blockAccess, int_t x, int_t y, int_t z, int_t side)`
     - `isProvidingStrongPower(IBlockAccess *blockAccess, int_t x, int_t y, int_t z, int_t side)`
     - Por defecto, cada bloque existente delega en sus métodos booleanos clásicos (`isPoweringTo` / `isIndirectlyPoweringTo`), retornando `15` si es `true` y `0` si es `false`. De este modo los bloques no portados aún (antorchas de redstone, repetidores 1.2.5, palancas, detectores, etc.) siguen funcionando con potencia máxima.
   - En `World.h` y `World.cpp`:
     - `isBlockProvidingPowerTo`: retorna ahora nivel `int_t` (0..15).
     - `getBlockPowerInput(int_t x, int_t y, int_t z)`: calcula la potencia recibida en las 6 caras evaluando tanto `isBlockProvidingPowerTo` como `getIndirectPowerLevelTo`.
     - `getIndirectPowerLevelTo(int_t x, int_t y, int_t z, int_t side)`: consulta la potencia débil y la propagación a través de bloques opacos estándar.
     - `getIndirectPowerOutput(int_t x, int_t y, int_t z, int_t side)`: potencia fuerte provista a una cara.
     - `getStrongestIndirectPower(int_t x, int_t y, int_t z)`: potencia máxima recibida de bloques vecinos.
     - Métodos booleanos `isBlockGettingPowered` e `isBlockIndirectlyGettingPowered` actualizados para retornar `> 0`.

2. **Cable de Redstone (`BlockRedstoneWire`) Portado a 1.5.2**:
   - Algoritmo completo de 1.5.2 con atenuación analógica de potencia (decremento de 1 por bloque de distancia).
   - Manejo de `wiresProvidePower` para evitar recursión de lecturas mutuas durante el recálculo.
   - `calculateCurrentChanges` utiliza `getStrongestIndirectPower` y la lectura directa de fuentes e hilos vecinos.
   - Provisión de potencia débil y fuerte conforme a las reglas 1.5.2 (`isProvidingWeakPower` y `isProvidingStrongPower`).

3. **Botones de Piedra y Madera**:
   - Separación de la jerarquía: `BlockButton` (clase base con flag `sensible`).
   - `BlockButtonStone` (ID 77): botón clásico de piedra (`sensible = false`, 20 ticks = 1 segundo).
   - `BlockButtonWood` (ID 143): botón de madera (`sensible = true`, 30 ticks = 1.5 segundos, activable también por impacto de flechas mediante chequeo de `EntityArrow`).

4. **Placas de Presión**:
   - `BlockBasePressurePlate`: clase abstracta base que maneja bounding box, interacción, setState y llamadas de potencia.
   - `BlockPressurePlate`: rediseñada como subclase de `BlockBasePressurePlate` para piedra (ID 70, activa solo con mobs) y madera (ID 72, activa con cualquier entidad).
   - `BlockPressurePlateWeighted`: placas ponderadas según peso de ítems (`EntityItem` stack count):
     - Oro (ID 147, `weightedPlate_light`): 1 nivel de señal por cada 4 entidades/ítems (hasta 64 ítems = 15).
     - Hierro (ID 148, `weightedPlate_heavy`): 1 nivel de señal por cada 43 ítems aprox (hasta 640 ítems = 15).

5. **Sensor de Luz Diurna (`BlockDaylightDetector` + `TileEntityDaylightDetector`)**:
   - ID 151 (`daylightDetector`), altura 6/16 (`renderType = 0`).
   - `TileEntityDaylightDetector` registrado en `TileEntity.cpp` como `"DLDetector"`.
   - Recálculo cada 20 ticks: evalúa `SkyBlock::Sky`, resta `skylightSubtracted`, atenúa con ángulo solar / lluvia / tormenta y actualiza el metadato del bloque entre 0 y 15.

6. **Cable Trampa y Gancho (`BlockTripWireSource` + `BlockTripWire`)**:
   - Gancho (`BlockTripWireSource`, ID 131, `renderType = 29`):
     - Soporta conexión de hasta 40 bloques de hilo.
     - Detección de entidades, estados conectado/armado/activo, sonidos de clic y desactivación silenciosa si se corta con tijeras (`ItemShears`).
   - Hilo / Cable Trampa (`BlockTripWire`, ID 132, `renderType = 30`):
     - Caja baja (0.15625 de altura), detección de paso de entidades.
   - Ítem Hilo (`Item::silk`, ID 287 / 31):
     - Instanciado con `ItemReed(31, Block::tripWire)` para permitir colocar directamente el cable trampa en el suelo.

7. **Renderers (RenderTypes 29 y 30)**:
   - Implementados en `RenderBlocks125.cpp` y declarados en `RenderBlocks.h`:
     - `renderBlockTripWireSource` (`renderType = 29`): renderizado del gancho con perno, palanca y anclaje rotados según orientación de pared.
     - `renderBlockTripWire` (`renderType = 30`): renderizado del hilo suspendido o a ras de suelo según metadato y conexiones.
   - Se empleó una estructura ligera en pila `TripWireVec3` para las rotaciones geométricas en lugar de llamar repetidamente a `Vec3D::createVectorHelper`, protegiendo el pool estático de 1024 vectores de OptiCraft contra agotamiento por frame.

8. **Recetas, Creativo, TooManyItems y Traducciones**:
   - Recetas añadidas en `CraftingManager.cpp`: botón de madera, placas de oro y hierro, sensor de luz, gancho de cable trampa.
   - Bloques añadidos a la pestaña de Redstone de `ContainerCreative.cpp`.
   - Nombres registrados en `StringTranslate.cpp` (inglés y español).

9. **Arreglo Extra Solicitado (Claude / Maceta / Metadatos)**:
   - Añadido `->setRequiresSelfNotify()` en `Block.cpp` a los 10 bloques señalados (`blockNetherQuartz`, `woodDoubleSlab`, `woodSingleSlab`, `cocoaPlant`, `enderChest`, `beacon`, `cobblestoneWall`, `flowerPot`, `skull`, `anvil`).
   - Añadido `->setRequiresSelfNotify()` a todos los bloques nuevos del encargo (`tripWireSource`, `tripWire`, `woodenButton`, `pressurePlateGold`, `pressurePlateIron`, `daylightSensor`).

---

## 2. Tabla de Bloques y Archivos

| ID | Nombre / Puntero | Clase C++ | `renderType` | Archivos de Implementación |
|---:|---|---|---:|---|
| 55 | `redstoneWire` | `BlockRedstoneWire` | `5` | `BlockRedstoneWire.h:1`, `BlockRedstoneWire.cpp:1` |
| 70 | `pressurePlateStone` | `BlockPressurePlate` | `0` | `BlockPressurePlate.h:1`, `BlockPressurePlate.cpp:1` |
| 72 | `pressurePlatePlanks` | `BlockPressurePlate` | `0` | `BlockPressurePlate.h:1`, `BlockPressurePlate.cpp:1` |
| 77 | `button` | `BlockButtonStone` | `0` | `BlockButtonStone.h:1`, `BlockButtonStone.cpp:1`, `BlockButton.cpp:1` |
| 131 | `tripWireSource` | `BlockTripWireSource` | `29` | `BlockTripWireSource.h:1`, `BlockTripWireSource.cpp:1` |
| 132 | `tripWire` | `BlockTripWire` | `30` | `BlockTripWire.h:1`, `BlockTripWire.cpp:1` |
| 143 | `woodenButton` / `buttonWood` | `BlockButtonWood` | `0` | `BlockButtonWood.h:1`, `BlockButtonWood.cpp:1`, `BlockButton.cpp:1` |
| 147 | `pressurePlateGold` | `BlockPressurePlateWeighted` | `0` | `BlockPressurePlateWeighted.h:1`, `BlockPressurePlateWeighted.cpp:1` |
| 148 | `pressurePlateIron` | `BlockPressurePlateWeighted` | `0` | `BlockPressurePlateWeighted.h:1`, `BlockPressurePlateWeighted.cpp:1` |
| 151 | `daylightSensor` / `daylightDetector` | `BlockDaylightDetector` | `0` | `BlockDaylightDetector.h:1`, `BlockDaylightDetector.cpp:1`, `TileEntityDaylightDetector.cpp:1` |
| - | `DLDetector` | `TileEntityDaylightDetector` | - | `TileEntityDaylightDetector.h:1`, `TileEntityDaylightDetector.cpp:1`, `TileEntity.cpp:78` |

---

## 3. Comprobación y Convivencia con Circuitos 1.2.5

Para asegurar que los circuitos construidos con los componentes clásicos no se rompen mientras no se porten a la arquitectura completa 1.5.2 (Grupo 2 partes B y C), se verificaron las siguientes garantías lógicas:

1. **Antorcha - Cable - Lámpara**:
   - `BlockRedstoneTorch` en 1.2.5 implementa `isPoweringTo` e `isIndirectlyPoweringTo` devolviendo `bool`.
   - Al heredar de `Block::isProvidingWeakPower` / `isProvidingStrongPower`, estos retornan `15` cuando la antorcha está encendida.
   - `BlockRedstoneWire` en `calculateCurrentChanges` llama a `world->getStrongestIndirectPower(...)`, recibiendo `15` de la antorcha.
   - El cable propaga la señal atenuándose de 15 a 0.
   - `BlockRedstoneLight` verifica `world->isBlockGettingPowered(x, y, z)`, que retorna `true` al ser la potencia recibida `> 0`. La lámpara se enciende correctamente.
2. **Repetidor con Retardo**:
   - `BlockRedstoneRepeater` en 1.2.5 implementa la detección de señal y emisión booleana.
   - Al ser consultado por `World::isBlockProvidingPowerTo`, delega en `isProvidingStrongPower` / `isProvidingWeakPower` que devuelve `15` si está activo.
   - `BlockRedstoneWire` identifica repetidores con `isPoweredOrRepeater` y toma la señal de nivel 15 sin degradación.
3. **Pistón con Palanca**:
   - `BlockLever` emite señal clásica booleana. Al activarse, los métodos de consulta analógica retornan `15`.
   - `BlockPistonBase` comprueba `isPowered(World, x, y, z, direction)` llamando a `isBlockGettingPowered` / `isBlockIndirectlyGettingPowered`. Al retornar `> 0`, el pistón se extiende.
4. **Puerta con Botón (Piedra y Madera)**:
   - Tanto `BlockButtonStone` como `BlockButtonWood` extienden `BlockButton` y emiten potencia `15` cuando el bit `0x8` del metadato está activo.
   - `BlockDoor` recibe la señal mediante `isBlockGettingPowered` y cambia el estado de la puerta. El botón de madera mantiene la puerta abierta durante 30 ticks (1.5s) y el de piedra durante 20 ticks (1s).

---

## 4. Estado de Compilación

Ambas configuraciones de Xbox fueron compiladas respetando el candado `BUILD.lock`:

1. **`xbox-release`**:
   - `ninja -C build\xbox-release`: **Exit code 0**.
   - Parcheo SSE2 -> SSE1: 215 instrucciones parcheadas.
   - Generación de ISO con `extract-xiso`: `OptiCraft.iso` (5 archivos, 71,441,787 bytes).
2. **`xbox-release-exp`**:
   - `ninja -C build\xbox-release-exp`: **Exit code 0**.
   - Parcheo SSE2 -> SSE1: 216 instrucciones parcheadas.
   - Generación de ISO con `extract-xiso`: `OptiCraft.iso` (5 archivos, 71,478,651 bytes).

---

## 5. Salida de `git diff --stat`

```
 src/net/minecraft/src/Block.cpp              |  71 +++--
 src/net/minecraft/src/Block.h                |  10 +
 src/net/minecraft/src/BlockButton.cpp        | 329 ++++++++++++-------
 src/net/minecraft/src/BlockButton.h          |  53 ++--
 src/net/minecraft/src/BlockPressurePlate.cpp | 174 ++--------
 src/net/minecraft/src/BlockPressurePlate.h   |  33 +-
 src/net/minecraft/src/BlockRedstoneWire.cpp  | 231 +++++++-------
 src/net/minecraft/src/BlockRedstoneWire.h    |   5 +-
 src/net/minecraft/src/ContainerCreative.cpp  |  10 +-
 src/net/minecraft/src/CraftingManager.cpp    |   7 +-
 src/net/minecraft/src/Direction.cpp          |   4 +
 src/net/minecraft/src/Direction.h            |   4 +
 src/net/minecraft/src/Item.cpp               |   2 +-
 src/net/minecraft/src/RenderBlocks.cpp       |   8 +
 src/net/minecraft/src/RenderBlocks.h         |   2 +
 src/net/minecraft/src/RenderBlocks125.cpp    | 453 +++++++++++++++++++++++++++
 src/net/minecraft/src/StringTranslate.cpp    |   5 +
 src/net/minecraft/src/TileEntity.cpp         |   4 +-
 src/net/minecraft/src/World.cpp              |  75 +++--
 src/net/minecraft/src/World.h                |   6 +-
 20 files changed, 1014 insertions(+), 472 deletions(-)
```
