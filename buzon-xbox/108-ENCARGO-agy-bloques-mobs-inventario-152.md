# 108. Encargo agy-2: bloques, mobs, inventario y pantalla de la 1.5.2 (22-11)

Plan 69, filas 13, 14, 15 y 17. Carpeta `wt-152-mods`, rama NUEVA `port152-blq` (base `ac018fe` = 1.5.2 con todo +
mods). Solo 1.5.2. agy-1 trabaja a la vez en `wt-152-rend` (superplano/marcador/combate): no toques su carpeta.
Sin reset, sin push, sin copiar al lanzador; commits con `git add <fichero>`; candado `buzon-xbox\BUILD.lock`.
Reglas: 60 fps (medir), 64 MB (medir `freeKB`), creativo NO, terreno de las semillas igual (¡ojo con hojas, troncos y
nieve: solo comportamiento/dibujo, nunca generación!), nunca /fp:fast, dos interfaces, mando, pantalla partida.
Referencias: Java 1.5.2 `mcp-diff\src152\` (manda), Java 1.2.5 `src125\`, LCE TU19 `lce-tu19\`.
Compara clase a clase con `sh mcp-diff/comparar.sh wt-152-mods` (lista `cambiadas`) para ver qué no se portó.

## Fila 13 — Bloques
Puertas 1.5.2 (bisagra/metadatos de arriba-abajo), cama que explota en el Nether/End, troncos tumbados (orientación
por metadatos y dibujo lateral), nieve en capas (8 alturas, `BlockSnow` 1.5.2), vallas que conectan con otros bloques
como 1.5.2, cacao (`BlockCocoa`: crece, se cae, se planta en jungla), hojas 1.5.2 (decaimiento y caída de manzanas/
retoños como Java). Lo que ya esté igual que Java: dilo y no lo toques.

## Fila 14 — Mobs
Collar del lobo teñible, ovejas con mezcla de colores al criar, Enderman 1.5.2 (bloques que coge, teletransporte con
agua/flecha), slimes en pantanos (fase lunar), gólem de hierro (flor al aldeano), gatitos (ocelote domesticado, crías
que siguen), crías de mobs 1.5.2. Compara cada IA con Java.

## Fila 15 — Inventario
Mayúsculas+clic (en mando: el botón equivalente que ya use el port) que mande armadura a su hueco y al horno lo que
toque, repartir pila arrastrando (en mando como lo haga la LCE TU19), XP al sacar del horno (`SlotFurnace`).

## Fila 17 — Pantalla
Nombre del objeto al cambiar de casilla (`GuiIngame` 1.5.2, fundido), calaveras puestas en la cabeza (render), códigos
de color § en carteles/chat/nombres.

## Aceptación
Lista por fila de lo hecho / ya estaba / no aplica. Capturas en `buzon-xbox\108-capturas\`. 60 fps y `freeKB`;
`nan`/`[X87]` = 0. Informe `buzon-xbox\108-INFORME-agy.md`; la señal de fin es la palabra CIENTOOCHO-LISTO seguida
del commit.

## TRASPASO (Claude, 22-11 15:57)
agy-2 se quedó sin cuota (vuelve ~19:07) leyendo las clases Java (BlockLog, BlockSnow, ItemSnow…); no escribió nada:
`wt-152-mods` limpio en `ac018fe` (rama `port152-blq`). Al volver: empezar el encargo desde el principio.
