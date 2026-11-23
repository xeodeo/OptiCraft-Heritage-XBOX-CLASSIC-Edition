# 111. Encargo agy: menú de texturas que desapareció, arreglos de interfaz y descargas de Modrinth (1.2.5 y 1.5.2) (22-11)

Visto por el usuario en la consola real (22-11 18:00). Ramas: 1.5.2 `port152-int` (último `2ff4078`), 1.2.5
`opti30-erluza` (último `a0addaf`); Claude te dirá la carpeta. Sin reset, sin push, sin copiar al lanzador; commits con
`git add <fichero>`; candado `buzon-xbox\BUILD.lock`. Reglas: 60 fps, 64 MB (medir `freeKB` en el menú y en el
juego), nunca /fp:fast, dos interfaces (Java y Legacy), **480p y 720p** (capturas de las dos), mando completo.

## REVISIÓN DE CLAUDE (22-11 18:15) — ANTES DE EMPEZAR
- Punto 1 HECHO por Claude (`e4af03f` 1.2.5 / `ee55f32` 1.5.2): botón «Texturas» en el catálogo (Java) y Start (Legacy).
- Punto 2: fondo que se mueve y panel centrado HECHOS por Claude; el cambio de estilo de TODAS las pantallas nuevas
  al aspecto LCE de Xbox (el usuario dice que parecen de Bedrock) lo hace un subagente de Claude (informe 112).
  NO toques esas pantallas.
- Punto 4 HECHO a medias: `XboxTls::shutdown()` existe y se llama al salir de buscar skin; falta llamarlo al salir
  de «Descargar packs» (tu pantalla) y la idea de la sección XBE a demanda.
- **Punto 3 (Modrinth) lo hace ahora un subagente de Claude (informe 116). Tu parte: SOLO el punto 5 (tirón al guardar).**, ramas `port152-int` (`ee55f32`) y `opti30-erluza`
  (`e4af03f`).

## 1. Ya no hay dónde cambiar las texturas (URGENTE)
Desde que entró el catálogo de mods (encargo 101, `GuiCompiledMods`/`GuiMods`), la entrada «Texture Packs» que había
en Mods ya no está accesible. Volver a ponerla (en las dos interfaces) y comprobar que se elige un pack y se aplica.

## 2. Interfaz del menú de mods (y todo lo nuevo) en 480p y 720p
- En el catálogo de mods el **fondo animado (panorama) se queda quieto** y el contenido queda **demasiado arriba**.
  Que el fondo se mueva como en el resto de menús y que el contenido quede centrado y dentro de la zona segura.
- Revisar en 480p y 720p (Java y Legacy) TODAS las pantallas nuevas: catálogo de mods, minimapa (menú, gestor de
  waypoints), Herobrine, chocobo, X-Ray/servidores, buscar skin, rueda de skins (grupo personalizadas), packs de
  texturas, superplano/marcador si ya están. Capturas de cada una en las dos resoluciones.

## 3. Descargar packs de texturas de Modrinth desde la consola
Ya hay HTTPS en la Xbox (`src/xbox/system/XboxTls.*`, mbedTLS, modo relajado; `JavaNetwork::httpGetUrl` acepta
https). MEDIDO por Claude: la API de Modrinth da ~244 packs para 1.2.5 y ~266 para 1.5.2:
`https://api.modrinth.com/v2/search?facets=[["project_type:resourcepack"],["versions:1.5.2"]]&query=...&offset=...`
(cabecera `User-Agent` propia), versiones del proyecto: `/v2/project/{id}/version?game_versions=["1.5.2"]`, fichero en
`cdn.modrinth.com` (zip).
- Pantalla «Descargar packs» (desde Texture Packs): buscador (teclado en pantalla), lista paginada (título, autor,
  descargas, resolución si se sabe), icono solo si es PNG (los WebP se omiten), ficha con descripción corta y
  Descargar. Descarga **a disco por trozos** (nunca el zip entero en RAM; variante de `httpGetUrl` a fichero con barra
  de progreso y cancelar), guarda en la carpeta `texturepacks` y aparece en la lista sin reiniciar.
- **Formato 1.5 (un PNG por bloque)**: muchos packs de 1.5.2 vienen así. Hoy el lector `.zip` los marca como «no se
  puede usar». Añadir el montaje del atlas EN LA CONSOLA tras descargar (una vez, guardando el resultado como `.pak`
  o carpeta convertida, igual que hace `texturas\convertir_pack.py` en el PC) con tope de memoria (≤ 32x; procesar
  una textura cada vez). Si un pack es 64x+ o raro: «No compatible con la consola».
- **Mods**: los de Modrinth son Java y no corren en la Xbox; el catálogo de mods sigue siendo solo de los nuestros.

## 4. Que el HTTPS no gaste memoria dentro del juego
- Añadir `XboxTls::shutdown()` que libere el estado global (config, generador aleatorio, entropía) y llamarlo al salir
  de las pantallas de descarga (skins y packs); `open()` lo vuelve a preparar.
- Medir cuánto añade mbedTLS al XBE y valorar ponerlo en una **sección del XBE que se cargue solo al abrir esas
  pantallas** (`XLoadSection`/`XFreeSection`, `#pragma code_seg`); hazlo si es seguro, si no explica por qué.

## 5. Tirón de 1,5 s al guardar un chunk al descargarlo (consola real 22-11 18:01)
`frame=1544ms ... worldTick=1532.9 chunkUnload=1532.2 ... save=1531.8ms` (`buzon-xbox\111-netlog-tiron-guardado.txt`).
Medir qué parte del guardado es la lenta (serializar NBT, comprimir, escribir/crecer el `.mca`, flush de FATX) con una
traza por fase, y quitar el tirón: guardado de descargas fuera del hilo principal o repartido (un chunk por tick,
cola), sin perder datos (respetar lo del encargo 91: no descargar sin guardar con poca memoria). Medir en xemu con
muchos chunks modificados y entidades.

## Aceptación
Capturas 480p y 720p de cada pantalla (Java y Legacy); elegir un pack desde Texture Packs; descargar de Modrinth 2
packs (uno formato 1.2.5 y uno formato 1.5) y verlos en el juego; cancelar a mitad; sin red → mensaje. `freeKB` en el
juego igual que antes de entrar al menú de descargas. Capturas en `buzon-xbox\111-capturas\`. Informe
`buzon-xbox\111-INFORME-agy.md`; la señal de fin es la palabra CIENTOONCE-LISTO seguida de los commits.
