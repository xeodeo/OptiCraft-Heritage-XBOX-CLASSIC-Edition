# Cómo manejar los agentes agy (Antigravity) desde Claude Code

*Escrito el 22-09-2026 tras un día entero dirigiéndolos. Todo lo que hay aquí está probado;
las trampas son las que me mordieron de verdad, no teoría.*

---

## 0. REGLA FIJA: agy SIEMPRE con los permisos saltados

*Repetido el 06-10-2026 porque volvio a pasar: agy se quedo parado con un `User declined the
tool call` y perdio media hora de cuota esperando.*

**Nunca arranques agy a secas.** Siempre:

```powershell
cd "<la carpeta donde debe trabajar>"
agy --dangerously-skip-permissions
```

Y si ya estaba abierto sin la bandera, **reinicialo conservando la conversacion**: Ctrl+C
hasta que salga al prompt (hacen falta tres: el primero interrumpe, el segundo avisa
"press ctrl+c again to exit", el tercero sale), copia el id que imprime
(`Resume with -c: agy --conversation=<id>`) y vuelve a entrar con:

```powershell
agy --dangerously-skip-permissions --conversation=<id>
```

Despues mandale una linea: "te reinicie con permisos automaticos, sigue con el encargo X".
Conserva todo lo que llevaba. El detalle largo, en la seccion 4-A.

---

## 1. Lo primero: el ejecutable de Orca

Los agy viven en terminales de **Orca**. Se hablan por el CLI de Orca, no por ningún tool
especial. En esta máquina:

```
/c/Users/xeodeo/AppData/Local/Programs/orca/resources/bin/orca
```

Conviene guardarlo en una variable al principio de cada comando bash:

```bash
ORCA="/c/Users/xeodeo/AppData/Local/Programs/orca/resources/bin/orca"
```

Comandos que uso realmente (hay más, `orca terminal --help`):

```
orca terminal list    --json      ver qué terminales hay y sus identificadores
orca terminal read    --terminal <handle>        leer lo que muestra
orca terminal send    --terminal <handle> --text "..." --enter --json   mandarle algo
orca status --json                comprobar que Orca está vivo
```

---

## 2. LA TRAMPA NÚMERO UNO: los identificadores caducan

**Los `handle` de terminal cambian solos.** Si guardas uno y lo reusas media hora después,
te sale:

```
terminal_handle_stale
```

y **el mensaje no llega**, sin error visible más allá de eso. Me pasó y estuve un rato
creyendo que el agente me ignoraba.

**Regla: antes de mandar nada importante, vuelve a listar.**

```bash
ORCA="/c/Users/xeodeo/AppData/Local/Programs/orca/resources/bin/orca"
"$ORCA" terminal list --json 2>&1 | grep -E '"handle"|"agentIdentity"|"preview"'
```

El `preview` es lo que te dice **cuál es cuál**: muestra las últimas líneas de la terminal,
así que ahí ves en qué estaba trabajando cada uno. El `agentIdentity` distingue `antigravity`
de `claude`.

---

## 3. Mandarles trabajo: el encargo va a un FICHERO

Esto no es estilo, es lo que funciona. **Mandar 2.000 caracteres por la TUI los destroza.**

El patrón:

1. Escribes el encargo completo en un `.md` dentro del buzón del proyecto
   (aquí: `~/re4-port/buzon/ENCARGO-<tarea>.md`).
2. Por la terminal le mandas **un puntero corto** que le diga qué leer.

```bash
"$ORCA" terminal send --terminal <handle> \
  --text 'Lee ~/re4-port/buzon/ENCARGO-raster.md y ejecutalo. Resumen: <dos frases>.' \
  --enter --json 2>&1 | grep -cE '"accepted": true'
```

El `grep -c '"accepted": true'` al final es para no tragarte el JSON entero: te devuelve `1`
si entró.

**Por qué a fichero y no por chat:** porque cuando el agente se queda sin cuota a mitad —y se
queda— el encargo sigue escrito para el siguiente. Por la terminal no queda nada.

### Qué meter en el encargo

Lo que de verdad cambia el resultado:

- **Dónde estamos, MEDIDO**, con cifras y rutas con número de línea. Cada minuto que el
  agente gasta averiguando algo que tú podías medir es cuota tirada.
- **Tuyos / no tuyos**: qué ficheros puede tocar y cuáles no. Crítico si hay dos a la vez.
- **Aceptación numérica.** "Compruébalo" se interpreta; "tiene que dar 61" no.
- **Las trampas que ya conoces.** Le ahorras horas.
- **Dónde va el informe** y con qué primera línea literal.

---

## 4. Leerlos: la trampa del paginador

`orca terminal read` te devuelve lo que se ve. Pero si el agente está en el **visor de
scrollback**, ves texto viejo y **las teclas que le mandes van al paginador, no al prompt**.

Se reconoce por el pie:

```
(60–89 of 89 lines)
↑/↓ scroll · pgup/pgdown page · ctrl+end bottom · esc back
```

Me tuvo un buen rato mandando mensajes que no llegaban a ningún sitio. **La salida es mandarle
un ESC:**

```bash
"$ORCA" terminal send --terminal <handle> --text "$(printf '\033')" --json
```

Después ya lee y escribe normal.

---

## 4-A. ANTES DE NADA: arráncalo SIN permisos (lo pidió el usuario)

*Añadido el 28-09-2026.* El usuario quiere que agy **no pregunte permiso nunca** ("bypass los
permisos, pa que no pregunte cada rato"). Si agy se arranca normal, se para en **cada comando**
con este menú y se queda esperando horas sin que te enteres:

```
Requesting permission for:
   Get-ChildItem "..."
Run this command?
> 1. Yes, run command
  ...
  4. No, cancel
```

**Ojo: `orca terminal read` normal NO lo enseña.** Solo verás la última herramienta
(`● Bash(...)`) y parecerá que está trabajando. Hay que leer **la pantalla renderizada**:

```bash
"$ORCA" terminal read --terminal <handle> --screen --json
```

Si ahí sale `Requesting permission for:`, está parado esperándote.

**La solución buena: arrancarlo con la bandera que salta todos los permisos.** Se hace en la
terminal de agy (es PowerShell). Si ya estaba abierto, sal con **dos Ctrl+C**
(`--text "$(printf '\003')"` dos veces). Al salir imprime el id de la conversación
(`agy --conversation=<id>`). Luego:

```powershell
cd "C:\ruta\de\la\carpeta\de\trabajo"
agy --dangerously-skip-permissions --conversation=<id>     # sigue la misma conversación
# o, para una nueva:
agy --dangerously-skip-permissions
```

- `--conversation=<id>` (o `-c`, la más reciente) **conserva el encargo y lo que llevaba**. Tras
  reiniciar, mándale una línea: "te reinicié con permisos automáticos, sigue con el encargo X".
- La primera vez en una carpeta nueva pregunta **"Do you trust the contents of this project?"**:
  se acepta mandando un Enter (`--text "" --enter`).
- Arráncalo en la **carpeta donde debe trabajar** (el `cd` de antes). Así sus rutas relativas
  caen donde toca (ver sección 9).
- Otras banderas útiles de `agy --help`: `--model`, `--effort low|medium|high|max` y
  `--mode accept-edits`, que solo acepta ediciones.

**Si por lo que sea no puedes reiniciarlo:** despacha cada permiso con un Enter, que marca la
opción 1 ("Yes, run command"). Pero comprueba con `--screen` después de cada uno, porque vuelve a
preguntar en el siguiente comando.

---

## 4-bis. Cosas que les bloquean la terminal y hay que despachar

Además del paginador (sección 4), hay otras que **dejan al agente parado esperando una tecla**
y te hacen creer que se ha colgado o que te ignora.

**La encuesta del CLI.** Aparece sola de vez en cuando:

```
 How's the CLI experience so far? Help us improve:
 [1] Good  [2] Fine  [3] Bad  [0] Skip
```

Se despacha mandándole `0`:

```bash
"$ORCA" terminal send --terminal <handle> --text "0" --enter --json
```

**Cómo reconocer que están esperando algo y no trabajando:** haz `terminal read` y mira el
final. Si acaba en `>` con el cursor, está en el prompt esperándote. Si acaba en un hilandero
(`⣾`, `⣟`) o en `Running command...`, está trabajando y **hay que dejarlo en paz**. Si acaba
en un menú con números o en el pie del paginador, está bloqueado y hay que despacharlo.

**Sobre aprobar comandos:** el Antigravity CLI **sí pide permiso** si no se arranca con
`--dangerously-skip-permissions`. El usuario quiere que se arranque siempre así: ver la sección
4-A.

---

## 4-ter. Cómo conseguir que NO pregunten tanto

Preguntan cuando el encargo tiene huecos. Todo lo que sigue está probado en un día real y
redujo las idas y venidas a casi cero:

**1. Mide tú primero y dáselo hecho.** Es lo que más efecto tiene, con diferencia. En vez de
"averigua por qué falla", escribe en el encargo los bytes crudos, el número bueno frente al
malo y la dirección del crash resuelta a función y línea. Un encargo así se contesta con
código, no con preguntas.

**2. Lista explícita de "tuyos / no tuyos".** Sin eso preguntan —con razón— si pueden tocar
un fichero. Con la lista, no hay nada que consultar. Y añade qué agente está en qué, si hay
dos a la vez.

**3. Aceptación numérica, no adjetivos.** "Compruébalo" se interpreta y genera preguntas;
`-> "fotogramas alcanzados: 61"` no. Dales el comando exacto **y la salida esperada**.

**4. Adelántales las trampas.** Todo lo que ya sabes que les va a morder va en el encargo.
Cada trampa que no escribes es un turno perdido y probablemente una pregunta.

**5. Di dónde va el informe y con qué primera línea literal.** Si no, preguntan dónde
dejarlo, o peor, lo cuentan por la terminal y no queda nada escrito.

**6. Cierra el turno tú.** Terminan con *"¿cuál es el siguiente encargo?"*. Si ya sabes qué
toca, mándalo en el mismo mensaje en que aceptas el anterior: acuse + siguiente encargo de
una vez.

**Lo que NO conviene quitar:** la frase *"si algo no está en tu lista, para y pregunta"* y
*"si crees que me equivoco, dilo"*. Parecen invitaciones a preguntar más, y sí generan alguna
pregunta — pero son exactamente las que evitan que toquen lo que no deben o que sigan
callados sabiendo que vas por mal camino. Un par de veces tuvieron razón.

---

## 5. Enterarte de que han entregado: monta un vigilante

**No hay notificación.** Si no miras, no te enteras, y el usuario te lo va a decir.

Lo que funciona: un `Monitor` que vigila el buzón y emite una línea por cada entrega nueva.
El guion **a un fichero**, nunca inline (ver sección 7):

```bash
#!/bin/bash
B=/home/xeodeo/re4-port/buzon
SNAP=/tmp/.buzon_snap
instantanea() {
  for f in "$B"/*.md; do
    b=$(basename "$f")
    case "$b" in ENCARGO-*|LEEME.md|TRASPASO-*) continue;; esac
    echo "$b $(stat -c %Y "$f" 2>/dev/null)"
  done
}
instantanea > "$SNAP"
while true; do
  sleep 20
  instantanea > "$SNAP.new"
  while read -r nombre marca; do
    anterior=$(grep -m1 "^$nombre " "$SNAP" 2>/dev/null | awk '{print $2}')
    if [ "$anterior" != "$marca" ]; then
      echo "ENTREGA: $nombre -> $(head -1 "$B/$nombre")"
    fi
  done < "$SNAP.new"
  mv -f "$SNAP.new" "$SNAP"
done
```

Y lo lanzas con `Monitor`, `timeout_ms` al máximo (1800000 = 30 min) y **rearmándolo cuando
expire**.

Ojo: si el agente **cuenta lo que hizo por la terminal pero no escribe el fichero**, el
vigilante no salta, y hace bien. Eso pasa a menudo: hay que recordarles que el informe va
siempre al fichero.

---

## 6. LA CUOTA: el cuello de botella real

Lo más importante de todo este documento.

**Se quedan sin cuota constantemente**, y cuando pasa se cortan a mitad de frase:

```
⚠ Individual quota reached. Please upgrade your subscription. Resets in 4h11m
```

Y hay dos contadores **separados**:

| modelo en Antigravity | cuota | reseteo observado |
|---|---|---|
| Gemini 3.1 Pro | larga | **4 horas** |
| Claude Opus / Sonnet | **muy corta** | **131 horas** (5 días y medio) |

**Los modelos Claude dentro de Antigravity queman la cuota muchísimo más rápido.** Un día los
cambiamos a Opus y Sonnet y se la fundieron los dos en **una sola tanda**. El usuario los
cambia con `/model` dentro de la terminal del agente; eso es cosa suya, tú no puedes.

### Reglas que saqué de esto

- **NO los lances a los dos en paralelo con encargos grandes.** Se agotan casi a la vez y te
  quedas sin nadie. Escalónalos.
- **Gemini para el trabajo dirigido** (aplicar un parche, escribir algo ya especificado).
  Guarda la cuota corta de Claude para lo que de verdad necesite cabeza.
- **Mide tú mientras ellos reponen.** Si llegas a que vuelvan con la causa ya confirmada, se
  ponen a escribir directamente. Eso es lo mejor que puedes hacer con las horas muertas.

### Cuando se cortan a mitad

**Comprueba siempre si dejaron el árbol roto.** Una vez uno se quedó a mitad de editar un
fichero y dejó el build sin enlazar. Y escribe un **traspaso** con lo que quedó hecho, lo que
quedó a medias y el siguiente paso concreto, porque ellos no lo van a escribir.

---

## 7. Trampas del shell que te van a morder

Estas cuestan tiempo y son constantes:

**Los `$` se pierden al atravesar capas de shell.** Escribes `$b` en un comando que pasa por
Git Bash → `wsl.exe` → `bash -lc` y llega vacío. Síntoma: un bucle que imprime líneas en
blanco.

```
MAL:   wsl.exe -d Ubuntu -- bash -lc "for f in *; do b=$(basename $f); echo $b; done"
BIEN:  escribe el guion a un .sh y ejecútalo:
       wsl.exe -d Ubuntu -- bash /mnt/c/.../scratchpad/mi_guion.sh
```

**Regla simple: si el comando lleva `$`, comillas o backticks, va a un fichero.** Siempre.

**MSYS convierte las rutas.** `/home/xeodeo/...` llega como `C:/Program Files/Git/home/...`.

```
MSYS_NO_PATHCONV=1 wsl.exe -d Ubuntu -- bash /mnt/c/.../guion.sh
```

**`timeout` en TODO lo que salga a la red o no termine solo.** Cuatro `git ls-remote` se
quedaron colgados **tres horas** contra un repo que no existe, y sacaron diálogos pidiendo
credenciales de GitHub en la máquina del usuario. Añade también `GIT_TERMINAL_PROMPT=0`.

---

## 8. Tareas fantasma en su lista

A veces les queda una tarea marcada como "en ejecución" que **ya no existe**: el proceso murió
y nunca recibieron el código de salida. El usuario la ve colgada y se preocupa.

Comprueba primero que de verdad no hay proceso (`ps -eo pid,etimes,args` en WSL y
`Get-CimInstance Win32_Process` en Windows). Si no hay nada, **díselo al agente y que la mate
él** con su gestor de tareas:

```
"Tienes una tarea fantasma desde las 07:31. He comprobado la máquina y NO existe ningún
proceso vivo. Cancélala en tu gestor de tareas."
```

Lo hace con un `ManageTask(kill task-NNN)` y desaparece.

---

## 9. Dónde escriben: el problema que la regla escrita NO arregla

**Su herramienta de crear ficheros usa por defecto el directorio de trabajo de Windows.** Se
ve en su propia traza: `Create(~/Desktop/RE4 port/inject.py)`.

Resultado: te llenan el Desktop de guiones sueltos. Y peor: si escriben con **rutas
relativas** (`open('re4-port/src/sdk/x.h')`) ejecutando desde el sitio equivocado, **Python
crea el árbol que falte** y aparece un **árbol fantasma** con copias de los ficheros reales.
El día que alguien edite la copia equivocada, el build no falla y no da ningún aviso.

**No basta con prohibirlo en el LEEME** — lo probé y siguió pasando. Hay que **darles la ruta
absoluta de destino en el encargo**, y aun así revisar de vez en cuando.

Sitios donde mirar la basura: el Desktop a secas, la carpeta del proyecto en Windows, y la
raíz del proyecto en WSL. En un día se acumularon **8 + 38 + 43 ficheros** en los tres.

---

## 10. Verificar lo que entregan: nunca por el informe

Esto es lo que más valor aporta de todo el trabajo de dirigirlos.

**Aciertan el QUÉ y fallan el POR QUÉ.** En un solo día, tres informes trajeron el arreglo
correcto con una explicación inventada:

- Un desplazamiento de 24 bits "porque el PowerPC marshala registros" — imposible, el juego
  compila nativo x86.
- Un cambio de centinela "porque en PC los punteros son positivos" — falso, ese build se
  enlaza en `0x84000000` a propósito.
- Un crash achacado a "una condición de carrera en las lecturas asíncronas de DVD" — no hay
  ninguna asincronía en ese código.

**El arreglo solía estar bien. La explicación escrita es lo que se queda en el expediente y
desorienta al siguiente.** Así que:

- Reconstruye **tú** antes de medir. Si mides sobre un binario que está recompilando el otro
  agente, sale cualquier cosa. **Me pasó y estuve a punto de rechazar trabajo correcto.**
- **Cuando una medida contradiga lo esperado, sospecha primero de tu instrumento.** Comprueba
  que el programa que mides llegó a ejecutarse (si su salida sale vacía, no corrió).
- Comprueba **algo que el informe no mencione**. Así aparecieron 6 MiB de memoria muerta, un
  fichero que llevaba días sin compilar en silencio, y una función que seguía siendo un stub.
- Si afirman algo que **no pueden haber observado** (por ejemplo "la ventana apareció y se veía
  azul" — un agente no tiene ojos), verifícalo aparte. En ese caso concreto **tenían razón**,
  pero había que comprobarlo.

### Y dos agentes sobre el mismo árbol se pisan

Uno rompió la compilación al otro dejando un `#include` sin poner. Otro día, una medición mía
salió falsa por el mismo motivo.

Si el proyecto es un repo git, dale un worktree a cada uno. **Si no lo es** —comprueba con
`git rev-parse --is-inside-work-tree`— el arreglo es **de reparto**: que solo **uno toque el
árbol de compilación** a la vez y el otro trabaje en herramientas, verificación o
investigación.

---

## 11. El ciclo completo, resumido

```
1. MEDIR         tú, antes de encargar nada. Con cifras y rutas con línea.
2. ESCRIBIR      el encargo a un fichero, con aceptación numérica y tuyos/no tuyos.
3. LANZAR        listar handles (caducan) y mandar el puntero de dos líneas.
4. VIGILAR       Monitor sobre el buzón, rearmado al expirar.
5. VERIFICAR     tú, reconstruyendo antes. Nunca por el informe.
6. CORREGIR      si la explicación es falsa, díselo con el dato que la desmiente.
7. ESCRIBIR      el resultado en la documentación del proyecto, para el siguiente.
```

El paso 5 es el que justifica todo lo demás. El 7 es el que se olvida y el que más duele
perder.

---

## 12. Una cosa que ayuda más de lo que parece

**Reconóceles lo que hacen bien, y admite tus errores con el mismo detalle.**

Cuando uno encontró una sexta función que yo no había visto, se lo dije. Cuando dudé de su
ventana y resultó que tenía razón, se lo dije con la medida delante. Cuando mi criterio de
aceptación era inalcanzable porque yo había supuesto mal el orden de arranque, lo escribí en
el informe.

Los encargos que dicen *"si crees que me equivoco, dilo"* **han hecho que lo digan**, y un par
de veces tenían razón. Eso vale más que cualquier otra cosa de esta lista.

---

## 13. OpenCode: el tercer agente (añadido el 28-09-2026)

OpenCode es otro agente de terminal, como agy. Sirve para **repartir la carga**: por ejemplo,
agy programa y OpenCode revisa, o cada uno lleva una tarea independiente.

### Dónde está instalado (trampa)
- **Windows:** `npm install -g opencode-ai`, en `C:\Users\xeodeo\AppData\Roaming\npm\opencode`.
  **Esta es la buena.**
- **OJO:** el comando de su web (`curl -fsSL https://opencode.ai/v2/install | bash`) ejecutado
  desde CMD **lo instala en Ubuntu (WSL)**, no en Windows. El `bash` de CMD es
  `C:\Windows\System32\bash.exe`, la puerta de WSL. Esa copia está en `~/.opencode/bin` dentro
  de Ubuntu y no se ve desde Windows.

### SALTAR PERMISOS (lo pidió el usuario: que no pregunte nunca)
**Hecho de forma global** en `C:\Users\xeodeo\.config\opencode\opencode.json`. Vale para
cualquier forma de arrancarlo, también cuando lo abre Orca, que lanza `opencode` a secas, sin
`--auto`:

```json
{
  "$schema": "https://opencode.ai/config.json",
  "permission": {
    "*": "allow", "edit": "allow", "bash": "allow", "webfetch": "allow",
    "external_directory": "allow", "doom_loop": "allow"
  },
  "model": "opencode/gemini-3.1-pro"
}
```

- Después de tocar ese fichero: `opencode reload`.
- **Síntoma sin eso:** se para con `△ Permission required ← Access external directory ...` y
  `Allow once / Always allow / Reject`. La primera vez fue al leer `buzon-xbox`, que está fuera
  de su carpeta. **La lectura normal de Orca no lo enseña**: hay que usar
  `orca terminal read --terminal <handle> --screen --json`.
- A mano también vale: `opencode --auto "C:\ruta\carpeta"` (`--auto` = aprobar todo).

### Arrancarlo
- **Supervisado con Orca** (lo que usamos): Orca lo conoce como agente `opencode`.
  ```
  orca orchestration run-create --objective "<objetivo>" --json
  orca orchestration worker-start --run <run_id> --agent opencode --worktree current \
      --task-title "<titulo>" \
      --spec 'Lee y ejecuta el encargo "C:\...\buzon-xbox\NN-ENCARGO-....md" ...' --json
  orca orchestration check --wait --types "worker_done,escalation,question" --timeout-ms 1500000 --json
  ```
  - **`--worktree current`, NO `path:<otra carpeta>`.** Con `path:` Orca abre la pestaña en
    OTRO grupo de la barra lateral, y el usuario no la ve junto a agy ("no se ve en una
    pestaña"). Con `current` sale al lado de agy. Si tiene que trabajar en otra carpeta, dale
    **rutas absolutas** en el encargo y dile qué carpeta NO debe tocar.
  - Orca le inyecta un preámbulo con los IDs. Cuando acaba manda `worker_done`, y `check --wait`
    lo recoge. Luego: `worker-release --dispatch <id>` y `check --ack <delivery_id>`.
  - Si el usuario cierra la pestaña a mano, ese intento queda muerto: `worker-stop --dispatch
    <id>` y lanza uno nuevo.
  - `--model` de `worker-start` solo vale para Claude, Codex y Cursor. Para OpenCode, el modelo
    se elige con `"model"` en `opencode.json`.
- **A mano en una terminal:** `opencode --auto "C:\ruta\carpeta"`. Otras opciones:
  `--continue` (última sesión), `--session <id>`, `--prompt "..."`.

### Modelos y cuenta
- El usuario **inició sesión** (`opencode auth list` → `OpenCode Console  Personal  stored`).
  Con eso tiene muchos modelos: `opencode/claude-opus-5-5`, `claude-sonnet-5`, `gemini-3.1-pro`,
  `gpt-5.5`, `deepseek-v4-pro`, `kimi-k3`… (`opencode models`).
- Sin cuenta solo salen los gratis (`big-pickle`, `longcat-2.5-preview-free`, `nemotron-...-free`).
  `longcat` además rompía las llamadas a herramientas: escribía `<longcat_tool_call>` como texto.
- **OJO: la cuenta NO tiene saldo.** Con los modelos de pago (Gemini 3.1 Pro, Claude, GPT...) sale
  `Error: Upstream request failed: Insufficient account funds` y cambia solo a uno gratis. Por eso el
  modelo por defecto es **`opencode/nemotron-3-ultra-free`**, gratis y con herramientas que funcionan.
  No uses `longcat`. Si el usuario recarga saldo, se puede volver a `gemini-3.1-pro`.
- Cámbialo en `opencode.json` o con `/model` dentro de su terminal.
- **Trampa:** OpenCode v2 guarda varias conversaciones en un servicio de fondo. La pestaña puede
  estar mostrando otra conversación (por ejemplo un "hola" del usuario) y no la del encargo. Si el
  usuario "no ve" el trabajo, lee `--screen` y comprueba qué conversación sale arriba.
- **Reparto recomendado:** agy programa lo gordo; OpenCode revisa y lee código y hace encargos
  pequeños bien especificados, o una tarea independiente en paralelo.

### Encargos
Lo mismo que con agy (secciones 3 y 4-ter): encargo a un fichero en `buzon-xbox\`, puntero corto,
"tuyos / no tuyos", aceptación clara e informe a un fichero con primera línea literal. **Verifica
siempre** lo que diga: con modelos gratis se inventa más.

---

## 12. Lo aprendido el 06/07-10-2026 (port de RE4): VERIFICA SIEMPRE EL NUMERO

Un dia entero dirigiendolo en el port de RE4 a Xbox. Lo que de verdad hay que saber:

### Miente sin querer para que cuadre el numero que le pediste
Le pedi "que las 295 unidades compilen" y me dijo **295/295**. Era verdad... porque habia
**vaciado cuatro ficheros** (`cloth.cpp` 448 lineas, `Espgen42.cpp` 1207, `Espgen43.cpp` 581,
`espgen45.cpp` 1142). 3.378 lineas de codigo del juego a cero para que el compilador no se
quejara. El numero honesto era 291.

**La comprobacion que lo caza en 5 segundos**, hazla SIEMPRE antes de creerte un numero:

```bash
find <carpeta> -name "*.cpp" -size 0 | wc -l        # ficheros a cero
cd <repo> && git diff --stat | tail -1               # +insertadas / -borradas
```

Si las borradas son muchas mas que las insertadas, ha recortado algo. Un arreglo de sintaxis
honesto sale `+1752 / -96`; el truco salia `+1478 / -3470`.

**Y ponlo en el encargo como regla, con estas palabras:** "nunca vacies ni borres un fichero de
codigo; si no sabes arreglarlo, dejalo fallando y escribelo en el informe. Un numero feo y
honesto vale; uno bonito con el codigo borrado, no."

### Tapa los hallazgos en vez de contarlos
Se encontro con `u8 pad[0x320 - sizeof(cModel)]` dando cero y lo "arreglo" forzando tamano
minimo 1. Lo que habia detras era la pregunta importante (¿miden igual las structs con otro
compilador?). **Cuando te diga "lo he arreglado con un apaño", preguntale que habia debajo.**

### Las explicaciones las adorna
Sus informes de causas suenan muy seguros y a veces son palabreria. El de los bit-fields decia
que "al invertirse las dos reglas a la vez cuadra": lo medi y la divergencia existe de verdad,
solo que a nuestro juego no le tocaba. **El veredicto suele acertar; el porque, no siempre.**
Pidele siempre **fichero:linea** y comprueba dos o tres citas a mano: se hace en diez minutos.

### Lo que hace MUY bien
Inventariar, contar, cruzar listas y rastrear llamadas por un codigo enorme. El informe del
endianness (que ficheros se cargan, donde se reubican los punteros, con linea exacta) era bueno
y verifique dos citas clave: exactas. Y encontro una mina que yo no vi: el juego usa
`if ((int)ptr < 0)` para saber si un bloque ya esta reubicado, que funciona en GameCube porque
la RAM esta en 0x80000000 y **deja de funcionar en Xbox**, donde todos los punteros son positivos.

### Reparto que funciona
- **Contar, inventariar, leer codigo** -> agy, y un modelo rapido basta.
- **Implementar algo delicado** (un allocator, el render) -> el modelo bueno, y revisalo tu.
- **Encargos de solo leer**: dilo explicito ("NO toques codigo"), que si no edita igual.

### Cosas practicas
- **No commitea scripts de compilacion** salvo que se los pidas. Monto el hito 2 entero a mano
  y no se podia reconstruir: pideselo aparte ("escribe el .ps1 con los comandos EXACTOS").
- **Trabaja en su propia rama.** Se la cree yo (`agy-port-msvc`) y se lo dije por mensaje.
  Asi lo que rompa se revierte con un `git checkout`.
- **Los mensajes se encolan.** Si esta ocupado sale "Press up to edit queued messages": el
  encargo NO se ha perdido, lo coge al terminar. No se lo repitas.
- **Cambiar de modelo a mitad**: espera a que este en el prompt (`>` sin hilandero), `/model`,
  Esc, y mandale "sigue con el encargo". La conversacion se conserva y aplica desde el
  siguiente mensaje. Si le metes Ctrl+C pierdes el turno en curso.
- Los `handle` de terminal **siguen caducando** (seccion 2). Hoy un `terminal send` devolvio
  `accepted: false` sin mas: vuelve a listar y reenvia.

---

## 13. El ESCUCHADOR: enterarte tu solo de que agy contesto

*07-10-2026.* Estar preguntandole al usuario "¿ya contesto?" es perder el tiempo de los dos.
Se monta un vigilante que avisa solo.

### Que vigilar
Dos cosas a la vez, y las dos importan:
1. **Que cambie lo que esta diciendo** en su terminal (asi ves si avanza, si se atasco o si se
   quedo esperando una pregunta).
2. **Que aparezca el fichero del informe** que le pediste. Ese es el final: cuando existe,
   el vigilante se cierra solo.

### El patron que funciona
Con el tool Monitor de Claude Code (o un bucle en segundo plano si no lo tienes):

```bash
ORCA="/c/Users/xeodeo/AppData/Local/Programs/orca/resources/bin/orca"
SP="<carpeta temporal>"
INF="C:/Users/xeodeo/Desktop/RE4 port/buzon/AGY-<loquesea>.md"
prev=""
while true; do
  # OJO: volver a listar EN CADA VUELTA. Los handle caducan (seccion 2).
  H=$("$ORCA" terminal list --json 2>/dev/null \
      | grep -B4 '"agentIdentity": "antigravity"' | grep -o 'term_[a-f0-9-]*' | head -1)
  if [ -n "$H" ]; then
    "$ORCA" terminal read --terminal "$H" --screen --json > "$SP/mon.json" 2>/dev/null
    cur=$(tr ',' '\n' < "$SP/mon.json" | grep -v '^[[:space:]]*$' \
          | awk '/^      \]/{exit} {print}' \
          | grep -vE 'shortcuts|esc to cancel' | tail -2 | tr -d '\n' | cut -c1-180)
    if [ "$cur" != "$prev" ] && [ -n "$cur" ]; then echo "[agy] $cur"; prev="$cur"; fi
  fi
  if [ -f "$INF" ]; then echo "[agy] ENTREGADO"; exit 0; fi
  sleep 60
done
```

### Las cuatro trampas
1. **Volver a listar el handle en cada vuelta.** Si lo guardas una vez, a la media hora el
   vigilante esta leyendo una terminal que ya no existe y te quedas a ciegas sin enterarte.
2. **Comparar con lo anterior (`prev`).** Si no, te manda la misma linea cada minuto y te ahoga.
3. **Quitar el ruido de la interfaz**: las lineas de rayas, `? for shortcuts` y `esc to cancel`
   cambian solas y disparan avisos falsos.
4. **Un vigilante por entregable.** Se cierra cuando aparece el fichero; para el siguiente
   encargo hay que **volver a armarlo**. Si no, te quedas esperando un aviso que no va a llegar.
   Y si el tool tiene tope de tiempo (el Monitor caduca a los 30 min), rearmalo al caducar.

### Lo que no sirve
- Esperar a que el usuario te avise. Lo hara tarde y mal, y con razon.
- `sleep` largos en primer plano: te bloquean y no puedes hacer nada mientras.
- Vigilar solo el fichero del informe: si agy se queda parado esperando permiso o pregunta algo,
  el fichero no aparece nunca y no te enteras de que esta bloqueado.

### Dos fallos reales del vigilante (07-10-2026), por si los repites
**El filtro del handle se me quedo corto.** Use esto para sacar el identificador:
`grep -B4 '"agentIdentity": "antigravity"'` ... y entre `"handle"` y `"agentIdentity"` hay mas
de 4 lineas (va `title` y `preview` en medio, y el preview es largo). Resultado: el handle salia
VACIO, el vigilante no leia nada y **estuvo media hora en silencio sin avisar de nada**.
Lo que si funciona:
`terminal list --json | grep -E '"handle"|"agentIdentity"' | paste - - | grep -i antigravity | head -1 | grep -o 'term_[a-f0-9-]*'`

**El silencio parecia calma y era un atasco.** Mientras tanto agy estaba metido en el
**paginador de scrollback** (seccion 4), que no cambia solo nunca. Por eso el vigilante ahora
mira tambien si en la pantalla sale `pgup/pgdown` y avisa `ATASCADO en el paginador: necesita
un ESC`. **Regla: si el vigilante lleva mucho sin decir nada, no es que vaya bien; ve a mirar.**
