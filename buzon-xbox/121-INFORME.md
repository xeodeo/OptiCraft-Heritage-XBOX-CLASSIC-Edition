# 121. Informe: entrar al SMP 1.12.2 (comunidad 3DS) desde la Xbox — traductor 1.2.5 ↔ 1.12.2

## Resumen
- El cliente de Xbox (1.2.5 y 1.5.2, los dos hablan el protocolo 29) ya entra a servidores **Java 1.12.2
  (protocolo 340), offline-mode**, con un traductor dentro del cliente (`ProtocolTranslator340*`). Arquitectura de la
  de erluza (traductor 1.8.9 enganchado en `NetworkManager`), pero hecho directamente para 1.12.2, que es lo que exige
  el SMP (`minecraft3ds.org` rechaza 1.8.9).
- **Probado en el SMP real** (`minecraft3ds.org` → SRV → `usc-tx-p30-a11-s.kineticpanel.net:25568`): entra, se ve la
  spawn con sus construcciones, jugadores, chat del servidor y el aviso de AuthMe («Please register… /register
  <password> <ConfirmPassword>»). Para registrarse basta escribir `/register clave clave` en el chat.

## Commits
- `wt-125-net`, rama `opti30-mp`: `37471edb` (traductor + selector de versión + SRV) y `a5ee300d` (ventana de
  chunks adaptativa a la memoria, carteles guardados por columna, bloques/mobs/ítems de 1.5.2).
- `wt-152-net`, rama `port152-mp`: `8716db26`, los mismos ficheros (idénticos entre árboles salvo el texto
  «Minecraft 1.5.2» del menú).
- Sin push, sin lanzador. Banco de pruebas y servidor de repetición en el scratchpad (`h340\`), no en el repo.

## Qué hay
- `ProtocolTranslator340.{h,cpp}`: primitivas, chat JSON → texto con colores (incluye `translate`/`with` de
  uniones, muertes, susurros…), tabla de bloques 1.12 → 1.2.5 (ids 125–255 y metas nuevas al más parecido: hormigón,
  terracota y cajas de shulker → lana de su color, losas/escaleras/troncos nuevos → las de roble/piedra, camino → tierra
  de cultivo, etc.; el 95 «cristal tintado» → cristal, porque en 1.2.5 el 95 es el cofre bloqueado), biomas 23+ → los
  0–22, ítems nuevos → el equivalente o vacío, NBT, slots (NBT re-gzip), y **decodificación de secciones con paleta
  directamente al formato del Packet51 nativo** (una sola copia; sin zlib intermedio: `Packet51MapChunk::setDecoded`).
  En el árbol 1.5.2 detecta solo (sonda del registro de bloques) que existen los bloques 125–158, losas superiores,
  escaleras invertidas, murciélago/bruja/wither, y los deja tal cual.
- `ProtocolTranslator340Packets.cpp`: login offline + compresión, keepalive (solo los que pidió el servidor: el 1.2.5
  manda keepalive 0 por su cuenta y Paper echaba por eso), chat (líneas largas partidas; action bar fuera), posición y
  mirada con **teleport confirm**, tiempo, salud, comida, xp, respawn, lluvia, modo de juego, spawn, chunks, descarga
  de chunks, cambios de bloque (simple y múltiple), jugadores (nombre por la lista de jugadores), mobs (los que no
  existen → el más parecido: caballo → vaca, bruja → aldeano, guardián → calamar…), objetos (barcos, vagonetas,
  flechas, bolas de nieve, TNT, arena que cae, pociones, ítems tirados), metadatos (fuego/agachado/sprint, bebés,
  ovejas, lobos, creepers, slimes, endermans…), movimientos relativos 1/4096 → 1/32 sin deriva (teleport si no cabe),
  pasajeros, efectos, animaciones, equipo, inventario y ventanas (cofre, mesa, horno, dispensador/soltador; las demás
  se cierran en el servidor con aviso), clic/transacciones/creativo/encantar, carteles (texto JSON de los block
  entities del chunk y de Update Block Entity), notas y pistones, sonidos de mundo (efectos), explosiones, cama.
  Al entrar manda `MC|Brand vanilla` + Client Settings como el cliente vanilla (**sin la marca el SMP echaba con
  «Internal server error»**).
- `ProtocolTranslator340IO.{h,cpp}`: tramas con VarInt + zlib, paquetes nativos por la fábrica normal, escritor que
  inyecta lo que pide el traductor (teleport confirm, settings), ping de estado 1.7+.
  **Ventana de chunks**: el SMP manda distancia 10 (441 columnas, ~32 MB decodificadas). Todas las columnas se guardan
  comprimidas (~4 KB cada una, ~1,9 MB las 441) y solo las de radio 5 alrededor del jugador están vivas en el mundo
  nativo; al moverse se descargan/vuelven a mandar desde la copia (con los cambios de bloque y carteles que llegaron
  entretanto). Si la memoria libre baja de 9 MB el radio se reduce solo (mín. 2) y vuelve a crecer con >16 MB.
- Multijugador: al añadir/editar servidor hay botón **«Server version: Auto / 1.2.5 / 1.12.2»** (la pantalla es la
  misma en interfaz Java y Legacy; no hay versión Legacy aparte de esa pantalla). Auto hace el ping 1.7+: si responde →
  1.12.2, si no → 1.2.5 nativo. Los servidores guardados antes quedan como 1.2.5. En la lista, los 1.12.2 muestran
  `[1.12.2]` + MOTD + jugadores. **Registros SRV**: si no se pone puerto se sigue `_minecraft._tcp` (8.8.8.8/1.1.1.1),
  así que basta escribir `minecraft3ds.org`.
- `NetworkManager`: elige traductor antes de arrancar los hilos; cola de lectura acotada a 6 MB en Xbox.
- Solo en builds de prueba (`XBOX_AUTOPILOT`): `D:\autoconnect.txt` («host puerto versión») entra directo a un
  servidor desde el menú. No afecta a las builds normales.

## Pruebas
- Paper 1.12.2 local: `servidor-1.12.2\` (Paper build 1620, offline, **puerto 25566**, `iniciar.bat`; usa el Java 8
  de `servidor-1.2.5\jre8`). xemu 1.2.5: entra, terreno correcto, andar ~80 bloques (la ventana de chunks se mueve
  bien), saltar, inventario (Y), muerte por ahogo → pantalla de muerte. Chat ida y vuelta y romper bloque probados
  con el banco de pruebas de PC.
- SMP real: 3 conexiones en total, espaciadas (1.ª: xemu, el server echó con «Internal server error» → arreglado con
  `MC|Brand`; 2.ª: banco de PC grabando el tráfico, entra bien; 3.ª: xemu, entra y se ve el mundo, el usuario lo vio).
  Con la grabación del SMP hice un **servidor de repetición** (`h340\replay_server.py`) para probar sin molestar al SMP.
- Con los datos del SMP la 1.ª versión se quedaba **sin memoria** (225 columnas vivas + 18 MB de listas de dibujo de
  la spawn) → por eso la ventana de radio 5 adaptativa. Ahora: estable con ~5–6 MB libres en la spawn del SMP
  (1.2.5 y 1.5.2), mundo normal ~19–22 MB libres.
- 1.5.2 (`wt-152-net`): mismo traductor; en xemu entra a la grabación del SMP y se ve la spawn. No probado contra el
  Paper local en vivo (el código de red es el mismo que el de 1.2.5).
- FPS (xemu, no representa la consola): 60–67 andando por el mundo local; 50–80 en la spawn del SMP (bajón a ~49
  mientras llegan los chunks). Banco de PC: 0,05 ms por columna convertida.
- Compilación: `wt-125-net` y `wt-152-net`, normal y exp, sin errores; 13 avisos «SSE2 left» en cada una, todos del CRT
  (los de siempre), ninguno en código nuevo (1.5.2 exp tiene 14: el extra es `McRegionChunkLoader::readChunkData`, no
  tocado aquí).

## Pendiente / limitaciones
- Cuenta premium (online-mode) no: solo servidores offline (el SMP lo es). Al pedir cifrado sale un aviso.
- No existen en 1.2.5: mano secundaria, escudos, élitros, caballos montables, aldeanos (comercio), yunque, tolva,
  faro, mesa de encantar y soporte de pociones de 1.9+ (cambiaron sus huecos) → se cierran con aviso.
- Títulos, barra de jefe, marcadores, partículas y sonidos con nombre se ignoran.
- La memoria en la spawn del SMP va justa (~5 MB libres en xemu); en la consola real hay que mirarlo.
- Probar en la consola real (red de verdad, 733 MHz): la conversión es barata pero la compresión de las 441 columnas
  del SMP al entrar cuesta algo de CPU en el hilo de red.
