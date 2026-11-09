# 77. Semilla 1621073931: océano en Xbox, tierra con cabaña de bruja en Java (09-11-2026)

## Causa
El generador NO estaba mal: los biomas son los de Java 1.5.2 (capas de `GenLayer.cpp` iguales a
`GenLayer.initializeAllBiomeGenerators`; en Xbox `PLATFORM_FAST_BIOME_SOURCE` = 0, se usa la cadena real).
En Java esta semilla también da **océano en 0,0** y `findBiomePosition` devuelve null. Java entonces camina hasta
1000 pasos al azar (±64) buscando césped (`canCoordinateBeSpawn`) y llega a tierra al oeste.
En Xbox (`PLATFORM_BOUNDED_WORLD`) el paseo se cortaba a **8 intentos** → se quedaba en el mar.

## Arreglo (`ac33f80`, rama port152-aldeanos, `World::generateSpawnPoint`)
- Mundo infinito: el mismo paseo de Java (mismo `Random(seed)`, mismo orden de `nextInt`), hasta 1000 pasos.
  Los pasos en océano / océano helado se descartan con el mapa de biomas (no pueden tener césped), sin generar chunk.
  Los pasos en tierra centran el proveedor en ese chunk (`setCurrentChunkOver`, si no el chunk "no puede existir")
  y prueban `canCoordinateBeSpawn`. Tope: 32 chunks generados; si no hay césped, el primer punto de tierra.
- Mundo limitado (isla): igual que antes (8 intentos, ±64).
- Log: `world spawn x,z found= chunkProbes=` en el netlog.

## Prueba (Java 1.5.2, arnés `mcp-diff/arnes152/Arnes.java` y `Paseo.java`)
- 1621073931: bioma en 0,0 = Ocean; spawn por biomas = null; paseo: intentos 0-12 en océano, tierra desde el 13
  (río -116,95), pantano en el 22 (-174,83). Cabaña posible en el chunk (-11,4) = bloques -176,64 (pantano).
- 12345, 404, -4172144997902289642: `findBiomePosition` encuentra tierra (spawn en el intento 0) → sin cambios.
- Falta verlo en consola: crear mundo con 1621073931 y mirar el `world spawn` del netlog.
