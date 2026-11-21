# 91. Informe: vagonetas con TNT — rendimiento `O(N²)`, explosiones, fuga de memoria y cuelgue por OOM

- **Rama 1.5.2**: `port152-erluza` en `wt-152-rend` (sobre `5b6f332` → **`1971302`**):
  - `1971302` — `fix(perf,memory): eliminate minecart O(N^2), TNT cart leak/recursion, explosion NaN hang, and OOM deadlock`
- **Rama 1.2.5**: `opti30-erluza` en `wt-opti30` (sobre `e1f6bb7` → **`d772f40`**):
  - `d772f40` — `fix(perf,memory): eliminate minecart O(N^2), explosion air-bloat/NaN hang, and OOM deadlock` *(todos los arreglos genéricos; `EntityMinecartTNT` no existe en 1.2.5).*
- **Reglas respetadas**: sin `reset`, sin `push`, sin copiar al lanzador, sin tocar Creativo ni worldgen, `BUILD.lock` respetado y liberado, 0 avisos nuevos de SSE2 (`sseLeft=13` idéntico al baseline), `ps2-release` compilado, sonda de stress eliminada (`hasProbe=False` verificado en los 4 XBE).

---

## Causas raíz encontradas en `91-netlog-vagonetas-tnt-20-11.txt` y GDB (`archivo:línea`)

### 1. 134 vagonetas TNT → `38 ms` (paradas) y `98 ms` (apiladas en movimiento) → 4 fps
1. **Doble escaneo `O(N²)` + `unordered_set::find` + `getParts()` en `Chunk::getEntitiesWithinAABBForEntity`** (`src/net/minecraft/src/Chunk.cpp:1392-1443`):
   - Cada una de las 134 vagonetas consultaba la misma sección del chunk hasta 2 veces por tick (`moveEntity` → `World::getCollidingBoundingBoxes` + `EntityMinecart::onUpdate`), haciendo `134 × 134 × 2 ≈ 35 912` búsquedas `worldObj->isLoadedEntityPointer(candidate)` (`std::unordered_set::find`) y `35 912` llamadas virtuales `candidate->getParts()` (que construían/destruían un `std::vector<Entity*>` vacío en cada entidad).
   - Además `Entity::moveEntity` (`src/net/minecraft/src/Entity.cpp:725-777`) lanzaba el barrido completo de colisiones incluso cuando el desplazamiento era `(0, 0, 0)` (vagoneta parada en raíl).
2. **`EntityMinecart::getCollisionBox` devolvía la caja de ítems y `World::getCollidingBoundingBoxes` metía 133 cajas por vagoneta** (`EntityMinecart.cpp:168-171`, `World.cpp:3045-3063`):
   - `getCollisionBox` devolvía `entity->boundingBox` sin comprobar `entity->canBePushed()` (colisionando incluso contra los 62 `EntityItem`).
   - En una pila de 134 vagonetas, `moveEntity` iteraba 133 AABBs × 3 ejes por vagoneta (`~53 466` pruebas/tick).
3. **Por qué saltaba de `38 ms` a `98 ms` al moverse/separarse ligeramente** (`EntityMinecart.cpp:632-641,853-861`):
   - Cuando estaban exactamente en el mismo punto (`d2 < 1e-4`), `applyEntityCollision` salía pronto; al moverse en fila/bloque (`d2 >= 1e-4`), cada par ejecutaba `134 × 133 = 17 822` llamadas/tick a `applyEntityCollision`, con **dos `dynamic_cast<EntityMinecart*>`** y **cuatro `Vec3D::createVector(...)` por par** = **~71 288 objetos `Vec3D` en un solo tick**, desbordando `Vec3D::POOL_SIZE = 1024` y creando decenas de miles de entradas en `g_vec3DOverflowPool` (que nunca se recortaba en partida).

### 2. Explosión única de `782.7 ms` y fuga de `1 548 KB` en un solo estallido (`L2839→L2894`)
1. **Fuga de memoria de `Explosion` exclusiva de `EntityMinecartTNT`** (`src/net/minecraft/src/EntityMinecartTNT.cpp:72`):
   - Era el **único** llamador de `worldObj->createExplosion(...)` en todo el proyecto que **no hacía `delete`** sobre el `Explosion *` devuelto (todos los demás —`EntityTNTPrimed`, `EntityCreeper`, `BlockBed`, `EntityWither`, etc.— sí hacen `delete`).
2. **Inserción de decenas de miles de bloques de AIRE en `destroyedBlockPositions` + partículas/rayos masivos** (`src/net/minecraft/src/Explosion.cpp:124-133,159-180,202-246`):
   - Una vagoneta TNT rápida alcanza potencia hasta `11.5`; los 1 352 rayos metían en `JavaHashSet<ChunkPosTuple> destroyedBlockPositions` **todos los bloques de aire** (`i5 == 0`) incluso con `isFlaming == false` (hasta ~30 000 entradas ≈ **1,5 MB** fugados por cada estallido + ~60 000 partículas creadas y descartadas en `doExplosionB`), mientras `getBlockDensity` lanzaba 8–27 `rayTraceBlocks` por cada uno de los 107 `EntityItem` presentes.

### 3. Explosiones en cadena recursivas, bucle infinito por `NaN` en `Explosion::doExplosionA` y cuelgue por OOM (`L3791→L3817`)
1. **Recursión síncrona de hasta 134 explosiones + `setEntityDead()` tarde** (`EntityMinecartTNT.cpp:51-75`):
   - `explodeCart()` llamaba a `createExplosion()` antes de `setEntityDead()`, y `killMinecart()` volvía a llamar a `explodeCart()` síncronamente dentro de `Explosion::doExplosionA()`, apilando explosiones anidadas en el mismo tick.
2. **División por cero `0/0 = NaN` + bucle infinito `while (true) { if (f1 <= 0.0f) break; }`** (`Explosion.cpp:144-169`, verificado con GDB en `xemu` en `Explosion::doExplosionA+0x155`):
   - Cuando dos vagonetas apiladas en el mismo punto detonaban, `d11 == 0.0` producía `d6/d11 = NaN` en `motionX/Y/Z`. Al detonar la siguiente vagoneta con `speedSq = NaN`, `speed > 5.0` daba `false`, `explosionSize` y `f1` quedaban en `NaN` y **`if (f1 <= 0.0f) break;` nunca se cumplía**, recorriendo el rayo infinitamente e insertando bloques en `destroyedBlockPositions` hasta devorar los ~20 MB libres en un solo frame (`freeKB: 10848 → 8 KB → std::bad_alloc`).
3. **Recuperación de `std::bad_alloc` atascada en pantalla roja (`freeKB=0`)** (`src/client/Minecraft.cpp:1156-1214,3010-3014`):
   - `freeMemoryForCrash()` llamaba a `resetVectorPool()`/`resetBoundingBoxPool()` (que **no** liberaban los overflow pools), no vaciaba `effectRenderer` ni la caché de texturas decodificadas, y llamaba a `changeWorld1(nullptr)`: esto intentaba **guardar todos los chunks en Anvil** con 8 KB libres (`Failed to queue Anvil chunk: bad allocation`) y luego **difería** el `delete oldWorld` a `worldsToDelete` sin liberar un solo byte antes de `displayGuiScreen(new GuiMainMenu())`.
   - Al lanzar `std::bad_alloc` ese `new GuiMainMenu()` fuera del `try` interior, saltaba al `catch` externo → `onMinecraftCrash` → bucle infinito `for (;;)` de `ClientPlatformPolicy_XBOX.cpp:72`.

---

## Arreglos aplicados (`1971302` en 1.5.2 / `d772f40` en 1.2.5)

1. **Colisiones y consultas de entidades `O(N)`**:
   - `Entity.h`, `EntityDragon.h`, `EntityMinecart.h` (1.2.5): `isMinecart()` y `hasParts()` virtuales `O(1)`.
   - `World.h`, `World.cpp`, `Chunk.h`, `Chunk.cpp`: contador `entityUntrackGeneration` + caché `entitySectionValidatedGen/Rev` por sección en `Chunk::getEntitiesWithinAABBForEntity` (mantiene 100 % la purga segura de punteros obsoletos sin repetir `unordered_set::find` cuando la sección no ha cambiado, y omite `getParts()` salvo en el dragón).
   - `Entity.cpp`: en `moveEntity`, si `(d, d1, d2) == (0, 0, 0)` omite el barrido de colisiones de bloques/entidades (mantiene intactos `onEntityCollidedWithBlock`, fuego y lava).
   - `EntityMinecart.cpp` y `World.cpp`: `getCollisionBox` comprueba `entity->canBePushed()`; en pilas densas se limita a 8 colisiones vagoneta-vagoneta por tick (rotando índice de inicio para empuje simétrico) y 8 AABBs vagoneta-vagoneta en `getCollidingBoundingBoxes`; `applyEntityCollision` usa `isMinecart()` y producto escalar 2D escalar (0 `Vec3D::createVector`, 0 `dynamic_cast`); guarda `std::isfinite` en velocidades.
2. **Explosiones rápidas, sin fugas ni bucles infinitos**:
   - `EntityMinecartTNT.h/.cpp` (1.5.2): bandera `hasExploded`, `setEntityDead()` **antes** de `createExplosion`, `delete worldObj->createExplosion(...)`, guardas `std::isfinite` en posición y velocidad, reacción en cadena con mecha corta (`1..4` ticks) en vez de recursión síncrona, y tope de **2** detonaciones de vagoneta TNT por tick en Xbox.
   - `Explosion.cpp`: caché de último `Chunk *` en el barrido de 1 352 rayos; no insertar bloques de aire en `destroyedBlockPositions` salvo si `isFlaming` sobre bloque sólido; guardia `std::isfinite` + `while (f1 > 0.0f && ++raySteps <= 256)`; guardia `if (d11 > 0.0)` contra `0/0 = NaN`; rayo único central para ítems pequeños o multitudes (`> 24` entidades) en Xbox; partículas escalonadas (~48 pares/explosión) y recorte de partículas/drops si `freeKB < 3072`.
3. **Memoria acotada, alivio proactivo y recuperación real ante OOM**:
   - `Vec3D.h/.cpp` y `AxisAlignedBB.h/.cpp`: `initialize()` / `clearBoundingBoxPool()` recortan los overflow pools a 256 entradas al final del tick si hubo picos; `resetVectorPool()`/`resetBoundingBoxPool()` llaman a `trim*()`.
   - `EffectRenderer.cpp`, `AnvilChunkLoader.cpp`, `ChunkProvider.cpp`, `Minecraft.cpp`: bajo memoria crítica en Xbox (`< 4 MB` / `< 3 MB` / `< 1.5 MB`), purga proactiva de partículas, caché de texturas y pools, descarga de emergencia de chunks fuera de radio, omisión de nueva generación/partículas/encolado Anvil para evitar `bad_alloc`.
   - `Minecraft.cpp` (`freeMemoryForCrash` y `catch (std::bad_alloc)`): libera primero `effectRenderer`, `renderGlobal`, caché de texturas y pools; si hay memoria crítica descarta `theWorld` sin intentar serializar Anvil y vacía `worldsToDelete`/`playersToDelete` **de inmediato** antes de crear `GuiMainMenu` dentro de un `try/catch` que impide caer en el cuelgue de `onMinecraftCrash`.

---

## Resultados medidos antes vs. después (`xemu` 64 MB, 1.5.2)

| Escenario | Antes (consola real `501b40d`) | Después (`xemu` 64 MB `1971302`) |
|---|---|---|
| **134 `EntityMinecartTNT` paradas en raíl** | `EntityMinecartTNT x134`: `38.1 ms/tick` (luego `98.2 ms` al desplazarse → **4 fps**) | `EntityMinecartTNT x134`: **`8.7 ms/tick`** (`0.065 ms`/vagoneta, **4,4×–11,3× más rápido**), `freeKB=37 884` estable |
| **Explosión individual de vagoneta TNT** | **`782.7 ms`** + **`1 548 KB`** fugados permanentemente | **`19.3–26.4 ms`** en cadena (pico inicial `83.0 ms` excavando el primer cráter), **0 KB fugados** |
| **Detonación en cadena de las 134 vagonetas TNT** | `watchdog 2000ms+`, `freeKB: 10 848 → 8 → 0`, **`bad allocation` + cuelgue permanente** | Las 134 detonan `134 → 64 → 24 → 0` en ~3 s (`60` ticks), **`freeKB` mínimo `34 384 KB`**, 0 `watchdog`, 0 `NaN`, 0 `[X87]` |
| **Falta de memoria forzada (`std::bad_alloc`)** | `Failed to queue Anvil chunk` × 10 → `Unexpected error: bad allocation` → cuelgue infinito (`freeKB=0`) | Libera memoria al instante (`34 384 → 38 740 KB`), vuelve a `GuiMainMenu` y sigue a **`88–92 fps`** (>1 500 frames sin cuelgue) |

---

## Compilación final limpia y arranque en `xemu` 64 MB

- **1.5.2 (`port152-erluza` `1971302`)**:
  - `xbox-release`: `rc=0`, `sseLeft=13` (0 nuevos), XBE `4 800 512 B` (`hasProbe=False`), `selftest: 0 failure(s)`, `double53=1`, `freeKB=45552`, `0` errores/`nan`/`[X87]`.
  - `xbox-release-exp`: `rc=0`, `sseLeft=13`, XBE `4 812 800 B` (`hasProbe=False`), `selftest: 0 failure(s)`, `double53=1`, `freeKB=45540`, `0` errores/`nan`/`[X87]`.
- **1.2.5 (`opti30-erluza` `d772f40`)**:
  - `xbox-release`: `rc=0`, `sseLeft=13`, XBE `4 358 144 B` (`hasProbe=False`), `selftest: 0 failure(s)`, `double53=1`, `freeKB=46328`, `0` errores/`nan`/`[X87]`.
  - `xbox-release-exp`: `rc=0`, `sseLeft=13`, XBE `4 374 528 B` (`hasProbe=False`), `selftest: 0 failure(s)`, `double53=1`, `freeKB=46308`, `0` errores/`nan`/`[X87]`.
  - `ps2-release`: `rc=0`.

---

## Qué probar en la Xbox real

1. En **1.5.2**, colocar **130+ vagonetas con TNT** juntas sobre un raíl: comprobar que paradas y al empujarlas el juego mantiene fluidez sin caer a 4 fps y que `freeKB` no cae progresivamente.
2. Hacer detonar la pila entera (empujándolas contra un bloque, con fuego o raíl activador): comprobar que estallan en cadena de forma fluida sin congelar el juego ni agotar la memoria.
3. En **1.2.5**, comprobar también vagonetas normales apiladas y explosiones de TNT/Creepers para confirmar la mejora compartida de colisiones, explosiones y estabilidad de memoria.
