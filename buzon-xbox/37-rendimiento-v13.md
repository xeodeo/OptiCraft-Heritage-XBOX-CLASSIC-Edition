# 37 — Rendimiento para la v1.3 (08-10)

## Hecho hoy (sin commit, en wt-opti30, junto con la luz del 36)
1. **Día/noche sin reconstruir chunks** (`World.cpp`, `World::tick`): con el lightmap, el cambio de
   luz del cielo ya no llama a `updateAllRenderers()` en Xbox (como la 1.2.5 original). Antes se
   reconstruían TODAS las secciones unas 11 veces en cada amanecer/atardecer.
2. **Compilador/enlazador:** `/Gw` (cl) + `/OPT:ICF` (link del XDK). XBE 4.521.984 → **4.206.592**
   bytes (−308 KB de RAM). SSE2: la misma lista de 13 funciones del CRT que antes (comprobado
   pasando `patch_sse2_moves.ps1` al exe anterior). Arranca en xemu.

## Opciones de compilador ya descartadas (no repetir)
- LTCG (`/GL`): imposible con el link 7.1 del XDK (LNK1262); con link moderno mete SSE2 del CRT
  y llena las "caves" del parche (17/18-INFORME-agy).
- Unity build: ~60 choques de nombres (16-INFORME-opencode).
- `/arch:SSE2`: la CPU es un Pentium III.

## Opciones posibles, sin probar
- `/fp:fast`: **NO, el usuario lo prohibió (08-10): nada que pueda cambiar el terreno.**
- `/Ob3` (más inlining): más velocidad, XBE más grande (= menos RAM). Medir.

## Siguientes (ideas de Legacy, `minecraft-legacy original\Minecraft.Client`)
1. Reconstruir primero el chunk que el jugador acaba de tocar (`LevelRenderer.cpp` 2314–2350, 1961–2005).
2. Elegir el chunk sucio más cercano con la distancia vertical ×4 y descartar los vacíos
   (`LevelRenderer.cpp` 1930–1995).
3. Copiar la columna de bloques a un array plano antes de mallar (`Chunk.cpp` 225–246):
   en Vita `getTile` bajó del 20 % al 4 %.
4. Medir antes: subfases de `chunkUnload` y `entTick` por tipo de mob (pendiente desde el 01-10).

## 08-10 tarde
- Partículas del vacío: 1 de cada 8 en Xbox (contador propio, sin tocar el rand del mundo).
- **El log de la consola NO llega:** la red Ethernet del PC pasó a "Pública" (IP ahora .18) y el
  firewall bloquea UDP 9999. Solución: regla de firewall o red Privada. Las builds de prueba ya
  llevan 192.168.0.18.
- Pendiente con log: tirón de generación en la exp (el usuario lo nota), RAM.

## 08-10 noche (commits 0c37b60..)
- Mobs enterrados/ahogándose al reaparecer: se colocaban junto a chunks que aún venían del hilo
  de fondo (se leen como aire). Ahora no aparecen si los chunks a ±2 bloques no están cargados;
  y si uno lleva 2 s dentro de un bloque, se sube a la superficie.
- Creepers de día: chunk recién llegado sin la luz del cielo calculada. En Xbox, de día y bajo el
  cielo abierto, ningún monstruo aparece.
- Log: `xbox.ent` (coste por tipo) + `xbox.ent slowest:` (la entidad más lenta, posición y si se
  cargaron chunks durante su tick). Un solo mob llega a 40-100 ms: falta saber qué hace.
- RAM: baja de 38 a ~19 MB y se queda plana (techo de la caché de chunks); morir no la baja.
- Generación: el hilo de fondo saca ~2 chunks/s y va al 100 % (workerMs≈5000/5 s); al correr se
  llega al borde. Probar "30 FPS Limit" (le da ~4 veces más CPU al hilo) y menos distancia.
