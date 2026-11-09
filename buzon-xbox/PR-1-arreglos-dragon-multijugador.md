Título:
fix: el Ender Dragon se congela fuera de la zona cargada y cuelgue del multijugador al dibujar el objeto de la mano

Descripción:

Hola! Soy xeodeo, hice el port de OptiCraft Heritage a la **Xbox original** (https://github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition). Encontrando fallos en la consola real aparecieron dos que no son de Xbox, sino del código común. Van aquí por si os sirven en PS2, Wii y PC.

### 1. El Ender Dragon se queda congelado en el aire (`World.cpp`)
- La retención de chunks por entidad (`PLATFORM_ENTITY_CHUNK_RETENTION`) solo mantenía cargado **el chunk donde está el dragón**.
- Pero una entidad solo se actualiza si existen los chunks a `PLATFORM_PLAYER_UPDATE_CHUNK_RANGE_BLOCKS` a su alrededor. En cuanto el dragón salía de la zona cacheada, los chunks vecinos no estaban y se quedaba quieto.
- Arreglo: se retiene y se carga **todo el cuadrado del radio** de la entidad (3x3 para el dragón), tanto en `ensureEntityChunkRetention` como en `isChunkRequiredByRetainedEntity`.
- Afecta a todas las plataformas que usan esa retención (PS2, Wii).

### 2. Cuelgue en multijugador (`ItemRenderer.cpp`)
- `itemToRender` guarda un puntero al `ItemStack` de la mano. En un servidor, los paquetes de inventario sustituyen y **borran** los `ItemStack`, y en el siguiente frame se leía el NBT de uno ya liberado: `hasEffect` → `isItemEnchanted` → `NBTTagCompound::hasKey`, y cuelgue.
- Arreglo: solo se usa el puntero mientras el inventario del jugador lo siga teniendo. Se comparan direcciones y nunca se lee el objeto.
- Encontrado con un servidor 1.2.5, con la dirección del fallo resuelta contra el `.map`.

### Probado
- En la Xbox real y en xemu.
- Esta rama compila en PS2 (`ps2-release`).
- Son 2 ficheros y unas 40 líneas, sin cambios de comportamiento fuera de esos dos casos.

Gracias por el proyecto!
