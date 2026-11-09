# ENCARGO 30 (agy): traer TODO lo nuevo de OptiJuegos a nuestra versión, con NUESTRO menú de crafteo como principal

Si algo no está en tu lista, para y pregunta. Si crees que me equivoco, dilo.

## Dónde
- Carpeta **`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30`**, rama `xbox-opti-30`, que sale de `xbox-unificada` (`5802c26`).
- **Solo trabajas ahí.** No toques `OptiCraftHeritageEdition\` ni otras carpetas.
- No hagas commit ni push: yo reviso y hago los commits.

## Qué ha pasado
- OptiJuegos aceptó las PR **#56, #57, #59 y #60** (y la #52). Su `origin/main` está en `ca93c04`.
- **Tú ayer ya uniste #56, #57, #59 y #60** en la rama `xbox-upstream-prs` (`b26a41f`, informe `buzon-xbox\29-INFORME-agy.md`). **Reutiliza ese trabajo.**
- Desde ayer, `xbox-unificada` tiene un commit nuevo mío, `5802c26`:
  - nuestro crafteo `XboxCraftingScreen` también en **PS2**;
  - arreglos para que compile en PS2: `SourceSelection.cmake` excluye los `*_XBOX` en otras plataformas, falta una llave en `GuiContainerCreative`, casts en `XboxCraftingScreen`.

## Pasos
1. `git merge --no-commit --no-ff xbox-upstream-prs`: trae tus resoluciones de ayer. Resuelve lo que choque con `5802c26`, sobre todo `EntityPlayerSP.cpp`, `Minecraft.cpp`, `GameSettings*`, `LegacyHeritageOptions.cpp` y `GuiContainerCreative.cpp`.
2. Después, `git merge --no-commit --no-ff origin/main`, para lo que falte (la #52 y los merges finales). Casi todo debería entrar ya resuelto.
3. Si git no te deja encadenar dos merges sin commit, avísame en el informe y para después del paso 1. Yo hago el commit y sigues.

## La regla del crafteo (lo más importante)
- Al usuario **no le gusta** el crafteo de OptiJuegos: "el mío está más lindo". El **nuestro** (`XboxCraftingScreen`) es el **principal en Xbox y en PS2**.
- El de ellos (`LegacyCraftingScreen`, que sigue cambiando con la PR #61) se queda en el código **sin tocar**. Es el que usan Wii y PC.
- **Choque de nombres:** OptiJuegos ahora tiene un ajuste `legacyCrafting` propio (por defecto `true`, significa "su crafteo legacy"), y nosotros teníamos uno con el mismo nombre. Solución:
  - El suyo **se queda como `legacyCrafting`**, con su significado y su clave de `options.txt`.
  - El nuestro pasa a llamarse **`xboxStyleCrafting`**:
    - clave `options.txt`: `xboxStyleCrafting`;
    - por defecto `true` en Xbox y PS2, `false` en el resto;
    - casilla en Opciones → Heritage con el texto **"Xbox-Style Crafting"**;
    - en Xbox y PS2, la casilla antigua "Console Crafting" que era nuestra se sustituye por esta.
  - **Dónde se abre el nuestro** (con `xboxStyleCrafting` activo):
    - **Xbox:** tecla X, mesa de trabajo, X del J2 (`XboxSplitScreen.cpp`), igual que ahora;
    - **PS2:** el botón de inventario y la mesa de trabajo, igual que en `5802c26`.
  - Con `xboxStyleCrafting` desactivado, se hace lo que diga el código de ellos (`legacyCrafting`, etc.).
- `platformGameSettingsApplyLegacyCrafting` y la acción `Crafting` de `LegacyControlPromptBackend` que añadió la #56: impleméntalas en Xbox como ayer.

## Nada de lo nuestro puede desaparecer (compruébalo con `grep` al final)
- Pantalla dividida con contexto J2 (`enterPlayer2Context`, `XboxSplitScreen.cpp`).
- `XboxCraftingScreen` (Xbox y PS2) y sus iconos incrustados (`TexturePackDefault.cpp`, ramas PS2 y Xbox).
- `SourceSelection.cmake` con `|XBOX` y `|D3D8_XBOX` en cada familia.
- Retención 3x3 del dragón en `World.cpp` (`isChunkRequiredByRetainedEntity` con radio y `ensureEntityChunkRetention` con doble bucle).
- `ItemRenderer.cpp`: `isHeldByInventory`.
- Todo lo async (`XBOX_ASYNC_CHUNK_IO`), el watchdog, `XboxCrashLog`, los límites de red de `XboxNetwork.cpp`.
- `GuiScreen.cpp`: los `|| PLATFORM_XBOX` del teclado en pantalla (teclado y navegación con mando).
- `Chunk.cpp`: `#if !PLATFORM_XBOX` alrededor del borrado de secciones.
- `AnvilChunkLoader`: `writingPending`/`writingDone`.
- Sonido de Xbox (`SoundManager_XBOX.cpp`): no se toca.

**Trampa conocida:** al quedarte con la versión de ellos en un conflicto se pierden nuestros `|| PLATFORM_XBOX`, porque sus bloques dicen solo `PLATFORM_PS2 || PLATFORM_WII`. Al final ejecuta:
```
git diff xbox-unificada -- src cmake | grep "^-.*XBOX"
```
y justifica en el informe cada línea que salga.

## Compilar (con candado `buzon-xbox\BUILD.lock`, como ayer)
- **Xbox, 2 compilaciones** (`xbox-release` y `xbox-release-exp`): los comandos son los del encargo 29, cambiando `wt-pr` por `wt-opti30`.
- **PS2** (`ps2-release`):
  - el compilador está en `OptiCraftHeritageEdition\psdevwindows`. Crea en `wt-opti30` un enlace de carpeta:
    ```
    New-Item -ItemType Junction -Path "<wt-opti30>\psdevwindows" -Target "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\psdevwindows"
    ```
  - luego:
    ```
    & $cm --preset ps2-release
    & $cm --build --preset ps2-release
    ```
    Debe acabar con `Linking CXX executable ...OptiCraft.elf`.
- xemu: arranca `bin\xbox\OptiCraft.iso` y `bin\xbox-exp\OptiCraft.iso` hasta el menú y guarda las capturas.
- No copies nada al loader.

## Informe
Escríbelo en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\30-INFORME-agy.md`, con primera línea literal `# INFORME 30 agy`. Incluye:
- cada conflicto y cómo lo resolviste, con `archivo:línea`;
- el `grep` de la lista de arriba y el del `^-.*XBOX`;
- `git diff --stat xbox-unificada`;
- el resultado de las **tres** compilaciones (Xbox normal, Xbox exp, PS2) y las capturas.

**No digas que algo está hecho si no lo está**: reviso el diff línea a línea.

---

## AÑADIDO (el usuario, probando en su PS2): botones de PS2 como en Xbox
**Fallo:** en PS2, **Triángulo suelta el objeto**, cuando debería **abrir el menú de crafteo** (el nuestro, `XboxCraftingScreen`).

**Lo que quiere el usuario:** el mando de PS2 y el de la Xbox clásica son prácticamente iguales. La única diferencia es que la Xbox no tiene L2/R2 sino Blanco/Negro. En PS2 las acciones deben ir en los **mismos sitios** que en Xbox:

| Acción | Xbox | PS2 |
|---|---|---|
| Abrir crafteo (nuestro) / inventario | Y | **Triángulo** |
| Soltar objeto | B | **Círculo** |
| Saltar | A | Cruz |
| (lo que haga X en Xbox) | X | Cuadrado |

Qué hacer:
1. **Averigua por qué** Triángulo suelta en PS2:
   - la #56 y sus arreglos (`41f9330`, `e6b8856`, `0f3991b`) cambiaron los botones por defecto de PS2: "Square=Inventory, Circle=Drop" cuando `legacyCrafting` está desactivado;
   - mira `src/platform/GameSettingsBackend_PS2.cpp`, `platformGameSettingsApplyLegacyCrafting` y `src/ps2/input/Ps2InputMapper.cpp`.

   Explica la causa en el informe con `archivo:línea`.
2. **En PS2 con `xboxStyleCrafting` activado:** Triángulo = abrir nuestro crafteo/inventario, Círculo = soltar. Toma los botones por defecto de Xbox como referencia (`src/platform/GameSettingsBackend_XBOX.cpp`: `keyBindInventory` = Y, `keyBindDrop` = B) y usa sus equivalentes de PS2.
3. Que funcione también con un `options.txt` de PS2 ya existente: los botones guardados de antes no deben dejar Triángulo en "soltar".
4. En PS2 con `xboxStyleCrafting` desactivado, déjalo como lo tengan ellos.
5. **Comprueba** el texto de las pistas de botones en pantalla (la fila de abajo en el juego): en PS2 debe decir Triángulo → Inventario/Crafteo y Círculo → Soltar.
