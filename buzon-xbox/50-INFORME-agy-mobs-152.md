# Informe 50: Mobs de Minecraft 1.5.2 (OptiCraft Xbox)

**Fecha:** 25 de Octubre de 2026  
**Agente:** Antigravity (agy)  
**Rama:** `port152-inventario`  
**Commit:** `e611f9c9d0a843d0ac97bd89ac56089a7aa97254` (`e611f9c`)  

---

## 1. Resumen de Implementación

Se ha implementado el soporte completo de los mobs de Minecraft 1.5.2 en OptiCraft para la Xbox Clásica (733 MHz, 64 MB RAM):
1. **Murciélago (`EntityBat`)**: Criatura ambiental con límites de spawn independientes (`EnumCreatureType::ambient`, límite 15), cuelga del techo, vuelo en cavernas oscuras (< Y=63), sonido de despegue (SFX 1015), sin colisión con placas de presión.
2. **Bruja (`EntityWitch`)**: Mob hostil a distancia (`IRangedAttackMob`), lógica de beber pociones (curación rápida, resistencia al fuego, respiración acuática, velocidad) y lanzar pociones arrojadizas (lentitud, veneno, debilidad, daño instantáneo). Resistencia mágica del 85%, partículas `witchMagic`, spawn nocturno en biomas de superficie.
3. **Zombie Aldeano / Bebé (`EntityZombie`)**: Variante aldeano (`isVillager()`), variantes bebé con renderizado proporcional de cabeza grande (`ModelBiped::render` con escalado infantil), proceso de curación mediante manzana dorada + debilidad (temblor en `RenderZombie::rotateCorpse`, temporizador 3600-6000 ticks, sonido `mob.zombie.remedy`, SFX 1017 al curar), infección de aldeanos (SFX 1016) y recogida de botín.
4. **Esqueleto del Wither (`EntitySkeleton`)**: Variante con `skeletonType == 1`, spawn en fortalezas del Nether (`MapGenNetherBridge`), espada de piedra en mano, efecto de Wither durante 10 segundos en melee, inmunidad al fuego, hitbox de 2.34m de altura, drop raro de cabeza de esqueleto del Wither (`Item::skull` metadata 1), carbón y huesos. Render con escala 1.2x (`RenderSkeleton`) y traslación de espada ajustada.
5. **Huevos de Spawn y Registro**: Mapeo en `EntityList` (Murciélago ID 65 con colores 4996656 / 986895; Bruja ID 66 con colores 3407872 / 5349438), traducción de nombres en `StringTranslate` y registro de fábricas en `SpawnerAnimals`.

---

## 2. Tabla de Mobs Implementados

| Mob | Ficheros Principales | Render / Modelo | Textura | Lógica de Spawn |
|---|---|---|---|---|
| **Murciélago (Bat)** | `EntityBat.h/.cpp`<br>`EntityAmbientCreature.h/.cpp` | `RenderBat.h/.cpp`<br>`ModelBat.h/.cpp` | `/mob/bat.png`<br>(64x64) | `EnumCreatureType::ambient` (cap 15). Cueva oscura (Y < 63, luz <= 4 o 7 en Halloween), bloque sólido arriba. `spawnableCaveCreatureList` en `BiomeGenBase`. |
| **Bruja (Witch)** | `EntityWitch.h/.cpp`<br>`IRangedAttackMob.h` | `RenderWitch.h/.cpp`<br>`ModelWitch.h/.cpp` | `/mob/villager/witch.png`<br>(64x128) | Hostil estándar en `spawnableMonsterList` (peso 5, min 1, max 1) en biomas de superficie. |
| **Zombie Aldeano & Bebé** | `EntityZombie.h/.cpp`<br>`ModelZombieVillager.h/.cpp` | `RenderZombie.h/.cpp`<br>`ModelZombieVillager.h/.cpp` | `/mob/zombie_villager.png`<br>(64x64) | 5% de zombies naturales spawnean como aldeanos; 5% como bebés. Infección de aldeanos al morir a manos de un zombie en dificultad normal (50%) o difícil (100%). |
| **Esqueleto del Wither** | `EntitySkeleton.h/.cpp`<br>`IRangedAttackMob.h` | `RenderSkeleton.h/.cpp`<br>`ModelSkeleton.h/.cpp` | `/mob/skeleton_wither.png`<br>(64x32) | Fortalezas del Nether (`MapGenNetherBridge`) con peso 10, grupo 4-4. En Nether (WorldProviderHell), 80% de esqueletos se inicializan como Wither Skeleton. |

---

## 3. Estado de Texturas en `assets.pak`

Se verificó el archivo `assets.pak` generado para la ISO de Xbox (`MCPK v1`, 1284 archivos):

- `assets/mob/bat.png` -> **PRESENTE** (64x64)
- `assets/mob/villager/witch.png` -> **PRESENTE** (64x128)
- `assets/mob/zombie_villager.png` -> **PRESENTE** (64x64)
- `assets/mob/skeleton_wither.png` -> **PRESENTE** (64x32)
- `assets/textures/items/skull_wither.png` -> **PRESENTE** (ícono de cabeza de Wither)

Todas las texturas requeridas están presentes en el pack de recursos y coinciden exactamente con los UV mappings de los modelos implementados.

---

## 4. Sonidos que faltan en `assets.pak` (Para futura tarea de audio 1.5.2)

Siguiendo la Regla 29, el código utiliza los nombres de sonido canónicos de Minecraft 1.5.2. Al inspeccionar `assets.pak`, se identifican los siguientes sonidos faltantes que deberán incorporarse en la tarea de audio:

1. **Murciélago (`EntityBat`)**:
   - `mob.bat.idle` (chillido ambiente)
   - `mob.bat.hurt` (daño)
   - `mob.bat.death` (muerte)
   - `mob.bat.takeoff` (aleteo al despegar del techo, SFX 1015)
2. **Bruja (`EntityWitch`)**:
   - `mob.witch.idle` (risa/gemido ambiente)
   - `mob.witch.hurt` (daño)
   - `mob.witch.death` (muerte)
3. **Zombie Aldeano (`EntityZombie`)**:
   - `mob.zombie.remedy` (sonido al darle la manzana dorada bajo debilidad)
   - `mob.zombie.infect` (sonido de conversión de aldeano a zombie, SFX 1016)
   - `mob.zombie.unfect` (sonido de cura a aldeano normal, SFX 1017)
4. **Esqueleto del Wither (`EntitySkeleton`)**:
   - Utiliza los sonidos existentes de `mob.skeleton.*` (say, hurt, death, step), los cuales sí están presentes en `assets.pak`.

---

## 5. Optimizaciones para Xbox Clásica (733 MHz, 64 MB RAM)

1. **Zero-Allocation en bucles de ticks**:
   - En `EntityBat`, `currentFlightTarget` se almacena como una estructura en línea `ChunkCoordinates` con bandera `hasFlightTarget`, evitando asignaciones `new/delete` en el heap en cada búsqueda de ruta de vuelo.
   - En `EntityWitch`, el temporizador de ataque y la poción sostenida se gestionan sin crear objetos temporales innecesarios.
   - En `EntityZombie`, la búsqueda de items arrojados en el suelo (`canPickUpLoot`) está regulada para evaluarse únicamente cada 20 ticks (1 segundo) cuando la mano está vacía, evitando consultas de AABB innecesarias por fotograma.
2. **Sistema de Tareas AI Desacoplado**:
   - Se extendió `EntityAITasks` con soporte para tareas no propietarias (`ownsAction = false`) y desregistro dinámico seguro (`removeTask`), permitiendo alternar entre arco (`EntityAIArrowAttack`) y espada (`EntityAIAttackOnCollide`) en `EntitySkeleton` sin duplicar instancias ni fugar memoria.
3. **Gestión de Memoria en Renderers**:
   - `RenderZombie` maneja tanto `ModelZombie` como `ModelZombieVillager` manteniendo la propiedad explícita de ambos y alternando el puntero activo de manera limpia, evitando fugas en el destructor base de `RenderLiving`.

---

## 6. Verificación de Compilación

Bajo estricto cumplimiento del protocolo `BUILD.lock`:

- **Perfil Release (`build\xbox-release`)**:
  - `.\ninja.exe -C build\xbox-release` -> **Exit code 0**
  - Generó `OptiCraft.xbe`, `default.xbe`, `OptiCraft_720p.xbe` y `OptiCraft.iso`.
- **Perfil Experimental (`build\xbox-release-exp`)**:
  - `.\ninja.exe -C build\xbox-release-exp` -> **Exit code 0**
  - Generó `OptiCraft_exp.xbe`, `default.xbe`, `OptiCraft_exp_720p.xbe` y `OptiCraft.iso`.

---

## 7. Commits y Git Diff Stat

```
commit e611f9c9d0a843d0ac97bd89ac56089a7aa97254
Author: Antigravity <agy@antigravity.local>
Date:   Sun Oct 25 16:14:08 2026 -0500

    feat(mobs): implement Minecraft 1.5.2 mobs (Bat, Witch, Zombie Villager, Wither Skeleton)

 src/net/minecraft/src/BiomeGenBase.cpp          |   6 +
 src/net/minecraft/src/BiomeGenBase.h            |   1 +
 src/net/minecraft/src/DamageSource.cpp          |  20 +-
 src/net/minecraft/src/DamageSource.h            |   4 +
 src/net/minecraft/src/Entity.h                  |   2 +
 src/net/minecraft/src/EntityAIArrowAttack.cpp   |  70 +++++-
 src/net/minecraft/src/EntityAIArrowAttack.h     |  12 +-
 src/net/minecraft/src/EntityAITaskEntry.cpp     |   9 +-
 src/net/minecraft/src/EntityAITaskEntry.h       |   3 +-
 src/net/minecraft/src/EntityAITasks.cpp         |  24 +-
 src/net/minecraft/src/EntityAITasks.h           |   3 +-
 src/net/minecraft/src/EntityAmbientCreature.cpp |   6 +
 src/net/minecraft/src/EntityAmbientCreature.h   |  12 +
 src/net/minecraft/src/EntityBat.cpp             | 250 ++++++++++++++++++++
 src/net/minecraft/src/EntityBat.h               |  52 +++++
 src/net/minecraft/src/EntityList.cpp            |   9 +
 src/net/minecraft/src/EntityLiving.h            |   1 +
 src/net/minecraft/src/EntityPotion.h            |   1 +
 src/net/minecraft/src/EntitySkeleton.cpp        | 220 ++++++++++++++++--
 src/net/minecraft/src/EntitySkeleton.h          |  36 ++-
 src/net/minecraft/src/EntityWitch.cpp           | 224 ++++++++++++++++++
 src/net/minecraft/src/EntityWitch.h             |  45 ++++
 src/net/minecraft/src/EntityZombie.cpp          | 293 +++++++++++++++++++++++-
 src/net/minecraft/src/EntityZombie.h            |  34 ++-
 src/net/minecraft/src/EnumCreatureType.cpp      |   1 +
 src/net/minecraft/src/EnumCreatureType.h        |   3 +-
 src/net/minecraft/src/IRangedAttackMob.h        |  11 +
 src/net/minecraft/src/ItemMonsterPlacer.cpp     |   1 +
 src/net/minecraft/src/MapGenNetherBridge.cpp    |   2 +
 src/net/minecraft/src/MathHelper.cpp            |  20 ++
 src/net/minecraft/src/MathHelper.h              |   2 +
 src/net/minecraft/src/ModelBat.cpp              | 106 +++++++++
 src/net/minecraft/src/ModelBat.h                |  28 +++
 src/net/minecraft/src/ModelBiped.cpp            |  72 ++++--
 src/net/minecraft/src/ModelBiped.h              |   1 +
 src/net/minecraft/src/ModelRenderer.cpp         |  35 ++-
 src/net/minecraft/src/ModelRenderer.h           |   3 +
 src/net/minecraft/src/ModelVillager.cpp         | 104 +++++----
 src/net/minecraft/src/ModelVillager.h           |  19 +-
 src/net/minecraft/src/ModelWitch.cpp            |  94 ++++++++
 src/net/minecraft/src/ModelWitch.h              |  26 +++
 src/net/minecraft/src/ModelZombieVillager.cpp   |  53 +++++
 src/net/minecraft/src/ModelZombieVillager.h     |  15 ++
 src/net/minecraft/src/Potion.cpp                |  21 +-
 src/net/minecraft/src/Potion.h                  |   1 +
 src/net/minecraft/src/RenderBat.cpp             |  33 +++
 src/net/minecraft/src/RenderBat.h               |  14 ++
 src/net/minecraft/src/RenderBiped.cpp           |   6 +-
 src/net/minecraft/src/RenderBiped.h             |   3 +-
 src/net/minecraft/src/RenderGlobal.cpp          |  18 ++
 src/net/minecraft/src/RenderManager.cpp         |  18 +-
 src/net/minecraft/src/RenderSkeleton.cpp        |  24 ++
 src/net/minecraft/src/RenderSkeleton.h          |  14 ++
 src/net/minecraft/src/RenderWitch.cpp           | 109 +++++++++
 src/net/minecraft/src/RenderWitch.h             |  21 ++
 src/net/minecraft/src/RenderZombie.cpp          |  55 +++++
 src/net/minecraft/src/RenderZombie.h            |  27 +++
 src/net/minecraft/src/SpawnerAnimals.cpp        |   4 +
 src/net/minecraft/src/StringTranslate.cpp       |   5 +-
 src/net/minecraft/src/World.cpp                 |   4 +
 src/net/minecraft/src/World.h                   |   1 +
 61 files changed, 2168 insertions(+), 143 deletions(-)
```
