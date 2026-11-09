# Port de OptiCraft Heritage a Xbox clásico — contexto común para todos los agentes

Léelo entero antes de tocar nada. Tu tarea concreta está en tu propio fichero de este buzón.

## Qué es el proyecto
- Repo: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition` (rama `xbox-port`).
- Motor C++17 de Minecraft Beta clásico, ya portado a PC (SDL2/OpenGL), PS2 y Wii.
- Código compartido: `src/net`, `src/java`, `src/client`, `src/platform`, `src/util`, `src/mods`.
- Cada consola tiene su árbol: `src/wii/`, `src/ps2/`, y ahora **`src/xbox/`**.
- Los *backends* de plataforma son `src/platform/<Familia>_<PLAT>.cpp`. Para Xbox se llaman `*_XBOX.cpp`
  (los renderers: `RenderAPI_D3D8_XBOX.cpp`, `RenderTerrainAPI_D3D8_XBOX.cpp`). Hoy son ficheros vacíos
  con un comentario `placeholder` — hay que implementarlos.
- **La referencia principal es el port de Wii** (`src/wii/` y `src/platform/*_WII.cpp`). Copia su
  estructura y adapta. El de PS2 es más raro (GS), úsalo solo como segunda referencia.

## Plataforma Xbox (ya resuelto, no lo toques)
- CPU Pentium III 733 MHz (x86, SSE **sin** SSE2), 64 MB RAM unificada, GPU NV2A (tipo GeForce 3).
- Toolchain híbrido YA FUNCIONANDO y probado en el emulador xemu:
  - compilador VS2022 `cl.exe` (C++17, STL y UCRT modernos),
  - linker/librerías/imagebld del XDK de Microsoft (D3D8, XAPI, DirectSound, XInput).
- `src/xbox/runtime/` (arranque + shim del CRT) y `cmake/xbox*.cmake` son del coordinador. **No los edites.**
- Excepciones C++, RTTI, `std::thread`, `thread_local`, streams: todo funciona en la Xbox.

## Reglas de cabeceras (IMPORTANTE)
- Un fichero que necesite APIs de Xbox (D3D8, XInput, DirectSound, XAPI) incluye **`"XboxXtl.h"`**
  (nunca `<xtl.h>` directo) y **nunca** `<windows.h>`. Esos ficheros deben vivir en `src/xbox/...`
  o llamarse `*_XBOX.cpp`: el build solo les da las cabeceras del XDK a ellos.
- El resto del código compartido NO puede incluir `XboxXtl.h`.
- Macros: `XBOX_PLATFORM` / `PLATFORM_XBOX` (ver `src/platform/PlatformConfig.h`). Muchos
  `#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)` ya se amplían con `|| defined(XBOX_PLATFORM)`.
- Donde el código tenga `#if defined(WII_PLATFORM) ... #elif defined(PS2_PLATFORM) ... #else <SDL/PC>`,
  añade una rama `#elif defined(XBOX_PLATFORM)` con la implementación Xbox.
- En Xbox no hay SDL, ni OpenGL, ni libogc, ni `<dirent.h>`, ni POSIX.
- Útiles de XAPI: `GetTickCount`, `QueryPerformanceCounter`, `Sleep`, `CreateThread`,
  `CreateFileA/ReadFile/WriteFile`, `FindFirstFileA`, `CreateDirectoryA`, `OutputDebugStringA`,
  `XInputGetState`, `XGetDevices`. Rutas: disco del juego `D:\`, datos persistentes del título `T:\`.

## Cómo compilar TU código (sin pisar a los demás)
Varios agentes trabajan a la vez en la misma carpeta. **No ejecutes ninja/cmake --build.** Compila solo tus ficheros:

```
powershell -NoProfile -ExecutionPolicy Bypass -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\compile_one.ps1" src\platform\TuFichero_XBOX.cpp src\xbox\otra\cosa.cpp
```
Imprime `OK` o `FAIL` + los errores. Tu criterio de aceptación es que todos tus ficheros den `OK`.
El enlazado final y la prueba en el emulador los hace el coordinador.

## Reglas de trabajo
- Edita **solo** los ficheros que tu tarea dice que son tuyos. Si necesitas cambiar otro, NO lo hagas:
  apúntalo en tu TRASPASO en "(5) siguiente paso".
- No hagas `git commit`, `git checkout`, `git stash`, ni borres ficheros ajenos.
- No toques PC/PS2/Wii: cada cambio en código compartido debe dejar esas plataformas igual
  (usa `#if defined(XBOX_PLATFORM)`).
- Mismo estilo que el código de alrededor (comentarios, nombres, llaves).
- Si algo no se puede hacer bien hoy, deja un stub que compile y funcione "en seco"
  (sin crashear) y documéntalo.

## Cómo entregar
Al terminar, añade AL FINAL de tu fichero de tarea una sección:

```
ESTADO: TERMINADO   (o BLOQUEADO)
# (1) QUE QUEDO HECHO Y FUNCIONANDO   (ficheros, y la salida OK de compile_one)
# (2) QUE QUEDO A MEDIAS
# (3) QUE HAS DESCARTADO Y POR QUE
# (4) QUE SABES QUE NO ES OBVIO
# (5) EL SIGUIENTE PASO CONCRETO     (incluye cambios que necesitas en ficheros de otros)
# (6) QUE NO VERIFICASTE
```

## Nota del coordinador (añadida después de lanzaros)
- Compilar a la vez NO os pisa: `compile_one.ps1` usa una carpeta temporal privada por llamada y
  no toca `build/` ni `bin/`. Solo el coordinador ejecuta ninja y el enlazado.
- Lo que SÍ puede pasar: tu fichero incluye una cabecera que otro agente está editando justo
  ahora (sobre todo `src/platform/PlatformCompat.h` y `ConsoleInputClock.h`, que son de
  agy-plataforma). Si `compile_one` falla por un error DENTRO de un fichero que no es tuyo,
  **no lo arregles**: sigue con lo tuyo, reintenta en unos minutos y, si persiste, apúntalo en
  tu TRASPASO sección (5).
- Estado medido a las 13:xx: quedan 18 .obj que no compilan en todo el juego, y todos están
  repartidos entre vosotros cuatro.
