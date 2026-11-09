# Encargo 70 (agy): cruceta configurable como la Legacy + iconos del joystick con su dirección

Claude ACEPTÓ el 67 (está en `port152-claude`), PERO con un arreglo: tu cambio dejó el stick izquierdo sin mover al
jugador (ligaste andar a las teclas 17/30/31/32 que nada pulsa en consola). Claude lo arregló en `db76200`/`1657f4b`:
el stick tiene sus teclas sintéticas `XBOX_KEY_LSTICK_UP/DOWN/LEFT/RIGHT` (y `PS2_KEY_LSTICK_*`). Mira ese commit.
Lección: prueba SIEMPRE que lo básico (andar, saltar, abrir inventario) sigue funcionando tras tocar controles.

## Tu sitio
`wt-152-agy`. ANTES de nada: `git reset --hard port152-claude` (ya contiene todo lo tuyo + arreglos de Claude;
árbol limpio, no hay nada tuyo sin integrar). Reconfigura: `"$C" -S . -B build/<p> "-DXBOX_VERSION_LABEL=1.5.2"`.

## Lo que dijo el usuario probando en la consola (03-11 18:00)
- «izquierda y derecha cambian el objeto de la barra» → bien.
- «arriba cambia la perspectiva, pero si ya está en el botón del joystick, ¿pa qué?» → quitar ese duplicado.
- «que funcione bien la cruceta y DEPENDA DE LA CONFIGURACIÓN»: cada flecha debe ser una acción que se pueda
  reasignar en Opciones > Controles, como cualquier otra.
- «los iconos del joystick deben ser alusivos: joystick hacia arriba, abajo, etc., como la LCE».
  LCE está en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\minecraft-legacy original` (solo leer).
  Iconos LCE: `Minecraft.Client\Common\Media\Graphics\X360ControllerIcons\` (`ButtonLeftStick*.png`, `ButtonDpad*.png`).

## Qué hacer
1. **Acciones nuevas reasignables** (`KeyBinding` en `GameSettings`, con texto traducible como las demás, y en la
   pantalla de Controles Legacy `LegacyControlsScreen`):
   - «Objeto anterior» / «Objeto siguiente» (barra) → por defecto cruceta IZQ / DER.
   - «Cambiar perspectiva» (lo que hace F5) → por defecto el CLIC DEL STICK DERECHO (lo que ya hacía). La cruceta
     arriba ya NO cambia la cámara.
   - «Chat» → por defecto cruceta ABAJO (ya va: B cierra el chat y el mensaje sale en pantalla, lo arregló Claude).
   - Cruceta ARRIBA: por defecto como la LCE en creativo volando = subir, y cruceta ABAJO volando = bajar
     (mira `LocalPlayer.cpp` ~395-465 de la LCE: `ullDpad_filtered` DPAD_UP/DOWN → flyY ±0.1). Si choca con Chat en
     abajo, en vuelo creativo manda el vuelo y en el suelo el chat; explícalo en el informe.
   Todo eso hoy está «a pelo» en `XboxInput.cpp` (`pushKeyEdge(... KEY_F5)`, `KEY_T`, `pushWheel`) y en
   `Ps2InputMapper.cpp`: pásalo a que lo decida la acción ligada (como saltar/soltar), no el botón físico.
2. **Iconos de dirección del stick**: en Controles, «Adelante/Atrás/Izquierda/Derecha» (teclas `*_LSTICK_*`) deben
   verse con el icono del stick izquierdo CON LA FLECHA de su dirección (estilo LCE). Añade los iconos al atlas de
   botones (`scripts/xbox/make_button_atlas.py` → `data/assets/gui/buttons_xbox.png` + `XboxButtonAtlasData.h`;
   recuerda: el usuario copia a veces solo los XBE, por eso el PNG también va embebido en el XBE — mira cómo se hizo
   en v1.1) y el mapeo en `LegacyButtonPrompt::iconFromName`. Haz lo mismo con las 4 flechas de la cruceta si no las hay.
3. **Pantalla partida**: jugador 2 con las mismas acciones (`XboxSplitScreen.cpp` ~260 tiene la cruceta a pelo).
4. **Inventarios**: la cruceta sigue moviendo por casillas EXACTAMENTE igual (el usuario lo confirmó bien; no lo rompas).
5. Opciones guardadas viejas: que se migren sin perder lo que el usuario cambió.

## Reglas
Candado `buzon-xbox\BUILD.lock`, compilar `xbox-release` y `xbox-release-exp`; si tocas PS2 dilo (no hace falta
compilar PS2 si no puedes). Nunca /fp:fast, no push, no ClassicXboxLoader, no xemu. Commits pequeños.
Antes de entregar: repasa a mano que andar con el stick, saltar, atacar/usar, inventario y soltar siguen ligados.

## Al acabar
Informe `buzon-xbox\70-INFORME-agy-cruceta.md` (tabla: acción → botón por defecto Xbox / PS2) y en tu terminal:
`ENCARGO 70 LISTO <commit>`.

## CAMBIO DEL USUARIO (03-11 18:15) — manda sobre lo de arriba
La cruceta IZQ/DER NO debe cambiar de objeto por defecto. Las 4 flechas hacen SOLO lo que el usuario les ponga en
Controles. Por defecto:
- IZQ y DER: sin acción (libres). «Objeto anterior/siguiente» existen como acciones pero por defecto en LB/RB
  (Blanco/Negro), donde ya estaban.
- ARRIBA/ABAJO: subir/bajar volando en creativo (como LCE) y nada más. El Chat como acción reasignable, sin botón por
  defecto en la cruceta si choca (dile al usuario en el informe dónde quedó).
Cualquier acción del juego (perspectiva, chat, objeto ant./sig., soltar, inventario...) se tiene que poder poner en
cualquier flecha desde Controles.
