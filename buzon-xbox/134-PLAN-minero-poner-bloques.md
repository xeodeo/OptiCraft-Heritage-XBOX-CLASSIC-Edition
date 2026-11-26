# 134 — Minero (bot de minería): estado y plan para que ponga bloques (26-11-2026, Claude)

## Estado (dónde está y qué va)
- Código: `src/mods/adminmode/AdminMiner.cpp/.h` (escrito por agy, encargo 133; terminado y probado por Claude).
  Integrado SIN COMMIT en `wt-build-152` (1.5.2 buena) y copia igual en `wt-agy-152` (rama agy-mineria).
  En la consola: build con Minero + comer solo (subida 26-11 ~13:00). Compiladas SIN SUBIR (falta de memoria del PC
  mató la subida): pico más barato primero + salir del agua + no ir a menas con lava cerca.
- Uso: modo admin (Back+X) → Mundo → **Minero** → elegir mineral. Lleva pico y comida en la barra rápida.
- Probado en xemu contra el Paper 1.12.2 local (builds de prueba con `D:\minero.txt`, que enciende mod+Minero solo):
  - 2 min: de y=71 a y=53 en escalera, **28 menas de hierro**, sin morir, memoria estable ~18 MB libres.
  - Con 2 fuentes de agua echadas encima a mitad: **39 menas**, se quedó ~15 s parado y siguió.
- Hecho después de la prueba (por petición del usuario):
  - **Come solo** (< 7 muslos, comida de la barra rápida, mantiene "usar"); para si hambre ≤ 3 y no hay comida. SIN PROBAR.
  - **Pico más barato que sirva** (el más lento de los eficaces): piedra/madera → hierro → diamante.
  - **Agua**: el agua cuenta como paso caro (4) y como sitio válido para empezar un camino; atascado en agua → nada
    hacia arriba y se aparta de lado (`stuck in water ... swimming out` en el log).
  - **Lava**: no elige menas con lava a ≤ 2 bloques (`lavaNear`); las abandonadas se apartan 1-2 min (antes 10-20 s).
- Fallos/avisos conocidos: el contador `mined` sale 0 en servidor (las menas sí llegan); el usuario vio que una
  corriente lo dejaba recalculando (arreglado arriba, sin confirmar en la real) y que se quedaba mirando menas con lava
  (arreglado arriba, sin probar).

## Qué hace Baritone (leído en su código, master: MineProcess.java, MovementHelper.java, Settings.java)
- `prune()` descarta menas "encased in bedrock, or touching lava"; `avoidBreaking`/`avoidAdjacentBreaking`: no rompe un
  bloque con líquido fuente al lado o justo encima.
- Si no hay camino: `blacklistClosestOnFailure=true` → lista negra PERMANENTE de la mena más cercana. Se rinde rápido:
  `failureTimeoutMS=2000`, `planAheadFailureTimeoutMS=5000`, `movementTimeoutTicks=100`.
- Herramienta: la MÁS RÁPIDA (`switchToBestToolFor`, `autoTool=true`); `itemSaver=false` (deja una herramienta antes de
  romperse). Lo de "la más barata" es petición del usuario, no de Baritone.
- NO tapa lava en la minería. Sí pone bloques para puentes/pilares: `allowPlace=true`,
  `acceptableThrowawayItems` = dirt, cobblestone, netherrack, stone; `blockPlacementPenalty=20`;
  `MovementHelper.attemptToPlaceABlock`.

## PLAN (siguiente): que el Minero ponga bloques
Pieza que ya existe: el módulo **Build** pone bloques de verdad también en servidor:
`mc->playerController->sendPlaceBlock(p, w, hand, nx, ny, nz, face)` (`AdminModeMod.cpp` ~1519).
1. **Bloques de relleno**: tierra, roca, piedra, netherrack del inventario (como `acceptableThrowawayItems`). Pasar a la
   barra rápida si hace falta (clic de inventario) o usar solo los de la barra.
2. **Tapar lava/agua**: antes de romper un bloque, si un vecino (o el de encima) es lava/agua FUENTE → poner un bloque de
   relleno en esa casilla (o en el hueco que se abriría) y luego romper. Con eso se pueden sacar las menas junto a lava
   (quitar el filtro `lavaNear` cuando haya relleno; mantenerlo si no hay).
3. **Puentes y pilares (scaffold)**: si el camino cruza un hueco de ≥ 2 de profundidad o lava en el suelo, poner bloque
   bajo los pies (puente); para subir sin escalera, salto + bloque debajo (pilar). Coste extra en el A* (≈ 20 como
   Baritone) para preferir caminos sin poner.
4. Ideas de Baritone pendientes: rendirse al buscar camino (~2 s) y apartar la mena para siempre; no romper con líquido
   fuente al lado/encima; `itemSaver` (no romper el pico de diamante).
5. Referencias de los clientes que pasó el usuario:
   - **creepy-salhack** (SalHack 1.12.2, github.com/CreepyOrb924/creepy-salhack): `module/world/ScaffoldModule.java`,
     `AutoBuilderModule.java`, `AutoTunnelModule.java`, `NukerModule.java`; `module/exploit/MountBypassModule.java`.
   - **wwe.jar** (cliente WWE 1.12.2, Downloads): ofuscado y con textos cifrados, trae **Baritone** dentro (mejor leer
     Baritone del repo oficial, LGPL-3: ideas, no copiar).
6. Probar igual que el 26-11: Paper local + `D:\minero.txt`; escenas: mena con lava al lado, río subterráneo, hueco
   que hay que cruzar. Medir menas/min, muertes, memoria libre, y que no rompa nada si no hay relleno.
7. Después: llevarlo a la **1.2.5** (mismo modo admin) y a la **1.6.4**.

## Hecho el 26-11 tarde (Claude) — SIN PROBAR, compilado 1.5.2 (normal + exp)
- **Tapa líquidos con su propia piedra**: antes de romper la mena o un bloque del camino, si al lado (arriba o los
  4 lados) hay lava/agua, pone un bloque de relleno haciendo clic en esa cara (`coverLiquidAround`,
  `sendPlaceBlock`). Relleno de la barra rápida: tierra, **roca (la que va sacando)**, piedra, netherrack
  (`findFillerSlot`). Con relleno ya no descarta menas con lava cerca; el A* cobra 20 por bloque a tapar (como
  Baritone). Sin relleno se comporta como antes. Log: `covered liquid at (x,y,z) with slot N`.
- **itemSaver**: no usa una herramienta con < 8 usos si hay otra que sirva.
- **Agua (módulo Agua)**: sin empuje de corriente (`World::handleMaterialAcceleration`) y nado al doble
  (`EntityLiving::moveFlying` 0.04). El Minero también ignora la corriente.
- **NoFall con opción "Cubo de agua"** (modo Bucket de SalHack): cayendo rápido (motionY < -0.6) con suelo a 1-4
  bloques, coge el cubo de agua de la barra, mira abajo y lo vacía; al tocar suelo lo recoge y vuelve al slot.
  Para el server que aún hace daño con "Paquete" (anticheat que cuenta la caída él mismo).
- Lo que tienen los clientes (SalHack): NoFall Packet/Anti/Bucket/Elytra; Jesus = nuestro Agua; Velocity NoPush =
  sin empuje de agua; Speed `SpeedInWater`; Scaffold = clic en la cara opuesta de un vecino sólido (lo mismo que
  usamos para tapar).
- Falta: puentes/pilares (scaffold en el camino), rendirse rápido + lista negra permanente, contador `mined`,
  pasarlo a 1.2.5.
