# 90. Encargo agy: partículas verdes del cofre, cursor en Logros, niebla del Nether 1.5.2 (20-11)

Mismas ramas: **1.5.2** `port152-erluza` (`wt-152-rend`), **1.2.5** `opti30-erluza` (`wt-opti30`). Sin reset, sin push,
sin copiar al lanzador. Mismas reglas de siempre (BUILD.lock, 60 fps, 480p/720p, creativo no se toca, terreno igual).

## Lo que dijo el usuario probando 87+88+89 en la consola real (20-11)
Bien: zona segura («perfecto»), niebla del Nether, cielo/niebla/sol/luna de 1.2.5, agachado+bloque sobre cofre,
cruceta, controles tras Restablecer, agacharse con interruptor, mantener A, Wither, skins, TNT, pistones, redstone.

## 1. Partículas al romper un cofre salen VERDES («wtf»)
**ACTUALIZACIÓN DEL USUARIO (20-11): solo pasa con el paquete de texturas LCE / «Legacy» activado.** Con las texturas
por defecto las partículas del cofre se ven bien. Con el pack LCE tampoco se aplica bien el **fuego**. Así que es el pack
LCE: su `terrain.png` (o cómo lo cargamos/convertimos) no coloca en los mismos índices las casillas que usan las
partículas del cofre (índice de textura del cofre, Java usa la de tablones/`planks` o la del propio bloque) y las del
fuego (Java 1.2.5/1.5.2 anima el fuego con `TextureFlamesFX` sobre las casillas 31/47 de terrain.png; en 1.5.2 son
iconos `fire_0`/`fire_1`). Compara el atlas LCE con el de Java casilla por casilla para esas texturas y arréglalo (en el
pack o en el mapeo) sin romper el pack por defecto. Mira si hay más casillas desplazadas (otros bloques, partículas).
Lo de abajo queda como contexto:
- Al romper un cofre, las partículas de rotura tienen textura/color verde en lugar de madera. Mira las dos versiones.
- Sospechas: el índice de textura que usa `EffectRenderer::addBlockDestroyEffects`/`EntityDiggingFX` para un bloque
  con render especial (`renderType 22`, el cofre se dibuja con el modelo del TileEntity, no con terrain.png); el
  `blockIndexInTexture` del cofre; el color multiplicador (`colorMultiplier` del bioma, como hierba/hojas) aplicado a un
  bloque que no debe teñirse; o algo que trajimos de erluza (Pocket/`c2b468f` partículas, `PcLegacyBlockRenderInfo`).
  Compara con Java (`EntityDiggingFX`, `Block.chest` 1.2.5/1.5.2 en `mcp-diff`). Revisa también cofre trampa, de
  ender, y otros bloques con modelo especial (cama, cartel, puerta) por si les pasa lo mismo.

## 2. Logros: no se ve el cursor encima de un logro (las dos versiones)
- En la pantalla de Logros, al poner el cursor encima de un logro el cursor desaparece (si «le atina» sí sale la
  información). Desplazar el mapa va perfecto. Probablemente el cursor/puntero de consola se dibuja ANTES que el
  logro/tooltip o queda recortado por el scissor de D3D que pusimos en el encargo 83 (mapa recortado al panel). Que el
  cursor se dibuje siempre encima de todo, también sobre el tooltip.

## 3. Niebla del Nether: en 1.2.5 se ve mejor que en 1.5.2
**CORRECCIÓN DEL USUARIO (20-11, la última vale):** la niebla del Nether de la **1.2.5 está genial: NO la toques**. La
1.5.2 tiene que verse **exactamente igual que la 1.2.5**. **Sin opción nueva.** Busca qué difiere en 1.5.2 (color,
densidad, distancias, cielo/OptiFine) y cópialo de la 1.2.5.
- El código de `setupFog` del encargo 88 es igual en las dos, pero el usuario ve mejor la 1.2.5. Busca qué difiere en
  1.5.2 (color de niebla `getFogColor`/`calcSunriseSunsetColors`, `fogColor` del proveedor del Nether, brillo, distancia
  de visión por defecto, la niebla «W-friendly» de Xbox, `ofFogStart` por defecto, opciones de cielo de OptiFine...).
  Deja la 1.5.2 como la 1.2.5. Valores (fogStart/fogEnd/color) de las dos en el informe.

## 4. Hojas: NO TOCAR
- La optimización de hojas de erluza se queda como está (activa, sin opción). Decisión del usuario 20-11. Claude la
  apagó y la volvió a poner (commit + revert en las dos ramas): el resultado es el mismo que en el 87.

## 5. Documentar la ocultación de entidades (pedido «muy importante dejarlo anotado»)
- Las entidades tapadas solo se ocultan si están 6+ bloques por debajo de la cámara (`EntityOcclusionCache.cpp`,
  `kMinBelowCameraBlocks`). Anótalo en `docs/XBOX_PORT.md` de las dos ramas (sección de rendimiento): qué hace, por qué
  (la vaca detrás de una colina desaparecía), dónde se ajusta y cómo apagarlo (`PLATFORM_ENTITY_OCCLUSION_CULLING`).

## Informe
`buzon-xbox\90-INFORME-agy.md` con causa (archivo:línea), arreglo, capturas en `buzon-xbox\90-capturas\` (las del 89
salieron bien con el volcado del backbuffer), qué probar. Señal: `ENCARGO 90 LISTO` + los dos commits finales.
