# 83. Informe (lo hizo Claude: agy sin cuota hasta ~16-11)

Rama `port152-arreglos-83` en `wt-152-rend`, sobre `9059476`. Compilan las 2 variantes (normal y exp) desde limpio
(las carpetas de build de `wt-152-rend` tenían objetos de otras ramas: hizo falta `--clean-first`).
Capturas xemu: `buzon-xbox\83-capturas\` (`skins-antes-*.png`, `skins-despues.png`, `crearmundo-despues.png`).

## 1. Skins LCE — `72f9675`
- **Rayas:** en xemu NO salen (xemu recorta). En la foto de la consola las rayas convergen en los modelos que el carrusel
  dibuja cortados por el borde de la pantalla: la GPU real no recorta bien triángulos muy fuera del borde. Ahora solo
  se dibujan los modelos que caben enteros (`slotX ± 0.8·escala` dentro de la pantalla). A 4:3 quedan 3 modelos, a 720p
  más. **Hay que confirmarlo en consola** (es la hipótesis que encaja con la foto; xemu no lo reproduce).
- **Pestañas:** cada paquete vecino sale una vez y en su lado (`< anterior` / `siguiente >`), atenuado si no tiene
  skins; con un solo paquete no hay pestañas laterales. Los clics van al paquete que se ve.

## 2. Crear mundo, interfaz Java — `e535b29`
- Tamaño de mundo y cofre de bonificación en la fila y=151 (izquierda/derecha), como Java 1.5.2 (allí van trucos y
  cofre). El texto «Aldeas, mazmorras, etc.» (y=122) ya no queda tapado.
- Textos más largos que el botón de 150 px se recortan con «...» (solo en la interfaz Java; la Legacy usa sus botones).

## 3. Logros — `4a48c6a` (PASAR TAMBIÉN A 1.2.5: mismo código en `wt-opti30`, cherry-pick directo)
- **Desplazamiento corto:** los límites eran `static const` calculados al arrancar, antes de crear los logros
  (todo valía 0 → ~35 px de recorrido). Ahora son funciones que se calculan al usarse: recorre todo el árbol como Java.
- **Líneas e iconos fuera del panel:** Java usa un truco de profundidad que D3D8 no reproduce. Nuevo
  `renderScissorPixels()` (en `platform/RenderAPI.h`; Xbox = `SetScissors` de la XDK, resto no hace nada) recorta el
  mapa al área 224x155 mientras se dibujan fondo, líneas e iconos; se quita antes del marco.
- No pude verlo en xemu (hace falta un mundo); probar en consola.

## 4. Wither: medido (NO cambiado)
Netlog de la pelea (`OptiCraftHeritageEdition\xbox-spike\netlog.txt` líneas ~91450-92300): la pelea fue DURANTE UN
ASEDIO ZOMBI. `xbox.ent` por segundo: 95-180 entidades, `entTick` 40-104 ms.
- **Zombis (45-60) y aldeanos (~30):** un solo zombi o aldeano llega a 25-55 ms en un tick → búsqueda de camino
  (asedio / huir / atacar aldeanos). Es lo que más pesa, con o sin Wither.
- **Calaveras del Wither:** 50-80 ms el tick que explotan (2-4 calaveras): explosión → `setBlock` + luz por bloque.
- **Wither:** 33-45 ms en algunos ticks (búsqueda de objetivos + romper bloques).
- **Objetos tirados:** poco (120 objetos = 4-5 ms). El 1/fuerza de `97af7c1` ya reduce cuántos salen.
- `getEntityByID` lineal: despreciable con ~180 entidades.

Propuestas (no cambian el resultado del juego):
1. **Presupuesto de búsquedas de camino por tick** (p.ej. 2-3 `PathFinder` por tick; el resto reintenta el tick
   siguiente, como hace Java cuando el navegador no tiene camino). Es lo que más ganaría en asedios y aldeas.
2. **Luz en explosiones:** cambiar los bloques del radio con aviso pero recalcular la luz del área una vez al final
   (o repartir el recalculo en varios ticks) en vez de por bloque.
3. **Revisar `VillageSiege`:** en Java un asedio trae como mucho 20 zombis; aquí había 45-60 (¿asedio + aparición
   normal de noche, o el asedio no cuenta bien?). Si es un fallo, arreglarlo baja mucho el coste.
4. `getEntityByID` con un mapa id→entidad (barato, solo ayuda con muchas entidades).

## Qué falta
- Confirmar en consola: rayas (punto 1) y logros (punto 3). Pasar `4a48c6a` a la 1.2.5.
- Copiar al lanzador (no lo hice).
