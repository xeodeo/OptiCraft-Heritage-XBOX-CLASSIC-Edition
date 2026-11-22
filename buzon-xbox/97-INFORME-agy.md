# Informe Encargo 97 — Fallos 1.5.2 (Cohetes, Crafteo de Consola, Faro, Pigman y Hielo en el Nether)

- **Rama / worktree 1.5.2**: `wt-152-rend` (`port152-erluza`)
- **Commit 1.5.2**: `b900220c` (`b900220cc6dfde03b54a283b037dafa0f3a88aa2`)
- **Rama / worktree 1.2.5**: `wt-opti30` (`opti30-erluza`) — **sin cambios** (`90faed9f`), ver Punto 5.
- **Capturas D3D8 640×480**: `buzon-xbox\97-capturas\` (`01` a `12`).

---

## 1. Crash y cuelgue al lanzar cohetes de fuegos artificiales

### Causa raíz del `[crash] Unexpected error: bound must be positive` y `main stuck`
1. **Evicción del propio `EntityFireworkStarterFX` durante `onUpdate()` (use-after-free)**:
   - Con `PocketOptimizations` activo el tope de partículas por capa (`fxLayers[0]`) es `256` (`1000` sin optimizaciones).
   - Al detonar un cohete con estrella (especialmente Bola Grande / Explosión / Estrella / múltiples cohetes), `EntityFireworkStarterFX::onUpdate()` inserta decenas o cientos de `EntityFireworkSparkFX` vía `EffectRenderer::addEffect()`.
   - El cursor circular `fxEvictCursor[0]` alcanzaba la propia ranura de `EntityFireworkStarterFX` y ejecutaba `fxLayers[0][cursor].reset(entityfx)`, destruyendo `this` y su `std::vector<ExplosionDef> explosions` en mitad de `createBall`/`createShaped`/`createBurst`.
   - En la siguiente iteración de `createParticle()`, `colors.size()` leía memoria liberada / `0` o basura negativa -> `rand.nextInt(bound <= 0)` lanzaba `std::invalid_argument("bound must be positive")` dos veces seguidas, dejando la consola colgada en `phase=frameBegin`.
2. **Excepción C++ sin red de seguridad en `Random::nextInt(int_t bound)`**:
   - En `src/java/Random.cpp`, `bound <= 0` lanzaba `std::invalid_argument` en Xbox, abortando el frame o colgando el hilo principal.
3. **Verificación adicional durante pruebas (doble liberación detectada y evitada)**:
   - `WatchableObject` (`WatchableObject.h`) ya posee y libera el `ItemStack*` del slot 8 de `DataWatcher` en `setObject()` y `~WatchableObject()`, por lo que `EntityFireworkRocket` se mantuvo limpio delegando esa propiedad a `WatchableObject` (comprobado con validador en vivo de `XapiProcessHeap` con 0 corrupciones).

### Arreglo aplicado
- **`src/net/minecraft/src/EntityFX.h`**: añadido `virtual bool canBeEvicted() const { return true; }`.
- **`src/net/minecraft/src/EntityFireworkStarterFX.h/.cpp`**:
  - `canBeEvicted() const override { return false; }` para proteger el emisor activo mientras detona.
  - Copia por valor `const ExplosionDef def = explosions[...]` antes de emitir chispas.
  - Guardas explícitas `colorCount > 0` y `fadeCount > 0` antes de `rand.nextInt(...)` en `createParticle()`.
- **`src/net/minecraft/src/EffectRenderer.h/.cpp`**:
  - Seguimiento de `currentlyUpdatingEffect` en `updateEffects()`.
  - En `addEffect()`, el barrido circular busca una víctima evictable (`candidate != currentlyUpdatingEffect && candidate->canBeEvicted()`); si toda la capa es no-evictable descarta la nueva partícula sin destruir `this`.
  - Tras `onUpdate()`, relee `fxLayers[i][j].get()` antes de comprobar `isDead`.
- **`src/java/Random.cpp`**: en `XBOX_PLATFORM`, si `bound <= 0`, registra `MC_LOG_ERROR` y devuelve `0` en lugar de lanzar una excepción que cuelgue la consola.
- **`src/net/minecraft/src/BlockGravel.cpp` y `FontRenderer.cpp`**: guardas de cota positiva antes de `nextInt`.

### Pruebas en `xemu` (64 MB)
- Lanzados **25 cohetes** con audio activado (`soundVolume = 1.0`):
  - **Fase A (19 cohetes, `ofPocketOptimizations = true`, tope `256`)**: 12 crafteados en el menú de consola + variantes de `RecipeFireworks` (sin NBT, vuelo 1..3, multi-estrella con brillo/estela/bola grande), lanzados tanto desde suelo (`ground=10`) como desde dispensador (`dispenser=9`).
  - **Fase B (6 cohetes, `ofPocketOptimizations = false`, tope `1000`)**: las 5 formas + parpadeo + estela.
- Resultado: **25/25 detonados y sobrevividos**, `0` crashes, `0` `non-positive bound`, `0` errores de heap, `freeKB = 36464 KB`.
- Captura: `97-capturas\12-cohetes.png`.

---

## 2. Menú de crafteo de consola (LCE TU19), duplicados e icono de estrella

*(Nota: conforme a la corrección del usuario, no se tocó el funcionamiento de la mesa de crafteo en creativo).*

### Duplicados eliminados
- **Faro (`Block::beacon`)**: eliminada la receta duplicada en `src/net/minecraft/src/CraftingManager.cpp` (estaba registrada dos veces).
- **Yunque (`Block::anvil`)**: eliminada la receta duplicada en `src/net/minecraft/src/RecipesCrafting.cpp`.

### Reorganización fiel a LCE TU19 (`lce-tu19\Minecraft.World\`)
- Actualizado `scripts/xbox/make_crafting_groups.py` para parsear `static_cast<T*>(...)`, literales `L'...'` (incluyendo `Tile::jukebox`) y el conjunto completo de ítems 1.5 de TU19, y regenerado `src/net/minecraft/src/legacy/LegacyCraftingGroups.h` (187 entradas).
- Mapeo exacto según `lce-tu19\Minecraft.World\`:
  - **Faro (`Tile::beacon`, 138)** -> **Mecanismos (`M`)** (`StructureRecipies.cpp:142`).
  - **Cofre trampa (`Tile::chest_trap`, 146)** -> **Estructuras (`S`, columna Cofres)** (`StructureRecipies.cpp:83`).
  - **Dispensador (23), Tolva (154), Soltador (158)** -> **Mecanismos (`M`, columna `redstoneContainer`)**.
  - **Sensor de luz solar (151), Placas de presión pesada oro/hierro (147, 148), Comparador (404)** -> **Mecanismos (`M`)**.
  - **Raíl activador (157, columna Raíles), Vagoneta con TNT (407), Vagoneta con tolva (408)** -> **Transporte (`V`)**.
  - **Tocadiscos (84), Bloque de redstone (152), Polvo de redstone (331), Cohete (401), Estrella de fuegos artificiales (402)** -> **Decoración (`D`)**, con Cohete (`baseType=45`) y Estrella (`baseType=46`) en columnas separadas.
  - **Bloque de cuarzo (155) y Escaleras de cuarzo (156)** -> **Estructuras (`S`)**.

### Icono de Estrella de Fuegos Artificiales y RenderItem 2D
- **`src/net/minecraft/src/RenderItem.cpp`**: habilitado `RenderCapability::AlphaTest` (`Greater, 0.1f`) al renderizar ítems 2D en GUI (tanto multipasada como pasada única), asegurando recorte limpio de las dos capas de la estrella (`fireworksCharge` + `fireworksCharge_overlay`).
- **`src/net/minecraft/src/legacy/XboxCraftingScreen.h/.cpp`**:
  - Añadidos `displayStackFor()`, `refreshFireworkPreviews()`, `firstMatchingDyeDamage()` y `firstInventoryStack()`.
  - El tinte comodín (`damage = -1`) resuelve al primer tinte disponible en el inventario del jugador (o al color vivo predeterminado del preset; nunca Saco de tinta negro si no lo tiene).
  - La estrella de fuegos artificiales muestra en la cuadrícula, la fila superior y el recuadro de resultado una estrella coloreada acorde al tinte real del inventario, y el cohete con estrella refleja la estrella real del inventario.

### Lista final de las 7 categorías del crafteo de consola (auditoría 3×3: 238 recetas totales)
| # | Categoría | Columnas | Contenido (por columnas / pilas) | Captura |
|---|-----------|----------|----------------------------------|---------|
| 0 | **Estructuras** | 12 | Cofres (Cofre, Cofre de Ender, Cofre trampa); Dispositivos (Horno, Mesa, Encantamientos, Soporte pociones, Yunque ×1); Escaleras (10 tipos incl. Cuarzo); Escalera de mano; Vallas/Barrotes/Muros (5); Puerta de valla; Puertas/Trampilla (3); Cama; Bloques estructurales (11 incl. Arenisca, Ladrillos, Cuarzo); Losas (11); Palo; Tablones (4) | `01-craft-estructuras.png` |
| 1 | **Herramientas** | 12 | Picos (5); Arco/Flecha (2); TNT; Utensilios (Caldero, Cubo, Cuenco, Frasco); Cañas (Pescar, Zanahoria en palo); Bolsillo (Reloj, Brújula, Mapa, Mapa vacío, Ojo de Ender, Libro y pluma); Hachas (5); Azadas (5); Tijeras/Mechero (2); Iluminación/Fuego (Antorcha, Piedra luminosa, Calabaza iluminada, Carga ígnea); Palas (5); Espadas (5) | `02-craft-herramientas.png` |
| 2 | **Comida** | 13 | Melón; Frutas doradas (Zanahoria dorada, Melón reluciente); Tarta; Manzana dorada; Galleta; Pan; Semillas (Calabaza, Melón); Azúcar; Estofado de champiñones; Pastel de calabaza; Ojo de araña fermentado; Crema de magma; Polvo de Blaze | `03-craft-comida.png` |
| 3 | **Armadura** | 4 | Petos (5); Pantalones (5); Cascos (5); Botas (5) | `04-craft-armadura.png` |
| 4 | **Mecanismos** | 12 | Lámpara de redstone; **Faro (×1)**; Bloque musical; **Sensor de luz solar**; Contenedores de redstone (Dispensador, **Tolva**, **Soltador**); Pistones (Normal, Pegajoso); **Comparador**; Repetidor; Palanca / Gancho de cuerda (2); Antorcha de redstone; Placas de presión (Piedra, Madera, **Oro**, **Hierro**); Botones (Piedra, Madera) | `05-craft-mecanismos.png` |
| 5 | **Transporte** | 7 | Raíles (Normal, Propulsor, Detector, **Activador**); Vagoneta; Barca; Vagoneta con cofre; Vagoneta con horno; **Vagoneta con TNT**; **Vagoneta con tolva** | `06-craft-transporte.png` |
| 6 | **Decoración** | 11 | Bloques minerales/decorativos (Oro, Hierro, Diamante, Lapislázuli, Esmeralda, **Redstone**); Descomposición de bloques (7); **Tocadiscos**; Papel/Libro/Estantería (3); Colgantes (Cartel, Cuadro, Marco); Panel de cristal; Maceta; Lana (17); Tintes (16); **Cohetes de fuegos artificiales (6 presets)**; **Estrellas de fuegos artificiales (8 presets)** | `07-craft-decoracion-estrella.png`, `08-craft-decoracion-cohete.png` |

Auditoría automática en `xemu`:
`CRAFT_AUDIT total=238 beacon=101 (×1 en Mecanismos) anvil=1 railAct=1 daylight=1 goldPl=1 ironPl=1 trapCh=1 mcTnt=1 mcHop=1 juke=1`, `STAR_PREVIEW id=402 color=0x6689d3`, `CONSOLE_STARS_CRAFTED=8`.

---

## 3. Interfaz del Faro (`GuiBeacon` + `GuiContainer`) — Tooltips encima de los iconos

### Causa raíz
- En `src/net/minecraft/src/GuiContainer.cpp`, `GuiScreen::drawScreen()` (que dibuja los botones `controlList`, incluidos los botones de poder del faro) se llamaba **después** de `drawGuiContainerForegroundLayer()` (que dibuja los tooltips de `GuiBeaconButton::func_82251_b`), haciendo que los botones e iconos se pintaran por encima del texto del tooltip.
- En Java 1.5.2 (`GuiContainer.java:59-92`), `super.drawScreen()` se ejecuta inmediatamente después de `drawGuiContainerBackgroundLayer()` y **antes** de slots, ítems y `drawGuiContainerForegroundLayer()`.

### Arreglo aplicado
- **`src/net/minecraft/src/GuiContainer.cpp`**: movido `GuiScreen::drawScreen(mouseX, mouseY, partialTick)` justo tras `drawGuiContainerBackgroundLayer()` y antes del renderizado de slots y `drawGuiContainerForegroundLayer()` (equivalente a Java 1.5.2).
- **`src/net/minecraft/src/GuiBeacon.cpp`**: actualizado `drawCreativeTabHoveringText()` al estilo exacto de Java 1.5.2 (`tx = x + 12`, `ty = y - 12`, fondo `0xf0100010`, borde degradado morado `0x505000ff` -> `0x5028007f`, `zLevel = 300.0f` restaurado a `0.0f`, iluminación/profundidad desactivadas y ajuste a bordes de pantalla para 480p y 720p).

### Verificación en `xemu`
- Capturado con pirámide de nivel 4 activa (`levels=4, buttons=8`) en **ambos modos de interfaz**, comprobando visualmente que el tooltip `"Speed"` se dibuja nítidamente **por encima** de los iconos de botones adyacentes (`Haste`):
  - Interfaz Java: `97-capturas\09-faro-java.png`
  - Interfaz Legacy: `97-capturas\10-faro-legacy.png`

---

## 4. Hombre Cerdo Zombi (`EntityPigZombie`) — Textura y agresión en grupo (32 bloques)

### Causas raíz
1. **Textura/modelo corrupto ocasional**: `EntityPigZombie` heredaba `EntityZombie::initCreature()` sin override, por lo que ocasionalmente lanzaba el 5% de probabilidad de marcarse como zombi aldeano (`setVillager(true)` -> `RenderZombie` usaba `zombieVillagerModel` con `/mob/pigzombie.png`!) o de equiparse espada/pala de hierro en `heldItemInstance`.
2. **Persecución incompleta al golpear uno**:
   - Al enfadarse (`becomeAngryAt`), un pigman que ya tenía una ruta aleatoria de deambulación no la limpiaba.
   - En `EntityCreature.cpp`, al agotarse el presupuesto `PLATFORM_PATHFIND_BUDGET_PER_TICK`, la ruta anterior se sobrescribía con `nullptr`; además, el pathfinder de entidad tiene un radio de búsqueda de ~16 bloques (`getPathEntityToEntity(..., 16.0f)`), por lo que pigmen enfadados entre 17 y 32 bloques no recibían camino hacia el jugador.

### Arreglo aplicado
- **`src/net/minecraft/src/EntityZombie.h`**: movido `heldItemInstance` a `protected`.
- **`src/net/minecraft/src/EntityPigZombie.h/.cpp`**:
  - Override de `initCreature()`: llama a `EntityZombie::initCreature()`, fuerza `setVillager(false)` y elimina cualquier espada/pala de hierro que haya caído en `heldItemInstance` (conservando su espada de oro).
  - Override de `interact()` devolviendo `false` (no acepta curación de zombi aldeano).
  - Forzado `setVillager(false)` en `readEntityFromNBT()` para sanear mundos ya guardados.
  - Comprobación `isEntityInvulnerable()` en `attackEntityFrom()` y limpieza de ruta `setPathToEntity(nullptr)` en `becomeAngryAt()`.
- **`src/net/minecraft/src/RenderZombie.cpp`**: `setupZombieModel()` garantiza que `EntityPigZombie` nunca use `zombieVillagerModel`.
- **`src/net/minecraft/src/EntityCreature.cpp`**: preserva `pathToEntity` si el presupuesto de pathfinding por tick devuelve `nullptr`, y usa un punto intermedio a 14 bloques cuando `playerToAttack` está a 17–36 bloques para que todos los pigmen enfadados en el radio de 32 bloques avancen hacia el jugador.

### Verificación en `xemu`
- 32 hombres cerdo zombi generados entre 4 y 28 bloques del jugador:
  - `PIGMAN_SPAWN total=32 villager=0 goldSword=32` (0 fallos de textura/modelo, 32/32 con espada de oro).
  - Al golpear a 1: `PIGMAN_AGGRO_IMMEDIATE angry=32/32` (100% enfadados al instante).
  - Tras 40 ticks de persecución: `PIGMAN_CHASE nearMoved=15/16 farMoved=13/16` (persecución activa tanto cerca como entre 17 y 28 bloques).
  - Captura: `97-capturas\11-pigman.png`.

---

## 5. Hielo roto o derretido en el Nether (`BlockIce`) — 1.5.2 vs 1.2.5

### 1.5.2 (`wt-152-rend`)
- **`src/net/minecraft/src/BlockIce.h/.cpp`** (fiel a Java 1.5.2 `BlockIce.java`):
  - En `harvestBlock()`: si el jugador tiene Toque de Seda, suelta bloque de hielo (y no coloca agua); en caso contrario, si `world->worldProvider->isHellWorld`, evapora dejando aire (`0`) y retorna sin colocar agua. En el Overworld sigue dejando agua en movimiento (`8`) sobre bloques sólidos/líquidos.
  - En `updateTick()`: si la luz de bloque supera `11 - lightOpacity[blockID]` y `isHellWorld`, evapora dejando aire (`0`) en lugar de agua quieta.
  - Eliminado el override `createStackedBlock()` que devolvía `nullptr` para que Toque de Seda suelte el bloque de hielo correctamente.
- Verificación en `xemu`:
  `ICE owBreak=8(expect=8) owSilk=0(expect=0) netherBreak=0(expect=0) netherMelt=0(expect=0) netherSilk=0(expect=0)` — **100% PASS**.

### 1.2.5 (`wt-opti30`) — sin modificar
- Se comprobó el comportamiento canónico de Minecraft Java 1.2.5 (`BlockIce.java`): en 1.2.5 el hielo **no** tiene comprobación `isHellWorld` y efectivamente genera agua al romperse en el Nether (esa comprobación se introdujo en Java 1.3.1 / snapshot 12w19a).
- Por ello `wt-opti30` se dejó completamente intacto en `90faed9f`.

---

## 6. Compilaciones limpias y arranque verificado en `xemu` (64 MB)

Se eliminó todo el código temporal de prueba (`Probe97.h` y ganchos en `Minecraft.cpp`, `Display_xbox.cpp`, `XboxCraftingScreen.h`, `GuiBeacon.h`), se compilaron ambos presets bajo `buzon-xbox\BUILD.lock` y se verificó el arranque limpio en `xemu` (64 MB):

- **`xbox-release`** (`bin\xbox\OptiCraft.iso`):
  - `BUILD 1.5.2 21-11 17:56`, `selftest: 0 failure(s)`, `226 recipes`, `DirectSound ready (16 voices, stream ok, stereo)`, `double53=1`, `freeKB=45388`, `GuiMainMenu`, 0 errores/crashes.
- **`xbox-release-exp`** (`bin\xbox-exp\OptiCraft.iso`):
  - `BUILD 1.5.2 21-11 17:56`, `selftest: 0 failure(s)`, `226 recipes`, `DirectSound ready (16 voices, stream ok, stereo)`, `double53=1`, `freeKB=45376`, `GuiMainMenu`, 0 errores/crashes.
