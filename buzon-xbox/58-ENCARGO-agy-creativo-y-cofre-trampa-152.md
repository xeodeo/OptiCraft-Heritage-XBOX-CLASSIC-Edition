# Encargo 58 (agy): menú creativo de la 1.5.2 y cofre trampa

Empieza SOLO cuando hayas entregado el 57 (`ENCARGO 57 LISTO`). Mismo worktree `wt-152-agy`, rama
`port152-inventario`, encima de tu commit del 57. Reglas, revisión y compilación: las del 57 y la guía 55.
Claude añadió en `port152-claude` el commit `153f374` (textura del hombre-cerdo zombi): haz
`git cherry-pick 153f374` al empezar (no hagas reset ni rebase).

## 1. Menú creativo como el de Java 1.5.2 (foto del usuario: `Building Blocks` con pestañas arriba y abajo)
El nuestro (`GuiContainerCreative.cpp`, 6 pestañas) es PROVISIONAL. Hay que traducir el de la 1.5.2 como se hizo
con los menús Legacy: igual que el Java, y además manejable con el mando.
- Java: `GuiContainerCreative` (567 líneas), `ContainerCreative`, `CreativeTabs` (+ las subclases `CreativeTabBlock`,
  `CreativeTabDeco`, ... `CreativeTabInventory`, `CreativeTabSearch`), `SlotCreativeInventory`.
- Las 12 pestañas: Building Blocks, Decoration Blocks, Redstone, Transportation, Miscellaneous, Search,
  Foodstuffs, Tools, Combat, Brewing, Materials, Survival Inventory. Iconos de pestaña como en Java.
- Cada bloque y objeto con su `setCreativeTab(...)` tal como en `Block.java` e `Item.java` de 1.5.2, y
  `getSubBlocks`/`getSubItems` con sus variantes (lanas, tintes, losas, maderas, pociones, huevos de mob, libros
  encantados...). Que no falte ni sobre nada respecto al Java.
- Texturas del pak: `/gui/allitems.png` (pestañas) y `/gui/creative_inv/` + `getBackgroundImageName()` (`list_items.png`,
  `search.png`, `survival_inv.png`) — compruébalo en el script `scripts/port152/build_textures_152.py` y en el pak; si
  faltan, añádelas al script.
- Pestaña de búsqueda: con el teclado en pantalla del juego (el de los carteles). Barra de desplazamiento.
- Pestaña "Survival Inventory": armadura y la casilla de borrar (destroy item).
- MANDO: moverse por casillas con la cruceta/stick, cambiar de pestaña con LB/RB (o los botones que ya usa el
  creativo actual: mira la lógica de "White/Black latch" en el `GuiContainerCreative.cpp` de ahora y respétala),
  desplazar con el stick derecho, coger/soltar/poner pila completa. Pantalla partida incluida.
- Lo que el creativo de ahora hace bien con el mando NO se puede perder: léelo antes de reemplazarlo.

## 2. Cofre trampa (bloque 146)
- `BlockChest` de 1.5.2 con `isTrapped` (tipo 0 normal, 1 trampa): potencia débil = número de jugadores mirando
  (máx. 15), potencia fuerte por la cara de abajo, notificar vecinos al abrir/cerrar.
- `TileEntityChest::func_98041_l()` (tipo), que solo se una con cofres del mismo tipo.
- `TileEntityChestRenderer` y `ChestItemRenderHelper`: textura `/item/chests/trap_small.png` y `trap_large.png`
  (mira las rutas exactas en el Java y que estén en el pak).
- Receta (cofre + gancho de cable trampa), crafteo de consola, creativo, comparador (lee su inventario).

## Al acabar
Informe `buzon-xbox\58-INFORME-agy-creativo-y-cofre-trampa-152.md` con foto si puedes (sin xemu no: entonces
explica cómo se maneja cada cosa con el mando), commit, y en tu terminal: `ENCARGO 58 LISTO <commit>`

## TRASPASO (Claude, 26-10 tarde): agy se quedó sin cuota a mitad
- Guardado en `wt-152-agy`, commit `adbf194` (WIP, SIN compilar; 41 archivos, +906/-414). Base: `a568c76` + `ffb167f`.
- Quien siga: lee el diff de `adbf194` entero, compáralo con el Java, termina, compila y entrega como dice arriba.

## REVISIÓN DE CLAUDE (03-11) del WIP `adbf194` — sigue desde ahí, sin reset
- Cofre trampa (BlockChest, TileEntityChest, render, receta, crafteo consola): FIEL al Java. Bien.
- `CreativeTabs.cpp` `func_92116_a`: QUITA `|| t == EnumEnchantmentType::all`. En Java solo cuenta `ench->type == t`
  (y salta `ench->type == null`). Si no, Misceláneos enseña TODOS los libros.
- `getTranslatedTabLabel`: vale el respaldo en inglés solo si falta la clave en el .lang.
- FALTA (lo grande):
  1. `setCreativeTab` de CADA bloque y objeto: 13 en `Block.java`, 44 en `Item.java` y además en los CONSTRUCTORES de
     ~95 clases (`grep -l setCreativeTab mcp-diff/src152/net/minecraft/src/*.java`). Ahora no hay casi ninguno puesto:
     las pestañas saldrían vacías. Hazlo por id desde el Java, que no falte ni sobre nada.
  2. `getSubBlocks`/`getSubItems` del resto de clases con variantes (losas, escaleras no; tintes, pociones, huevos de
     mob, carbón, manzana dorada, calaveras, muros, cuarzo, piedra, arena... mira cada `getSubBlocks/getSubItems` del Java).
  3. `GuiContainerCreative` 1.5.2 entero (567 líneas Java) con el mando, como dice arriba; NO se ha empezado.
  4. Compilar `xbox-release` y `xbox-release-exp` con el candado, y el informe.
- Quién libera los `ItemStack*` de `itemList`: deja claro en `clearItemList` que no hay fugas al cambiar de pestaña
  (la consola va con ~9 MB libres).
- 03-11: la 1.5.2 se cerraba al arrancar (StatList leía la salida null de RecipesMapCloning). Arreglado en
  `port152-claude` `a44d195`: haz `git cherry-pick a44d195` en tu rama.
- 03-11: también `git cherry-pick 32023b0` (EntityItem borraba una caja del pool → memoria corrupta al cargar mundo;
  GuiBeacon borraba el botón pulsado). NUNCA `delete` de lo que devuelve `expand/getBoundingBoxFromPool/getAABB`.
- 03-11 prueba del usuario en consola: en el creativo de ahora solo sale 1 libro encantado y ninguna poción.
  En el tuyo tienen que salir TODOS: libros por pestaña (func_92116_a) y pociones (`ItemPotion::getSubItems` del Java:
  todas las variantes normales y arrojadizas) + huevos de mob. Cherry-pick también `5696f7f` y `92f119e`.
- 03-11 tarde: en `port152-claude` hay más arreglos (35abb81, 2932dfd, 706705a: corazones wither, yunque, mapa, marco).
  `git cherry-pick 35abb81 2932dfd 706705a` antes de compilar.
- 03-11: `git cherry-pick 4214800` (en Xbox, la A con la cruceta en cofres/inventario hacía doble clic). Si tu
  creativo nuevo tiene su propio `mouseClicked`, que respete lo mismo (no doble clic con la cruceta).
