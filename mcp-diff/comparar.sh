#!/bin/sh
# Revision rapida de un encargo: lineas Java 1.5.2 contra lineas C++ por clase.
# Uso:  sh comparar.sh <worktree> Clase1 Clase2 ...
#       sh comparar.sh <worktree> --commit <hash>    (clases que toca ese commit)
# Una clase con C++ muy por debajo del Java (menos de ~60 %) es casi seguro un cascaron: abrirla y comparar.
# C++ suele salir algo MAS largo que el Java (cabecera .h aparte, comentarios).
W="$(cd "$(dirname "$0")" && pwd)"
T="$1"; shift
S="$T/src/net/minecraft/src"
if [ "$1" = "--commit" ]; then
  set -- $(git -C "$T" show --name-only --format= "$2" | grep -E 'src/net/minecraft/src/.*\.(cpp|h)$' \
           | sed 's|.*/||;s|\.[ch]p*$||' | sort -u)
fi
printf '%-34s %6s %6s %5s\n' clase java cpp+h '%'
for c in "$@"; do
  jf=$(ls "$W"/src152/net/minecraft/*/"$c".java 2>/dev/null | head -1)
  j=$([ -n "$jf" ] && wc -l < "$jf" || echo 0)
  k=$(cat "$S/$c.cpp" "$S/$c.h" 2>/dev/null | wc -l)
  p=$([ "$j" -gt 0 ] && echo $((k * 100 / j)) || echo -)
  flag=""; [ "$j" -gt 30 ] && [ "$p" != - ] && [ "$p" -lt 60 ] && flag="  <-- revisar"
  printf '%-34s %6s %6s %5s%s\n' "$c" "$j" "$k" "$p" "$flag"
done
