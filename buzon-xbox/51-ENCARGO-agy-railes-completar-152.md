# Encargo 51 (agy): completar raíles y vagonetas (lo que le faltó a Codex, que se quedó sin cuota)

Claude ACEPTÓ el 50 (mobs). Tu rama `port152-inventario` está rebasada sobre `port152-base` (termina en `82be65c`):
ya trae los raíles/vagonetas de Codex (`455d517`), el arreglo del cuelgue del pistón (`3eee9da`) y los sonidos 1.5
(`sound3` en el pak, `63bff49`). No hagas reset ni rebase. Codex está parado: su worktree no lo toques.

Haz la sección "REVISION DE CLAUDE (455d517)" de
`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\49-ENCARGO-codex-railes-152.md` (puntos 1 a 4),
traduciendo del Java 1.5.2 (EntityMinecartTNT, EntityMinecartEmpty, EntityMinecart, RenderTntMinecart,
BlockRailPowered, BlockRailBase). Lo que hay ahora en `EntityMinecartVariants.cpp` es un atajo de Codex.

Reglas de siempre: `setRequiresSelfNotify()` en bloques que cambian metadato; `setBlockMetadataWithNotify152(..., 2)`
donde el Java usa flags 2; iconos con TEX152_*; ojo con `removeBlockTileEntity`: fuera del tick de tile entities BORRA el
objeto (no lo uses después, mira el commit `3eee9da`). Candado BUILD.lock, xbox-release y xbox-release-exp, nunca
/fp:fast, no push, no copies a ClassicXboxLoader.

Al acabar: informe `buzon-xbox\51-INFORME-agy-railes-completar-152.md`, commit, y en tu terminal: `RAILES 152 LISTO <commit>`
