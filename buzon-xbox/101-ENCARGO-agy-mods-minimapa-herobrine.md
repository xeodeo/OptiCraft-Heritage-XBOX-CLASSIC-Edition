# 101. Encargo agy-2: mods — minimapa (Rei + Xaero), Herobrine clásico y ChocoCraft (1.2.5 y 1.5.2) (22-11)

Pedido por el usuario: más soporte de mods, los mejores minimapas (Rei's Minimap, Xaero's Minimap) y el mod clásico
de **Herobrine** de la beta (el del altar). **Eres el SEGUNDO agy**: el otro trabaja a la vez en `wt-152-rend` y `wt-opti30` — NO toques esas carpetas.
Tus ramas/carpetas propias: 1.5.2 `port152-mods` (`wt-152-mods`, base `a2ec7d1`) y 1.2.5 `opti30-mods`
(`wt-125-mods`, base `f1bf66a`). Claude las juntará luego. Comparte el candado de compilación/xemu con el otro agy:
`buzon-xbox\BUILD.lock` (espera si está cogido). Sin reset, sin push, sin copiar al lanzador; commits con `git add <fichero>`; candado
`buzon-xbox\BUILD.lock`. Reglas: 60 fps (medir con el mod activo y sin él), 64 MB (medir `freeKB`), creativo NO, terreno
igual, nunca /fp:fast, dos interfaces (Java y Legacy) para toda pantalla nueva, pantalla partida, mando completo
(nada que exija teclado/ratón).

## Lo que ya hay (léelo antes)
`src/mods/`: `ModManager` (activa mods internos cuando hay un `.och` con su id en la carpeta de mods; ver
`OchPackReader`, `GuiMods`, `GuiLoadModsMenu`, `GuiConfirmModInstall`), `reiminimap/` (ReiMinimap, waypoints,
GuiWaypointManager), `toomanyitems/`, `strongholdlocator/`. En la consola la carpeta es
`E:\TDATA\FFFF4F43\.minecraft\mods` (hoy NO hay `mods.txt` ni packs allí).

## 1. Que los mods se puedan usar en la Xbox de verdad
- Que el jugador pueda activar/desactivar los mods internos DESDE EL MENÚ con el mando (sin tener que copiar `.och`
  por FTP). Si hace falta un `.och`, que el juego lo cree solo o que vengan en el `assets.pak`/carpeta del juego.
- Comprueba TooManyItems y Stronghold Locator en Xbox (mando, 480/720p) y anota qué funciona.

### 1b. Buscador / catálogo de mods (pedido del usuario 22-11)
Pantalla «Mods» tipo tienda, con mando, en las dos interfaces: lista de TODOS los mods internos (TooManyItems,
Stronghold Locator, minimapa, Herobrine, ChocoCraft y los que vengan), con buscador por nombre, descripción corta,
captura/icono, versión, y un botón **Instalar/Activar** de un clic (y Desactivar). Nada de copiar ficheros a mano.
Los mods son código C++ dentro del XBE: NO se pueden bajar mods nuevos de internet (los de Java no corren aquí); si
quieres, deja preparado que el catálogo lea una lista extra (para el futuro) pero sin red obligatoria.

### 1c. Traer los mods interesantes de erluza (`erluza/main` en el repo)
`git show erluza/main:src/mods/...`: **GuiCompiledMods** (menú para añadir/quitar mods compilados: úsalo como base
del catálogo 1b), **xray** (`XRayMod`, + `XRayMode.cpp/.h` y sus ganchos en `RenderGlobal`/`WorldRenderer`; en Xbox
sin el camino PS2 greedy), **villagelocator** (`VillageLocatorMod`), **serverutils** (botones de info/jugadores en la
lista de servidores) y las **luces dinámicas** (`src/net/minecraft/src/DynamicLights.cpp/.h` + ganchos en `World.cpp`:
antorcha en la mano ilumina) como mod/opción apagada por defecto — mide su coste en fps, que en Xbox puede doler.
No traigas `autologin`, `oomtest` ni `ps2playlist`. Adáptalos a mando, dos interfaces, 480/720p.

## 2. Minimapa (Rei's Minimap + lo mejor de Xaero's)
- Rei's Minimap ya está: que funcione en Xbox con mando (abrir menú del minimapa, crear/editar waypoints con el
  teclado en pantalla, zoom), en las dos interfaces, 480/720p, zona segura del HUD, pantalla partida (uno por jugador).
- Añade lo que hace famoso a Xaero's Minimap (sin copiar su código, que es 1.8+): forma redonda/cuadrada, radar de
  entidades (mobs hostiles rojo, animales verde, jugadores), modo cueva, flecha de orientación, coordenadas y bioma
  debajo, puntos de muerte automáticos. Como opciones del mismo minimapa.
- **Rendimiento**: el mapa NO se redibuja entero cada frame: textura actualizada por trozos cada N ticks. Medir fps
  con el minimapa activo (meta: <2 fps de pérdida) y la memoria de su textura.

## 3. Herobrine clásico (el de la beta, con altar)
Como el mod original "Herobrine Mod" de la era beta/1.2:
- **Altar**: base 3×3 de piedra musgosa, bloque de oro en el centro, netherrack encima del oro y 4 antorchas de
  redstone en los lados del oro; al encender el netherrack cae un rayo y Herobrine queda «invocado» (mensaje en el chat
  como el mod: "You have summoned Herobrine" o lo que tuviera).
- Después: Herobrine (jugador con los ojos blancos, skin clásica de Steve con ojos brillantes) aparece a lo lejos
  mirándote y desaparece cuando te acercas o lo miras un rato; a veces detrás de ti; sonidos ambiente raros; cosas del
  mito con moderación (túneles 2×2 en piedra, pirámides de arena, árboles sin hojas, carteles) — sin romper el mundo
  del jugador ni sus construcciones, y que se pueda desactivar desde el menú de mods.
- Todo como mod interno (`src/mods/herobrine/`), apagado por defecto, en las dos versiones.
- Si no tienes claro qué hacía el mod original, documenta lo que implementas en el informe; nada de inventar textos
  como si fueran oficiales.

## 4. ChocoCraft (el mod de los chocobos)
Como el mod clásico «ChocoCraft» (Final Fantasy) de la era 1.2.5/1.5.2: chocobos salvajes en llanuras/bosques,
gysahl greens (planta que se cultiva y se usa para domesticarlos y criarlos), montar con silla, colores (amarillo,
verde que sube montañas, azul que nada, blanco, negro, dorado que vuela) por cría, alforjas para inventario, ponerles
nombre. Textura y modelo propios (si no hay recursos libres, haz modelo sencillo con caja estilo Minecraft y
textura propia; NO copies recursos con licencia sin decirlo). Mod interno `src/mods/chococraft/`, apagado por
defecto, en las dos versiones, con mando (montar/desmontar, saltar, volar con el dorado). Si algo del original no
se sabe con certeza, documenta lo que implementas.

## Aceptación
Capturas de: menú de mods con mando, minimapa (redondo, radar, cueva, waypoints) en 480 y 720p y pantalla partida,
altar encendido + Herobrine apareciendo, chocobos (salvaje, montado, colores). fps y `freeKB` con/sin cada mod. Guardar/cargar (waypoints, estado de
Herobrine). En `buzon-xbox\101-capturas\`. Informe `buzon-xbox\101-INFORME-agy.md`; la señal de fin es la palabra
CIENTOUNO-LISTO seguida de los commits.
