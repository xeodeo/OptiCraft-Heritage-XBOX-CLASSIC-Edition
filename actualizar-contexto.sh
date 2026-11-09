#!/bin/sh
# Copia el contexto del proyecto a esta carpeta y lo sube como rama "contexto" al remoto "xbox".
# Uso (Git Bash): sh actualizar-contexto.sh
set -e
AQUI="$(cd "$(dirname "$0")" && pwd)"
RAIZ="$AQUI/.."
MEM="/c/Users/xeodeo/.claude/projects/C--Users-xeodeo-Desktop-Minecraft-en-xboox-clasico-OptiCraftHeritageEdition/memory"

rm -rf "$AQUI/buzon-xbox" "$AQUI/memoria-claude" "$AQUI/mcp-diff" "$AQUI/claude"
mkdir -p "$AQUI/mcp-diff/optifine152" "$AQUI/claude/skills"

cp -r "$RAIZ/buzon-xbox" "$AQUI/buzon-xbox"
rm -f "$AQUI/buzon-xbox/BUILD.lock"
cp -r "$MEM" "$AQUI/memoria-claude"

# MCP: scripts, listas, arnés y herramientas (el código descompilado se regenera con rehacer.sh)
for f in rehacer.sh comparar.sh solo125 solo152 comunes cambiadas contenido152 urls125.txt l125 l152 n152 dec125.log dec152.log; do
  [ -e "$RAIZ/mcp-diff/$f" ] && cp -r "$RAIZ/mcp-diff/$f" "$AQUI/mcp-diff/"
done
cp -r "$RAIZ/mcp-diff/tools" "$RAIZ/mcp-diff/arnes152" "$AQUI/mcp-diff/"
rm -f "$AQUI/mcp-diff/arnes152/"*.class
for f in clases-tocadas.txt dec.log remap.log; do
  [ -e "$RAIZ/mcp-diff/optifine152/$f" ] && cp "$RAIZ/mcp-diff/optifine152/$f" "$AQUI/mcp-diff/optifine152/"
done

# Guías y skills de Claude/agy
cp /c/Users/xeodeo/Desktop/COMO-USAR-AGY.md "$AQUI/claude/" 2>/dev/null || true
cp /c/Users/xeodeo/.claude/CLAUDE.md "$AQUI/claude/CLAUDE-global.md" 2>/dev/null || true
for s in agy orca-cli; do
  [ -d "/c/Users/xeodeo/.claude/skills/$s" ] && cp -r "/c/Users/xeodeo/.claude/skills/$s" "$AQUI/claude/skills/"
done
find "$AQUI/claude" -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true

cd "$AQUI"
[ -d .git ] || { git init -q -b contexto; git remote add xbox https://github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition.git; }
git add -A
NOMBRE="$(git -C "$RAIZ/OptiCraftHeritageEdition" config user.name)"
CORREO="$(git -C "$RAIZ/OptiCraftHeritageEdition" config user.email)"
git -c user.name="$NOMBRE" -c user.email="$CORREO" commit -q -m "Contexto del proyecto $(date +%d-%m-%Y_%H:%M)" || echo "sin cambios"
git push -u xbox contexto
