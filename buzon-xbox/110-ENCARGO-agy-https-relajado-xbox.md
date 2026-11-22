# 110. Encargo agy: HTTPS en OptiCraft reutilizando el de la Classic Xbox Store (22-11)

Pedido por el usuario: poder bajar de internet desde la consola (packs, skins…) aunque las webs solo sirvan HTTPS.
Usa el mismo modo que la tienda (verifica certificados). Solo si sus certificados raíz pesan demasiado en OptiCraft,
se admite el modo «relajado» (cifrado sin verificar), comentado en el código y solo para packs y skins.

Ramas: las que diga Claude al darte el encargo (1.5.2 `port152-int`, 1.2.5 `opti30-erluza`), carpeta propia. Sin reset,
sin push; commits con `git add <fichero>`; candado `buzon-xbox\BUILD.lock`. Nunca /fp:fast; memoria 64 MB.

## YA LO TENEMOS HECHO: la Classic Xbox Store (nuestra)
`C:\Users\xeodeo\Downloads\Desktop\de kevin\Classic Xbox Store\` (repo github.com/xeodeo/Classic-Xbox-Store,
release v1.0 en `Desktop\release\Classic Xbox Store`): XBE nativo que descarga juegos de archive.org por HTTPS.
- `src/net/Http.cpp` / `Http.h` (cliente HTTP/HTTPS, descarga a disco por trozos, redirecciones),
  `src/net/mbedtls_config.h`, `external/mbedtls/` (CMake: `file(GLOB MBEDTLS_SOURCES ... library/*.c)` y la
  definición `MBEDTLS_CONFIG_FILE`). Verifica certificados (`MBEDTLS_SSL_VERIFY_REQUIRED`) con SNI.

## Qué hacer
1. Traer ese cliente y mbedTLS a OptiCraft (mismas cabeceras y config; copia igual de dónde saca los certificados
   raíz y la entropía). Que compile con nuestro toolchain sin subir los 13 «SSE2 left in».
2. `JavaNetwork::httpGetUrl` acepta `https://` (y redirecciones http→https) usando ese cliente; variante a fichero por
   trozos (como la tienda) para descargas grandes. Mantén el DNS de respaldo (8.8.8.8/1.1.1.1) de `XboxNetwork`.
3. Probar en xemu: `https://minotar.net/skin/Notch`, un pack pequeño de `https://archive.org/download/...`,
   `https://api.mojang.com/users/profiles/minecraft/Notch`; tiempo de handshake, RAM durante la conexión, tamaño
   añadido al XBE. Sin cuelgues sin red.
4. Anotar en `PLATAFORMAS-QUE-HACER.md` qué haría falta en PS2/PS3.

## Después (otro encargo)
Usarlo en «Descargar packs» (109) con un catálogo en internet además del servidor del PC.

## Informe
`buzon-xbox\110-INFORME-agy.md`; la señal de fin es la palabra CIENTODIEZ-LISTO seguida de los commits.
