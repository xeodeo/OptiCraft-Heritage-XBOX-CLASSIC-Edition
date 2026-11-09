# ENCARGO 19 (agy): la descarga de chunks ESPERA al hilo de generación

## Medido en la consola real (28-09 16:24, versión `3b9c8a8` = tu async IO)
- Mejoró un poco, pero siguen 376 tirones de >45 ms (63,7 ms de media, sin contar la entrada al mundo).
- `chunkUnload` aparece en **153** tirones: media **28,3 ms**, máximo 90,5.
- En esos mismos tirones, `save` tiene media **2,8 ms**: el guardado ya NO es el problema. Tu cambio del NBT funcionó.
- **`gen` vale casi lo mismo que `chunkUnload`** en el mismo fotograma:
  ```
  frame=71ms {worldTick=35.2 chunkUnload=33.5} gen=33.4ms save=0.0ms
  frame=57ms {worldTick=23.8 chunkUnload=20.6} gen=20.5ms save=0.0ms
  ```
  Conclusión: **dentro de la descarga, el hilo principal se queda esperando a que el hilo del `ChunkGenerationScheduler` termine una generación** (cancelación síncrona, join, mutex o drenado). Como ese hilo tiene prioridad POR DEBAJO de lo normal, la espera es de 20-90 ms: una inversión de prioridad.
- `present=0.1ms`: ya no sobra tiempo de vsync, porque el hilo de fondo lo usa. Eso está bien.

## Qué hacer, en tu worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-async` (rama `xbox-async-io`)
1. **Actualiza tu rama:** ya contiene `3b9c8a8`. Comprueba `git -C wt-async log --oneline -2`.
2. **Busca con `archivo:línea`** qué hace `unload100OldestChunks` (`ChunkProvider.cpp`) cuando el chunk (o uno vecino) tiene un trabajo pendiente o en curso en el scheduler, y dónde se mide `gen` durante la descarga.
3. **Arréglalo sin esperar nunca al hilo de fondo desde el hilo principal.** Opciones, elige y justifica:
   - no descargar este tick los chunks con un trabajo pendiente o en curso: se reintenta en el siguiente;
   - cancelar el trabajo pendiente sin bloquear: marcarlo y descartar el resultado al adoptarlo;
   - si hay un mutex compartido, que el hilo principal use `try_lock` y lo reintente en el siguiente tick.
4. **No toques** la población (`drainPendingPopulate`) ni el Ender Dragon. Tampoco cambies la prioridad del hilo sin medirlo.
5. **Compila con candado** (`buzon-xbox\REGLA-BUILD-LOCK.md`), con los comandos del encargo 15, y comprueba que xemu llega al menú (con candado).

## Tuyos / no tuyos
- **Tuyos:** los mismos del encargo 15 (`ChunkProvider*`, `AnvilChunkLoader*`, `RegionFile*`, `ThreadedFileIOBase*`, `ChunkGenerationScheduler*`, `SpawnerAnimals*`, `PlatformConfig.h`, `XboxTuning.h`), en `wt-async`.
- **No tuyos:** el resto de `src\`, el repo principal, `wt-ltcg\`, ClassicXboxLoader. No hagas commit.
- Si crees que el diagnóstico está mal, dilo con `archivo:línea`.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\19-INFORME-agy.md`. Primera línea literal: `# INFORME 19 agy`. Incluye:
- dónde esperaba el hilo principal (`archivo:línea`);
- el arreglo;
- el diff;
- la compilación y xemu.
