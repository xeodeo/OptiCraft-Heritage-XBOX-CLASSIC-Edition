# ENCARGO 27 (agy): cuelgue DENTRO del render + el worker sigue bloqueado

## Medido (consola real, `8d38130`, 28-09 21:47)
- **Vigilante** (funciona, gracias):
  ```
  main still stuck 12000ms phase=render subphase=- frame=5872 pending=0 freeKB=30600
  ... 17000 / 22000 / 27000 ms, la misma fase
  ```
  El hilo principal se atasca en la **fase `render`**, sin subfase: fuera del mallado (que sí tiene subfase).
- Antes del cuelgue, **el worker SIGUE bloqueado**, a pesar del `try/catch` del encargo 26:
  ```
  workerMs=0 ... qFull=123 popN=2000 adopted=0 pending=4 done=0
  workerMs=0 ... qFull=211 popN=2000 adopted=0 pending=4 done=0
  ```
  Además, el vigilante dice `pending=0` y `xbox.async` dice `pending=4`: `g_mainPendingCount` no se actualiza bien.

## Qué hacer, en `wt-async` (ya en `8d38130`)
1. **Subfases dentro de `render`**, en `EntityRenderer.cpp` (`updateCameraAndRender`, `renderWorld`, `renderSplitScreen`) y en `RenderGlobal`. Como mínimo:
   - `getMouseOver`;
   - `frustum`;
   - `sortAndRender` / `terrainVisibility`;
   - `opaque`, `entities`, `tileEntities`, `particles`, `transl`, `weather`, `clouds`, `sky`, `hand`, `hud`;
   - pantalla (`screen`).
2. **EIP del hilo principal** en el aviso del vigilante: `SuspendThread` + `GetThreadContext` + `ResumeThread`, con `Eip` y `Ebp` en hex, más 8 palabras de la pila que parezcan código. Mira cómo lo hace `src\xbox\system\XboxCrashLog.cpp`. Así Claude lo busca en el `.map`.
3. **Worker bloqueado:** con `pending=4 done=0 workerMs=0` durante 15-20 s, el worker NO está haciendo nada. Añade a la traza `workerState` (que ya creaste) Y el EIP del worker cuando `done=0` dos ventanas seguidas. Revisa `runWorker` (`ChunkGenerationScheduler.cpp`): condvar, espera por la cola de resultados llena, espera a que el principal consuma… Explica con `archivo:línea` dónde se queda.
4. Arregla `g_mainPendingCount`.
5. **Compila con candado** (¡bórralo al terminar!) `xbox-release`, `-DMC_LOG_LEVEL=1`.

## Tuyos / no tuyos
Los del encargo 26, más `src\net\minecraft\src\EntityRenderer.cpp` y `RenderGlobal*.cpp` (SOLO marcas de subfase, sin cambiar el comportamiento). No hagas commit.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\27-INFORME-agy.md`. Primera línea literal: `# INFORME 27 agy`.
