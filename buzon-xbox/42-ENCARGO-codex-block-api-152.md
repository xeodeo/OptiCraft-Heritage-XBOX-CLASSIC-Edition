# Encargo 42 (Codex): métodos nuevos de Block de la 1.5.2, sin cambiar el comportamiento

Sigue en tu worktree `wt-152-codex`, rama `port152-base`, encima de tu `8656556` (aceptado por Claude).
Mismas reglas que el encargo 40: candado BUILD.lock, no push, no tocar otros worktrees, nunca /fp:fast,
el terreno de una semilla no puede cambiar.

Referencia: `mcp-diff\src125\...\Block.java` contra `mcp-diff\src152\...\Block.java` (`diff -w`).

## Qué hacer
1. En `Block.h/.cpp` añade como métodos virtuales, con el cuerpo por defecto EXACTO de la 1.5.2, solo los que
   NO existen en nuestro C++ con otro nombre. Candidatos (compruébalo uno a uno):
   `hasComparatorInputOverride`, `getComparatorInputOverride`, `onPostBlockPlaced`, `onBlockHarvested`,
   `onSetBlockIDWithMetaData`, `fillWithRain`, `isFlowerPot`, `func_82506_l`, `canDropFromExplosion`,
   `isAssociatedBlockID` (las dos), `isProvidingStrongPower`, `tickRate(World*)`, `onBlockEventReceived`.
   Si uno ya existe con otro nombre (p. ej. en 1.2.5 `isIndirectlyPoweringTo` ≈ `isProvidingStrongPower`,
   `powerBlock` ≈ `onBlockEventReceived`), NO lo dupliques: apúntalo en la tabla del informe como renombre.
2. **No los llames todavía** desde ningún sitio nuevo, salvo `hasComparatorInputOverride` en el punto 3.
3. Implementa `World::func_96440_m` como en la 1.5.2 (World.java, `func_96440_m`, ~línea 2776) y que
   `setBlockNotify152` / `setBlockMetadataWithNotify152` solo lo llamen si `hasComparatorInputOverride()` del bloque,
   igual que en Java. Como no hay comparador todavía, deja un `Block::isComparatorId(int)` que devuelva false
   (o el equivalente) con un comentario de que lo activará el bloque comparador.
4. NO toques: el sistema de texturas/Icon/registerIcons, los nombres de métodos existentes, las subclases de Block.

## Aceptación
- Compila `xbox-release` sin errores ni avisos nuevos (también "SSE2 left in").
- Tabla en el informe: método 1.5.2 -> "añadido" (archivo:línea) o "ya existe como X" (archivo:línea).
- Ojo con la vtable: si añadir virtuales a Block cambia algo en PS2/Wii (tablas estáticas, tamaños), dilo.

## Al acabar
- Informe `buzon-xbox\42-INFORME-codex-block-api-152.md`, commit, y en tu terminal: `ENCARGO 42 LISTO <commit>`
