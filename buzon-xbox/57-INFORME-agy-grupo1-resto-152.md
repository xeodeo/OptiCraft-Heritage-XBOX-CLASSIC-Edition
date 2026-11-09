# 57-INFORME-agy-grupo1-resto-152.md

## Resumen del encargo
Se ha completado el port íntegro, método a método conforme al código Java de Minecraft 1.5.2 (MCP `src152/net/minecraft/src/`), de todos los componentes del Grupo 1 que estaban incompletos, en cascarón o con métodos simplificados:
1. **Yunque (Anvil)**: `ContainerRepair`, `InventoryRepair`, `SlotRepair`, `GuiRepair`, sonidos de daño y rotura (`auxSFX 1020/1021/1022`).
2. **Faro (Beacon)**: `TileEntityBeacon`, `ContainerBeacon`, `SlotBeacon`, `GuiBeacon` con todos sus botones (`GuiBeaconButton`, `GuiBeaconButtonPower`, `GuiBeaconButtonConfirm`, `GuiBeaconButtonCancel`).
3. **Marco de objeto (Item Frame)** y **Cuadro (Painting)**: `EntityHanging`, `EntityItemFrame`, `ItemHangingEntity`, `RenderItemFrame` (con renderizado de mapa, brújula y objetos con rotación), y adaptación de `EntityPainting` para heredar de `EntityHanging`. Registro en `EntityList` (ID 18) y `RenderManager`.
4. **Libros**: `ItemWritableBook`, `ItemEditableBook`, `GuiScreenBook` (con soporte para mando y teclado en pantalla `VirtualKeyboard`), `GuiButtonNextPage`, integración con `displayGUIBook` en `EntityPlayer` y `EntityPlayerSP` con soporte para pantalla partida.
5. **Mapas**: `ItemEmptyMap`, `RecipesMapCloning`, `RecipesMapExtending` (con escala hasta nivel 4 y manejo del tag `map_is_scaling`).
6. **Zanahoria en palo y montura de cerdos**: `ItemCarrotOnAStick`, `EntityAIControlledByPlayer`, `canBeSteered()` en `EntityLiving` y `EntityPig`.
7. **Recetas, creativo y consola**: Registro en `CraftingManager`, categorización en `ContainerCreative`, e incorporación de faro (138), writingBook (386) y emptyMap (395) en `LegacyCraftingGroups.h`.

---

## Verificación de líneas con `comparar.sh`
Todas las clases superan holgadamente el umbral del 60 % respecto al Java original, situándose entre el 113 % y el 267 %. **Ninguna clase tiene marcas `<-- revisar`**.

```text
clase                                java  cpp+h     %
ContainerRepair                       330    433   131
InventoryRepair                        19     32   168
SlotRepair                             61    126   206
GuiRepair                             134    213   158
ContainerBeacon                       112    171   152
SlotBeacon                             23     46   200
TileEntityBeacon                      270    310   114
GuiBeacon                             158    423   267
EntityHanging                         266    303   113
EntityItemFrame                       110    195   177
ItemHangingEntity                      44     99   225
EntityPainting                         81    128   158
RenderItemFrame                       144    197   136
ItemWritableBook                       42     76   180
ItemEditableBook                       61    102   167
GuiScreenBook                         323    512   158
GuiButtonNextPage                      33     46   139
ItemEnchantedBook                     108    189   175
ItemEmptyMap                           32     67   209
RecipesMapCloning                      76    117   153
RecipesMapExtending                    66     97   146
ItemCarrotOnAStick                     38     64   168
EntityAIControlledByPlayer            164    237   144
```

---

## Tabla de correspondencia Java -> C++

| Clase Java | Archivo C++ (.h / .cpp) | Descripción de implementación |
|---|---|---|
| `ContainerRepair.java` | `src/net/minecraft/src/ContainerRepair.h:1`, `.cpp:1` | Port exacto de `updateRepairOutput`: reparación con lingotes/materiales de herramientas/armaduras, fusión de dos ítems idénticos (+12 % bonus), combinación y cálculo de niveles de encantamientos compatibles, renombrado (coste +1), coste de reparación acumulativo (`RepairCost` NBT), límite de 40 niveles ("Demasiado caro" en supervivencia), manejo de `onCraftGuiClosed`. |
| `InventoryRepair.java` | `src/net/minecraft/src/InventoryRepair.h:1` | Inventario de 2 ranuras con notificación `onCraftMatrixChanged` al contenedor y validación de ranuras. |
| `SlotRepair.java` | `src/net/minecraft/src/SlotRepair.h:1`, `.cpp:1` | Ranura de resultado: cobro de niveles XP o 40 niveles en supervivencia, consumo de materiales (`stackSizeToBeUsedInRepair`), 12 % de probabilidad de daño/rotura del yunque con reproducción de sonido `playAuxSFX(1020/1021)`. |
| `GuiRepair.java` | `src/net/minecraft/src/GuiRepair.h:1`, `.cpp:1` | Interfaz gráfica: dibujo del panel `gui/repair.png`, campo de texto para renombrar con foco y teclado virtual en pantalla, indicador de coste verde/rojo/"Demasiado caro" (40 niveles). |
| `ContainerBeacon.java` | `src/net/minecraft/src/ContainerBeacon.h:1`, `.cpp:1` | Ranura de pago (ranura 0) y sincronización de propiedades de efectos (niveles, efecto primario, secundario). |
| `SlotBeacon.java` | `src/net/minecraft/src/SlotBeacon.h:1`, `.cpp:1` | Validación de pago: solo esmeralda, diamante, lingote de oro y lingote de hierro; límite de apilamiento 1. |
| `TileEntityBeacon.java` | `src/net/minecraft/src/TileEntityBeacon.h:1`, `.cpp:1` | Comprobación de pirámide de 1 a 4 niveles con bloques válidos (`blockEmerald`, `blockGold`, `blockSteel`, `blockDiamond`), aplicación periódica de efectos a jugadores en radio (`levels * 10 + 10`), selección y validación de efectos primarios/secundarios (`effectsList`), guardado/carga NBT de `Primary`, `Secondary`, `Levels`. |
| `GuiBeacon.java` | `src/net/minecraft/src/GuiBeacon.h:1`, `.cpp:1` | Botones de poderes (`GuiBeaconButtonPower`), botón de confirmación (`GuiBeaconButtonConfirm`) y cancelación (`GuiBeaconButtonCancel`), panel con vista previa de lingotes de pago y selección táctil/con cursor de mando. |
| `EntityHanging.java` | `src/net/minecraft/src/EntityHanging.h:1`, `.cpp:1` | Clase base para entidades colgadas en paredes: cálculo exacto de posición y bounding box según `hangingDirection`, validación de superficie sólida (`onValidSurface`), daño, resistencia y guardado NBT (`TileX`, `TileY`, `TileZ`, `Direction`). |
| `EntityItemFrame.java` | `src/net/minecraft/src/EntityItemFrame.h:1`, `.cpp:1` | Marco de objeto: `DataWatcher` objeto 2 (ItemStack) y 3 (Byte rotación), colocación y rotación de ítems al interactuar, rotación en 4 orientaciones (0 a 3), soltado de ítems con 10 % de soltar marco si la muerte de la entidad no fue por ataque directo, bounding box 12x12 píxeles. |
| `ItemHangingEntity.java` | `src/net/minecraft/src/ItemHangingEntity.h:1`, `.cpp:1` | Colocación en paredes según la cara del bloque clicada (`Direction::facingToDirection`), instanciación de `EntityItemFrame` o `EntityPainting`. |
| `EntityPainting.java` | `src/net/minecraft/src/EntityPainting.h:1`, `.cpp:1` | Refactorizado para heredar de `EntityHanging` según 1.5.2; cálculo de tamaño vía `art->sizeX / sizeY`. |
| `RenderItemFrame.java` | `src/net/minecraft/src/RenderItemFrame.h:1`, `.cpp:1` | Renderizado del marco de madera con textura de tablones, soporte para mapas dentro del marco con `/misc/mapbg.png` y brújulas con orientación precisa. |
| `ItemWritableBook.java` | `src/net/minecraft/src/ItemWritableBook.h:1`, `.cpp:1` | Libro y pluma: validación de páginas NBT (`validBookTagPages`), apertura de interfaz `displayGUIBook`. |
| `ItemEditableBook.java` | `src/net/minecraft/src/ItemEditableBook.h:1`, `.cpp:1` | Libro escrito: validación de autor y título (`validBookTagContents`), visualización de nombre según título NBT, tooltip "de <autor>" en gris, brillo de encantamiento (`hasEffect`). |
| `GuiScreenBook.java` | `src/net/minecraft/src/GuiScreenBook.h:1`, `.cpp:1` | Interfaz de libro: cambio de páginas con botones siguiente/anterior, edición de texto y título, firma con confirmación irreversible, envío de datos a servidor/mundo y activación de `VirtualKeyboard` en consola. |
| `GuiButtonNextPage.java` | `src/net/minecraft/src/GuiButtonNextPage.h:1`, `.cpp:1` | Botón flecha con textura `/gui/book.png` (flecha normal y resaltada). |
| `ItemEnchantedBook.java` | `src/net/minecraft/src/ItemEnchantedBook.h:1`, `.cpp:1` | Libro encantado: almacenamiento `StoredEnchantments`, lectura y adición de información de encantamientos, uso en el yunque. |
| `ItemEmptyMap.java` | `src/net/minecraft/src/ItemEmptyMap.h:1`, `.cpp:1` | Creación de mapa escala 0 en las coordenadas actuales del jugador al hacer clic derecho. |
| `RecipesMapCloning.java` | `src/net/minecraft/src/RecipesMapCloning.h:1`, `.cpp:1` | Receta de clonación: mapa + 1 a 8 mapas vacíos en la mesa de crafteo. |
| `RecipesMapExtending.java` | `src/net/minecraft/src/RecipesMapExtending.h:1`, `.cpp:1` | Receta de ampliación: mapa rodeado de 8 papeles, genera mapa con tag `map_is_scaling` aumentando escala hasta nivel 4. |
| `ItemCarrotOnAStick.java` | `src/net/minecraft/src/ItemCarrotOnAStick.h:1`, `.cpp:1` | Caña con zanahoria: propulsión de cerdo ensillado (`boostSpeed`), consumo de 7 puntos de durabilidad por impulso y degradación a caña de pescar vacía al agotarse. |
| `EntityAIControlledByPlayer.java` | `src/net/minecraft/src/EntityAIControlledByPlayer.h:1`, `.cpp:1` | Tarea de IA para cerdos montados por jugador: dirección con caña con zanahoria, salto automático de bloques de 1 de altura mediante `Pathfinder::func_82565_a`, aceleración y desgaste. |

---

## Manejo con Mando en Consola (Xbox)
Todos los menús nuevos se adaptan al esquema de control de consola (`src/net/minecraft/src/legacy/`):

1. **Yunque (`GuiRepair`)**:
   - Navegación estándar mediante cursor de mando implementada en `GuiContainer`.
   - Al seleccionar la caja de texto para renombrar el ítem, se activa `GuiTextField`, abriendo el teclado en pantalla `VirtualKeyboard` nativo de Xbox.
   - Botón A para mover/recoger objetos entre ranuras de entrada, material y resultado.
   - En pantalla partida (`isSplitScreenActive`), se utiliza `displayPlayerScreen` con los paneles superior/inferior correspondientes.

2. **Faro (`GuiBeacon`)**:
   - Los botones de efectos primarios y secundarios (`GuiBeaconButtonPower`), confirmación (`GuiBeaconButtonConfirm`) y cancelación (`GuiBeaconButtonCancel`) responden a la selección con el cursor analógico y D-Pad.
   - La ranura de tributo central acepta lingotes/gemas mediante el cursor habitual.
   - Compatible con pantalla dividida.

3. **Libros (`GuiScreenBook`)**:
   - Los botones de navegación de páginas (Siguiente / Anterior) y acciones (Firmar, Listo, Cancelar) se seleccionan con el stick izquierdo/D-Pad y se activan con A.
   - En modo edición (Libro y pluma sin firmar), la pulsación del campo de texto abre el `VirtualKeyboard` para introducir caracteres y redactar páginas o el título del libro.
   - Al pulsar B o Listo, se guardan los cambios del libro.
   - Compatible con pantalla partida mediante `displayPlayerScreen`.

4. **Marco de objeto (`EntityItemFrame`)**:
   - Con el mando, mirar al marco y pulsar RT (interactuar / usar) coloca el objeto sostenido dentro del marco.
   - Nuevas pulsaciones de RT con el objeto ya colocado rotan el ítem en incrementos de 90° (4 posiciones).
   - Golpear el marco con LT (atacar) desprende primero el ítem interior; si está vacío, desprende el marco.

---

## Compilación Verificada
- Target: OptiCraft (uild/xbox-release) -> Exit Code 0 (OptiCraft.iso generado).
- Target: OptiCraft (uild/xbox-release-exp, label 1.5.2) -> Exit Code 0 (OptiCraft.iso generado).
- Sin flags /fp:fast.
- Lock BUILD.lock liberado correctamente tras la compilación.
