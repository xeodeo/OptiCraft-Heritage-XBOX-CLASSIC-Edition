# INFORME 22 agy

## Sitios de espera del hilo principal (archivo:línea)

### 1. `isWorkingOn()` — `ChunkGenerationScheduler.cpp:208`
El método `isWorkingOn()` usaba `std::lock_guard<std::mutex>` sobre `impl_->mutex`, el mismo mutex que el worker mantiene mientras decide qué chunk generar (`runWorker()`, línea 251). En la Xbox (CPU de un solo núcleo), el hilo principal llama a `isWorkingOn()` desde `unload100OldestChunks` para **cada chunk candidato × 9 vecinos** (81 adquisiciones de mutex si hay 9 chunks candidatos). Con el worker de prioridad 1, si el worker tiene el mutex mientras genera (no, lo suelta antes de generar), o si lo tiene brevemente para push de resultados (línea 311/322/353/371), el main thread espera. Pero el verdadero problema es la **inversión de prioridad**: en single-core, cada `lock_guard` implica un cambio de contexto potencial donde el OS podría ceder la CPU al worker de prioridad baja — y luego el main thread queda esperando a que el worker suelte el mutex.

**Arreglo:** Cambié `isWorkingOn()` a `std::try_to_lock`. Si no consigue el mutex, devuelve `true` (asumir ocupado = no descargar el chunk este tick), eliminando toda posibilidad de bloqueo.

### 2. `requestDetailed()` — `ChunkGenerationScheduler.cpp:117`
Cada petición de chunk pasa por `requestDetailed()` que toma `impl_->mutex`. Con `async=2394` (miles de peticiones por ventana de 5s), esto son miles de adquisiciones de mutex por tick. La mayoría son duplicadas (`AlreadyQueued`), pero cada una bloquea.

**No se modifica:** Es un lock breve y necesario. El arreglo está en reducir las veces que se llama (deduplicación por `AlreadyQueued`).

### 3. `dispatch()` — `ChunkGenerationScheduler.cpp:147,161`
Toma el mutex dos veces por chunk dispatched. Necesario, breve. No hay riesgo de deadlock.

### 4. `popResult()` — `ChunkGenerationScheduler.cpp:183`
Breve. No hay riesgo.

### 5. `complete()` — `ChunkGenerationScheduler.cpp:198`
Breve. No hay riesgo.

**Conclusión sobre la congelación:** No hay un deadlock clásico (A espera a B, B espera a A). El problema es **starvation del worker**: en single-core con prioridad 1, el main thread corre continuamente haciendo miles de `requestDetailed()` y `isWorkingOn()` cada tick. El worker nunca recibe CPU suficiente para terminar un chunk. El main thread no puede avanzar porque no hay chunks adoptados → bucle infinito de re-peticiones → congelación aparente. `adopted=1` en 5 segundos lo demuestra.

## Deduplicación
En la versión anterior, `stat_async` contaba tanto `Accepted` (nuevo) como `AlreadyQueued` (duplicado), inflando el número a 2394 cuando la mayoría eran repeticiones. Ahora:
- `stat_async` solo cuenta `Accepted` (chunks realmente nuevos encolados)
- `stat_alreadyQueued` cuenta las repeticiones (informativo; no generan trabajo)

El scheduler ya deduplicaba internamente (`ChunkGenerationScheduler.cpp:118`: `if (impl_->queued.count(k) != 0) return AlreadyQueued`), pero los contadores no lo reflejaban.

## Adopción
Subí `PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK` y `..._PER_FRAME` de **1 a 4** (`PlatformAsyncTuning.h:44,51`). Con PUBLISH=1, el main thread solo adoptaba 1 chunk por tick, lo que significaba ~20 chunks/s. Con 4, el ritmo sube a hasta 4 por tick (hasta 80/s a 20fps), permitiendo drenar la cola de resultados mucho más rápido.

## ¿Qué mide `gen` con `sync=0`?
Con `sync=0`, la métrica `gen` sigue contando porque `drainPendingGeneration()` (`ChunkProvider.cpp:1435`) ejecuta la **cola de generación incremental** (`generationQueue`). Esta cola es independiente del scheduler asíncrono. Cuando `PLATFORM_INCREMENTAL_CHUNK_GENERATION` está activo (lo está en Xbox: `XboxTuning.h:146`), las peticiones que no van al scheduler asíncrono (por ejemplo, chunks en la fase `critical`) encolaron trabajo incremental. `drainPendingGeneration` avanza ese trabajo paso a paso en el hilo principal, y su tiempo es profiled como `gen` (`ChunkProvider.cpp:1436-1438`). No tiene nada que ver con el scheduler asíncrono.

Además, la traza `gen=84ms` en el fotograma pre-congelación sugiere que el generador incremental procesó pasos acumulados, probablemente de chunks que se pidieron en la fase `critical` (radio síncrono 1).

## Diff

```diff
--- a/src/platform/chunks/ChunkGenerationScheduler.cpp
+++ b/src/platform/chunks/ChunkGenerationScheduler.cpp
@@ -205,7 +205,9 @@ bool ChunkGenerationScheduler::isWorkingOn(int_t x, int_t z) const
 {
 #if PLATFORM_ASYNC_CHUNK_GENERATION
-    std::lock_guard<std::mutex> guard(impl_->mutex);
+    std::unique_lock<std::mutex> lock(impl_->mutex, std::try_to_lock);
+    if (!lock.owns_lock())
+        return true; // Assume busy if we cannot check — safer than blocking
     return impl_->queued.count(key(x, z)) != 0;

--- a/src/platform/tuning/PlatformAsyncTuning.h
+++ b/src/platform/tuning/PlatformAsyncTuning.h
@@ -44,9 +44,9 @@
-#      define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   1
+#      define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   4
-#      define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  1
+#      define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  4

--- a/src/net/minecraft/src/ChunkProvider.cpp
+++ b/src/net/minecraft/src/ChunkProvider.cpp
 // Added stat_alreadyQueued counter
 // Split Accepted/AlreadyQueued counting in provideChunk
 // Updated log format:
 //   "sync=%d async=%d queued=%d qFull=%d popN=%d adopted=%d pending=%d done=%d"
 // queueSizes() now called for pending/done counts in the trace
```

## Compilación y xemu
Compilé con `xbox-release` + `-DMC_LOG_LEVEL=1` usando `BUILD.lock`. Xemu arranca y llega al menú.
