# 60. Lo que falta de verdad en la 1.5.2 (análisis del código, 26-10)

Cómo se midió (rama `port152-claude`, `153f374`):
- Clases NUEVAS de contenido (`mcp-diff\contenido152`, 277): existen 87 en C++. De las 190 que faltan, la mayoría es
  infraestructura que no hace falta (atlas dinámico `TextureMap/Stitcher`, pools `Vec3Pool/AABBPool`, registros,
  servidor, chat, LAN, snooper, excepciones).
- Clases COMUNES con lógica nueva (`docs\port152\cambiadas.md`, tipo `logica`, 386): 288 NO se han tocado desde que
  empezó el port (`git diff 3ae714b..HEAD`). Abajo solo las que cambian el juego; fuera red/paquetes/texturas/opciones.

## En marcha
- agy 57: yunque, faro, marco (`EntityHanging`, `EntityPainting` pasa a colgante), libros, mapa vacío, caña con zanahoria.
- agy 58: menú creativo 1.5.2 (12 pestañas, `CreativeTabs`), cofre trampa (`TileEntityChest`).
- Codex 59: pociones (visión nocturna, invisibilidad, `PotionHelper`, soporte, dispensador de pociones, manzana encantada).

## Funciones grandes que NO existen
1. **Aldeanos**: comercio (`EntityVillager` 475 líneas cambiadas, `MerchantRecipe(List)`, `InventoryMerchant`,
   `ContainerMerchant`, `GuiMerchant`, `SlotMerchantResult`, `EntityAITradePlayer`...); aldeas del desierto
   (`ComponentVillage*`, `StructureVillagePieces`); reputación y `villages.dat` (`Village`, `VillageCollection`).
2. **Wither (jefe)**: `EntityWither`, `EntityWitherSkull`, `RenderWither`, `ModelWither`, barra de jefe (`BossStatus`,
   `GuiIngame`). El dragón también usa la barra nueva.
3. **Fuegos artificiales**: `EntityFireworkRocket`, `ItemFirework`, `ItemFireworkCharge`, `RecipeFireworks`, partículas.
4. **Tinte de cuero**: `ItemArmor` (color), `RecipesArmorDyes`, `RenderPlayer` (2ª pasada), lavar en caldero. Ahora el
   cuero va pintado de marrón fijo en el pak.
5. **Mapas**: zoom/escala, clonar y ampliar (`RecipesMapCloning/Extending`), marcadores en marcos (`ItemMap`, `MapData`).
6. **Encantamientos**: Espinas (`EnchantmentThorns`), encantar libros en la mesa (`ContainerEnchantment`),
   `EnchantmentHelper`, daño con arma encantada en mobs (`EntityMob::getAttackStrength`).
7. **Reglas de juego** (`GameRules`: mobGriefing, keepInventory, doMobSpawning...) y **modo aventura** (`EnumGameType`).
8. **Dispensador 1.5.2**: cubos, carga de fuego, polvo de hueso, poner armadura, barcas, vagonetas, TNT, huevos,
   fuegos (`DispenserBehavior*`, `TileEntityDispenser`).
9. **Generador de mobs** nuevo (`MobSpawnerBaseLogic`) y **vagoneta con generador**.
10. **Superplano personalizable** (`FlatGeneratorInfo`, `GuiCreateFlatWorld`, presets).
11. Bloque de comandos (necesita comandos: probablemente NO).

## Cambios de comportamiento sin portar (clases existentes)
- **Bloques**: troncos tumbados (`BlockLog`, orientación en 3 ejes), nieve por capas (`BlockSnow` 8 niveles, `ItemSnow`),
  trampillas arriba (`BlockTrapDoor`), vallas de 1,5 de alto y antorchas encima de vallas/muros (`BlockFence`,
  `BlockTorch`), cacao en árboles de jungla (`WorldGenTrees`), hoja con Fortuna da manzanas (`BlockLeaves`), tallo con
  polvo de hueso (`BlockStem`), hombres-cerdo saliendo de portales (`BlockPortal`), TNT encendida por flecha de fuego
  (`BlockTNT`, `EntityArrow`), flechas que pulsan botones de madera, cama que explota en el Nether (`BlockBed`).
- **Mobs**: lobo con collar de color (`EntityWolf`), ovejas que mezclan color al criar (`EntitySheep`), gallinas con
  cualquier semilla, gatitos al aparecer ocelotes, gólem que ataca slimes, Enderman gritando, creeper/ghast con
  potencia en NBT, slimes del pantano por fase lunar, crías con hitbox pequeño (`EntityAgeable`), XP al criar y al
  pescar (`EntityAIMate`, `EntityFishHook`), XP del horno (`SlotFurnace`), mobs con arma encantada.
- **Objetos sueltos**: `EntityItem` junta pilas iguales cercanas (importante: con 247 objetos tirados el tick se
  disparó) y `EntityXPOrb` busca jugador cada 20 ticks (rendimiento).
- **Inventario**: shift-clic pone armadura (`ContainerPlayer`) y lleva combustible/mineral a su casilla en el horno
  (`ContainerFurnace`); repartir pila arrastrando y doble clic (`GuiContainer`, adaptar al mando).
- **Explosiones**: protección contra explosiones y resistencia por entidad (`Explosion`).
- **Barca**: aceleración nueva, rompe nenúfares (`EntityBoat`).
- **Nether**: `Teleporter` guarda los portales 300 s (menos tirón al cruzar); `MapGenCavesHell` cambia cómo pasa la
  semilla (las cuevas del Nether salen distintas a Java 1.5.2 con la misma semilla).
- **Pantalla**: nombre del objeto al cambiar de casilla (`GuiIngame`), calaveras en la cabeza (`RenderPlayer`),
  sombra de invisibles (`Render`).
- **Aldeas**: `VillageSiege` llama a `initCreature` (zombis de asedio con equipo).

## Para decidir con el usuario
Orden propuesto: aldeanos (comercio) -> Wither -> fuegos -> cuero teñido -> bloques (troncos, nieve, vallas...) ->
mobs pequeños -> dispensador -> reglas de juego -> mapas -> superplano. Rendimiento (`EntityItem`, `EntityXPOrb`,
`Teleporter`) conviene pronto porque se nota en la consola.
