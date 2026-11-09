# Encargo 50 (agy): grupo 3, parte A — mobs nuevos de la 1.5.2

Claude ACEPTÓ el 47 (redstone A) con un arreglo suyo encima: el cable y la placa de presión cambiaban la fuerza con el
setter viejo que AVISA a los vecinos (y quitaste el `editingBlocks` de 1.2.5). Ahora usan
`setBlockMetadataWithNotify152(..., 2)` como el Java (commit `ce3de78`). Ojo con eso en adelante.

## Tu sitio
- Worktree `wt-152-agy`, rama `port152-inventario`, en `ce3de78`. No hagas reset ni rebase.
- Codex trabaja a la vez en `wt-152-codex` (rama `port152-base`) en RAÍLES y VAGONETAS (encargo 49). No toques ese
  worktree ni nada de raíles/vagonetas. Los dos tocaréis `EntityList`: tú solo añade tus mobs, en un bloque aparte.
- Java 1.5.2: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`. Tus docs en `docs\port152\`.

## Qué portar (traducción del Java 1.5.2)
1. `EntityBat` + `ModelBat` + `RenderBat` (murciélago: cuelga del techo, vuela, aparece en cuevas; `EntityAmbientCreature`
   y el tipo de criatura ambiental en el spawn, con el límite de 1.5.2). Textura `/mob/bat.png` (ya en el pak).
2. `EntityWitch` + `ModelWitch` + `RenderWitch` (pociones que lanza y bebe, `IRangedAttackMob` si hace falta).
   Textura `/mob/villager/witch.png`. Aparece de noche como los demás monstruos (la cabaña es del grupo 4, no la hagas).
3. Zombi aldeano: `EntityZombie` 1.5.2 (variante aldeano, bebés, curación con manzana de oro + debilidad, recoger objetos
   si el Java lo hace) + `ModelZombieVillager` + `RenderZombie`. Textura `/mob/zombie_villager.png`.
4. Esqueleto Wither: `EntitySkeleton` 1.5.2 con `skeletonType` (aparece en fortalezas del Nether, espada de piedra,
   suelta cabeza de esqueleto Wither a veces) + `RenderSkeleton`. Textura `/mob/skeleton_wither.png`.
5. Huevos de generación de estos mobs (ItemMonsterPlacer + colores de EntityList) y su registro en EntityList con los
   IDs de 1.5.2 (65 murciélago, 66 bruja).
- NO: el Wither jefe, el comercio con aldeanos, la cabaña de bruja (otros encargos).

## Reglas (las de siempre, más estas)
- Las texturas de mobs que en 1.5.2 son 64x64 se recortan a 64x32 en el pak (así lo hace el script del atlas): los
  modelos usan el UV de 1.2.5. Si un modelo nuevo necesita la parte de abajo de una textura 64x64, DILO, no toques el script.
- Sonidos: usa los nombres de 1.5.2; si un sonido no existe en el pak, dilo en el informe.
- Candado BUILD.lock. Compila `xbox-release` y `xbox-release-exp`. Nunca /fp:fast. No push. No copies a ClassicXboxLoader.
- Rendimiento Xbox: la IA corre cada tick en 733 MHz; nada de `new` por tick ni búsquedas que no estén en el Java.
  El pathfinding ya tiene límite (`PLATFORM_PATHFIND_BUDGET_PER_TICK`): respétalo.

## Al acabar
Informe `buzon-xbox\50-INFORME-agy-mobs-152.md` (tabla mob -> archivos:línea, texturas, sonidos que falten,
`git diff --stat`), commits, y en tu terminal: `MOBS 152 LISTO <commit>`
