# 113. Informe agy: ChocoCraft «de verdad» (port real de ChocoCraft 4.1.5 — 1.7.10) para 1.5.2 y 1.2.5

## Commits
- **1.5.2** (`wt-152-choco`, rama `port152-choco`): **`45edeba0`**
- **1.2.5** (`wt-125-choco`, rama `opti30-choco`): **`2ee1d286`**

## Resumen de lo portado desde ChocoCraft 4.1.5 (`mcp-diff\chococraft-1710`)
1. **Modelos reales 1:1 (`ModelChocobo` y `ModelChicobo`)**:
   - **`ModelChocobo`** (`src/mods/chococraft/ModelChocobo.h/.cpp`): port exacto de las 23 cajas, pivotes, rotaciones, UVs 128×128 y animaciones de `ModelChocobo.java` (cabeza, cuello, 2 crestas de plumas, torso, 3 capas de cola, alas desplegables/aleteo en aire, muslos/espinillas/3 garras por pata, alforjas izquierda/derecha y mochila `packBag`). Escala `0.8f` en `RenderChocobo` igual que `RenderChocobo.java`.
   - **`ModelChicobo`**: modelo propio de la cría portado 1:1 de `ModelChicobo.java` (textura 64×32, escala `1.32f` en `RenderChocobo`, cabeza perfectamente unida al cuerpo, arreglando el typo histórico `rotateAngleY -> rotateAngleX` de la pata izquierda del original).
2. **Texturas reales y sonido `choco_kweh.ogg` (`scripts/chococraft/`, 140 archivos, 213.4 KB)**:
   - Los **9 colores reales** de ChocoCraft 4.1.5: **Amarillo, Verde, Azul, Blanco, Negro, Dorado, Rosa, Rojo, Morado** — con todos sus estados (`Untamed`, `Tamed`, `Saddled`, `SaddleBagged`, `PackBagged`), géneros (`Male`/`Female` con collar rojo/azul) y texturas propias de crías `Chicobos/` y `Chicobos/Tamed/`.
   - **Iconos reales 16×16** para todos los ítems de ChocoCraft (`Item::getCustomIconTexture()` en `RenderItem.cpp` e `ItemRenderer.cpp` con fallback automático al atlas `/gui/items.png` si se usa un `assets.pak` antiguo): Silla, Alforjas, Mochila, Pluma, Semillas/Verdura/Amoroso/Dorado/Rosa/Rojo/Pastel/Encurtido Gysahl, Muslo crudo/asado, Silbato y Huevo de Chocobo Morado.
   - **Sonido real** `choco_kweh.ogg` instalado en `resources/newsound/mob/chocobo/kweh.ogg` (`"mob.chocobo.kweh"`, pitch agudo en crías Chicobo, fallback a `"mob.chicken"` si faltara en un pak antiguo).
   - Integrado en `scripts/port152/build_textures_152.py`, `cmake/xbox.cmake` (ambas versiones, incluyendo soporte `XBOX_ASSETS_PAK` en 1.2.5) y `scripts/chococraft/patch_pak.py` para inyectar los 140 assets en cualquier `assets.pak` existente sin tocar los ficheros del launcher.
   - Créditos y licencias (ChocoCraft 4.1.5 + referencia ChocoCraft4 MIT / CC-BY-SA 4.0) añadidos en `docs/CHOCOCRAFT_CREDITS.md` en ambas ramas.
3. **Arreglos de los 5 problemas reportados en consola real**:
   - **Cría con cabeza despegada**: resuelto con `ModelChicobo` y texturas `Chicobos/*.png` reales.
   - **Aspecto hecho por código**: sustituido por completo por `ModelChocobo` + texturas reales 128×128 / 16×16.
   - **El dorado no volaba**: causa raíz encontrada — cuando el jugador monta una entidad (`ridingEntity != nullptr`), `World::updateEntityWithOptionalForce` llama a `updateRidden()` y NO a `EntityPlayerSP::onLivingUpdate()`, por lo que `sp->movementInput->updatePlayerMoveState(sp)` dejaba de actualizarse en Xbox. Al refrescar `movementInput` cada tick en `EntityChocobo::onLivingUpdate()` (compatible con P1 y P2 en pantalla partida), el **Dorado (y Rosa, Rojo, Morado)** despega y vuela manteniendo **A**, planea suavemente al soltar **A**, desciende rápido manteniendo **L3** en el aire (como el vuelo de creativo, sin desmontarte en pleno cielo) y desmonta limpiamente con **L3** al tocar suelo/agua.
   - **Montado se movía solo y daba igual la silla**: `EntityChocobo::updatePlayerActionState()` apaga el 100% de la IA al ir montado (`if (riddenByEntity != nullptr) return;`, `drift = 0.0000` verificado en 30 ticks sin input), y `EntityChocobo::interact()` bloquea montar si `!isSaddled()` o `hasPackbags()` o `isChild()` (muestra humo; `mounted = 0` verificado). Al desmontar coloca al jugador lateralmente a 1.15 bloques si hay sitio libre.
   - **Daño por caída al planear/aterrizar**: `fallDistance` del jinete y del chocobo se mantiene a `0.0f` durante todo el vuelo/planeo/aterrizaje y desmontaje, `EntityChocobo::fall()` anula el daño al ir montado/planeando/volando y `EntityPlayer::fall()` ignora caída mientras `ridingEntity != nullptr`. Verificado `20/20 HP` antes y después de aterrizar tanto con Dorado desde Y=20 como con Amarillo planeando desde +18 bloques.
4. **Habilidades de los 9 colores, cría, ítems, recetas y menú LCE**:
   - Amarillo (clásico), Verde (escala `stepHeight 2.0` + gran salto), Azul (nada rápido, flota, Respiración Acuática), Blanco (escala + agua + gran salto), Negro (veloz + Visión Nocturna + sin daño de caída), Dorado (vuela + escala + agua), Rosa/Rojo (pintando un Dorado con Gysahl Rosa/Rojo, los más rápidos volando), Morado (Huevo Morado, vuela + inmune al fuego/lava + Resistencia al Fuego).
   - Genética exacta de ChocoCraft 4.1.5, cultivo Gysahl con mutación a Loverly/Golden Gysahl, bloque de Paja (`BlockChocoboStraw`) con curación junto a caldero lleno, Pastel Gysahl (crece cría al instante), Silbato (llama/teletransporta chocobo), Alforjas (27 huecos) y Mochila (54 huecos), recetas de mesa y horno.
   - Aspecto visual LCE de `GuiChocoboMenu` conservado intacto; solo actualizado el contenido (alforjas 27 / mochila 54 y llamada a `mountChocobo`).

## Verificación en `xemu` (ambas versiones, `xbox-release`)
Todas las pruebas pasaron sin crashes (`crashed=False`, `done=True`):

| Prueba | 1.5.2 (`wt-152-choco`) | 1.2.5 (`wt-125-choco`) |
|---|---|---|
| Intento sin silla (`NO_SADDLE`) | `mounted=0, saddled=0` ✅ | `mounted=0` ✅ |
| Montar con silla (`MOUNT_WITH_SADDLE`) | `mounted=1, saddled=1` ✅ | `mounted=1` ✅ |
| IA apagada montado (30 ticks sin input) | `drift=0.0000` ✅ | `drift=0.0000` ✅ |
| Vuelo del Dorado (manteniendo A) | `startY=4.00 -> peakY=20.02 (+16.02), flying=1` ✅ | `deltaY=+15.58, flying=1` ✅ |
| Aterrizaje Dorado sin daño | `hpBefore=20 -> hpAfter=20` ✅ | `hpBefore=20 -> hpAfter=20` ✅ |
| Planeo Amarillo desde +18 blq + desmontar | `hpBefore=20 -> hpAfter=20 -> dismountHp=20` ✅ | Verificado código idéntico ✅ |
| Guardar / Cargar mundo (NBT) | `total=2, goldSaddled=1, purpleBaby=1` ✅ | `foundGold=1` ✅ |
| Rendimiento (FPS en gameplay) | **68.3 – 86.7 FPS** ✅ | **76.7 – 87.6 FPS** ✅ |
| Memoria libre (`freeKB`, 64 MB) | **34.1 – 38.3 MB libres** ✅ | **35.3 – 42.6 MB libres** ✅ |

## Capturas (`buzon-xbox\113-capturas\`)
- `01_152_9_colores_adultos.png` — Los 9 colores adultos reales (1.5.2) con sillas, alforjas, mochila e iconos reales en la hotbar.
- `02_152_cria_chicobo.png` — 5 crías Chicobo (1.5.2) con cabeza pegada al cuerpo y pajarita de domesticado.
- `03_152_sin_silla_no_monta.png` — Intento de montar sin silla (1.5.2): no monta, jugador en pie.
- `04_152_montado_con_silla_3p.png` — Montado con silla y alforjas en 3ª persona (1.5.2), `drift=0.0000`.
- `05_152_dorado_volando.png` — Chocobo Dorado volando a `Y=22` con alas abiertas (1.5.2).
- `06_152_aterrizaje_sin_dano.png` — Aterrizaje y desmontaje lateral sin daño (`20/20` corazones llenos, 1.5.2).
- `07_125_9_colores_adultos.png` — Los 9 colores adultos reales (1.2.5).
- `08_125_cria_chicobo.png` — 5 crías Chicobo con cabeza en su sitio (1.2.5).
- `09_125_dorado_volando.png` — Chocobo Dorado volando a `Y=22` (1.2.5).
- Logs completos: `verify_152.log` y `verify_125.log`.

## Nota sobre `assets.pak` para despliegue en consola
Al generar o actualizar los `assets.pak` de 1.5.2 y 1.2.5 para la consola:
- En **1.5.2**, `scripts/port152/build_textures_152.py` ya copia automáticamente `scripts/chococraft/` dentro del `assets.pak`.
- Para parchear un `assets.pak` ya generado (1.2.5 o 1.5.2), basta ejecutar:
  `python scripts/chococraft/patch_pak.py --base-pak <assets.pak> --overlay scripts/chococraft --output <nuevo_assets.pak>`

**CIENTOTRECE-LISTO** `45edeba0` `2ee1d286`
