# 98. Encargo agy: traer lo útil del fork ramide1 (1.2.5 y 1.5.2) (21-11)

Como el 87 (erluza), pero con otro fork de bolha.dev: https://bolha.dev/ramide1/OptiCraftHeritage (rama `main`,
descargada en el repo como `ramide1/main`:
`git fetch https://bolha.dev/ramide1/OptiCraftHeritage.git main:refs/remotes/ramide1/main`).
Tiene 46 commits que NO están ni en erluza ni en nuestras ramas. Ramas: 1.2.5 `opti30-erluza` (`wt-opti30`) y
1.5.2 `port152-erluza` (`wt-152-rend`). Sin reset, sin push, sin copiar al lanzador; commits con `git add <fichero>`.
Reglas de siempre (BUILD.lock, 60 fps, memoria, creativo NO, terreno igual, nunca /fp:fast, dos interfaces).

## Qué traer (revisa cada uno; solo lo que sirva a Xbox o arregle fallos comunes)
Candidatos claros:
- `b2ca555` el arco carga, dispara y gasta flechas como Java 1.2.5 (comprueba si nuestro arco falla).
- `672b3da` liberar jugadores en diferido al cambiar de dimensión (posible crash).
- `fce6e6c` arreglos de multijugador (texto de carteles diferido, poda de entidades, radio de render de jugadores).
- `6d5d743` arreglo de display lists de PC (emparejar bases, no conteos): mira si aplica a nuestras listas de Xbox.
- `e8eb6a9` opción «Pistas en pantalla» (si no la tenemos ya).
NO traer: 3DS, CI, Forgejo, Linux, explorador de ficheros, FPS con fondo (decide el usuario), cambios de generación.

Lista completa (opti30-erluza..ramide1/main):
- `87d65fe5` 2026-10-29 feat(ui): the FPS counter gets a background and the position joins it
- `48e51064` 2026-10-29 feat(ui): the text fields answer Ctrl+A/C/V/X and the mouse drag
- `cb495956` 2026-10-29 feat(ui): an in-game file browser picks texture packs, mods and worlds
- `b2ca555e` 2026-10-28 fix(gameplay): the bow charges, fires and consumes the stack like vanilla 1.2.5
- `e8eb6a94` 2026-10-28 feat(options): an "On-Screen Hints" switch for every platform
- `ffd6f9bd` 2026-10-28 fix(3ds): the world was empty because the eye bind wiped it (spec 020)
- `96a87059` 2026-10-28 audio(3ds): keep the wave nodes where the DSP expects them (spec 034)
- `d4175803` 2026-10-28 i18n: the official language file decides the text (spec 035)
- `95b3c631` 2026-10-28 ci: free the build tree before the packaging, the disk is the scarce resource
- `0642d536` 2026-10-28 ci: key the compiler cache on the commit and make the release path debuggable
- `154a38bf` 2026-10-28 ci: bound the build parallelism and make the concurrency group work
- `4b198131` 2026-10-28 audio(3ds): never cut a sound that is playing (spec 034)
- `065046ef` 2026-10-28 ci: do not install a glibc CMake in the Alpine containers
- `4d983fa4` 2026-10-28 ci: bound the release-dispatch job with a timeout
- `29a1fae6` 2026-10-28 ci: stop the platform packages from swallowing each other
- `1b2d27ba` 2026-10-28 ci: make the compiler cache actually work on PS2, PSP and 3DS
- `375d618e` 2026-10-28 ci: add the Windows Zen 4 target and tighten the final release steps
- `fe077ce5` 2026-10-27 i18n: drop the unreachable 1.8-era Spanish name table
- `92a164f9` 2026-10-27 ci: bring back the Windows x86 and CPU-variant builds
- `6f7bb2f2` 2026-10-27 fix(cmake): resolve OPTICRAFT_OUTPUT_DIR against the source tree
- `2c9bb038` 2026-10-27 ci: fix the Linux variants configure step and add an ARM64 cross build
- `f5bbaf7b` 2026-10-27 ci: let the Drive remote be rooted at a single folder
- `0b8baef8` 2026-10-27 ci: publish the builds to Google Drive with rclone
- `d9a8e677` 2026-10-27 fix(ps2): silence the console mbedTLS and gsKit DMA redefinition warnings
- `c60d1925` 2026-10-27 fix(i18n): use the tuteo forms the rest of the Spanish UI uses
- `ab88449e` 2026-10-27 feat(diagnostics): port the OOM report and the OOM test trigger to PC and 3DS
- `c3375299` 2026-10-27 feat: port upstream .ochworld import, console skin selector and Wii prompt atlas
- `6b266092` 2026-10-27 Optimize Forgejo artifact uploads
- `d753d7b2` 2026-10-22 remove agents
- `08926095` 2026-10-22 fix(3ds): enable the LCD's stereoscopic mode when the right-eye target is armed
- `51a529fc` 2026-10-22 fix: stop the 3DS multiplayer black screen (resident columns, wire height order)
- `062d2eff` 2026-10-21 Merge branch 'feature/port-upstream-2026-10-21'
- `159db7c6` 2026-10-21 feat(wii): ship the install guide and the drop-folder READMEs
- `6426408c` 2026-10-21 feat(wii): finish the Wii port bits
- `05c99c47` 2026-10-21 feat(ps2): OOM diagnostics and a test trigger for the error screen
- `672b3dab` 2026-10-21 fix: defer player frees during dimension transitions
- `faf9eb01` 2026-10-21 Use Forgejo's patched artifact actions, and drop the ARM64 job
- `7b08946b` 2026-10-21 Run every CI job in its own container: ubuntu:26.04 and the SDK images
- `a0bd43b2` 2026-10-21 Rework the Forgejo workflows: ubuntu-22.04 everywhere, Windows crossed, no macOS
- `8825a23d` 2026-10-21 Index the texture pack zip: direct seeks instead of a scan per read
- `ddfb9514` 2026-10-21 Take the upstream legacy menu sizing: compact column, secondary entries under Help & Options
- `fce6e6cf` 2026-10-21 Take the upstream multiplayer fixes: deferred sign text, PS2 entity prune, player render radius
- `6d5d7439` 2026-10-21 Take the upstream PC display-list fix: match pair bases, not counts
- `cc120540` 2026-10-21 Port the Mbed TLS usage to the 4.2 submodule, in every build shape
- `8d62b05f` 2026-10-21 Move pak source art to resources/ and convert CI workflows to Forgejo
- `3dd4168f` 2026-10-21 initial commit

## Aceptación
Informe con cada commit: traído / no aplica / ya lo teníamos, y por qué. Probado en xemu en las dos versiones;
60 fps igual. `buzon-xbox\98-INFORME-agy.md`. Señal: `ENCARGO 98 LISTO` + commits.
