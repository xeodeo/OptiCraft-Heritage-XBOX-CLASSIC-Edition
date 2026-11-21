# 96. (HECHO por Claude 21-11: c027da6 en 1.2.5, 016fe22 en 1.5.2; sin opción en menús todavía) Encargo agy: personaje animado en el HUD, como la LCE (1.2.5 y 1.5.2) (20-11)

Pedido por el usuario: el muñequito del jugador arriba a la izquierda que sale al agacharse, correr o volar, como en
la Legacy Console Edition. No existe en ninguna de las dos ramas (buscado: nada parecido en `GuiIngame`).

Ramas: 1.2.5 `opti30-erluza` (`wt-opti30`) y 1.5.2 `port152-erluza` (`wt-152-rend`). Sin reset, sin push, sin copiar
al lanzador. Reglas de siempre: BUILD.lock, 60 fps, memoria (64 MB), creativo NO se toca, terreno igual, dos
interfaces (Java y Legacy `legacyUI`) para la opción, pantalla partida.

## Referencia LCE (solo lectura, permitido usarla)
`Desktop\Minecraft en xboox clasico\minecraft-legacy original\Minecraft.Client\Gui.cpp` líneas ~564-668
(«4J - do render of crouched player»):
- Opción `eGameSetting_AnimatedCharacter` («Personaje animado», activada por defecto).
- Temporizador por jugador: agachado o corriendo → 30 ticks; volando → 5; si no, baja de 1 en 1. Se ve mientras esté
  agachado/corriendo/volando o el temporizador > 0; no se ve con el HUD oculto.
- Se dibuja el jugador como en el inventario (`renderEntity`/`RenderManager` con rotación fija): escala 12,
  `glTranslatef(xo + 51, yo + 75, 50)`, cabeza quieta = cuerpo, sin fuego, `heightOffset`, luces del inventario.
- Posiciones por sección de pantalla (completa, partida arriba/abajo/izquierda/derecha, cuadrantes): tabla `xo/yo`
  del switch. Respeta nuestra zona segura del HUD (720p y 480).

## Qué hacer
- Implementarlo en `GuiIngame` de las dos ramas (con la skin del jugador, capa y armadura como en el inventario).
- Opción «Personaje animado» en Opciones (Java y Legacy) guardada en options.txt; por defecto activada.
- Pantalla partida: cada jugador su muñeco en su sección.
- Coste: medir fps con el muñeco visible (corriendo) frente a oculto; no debe bajar de forma apreciable.

## (El icono de guardado ya lo hizo Claude: d84e996/1973ac2 en 1.2.5, 3eff900/0a60f6a en 1.5.2. No tocarlo.)

## Aceptación
- Capturas: agachado, corriendo, volando (creativo), en 480 y 720p, y en pantalla partida, en las dos versiones.
- Se oculta ~1,5 s después de dejar de agacharse/correr y casi al instante al dejar de volar.
- fps antes/después. Capturas en `buzon-xbox\96-capturas\`.

## Informe
`buzon-xbox\96-INFORME-agy.md`. Señal: `ENCARGO 96 LISTO` + commits de las dos ramas.
