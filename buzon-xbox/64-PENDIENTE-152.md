# 64. Todo lo que falta de la 1.5.2 (29-10)

Rama buena: `port152-claude` en `wt-152-codex` (`0327d35`). Detalle por clases: `60-LO-QUE-FALTA-152.md`.
Guía para revisar contra Java: `55-GUIA-mcp-descompilar-y-revisar.md`.

## Ya hecho (en la rama buena)
Redstone 1.5.2, tolva, vagonetas (tolva/TNT/cofre/horno), raíl activador, placas de peso, sensor de luz,
soltador, templos, cofre extra, biomas grandes, bruja/murciélago, esqueleto Wither, pigman con su skin,
pociones (visión nocturna, invisibilidad...), yunque, faro, marco, libros, mapa vacío, caña con zanahoria,
rendimiento (objetos que se juntan, orbes, caché de portales), pistón y guardado arreglados.

## A medias (WIP sin compilar, retomar cuando haya cuota)
1. **Menú creativo 1.5.2 (12 pestañas) + cofre trampa** — `wt-152-agy`, commit `adbf194` (encargo 58, ver TRASPASO).
2. **Aldeanos: comercio + aldeas del desierto** — `wt-152-pociones`, commit `c746c8f` (encargos 62/63).
   Falta: menú con mando y pantalla partida, `EntityAIVillagerMate`, `VillageSiege`, compilar.

## Sin empezar (orden propuesto)
1. Wither (jefe) + barra de jefe (también para el dragón).
2. Fuegos artificiales.
3. Cuero teñido (y lavarlo en caldero).
4. Bloques: troncos tumbados, nieve por capas, vallas 1,5, trampillas arriba, cacao en jungla,
   manzanas con Fortuna, cama que explota en el Nether, TNT con flecha de fuego, flechas en botones.
5. Mobs pequeños: collar del lobo, colores de oveja al criar, gatitos, XP al criar/pescar/horno,
   mobs con arma encantada, crías con hitbox pequeño, pigmen saliendo de portales.
6. Dispensador 1.5.2 (cubos, fuego, polvo de hueso, armadura, barcas, vagonetas, TNT, huevos, fuegos).
7. Encantamientos: Espinas, encantar libros en la mesa.
8. Reglas de juego (`GameRules`) y modo aventura.
9. Mapas: zoom, clonar, ampliar, marcadores en marcos.
10. Generador de mobs nuevo + vagoneta con generador.
11. Superplano personalizable.
12. Inventario: shift a armadura/horno, repartir pila (adaptado al mando).
13. Pantalla: nombre del objeto al cambiar casilla, calaveras en la cabeza, sombra de invisibles.
14. Barca nueva (rompe nenúfares), protección contra explosiones.

## Problemas abiertos
- Tirón de ~10 s en 92,64,303 tras usar varias veces el menú creativo; tick de EntityPlayerSP 74→250 ms;
  solo ~9 MB libres. Investigar memoria.

## Que tiene que probar el usuario
- Esqueleto Wither con huevo de esqueleto en el Nether, cabaña de bruja, esmeraldas, cuarzo.
- Guardar/cargar con los mobs nuevos.
- Build del 26-10 13:26 (pociones, yunque, faro, marco, libros).
