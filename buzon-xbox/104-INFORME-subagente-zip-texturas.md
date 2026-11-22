# 104 — Informe (subagente): packs .zip en la consola y animaciones de packs

Fecha: 22-11-2026. Sigue al 102 (cambios 2 y 3 de su sección 6). Solo se ha tocado `wt-152-tex`
(rama `port152-tex`) y `wt-125-tex` (rama `opti30-tex`). El código es el mismo en las dos: los ficheros
`TexturePack*` eran idénticos y siguen siéndolo. No se ha abierto xemu ni se ha subido nada.

## Qué cambia

### 2. Animaciones `anim/*.properties` de packs `.pak` y de carpeta
* `TexturePackPak::listResources`: recorre la tabla del `.pak` (ya está en RAM) y devuelve las claves que
  empiezan por el prefijo y acaban en el sufijo, sin distinguir mayúsculas.
* `TexturePackFolder::listResources`: lista `texturepacks\<pack>\anim\` con `File::listFiles` (y subcarpetas,
  con pila explícita, no recursión).
* Con esto `RenderEngine.cpp:1300` (`listResources("/anim/", ".properties")`) ya encuentra las animaciones
  genéricas de OptiFine de un pack en consola. Solo se ejecuta al cargar texturas, nunca por fotograma.

### 3. `.zip` de Java directamente en la consola (`TexturePackZip`, nuevo)
* `src/net/minecraft/src/TexturePackZip.h/.cpp` (todo el `.cpp` va dentro de `#if defined(XBOX_PLATFORM)`:
  solo la Xbox enlaza minizip).
* Al listar el pack: `unzOpen2` con las funciones de stdio de 32 bits (`fill_fopen_filefunc`, el mismo `FILE`
  que usa `PakFile`; así no dependemos de `_fseeki64`). Se recorre el directorio central **una sola vez**
  (`unzGoToFirstFile/NextFile` + `unzGetFilePos`) y se guarda un vector ordenado `clave en minúsculas →
  posición + tamaño` (unos 50–60 B por fichero). Después se cierra el fichero.
* Carpeta dentro del zip: la raíz del pack es la carpeta del `terrain.png` o `pack.txt` menos profundo; se
  quita ese prefijo y se descarta lo que quede fuera (`__MACOSX/`, otros packs, carpetas de mods).
  También se ignoran directorios y `.DS_Store`. Máximo 8192 ficheros.
* Leer un fichero: búsqueda binaria en el índice → `unzGoToFilePos` + `unzOpenCurrentFile` + leer exactamente
  `uncompressed_size` en un `std::string` → `istringstream` (igual que `TexturePackPak`). Se comprueba el
  CRC al cerrar. Ficheros de más de 4 MB se saltan con aviso (se usa la textura del juego).
* Asas: mientras el pack es el seleccionado (`loadTexturePack`) el zip se queda abierto para la ráfaga de
  cargas; `closeTexturePackFile` (cambiar de pack, destructor) lo cierra. Los packs no seleccionados no
  tienen nada abierto: si hace falta leer (pack.txt/pack.png al listar) se abre y se cierra en la misma
  llamada. Todo lo de minizip va bajo un `std::mutex`.
* `listResources` sale del índice (así también funcionan las `anim/` de un zip).
* Comprobaciones al listar, **antes de decodificar nada** (se leen solo los 24 bytes de la cabecera PNG):
  * `terrain.png` o `gui/items.png` de más de 512 px (más de 32x) → `MC_LOG_WARN("texturepack", ...)` con
    el tamaño, y el pack sale en la lista con las líneas grises
    "Pack de 64x: no cabe en memoria" / "Conviértelo con el conversor PC".
  * Sin `terrain.png` pero con `textures/blocks/` (1.5, o 1.6 en `assets/minecraft/...`) →
    `MC_LOG_WARN` y en la lista "Formato 1.5: no se puede usar" / "Conviértelo con el conversor PC".
  * En los dos casos el pack **no se puede elegir** y del índice solo se conservan `pack.txt` y `pack.png`.
  * `pack.png` de más de 256 px no se decodifica (la miniatura se queda en RAM mientras dure la lista).
  * Un zip ilegible o vacío no se lista (como un `.pak` roto), con aviso en el log.
* `TexturePackBase::isUsable()` (nuevo, virtual, `true` por defecto). `TexturePackList::setTexturePack`
  rechaza un pack no usable, y al arrancar no se reselecciona aunque su nombre esté en `options.txt`
  (se queda el pack por defecto).
* `TexturePackList.cpp` (rama Xbox): acepta `*.zip` en `D:\texturepacks\` junto a carpetas y `.pak`,
  con clave `zip:<nombre>` en `soundPool`. El nombre que se muestra es el del fichero sin `.zip`.

Las pantallas (Java y Legacy) no se han tocado: el aviso va en las dos líneas de descripción que ya pinta
`GuiTexturePackSlot`. (Pulsar A sobre un pack no usable no lo cambia; la pantalla sigue haciendo su
`refreshResources()` de siempre, igual que al pulsar sobre el pack ya elegido.)

### 4. Montar packs de 1.5 en la consola: **no hecho**
Motivos: hace falta un camino nuevo para entregar un atlas ya decodificado sin pasar por PNG (hoy todo va por
`istream`), decodificar ~450 PNG pequeños sobre el atlas por defecto ampliado, las tiras animadas, los
cuadrantes de `water_flow`/`lava_flow`, el cuero con `_overlay`, y para 1.2.5 una tabla filtrada que no está
compilada. Pico de 4 MB más el atlas por defecto a 32x, 1–2 s de carga y mucho código sin poder probarlo en
consola. No es "barato y seguro"; el conversor del PC ya lo hace bien. Esos zips salen en la lista como
"Formato 1.5".

## Memoria
* Por zip listado: el índice (≈ 60 B × ficheros; un pack 1.2.5 típico de 200–500 ficheros, 12–30 KB) más la
  miniatura (como `.pak`/carpeta). Los zips rechazados se quedan con 2 entradas.
* Al cargar una textura: el PNG descomprimido (igual que con `.pak`) + ~45 KB de estado de inflate solo
  mientras se lee. El zip nunca está entero en RAM.
* Con el pack elegido: un `FILE` abierto. Los atlas, igual que con un `.pak` del mismo tamaño (32x = 4 MB).
* Nada se ejecuta por fotograma: todo es al listar packs o al (re)cargar texturas.

## Ficheros
`src/net/minecraft/src/`: `TexturePackBase.h`, `TexturePackFolder.h/.cpp`, `TexturePackPak.h/.cpp`,
`TexturePackList.cpp`, `TexturePackZip.h/.cpp` (nuevos). Iguales en `wt-152-tex` y `wt-125-tex`.

## Sin probar
* Nada se ha probado en consola ni en xemu: solo compila (xbox-release y xbox-release-exp en las dos ramas).
* No he hecho prueba en PC del índice (habría que montar stubs de medio motor). Revisado a mano.
* No comprobado: zips con nombres no ASCII (se guardan tal cual en minúsculas ASCII), zips ZIP64 o cifrados
  (no se abrirán: el fichero no carga y se usa la textura del juego), y que el FATX acepte el nombre largo del
  `.zip` (máximo 42 caracteres: renómbralo si hace falta).

## Cómo probarlo en la consola
1. Copiar por FTP (consola en el dashboard, `192.168.0.3`, `xbox`/`xbox`) a
   `/F/Games/OptiCraft/<1.2.5|1.5.2>/texturepacks/`:
   * un zip de 1.2.5 de 16x o 32x tal cual (p. ej. `texturas\descargas\jolicraft16_12x\...zip` o
     `faithful32_125`), con nombre corto (≤ 42 caracteres);
   * para ver los avisos: un zip de 64x (`soartex64_125`) y uno de 1.5 (`jolicraft16_152`);
   * para el cambio 2: un `.pak` o una carpeta con `anim/*.properties` (p. ej. el `.pak` de Sphax 32x de
     `texturas\convertidos\sphax32\`, que trae muchas `anim/`).
2. Menú principal → Mods → Texture Packs. Deben salir los 4; el de 64x y el de 1.5 con las dos líneas grises
   y sin poder elegirse.
3. En el log UDP: líneas `texturepack` ("N files indexed", los avisos de 64x / 1.5) y RAM libre antes y
   después de elegir el zip (debe ser parecida a la del `.pak` equivalente).
4. Elegir el zip, entrar en un mundo, mirar que agua/lava/fuego se animan y que no hay tirones nuevos.
