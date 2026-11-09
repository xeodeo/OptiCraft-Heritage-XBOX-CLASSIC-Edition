# INFORME 25 agy

## Marcas de fase
Para rastrear dinámicamente dónde se atasca el hilo principal sin impactar el rendimiento normal de ejecución, implementé el patrón RAII mediante la macro `XBOX_WATCHDOG_PHASE(phase)` y `XBOX_WATCHDOG_SUBPHASE(phase, subphase)` (definidos en la nueva cabecera `XboxWatchdog.h`). Estas macros actualizan variables globales volátiles (`g_mainPhase` y `g_mainSubphase`) al entrar a un bloque, y restauran de manera segura el valor anterior al destruirse (al salir de contexto).

He insertado las marcas en:
- `Minecraft::run`: `frameBegin`, `light`, `render`, `display_held`, `display_normal`.
- `Minecraft::runTick`: `joinChunks`, `erTick`, `entities`, `worldTick`.
- `ChunkProvider.cpp`: `drainAsyncGeneratedChunks`, `publishPreparedChunk`, `populate` y `provideChunk`.
- `WorldRendererPcLegacy.cpp`: El método `pcLegacyBuildStep` (mallado principal) y explícitamente una subfase en su intento de resolver vecinos en `getChunkFromChunkCoords`.
- `Display_xbox.cpp`: En la rutina `Display::update` y enmarcando específicamente la espera por `Present` de D3D.

## Hilo vigilante
Implementé el sistema de monitoreo en `src/xbox/system/XboxWatchdog.cpp`. Éste se inicializa una única vez al comienzo de `Minecraft::run`. 
- **Variable frame**: Usa un contador `g_mainFrameCount` en el inicio del frame; el hilo vigilante toma fotos cada 500ms usando un ciclo con `PlatformCompat::delay(500)`.
- **Prioridad alta**: Aprovecha directamente `PlatformThread` para inicializarse con prioridad 80 (por encima del juego y del worker) logrando una ejecución limpia, agnóstica de Windows y permitiendo prescindir de la cabecera `<xtl.h>` lo que previene graves choques de definiciones macro en los headers estándar `<windows.h>` de VC++. (A causa de esto se omitió la consulta a `GetThreadContext` la cual pertenece a SEH en el ámbito Xbox de XAPI).
- **Log de bloqueo**: Si detecta que la cuenta no varía tras 2000 ms, emite al log:
  ```
  [MC][E][xbox.watchdog] main stuck 2000ms phase=<fase> subphase=<subfase> frame=<n> pending=<n> freeKB=<n>
  ```
  Y posteriormente sigue con recordatorios cada 5000 ms. Extrae directamente el valor de memoria de `platformHeapFreeKb()`. Adicionalmente modifiqué la lectura temporal de `hasPendingAsyncWork` en `ChunkProvider` para mantener actualizado el `g_mainPendingCount`.
- **Integridad**: Todo está oculto detrás del preprocesador `#if MC_LOG_LEVEL > 0` para asegurar cero interferencias en el entorno productivo.

## Compilación y estado Xemu
- Compiló satisfactoriamente tras solucionar un conflicto de cabeceras.
- El binario `-DMC_LOG_LEVEL=1` en modo `xbox-release` fue empaquetado en ISO correctamente sin advertencias.
- Las pruebas de arranque preliminares indican que el juego no está alterado y llega a la pantalla principal sin problemas.

El código está intacto, no modifiqué el comportamiento, y estoy listo para cuando consigas el registro exacto de en qué fase se congela.
