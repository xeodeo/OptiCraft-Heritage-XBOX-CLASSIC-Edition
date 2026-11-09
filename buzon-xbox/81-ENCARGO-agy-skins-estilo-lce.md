# 81. Encargo agy: selector de skins al estilo LCE (09-11) — EN COLA tras el 80

Pedido del usuario: la pantalla de elegir skin se ve «rara»; quiere una más LCE. (Medido por Claude: `GuiSkinSelector` no
se toca desde el 28-09 y es igual en 1.2.5; el diseño viene de OptiJuegos. No es una regresión.)

## Dónde
- Worktree y rama: Claude te lo dirá al lanzarlo (sobre lo último de la rama de prueba). SIN reset.
- Pantalla actual: `src/net/minecraft/src/skin/GuiSkinSelector.cpp` (946 líneas), `SkinManager`, `GuiLoadSkins*`.
  La usan también la PS2 y el jugador 2 en pantalla partida: no los rompas.
- Referencia LCE (solo leer, nunca modificar): `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\minecraft-legacy original\
  Minecraft.Client\Common\UI\UIScene_SkinSelectMenu.*` y `UIControl_PlayerSkinPreview.*` (y su escena/colores/geometría).
  Estilo y escalado 1280x720 como `legacy/LegacyCraftingScreen.cpp` y `legacy/LegacyTradingScreen.cpp`.

## Qué hacer
1. Con `legacyUI` activo: pantalla nueva al estilo LCE (carrusel de skins a izquierda/derecha con el jugador en 3D
   girando en el centro, nombre de la skin y del paquete, pestañas/paquetes arriba si los hay, iconos de botones abajo).
   Con `legacyUI` apagado: la pantalla actual tal cual (dos interfaces, regla del proyecto).
2. Misma lógica de fondo que ahora (`SkinManager`: lista, elegir, guardar la elegida, skins que faltan → Steve).
3. Mando completo (izq/der cambia skin, LB/RB o arriba/abajo cambia paquete, A elegir, B volver) y pantalla partida (J2).
4. Que se vea bien a 480p y a 720p. Sin bajar fps (la vista 3D solo se dibuja en esta pantalla).

## Trampas
- Texto encima de modelos/iconos: sin depth test. Restaurar estados de render al salir (color, blend, cull).
- Dueño de cada textura de skin cargada; nada de fugas al pasar muchas skins.

## Compilar y entregar
- `BUILD.lock`; `cmake --build build/xbox-release` y `build/xbox-release-exp`. NO copies al lanzador.
- Capturas en xemu a 480p y 720p si puedes, en `buzon-xbox\81-capturas\`.
- Informe `buzon-xbox\81-INFORME-agy-skins-lce.md`. Señal final literal: `ENCARGO 81 LISTO <commit>`
