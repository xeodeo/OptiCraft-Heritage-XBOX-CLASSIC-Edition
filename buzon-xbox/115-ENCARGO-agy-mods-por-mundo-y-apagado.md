# 115. Encargo agy: apagar un mod de verdad + detector de mods y versión por mundo; Herobrine como el original (22-11)

Visto por el usuario en la consola real (22-11 18:35). Ramas `port152-int` y `opti30-erluza` (carpeta que diga
Claude). Sin reset, sin push; `git add <fichero>`; candado `buzon-xbox\BUILD.lock`. Dos interfaces (aspecto LCE),
mando, 480p/720p, pantalla partida. Va DESPUÉS del 113 (chocobos) si tocas los mismos ficheros.

## 1. Apagar un mod no lo apaga
Apagó ChocoCraft en el catálogo y **los chocobos se siguen viendo**. Al apagar un mod (y al arrancar con él apagado):
sus entidades no se crean ni se actualizan ni se dibujan, sus bloques/objetos no funcionan, no aparecen nuevos
(spawns), sus recetas salen del crafteo (Java y consola), sus HUD/combos se apagan. Comprobar con los 9 mods.
**Visto otra vez por el usuario (22-11 18:50):** con ChocoCraft apagado **sus objetos siguen saliendo en los menús**
(inventario creativo, crafteo Java y de consola, etc.). Con un mod apagado, sus objetos/bloques no deben aparecer en
ninguna lista (creativo, crafteo de consola, recetas, TooManyItems) ni poder conseguirse; los que el jugador ya tenga
se conservan en su inventario/cofres (NBT intacto) aunque no se puedan usar mientras esté apagado.
**Sin borrar datos del jugador**: las entidades/objetos del mod que ya están en el mundo se conservan en el chunk
(guardar su NBT tal cual al volver a guardar el chunk) para que reaparezcan si se vuelve a encender.

## 2. Detector de mods y versión por mundo (como Java/Forge)
- Guardar en `level.dat` (clave propia, p. ej. `OptiCraftMods`) la lista de mods activos con su versión y la versión
  del juego (1.2.5 / 1.5.2 + build) con la que se jugó por última vez.
- Al abrir un mundo: si faltan mods que el mundo usaba, hay mods nuevos, o la versión del juego es distinta (p. ej. un
  mundo de 1.5.2 abierto en 1.2.5, o al revés), mostrar un aviso claro con la lista y opciones:
  «Activar los mods y entrar», «Entrar sin ellos» (se conservan, no se borran) y «Volver». Mundo de una versión
  más nueva abierto en una más vieja: avisar fuerte (puede perder bloques/objetos nuevos).
- En la lista de mundos, una marca pequeña si el mundo usa mods (y cuáles al seleccionarlo).

## 3. Herobrine igual que el original (iLoveYou de tkelly, `mcp-diff\herobrine-iloveyou\src\AltarEvent.java`)
Original: noche → rompe antorchas en radio 5/10/20 (las suelta) → a los ~7 s aparece en el altar con antorcha de
redstone y frase → 5 s después **maldición: prende fuego al jugador 10 s** y mientras dura le vuelve a prender fuego
cada vez que se mueve (le mira) → 10 s después borra el altar y se va. **No pega cuerpo a cuerpo.**
En el nuestro «Maldición altar» está APAGADA por defecto: ponerla **encendida por defecto** (como el original), con
la opción para apagarla en el menú del mod. Revisar que todo lo demás coincida con el código original.

## Aceptación
Capturas: aviso de mods al abrir un mundo (faltan / nuevos / otra versión), mundo con chocobos con el mod apagado
(no se ven) y vuelto a encender (reaparecen), Herobrine quemando al jugador. Capturas en `buzon-xbox\115-capturas\`.
Informe `buzon-xbox\115-INFORME-agy.md`; la señal de fin es la palabra CIENTOQUINCE-LISTO seguida de los commits.

## REVISIÓN DE CLAUDE (22-11 19:45) — ANTES DE EMPEZAR
- Punto 3 (Herobrine maldición encendida por defecto) HECHO por Claude.
- Punto 2 (detector por mundo) HECHO por Claude: `9f442e3` en `opti30-erluza` (cherry-pick en `port152-int`).
  `level.dat` guarda `OptiCraftVersion` y `OptiCraftMods` ("id@ver;..."); al abrir un mundo (Java y Legacy, ambas
  pasan por `GuiSelectWorld::selectWorld`) sale `GuiWorldModsWarning` si faltan/sobran mods de contenido
  (`ModManager::isWorldContentMod`: chococraft, herobrine) o la versión es otra (más nueva = aviso en rojo).
  Falta solo la marca pequeña en la lista de mundos (opcional).
- **Tu parte: SOLO el punto 1** (apagar de verdad: entidades, spawns, objetos fuera de listas/recetas, conservar NBT).

## CARPETAS (Claude, 22-11 20:15)
Tus chocobos (113) ya están juntados. Trabaja en `wt-152-choco` (rama nueva `port152-off`, desde `port152-int`
`49cf07e`, ya con 113 + Modrinth + detector) y `wt-125-choco` (rama `opti30-off`, desde `opti30-erluza` `88772bf`).

## CIERRE (Claude, 22-11 20:40)
agy se quedó sin cuota (vuelve en ~136 h) con el punto 1 casi hecho y sin commit; Claude lo revisó, compiló (4 builds
OK) y commiteó: 1.5.2 `6121e6c` (`port152-off`), 1.2.5 `a20d430` (`opti30-off`), ya en `port152-int` y
`opti30-erluza`. Qué hace: al apagar un mod de contenido sus entidades se guardan "dormidas" en el NBT del chunk
(`Chunk::dormantModEntities`, se reescriben tal cual al guardar y reviven al encender), no entran nuevas
(`World::entityJoinedWorld`), sus objetos salen del creativo, búsqueda, crafteo (Java y consola), recetas y
TooManyItems, y sus bloques/objetos no hacen nada. SIN informe de agy y SIN probar en xemu ni consola.
