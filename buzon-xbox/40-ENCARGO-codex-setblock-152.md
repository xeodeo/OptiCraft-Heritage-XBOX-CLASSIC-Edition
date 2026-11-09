# Encargo 40 (Codex): API de setBlock de la 1.5.2, sin cambiar el comportamiento

## Contexto
OptiCraft Heritage es una traducción a C++ de Minecraft **Java 1.2.5** (descompilado con MCP).
Vamos a portarlo a **1.5.2**. Lee primero `buzon-xbox\SALTO-1.5.2-21-10.md`.
Fuente Java de referencia (solo lectura):
- 1.2.5: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src125\net\minecraft\src\`
- 1.5.2: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`

## Lo tuyo
- Worktree: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-codex`, rama `port152-base`.
- NO toques otros worktrees, ni `OptiCraftHeritageEdition`, ni `wt-opti30`, ni `wt-152-agy` (ahí trabaja agy).
- No hagas push. Commits locales en `port152-base`, en inglés, estilo de los commits existentes.

## Qué hacer
1. Estudia en `src152\...\World.java` el `setBlock(x,y,z,id,meta,flags)` (línea ~245), `setBlockMetadataWithNotify(x,y,z,meta,flags)`,
   `setBlockToAir`, `markBlockForUpdate`, `notifyBlocksOfNeighborChange(x,y,z,id,exceptSide)` y `func_96440_m` (aviso a comparadores).
   Significado de flags: 1 = avisar a vecinos, 2 = mandar el cambio al cliente/render, 4 = no re-renderizar.
2. **Añade** esa API a `World.h/.cpp` (y lo necesario en `Chunk`) **junto a la vieja**, sin quitar nada.
3. Haz que la API vieja de 1.2.5 llame a la nueva con los flags que dan **exactamente el mismo comportamiento** que ahora.
   Si eso no es posible para alguna, déjala como está y explícalo en el informe.
4. **No migres** todavía las llamadas de los bloques (hay ~450). Eso se hará clase a clase al portar cada bloque.

## TRAMPA (lee esto dos veces)
- En 1.2.5 `setBlock(x,y,z,id)` = SIN aviso. En 1.5.2 `setBlock(x,y,z,id)` = flags 3, CON aviso.
  Mismo nombre, distinto efecto. **No cambies el significado de ninguna firma existente.** Para la nueva de 4 argumentos
  usa otro nombre (p. ej. `setBlockNotify152`) o solo la de 6 argumentos.
- El generador usa `setBlockAndMetadataForPopulation` y rutas propias de la plataforma (Xbox, PS2, Wii): no las toques.
- El terreno de una semilla NO puede cambiar ni un bloque. Nunca `/fp:fast`.
- Respeta las guardas `#if PLATFORM_...` que haya.

## Compilar (una compilación a la vez)
- Candado: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock` (regla en `REGLA-BUILD-LOCK.md`).
  Si existe, espera. Si no, créalo con `codex setblock <hora>`, compila y **bórralo siempre al acabar**.
- Cómo se compila Xbox: mira `buzon-xbox\build_opti30.ps1`. En tu worktree hay que configurar el preset la primera vez:
  `& $cm --preset xbox-release "-DXBOX_DATA_DIR=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data"`
  y luego `& $cm --build --preset xbox-release`. Basta con que compile `xbox-release`; no hace falta xemu.

## Aceptación
- Compila `xbox-release` sin errores ni avisos nuevos (busca también "SSE2 left in").
- `git diff --stat` pequeño y centrado en World/Chunk.
- Una tabla en el informe: cada función vieja -> qué llama ahora y con qué flags, con archivo:línea.

## Al acabar
- Informe: `buzon-xbox\40-INFORME-codex-setblock-152.md`.
- Escribe en tu terminal la línea: `ENCARGO 40 LISTO <commit>`

## REVISION DE CLAUDE (1)
Fallo: `World::setBlock` ahora llama a `setBlockNotify152`, que llama a `setBlockAndMetadata`, que es VIRTUAL.
`WorldClient` sobrescribe `setBlock` y `setBlockAndMetadata` (WorldClient.cpp:1076 y 1084), y las dos meten un
`new WorldBlockPositionType` en `pendingBlockChanges`. En multijugador cada `setBlock` registra el cambio DOS veces.
Arreglo pedido: deja `World::setBlock` con su cuerpo ORIGINAL (como en xbox-opti-30), sin pasar por la API nueva.
Dentro de `setBlockNotify152` llama a `World::setBlockAndMetadata(...)` sin despacho virtual o explica por qué no.
Revisa igual cualquier otra llamada nueva a una función virtual que `WorldClient` sobrescriba. Recompila, commit y señal.
