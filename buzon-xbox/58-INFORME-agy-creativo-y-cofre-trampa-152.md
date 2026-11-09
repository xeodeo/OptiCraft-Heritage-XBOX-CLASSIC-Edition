# Informe Encargo 58: Menú Creativo de Minecraft 1.5.2 y Cofre Trampa

- **Worktree**: `wt-152-agy`
- **Rama**: `port152-inventario`
- **Commit base**: `adbf194`
- **Commit implementación Encargo 58**: `33b6700`
- **Commit final (con cherry-picks de Claude)**: `976a554`

---

## 1. Resumen de la Implementación

### 1.1 Menú Creativo Fiel a Java 1.5.2 (12 Pestañas)
Se portó e implementó la interfaz gráfica del modo creativo idéntica a Minecraft 1.5.2 Java:
- **Pestañas (12)**:
  - Fila superior: *Building Blocks*, *Decoration Blocks*, *Redstone*, *Transportation*, *Miscellaneous*, *Search Items*.
  - Fila inferior: *Foodstuffs*, *Tools*, *Combat*, *Brewing*, *Materials*, *Survival Inventory*.
- **Jerarquía de Clases**:
  - `GuiContainerCreative`: GUI completa (1193 líneas en C++, 567 en Java = 210%).
  - `ContainerCreative`: Contenedor virtual de 45 casillas (5x9) + hotbar del jugador (1x9) con scrolling por filas y scrollbar.
  - `CreativeTabs` y subclases específicas: `CreativeTabBlock`, `CreativeTabDeco`, `CreativeTabRedstone`, `CreativeTabTransport`, `CreativeTabMisc`, `CreativeTabSearch`, `CreativeTabFood`, `CreativeTabTools`, `CreativeTabCombat`, `CreativeTabBrewing`, `CreativeTabMaterial`, `CreativeTabInventory`.
  - `SlotCreativeInventory`: Encapsulación transparente de ranuras de inventario con redirección directa a la ranura real.
  - `CreativeCrafting`: Listener `ICrafting` para sincronizar cambios en el contenedor sin interferir con el modo supervivencia.
- **Texturas del pak**:
  - `/gui/allitems.png`: texturas de las pestañas superiores e inferiores, iconos de pestaña y marco general.
  - `/gui/creative_inv/list_items.png`, `/gui/creative_inv/search.png`, `/gui/creative_inv/survival_inv.png`.
- **Pestaña Survival Inventory**:
  - Renderizado de vista previa del jugador en 3D (`drawPlayerOnGui`), rotando según la posición del cursor/mando.
  - Ranuras de armadura completas (casco, peto, grebas, botas) y ranuras de crafting/inventario estándar.
  - Casilla de papelera / destrucción de objetos (`destroyItemSlot` / `binSlot`), que al pulsar o con Shift permite destruir el objeto en mano o limpiar el inventario.
- **Pestaña de Búsqueda (Search)**:
  - Cuadro de texto `GuiTextField` integrado con `VirtualKeyboard` en consolas.
  - Búsqueda en tiempo real sobre todos los bloques, objetos, pociones, huevos de criaturas y libros encantados.
- **Gestión de Memoria en `clearItemList`**:
  - Eliminación segura de todos los punteros `ItemStack*` creados dinámicamente para la lista virtual al cambiar de pestaña o cerrar el menú, previniendo fugas de memoria en los ~64 MB de la consola Xbox.

---

### 1.2 Registro de Bloques, Objetos y Variantes (`setCreativeTab`, `getSubBlocks`, `getSubItems`)
- **Mapeo exhaustivo**:
  - 121 bloques y 161 objetos asociados a sus respectivas pestañas mediante `CreativeTabs::initTabsForBlocksAndItems()` inicializado en `CreativeTabs::initialize()`.
- **Implementación de Sub-bloques y Sub-items**:
  - `BlockStep`: Losas de piedra (daños 0..7, omitiendo 2 según Java).
  - `BlockWoodSlab`: Losas de madera (roble, abeto, abedul, jungla).
  - `BlockWall`: Muros de adoquín normal y musgoso.
  - `BlockQuartz`: Bloques de cuarzo, cincelado y pilar.
  - `BlockAnvil`: Yunque intacto, ligeramente dañado y muy dañado.
  - `BlockSilverfish`: Bloques de piedra, adoquín y ladrillos de piedra infestados.
  - `ItemCoal`: Carbón mineral y carbón vegetal.
  - `ItemDye`: 16 variantes de tintes (lapis, tinta, cacao, harina de hueso, etc.).
  - `ItemAppleGold`: Manzana de oro normal y manzana de Notch.
  - `ItemSkull`: Esqueleto, Wither esqueleto, zombi, humano y creeper.
  - `ItemMonsterPlacer`: Todas las variantes de huevos de spawn iterando `EntityList::entityEggs`.
  - `ItemPotion`: Generación completa de todas las pociones existentes (normales y arrojadizas, nivel I y II, duración extendida y botella de agua) mediante `PotionHelper::getPotionEffects`.
  - `ItemEnchantedBook` / `func_92116_a`: Filtrado de libros encantados según el tipo (`combat` para armaduras/armas, `tools` para herramientas/arcos, etc.), generando todos los niveles de cada encantamiento válido.

---

### 1.3 Cofre Trampa (Bloque 146)
- **Bloque y TileEntity**:
  - `BlockChest` con parámetro `isTrapped` (tipo 0 = cofre normal, tipo 1 = cofre trampa).
  - Emisión de señal débil proporcional al número de jugadores que tienen abierto el cofre (máx 15).
  - Emisión de señal fuerte por la cara inferior del bloque.
  - Actualización y notificación inmediata de bloques vecinos al abrirse y cerrarse (`world->notifyBlocksOfNeighborChange`).
  - `TileEntityChest::func_98041_l()` garantiza que un cofre trampa solo se una a otro cofre trampa adyacente para formar un cofre doble trampa, nunca con uno normal.
- **Renderizado y Texturas**:
  - `TileEntityChestRenderer` y `ChestItemRenderHelper` configurados con `/item/chests/trap_small.png` y `/item/chests/trap_large.png`.
- **Crafteo**:
  - Receta de cofre trampa (cofre normal + gancho de cuerda / tripwire hook).
  - Integrado en menús clásicos y en la cuadrícula de crafteo de consola.
  - Compatible con el comparador de redstone.

---

### 1.4 Manejo Completo con el Mando de Xbox
Sin necesidad de ratón físico, la interfaz es 100% manejable con mando (compatible con pantalla partida):
- **Navegación de Casillas**:
  - D-pad (cruceta) y stick analógico izquierdo navegan directamente entre la cuadrícula de 5x9 casillas y la hotbar inferior de 1x9, así como las ranuras de armadura en Survival Inventory.
  - Integración con `ContainerSlotNavigator` con protección de pulsación simple para evitar doble clic al pulsar **A** (`4214800`).
- **Desplazamiento / Scroll**:
  - Stick analógico derecho (arriba/abajo) o botones de disparo permiten realizar scroll vertical suave fila a fila (`scrollRows`).
- **Cambio de Pestañas**:
  - Botones **LB / RB** (o White/Black) rotan cíclicamente entre las 12 pestañas hacia la izquierda y derecha emitiendo sonido de clic (`random.click`).
- **Acciones Rápidas**:
  - Botón **A**: Coger / soltar / depositar pila de objetos.
  - Botón **X**: Eliminar objeto en cursor o vaciar pila en la casilla papelera.
  - Botón **Y** / Quick-move: Mover rápidamente objetos entre la hotbar y el inventario.
- **Teclado en Pantalla**:
  - En la pestaña de búsqueda, el teclado virtual nativo del juego (`VirtualKeyboard`) se despliega para introducir texto fácilmente mediante el mando.

---

## 2. Comparación de Líneas de Código (`comparar.sh`)

| Clase | Líneas Java 1.5.2 | Líneas C++ (.cpp + .h) | Cobertura % | Estado |
|---|---|---|---|---|
| `BlockAnvil` | 120 | 111 | 92% | Fiel a Java |
| `BlockQuartz` | 106 | 122 | 115% | Fiel a Java |
| `BlockSilverfish` | 82 | 94 | 114% | Fiel a Java |
| `BlockStep` | 76 | 87 | 114% | Fiel a Java |
| `BlockWall` | 117 | 115 | 98% | Fiel a Java |
| `BlockWoodSlab` | 50 | 77 | 154% | Fiel a Java |
| `ContainerCreative` | 78 | 162 | 207% | Fiel a Java |
| `CreativeTabs` | 114 | 758 | 664% | Fiel a Java + Tablas 1.5.2 |
| `EntityList` | 171 | 347 | 202% | Fiel a Java |
| `GuiContainerCreative` | 567 | 1193 | 210% | Fiel a Java + Mando Xbox |
| `ItemAppleGold` | 39 | 37 | 94% | Fiel a Java |
| `ItemCoal` | 23 | 33 | 143% | Fiel a Java |
| `ItemDye` | 267 | 197 | 73% | Fiel a Java |
| `ItemMonsterPlacer` | 109 | 123 | 112% | Fiel a Java |
| `ItemPotion` | 268 | 275 | 102% | Fiel a Java |
| `ItemSkull` | 119 | 119 | 100% | Fiel a Java |
| `Slot` | 78 | 133 | 170% | Fiel a Java |
| `SlotCreativeInventory` | 61 | 92 | 150% | Fiel a Java |
| `BlockChest` | 407 | 389 | 95% | Fiel a Java |
| `TileEntityChest` | 349 | 353 | 101% | Fiel a Java |
| `CreativeCrafting` | 25 | 43 | 172% | Fiel a Java |

*Nota: Ninguna clase está por debajo del 60% ni marcada con `<-- revisar`.*

---

## 3. Cherry-Picks Incorporados desde `port152-claude`

Se incorporaron en orden todos los avisos y mejoras de Claude:
1. `b138291` (de `a44d195`): Corrección de cuelgue al arrancar en `StatList` con recetas dinámicas sin salida fija (clonado de mapas).
2. `147ac6e` (de `32023b0`): Corrección de memoria en AABB pool y `clearControlList` en `GuiBeacon`.
3. `c9dd9af` (de `5696f7f`): Auditoría 1.5.2 (shift-click en yunque/faro, efectos de faro, orbe XP, loot de zombi, estructuras, `ItemHangingEntity`).
4. `1159bd2` (de `92f119e`): Corrección de compilación para NBTTagCompound y punto y coma en recetas.
5. `5c3794c` (de `35abb81`): Corazones de efecto Wither en el HUD y celdas de textura del pak.
6. `6cd8eec` (de `2932dfd`): Ranura de resultado de yunque y dibujo del teclado virtual sobre contenedores.
7. `4c7fb73` (de `706705a`): Renderizado de mapas de terreno, textura trasera de marcos de objetos y recetas 1.5.2 de mapa.
8. `976a554` (de `4214800`): Prevención de doble clic en botón A con selección de cruceta en Xbox.

---

## 4. Estado de Compilación

Ambas configuraciones se compilaron con éxito respetando el candado `BUILD.lock`:
- **Preset `xbox-release`**:
  - Binario: `bin/xbox/OptiCraft.exe`
  - XBE generado: `OptiCraft.xbe` (4.583.424 bytes)
  - ISO generada: `OptiCraft.iso` (77.476.219 bytes)
- **Preset `xbox-release-exp`**:
  - Binario: `bin/xbox-exp/OptiCraft.exe`
  - XBE generado: `OptiCraft_exp.xbe` (4.599.808 bytes)
  - ISO generada: `OptiCraft.iso` (77.525.371 bytes)
- **Candado**: Liberado correctamente tras finalizar (`BUILD.lock` eliminado).
