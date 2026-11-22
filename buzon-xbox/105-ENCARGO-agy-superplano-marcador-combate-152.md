# 105. Encargo agy-1: superplano personalizable, marcador (scoreboard) y combate 1.5.2 (22-11)

Plan 69, filas 10, 11 y 12. Rama `port152-erluza` (`wt-152-rend`, último `a2c217e`). Solo 1.5.2. Sin reset, sin push,
sin copiar al lanzador; commits con `git add <fichero>`; candado `buzon-xbox\BUILD.lock` (lo comparte agy-2).
Reglas: 60 fps (medir), 64 MB (medir `freeKB`), creativo NO, terreno de las semillas normales igual, nunca /fp:fast,
dos interfaces (Java y Legacy `legacyUI`) para toda pantalla nueva, mando completo, pantalla partida.
Referencias: Java 1.5.2 `mcp-diff\src152\` (manda); LCE TU19 `lce-tu19\` (cómo lo hizo 4J en consola).

## 1. Superplano personalizable (fila 10)
`FlatGeneratorInfo` / `FlatLayerInfo` (cadena de capas `2;7,2x3,2;1;village`), `ChunkProviderFlat` 1.5.2 (bioma,
estructuras: aldeas, etc.), pantalla `GuiCreateFlatWorld` + `GuiFlatPresets` (preajustes de Java: Classic Flat,
Tunnelers' Dream, Water World, Overworld, Snowy Kingdom, Bottomless Pit, Desert, Redstone Ready) en las dos
interfaces, con mando y teclado en pantalla para la cadena. Se guarda en `level.dat` (`generatorOptions`) como Java.
Los mundos superplanos viejos cargan igual.

## 2. Marcador / scoreboard (fila 11; quedó fuera del encargo 93)
`Scoreboard`, `ScoreObjective`, `ScoreObjectiveCriteria` (dummy, deathCount, playerKillCount, totalKillCount,
health), `Score`, `ScorePlayerTeam`, `ScoreboardSaveData` (`data/scoreboard.dat`), comandos `/scoreboard objectives|players|teams`
con nuestro `ServerCommandManager` (encargo 93), y su dibujo: lateral (`sidebar`), lista de jugadores y bajo el nombre
(`belowName`), en las dos interfaces y zona segura del HUD. Equipos: prefijo/sufijo de color, fuego amigo.
Si algo de multijugador no aplica a un jugador, dilo.

## 3. Combate 1.5.2 (fila 12)
- Mensajes de muerte de 1.5.2 (`CombatTracker`, `CombatEntry`, `DamageSource.getDeathMessage`, textos `death.*` del
  `.lang`), en el chat, en español si el idioma es español.
- Flechas en llamas (arco con Fuego, flecha que atraviesa lava/fuego), y los cambios de explosiones de 1.5.2
  (`Explosion` / `EntityTNTPrimed` / creeper cargado) que no estén ya hechos (compara con Java; no rompas lo del 91).
- Comprueba también que suenen la mecha y la explosión del creeper (`random.fuse`, `random.explode`): el usuario cree
  que no suenan en consola; los ficheros están en el pak y el código los llama — si en xemu suenan, dilo.

## Aceptación
Capturas de cada pantalla (480/720p, Java y Legacy), superplano de 3 preajustes generado y recargado, marcador
lateral con 2 objetivos y equipos, 5 tipos de muerte con su mensaje, flecha en llamas. 60 fps y `freeKB`
antes/después; `nan`/`[X87]` = 0. Capturas en `buzon-xbox\105-capturas\`. Informe `buzon-xbox\105-INFORME-agy.md`;
la señal de fin es la palabra CIENTOCINCO-LISTO seguida del commit.
