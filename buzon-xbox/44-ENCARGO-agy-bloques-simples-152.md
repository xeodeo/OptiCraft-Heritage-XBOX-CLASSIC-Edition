# Encargo 44 (agy): bloques e ítems sencillos de la 1.5.2 (grupo 1, parte agy)

Claude revisó y ACEPTÓ tu inventario (encargo 41). Buen trabajo: tenías razón en lo de las alturas de los biomas.

## Lo tuyo
- Worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-agy`, rama `port152-inventario`.
  Claude la ha puesto encima de `port152-base` (commits de Codex: `setBlockNotify152`, métodos nuevos de Block).
  Tu commit de docs ahora es `ce5e050`. **No hagas reset ni rebase.**
- No toques otros worktrees. En `wt-152-codex` Codex está con las TEXTURAS (encargo 43).
- Usa tus propios docs `docs\port152\*.md` (render.md, ids.md) como guía.
- Fuente Java: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`.

## Qué portar (C++ traducido del Java 1.5.2, mismo estilo que los bloques existentes)
Bloques:
- 128 `stairsSandStone`, 134 `stairsWoodSpruce`, 135 `stairsWoodBirch`, 136 `stairsWoodJungle`, 156 `stairsNetherQuartz` (BlockStairs ya existe).
- 129 `oreEmerald`, 153 `oreNetherQuartz` (BlockOre: revisa qué suelta cada uno y la experiencia en 1.5.2).
- 133 `blockEmerald` (BlockOreStorage).
- 152 `blockRedstone` (BlockPoweredOre): da señal a todos lados. Comprueba que el cable de redstone se conecta a él
  (en 1.2.5 `BlockRedstoneWire::isPowerProviderOrWire` solo mira ids concretos; adáptalo como en 1.5.2).
- 155 `blockNetherQuartz` (BlockQuartz: normal, cincelado, pilar). El pilar en 1.5.2 usa renderType 39:
  mira render.md y dilo si hace falta un render nuevo; si sí, porta solo normal y cincelado y deja el pilar para Codex.
- 141 `carrot`, 142 `potato` (BlockCrops ya existe).
Ítems: 388 `emerald`, 391 `carrot`, 392 `potato`, 393 `bakedPotato`, 394 `poisonousPotato`, 396 `goldenCarrot`,
400 `pumpkinPie`, 405 `netherrackBrick`, 406 `netherQuartz`.
Y además, como en 1.5.2: recetas (CraftingManager / RecipesXXX), hornear (FurnaceRecipes), lo que suelta cada cultivo,
aldeanos/zombis que suelten zanahoria y patata si en 1.5.2 lo hacen, pestaña/lista del creativo (ContainerCreative) y
la lista de TooManyItems (ids.md línea ~172), y el crafteo de consola de LCE si lista recetas a mano (búscalo).

## Texturas (IMPORTANTE)
Codex está rehaciendo los atlas con TODAS las texturas de 1.5.2. Tú NO toques terrain.png/items.png ni índices de bloques viejos.
Para cada icono nuevo pon un índice provisional del atlas 1.2.5 parecido y al lado el comentario exacto
`// PORT152_ICON: <nombre del png de 1.5.2 sin .png>` (p. ej. `// PORT152_ICON: oreEmerald`). Así se cambia luego automáticamente.

## Nombres y textos
- Ojo: en 1.2.5 el DIAMANTE tiene `setItemName("emerald")` (Item.cpp:482, ids.md línea ~131). En 1.5.2 se llama "diamond".
  Que la esmeralda nueva no herede el nombre ni la traducción del diamante. Revisa `lang/en_US.lang` (y es_ES si existe en data)
  y lista en el informe las claves que faltan; no edites `OptiCraftHeritageEdition\data`.

## Reglas
- Nada de la generación del mundo todavía (las esmeraldas en Extreme Hills van en el grupo 4).
- Candado `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock` antes de compilar (REGLA-BUILD-LOCK.md).
  Compila `xbox-release` como en `buzon-xbox\build_opti30.ps1`, pero en tu worktree. Primera vez:
  `& $cm --preset xbox-release "-DXBOX_DATA_DIR=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data"`.
  Borra el candado SIEMPRE al acabar.
- Si añades `shouldSideBeRendered`/`getBlockTexture`/`colorMultiplier`, respeta las REGLAS CRÍTICAS de tu render.md
  (`usesDefaultFaceCulling`, `usesDefaultWorldTextureLookup`, `usesDefaultColorMultiplier`).
- Nunca /fp:fast. No push.

## Aceptación
- Compila `xbox-release` sin errores ni avisos nuevos (también "SSE2 left in").
- Tabla en el informe: cada bloque/ítem -> archivo:línea, receta, qué suelta, icono provisional y PORT152_ICON.
- `git diff --stat` y lista de lo que NO pudiste portar y por qué.

## Al acabar
Informe `buzon-xbox\44-INFORME-agy-bloques-simples-152.md`, commit, y en tu terminal: `BLOQUES 152 LISTO <commit>`
