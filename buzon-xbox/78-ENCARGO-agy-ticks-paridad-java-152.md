# 78. Encargo agy: ticks programados iguales que Java 1.5.2 (09-11)

## Dónde
- Worktree: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-rend`, rama `port152-rendimiento`, desde `ff8a92f`
  (SIN reset; es tuyo para este encargo). NO toques `wt-152-pociones`, `wt-152-codex` ni `wt-opti30`.
- Contexto: `buzon-xbox\76-INFORME-rendimiento-152.md` (planificador de ticks por chunk `PcLegacyTickScheduler`,
  activado en Xbox con `XBOX_CHUNK_TICK_SCHEDULER` en `src/xbox/XboxTuning.h`, commit `6f1fcfa`).
- Java de referencia: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\WorldServer.java`
  (`tickUpdates`, `scheduleBlockUpdateWithPriority`, `func_82740_a`, `isBlockTickScheduled`, `getPendingBlockUpdates`,
  `field_94579_S`), `NextTickListEntry.java`, `AnvilChunkLoader.java` (`TileTicks`).

## Qué arreglar (auditoría del 09-11; cada punto = diferencia real con Java)
1. `isBlockTickScheduled` debe mirar la lista de ticks que se ESTÁN ejecutando en este tick (`field_94579_S`), no la cola
   pendiente. Afecta a repetidores y comparadores.
2. Java saca primero todo el lote del tick (hasta 1000) a esa lista y LUEGO lo ejecuta. Un tick con retardo 0 programado
   durante el proceso NO se ejecuta en el mismo tick. Hoy se saca de uno en uno.
3. Chunk no cargado: Java usa radio 0 (`checkChunksExist(x-0,...)`) y lo vuelve a programar con retardo 0. Hoy: radio 8 y
   se descarta.
4. Comparar el bloque con `Block.isAssociatedBlockID` como Java, no con `==`.
5. `getWorldTotalTime` frente a `getWorldTime`: usar lo que usa Java. Comprueba si en el port son distintos: con `/time set`
   o al dormir los ticks NO deben moverse.
6. Guardar y cargar la prioridad `"p"` en `TileTicks` (`AnvilChunkLoader.cpp` ~403-420 y ~550). Un mundo viejo sin `"p"`
   debe cargar (prioridad 0).
7. Guardar también los ticks a 2 bloques del borde del chunk (`-2`), como Java.
8. Al descargar un chunk, quitar sus ticks (`remove=true` donde Java lo hace) sin punteros colgantes.
9. `detachFromChunk` no debe recorrer todo el chunk para sacar un tick: guarda un índice/iterador en la entrada.
10. (Opcional, solo si es barato) `RenderAPI_D3D8_XBOX.cpp` ~1747-1756: 2-3 rectángulos sucios separados en vez de unir el
    portal (fila 0) con la lava (fila 14) en uno que cubre casi todo el atlas.

Aplica 1-8 también a la ruta vieja (sin planificador) si comparte código, y a PC-legacy si es el mismo fichero. Commit por
punto o por grupo, con el mensaje en inglés como el repo.

## Trampas (cada una ya dio un fallo real)
- El orden de ejecución de ticks es sagrado: tiempo, luego prioridad, luego id. Redstone, agua y lava deben comportarse
  EXACTAMENTE como Java 1.5.2. Relojes de redstone, repetidores con retardo, comparadores, agua y lava fluyendo.
- Nada de `/fp:fast`. No cambies la generación del mundo.
- No borres entradas mientras iteras; nada de punteros a entradas ya liberadas.

## Compilar
Con el candado `buzon-xbox\BUILD.lock` (si existe, espera; si no, créalo y bórralo al acabar aunque falle):
`cmake --build build/xbox-release` y `cmake --build build/xbox-release-exp` en `wt-152-rend`
(`XBOX_XDK_ROOT` como en `OptiCraftHeritageEdition\xbox-spike\build_preset.ps1`). NO copies nada al lanzador.

## Entrega
- Añade una sección "09-11 agy" a `buzon-xbox\76-INFORME-rendimiento-152.md`: qué cambiaste en cada punto (archivo:línea),
  qué probaste y qué no.
- Señal de fin, literal, en tu última línea: `ENCARGO 78 LISTO <commit>`

## REVISION DE CLAUDE (09-11, sobre b092688)
Bien: Time/DayTime, lote de 1000 con ranuras nulas, prioridad "p", índice de cubeta O(1), rectángulos sucios. Arregla esto
(commits nuevos sobre b092688, sin reset):
1. **Quitar el borrado de ticks al descargar chunk** (`Chunk.cpp:1354`, `onChunkUnload` con `remove=true` + `delete`): Java
   1.5.2 NO lo hace. Con el margen [-2,+18) además borra ticks del chunk VECINO que sigue cargado: agua o un reloj de
   redstone a 1-2 bloques del borde se congela. En PS2/Wii-low (sin el guardado forzado) los ticks se pierden para siempre.
   Deja que caduquen como en Java.
2. **Quitar el guardado forzado de chunks no modificados con ticks** (`ChunkProvider.cpp:1130`,
   `ChunkProviderLoadOrGenerate.cpp:420`): Java no lo hace y en el Nether (fuego eterno) haría escribir a disco casi cada
   descarga y la de sus vecinos. Guardar los ticks solo cuando el chunk se guarda por lo normal, como Java.
3. **Tu punto 4/rama inmediata es falso**: `func_82506_l()` (`World.cpp:3282`) no hace nada porque faltan los overrides de
   `BlockFlowing` y `BlockFire`, que en Java devuelven `false`. Añádelos como en `BlockFlowing.java`/`BlockFire.java` de
   1.5.2 y pon `IMMEDIATE_RANGE = 0` como Java (no 8). Con eso los manantiales de la generación fluyen con retardo 1, como Java.
4. Tu punto 3 ("reprograma con retardo 0 en vez de descartar") en la práctica se descarta igual (func_82740_a repite el
   radio 0). Eso coincide con Java: déjalo, pero corrige el informe.
5. En el informe, "compilación limpia" fue solo de Xbox: dilo así.
Compila las 4 builds con el candado. Señal: `ENCARGO 78b LISTO <commit>`.
