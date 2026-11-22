# 106. Encargo agy-2: auditoría PS2 — qué hace falta para llevar todo lo nuevo a la PS2 (22-11)

El usuario quiere, más adelante, pasar a la PS2 lo que hemos hecho en Xbox, y le preocupa. **Este encargo es SOLO
revisar y ANOTAR**, no portar. Carpetas nuevas solo tuyas: 1.2.5 `wt-125-ps2` (rama `opti30-ps2`, base `c66352d` =
opti30-erluza con todo + mods) y 1.5.2 `wt-152-ps2` (rama `port152-ps2`, base `ac018fe` = 1.5.2 con todo + mods).
No toques otras carpetas. Sin push, sin reset. Si para ver errores haces arreglos triviales de compilación (un
`#if`, un include), commitéalos en esas ramas con `git add <fichero>` y anótalos; nada más grande.
Candado `buzon-xbox\BUILD.lock` para compilar (lo comparte agy-1).

## Contexto
- La PS2 tiene 32 MB de RAM (Xbox 64 MB), GPU GS muy distinta (dibujado propio: `src/ps2/render/`, `Ps2GreedyMesh`,
  `WorldRendererPs2`), red propia, mando DualShock, USB. Base PS2 que funcionaba: erluza / OptiJuegos 1.2.5.
- Preset `ps2-release` en `CMakePresets.json` con la toolchain incluida (`psdevwindows/ps2sdk`). El último arreglo
  para que compile en PS2 fue `5802c26` en `xbox-unificada` (desde entonces hemos tocado muchísimo).
- Lo hecho desde entonces (lista en `buzon-xbox\ESTADO-ACTUAL.md` y encargos 87–105): arreglos de erluza/ramide1,
  personaje animado, icono de guardado, chat/teclas, texturas .zip, skins online (red Xbox + DNS), mods (catálogo,
  minimapa, X-Ray, luces dinámicas, Herobrine, ChocoCraft…), y en 1.5.2 todo el port (aldeanos, comandos, Wither,
  fuegos artificiales, dispensador, generador, encantar, mapas…).

## Qué hacer
1. Compilar `ps2-release` en las dos carpetas y apuntar TODOS los errores, agrupados por causa.
2. Recorrer el código nuevo y clasificar cada función/fichero: **común** (sirve igual en PS2), **necesita equivalente
   PS2** (red `XboxNetwork`/HTTP/DNS, pantalla partida `XboxSplitScreen`, combos de mando, `XboxCraftingScreen`,
   iconos de botones, rutas `T:`/`D:`, zip/minizip, D3D8, partes del X-Ray/minimapa que tocan el dibujado), o
   **no viable en PS2** y por qué.
3. **Memoria**: estimar cuánto suma cada cosa nueva (texturas, mods, tablas de 1.5.2, atlas) frente a lo que la PS2
   tiene libre hoy con la 1.2.5 de erluza; decir qué cabe y qué habría que recortar. Opinión razonada sobre la 1.5.2
   en PS2 (¿viable? ¿con qué recortes?).
4. Plan por pasos para llevarlo a PS2 (primero 1.2.5), con orden, riesgo y tamaño de cada paso.

## Entrega
`buzon-xbox\106-INFORME-agy-auditoria-ps2.md` (en español, con fichero:línea). La señal de fin es la palabra
CIENTOSEIS-LISTO seguida del último commit (o de «sin-commit» si no tocaste nada).
