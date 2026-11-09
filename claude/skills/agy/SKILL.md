---
name: agy
description: Dirigir a los agentes agy (Antigravity CLI, Gemini) que viven en terminales de Orca - arrancarlos sin permisos, mandarles encargos por fichero, escucharlos solos con un vigilante (escuchador.py), despachar atascos y verificar lo que entregan. Usala siempre que haya que lanzar, hablar con, esperar a o revisar el trabajo de agy.
---

# Dirigir a agy desde Claude Code

La guia larga con la historia de cada trampa: `C:\Users\xeodeo\Desktop\COMO-USAR-AGY.md`. Aqui va lo que hay que hacer.

## 1. Orca y los handle
- CLI: `/c/Users/xeodeo/AppData/Local/Programs/orca/resources/bin/orca.exe` (tambien `orca.cmd`).
- `orca terminal list --json` → `result.terminals[]` con `handle`, `agentIdentity` (`antigravity` = agy) y `preview`.
- `orca terminal read --terminal <h> --screen --json` → `result.terminal.tail[]`, la pantalla renderizada. Es la UNICA lectura que enseña permisos y menus.
- `orca terminal send --terminal <h> --text "..." --enter --json`. Si sale `input_accepted`, ha entrado.
- **Los handle caducan** (`terminal_handle_stale`): vuelve a listar antes de mandar. Puede haber varios agy; identifica el tuyo por lo que muestra su pantalla (la carpeta o el encargo).

## 2. Arrancarlo: SIEMPRE sin permisos
```
cd "<carpeta de trabajo>"
agy --dangerously-skip-permissions --effort high
```
- Si ya estaba abierto sin la bandera: tres Ctrl+C (`--text "$(printf '\003')"`), copia el id que imprime y vuelve a entrar con `agy --dangerously-skip-permissions --conversation=<id>`. Luego dile "te reinicie con permisos automaticos, sigue con el encargo X".
- En una carpeta nueva pregunta "Do you trust...": se contesta con un Enter (`--text "" --enter`).

## 3. Encargos: a un FICHERO, y por la terminal solo un puntero
- Escribe `buzon\NN-ENCARGO-agy-<tema>.md`. Mandar textos largos por la TUI los rompe, y si se queda sin cuota el encargo sigue escrito.
- Por la terminal, dos o tres lineas: "Lee <ruta absoluta> entero y siguelo. Al acabar escribe <SEÑAL> <commit>".
- Que lleve:
  - el estado MEDIDO, con lineas de log y archivo:linea;
  - que es suyo y que no: su worktree, y lo que NO debe tocar;
  - la aceptacion en numeros y en fotos;
  - las trampas conocidas;
  - la ruta del informe y la señal literal de fin (p. ej. `FASE 2 LISTA <commit>`).
- Si esta ocupado, tu mensaje queda en cola ("Press up to edit queued messages"): NO lo repitas.
- Un worktree y una rama para cada agente. Nunca dos agentes sobre el mismo arbol.
- Si tienes que commitear tu en su worktree, avisale de que no haga reset.

## 4. ESCUCHARLO: no esperes a que el usuario te avise
Lanza el vigilante con el tool **Monitor** (`timeout_ms` 1800000) y **rearmalo cada vez que expire**:
```
cd /tmp && python -I ~/.claude/skills/agy/escuchador.py --hecho "FASE 2 LISTA [0-9a-f]{7,}" --clave render-nativo --handle <su handle> --cada 60 --avance 900
```
- Una vez encontrada su terminal, el vigilante se queda con ella mientras exista, aunque la `--clave` se salga de la pantalla. Pasa `--handle` si ya sabes cual es: con varios agy abiertos, la clave puede dejar de verse.
Que hace:
- relista los handle en cada vuelta;
- elige la terminal de agy cuya pantalla contiene `--clave`;
- `--informe <ruta>` hace que tambien cuente como entrega la aparicion de ese fichero.

Que avisa:
- `LISTO`: aparece la señal. El vigilante se cierra; arma otro para la siguiente entrega.
- `ATASCADO`, con la salida de cada caso:

  | Lo que sale | Que mandarle |
  |---|---|
  | permiso pedido | Enter (o reinicialo sin permisos) |
  | paginador (`pgup/pgdown`) | ESC: `--text "$(printf '\033')"` |
  | encuesta del CLI | `0` + Enter |
  | "Do you trust..." | Enter |
  | cuota agotada | nada: escribe un traspaso y espera al reseteo |
  | no encuentra su terminal | relista y mira las `preview` |
- `PARADO`: esta en el prompt `>` sin trabajar. Ha acabado sin dar la señal, o pregunta algo: ve a leer su pantalla.
- `AVANCE`: lo que dice, como mucho cada `--avance` segundos.

No hagas esperas con `until ... --terminal <handle fijo>`: el handle caduca y te quedas ciego.

## 5. Verificar SIEMPRE: el informe no basta
agy con Gemini 3.8 Flash acierta a menudo el QUE, pero exagera o inventa cifras y causas. Antes de aceptar:
1. **Recompila tu** su commit (`-Demo -SinLoader`, con el candado) y prueba tu.
2. **Fotos de la MISMA escena, viejo contra nuevo** (PrintWindow, nunca la pantalla entera: el usuario usa el PC).
   - Una vez dio por buena una foto entera negra.
   - Otra vez subio de 11 a 18 fps porque NO dibujaba el escenario.
3. Busca en su log las lineas que contradigan su informe, como "0 entradas" o "0 verts".
4. Comprueba lo que el informe no menciona:
   - la memoria libre (`KB libres`);
   - `nan` y `[X87]` en el netlog;
   - `git diff --stat`, por si ha borrado mucho;
   - avisos nuevos de "SSE2 left in".
5. Si la explicacion es falsa, diselo con el dato que la desmiente y anotalo en su encargo, en una seccion "REVISION DE CLAUDE".
6. En la consola real pasan cosas que xemu no enseña:
   - la pila x87 (NaN);
   - triangulos detras de la camara, que sin recorte por la CPU hacen agujeros;
   - quedarse sin memoria (64 MB): texturas que desaparecen, cuadrados.

   Pidele siempre que mida esas tres cosas.

## 6. Cuota y reparto
- Se cortan a mitad cuando se acaba la cuota ("quota reached... Resets in"). Si pasa, mira si dejo el arbol roto y escribe tu el traspaso.
- No lances dos encargos grandes a la vez a agy: se quedan sin cuota los dos juntos.
- agy rinde bien en inventarios, rastreo de llamadas y codigo ya especificado. Lo delicado reviselo Claude linea a linea.
- Cierra cada turno tu: al aceptar una entrega, manda ya el siguiente encargo.
