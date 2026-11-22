# 109. Encargo agy: descargar packs de texturas desde la consola (1.2.5 y 1.5.2) (22-11)

Pedido por el usuario: elegir y descargar packs de texturas desde la propia consola. MEDIDO por Claude (22-11):
las webs de packs (archive.org, CurseForge, Planet Minecraft, resourcepack.net) solo sirven por HTTPS y la Xbox no
tiene HTTPS. Solución: **el PC hace de servidor de packs en la red de casa por HTTP plano**; la consola lista y descarga.

Ramas: las que diga Claude al darte el encargo (1.5.2 buena = `port152-int`; 1.2.5 = `opti30-erluza`), en una carpeta
propia. Sin reset, sin push, sin copiar al lanzador; commits con `git add <fichero>`; candado `buzon-xbox\BUILD.lock`.
Reglas: 60 fps, 64 MB, dos interfaces (Java y Legacy), mando, pantalla partida.

## 1. Servidor en el PC (`texturas\servidor_packs.py`, Python 3 sin dependencias)
- Sirve por HTTP en el puerto 8099 la carpeta `texturas\convertidos\<versión>\texturepacks\` (los `.pak` ya
  convertidos con `convertir_pack.py`) y un catálogo `catalogo.json` por versión: nombre, fichero, tamaño, resolución,
  sha256 y una miniatura PNG pequeña (64×64, sacada de `pack.png`).
- Anuncia su IP en la red local para que la consola lo encuentre sola (respuesta a un UDP broadcast, p. ej. puerto
  8098, «OPTICRAFT-PACKS?» → «OPTICRAFT-PACKS <ip>:8099»); si no, la consola deja escribir la IP a mano.
- `Abrir servidor de packs.bat` para lanzarlo con doble clic; LEEME en `texturas\LEEME.md`.

## 2. Pantalla «Descargar packs» en la consola
- Desde Mods → Texture Packs: busca el servidor (broadcast), pide el catálogo de su versión, lista con miniatura,
  tamaño y resolución; botón Descargar con barra de progreso; guarda en la carpeta `texturepacks` que ya lee el juego
  (comprobar sha256), y aparece en la lista sin reiniciar.
- Descarga en hilo, **escribiendo a disco por trozos** (nada de cargar el pack entero en RAM: el `httpGetUrl` actual
  tiene tope de 64 KB; añade una variante que vuelque a fichero), cancelar sin cuelgue, «Servidor no encontrado» y
  «Sin conexión» claros. Reutiliza la red de las skins (`JavaNetworkXbox.cpp`, `XboxNetwork`).
- Rechazar packs > 32x igual que hoy.
- Anotar en `buzon-xbox\PLATAFORMAS-QUE-HACER.md` qué haría falta en PS2.

## Aceptación
En xemu con el servidor en el PC: encontrarlo solo, listar, descargar 2 packs (16x y 32x), elegirlos y verlos;
cancelar a mitad; servidor apagado → mensaje. 60 fps al volver; `freeKB` antes/después. Capturas en
`buzon-xbox\109-capturas\`. Informe `buzon-xbox\109-INFORME-agy.md`; la señal de fin es la palabra
CIENTONUEVE-LISTO seguida de los commits.

## Más adelante (no en este encargo)
HTTPS en la Xbox con mbedTLS (ramide1 lo usa en PS2) para bajar de internet sin el PC.
