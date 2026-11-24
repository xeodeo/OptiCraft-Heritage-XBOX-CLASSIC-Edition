# TODO lo pendiente (23-11-2026, 21:20)

Documento único con todo lo que falta. Detalle de cada cosa en su encargo (`NN-ENCARGO-*.md`) y en
`ESTADO-OPTICRAFT.html`. Qué rehacer en cada consola: `PLATAFORMAS-QUE-HACER.md`.

## Subido a la consola (23-11 noche) — 1.2.5 y 1.5.2, normal y exp, 480p/720p
- Servidores 1.12.2 (SMP minecraft3ds.org): en Multijugador «Server version: Auto/1.2.5/1.12.2». Entra al SMP (121).
- Arreglos de cuelgue: al morir en el servidor (jugador borrado dos veces) y al descargar un chunk con jugador
  liberado en la lista (6fa1c4b/59b1481, 65669ad/fc81537). Probado el 1.º en xemu.
- Losas: losas de arriba y escaleras al revés ya no te pegan/empujan en la 1.2.5 (ef44334). En 1.5.2 el mismo tirón
  puede seguir; el log ahora apunta cada corrección de posición del servidor («server teleport») para diagnosticarlo.
- Modo admin (mod «adminmode», encargo 122, 2958e68/c7653bf): utilidades de Nodus; combo Back+X o desde el catálogo.
  23-11 noche (decidido por el usuario): SIN candado, funciona en cualquier mundo y servidor, sin op (Nuker y Build
  también: en servidor mandan los paquetes de romper/poner). Menú por pestañas (Combate, Mover, Visual, Mundo,
  Objetos, Otros; Blanco/Negro cambian). Módulo Dupe (Mano / Llenar pila / Inventario): directo en mundo local; en
  servidor manda /optidupe al plugin OptiDupe (servidor-plugins\OptiDupe, puesto en servidor-1.12.2\plugins; el
  1.2.5 vanilla no carga plugins). Commits f722475b (152) y a484ddd4 (125), juntados en port152-int / opti30-erluza.
- Chocobos completos (113/117/119), bloques y mobs 1.5.2 (108), Modrinth con «Para <versión>» y búsqueda más rápida.

## Probado en consola 1.2.5 (usuario 23-11 mañana)
- BIEN: minimapa, creeper (sonidos), personaje animado, chunks/memoria (11-18 MB libres), cambios de dimensión
  (Nether da un tirón pero bien), catálogo de mods, HTTPS (abre Modrinth), skins 3D 1.8, tirón al guardar, reloj (no
  existe en 1.2.5: correcto). Apagar mods dentro de la partida: se deja la opción por ahora.
- HECHO tras la prueba (sin probar): «Mods y texturas» (packs solo ahí), pausa centrada en 480p, agacharse siempre
  mantener al volar/montar, Modrinth pestaña «Para <versión>» que revisa el archivo de cada pack (Faithful) + reintento.
- EN MARCHA: chocobos = mod ORIGINAL completo (subagente, encargo 117): volar, «Montar» que te baja, correr, brazo
  partido, semilla sin traducir, cría fea.
- FALTA: skins sin brazos (Halo, doctora Helsing: pedir nombres), +3 MB con pack (normal si 32x; medir si 16x),
  pruebas de servidor 1.2.5, sección XBE a demanda para mbedTLS, aligerar armar el chunk al guardar (50-130 ms).
- 1.6.4 (después de la 1.5.2): en SU PROPIA carpeta del lanzador `OptiCraft.6.4`.

## 0. Versiones y dónde está cada cosa
- **EN LA CONSOLA (subido 23-11 09:25)**: 1.5.2 `6121e6c`
  (`port152-int`) y 1.2.5 `a20d430` (`opti30-erluza`): todo lo de abajo + minimapa sin redibujos (114) + sin tirón
  al guardar (111.5) + Modrinth desde la consola (116) + chocobos reales de ChocoCraft 4.1.5 (113) + detector de mods
  por mundo y apagar mods de verdad (115). `assets.pak` de las dos versiones parcheado con las 140 texturas del
  chocobo (copia del anterior en `%TEMP%ssets-<ver>-backup.pak`). agy sin cuota hasta ~28-11. Falta: 108
  (bloques/mobs, `port152-blq`), marca de mods en la lista de mundos, 1.6.4 después.
- **En la consola (subido 22-11 18:40)**: 1.5.2 `ee55f32` (rama `port152-int`) y 1.2.5 `e4af03f` (`opti30-erluza`):
  todo lo de los encargos 87–103 + mods (101) + skins 3D 1.8 (107) + DNS de respaldo + HTTPS relajado (mbedTLS de
  la Classic Xbox Store; skins reintentan por HTTPS) + Texture Packs de vuelta en el catálogo (botón/Start) + fondo
  animado y panel centrado en los menús nuevos + HTTPS se libera al salir de buscar skin.
- **Ramas a juntar cuando acaben**: `port152-erluza` (agy-1, encargo 105 sin commit en `wt-152-rend`) y
  `port152-blq` (agy-2, encargo 108) → `port152-int`.
- Lanzador: `Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\OptiCraft\<versión>\` (+ `texturepacks\`).

## 1. Probar en la consola (lo hecho y aún no confirmado)
- **Skins**: búsqueda por usuario (ahora con DNS de respaldo y reintento HTTPS), Notch con cara (borrarlo y
  volver a bajarlo), skins 3D: Alex (brazos finos), skin con capas, armadura, agacharse, cama, pantalla partida,
  primera persona, packs Halo/ME/Skyrim (modelo clásico).
- **Mods** (todos apagados; Mods → catálogo): minimapa (Back + cruceta), X-Ray (LT+RT+White+Black), luces dinámicas,
  Herobrine (altar de tkelly), ChocoCraft, localizadores, TMI, info de servidores.
- **Texturas**: los 8 packs en Mods → Texture Packs (16x JoliCraft el más seguro), `.zip` directo, animaciones.
- **1.5.2**: dispensador (vertical, 16 comportamientos), soltador, bloque de comandos, generador + vagoneta, cuero
  teñido, caldero, Espinas (con sonido), libros en la mesa (valores 1.5.2 para todo), mapas (copiar/ampliar/marco),
  fuegos artificiales (crash arreglado), crafteo de consola (categorías TU19), faro (texto), pigman, hielo Nether.
- **Ambas**: reloj/brújula (pack LCE), personaje animado (barra ya no se oscurece), chat en botón tras reiniciar,
  options.txt sin duplicados, arco como Java, «Pistas en pantalla», sonido del creeper (¿se oye?).
- **A vigilar**: RAM al volver a zonas ya cargadas (en xemu bajó de 24 a 11 MB); memoria en el End (isla residente).

## 2. Trabajo de agy (cuota vuelve ~19:05)
- **116**: HECHO (`71bc987`/`1928c76`, sin probar en consola; informe 116 con lista de pruebas). Antes: descargar packs de Modrinth desde la consola por versión + pantalla de
  texturas con estilo LCE + conversión del formato 1.5 en la consola.
- **111 (URGENTE, del usuario 22-11 18:00)**: (1) volvió a desaparecer la entrada «Texture Packs» (no hay dónde
  cambiar texturas); (2) menú de mods: fondo quieto y contenido muy arriba, revisar TODAS las pantallas nuevas en
  480p/720p; (3) descargar packs de Modrinth desde la consola (≈244 para 1.2.5, ≈266 para 1.5.2) con montaje del
  formato 1.5 en la consola; (4) HTTPS sin gasto dentro del juego (`XboxTls::shutdown`, sección XBE a demanda).
- **Tirón de 1,5 s guardando un chunk al descargarlo** (consola 22-11 18:01): HECHO por Claude 19:30 (`820d6d0` 1.5.2 /
  `709aac3` 1.2.5): cada guardado de chunk releía `session.lock` del disco en el hilo principal (en FATX espera a las
  escrituras del hilo de IO); ahora se comprueba cada 30 s. Si vuelve, el log dirá `slow chunk save x,z: lock= nbt= queue=`.
- **Cuelgue en consola 22-11 ~18:00**: el receptor del log estaba apagado (se cayó al hibernar); reproducir con
  `escuchar_log.py` encendido o leer `E:\TDATA\FFFF4F43\debug.log` por FTP.
- **113 (chocobos)**: HECHO por agy (`45edeba`/`2ee1d28`, probado en xemu; assets.pak del lanzador parcheado). Antes: cría con la cabeza despegada; no parece un chocobo (usar modelo y
  texturas de ChocoCraft4, MIT/CC-BY-SA, bajado en `mcp-diff\chococraft4-ref`); el dorado no vuela; montado se mueve
  solo y monta sin silla; daño por caída al planear/aterrizar.
- **115 (mods por mundo)**: HECHO 20:40 (detector `9f442e3`; apagar de verdad `6121e6c`/`a20d430`), SIN PROBAR. Antes: apagar un mod no lo apaga (los chocobos se siguen viendo); detector de mods y versión
  por mundo con aviso al abrirlo (activar / entrar sin ellos conservándolos / volver); Herobrine con la maldición
  (fuego) encendida por defecto como el original.
- **114 (minimapa)**: HECHO por Claude 19:25 (`7d51ba7` 1.2.5 / `d7197b2` 1.5.2): el modo cueva automático cambiaba en
  cada boca de cueva/árbol y redibujaba todo a 1 fila cada 4 ticks (~13 s); ahora espera 1,5 s estable y el
  redibujado rápido tarda ~0,2 s. Probar en consola.
- **112 (subagente de Claude, en marcha)**: todas las pantallas nuevas con aspecto LCE de Xbox (parecían Bedrock).
- **105** (agy-1, A MEDIAS, 54 ficheros sin commit + sonda `XboxProbe105.cpp`): superplano personalizable, marcador
  (scoreboard), combate 1.5.2 (mensajes de muerte, flechas en llamas). NO reset.
- **108** (agy-2, sin empezar): bloques (puertas, cama Nether, troncos, nieve capas, vallas, cacao, hojas), mobs
  (collar lobo, ovejas, Enderman, slimes pantano, gólem, gatitos, crías), inventario (Mayús+clic, repartir, XP horno),
  pantalla (nombre del objeto, calaveras, colores §).
- **109**: descargar packs desde la consola (servidor de packs en el PC por la red de casa; ahora también se puede
  con HTTPS desde internet).
- **110**: ✔ HECHO por Claude (HTTPS relajado). Falta: usarlo en 109 con un catálogo en internet; certificados
  verificados solo si sobra memoria.

## 3. Plan 1.5.2 que queda (plan 69)
- Fila 16: Nether con las cuevas de la semilla 1.5.2 (`MapGenCavesHell`) — cambia el terreno del Nether nuevo, decidir.
- Lo que salga de las pruebas de 105 y 108.
- Juntar `port152-int` en `port152-claude` (rama «buena») cuando el usuario lo confirme todo; lo mismo
  `opti30-erluza` → `xbox-opti-30`.

## 4. Ideas aprobadas / anotadas, sin encargo aún
- Portal del End: 4 capas por pasada (multitextura) en vez de 16, mismo dibujo (el usuario lo pidió sin bajar capas).
- Historial de skins de un usuario (NameMC: solo con programa en el PC).
- Opción en menús para quitar el personaje animado.
- Herobrine: revisar eventos que roban/rompen (apagados por defecto).
- Mods/descargas por internet con el HTTPS nuevo (catálogo de packs, skins por API de Mojang).

## 5. Futuro
- **1.6.4** (caballos, riendas, etiquetas, alfombras, paja, atributos, EntityLivingBase, packs de recursos): ya
  descompilada en `mcp-diff\src164` y su OptiFine en `mcp-diff\srcOF164` (guía 55). Después **1.7.10** (techo en Xbox).
- **PS2**: plan de 7 pasos del informe 106 (`PLATAFORMAS-QUE-HACER.md`). **PS3**: auditoría pendiente (port en otro chat).
- Publicar release pública cuando todo esté probado (README ya rediseñado en `main`).

## 6. Herramientas y referencias que ya tenemos
- `texturas\` (conversor `.bat`/ventana, 8 packs, `descargar_y_subir.py`), `lce-tu19\` (LCE TU19), `mcp-diff\`
  (1.2.5/1.5.2/1.6.4 + OptiFine 1.5.2/1.6.4 + Herobrine iLoveYou), Classic Xbox Store (HTTPS),
  receptor de log `xbox-spike\escuchar_log.py`, página `ESTADO-OPTICRAFT.html`.
