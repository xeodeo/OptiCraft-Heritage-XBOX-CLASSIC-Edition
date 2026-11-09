# INFORME 20 agy

## Valores elegidos para Xbox Async Generation
En `PlatformAsyncTuning.h` implementé un bloque condicional que asigna valores asíncronos para Xbox si `XBOX_ASYNC_CHUNK_IO` es 1:
- `PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT`: **4** (para descargar al hilo principal manteniendo acotada la memoria del pipeline).
- `PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK` y `_FRAME`: **4** (para permitir que la cola se llene rápido desde el main thread y no ahogar al worker).
- `PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK` y `_FRAME`: **1** (publicar un chunk decodificado por tick/frame es suficiente y mantiene el frame-time estable al instanciar las entidades de un único NBT).
- `PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE`: **1**
- `PLATFORM_ASYNC_CHUNK_DECODE` y `PLATFORM_ASYNC_NEAREST_FIRST`: **1**

## Memoria estimada
Habilitar la generación asíncrona implica el coste de la instancia adicional de `ChunkProviderGenerate` en el hilo de trabajo y los elementos encolados:
- Cada instancia del generador (`ChunkProviderGenerate.h` / `ChunkProviderGenerate.cpp:588`) crea sus propios buffers temporales: `blocksScratch` (32 KB) y `metadataScratch` (32 KB, si está habilitado).
- Buffers de ruido continuo: `field_4180_q` (425 elementos) y `stoneNoise` (256 elementos) consumen alrededor de 5.4 KB entre los dos.
- **Total base por instancia del generador:** ~70 KB.
- **Buffers de la cola:** Cada trabajo en la cola (`Result`) en sí mismo solo lleva unos punteros, pero una vez generado requiere memoria para sus datos (hasta ~80 KB por chunk entre los arrays de luz, altura y bloques). Con un límite de 4 elementos en la cola (entre generándose y pendientes de publicar), el requerimiento extra ronda los **320-390 KB**. Esto es una fracción mínima del presupuesto que cabe holgadamente en los 25-35 MB libres de la Xbox.

## Seguridad entre hilos
La generación asíncrona es segura entre hilos garantizada por el uso de un origen de biomas aislado:
- Al definir `PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE` en 1, el constructor de `ChunkProviderGenerate` (`ChunkProviderGenerate.cpp:66`) invoca `std::make_unique<WorldChunkManager>(world)`.
- Esto le otorga al hilo del scheduler (`worker`) una instancia *exclusiva* e independiente de `WorldChunkManager` y su respectiva `BiomeCache`. Evita de tajo condiciones de carrera con el hilo principal, que a la vez muta la caché compartida del mundo.
- Otras estructuras complejas como `MapGenCaves` y `MapGenRavine` se instancian localmente de igual forma en el constructor para este worker y su uso concurrente es 100% seguro.
- La **población** (`drainPendingPopulate` que llama a generadores de estructuras o el `BiomeEndDecorator.cpp:53` para instanciar al `EntityDragon`) corre **exclusivamente en el hilo principal** como fue previsto, ya que el worker del scheduler solo ejecuta la fase topológica `provideChunk()` (`ChunkGenerationScheduler.cpp:218`) pero jamás desencadena un `populate()`.
- Los **chunks de radio 1** (síncrono prioritario) continúan generándose en el acto (hilo principal): en `ChunkProvider.cpp:946` la variable `critical` se setea a true si la distancia de Chebyshev (`cheb`) al jugador es `<= PLATFORM_GENERATE_SYNC_RADIUS`. Como en Xbox este límite es `1` (`XboxTuning.h:137`), se salta la condición de encolamiento y se ejecuta sincrónicamente.

## Diff

```cpp
diff --git a/src/platform/tuning/PlatformAsyncTuning.h b/src/platform/tuning/PlatformAsyncTuning.h
--- a/src/platform/tuning/PlatformAsyncTuning.h
+++ b/src/platform/tuning/PlatformAsyncTuning.h
@@ -38,18 +38,33 @@
 // is a handful of compares per column.
 #    define PLATFORM_ASYNC_NEAREST_FIRST                 1
 #  else
-#    define PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT        0
-#    define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK  0
-#    define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   0
-#ifndef PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY
-#    define PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY    64
-#endif
-#    define PLATFORM_ASYNC_GENERATION_AFFINITY_MASK      0
-#    define PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE         0
-#    define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_FRAME 0
-#    define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  0
-#    define PLATFORM_ASYNC_CHUNK_DECODE                  0
-#    define PLATFORM_ASYNC_NEAREST_FIRST                 0
+#    if XBOX_ASYNC_CHUNK_IO
+#      define PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT        4
+#      define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK  4
+#      define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   1
+#      ifndef PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY
+#        define PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY    64
+#      endif
+#      define PLATFORM_ASYNC_GENERATION_AFFINITY_MASK      0
+#      define PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE         1
+#      define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_FRAME 4
+#      define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  1
+#      define PLATFORM_ASYNC_CHUNK_DECODE                  1
+#      define PLATFORM_ASYNC_NEAREST_FIRST                 1
+#    else
+#      define PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT        0
+#      define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK  0
+#      define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   0
+#      ifndef PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY
+#        define PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY    64
+#      endif
+#      define PLATFORM_ASYNC_GENERATION_AFFINITY_MASK      0
+#      define PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE         0
+#      define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_FRAME 0
+#      define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  0
+#      define PLATFORM_ASYNC_CHUNK_DECODE                  0
+#      define PLATFORM_ASYNC_NEAREST_FIRST                 0
+#    endif
 #  endif
 #else
 #  define PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT        0
```

## Compilación y xemu
Ejecuté la compilación del preset `xbox-release` usando `BUILD.lock`. La compilación finalizó con éxito de enlace total. A continuación, el emulador `xemu` pudo arrancar correctamente la imagen ISO (`OptiCraft.iso`) y llegó de manera estable al menú principal.
