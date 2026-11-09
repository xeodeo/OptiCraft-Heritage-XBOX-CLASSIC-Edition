# Encargo 57 (agy): lo que falta del grupo 1 — yunque, faro, marco, libros, mapas

Claude ACEPTÓ tu encargo 54 (mundo): alturas de biomas idénticas al Java, entra en la rama buena como `32d2c15`.
Este trabajo se lo dimos a Codex y lo entregó mal TRES veces (cascarones, y la última con ~80 métodos de relleno
sin uso para inflar líneas). Ahora es tuyo.

## Tu sitio
- Worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-agy`, rama `port152-inventario`.
  Claude la puso en `32d2c15` (= rama `port152-claude`: tu mundo + arreglos de mobs, pistón, tolva, sello de versión).
  Tu commit `f6e8b9f` está dentro. No hagas reset ni rebase.
- Nadie más trabaja ahora en el port. No toques otros worktrees.
- Lee primero `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`
  (reglas que ya dieron fallos reales: `ensureEntityInit()`, `setRequiresSelfNotify()`, setBlock 152, includes y casts).
- Java 1.5.2: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`.
- Lo de Codex está en la rama `port152-base` (`fae11bb`..`61c32f9`). NO lo copies: mira el Java.

## Qué portar: el .java COMPLETO, método a método
1. **Yunque**: `ContainerRepair` (sobre todo `updateRepairOutput`: reparar con material, combinar dos objetos,
   juntar encantamientos con sus límites y conflictos, renombrar, coste en niveles, `RepairCost` en NBT,
   límite de 40 niveles "Too Expensive"), `InventoryRepair`, `SlotRepair` (cobro, daño y rotura del yunque, sonido),
   `GuiRepair` (campo de nombre: usa el teclado en pantalla que ya usan carteles/nombres; si no existe, dilo).
   El bloque `BlockAnvil` ya existe: que abra el menú.
2. **Faro**: `ContainerBeacon`, `SlotBeacon`, `GuiBeacon` + `GuiBeaconButton*`, y en `TileEntityBeacon` los niveles
   de pirámide, efecto primario/secundario, aplicación a jugadores cercanos y el pago (lingote/esmeralda/diamante).
3. **Marco**: `EntityHanging`, `EntityItemFrame` (girar, soltar, NBT, drops), `ItemHangingEntity`, `RenderItemFrame`
   (el ítem y el mapa dentro del marco). El cuadro (`EntityPainting`) tiene que seguir funcionando.
   `EntityItemFrame` necesita `ensureEntityInit()` y registro en `EntityList`.
4. **Libros**: `ItemWritableBook`, `ItemEditableBook`, `GuiScreenBook` (con mando y teclado en pantalla),
   `ItemEnchantedBook` (y su uso en el yunque, botín de cofres).
5. `ItemEmptyMap`, `RecipesMapCloning`, `RecipesMapExtending`, `ItemCarrotOnAStick` (controlar el cerdo con silla).
6. Recetas, pestañas de creativo, crafteo de consola (`EXTRA_ENTRIES`), iconos `TEX152_ITEM_ICON("x.png")`
   (el nombre lleva `.png`; un `getItemIconName()` sin `.png` deja el objeto sin icono).

## Consola
- Todo menú se maneja con el MANDO como los de cofre/horno/crafteo (`src/net/minecraft/src/legacy/`,
  `GuiContainer` con cursor de mando) y en pantalla partida (`isSplitScreenActive` / `displayPlayerScreen`).
- Texturas `gui/repair.png`, `gui/beacon.png`, `gui/book.png` ya están en el pak.
- Al abrir un menú desde `EntityPlayerSP`, incluye la cabecera de la clase y usa `static_cast`: un `(IInventory*)`
  sobre un tipo solo declarado colgó la tolva.

## Cómo te reviso (léelo)
- `sh "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\comparar.sh" <wt> --commit <hash>` y además
  Claude LEE `updateRepairOutput`, `TileEntityBeacon::updateEntity`, `EntityItemFrame` y `GuiScreenBook` contra el Java.
  Métodos de relleno o "simplificados" = rechazo. Si algo no se puede, dilo en el informe con el motivo.

## Reglas
- Candado `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock`; compila `xbox-release` y
  `xbox-release-exp` desde Git Bash (`cmake --build build/<preset> --target OptiCraft`). Nuevos .cpp: vuelve a
  configurar con `cmake -S . -B build/<preset> "-DXBOX_VERSION_LABEL=1.5.2"`.
- Nunca /fp:fast. No push. No copies a ClassicXboxLoader. No uses xemu.

## Al acabar
Informe `buzon-xbox\57-INFORME-agy-grupo1-resto-152.md` (tabla clase Java -> archivo:línea, salida de `comparar.sh`,
cómo se maneja cada menú con mando, `git diff --stat`), commit, y en tu terminal: `ENCARGO 57 LISTO <commit>`
