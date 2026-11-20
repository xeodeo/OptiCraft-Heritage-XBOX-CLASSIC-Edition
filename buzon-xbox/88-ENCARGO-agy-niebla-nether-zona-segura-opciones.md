# 88. Encargo agy: niebla del Nether, zona segura del HUD y mismas opciones en 1.2.5 y 1.5.2 (19-11)

Sigue sobre tus ramas del 87: **1.5.2** `port152-erluza` en `wt-152-rend` y **1.2.5** `opti30-erluza` en `wt-opti30`.
Claude ya añadió un commit en cada una (`a25a953` / `9787bb1`: la ocultación de entidades solo actúa 6+ bloques por
debajo de la cámara). NO hagas reset. Sin push. No toques otras carpetas.

## Lo que dijo el usuario probando el 87 en la consola real (19-11)
- Bien: reloj de redstone; en general «va bien».
- «nether fog muy cerca de la cara, ojo con eso» (ya lo había dicho antes).
- «en las 720 se está saliendo un poco el HUD hacia abajo, las teclas [iconos de botones]; en las opciones como
  calibrar, establecer zona segura de la pantalla para que se ajuste bien».
- «la 1.2.5 tiene opciones diferentes a las de la 1.5.2, y en la 1.5.2 no está eso del Pocket [está en Gráficos →
  Avanzado → Rendimiento, no lo encontró] → combina en ambas: la 1.2.5 debe tener todo lo de OptiFine igual que la
  1.5.2, y la 1.5.2 lo del Pocket etc.»

## 1. Niebla del Nether (las dos versiones)
- Java (1.2.5 y 1.5.2, `EntityRenderer.setupFog`, ver `mcp-diff/src125` y `src152`): en el Nether
  (`doesXZShowFog`) `fogStart = farPlaneDistance * 0.05`, `fogEnd = min(farPlaneDistance, 192) * 0.5`. Con la
  distancia de visión corta de la Xbox eso deja la niebla pegada a la cara. Además en Xbox la niebla va por nuestro
  camino propio (proyección «W-friendly», vista/proyección en mano izquierda; ver `ESTADO-ACTUAL.md` «Niebla en Xbox»).
- Mide qué valores salen en la consola (distancia de visión por defecto y máxima) y compáralos con Java a distancia
  «Normal»/«Lejos» en PC. Propón y aplica una niebla del Nether que se parezca a la de Java en Lejos (que se vea el
  Nether, con niebla roja densa a media distancia, sin muro a 1-2 bloques). Respeta la opción de niebla de OptiFine
  (Fast/Fancy/Off y «Fog start» si existe en la rama). Solo Xbox (`#if PLATFORM_XBOX`).
- Fotos de xemu (misma escena, antes/después) en `buzon-xbox\88-capturas\`. Si PrintWindow sale negro, usa el
  volcado de la propia consola/emulador que encuentres o describe con números (fogStart/fogEnd en el log).

## 2. Zona segura del HUD (las dos versiones, las dos interfaces)
- Opción nueva «Zona segura» / «Safe area» (porcentaje del borde, p. ej. 0-10 %, pasos de 1 %) en Opciones →
  Vídeo de Java **y** en el menú Legacy (`legacyUI`). Guardar en options.txt.
- Mueve hacia dentro TODO lo que va pegado al borde: barra de objetos, corazones/comida/armadura/experiencia, iconos
  de botones del mando (la fila Legacy de abajo), chat, textos de las esquinas, el selector de la pantalla partida y
  el HUD del jugador 2. No escales el mundo 3D, solo el HUD/2D.
- Valor por defecto: el que deje el HUD entero dentro de la tele del usuario en 720p (él dice que se sale «un poco»
  por abajo); en 480p el que ya se veía bien (no lo empeores). Mira `GuiIngame.cpp` ~260 (overscan de esquinas) y el
  margen de Wii ~348: reutiliza esa idea en vez de duplicarla.
- Al mover el deslizador, que se vea el efecto (vista previa o aplicarlo al momento).

## 3. Mismas opciones en 1.2.5 y 1.5.2
- 1.5.2 tiene en Legacy «Gráficos → Avanzado» (`LegacyAdvancedVideoOptions`, secciones y páginas). La 1.2.5 NO lo
  tiene (allí metiste Hielo opaco y Pocket en `LegacyVideoOptions` con paginación). Porta `LegacyAdvancedVideoOptions`
  a la 1.2.5 con las mismas secciones y deja Hielo opaco y Pocket dentro, igual que en 1.5.2.
- Haz una tabla de TODAS las opciones (Java: GuiVideoSettings, GuiDetail/Quality/Animation/Performance/Other/World
  SettingsOF, GuiOptiCraftOptions; Legacy: todas las `Legacy*Options`) de cada rama y deja las dos iguales, salvo lo
  que una versión no puede tener por ser de otra versión de Java (dilo en la tabla). Lo que falte en una, se añade.
- Pocket y Hielo opaco que se encuentren fácil: además de Avanzado → Rendimiento, ponlos en la primera página de
  Gráficos/Vídeo si cabe.
- Explica en el informe qué hace cada opción nueva en una frase.
- **El creativo NO se toca.**

## Reglas
- 60 fps de la 1.5.2 intactos; mide fps y `KB libres` antes/después en xemu. Sin `nan` ni `[X87]`, sin avisos nuevos
  «SSE2 left in». Nada que cambie el terreno.
- 480p y 720p: fotos o al menos comprobación del layout en las dos.
- `BUILD.lock`. 1.5.2: `cmake --build build/xbox-release` y `build/xbox-release-exp` en `wt-152-rend`.
  1.2.5: `powershell -File buzon-xbox\build_opti30.ps1`. NO copies al lanzador ni a la consola (lo hace Claude).
- Un commit por punto y por rama.

## Informe
`buzon-xbox\88-INFORME-agy.md`: qué cambiaste (archivo:línea), la tabla de opciones, valores de niebla antes/después,
qué tiene que probar el usuario. Señal en tu terminal: `ENCARGO 88 LISTO` + los dos commits finales.
