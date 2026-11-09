# Encargo 61 (Codex): rendimiento de la 1.5.2 — objetos sueltos, orbes y portales

Claude ACEPTÓ el 59 (pociones, `d813e7e`): entra en la rama buena. Gracias.

## Tu sitio
- Mismo worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-pociones`, rama `port152-pociones`.
  Antes de empezar, con el árbol limpio: `git reset --hard port152-claude` (ya contiene tus pociones como `6d20da3`).
- NO toques `wt-152-codex` (rama de Claude) ni `wt-152-agy`.
- agy está con yunque/faro/marco/libros y luego menú creativo/cofre trampa: no toques esas clases.
- Guía: `buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`. Java 1.5.2: `mcp-diff\src152\net\minecraft\src\`.

## Problema medido en la consola (log de red, 1.2.5 y 1.5.2)
- `entTick=41.5ms entities=292 top: EntityPlayerSP 29.1ms EntityItem x247 9.6ms`: con 247 objetos tirados el tick
  del mundo pasa de 50 ms. En la 1.5.2 los objetos iguales cercanos se JUNTAN en una pila.
- Al cruzar el portal del Nether hay tirones de varios segundos.

## Qué portar (traducción fiel del Java 1.5.2)
1. `EntityItem` 1.5.2: el `ItemStack` en el DataWatcher (índice 10, `getEntityItem`/`setEntityItemStack`),
   `searchForOtherItemsNearby` + `combineItems` (juntar pilas iguales cercanas, edad y `delayBeforeCanPickup`),
   y lo demás que cambie en la clase. Cuidado: `EntityItem::item` se usa en muchos sitios (render, recoger, tolva,
   NBT `Item`): búscalos todos. `ensureEntityInit()` en el constructor (guía 55).
2. `EntityXPOrb` 1.5.2: busca jugador cada 20 ticks (`xpTargetColor`/`closestPlayer`), no cada tick.
3. `Teleporter` 1.5.2: caché `destinationCoordinateCache` de portales (300 s, `removeStalePortalLocations`),
   `PortalPosition`. Que se limpie al cambiar de mundo.
4. Mide en el informe cuánto cuesta cada cosa (complejidad, nº de búsquedas por tick) frente a 1.2.5.

## Reglas
Las del 59: candado `buzon-xbox\BUILD.lock`, compilar `xbox-release` y `xbox-release-exp`, nunca /fp:fast,
no push, no ClassicXboxLoader, no xemu. Nada de métodos de relleno.

## Al acabar
Informe `buzon-xbox\61-INFORME-codex-rendimiento-entidades-152.md` y en tu terminal: `ENCARGO 61 LISTO <commit>`.
ESCRIBE el informe: en el 59 no lo hiciste.
