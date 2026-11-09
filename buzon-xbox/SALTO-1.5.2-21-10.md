# Salto 1.2.5 -> 1.5.2 (21-10-2026)

OptiCraft es una traducción a C++ de Java 1.2.5 descompilado con MCP (nombres field_/func_ en el código).

## Herramientas (fuera del repo)
`Minecraft en xboox clasico\mcp-diff\`
- `src125\` y `src152\`: fuente Java descompilada con nombres MCP (983 y 1464 clases).
- Cómo se hizo: jars de `Downloads\xeolauncher\libraries\com\mojang\minecraft\` (mismo md5 que Mojang),
  mapeos `conf\mappings.tiny` de RetroMCP (v125: client->named, v152: official->named, `--ignoreConflicts`),
  `tools\tiny-remapper.jar` + `tools\vineflower.jar` (`--only=net/minecraft`).
  El setup de RetroMCP falla porque maven.glass-launcher.net no responde; no hace falta.
- Java: `Downloads\xeolauncher\java\java-runtime-delta\bin\java.exe` (21).
- Listas: `solo125` (52 clases quitadas), `solo152` (533 nuevas), `cambiadas` (759, con nº de líneas cambiadas).

## Hallazgos
- Ruido, cuevas y capas de biomas (dónde cae cada bioma): 0 cambios o solo de nombres.
  CORREGIDO (agy, inventario 41): el terreno SÍ cambia en colinas y montañas. BiomeGenBase sube setMinMaxHeight de
  Extreme Hills (0.2,1.3 -> 0.3,1.5), Ice Mountains (0.2,1.2 -> 0.3,1.3), DesertHills/ForestHills/TaigaHills (+0.1)
  y JungleHills (1.8,0.2 -> 1.8,0.5). BiomeGenHills añade esmeraldas y lepismas en decorate().
  Llanuras, bosques, desiertos, océanos y ríos: igual. Lo nuevo en ChunkProviderGenerate: MapGenScatteredFeature
  (templos, cabaña de bruja) y LARGE_BIOMES. Las huellas de prueba (131 chunks) cambiarán: la referencia nueva
  tiene que salir del Java 1.5.2, no de nuestra 1.2.5. En mundos viejos habrá cortes entre chunks viejos y nuevos.
- Nether (ChunkProviderHell) cambia: cuarzo y fortaleza.
- ~205 clases nuevas son servidor/infra (comandos, dedicado, Realms/Mco, informes de fallo, RCon, LAN). No hacen falta.
- Riesgo grande: 1.3 pasa el modo un jugador a servidor integrado (IntegratedServer, WorldServer, EntityPlayerMP).
  Con 64 MB no cabe -> traer contenido SIN ese cambio de arquitectura.
- 1.5 cambia las texturas a archivos sueltos + atlas cosido (TextureMap/Stitcher). Mantener nuestro render,
  generar el atlas fuera de la consola.
- Lo que más pesa de lo nuevo: tolvas (tick por TileEntity), comparadores, mobs nuevos (Wither, brujas, murciélagos).

## Plan
1. Terminar optimización de biomas y cuevas (sigue valiendo, el terreno no cambia).
2. Portar por versiones: 1.3 -> 1.4 -> 1.5, comparando clase a clase con `diff -w src125/... src152/...`.
   Probar cada versión en la consola real.
