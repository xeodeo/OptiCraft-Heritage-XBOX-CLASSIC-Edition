# ENCARGO 22 (agy): `7e10e5c` (async) se congela en la consola

## Medido (consola real, `OptiCraft_async.xbe` = `7e10e5c`, 28-09 ~17:20; log `xbox-spike\netlog.txt`, ~320 líneas)
- Tus trazas cada 5 s:
  ```
  sync=0 async=668  queueFull=18 adopted=18
  sync=0 async=159  queueFull=0  adopted=6
  sync=0 async=440  queueFull=0  adopted=13
  sync=0 async=1551 queueFull=0  adopted=7
  sync=0 async=2394 queueFull=82 adopted=1
  ```
  **Miles de peticiones para 1-18 adopciones.** Casi seguro, se piden los MISMOS chunks cada tick (sin deduplicar, o contando reintentos), y la adopción (`PUBLISH_PER_TICK=1`) es muy lenta.
- Últimas líneas antes de cortarse:
  ```
  spike frame=100ms {entities=88.6 entTick=88.6}
  spike frame=125ms gen=84.3ms render=114.2ms [build=108.5 ...]
  ```
  Después, NADA. **`XboxCrashLog` (`src\xbox\system\XboxCrashLog.cpp`) NO reportó nada**, y reporta access violations y excepciones de cualquier hilo por este mismo log. Por eso sospecho un **cuelgue o deadlock**, no un crash:
  - ¿el hilo principal espera (mutex, spin, join o busy-wait) algo que tiene el hilo del scheduler, con prioridad BAJA, y que nunca llega a ejecutarse? En una CPU de un solo núcleo, un spin del hilo principal no deja correr al de prioridad baja.
  - Nota: `gen=84ms` dentro de un fotograma con `sync=0` indica que el hilo principal hizo trabajo de generación o esperó a pesar de `sync=0`.
- Memoria bien: `freeKB` alrededor de 34.500.

## Qué hacer, en `wt-async` (ya en `7e10e5c`)
1. Busca con `archivo:línea` todo sitio donde el hilo PRINCIPAL pueda esperar al scheduler: `lock` de mutex compartido con el worker, `wait`, bucles de reintento, `join`, `drain` que bloquee. Arregla para que el hilo principal use solo `try_lock` o colas sin bloqueo, y nunca espere.
2. **Deduplicación:** que una columna ya pedida (en cola o en curso) no se vuelva a pedir ni cuente. `async` debe contar pedidos NUEVOS.
3. **Adopción:** sube `PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK` y `..._PER_FRAME` a un valor razonable (2-4) y mide el coste en el log. Añade a la traza `pending=` (tamaño de la cola).
4. Explica qué hace `gen` en el profiler con `sync=0`: ¿qué se está midiendo ahí?
5. Compila con candado `xbox-release` con `-DMC_LOG_LEVEL=1` y comprueba que xemu llega al menú. Entra a un mundo en xemu si puedes (autotest con `-Script`) y copia las trazas `xbox.async` que veas.

## Tuyos / no tuyos
Igual que en el encargo 20. No hagas commit.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\22-INFORME-agy.md`. Primera línea literal: `# INFORME 22 agy`. Incluye:
- los sitios de espera, con `archivo:línea`;
- la deduplicación;
- la adopción;
- la respuesta sobre `gen`;
- el diff;
- la compilación y xemu.
