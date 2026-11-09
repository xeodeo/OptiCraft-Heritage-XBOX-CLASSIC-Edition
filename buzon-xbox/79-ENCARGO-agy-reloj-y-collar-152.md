# 79. Encargo agy: reloj animado y collar del lobo, como Java 1.5.2 (09-11)

## Dónde
- Worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-rend`, rama NUEVA `port152-reloj-collar`
  (ya creada por Claude desde `5e85bff` de `port152-aldeanos`). SIN reset. NO toques `wt-152-pociones`, `wt-152-codex`, `wt-opti30`.
- Java de referencia: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`.
- Datos del juego (texturas del pak): `wt-152-codex\build\xbox-release\port152-data`; script de datos
  `scripts/port152/build_textures_152.py`; el pak se rehace con `python -I scripts/make_pak.py <datos> <salida.pak>`.

## 1. El reloj no se mueve (siempre marca el día)
Medido por Claude: `TextureWatchFX.cpp` anima el reloj como en la 1.2.5: busca en el icono píxeles clave
(`k == i1 && l == 0 && i1 > 0`) y los cambia por la esfera `misc/dial.png` girada. El icono del atlas de la 1.5.2 es el
primer dibujo de la tira `clock.png` de 1.5.2 y NO tiene esos píxeles, así que nunca cambia nada.
Java 1.5.2 usa `TextureClock.java`: una tira de 64 dibujos (`textures/items/clock.png` de 1.5.2) y elige el dibujo según
`getCelestialAngle` (con el mismo amortiguado `field_94239_h/field_94240_i`; en el Nether, aleatorio).
Hazlo igual: incluye la tira de 64 dibujos en los datos (no en el atlas: aparte, p.ej. `textures/items/clock.png` en el pak,
o embebida si es pequeña) y que el TextureFX del reloj copie al hueco del icono el dibujo que toca. Respeta lo que ya hay:
solo se sube el tile si cambia (commit 8db6978) y la opción de animaciones. La brújula ya funciona: no la toques.

## 2. Collar del lobo domado (rojo por defecto, se tiñe con tintes)
Hoy no se dibuja. Porta de 1.5.2: `EntityWolf` (DataWatcher 20 = color del collar, por defecto 14 = rojo;
`getCollarColor/setCollarColor`; NBT `CollarColor`; teñir con tinte en `interact` si es tuyo y está domado),
`RenderWolf.shouldRenderPass` (pasada con `/mob/wolf_collar.png`, ya está en los datos, color de
`EntitySheep.fleeceColorTable[BlockCloth.getBlockFromDye(color)]`), y lo que haga falta en `ModelWolf`.
Recuerda `ensureEntityInit()` y el orden del DataWatcher. Mundos viejos sin `CollarColor` → rojo.

## Trampas
- Nada de `/fp:fast`. No toques la generación del mundo ni los ticks.
- Lobos con dueño de mundos viejos deben cargar bien.
- Si una pasada de render cambia color/blend, déjalo como estaba después (los demás mobs no deben salir teñidos).

## Compilar y entregar
- Con `buzon-xbox\BUILD.lock`: `cmake --build build/xbox-release` y `build/xbox-release-exp` en `wt-152-rend`
  (OJO: esas carpetas de build apuntan a este worktree; el label 1.5.2 y el log ya están configurados).
- Si cambian los datos, rehaz el pak en `wt-152-rend\bin\assets.pak` y dilo.
- NO copies nada al lanzador.
- Informe: `buzon-xbox\79-INFORME-agy-reloj-collar.md` (qué, archivo:línea, qué probaste y qué NO).
- Señal final literal: `ENCARGO 79 LISTO <commit>`
