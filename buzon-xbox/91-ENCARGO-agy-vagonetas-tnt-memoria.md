# 91. Encargo agy: vagonetas con TNT — 4 fps y cuelgue por falta de memoria (1.5.2) (20-11)

Hazlo DESPUÉS del 90. Rama `port152-erluza` (`wt-152-rend`); si el arreglo es genérico, pásalo también a `opti30-erluza`
(`wt-opti30`, la 1.2.5 tiene vagoneta con TNT? NO: llegó en 1.5; mira solo lo que aplique). Sin reset, sin push, sin
copiar al lanzador. Reglas de siempre (BUILD.lock, 60 fps, memoria, creativo no se toca, terreno igual).

## Lo medido en la consola real (netlog completo: `buzon-xbox\91-netlog-vagonetas-tnt-20-11.txt`)
- Build 1.5.2 `501b40d`. El usuario puso **134 `EntityMinecartTNT`** (más 62 objetos, 11 aldeanos...).
- `xbox.ent`: con 134 vagonetas TNT `entTick` 38 ms al principio y luego **~98 ms** (0,73 ms cada una) → 4 fps,
  `ticks=220ms (2 por frame)`. En Java una vagoneta parada es barata: busca por qué la nuestra cuesta tanto
  (¿colisiones vagoneta-vagoneta O(n²) en `EntityMinecart.onUpdate` / `getEntitiesWithinAABBExcludingEntity`, empuje
  `applyEntityCollision`, raíles, `getCollisionBox`, sin la caché de vecinos?). La subida de 38 a 98 ms pasa sin que
  cambie el número: ¿se apilan en el mismo bloque?
- Una sola vagoneta TNT llegó a costar **782 ms** en un tick (explosión): mira esa explosión (rayos, lista de bloques,
  partículas, entidades afectadas, sonido).
- **Memoria**: `freeKB` bajó de ~24 MB a ~20 MB en frames 25k-30k y en frame 32379 cayó a **0**:
  `[memory] out of memory: free=8KB`, `Failed to queue Anvil chunk ...: bad allocation`, `[crash] Unexpected error:
  bad allocation`, y el hilo principal se quedó trabado para siempre (`watchdog ... freeKB=0`). Busca qué se come la
  memoria (¿colas de explosiones, partículas, listas de colisiones por tick, AABB del pool que no se devuelven,
  `ItemStack` de las vagonetas rotas?). Aldeanos también tienen picos de 45-60 ms (búsqueda de camino).
- El juego no debe colgarse por falta de memoria: si una asignación falla, degradar (no generar ese chunk, soltar
  partículas, purgar cachés) en vez de quedarse muerto.

## Aceptación
- Reproducir en xemu 64 MB: 130+ vagonetas TNT juntas (y en raíles), detonar unas cuantas; medir `entTick`, fps y
  `freeKB` antes/después. Objetivo: que 134 vagonetas paradas no bajen de ~30 fps y que la memoria no se agote.
- Que una explosión de vagoneta no se lleve cientos de ms.
- Sin cuelgue aunque se fuerce poca memoria.

## Informe
`buzon-xbox\91-INFORME-agy.md` (causa con archivo:línea, arreglo, números antes/después). Señal: `ENCARGO 91 LISTO`
+ commit(s) final(es).
