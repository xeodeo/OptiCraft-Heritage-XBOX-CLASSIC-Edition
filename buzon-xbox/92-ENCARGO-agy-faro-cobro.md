# 92. Encargo agy: el faro (beacon) da el efecto sin cobrar el pago (1.5.2) (20-11)

Rama `port152-erluza` (`wt-152-rend`, último `9ca1a59` de Claude). Sin reset, sin push, sin copiar al lanzador. Reglas
de siempre (BUILD.lock, 60 fps, memoria, creativo no se toca). Solo 1.5.2 (la 1.2.5 no tiene faro).

## Fallo (detectado por el usuario 19-11)
El faro activa el efecto SIN consumir el objeto de pago (lingote de hierro/oro, esmeralda o diamante). En Java 1.5.2,
al pulsar «Hecho» con un efecto elegido, `GuiBeacon.actionPerformed` manda el paquete `MC|Beacon` con los efectos;
`NetServerHandler` («MC|Beacon») comprueba `ContainerBeacon`/`SlotBeacon`, consume el objeto con
`decrStackSize(0, 1)` y aplica los efectos al `TileEntityBeacon`. Sin objeto en la casilla de pago no se puede
confirmar (el botón «Hecho» está desactivado).

Mira `GuiBeacon.cpp`, `ContainerBeacon.cpp`, `SlotBeacon.cpp`, `TileEntityBeacon.cpp` contra `mcp-diff/src152`
(`GuiBeacon`, `ContainerBeacon`, `SlotBeacon`, `TileEntityBeacon`, `NetServerHandler.handleCustomPayload "MC|Beacon"`).
Las dos interfaces (Java y la de consola/Legacy si el faro tiene pantalla Legacy). Pantalla partida (jugador 2) también.

## Aceptación
- Con pago: el efecto se activa y el objeto desaparece (1 unidad de la pila).
- Sin pago: «Hecho» desactivado / no se puede confirmar.
- Cambiar de efecto vuelve a cobrar; cerrar sin confirmar no cobra.
- Guardar y cargar conserva el efecto elegido.
- Capturas del backbuffer en `buzon-xbox\92-capturas\`.

## Informe
`buzon-xbox\92-INFORME-agy.md`. Señal: `ENCARGO 92 LISTO` + commit final.
