# Encargo 67 (agy): detalles del creativo, cruceta separada, vídeo en 480p e iconos de controles

Claude ACEPTÓ el 58: tu creativo y el cofre trampa ya están en `port152-claude` (`cc5f988` + `da5b0a4`).
Al usuario le encantó («el menú creativo está hermoso»). Ahora los detalles que vio en la consola.

## Tu sitio
- `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-agy`, rama `port152-inventario`. Claude ya la puso en
  `port152-claude` (reset hecho, árbol limpio). No hagas reset otra vez.
- NO toques `wt-152-codex`, `wt-152-idiomas` (otro agy con los idiomas: no toques `StringTranslate`, `.lang`),
  ni `GuiRepair`/`ContainerRepair`/`EntityItemFrame`/`RenderItemFrame`/`ItemMap` (los lleva Claude).
- Guía: `buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`. Java: `mcp-diff\src152\net\minecraft\src\`.

## 1. Creativo (lo que vio el usuario)
a. Pestaña de abajo «Inventario de supervivencia»: el icono del cofre se ve SIN la tapa (se ve el cuerpo, la tapa no).
   Mira cómo se dibujan los iconos de pestaña (`renderItemIntoGUI` 3D del cofre vía `ChestItemRenderHelper`, zLevel,
   recorte/profundidad con la textura de la pestaña encima). Igual que Java `GuiContainerCreative.renderCreativeTab`.
b. Salen cosas que en Java NO están en el creativo, con icono equivocado. Ejemplo: en Decoración una «zanahoria» con
   el icono del brote = el bloque de cultivo 141. En Java `BlockCrops`, `BlockStem`, `BlockNetherStalk`, `BlockCarrot`,
   `BlockPotato`, `BlockCocoa`... ponen `setCreativeTab(null)` en su constructor o no tienen pestaña. Revisa TODA la tabla
   `CreativeTabs::initTabsForBlocksAndItems()` bloque a bloque contra el CONSTRUCTOR de cada clase Java (y sus padres:
   `BlockFlower` pone tabDecorations, pero `BlockCrops` lo anula). Que no sobre ni falte nada.
c. El marco del portal del End (bloque 120) se ve plano (una cara) en el inventario; en Java es 3D. Mira
   `RenderBlocks.renderBlockAsItem` / `renderItemIn3d` para su renderType (26) y haz que se dibuje en 3D como en Java.
   Revisa de paso otros bloques con renderType especial que salgan planos en el creativo (lista en el informe).
d. Buscador: al entrar en la pestaña de búsqueda el teclado en pantalla NO se abre solo (en Java el campo tiene el foco
   al entrar: ábrelo directamente). Si cierras el teclado con B y luego pulsas otra vez en la barra de búsqueda, ya no se
   abre. Arréglalo (foco del `GuiTextField` + `VirtualKeyboard`), también en pantalla partida.

## 2. Cruceta separada del joystick (Xbox, PS2, Wii y que valga para PS3 luego)
Ahora en juego la cruceta mueve al jugador igual que el stick (`MovementInputFromOptions.cpp` ~105,
`GameSettingsBackend_PS2.cpp` 39-50 asigna las flechas a andar). Haz como en la Legacy de consola:
- Stick izquierdo = andar. La cruceta YA NO anda.
- Cruceta en juego: ARRIBA = cambiar cámara (1ª/3ª persona, `thirdPersonView`), ABAJO = abrir el chat (para los
  comandos cuando existan), IZQUIERDA/DERECHA = cambiar de objeto en la barra (como LB/RB). Mira en
  `minecraft\` (Legacy LCE, recursos permitidos, sin tocar esa carpeta) qué hace cada flecha y si difiere, dilo.
- DENTRO de menús/inventarios la cruceta sigue moviendo por las casillas EXACTAMENTE como ahora (el usuario lo
  pidió: «que no afecte el inventario»). No toques `ContainerSlotNavigator` salvo que haga falta.
- Pantalla partida: cada jugador con su mando.
- Que los controles se puedan ver en Opciones > Controles con sus iconos (punto 4).

## 3. Vídeo en 480p
En Opciones > Ajustes de vídeo (y el resto de pantallas de opciones Legacy) a 640x480 no se ve completo: la lista se sale.
Que quede centrado y entero en 480p. Si hace falta, QUITA el logo de Minecraft en esas pantallas cuando la altura sea
480 (en 720p que siga igual). Revisa todas las pantallas de opciones/controles a 480p y 720p.

## 4. Controles: iconos desde el principio
En Opciones > Controles salen «Botón 1», «Botón 2», «A», «RT», «LT» como texto; si reasignas una acción, entonces sí se
ve el icono del botón. Que se vean los ICONOS desde el inicio (mismo dibujo que tras reasignar).

## Reglas
Compila `xbox-release` y `xbox-release-exp` con el candado `buzon-xbox\BUILD.lock` (este worktree ya está configurado;
tras cambiar ramas re-ejecuta el configure: `"$C" -S . -B build/<p> "-DXBOX_VERSION_LABEL=1.5.2"` desde Git Bash).
Si tocas PS2/Wii, que compile también su preset si tienes tiempo, si no dilo. Nunca /fp:fast, no push, no
ClassicXboxLoader, no xemu. Nada de relleno. Commits pequeños por punto (1a, 1b, ... 4).

## Al acabar
Informe `buzon-xbox\67-INFORME-agy-detalles.md` (qué cambió en cada punto y cómo se maneja ahora la cruceta),
y en tu terminal: `ENCARGO 67 LISTO <commit>`.
- 03-11 17:00: `port152-claude` tiene ya los idiomas del encargo 65 y diagnósticos temporales. Antes de compilar:
  `git cherry-pick 3b71bea c78f54a 23350b7 41ab79b` (si choca en StringTranslate/ItemStack, quédate con port152-claude).
