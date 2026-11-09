# Informe 42: métodos nuevos de Block de 1.5.2

## Comparación método por método

| Método Java 1.5.2 | Resultado C++ | Ubicación / observación |
|---|---|---|
| `hasComparatorInputOverride()` | Añadido | `Block.h:193`, `Block.cpp:877`; default `false`. Consultado desde los dos setters con flags. |
| `getComparatorInputOverride(World,x,y,z,side)` | Añadido | `Block.h:194`, `Block.cpp:882`; default `0`. |
| `onPostBlockPlaced(World,x,y,z,meta)` | Añadido | `Block.h:185`, `Block.cpp:819`; no-op. |
| `onBlockHarvested(World,x,y,z,meta,player)` | Añadido | `Block.h:186`, `Block.cpp:828`; no-op. |
| `onSetBlockIDWithMetaData(World,x,y,z,meta)` | Añadido | `Block.h:187`, `Block.cpp:839`; no-op. |
| `fillWithRain(World,x,y,z)` | Añadido | `Block.h:188`, `Block.cpp:848`; no-op. |
| `isFlowerPot()` | Añadido | `Block.h:189`, `Block.cpp:856`; default `false`. |
| `func_82506_l()` | Añadido | `Block.h:190`, `Block.cpp:861`; default `true`. |
| `canDropFromExplosion(Explosion)` | Añadido | `Block.h:191`, `Block.cpp:866`; default `true`. |
| `isAssociatedBlockID(id)` de instancia | Añadido | `Block.h:192`, `Block.cpp:872`; default compara `blockID`. |
| `isAssociatedBlockID(id1,id2)` estático | Añadido | `Block.h:196`, `Block.cpp:892`; conserva el caso de IDs iguales y comprueba registros válidos antes de indexar la tabla de bloques. |
| `isProvidingStrongPower(...)` | Ya existe como `isIndirectlyPoweringTo(...)` | `Block.h:161`, `Block.cpp:740`; no duplicado, según la correspondencia indicada en el encargo. |
| `tickRate(World)` | Añadido como overload | `Block.h:184`, `Block.cpp:813`; el antiguo `tickRate()` sigue en `Block.h:112`, `Block.cpp:503`; default nuevo `10`. |
| `onBlockEventReceived(...)` | Ya existe como `powerBlock(...)` | `Block.h:160`, `Block.cpp:733`; no duplicado, según la correspondencia indicada en el encargo. |

El helper pedido `Block::isComparatorId(int)` se añadió en `Block.h:198` y `Block.cpp:903`. Devuelve `false` hasta que se registre/porte el bloque comparador.

## Avisos a comparadores

`World::func_96440_m` (`World.cpp:1779`) recorre los cuatro lados horizontales y revisa también detrás de un bloque normal, siguiendo la lógica de Java 1.5.2. En `setBlockNotify152` (`World.cpp:1175`) y `setBlockMetadataWithNotify152` (`World.cpp:1566`) solo se invoca cuando el bloque modificado existe y `hasComparatorInputOverride()` es true. Como no hay bloques comparadores portados, `isComparatorId` mantiene la ruta inactiva.

No se añadieron call sites para los demás hooks. No se tocaron subclases de `Block` ni APIs de texturas/registro de iconos.

## Vtable y plataformas

Se añadieron 11 métodos virtuales después de los virtuales antiguos, incluso después del destructor, para conservar los índices de las entradas virtuales existentes. No se añadieron campos de instancia: el tamaño de los objetos `Block` no cambia. Las tablas estáticas de registros (`blocksList` y demás) tampoco cambian de tamaño. Las vtables y el código sí crecen; PS2/Wii deben recompilarse para este cambio. No se ejecutaron builds PS2/Wii en este encargo.

## Verificación

- `xbox-release`: compiló y enlazó correctamente; se generó `OptiCraft.iso`.
- `World.cpp` y `Block.cpp` no emitieron avisos del compilador.
- El enlace mostró los avisos existentes del XDK y `SSE2 left in` en símbolos CRT/librería estándar; no aparecen símbolos de `Block.cpp`/`World.cpp` en esos avisos SSE2.
- `git diff --check`: sin errores.
- `BUILD.lock`: eliminado al terminar.
