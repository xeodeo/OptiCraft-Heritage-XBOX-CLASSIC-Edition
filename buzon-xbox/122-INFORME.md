# 122. Informe: mod «Modo admin (pruebas)» (Nodus 1.5.2) — subagente Claude (23-11)

## Qué es
Mod nuevo del catálogo, id `adminmode`, **apagado por defecto**, en 1.5.2 (`wt-152-choco`, rama `port152-admin`) y
1.2.5 (`wt-125-choco`, rama `opti30-admin`). Porta los módulos del cliente Nodus 1.5.2 (leídos del descompilado
`mcp-diff\nodus-152`: `xk` = estados, `zg` = lista, `zA` = lógica de tick, ganchos en clases vanilla).
Código: `src/mods/adminmode/` (`AdminModeMod`, `AdminHooks.h`, `GuiAdminMenu`). Config en `adminmode.txt`
(junto a `mods.txt`): estado y ajuste de cada módulo.

## Dónde funciona
En cualquier mundo y servidor, sin op (decidido por el usuario el 23-11). Respeta el apagado de mods (115):
con el mod apagado todos los ganchos leen `false` y se deshace lo que tocaba (vuelo, altura de paso, Timer, noClip, modo cueva).

## Menú y combo
- **Combo: Back + X** (jugador 1 y jugador 2 en pantalla partida, cada uno su menú). No choca: Back+cruceta =
  minimapa, Back+Y = aldeas, LT+RT+White+Black = X-Ray, L3+R3 = fortaleza. Con el mod activo, X no abre además la
  mesa de crafteo mientras se mantiene Back. En PC: F9. También desde el catálogo de Mods (botón de configurar).
- Menú en las dos interfaces: Legacy (panel LCE con TickBox, barra de scroll, descripción y fila de ayudas) y Java
  (lista de botones con scroll). Mando: cruceta = mover, A = activar, Izq/Der = ajuste del módulo, LB/RB = saltar 8,
  X = apagar todo, B = volver. Arriba se ve el estado (mundo local activo / servidor / mod
  apagado). En partida, lista verde de módulos activos arriba a la izquierda (como la «Enabled Mods» de Nodus;
  se oculta con F3).

## Módulos (43) y ajustes
Aimbot (objetivo), AutoEgg (x2..x8), AutoEnchant (máximo/5..30, rebaraja como Nodus), AutoFish, AutoMine, AutoSign
(OptiCraft / coordenadas / nombre), AutoTool, AutoWalk, BlockESP (diamante, oro, hierro, carbón, redstone, lapis,
esmeralda, generador, lava; barrido incremental de 33×33×33 en ~1,7 s, máx. 400 cajas), BowAimbot (objetivo, con
caída), Build (Poste/Pared/Suelo — sin el modo «Swastika»), CaveFinder (usa el X-Ray: solo piedra + FullBright),
ChestFinder (cofres y cofres de Ender), ClickAimbot, Day, Derp (aleatorio/girar, solo cabeza), Disable Nametags,
ESP (objetivo; jugadores azul, criaturas rojo), FastBreak (rápido 0,55 / instantáneo), FastPlace, Flood (2/5/10 s),
Fly (vuelo de creativo, A dos veces), Forcefield (objetivo, cada 5 ticks a 4 bloques), Freecam (atraviesa bloques
sin daño; al apagar vuelves a donde estabas), FullBright, Health (aviso a 2/3/4/6 corazones), HighJump (x1,5..x5),
NoFall, NoFlinch (sin empuje ni sacudida de cámara), NoRender (partículas y lluvia / + objetos tirados), NoSwing,
Notifications (jugadores que entran/salen de vista), Nuker (bloque elegido / rompibles al toque / todo / al picar
3×3×3; 4 bloques por tick, nunca por debajo de los pies, ni roca madre ni líquidos), NoSlowDown (comer/arco y
telaraña), Sneak, Spider, Sprint, Step (1/1,5/2/3), Timer (x0,5/1,5/2/3), Tracers (objetivo), WallHack, Water
(andar sobre el agua, agacharse para hundirse), Weather.
**No duplicados:** XRay y Waypoints de Nodus no están: son los mods X-Ray y Minimapa que ya existen.

## Ganchos en el motor (mínimos, cada uno lee un `bool`)
EntityPlayerSP (AutoWalk, NoSlowDown, Sneak, pushOutOfBlocks con noClip), Entity (Freecam sin asfixia, telaraña),
EntityLiving (HighJump en jump, NoFlinch en knockBack), EntityPlayer (NoSwing), EntityRenderer (FullBright en el
lightmap, NoFlinch en la cámara, NoRender lluvia, dibujo de ESP/Tracers/ChestFinder/BlockESP antes de la lluvia),
EffectRenderer (NoRender), RenderLiving (WallHack, nombres), RenderItem (NoRender objetos), BlockFluid + World
(Water), PlayerControllerSP (FastBreak), Minecraft::clickMouse (Build), XRayMode (modo cueva), EntityFish/GuiEditSign
(getters), Minecraft.h (friend para FastPlace/Timer), XboxInput/XboxSplitScreen (combo), ModManager (registro),
traductor 1.12.2 + NetClientHandler (nivel de op).
**Ficheros compartidos con el arreglo de Claude del cuelgue al morir** (59b1481 / 6fa1c4b): toco `World.cpp` (2
líneas en `getCollidingBoundingBoxes`), `client/Minecraft.cpp` (2 líneas tras `sendPlaceBlock`) y
`NetClientHandler.cpp` (1 línea en el constructor + include). Rebase hecho encima de esos arreglos, sin conflictos (ver Commits).

## Probado en xemu (build autotest con sonda temporal, ya quitada)
Autotest en las dos versiones (xemu, semilla 20261008, supervivencia; sonda que crea el mundo y va encendiendo
módulos; el combo lo pulsa el autopiloto del mando). Mismos resultados en 1.5.2 y 1.2.5:
- **Combo Back+X** abre el menú (detectado por la sonda) ✅. Menú **Legacy** ✅ y **Java** ✅ (capturas 01, 02, 06, 07).
- **ESP + Tracers + FullBright + BlockESP (carbón, 79 cajas) + ChestFinder** ✅ (03, 08). **Day** (noche → 1110) ✅,
  **Weather** (lluvia forzada → 0) ✅, **Step** (altura 1.0) ✅, **Fly** (permite volar; al tocar suelo deja de volar
  como en creativo, A dos veces para volver) ✅, NoFall/Sprint/Spider encendidos sin problemas.
- **Forcefield**: 3 criaturas a 4 bloques → 0 en 10 s ✅.
- **HighJump x3**: salto de 8,9 bloques (vanilla 1,25) ✅ (en una pasada de 1.5.2 salió 0 porque el jugador estaba en el
  agua al pulsar; en las otras dos, 8,9).
- **Freecam**: noClip, sin asfixia dentro del suelo; al apagar vuelve exactamente a la posición inicial ✅.
- **Nuker (Todo)**: 85 → 14 bloques alrededor en 6 s (1.5.2) y 12 → 0 (1.2.5) ✅.
- **Build (Poste)**: 6 bloques de alto ✅ (05).
- **CaveFinder + WallHack + NoRender**: solo piedra, criaturas a través de la pared ✅ (04, 09).
- Compilan las 4 builds (152/125, normal/exp); «SSE2 left in» 13–15, ninguno en código del mod.

**No probado** (necesita manos o servidor): AutoTool, AutoMine, FastBreak/FastPlace, AutoFish, AutoEgg, AutoEnchant,
AutoSign, AutoWalk, ClickAimbot/BowAimbot/Aimbot, Water, NoSlowDown, NoFlinch, NoSwing, Sneak, Derp, Health,
Notifications, Flood, Timer, Disable Nametags, pantalla partida con J2 abriendo su menú, servidor propio como op,
consola real (60 fps con ESP/BlockESP: el dibujo es un lote de líneas por categoría; BlockESP barre en ~1,7 s).
Sonda y scripts de prueba en `buzon-xbox\122-trabajo\` (fuera de las ramas).

## Assets
No hace falta parchear el `assets.pak`: no hay texturas ni cadenas nuevas (los textos están en el código, ES/EN).

## Commits
- 1.5.2 `wt-152-choco`, rama `port152-admin`: **2958e685** (el WIP 217e5394 enmendado sin la sonda, rebasado
  encima de port152-int 65669ad3, que ya trae los arreglos de Claude 59b1481/65669ad3; sin conflictos).
- 1.2.5 `wt-125-choco`, rama `opti30-admin`: **c7653bfb** (rebasado encima de opti30-erluza fc815376, con 6fa1c4b;
  sin conflictos).
- Sin push, sin lanzador. Señal: CIENTOVEINTIDOS-LISTO.

## Cambio 23-11 noche (Claude, decidido por el usuario): sin candado
La restricción de arriba ya NO existe: el modo admin funciona en cualquier mundo y servidor, sin op. Nuker y Build
también en servidor (Nuker manda inicio+fin de picar; Build pone de verdad clicando un vecino con el bloque de la
mano). Menú por pestañas (Blanco/Negro). Módulo nuevo Dupe (pestaña Objetos): Mano / Llenar pila / Inventario;
directo en mundo local, en servidor manda `/optidupe <mano|llenar|inv>` al plugin OptiDupe
(`servidor-plugins\OptiDupe`, ya en `servidor-1.12.2\plugins`, carga al reiniciar el servidor).
