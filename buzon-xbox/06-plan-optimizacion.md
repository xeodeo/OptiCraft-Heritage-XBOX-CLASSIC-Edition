# Plan: quitar el log al disco, backup 12, push y hoja de ruta de optimización

## Contexto
La versión de las 22:11 del 23-09-2026 va mejor en la Xbox real: Smooth FPS sin espera a la GPU, generación de chunks por pasos y 30 MB libres. Pero quedan micro-lags: 286 frames de más de 45 ms en 5 minutos, unos 50-60 ms de media.

Una causa barata de quitar es el log al disco. En Xbox, `McLog` cierra y reabre `T:\debug.log` en cada línea (`MC_LOG_SYNC_WRITES=1`, `MC_LOG_COMMIT_EVERY=1`, en `src/platform/Log.h`). Eso son varias escrituras al disco duro por segundo en mitad del juego.

Esta noche el usuario pide:
- quitar el log al disco;
- guardar esta versión como backup;
- hacer push;
- dejar la hoja de ruta de optimización con el análisis a fondo (solo optimización: el menú de crafteo y el aldeano quedan fuera);
- compilar y probar en xemu, dejarlo abierto e hibernar el PC.

La hoja de ruta se hace en sesiones siguientes. Esta noche solo se implementa la parte "Esta noche".

---

## Esta noche

1. **Log al disco apagado, con el código conservado** (el usuario prefiere interruptores a borrar código).
   - `src/xbox/main_xbox.cpp`: `McLog::openSessionFile(logDir)` queda bajo `#ifndef XBOX_DISK_LOG / #define XBOX_DISK_LOG 0`.
   - Siguen funcionando el log por red (UDP, `xboxNetLogInit`), el anillo en memoria y la pantalla roja de crash.
   - La línea de arranque pasa a decir `debug.log disabled`.
2. **Diagnósticos temporales de hoy bajo `XBOX_MESH_DIAGNOSTICS 0`**, para que no escriban líneas ni cuesten CPU:
   - `World.cpp`: `s_xboxDirty*` y `xboxTakeWorldDirtyStats`, con cuerpo vacío si está a 0.
   - `PcLegacyMeshScheduler.cpp`: `s_hot` / `noteHotPublish` y las líneas "rebuilt N times".
   - Se quedan activos `xbox.spike`, `xbox.perf`, `xbox.draw`, `xbox.terrain` y el resumen de `xbox.mesh`.
3. **Compilar el release** (`xbox-spike\build_preset.ps1 -Preset xbox-release`) y comprobar que están los dos XBE: `default.xbe` y `OptiCraft_720p.xbe`.
4. **Backup "OptiCraft funcional 12"** en `Desktop\Minecraft en xboox clasico\`:
   - `carpeta-xbox` (copia de `bin\xbox\iso`), el ISO, `.exe` y `.map`;
   - el ISO también en `C:\RetroXeo\roms\xbox\OptiCraft funcional 12.iso`.
5. **Documentación:** en `docs/XBOX_PORT.md`, una tabla de esta ronda (Smooth FPS 10,5 → 1,6 ms en el pase opaco, generación incremental, log al disco apagado) y la hoja de ruta de abajo resumida en "Known issues / next steps".
6. **Commit y push:**
   - `git -c user.name=xeodeo -c user.email=kelopez@seedem.co commit` en `xbox-port`;
   - normalizar a LF cualquier archivo que salga con el diff completo, como pasó con `Minecraft.cpp`;
   - `docs/XBOX_PORT.md` con `git add` explícito; nada de `.xbe`, `.iso` ni `.exe`;
   - `git push xbox xbox-port:main`.
7. **Prueba en xemu:** `autotest.ps1 -Seconds 90`. Comprobar que el mundo se ve bien, que llegan `xbox.perf` y `xbox.spike` y que no aparece `debug.log open`.
8. **Dejar para el usuario:** copiar el ISO a `C:\RetroXeo\roms\xbox\OptiCraft.iso`, abrir **xemu** con él y dejarlo abierto.
9. Escribir el resumen final y **hibernar el PC** con `shutdown /h`.

---

## Hoja de ruta de optimización (sesiones siguientes)

Viene del log de la consola y de 4 análisis: tick del mundo, chunks, render/HUD y documentación pública de Xbox (Halo 2 HD, ClassiCube Xbox, xboxdevwiki).

**Exacto** quiere decir que el juego se ve y se comporta igual.

### Fase 1: medir bien (exacto, primero)
- **1.1 Arreglar el profiler de ticks.** `platformProfileTickPhase` (`Profiler_XBOX.cpp:52`) descarta el nombre y suma las fases anidadas dos veces, así que el `world=` del spike sale el doble.
  - Guardar el tiempo por nombre, sumando solo las fases de primer nivel.
  - Mostrar en `xbox.spike` el top 3 de fases.
  - Añadir timers sin cubrir: `unload100OldestChunks`, autosave, villages y `updateAllRenderers` por cambio de skylight (`World.cpp:4677-4773`).

### Fase 2: tirones de chunks (el mayor impacto)
- **2.1 La carga del disco no debe bloquear.** `prepareChunkInternal` carga del disco de forma síncrona antes de plantearse diferir (`ChunkProvider.cpp:614` frente a `:626`). Por eso `load` llega a 25-67 ms.
  - Añadir una cola de carga, junto a `generationQueue`, que se vacía con el mismo presupuesto.
  - Solo radio ≤1 sigue siendo síncrono.
  - Las peticiones de dependencia del constructor de chunks (`WorldRendererPcLegacy.cpp:332`) pasan a solo encolar.
  - Exacto, salvo en cuándo ocurre cada carga.
- **2.2 El `populate` (decoración) ocurre al publicar, dentro del paso de generación.** Es el `gen=25-35 ms` real (`ChunkProvider.cpp:856`, `:553-580`).
  - Activar `PLATFORM_DEFERRED_POPULATE` + `PLATFORM_INCREMENTAL_POPULATE` con los valores vanilla y `PLATFORM_DEFER_MESH_DURING_POPULATE`.
  - Es equivalente por semilla, no idéntico byte a byte, igual que el orden de carga en vanilla. Es la mayor ganancia individual.
- **2.3 Partir los pasos atómicos que quedan**, todo exacto:
  - `BaseTerrain`: biomas → cada octava de ruido → mezcla por `localX` → relleno por `cellX` → superficie por fila (`ChunkProviderGenerate.cpp:353-376`). La tarea debe tener su propio `Random` y sus buffers.
  - `BuildChunk` por sección (8 pasos) y `Skylight` por columnas (`Chunk::generateSkylightMap(columnMask)`).
  - `STRUCTURE_SOURCE_COLUMNS_PER_STEP` de 289 a unos 32.
- **2.4 Flags exactos:** `PLATFORM_BATCH_INITIAL_SKYLIGHT_RENDER_UPDATES` (quita ~7000 marcas de render por chunk) y `PLATFORM_BULK_GENERATED_CHUNK_IMPORT`.
- **2.5 Guardado al descargar**: repartirlo en 1-2 chunks por tick en vez de 8 (`ChunkProvider.cpp:1048-1064`). Exacto.
- **2.6 Carga más barata** (exacto):
  - usar `takeByteArray` también en mundos escribibles (`AnvilChunkLoader.cpp:448`);
  - leer la cabecera de región de 8 KB de una vez, no de 4 en 4 bytes (`RegionFile.cpp:238-300`).
- **2.7 (Más adelante) Hilo de carga de baja prioridad** que haga la lectura, el inflate y la decodificación durante las esperas de vsync/present. Es la técnica de Halo 2 y de ClassiCube, y la ruta de trabajo de McRegion ya existe (`ChunkGenerationScheduler.cpp:278-313`).

### Fase 3: tick del mundo (picos de 35-45 ms)
- **3.1 Programador de ticks de PC Legacy** en Xbox (`PLATFORM_PC_LEGACY || PLATFORM_XBOX`, `World.cpp:3133, 3162, 3187, 3248, 3303, 4749, 5265`). Evita copiar toda la lista pendiente por cada chunk guardado. Casi exacto: solo cambia el orden de empates tras recargar.
- **3.2 Menos asignaciones de memoria** (exacto):
  - un búfer reutilizable en `getEntitiesWithinAABB` (`World.cpp:5433`);
  - un mapa id→entidad en `getEntityByID` (`:5478`);
  - `removeEntityFromLoadedChunks` que mire solo en el chunk de la entidad;
  - quitar el borrado O(n²) de `tileEntitiesToAdd` (`:3530`).
- **3.3 Evitar los dos recorridos de `chunkMap` por tick** en la expulsión de chunks, salvo que el jugador cambie de chunk o toque descargar (`ChunkProvider.cpp:1478, 1491`). Exacto.
- **3.4 Spawn de mobs:** radio 8 = 289 chunks, y se construye y destruye un mob completo por cada intento fallido (`SpawnerAnimals.cpp:257, 329-355`).
  - Exacto: comprobar el hueco antes de construir el mob.
  - No exacto (preguntar al usuario): radio 4-5 o intervalo de 2 ticks.

### Fase 4: HUD y render (exacto)
- **4.1 Fallo de la fuente:** `FontRenderer::getCharIndex` → `String::indexOfUtf16Unit` reconstruye en el heap la tabla de ~220 caracteres **por cada letra** (`FontRenderer.cpp:775`, `String.cpp:323`). Sustituirla por una tabla fija. Probablemente es la mayor parte de los 2,4 ms del HUD.
- **4.2 Menos texto temporal:** `formatCodes` estático y sin `toUtf16` por cadena (`FontRenderer.cpp:372-373`). Sombra y texto en un solo lote.
- **4.3 Barras de vida y comida en una sola llamada de dibujo:** hoy son 40-52 `DrawVerticesUP` por frame (`GuiIngame.cpp:441-547`). La captura ya existe para PS2 (`:710-744`).
- **4.4 Modelos de mobs:**
  - una llamada de dibujo por modelo en lugar de una por cara (`TexturedQuad.cpp:107`, `ModelBox.cpp:106`);
  - precompilar modelos y precargar texturas de mobs al entrar al mundo, para quitar los picos de 7-35 ms de la primera vez que se ve un mob (`ModelRenderer.cpp:189`).
- **4.5 Cielo:** 169 llamadas de dibujo → 1 (`RenderGlobal.cpp:235-252`). Además, un camino con búfer de vértices para mallas sin color y sin compactar (factor de textura).
- **4.6 Nubes grabadas una vez** y movidas con matrices (`RenderGlobal.cpp:2001`). Casi exacto.
- **4.7 Backend** (`RenderAPI_D3D8_XBOX.cpp`):
  - salida rápida en `applyTextureStage` si nada cambió;
  - quitar los `s_textures[...]` que no se usan en `renderBindTexture`;
  - timers `rdtsc` solo con el profiling activado;
  - `SetRenderStateNotInline`.
- **4.8 Texturas animadas:** subir solo los tiles cambiados (`XGSwizzleRect` con `pRect/pPoint`, esta vez bien probado; antes falló), o doble textura para no esperar a la GPU.
- **4.9 Partículas:** no calcular un brillo que la Xbox descarta (`EffectRenderer.cpp:108, 131`). Caché por columna para la lluvia y la nieve (`EntityRenderer.cpp:2110`).

### Fase 5: hardware (según la investigación)
- **5.1 Triple buffer** (`BackBufferCount=2`): en Halo 2 HD quitó esperas en el swap y dio ~+10 % de FPS. Cuesta ~1,2 MB a 480p y 3,5 MB a 720p.
- **5.2 `_mm_prefetch`** en los bucles del mallado y la generación, y escrituras en streaming (`movntps`) al copiar vértices a memoria write-combined (ClassiCube Xbox). Medirlo.
- **5.3 Conversión float→int con SSE1** (`_mm_cvttss_si32`) en `floor_double` y en las conversiones calientes. Exacto solo si se respeta el redondeo: verificar.
- **5.4 Descartado:** `/GL`/LTCG (el enlazador del XDK no lo admite) y `/fp:fast` (cambia la generación del mundo).

### Fuera de este plan (a petición del usuario)
Menú de crafteo estilo consola, el aldeano con partes negras, la zona pregenerada como función, y la calidad (MSAA / anti-parpadeo / mipmaps).

---

## Verificación de esta noche
- La compilación termina con "sucessfully created OptiCraft.iso" y hay dos XBE con la misma hora.
- Piloto automático en xemu: el mundo se ve bien, llegan `xbox.perf` y `xbox.spike` y no aparece `debug.log open`.
- Existen `OptiCraft funcional 12\` y su ISO en RetroXeo.
- `git push` muestra `xbox-port -> main`.
- xemu queda abierto con la versión nueva y el PC se hiberna al final.
