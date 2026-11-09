# 84. Informe: agacharse con interruptor y mantener A en inventarios (09-11, Claude)

## Ramas y commits
- 1.5.2: `wt-152-rend`, rama `port152-controles-84` (desde `cae78c0`): `7480cc8` (agacharse), `316988d` (mantener A).
- 1.2.5: `wt-opti30`, rama `opti30-cruceta`: `500db47` (agacharse), `84b4059` (mantener A), `cdee032` (adaptación a 1.2.5:
  sin SlotMerchantResult/canTakeStack, `updateCraftingResults`).
- Compilan: 1.5.2 Xbox normal y exp; 1.2.5 Xbox normal, exp y PS2. Nada copiado al lanzador. Sin probar en consola/xemu.

## A. Agacharse: mantener / alternar
- Ajuste `toggleSneak` (por defecto NO), guardado en options.txt (`toggleSneak:`), textos «Sneak: Hold/Toggle» →
  «Agacharse: mantener / alternar».
- Menú Legacy de Controles: botón al lado de «Restablecer valores» (misma fila, media anchura cada uno).
- Menú Java de Controles: botón a la izquierda de «Hecho» (misma fila).
- `MovementInputFromOptions::applySneak`: con la opción, cada pulsación nueva en juego alterna; una pulsación hecha
  con una pantalla abierta no alterna; el estado se mantiene al abrir/cerrar inventarios (dentro de pantallas no se
  agacha, como antes) y se reinicia al morir o cambiar de mundo (se crea un MovementInput nuevo). J1, J2 Xbox (clic del
  stick izq.) y J2 PS2 (R3), cada uno con su estado. Volando en creativo: mientras esté activo, baja.

## B. Mantener A en inventarios (Xbox)
- `GuiContainer::tickHoldRepeat` (cada frame, desde `drawScreen`): lee A del mando del dueño de la pantalla sin consumir
  pulsaciones. El toque normal sigue igual; tras 0,35 s con A pulsada sobre la misma casilla repite cada 80 ms,
  acelerando hasta 40 ms. Se para al soltar A, al cambiar de casilla (hasta soltar) o al cerrar. Si la A venía pulsada
  al abrir la pantalla, no cuenta hasta soltarla. Solo compara el puntero de la casilla; nunca lo usa si cambió.
- Creativo (paleta): cada repetición suma 1 al cursor si lleva el mismo objeto y no está lleno.
- Resto de casillas: pasa 1 objeto de la casilla al cursor (cursor vacío o mismo objeto/etiquetas y sin llenar), con
  `decrStackSize(1)`. No en casillas de resultado (crafteo, horno, comercio) ni en mundos de servidor.
- En PS2/Wii no hace nada (solo Xbox).

## Qué probar
1. Controles (Legacy y Java): el botón cambia entre mantener/alternar y se guarda al reiniciar.
2. Con «alternar»: pulsar agacharse una vez y soltar → sigue agachado; otra vez → se levanta. Abrir inventario y
   cerrarlo: sigue igual. Morir → de pie. Volar en creativo con alternar activo: baja.
3. Pantalla partida: J2 con alternar (clic del stick izquierdo).
4. Con «mantener» (por defecto): todo como antes. Andar, saltar, inventario y cruceta normales.
5. Creativo: coger un objeto de la paleta y mantener A → va subiendo de 1 en 1 rápido hasta 64.
6. Cofre/inventario: el primer toque de A hace lo de siempre (coger la pila entera, o soltar/juntar la del cursor).
   OJO, consecuencia de «el primer toque igual»: con el cursor vacío sobre una pila, el toque ya coge toda la pila y
   mantener no hace nada más; con el cursor lleno del mismo objeto, el toque lo suelta en la casilla y al mantener se
   van recogiendo de 1 en 1. Si el usuario prefiere que mantener A coja de 1 en 1 desde el principio (sin el toque
   inicial de pila entera), hay que retrasar el clic normal hasta soltar A: es un cambio pequeño en `tickHoldRepeat`.
7. J2 en pantalla partida: lo mismo en su inventario.
