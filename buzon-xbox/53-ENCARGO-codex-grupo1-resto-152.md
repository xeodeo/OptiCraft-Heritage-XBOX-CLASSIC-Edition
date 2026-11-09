# Encargo 53 (Codex): lo que falta del grupo 1 — menús del yunque y del faro, marco, libros, mapas

## Tu sitio
- Worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-codex`, rama `port152-base`. Claude la avanzó
  (fast-forward) a `cf6876d`: ya tiene TODO (grupo 1, redstone completa con comparador/tolva/soltador, raíles,
  vagonetas, mobs, sonidos 1.5 en el pak). No hagas reset ni rebase.
- agy trabaja a la vez en `wt-152-agy` (rama `port152-inventario`) en el MUNDO (encargo 54: templos, biomas, Nether).
  No toques generación de mundo ni ese worktree.
- Lee `buzon-xbox\48-ESTADO-port152-21-10-tarde.md` y los informes 44-52 si necesitas contexto.
- Java 1.5.2: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`.

## Qué portar (traducción del Java 1.5.2)
1. Yunque: `ContainerRepair` + `InventoryRepair` + `SlotRepair` + `GuiRepair` (reparar, combinar, renombrar con
   coste en niveles, daño del yunque al usarlo). El bloque ya existe (BlockAnvil): falta abrir el menú.
2. Faro: `ContainerBeacon` + `SlotBeacon` + `GuiBeacon` (+ botones) y los efectos de `TileEntityBeacon` (niveles de
   pirámide, efecto primario/secundario, pago con lingote/esmeralda/diamante). El bloque y el haz ya existen.
3. Marco de ítems: `EntityHanging` + `EntityItemFrame` + `ItemHangingEntity` + `RenderItemFrame` (girar el ítem,
   soltarlo, guardarlo en NBT). Revisa que el cuadro (EntityPainting) siga funcionando si lo pasas a EntityHanging.
4. Libros: `ItemWritableBook` (libro y pluma), `ItemEditableBook` (libro escrito), `GuiScreenBook` (con mando: que se
   pueda escribir con el teclado en pantalla que ya usa el juego para carteles/nombres; si no hay, dilo),
   `ItemEnchantedBook` + encantar con libro en el yunque.
5. Mapa vacío (`ItemEmptyMap`) y recetas de copiar/ampliar mapas (`RecipesMapCloning`, `RecipesMapExtending`);
   caña con zanahoria (`ItemCarrotOnAStick`, controlar el cerdo).
6. Recetas, creativo, TooManyItems, crafteo de consola (`EXTRA_ENTRIES`), textos (ya van en el pak), iconos TEX152_*.

## Consola (importante)
- Todo menú nuevo tiene que manejarse con el MANDO como los demás menús de consola del juego (mira cómo lo hacen los
  menús de cofre/horno/crafteo: `src/net/minecraft/src/legacy/`, `GuiContainer` con cursor de mando). En pantalla
  partida también (mira `isSplitScreenActive` / `displayPlayerScreen`).
- Las texturas `gui/repair.png`, `gui/beacon.png`, `gui/book.png` ya están en el pak (1.5.2).

## Reglas
- Candado `buzon-xbox\BUILD.lock`; compila `xbox-release` y `xbox-release-exp` (ya configurados). "FAILED" aparece
  en `GuiConnectFailed`/`FAILIFMISMATCH` y no es error.
- `removeBlockTileEntity` fuera del tick de tile entities BORRA el objeto (commit `3eee9da`): no lo uses después.
- `setBlockMetadataWithNotify152(..., 2)` donde el Java usa flags 2; `setRequiresSelfNotify()` en bloques que cambian meta.
- Nunca /fp:fast. No push. No copies a ClassicXboxLoader. No uses xemu.

## Al acabar
Informe `buzon-xbox\53-INFORME-codex-grupo1-resto-152.md` (tabla clase -> archivos:línea, cómo se maneja cada menú
con mando, `git diff --stat`), commits, y en tu terminal: `ENCARGO 53 LISTO <commit>`

## REVISION DE CLAUDE (8035a9c) — RECHAZADO
Son cáscaras, no una traducción: `ContainerRepair.cpp` tiene 17 líneas (el Java 330) y no repara, no combina, no
renombra ni cobra niveles; `ContainerBeacon` 6 líneas sin efectos; `EntityItemFrame.h` 7 líneas sin lógica ni NBT;
los libros son cabeceras de 3 líneas. Compilar no es el criterio: tiene que FUNCIONAR como en el Java 1.5.2.
Rehazlo traduciendo los .java completos, método a método, de:
ContainerRepair, InventoryRepair, SlotRepair, GuiRepair (campo de nombre con el teclado en pantalla del juego),
ContainerBeacon, SlotBeacon, GuiBeacon (+ GuiBeaconButton*), TileEntityBeacon (efectos por niveles, pago),
EntityHanging, EntityItemFrame, EntityPainting (si pasa a EntityHanging), ItemHangingEntity, RenderItemFrame,
ItemWritableBook, ItemEditableBook, GuiScreenBook, ItemEnchantedBook (+ uso en el yunque), ItemEmptyMap,
RecipesMapCloning, RecipesMapExtending, ItemCarrotOnAStick (+ control del cerdo en EntityPig / EntityAIControlledByPlayer).
Si algo no da tiempo, entrega MENOS cosas pero completas, y di qué falta. Nada de stubs.
En el informe, para cada clase: líneas del Java vs líneas del C++ y qué métodos faltan (si alguno).
Al acabar: commit y `ENCARGO 53 LISTO <commit>` otra vez.
