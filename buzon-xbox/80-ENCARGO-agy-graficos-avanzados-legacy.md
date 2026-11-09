# 80. Encargo agy: botón «Avanzado» en Gráficos del menú Legacy (09-11) — EN COLA tras el 79

Pedido del usuario: en Java hay muchas opciones gráficas avanzadas (las de OptiFine: partículas, humo animado, etc.)
que en el menú Legacy no salen. Añadir un botón **«Avanzado»** dentro de Gráficos del menú Legacy que abra esas opciones,
y que se vea y maneje bien a **720p y a 480p**, con mando.

## Dónde
- Worktree `wt-152-rend`, rama `port152-graficos-legacy` (ya creada desde `91b5f42`). SIN reset.
- Menú Legacy de gráficos: `src/net/minecraft/src/legacy/LegacyVideoOptions.cpp` (12 opciones hoy: distancia, gráficos,
  luz suave, balanceo, nubes, niebla, brillo, antiparpadeo, límite de fps, coordenadas, fullbright). Piezas:
  `LegacyOptionCheckbox`, `LegacyOptionSlider`, `LegacyOptionsLayout` (`rowY`, filas), `LegacyOptionsScreen`, `LegacyGuiButton`.
- Pantallas Java tipo OptiFine que ya existen y funcionan (fuente de las opciones y su lógica):
  `GuiDetailSettingsOF`, `GuiAnimationSettingsOF`, `GuiQualitySettingsOF`, `GuiPerformanceSettingsOF`,
  `GuiOtherSettingsOF`, `GuiWorldSettingsOF` (≈34 `EnumOptions::`). Referencia OptiFine 1.5.2: `mcp-diff\srcOF152`.

## Qué hacer
1. Botón «Avanzado» al final de Gráficos (antes de «Hecho») que abra una pantalla Legacy nueva (p.ej.
   `LegacyAdvancedVideoOptions`), con sub-botones o secciones: Detalles, Animaciones, Calidad, Rendimiento, Otros
   (mismos grupos que OptiFine). Cada opción con el control Legacy adecuado (casilla si es sí/no, deslizador o
   botón que cicla si tiene varios valores), usando EXACTAMENTE la misma `EnumOptions`/`GameSettings` que la pantalla
   Java, para que cambiar en un menú se vea en el otro y se guarde en `options.txt`.
2. Que todo quepa: si no cabe en una pantalla a 480p, desplazamiento (scroll) con la cruceta/stick como hacen otras
   listas Legacy, o páginas. Comprobar a 640x480 y a 1280x720 (escala Legacy grande). Nada cortado ni encimado.
3. Navegación con mando completa (arriba/abajo, izquierda/derecha cambia valor, A, B vuelve), y textos en los idiomas
   (`StatCollector`/`uiText` como las otras opciones Legacy; si falta traducción, la de OptiFine en inglés).
4. Opciones que en Xbox no tengan sentido (p.ej. VSync de PC, pantalla completa, mipmaps si no se usan): no las pongas,
   y anótalo en el informe.

## Trampas
- No cambies valores por defecto de ninguna opción. No toques el menú Java ni el creativo.
- Texto encima de iconos: sin depth test. Revisa que el menú Legacy existente sigue igual.

## Compilar y entregar
- `BUILD.lock`; `cmake --build build/xbox-release` y `build/xbox-release-exp` del worktree. NO copies al lanzador.
- Capturas en xemu a 480p y a 720p si puedes (con `xbox-spike\captura.ps1`), en `buzon-xbox\80-capturas\`.
- Informe `buzon-xbox\80-INFORME-agy-graficos-avanzados.md`. Señal final literal: `ENCARGO 80 LISTO <commit>`
