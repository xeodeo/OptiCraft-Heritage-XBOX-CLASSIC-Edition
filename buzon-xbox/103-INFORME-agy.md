# Informe Encargo 103 — Buscar y descargar skins por nombre de usuario desde la consola (1.2.5 y 1.5.2)

- **Ramas y commits**:
  - **1.5.2 (`port152-erluza`, `wt-152-rend`)**: `a2c217e4` (sobre `950da989`)
  - **1.2.5 (`opti30-erluza`, `wt-opti30`)**: `9b7c8f06` (sobre `0e1a3b67`)
- **Presets compilados en limpio (sin sonda)**: `xbox-release` y `xbox-release-exp` en **ambas** versiones (4/4 OK, 0 errores). Sin `git push`, sin `git reset`, sin copiar binarios al lanzador.
- **Capturas**: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\103-capturas\`

---

## 1. Resumen de Implementación

### A. Red HTTP plano en Xbox con timeouts estrictos (`XboxNetwork.h/.cpp`, `JavaNetwork.h`, `JavaNetworkXbox.cpp`, `JavaNetwork.cpp`)
- Añadidas variantes con timeout configurable en `XboxNetwork`: `resolveIPv4Timeout`, `tcpConnectTimeout`, `tcpRecvTimeout`, `tcpSendTimeout`, `hasLink`, `tickMs`.
- Detección inmediata sin enlace Ethernet (`XNetGetEthernetLinkStatus() == 0`) para no esperar DHCP/DNS si la consola está sin red (devuelve «Sin conexión» en ~50 ms en lugar de bloquearse).
- Nuevo cliente HTTP plano `JavaNetwork::httpGetUrl(url, outData, &statusCode, maxBytes=64 KB, timeoutMs=8000)` sobre `XNet` (`XNET_STARTUP_BYPASS_SECURITY`):
  - Soporta `HTTP/1.0`, `Connection: close`, `Transfer-Encoding: chunked`, hasta 3 redirecciones `http://`, límite estricto de 64 KB y presupuesto global de tiempo ≤ 8 s.
  - Distingue limpiamente `HttpResult::Ok` (200), `HttpResult::NotFound` (404/400/410/422), `HttpResult::NoConnection` (sin red/DNS/connect/timeout), `HttpResult::TooLarge` y `HttpResult::InvalidResponse`.
  - `JavaNetwork::readUrl` y `getResponseCode` en Xbox reutilizan también `httpGetUrl`.
- **Fuentes HTTP plano soportadas**:
  - Principal (por nombre de usuario `1..16` chars `[A-Za-z0-9_]`): `http://minotar.net/skin/<usuario>`
  - Respaldo 1 (UUID de 32 hex chars con o sin guiones): `http://crafatar.com/skins/<uuid-sin-guiones>`
  - Respaldo 2 (hash de textura de 64 hex chars): `http://textures.minecraft.net/texture/<hash>`

### B. Validación PNG, conversión 64×64 → 64×32, detección Slim (Alex) y guardado (`SkinManager.h/.cpp`)
- Validación estricta de firma PNG (`\x89PNG\r\n\x1a\n`), decodificación en memoria con `stb_image` y validación de dimensiones (`64×32` o `64×64`).
- Conversión `SkinManager::convertSkinToRetro32` y `makeRetro32`:
  - Para skins `64×64`: fusiona las capas de ropa del torso, brazo derecho y pierna derecha sobre la mitad superior `64×32` y fuerza alfa opaco en regiones base de 1.2.5/1.5.2.
  - **Detección automática de skins slim (Alex, brazos de 3 px)**: comprueba las columnas exclusivas del brazo clásico (`x=54..55, y=20..31`; `x=50..51, y=16..19`; `x=46..47, y=52..63`). Cuando la skin es slim, **remapea limpiamente el brazo de 3 px a 4 px** para evitar franjas negras en los brazos y avisa al jugador (`"Skin lista (aviso: slim/Alex adaptada a 64x32)"`).
- `SkinManager::saveDownloadedSkin`:
  - Guarda `<usuario>.png`, `<usuario>_32.png` (si era 64×64) y `<usuario>_Front.png` en `SkinManager::getSkinsDir()` (`T:/.minecraft/skins/` en Xbox, escaneado junto con `D:\skins` para copias manuales por FTP).
  - Conserva el `_` final en nombres legítimos de Minecraft como `jeb_`.
  - Invalida la textura en caché de `RenderEngine` si se re-descarga el mismo usuario y, si se elige «Usar», equipa inmediatamente la skin en J1 o J2 (incluida pantalla partida) y guarda `options.txt`.
- `SkinManager::deleteCustomSkin`: borra `.png` + `_32.png` + `_Front.png`, libera sus texturas GPU y, si la skin borrada estaba equipada por J1 o J2, revierte limpiamente a la skin por defecto.

### C. Pantalla «Buscar skin» (`GuiSearchSkin.h/.cpp`) e integración en rueda (`GuiSkinSelector.h/.cpp`, `GuiLoadSkinsMenu.cpp`)
- **Descarga en hilo secundario** (`std::thread` desacoplado con `std::shared_ptr<SearchJob>` + `std::mutex` + `std::atomic`): la UI nunca se congela durante la descarga (mantiene >65 fps y permite cancelar en cualquier instante sin bloqueo ni use-after-free).
- **Dos interfaces completas**:
  - **Legacy UI (`legacyUI = true`)**: fondo panorama LCE / pausa, panel 3D clásico biselado, barra inferior de iconos de mando Xbox (`A Elegir`, `Y Teclado`, `X Popular`, `B Volver`), navegación completa por D-Pad/stick y rotación 3D con stick derecho.
  - **Java UI (`legacyUI = false`)**: `drawDefaultBackground()`, paneles oscuros con bordes limpios, botones estándar navegables por puntero/ratón o teclado/mando.
  - **Pantalla dividida (J2)**: layout compacto automático cuando `height < 180`, respetando `mc->isScreenOwnedByPlayer2()` para equipar directamente sobre el Jugador 2.
- **Vista previa 3D** con `ModelBiped` giratorio y textura temporal en GPU (`allocateAndSetupTexture` / `deleteTexture` al cambiar o salir, sin fugas de RAM).
- **Teclado en pantalla** integrado vía `GuiTextField` + `VirtualKeyboard` (al pulsar Enter lanza la búsqueda automáticamente).
- **Skins populares sugeridas** (punto 4 opcional incluido): selector rápido `< Popular: Notch >` (`Notch`, `jeb_`, `Dinnerbone`, `Dream`, `Technoblade`, `Grian`, `Alex`, `MHF_Steve`, `Herobrine`, `Vegetta777`) para probar y descargar con un botón sin escribir.
- **Rueda de skins (`GuiSkinSelector`)**:
  - Grupo mostrado en español como **«Skins personalizadas»** (tanto en modo Legacy 3D como en modo Java), incluyendo skins descargadas online y skins copiadas por FTP a `D:\skins` o `skins/`.
  - Acceso directo a «Buscar skin»: botón `Y` (`Buscar`) en modo Legacy, botón `Buscar Skin` en modo Java (4 botones inferiores repartidos sin solapamiento a 640×480), y primera opción en `GuiLoadSkinsMenu` (ahora también con mando Xbox completo).
  - Borrado de skin personalizada desde la rueda (`X` en Legacy / `Eliminar` en Java) sin expulsar del selector al borrar la última.

---

## 2. Resultados de Pruebas en `xemu` (64 MB RAM)

### A. Prueba completa con red, reinicio y sin red (`1.5.2-exp`, `xemu` 64 MB)

| Escenario | Resultado | Evidencia medida |
|---|---|---|
| Usuario 1: `Notch` (64×32, Legacy UI) | **OK** | `USER1_NOTCH ok=1 slim=0 dtMs=176 ticks=4 status='Skin lista (64x32)'`, guardado OK |
| Usuario 2: `Dinnerbone` (64×64 clásico → 64×32) | **OK** | `USER2_DINNERBONE ok=1 slim=0 saved=1 dtMs=115 status='Guardada en Skins personalizadas (Dinnerbone)'` |
| Usuario 3: `Dream` (64×64 **slim/Alex** → 64×32, Java UI) | **OK** | `USER3_DREAM ok=1 slim=1 dtMs=116 status='Skin lista (aviso: slim/Alex adaptada a 64x32)'`, `USE_DREAM=1 customCount=3 selId=custom_Dream` |
| Borrado de skin personalizada (`jeb_`) | **OK** | `DELETE_TEST saved=1 before=3 deleted=1 after=2` |
| Usuario inexistente (`NoExisteUsuario99887766`) | **OK** | `NOT_FOUND dtMs=14 status='Usuario no encontrado'` (HTTP 404, sin cuelgue) |
| En partida (1ª y 3ª persona con `Dream`) | **OK** | `INWORLD tex='T:/.minecraft/skins/Dream_32.png'`, brazo 1ª persona y cuerpo 3ª persona sin franjas negras |
| **Reinicio completo de `xemu`** | **OK** | `RESTART_CHECK count=3 names=[Notch,Dinnerbone,Dream] selId=custom_Dream activeTex=T:/.minecraft/skins/Dream_32.png`; al volver a entrar al mundo `RESTART_INWORLD tex='T:/.minecraft/skins/Dream_32.png'` |
| **Sin red (`[net] enable = false`)** | **OK** | `OFFLINE_RESULT dtMs=50 ticks=1 status='Sin conexion'` (sin cuelgue, menú a >87 fps) |

### B. Rendimiento, Memoria (64 MB) y Estabilidad

- **FPS en mundo tras usar la pantalla de skins**: `76.5 fps` (`frame=13.1ms`, `double53=1`).
- **Memoria `freeKB` (1.5.2, 64 MB)**:
  - Inicio título (con sonda): `40124 KB` → tras descargar y previsualizar 4 usuarios: `39340 KB` → en partida: `40192 KB` → estable al terminar: `37792 KB` → tras reiniciar: `40124 KB` (en mundo `40500 KB`).
- **Arranque limpio (ISOs finales SIN sonda en `xemu` 64 MB)**:
  - **1.5.2-exp**: `double53=1`, `freeKB = 45512 -> 45232 KB`, `fps = 81.5`, `0` crashes, `0` `[X87]`, `0` `NaN`.
  - **1.2.5-exp**: `double53=1`, `freeKB = 46476 -> 46212 KB`, `fps = 87.6`, `0` crashes, `0` `[X87]`, `0` `NaN`.

---

## 3. Capturas en `buzon-xbox\103-capturas\`

1. `01_search_legacy_notch.png` — Pantalla «Buscar skin» en **Legacy UI** tras descargar `Notch` (64×32) con vista previa 3D y prompts de mando Xbox.
2. `02_search_java_dream_slim.png` — Pantalla «Buscar skin» en **Java UI** tras descargar `Dream` (64×64 slim/Alex) mostrando el aviso `"Skin lista (aviso: slim/Alex adaptada a 64x32)"` en 2 líneas y vista previa 3D sin rayas negras.
3. `03_search_not_found.png` — Error `"Usuario no encontrado"` en rojo tras buscar `NoExisteUsuario99887766`.
4. `04_wheel_custom_skins_legacy.png` — Rueda 3D **Legacy UI** en el grupo **`Skins personalizadas (3)`** mostrando `Dinnerbone`, `Dream` (`Seleccionado`) y `Notch`, con los prompts `A Seleccionar  B Volver  Y Buscar  X Eliminar`.
5. `05_ingame_1st_person_dream.png` — En partida en **1ª persona** mostrando el brazo de la skin descargada (`Dream`).
6. `06_ingame_3rd_person_dream.png` — En partida en **3ª persona** mostrando el cuerpo completo de `Dream` (brazos slim adaptados limpiamente a 64×32).
7. `07_after_restart_wheel_java.png` — Rueda **Java UI** tras **reiniciar `xemu`** mostrando las 3 skins persistidas en `Skins personalizadas (3 / 3)` y los 4 botones inferiores alineados (`Eliminar`, `Buscar Skin`, `Cargar`, `Skin Jug. 2`).
8. `08_no_connection_error.png` — Prueba **sin red (`[net] enable = false`)** mostrando el mensaje `"Sin conexion"` en 50 ms sin congelar la consola.

---

## 4. Ficheros Modificados y Creados (12 ficheros en ambas ramas)

- **Nuevos (2)**:
  - `src/net/minecraft/src/skin/GuiSearchSkin.h`
  - `src/net/minecraft/src/skin/GuiSearchSkin.cpp`
- **Modificados (10)**:
  - `src/xbox/system/XboxNetwork.h` / `.cpp`
  - `src/java/JavaNetwork.h` / `JavaNetworkXbox.cpp` / `JavaNetwork.cpp`
  - `src/net/minecraft/src/skin/SkinManager.h` / `.cpp`
  - `src/net/minecraft/src/skin/GuiSkinSelector.h` / `.cpp`
  - `src/net/minecraft/src/skin/GuiLoadSkinsMenu.cpp`
