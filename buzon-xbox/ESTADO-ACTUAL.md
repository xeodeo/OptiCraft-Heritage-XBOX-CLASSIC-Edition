# Estado actual del proyecto (23-11-2026)

## AHORA (23-11 09:15) — LEER PRIMERO
- **Ramas buenas**: 1.5.2 `port152-int` `6121e6c` (carpeta `wt-build-152`) y 1.2.5 `opti30-erluza` `a20d430`
  (`wt-opti30`; build en `wt-build-125`). Respaldadas en el remoto `xbox`.
- **En el lanzador, SIN SUBIR a la consola** (estaba apagada): las 4 XBE de cada versión + `assets.pak` parcheado con
  las texturas del chocobo. Subir por FTP (192.168.0.3, `F:\Games\OptiCraft\<versión>`) y probar.
- Nuevo desde lo último en consola: minimapa sin redibujos (114), sin tirón al guardar (111.5), chocobos reales de
  ChocoCraft 4.1.5 (113, probado en xemu), detector de mods por mundo + apagar mods de verdad (115, sin probar),
  descargar packs de Modrinth desde la consola (116, sin probar; lista de pruebas en el informe 116).
- agy sin cuota hasta ~28-11. Falta: 108 (bloques/mobs), marca de mods en la lista de mundos, luego 1.6.4.
- Lista completa: `PENDIENTES-TODO.md`. Estado de cada cosa: `ESTADO-OPTICRAFT.html`.


Resumen para empezar un chat nuevo. Contexto de fondo: `00-CONTEXTO.md`. Lo técnico del port: `docs/XBOX_PORT.md`
(en cada rama). Plan de la 1.5.2: `69-PLAN-COMPLETO-152.md`. Lo más reciente: sección «21-11» justo abajo; estado de cada cosa en `ESTADO-OPTICRAFT.html`.

## Ramas y carpetas (20-11)
| Qué | Rama | Carpeta | Último |
|---|---|---|---|
| **1.5.2 en prueba en la consola** (87-94 + Claude) | `port152-erluza` | `wt-152-rend` | `b73c465` |
| **1.2.5 en prueba en la consola** (87-91 + Claude) | `opti30-erluza` | `wt-opti30` | `d0bff75` (+ 95 de agy SIN commit) |
| 1.5.2 anterior (base de erluza) | `port152-aldeanos` | `wt-152-pociones` | `ce80a3b` |
| 1.5.2 buena (confirmada 09-11) | `port152-claude` | `wt-152-codex` | `91b5f42` (+ datos `port152-data`) |
| 1.2.5 buena / anterior | `xbox-opti-30` / `opti30-cruceta` | — | `a6a19ea` / `fd2d3a0` |
| Builds sueltas de Claude | (detached) | `wt-build-152`, `wt-build-125` | script `build88.ps1` (XDK por env) |

agy trabaja en `wt-152-rend` y `wt-opti30` (encargo 90). Cuando el usuario confirme todo: avanzar `port152-claude` a
`port152-erluza` y `xbox-opti-30` a `opti30-erluza`.
Remotos: `xbox` (del usuario, push aquí), `fork` (403), **`origin` = OptiJuegos (DMCA, no existe): nunca push**,
`erluza/main` = fork de bolha.dev descargado a mano (`git fetch https://bolha.dev/erluza/OptiCraftHeritage.git main:refs/remotes/erluza/main`).
Consola por FTP: `192.168.0.3` `xbox:xbox`, juego en `F:\Games\OptiCraft\<versión>` (solo con el dashboard abierto).

## 21-11: dónde estamos
- **Subido a la consola (21-11 12:2x, XBE de las dos + paks del 20-11 18:55):** 1.2.5 `d0bff75`, 1.5.2 `b73c465`.
  Incluye: arreglo crash al volver del End en 1.2.5 (`dc0285e`, puntero del dragón de la barra de jefe), lava roja con
  el pack LCE (fotograma estático se «calienta» 100 ticks: `fdd2911`/`0db2440`), **icono de guardado LCE** (cofre +
  flecha que baja, arriba a la derecha ~1,6 s en cada autoguardado = cada minuto; `World::lastAutosaveMillis`,
  `GuiIngame::renderSaveIcon`, PNG `gui/save_chest.png`/`save_arrow.png` en los paks y datos) y en 1.5.2 los fuegos
  artificiales (encargo 94 `3d283f8`).
- **Probado por el usuario en 1.2.5 (20-11):** End, cofres/hornos al descargar, Pocket, hielo, niebla Nether, cofre en
  mano (un poco bajo, vale), logros, chat/stick, ticks por chunk, cuelgue tras apagar, bordes 864 (-432..431) y 256
  (-128..127). Todo en la experimental; la normal solo necesita prueba corta (misma base, cambia el hilo de chunks).
- **agy (terminal `term_0ea8d336…`, cuenta con cuota nueva 21-11):** encargo 95 (1.2.5: reloj que no gira + tirones del
  dragón 100-170 ms/tick) EN MARCHA desde su árbol sin commit en `wt-opti30`. Luego **96**: personaje animado en el
  HUD al agacharse/correr/volar (LCE `Gui.cpp`), las dos versiones.
- **Repo erluza (bolha.dev) revisado 20-11** (`f8ab704`): solo novedades de versiones nuevas (comercio, bloques, X-Ray,
  lanza, luces dinámicas, menú nuevo) y cambios de generación → NO se traen. Nada nuevo de Pocket/PS2 que sirva.
- Cuidado al hacer commit en `wt-opti30`/`wt-152-rend` mientras agy trabaja: usar `git add <fichero>`, nunca `-a`.

## Builds en el lanzador (`Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\`)
- `OptiCraft\1.5.2\` y `OptiCraft\1.2.5\` (20-11): 4 XBE de cada una del 19-11 18:05 (87+88+89: `501b40d` /
  `410957a`) + `assets.pak` re-empaquetado con el splash nuevo. **Log por red activado** (`MC_LOG_LEVEL=1`, IP
  192.168.0.18). Receptor: `python -u OptiCraftHeritageEdition\xbox-spike\escuchar_log.py`. SUBIDO a la consola el 20-11.
  Respaldo de antes de erluza: `OptiCraft_respaldo_19-11_antes_erluza`.
- Paks: para añadir algo sin perder nada, desempaquetar el pak del lanzador (`scripts/unpack_pak.py <pak> <dir>`),
  cambiar y re-empaquetar (`make_pak.py <dir> <pak>`); copiar también el cambio a los datos de origen.
- `OptiCraft_actualizar\`: solo lo que cambió, con `LEEME.txt` (se copia encima de `OptiCraft\` en la Xbox).
- Compilar 1.2.5: `powershell -File buzon-xbox\build_opti30.ps1`. 1.5.2: `cmake --build build/xbox-release` y
  `build/xbox-release-exp` en la carpeta de la rama. Siempre con `BUILD.lock`.
- Pak de 1.5.2: datos en `wt-152-codex\build\xbox-release\port152-data`; `python -I scripts/make_pak.py <datos> <pak>`.
  Sonidos que Java 1.5.2 no traía: `scripts/port152/extra_sounds/` (los copia `build_textures_152.py`).

## Hecho el 06 y el 09-11 (1.5.2, rama de prueba)
- **Aldeanos** completos (informe 72): comercio Java y **Legacy** (copiado de la LCE, con `legacyUI`; oferta elegida con
  marco amarillo), aldeas del desierto, reputación, gólem que defiende, zombi curado, asedio. Auditado (9 arreglos).
- **Eventos de entidad** (`World::setEntityState` estaba vacío): rayitos/chispas/corazones de aldeanos, domar lobos y
  gatos, oveja comiendo, bruja, zombi curándose. Daño, muerte, golpe del gólem y mecha de la vagoneta TNT NO se
  reenvían (doblarían o se llamarían a sí mismos).
- **Sonidos**: aldeano (de la 1.6) y bruja (de la 1.9); Java 1.5.2 no los tenía.
- **Spawn** como Java (paseo de 1000 pasos sin generar chunks de océano): la semilla 1621073931 sale junto a la cabaña
  de bruja (informe 77).
- **Niebla en Xbox** (1.5.2 y 1.2.5): la proyección debe ser "W-friendly" ((3,4)=+1, XDK); se mandan vista y proyección
  en mano izquierda. **Cielo** como OptiFine: se dibuja según sus opciones a cualquier distancia, plano lejano ≥128
  (sol y luna), nubes con niebla más larga. Solo Xbox.
- **Rendimiento** (informes 74-76): ticks programados por chunk (`XBOX_CHUNK_TICK_SCHEDULER`), texturas animadas solo si
  cambian / solo el rectángulo / sin esperar a la GPU, fila de botones Legacy en un draw. Auditado.

## Decisiones del usuario (cumplirlas)
- El Wither se llama «Eustaquio» en español (traducción oficial de Java 1.5.2): se DEJA así (09-11).
- **Los ~60 fps estables con niebla, nubes, sol y luna (build 1.5.2 del 09-11) NO se pueden perder.** Todo cambio nuevo
  se mide en el netlog (`xbox.perf`, `xbox.spike`) contra esa referencia: mediana 59,9 fps, mínimo 55, HUD 0,2-0,4 ms.
- Toda pantalla nueva en dos interfaces (Java y Legacy por `legacyUI`); **el creativo NO se toca** (está perfecto).
- El código de **OptiFine** (descompilado en `mcp-diff\srcOF152`) y la **LCE** (`minecraft-legacy original\`) se pueden reutilizar.
- **No** se hacen objetos tirados planos ni apagar partículas de pociones.
- Agua/lava quietas = opción de animaciones apagada (no es fallo).
- Implementación larga → **encargo a agy** (skill agy); Claude revisa y audita. No crear muchos subagentes.
- Por ahora no portar cosas de la 1.5.2 a la 1.2.5 salvo arreglos de Xbox (niebla, cielo).

## Confirmado por el usuario (09-11, build 10:55)
Sin Legacy UI ~60 fps estables (netlog: mediana 59,9, mín. 55; HUD 0,2-0,4 ms; texturas animadas ~0 subidas/s).
Cielo, sol, luna, nubes y niebla; redstone 10/10 (con la build de 10:55, ANTES de los ticks del encargo 78); brújula;
rayitos de aldeano; sonidos de bruja; aldeanos; comercio; aldea del desierto; cabaña de bruja de la semilla.
Humo al domar: va con la opción «humo animado» (apagada) → se queda así.

Confirmado también con la build 11:26: redstone/agua/lava con los ticks nuevos, guardar y salir con redstone,
mundo viejo conserva la hora, creativo sin bloques, corazones al domar. Con Legacy UI: mediana 52-59 fps (sin Legacy
59,9), pero el HUD gasta lo mismo (0,3-0,5 ms): la bajada parece de lo que hacía (redstone, construir), no del menú.
Prueba quieta hecha con la build 11:43 (ver abajo).

## Por probar en consola (build 1.5.2 del 09-11 11:43)
(Confirmado 09-11 con la build 11:43: reloj, collar del lobo, agachado+bloque sobre cofre, comercio. Fps con Legacy UI
quieto: mediana 59,8-59,9, mínimo 57-59, HUD 0,1-0,4 ms → el menú Legacy YA NO quita fps. Moviéndose 56-59 por chunks.)
1. Corazones de aldeanos al aparearse (el usuario lo prueba después).
2. **Botón «Avanzado»** en Gráficos (menú Legacy): secciones, páginas, cambiar opciones y que se guarden; 480p y 720p.
3. **Skins estilo LCE** (con Legacy UI): carrusel 3D, cambiar paquete, elegir, jugador 2; sin Legacy UI la pantalla de antes.
4. **Wither** (build 12:3x): invocar con T de arena de almas + 3 calaveras de esqueleto wither; barra de jefe (y la del
   dragón), explosión al nacer (tirón: medir en netlog), calaveras azules, escudo bajo media vida, estrella del Nether,
   guardar/cargar con el Wither vivo. Informe 82; auditado (sin cuelgues).

## Build 1.5.2 del 09-11 13:33 — CONFIRMADO: skins LCE, crear mundo, logros, «Minecraft 1.5.2». Wither en prueba.
- Skins LCE sin rayas y pestañas sin repetir; Crear mundo (sin Legacy UI) sin botones montados; Logros recortados al
  panel y desplazamiento hasta el final; en el menú sin Legacy UI pone «Minecraft 1.5.2».

## En marcha (09-11 15:30): encargo 84 (Claude, agy sin cuota)
Agacharse con interruptor (opción en Controles, Legacy y Java) y mantener A en inventarios (coge DE UNO EN UNO, rápido, mientras se mantiene;
creativo y cofres). En 1.5.2 (`port152-controles-84`) y 1.2.5 (`opti30-cruceta`). Informe 84.

## Build del 09-11 16:26 (1.5.2 y 1.2.5, con assets.pak NUEVO en las dos): probar
- Skins de Halo (40), Mass Effect (36) y Skyrim (40) como pestañas en el selector (Legacy y Java), J1 y J2.
- Mantener A coge de 1 en 1 desde el principio (creativo y cofres; contar objetos: ni duplica ni pierde).
- Agacharse con interruptor (Controles). Carpeta `skins` junto al juego para skins propias.
- Paks anteriores en `OptiCraft_respaldo_paks_09-11`. agy: encargo 86 (portales) hecho pero APARCADO.

## Pendiente de probar en 1.2.5 (el usuario la deja para DESPUÉS; rama `opti30-cruceta`, build 09-11 12:10)
Niebla, cielo, sol, luna, nubes; agachado+bloque sobre un cofre; cruceta separada del stick y configurable (Controles),
iconos del stick, chat (B sale); texturas animadas y HUD Legacy más rápidos; ticks por chunk (sin prioridades: 1.2.5 no
las tiene); logros (recorte y desplazamiento, build 13:33); yunque/huevo de dragón cayendo; agacharse
con interruptor y mantener A (cuando entre el 84). Si va bien: avanzar `xbox-opti-30` a `opti30-cruceta`.

## En marcha (19-11): encargo 87 a agy, traer lo útil de erluza
Fork de OptiJuegos en https://bolha.dev/erluza/OptiCraftHeritage (descargado como `erluza/main`). El usuario quiere sus
arreglos y optimizaciones (PS2, etc.) en 1.2.5 y 1.5.2, **sin 1.8.9 de momento**. Ramas `port152-erluza` (wt-152-rend) y
`opti30-erluza` (wt-opti30, que estaba en `opti30-cruceta`). Encargo `87-ENCARGO-agy-traer-arreglos-erluza.md`; agy
en la terminal de Orca «agy erluza 87». **HECHO** (informe 87): 18 commits por rama, 1.5.2 `1eef258`, 1.2.5 `e728cf6`.
Recompilado por Claude y copiado al lanzador el 19-11 13:05 (8 XBE; respaldo en `OptiCraft_respaldo_19-11_antes_erluza`).
PROBAR en consola: vuelta del End al Overworld (antes caía en 100,50,0), hojas Fancy (ahora sin caras hoja-hoja),
entidades tapadas que no desaparezcan mal, Hielo opaco y Pocket (OFF por defecto) en las dos interfaces, TNT en cadena.
Si va bien: avanzar `port152-aldeanos`/`port152-claude` y `opti30-cruceta`/`xbox-opti-30`.
Sin traer (decide el usuario): panorama nocturno Legacy, reproductor de música, coordenadas Bedrock, FPS con fondo.
Subido a la consola por FTP 19-11 (F:\Games\OptiCraft, XBE + assets.pak). Usuario: va bien, reloj redstone OK; NO le
gustó que desaparezcan mobs tapados → Claude: solo se ocultan 6+ bloques por debajo de la cámara (`a25a953`/`9787bb1`).
Perla de ender no se lanza en creativo = Java (ItemEnderPearl). Encargo 88 a agy (misma terminal): niebla del Nether
pegada a la cara, zona segura del HUD (720p se sale por abajo), mismas opciones en 1.2.5 y 1.5.2 (Avanzado en 1.2.5).
Confirmado 19-11: TNT bien, pistones bien en las dos. Encargo 89 en cola tras el 88: stick sin icono ni reasignable
tras «Restablecer» en Controles; cofre en la mano desplazado a la derecha. Splash «I love weed smoke everyday» añadido
(paks del lanzador re-empaquetados desde el pak anterior + datos de origen); FALTA subirlo a la consola (FTP no
respondía), subir con la build del 88.
88 HECHO (1.5.2 `7b69e69`, 1.2.5 `0b46eac`, informe 88) y 89 hecho por agy pero SIN su prueba final (agy sin cuota
~21:30): 1.5.2 `e6d2065`+`501b40d`, 1.2.5 `af2f850`+`410957a`. Claude compila en `wt-build-152`/`wt-build-125`
(worktrees sueltos fijados al commit, script `build88.ps1` en el scratchpad) y copia al lanzador. Cofre en mano
y vagoneta con cofre en 1.2.5: verificar en consola. README de `main` rediseñado (portadas u/SuperNitro58).
20-11 11:2x: builds 87+88+89 (1.5.2 `501b40d`, 1.2.5 `410957a`) + paks con el splash SUBIDOS a la consola por FTP.
agy (terminal «agy 89 cierre») terminando la batería de pruebas del 89 y su informe.
20-11 probado en consola: zona segura perfecta, Nether bien, Wither/skins/controles/mantener A/agacharse/cruceta/
cielo 1.2.5/agachado+cofre bien. Hojas de erluza: SE QUEDAN como están (decisión del usuario; no opción, no quitar).
**Recordar: los mobs tapados se ocultan solo 6+ bloques por debajo (EntityOcclusionCache) — el usuario pidió
dejarlo anotado.** Encargo 90 a agy: partículas VERDES al romper cofre, cursor invisible sobre un logro (ambas),
niebla del Nether de 1.5.2 peor que la de 1.2.5, y documentar la ocultación en docs/XBOX_PORT.md.
Partículas verdes del cofre y fuego mal = SOLO con el paquete de texturas LCE activado (usuario 20-11).
Cuelgue al entrar a un mundo tras apagar la Xbox (1.2.5): NO se reprodujo con el log escuchando (entró bien varias
veces). Si vuelve: receptor `python -u xbox-spike\escuchar_log.py` y mirar `crash:`/`watchdog` en netlog.txt.
El otro repo (CPPEditionLegacy, 1.8.9 en C++17 con shared_ptr) NO sirve para Xbox; solo como referencia.

## Bugs detectados por el usuario, pendientes
- **(19-11) Faro (beacon) 1.5.2:** activa el efecto SIN cobrar el objeto de pago (lingote/esmeralda/diamante). En Java,
  al pulsar «Hecho» con un efecto elegido, `GuiBeacon`/`ContainerBeacon` manda el efecto y el objeto de la casilla de pago
  (`SlotBeacon`) se consume (`decrStackSize(0, 1)` en el servidor, `Packet250 MC|Beacon` en Java). Mirar
  `GuiBeacon.cpp`, `ContainerBeacon.cpp`, `SlotBeacon.cpp`, `TileEntityBeacon.cpp` contra `mcp-diff/src152`
  (`GuiBeacon.actionPerformed`, `NetServerHandler` "MC|Beacon"). Comprobar también que sin objeto no se pueda activar.

## Lo siguiente
0. **agy sin cuota hasta ~16-11** (09-11 13:00). El encargo 83 lo hizo Claude (`83-INFORME.md`): en prueba como build 13:33.
   Wither: propuestas en el informe 83 (limitar búsquedas de camino por tick, luz de explosión una vez, asedio ≤20 zombis).
1. agy encargo 79 ACEPTADO (`42c1148`, en prueba como `91b5f42`).
2. agy encargo 80 EN MARCHA (`wt-152-rend`, rama `port152-graficos-legacy`): botón «Avanzado» en Gráficos del menú Legacy con todas las opciones tipo
   OptiFine (partículas, humo animado...), bien a 720p y 480p. `80-ENCARGO-agy-graficos-avanzados-legacy.md`.
3. agy encargo 81 (EN COLA tras el 80): selector de skins al estilo LCE con legacyUI (`81-ENCARGO-agy-skins-estilo-lce.md`).
4. Merge a `port152-claude`: HECHO 09-11 (`91b5f42`). Lo nuevo de agy se prueba en `port152-aldeanos` y luego se avanza.
3. **Entidades por portales: APARCADO por el usuario (09-11).** agy lo hizo (encargo 86, rama `port152-portales`
   `e816d58`, informe 86) pero NO está auditado ni en prueba: escribe en las regiones de la otra dimensión. Antes de
   usarlo: auditar (corrupción de .mca, caché de regiones, hilo async de la exp, duplicar/perder, tirones).
   Contexto: `Entity::setInPortal` estaba VACÍO; el port solo tiene cargado
   un mundo. Hacerlo: posición destino ×/÷8 + Teleporter (buscar/crear portal) + guardar la entidad en el chunk de la otra
   dimensión sin cargarla. Pedido por el usuario (aldeano no cruza), 09-11.
3. Plan 69: comandos + reglas + modo aventura → Wither → fuegos artificiales → dispensador → ...
4. **Pantalla partida: el jugador 2 tiene la cruceta fija. El usuario dice dejarlo para después (anotado 09-11).**
   Mando: Y = mover rápido (si el usuario lo quiere).

## Limpieza y respaldo (18-11)
- Carpetas quitadas (ramas en GitHub; builds viejas a la Papelera): OptiCraft-prueba-opti, pr-arreglos, pr-menu-ps2,
  wt-async, wt-ltcg, wt-pr, wt-prev, wt-152-idiomas, wt-152-agy, wt-152-wither, «OptiCraft crafteo consola v1»,
  «OptiCraft jugable estable», «OptiCraft prueba opti», pak-prueba.
- Quedan: OptiCraftHeritageEdition, wt-opti30 (1.2.5), wt-152-codex (1.5.2 buena + datos port152-data),
  wt-152-pociones (1.5.2 prueba), wt-152-rend (agy), buzon-xbox, mcp-diff, contexto-respaldo, xbox-spike, xbox-tools,
  servidor-1.2.5, xdk, xbox-sdks, minecraft-legacy original, y lo de PS3.
- README de `main` en GitHub con los enlaces de archive.org (LCE: https://archive.org/details/minecraft_20260301,
  XDK: https://archive.org/download/xbox-sdks). Herramientas sueltas en la rama `contexto` (`herramientas/`).

## 21-11 13:30 (antes de hibernar)
- Encargo 95 de agy HECHO y revisado (1.2.5 `9629741`: reloj/brújula con packs LCE + dragón <1 ms/tick, isla del End
  residente con `PLATFORM_END_RESIDENT_CHUNK_RADIUS 6`; 1.5.2 `b933663`: reloj/brújula). El 96 (personaje animado) lo
  hizo Claude: 1.2.5 `c027da6`→`474730f`→`f5282f6`, 1.5.2 `016fe22`→`2a668af`→`de360fd` (sin sombra y apagando el
  lightmap después: la barra se oscurecía). Ramas pusheadas a `xbox`.
- **PENDIENTE: subir a la consola los 8 XBE del lanzador (f5282f6 / de360fd)**: el FTP no respondía. Luego que el
  usuario confirme que la barra ya no se oscurece. Confirmado 21-11: icono de guardado, mobs tapados, lava LCE (1.2.5),
  vagonetas TNT (1.5.2).
- Falta: opción «Personaje animado» en Opciones (Java y Legacy) para poder quitarlo.

## 21-11 19:20 (antes de hibernar)
- Encargo 97 (agy, 1.5.2 `b900220`): crash del cohete (el emisor se borraba al llenarse el tope de partículas;
  `Random::nextInt(<=0)` ya no lanza en Xbox), crafteo de consola con categorías de la LCE TU19 (`lce-tu19\`) y sin
  duplicados, textos del faro, pigman (nunca aldeano; enfado en grupo), hielo del Nether evapora (solo 1.5.2: Java 1.2.5
  deja agua). Encargo 98 (agy, 1.2.5 `f1bf66a`, 1.5.2 `a2ec7d1`): arco como Java, `findDisplayListPair`, carteles y
  columnas en multijugador, opción «Pistas en pantalla». Probado en mundo 5 min.
- **A vigilar:** en la prueba del 98, al volver sobre chunks ya guardados `freeKB` bajó de ~24 a ~11 MB en 65 s
  (listas 529→895). Mirar si se estabiliza o si hay que limitar (¿pasa también antes del 98?).
- **PENDIENTE: subir a la consola los 8 XBE del lanzador** (f1bf66a / a2ec7d1): FTP no respondía.
- Portal del End: idea de pintar 4 capas por pasada (multitextura) en vez de 16; el usuario no ha dicho sí todavía.
- Futuro: 1.6.4 y luego 1.7.10 (techo); referencias en memoria `lce-repos-referencia`.

## 22-11 12:25
Ver `TRASPASO-hibernado.md` (juntado 950da98/0e1a3b6, sin subir; agy-2 con el 101 casi acabado sin commit; agy-1
empezando el 103). 1.6.4 y OptiFine 1.6.4 ya descompilados en `mcp-diff\src164` y `srcOF164` (guía 55).
Mods/skins/texturas: encargos 101, 103 y conversor `texturas\`.

## 22-11 tarde
- **Qué hacer en cada consola: `PLATAFORMAS-QUE-HACER.md`** (Xbox, PS2 con el plan de 7 pasos del informe 106,
  PS3, otras).
- En consola: 1.2.5 `c66352d` (todo + mods + skins + DNS de respaldo); 1.5.2 `ac018fe` (`port152-int`) en el lanzador,
  falta subir. Skins: Notch arreglado (siempre se guarda la textura corregida, regla «Notch hack» de Java).
- En marcha: agy-1 encargo 105 (superplano/marcador/combate, `wt-152-rend`), agy-2 encargo 108
  (bloques/mobs/inventario/pantalla, `wt-152-mods` rama `port152-blq`), subagente de Claude: skins 3D como la 1.8
  (`wt-152-skin`/`wt-125-skin`, informe 107). Cuando acaben: juntar en `port152-int`/`opti30-erluza`.

## 22-11 16:10 (hibernado)
Ver `TRASPASO-hibernado.md`: 1.5.2 buena = `port152-int` `ad160f2`, 1.2.5 `opti30-erluza` `83278cd` (con skins 3D 1.8,
informe 107), sin subir. agy-1 105 a medias (sin commit), agy-2 108 sin empezar.

## 22-11 17:55
**Lista completa de pendientes: `PENDIENTES-TODO.md`.** En consola: 1.5.2 `2ff4078` (`port152-int`) y 1.2.5 `a0addaf`
con HTTPS relajado hecho por Claude (`src/xbox/system/XboxTls.*`, `external/mbedtls`, `mbedtls_config.h` de la
Classic Xbox Store; `httpGetUrl` acepta https; skins reintentan por HTTPS).

## 22-11 18:40
Subido a la consola: 1.5.2 `ee55f32` (`port152-int`) y 1.2.5 `e4af03f`. Lista completa: `PENDIENTES-TODO.md`.
Cola de agy (~19:05): 105 → 108 → 111 (Modrinth + tirón al guardar) → 113 (ChocoCraft real 1.7.10 descompilado en
`mcp-diff\chococraft-1710`) → 114 (minimapa redibujo) → 115 (apagar mods de verdad, detector por mundo, Herobrine
con maldición). Subagente de Claude: estilo LCE en las pantallas nuevas (informe 112, ramas `port152-ui`/`opti30-ui`).
