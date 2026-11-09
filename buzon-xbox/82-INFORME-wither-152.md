# 82. Informe: Wither (jefe) en la 1.5.2 (09-11, Claude)

Rama `port152-wither` en `wt-152-wither`, commit `238ecee` sobre `6ea0ac1` (port152-aldeanos). Compilan las builds normal
y experimental (480 y 720p). No probado en consola ni xemu. **El pak NO cambia** (texturas `mob/wither.png`,
`mob/wither_invul.png`, `armor/witherarmor.png` y sonidos `sound3/mob/wither/*` ya estaban en los datos).
XBE: `wt-152-wither\bin\xbox\iso\` y `wt-152-wither\bin\xbox-exp\iso\`. Builds con log (192.168.0.18).

## Qué entró (comparar.sh: todas las clases ≥ 100 % del Java)
- `EntityWither.cpp/.h`: 3 cabezas (DataWatcher 16 vida, 17-19 objetivos, 20 invulnerable, como Java), IA (nado,
  disparo a distancia, vagar, mirar, objetivo: todo lo vivo que no sea no-muerto), vuelo hacia el objetivo, partículas,
  calaveras de las cabezas laterales (azules al azar en Normal/Difícil), escudo bajo media vida (las flechas rebotan),
  rompe los bloques de su alrededor 1 s después de recibir daño (no bedrock ni portal del End), regenera, inmune a
  pociones, ahogo, fuego y caída; NBT `Invul`; estrella del Nether al morir; 50 de XP; no desaparece.
- Invocación (`BlockSkull::makeWither`, llamada desde `ItemSkull`): T de arena de almas + 3 calaveras de esqueleto
  wither, en X o en Z; dificultad > Pacífico. Nace invulnerable 220 ticks, crece, con 1/3 de vida que se rellena, y al
  terminar explota (fuerza 7) y suena `mob.wither.spawn` (aviso 1013, desde la dirección del jefe).
- `EntityWitherSkull`: daño 8 (cura 5 al Wither si mata), efecto Wither 10 s (Normal) / 40 s (Difícil), explosión 1;
  la azul es más lenta (0.73) y rompe hasta obsidiana (resistencia máx. 0.8 vía `Entity::func_82146_a`).
- `ModelWither`, `RenderWither` (escala 2, crecimiento al nacer, caparazón de armadura animado con la matriz de
  textura y blend aditivo), `RenderWitherSkull`; registrados en `RenderManager` y `EntityList` (ids 64 y 19).
- **Barra de jefe** (`BossStatus` + `GuiIngame::renderBossHealth`) como Java: nombre traducido y vida; la usan el
  Wither y el dragón (antes el dragón dejaba un puntero `RenderDragon::entityDragon`, ahora se copian valores). Es la
  misma función para HUD Java y Legacy. Con el Wither cerca la luz se oscurece (como Java, `bossColorModifier`).
- Sonido 1014 (`mob.wither.shoot`) en `RenderGlobal::playAuxSFX`.
- Cambios compartidos: `EntityFireball::getMotionFactor` virtual; `EntityAINearestAttackableTarget` acepta un filtro
  (IEntitySelector de Java); `addPotionEffect` y `mountEntity` virtuales; `Explosion` pregunta la resistencia al
  causante (`Entity::func_82146_a`, por defecto igual que antes).

## Rendimiento
Sin Wither no hay trabajo nuevo por frame: la barra comprueba un entero, el oscurecido un float. Con el Wither: sus
partículas (3 humos + magia por tick, como Java) y las de las calaveras.

## No hay (y por qué)
- `mobGriefing`: el port no tiene reglas de juego; siempre rompe bloques (el valor por defecto de Java).
- Huevo de Wither: Java tampoco lo tiene.
- Revisión con subagente: no pude (soy un fork). Revisé a mano: orden e ids del DataWatcher (16-20 no chocan con 0,1,8,9
  de la base; la calavera usa 10), dueño de `PotionEffect` (el Wither la borra al ignorarla), explosión con el propio
  Wither invulnerable en ese tick, `renderPassModel = mainModel` no se borra dos veces (`~RenderLiving` lo pone a nulo),
  bucles que borran de la lista de candidatos, puntero de la calavera al disparador con `getShootingEntity()`.
  **Conviene una auditoría aparte antes de la consola.**

## Qué probar en consola
1. Invocar (T de arena de almas + 3 calaveras de esqueleto wither) en X y en Z: crece, explota, barra con nombre.
2. Pelea: calaveras negras y azules (la azul rompe obsidiana), efecto Wither en el jugador, escudo bajo media vida
   (las flechas rebotan), rompe bloques al recibir daño, se oscurece la luz cerca de él.
3. Muerte: estrella del Nether, XP, la barra desaparece sola.
4. Guardar y cargar con el Wither vivo (y recién invocado, aún invulnerable).
5. Dragón del End: su barra sigue saliendo (ahora con el nombre «Dragón del Fin»).
6. Fps: sin Wither, igual que la referencia de 60; con Wither, medir con el log.
