# ENCARGO para OpenCode: revisar (SOLO LEER) el trabajo de agy en la versión de prueba

Eres un revisor. **No edites ningún fichero de código.** Solo escribe tu informe. Si crees que me equivoco en algo, dilo.

## Contexto
- Proyecto: port de OptiCraft Heritage (Minecraft Beta en C++17) a la Xbox original (Pentium III sin SSE2, 64 MB). Contexto general en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\00-CONTEXTO.md`. Léelo primero.
- Carpeta a revisar: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraft-prueba-opti` (rama `prueba-opti`, merge de `origin/main` sin commit).
- Otro agente (agy) hizo en esa carpeta los encargos `buzon-xbox\09-ENCARGO-agy-prueba-opti.md` y `buzon-xbox\10-ENCARGO-agy-parteC-mejoras-opti.md`. Su informe está en `buzon-xbox\09-INFORME-agy-prueba-opti.md`.
- Ver sus cambios: `git -C "<carpeta>" diff` (lo que no está en el índice es de agy) y `git -C "<carpeta>" diff --cached`.
- La versión compila. Lo que hay que buscar son **fallos de lógica**, no de compilación.

## Qué revisar (en este orden)
1. `src/client/Ps2SplitScreen.cpp`, capa de compatibilidad Xbox:
   - ¿cada botón de PS2 queda bien traducido al de Xbox? Cross→A, Circle→B, Square→X, Triangle→Y, L1/R1→White/Black, L2/R2→LT/RT, R3→clic del stick derecho, Start→START, Select→BACK;
   - ¿`ps2PadGetSnapshot` para Xbox convierte bien los sticks y el signo de los ejes?
   - ¿`platformSetMenuPad` / `platformClearMenuPad` se llaman en los mismos sitios que el original de PS2?
2. `src/net/minecraft/src/MovementInputFromOptions.cpp` y `EntityRenderer.cpp`: el J2 en Xbox ¿se mueve y gira la cámara con el mando 2 y no con el 1?
3. `src/net/minecraft/src/ChunkProviderLoadOrGenerate.cpp` y `World.cpp`: la retención de chunks del dragón (`PLATFORM_ENTITY_CHUNK_RETENTION`) ¿puede retener chunks para siempre o gastar mucha memoria? Límite: 64 MB en total.
4. `legacy/LegacyCraftingScreen.cpp`, `GuiContainerCreative.cpp`, `GuiContainer.cpp`: ¿las ramas nuevas de Xbox leen el mando del jugador correcto (`platformMenuPad()`)? ¿Pueden disparar dos acciones con una sola pulsación?
5. `src/mods/strongholdlocator/StrongholdLocatorMod.cpp` y `src/mods/reiminimap/GuiWaypointManager.cpp`: ramas de Xbox.

## Informe
Escríbelo en:
`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\11-INFORME-opencode-revision.md`

- **Primera línea literal:** `ESTADO: REVISADO` o `ESTADO: BLOQUEADO`.
- **Después:** una lista de hallazgos. Cada uno con fichero:línea, qué pasa, la gravedad (alta/media/baja) y cómo arreglarlo. Si algo está bien, dilo en una línea.
- Solo cosas que hayas comprobado leyendo el código. Nada inventado.
