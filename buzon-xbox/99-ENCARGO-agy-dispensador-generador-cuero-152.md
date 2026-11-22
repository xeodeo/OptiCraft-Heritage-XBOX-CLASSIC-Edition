# 99. Encargo agy: dispensador/soltador + bloque de comandos, generador de mobs, cuero teñido (1.5.2) (22-11)

Plan 69, filas 5, 6 y 7. Rama `port152-erluza` (`wt-152-rend`, último `a2ec7d1`). Solo 1.5.2. Sin reset, sin push,
sin copiar al lanzador. Commits SOLO con `git add <fichero>`. Candado `buzon-xbox\BUILD.lock` antes de compilar o
usar xemu. Reglas: 60 fps (medir), memoria 64 MB (medir `freeKB`), creativo NO se toca (salvo que Java 1.5.2 meta el
objeto nuevo en una pestaña: entonces solo en su pestaña), terreno de la semilla igual, nunca /fp:fast, dos
interfaces (Java y Legacy `legacyUI`) para toda pantalla nueva, pantalla partida (J2), crafteo de consola incluido
(categorías como la LCE TU19 `lce-tu19\Minecraft.World\Recipes.cpp`).

Referencias: Java 1.5.2 en `mcp-diff\src152\net\minecraft\src\` (manda); la LCE TU19 en `lce-tu19\` (solo lectura)
para ver cómo 4J lo dejó en consola (pantallas `Minecraft.Client\Common\UI\IUIScene_*`, recortes de memoria).

## 1. Dispensador 1.5.2 + soltador + bloque de comandos (fila 5)
- `BlockDispenser` 1.5.2 con orientación vertical (arriba/abajo, `BlockPistonBase.determineOrientation`), y los
  comportamientos de `BlockDispenser.dispenseBehaviorRegistry` / `DispenserBehaviors`: flechas, huevos, bolas de nieve,
  pociones arrojadizas, botella de experiencia, huevos de generación, fuegos artificiales (ya básico: completar vertical),
  carga ígnea, barca, cubo lleno/vacío, vagonetas en raíles, TNT encendida, mechero, polvo de hueso, armaduras al
  jugador. `BehaviorDefaultDispenseItem` (soltar). Sonidos y partículas como Java.
- `BlockDropper` (soltador, id 158) y `TileEntityDropper`: suelta o mete en el inventario de delante (tolva incluida).
- `BlockCommandBlock` + `TileEntityCommandBlock`: ejecuta su comando con redstone usando nuestro `ServerCommandManager`
  (encargo 93), `@p/@a/@r`, regla `commandBlockOutput`. Pantalla de edición (`GuiCommandBlock`) en las dos interfaces y
  con el teclado en pantalla del mando. Solo en creativo, como Java (no se craftea, no está en las pestañas de 1.5.2).
- Guardar y cargar (NBT de los tres).

## 2. Generador de mobs 1.5.2 + vagoneta con generador (fila 6)
- `MobSpawnerBaseLogic` (1.5.2 separa la lógica: `SpawnPotentials`, `SpawnData`, `MaxNearbyEntities`,
  `RequiredPlayerRange`, `SpawnCount`, `SpawnRange`, `Min/MaxSpawnDelay`), `TileEntityMobSpawner` usando esa lógica,
  render del mob girando dentro.
- `EntityMinecartMobSpawner` (vagoneta con generador): solo por NBT/creativo como en Java.
- Que los generadores de mazmorras/fortalezas de mundos generados sigan saliendo igual (terreno igual) y que los
  mundos viejos carguen.
- **Rendimiento**: 4 generadores activos a la vez: fps y `freeKB`, sin cuelgues.

## 3. Cuero teñido + caldero con lava (fila 7)
- Armadura de cuero teñible (`ItemArmor` color NBT `display.color`, `RecipesArmorDyes`), render con color en
  inventario, mano y puesta (dos capas `leather_layer_1/2` + overlay), lavar en el caldero con agua (baja el nivel).
  Crafteo de consola: tintar armadura (como la LCE TU19 `ArmorDyeRecipe`).
- Caldero 1.5.2: lo que cambió respecto al nuestro (niveles, lavar cuero, rellenar con cubo/botella) — si «caldero con
  lava» no existe en Java 1.5.2, NO lo inventes: dilo en el informe.

## Aceptación
- Cada comportamiento del dispensador probado en xemu (lista con resultado), vertical incluido; soltador a tolva;
  bloque de comandos ejecutando `/time set 0` y `/give @p` con redstone; generador y vagoneta con generador
  funcionando; cuero teñido de 3 colores, lavado en caldero.
- Guardar y cargar todo. Pantalla partida. 60 fps y `freeKB` antes/después; `nan`/`[X87]` = 0.
- Capturas en `buzon-xbox\99-capturas\`.

## Informe
`buzon-xbox\99-INFORME-agy.md`. Al terminar, la señal es la palabra NOVENTAYNUEVE-LISTO seguida del commit.
