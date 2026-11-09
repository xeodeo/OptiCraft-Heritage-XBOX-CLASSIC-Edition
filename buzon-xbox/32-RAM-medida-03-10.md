# 32 — La RAM al morir: MEDIDO en consola real (03-10)

Sustituye a las sospechas de `30-RAM-al-morir.md`. **Ya hay una partida con cuatro muertes
grabada**: `xbox-spike\netlog.txt` (la anterior está guardada en `netlog.txt.01-10-tarde.txt`).

Build probada: `OptiCraft_exp_720p.xbe` del 01-10 14:36 (experimental, async).
Partida: ~14 min de reloj, ~9 min dentro del mundo (frames 17.701 a 48.601).
El usuario dijo que **el juego iba bien** toda la sesión.

## Primero, una corrección importante

`30-RAM-al-morir.md` decía que el **% de la pantalla de depuración** y el **`freeKB` del log**
no eran comparables. **Sí lo son, y son el mismo número.** En `src\xbox\java\Runtime_xbox.cpp`
tanto `totalMemory()` como `maxMemory()` devuelven `dwTotalPhys` (64 MB), así que:

```
Used % = (64 MB − freeKB) / 64 MB
```

- 90 % usado  = **6,4 MB libres**
- 80 % usado  = 12,8 MB libres
- 75 % usado  = 16,0 MB libres

## Cómo se localizaron las muertes

No hay ninguna línea de log que marque una muerte, pero sí está `xbox.player pos=`.
Al reaparecer el jugador salta al punto de reaparición. Buscando saltos de más de 50 bloques
que acaban en `(-251.5, 70.62, 232.5)` salen **cuatro muertes limpias**, en las líneas
1461, 2366, 3162 y 3432 del netlog. Las tres primeras fueron por caída (la `y` baja de golpe
justo antes).

## Resultado: morir NO es lo que llena la memoria

| | Libre antes | Mínimo justo después | Ya recuperado (~1 min) | Neto |
|---|---|---|---|---|
| Muerte 1 (frame ~26.500) | 22,7 MB | 20,1 MB | 21,0 MB | **−1,7 MB** |
| Muerte 2 (frame ~37.000) | 18,3 MB | 17,6 MB | 18,1 MB | **−0,2 MB** |
| Muerte 3 (frame ~45.400) | 15,7 MB | 14,8 MB | 16,1 MB | **+0,4 MB** |
| Muerte 4 (frame ~48.450) | 15,9 MB | 15,2 MB | 15,3 MB (en menú) | **−0,6 MB** |

Cada muerte da un bajón de 0,7 a 2,6 MB (es el `preloadWorld("Respawning", 16)`, que genera
3x3 columnas de golpe) y **la mayor parte vuelve en menos de un minuto**. No hay escalón fijo
que se acumule. La tercera muerte incluso acabó con **más** memoria libre que antes.

**No es una fuga por muerte.** La pista 1 de `30-RAM-al-morir.md` (el `EntityPlayerSP` viejo
que no se borra en `World.cpp:172`) queda **descartada en la práctica**: una fuga así sería un
escalón igual y permanente en las cuatro, y no lo es.

## Lo que SÍ baja la memoria: jugar normal

La bajada es continua mientras caminas, con muertes o sin ellas:

```
frame 17.701  30,6 MB   <- acaba de entrar al mundo
frame 19.201  25,0 MB
frame 26.401  22,7 MB   <- 2 min andando, sin morir: −2,3 MB
frame 31.201  17,7 MB
frame 41.401  15,9 MB
frame 48.601  15,2 MB   <- mínimo de la sesión (76 % usado)
```

De 30,6 a 15,2 MB en 9 minutos. Los tramos **sin** ninguna muerte pierden más memoria que
cualquiera de las cuatro muertes. El culpable es la caché de chunks llenándose, más dos cachés
que **solo crecen**:

- **Listas de display** (`xbox.mem lists=`): 154 → 929 en la sesión, **nunca baja ni una vez**.
  Los KB en uso (`listKB`) sí oscilan entre 1,2 y 5,0 MB, así que `lists` parece ser el número
  de ranuras reservadas (marca de agua), no lo vivo.
- **Texturas**: 18 (2,6 MB) → 41 (5,0 MB), **+2,5 MB que no vuelven**. Sube cuando aparecen
  mobs nuevos; se parece a las pieles de entidad (`obtainEntitySkin` / `releaseEntitySkin`),
  que probablemente se cachean a propósito. Parece estabilizarse en 41.

## Lo que NO se reprodujo

**No se llegó al 90 %.** El mínimo fue 14,8 MB libres = **77 % usado**. El usuario vio 90 %
(6,4 MB) el 01-10. Con esta partida no se puede decir si aquello fue una sesión más larga,
ir mucho más lejos, o algo distinto.

## Qué queda por mirar (por orden)

1. **¿Tiene techo la caché de chunks?** Es la pregunta de verdad. Si al andar sin parar la
   memoria libre sigue cayendo y la expulsión no la alcanza, con una sesión larga se llega al
   90 % sin que haya ninguna fuga. Hay que mirar el radio de descarga y
   `PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD` frente al ritmo al que entran chunks nuevos.
2. **`lists` que solo sube** (154 → 929). Saber si es marca de agua (normal) o ranuras que se
   pierden.
3. **Partida larga de verdad**, 30-40 min andando lejos y sin morir, para ver si la curva se
   aplana o sigue bajando. Esto es lo que decide si hay un problema o no.
4. Lo de morir se puede dejar: queda **medido y descartado**.

## Para la próxima prueba
- El escuchador se abre con `cd xbox-spike` y `python -u escuchar_log.py`.
- El resumen de memoria sale con el script de análisis (saca una fila por muestra de `xbox.fpu`
  con su `xbox.mem`, y marca las muertes por el salto de posición).
- Las muestras llegan cada 300 frames (~5 s a 60 fps).
