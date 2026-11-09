# 39 — Release v1.3 montada (08-10, 19:10)

`Desktop\release\v1.3\`: zip (OptiCraft\ con lanzador + 4 XBE públicos sin log + data),
ISO (arranca en el lanzador en xemu, captura 39-v13-xemu.png) y zip aparte de packs
(`OptiCraft\texturepacks\*.pak`, 11 packs). RELEASE.md escrito. Rama wt-opti30, HEAD efe62a7.
MD5 (19:21, con Legacy Look): zip 40bb095c…, iso 839e496d…, packs 28562568….

Falta: probar el zip en la consola, push a `xbox` y tag `xbox-v1.3` (el tag remoto `xbox-v1.2`
apunta mal, a 965efdb). **Ojo:** el zip de packs lleva los DLC de Legacy (Halo, Skyrim, Mass
Effect...), contenido de pago de terceros: no deberían ir en la release pública de GitHub.

La v1.2 vuelve a ser la original; la de pruebas con lanzador quedó en `v1.2-borrador-con-lanzador`.

## Ideas de Legacy (informe del agente, para después)
- "Legacy Look" está activado en Xbox pero no hace nada: falta `SetGammaRamp` con pow(x,0.8)
  (`renderSetLegacyPresentationGamma` vacía en RenderAPI_D3D8_XBOX.cpp) y el `#if PLATFORM_PS2`
  de EntityRenderer.cpp:1418. Coste cero. Niebla desde 0,25·dist, sin viñeta.
- Nubes hasta el horizonte: niebla propia para nubes y plano lejano >= 512.
- Generación sin cambiar terreno: floor_double más rápido, calcular solo el ruido que se usa
  (blend<0 / >1), quitar dos borrados de 32 KB. Verificar con hash de chunks por semilla.

## 08-10 19:15 — Legacy Look en Xbox (commit tras efe62a7, NO entra en la release v1.3 montada)
- `renderSetLegacyPresentationGamma` hecho con `SetGammaRamp` (x^0.8, LegacyLook.h); se aplica
  cada fotograma según la opción, también a menús y HUD. Con Legacy Look no hay viñeta.
- Antes Legacy Look no hacía nada en Xbox (todo su código era de PS2/Wii/PC): no bajaba FPS.
- Si el usuario lo ve muy claro: el Brillo de Xbox viene al máximo por defecto; Legacy usa 50 %.

## Generación más rápida (PENDIENTE, anotado a petición del usuario)
Sin cambiar el terreno de una semilla (comprobar con hash de chunks antes/después):
1. `MathHelper::floor_double` / `NoiseGeneratorPerlin.cpp:12-18`: floor por el truco de
   6755399441055744.0 en vez de doubleToInt (NaN + rangos + cambio de modo FPU).
2. `ChunkProviderGenerate::initializeNoiseField` (:770-773, :880-889): si blend<0 solo el ruido
   low, si >1 solo high; sumar octavas en el mismo orden y guardar en double.
3. Quitar los borrados sobrantes de 32 KB (:358 std::fill, :522 data.assign).
Además: el hilo de fondo va al 100 % con ~2 chunks/s; probar 30 FPS Limit.

## 08-10 19:40 — Generación: método y primeras medidas
- Prueba de "no cambia nada": build autotest (wt-opti30/build/xbox-autotest, netlog a .18) crea
  siempre el mundo con semilla 20261008 y registra `[genhash] x z hash` (bloques tras cuevas y
  barrancos + biomas). `xbox-spike\genhash_run.ps1 -Out <f>` lo lanza en xemu y guarda las
  huellas. Referencia: `xbox-spike\genhash_base.txt` (131 chunks). Dos referencias seguidas:
  131/131 iguales. Toda optimización debe dar 131/131 iguales.
- Reparto por chunk en xemu (xbox.gen): ruido de densidad ~11 ms (la mitad), biomas ~5 ms,
  cuevas ~3-4 ms, superficie ~1,6 ms, relleno ~0,9 ms, barrancos ~0,3 ms. En la consola real
  sale en el log de cualquier build de prueba (línea xbox.gen).

## 09-10 — Primera optimización de generación (commit f316ecf)
- NoiseGeneratorPerlin: Y y Z (celda, fracción, suavizado) calculados una vez por octava en vez
  de por columna. Huellas 131/131 idénticas a la referencia. Ruido 11 → 7 ms por chunk en xemu
  (total ~22 → ~18 ms). Sin probar en consola. NO está en la release v1.3 montada.
- Siguiente: biomas (~5-6 ms), cuevas (~3-4 ms).
