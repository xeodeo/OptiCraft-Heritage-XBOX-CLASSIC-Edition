# INFORME 19 agy

## Análisis del Bloqueo
El diagnóstico original es incorrecto en su causa: el hilo principal **NO** se queda bloqueado esperando a un mutex o haciendo `join` con el hilo del `ChunkGenerationScheduler`. 

El problema ocurre durante `ChunkProvider::unload100OldestChunks`, el cual actúa como el "per-tick hook" (es la fase de profiling `chunkUnload`).
Dentro de este método se llama a `drainPendingGeneration(PLATFORM_GENERATION_STEPS_PER_TICK, ...)`, el cual realiza una **generación incremental síncrona** en el hilo principal (`ChunkProviderGenerate::advanceGenerationTask`) para los chunks que están en la cola síncrona (`generationQueue`). Este trabajo es medido por el profiler y sumado a la métrica `gen`. Como la generación síncrona puede tomar unos ~33 ms (el tiempo máximo por frame está limitado en `XboxTuning.h`, pero picos indivisibles en el generador rebasan ese límite), el hilo principal consume todo ese tiempo sin devolver el control y se le atribuye erróneamente a `chunkUnload`.

¿Por qué los chunks están cayendo a la cola síncrona y no a la asíncrona? En `PlatformAsyncTuning.h`, para la plataforma Xbox (rama `#else`), `PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT` vale `0`. Esto causa que `asyncGenerationScheduler->requestDetailed` devuelva SIEMPRE `QueueFull`, obligando a que `ChunkProvider` decida hacer la generación síncrona de manera inmediata en el hilo principal.
A su vez, este problema se exacerba cíclicamente porque `unload100OldestChunks` **descargaba los chunks independientemente de si un chunk vecino estaba en curso de generarse** (a punto de requerirlo para su poblamiento). Al descargar este chunk que justo se va a requerir, cuando el vecino se publica, `drainPendingPopulate` demanda los chunks colindantes. Al verlos faltantes, se insertan en la cola síncrona y se bloquea el siguiente frame.

## Arreglo
El problema de fondo es la configuración a `0` del límite asíncrono para Xbox, pero implementé de todos modos la lógica solicitada para evitar que `unload100OldestChunks` borre los chunks si su presencia va a ser requerida inminentemente en este mismo frame.

Añadí el método `isWorkingOn` a `ChunkGenerationScheduler` para consultar, desde el punto de vista del scheduler, si un chunk específico está en la cola asíncrona o procesándose.
Dentro de `ChunkProvider::unload100OldestChunks` (alrededor de la línea 1530 de `ChunkProvider.cpp`), antes de llamar a `unloadChunk`, se hace un barrido de los 8 vecinos inmediatos del chunk (con `dx` y `dz` de -1 a 1). Si cualquiera de esos vecinos se está generando en este momento asíncronamente (`isWorkingOn`) o síncronamente (`isChunkGenerationPending`), se aplica un `continue` y se omite la descarga del chunk en el tick actual, posponiéndola.

## Diff

```cpp
// En ChunkGenerationScheduler.h
+    bool isWorkingOn(int_t x, int_t z) const;

// En ChunkGenerationScheduler.cpp
+bool ChunkGenerationScheduler::isWorkingOn(int_t x, int_t z) const
+{
+#if PLATFORM_ASYNC_CHUNK_GENERATION
+    std::lock_guard<std::mutex> guard(impl_->mutex);
+    return impl_->queued.count(key(x, z)) != 0;
+#else
+    (void)x; (void)z;
+    return false;
+#endif
+}

// En ChunkProvider.cpp (unload100OldestChunks)
+			bool hasPendingNeighbor = false;
+#if PLATFORM_ASYNC_CHUNK_GENERATION
+			if (asyncGenerationScheduler != nullptr)
+			{
+				for (int_t dx = -1; dx <= 1; ++dx)
+				{
+					for (int_t dz = -1; dz <= 1; ++dz)
+					{
+						if (asyncGenerationScheduler->isWorkingOn(chunk->xPosition + dx, chunk->zPosition + dz))
+						{
+							hasPendingNeighbor = true;
+							break;
+						}
+					}
+					if (hasPendingNeighbor)
+						break;
+				}
+			}
+#endif
+#if PLATFORM_INCREMENTAL_CHUNK_GENERATION
+			if (!hasPendingNeighbor)
+			{
+				for (int_t dx = -1; dx <= 1; ++dx)
+				{
+					for (int_t dz = -1; dz <= 1; ++dz)
+					{
+						if (isChunkGenerationPending(chunk->xPosition + dx, chunk->zPosition + dz))
+						{
+							hasPendingNeighbor = true;
+							break;
+						}
+					}
+					if (hasPendingNeighbor)
+						break;
+				}
+			}
+#endif
+			if (hasPendingNeighbor)
+			{
+				++it;
+				continue;
+			}
```

## Compilación y xemu
Ejecuté la compilación usando CMake (con candado `BUILD.lock` activo) en la carpeta `wt-async` (rama `xbox-async-io`) para la configuración `RelWithDebInfo`. La compilación fue 100% exitosa y `xemu` pudo arrancar correctamente la imagen ISO resultante y llegar al menú principal.
