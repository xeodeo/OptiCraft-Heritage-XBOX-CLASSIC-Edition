# 83. Encargo agy: rayas en skins LCE, menú Crear mundo (Java), logros, y medir el Wither (09-11)

Fotos del usuario en consola real: `buzon-xbox\83-fotos\` (míralas TODAS antes de empezar).

## Dónde
- Worktree `wt-152-rend`, rama NUEVA `port152-arreglos-83` (ya creada por Claude desde `9059476`). SIN reset.
- Los puntos 3 también afectan a la 1.2.5: haz el arreglo en 1.5.2 y anota en el informe qué commit hay que pasar a
  la 1.2.5 (`wt-opti30`, rama `opti30-cruceta`); NO toques `wt-opti30` tú.

## 1. Skins estilo LCE (`GuiSkinSelector`, tu commit b48891e): rayas de colores (foto 1)
- Los modelos laterales «estiran» triángulos hasta el borde de la pantalla (rayas naranjas/verdes). Es lo típico de la
  Xbox: vértices detrás de la cámara o cruzando el plano cercano sin recorte, o una proyección/profundidad mal puesta
  para el modelo 3D (w negativo). Revisa cómo dibujas los 9 modelos: proyección, `z` de cada modelo, near/far, que
  ningún modelo quede detrás del plano cercano, escala con signo. Compara con cómo se dibuja el jugador en el
  inventario (`GuiInventory`/`GuiContainerCreative` drawPlayerModel) que en consola se ve bien.
- Las dos pestañas laterales dicen lo mismo («< Personalizado (0)» y «Personalizado (0) >»). Con 2 paquetes, muestra
  solo el otro una vez; si «Personalizado» tiene 0 skins, que se vea atenuado o no salga.
- Acéptación: sin rayas a 480p y 720p (captura en xemu), carrusel suave.

## 2. Crear un mundo nuevo, interfaz Java (sin Legacy UI) en 1.5.2 (fotos 2 y 3)
- En «más opciones» los botones se pisan: «Cofre de bonificación: Desac…» encima de «…: Infinito» (tamaño de mundo de
  OptiJuegos) y la etiqueta «Aldeas, mazmorras, etc.» queda tapada. Reordena como Java 1.5.2 (`GuiCreateWorld.java` en
  `mcp-diff\src152`: estructuras / tipo de mundo / trucos / cofre) añadiendo el botón de tamaño de mundo en su propia
  fila, sin que nada se corte ni se pise a 480p y 720p. Texto que no quepa: que se ajuste, no que se salga.

## 3. Logros (`GuiAchievements`), en 1.5.2 y 1.2.5 (foto 4)
- Las líneas de conexión y los iconos se dibujan FUERA del panel (no hay recorte). Java usa un truco de profundidad
  (`glDepthFunc(GL_GEQUAL)` con una capa a z alto) que en nuestro D3D8 no recorta. Haz el recorte de verdad: recorta
  por CPU las líneas/iconos al rectángulo del panel, o usa recorte de la XDK si existe (mira `D3DDevice_SetScissors` /
  viewport en la doc extraída `scratchpad\xdkdoc` o en la XDK). Que no quede nada fuera del panel.
- Desplazamiento: «se mueve solo un poco». Debe recorrer todo el árbol como Java (`field_74117_m/n`... límites
  `guiMapLeft/Top/Right/Bottom` de `GuiAchievements.java`), con stick y cruceta, a velocidad razonable.

## 4. Wither: medir y proponer (NO cambies el juego todavía)
- Netlog de la pelea (`OptiCraftHeritageEdition\xbox-spike\netlog.txt`, desde la línea ~91450): mediana ~27 fps,
  mínimo 6, tirón de 470 ms en `entities` al nacer. Averigua de dónde sale (¿cientos de EntityItem de la explosión?
  ¿EntityItem que se juntan 1.5.2 `searchForOtherItemsNearby`? ¿calaveras y `getEntityByID` que recorre la lista?)
  leyendo el código, y propón arreglos que no cambien el resultado del juego. Escríbelo en el informe.

## Reglas
- Los ~60 fps de referencia no se pueden perder (sin trabajo por frame fuera de estas pantallas).
- Texto encima de iconos/modelos sin depth test; restaurar estados de render al salir.
- `BUILD.lock`; compila `build/xbox-release` y `build/xbox-release-exp` del worktree. NO copies al lanzador.
- Capturas xemu en `buzon-xbox\83-capturas\` si puedes. Informe `buzon-xbox\83-INFORME-agy.md`.
- Señal final literal: `ENCARGO 83 LISTO <commit>`

## 09-11 13:00: agy SIN CUOTA (se reinicia en ~165 h, hacia el 16-11). No dejó cambios (rama limpia en 9059476).
Lo hace Claude con un solo agente en el mismo worktree y rama.
