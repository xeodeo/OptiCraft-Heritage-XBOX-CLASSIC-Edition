# Informe encargo 61 — rendimiento de entidades 1.5.2

Commit: `7f52b98` en `port152-pociones`, basado en `6d20da3` (pociones aceptadas).

## Port y archivos

| Java 1.5.2 | C++ y líneas | Implementación |
|---|---|---|
| `EntityItem` | `wt-152-pociones/src/net/minecraft/src/EntityItem.cpp:29,69,240,251,256,269`; header `EntityItem.h:30` | Watcher 10, getter/setter, mezcla de pilas iguales, búsqueda local, edad/retraso, NBT y edad de despawn creativo. `ensureEntityInit()` es la primera instrucción de ambos constructores. |
| Usos de `EntityItem.item` | `BlockHopper.cpp:128`, `BlockPressurePlateWeighted.cpp:35`, `EntityPickupFX.cpp:42`, `EntityZombie.cpp:102`, `Packet21PickupSpawn.cpp:12`, `RenderItem.cpp:108`, `TileEntityHopper.cpp:304` | Reemplazados por el getter/setter; el `DataWatcher` toma propiedad de la pila y marca el objeto observado al actualizarse. |
| `EntityXPOrb` | `EntityXPOrb.cpp:58`, `EntityXPOrb.h:41` | Mantiene `closestPlayer` y limita su refresco a una ventana cada 20 ticks; solo vuelve a buscar si no hay jugador en caché o salió del radio de 8 bloques. |
| `Teleporter` / `PortalPosition` | `Teleporter.cpp:53,128,148`, `Teleporter.h:15` | Caché por dimensión y coordenadas de entrada, reutilización de portales, expiración a 6000 ticks y poda cada 100 ticks. |
| Ciclo de mundo | `Minecraft.cpp:1794,2495,2632`; `Minecraft.h:294` | Teleporter persistente durante la sesión; se vacía al salir/cargar otro mundo guardado, y las entradas se separan por dimensión. |
| Cambio de `DataWatcher` | `DataWatcher.cpp:127`, `DataWatcher.h:33` | Añade `setObjectWatched`, usado al modificar la pila existente en sitio para que el cambio se replique. |

`EntityItem::travelToDimension` no tiene equivalente en la clase C++ `Entity`; el port cliente tampoco contiene `NetServerHandler`, que en Java llama `setAgeToCreativeDespawnTime`. El setter Java sí está portado, pero ese llamador de servidor queda fuera de este repositorio.

## Comparación Java/C++

`comparar.sh --commit 7f52b98`:

| Clase | Java líneas | C++ `.cpp + .h` | C++/Java |
|---|---:|---:|---:|
| `EntityItem` | 246 | 353 | 143 % |
| `EntityXPOrb` | 237 | 227 | 95 % |
| `Teleporter` | 375 | 361 | 96 % |

También se compararon las clases afectadas: `DataWatcher` 140 %, `BlockHopper` 120 %, `BlockPressurePlateWeighted` 145 %, `EntityPickupFX` 222 %, `EntityZombie` 120 %, `Packet21PickupSpawn` 90 líneas C++ sin Java compartido, `RenderItem` 97 % y `TileEntityHopper` 143 %.

## Coste estimado frente a 1.2.5

Estas cifras salen de los bucles y frecuencias del código, no de un perfil ejecutado en consola.

- **Objetos tirados:** 1.2.5 mantiene los 247 objetos como 247 entidades y actualiza cada entidad cada tick (247 actualizaciones por tick). En 1.5.2 una pila asentada consulta la caja local como máximo cada 25 ticks (0,04 consultas por tick; la frecuencia cae 96 % respecto a consultar cada tick) y las pilas compatibles se consolidan. Si los 247 objetos son del mismo tipo, sin diferencias de daño/subtipo ni NBT, caben en 4 pilas de máximo 64: después de mezclarse quedan 4 actualizaciones de entidad por tick en vez de 247 (98,4 % menos). Objetos distintos o NBT incompatible no se juntan. Mientras cruza bloques, una entidad puede hacer una consulta por tick; la consulta solo compara candidatos dentro de su caja expandida.
- **Orbes XP:** 1.2.5 llama a `getClosestPlayerToEntity` cada tick por orbe (hasta 1 búsqueda/tick/orbe). Ahora la ventana de revisión es una vez cada 20 ticks (máximo 0,05 revisiones/tick, 95 % menos); la búsqueda real solo ocurre si falta el objetivo o queda fuera de 8 bloques, por lo que con un objetivo cercano puede ser menor.
- **Portal:** sin caché, cada fallo de caché recorre X/Z desde -128 a +128 y la altura completa (en este port, 257 × 257 × 128 = 8.454.272 coordenadas consultadas en el peor caso), en cada llamada a `placeInExistingPortal`. Un acierto por dimensión y coordenada de entrada evita ese escaneo completo; se renueva mientras se usa y se elimina al superar 6000 ticks (300 s), con poda cada 100 ticks. Es una cota del código, no un tiempo medido.

No se ejecutó el juego en consola ni emulador; por eso el informe no atribuye milisegundos medidos a la revisión.

## Verificación

- `cmake --build build/xbox-release --target OptiCraft -j 4`: correcto; ISO `bin/xbox/OptiCraft.iso` (14.024.704 bytes).
- `cmake --build build/xbox-release-exp --target OptiCraft -j 4`: correcto; ISO `bin/xbox-exp/OptiCraft.iso` (14.090.240 bytes).
- `git diff --check`: correcto.
- `git diff --stat`: 17 archivos, 223 inserciones y 47 eliminaciones.
- Este worktree no tiene el conjunto `data/assets`; las ISO de build solo contienen los cinco archivos disponibles, incluido `logo3.png`.
