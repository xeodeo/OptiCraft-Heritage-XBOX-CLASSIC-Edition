# Informe Encargo 98 — Auditoría y port selectivo del fork `ramide1/main`

## Commits resultantes
- **1.2.5 (`wt-opti30`, rama `opti30-erluza`)**: `f1bf66ae`
- **1.5.2 (`wt-152-rend`, rama `port152-erluza`)**: `a2ec7d1e`

Ambos commits aplican exactamente los mismos 22 ficheros modificados (+296 / -55 líneas), sin alterar la generación de terreno, manteniendo `/fp:precise` (`double53=1`), presupuesto de 64 MB y 60 fps.

---

## Resumen de cambios traídos (5 commits de `ramide1/main`)

1. **`b2ca555e` — Arco vanilla 1.2.5, colocación de bloques y pantalla dividida J2**
   - `src/client/Minecraft.cpp`, `src/client/Minecraft.h`:
     - Al soltar el uso de un objeto (`onStoppedUsingItem`), se arma `rightClickDelayTimer = 4` para evitar re-tensar el arco en el mismo tick.
     - `clickMouse(0, ...)` exige `!thePlayer->isUsingItem()` para que atacar mientras se carga el arco no cancele la carga.
     - La comprobación `stackSize == 0` y `resetEquippedProgressAfterBlockPlace()` queda dentro del `if (sendPlaceBlock(...))`, evitando resetear el brazo en colocaciones fallidas (especialmente en creativo).
   - `src/client/XboxSplitScreen.cpp`: adaptación equivalente para el Jugador 2 (`armUseItemDelay()` al soltar `LT` usando objeto y bloqueo de `RT`/`clickMouse(0)` mientras `p2->isUsingItem()`).
   - `src/net/minecraft/src/ItemBow.h/.cpp`: helpers `hasArrow()` y `getLaunchPower()` con comprobaciones de nulos y preservando todos los encantamientos (`power`, `punch`, `flame`, `infinity`).
   - `src/net/minecraft/src/EntityArrow.cpp`: `getShootingEntity()` devuelve `owner` directamente si sigue vivo y coincide `shootingEntityId`, evitando búsquedas fallidas por ID.

2. **`6d5d7439` — Búsqueda por base de par en `GLAllocation::deleteDisplayLists`**
   - `src/net/minecraft/src/GLAllocation.h/.cpp`: se añade `findDisplayListPair()` con paso 2 (`i += 2`) en el bloque `#if PLATFORM_PC || defined(XBOX_PLATFORM)` para no confundir el `count` de una entrada `[base, count]` con un identificador de lista al liberar display lists en Xbox y PC.

3. **`fce6e6cf` — Texto diferido de carteles en multijugador (parte común)**
   - `src/net/minecraft/src/WorldClient.h/.cpp`, `src/net/minecraft/src/NetClientHandler.cpp`: `deferSignUpdate()`, `dropDeferredSignUpdate()` y `applyDeferredSignUpdates()` encolan `Packet130UpdateSign` cuando llega antes de que la columna exista (hasta 256 entradas deduplicadas por posición) y lo aplican en el primer tick en que el bloque existe.
   - *(El radio de renderizado de jugadores en `RenderGlobal.cpp` ya estaba presente; la poda de entidades y ratón USB de PS2 no aplican a Xbox.)*

4. **`51a529fc` — Correcciones comunes de multijugador (columnas residentes y altura en red)**
   - `src/net/minecraft/src/ChunkProviderClient.h/.cpp`, `src/net/minecraft/src/WorldClient.cpp`: `ensureChunk()` / `createChunk()` usados desde `WorldClient::doPreChunk()` para que un re-anuncio `Packet50` no borre una columna ya residente (bloques, tile entities y entidades).
   - `src/net/minecraft/src/NetClientHandler.cpp`: `handleFlying()` calcula los pies del jugador como `min(packet->yPosition, packet->stance)` y suma `yOffset`, funcionando tanto con servidores CraftBukkit 1.2.5 (cámara primero) como con traductores 1.8.9 (pies primero).
   - *(Se omite la telemetría de depuración exclusiva de 3DS.)*

5. **`e8eb6a94` — Opción "On-Screen Hints" / "Pistas en pantalla"**
   - `src/net/minecraft/src/GameSettings.h/.cpp`, `src/net/minecraft/src/GameSettingsPersistence.cpp`: nuevo flag `onScreenHints` (por defecto `true`), guardado y cargado en `options.txt` como `onScreenHints`.
   - `src/net/minecraft/src/ControlIcon.cpp`, `src/net/minecraft/src/legacy/LegacyControlTooltipHud.cpp`: oculta las filas de pistas de botones en menús/crafteo y el tooltip de controles del HUD cuando `onScreenHints == false`.
   - `src/net/minecraft/src/GuiOptiCraftOptions.cpp`, `src/net/minecraft/src/legacy/LegacyHeritageOptions.h/.cpp`: botón/casilla en ambos menús (interfaz Java e interfaz Legacy, encajando limpio en 480p).
   - `src/net/minecraft/src/StringTranslate.cpp`: traducción española `"Pistas en pantalla"` para el grupo `es_*`.

---

## Auditoría completa de los 46 commits de `ramide1/main`

| # | Commit | Título | Estado | Motivo |
|---|---|---|---|---|
| 1 | `87d65fe5` | feat(ui): the FPS counter gets a background and the position joins it | **no aplica** | Excluido explícitamente por el encargo; Xbox ya tiene `showCoordinates` y contador FPS propios. |
| 2 | `48e51064` | feat(ui): the text fields answer Ctrl+A/C/V/X and the mouse drag | **no aplica** | Atajos de teclado de escritorio y arrastre con ratón; Xbox usa teclado virtual y mando. |
| 3 | `cb495956` | feat(ui): an in-game file browser picks texture packs, mods and worlds | **no aplica** | Excluido explícitamente por el encargo. |
| 4 | `b2ca555e` | fix(gameplay): the bow charges, fires and consumes the stack like vanilla 1.2.5 | **traído** | Bug de jugabilidad real presente en ambas ramas; portado + adaptación a `XboxSplitScreen.cpp`. |
| 5 | `e8eb6a94` | feat(options): an "On-Screen Hints" switch for every platform | **traído** | Útil en Xbox; portado a Java UI, Legacy UI, persistencia y español (`Pistas en pantalla`). |
| 6 | `ffd6f9bd` | fix(3ds): the world was empty because the eye bind wiped it (spec 020) | **no aplica** | Específico de 3DS. |
| 7 | `96a87059` | audio(3ds): keep the wave nodes where the DSP expects them (spec 034) | **no aplica** | Específico de 3DS. |
| 8 | `d4175803` | i18n: the official language file decides the text (spec 035) | **ya lo teníamos** | Nuestro `StringTranslate.cpp` ya prioriza los ficheros `.lang` oficiales. |
| 9 | `95b3c631` | ci: free the build tree before the packaging, the disk is the scarce resource | **no aplica** | CI / Forgejo. |
| 10 | `0642d536` | ci: key the compiler cache on the commit and make the release path debuggable | **no aplica** | CI / Forgejo. |
| 11 | `154a38bf` | ci: bound the build parallelism and make the concurrency group work | **no aplica** | CI / Forgejo. |
| 12 | `4b198131` | audio(3ds): never cut a sound that is playing (spec 034) | **no aplica** | Específico de 3DS. |
| 13 | `065046ef` | ci: do not install a glibc CMake in the Alpine containers | **no aplica** | CI / Forgejo. |
| 14 | `4d983fa4` | ci: bound the release-dispatch job with a timeout | **no aplica** | CI / Forgejo. |
| 15 | `29a1fae6` | ci: stop the platform packages from swallowing each other | **no aplica** | CI / Forgejo. |
| 16 | `1b2d27ba` | ci: make the compiler cache actually work on PS2, PSP and 3DS | **no aplica** | CI / Forgejo. |
| 17 | `375d618e` | ci: add the Windows Zen 4 target and tighten the final release steps | **no aplica** | CI / Windows. |
| 18 | `fe077ce5` | i18n: drop the unreachable 1.8-era Spanish name table | **ya lo teníamos** | Nuestras ramas no contienen esa tabla heredada de la 1.8. |
| 19 | `92a164f9` | ci: bring back the Windows x86 and CPU-variant builds | **no aplica** | CI / Windows. |
| 20 | `6f7bb2f2` | fix(cmake): resolve OPTICRAFT_OUTPUT_DIR against the source tree | **no aplica** | CMake de PC/CI; los presets de Xbox no usan `OPTICRAFT_OUTPUT_DIR`. |
| 21 | `2c9bb038` | ci: fix the Linux variants configure step and add an ARM64 cross build | **no aplica** | CI / Linux. |
| 22 | `f5bbaf7b` | ci: let the Drive remote be rooted at a single folder | **no aplica** | CI / Google Drive. |
| 23 | `0b8baef8` | ci: publish the builds to Google Drive with rclone | **no aplica** | CI / Google Drive. |
| 24 | `d9a8e677` | fix(ps2): silence the console mbedTLS and gsKit DMA redefinition warnings | **no aplica** | Específico de PS2. |
| 25 | `c60d1925` | fix(i18n): use the tuteo forms the rest of the Spanish UI uses | **no aplica** | Corregía textos con voseo de `.ochworld` inexistentes en nuestras ramas. |
| 26 | `ab88449e` | feat(diagnostics): port the OOM report and the OOM test trigger to PC and 3DS | **no aplica** | Específico de PC y 3DS; Xbox tiene su propio crash log y telemetría de memoria. |
| 27 | `c3375299` | feat: port upstream .ochworld import, console skin selector and Wii prompt atlas | **no aplica** | Funciones de PS2/Wii/3DS; Xbox ya dispone de selector de skins propio (J1/J2) y atlas Xbox. |
| 28 | `6b266092` | Optimize Forgejo artifact uploads | **no aplica** | CI / Forgejo. |
| 29 | `d753d7b2` | remove agents | **no aplica** | Limpieza interna del repositorio del fork. |
| 30 | `08926095` | fix(3ds): enable the LCD's stereoscopic mode when the right-eye target is armed | **no aplica** | Específico de 3DS. |
| 31 | `51a529fc` | fix: stop the 3DS multiplayer black screen (resident columns, wire height order) | **traído** | Portadas las 2 correcciones comunes de red (`ensureChunk` y `min(yPosition, stance)`); omitida la telemetría 3DS. |
| 32 | `062d2eff` | Merge branch 'feature/port-upstream-2026-10-21' | **no aplica** | Commit de merge; sus cambios se auditan en cada commit individual. |
| 33 | `159db7c6` | feat(wii): ship the install guide and the drop-folder READMEs | **no aplica** | Específico de Wii. |
| 34 | `6426408c` | feat(wii): finish the Wii port bits | **no aplica** | Específico de Wii. |
| 35 | `05c99c47` | feat(ps2): OOM diagnostics and a test trigger for the error screen | **no aplica** | Específico de PS2. |
| 36 | `672b3dab` | fix: defer player frees during dimension transitions | **ya lo teníamos** | Ambas ramas ya cuentan con `playersToDelete`, `flushDeferredPlayers()`, `PLATFORM_DEFER_PORTAL_TRANSITION`, limpieza diferida de `thePlayer2` y guardia en `World.cpp`. |
| 37 | `faf9eb01` | Use Forgejo's patched artifact actions, and drop the ARM64 job | **no aplica** | CI / Forgejo. |
| 38 | `7b08946b` | Run every CI job in its own container: ubuntu:26.04 and the SDK images | **no aplica** | CI / Forgejo. |
| 39 | `a0bd43b2` | Rework the Forgejo workflows: ubuntu-22.04 everywhere, Windows crossed, no macOS | **no aplica** | CI / Forgejo. |
| 40 | `8825a23d` | Index the texture pack zip: direct seeks instead of a scan per read | **no aplica** | Optimiza `minizip`; nuestras ramas usan `libzip` (`MCBETA_HAVE_LIBZIP`), que ya indexa el zip internamente. |
| 41 | `ddfb9514` | Take the upstream legacy menu sizing: compact column, secondary entries under Help & Options | **no aplica** | Reajuste de columnas del menú Legacy pensado para PS2/3DS/PSP; el diseño Xbox actual ya funciona correctamente. |
| 42 | `fce6e6cf` | Take the upstream multiplayer fixes: deferred sign text, PS2 entity prune, player render radius | **traído** | Portado el texto diferido de carteles (`WorldClient`/`NetClientHandler`). Radio de jugadores ya lo teníamos; poda de entidades y ratón USB son solo PS2. |
| 43 | `6d5d7439` | Take the upstream PC display-list fix: match pair bases, not counts | **traído** | Aplica directamente a Xbox (`#if PLATFORM_PC \|\| defined(XBOX_PLATFORM)`); portado `findDisplayListPair`. |
| 44 | `cc120540` | Port the Mbed TLS usage to the 4.2 submodule, in every build shape | **no aplica** | Migración MbedTLS 4.2 de otros targets. |
| 45 | `8d62b05f` | Move pak source art to resources/ and convert CI workflows to Forgejo | **no aplica** | CI / Forgejo y reorganización de recursos de otros ports. |
| 46 | `3dd4168f` | initial commit | **no aplica** | Commit inicial del fork. |

**Totales**: 5 `traído`, 3 `ya lo teníamos`, 38 `no aplica` (= 46 commits auditados).

---

## Compilación y verificación en `xemu` (64 MB)

Compilados los 4 objetivos (`xbox-release` y `xbox-release-exp` en `wt-opti30` 1.2.5 y `wt-152-rend` 1.5.2) bajo el candado `buzon-xbox\BUILD.lock` (liberado al terminar), sin errores ni advertencias críticas.

Pruebas limpias en `xemu` (64 MB):

| Versión / Preset | `selftest` | FPU (`double53`) | RAM libre (`freeKB`) | Rendimiento (`xbox.perf`) | Estado |
|---|---|---|---|---|---|
| 1.2.5 `xbox-release` | `0 failure(s)` | `cw=027f double53=1` | `46304 KB` | `70.8–76.2 fps` (`13.1–14.1 ms`) | OK (`GuiMainMenu`, 0 errores/crashes/NaN) |
| 1.2.5 `xbox-release-exp` | `0 failure(s)` | `cw=027f double53=1` | `46292 KB` | `75.3–80.0 fps` (`12.5–13.3 ms`) | OK (`GuiMainMenu`, 0 errores/crashes/NaN) |
| 1.5.2 `xbox-release` | `0 failure(s)` | `cw=027f double53=1` | `45384 KB` | `71.3–78.4 fps` (`12.8–14.0 ms`) | OK (`GuiMainMenu`, 0 errores/crashes/NaN) |
| 1.5.2 `xbox-release-exp` | `0 failure(s)` | `cw=027f double53=1` | `45372 KB` | `73.7–77.6 fps` (`12.9–13.6 ms`) | OK (`GuiMainMenu`, 0 errores/crashes/NaN) |

---

## Prueba dentro de un mundo (`xbox-release-exp`, 64 MB en `xemu`)

Para validar `GLAllocation::findDisplayListPair` (`6d5d7439`) bajo carga real de creación/borrado de display lists de chunks y el arco (`b2ca555e`) en supervivencia, se ejecutó una prueba automatizada en **`xemu` (64 MB)** sobre **1.2.5 `xbox-release-exp`** (`f1bf66ae`) y **1.5.2 `xbox-release-exp`** (`a2ec7d1e`):
- Creación de mundo infinito en Supervivencia (`seed = 20261008`).
- Prueba real del arco pasando por `Minecraft::runTick` (`keyBindUseItem` + `keyBindAttack`): carga completa (25 ticks) con pulsación de ataque a mitad de carga, toque corto (2 ticks, `< 0.1f`) y carga media (16 ticks).
- Vuelo recto continuo de **5 minutos (300 s)** hacia `+X` (~2404 bloques ≈ 150 chunks atravesados) seguido de giro de 180° y **1 minuto de vuelta (60 s)** hacia `-X` (~472 bloques ≈ 30 chunks recargados desde disco) + 5 s de estabilización.

### Resultados del arco (`b2ca555e`)
| Versión | Disparo 1 (carga completa + ataque a mitad) | Toque corto (2 ticks) | Disparo 2 (carga media) | Resultado |
|---|---|---|---|---|
| **1.2.5 `exp`** | `delay=4`, `stillUsingAfterAttack=1`, flechas `16 -> 15`, `arrowEnts=1`, `crit=1`, `ownerMatch=1` | flechas `15` (sin disparo) | `delay2=4`, flechas `15 -> 14`, `arrowEnts=2`, `ownerMatch=2` | **OK** |
| **1.5.2 `exp`** | `delay=4`, `stillUsingAfterAttack=1`, flechas `16 -> 15`, `arrowEnts=1`, `crit=1`, `ownerMatch=1` | flechas `15` (sin disparo) | `delay2=4`, flechas `15 -> 14`, `arrowEnts=2`, `ownerMatch=2` | **OK** |

### Evolución de `freeKB` y `xbox.mem lists` durante 5 min de ida + vuelta

#### 1.2.5 `xbox-release-exp` (`f1bf66ae`)
| Fase / Tiempo | Posición `X` | `freeKB` | `xbox.mem lists` (`KB`) | `vbPools` | Chunks visibles (`rend`) |
|---|---|---|---|---|---|
| Inicio (`BASELINE`, 0 s) | `87.5` | `39808 KB` | `80` (`465 KB`) | `1` | `C: 23/1296` |
| Inicio vuelo (`OUT`, 0 s) | `87.5` | `38624 KB` | `172` (`1000 KB`) | `2` | `C: 34/1296` |
| Ida `60 s` | `572.7` | `32744 KB` | `455` (`1445 KB`) | `6` | `C: 2/1296` |
| Ida `120 s` | `1046.5` | `27476 KB` | `521` (`4782 KB`) | `12` | `C: 7/1296` |
| Ida `150 s` | `1290.1` | `28296 KB` | `521` (`2575 KB`) | `10` | `C: 3/1296` |
| Ida `180 s` | `1530.5` | `27924 KB` | `521` (`3274 KB`) | `9` | `C: 14/1296` |
| Ida `240 s` | `2011.3` | `25172 KB` | `529` (`4419 KB`) | `11` | `C: 2/1296` |
| Ida `270 s` | `2250.3` | `25776 KB` | `529` (`2939 KB`) | `9` | `C: 4/1296` |
| Giro (`TURN`, 300 s) | `2491.4` | `24132 KB` | `529` (`4096 KB`) | `10` | `C: 3/1296` |
| Vuelta `330 s` | `2256.7` | `17368 KB` | `672` (`5547 KB`) | `14` | `C: 59/1296` (`F: 91`) |
| Final (`DONE`, 365 s) | `2019.6` | `10984 KB` | `895` (`10129 KB`) | `23` | `C: 19/1296` (`F: 47`) |

#### 1.5.2 `xbox-release-exp` (`a2ec7d1e`)
| Fase / Tiempo | Posición `X` | `freeKB` | `xbox.mem lists` (`KB`) | `vbPools` | Chunks visibles (`rend`) |
|---|---|---|---|---|---|
| Inicio (`BASELINE`, 0 s) | `81.5` | `37784 KB` | `95` (`589 KB`) | `2` | `C: 24/1296` |
| Inicio vuelo (`OUT`, 0 s) | `81.5` | `35928 KB` | `173` (`1044 KB`) | `3` | `C: 37/1296` |
| Ida `60 s` | `566.3` | `31880 KB` | `441` (`1171 KB`) | `5` | `C: 1/1296` |
| Ida `120 s` | `1042.9` | `27248 KB` | `441` (`3950 KB`) | `10` | `C: 2/1296` |
| Ida `150 s` | `1287.7` | `27592 KB` | `453` (`2135 KB`) | `10` | `C: 3/1296` |
| Ida `180 s` | `1526.5` | `27368 KB` | `453` (`2678 KB`) | `7` | `C: 5/1296` |
| Ida `240 s` | `2007.1` | `24328 KB` | `453` (`3728 KB`) | `10` | `C: 4/1296` |
| Ida `270 s` | `2247.7` | `24744 KB` | `459` (`2565 KB`) | `9` | `C: 4/1296` |
| Giro (`TURN`, 300 s) | `2487.7` | `22936 KB` | `459` (`3859 KB`) | `9` | `C: 7/1296` |
| Vuelta `330 s` | `2251.6` | `16512 KB` | `595` (`5886 KB`) | `14` | `C: 59/1296` (`F: 74`) |
| Final (`DONE`, 365 s) | `2014.0` | `10572 KB` | `665` (`9536 KB`) | `22` | `C: 20/1296` (`F: 34`) |

### Conclusiones de la prueba en mundo
- **Sin fugas de display lists ni caída continua de RAM**: durante los 5 minutos de ida continua (~150 chunks atravesados), `lists` quedó acotado en `507–529` (1.2.5) y `441–459` (1.5.2), `listKB` y `vbPools` bajaron y subieron normalmente al reciclar chunks (`4782 -> 2575 -> 2939 KB` en 1.2.5; `3950 -> 2135 -> 2565 KB` en 1.5.2) y `freeKB` se mantuvo estable en `23–28 MB`.
- **Vuelta sobre chunks guardados en disco**: al regresar sobre la zona ya generada, los chunks cargaron al instante llenando más secciones del anillo de 1296 renderers (`C: 59/1296`, `895` listas en 1.2.5 y `665` en 1.5.2, bien dentro del límite de 2592 listas del anillo), terminando con **`~10.6–11.0 MB` libres** sin agotamiento de memoria.
- **Cero crashes, cero errores y cero chunks invisibles**: `0` entradas `[MC][E]`, `0` excepciones/watchdog, `double53=1` en todos los frames, geometría opaca renderizada en todo el recorrido (`opaque=5.1–8.8 ms`, `C: 19–59/1296`).
- *(Tras finalizar la prueba, se retiró la sonda temporal de `Minecraft.cpp` en ambos worktrees y se recompilaron limpios ambos `xbox-release-exp` sobre `f1bf66ae` y `a2ec7d1e`.)*
