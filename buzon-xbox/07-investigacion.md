# Investigación de optimización — 24-09-2026

Estado del juego: versión "funcional 13" / commit `5fc5d44` (decoración por pasos, carga del disco pausada, medición del render).
- En la consola (versión 13): ~58-60 FPS, 31-33 MB libres. Quedan tirones de 50-100 ms cada pocos segundos.
- En xemu con `5fc5d44`: 0-5 tirones en 100 segundos. Falta probarla en la Xbox.

Fuentes: 3 agentes en paralelo.
- Revisión del repositorio de OptiJuegos.
- Auditoría del código en zonas no revisadas.
- Investigación en internet sobre cómo se programaba en Xbox, PS2 y GameCube (2000-2006).

"Exacto" = el juego se ve y se juega igual; solo cambia cuándo o qué tan rápido se hace el trabajo.

## 1. Repositorio de Opti: nada nuevo que sirva
- `main` no tiene commits nuevos. Todo lo útil de los PR (#5, 7, 10, 11, 13, 15, 16, 19, 20, 21) ya está en nuestra rama.
- PR #23, abierto: iconos de botones y editor de controles. Es solo interfaz; podría servir más adelante para iconos de mando de Xbox.
- El único fork activo es el de PS2 de TheBrokenWaLL: menús, iconos y edición de mundos, nada de motor.
- Los otros repos de OptiJuegos no son de Minecraft.

## 2. Hallazgos en el código (por impacto)

| # | Qué | Dónde | Arreglo | ¿Exacto? |
|---|---|---|---|---|
| 1 | **El sonido decodifica cada efecto en el hilo principal.** La caché es de 1 MB, pero los sonidos del mundo necesitan ~5 MB, así que pasos y mobs se decodifican una y otra vez. Además lee el Ogg byte a byte con `fgetc` | `SoundManager_XBOX.cpp:101, :183`; `VorbisAssetOpen.h` | Caché de ~6 MB, o guardar los Ogg comprimidos en RAM (~3 MB) y decodificar desde memoria. El hilo de música, a prioridad baja y por trozos | Sí (audio idéntico). **Ojo: pediste no tocar el sonido; necesita tu permiso** |
| 2 | **Iluminación con los ajustes lentos de PC**: ~30 búsquedas de chunk por celda cambiada y sin presupuesto por frame | `PlatformGameTuning.h:446-460`, `MetadataChunkBlock.cpp:39` | `PLATFORM_FAST_LIGHTING_CHUNK_ACCESS 1` (3-5× más rápido). Presupuesto de 2 ms con cola interactiva, como en Wii | Luz final idéntica; en los bordes nuevos tarda unos frames más en asentarse |
| 3 | **Cada búsqueda de chunk calcula un hash de 64 bits** (ticks, líquidos, luz, colisiones) | `ChunkProvider.cpp:333-350` | Comprobar antes el último chunk usado y una caché pequeña | Sí |
| 4 | **`sqrt` por software** (fdlibm): ~1000 ciclos frente a ~50 de `fsqrt`, en 163 sitios (mobs, flechas, vectores, render) | `Math.h:20`, `StrictMathCompat.cpp:23` | Usar `std::sqrt` en Xbox (con precisión de 53 bits es igual de correcto) | Sí |
| 5 | **`floor_double` usa `__ftol2`**, que cambia el modo de redondeo dos veces; 520 sitios | `Arithmetic.h:139` | Truco del "número mágico": ~10 ciclos en vez de ~40 | Sí |
| 6 | **Ticks programados**: copia la lista entera por cada chunk guardado, y cada tick hace 4 reservas de memoria | `World.cpp:3189-3217`, `AnvilChunkLoader.cpp:269` | Recorrer sin copiar, pool de entradas y reservar el búfer de guardado | Sí |
| 7 | **Hojas que se caen**: array de 128 KB (todo el L2) para una ventana de 9³ | `BlockLeaves.cpp:96` | Array de 11³ | Sí |
| 8 | **Agua y lava**: la búsqueda de caída relee las mismas celdas (~320 lecturas por tick) | `BlockFlowing.cpp:118-160` | Memorizar en una rejilla local | Sí |
| 9 | **Texturas por nombre** (`std::map<string>`) en cada mob y cada casilla, cada frame. Los iconos 3D del inventario se re-teselan cada frame | `RenderEngine.h:109`, `RenderItem` | Caché por puntero y `unordered_map`; iconos 3D en búfer | Sí |
| 10 | **Memoria temporal en la IA**: vectores nuevos por mob y por tick, copias en la búsqueda de objetivos | `EntityAITasks.cpp:21`, `World.cpp:5461` | Vectores reutilizados | Sí |

Revisado y descartado: el `Random` ya usa multiplicaciones de 32 bits; el pathfinding ya está acotado; la compresión ya va en otro hilo a nivel 1; la tabla de senos no se puede reducir sin cambiar valores.

## 3. Técnicas de la época (investigación)
1. **Presupuesto por subsistema y nunca "ponerse al día" en un frame** (Fiedler, "Fix your timestep"): como máximo 1 tick por frame, líquidos y ticks de bloques con cola limitada, y la IA de los mobs repartida por id (`(tick + id) % N`).
2. **Nada de memoria dinámica dentro del frame** (EASTL 2007, DICE "Scope Stack"): arenas por frame, pools fijos y vectores reutilizados.
3. **Conversión decimal→entero** (stereopsis): `(int)f` cuesta más de 80 ciclos en x87. Coincide con el hallazgo 5.
4. **Guardado partido**: construir el NBT en el hilo principal y comprimir/escribir en un hilo de baja prioridad. Leer los sectores de región de una vez.
5. **Aprovechar la espera del vsync**: un hilo de baja prioridad que descomprima y decodifique chunks mientras el principal espera. Es lo de Halo 2 / Jak & Daxter.
6. Iluminación por colas con límite (Starlight, 0fps), datos de entidades compactos y detalles del P3 (bloqueos de registros parciales, `prefetchnta`).

## 4. Orden propuesto para la próxima versión (todo exacto)
1. Iluminación rápida (hallazgo 2, el acceso rápido; el presupuesto va aparte).
2. `sqrt` y `floor_double` rápidos (hallazgos 4 y 5).
3. Búsqueda de chunk con atajo (hallazgo 3).
4. Ticks programados sin copias y búfer de guardado reservado (hallazgo 6).
5. Hojas, líquidos, texturas por nombre e IA (hallazgos 7-10).
6. **Si lo autorizas:** la caché de sonido (hallazgo 1). Es probablemente de lo que más ayuda contra los micro tirones al caminar y con mobs.
7. Más adelante: guardado y carga en un hilo de baja prioridad durante el vsync, y máximo 1 tick por frame.

Detalle técnico completo en el plan anterior: `06-plan-optimizacion.md`.
