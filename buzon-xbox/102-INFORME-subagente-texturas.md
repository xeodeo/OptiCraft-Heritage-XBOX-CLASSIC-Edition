# 102 — Informe (subagente): texture packs en Xbox, convertidor y packs clásicos

Fecha: 22-11-2026. No se ha tocado C++, ni los worktrees `wt-152-rend`, `wt-opti30` ni `wt-152-sub`
(solo se han leído). No se ha compilado, no se ha abierto xemu y no se ha subido nada a la consola.
Todo lo nuevo está en `Minecraft en xboox clasico\texturas\`.

## 1. Cómo carga packs la consola hoy

### Dónde los busca
* `D:\texturepacks\`: `wt-opti30` (1.2.5, `opti30-erluza`) y `wt-152-rend` (1.5.2, `port152-erluza`),
  `src/net/minecraft/src/TexturePackList.cpp:118-162`. Los dos worktrees tienen el mismo código.
* `D:` es la carpeta del XBE que se está ejecutando. `main_xbox.cpp:67` llama a
  `XboxGameDrive::mapToImageFolder()` (`src/xbox/system/XboxGameDrive.cpp:25-50`), que vuelve a enlazar `D:`
  a la carpeta del XBE. **En la consola real la ruta es `F:\Games\OptiCraft\<1.2.5|1.5.2>\texturepacks\`**
  (por FTP, `/F/Games/OptiCraft/<versión>/texturepacks/`). No se usa `E:\TDATA\...`.
* **`xbox-unificada` (el checkout principal) no tiene nada de esto.** Su `TexturePackList.cpp:74-112` solo
  busca packs en PC (`#if !PS2 && !WII && !XBOX`), así que una build de esa rama solo ve el pack por defecto.
  Para tenerlo hay que traer `TexturePackFolder`/`TexturePackPak` y la rama `#elif XBOX_PLATFORM` de opti30/port152.
* En el juego se llega por el menú principal: **Mods → Texture Packs** (`GuiMainMenu.cpp:310,354` →
  `mods/GuiMods.cpp:174,190-193`).

### Qué formatos acepta
| Formato | Xbox | Dónde |
|---|---|---|
| Carpeta descomprimida `texturepacks\<pack>\terrain.png…` | **Sí** | `TexturePackFolder.cpp` (lee fichero a fichero con `File::toStreamIn`) |
| `texturepacks\<pack>.pak` (MCPK de `make_pak.py`, claves sin `assets/`) | **Sí** | `TexturePackPak.cpp` + `platform/storage/PakArchive` |
| `.zip` | **No** | `TexturePackList.cpp:133` descarta lo que no sea carpeta ni `.pak` |

* `.zip` en general: `TexturePackCustom.cpp` está escrito para **libzip** (`zip_open`/`zip_fread`) y todo va
  dentro de `#ifdef MCBETA_HAVE_LIBZIP` (`TexturePackCustom.h:7`). **Ese define no está en ningún CMake de
  ninguna plataforma**, así que en PC también es código muerto: ni lee pack.txt ni sirve ficheros.
* La build de Xbox **sí enlaza minizip** (`cmake/xbox.cmake:103-104` `unzip.c`/`ioapi.c`, include en `:213`),
  pero solo lo usa `src/mods/OchPackReader.cpp` para los mods `.och`: carga el fichero entero en RAM y lo abre
  con un ioapi en memoria (`OchPackReader.cpp:259-267`). Con texture packs no se usa.
* El nombre de un `.pak` es su nombre sin `.pak` (`TexturePackPak.cpp:14-24`). FATX admite como mucho 42
  caracteres por nombre, así que en una carpeta suelta los nombres largos de algunos packs no caben. Con `.pak`
  ese problema desaparece.
* Lo que el pack no trae sale de los datos del juego (`TexturePackBase::getResourceAsStream` →
  `GameResources::open`).

### Qué esperan 1.2.5 y 1.5.2
**Las dos usan atlas de 1.2.5**: `terrain.png` y `gui/items.png`, de 16×16 celdas (`RenderEngine.cpp`
`isTileAtlasResource`, y más de 15 llamadas `getTexture("/terrain.png")`). El port 1.5.2 **no** lee
`textures/blocks/*.png` en tiempo de ejecución: `scripts/port152/build_textures_152.py` los monta en el atlas
al compilar, y la tabla nombre→celda está en `src/net/minecraft/src/Texture152Atlas.h` (248 bloques y 203
objetos). He comprobado que el `assets.pak` 1.5.2 compilado cuadra al 100 % con esa tabla. En tiempo de
ejecución solo lee `textures/items/clock.png` y `compass.png` (`TextureCompassFX.cpp:80`, `TextureWatchFX.cpp:34`).
Por tanto:
* Un pack de 1.2–1.4 vale casi tal cual en las dos. En 1.5.2 hay 39 celdas de terreno y 40 de objetos que el port
  reasignó (esmeralda, cuarzo, patata, tolva, cohetes…): hay que rellenarlas.
* Un pack de 1.5 (un fichero por textura) **no se ve** si se copia tal cual. Hay que montarlo en el atlas.
* Los modelos son los de 1.2.5: las texturas de mobs que 1.5 hizo más altas (zombi de 64×64) se recortan, y el
  cuero de 1.5 es gris y se tiñe al dibujar. El port lo hornea en marrón (`build_textures_152.py` `bake_leather`).
* Animaciones/CTM: el port entiende el formato OptiFine de 1.2.5 (`custom_*.png`, `anim/`, `ctm.png`,
  `ctm/terrainN.properties`, `color.properties` + `misc/*color*.png`). El formato de 1.5 (por nombre) no lo
  entiende. Cielos personalizados (`sky0/`) y mobs aleatorios no están implementados.
* Fallo menor: `TexturePackFolder` y `TexturePackPak` no sobrescriben `listResources` (solo lo hace
  `TexturePackCustom.h:30`, y la base devuelve una lista vacía en Xbox, `TexturePackBase.cpp:55-81`). Por eso en
  consola **no se cargan las animaciones genéricas `/anim/*.properties`** de un pack. Las `custom_*` sí, porque se
  buscan por nombre con `hasResource`.

## 2. Coste de memoria (Xbox, 64 MB)

Cómo se sube una textura en Xbox (`platform/RenderAPI_D3D8_XBOX.cpp`):
* Siempre `D3DFMT_A8R8G8B8` (4 B/píxel), con lado redondeado a potencia de 2 (`:400`) y **sin mipmaps** (`:1743`).
* Después de subirla se libera la copia en CPU (`:485`), **salvo si la textura es dinámica**. `terrain.png` e
  `items.png` se animan (agua, lava, fuego, brújula…), así que **guardan una copia en CPU además de la de GPU**
  (`:1765`). En la práctica cada atlas cuesta el doble.
* Mientras se decodifica: stb_image (`java/BufferedImage.cpp:163`) tiene los bytes PNG + los datos inflados
  + el RGBA, y luego copia al `BufferedImage`. El pico ronda 2–3 veces w·h·4 durante un momento.
* `readTextureImageData` guarda los píxeles decodificados para siempre en Xbox (`RenderEngine.cpp:286`; el tope
  solo existe en PS2, `PlatformConfig.h:315`). Es poco (tablas de color e `items.png` para brújula y reloj),
  pero también crece con la resolución.

| Pack | terrain.png | 2 atlas en RAM (GPU+CPU) | pico al decodificar 1 atlas |
|---|---|---|---|
| 16x | 256² = 256 KB | **1 MB** | ~0,75 MB |
| 32x | 512² = 1 MB | **4 MB** | ~3 MB |
| 64x | 1024² = 4 MB | **16 MB** | ~12 MB |
| 128x | 2048² = 16 MB | 64 MB → imposible | ~48 MB |

Además están mobs, armaduras y demás, que se cargan al usarse. Con los packs probados a 32x eso sube de unos
2–6 MB a 6–12 MB (ver tabla de la sección 4). Conclusión: **32x es el máximo razonable y 16x es lo seguro.**
64x o más hay que reducirlo siempre en el PC.

**Zip frente a pak (en consola)**
* `.pak` hoy: al abrir guarda en RAM la tabla y los nombres (16 B por entrada + nombres, unos KB). Cada lectura
  copia la entrada a un `std::string` y de ahí a un `istringstream`. Coste extra: el tamaño del PNG comprimido
  (×2 mientras dura la copia).
* `.zip` con minizip: `unzOpen` no carga el directorio central. `unzLocateFile` **lo recorre leyendo del disco en
  cada búsqueda**, y un pack de 1.5 tiene unas 1.400 entradas (unos 100 KB de directorio): cientos de lecturas
  lentas del disco duro al cargar texturas. Con un índice hecho una sola vez al abrir se arregla.
  Inflar es barato: unos 56 KB de estado de zlib por fichero abierto, y el PNG ya va comprimido, así que zip apenas
  lo reduce. La memoria por fichero es la misma que con `.pak`: se carga el fichero entero, porque stb necesita el
  búfer completo y no tiene sentido ir leyendo a trozos.
* **Lo caro no es el zip, es su contenido.** Un zip de Java puede ser 64x/128x (ver la tabla: no cabe) o tener
  formato 1.5 (hay que montar el atlas en la consola). El `.pak` convertido en el PC ya viene reducido y montado.

## 3. Herramientas nuevas (`texturas\`)

| Fichero | Qué es |
|---|---|
| `convertir_pack.py` | Convertidor (CLI y API `convertir()`). Python 3 y nada más: usa Pillow si está, si no `png_puro.py` |
| `png_puro.py` | Lector/escritor PNG en Python puro (todos los tipos de color y profundidades, tRNS, Adam7) y reducción por cajas ponderada por alfa |
| `Convertir pack para Xbox.bat` | Arrastrar y soltar uno o varios `.zip`/carpetas. Comprueba Python (`py -3` o `python`) y, si falta, explica cómo instalarlo. Sin argumentos abre la ventana |
| `convertir_pack_gui.pyw` | Ventana Tkinter: pack → versión (1.2.5/1.5.2/las dos) → 16/32 → Convertir, con barra de progreso, resumen y botón para abrir la carpeta. Probada |
| `descargar_y_subir.py` + `catalogo.json` | Compañero de PC (sección 5) |
| `generar_datos.py` | Solo para mantenimiento: regenera `datos/` desde el repo (`Texture152Atlas.h`, `assets.pak` 1.5.2, `data/assets` de 1.2.5) |
| `datos/` | `atlas152.json` (nombre→celda, celdas nuevas de 1.5.2, cuadrantes del agua y la lava que fluyen), `tamanos_125.json`/`tamanos_152.json` (ficheros que trae cada versión y su tamaño), atlas por defecto de 1.2.5 y 1.5.2 |
| `LEEME.md` | Instrucciones para jugadores: arrastrar y soltar, ventana, CLI y FTP paso a paso |

Qué hace `convertir_pack.py`:
* **Entrada**: `.zip` o carpeta. Busca la raíz aunque el zip tenga una carpeta dentro. Detecta los formatos
  1.2–1.4 (`terrain.png`), 1.5 (`textures/blocks`) y 1.6+ (`assets/minecraft`: aprovecha bloques y objetos con
  nombres de 1.5/1.6, avisa del resto y convierte `pack.mcmeta` en `pack.txt`). Para `.rar`/`.7z` da un mensaje
  claro. Nunca extrae a disco ni ejecuta nada. Se salta rutas con `..` y ficheros de más de 64 MB, y para si el
  total pasa de 1 GB.
* **Atlas**: monta `terrain.png` y `gui/items.png` para la versión pedida:
  * Pack de 1.5 → celdas de `atlas152.json`. En 1.2.5 sin las 79 reasignadas; con primer fotograma de las tiras
    animadas, cuadrantes de `water_flow`/`lava_flow` y cuero teñido con su `_overlay`.
  * Pack antiguo en 1.5.2 → las celdas reasignadas con la textura por defecto.
  * Lo que falta → atlas por defecto ampliado al tamaño de celda.
* **Reducción**: `--max 16|32` (32 por defecto) y en múltiplos enteros. Usa los tamaños de referencia del juego.
  Las tablas de color se dejan siempre en 256×256 y `pack.png` en 128 como mucho. Recorta los mobs más altos de 1.5.
* **Filtro**: copia solo lo que el juego usa:
  * Lo de `tamanos_<versión>.json`.
  * Lo de OptiFine 1.2.5 (`custom_*`, `anim/`, `ctm*`, `color.properties`).
  * `pack.txt` (dos líneas, UTF-8) y `pack.png`.
  * `gui/` solo con `--con-gui`, igual que `make_legacy_texture_packs.py`, que también deja la gui del juego
    menos `items.png`.
  * Omite y cuenta por motivo: sonidos, mods, cielos, CTM y animaciones en formato 1.5.
* **Salida**: `<salida>\<versión>\texturepacks\<nombre>.pak`, con la misma estructura que `F:\Games\OptiCraft\`.
  El formato del `.pak` es idéntico al de `make_pak.py`: lo he comprobado con el mismo contenido (mismas entradas,
  hashes ordenados y alineación de 64). Con `--formato carpeta|ambos` también deja una carpeta (avisa de nombres
  de más de 42). El nombre se adapta a FATX.
* **Resumen**: formato, resolución, ficheros, tamaño, imágenes reducidas, omitidos y RAM estimada (atlas ×2,
  resto, pico; comparado con el juego sin pack).
* Rendimiento: con Pillow, menos de 1 s por pack. En Python puro, unos 11 s un pack de 64x.

## 4. Packs descargados (`texturas\descargas\<id>\`, convertidos en `texturas\convertidos\<id>\`)

Todos vienen de archive.org (descarga directa, sin cuenta). Los sha256 están en `catalogo.json`.
**Licencia**: ninguno tiene licencia libre. Son de sus autores y los espejos no son oficiales: solo uso personal,
no redistribuir el pack ni el `.pak` (no he podido comprobar los términos originales). `John_Smith.7z`
(colección de 35 MB con todas las versiones) viene marcado en archive.org como "Public Domain Mark", pero lo
puso quien lo subió, no el autor: no debe tomarse como licencia libre.

| id | Pack (versión MC) | URL | Zip | Formato → salida | .pak 1.2.5 / 1.5.2 | RAM atlas / resto (sin pack) |
|---|---|---|---|---|---|---|
| faithful32_125 | Faithful 32x de Vattic (b1.8–1.2.5) | archive.org/download/mc125tpacks/faithful_1.2.5_32x.zip | 1,2 MB | atlas 32x→32x | 1,0 / 1,0 MB | 4 MB / 5,9 (1,5) MB |
| sphax32 | Sphax PureBDCraft 32x v1.3.0 (MC 1.3) | archive.org/download/SphaxPureBDCraft32x.zip_opensource_media/… | 3,6 MB | atlas 32x→32x | 2,2 / 2,2 MB | 4 MB / 12,3 (6,0) MB |
| jolicraft16_12x | JoliCraft 16x (1.2.x) | archive.org/download/mc125tpacks/Jolicraft_1.2.x_16x.zip | 2,0 MB | atlas 16x | 0,9 / 0,9 MB | 1 MB / 5,6 (5,7) MB |
| jolicraft16_152 | JoliCraft 16x (1.5.2) | archive.org/download/mc15tpacks/JoliCraft_1.5.2_16x.zip | 3,0 MB | 1.5 → atlas 16x (206/243 bloques) | 0,8 / 0,9 MB | 1 MB / 5,4–5,7 MB |
| johnsmith_legacy32_152 | John Smith Legacy 32x (1.5.2) | archive.org/download/mc15tpacks/John_Smith_Legacy_1.5.2_32x.zip | 11 MB | 1.5 → atlas 32x (209/248 bloques) | 2,0 / 2,2 MB | 4 MB / 7,8–8,9 (4,6) MB |
| dokucraft_tsc_light_152 | Dokucraft TSC Light 32x (1.5.2) | archive.org/download/1.5.2-dokucraft-tsc-light/… | 8,5 MB | 1.5 → atlas 32x (todos los bloques) | 2,0 / 2,5 MB | 4 MB / 10,5–11,6 (5,7) MB |
| soartex64_125 | Soartex Fanver 10.9 64x (1.2.5) | archive.org/download/mc125tpacks/Soartex_Fanver10.9_1.2.5_64x.zip | 7,1 MB | atlas **64x→32x** | 1,2 / 1,3 MB | 4 MB / 7,0 (3,6) MB |
| johnsmith (extra) | John Smith 9.5 32x (1.2.5), sacado de `John_Smith.7z` | archive.org/download/john-smith-pack/John%20Smith.7z | 8 MB | atlas 32x | 3,1 / 3,1 MB | 4 MB / 10,4 (6,8) MB |

Los 8 se han convertido bien a las dos versiones. Resumen completo en `texturas\convertidos\resumen.txt`.
Revisé a ojo los atlas de JoliCraft 1.5.2, Faithful→1.5.2 y Dokucraft→1.5.2 (bloques, objetos y cuero teñido).
**No se han probado en consola ni en xemu.**
Notas:
* Dokucraft está hecho para MCPatcher. Se omiten 654 ficheros de CTM/animaciones en formato 1.5 y sus cielos
  `sky0` de 1536×1024: cada uno costaría 6 MB.
* John Smith 9.5 trae `terrain/sky0/` (también omitido) y la carpeta de mod `aether/`.
* Sphax tiene muchas animaciones `anim/`: por eso el "resto" llega a 12 MB.
* No encontré Misa ni Faithful 1.5 en archive.org con búsqueda directa. `mcb-173-texturepacks` tiene Misa221 para b1.7.3.
* No he usado CurseForge ni PlanetMinecraft: piden JS o descarga con redirecciones.

## 5. Bajar packs de internet: compañero en el PC

La Xbox no puede usar HTTPS moderno (TLS 1.2+ y certificados actuales) sin meter una pila TLS en el XBE, y
tampoco tiene sentido descargar y convertir 64x en 64 MB. Lo práctico es que el PC haga de intermediario.
Está hecho: `texturas\descargar_y_subir.py`.
* `--listar` → catálogo con nota de licencia.
* Por defecto descarga los packs de `catalogo.json` (lista curada con URL y sha256: si el fichero cambia no se usa),
  los convierte a `convertidos\<id>\` y **no sube nada**.
* `--subir` (desactivado por defecto) → FTP (`ftplib`) a `--host 192.168.0.3`, `xbox`/`xbox`, en
  `/F/Games/OptiCraft/<versión>/texturepacks/`:
  * solo a versiones cuya carpeta ya existe en la consola;
  * crea `texturepacks` si falta;
  * compara el tamaño con `SIZE` al terminar.
  `--simular` muestra lo que subiría sin conectar.
* Probado: listar, descarga real (JoliCraft, sha256 correcto) y `--subir --simular`. **La subida real no se ha
  probado**, por la orden de no subir nada.
* Posible mejora: un pequeño servidor HTTP **sin TLS** en el PC (o el FTP de la consola al revés) y un menú en el
  juego "Descargar packs" que lea un índice en texto plano. Lo dejaría para después: el FTP desde el PC ya cubre
  el caso sin tocar el XBE.

## 6. Cambios en C++ recomendados (no hechos)

Por orden de coste y beneficio:
1. **`xbox-unificada`: traer el soporte de packs de opti30/port152** (`TexturePackFolder`, `TexturePackPak` y la
   rama `#elif defined(XBOX_PLATFORM)` de `TexturePackList.cpp:118-162`). Sin esto, esa rama no ve ningún pack.
2. **`listResources` para `TexturePackPak` y `TexturePackFolder`.** `PakArchive` ya tiene `listChildren`
   (`PakArchive.h:62`) y la carpeta se puede listar con `File::listFiles`. Arregla las animaciones
   `/anim/*.properties` en consola. Unas 30 líneas.
3. **`.zip` directo y barato (`TexturePackZip`, con el minizip que ya se enlaza)**:
   * Al abrir, `unzOpen64` sobre el fichero (con un ioapi sobre `PlatformFile::readAt` como el de `PakArchive`,
     o `fopen`). Se recorre el directorio central **una vez** (`unzGoToFirstFile`/`unzGoToNextFile` +
     `unzGetFilePos64`) para hacer un `unordered_map<clave en minúsculas, unz64_file_pos>`, unos 60 B por entrada.
     Detecta la carpeta raíz y el prefijo `assets/minecraft/`.
   * `getResourceAsStream`: `unzGoToFilePos64` + `unzOpenCurrentFile` + leer `uncompressed_size` en un
     `std::string` + `istringstream(std::move(...))`. El pico es el tamaño del fichero más unos 56 KB, como con `.pak`.
   * `listResources` desde el mapa.
   * **Antes de decodificar**, leer los 24 bytes de cabecera de `terrain.png`/`items.png`:
     * si pasan de 512 px (más de 32x), no ofrecer el pack (o enseñarlo deshabilitado con "conviértelo en el PC");
     * si son 1024 (64x), se podrían reducir a la mitad justo después de decodificar, con un pico de unos 12 MB.
       Demasiado arriesgado en partida.
   * Añadir `.zip` al filtro de `TexturePackList.cpp:132-133`.
   * Coste: unas 200 líneas, sin dependencias nuevas. Vale para packs de 1.2–1.4 de ≤32x.
4. **Packs de 1.5 en zip (opcional y más caro)**: si el zip no tiene `terrain.png` pero sí `textures/blocks/`,
   montar el atlas al seleccionar el pack con `Texture152::kBlockIcons`/`kItemIcons`, que ya están compilados en
   1.5.2:
   * Hay que decodificar unos 450 PNG pequeños con stb y pegarlos sobre el atlas por defecto.
   * Devolver el atlas sin pasar otra vez por PNG requiere un camino nuevo (`BufferedImage` directo), porque hoy
     todo pasa por `istream`.
   * Tiempo estimado en consola: 1–2 s al cambiar de pack. RAM: 1 MB (16x) o 4 MB (32x) mientras se monta.
   * Para 1.2.5 haría falta además la tabla filtrada (la de `datos/atlas152.json`, `nuevos152`).
   * Mientras no exista, el convertidor del PC es la forma de usar packs de 1.5.
5. Menor: con muchos `.pak`, cada uno deja un `PakArchive` abierto (descriptor + tabla) en `soundPool` desde que
   aparece en la lista. Se podría abrir solo al seleccionarlo o al pedir pack.png. No es urgente.

## 7. Siguiente paso sugerido
Subir 2 packs por FTP (por ejemplo `jolicraft16_12x` 16x y `faithful32_125` 32x) a una build de opti30 o port152,
mirar en el log UDP la RAM libre con 16x y con 32x, y decidir si en el `.bat` el valor por defecto se queda en 32
o baja a 16.
