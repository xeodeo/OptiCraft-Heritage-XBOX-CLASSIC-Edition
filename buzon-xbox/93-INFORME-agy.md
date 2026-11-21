# Informe Encargo 93 — Comandos, reglas del juego (`GameRules`) y modo Aventura (1.5.2)

- **Rama y commit (1.5.2)**: `port152-erluza` -> `7a3b267` (`Port 1.5.2 commands, GameRules, Adventure mode, Allow Cheats and PlayerSelector`), sobre `1da8442`.
- **1.2.5 (`wt-opti30` / `opti30-erluza`)**: sin cambios (solo 1.5.2).
- **Lanzador (`ClassicXboxLoader`)**: sin copias ni modificaciones.

---

## 1. Qué se ha portado desde Java 1.5.2 (`mcp-diff/src152`)

### 1.1 Sistema de comandos y selectores (`src/net/minecraft/src/commands/`)
- [CommandExceptions.h](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/commands/CommandExceptions.h): `CommandException`, `WrongUsageException`, `NumberInvalidException`, `PlayerNotFoundException`, `CommandNotFoundException` usando las claves oficiales `commands.*` de los 58 idiomas (y soporte añadido en [StringTranslate.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/StringTranslate.cpp#L551-L600) para especificadores `%.2f` / `%f`).
- [ICommandSender.h](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/commands/ICommandSender.h), [ICommand.h](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/commands/ICommand.h), [CommandBase.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/commands/CommandBase.cpp), [CommandHandler.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/commands/CommandHandler.cpp), [ServerCommandManager.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/commands/ServerCommandManager.cpp).
- [PlayerSelector.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/commands/PlayerSelector.cpp): parser ligero en C++ (sin `std::regex` para evitar sobrecoste en 64 MB) con `@p`, `@a`, `@r`, argumentos posicionales `[x,y,z,r]` y clave-valor `[x=,y=,z=,r=,rm=,l=,lm=,m=,c=,name=]` con negación `!`, operando sobre P1 y Jugador 2 en pantalla dividida.
- **19 comandos de un jugador registrados** (con sus alias estándar `/?`, `/w`, `/msg`; **`/effect` NO registrado**, según el encargo):
  `/gamemode`, `/gamerule`, `/time`, `/weather`, `/toggledownfall`, `/give`, `/tp`, `/kill`, `/xp`, `/enchant`, `/difficulty`, `/defaultgamemode`, `/seed`, `/me`, `/say`, `/tell`, `/clear`, `/spawnpoint`, `/help`.
- **Trucos (`Allow Cheats`)**:
  - Guardado en `level.dat` como `allowCommands` ([WorldInfo.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/WorldInfo.cpp#L16-L24), [WorldSettings.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/WorldSettings.h)). En mundos antiguos sin `allowCommands`, sigue la regla de Java 1.5.2: `allowCommands = (gameType == CREATIVE)`.
  - Opción **Allow Cheats** añadida a **ambas interfaces** al crear mundo (deshabilitada en Hardcore, activada automáticamente al elegir Creativo si no se tocó manualmente):
    - Interfaz Java: [GuiCreateWorld.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiCreateWorld.cpp#L75-L90) (además se cierra el foco del teclado en pantalla al pulsar «Más opciones del mundo» para que no tape los botones).
    - Interfaz Legacy (`legacyUI`): [LegacyCreateWorldScreen.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/legacy/LegacyCreateWorldScreen.cpp#L75-L185) (recolocación dinámica a 7 filas en «Más opciones», sin solapamientos en 480p).
  - Sin trucos activados, sólo funcionan los comandos permitidos por Java 1.5.2 (nivel 0: `/help`, `/kill`, `/me`, `/tell`, más `/seed` en un jugador); el resto devuelve `§cYou do not have permission to use this command.`.
- **Chat y autocompletado**:
  - Se conservó nuestro `GuiChat` actual (con el teclado en pantalla intacto para mando), añadiendo historial de comandos en `sentMessages`, tecla `/` en un jugador ([Minecraft.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/client/Minecraft.cpp#L2373)), envío vía `Minecraft::lineIsCommand()` y autocompletado con `Tab` en un jugador ([GuiChat.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiChat.cpp#L202-L242)).
- **Marcador (scoreboard, fila 11)**: diferido a su propio encargo para mantener acotados el tamaño de binario/RAM y el alcance de esta entrega.

### 1.2 Reglas del juego (`GameRules`)
- [GameRules.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GameRules.cpp): las 7 reglas de Java 1.5.2 persistidas como compuesto `GameRules` en `level.dat` (`WorldInfo`) y accesibles mediante `World::getGameRules()`:
  1. `doFireTick` (`true`) — respetada en [BlockFire.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/BlockFire.cpp#L82-L86).
  2. `mobGriefing` (`true`) — respetada en [Explosion.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/Explosion.cpp#L256-L267) (`isSmoking`), [World.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/World.cpp#L4348-L4356), [EntityCreeper.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityCreeper.cpp#L79-L83), [EntityFireball.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityFireball.cpp#L224-L226), [EntityEnderman.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityEnderman.cpp#L119), [EntityDragon.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityDragon.cpp#L496-L500), [BlockFarmland.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/BlockFarmland.cpp#L54-L61), [EntityAIBreakDoor.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityAIBreakDoor.cpp#L20-L28).
  3. `keepInventory` (`false`) — respetada en [EntityPlayer.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityPlayer.cpp#L497-L526) (`onDeath` y `getExperiencePoints`), [Minecraft.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/client/Minecraft.cpp#L3227-L3247) (`respawn`) y [XboxSplitScreen.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/client/XboxSplitScreen.cpp#L245-L261) (Jugador 2).
  4. `doMobSpawning` (`true`) — respetada en [World.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/World.cpp#L5175).
  5. `doMobLoot` (`true`) — respetada en [EntityLiving.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityLiving.cpp#L754-L756) (orbes XP) y `onDeath` (ítems y drops raros).
  6. `doTileDrops` (`true`) — respetada en [Block.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/Block.cpp#L653-L658) (liberando `itemstack` sin fuga).
  7. `commandBlockOutput` (`true`) — guardada/consultable y respetada en `ServerCommandManager::notifyAdmins`.

### 1.3 Modo Aventura (`EnumGameType::ADVENTURE`)
- [EnumGameType.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EnumGameType.cpp) y [PlayerCapabilities.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/PlayerCapabilities.cpp) (`allowEdit`, persistido como `mayBuild` en NBT del jugador, y `playerGameType` en [EntityPlayer.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityPlayer.cpp#L650-L686)).
- [Material.h/.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/Material.cpp#L232-L237): `isAlwaysHarvested()` activado exactamente en `glass`, `redstoneLight` e `ice` como Java 1.5.2.
- [EntityPlayer.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/EntityPlayer.cpp#L1503-L1528): `isCurrentToolAdventureModeExempt(x,y,z)` y `canPlayerEdit(...)` aplicados en [PlayerControllerSP.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/PlayerControllerSP.cpp#L62-L93) y [PlayerController.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/PlayerController.cpp#L33-L41) (protege también a Jugador 2 en pantalla partida).
- Corregido [GuiSelectWorld.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiSelectWorld.cpp#L117), [GuiWorldSlot.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/GuiWorldSlot.cpp#L64) y [LegacyTutorialWorld.cpp](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/wt-152-rend/src/net/minecraft/src/legacy/LegacyTutorialWorld.cpp#L141) para que `gameType == 2` cargue con `PlayerControllerSP` + `allowEdit = false` (antes cualquier `gameType != 0` abría Creativo) y muestre la etiqueta `Adventure Mode`.
- Igual que en Java 1.5.2, Aventura se selecciona mediante `/gamemode adventure` o `/defaultgamemode adventure` y se persiste al guardar/recargar y al reaparecer (`Minecraft::changeWorld2` y `Minecraft::respawn`).

---

## 2. Verificación en `xemu` (64 MB)

### 2.1 Resultados por comando (todos probados en `xemu` 64 MB)

| # | Comando | Pruebas ejecutadas | Resultado en `xemu` |
|---|---|---|---|
| 1 | `/help` (`/?`) | `/help`, `/help 2`, `/help time`, con y sin trucos | PASS — Paginación (7/pág), uso por comando y filtrado por permisos. |
| 2 | `/seed` | `/seed` con y sin trucos | PASS — Devuelve `Seed: 1621073931` incluso sin trucos. |
| 3 | `/gamemode` | `/gamemode creative`, `/gamemode adventure`, `/gamemode 1` sin trucos | PASS — `Set own game mode to Creative Mode` / `Adventure Mode`; bloqueado sin trucos (`isCreative=0`). |
| 4 | `/defaultgamemode` | `/defaultgamemode adventure`, `/defaultgamemode survival` | PASS — `DEFGM adv=2 surv=0` y mensaje traducido de Java. |
| 5 | `/gamerule` | `/gamerule`, `/gamerule keepInventory`, `/gamerule fakeRule true`, las 7 reglas a `true/false` | PASS — Lista las 7 reglas, error `No game rule called 'fakeRule' is available`, actualiza y persiste todas. |
| 6 | `/time` | `/time set day`, `/time add 6000`, `/time bogus` | PASS — `TIME day=0 add6000=6000`; error de uso en rojo para `/time bogus`. |
| 7 | `/weather` | `/weather rain 60`, `/weather thunder 60`, `/weather clear 600` | PASS — `WEATHER rain=1 thunder=1 clear=1`. |
| 8 | `/toggledownfall` | `/toggledownfall` | PASS — `Toggled downfall`. |
| 9 | `/difficulty` | `/difficulty hard`, `/difficulty 99`, `/difficulty normal` | PASS — `DIFF hard=3 normal=2`; error `tooBig` para `99`. |
| 10 | `/give` | `/give @p 278 1`, `/give @r 1 5`, `/give @a 264 2`, `/give @p 9999 1` | PASS — `pick=1 stone=5 diam=2` entregados con selectores `@p/@r/@a`; error `notFound` para `9999`. |
| 11 | `/enchant` | `/enchant @p 32 3`, `/enchant @p 99 1` | PASS — `Enchanting succeeded` (`ench=1` en pico de diamante); error rojo `There is no such enchantment with ID 99`. |
| 12 | `/xp` | `/xp 50 @p`, `/xp 5L @p`, `/xp -2L @p`, `/xp -5 @p` | PASS — `XP level=7 total=50`; error rojo `Usage: Cannot give player negative experience points`. |
| 13 | `/tp` | `/tp @p 10 80 10`, `/tp ~0 ~1 ~0` | PASS — `TP x=10.5 y=80.5 z=10.5 relY=81.5` con coordenadas relativas `~`. |
| 14 | `/spawnpoint` | `/spawnpoint @p 10 80 10` | PASS — `SPAWN forced=1 coords=(10,80,10)`; respetado al reaparecer tras `/kill` sin cama. |
| 15 | `/clear` | `/clear @p 1`, segundo `/clear @p 1` | PASS — `Cleared the inventory of Player, removing 5 items` (`stoneAfter=0`); error rojo `Could not clear the inventory of Player, no items to remove` al repetir. |
| 16 | `/kill` | `/kill` con `keepInventory true` | PASS — `Ouch. That looks like it hurt.` y reaparición manteniendo inventario y nivel (`pick=1 diam=2 lvl=7 pos=(10.5,80.1,10.5)`). |
| 17 | `/me` | `/me says hello` (sin trucos), `/me tests @p` (con trucos) | PASS — `* Player tests Player` (resuelve `@p` cuando hay permisos). |
| 18 | `/say` | `/say Hello @p` | PASS — `[Player] Hello Player`. |
| 19 | `/tell` (`/w`, `/msg`) | `/tell @p hi` / `/tell Player hi` | PASS — Error rojo de Java `You can't send a private message to yourself!` en 1 jugador (y enrutado entre P1/P2 en pantalla partida). |
| — | `/effect` (excluido) | `/effect @p 1` | PASS — No existe (`Unknown command. Try /help for a list of commands.`). |

### 2.2 Reglas, Aventura, UI, Tab y Persistencia
- **Sin trucos (`allowCommands=0`)**: `/gamemode 1`, `/time set 0`, `/gamerule keepInventory true` bloqueados (`afterBlocked isCreative=0 keepInv=0`).
- **UI Crear mundo**: `JAVA_MORE_OPTS cheatsFound=1 before=Allow Cheats: OFF -> Allow Cheats: ON` y `LEGACY_MORE_OPTS cheatsFound=1 before=Allow Cheats: OFF -> Allow Cheats: ON`.
- **Autocompletado Tab**: `/gam` -> 2 (`gamemode`, `gamerule`), `/gamerule keep` -> `keepInventory`, `/gamemode adv` -> `adventure`.
- **Reglas reales**:
  - `keepInventory true`: tras `/kill` + `respawn()`, `pick=1 diam=2 lvl=7 pos=(10.5,80.1,10.5) forced=1`.
  - `mobGriefing false` vs `true`: explosión de Creeper junto a pared/suelo de piedra -> `MOBGRIEF_OFF intact=1`; al activarlo -> `MOBGRIEF_ON broke=1`.
- **Modo Aventura**: `ADV wasC=1 allowEdit=0 handStone=0 handDirt=0 handGlass=1 handEdit=0 handBroke=0 pickStone=1 pickBroke=1` (no rompe piedra/tierra ni coloca bloques con mano vacía; sí rompe cristal y sí rompe piedra con pico de diamante).
- **Guardar y recargar**: `RELOAD allowCmd=1 pGameType=2 allowEdit=0 isCreative=0 keepInv=1 mobSpawn=0 mobGrief=0 fire=0 mobLoot=0 tileDrops=0 cmdOut=0 spawnForced=1 freeKB=37076`.

### 2.3 Capturas del backbuffer D3D8 (`buzon-xbox\93-capturas\`)
- [create_world_java_cheats.png](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/buzon-xbox/93-capturas/create_world_java_cheats.png): Crear mundo (UI Java) con «Allow Cheats: ON» visible sin solapamientos ni teclado encima.
- [create_world_legacy_cheats.png](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/buzon-xbox/93-capturas/create_world_legacy_cheats.png): Crear mundo (UI Legacy) con «Allow Cheats: ON» integrado en el panel Legacy.
- [commands_chat_output.png](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/buzon-xbox/93-capturas/commands_chat_output.png): Chat con teclado en pantalla y salida/errores en rojo de los comandos de Java 1.5.2.
- [mobgriefing_off_creeper.png](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/buzon-xbox/93-capturas/mobgriefing_off_creeper.png): Creeper con `mobGriefing false`, pico de diamante encantado + 2 diamantes + nivel 7 conservados tras `/kill` (`keepInventory true`).
- [adventure_mode_hud.png](file:///C:/Users/xeodeo/Desktop/Minecraft%20en%20xboox%20clasico/buzon-xbox/93-capturas/adventure_mode_hud.png): Modo Aventura rompiendo piedra con pico de diamante junto a bloques de tierra y cristal.

### 2.4 Builds limpios finales, rendimiento y memoria (sin sonda)

Sonda temporal retirada (`PROBE_HITS=0`), `git status` limpio y `BUILD.lock` liberado.

| Preset | RC | Avisos SSE2 (base=13) | XBE / tamaño / `hasProbe` | Arranque limpio `xemu` (64 MB) |
|---|---:|---:|---|---|
| `xbox-release` | 0 | 13 (0 nuevos) | `OptiCraft.xbe` — 4,907,008 B (+106 KB) — `hasProbe=False` | `BUILD 1.5.2 20-11 14:31 7a3b267`, `selftest: 0 failure(s)`, `double53=1`, `freeKB=45448` (antes `45548`), `fps=74.2`, `0 crashes`, `0 nan`, `0 [X87]` |
| `xbox-release-exp` | 0 | 13 (0 nuevos) | `default.xbe` — 4,927,488 B (+114 KB) — `hasProbe=False` | `BUILD 1.5.2 20-11 14:31 7a3b267`, `selftest: 0 failure(s)`, `double53=1`, `freeKB=45424` (antes `45540`), `fps=72.2`, `0 crashes`, `0 nan`, `0 [X87]` |

---

## 3. Qué probar en consola Xbox real

1. Crear un mundo con **Allow Cheats: OFF** (en UI Java y en UI Legacy) y comprobar que `/gamemode 1`, `/time set 0` y `/gamerule` muestran el error rojo de permisos, mientras `/help`, `/seed` y `/me` sí funcionan.
2. Crear un mundo con **Allow Cheats: ON** y abrir el chat con mando (teclado en pantalla) o teclado USB (con `/` y `Tab` para autocompletar) para probar `/gamemode`, `/gamerule`, `/time`, `/weather`, `/give`, `/tp`, `/xp`, `/enchant`, `/clear`, `/spawnpoint`, `/kill`.
3. Probar `/gamerule keepInventory true` + morir, `/gamerule mobGriefing false` frente a un Creeper, y `/gamemode adventure` (mano vacía vs pico/pala/cristal).
4. Guardar, salir al menú y volver a entrar: comprobar que el modo Aventura, las `GameRules` y `Allow Cheats` se conservan intactos.
5. En mundo Limited con pantalla partida (Jugador 2), probar `/tp Player2 @p`, `/give @a 264 1`, `/tell Player2 hola`, y que P2 respeta Aventura y `keepInventory`.
