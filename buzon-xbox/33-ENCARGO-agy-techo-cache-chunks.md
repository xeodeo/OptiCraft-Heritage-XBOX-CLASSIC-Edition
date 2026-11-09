# 33 — ENCARGO para agy: ¿tiene techo la caché de chunks en Xbox?

Rama: **`xbox-opti-30`**, carpeta `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30`.
Lee antes: `buzon-xbox\32-RAM-medida-03-10.md` (los datos medidos) y `00-CONTEXTO.md`.

## Contexto: lo que ya está resuelto, no lo repitas

Se midió en la consola real una partida con cuatro muertes. **Morir NO es el problema**:
cada muerte cuesta 0,7-2,6 MB y casi todo vuelve en menos de un minuto. La sospecha de que
se fugaba un `EntityPlayerSP` por muerte queda **descartada**. No trabajes sobre eso.

Lo que sí se vio: **la memoria libre baja de forma continua mientras se juega normal**,
de 30,6 MB al entrar al mundo a 15,2 MB a los 9 minutos, y los tramos sin ninguna muerte
pierden más que las muertes. En Xbox `freeKB` es exactamente lo que enseña la pantalla de
depuración: `Used % = (64 MB − freeKB) / 64 MB`.

## Lo que hay que averiguar (solo análisis, NO toques código todavía)

### A. ¿La caché de chunks tiene un techo duro, o depende de que la expulsión gane la carrera?

Mira `ChunkProviderLoadOrGenerate` y `ChunkProvider` (`setChunkLoadRadiusFromRenderDistance`,
`setCurrentChunkOver`, la ruta de `unloadAllChunks` / expulsión) y
`PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD` en `src\platform\PlatformConfig.h`.

Responde con números concretos:
1. ¿Cuántos chunks como máximo puede tener vivos a la vez? ¿Hay un límite por número, o solo
   la regla "fuera del radio de descarga **y** sin tocar durante N ticks"?
2. Con la distancia de render que usa la Xbox, ¿cuántas columnas entran en el radio de carga
   y cuántas en el de descarga? ¿Cuánta memoria son (a ~80 KB por columna)?
3. Andando en línea recta a velocidad normal, ¿entran chunks más rápido de lo que salen?
   Si la respuesta es sí, la memoria baja hasta que te paras, y el 90 % que vio el usuario
   se explica **sin ninguna fuga**.
4. ¿Qué pasa cuando no queda memoria? ¿Hay alguna salvaguarda (expulsión forzada por presión
   de memoria) o simplemente falla la reserva?

### B. `lists` solo sube y nunca baja

En el log, `xbox.mem lists=` fue 154 → 929 en 9 minutos, **sin bajar ni una sola vez**, mientras
`listKB` sí oscilaba entre 1,2 y 5,0 MB. Mira `xboxRenderMemoryStats` y quien reserva esas
listas (`src\xbox\...`). Di si `lists` es una marca de agua (se reutilizan, normal) o son
ranuras que se van perdiendo. Si se pierden, di dónde y cuántos KB por ranura.

### C. Texturas: 18 (2,6 MB) → 41 (5,0 MB)

Sube al aparecer mobs nuevos y no vuelve. Confirma si son las pieles de entidad
(`obtainEntitySkin` / `releaseEntitySkin` en `World.cpp`) y si el número está acotado por el
número de tipos de mob (entonces es caché a propósito y está bien) o puede crecer sin fin.

## Qué entregar

Un informe en `buzon-xbox\33-INFORME-agy.md` con:
- la respuesta a A, B y C con **ficheros y líneas concretas**;
- un veredicto claro: *la memoria tiene techo* / *no lo tiene*;
- si no lo tiene, **propón** el cambio más pequeño que lo arregle, pero **no lo apliques**.

## Reglas
- **No cambies nada que esté estable.** Esta build va bien y el usuario está contento con ella.
- **No toques el sonido ni la música** (`00-CONTEXTO.md`, la música es como el original a
  propósito, con guardas de plataforma).
- **No compiles** salvo que te lo pidamos. Si compilas, antes el candado
  `buzon-xbox\BUILD.lock` (`REGLA-BUILD-LOCK.md`), y borrarlo al acabar.
- **No digas que has compilado si no lo has hecho.** Ya pasó dos veces. Si dices que sí, pega
  la fecha del `.xbe`.
- Cita siempre `fichero:línea`. Nada de conclusiones sin la línea que las respalda.
