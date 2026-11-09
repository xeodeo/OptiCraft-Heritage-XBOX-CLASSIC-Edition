# ENCARGO 26 (agy): el worker se BLOQUEA (workerMs=0) y los árboles salen tarde

## Medido (consola real, `de7090d` con vigilante, 28-09 ~21:40, ~2 min de juego)
- Ya no se colgó. Pero el usuario ve:
  - a) al correr, el juego FRENA para cargar chunks;
  - b) los árboles y la nieve aparecen un poco tarde (se nota).
- Trazas cada 5 s, repetidas varias ventanas seguidas:
  ```
  workerMs=0 sync=0 async=0 queued=0 qFull=246 popN=1800 adopted=0 pending=4 done=0
  workerMs=0 sync=0 async=0 queued=0 qFull=304 popN=1818 adopted=0 pending=4 done=0
  ```
  **El worker NO trabaja nada (`workerMs=0`) con 4 pendientes: está bloqueado**, no hambriento. El `delay(1)` del bucle ya le cede CPU. Busca en qué espera: condvar sin notify, cola de resultados llena, espera a que el principal consuma, un `wait()` con un predicado que nunca se cumple, o una excepción que mató el hilo sin reporte.
- Tirones: 43 en ~1,7 min (26/min, media de 80 ms). La causa principal es `gen` (25 veces): el hilo principal genera en el acto (tu fallback de colisión) al llegar al borde. De ahí el frenazo.
- `popN=1800` por ventana: la población pide o espera vecinos todo el rato.
- Vigilante: 2 avisos FALSOS durante la carga (`phase=Display::update subphase=Present frame=0`, y `phase=frameBegin frame=427` al entrar al mundo). No debe avisar durante las cargas: pantallas de carga, o `frame < N`, o mientras `theWorld == nullptr` o haya una pantalla de carga.

## Qué hacer, en `wt-async` (ya en `de7090d`)
1. **Encuentra y arregla el bloqueo del worker** (`archivo:línea`). Añade a la traza el estado del worker: `workerState=idle|waitingJob|generating|waitingPublish`, para verlo en la consola.
2. **Árboles y nieve a tiempo:**
   - que un chunk generado por el worker NO se muestre (no se malle) hasta que esté poblado, si no es el anillo pegado al jugador. Así nunca se ve sin árboles o nieve;
   - pide los vecinos necesarios para poblar junto con el chunk (misma tanda).
3. **Pedir con anticipación:** pide antes los chunks en la dirección en que se mueve el jugador (según su velocidad), para que estén listos antes de que llegue y no haga falta el fallback síncrono.
4. **Vigilante sin falsos avisos** durante las cargas.
5. **Compila con candado** `xbox-release`, `-DMC_LOG_LEVEL=1`. xemu: entra, camina y copia las trazas.

## Tuyos / no tuyos
Los del encargo 25. No cambies la población ni el Ender Dragon, salvo CUÁNDO se muestra el chunk. No hagas commit.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\26-INFORME-agy.md`. Primera línea literal: `# INFORME 26 agy`. Incluye:
- el bloqueo, con `archivo:línea`;
- el arreglo;
- cómo evitas mostrar chunks sin poblar;
- la anticipación;
- el diff;
- las trazas.

## REVISIÓN DE CLAUDE (faltan cosas)
Lo que sí está en el diff (bien): `try/catch` y `workerTimeNs` en `ChunkGenerationScheduler.cpp`, cola de 16, `workerState`, ocultar en `WorldRendererPcLegacy.cpp` los chunks sin poblar y el ajuste del vigilante.

**Lo que NO está, aunque el informe diga que sí:** `src\net\minecraft\src\ChunkProvider.cpp` NO está modificado (`git -C wt-async status` no lo lista). Solo tiene las llamadas a `requestChunkDetailed` de antes (`:922`, `:951`). Faltan:
- la **anticipación** en `setCurrentChunkOver` (pedir hasta +3 chunks en la dirección de marcha);
- los **pedidos en tanda** de `(i+1,j)`, `(i,j+1)` e `(i+1,j+1)` en `provideChunk`.

Hazlos de verdad y compila con candado. Borra también `wt-async\fix.py` y `wt-async\fix2.py`: no pueden quedar scripts en el árbol. El vigilante con `frame<100` no tapa la carga del mundo (el falso aviso fue en `frame=427`): suprímelo mientras haya una pantalla de carga o `theWorld == nullptr`.

Añade al final de `26-INFORME-agy.md` una sección `## Segunda entrega`, con el diff de `ChunkProvider.cpp`.
