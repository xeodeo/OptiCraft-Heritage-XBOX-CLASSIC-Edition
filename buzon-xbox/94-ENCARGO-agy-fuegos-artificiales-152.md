# 94. Encargo agy: fuegos artificiales (1.5.2) (20-11)

Plan de la 1.5.2 (`69-PLAN-COMPLETO-152.md`, fila 4). Rama `port152-erluza` (`wt-152-rend`, último `7a3b267`).
Sin reset, sin push, sin copiar al lanzador. Solo 1.5.2. Reglas de siempre: BUILD.lock, 60 fps, memoria (64 MB),
creativo NO se toca (salvo que Java 1.5.2 ponga cohete/estrella en una pestaña: entonces sí, en su pestaña y nada más),
terreno igual, dos interfaces (Java y Legacy), pantalla partida, crafteo de consola incluido.

## Qué portar desde Java 1.5.2 (`mcp-diff/src152`)
- Objetos: `ItemFirework` (cohete), `ItemFireworkCharge` (estrella) con su NBT (`Fireworks`: `Flight`, `Explosions`:
  `Type`, `Colors`, `FadeColors`, `Flicker`, `Trail`) y sus textos al pasar el cursor (como Java).
- Recetas: `RecipeFireworks` (cohete con papel+pólvora+estrellas; estrella con pólvora+tintes+extras: bola de fuego,
  pepita de oro, pluma, calavera, diamante, polvo de piedra luminosa; tinte de desvanecido). También en el menú de
  crafteo de consola si nuestras recetas dinámicas lo permiten (si no, anótalo).
- Entidad `EntityFireworkRocket` (vuelo, `lifetime`, `Flight`), lanzar desde el suelo y desde dispensador (si el
  dispensador 1.5.2 aún no existe, solo el comportamiento básico o anótalo para el encargo del dispensador).
- Partículas `EntityFireworkStarterFX`, `EntityFireworkSparkFX`, `EntityFireworkOverlayFX` (formas: pequeña, grande,
  estrella, creeper, explosión), parpadeo y estela, sonidos `fireworks.*` (si no están en el pak, anótalo; si hay que
  añadirlos, como hicimos con aldeano/bruja en `scripts/port152/extra_sounds/`).
- **Rendimiento**: las explosiones grandes generan cientos de partículas; ponles tope en Xbox (como el resto de
  partículas) sin que se vea pobre. Medir fps con 10 cohetes a la vez.

## Aceptación
- Craftear estrella y cohete (Java y crafteo de consola), lanzarlos, ver las 5 formas, colores, desvanecido, parpadeo
  y estela; sonidos. Capturas en `buzon-xbox\94-capturas\`.
- 10 cohetes a la vez: fps y `KB libres` antes/después; sin cuelgues ni fugas.
- Guardar con un cohete en el aire o en un cofre: se conserva el NBT.

## Informe
`buzon-xbox\94-INFORME-agy.md`. Señal: `ENCARGO 94 LISTO` + commit final.

## TRASPASO (Claude, 20-11 ~14:50)
agy se quedó sin cuota (vuelve ~16:20) mientras leía el código (XboxCraftingScreen, EntityHugeExplodeFX,
EntityLargeExplodeFX...). No había escrito nada: árbol limpio en `7a3b267`. Al volver: empezar el encargo desde el
principio, ya con el código leído en mente.
