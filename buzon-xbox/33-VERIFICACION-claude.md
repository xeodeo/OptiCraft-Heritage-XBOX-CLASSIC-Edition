# 33 — Verificación del informe de OpenCode (03-10, Claude)

Comprobado línea por línea contra `wt-opti30`. **Los datos son correctos; la explicación
principal no se sostiene.** Es el patrón de siempre: aciertan el QUÉ y fallan el POR QUÉ.

Quien lo hizo fue **OpenCode (Nemotron 3 Ultra Free)**, no agy: agy se quedó sin cuota a los
8 pasos. El informe dice "agy" en el título porque así se llamaba el encargo.

## Lo que está bien (comprobado)

| Afirmación | Comprobado |
|---|---|
| `PLATFORM_CHUNK_CACHE_RADIUS 5` | `XboxTuning.h:41` ✔ |
| `PLATFORM_CHUNK_UNLOAD_RADIUS 6` | `XboxTuning.h:43` ✔ |
| `PLATFORM_MAX_CHUNK_UNLOADS_PER_TICK 2` | `XboxTuning.h:49` ✔ |
| `PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD 20` | `XboxTuning.h:51` ✔ |
| El cap de Xbox sobre el radio | `ChunkProviderLoadOrGenerate.cpp:132-133` ✔ |
| Expulsión de emergencia | `ChunkProvider.cpp:1585-1604` ✔ (`maxResidentChunks = (unloadRadius*2+1)²` = 169; con `emergency` se salta la espera de 20 ticks) |
| `unloadLimit` sube a 16 en emergencia | `ChunkMemoryPolicy.h`, `unloadLimit()` ✔ |
| **Hay techo duro: 169 columnas ≈ 13,5 MB** | ✔ **Esto es lo importante y es correcto** |

## Lo que está mal

**1. La cuenta de `Config.cpp:208-218` está mal, aunque el resultado final salga bien.**
Escribió `j/16+1 = 9; /2 = 4; +1 = 5`. Ese `+1` no está en la función: el return es
`(j / 16 + 1) / 2` = `(8+1)/2` = **4**, no 5. El `+1` lo pone quien llama
(`ChunkProviderLoadOrGenerate.cpp:132`). Da 5 de radio de carga igual, pero por otro camino.

**2. Lo de "entran chunks más rápido de lo que salen" se contradice con sus propios números.**
Dice que entran ~3 columnas/s (0,05 por tick) y que la expulsión hace 2 por tick. Dos por tick
a 20 ticks por segundo son **40 columnas por segundo**: más de **diez veces** lo que entra.
La espera de 20 ticks solo retrasa cada columna **un segundo**, no la bloquea. **No hay ninguna
carrera que se pierda.** Lo que pasa de verdad es más simple: la caché se llena hasta su techo
y ahí se queda.

**3. No explica el 90 %, y dice que sí.** Si el techo son 169 columnas y por encima salta la
expulsión de emergencia, la caché de chunks **no puede** llevar la memoria libre de 15 MB a
6,4 MB. Lo despacha con "en sesiones más largas o la fragmentación suma overhead", que no es
una prueba de nada. **Ese sigue siendo el agujero.**

**4. "`lastAccessTick` se actualiza cada vez que el renderer las toca (mesh, colisiones)" es falso.**
Solo se escribe en las rutas de búsqueda del proveedor de chunks: `ChunkProvider.cpp:385`, `538`,
`618`, `1038` y `ChunkProviderLoadOrGenerate.cpp:216`, `231`, `251`, `268`, `303`. El renderer no
lo toca. Las columnas que quedan detrás dejan de consultarse y cumplen la espera enseguida.

## Un hallazgo de verdad: un comentario del código está obsoleto

`ChunkProviderLoadOrGenerate.cpp:126-131` dice *"At a short render distance this holds 7x7
columns instead of 11x11"*. Con la fórmula actual, a distancia corta salen **11x11 de carga y
13x13 residentes**, no 7x7. El comentario engaña a quien lo lea.

## Qué sabemos ahora de verdad

El techo encaja casi exacto con lo medido:

```
169 columnas x 80 KB  = 13,5 MB   (caché de chunks, techo duro)
texturas de entidad   =  2,5 MB   (18 -> 41, medido)
                        -------
                        16,0 MB   frente a los 15,4 MB que se perdieron de verdad
```

Es decir: **a 15,2 MB libres la caché ya estaba en su techo**, y por eso el log se queda plano.
Eso es comportamiento acotado y normal. **No hay nada que arreglar aquí.**

**Lo que sigue abierto es el 90 %** (6,4 MB libres) que vio el usuario el 01-10. Con un techo de
13,5 MB de chunks, eso no sale de la caché. Falta por saber de dónde salen los otros ~9 MB.

## Qué hacer

1. **Una partida larga de verdad**: 30-40 minutos andando lejos, sin morir. Si la memoria se
   queda plana en 15-16 MB, no hay problema y se cierra el tema. Si sigue bajando, hay algo
   más y entonces sí hay que buscarlo.
2. **NO aplicar los cambios que propone** (bajar la espera de 20 ticks a 5-10, o subir las
   expulsiones de 2 a 4-6 por tick). Arreglan una carrera que los números dicen que no existe,
   y a cambio meten más guardados por tick, que es justo lo que causa los tirones de
   `chunkUnload` (el comentario de `XboxTuning.h:44-47` lo explica: a 8 por tick costaban
   60-110 ms).
3. Arreglar el comentario obsoleto de `ChunkProviderLoadOrGenerate.cpp:126-131` cuando se toque
   ese fichero por otra cosa.
