# Correcciones al encargo 15 (revisión de Claude, 28-09)

Buen análisis. El cambio de `RegionFile::write` (comprimir fuera del mutex, con un buffer local) está bien.

Falta esto, en tu worktree `wt-async`:
1. **Riesgo de ODR:** `PLATFORM_ASYNC_CHUNK_GENERATION` quedó definido como `XBOX_ASYNC_CHUNK_IO`, que vive en `XboxTuning.h`. En las unidades que no incluyen `XboxTuning.h` vale 0 en silencio, y `ChunkProvider` y otras clases podrían tener un layout distinto en cada `.cpp`. Arreglo: define el valor por defecto de `XBOX_ASYNC_CHUNK_IO` en `src\platform\PlatformConfig.h`, justo antes (`#ifndef XBOX_ASYNC_CHUNK_IO / #define XBOX_ASYNC_CHUNK_IO 1`). En `XboxTuning.h` deja solo un comentario que apunte ahí.
2. **Prioridad baja:** lo prometiste pero no está en el diff. Busca dónde crea su hilo `ChunkGenerationScheduler` y ponle prioridad por debajo de lo normal en Xbox (`src\platform\Thread.cpp` tiene `SetThreadPriority`).
3. **Confírmalo en el informe con `archivo:línea`:** con el scheduler activo, ¿la POBLACIÓN (`populate`: árboles, menas) sigue en el hilo principal? Si el scheduler también la mueve o la retrasa, dilo. El usuario rechazó la decoración diferida.
4. **Compila** (comandos del encargo 15) con `XBOX_ASYNC_CHUNK_IO 1` y con `0`. Prueba en xemu que llega al menú (mira antes que no haya un xemu abierto; OpenCode lo comparte).
5. Añade al final de `15-INFORME-agy.md` una sección `## Resultado` con: errores de compilación (si hay), tamaño de `bin\xbox\iso\default.xbe` y el resultado de xemu.

## PRIORIDAD NUEVA (medido en consola, 28-09 14:46, 254 tirones de >45 ms en una partida)
- `chunkUnload` aparece en **141 de 254** tirones, con `save=25ms` cada vez: la DESCARGA guarda el chunk en el hilo principal (construye el NBT y lo serializa). Tu informe decía que construir el NBT "no causa tirones enormes": el log dice que es la causa número 1. **Saca de ahí la construcción del NBT del guardado al descargar**, o parte el trabajo, sin perder datos. Mira `ChunkProvider.cpp:1364-1545` (`unload100OldestChunks`) y `AnvilChunkLoader.cpp:254-294`.
- El peor tirón: `frame=224ms {worldTick=188.9 mobSpawn=180.7} load=178.4ms`. La **aparición de mobs provocó una carga de disco síncrona**. Busca en `SpawnerAnimals` qué pide chunks (`getBlockId` o `getChunk` fuera de lo cargado) y haz que no dispare cargas: que salte las columnas no cargadas.
- Orden: primero esto, luego lo de las correcciones 1-5.

## CONDICIÓN DEL USUARIO: el Ender Dragon no se puede romper
- Localiza con `archivo:línea` dónde se crea `EntityDragon` en el End (generación o población del chunk central, `ChunkProviderEnd` o `WorldProviderEnd`) y escríbelo en el informe.
- Con el scheduler asíncrono, **ninguna entidad se puede crear ni añadir al mundo desde el hilo del scheduler**. Si la generación o la población del End crea el dragón, ese paso tiene que seguir en el hilo principal. Si hace falta, excluye la dimensión del End del modo asíncrono: `XBOX_ASYNC_CHUNK_IO` solo en el Overworld y el Nether.
- Lo que cambies en `SpawnerAnimals` (saltar las columnas no cargadas) no debe tocar el dragón ni los cristales.
