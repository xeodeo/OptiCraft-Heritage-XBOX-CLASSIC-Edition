# Tarea agy-storage — Ficheros, recursos y guardado en Xbox

Lee primero `00-CONTEXTO.md` (mismo buzón).

## Objetivo
Que el juego lea sus datos del disco y guarde mundos/opciones en la Xbox.

## Modelo de rutas en Xbox
- Datos del juego (texturas, sonidos, recursos): dentro de la ISO, se ven como **`D:\data\...`**
  (el coordinador copia `data/assets` y `data/resources` a la raíz de la ISO en `data/`).
- Guardado persistente del título (mundos, options.txt, stats): **`T:\`** (XAPI lo monta por título).
  Crea subcarpetas con `CreateDirectoryA`. Si `T:` fallara, usa `Z:\` como temporal y documéntalo.
- Separador `\`. No existe directorio "actual": resuelve todo a rutas absolutas.
- En Xbox NO hay `<dirent.h>` ni POSIX: enumera con `FindFirstFileA`/`FindNextFileA`/`FindClose`.
- El shim del runtime ya cubre `fopen`/`fread`/`_mkdir` del UCRT sobre XAPI (el UCRT llama a
  `CreateFileW` y compañía; están implementados). Puedes usar stdio normal o XAPI directo.

## Qué hay que hacer
1. `src/platform/storage/StorageBackend_XBOX.cpp` (modelo: `StorageBackend_WII.cpp` y el de PS2).
2. `src/platform/storage/PosixFileSystem.cpp` falla: `No se puede abrir 'dirent.h'`. Añade una
   implementación Xbox (rama `XBOX_PLATFORM`, o un `src/platform/storage/XboxFileSystem_XBOX.cpp`
   si la estructura lo permite limpiamente) usando FindFirstFileA.
3. `src/platform/storage/AssetPak.cpp`: `memalign` no existe -> en Xbox usa `_aligned_malloc`/
   `_aligned_free` (rama Xbox).
4. `src/platform/Resources_XBOX.cpp` (modelo `Resources_WII.cpp`): raíz de recursos `D:\data`.
5. Java compat: el Wii tiene `src/wii/java/File_wii.cpp` y `Runtime_wii.cpp`. Mira si
   `src/java/File*.{h,cpp}` necesita implementación por plataforma; si sí, crea
   `src/xbox/java/File_xbox.cpp` (y `Runtime_xbox.cpp` si hace falta: memoria libre con
   `GlobalMemoryStatus` de XAPI) — ojo, estos incluyen XboxXtl.h, así que van en `src/xbox/`.
6. Revisa guardas de almacenamiento en `src/net/minecraft/src/` (RegionFileCache, SaveConverter*,
   SaveOldDir, SaveHandler, ISaveFormat...) que incluyen `<direct.h>`/`<windows.h>` o eligen rutas
   por plataforma, y dales el comportamiento Xbox correcto.

## Ficheros tuyos
`src/platform/storage/**`, `src/platform/Resources_XBOX.cpp`, `src/xbox/java/**`, `src/xbox/storage/**`,
y los ficheros de guardado/regiones de `src/net/minecraft/src/` del punto 6.
**No toques** `PlatformCompat.h`/`PlatformConfig.h` (agy-plataforma), render ni input.

## Aceptación
`compile_one.ps1` da `OK` en todos los ficheros que creaste/tocaste. En el TRASPASO, explica el
árbol de rutas final (dónde lee y dónde escribe cada cosa).

ESTADO: PAUSADO

# (1) QUE QUEDO HECHO Y FUNCIONANDO
- Fase de exploración y diagnóstico completada:
  - Verificada la ejecución de `compile_one.ps1` contra `src/platform/storage/PosixFileSystem.cpp`. Confirmado el fallo fatal `C1083: No se puede abrir el archivo incluir: 'dirent.h'` debido a las inclusiones POSIX (`<dirent.h>`, `<unistd.h>`).
  - Inspeccionada la estructura existente de backends en `src/platform/storage/` (`StorageBackend_WII.cpp`, `StorageBackend_PC.cpp`, `StorageBackend_PS2.cpp`, `PosixFileSystem.h`, `PosixFileSystem.cpp`).
  - Inspeccionado `src/platform/Resources_WII.cpp` y el árbol `src/wii/java/` (`File_wii.cpp`, `Runtime_wii.cpp`) para replicar su diseño en Xbox.
  - Ningún fichero de código fuente fue modificado antes de la pausa (cero regresiones o conflictos con otros agentes).

# (2) QUE QUEDO A MEDIAS
- Implementación de `src/platform/storage/StorageBackend_XBOX.cpp` (actualmente placeholder).
- Reemplazo/adaptación de `src/platform/storage/PosixFileSystem.cpp` para Xbox o desacoplamiento hacia backend Win32/XAPI (`FindFirstFileA`/`FindNextFileA`/`FindClose`).
- Modificación de `src/platform/storage/AssetPak.cpp` para cambiar `memalign` por `_aligned_malloc`/`_aligned_free` bajo `XBOX_PLATFORM`.
- Creación de `src/platform/Resources_XBOX.cpp` resolviendo rutas a `D:\data\...`.
- Implementación de `src/xbox/java/File_xbox.cpp` y `Runtime_xbox.cpp`.
- Revisión de guardas en `src/net/minecraft/src/` (RegionFileCache, SaveConverter*, SaveOldDir, SaveHandler, ISaveFormat...).

# (3) QUE HAS DESCARTADO Y POR QUE
- Se descartó incluir `XboxXtl.h` directamente dentro de `src/platform/storage/PosixFileSystem.cpp` porque las reglas de 00-CONTEXTO prohíben `XboxXtl.h` fuera de `src/xbox/...` o ficheros `*_XBOX.cpp`. La solución limpia para Xbox requiere o bien aislar la implementación de sistema de archivos en `XboxFileSystem_XBOX.cpp` / `StorageBackend_XBOX.cpp`, o excluir `PosixFileSystem.cpp` del build de Xbox si `StorageBackend_XBOX.cpp` cubre la API de almacenamiento.

# (4) QUE SABES QUE NO ES OBVIO
- La regla de cabeceras es estricta: `XboxXtl.h` solo se puede incluir en ficheros bajo `src/xbox/...` o que terminen en `*_XBOX.cpp`. Cualquier abstracción que use XAPI (`FindFirstFileA`, `CreateDirectoryA`, `GlobalMemoryStatus`) debe residir en ficheros con ese sufijo o ruta.
- `PosixFileSystem.cpp` tiene la guarda `#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)`. Si se quita `|| defined(XBOX_PLATFORM)` de allí, el fichero queda vacío para Xbox y no falla por `dirent.h`, permitiendo que `StorageBackend_XBOX.cpp` (que sí puede incluir `XboxXtl.h`) implemente las operaciones directamente con APIs de Win32/XAPI o stdio.
- En Xbox, el UCRT y el shim del runtime ya mapean `fopen`/`fread`/`_mkdir`, por lo que las operaciones estándar de lectura/escritura funcionan directamente; la enumeración de directorios es lo único que requiere `FindFirstFileA`/`FindNextFileA`/`FindClose`.

# (5) EL SIGUIENTE PASO CONCRETO
1. Retirar `defined(XBOX_PLATFORM)` de `src/platform/storage/PosixFileSystem.cpp` (o condicionar sus includes de `dirent.h`/`unistd.h` para que no rompa en Xbox).
2. Implementar `src/platform/storage/StorageBackend_XBOX.cpp` usando `FindFirstFileA` / `FindNextFileA` / `CreateDirectoryA` y stdio / XAPI.
3. Actualizar `src/platform/storage/AssetPak.cpp` con `_aligned_malloc`/`_aligned_free` para `XBOX_PLATFORM`.
4. Implementar `src/platform/Resources_XBOX.cpp` apuntando a `D:\data`.
5. Implementar `src/xbox/java/File_xbox.cpp` y `Runtime_xbox.cpp` si `src/java/File.cpp` lo requiere.
6. Revisar y adaptar las rutas y headers (`direct.h`) en `src/net/minecraft/src/` (SaveHandler, etc.) a `T:\` para guardados persistentes.

# (6) QUE NO VERIFICASTE
- No se llegó a compilar `StorageBackend_XBOX.cpp`, `AssetPak.cpp` ni `Resources_XBOX.cpp` con las modificaciones finales en `compile_one.ps1`.

