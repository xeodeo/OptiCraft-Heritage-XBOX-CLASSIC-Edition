# 116 — Informe (subagente): descargar texture packs de Modrinth en la consola

Fecha: 22-11-2026. Ramas: `opti30-net` (`wt-125-net`, commit **1928c764**) y `port152-net`
(`wt-152-net`, commit **71bc9877**, mismo cambio por cherry-pick). No se ha tocado nada fuera de esos dos
worktrees, no se ha usado xemu ni se ha subido nada a la consola.

Compila: xbox-release y xbox-release-exp en las dos ramas (ver sección 8). "SSE2 left in" = 13.

## 1. Qué ve el jugador

### Pantalla de texture packs (`GuiTexturePacks`)
* **Legacy (LCE)**: panel gris con el título, cada pack como fila de botón de piedra (como el catálogo de
  mods): miniatura `pack.png`, nombre (amarillo al seleccionarlo), descripción en una línea, casilla con tick en
  el pack en uso. Los packs que no se pueden usar salen en gris. Debajo, botón **"Descargar de internet"**
  (se llega bajando con la cruceta). Pistas abajo: Cruceta Mover · A Usar/Abrir · X Descargar · B Volver.
* **Java**: igual que antes, pero en Xbox el botón izquierdo ("Abrir carpeta", que en consola no servía) pasa
  a ser **"Descargar de internet"**, y X también abre la descarga. El texto gris de abajo lo dice.

### Pantalla de descarga (`texdl/GuiTexturePackDownload`)
* Se abre **directamente con listas listas**, sin escribir nada, con el **icono de cada pack**:
  Populares (más descargados), Nuevos, Al día (actualizados), Seguidos (más seguidos). **White/Black**
  cambian de lista; **Izquierda/Derecha** cambian de página (10 packs por página, "Página 2/25"); bajar en la
  última fila pasa a la siguiente página. **X** abre el teclado en pantalla para buscar por nombre (Start
  busca, B cierra); la búsqueda aparece como 5ª pestaña "Buscar".
* Fila: icono, título, "de Autor · 1,2 M descargas". En 720p hay una ficha a la derecha (icono 48 px,
  autor, descargas, seguidores, descripción entera); en 480p una tira de dos líneas bajo la lista.
* **A** abre la ficha del pack: pide a Modrinth la versión más nueva que diga ser para 1.2.5 / 1.5.2 y
  muestra nombre de la versión, tamaño, rango de versiones de MC, y un aviso:
  * verde "Listo para descargar";
  * amarillo "Pack multiversión: puede que la consola no lo pueda usar" si el archivo dice valer también
    para 1.13+ (casi siempre es un pack moderno con nombres de textura cambiados);
  * rojo "Demasiado grande" (> 120 MB) o "No hay archivo para 1.x".
* **A** en la ficha descarga: barra de progreso con MB, velocidad y "Preparando para la consola… NN %"
  mientras convierte. **B cancela** (borra lo descargado).
* Al acabar: "Instalado" + `texturepacks\<nombre>.pak|.zip` + formato y resolución, **A "Usar ahora"**
  (lo elige y recarga texturas) o B volver. Al volver a la lista de packs ya aparece (sin reiniciar).
* Errores claros: sin conexión, el archivo ya no existe, demasiado grande, no se pudo guardar (¿disco
  lleno?), archivo roto (tamaño o SHA-1 no cuadran), **"No compatible con la consola"**, pack de más de 64x,
  falta memoria (prueba desde el menú principal).
* Todo cabe en 480p y 720p (usa `legacyMenuLayout`, zona segura del HUD, fila de pistas abajo con 2–4
  pistas; las de White/Black y página van dentro del panel como en el catálogo de mods). Solo mando.
* Al salir: se cancelan los trabajos, se liberan los iconos y se llama a `XboxTls::shutdown()`.

## 2. Ficheros
| Fichero | Qué es |
|---|---|
| `src/net/minecraft/src/GuiTexturePacks.{h,cpp}` | Lista LCE + entrada de descarga |
| `src/net/minecraft/src/texdl/GuiTexturePackDownload.{h,cpp}` | Pantalla de tienda/descarga (todo dentro de `#if XBOX_PLATFORM`; en otras plataformas es un stub) |
| `src/net/minecraft/src/texdl/TexDlJson.{h,cpp}` | Lector JSON mínimo (pull parser, sin árbol): búsqueda, versiones, `pack.mcmeta` |
| `src/net/minecraft/src/texdl/TexDlImage.{h,cpp}` | Iconos → 32×32 RGBA (PNG/JPG/GIF con stb, WebP con simplewebp) |
| `src/net/minecraft/src/texdl/TexDlConvert.{h,cpp}` | Conversión en consola (sección 4) y escritor de `.pak` |
| `src/net/minecraft/src/texdl/TexDlTables.h` | Tablas generadas de `texturas/datos` (celdas 1.5, celdas nuevas de 1.5.2, cuadrantes de agua/lava, tamaños de referencia de 1.2.5 y 1.5.2) |
| `src/java/JavaNetworkStream.h` + `src/java/JavaNetworkXbox.cpp` | `httpGetStream` / `httpGetToMemory` |
| `src/external/simplewebp.{h,c}` | Decodificador WebP (BSD-3, de libwebp; C puro, sin SIMD ni coma flotante) |

La versión del juego se decide al compilar con `__has_include("Texture152Atlas.h")` (solo existe en el port
1.5.2), así el código es idéntico en las dos ramas.

## 3. Red
* `httpGetStream`: GET HTTP/1.1 con `Connection: close`, redirecciones (hasta 6), cuerpo con
  Content-Length, por trozos (decodificador incremental) o hasta el cierre. Timeouts: 15–20 s para
  conectar+TLS, 20–30 s de espera máxima entre lecturas (no un total fijo de 8 s como `httpGetUrl`). Bandera de
  cancelar comprobada entre lecturas. User-Agent `OptiCraft-Xbox/1.0 (texture packs; Xbox)`.
* Búsqueda: `api.modrinth.com/v2/search?limit=10&offset=…&index=downloads|newest|updated|follows|relevance`
  `&facets=[["project_type:resourcepack"],["versions:<ver>"]]` (+ `query=` al buscar), URL-codificado.
* Versiones: `/v2/project/<id>/version?game_versions=["<ver>"]` → la de `date_published` más reciente que
  lista nuestra versión, su fichero primario (o el primer `.zip`), tamaño y SHA-1.
* Descarga: a `D:\texturepacks\<nombre>.dl` (el juego no lo lista), SHA-1 calculado mientras llega
  (mbedTLS), comprobado tamaño y hash. Nombre = título apto para FATX (≤ 34 letras + " 2", " 3"… si ya existe).
* Todo en hilos de trabajo (`std::thread` separado), el hilo de dibujo solo consulta atómicos.

Probado en el PC: el lector JSON con respuestas reales de Modrinth (8 búsquedas: 1.2.5 → 244 packs,
1.5.2 → 266; 5 respuestas de versiones, incluidas con 334 versiones de juego y snapshots).

## 4. Packs formato 1.5 en la consola (cómo funciona)
Después de descargar, `convertDownloadedPack` (hilo de trabajo):
1. Abre el zip con minizip y hace un índice ordenado (clave en minúsculas → posición). Busca la raíz del pack
   como `convertir_pack.py` (carpeta menos profunda con `pack.txt`/`pack.mcmeta`/`terrain.png`/`pack.png`,
   o encima de `textures/blocks/` / `assets/minecraft/`). Detecta 1.2–1.4 (`terrain.png`), 1.5
   (`textures/blocks/*.png`) y 1.6+ (`assets/minecraft/...`).
2. **Se queda tal cual** (renombrado a `.zip`, lo lee `TexturePackZip`): pack 1.2–1.4 de hasta 32x en 1.2.5.
3. **Se convierte a `.pak`** en todo lo demás: 1.5, 1.6+, atlas de 64x (se reduce a 32x; más de 1024 px →
   "demasiado grande"), y cualquier pack antiguo en 1.5.2 (para rellenar las 79 celdas que el port reasignó —
   esmeralda, cuarzo, patatas… — con las del juego; si no, salían mal).
   * Celda = potencia de 2 entre 16 y 32 según la resolución del pack.
   * Atlas `terrain.png` / `gui/items.png`: base = atlas del pack (reducido) o el **atlas del propio juego**
     (leído con `GameResources::open` en el hilo de dibujo antes de empezar) ampliado; encima, **una textura
     cada vez**: decodificar PNG → primer fotograma de las tiras animadas → reducir con media ponderada por
     alfa (o ampliar al vecino) → pegar en su celda según la tabla de 1.5. En 1.2.5 se saltan las celdas
     nuevas de 1.5.2 y el cuero de 1.5 se tiñe de marrón con su `_overlay`. `water_flow`/`lava_flow` van a
     sus 4 cuadrantes.
   * Resto de ficheros que el juego lee (mob/, armor/, item/, art/, misc/, environment/, terrain/,
     particles.png, reloj/brújula de 1.5.2, OptiFine de packs 1.2): recortados como en el PC (zombi 64×64 →
     64×32) y reducidos a la celda. `pack.png` ≤ 128 px. `pack.txt` (2 líneas) o la descripción de
     `pack.mcmeta`, o "Título / Modrinth - autor".
   * PNG codificado con stb_image_write usando el deflate de zlib. El `.pak` (formato MCPK de `make_pak.py`) se
     escribe en `<nombre>.pak.tmp` en streaming (datos primero, tabla y cabecera al final) y se renombra.
4. Se borra el zip. Si no sale ningún atlas ni textura útil (pack 1.13+ con `textures/block/`, packs solo de
   GUI o sonidos) → **"No compatible con la consola"** y se borra todo.

**Comprobado en el PC** (mismo código compilado con `TEXDL_HOST_TEST`) contra `convertir_pack.py` con los 8
packs de `texturas\descargas`: JoliCraft 1.5.2 16x y John Smith Legacy 32x dan **atlas idénticos píxel a
píxel** y los mismos ficheros que el conversor del PC; Soartex 64x → 32x difiere en < 300 píxeles (redondeo de
la media). Los `.pak` se leen bien con el lector de formato (hash FNV y orden comprobados). Tiempo en PC:
50–340 ms por pack; en la Xbox calculo de 10 a 60 s según el pack.

## 5. Memoria
* Lista: 1 página JSON (10 packs, ~40 KB, se libera tras leerla) + textos recortados.
* Iconos: Modrinth da casi siempre `_96.webp` (96×96). Se baja uno cada vez (máx. 384 KB), se decodifica
  (96×96×4 = 36 KB; se rechaza > 1024×1024 sin decodificar) y se reduce a 32×32. **Caché LRU de 12 texturas
  de 32×32 (4 KB cada una)**, liberadas al salir. Sin icono o formato raro → `unknown_pack.png`.
  WebP con simplewebp: con 64 MB va sobrado (tablas de decodificación de unos KB). Los GIF salen con el primer
  fotograma; WebP animado probablemente sale como sin icono.
* Descarga: búfer de 8 KB + 64 KB de `setvbuf`; el zip nunca está en RAM.
* Conversión (pico): índice del zip (~60 B/fichero), un atlas 512×512 (1 MB) + atlas por defecto ampliado
  (1 MB) + la imagen que se decodifica (máx. 16 MB o 1/3 de la RAM libre; las que no caben se saltan) +
  compresión zlib (~300 KB). Para 64x: + 4 MB del atlas grande mientras se reduce. Si quedan < 6 MB libres no
  empieza ("Falta memoria").
* TLS: el de siempre (compartido) y `XboxTls::shutdown()` al salir de la pantalla.

## 6. Sin probar
* **Nada se ha probado en la consola ni en xemu**: solo compila. Lo probado en PC: JSON, iconos (65 de 66
  iconos reales decodificados; el que falla es un PNG de más de 1024 px) y la conversión.
* `httpGetStream` no se ha ejecutado nunca (es nuevo; HTTP/1.1, chunked y redirecciones revisados a mano).
* Velocidad real de descarga (una conexión, ~0,3 MB/s según la Classic Xbox Store) y de la conversión en
  el Pentium III. Hilos `std::thread` con la pila por defecto (igual que la búsqueda de skins).
* Que FATX acepte todos los nombres saneados y que `MoveFile` sobre `D:\texturepacks` funcione desde la
  carpeta del XBE en el disco duro (en DVD/ISO de xemu `D:` no se puede escribir: saldrá "No se pudo guardar").
* Fuente: los textos con letras fuera de Latin-1 (chino, emoji) se cambian por "?".
* "Usar ahora" llama a `refreshResources()` como al elegir un pack a mano.

## 7. Qué probar en la consola
1. Copiar los XBE de `wt-125-net\bin\xbox…` y `wt-152-net\bin\xbox…` a `F:\Games\OptiCraft\<versión>\`
   (con cable de red y el log UDP). Menú → Mods → Texturas (Start), o Ayuda y opciones → Texture Packs.
2. Lista LCE: miniaturas, tick en el pack en uso, botón "Descargar de internet" abajo. En Java: botón izquierdo.
3. En la descarga: que salgan las 4 listas con iconos, White/Black, izquierda/derecha (páginas), X + teclado →
   buscar "faithful", "jolicraft", "soartex".
4. Descargar un pack pequeño y de formato 1.5 (p. ej. "MC New Textures", 0,6 MB, o "Simple Wool") y uno grande
   (Soartex Grove 8 MB). Mirar en el log líneas `texdl` (búsqueda, versiones, `download … bytes`, `convert …
   result … free X -> Y KB`).
5. "Usar ahora" → entrar en un mundo y mirar bloques/objetos (agua/lava animadas, cuero marrón en 1.2.5).
6. Cancelar con B a mitad de descarga y de conversión: no deben quedar `.dl`/`.pak.tmp` en `texturepacks\`.
7. Probar sin cable de red: "Sin conexión a internet".
8. Un pack multiversión moderno (Cubic Sun & Moon, 16 MB): debe acabar en "No compatible con la consola" o
   en un `.pak` con pocas texturas.

## 8. Compilación
`build-net.ps1` (candado `BUILD.lock`): wt-125-net y wt-152-net, presets xbox-release y xbox-release-exp.
Las 4 compilan sin errores y generan ISO. "SSE2 left in": **13** en 1.2.5 release, 1.2.5 exp y 1.5.2 release;
**15 en 1.5.2 exp**: las dos de más son `lwjglKeyDisplayName` (`extrq`, instrucción SSE4a de AMD que el
compilador nunca emite) y `GuiIngame::renderGameOverlay` (`cvtdq2ps`), ninguna de código nuevo. Parecen
falsos positivos del escáner al desplazarse los bytes del ejecutable; conviene mirarlo antes de publicar esa build.
