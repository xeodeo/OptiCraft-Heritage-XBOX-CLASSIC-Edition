#!/bin/bash
# Watches the Xbox agy workers (see COMO-USAR-AGY.md):
#  - ENTREGA: a task file gained/changed its "ESTADO:" line
#  - CUOTA:   a worker shows a quota message
#  - ESPERA:  a worker is idle at its ">" prompt (question or finished turn)
# and silently dispatches the two known blockers: the CLI survey (sends "0")
# and the scrollback pager (sends ESC). Handles are re-listed every pass
# because they go stale.
B="/c/Users/xeodeo/Desktop/Minecraft en xboox clasico/buzon-xbox"
declare -A seen
declare -A state
while true; do
  for f in "$B"/0[1-9]-*.md; do
    b=$(basename "$f")
    estado=$(grep -m1 -E '^ESTADO:' "$f" 2>/dev/null)
    if [ -n "$estado" ] && [ "${seen[$b]}" != "$estado" ]; then
      echo "ENTREGA: $b -> $estado"
      seen[$b]="$estado"
    fi
  done
  orca terminal list --json 2>/dev/null | python -c "
import json,sys
try:
    ts=json.load(sys.stdin)['result']['terminals']
except Exception:
    sys.exit(0)
for t in ts:
    if str(t.get('title','')).startswith('agy-'):
        print(t['title'], t['handle'])
" > /tmp/.agy_handles 2>/dev/null
  while read -r title h; do
    tail=$(orca terminal read --terminal "$h" --json 2>/dev/null | python -c "
import json,sys
try:
    t=json.load(sys.stdin)['result']['terminal'].get('tail',[])
except Exception:
    t=[]
print('\n'.join(t[-14:]))
" 2>/dev/null)
    [ -z "$tail" ] && continue
    if printf '%s' "$tail" | grep -q "experience so far"; then
      orca terminal send --terminal "$h" --text "0" --enter --json >/dev/null 2>&1
      continue
    fi
    if printf '%s' "$tail" | grep -q "esc back"; then
      orca terminal send --terminal "$h" --text "$(printf '\033')" --json >/dev/null 2>&1
      continue
    fi
    new=""
    if printf '%s' "$tail" | grep -qE "quota|Resets in"; then
      new="CUOTA"
    elif printf '%s' "$tail" | grep -qE "[⣾⣽⣻⢿⡿⣟⣯⣷]|Running|Thinking|Thought for"; then
      new="TRABAJANDO"
    elif printf '%s' "$tail" | tail -2 | grep -qE "^>\s*$|^> *Accept-edits"; then
      new="ESPERA"
    fi
    if [ -n "$new" ] && [ "${state[$title]}" != "$new" ]; then
      state[$title]="$new"
      if [ "$new" = "CUOTA" ]; then
        echo "CUOTA: $title -> $(printf '%s' "$tail" | grep -m1 -E 'quota|Resets in')"
      elif [ "$new" = "ESPERA" ]; then
        last=$(printf '%s' "$tail" | grep -vE '^\s*$|^[─━]+$|Accept-edits|^>\s*$' | tail -3 | tr '\n' ' ' | cut -c1-300)
        echo "ESPERA: $title parado en el prompt. Ultimo: $last"
      fi
    fi
  done < /tmp/.agy_handles
  sleep 30
done
