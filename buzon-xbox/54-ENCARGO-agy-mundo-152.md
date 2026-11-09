# Encargo 54 (agy): grupo 4 — el mundo de la 1.5.2

Claude ACEPTÓ el 52 (comparador, tolva, soltador, repetidor). Rama `port152-inventario` en `cf6876d`.
No hagas reset ni rebase. Codex trabaja a la vez en `wt-152-codex` en menús (yunque, faro, libros, marco): no toques
eso ni ese worktree.

## Qué portar (traducción del Java 1.5.2)
1. Alturas de biomas 1.5.2 (`BiomeGenBase`: Extreme Hills, Ice Mountains, DesertHills, ForestHills, TaigaHills,
   JungleHills; ver `buzon-xbox\SALTO-1.5.2-21-10.md`) y `BiomeGenHills.decorate` con esmeraldas y lepismas.
   Los murciélagos en `spawnableCaveCreatureList` ya los añadiste en el 50: compruébalo.
2. `MapGenScatteredFeature` + `StructureScatteredFeatureStart` + `ComponentScatteredFeature*`: pirámide del desierto
   (con su trampa de TNT y cofres), templo de la jungla (palancas, cable trampa, dispensadores con flechas, cofres) y
   cabaña de bruja (con su bruja que aparece ahí). Engánchalo en `ChunkProviderGenerate` como en 1.5.2
   (`generate` + `generateStructuresInChunk` + `recreateStructures` + lista de spawn de la cabaña).
   `WeightedRandomChestContent` para el botín.
3. Nether: mena de cuarzo en `ChunkProviderHell` (WorldGenMinable con netherrack como bloque reemplazable, ya admite
   el tercer parámetro) y los cambios de la fortaleza 1.5.2 (`MapGenNetherBridge`/`ComponentNetherBridge*`).
4. Cofre extra al empezar (`WorldGeneratorBonusChest`) con su opción en crear mundo, y biomas grandes
   (`WorldType.LARGE_BIOMES`) si cabe en el menú de crear mundo de consola.
5. Mundo plano personalizado (`FlatGeneratorInfo`, `FlatLayerInfo`, `GuiCreateFlatWorld`, `GuiFlatPresets`) SOLO si
   el resto está terminado; con mando.

## MUY IMPORTANTE: el terreno
- El ruido, las cuevas y las capas de biomas NO cambian en 1.5.2: no los toques. Nunca /fp:fast.
- La Xbox genera chunks también en un hilo de fondo (build experimental, `XBOX_ASYNC_CHUNK_IO`) y hay rutas propias
  de población (`setBlockAndMetadataForPopulation`, `chunkLocalDecoration`, `PLATFORM_*POPULATE*`). Las estructuras
  nuevas tienen que pasar por las mismas rutas que las de 1.2.5 (aldeas, minas, fortalezas): mira cómo se escriben
  bloques en `ComponentVillage*` / `StructureComponent` en nuestro C++ y haz lo mismo. NADA de llamar al mundo desde el
  hilo de fondo por otro camino.
- Rendimiento: un chunk ya cuesta ~100 ms en la consola. Las estructuras dispersas en 1.5.2 son baratas; no añadas
  recorridos por chunk que no estén en el Java.
- Memoria: 64 MB. Nada de tablas grandes nuevas.

## Reglas de siempre
Candado BUILD.lock; xbox-release y xbox-release-exp; `removeBlockTileEntity` borra fuera del tick (commit `3eee9da`);
flags 2 con `setBlockMetadataWithNotify152`; `setRequiresSelfNotify()`; no push; no copies a ClassicXboxLoader.

## Al acabar
Informe `buzon-xbox\54-INFORME-agy-mundo-152.md` (tabla estructura -> archivos:línea, por qué ruta escribe bloques,
coste estimado por chunk), commits, y en tu terminal: `MUNDO 152 LISTO <commit>`
