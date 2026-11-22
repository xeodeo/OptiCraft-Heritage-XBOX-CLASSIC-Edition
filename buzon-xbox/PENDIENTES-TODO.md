# TODO lo pendiente (22-11-2026, 17:55)

Documento único con todo lo que falta. Detalle de cada cosa en su encargo (`NN-ENCARGO-*.md`) y en
`ESTADO-OPTICRAFT.html`. Qué rehacer en cada consola: `PLATAFORMAS-QUE-HACER.md`.

## 0. Versiones y dónde está cada cosa
- **En la consola (subido 22-11 17:55)**: 1.5.2 `2ff4078` (rama `port152-int`) y 1.2.5 `a0addaf` (`opti30-erluza`):
  todo lo de los encargos 87–103 + mods (101) + skins 3D 1.8 (107) + DNS de respaldo + **HTTPS relajado** (mbedTLS
  como la Classic Xbox Store; las skins reintentan por HTTPS si HTTP falla).
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
