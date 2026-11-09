# ENCARGO 20 (agy): abrir la cola asíncrona de generación en Xbox

Tu informe 19 es correcto: yo me había equivocado al decir que el hilo principal esperaba al de fondo. Gracias por corregirlo con `archivo:línea`. Tu arreglo de no descargar chunks con vecinos pendientes se queda.

## Qué hacer, en `wt-async` (rama `xbox-async-io`, sin commit)
1. En `src\platform\tuning\PlatformAsyncTuning.h`, para Xbox y SOLO si `XBOX_ASYNC_CHUNK_IO` vale 1, pon `PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT` en un valor pequeño, **4**, para que la generación de terreno pase al hilo del scheduler.
2. **Revisa y anota con `archivo:línea`:**
   - memoria extra por trabajo en cola: buffers de ruido, `TerrainNoiseBuffer`, bloques de 32 KB… Con 64 MB, cada MB cuenta; ahora hay 25-35 MB libres;
   - que la población (`drainPendingPopulate`: árboles, menas, el Ender Dragon en `BiomeEndDecorator.cpp:53`) sigue en el hilo principal;
   - que los chunks del radio síncrono 1 siguen generándose en el acto;
   - si el generador es seguro entre hilos (el Wii y el PC legacy ya lo usan: ¿hay algún estado global compartido, como `WorldChunkManager` o `BiomeCache`?).
3. **Compila con candado** el preset `xbox-release` (no `RelWithDebInfo`), con los comandos del encargo 15, y comprueba que xemu llega al menú.

## Tuyos / no tuyos
- **Tuyos:** los del encargo 15, más `src\platform\tuning\PlatformAsyncTuning.h` y `src\platform\chunks\ChunkGenerationScheduler.*`.
- **No tuyos:** el resto de `src\`, el repo principal, `wt-ltcg\`, ClassicXboxLoader.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\20-INFORME-agy.md`. Primera línea literal: `# INFORME 20 agy`. Incluye:
- el valor elegido;
- la memoria estimada;
- la seguridad entre hilos, con `archivo:línea`;
- el diff;
- la compilación y xemu.
