# 114. Encargo agy: el minimapa a veces se redibuja entero (1.2.5 y 1.5.2) (22-11)

Visto por el usuario en la consola real (22-11 18:25): el minimapa (Rei + Xaero, encargo 101, `src/mods/reiminimap/`)
funciona bien, pero **a veces se vuelve a redibujar** (se ve cómo se repinta entero / parpadea).

## Qué mirar
- `ReiMinimap::update()` / `uploadPlayerTexture()`: el refresco por filas (`ROWS_PER_TICK`) debería ir poco a poco;
  buscar qué reinicia el mapa completo: cambio de chunk, de dimensión, de zoom, al cargar/descargar chunks, al cambiar
  de modo cueva Auto (entrar/salir de techo), al recrear la textura, al cambiar de resolución/pantalla partida,
  o cuando la textura se pierde y se vuelve a subir.
- Conservar lo ya dibujado al moverse (desplazar el buffer y pintar solo la franja nueva, como hace Xaero), y no
  vaciarlo en los cambios que no lo requieren. Si el modo cueva Auto alterna en el borde de una cueva, añadir
  histéresis.
- Medir: con el jugador andando/volando 2 min, cuántas veces se redibuja entero (log) antes/después; fps.

Ramas `port152-int` y `opti30-erluza` (carpeta que diga Claude). Sin reset, sin push; `git add <fichero>`; candado.
No cambies el aspecto de su menú (lo restiliza a LCE el subagente de Claude, informe 112).
Informe `buzon-xbox\114-INFORME-agy.md`; la señal de fin es la palabra CIENTOCATORCE-LISTO seguida de los commits.
