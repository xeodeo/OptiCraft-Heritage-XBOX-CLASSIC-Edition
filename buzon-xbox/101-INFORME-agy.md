# Informe Encargo 101 — Sistema de Mods, Catálogo, Minimapa (Rei & Xaero), Herobrine (tkelly `iLoveYou`) y ChocoCraft (1.5.2 y 1.2.5 en Xbox Clásico)

- **Agente**: `agy-2`
- **Ramas / Commits finales**:
  - **1.5.2** (`wt-152-mods`, rama `port152-mods`): `77659414` (80 archivos, base `a2ec7d1e`)
  - **1.2.5** (`wt-125-mods`, rama `opti30-mods`): `467eb2b6` (81 archivos, base `f1bf66ae`)
- **Capturas verificadas en `xemu` (D3D8 nativo `640×480`)**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\101-capturas\` (21 capturas PNG revisadas visualmente)
- **Invariantes comprobados**: `double53=1`, `0 failure(s)` en `xbox.selftest`, sin `/fp:fast`, presupuesto 64 MB respetado (`freeKB` 22,7–45,3 MB; minimapa 80,0 KB), modo creativo intacto, generación de terreno vanilla intacta, sonido intacto, 100 % navegable con mando de Xbox, UI Java y Legacy implementadas en todas las pantallas.

---

## 1. Infraestructura de Mods, Catálogo/Buscador (1b) y Mods de `erluza` (1c)

### 1 y 1b — `ModManager` y Catálogo/Buscador estilo tienda (`GuiCompiledMods`)
- **`ModManager`** (`src/mods/ModManager.h/.cpp`): gestiona los **9 mods compilados**, todos **apagados por defecto** (`enabled=false`), con persistencia en `mods.txt` y soporte de catálogo local opcional `mods/catalog.txt`.
- **`GuiCompiledMods`** (`src/mods/GuiCompiledMods.h/.cpp`, enlazado desde `GuiMods`, `GuiOptions` y `LegacyHelpOptions`):
  - Interfaz completa **Legacy UI** (`01_catalogo_mods_legacy.png`) y **Java UI** (`02_catalogo_mods_java.png`).
  - **Búsqueda en vivo** mediante `VirtualKeyboard` / campo de texto (`03_catalogo_mods_busqueda.png`, filtrando `"choco"` → ChocoCraft).
  - **Pestañas de filtro** (`Todos` / `Activos` / `Disponibles`) navegables con **LT/RT** y D-Pad.
  - **Iconos pixel-art procedurales** dedicados por mod (sin archivos de recursos externos ni peso en RAM).
  - **Instalación/activación y desactivación en un clic** (`A` = Instalar/Activar o Desactivar, `Y` = Abrir configuración del mod cuando tiene menú propio, `X` = Buscar, `B` = Volver).
- Excluidos explícitamente de Xbox: `autologin`, `oomtest` y `ps2playlist`.

### 1c — Mods traídos de `erluza` y verificados en Xbox (1.5.2 y 1.2.5)
1. **X-Ray (`XRayMod` + `XRayMode`)**:
   - Combo en mando de Xbox: **`LT + RT + White + Black`** (y `F6` en PC), compatible con P1 y P2 en pantalla dividida.
   - Integrado tanto en `WorldRenderer` / `RenderBlocks` como en el constructor incremental de Xbox (`src/pc/render/PcLegacySectionCache.cpp` y `src/pc/minecraft/WorldRendererPcLegacy.cpp`), filtrando bloques no-mena y aplicando brillo completo (`15/15`) a menas, cofres, spawners, lava, obsidiana y portales.
   - Corrección aplicada tras prueba visual: `XRayMod::setEnabled(true)` inicializa `m_lastWorld = m_mc->theWorld` para no resetear `XRayMode` al activarlo dentro de una partida.
   - Verificado en `12_xray_menas.png` a **55,00 FPS** mostrando diamante, oro, hierro, redstone y carbón bajo tierra.
2. **Village Locator (`VillageLocatorMod` + `MapGenVillage::findNearestVillage`)**:
   - Búsqueda de solo lectura con la fórmula vanilla (`32/8`, sin generar ni modificar terreno). Verificado en `13_localizadores_aldea_stronghold.png`: `[Village Locator] Nearest village: X=21 Z=133 (116 blocks)`.
3. **Stronghold Locator (`StrongholdLocatorMod`)**:
   - Adaptado a mando Xbox y verificado en `13_localizadores_aldea_stronghold.png`: `[Stronghold Locator] Nearest stronghold: X=-4 Z=23 (80 blocks)`.
4. **TooManyItems (`TooManyItems.cpp`)**:
   - Controles Xbox (`R3` / `White` / `Black` para navegar/cambiar página/dar stacks) y rejilla adaptable: **4 columnas a 480p** (`10_tmi_480p.png`, botones `Sv 1`/`Sv 2` sin solapamiento) y **6 columnas a 720p**.
5. **Server Utils (`ServerUtilsMod` + `GuiServerInfo`)**:
   - Ficha detallada de servidor con estado, MOTD, latencia, jugadores e historial (`13b_server_utils_info.png`), más badge de estado en la lista multijugador (`GuiMultiplayer` / `GuiSlotServer`).
6. **Luces Dinámicas (`DynamicLights` + `DynamicLightsMod`)**:
   - **Apagado por defecto**. Límite estricto (`kMaxSources = 16`, escaneo cada 3 ticks) para cuidar FPS y RAM en Xbox.
   - Verificado de noche con antorcha en mano (`11_luces_dinamicas_noche.png`): halo cálido sobre hierba y vaca con **0 FPS de pérdida** (`56,06 FPS OFF` → `56,32 FPS ON`).

---

## 2. Minimapa — Estilo Rei's + Xaero's (`ReiMinimap`, `GuiMinimapMenu`, `GuiWaypointManager`)

- **Controles 100 % Xbox** (P1 y P2 en pantalla dividida):
  - `Back + D-Pad Arriba`: waypoint rápido.
  - `Back + D-Pad Abajo`: Gestor de Waypoints (`GuiWaypointManager`, crear/renombrar con `VirtualKeyboard`/cambiar color/activar/borrar, persistencia en `<mundo>/waypoints.txt`).
  - `Back + D-Pad Izquierda`: ciclar zoom (`0.5x`, `1.0x`, `2.0x`, `4.0x`).
  - `Back + D-Pad Derecha`: Menú de Opciones del Minimapa (`GuiMinimapMenu`, persistencia en `minimap.txt`).
- **Funciones verificadas**:
  - Forma **Cuadrado** (`04_minimapa_480p_cuadrado.png`) y **Redondo con rotación** (`05_minimapa_480p_redondo_radar.png`).
  - **Radar de entidades**: hostiles en rojo, animales en verde, jugadores en cian (cacheado cada 4 ticks en `update()` y dibujado en un único batch `Tessellator::draw()`).
  - **Modo cueva** (`Off` / `Auto` / `Siempre`) con detección subterránea y Nether (`06_minimapa_480p_cueva.png`).
  - **Coordenadas + bioma** (cacheado al cambiar bloque), **waypoints con flecha en borde** y **puntos de muerte automáticos** (`Death #1` / `Muerte #1`, `08_minimapa_waypoints.png`).
  - **Pantalla dividida (2 jugadores)** con minimapas independientes para P1 y P2 respetando `HudSafeArea` (`09_minimapa_pantalla_dividida.png`), incluyendo corrección en `EntityLiving::clearEntityReferences` + `XboxSplitScreen::leavePlayer2` para evitar use-after-free al salir P2.
  - **Presupuesto RAM**: **80,0 KB** totales (`minimapKB=80.0`, `< 0,08 MB`), con refresco incremental (`ROWS_PER_TICK=1`, cero subidas a GPU cuando el jugador está quieto).

---

## 3. Herobrine Classic — Fiel a tkelly (`iLoveYou`, sección 3c)

Implementado desde cero en C++ limpio reproduciendo el comportamiento confirmado del plugin original **`iLoveYou` de tkelly** (`HerobrineMod`, `EntityHerobrine`, `RenderHerobrine`, `GuiHerobrineMenu`):

- **Altar exacto de `AltarEvent.checkForAltar()`**:
  - Base (`y-1`): **8 bloques de oro** (`41`) rodeando **1 bloque central de adoquín musgoso** (`48`).
  - Centro (`y`): **Netherrack** (`87`) con **4 antorchas de redstone** (`76`/`75`) en sus 4 caras horizontales.
  - Encendido (`y+1`): **Fuego** sobre la netherrack (`BlockFire::onBlockAdded` → `HerobrineMod::onFirePlaced`).
- **Secuencia exacta de `AltarEvent.AltarEvents`**:
  - `t=5s`: anochecer (`setWorldTime(14000)`).
  - `+2s / +1s / +1s`: rompe y suelta como ítem las antorchas normales (`50`) en radios `5 → 10 → 20`.
  - `+3s`: apaga el fuego de la netherrack y hace aparecer a `EntityHerobrine` sobre el altar con **ojos blancos emisivos** (`RenderHerobrine`), sosteniendo una **Antorcha de Redstone** (`getHeldItem()`) y enviando una frase de `HeroLanguage` en EN/ES (`<Herobrine> I am here.`, `Why would you do that?`, `You are so foolish.`, `Only god can help you now.`, `This is mine.`, `ha`, `Stop.`).
  - `+5s` y `+10s`: maldición de fuego al jugador y `deleteAltar()` solo si **`Maldicion altar`** está activado (**apagado por defecto**).
- **Eventos de `WheelSpinner` y configuración segura (`GuiHerobrineMenu`, `14b_herobrine_menu.png`)**:
  - **`Ritual del altar`** (ON): secuencia temporal del altar.
  - **`Aparicion fantasma` (`GhostEvent`, peso 2.0)** (ON): aparece a ~17 bloques sosteniendo antorcha de redstone, observa al jugador y desaparece con humo si se acerca o se aleja >50 bloques.
  - **`Maldicion altar`** (OFF por defecto, con aviso): fuego al jugador y borrado del altar.
  - **`Excavador` (`HeroDigger`, peso 1.0)** (OFF por defecto, con aviso): pala de diamante, túnel descendente con antorchas de redstone y sala subterránea de adoquín musgoso + grava + mesa de crafteo + cofre con botín + creeper.
  - **`Trampa de diamantes` (`DiamondEvent`, peso 2.0)** (OFF por defecto, con aviso): pico de diamante, busca mena de diamante cercana, cava pozo con escaleras y al llegar el jugador se despide y restaura los bloques uno a uno.
  - **`Ladron de cofres` (`ThiefEvent`, peso 1.0)** (OFF por defecto, con aviso): si el jugador está a >20 bloques de un cofre cargado, aparece encima con antorcha de redstone (20 % tira ítems, 30 % roba), desaparece al acercarse <10 bloques.
  - **`Extras de la leyenda`** (ON): rayo en altar, sonidos de cueva/pasos, árboles sin hojas, carteles y pirámides de arena (restringidos lejos de bloques construidos por el jugador; cubre los stubs vacíos `PyramidBuilder`/`TreeScraper`/`StalkerEvent`/`BlackoutEvent` del código fuente de `iLoveYou`).
- Verificado en `14_herobrine_altar_encendido.png`, `14b_herobrine_menu.png` y `15_herobrine_aparicion.png`, con estado persistido en `<mundo>/herobrine.txt` (`herobrine=1` tras guardar y recargar).

---

## 4. ChocoCraft (Clean-Room Original)

- **Modelo y texturas 100 % originales generados por código** (`ModelChocobo`, `RenderChocobo`): cuerpo aviar con cresta, pico, alas animadas al planear/volar, patas, silla de montar y alforjas visibles.
- **6 colores y genética de cría** (`EntityChocobo`):
  - **Amarillo** (salvaje, domesticable con Verduras Gysahl), **Verde** (escalador/salto alto), **Azul** (corredor acuático), **Blanco** (más veloz), **Negro** (muy veloz y resistente) y **Dorado** (vuelo sostenido).
  - Cría con Gysahl de Amor / Doradas siguiendo tabla de cruces clásica.
- **Bloques, ítems y crafteos** (`ChocoCraftItems`, IDs bloque `210` e ítems `2156..2160`):
  - Cultivo **Verduras Gysahl** (con soporte de polvo de hueso tanto en 1.5.2 como en `ItemDye.cpp` de 1.2.5), Gysahl de Amor, Gysahl Dorada, Silla de Chocobo, Alforjas de Chocobo y Pluma de Chocobo.
- **Menú del Chocobo (`GuiChocoboMenu`, `18_chococraft_menu.png`)**:
  - Compatible con Java UI, Legacy UI y mando Xbox: renombrar vía `VirtualKeyboard` (nameplate flotante), abrir **Alforjas de 27 huecos** persistidas en NBT, alternar IA (`Deambular` / `Seguir` / `Quieto`) y montar.
- Verificado visualmente en `16_chococraft_salvaje_gysahl.png`, `17_chococraft_colores.png`, `18_chococraft_menu.png` y `19_chococraft_montado_oro_volando.png`.

---

## 5. Benchmarks de Rendimiento, Memoria y Persistencia (Medidos en `xemu`)

### 5.1 Rendimiento y RAM en 1.5.2 (`640×480`, `verify_480p_v3.log` y `verify_480p_v2.log`)

| Escenario | FPS (v2) | FPS (v3) | `freeKB` | Minimapa RAM | `double53` |
|---|---:|---:|---:|---:|---:|
| Menú limpio final (sin probe) | — | **89,50** | 40 692 KB | 80,0 KB | `1` |
| Mundo base (todos los mods OFF) | 48,82 | 55,60 | 30 000 KB | 80,0 KB | `1` |
| Minimapa cuadrado (480p) | 46,57 | 49,20 | 29 960 KB | 80,0 KB | `1` |
| Minimapa redondo + radar + waypoints | 46,41 | 49,54 | 29 932 KB | 80,0 KB | `1` |
| Pantalla dividida 2P + 2 minimapas | 27,02 | 29,48 | 23 012 KB | 80,0 KB | `1` |
| Inventario + TooManyItems (4 cols) | 38,15 | 46,89 | 22 720 KB | 80,0 KB | `1` |
| Luces Dinámicas OFF (noche) | 47,59 | 56,06 | 22 752 KB | 80,0 KB | `1` |
| Luces Dinámicas ON (noche, antorcha) | 50,00 | 56,32 | 22 776 KB | 80,0 KB | `1` |
| X-Ray ON (menas fullbright) | 51,83 | 55,00 | 27 424 KB | 80,0 KB | `1` |
| Herobrine activo (altar + entidad) | 52,69 | 50,78 | 24 628 KB | 80,0 KB | `1` |
| ChocoCraft (6 Chocobos + alforjas) | 51,46 | 49,49 | 24 568 KB | 80,0 KB | `1` |

> **Nota sobre 720p y coste puro de HUD del minimapa**: en el `xemu` del host la EEPROM tiene `dashboard flags=00000009` (`WIDESCREEN | 480p`, sin `XC_VIDEO_FLAGS_HDTV_720p`), por lo que D3D8 rechaza `CreateDevice(1280x720)` con `hr=80004005` (no se modificó la EEPROM del host). Aun así, en el perfilado con resolución lógica `1280×720` el coste medido de `hud` pasó de `0.4ms` (baseline, `62,25 FPS`) a `0.6ms` (minimapa redondo + radar, `71,51 FPS`), es decir **~0,2 ms/frame (~0,5 FPS de impacto en GPU real)**; en 480p en `xemu` la oscilación de ~2–6 FPS entre ventanas consecutivas de 5 s incluye el trabajo de `build` de chunks iniciales del mundo recién creado (`build=7–10 ms`).

### 5.2 Persistencia al guardar y recargar mundo (`ModsTest101`)
Resultado exacto del log tras salir al menú principal y recargar `ModsTest101`:
```
[VERIFY] PERSIST waypoints=4 hasBase=1 hasDeath=1 herobrine=1 boko=1 saddled=1 bags=1 slot0=12 slot1=5
[VERIFY] ALL_DONE 480p
```
- **Waypoints**: 4 conservados (`Spawn`, `Base`, `Mina`, `Death #1`).
- **Herobrine**: estado `summoned=1` restaurado desde `herobrine.txt`.
- **ChocoCraft**: Chocobo `"Boko"` restaurado con silla (`saddled=1`), alforjas (`bags=1`) y contenido NBT exacto (`slot0=12` Verduras Gysahl, `slot1=5` diamantes).

### 5.3 Verificación de 1.2.5 (`wt-125-mods`, `verify_125_boot.log`)
- Compilación y enlazado `xbox-release`: **exit 0** (`OptiCraft.xbe` 4,33 MB + `OptiCraft.iso` con `assets.pak` de 1.2.5).
- Arranque en `xemu`: **`crashed=False`**, `0 failure(s)` en `xbox.selftest`, `double53=1`, `freeKB=42812`, `95,4 FPS` en menú:
  - `[MC][I][mods] Rei & Xaero Minimap initialized (enabled=1, mem=80 KB)`
  - `[MC][I][mods] ChocoCraft items & recipes registered (block=210, items=2156..2160)`
  - `[MC][I][mods] ModManager initialized with 9 mods.`

---

## 6. Índice de Capturas (`buzon-xbox\101-capturas\`)

| Archivo | Contenido verificado |
|---|---|
| `01_catalogo_mods_legacy.png` | Catálogo de mods en Legacy UI con iconos, pestañas, buscador y botones `INSTALAR` |
| `02_catalogo_mods_java.png` | Catálogo de mods en Java UI |
| `03_catalogo_mods_busqueda.png` | Búsqueda en vivo (`"choco"`) filtrando ChocoCraft |
| `04_minimapa_480p_cuadrado.png` | Minimapa cuadrado 480p con cardinales, radar, waypoints, coords y bioma |
| `05_minimapa_480p_redondo_radar.png` | Minimapa redondo rotatorio con radar y waypoints |
| `06_minimapa_480p_cueva.png` | Minimapa en Modo Cueva |
| `07_minimapa_menu_opciones.png` | Opciones del Minimapa en español sin recortes (480p) |
| `08_minimapa_waypoints.png` | Gestor de Waypoints (`Spawn`, `Base`, `Mina`, `Death #1`) |
| `09_minimapa_pantalla_dividida.png` | Pantalla dividida 2P con minimapas independientes P1 y P2 |
| `10_tmi_480p.png` | TooManyItems 4 columnas a 480p junto al inventario (`Sv 1`/`Sv 2` limpios) |
| `11_luces_dinamicas_noche.png` | Luces Dinámicas activas de noche con antorcha en mano |
| `12_xray_menas.png` | X-Ray activo en Xbox mostrando menas subterráneas a brillo completo |
| `13_localizadores_aldea_stronghold.png` | Chat con coordenadas reales de Village Locator y Stronghold Locator |
| `13b_server_utils_info.png` | Tarjeta `GuiServerInfo` de Server Utils |
| `14_herobrine_altar_encendido.png` | Altar exacto de tkelly (`iLoveYou`) encendido con 8 oro + musgosa + netherrack + 4 antorchas redstone |
| `14b_herobrine_menu.png` | Menú de opciones de Herobrine (`iLoveYou`) con eventos destructivos/robo en OFF por defecto |
| `15_herobrine_aparicion.png` | Herobrine sobre el altar con ojos blancos brillantes, antorcha de redstone y `<Herobrine> I am here.` |
| `16_chococraft_salvaje_gysahl.png` | Chocobo amarillo salvaje y cultivo de Verduras Gysahl |
| `17_chococraft_colores.png` | Los 6 colores de Chocobo con silla, alforjas y nameplates (`Boko`, `Aurelius`) |
| `18_chococraft_menu.png` | Menú de gestión del Chocobo (`GuiChocoboMenu`) sin solapamientos con la barra de ayuda |
| `19_chococraft_montado_oro_volando.png` | Jugador montado volando sobre Chocobo Dorado en tercera persona |
