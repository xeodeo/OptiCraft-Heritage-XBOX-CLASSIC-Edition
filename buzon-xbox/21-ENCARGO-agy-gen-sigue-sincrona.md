# ENCARGO 21 (agy): la generación SIGUE en el hilo principal (va peor)

## Medido en la consola real (versión `4f29550` = tus encargos 19+20, partida de ~8 min, 28-09 17:02)
| | antes (`3b9c8a8`, 16:24) | ahora (`4f29550`) |
|---|---|---|
| tirones >45 ms | 377 | **466** |
| `chunkUnload` en tirones | 153 (28 ms de media) | **242 (26 ms de media)** |
| `gen` >5 ms en tirones | 121 | **227 (26 ms de media)** |
| FPS mínimo cada 5 s | — | 29,1 |
| `freeKB` al final | — | 27.568 |

- `gen` vuelve a valer casi lo mismo que `chunkUnload` en el mismo fotograma: la generación síncrona (`drainPendingGeneration` dentro de `unload100OldestChunks`) sigue pasando, y ahora MÁS veces.
- **Hipótesis a comprobar** (no las des por buenas):
  - a) la población (`drainPendingPopulate`) necesita vecinos y los mete en la cola SÍNCRONA;
  - b) la cola asíncrona de 4 se llena (`QueueFull`) y todo cae a la síncrona;
  - c) tu cambio del 19 (no descargar si hay un vecino pendiente) mantiene más chunks vivos y provoca más trabajo;
  - d) `PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK` o `PUBLISH_PER_TICK` limitan y el resto va síncrono.

## Qué hacer, en `wt-async` (rama `xbox-async-io`, ya contiene `4f29550`)
1. **Añade trazas temporales** detrás de `MC_LOG_LEVEL > 0`, sin inundar el log (resumen cada 5 s como máximo, como `xbox.perf`):
   - cuántos chunks van a la cola síncrona y cuántos a la asíncrona, y por qué motivo (radio síncrono, `QueueFull`, vecino de población, límite por tick…);
   - cuántos adopta el hilo principal desde el scheduler.

   Formato: `[MC][I][xbox.async] sync=N async=N queueFull=N popNeighbour=N adopted=N`.
2. **Con el código**, confirma cuál de las hipótesis a-d es la causa principal (`archivo:línea`) y arréglala para que la generación de terreno fuera del radio síncrono 1 vaya SIEMPRE al hilo de fondo. Si la cola se llena, se espera al siguiente tick en vez de generar síncrono.
3. **No toques** la población ni el Ender Dragon. Sin pérdida visual y sin retrasar árboles ni mobs más allá de lo necesario.
4. **Compila con candado** `xbox-release` y comprueba que xemu llega al menú.

## Tuyos / no tuyos
- Igual que en el encargo 20. No hagas commit.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\21-INFORME-agy.md`. Primera línea literal: `# INFORME 21 agy`. Incluye:
- la hipótesis confirmada, con `archivo:línea`;
- el arreglo;
- el diff;
- la compilación y xemu.
