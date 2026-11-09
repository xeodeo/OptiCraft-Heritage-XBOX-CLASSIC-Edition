# 30 — La RAM sube al morir (sin confirmar, 02-10)

## Lo que vio el usuario
Jugando la **experimental a 720p** (`OptiCraft_exp_720p.xbe` del 01-10 14:36), en dos partidas:

- la versión va **muy bien** y es estable;
- pero **al morir, la memoria usada va subiendo** y llega **casi al 90 %**;
- aun así **nunca consiguió que el juego se cayera**, ni poniendo pistones, luces y un reloj
  de redstone con la memoria al 90 %;
- su sospecha: algo se está quedando sin liberar.

El 90 % es el número de la **pantalla de depuración** (botón Back): `GuiIngame.cpp:372`,
`Used memory: N% (X MB) of Y`. Son `totalMemory - freeMemory` sobre `maxMemory`, no el
`freeKB` del log, así que los dos números **no son comparables** directamente.

## Lo que NO se pudo comprobar
**En los logs que hay no aparece ninguna muerte.** `xbox-spike\netlog.txt` tiene tres partidas
(10,8 min / 2,0 min / 1,1 min) y en ninguna hay un respawn. Sin un log con una muerte dentro no
se puede confirmar ni descartar nada: **lo de abajo son sospechas, no hechos**.

Lo que sí se ve en los logs, en las tres partidas por igual: la memoria libre baja desde unos
39 MB y **se queda plana** (16,8 / 22,4 / 26,5 MB). Que se estabilice encaja con la caché de
chunks llenándose, que es lo normal, no con una fuga. Pero ninguna de esas partidas tuvo muertes.

## Dónde mirar (ya revisado, por orden de sospecha)

1. **`World.cpp:172` `deleteWorldOwnedEntity`** — es lo más sospechoso.
   No borra la entidad si es `mc->thePlayer`, `thePlayerOne`, `thePlayer2` o `renderViewEntity`.
   En `Minecraft::respawn` el orden es: `updateEntityList()` (2972) → `setEntityDead(thePlayer)`
   (2978) → `thePlayer = createPlayer(...)` (2982) → `thePlayerOne = thePlayer` (3011).
   **En teoría está bien**, porque la limpieza de entidades muertas ocurre en un tick posterior,
   cuando `thePlayer` ya apunta al jugador nuevo y el viejo deja de estar protegido.
   **Pero si alguna limpieza cae entre medias**, el jugador viejo se saca de la lista y nunca se
   borra: una fuga de un `EntityPlayerSP` entera por cada muerte. Hay que comprobarlo de verdad.

2. **`Minecraft.cpp:3007`** — `thePlayer->movementInput = new MovementInputFromOptions(...)`
   sin `delete` previo. **Comprobado: no es fuga**, porque el constructor de `EntityPlayerSP`
   deja `movementInput` en `nullptr` (`EntityPlayerSP.cpp:45`) y el jugador es nuevo.
   Aun así, las otras tres asignaciones iguales (`Minecraft.cpp:2737`, `XboxSplitScreen.cpp:146`,
   `Ps2SplitScreen.cpp:123`) sí hacen `delete` antes. Conviene dejarlo igual por seguridad.

3. **`preloadWorld("Respawning", 16)`** (`Minecraft.cpp:3016`, solo Xbox) genera 3x3 columnas en
   el acto. Si el punto de reaparición está lejos, la caché puede quedarse con los chunks del
   sitio donde moriste **más** los del renacimiento. Mirar si el radio de descarga los saca.

4. **Pistones, luces y redstone**: el usuario los usó a propósito. Si la memoria sube con eso y
   no baja al alejarse, mirar las `TileEntity` (`World.cpp:256`, `3516`, `4240`) y la cola de luz.

## Cómo confirmarlo (esto es lo primero que hay que hacer)

1. Abrir el escuchador: `cd xbox-spike` y `python -u escuchar_log.py`.
2. Jugar la **exp 720** y **morir tres o cuatro veces seguidas**, a ser posible lejos del punto
   de reaparición, dejando un rato entre muertes.
3. Comparar `freeKB` **antes de cada muerte y después de reaparecer**:
   - si baja un escalón fijo en cada muerte y **no se recupera**, es fuga;
   - si baja y luego se recupera al alejarse, es la caché de chunks y no hay fallo.
4. En el log sirven las líneas `xbox.mem` (listas, pools, texturas) y `freeKB`. Si hace falta más
   detalle, `platformMemoryCheckpoint` (`Diagnostics_XBOX.cpp:11`) escribe `total=` y `free=`,
   pero es `MC_LOG_DEBUG`: necesita **`MC_LOG_LEVEL=2`**, y en la build actual está en 1.

## Importante
- **No es urgente:** el juego no se cayó ni al 90 %. No hay que romper nada estable por esto.
- **No tocar el sonido** ni retrasar contenido para arreglarlo.
- Medir **siempre en la consola real**: xemu tiene mucha más memoria y no lo reproduce.
