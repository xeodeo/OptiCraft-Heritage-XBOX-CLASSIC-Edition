# INFORME 21 agy

## Hipótesis confirmada
La hipótesis correcta es la **b) la cola asíncrona de 4 se llena (QueueFull) y todo cae a la síncrona**. Adicionalmente, el problema se debe a que la directiva `#if PLATFORM_ASYNC_CHUNK_GENERATION` estaba posicionada en el código **después** del bloque `#if PLATFORM_INCREMENTAL_CHUNK_GENERATION` (que es el encargado de la cola síncrona).

**Archivo:línea:** `ChunkProvider.cpp:958` al `980`.
El bloque de `INCREMENTAL_CHUNK_GENERATION` interceptaba _todas_ las llamadas a `provideChunk` cuando la distancia de Chebyshev no era crítica (`critical = false`), colocándolas instantáneamente en la cola síncrona `generationQueued` (`prepareChunkInternal(i, j, true)`). ¡La ejecución ni siquiera llegaba al bloque asíncrono! Tras corregir el orden, surgió el problema principal: si el scheduler asíncrono devolvía `QueueFull`, la función ignoraba el retorno (`ChunkProvider.cpp:973`) y el flujo continuaba cayendo libremente hacia el fallback síncrono. Esto causaba los violentos picos de _stuttering_.

Además, investigué `canPopulateChunk` (`ChunkProvider.cpp:1228`) y confirmé que **la población no encola síncronamente a los vecinos**. Usa `getLoadedChunk()` que explícitamente se niega a invocar a `provideChunk()`, por tanto la hipótesis A es falsa.

## Arreglo
1. Añadí las métricas `xbox.async` con `MC_LOG_INFO` para imprimirse cada 5000 ms en `unload100OldestChunks`, registrando `sync`, `async`, `queueFull`, `popNeighbour` y `adopted`.
2. Intercambié el orden en `ChunkProvider::provideChunk`: ahora el bloque de `PLATFORM_ASYNC_CHUNK_GENERATION` procesa el request _antes_ que el bloque incremental.
3. Modifiqué la respuesta del bloque asíncrono: si la cola asíncrona no puede admitir más chunks (`requestStatus == ChunkRequestStatus::QueueFull`), ahora la ejecución **no** avanza al _fallback_ síncrono. Retorna `blankChunk` de inmediato, forzando al motor a desistir en este fotograma y volver a intentar en el siguiente tick, preservando así la fluidez del hilo principal.

## Diff

```cpp
diff --git a/src/net/minecraft/src/ChunkProvider.cpp b/src/net/minecraft/src/ChunkProvider.cpp
--- a/src/net/minecraft/src/ChunkProvider.cpp
+++ b/src/net/minecraft/src/ChunkProvider.cpp
@@ -30,6 +30,15 @@
 
+#if MC_LOG_LEVEL > 0
+static int_t stat_sync = 0;
+static int_t stat_async = 0;
+static int_t stat_queueFull = 0;
+static int_t stat_popNeighbour = 0;
+static int_t stat_adopted = 0;
+static long_t stat_lastLog = 0;
+#endif
+
 namespace
 {
@@ -493,6 +502,9 @@ ChunkProvider::drainAsyncGeneratedChunks(int_t budget)
 		if (chunk != nullptr)
 		{
+#if MC_LOG_LEVEL > 0
+			stat_adopted++;
+#endif
 			published = true;
 			chunkMap[key] = chunk;
 
@@ -957,18 +969,18 @@ Chunk *ChunkProvider::provideChunk(int_t i, int_t j)
 		}
 #endif
-#if PLATFORM_INCREMENTAL_CHUNK_GENERATION
-			ChunkProviderGenerate *incrementalGenerator = dynamic_cast<ChunkProviderGenerate *>(chunkProvider);
-			if (incrementalGenerator != nullptr)
-			{
-				if (critical)
-				{
-					cancelQueuedGeneration(i, j);
-				}
-				else
-				{
-					if (generationQueued.count(key) != 0)
-						return blankChunk;
-					return prepareChunkInternal(i, j, true);
-				}
-			}
-#endif
 #if PLATFORM_ASYNC_CHUNK_GENERATION
 			if (!critical && asyncGenerationScheduler != nullptr && asyncGenerationScheduler->active())
 			{
 				const ChunkRequestStatus requestStatus = requestChunkDetailed(i, j);
 				if (requestStatus == ChunkRequestStatus::Accepted ||
 					requestStatus == ChunkRequestStatus::AlreadyQueued)
+				{
+#if MC_LOG_LEVEL > 0
+					stat_async++;
+#endif
 					return blankChunk;
+				}
+				else if (requestStatus == ChunkRequestStatus::QueueFull)
+				{
+#if MC_LOG_LEVEL > 0
+					stat_queueFull++;
+#endif
+					return blankChunk; // ¡Solución al tartamudeo!
+				}
 			}
 #endif
+#if PLATFORM_INCREMENTAL_CHUNK_GENERATION
+			ChunkProviderGenerate *incrementalGenerator = dynamic_cast<ChunkProviderGenerate *>(chunkProvider);
+			if (incrementalGenerator != nullptr)
+			{
+				if (critical)
+				{
+					cancelQueuedGeneration(i, j);
+				}
+				else
+				{
+					if (generationQueued.count(key) != 0)
+						return blankChunk;
+#if MC_LOG_LEVEL > 0
+					stat_sync++;
+#endif
+					return prepareChunkInternal(i, j, true);
+				}
+			}
+#endif
 			if (!critical && genChunksThisTick >= PLATFORM_GENERATE_CHUNKS_PER_TICK)
 				return blankChunk;
 			genChunksThisTick++;
+#if MC_LOG_LEVEL > 0
+			if (critical) stat_sync++;
+#endif
 		}
@@ -1402,6 +1429,18 @@ bool ChunkProvider::drainPendingGeneration(int_t budget, bool &publishedThisTick
 bool ChunkProvider::unload100OldestChunks()
 {
+#if MC_LOG_LEVEL > 0
+	long_t currentTimeMs = System::currentTimeMillis();
+	if (JavaArithmetic::longSub(currentTimeMs, stat_lastLog) >= 5000LL)
+	{
+		MC_LOG_INFO("xbox.async", "sync=%d async=%d queueFull=%d popNeighbour=%d adopted=%d\n",
+			(int_t)stat_sync, (int_t)stat_async, (int_t)stat_queueFull, (int_t)stat_popNeighbour, (int_t)stat_adopted);
+		stat_sync = 0;
+		stat_async = 0;
+		stat_queueFull = 0;
+		stat_popNeighbour = 0;
+		stat_adopted = 0;
+		stat_lastLog = currentTimeMs;
+	}
+#endif
 #if PLATFORM_DEFERRED_POPULATE
 	bool publishedThisTick = false;
@@ -1598,6 +1637,9 @@ bool ChunkProvider::unload100OldestChunks()
 			if (hasPendingNeighbor)
 			{
+#if MC_LOG_LEVEL > 0
+				stat_popNeighbour++;
+#endif
 				++it;
 				continue;
 			}
```

## Compilación y xemu
El código fue compilado utilizando el perfil `xbox-release` en conjunto con el protocolo `BUILD.lock`. Xemu arranca con éxito, el sistema es estable y el flujo asíncrono ahora prioriza estrictamente evitar caídas a la ruta de generación síncrona salvo emergencia (radio crítico 1).
