# ENCARGO 25 (agy): vigilante que diga DÓNDE se atasca el hilo principal

## Medido (consola real, `d233545`, 28-09 ~21:20)
- Iba mejor: `sync=0`, sin tirones de terreno, `freeKB` en 32.124.
- A los ~45 s la **pantalla se queda negra y la MÚSICA sigue**. El hilo de sonido está vivo; el principal está atascado.
- No hay reporte de `XboxCrashLog`. El log termina con líneas normales (`xbox.player`, un spike de `chunkUnload`/`save` de 25 ms).
- Última traza: `workerMs=5009 ... adopted=9 pending=2`. **Ojo:** `workerMs` se mide con `nanoTime` de pared, e incluye el tiempo en que el worker estaba desalojado. No es CPU real.

## Qué hacer, en `wt-async` (ya en `d233545`)
1. **Marcador de fase del hilo principal:** una variable global `volatile const char* g_mainPhase` (o un entero con tabla de nombres) que el hilo principal actualice en los puntos clave. Reusa los nombres del profiler que ya existen (`ClientProfiler::tickPhase`, las fases de `xbox.spike`: `input`, `entities`, `worldTick`, `chunkUnload`, `gen`, `light`, `render`, `build`, `opaque`, `display`/`present`…). Añade también marcas dentro de:
   - `drainAsyncGeneratedChunks`;
   - `publishPreparedChunk` / `populate`;
   - `provideChunk`;
   - el mallado (`WorldRendererPcLegacy`);
   - `Display::update` / `Present`.
2. **Hilo vigilante** (prioridad ALTA, sin tocar la lógica del juego): cada 500 ms mira un contador de fotogramas del hilo principal. Si en **2 s** no avanzó, escribe UNA vez por el log normal (que llega por red):
   ```
   [MC][E][xbox.watchdog] main stuck 2000ms phase=<fase> subphase=<subfase> frame=<n> pending=<n> freeKB=<n>
   ```
   Después, cada 5 s mientras siga atascado, repite la fase (por si cambia). Si puedes, añade el EIP del hilo principal (`GetThreadContext`, ver cómo lo hace `XboxCrashLog.cpp`) para buscarlo en el `.map`.
3. Todo detrás de `MC_LOG_LEVEL > 0`, para que la versión pública no lo lleve.
4. **Compila con candado** `xbox-release`, `-DMC_LOG_LEVEL=1`, y comprueba que xemu llega al menú.

## Tuyos / no tuyos
Los del encargo 24, más `src\client\Minecraft.cpp` (solo marcas de fase), `src\xbox\system\*` (el vigilante) y `src\xbox\lwjgl\Display_xbox.cpp` (marca en `present`). **NO cambies nada del comportamiento del juego.** No hagas commit.

## Informe
En `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\25-INFORME-agy.md`. Primera línea literal: `# INFORME 25 agy`. Incluye:
- dónde pusiste las marcas;
- cómo funciona el vigilante;
- el diff;
- la compilación y xemu.
