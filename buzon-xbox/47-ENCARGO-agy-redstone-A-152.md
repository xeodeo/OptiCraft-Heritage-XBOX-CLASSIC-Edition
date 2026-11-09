# Encargo 47 (agy): grupo 2, parte A — potencia de redstone 1.5 + botones, placas, sensor de luz, cable trampa

Claude ACEPTÓ el 46 (bloques con forma). Arreglos de Claude encima: el cofre de Ender ya no se borra al cerrar
(`GuiChest`, commit "never free the player's ender chest inventory") y los idiomas 1.5.2 van en el pak.
Tu rama `port152-inventario` está rebasada sobre `port152-base` (termina en `1097f9c`). No hagas reset ni rebase.

Fuente: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\` (y src125 para comparar).
Tus docs: `wt-152-agy\docs\port152\` (cambiadas.md, nuevas.md, render.md, ids.md).

## 1. Base: potencia 0..15 (lo primero y lo más delicado)
En 1.2.5 `isPoweringTo`/`isIndirectlyPoweringTo` devuelven bool. En 1.5.2 son `isProvidingWeakPower` / `isProvidingStrongPower`
y devuelven int 0..15, y World tiene `getBlockPowerInput`, `getIndirectPowerLevelTo`, `getStrongestIndirectPower`,
`isBlockIndirectlyGettingPowered` (bool, = >0). Haz que los dos mundos convivan:
- Añade la API int de 1.5.2 en Block y World, traducida tal cual del Java.
- Por defecto, la versión int de cada bloque viejo devuelve 15 si su bool viejo es true, 0 si no, y al revés, para que
  NADA cambie en los bloques que no portes todavía (cable, antorcha, repetidor, palanca, pistón, lámpara, raíles...).
- Porta el CABLE DE REDSTONE (BlockRedstoneWire) a la lógica 1.5.2 (fuerza 0..15 con la API int). Compara línea a línea:
  la caída de 1 por bloque y los `wiresProvidePower` deben quedar como en 1.5.2.
- Comprueba con circuitos que un circuito 1.2.5 sigue funcionando igual: antorcha-cable-lámpara, repetidor con retardo,
  pistón con palanca, puerta con botón. Escribe en el informe cómo lo comprobaste.

## 2. Bloques nuevos
- `BlockButtonStone` (77) / `BlockButtonWood` (143, se activa también con flechas): separa BlockButton como en 1.5.2.
- `BlockBasePressurePlate` + `BlockPressurePlate` reescrita + `BlockPressurePlateWeighted` (147 oro: 1 por entidad hasta 15;
  148 hierro: 1 cada 10). Con potencia int.
- `BlockDaylightDetector` (151) + `TileEntityDaylightDetector` (registrar "DLDetector"): potencia según la luz del sol.
- `BlockTripWireSource` (131) + `BlockTripWire` (132) + el ítem hilo que coloca cable trampa.
- Recetas, creativo, TooManyItems y textos como en el 44/46. Iconos con `TEX152_BLOCK_ICON("...")` (ya están todos en el atlas:
  tripWire, tripWireSource, daylightDetector_side/top, ...). No toques el atlas.
- Render: cable trampa y gancho usan renderType 30 y 29 en 1.5.2; sensor de luz es un bloque bajo (renderType 0 con límites).
  Mismas REGLAS CRÍTICAS de render.md.

## NO hagas (vienen en las partes B y C)
Raíles nuevos, vagonetas, comparador, tolva, soltador, repetidor 1.5 (BlockRedstoneLogic), bloque de comandos.

## Reglas
- Candado BUILD.lock (REGLA-BUILD-LOCK.md), bórralo siempre. Compila `xbox-release` y `xbox-release-exp` (ya configurados).
- Nunca /fp:fast. No push. No copies nada a ClassicXboxLoader. No toques `wt-152-codex` ni `wt-opti30`.
- Rendimiento: la redstone se ejecuta en cada tick en la Xbox (733 MHz). No añadas búsquedas, asignaciones de memoria
  ni recorridos nuevos por tick que no estén en el Java.

## Al acabar
Informe `buzon-xbox\47-INFORME-agy-redstone-A-152.md` (tabla bloque -> archivos:línea; cómo comprobaste los circuitos viejos;
`git diff --stat`), commits, y en tu terminal: `REDSTONE A LISTO <commit>`

## ARREGLO EXTRA (Claude, lo vio el usuario en la consola) — hazlo también en este encargo
La maceta no muestra la flor al ponerla: `World::setBlockMetadataWithNotify` (World.cpp ~1553) solo redibuja si el bloque
tiene `setRequiresSelfNotify()`. En `Block.cpp` (~1137-1147) faltan en: blockNetherQuartz, woodDoubleSlab, woodSingleSlab,
cocoaPlant, enderChest, beacon, cobblestoneWall, flowerPot, skull, anvil. Añade `->setRequiresSelfNotify()` a todos ellos,
y a TODOS los bloques nuevos de este encargo (botones, placas, sensor de luz, cable trampa y gancho), porque cambian metadato.
