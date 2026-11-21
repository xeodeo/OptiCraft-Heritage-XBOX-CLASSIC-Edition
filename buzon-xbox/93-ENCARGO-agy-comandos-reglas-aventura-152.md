# 93. Encargo agy: comandos, reglas del juego y modo aventura (1.5.2) (20-11)

Primer punto pendiente del plan de la 1.5.2 (`69-PLAN-COMPLETO-152.md`, fila 2). Rama `port152-erluza`
(`wt-152-rend`, último `1da8442`). Sin reset, sin push, sin copiar al lanzador. Solo 1.5.2.
Reglas de siempre: BUILD.lock, 60 fps, memoria (64 MB), creativo NO se toca, terreno igual, dos interfaces (Java y
Legacy `legacyUI`) para toda pantalla nueva, pantalla partida (jugador 2).

## Qué portar desde Java 1.5.2 (`mcp-diff/src152`)
1. **Comandos** (`CommandHandler`, `ServerCommandManager`, `CommandBase`, `PlayerSelector` `@p @a @r`,
   `WrongUsageException`/`CommandException`), los de un jugador como mínimo:
   `/gamemode` (con aventura), `/gamerule`, `/time set|add`, `/weather`, `/toggledownfall`, `/give`, `/tp`,
   `/kill`, `/xp`, `/effect`(1.5.2 no lo tiene: NO), `/enchant`, `/difficulty`, `/defaultgamemode`, `/seed`, `/me`,
   `/say`, `/tell`, `/clear`, `/spawnpoint`, `/help`. Autocompletar con Tab si hay teclado; en mando, que el chat
   siga funcionando como ahora (teclado en pantalla). Solo con «trucos» activados en el mundo (opción de Java
   «Allow cheats» al crear mundo, guardada en level.dat; los mundos viejos: como Java).
2. **Reglas del juego** (`GameRules`: doFireTick, mobGriefing, keepInventory, doMobSpawning, doMobLoot,
   doTileDrops, commandBlockOutput), guardadas en level.dat como Java, y que el juego las respete de verdad.
3. **Modo aventura** (`EnumGameType.ADVENTURE`): no romper/poner bloques salvo con la herramienta adecuada
   (`canPlayerEdit`, `isCurrentToolAdventureModeExempt`), seleccionable en Crear mundo si Java lo deja (en 1.5.2 solo
   por comando/`defaultgamemode`) y que se guarde.
4. Chat de la 1.5.2 (`GuiNewChat`) solo si hace falta para los comandos; si nuestro chat ya sirve, no lo cambies.
5. El **marcador** (scoreboard, fila 11 del plan) va con los comandos: solo si es razonable, si no anótalo.

## Aceptación
- Cada comando probado en xemu (lista en el informe con su resultado) y los errores con el texto de Java.
- Reglas: keepInventory al morir, doMobSpawning off, mobGriefing off (creeper no rompe), guardar y cargar.
- Aventura: no se puede romper con la mano, sí con la herramienta exenta; guardar y cargar.
- Sin trucos activados, los comandos no funcionan (salvo los que Java permite).
- 60 fps sin cambios y `KB libres` igual (mide).
- Capturas en `buzon-xbox\93-capturas\`.

## Informe
`buzon-xbox\93-INFORME-agy.md`. Señal: `ENCARGO 93 LISTO` + commit final.
