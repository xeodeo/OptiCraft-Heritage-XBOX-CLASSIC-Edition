# 86. Encargo agy: mobs, objetos y vagonetas cruzan los portales (1.5.2) (09-11)

Pedido del usuario: «¿el aldeano debería entrar al portal? no pasa». En Java 1.5.2 cualquier entidad cruza.

## Dónde
- Worktree `wt-152-rend`, rama NUEVA `port152-portales` (ya creada por Claude desde `ce80a3b`). SIN reset.
- Solo 1.5.2 (la 1.2.5 no lo tenía en Java). NO toques otras carpetas.

## Estado medido por Claude
- `Entity::setInPortal()` está VACÍO (`src/net/minecraft/src/Entity.cpp` ~1641). Java: `Entity.java` 1.5.2 líneas
  ~210-245 (`inPortal`, `field_82153_h`, `getMaxInPortalTime`, `getPortalCooldown`, `timeUntilPortal`) y
  `travelToDimension` (~1528), `Teleporter` (`placeInExistingPortal`, `makePortal`, `func_85188_a`),
  `ServerConfigurationManager.transferEntityToWorld`.
- Nuestro port solo tiene cargado el mundo donde está el jugador (viene de 1.2.5). El jugador cambia de dimensión con
  `Minecraft::usePortal` (`src/client/Minecraft.cpp` ~2520), que guarda el mundo y abre el otro.

## Qué hacer
1. Contador de portal como Java para todas las entidades (no jugadores): al cumplir el tiempo, la entidad se
   transfiere a la otra dimensión.
2. Transferencia sin cargar el otro mundo entero: calcula la posición destino como Java (÷8 / ×8 y el Teleporter:
   busca un portal existente en la otra dimensión leyendo sus chunks guardados cerca de la posición; si no hay, usa la
   posición calculada y deja una marca para crear el portal cuando el jugador llegue, o crea el portal escribiendo los
   bloques en el chunk guardado — elige lo más seguro y explícalo). Guarda la entidad (NBT completo con `writeToNBT`,
   nuevo `Dimension`) en el chunk destino de la región de la otra dimensión (`DIM-1` / raíz), usando el cargador de
   chunks Anvil del port, y quítala del mundo actual. Si el chunk destino no existe aún, guárdala en una cola por
   dimensión (fichero en la carpeta del mundo) que se vacía al generar/cargar ese chunk.
3. Al entrar el jugador en la otra dimensión, las entidades aparecen donde deben. Ida y vuelta varias veces sin
   duplicar ni perder (aldeano con ofertas, lobo domado con dueño y collar, objetos, vagoneta con cofre).
4. Jinetes/montura: como Java (no cruzan si `ridingEntity != null`). El dragón y el Wither no cruzan (Java tampoco del End).

## Reglas
- `ensureEntityInit`, sin `delete` de cajas del pool, dueños de ItemStack/NBT, sin punteros a entidades borradas.
- No tocar la generación del mundo. 60 fps intactos (nada de trabajo por tick si no hay entidades en portales).
- Guardado robusto: si la consola se apaga a mitad, no se corrompe la región.
- `BUILD.lock`; `cmake --build build/xbox-release` y `build/xbox-release-exp` en `wt-152-rend` (reconstruye limpio si hay
  objetos de otra rama). NO copies al lanzador.
- Informe `buzon-xbox\86-INFORME-agy-portales.md` (diseño, archivo:línea, qué probar). Señal: `ENCARGO 86 LISTO <commit>`
