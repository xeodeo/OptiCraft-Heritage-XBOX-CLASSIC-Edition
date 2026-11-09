# Estado actual del proyecto (09-11-2026, 11:00)

Resumen para empezar un chat nuevo. Contexto de fondo: `00-CONTEXTO.md`. Lo técnico del port: `docs/XBOX_PORT.md`
(en cada rama). Plan de la 1.5.2: `69-PLAN-COMPLETO-152.md`. Historia de la última semana: `71-TRASPASO-06-11.md`.

## Ramas y carpetas
| Qué | Rama | Carpeta | Último |
|---|---|---|---|
| **1.2.5 buena** | `xbox-opti-30` | `wt-opti30` | `83e0ad8` (niebla + cielo) |
| **1.5.2 en prueba** (todo lo nuevo) | `port152-aldeanos` | `wt-152-pociones` | `04c22cd` (+ ticks Java, agy 78) |
| **1.5.2 buena** (confirmada 09-11) | `port152-claude` | `wt-152-codex` | `91b5f42` (= prueba, avance rápido) |
| 1.5.2 rendimiento (agy, encargo 78 aceptado) | `port152-rendimiento` | `wt-152-rend` | `bf9ac4e` (ya pasado a prueba) |
| agy (encargos viejos) | — | `wt-152-agy` | libre |

Cuando el usuario confirme la build de prueba: merge de `port152-aldeanos` a `port152-claude`, y luego lo de agy (78).
Remotos: `xbox` y `fork` son del usuario; **`origin` es OptiJuegos: nunca push ahí**.

## Builds en el lanzador (`Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\`)
- `OptiCraft\1.5.2\`: 4 XBE del 09-11 10:55 + `assets.pak` 10:44 (sonidos de aldeano y bruja). **Log por red activado**
  (`MC_LOG_LEVEL=1`, IP 192.168.0.18). Receptor: `python -u OptiCraftHeritageEdition\xbox-spike\escuchar_log.py`.
- `OptiCraft\1.2.5\`: 4 XBE del 09-11 10:56; pak sin cambios. Los anteriores en `OptiCraft_respaldo_125_03-11`.
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

## Build 1.5.2 del 09-11 13:33: probar también
- Skins LCE sin rayas y pestañas sin repetir; Crear mundo (sin Legacy UI) sin botones montados; Logros recortados al
  panel y desplazamiento hasta el final; en el menú sin Legacy UI pone «Minecraft 1.5.2».

## Pendiente de probar en 1.2.5 (el usuario la deja para DESPUÉS; rama `opti30-cruceta`, build 09-11 12:10)
Niebla, cielo, sol, luna, nubes; agachado+bloque sobre un cofre; cruceta separada del stick y configurable (Controles),
iconos del stick, chat (B sale); texturas animadas y HUD Legacy más rápidos; ticks por chunk (sin prioridades: 1.2.5 no
las tiene); logros (recorte y desplazamiento, build 13:33). Si va bien: avanzar `xbox-opti-30` a `opti30-cruceta`.

## Lo siguiente
0. **agy sin cuota hasta ~16-11** (09-11 13:00). El encargo 83 lo hizo Claude (`83-INFORME.md`): en prueba como build 13:33.
   Wither: propuestas en el informe 83 (limitar búsquedas de camino por tick, luz de explosión una vez, asedio ≤20 zombis).
1. agy encargo 79 ACEPTADO (`42c1148`, en prueba como `91b5f42`).
2. agy encargo 80 EN MARCHA (`wt-152-rend`, rama `port152-graficos-legacy`): botón «Avanzado» en Gráficos del menú Legacy con todas las opciones tipo
   OptiFine (partículas, humo animado...), bien a 720p y 480p. `80-ENCARGO-agy-graficos-avanzados-legacy.md`.
3. agy encargo 81 (EN COLA tras el 80): selector de skins al estilo LCE con legacyUI (`81-ENCARGO-agy-skins-estilo-lce.md`).
4. Merge a `port152-claude`: HECHO 09-11 (`91b5f42`). Lo nuevo de agy se prueba en `port152-aldeanos` y luego se avanza.
3. Plan 69: comandos + reglas + modo aventura → Wither → fuegos artificiales → dispensador → ...
4. **Pantalla partida: el jugador 2 tiene la cruceta fija. El usuario dice dejarlo para después (anotado 09-11).**
   Mando: Y = mover rápido (si el usuario lo quiere).
