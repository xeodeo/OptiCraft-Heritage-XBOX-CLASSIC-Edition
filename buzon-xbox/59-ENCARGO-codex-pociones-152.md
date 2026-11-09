# Encargo 59 (Codex): pociones de la 1.5.2

## Tu sitio (NUEVO)
- Worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-pociones`, rama `port152-pociones`,
  creada desde `153f374` (rama buena `port152-claude`). Trabaja SOLO ahí.
- `wt-152-codex` ya NO es tuyo: ahora es la rama de Claude. No lo toques.
- agy trabaja a la vez en `wt-152-agy` (yunque, faro, marco, libros; luego menú creativo y cofre trampa).
  Para no chocar: no toques `GuiContainerCreative`, `ContainerCreative`, `CreativeTabs`, `BlockChest`,
  `ContainerRepair`, `TileEntityBeacon`, ni la lista de pestañas creativas de `Block.cpp`/`Item.cpp`.
- Lee primero `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`.
- Java 1.5.2: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\` (1.2.5 en src125).
  Lista de cambios: `mcp-diff\cambiadas` (líneas distintas por clase).

## Qué portar (traducción fiel del Java 1.5.2, método a método)
Clases y nº de líneas que cambian entre 1.2.5 y 1.5.2:
`Potion` (51), `PotionHelper` (100), `PotionEffect` (60), `ItemPotion` (149), `TileEntityBrewingStand` (50),
`ContainerBrewingStand` (73), `EntityPotion` (60), `ItemAppleGold` (26), `RenderLiving` (177, solo la parte de
pociones/invisibilidad), `EntityRenderer` (501, solo la parte de visión nocturna y ceguera).
Nuevas: `DispenserBehaviorPotion`, `DispenserBehaviorPotionProjectile` (si el dispensador de 1.5.2 ya está portado;
si no, dilo).
1. **Visión nocturna** usable: zanahoria dorada en el soporte (`goldenCarrotEffect`), receta de la zanahoria dorada,
   y el efecto en pantalla (brillo de `EntityRenderer`, parpadeo al final).
2. **Invisibilidad** usable: ojo de araña fermentado sobre visión nocturna. Mobs y jugador invisibles en
   `RenderLiving` (y armadura/objeto en mano como en Java).
3. `PotionHelper` 1.5.2 completo (fórmulas, `potionRequirements`/`potionAmplifiers`, colores, nombres y prefijos),
   `ItemPotion` (tooltips, `getSubItems` con las variantes de 1.5.2, pociones arrojadizas), `PotionEffect`
   (`isAmbient`, NBT `Ambient`), `Potion` (lo que cambie: wither, `isUsable`, etc.).
4. Soporte de pociones: `TileEntityBrewingStand` y `ContainerBrewingStand` de 1.5.2 (comparador incluido si el
   Java lo tiene), y que funcione con el mando como ya funciona.
5. Manzana dorada encantada (`ItemAppleGold` 1.5.2: efectos y brillo).

## Cómo te reviso
`comparar.sh` y además LEO los métodos principales contra el Java (`PotionHelper`, `ItemPotion::onEaten`,
`TileEntityBrewingStand::brewPotions`, el render de invisibilidad). En el encargo 53 hubo métodos de relleno sin
uso para inflar líneas: eso es rechazo directo. Si algo no se puede, escríbelo con el motivo.

## Reglas
- Candado `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock` (créalo con tu nombre, bórralo al acabar).
- Worktree nuevo: configura las dos carpetas la primera vez (desde Git Bash, comillas obligatorias):
  `cmake --preset xbox-release "-DXBOX_VERSION_LABEL=1.5.2"` y `cmake --preset xbox-release-exp "-DXBOX_VERSION_LABEL=1.5.2"`,
  luego `cmake --build build/<preset> --target OptiCraft`. El primer build es largo.
- Nunca /fp:fast. No push. No copies a ClassicXboxLoader. No uses xemu.

## Al acabar
Informe `buzon-xbox\59-INFORME-codex-pociones-152.md` (tabla clase Java -> archivo:línea, qué se probó, `git diff --stat`),
commit, y en tu terminal: `ENCARGO 59 LISTO <commit>`
