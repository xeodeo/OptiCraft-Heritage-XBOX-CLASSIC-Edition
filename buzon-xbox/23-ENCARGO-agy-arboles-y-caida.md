# ENCARGO 23 (agy): la async (`36531e0`) no pone árboles y se congela al caer

## Medido en la consola real (28-09 ~17:55, `OptiCraft_async.xbe` = `36531e0`)
- **Bien:** se siente más fluida. Las trazas cada 5 s:
  ```
  sync=0 async=23 queued=806  qFull=45  popN=0 adopted=22 pending=2 done=0
  sync=0 async=21 queued=1379 qFull=0   popN=0 adopted=23 pending=0 done=0
  sync=0 async=17 queued=1681 qFull=226 popN=0 adopted=15 pending=4 done=0
  ```
  La generación va al hilo (~5 chunks/s). Pero se repiten ~1.000 pedidos por ventana (`queued`, sin reintentar menos).
- **MAL 1: NO APARECEN ÁRBOLES** (ni, probablemente, menas u otra decoración) en el terreno nuevo. El usuario ya rechazó antes la decoración tardía. **Tiene que decorarse igual que en la versión síncrona.**
  - Hipótesis: los chunks adoptados desde el scheduler (`drainAsyncGeneratedChunks` o equivalente) no pasan por `publishPreparedChunk` / `populate` como los síncronos (`ChunkProvider.cpp:553-580`, "populate up to 4 neighbours"). Por eso nunca se pueblan, o solo cuando falta un vecino que nunca llega.
  - `popN=0` siempre.
- **MAL 2: se congela.** No hay reporte de `XboxCrashLog`. Últimas líneas:
  ```
  fps=59.3 ... loop ticks=1.6ms
  [xbox.player] pos=-2805.622 88.823 267.132 motionY=-0.9267 onGround=0
  ```
  El jugador CAÍA justo antes del cuelgue. Sospecha: entró en un `blankChunk` temporal (el que devuelve `provideChunk` con `QueueFull`, encargo 21) y algo entra en un bucle infinito (colisión, luz, `getHeightValue`, `preparePlayerToSpawn`) o escribe en el `blankChunk` compartido.

## Qué hacer, en `wt-async` (ya en `36531e0`)
1. **Árboles:** asegúrate de que todo chunk adoptado del hilo recorre EXACTAMENTE el mismo camino de publicación y población que uno síncrono (el mismo `populate` de hasta 4 vecinos, en el hilo principal). `archivo:línea` del antes y el después.
2. **Caída / cuelgue:** revisa qué pasa cuando el jugador (o un mob) pisa o consulta un `blankChunk`:
   - que nada escriba en él;
   - que la colisión trate las columnas no cargadas como en la versión síncrona (el jugador no debe caer en un hueco; si hace falta, la columna bajo el jugador y la de alrededor se generan en el acto: radio síncrono 1);
   - busca bucles que no terminen.
3. **Pedidos repetidos:** que no se vuelva a pedir cada tick lo ya pedido (`queued` debe bajar mucho).
4. **Compila con candado** `xbox-release`, `-DMC_LOG_LEVEL=1`. En xemu entra a un mundo, camina, y comprueba con una captura que hay árboles en el terreno nuevo.

## Tuyos / no tuyos
Igual que en el encargo 20. No hagas commit.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\23-INFORME-agy.md`. Primera línea literal: `# INFORME 23 agy`. Incluye:
- la causa de los árboles y del cuelgue, con `archivo:línea`;
- el arreglo;
- el diff;
- la captura de xemu con árboles.
