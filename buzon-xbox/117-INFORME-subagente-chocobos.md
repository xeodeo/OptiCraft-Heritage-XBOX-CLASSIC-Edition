# 117. Informe subagente: ChocoCraft igual que el original (montar, correr, volar, pose, nombres, cría)

Commits: **1.2.5 `1bcebc9d`** (`opti30-choco2`) · **1.5.2 `00ff457d`** (`port152-choco2`). Compilan normal y exp; SSE2 = 13 (1.2.5 exp da 14: el extra es `GuiTextField::textboxKeyTyped`, fichero no tocado).

Ramas: **1.2.5** `wt-125-choco` → `opti30-choco2` (desde `opti30-erluza`) · **1.5.2** `wt-152-choco` → `port152-choco2` (desde `port152-int`). Commits al final.

## Qué se cambió (en las dos versiones, el mismo código)

### 1. Controles de montar, tal como el original (ChocoCraft 4.1.5 `EntityChocoboRideable.moveEntity`)
En el original: **salto** = subir/volar, **agacharse** = bajar (y en 1.7.10 agacharse desmonta). Adaptado al mando:
| Botón | Amarillo/Verde/Azul | Blanco/Negro | Dorado/Rosa/Rojo/Morado |
|---|---|---|---|
| **A** (mantener) | saltar | salto alto (+0,4 una vez por salto) | **volar y subir** (velocidad vertical 0,5) |
| soltar A en el aire | planea (cae a ~3,5 bloques/s, como el original: `motionY = -0.1`) | igual | planea igual |
| **L3** en el aire | — | — | **bajar rápido** (`motionY = -0.5`) |
| **L3** en el suelo | **desmontar** | desmontar | desmontar |
| L3 bajo el agua (solo Azul) | bucear | | |
| Stick izq. | avanza; de lado a media velocidad (`strafe*0.5`, como el original) | | |

- L3 solo desmonta con una **pulsación nueva**: si L3 venía pulsado (o activado con "Agacharse: Alternar") al montar, o se usó para bajar en el aire, hay que soltarlo y volver a pulsarlo. Así al aterrizar con L3 pulsado no te tira.
- **Ayuda**: al montar sale en el chat la línea de controles de ese color (p. ej. «Montado: mantén A para volar y subir, suelta para planear, L3 para bajar; en el suelo L3 desmonta»). En el menú del chocobo (las dos interfaces, Java y Legacy) se ve una línea corta con los controles del color. Las habilidades de cada color en el menú se corrigieron (Verde ya no "salta alto", etc.).

### 2. «Montar» y al rato te bajaba solo → arreglado
Causa: el menú se abre con agacharse + LT. Con "Agacharse: Alternar" el agacharse seguía activado al elegir «Montar», y a los 12 ticks de gracia que había puesto agy el chocobo lo tomaba como «desmontar». Ahora:
- `mountChocobo` y `dismountChocobo` limpian el estado de alternar (`MovementInputFromOptions::clearSneakToggle()`, método nuevo de una línea).
- El desmonte necesita una pulsación nueva de L3 (ver arriba). Se quitó la «gracia» de 12 ticks.
- Además, como en 1.7.10 (`Entity.canRiderInteract`), el jinete ya **no puede apuntar a su propio chocobo** (`EntityRenderer::getMouseOver` lo salta): antes, en 1.2.5/1.5.2, cualquier LT mirando hacia abajo «re-montaba» = desmontaba (y RT le pegaba). Por defecto `canRiderInteract()` es true, así cerdos, barcas y vagonetas siguen igual que en Java (se bajan con LT sobre ellos).

### 3. El chocobo no corría → física del original
Se sustituyó el movimiento inventado de agy por el del original (`EntityChocobo.moveEntityWithHeading` + `getAIMoveSpeed` + `updateEntityActionState`):
- En suelo, velocidad = `landSpeedFactor/100` de `Constants`: Amarillo 0,20 · Verde/Azul 0,27 · Blanco 0,35 · Negro/Morado 0,40 · Dorado 0,50 · Rosa/Rojo 0,55; tope horizontal 0,375 por tick como el original.
- En el aire, los voladores usan `airSpeedFactor` (Dorado/Morado 0,55, Rosa/Rojo 0,60).
- Medido en xemu: **Amarillo 8,8 bloques/s** (andar = 4,3, correr = 5,6), **Dorado volando 18,5 bloques/s**, subida 16,5 bloques en 2 s, planeo 3,5 bloques/s. Son exactamente los números del original.
- Otras correcciones de habilidades contra el original: salto alto solo **Blanco y Negro** (el Verde no); respirar bajo el agua solo **Azul**; el **Negro sí recibe daño de caída** (solo los voladores no); los que cruzan agua flotan con +0,04 como el original. Se quitó la caída lenta de los chocobos salvajes (en el original solo era visual en el cliente).

### 4. Brazo del jinete partido → arreglado
- **Primera persona**: el modelo del jugador guardaba `isRiding` del último render en 3ª persona, y el brazo en primera persona salía girado 36° y cortado (el error clásico de monturas). `RenderPlayer::drawFirstPersonHand` ahora pinta el brazo con `isRiding/isSneak/heldItem` a cero.
- **Tercera persona**: el cuerpo del jinete sigue al cuerpo de la montura y la cabeza gira como mucho 85° (igual que `RendererLivingEntity` de 1.7.10), así las piernas y los brazos quedan alineados con el chocobo. Encima de una montura no se pone la pose de agachado (L3 = bajar al volar). Vale también para el modelo 1.8 de skins 3D (las capas siguen copiando la pose).
- Capturas: `117-capturas/01_125_montado_3p_frente.png`, `02_125_montado_3p_espalda.png`, `03_125_montado_1p_brazo.png`.

### 5. Nombres «tile.cho…» → arreglados
Era el **bloque de Paja** (`tile.chocoboStraw`, con icono de semilla de trigo porque usaba la casilla 88 de terrain.png) y el item del **Tallo de Gysahl** (`tile.gysahlGreens`). Ahora tienen item propio (`ItemChocoBlock`) con nombre en inglés del `.lang` original y en español, y su icono plano original (`blocks/chocoboStraw.png`, `blocks/gysahl_stem05.png`). `RenderItem`/`ItemRenderer` dibujan el icono plano si un item de bloque trae icono propio. En el mundo la paja usa ahora el trigo maduro teñido de paja (casilla 95) en vez del brote.
Nombres en inglés alineados con `en_US.lang` original: «Chocobo Saddle Bags», «Gysahl Raw Pickles», «Gysahl Green», «Straw», «Gysahl Stem». En español: Paja, Tallo de Gysahl, Verdura de Gysahl, Semillas de Gysahl, Gysahl amorosa/dorada/rosa/roja, Pastel de Gysahl, Encurtido de Gysahl (crudo), Alforjas/Mochila/Silla/Pluma/Silbato de Chocobo, etc.
La **Verdura de Gysahl ya no se come** (en el original no es comida). En 1.5.2 pasa de la pestaña Comida a Varios.

### 6. La cría (chicobo)
Revisada contra el original: la textura es **la original bien aplicada** (64x32; el dibujo solo ocupa 16x15 y las UV de cabeza 3x3x3 en 0,0, cuerpo 4x4x4 en 0,6 y patas en 12,0 coinciden píxel a píxel; escala 1,32 como `RenderChicobo`). El chicobo del original es así de simple: **no se cambió la textura**. Sí se dejaron como el original dos detalles de animación: la cabeza cabecea con el signo del original y la pata izquierda gira en Y (es un error del original, se conserva para que se vea igual). Las alas del adulto se abren solo con |motionY| > 0,1, como el original.

### 7. Otras cosas del original añadidas
- **Cuerda (hilo)** sobre un chocobo domado: alterna Seguir/Quieto (`onSilkUse`).

## Probado
- Compila: 1.2.5 y 1.5.2, normal y exp (ver respuesta final); «SSE2 left in» = 13.
- xemu (1.2.5, build de prueba con autopiloto y una sonda temporal, ya quitada; log en `117-capturas/probe_125_flat.log`), mundo plano:
  - montar con agacharse pulsado: sigue montado a los 3 s ✅
  - Amarillo 8,81 bloques/s ✅ · L3 nuevo en el suelo desmonta ✅
  - Dorado: sube 16,5 bloques en 2 s ✅, 18,5 bloques/s volando ✅, planeo 3,5 bloques/s ✅, baja con L3 y aterriza **sin desmontar** y sin daño ✅, luego L3 nuevo desmonta ✅
  - pose montado 3ª persona (frente/espalda) y brazo en 1ª persona ✅ (capturas)
- No probado: 1.5.2 en xemu (el código del chocobo y de render es idéntico línea a línea, solo cambian comentarios); pantalla partida con P2 montado; consola real; nombres/iconos de Paja en pantalla (necesitan el assets.pak con las texturas de 113).

## assets.pak
No hay texturas ni sonidos nuevos: se usan las de `scripts/chococraft/` que ya metió el encargo 113 (`blocks/chocoboStraw.png`, `blocks/gysahl_stem05.png`). Si el assets.pak del lanzador ya está parcheado con `scripts/chococraft/patch_pak.py`, no hay que hacer nada más; si no, parchear como en 113. Sin el pak parcheado, la Paja sale como bloque 3D (fallback) pero con nombre bien.

## Diferencias que quedan con ChocoCraft 4.1.5
1. **Chocopedia** (libro con nombre/dueño/ocultar nombre/seguir) no existe; en su lugar está el menú del chocobo (agacharse + LT) con renombrar, alforjas, modo y montar. El cambio de dueño no existe.
2. **Chocodisfraz** (armadura de plumas, 4 piezas) no portado.
3. La **Verdura de Gysahl** es un item, no un arbusto colocable; y no hay **generación de Gysahl en el mundo** (cambiaría el terreno de las semillas). En su lugar aparecen Gysahl salvajes cerca del jugador mientras juegas.
4. **Aparición de salvajes**: el original los genera en grupos por bioma (Morado en el Nether); aquí solo Amarillos de uno en uno cerca del jugador (máx. 6). No hay hambre de chocobo (`isHungry/feeding`) ni el canje Libro y pluma → Chocopedia.
5. En el original **agacharse desmonta siempre** (también volando, y caes); aquí L3 en el aire baja y solo desmonta en el suelo (para no matarte con el mando).
6. El chocobo montado **no recibe daño de caída** aquí (en el original sí); el jinete no recibe en ninguno de los dos.
7. Como en el original, **los factores de agua no se notan** (el original los guarda pero el agua usa siempre 0,02): con chocobo, nadar es lento; el Azul solo se distingue por respirar bajo el agua y bucear.
8. La Paja en el mundo usa una casilla de terrain.png de vanilla (trigo maduro teñido), no su textura propia (los bloques no tienen texturas sueltas en este motor).
9. Visión nocturna del Negro dura 11 s y se renueva (en el original 5 s, parpadea); se mantuvo lo de agy.
10. Velocidades altas (Dorado/Rosa/Rojo ~18 bloques/s) son las del original; en consola pueden adelantarse a la carga de chunks. Si da problemas, se puede bajar solo en Xbox.
