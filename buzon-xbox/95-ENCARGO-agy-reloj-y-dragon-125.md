# 95. Encargo agy: el reloj no gira y tirones del dragón (1.2.5) (20-11)

Rama `opti30-erluza` (`wt-opti30`, último `fdd2911` de Claude). Sin reset, sin push, sin copiar al lanzador.
Reglas de siempre: BUILD.lock, 60 fps, memoria (64 MB), creativo NO se toca, terreno igual.

## 1. El reloj no gira (fallo, visto por el usuario en la consola real 20-11)
En la 1.2.5 el reloj (`Item::pocketSundial`, icono 6,4 de `/gui/items.png`) no gira con la hora: ni en la mano, ni
en la barra rápida, ni en el inventario. Comprueba también la brújula. Revisa `TextureWatchFX` (registro en
`Minecraft.cpp:699`), `RenderEngine::updateDynamicTextures` / `updateSpecialTextureFx` / `uploadTextureFxTile`
para `tileImage == 1` (atlas de objetos), `PLATFORM_DYNAMIC_TEXTURE_INTERVAL_TICKS`, opción «Objetos animados»
(`ofAnimatedItems`), y si la barra rápida / el objeto en mano usan alguna caché que se quede con el icono viejo.
Prueba con el pack por defecto y con el pack LCE (el usuario lo usa: los píxeles magenta del icono los sustituye el
dial; si el pack LCE no los tiene, haz lo que haga la LCE/Java con ese pack). Compara con la 1.5.2 (`wt-152-rend`),
donde hay que confirmar si gira; si allí también falla, arréglalo en las dos ramas y dilo.

## 2. Tirones del dragón en el End (log de la consola real)
`[xbox.ent] entTick=110-170ms ... EntityDragon x1 107-169ms` cada pocos segundos (resto de ticks ~12 ms). Busca qué
parte de `EntityDragon::onLivingUpdate` se dispara (búsqueda de cristales, `destroyBlocksInAABB`, colisiones con las
partes, `getEntitiesWithinAABB`, ruta) y déjalo por debajo de ~10 ms sin cambiar su comportamiento de Java 1.2.5.
Mide en xemu con el dragón vivo y 40 enderman (como en la consola).

## Aceptación
- Reloj gira de día a noche (capturas a 2 horas distintas), en mano, barra e inventario; pack por defecto y LCE.
- Dragón: `entTick` sin picos >20 ms durante 2 min de combate; mismo comportamiento.
- 60 fps sin cambios. Capturas en `buzon-xbox\95-capturas\`.

## Informe
`buzon-xbox\95-INFORME-agy.md`. Señal: `ENCARGO 95 LISTO` + commit final.

## TRASPASO (Claude, 20-11 19:18)
agy se quedó sin cuota a mitad del encargo. Nada commiteado: cambios SIN COMMIT en `wt-opti30` sobre `d0bff75`
(15 ficheros, +834/-58): reloj/brújula (`TextureWatchFX`, `TextureCompassFX`, `TextureFX`, `RenderEngine`,
`scripts/xbox/make_legacy_texture_packs.py` — parece que el pack LCE no tenía los píxeles del dial) y dragón
(`EntityDragon`, `EntityDragonPart`, `World.cpp`, `Minecraft.cpp`, `XboxTuning.h`). NO hacer reset ni checkout:
al volver la cuota, seguir desde ese árbol, compilar, probar y terminar el informe.
