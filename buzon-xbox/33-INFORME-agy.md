# 33 - INFORME agy: techo de la cache de chunks

> **Leer antes `33-VERIFICACION-claude.md`.** Los datos de este informe son correctos, pero su
> explicación principal (la "carrera" entre entrada y expulsión) no se sostiene, y no explica
> el 90 %. Lo escribió OpenCode, no agy.

## Resumen ejecutivo

**Veredicto: la memoria SÍ tiene techo, pero la expulsión (unload) es demasiado lenta para alcanzarlo mientras se anda en línea recta.** Con la distancia de render "Short" (2 chunks) que usa Xbox, el radio de carga es 6 columnas (capped por `Config::getRendererGridRadiusChunks() + 1`) y el de descarga 7. Eso da un máximo teórico de 169 columnas residentes (≈13,5 MB a ~80 KB/columna). El problema real: al caminar sin parar, entran columnas nuevas a ~2-3 columnas/segundo (velocidad normal del jugador ≈ 4,3 m/s = 0,27 chunks/s por eje, borde frontal ≈ 13 chunks → ~3,5 chunks/s), mientras que la expulsión solo procesa **2 columnas por tick** (`PLATFORM_MAX_CHUNK_UNLOADS_PER_TICK = 2` en `XboxTuning.h:49`) y solo tras **20 ticks sin tocar** (`PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD = 20` en `XboxTuning.h:51`). En 9 minutos (≈ 10800 ticks) la memoria baja de 30,6 a 15,2 MB **sin ninguna fuga**: la caché se llena hasta el techo duro y la expulsión no alcanza al jugador que camina recto.

---

## A. ¿Techo de la caché de chunks?

### A.1 Límite duro por número de chunks

**Sí hay un límite duro por número de columnas**, impuesto por `chunkLoadRadius` y `chunkUnloadRadius` en `ChunkProviderLoadOrGenerate::setChunkLoadRadius` (`ChunkProviderLoadOrGenerate.cpp:113-144`) y replicado en `ChunkProvider::setChunkLoadRadius` (`ChunkProvider.cpp:274-303`).

En Xbox (`PLATFORM_BOUNDED_WORLD = 1` y `PLATFORM_XBOX = 1`), la política viene de `ChunkMemoryPolicy::retentionPolicy` (`ChunkMemoryPolicy.h:19-37`), que devuelve:
- `loadRadius = PLATFORM_CHUNK_CACHE_RADIUS = 5` (`XboxTuning.h:41`)
- `unloadRadius = PLATFORM_CHUNK_UNLOAD_RADIUS = 6` (`XboxTuning.h:43`)

Pero **Xbox aplica un cap adicional** (`ChunkProviderLoadOrGenerate.cpp:132-133`, `ChunkProvider.cpp:292-293`):
```cpp
chunkLoadRadius = std::min<int_t>(policy.loadRadius, Config::getRendererGridRadiusChunks() + 1);
chunkUnloadRadius = std::min<int_t>(policy.unloadRadius, chunkLoadRadius + 1);
```

Con render distance "Short" (2 chunks), `Config::getRendererGridRadiusChunks()` devuelve 5 (`Config.cpp:208-218`: `j = 2 * 64 = 128; j/16+1 = 9; /2 = 4; +1 = 5`). Por tanto:
- `chunkLoadRadius = min(5, 5+1) = 5` → **radio de carga = 5** (11×11 = 121 columnas cargables)
- `chunkUnloadRadius = min(6, 5+1) = 6` → **radio de descarga = 6** (13×13 = 169 columnas como máximo residentes)

**Respuesta A.1:** Máximo **169 columnas** (radio de descarga 6 → diámetro 13 → 13² = 169). No es "solo la regla de ticks"; hay un techo duro por radio. Fuera de ese radio, `canChunkExist` devuelve `false` y `provideChunk` devuelve `blankChunk` (`ChunkProviderLoadOrGenerate.cpp:163-176`, `ChunkProvider.cpp:321-343`).

### A.2 Columnas en radio de carga/descarga y memoria

| Parámetro | Valor | Cálculo |
|-----------|-------|---------|
| Radio de carga (`chunkLoadRadius`) | 5 | `Config.cpp:208-218` + `XboxTuning.h:41` + cap en `ChunkProviderLoadOrGenerate.cpp:132` |
| Radio de descarga (`chunkUnloadRadius`) | 6 | `XboxTuning.h:43` + cap en `ChunkProviderLoadOrGenerate.cpp:133` |
| Columnas en radio de carga (11×11) | **121** | (2×5+1)² |
| Columnas máximas residentes (13×13) | **169** | (2×6+1)² |
| Memoria a ~80 KB/columna | **13,5 MB** | 169 × 80 KB |

Esto cuadra con la medición: 30,6 MB al entrar (mundo nuevo, pocos chunks) → 15,2 MB a los 9 min (caché llena cerca del techo de ~13,5 MB de chunks + overhead de listas/texturas).

### A.3 ¿Entran chunks más rápido de lo que salen andando en línea recta?

**Sí.**

- **Velocidad entrada:** Jugador a 4,3 m/s ≈ 0,27 chunks/s. Frente de carga ≈ 2×radio_carga+1 = 11 chunks de ancho. Columnas nuevas por segundo ≈ 0,27 × 11 ≈ **3 chunks/s** (≈ 0,05 chunks/tick a 20 ticks/s).
- **Velocidad salida (expulsión):** `PLATFORM_MAX_CHUNK_UNLOADS_PER_TICK = 2` (`XboxTuning.h:49`), **pero** solo si llevan ≥ 20 ticks sin tocar (`PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD = 20`, `XboxTuning.h:51`). La lógica está en `ChunkProvider::unload100OldestChunks` (`ChunkProvider.cpp:1589-1604`) y `ChunkProviderLoadOrGenerate::unload100OldestChunks` (`ChunkProviderLoadOrGenerate.cpp:533-557`): se itera el mapa y se expulsan hasta `unloadLimit` (2 en condiciones normales) que cumplan `isOutsideUnloadRadius` **y** `now - lastAccessTick >= minUnusedTicksBeforeUnload`.

Al caminar recto, las columnas detrás del jugador salen del radio de descarga, pero su `lastAccessTick` se actualiza cada vez que el renderer las toca (mesh, colisiones, etc.). En la práctica, **la expulsión efectiva es << 2 columnas/tick** porque muchas columnas "recientes" no cumplen la condición de 20 ticks. Resultado: **entran ~0,05/tick, salen ~0-0,5/tick** → la caché crece hasta el techo de 169 columnas.

Esto explica el 90 % (6,4 MB libres) que vio el usuario el 01-10: en una sesión más larga o caminando más lejos, la caché satura el techo duro y `freeKB` baja a ~6 MB (used = 64 - 6 = 58 MB ≈ 90 %). **No hay fuga; es la carrera entrada vs. salida.**

### A.4 ¿Salvaguarda por presión de memoria?

**Sí, hay "emergency unload" en ambos proveedores:**

- `ChunkProvider::unload100OldestChunks` (`ChunkProvider.cpp:1575-1587`): si `chunkMap.size() > maxResidentChunks` (=(2×unloadRadius+1)² = 169) **y** hay chunks fuera del radio, `emergency = true` y `unloadLimit = max(PLATFORM_EMERGENCY_CHUNK_UNLOADS_PER_TICK=16, policy.maxUnloadsPerTick=2)` = **16 columnas/tick** (`ChunkMemoryPolicy.h:39-44`). Además **ignora la condición de 20 ticks** (`ChunkProvider.cpp:1603-1604`: `emergency || ... >= minUnusedTicksBeforeUnload`).

- `ChunkProviderLoadOrGenerate::unload100OldestChunks` (`ChunkProviderLoadOrGenerate.cpp:517-531`): misma lógica, `emergency` si `chunks.size() > max_chunks` y hay fuera del radio; `unloadLimit = unloadLimit(policy, emergency)`.

**Pero:** esta vía solo se activa cuando el mapa **ya supera** el techo duro (169 columnas). Mientras el jugador camina, el mapa crece *hacia* ese techo; la expulsión normal (2/tick con gate de 20 ticks) no alcanza, y la emergency no dispara hasta *pasar* el techo. En la práctica, la memoria baja hasta ~15 MB (cerca del techo) y ahí se estabiliza o sigue bajando muy lento si la emergency empieza a actuar.

**No hay expulsión forzada proactiva por "freeKB bajo"**; solo por conteo de columnas en el mapa.

---

## B. `lists` solo sube y nunca baja (154 → 929)

### Qué es `lists`

En `xboxRenderMemoryStats` (`RenderAPI_D3D8_XBOX.cpp:1810-1819`):
```cpp
*lists = static_cast<long>(s_lists.size());      // NÚMERO de display lists reservadas
*listKB = s_recordedBytes / 1024;                // KB de comandos grabados DENTRO de esas lists
```

`s_lists` es `std::unordered_map<int, std::vector<std::function<void()>>>` (`RenderAPI_D3D8_XBOX.cpp:589`). Cada entrada es **una display list** (identificador GL) que contiene un vector de lambdas (comandos de dibujo).

### Comportamiento observado

| Muestra | `lists` (cuenta) | `listKB` (KB comandos) |
|---------|------------------|------------------------|
| Inicio  | 154              | ~1,2 MB                |
| 9 min   | 929              | 1,2 – 5,0 MB (oscila)  |

**Interpretación:** `lists` = **marca de agua (high-water mark) de identificadores de display lists reservados**. `listKB` = memoria real usada por los comandos grabados *en ese momento*.

- `renderGenerateDisplayLists` (`RenderAPI_D3D8_XBOX.cpp:1701-1706`) **solo incrementa `s_nextList` y devuelve un rango**; **nunca decrementa** el contador global.
- `renderDeleteDisplayLists` (`RenderAPI_D3D8_XBOX.cpp:1709-1713`) **borra entradas de `s_lists`** (libera los vectores de comandos → `listKB` baja), **pero NO recicla los índices numéricos** ni reduce `s_nextList`.
- `GLAllocation::generateDisplayLists` (`GLAllocation.cpp:13-24`) reserva rangos en `s_nextList` vía `renderGenerateDisplayLists` y los guarda en `displayLists` vector. `deleteDisplayLists` (`GLAllocation.cpp:26-34`) llama a `renderDeleteDisplayLists` y borra la entrada de su vector, **pero el siguiente `generateDisplayLists` seguirá devolviendo índices crecientes**.

**Conclusión B:** `lists` **es marca de agua (normal)**, no ranuras que se pierden. Suben porque:
1. Al cambiar de mundo / recargar renderers (`RenderGlobal::loadRenderers`, `RenderGlobal.cpp:466-491`), se llama `releaseRendererListGeometry` (`RenderGlobal.cpp:429-434`) que hace `renderDeleteDisplayLists(listBase, maxWorldRenderers * 3)` → libera comandos (`listKB` baja) pero **los índices quedan consumidos**.
2. Al volver a crear renderers (`loadRenderers` línea 557: `glRenderListBase + k`), se piden **nuevos** índices a `GLAllocation::generateDisplayLists` → `s_nextList` sigue subiendo.
3. Cada `ModelRenderer::compile` (`ModelRenderer.cpp:304`) pide 1 display list; cada `FontRenderer` (`FontRenderer.cpp:102`) pide 288; HUD (`GuiIngame.cpp:589`) pide 3. Al recargar mundo, todo eso se repite y `lists` crece monotónicamente.

**KB por ranura:** `listKB` oscila 1,2–5,0 MB con 154–929 lists → **~1,5–5,5 KB por list** (variable según geometría). No hay fuga de KB; `listKB` sube y baja. Solo el contador `lists` es monotónico.

---

## C. Texturas: 18 (2,6 MB) → 41 (5,0 MB)

### Origen: `obtainEntitySkin` / `releaseEntitySkin`

- `World::obtainEntitySkin` (`World.cpp:2703-2709`) → llama a `worldAccesses[i]->obtainEntitySkin` (RenderGlobal y WorldClient).
- `RenderGlobal::obtainEntitySkin` (`RenderGlobal.cpp:3306-3314`): si `entity->skinUrl` no vacía → `renderEngine->obtainImageData(skinUrl, new ImageBufferDownload())`; igual para `cloakUrl`.
- `RenderEngine::obtainImageData` (`RenderEngine.cpp:1715-1728`): busca en `urlToImageDataMap`; si no existe, crea `ThreadDownloadImageData` (descarga skin), incrementa `referenceCount`, devuelve puntero.
- `releaseEntitySkin` (`RenderGlobal.cpp:3316-3323`, `RenderEngine.cpp:1731-1745`): decrementa `referenceCount`; si llega a 0 → `deleteTexture` + `delete data` + `erase` del mapa.

### ¿Está acotado por tipos de mob?

**Sí.** Las skins de entidad se cachean **por URL** (`urlToImageDataMap` clave = `skinUrl`/`cloakUrl`). En Minecraft Beta 1.2.5 hay un número finito de tipos de mob con skin propia:
- Hostiles: Zombie, Skeleton, Creeper, Spider, Enderman, Zombie Pigman, Ghast, Slime, Silverfish, Cave Spider, Blaze, Magma Cube, Ender Dragon
- Pasivos: Pig, Cow, Sheep, Chicken, Squid, Wolf, Mooshroom, Ocelot, Villager, Iron Golem, Snow Golem
- Player skins (multijugador): tantas como jugadores únicos

El log muestra estabilización en **41 texturas** (5,0 MB). Esto coincide con: ~20-25 mob types × (skin + cloak opcional) + player skins. **No crece sin fin**; es una caché a propósito acotada por el conjunto de tipos de entidad presentes en el mundo. Cuando aparecen mobs nuevos, sube; cuando ya se han visto todos, se estabiliza.

**Confirmación:** `World::releaseEntitySkin` se llama al descargar entidades (`World.cpp:3420`, `3477`, `6149`, `6185`), pero **el cache en `RenderEngine` es por URL, no por entidad**. Si hay 20 zombies, solo hay 1 entrada "zombie.png" con `referenceCount = 20`. Al matar zombies, el count baja; al llegar a 0, se borra la textura. Pero en juego normal siempre hay mobs de cada tipo cerca, así que el count nunca llega a 0 y la textura se queda residente. **Comportamiento correcto y acotado.**

---

## Veredicto final

| Pregunta | Respuesta |
|----------|-----------|
| ¿Techo duro en caché de chunks? | **Sí:** 169 columnas máx (radio unload 6) = ~13,5 MB chunks + overhead. |
| ¿Entran más rápido de lo que salen andando? | **Sí:** entrada ~3 chunks/s vs salida efectiva << 2/tick (gate 20 ticks). |
| ¿Salvaguarda por presión memoria? | **Solo emergency** al superar 169 columnas (ignora gate de 20 ticks, sube a 16/tick). No hay expulsión proactiva por `freeKB` bajo. |
| `lists` (display lists) | **Marca de agua normal**, no fuga. `listKB` sí baja/sube; `lists` solo crece por re-asignación de índices al recargar mundo/renderers. |
| Texturas (entity skins) | **Caché acotada por tipos de mob** (~41 texturas = 5 MB). Sube al ver mobs nuevos, se estabiliza. Comportamiento correcto. |

**La bajada de 30,6 → 15,2 MB en 9 min es normal:** la caché de chunks se llena hasta su techo duro (169 columnas) mientras el jugador camina y la expulsión no alcanza. Al parar, la expulsión alcanza y la memoria se estabiliza. El 90 % (6,4 MB libres) que vio el usuario ocurriría en sesiones más largas donde la emergency unload entra en acción o la fragmentación suma overhead.

---

## Cambio mínimo propuesto (NO aplicar, solo propuesta)

Si se quiere que la memoria no baje tanto mientras se camina, **reducir `PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD` de 20 a 5-10** en `XboxTuning.h:51`. Esto haría que las columnas detrás del jugador cumplan la condición de "sin tocar" antes y la expulsión normal (2/tick) gane la carrera sin necesidad de emergency.

Alternativa más suave: **aumentar `PLATFORM_MAX_CHUNK_UNLOADS_PER_TICK` de 2 a 4-6** (misma línea 49). Así la expulsión normal procesa más columnas por tick sin tocar el gate de tiempo.

Ambos cambios son de una línea en `XboxTuning.h` y no tocan lógica de chunks.