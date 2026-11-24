# 119. Informe subagente: ChocoCraft, lo que faltaba del original (Chocopedia, Chocodisfraz, Gysahl en el mundo, salvajes, paja)

Commits: **1.2.5 `e69f8169`** (`opti30-choco3`, desde `opti30-erluza`) · **1.5.2 `926dca9c`** (`port152-choco3`, desde `port152-int`).
Compilan normal y exp en las dos; «SSE2 left in» = 13 (las mismas funciones de la CRT de siempre).

Todo es el mismo código en las dos versiones (solo cambian: pestañas del creativo en 1.5.2 y lo del Libro y pluma, que no existe en 1.2.5).

## 1. Chocopedia (objeto nuevo, id 2173) y cambio de dueño
Port de `GuiChocopedia` + `GuiNameChocobo` + `GuiSelectNewOwner` + `GuiChangeOwner` en una sola pantalla nueva, `src/mods/chococraft/GuiChocopedia.cpp`, con las **dos interfaces** (Java y Legacy/LCE como `GuiChocoboMenu`) y **mando** (cruceta/A/B; en el libro cruceta o LB/RB para pasar página, A siguiente, B cerrar). Funciona para P1 y para P2 en pantalla partida (se abre en la mitad de quien la usa).
- **Usar el libro en el aire** → el libro con las 26 páginas del original (portada «The Chocopedia / by Clienthax», índice, I–VII). Texto en inglés como el original y traducido al español; colores de las variedades como el original. En 1.5.2 (Java) se dibuja sobre `/gui/book.png` como el original; en 1.2.5 (no tiene ese PNG) sobre una página color pergamino.
- **Usar el libro sobre un chocobo domado** → su página: nombre, dueño, salud (con «(hambriento)» si lo estuviera), sexo y si puede criar, color y habilidad. Botones del original: **Cambiar nombre**, **Nombre visible/oculto**, **Modo Seguir/Quedarse/Deambular** (`toggleFollowWanderStay`), **Cambiar dueño**, **Quitar equipo** (`ChocoboDropGear`: suelta silla, alforjas/mochila y lo que llevan; solo si no va montado) y **Confirmar**. Si no eres el dueño, los botones salen desactivados (como `checkButtonOwner`). Sobre un chocobo salvaje: humo, como el original.
- **Cambiar dueño**: lista de los jugadores del mundo (en pantalla partida, el otro jugador) → «¿Dar tu Chocobo a X?» Aceptar/Cancelar. Si no hay otro jugador lo dice.
- **Cómo se consigue** (como el original): receta `FGF/IBI/FLF` (plumas de chocobo, lingote de oro, sacos de tinta, libro, lapislázuli); **pluma de chocobo sobre una librería** (la librería se convierte en una Chocopedia); en 1.5.2 además **Libro y pluma sobre un chocobo** → Chocopedia, y la receta de Libro y pluma con pluma de chocobo.
- **Nombres del original** (`ChocoboNames`, 100 de macho y 96 de hembra): al domar se le pone uno al azar, oculto (como el original, `setHidename(true)`); se enseña desde la Chocopedia. Arreglado de paso: al cargar el mundo se perdía «nombre oculto».

## 2. Chocodisfraz (4 piezas, ids 2174–2177)
`ChocoboItemDisguise`: casco, cuerpo, patas y botas de plumas, con sus iconos y sus texturas de armadura del original (`armor/chocoDisguise_1.png`, `_2.png` para las patas), sin protección, durabilidad 1 y sin encantar, como `ChocoboArmourMaterial.CHOCOFEATHER`. Recetas del original con plumas de chocobo. Para que el jugador lo dibuje se añadió a `ItemArmor` un `getCustomArmorTexture(capa)` (nullptr = armadura vanilla) y `RenderPlayer` lo usa. En 1.5.2 salen en la pestaña Combate.

## 3. Matas de Gysahl en el mundo (bloque nuevo 212) — solo con el mod activo
- Bloque `BlockGysahlGreen` del original (un arbusto, se dibuja en cruz con su textura). La **Verdura de Gysahl** vuelve a ser, como en el original, el objeto que **planta la mata** (además de domar/curar chocobos); romper la mata da la verdura.
- `WorldGenGysahls`: en cada chunk de superficie nuevo, `nextInt(1000) <= 160` → una mancha de matas (`WorldGenFlowers`) a la altura del terreno. Se engancha **después** de la población vanilla (`IChunkProvider::modPopulateHook`, en `ChunkProviderGenerate` —normal e incremental— y `ChunkProviderFlat`) y usa un **Random propio** con la semilla de Forge (`semilla del mundo` + chunk, `IWorldGenerator`), nunca el del generador. Con el mod apagado el gancho vuelve antes de crear nada.
- Única diferencia con el original: la mancha va con +8 como las flores vanilla, para no salirse a chunks sin generar (el de 1.7.10 sí se sale y fuerza chunks vecinos).
- Como el original (`ChocoCraftWorld.addGrassDrops`), la **hierba alta** puede dar **semillas de Gysahl** (peso 3 frente a 10 del trigo) — solo con el mod activo. Así también salen en mundos ya generados.
- Se quitaron las «Gysahl que aparecían solas cerca del jugador» (no son del original).

## 4. Chocobos salvajes como el original (`ChocoboSpawner` + `ServerSpawnTickHandler`)
Cada 200 ticks, alrededor de cada jugador (los dos en pantalla partida): probabilidad 2 %, a 32–96 bloques, en los biomas del original (colinas extremas y su borde, bosque y colinas, jungla y colinas, llanura, pantano, taiga y colinas, llanura y montañas heladas), sin otro jugador cerca, como mucho 3 salvajes en 100 bloques y a 300 bloques del salvaje más cercano; **bandada de 2 o 3**, sobre suelo de 3×3 bloques sólidos con 3 de aire encima. **Color**: el original solo saca **Amarillos** en el mundo normal y **Morados en el Nether** (y el End): no hay otros colores salvajes en 4.1.5 (los demás salen criando), así que se ha dejado igual. Los salvajes **no desaparecen** (`wildCanDespawn = false`), como el original.
- Diferencia: en el Nether el original busca la altura con el mapa de alturas, que allí es el techo de roca madre; aquí busca el primer suelo con sitio (y 32–100) para que los Morados salgan en las cuevas.

## 5. Hambre del chocobo y L3
- El original tiene el temporizador de hambre pero **apagado** (`DEFAULT_HUNGER_ENABLED = false`, «hunger not yet active»). Portado igual: temporizador (3000 / 1000 ticks en cría), se guarda en el NBT, la Gysahl lo alimenta si tuviera hambre y la Chocopedia pondría «(hambriento)»; con el interruptor apagado como en 4.1.5 nunca tiene hambre (`EntityChocobo::HUNGER_ENABLED`).
- L3 en el aire sigue **bajando** (no desmonta), como pidió el usuario. No se ha tocado.

## 6. Paja con su textura propia
El motor solo dibuja bloques desde casillas de `terrain.png`. Hay dos casillas que están vacías en 1.2.5 y sin usar en el atlas de 1.5.2 (**180 y 181**): un `TextureFX` estático (`ChocoCraftTileFX`, en `ChocoCraftWorld.cpp`) las rellena con `blocks/gysahl_green.png` y `blocks/chocoboStraw.png` del original (se recargan al cambiar de pack de texturas y se escalan a packs HD como las demás casillas dinámicas). Paja = casilla 181, mata de Gysahl = 180. La paja ya no usa el trigo teñido. (Los colores salen como el PNG original: paja anaranjada, hojas verde azulado con raíz rosa.)

## 7. Otros ajustes contra el original
- Recetas nuevas: Chocopedia, 4 piezas de Chocodisfraz, (1.5.2) Libro y pluma con pluma de chocobo.
- `ModManager`: bloque 212 y el rango de objetos hasta 2177 son del mod; con el mod apagado desaparecen del creativo, recetas y TooManyItems y no actúan (como lo del encargo 115).

## Probado (xemu, 1.2.5, build de prueba con una sonda temporal ya quitada)
Mundo nuevo, semilla 20261008, creativo:
- Matas de Gysahl generadas: **20** en los 79 chunks de alrededor (≈16 % de chunks, lo esperado); con el mod apagado: **0**.
- Semillas de la hierba: 2988 de 13000 tiradas (esperado ≈3000).
- Salvajes: sale una bandada de **2 Amarillos**; un segundo intento a menos de 300 bloques no saca ninguno ✅.
- Textura de la mata y de la paja en el mundo ✅ (captura 01). Chocodisfraz puesto en 3ª persona ✅ (02). Libro en Java ✅ (03) y Legacy ✅ (04). Página del chocobo en Legacy ✅ (05; dueño, salud, sexo, color). Cambiar dueño sin más jugadores, Java ✅ (06) y Legacy ✅ (07). Modo cambia a Seguir; «Nombre visible» → el nombre «Locke» sale encima; «Quitar equipo» suelta la silla (`saddled=0`) ✅ (08).
- Terreno con el mod apagado: el gancho vuelve antes de crear el Random o tocar un bloque, y el generador vanilla vuelve a sembrar su Random en cada chunk, así que no puede cambiar nada. Comprobación en xemu (huella de bloques+metadatos por chunk, quitando las matas): al cargar el mundo, 7 de 9 chunks salen idénticos bit a bit en apagado/apagado/encendido; los 2 que cambian son distintos en cada partida e igual cambian entre **dos partidas apagadas con el mismo ejecutable** (orden de población y ticks aleatorios del motor: a los 25 s difieren ~48 de 78 chunks entre dos partidas apagadas idénticas). O sea, la comparación byte a byte entre partidas no es exacta en este motor ni sin el mod; con el mod apagado no hay ni una mata (0) y no se ejecuta nada de la generación.
- La captura 03 es de antes de un retoque: los botones del libro en Java se subieron después para que no tapen la fila de ayudas (no se volvió a capturar). La página del chocobo en Java no quedó capturada (la sonda la abrió entre dos capturas).
- No probado: 1.5.2 en xemu (el código del mod es idéntico; solo compila), pantalla partida con P2 abriendo la Chocopedia, consola real.

## assets.pak
**No hay ficheros nuevos**: todo lo que se usa (`armor/chocoDisguise_1/2.png`, `items/chocopedia.png`, `items/disguise*.png`, `blocks/gysahl_green.png`, `blocks/chocoboStraw.png`) ya estaba en `scripts/chococraft/` desde el encargo 113. Si el `assets.pak` del lanzador ya se parcheó con `scripts/chococraft/patch_pak.py`, no hay que hacer nada; si no, parchearlo como en 113. Sin parchear: la mata y la paja salen de color liso (verde / paja) y el disfraz sin textura.

## Diferencias con ChocoCraft 4.1.5 que quedan
1. **Chocopedia en cofres de mazmorra/mina/templo/fortaleza** (`registerDungeonLoot`) no está: meterla cambiaría las tiradas del generador vanilla y por tanto el terreno con el mod activo. Se consigue por receta o con pluma sobre una librería.
2. Los **tallos de Gysahl** (cultivo) siguen con la casilla del trigo teñida: solo hay dos casillas libres en `terrain.png` y se usaron para la mata y la paja.
3. **Mata generada con +8** respecto al original (para no generar chunks vecinos) y **Morados del Nether** buscando suelo en vez del techo (ver 3 y 4).
4. **Hambre**: portada pero apagada, igual que en 4.1.5.
5. L3 en el aire baja y solo desmonta en el suelo (decisión del usuario); el chocobo montado no recibe daño de caída (en el original sí); la Visión nocturna del Negro dura 11 s (original 5 s).
6. «Cambiar dueño» del original solo sale en servidores; aquí sale siempre (en un jugador dice que no hay a quién dárselo).
