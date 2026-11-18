#!/bin/bash
# (Re)launches one agy worker in a visible Orca tab of the OptiCraft worktree
# and hands it its buzon task.   usage: lanzar_agy.sh <nombre> <tarea.md> <modelo>
set -u
NAME="$1"; TASK="$2"; MODEL="$3"
WT='id:47f8062e-c12e-4ef5-9ad0-e94f90f44588::C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition'
B='C:\Users\xeodeo\Desktop\Minecraft en xboox clasico'
CMD="agy --model $MODEL --dangerously-skip-permissions --add-dir \"$B\\buzon-xbox\" --add-dir \"$B\\xbox-spike\" --add-dir \"$B\\xdk\\5849\\sdk\\XDK\\xbox\\include\""
H=$(orca terminal create --worktree "$WT" --title "agy-$NAME" --command "$CMD" --json 2>/dev/null \
    | python -c "import json,sys; print(json.load(sys.stdin)['result']['terminal']['handle'])")
echo "agy-$NAME handle=$H model=$MODEL"
# Wait for the prompt line of the agy TUI (up to ~90 s).
for i in $(seq 1 30); do
  sleep 3
  tail=$(orca terminal read --terminal "$H" --json 2>/dev/null \
    | python -c "import json,sys; print('\n'.join(json.load(sys.stdin)['result']['terminal'].get('tail',[])))" 2>/dev/null)
  if printf '%s' "$tail" | grep -qiE "Accept-edits mode|skip|bypass|permissions"; then break; fi
  if printf '%s' "$tail" | grep -q "trust this folder"; then
    orca terminal send --terminal "$H" --text $'\r' --json >/dev/null 2>&1
  fi
done
MSG="Eres agy-$NAME y CONTINUAS una sesion anterior. Lee $B\\buzon-xbox\\00-CONTEXTO.md (incluida la Nota del coordinador) y tu tarea $B\\buzon-xbox\\$TASK entera, incluido tu TRASPASO 'ESTADO: PAUSADO' y la 'Verificacion del coordinador' si la hay. Continua desde el siguiente paso que dejaste. Solo tus ficheros, compila con compile_one.ps1, sin git, sin crear scripts dentro del repo (usa $B\\xbox-spike para temporales). Al terminar, sustituye tu TRASPASO por uno nuevo con primera linea ESTADO: TERMINADO."
orca terminal send --terminal "$H" --text "$MSG" --enter --wait-submit 10 --json 2>&1 | grep -m1 '"ok"' | sed "s/^/agy-$NAME send: /"
