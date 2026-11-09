# Tarea 05 — Documentación del port y limpieza del log (agy-docs)

Estado actual del proyecto (23-09-2026): el port de Xbox **funciona en la consola real**.
Repositorio: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition`
(rama `xbox-port`). Olvida las notas antiguas de `00-CONTEXTO.md` sobre placeholders: ya no aplican.

## Reglas
- NO uses git. NO compiles con cmake/ninja. Para comprobar que un .cpp compila usa
  `powershell -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\compile_one.ps1" <ruta del .cpp>`.
- Solo toca los ficheros de esta tarea. Temporales en `...\xbox-spike`, nunca dentro del repo.
- Escribe en inglés dentro del repo (el resto de la documentación está en inglés), con el mismo
  tono y formato que el documento existente.

## Parte A — `docs/XBOX_PORT.md`: añadir lo hecho hoy
El documento ya existe; léelo entero. Añade o actualiza (sin borrar lo que hay):

1. **Sección 1 (Status)** y **sección 9 (Known issues)**: el juego va a ~50-60 FPS a 2 chunks en la
   consola real (antes ~17), con ~25 MB libres en partida. 3 chunks todavía llega al límite de
   memoria (hace falta la compactación de vértices, pendiente).
2. **Nueva subsección en la sección 6 "Performance work"** con una tabla antes/después
   (datos medidos en la consola real, del log `xbox.perf`):

   | Cambio | Efecto medido |
   |---|---|
   | Audio SFX en memoria física contigua (`XPhysicalAlloc`, write-combine) | arregla el cuelgue con ruido en hardware real |
   | Filtro de estados D3D redundantes | menos coste por dibujo |
   | Display lists en vertex buffers estáticos (pools compartidos, fences) | 17 → ~43 FPS, pasada opaca 19 → 6,5 ms |
   | Texturas estáticas sin copia en RAM | ~6,7 → ~10,5 MB libres |
   | Caché de chunks según la distancia de render | ~10,5 → ~25 MB libres, ~50-60 FPS |
   | Pathfinding acotado reactivado (300 nodos, 3 por tick) | IA de mobs más barata |
   | Liberar geometría de chunks al salir al menú | ~31 MB libres en el menú tras jugar |
   | Pools de vértices de 512 KB | menos fragmentación |

   Explica en 2-3 frases cada técnica (qué problema había y cómo se resolvió). Los ficheros son
   `src/platform/RenderAPI_D3D8_XBOX.cpp`, `src/platform/audio/SoundManager_XBOX.cpp`,
   `src/net/minecraft/src/ChunkProvider*.cpp`, `src/net/minecraft/src/RenderGlobal.cpp`,
   `src/xbox/XboxTuning.h`, `src/platform/Profiler_XBOX.cpp`.
3. **Sección 4 (Running)**: nuevas opciones en Vídeo: "30 FPS Limit" (espera un vblank extra, se
   quita al instante), "Show Coordinates" (X/Y/Z + orientación, en la zona segura de la TV) y la
   casilla "Dolby Digital" en la pantalla de Música/Sonido. Borrar mundos con X en la lista.
4. **Sección 8 (Debugging)**: el informe `xbox.perf` cada 5 s (fps, present, fases de render,
   ticks, luz, carga/guardado de chunks) y `xbox.mem` (listas, `vbPools`, texturas).
5. **Sección 10 (cómo se hizo)**: añade una "Phase 7: optimization" resumiendo lo anterior en el
   mismo estilo que las fases existentes, incluido el fallo de `vector too long` (límites de
   pathfinding de escritorio = infinito) y el cuelgue de audio en hardware real.

## Parte B — Quitar ruido del log (un cambio pequeño)
En `src/net/minecraft/src/VirtualKeyboard.cpp`, `VirtualKeyboard::notifyFocus` escribe una línea
`keyboard focus field=... focused=... -> active=...` en cada llamada, y la pantalla de crear mundo
la llama cada frame. Haz que solo registre cuando el estado activo del teclado **cambia**
(guarda el `isActive()` anterior al principio de la función y compara al final). No cambies nada
más de la lógica. Compílalo con `compile_one.ps1`.

## Al terminar
Escribe al final de este fichero un bloque:

```
## TRASPASO
ESTADO: TERMINADO
- qué cambiaste (ficheros y secciones)
- cualquier duda
```

## TRASPASO
ESTADO: TERMINADO
- Actualizadas secciones 1 (Status) y 9 (Known issues) con los FPS (~50-60 a 2 chunks) y memoria (~25 MB libres).
- Añadida sección 'Performance work' bajo la sección 6 con la tabla y explicaciones de cada optimización.
- Documentadas nuevas opciones (30 FPS Limit, Show Coordinates, Dolby Digital) en la sección 4.
- Actualizada sección 8 con `xbox.perf` y `xbox.mem`.
- Añadida 'Phase 7: optimization' a la sección 10 (incluido el cuelgue por pathfinding infinito y cuelgues de audio).
- Corregido log en VirtualKeyboard.cpp (notifyFocus) para registrar solo cuando isActive() cambia. El archivo fue compilado exitosamente.
- Ninguna duda pendiente.
