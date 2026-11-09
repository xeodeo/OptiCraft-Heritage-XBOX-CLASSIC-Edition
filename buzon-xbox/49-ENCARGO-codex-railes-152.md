# Encargo 49 (Codex): grupo 2, parte B — raíles y vagonetas de la 1.5.2

Bienvenido de vuelta. Desde tu último encargo (43) han pasado muchas cosas; lee primero
`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\48-ESTADO-port152-21-10-tarde.md`.

## Tu sitio
- Worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-codex`, rama `port152-base`.
  Claude la ha avanzado (fast-forward) a `ce3de78`: ya tiene TODO lo hecho (grupo 1, redstone parte A con la API de
  potencia 0..15 en Block/World, arreglos de dibujado de la Xbox). No hagas reset ni rebase.
- agy trabaja a la vez en `wt-152-agy` (rama `port152-inventario`) en los MOBS (encargo 50). No toques ese worktree.
  Para no chocar: tú no toques EntityList salvo para registrar vagonetas, ni nada de mobs.
- Docs del port: `wt-152-agy\docs\port152\` (cambiadas.md, nuevas.md, render.md, ids.md). Java 1.5.2 en
  `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\` (1.2.5 en src125).

## Qué portar (traducción del Java 1.5.2)
1. Raíles: `BlockRailBase` + `BlockBaseRailLogic` (sustituyen a la RailLogic de 1.2.5), `BlockRail`, `BlockDetectorRail`
   y `BlockRailPowered` para el raíl propulsor (27) y el ACTIVADOR (157, nuevo). Con la API de potencia int que ya existe.
2. Vagonetas: el refactor de 1.5.2 (`EntityMinecart` base + `EntityMinecartEmpty`, `EntityMinecartChest`,
   `EntityMinecartFurnace`, `EntityMinecartTNT` nueva, `EntityMinecartMobSpawner` si es sencillo).
   La vagoneta con TOLVA no: va en la parte C con la tolva.
   - Los mundos viejos guardan "Minecart" con `Type`: cárgalos como el Java 1.5.2 (mira cómo lo resuelve
     `EntityList`/`EntityMinecart.createMinecart`). Un mundo 1.2.5 con vagonetas tiene que seguir cargando.
   - Ítems: vagoneta con TNT (407) y lo que haga falta; `DispenserBehaviorMinecart` si el dispensador ya lo admite.
   - Render de las vagonetas nuevas (TNT parpadeando, bloque dentro) como en 1.5.2 (`RenderMinecart`, `RenderTntMinecart`).
3. Iconos: `TEX152_BLOCK_ICON("x.png")` / `TEX152_ITEM_ICON("x.png")` (Texture152Atlas.h; todos los 1.5.2 están).
   Ítems nuevos con `->setIconIndex(TEX152_ITEM_ICON(...))`, nunca `setIconCoord`. No toques el atlas ni el script.
4. Bloques nuevos o que cambian metadato: `->setRequiresSelfNotify()` (si no, no se redibujan en la Xbox).
5. Cambios de metadato: donde el Java 1.5.2 usa `setBlockMetadataWithNotify(..., 2)` o `setBlock(..., 2)` (solo
   redibujar), usa `setBlockMetadataWithNotify152(..., 2)` / `setBlockNotify152(..., 2)`. La versión vieja de 4
   argumentos AVISA a los vecinos (ese error rompió el cable de redstone, ver commit `ce3de78`).
6. Recetas, creativo (ContainerCreative), TooManyItems, textos (los .lang 1.5.2 ya van en el pak), y el crafteo de consola
   (`scripts/xbox/make_crafting_groups.py` tiene `EXTRA_ENTRIES` para lo que LCE no traiga).

## Reglas
- Candado `buzon-xbox\BUILD.lock` (REGLA-BUILD-LOCK.md), bórralo siempre. Compila `xbox-release` y `xbox-release-exp`
  (ya configurados con el pak y el log). Al buscar errores en el log de compilación distingue mayúsculas: "FAILED" sale en
  `GuiConnectFailed`/`FAILIFMISMATCH` y no es error.
- Nunca /fp:fast. No push. No copies nada a ClassicXboxLoader (lo hace Claude). No uses xemu.
- Rendimiento Xbox (733 MHz, 64 MB): nada de búsquedas, `new` ni recorridos por tick que no estén en el Java.

## Aceptación
- Compila sin errores ni avisos nuevos (compara los "SSE2 left in" con los 13 conocidos).
- Informe: tabla clase -> archivos:línea; cómo se cargan las vagonetas de un mundo viejo; `git diff --stat`.

## Al acabar
Informe `buzon-xbox\49-INFORME-codex-railes-152.md`, commits, y en tu terminal: `ENCARGO 49 LISTO <commit>`

## REVISION DE CLAUDE (455d517) — aceptado como base, falta completar como en el Java 1.5.2
Claude añadió encima el arreglo del cuelgue de los pistones (`Pistons: no use of a moving piston after
clearPistonTileEntity frees it`). Tu rama ahora termina en ese commit; no hagas reset.
Lo que hiciste funciona pero es un atajo. Completa traduciendo el Java (EntityMinecartTNT.java, EntityMinecartEmpty.java,
EntityMinecart.java, RenderTntMinecart.java, BlockRailPowered.java, BlockRailBase.java):
1. Vagoneta TNT: guardar/cargar la mecha (NBT "TNTFuse"); fuerza de la explosión según la velocidad como en 1.5.2;
   se enciende con daño de fuego/flecha en llamas y al caer desde altura (fall); al romperla suelta TNT + vagoneta (o
   explota si va rápida, como el Java); `RenderTntMinecart`: parpadeo blanco y escala al final de la mecha.
2. Vagoneta normal en raíl activador encendido: expulsa al pasajero (EntityMinecartEmpty.onActivatorRailPass).
3. Comprueba que el raíl activador propaga la energía 8 raíles como el propulsor (BlockRailPowered 1.5.2) y que el
   detector sigue avisando a vecinos y raíles igual que antes. Di en el informe cómo lo comprobaste en el código.
4. El resto de vagonetas puede quedarse con la lógica actual si se comporta igual que 1.5.2; dilo donde no.
Mismas reglas. Al acabar: informe actualizado, commit y `ENCARGO 49 LISTO <commit>` otra vez.
