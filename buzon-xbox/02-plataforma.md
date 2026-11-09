# Tarea agy-plataforma — Núcleo de plataforma Xbox (arranque, tiempo, backends pequeños, audio stub)

Lee primero `00-CONTEXTO.md` (mismo buzón).

## Objetivo
Que todo el código de plataforma "pequeño" compile para Xbox y haga lo correcto, tomando como
modelo el port de Wii.

## Qué hay que hacer
1. **Entry point**: `src/xbox/main_xbox.cpp` con `void __cdecl main()` (así lo espera el runtime Xbox;
   NO `int main(argc, argv)`), equivalente a `src/wii/main_wii.cpp`: bootstrap mínimo y
   `Minecraft::start(&username, &auth)`. Si necesitas un bootstrap, créalo en `src/xbox/system/`.
2. **`src/platform/PlatformCompat.h`**: añadir ramas `#elif defined(XBOX_PLATFORM)` a cada
   `#if defined(WII_PLATFORM) ... #elif defined(PS2_PLATFORM) ... #else <SDL>` (ticks en ms con
   `GetTickCount`/`QueryPerformanceCounter`, sleep con `Sleep`, prioridad de hilo, ratón, etc.).
   Ojo: es una cabecera compartida; NO incluyas `XboxXtl.h` ahí — declara lo mínimo tú mismo
   (p.ej. `extern "C" unsigned long __stdcall GetTickCount(void);`) o mueve la implementación a un
   `.cpp` en `src/xbox/`.
3. **`src/platform/ConsoleInputClock.h`**: incluye `ogc/lwp_watchdog.h` para Wii; añade rama Xbox.
4. **`src/net/minecraft/src/Config.cpp`** (usa `SDL_Delay`/`Uint32`), **`GuiScreen.cpp`**
   (`SDL_clipboard.h` -> en Xbox no hay portapapeles: devuelve vacío), **`NetworkManager.cpp`**
   (`threadSleepCondition`/`threadSleepLock` no declarados con `NO_NETWORK` + Xbox): arreglar con
   ramas Xbox que no cambien PC/PS2/Wii.
5. **Backends pequeños** (copia el `_WII.cpp` y adapta; stub honesto si no aplica):
   `Diagnostics_XBOX`, `Profiler_XBOX`, `ClientProfilerBackend_XBOX`, `ClientPlatformPolicy_XBOX`,
   `LegacyControlPromptBackend_XBOX`, `PlatformUserSettings_XBOX`, `GameSettingsBackend_XBOX`,
   `RenderLightingProfile_XBOX`, `TextureResidencyPolicy_XBOX`, `ScreenshotBackend_XBOX` (stub),
   todos en `src/platform/`.
6. **Audio**: `src/platform/audio/SoundManager_XBOX.cpp` — hoy se compila con `NO_SOUND`; haz un
   backend silencioso que cumpla la interfaz (mira `SoundManager_WII.cpp` y cómo se comporta con NO_SOUND).
7. **Tuning de memoria**: la Xbox tiene 64 MB. Crea `src/xbox/XboxTuning.h` basado en
   `src/wii/WiiTuning.h` + `src/wii/tuning/*` (radios de chunks, cachés) y haz que
   `PlatformConfig.h` active `PLATFORM_BOUNDED_WORLD` para Xbox igual que para Wii (mira cómo se
   engancha WiiTuning). Esto sí puedes tocarlo en `PlatformConfig.h`.
8. Revisa las ~108 guardas "solo Wii" (`#ifdef WII_PLATFORM` sin PS2) del código compartido
   (`grep -rn "WII_PLATFORM\|PLATFORM_WII" src --include=*.cpp --include=*.h`, excluyendo
   `src/wii`, `src/ps2`, y los ficheros de render de la tarea agy-render). Añade Xbox SOLO donde sea
   comportamiento de consola genérico (tuning, memoria, UI de mando), no hardware Wii. Lista en el
   TRASPASO las que dejaste sin tocar y por qué.

## Ficheros tuyos
`src/xbox/main_xbox.cpp`, `src/xbox/system/**`, `src/xbox/XboxTuning.h`, `src/xbox/tuning/**`,
`src/platform/PlatformCompat.h`, `src/platform/ConsoleInputClock.h`, `src/platform/PlatformConfig.h`,
los backends listados en el punto 5 y 6, `Config.cpp`, `GuiScreen.cpp`, `NetworkManager.cpp`,
y las guardas "solo Wii" del punto 8 (fuera de render, input y storage).
**No toques**: storage/ficheros (agy-storage), input/lwjgl Keyboard/Mouse (agy-input),
render/Display (agy-render).

## Aceptación
`compile_one.ps1` da `OK` para cada fichero que creaste o tocaste, y para 5 .cpp de
`src/net/minecraft/src/` al azar que incluyan `PlatformCompat.h`.

ESTADO: PAUSADO
# (1) QUE QUEDO HECHO Y FUNCIONANDO
- Análisis detallado de los requisitos y verificación del entorno de compilación `compile_one.ps1` ejecutado contra `src/net/minecraft/src/Config.cpp`.
- Comprobación del mecanismo de arranque en `src/xbox/runtime/XboxEntry.cpp` (que invoca `main()` de tipo `void __cdecl main()`).
- Identificación de las correcciones exactas para:
  * `src/platform/PlatformCompat.h`: adición de ramas `#elif defined(XBOX_PLATFORM)` para ticks (`GetTickCount`), sleep (`Sleep`), y monotonic micros (`QueryPerformanceCounter` / `QueryPerformanceFrequency`).
  * `src/platform/ConsoleInputClock.h`: eliminación de la inclusión errónea de `<ogc/lwp_watchdog.h>` en Xbox agregando la rama `#elif defined(XBOX_PLATFORM)` con `GetTickCount()`.
  * `src/net/minecraft/src/Config.cpp`: uso de `PlatformCompat::delay((uint32_t)ms)` en Xbox evitando `SDL_Delay` y `Uint32`.
  * `src/net/minecraft/src/GuiScreen.cpp`: exclusión de `SDL_clipboard.h` y retorno vacío/stub para portapapeles en Xbox.
- Ningún archivo de código fue modificado antes de la pausa (cero desincronizaciones en el árbol).

# (2) QUE QUEDO A MEDIAS
- La edición física de los archivos de compatibilidad (`PlatformCompat.h`, `ConsoleInputClock.h`, `Config.cpp`, `GuiScreen.cpp`, `NetworkManager.cpp`).
- La creación de `src/xbox/main_xbox.cpp` y el bootstrap en `src/xbox/system/`.
- La implementación de los backends pequeños (`Diagnostics_XBOX`, `Profiler_XBOX`, `ClientProfilerBackend_XBOX`, `ClientPlatformPolicy_XBOX`, `LegacyControlPromptBackend_XBOX`, `PlatformUserSettings_XBOX`, `GameSettingsBackend_XBOX`, `RenderLightingProfile_XBOX`, `TextureResidencyPolicy_XBOX`, `ScreenshotBackend_XBOX`).
- El backend silencioso de audio `src/platform/audio/SoundManager_XBOX.cpp`.
- `src/xbox/XboxTuning.h` y `src/platform/PlatformConfig.h` para `PLATFORM_BOUNDED_WORLD`.
- Revisión de las ~108 guardas `#ifdef WII_PLATFORM`.

# (3) QUE HAS DESCARTADO Y POR QUE
- Incluir `XboxXtl.h` en `PlatformCompat.h`: descartado taxativamente por las reglas del proyecto (cabecera compartida). Se declara externamente `GetTickCount` y `Sleep`, o se encapsula `QueryPerformanceCounter` en un `.cpp` en `src/xbox/system/XboxClock.cpp`.
- Modificar archivos en `src/xbox/runtime/`: descartado por pertenecer al coordinador.

# (4) QUE SABES QUE NO ES OBVIO
- En `ConsoleInputClock.h`, la rama `#if defined(PS2_PLATFORM) ... #else <ogc/lwp_watchdog.h>` causaba que cualquier compilación de Xbox intentara incluir cabeceras de Nintendo Wii al activar `|| defined(XBOX_PLATFORM)` en el encabezado principal sin el `#elif defined(XBOX_PLATFORM)`.
- El tipo `union _LARGE_INTEGER` en `winnt.h` del XDK no tiene guardia de `#ifndef _LARGE_INTEGER_DEFINED`. Declarar un helper `xboxGetMonotonicMicros()` en un archivo `.cpp` propio de Xbox (`src/xbox/system/`) evita cualquier colisión de redefinición entre tipos de la STL moderna y el XDK.
- En Xbox, el punto de entrada esperado por `XboxEntry.cpp` / CRT startup es `void __cdecl main()`, no `int main(int argc, char** argv)`.

# (5) EL SIGUIENTE PASO CONCRETO
1. Aplicar las ediciones en `src/platform/PlatformCompat.h`, `src/platform/ConsoleInputClock.h`, `src/net/minecraft/src/Config.cpp` y `src/net/minecraft/src/GuiScreen.cpp`.
2. Revisar `NetworkManager.cpp` para asegurar las condiciones de compilación con `NO_NETWORK` en Xbox.
3. Crear `src/xbox/system/XboxClock.cpp` (para `xboxGetMonotonicMicros` y utilidades de tiempo) y `src/xbox/main_xbox.cpp`.
4. Implementar los 10 backends pequeños en `src/platform/` adaptados desde sus equivalentes `_WII.cpp`.
5. Implementar el stub de audio `src/platform/audio/SoundManager_XBOX.cpp`.
6. Crear `src/xbox/XboxTuning.h` y configurar `PlatformConfig.h`.
7. Validar cada archivo con `compile_one.ps1`.

# (6) QUE NO VERIFICASTE
- El comportamiento del enlazado final y la ejecución en el emulador xemu (tarea del coordinador).
- La resolución final de `NetworkManager.cpp` bajo `NO_NETWORK`.

