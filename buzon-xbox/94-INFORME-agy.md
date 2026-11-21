# Informe 94 — Fuegos artificiales (1.5.2)

- **Rama / worktree**: `port152-erluza` (`wt-152-rend`), solo 1.5.2 (`wt-opti30` sin tocar).
- **Commit final**: `3d283f8` (`Port 1.5.2 fireworks: rocket, star, recipes, entity, particles, sounds and console crafting`).
- **Árbol**: limpio (`git status` vacío), sin sonda temporal (`PROBE_HITS=0`, `hasProbe=False`), sin reset, sin push, sin copiar al lanzador.

## Qué se ha portado desde Java 1.5.2

### 1. Objetos, NBT, tintado en 2 pasadas y tooltips
- `ItemFirework` (`ID 401`, ctor `145`, icono `fireworks.png`, sin pestaña en creativo igual que Java 1.5.2):
  - Lanzamiento desde cualquier cara de bloque (`onItemUse`), consume 1 unidad fuera de creativo.
  - Tooltip (`addInformation`): `Flight Duration: N` + detalle indentado de cada explosión en `Fireworks.Explosions`.
- `ItemFireworkCharge` (`ID 402`, ctor `146`, iconos `fireworksCharge.png` + `fireworksCharge_overlay.png`, pestaña `CreativeTabs::tabMisc` ya definida en `CreativeTabs.cpp` igual que Java 1.5.2):
  - Renderizado en 2 pasadas (`func_46058_c() == true`, `func_46057_a`) con tintado dinámico por NBT `Explosion.Colors` (media RGB si hay varios tintes, `0x8A8A8A` por defecto si no hay colores).
  - Se añadió `Item::getColorFromItemStack(ItemStack*, int)` y se conectó en `RenderItem` (GUI, entidad caída y marco), `ItemRenderer` (1.ª persona) y `RenderPlayer` (3.ª persona).
  - Tooltip (`addInformation` / `addExplosionInfo`): forma (`Small Ball`, `Large Ball`, `Star-shaped`, `Creeper-shaped`, `Burst`), colores, `Fade to ...`, `Trail` y `Twinkle` (todas las claves `item.fireworks*` ya estaban en `en_US.lang`).
  - Corregido en `ItemDye::worldChunkManager[7]` el color `silver` (gris claro) de `0x287697` (duplicado de 1.2.5) a `0xababab` (`11250603`) de Java 1.5.2.

### 2. Recetas (Java y crafteo de consola)
- `RecipeFireworks` (`IRecipe` dinámica registrada en `CraftingManager`, gestión de memoria sin fugas con `std::unique_ptr` y copia profunda de cada `Explosion` al crear cohetes):
  - **Estrella**: 1 pólvora + 1+ tintes + modificador de forma opcional (`fireballCharge` -> Bola grande, `goldNugget` -> Estrella, `skull` -> Creeper, `feather` -> Explosión/Burst) + modificadores de efecto (`lightStoneDust` -> Parpadeo/Twinkle, `diamond` -> Estela/Trail).
  - **Desvanecido**: 1 estrella existente + 1+ tintes -> añade `FadeColors` conservando forma/colores/efectos.
  - **Cohete**: 1 papel + 1–3 pólvoras (`Flight` 1–3) + 0..N estrellas (`Fireworks.Explosions`).
- **Crafteo de consola (`XboxCraftingScreen` + `LegacyCraftingGroups.h`)**:
  - Como `RecipeFireworks` es dinámica (`getRecipeOutput()` depende de la cuadrícula), se añadieron presets dinámicos en la pestaña **Decoration Blocks** (columnas separadas para cohetes y estrellas):
    - Cohetes: Vuelo 1–3 con y sin estrella (cualquier estrella en inventario).
    - Estrellas: las 5 formas + Twinkle + Trail + Fade (usando tinte comodín `-1`, de modo que `XboxCraftingScreen::craftSelected()` coloca el tinte/estrella real del inventario en la cuadrícula real y ejecuta `RecipeFireworks` produciendo el NBT auténtico).

### 3. Entidad, dispensador y persistencia
- `EntityFireworkRocket` registrada en `EntityList` (`"FireworksRocketEntity"`, ID `22`) y `RenderManager` (`RenderSnowball(Item::firework)`):
  - `DataWatcher` slot 8 (`ItemStack*`, clonado para evitar aliasing), vuelo acelerado, `fireworkAge`/`lifetime` según `Flight`, sonido `fireworks.launch`, estela continua `"fireworksSpark"` visible hasta 64 bloques y detonación vía `World::setEntityState(this, 17)` -> `handleHealthUpdate(17)` -> `World::makeFireworks(...)`.
  - Persistencia NBT completa (`Life`, `LifeTime`, `FireworksItem`) para cohetes en el aire y en cofres.
- **Dispensador (`BlockDispenser.cpp`)**: añadido lanzamiento básico de `EntityFireworkRocket` (orientación horizontal 2/3/4/5 actual; la orientación vertical arriba/abajo quedará para el encargo de dispensadores 1.5.2).

### 4. Partículas, blending alfa, sonidos y rendimiento Xbox (64 MB)
- `EntityFireworkStarterFX`, `EntityFireworkSparkFX` y `EntityFireworkOverlayFX`:
  - Las 5 formas de Java 1.5.2 (Small Ball, Large Ball, Star, Creeper, Burst), colores múltiples, desvanecido (`FadeColors`), parpadeo (`Twinkle`), estela (`Trail`), flash translúcido (`EntityFireworkOverlayFX`) y `particles.png` (índices 160–167 + overlay).
  - `EntityFireworkStarterFX` copia las definiciones de explosión en `std::vector<ExplosionDef>` al construirse (cero punteros colgantes cuando muere el cohete).
  - Se añadió soporte de `particleAlpha` en `EntityFX` y alpha-blending en `EffectRenderer::renderParticles` igual que Java 1.5.2.
  - **Sonidos**: los 7 sonidos `fireworks.*` (`launch`, `blast`, `blast_far`, `largeBlast`, `largeBlast_far`, `twinkle`, `twinkle_far`) ya estaban presentes en `resources/sound3/fireworks/` y registrados en `SoundPool` (`7/7` verificados en tiempo de ejecución).
- **Optimización y tope en Xbox (64 MB)**:
  - Vía rápida aérea en `EntityFireworkSparkFX::onUpdate()`: cuando el bloque destino es aire, evita `Entity::moveEntity`/`getCollidingBoundingBoxes` (colisión completa solo al entrar en un bloque sólido).
  - Tope por capa en `EffectRenderer` (`1000` en Xbox) + escalado adaptativo en `EntityFireworkStarterFX` y cadencia de estelas en `EntityFireworkSparkFX` según partículas activas en capa 0: con 1–2 cohetes mantiene densidad completa y formas nítidas, y con 10 cohetes simultáneos limita el pico alrededor de ~750 partículas sin empobrecer el efecto.

## Verificación en `xemu` (64 MB)

### Pruebas funcionales (sonda temporal, eliminada antes del commit)
- **Objetos y creativo**: `fwId=401` (`tab=null`), `fcId=402` (`tab=tabMisc`), tintes `red=b3312c`, `multi=e4ab37`, `default=8a8a8a`.
- **Sonidos**: `7/7` presentes en `SoundPool` (`fireworks.launch`, `blast`, `blast_far`, `largeBlast`, `largeBlast_far`, `twinkle`, `twinkle_far`).
- **Recetas Java**: crafteadas y verificadas las 5 formas, colores múltiples, `FadeColors`, `Trail`, `Twinkle` y cohetes `Flight 1/2/3` con 1–3 estrellas + tooltips exactos.
- **Crafteo de consola (`XboxCraftingScreen`)**: estrella `Star-shaped | Red` (`ok=1`) y cohete `Flight Duration: 1 | Star-shaped | Red` (`ok=1`) crafteados desde la interfaz de consola.
- **Dispensador**: `rocketsBefore=0 -> rocketsAfter=1 (spawned=1)`.
- **10 cohetes simultáneos (mezcla de las 5 formas + Trail + Twinkle + Fade)**:
  - **Antes**: `51.0–58.6 fps`, `freeKB=27928–29708`, `particles=0`.
  - **Pico simultáneo**: `47.8–54.7 fps` (mínimo `30.4–45.3 fps`), `peakParticles=756–766`, `freeKB=26200–29036`.
  - **Después** (`curParticles=0`): `50.0–53.6 fps`, `0` cuelgues, `0` fugas (la variación de `freeKB` entre antes y después corresponde a la malla de chunks cargándose en paralelo tras el teletransporte; al terminar el frame 1501 la memoria libre se estabiliza sin drenaje).
- **Guardado y recarga**:
  - Cofre: `chestStack=1 flight=2(2) exps=2(2)` — NBT intacto.
  - Cohete en el aire: `midAirRockets=1 age=10 life=30 flight=2(2) exps=2(2)` — NBT y vuelo intactos.

### Capturas (`buzon-xbox\94-capturas\`)
- `01-crafteo-consola-fuegos.png`: menú de crafteo de consola (`XboxCraftingScreen`, pestaña Decoration Blocks, selector de estrellas tintadas y receta `Star: Star-shaped`).
- `02-inventario-tooltip-fuegos.png`: inventario Java con cohete, 5 estrellas tintadas en 2 pasadas y tooltip completo (`Firework Star`, `Creeper-shaped`, `Lime`, `Fade to Magenta`, `Trail`, `Twinkle`).
- `03-formas-bolas-y-estrella.png`: explosiones Small Ball (con Fade), Large Ball (Twinkle) y Star-shaped (Trail) en cielo nocturno + cohete en mano.
- `04-forma-creeper-y-burst.png`: formas Creeper-shaped (Trail + Twinkle) y Burst (multicolor + Fade) en cielo nocturno.
- `05-10-cohetes-simultaneos.png`: 10 cohetes simultáneos explotando con las 5 formas, flash translúcido y estelas.

### Compilaciones finales limpias y arranque en `xemu` (64 MB)
| Preset | Build RC | SSE2 warnings | XBE (bytes) | `hasProbe` | Selftest | `double53` | `freeKB` | FPS menú | Errores/NaN/X87 |
|---|---:|---:|---:|---|---|---:|---:|---:|---|
| `xbox-release` | 0 | 13 (0 nuevos) | 4,956,160 | `False` | `0 failure(s)` | `1` | `45404` | `69.5–77.2` | `0` |
| `xbox-release-exp` | 0 | 13 (0 nuevos) | 4,972,544 | `False` | `0 failure(s)` | `1` | `45428` | `73.0–76.4` | `0` |
