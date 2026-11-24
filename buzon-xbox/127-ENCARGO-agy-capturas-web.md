# Encargo agy — capturas de OptiCraft para la web

Objetivo: sacar varias capturas BONITAS del juego (mundo tutorial de OptiCraft) y dejarlas en
`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\web\img\` como `3.png`, `4.png`, `5.png`, `6.png`.
(`1.png` y `2.png` ya existen, no las toques.)

## Cómo arrancar y capturar
- El emulador y el capturador ya están hechos: `OptiCraftHeritageEdition\xbox-spike\run-xemu.ps1`.
  - Uso: `powershell -ExecutionPolicy Bypass -File "...\xbox-spike\run-xemu.ps1" -Iso "<ISO>" -Seconds <n> -Shot "<salida.png>" -KeepOpen`
  - Arranca xemu con el ISO, espera `-Seconds`, captura la ventana de xemu con PrintWindow (NO toda la pantalla) y guarda el PNG. Con `-KeepOpen` deja xemu abierto para seguir capturando.
- ISO a usar (exp 1.5.2): `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-build-152\bin\xbox-exp\OptiCraft.iso`
- El build arranca directo al **mundo tutorial** ("Welcome to OptiCraft Heritage!"). Es el mundo que queremos enseñar.

## Qué hacer
1. Arranca el ISO con `-KeepOpen` y `-Seconds 45` (que cargue el mundo del todo) → guarda una primera captura en `buzon-xbox\web\img\3.png`.
2. Con xemu abierto, muévete/mira alrededor para encuadrar algo bonito (xemu mapea teclado a mando: WASD mover, ratón mirar; si no puedes mover, al menos espera y captura en otros momentos). Vuelve a capturar con el mismo método a `4.png`, `5.png`, `6.png` en zonas/ángulos distintos.
3. Criterios de "bonita": que se vea el mundo (árboles, hierba, cielo), SIN menús ni el cartel de bienvenida tapando, buena luz. Descarta capturas negras, con menú abierto, o con el HUD de controles tapando todo.
4. Cierra xemu al terminar.

## Trampas
- Captura SOLO la ventana de xemu (run-xemu.ps1 ya lo hace con PrintWindow). No hagas screenshot de todo el escritorio (el usuario usa el PC).
- Verifica cada PNG: que NO sea negro y que el pixel central no sea del color del cielo vacío. Si sale mal, repite con otro `-Seconds`.
- No toques código ni hagas commits. Solo escribes los 4 PNG en `buzon-xbox\web\img\`.
- Candado de compilación: NO hace falta compilar; el ISO ya está hecho. Si xemu no abre, dilo en el informe.

## Entrega
- Deja un informe corto en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\127-INFORME-agy-capturas.md` con qué capturaste y cuáles quedaron bien.
- Al terminar escribe en la terminal exactamente: `CAPTURAS LISTAS` seguido de cuántos PNG válidos dejaste.
