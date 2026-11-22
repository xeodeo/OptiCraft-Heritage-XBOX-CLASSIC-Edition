# 100 · Informe del subagente: encantamientos y mapas (filas 8 y 9) — 22-11-2026

Worktree `wt-152-sub`, rama `port152-sub` (sobre `a2ec7d1`). Sin push. **No se ha probado en la consola ni en xemu**:
solo compila. Todo lo que sigue hay que probarlo en partida.

## Commits
- `356b334b` 1.5.2 enchanting: Thorns, libros en la mesa, valores 1.5.2
- `06cc60bf` 1.5.2 maps: comprobación de zoom, clonar conserva el mapa, marcas de marco

## Fila 8 — Encantamientos
Lo que había: el sistema de encantar era **todavía el de 1.2.5** (fórmulas, niveles, sin libro). `ItemEnchantedBook` ya estaba
y coincide con Java (brillo, rareza, tooltip, `StoredEnchantments`, uso en el yunque vía `EnchantmentHelper::get/setEnchantments`). No lo toqué.

Portado (Java 1.5.2 → C++):
- `EnchantmentThorns` → `EnchantmentThorns.h/.cpp` (nuevo). Id 7, peso 1, registrado en `Enchantment::initEnchantments`.
  `func_92094_a`, `func_92095_b`, `func_92096_a` igual que Java. Enganchado donde Java lo llama:
  `EntityPlayer` (golpe cuerpo a cuerpo), `EntityMob::attackEntityAsMob`, `EntityArrow` (al tirador).
  `DamageSource::causeThornsDamage` ("thorns", mágico).
- `EnchantmentHelper`: `calcItemStackEnchantability`, `buildEnchantmentList` (enchantability/2, 0.15, bucle desde var7),
  `mapEnchantmentData` (el libro acepta todo), `addRandomEnchantment` (libro → libro encantado), `func_92098_i`, `func_92099_a`,
  `getMaxEnchantmentLevel(id, entidad)`.
- `ItemBook` → `ItemBook.h/.cpp` (nuevo); `Item::book` ahora es `ItemBook(84)`.
- `ContainerEnchantment`: con libro guarda UNA de las tiradas al azar (`func_92115_a`), `addExperienceLevel`, mayús+clic de 1.5.2 (mete 1 objeto en la ranura).
- Valores y `canApply` de 1.5.2: Protection (11 y fórmula float), Damage (11, ×2.75/×4.5, hacha), Digging (+10, tijeras),
  Durability (+8, cualquier objeto dañable, `negateDamage` con 60 % para armadura), LootBonus (15+9), Untouching (15, tijeras).
- `EnchantmentProtection::func_92093_a` (Protección contra fuego acorta el fuego, en `Entity::setFire`) y `func_92092_a`
  (Protección contra explosiones reduce el empuje, en `Explosion`).
- `ItemStack::attemptDamageItem` + `damageItem` de 1.5.2: Irrompible mira el propio objeto (antes miraba el objeto en la mano,
  también para la armadura) y en creativo no se gasta.

Desviaciones (por C++, no inventos de jugabilidad):
- `getLastActiveItems`: en el port los mobs no llevan armadura; jugador = armadura, mob = objeto en la mano.
- Si Espinas rompe una pieza de armadura del jugador se libera la ranura (Java deja una pila de tamaño 0; aquí la ranura es dueña del puntero).
- No hay sonido `damage.thorns` en los datos 1.5.2 del build: el sonido no sonará (no falla).

## Fila 9 — Mapas
Lo que había: `RecipesMapCloning`, `RecipesMapExtending`, `ItemEmptyMap` (escala 0), `RenderItemFrame` dibujando mapas y
`MapData.scale` guardado/cargado ya estaban. Pero `MapData`/`ItemMap` seguían en 1.2.5, con dos fallos:
- **Clonar no servía**: `ItemMap::onCreated` daba un id nuevo a todo mapa fabricado, así que las copias eran mapas vacíos nuevos.
- **Ampliar no comprobaba la escala máxima** (no tenía mundo en `matches`).

Portado:
- `MapData` (1.5.2): `updateVisiblePlayers`, `func_82567_a` (marca fuera del mapa = icono 6, rotación en el Nether),
  `func_82568_a`, marcas con nombre en orden (LinkedHashMap → vector de pares), **marca verde del marco** ("frame-<id>"),
  `updateMPMapData` con el paquete de escala (tipo 2) y el orden de bits de 1.5.2. `MapInfo.field_82569_d`.
- `ItemMap`: apila a 64 y `setHasSubtypes` (como Java 1.5.2; antes 1), centro ajustado a la rejilla, contador de columna por
  jugador, búsqueda de altura de 1.5.2 (no lee y=-1), `onUpdate` → `updateVisiblePlayers`, `onCreated` solo amplía
  (y deja la etiqueta `map_is_scaling`, como Java).
- `RecipesMapExtending.matches(inv, world)` con `scale < 4`: añadido `IRecipe::matches(inv, World*)` (por defecto llama al de
  siempre) y `CraftingManager::findMatchingRecipe(inv, world)`; mesa y inventario pasan su mundo.
- `EntityItemFrame::onUpdate`: cada 10 ticks refresca el mapa del marco para los jugadores a ≤160 bloques (en Java lo hace
  `EntityTrackerEntry`, que aquí no existe). `MapItemRenderer` separa los iconos en Z como 1.5.2.
- Seguridad: `updateVisiblePlayers` descarta jugadores que ya no están en `playerEntities` antes de leerlos (al reaparecer se
  borra el jugador viejo; antes podía leerse memoria liberada).
- `ItemRenderer`: no dibuja si `getMapData` devuelve null (mundo multijugador sin datos, como Java).

No hecho:
- Tooltip «Scaling at 1:N» de `ItemMap.addInformation`: en Java solo sale con F3+H (tooltips avanzados), que aquí no hay.
- `getUpdatePacketData`/`MapInfo.getPlayersOnMap` (lado servidor): el port no tiene servidor.

### Menú de crafteo de consola
Los mapas dinámicos **no encajan**: el resultado depende del mapa concreto de la rejilla. En LCE TU19 están comentados
(`Recipes.cpp` líneas 48-49). `RecipesMapExtending` es un `ShapedRecipes`, así que el menú de consola lo mostraba como un
segundo «mapa vacío» (8 papel + mapa) que nunca salía bien; ahora `XboxCraftingScreen::buildEntries` lo salta.
Clonar y ampliar funcionan en la **interfaz Java** (mesa 3×3 / inventario 2×2). No toqué `LegacyCraftingGroups.h` ni el script.

## Compilación
`build-sub.ps1` (candado BUILD.lock): xbox-release y xbox-release-exp OK, sin errores, 13 avisos «SSE2 left in» en cada una (igual que la base). ISO generado en ambas.

## Qué probar en la Xbox
1. Mesa de encantar con 1 libro → libro encantado con un solo encantamiento; con 2 libros en la pila no deja.
2. Peto con Espinas: un zombi/esqueleto que te pega a veces recibe daño y el peto se gasta más.
3. Copiar mapa (mapa + mapas vacíos) → copias con el mismo dibujo. Ampliar (8 papel + mapa) hasta 4; con escala 4 no sale.
4. Mapa en un marco: marca verde en el mapa que llevas en la mano; tu flecha blanca sigue saliendo.
5. Partidas viejas: mapas y encantamientos cargan igual (mismos nombres NBT).
