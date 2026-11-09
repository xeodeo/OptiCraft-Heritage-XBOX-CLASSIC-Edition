# ENCARGO 24 (agy): la async se cuelga porque el hilo de fondo se queda SIN CPU

## Medido en la consola real (`28efe0c`, 28-09 ~21:00, se colgó dos veces)
- **Bien:** ya salen árboles (confirmado por el usuario). Gracias.
- Última traza antes de colgarse:
  ```
  perf fps=44.4 frame=22.5ms present=0.0ms ... render=17.6ms
  async sync=0 async=0 queued=0 qFull=26 adopted=0 pending=4 done=0
  spike frame=103ms gen=44.1ms render=88.7ms [build=73.4 ...]
  spike frame=138ms gen=38.7ms save=15.1ms render=98.4ms [build=83.3 ...]
  (fin del log: sin reporte de XboxCrashLog)
  ```
- **Diagnóstico** (compruébalo):
  - con `present=0.0ms` al hilo principal ya no le sobra tiempo de vsync, y el worker, de prioridad BAJA en un solo núcleo, **no recibe CPU nada**: `pending=4 done=0` durante 5 s;
  - mientras, el mallado (`build` de 73-83 ms, `gen` dentro de render: `WorldRendererPcLegacy.cpp:332` pide columnas vecinas) sigue pidiendo chunks que devuelven `blankChunk` para siempre;
  - resultado: bloqueo o bucle, no crash.
- También vio el usuario: al caminar alcanza el borde (vacío) antes de que se genere. El worker solo termina 2-9 chunks cada 5 s en general.

## Qué hacer, en `wt-async` (ya en `28efe0c`)
1. **El worker debe tener CPU garantizada**, sin quitarle fotogramas al juego. Opciones (elige, justifica y mide en el log):
   - a) cuando hay trabajos pendientes, el hilo principal cede CPU cada fotograma (por ejemplo `Sleep(1)`, o esperar el vsync de forma que ceda). Ojo: `Sleep(0)`/`SwitchToThread` NO dan CPU a un hilo de menor prioridad;
   - b) prioridad NORMAL para el worker, pero trocea su trabajo (ceder tras cada paso de generación) para que no le robe más de ~X ms seguidos al principal;
   - c) si el worker lleva más de N ticks sin terminar un pedido que alguien necesita (mallado, colisión, jugador), el hilo principal lo genera él mismo con presupuesto por pasos, como la estable (`drainPendingGeneration` / generación incremental).

   Lo más robusto quizá sea combinar b) y c).
2. **Nunca un bucle ni una espera sin fin** por un `blankChunk`. Revisa el mallado (`WorldRendererPcLegacy.cpp:332`), la colisión (tu cambio en `PlatformBlockCollisionSweeper.cpp`), el spawn y la luz. Si el chunk no está, se sigue adelante y se reintenta en otro tick.
3. **Pedir con anticipación** (opcional si queda tiempo): pedir primero los chunks en la dirección de movimiento del jugador, para que estén antes de que llegue al borde.
4. Añade a la traza `xbox.async` el tiempo de CPU que usó el worker en la ventana (`workerMs=`).
5. Compila con candado `xbox-release`, `-DMC_LOG_LEVEL=1`. xemu: entra a un mundo y camina.

## Tuyos / no tuyos
Los del encargo 20, más `src\platform\world\PlatformBlockCollisionSweeper.cpp` y `src\net\minecraft\src\WorldRendererPcLegacy.cpp` (solo para no bloquear por `blankChunk`). No hagas commit.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\24-INFORME-agy.md`. Primera línea literal: `# INFORME 24 agy`. Incluye:
- el diagnóstico confirmado, con `archivo:línea`;
- la opción elegida;
- el diff;
- las trazas de xemu (`workerMs`, `done`).
