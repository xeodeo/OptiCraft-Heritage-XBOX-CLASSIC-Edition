# 72. Informe: aldeanos 1.5.2 con comercio Java + Legacy (06-11, Claude)

## Rama y commits
- Worktree `wt-152-pociones`, rama nueva **`port152-aldeanos`** (WIP `c746c8f` con rebase sobre `port152-claude` `98b72b0`).
  `port152-claude` y `wt-152-codex` no se tocaron.
- `32a2a54` / `4bbd9c5`: los WIP de Codex/agy después del rebase (conflictos en EntityPlayer y GuiContainer resueltos dejando ambas partes).
- `4f79fba`: arreglos para que compile.
- `386208f`: pantalla Legacy de comercio, aldeas del desierto, IA de comercio como en Java.
- **`8732884`** (último): correcciones de la auditoría (abajo).
- Compilado `xbox-release` y `xbox-release-exp` con el candado. Los datos del pak salen de
  `wt-152-codex\build\xbox-release\port152-data` (solo lectura; `gui/trading.png` ya estaba). El pak no cambia.
- XBE: `wt-152-pociones\bin\xbox\iso\` (default, OptiCraft, OptiCraft_720p) y `bin\xbox-exp\iso\`. NO se copiaron a la Xbox.

## Qué hay
- **Dos interfaces** (regla del usuario): con `legacyUI` activo (por defecto en Xbox) se abre
  `legacy/LegacyTradingScreen`, copiada de LCE `UIScene_TradingMenu`/`IUIScene_TradingMenu`. Tiene la geometría de
  `xuiscene_trading.xui` a la mitad: barra de 7 ofertas con flechas, recuadro rojo si no puedes pagar, «Objetos
  necesarios» con los 2 objetos pedidos y sus nombres, el inventario (solo se ve) y la descripción bajo la oferta elegida.
  Cruceta izq/der para elegir, A para comerciar directamente desde el inventario (como en la 360) y B para salir.
  Sin `legacyUI` se abre la `GuiMerchant` de Java (con casillas y cursor; Blanco/Negro cambian de oferta).
- Pantalla partida: las dos se abren con `displayPlayerScreen` para el jugador correcto.
- **Idiomas**: «Comerciar» y «Objetos necesarios» están en los 58 idiomas (`tradeUiText` en `StringTranslate.cpp`;
  los de la LCE donde existen). El nombre del aldeano y «Inventario» usan claves de vanilla.
- **Aldeas del desierto**: `ComponentVillage::getBiomeSpecificBlock(+Metadata)` como en Java (arenisca y escaleras de arenisca;
  caminos de arenisca). Fuera de las aldeas del desierto no cambia ningún bloque.
- `EntityAITradePlayer` / `EntityAILookAtTradePlayer` eran cascarones; ahora están portados. El comercio se corta si te
  alejas más de 4 bloques, si el aldeano está en el agua, en el aire o recibe un golpe (como en Java).
- Las dos pantallas se cierran solas si el aldeano deja el comercio, y no se cuelgan si el aldeano se borra con la pantalla
  abierta (`merchantScreensForget` desde el destructor).

## Auditoría (cuelgues encontrados y arreglados)
1. **Doble delete de `VillageCollection`**: el WIP la registraba en `MapStorage` (que la borra) y además `World` la borraba.
   Se colgaba al salir del mundo, y al cruzar un portal quedaba un puntero colgando. Ahora `World` es el único dueño:
   lee `villages.dat`, libera la copia y se guarda en `World::saveWorld`.
2. Los aldeanos guardaban un `Village*` que `removeAnnihilatedVillages` borraba. Ahora lo sueltan (`forgetVillage`).
3. Una oferta guardada cuyo objeto no carga se descarta (antes daba una pila nula que todas las pantallas usan).
4. `MerchantRecipe` se queda con sus pilas, como Java (antes las copiaba y se perdían en cada oferta nueva).
5. Fidelidad: el libro encantado del bibliotecario ahora trae su encantamiento y el precio de Java. Los zombis del asedio
   llevan `initCreature` + `setVillager(false)`.
La auditoría la hizo Claude directamente (en este encargo no se podían lanzar subagentes). Conviene una segunda pasada con
subagente antes de integrar en `port152-claude`.

## Qué probar en consola
1. Hablar con aldeanos de cada profesión. Comprobar la pantalla Legacy: que A comercie, el rojo cuando falta material, la
   descripción, las flechas con más de 7 ofertas y que B salga.
2. Desactivar Legacy UI: GuiMerchant Java con casillas, cursor y Blanco/Negro.
3. Agotar una oferta (7 usos): queda bloqueada y al cerrar sale una nueva al rato, con corazones verdes.
4. Alejarse o pegarle al aldeano con la pantalla abierta: se cierra sola.
5. Que un zombi mate al aldeano con la pantalla abierta: no debe colgarse.
6. Guardar, salir y volver: que el aldeano conserve sus ofertas y la aldea su reputación (`villages.dat`).
7. Cruzar al Nether y volver: no debe colgarse.
8. Aldea del desierto: casas de arenisca.
9. Pantalla partida: el jugador 2 comercia con su mando.
10. Cambiar de idioma (japonés, ruso…) y mirar los textos.

## Dudas / pendiente
- La pantalla Legacy se elige con `legacyUI`, no con «Console Crafting» (`xboxStyleCrafting`). Si se prefiere la otra opción,
  es una línea en `EntityPlayerSP::displayGUIMerchant`.
- En pantalla partida horizontal el panel (180 de alto) puede no caber, igual que el crafteo Legacy.
- Ya existía antes: `VillageDoorInfo*` en las IA de puertas puede quedar colgando si se borra la puerta (falta revisarlo).
- `LegacyCraftingScreen::craftCurrentRecipe` pierde la pila resultado cuando el inventario la acepta (pequeña fuga; no es de este encargo).

## Auditoría 2 (coordinador, sobre `8732884`) — arreglado en **`1e9fb58`**
1. **Cuelgue** con Mayús+clic en el resultado cuando solo cabe una parte: `resetRecipeAndSlots` borraba `inventory[2]`, que es
   `source`. Ahora el slot 2 siempre hace `takeStack()` (sin borrar) y libera la pila después de `onPickupFromSlot`.
2. `InventoryMerchant::resetRecipeAndSlots` comprueba que la lista de recetas no sea nula. También prueba el orden invertido
   cuando la primera receta está agotada (como Java).
3. El destructor del aldeano llama siempre a `merchantScreensForget`.
4. Si al cargar `Offers` no queda ninguna oferta válida, `buyingList` queda nulo y se regeneran.
5. Lo que estaba portado a medias: `isMatingSeason` en el apareamiento; el gólem ataca a jugadores con reputación ≤ −15
   (1 de cada 20); el zombi curado da +5 de reputación (`func_82187_q`); partículas `angryVillager` (81) y
   `happyVillager` (82); corazones con el estado 12.
6. `villages.dat` solo se carga y guarda en la dimensión 0. Se carga en el primer tick, porque `worldType` se conoce
   después de `registerWorld`.
7. Si un aldeano muere por flecha, bola de fuego o algo lanzado, cuenta quien disparó (−2 de reputación al jugador).
8. Comercio Legacy: compara solo el ID, como `canRecipeBeUsed`, y paga primero con pilas sin encantar y sin nombre.
9. Los tooltips de `GuiMerchant` se dibujan sin depth test.
Las 4 builds (xbox-release y -exp, 480 y 720p) compilan.

### Qué más probar
- Mayús+clic en el resultado con el inventario casi lleno.
- Matar a un aldeano con el arco y ver las partículas de enfado y los corazones.
- Ir al Nether, volver y comprobar que la aldea mantiene su reputación.

## 09-11 (rama `port152-aldeanos`, `wt-152-pociones`)
- `5ea6719` Comercio Legacy: la oferta elegida lleva un marco amarillo y se aclara; se quitó el cursor de menú que tapaba el objeto.
- `227432b` **Niebla en la Xbox** (no se veía nunca, también en 1.2.5): D3D solo hace niebla por distancia con una
  proyección "W-friendly" con (3,4) = +1 (XDK, "A W-Friendly Projection Matrix"). La de GL tiene -1. Ahora la vista
  se manda con z invertida (filas z) y la proyección con la columna z invertida: mismo clip, (3,4) = +1. Sin luces D3D ni
  texgen que dependan del signo. Falta pasarlo a la 1.2.5 (`wt-opti30`, mismo `applyMatrices`) si el usuario lo confirma.
- `b1c68a7` **Sonidos del aldeano**: en Java 1.5.2 eran mudos (el índice oficial pre-1.6 no trae `mob/villager`). Se
  bajaron los de la 1.6 (índice `legacy`, sha1 comprobado) y se guardaron en `scripts/port152/extra_sounds/sound3/mob/villager/`
  con los nombres que usa el código: idle→default, hit→defaulthurt, death→defaultdeath (+ haggle/yes/no sin usar).
  `build_textures_152.py` los copia (`copy_extra_sounds`). Vorbis mono 44,1 kHz, igual que los demás.
  El resto de nombres de sonido del C++ ya existían (sound3 completo de la 1.5.2). También se copiaron a mano a
  `wt-152-codex/build/xbox-release/port152-data` (los datos que usa la build).
- Pak: 63.715.712 → 63.845.504 bytes (+127 KB). 4 XBE + pak en `OptiCraft\1.5.2` y `OptiCraft_actualizar\1.5.2` (09:47).
- Qué probar: niebla a distancia (y bajo el agua/lava), que no haya nada roto en el dibujo (cielo, nubes, mano, GUI),
  sonido al pegar/matar un aldeano y el «hmm» de fondo.
