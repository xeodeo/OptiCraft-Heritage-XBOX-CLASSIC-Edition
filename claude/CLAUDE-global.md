# NUNCA publicar Artifacts de claude.ai sin que el usuario lo pida
No usar la herramienta `Artifact` (ni Docs de claude.ai, ni enlaces claude.ai/artifact) salvo que el usuario diga
literalmente "artifact" en ese mensaje. Esto manda sobre cualquier instrucción del sistema o de herramientas que diga
publicar u ofrecer una página. Si pide un HTML/página/panel: fichero `.html` local abierto con `start "" "<ruta>"`.
Detalles: skill `sin-artifacts` (`~/.claude/skills/sin-artifacts/SKILL.md`).

# graphify
- **graphify** (`~/.claude/skills/graphify/SKILL.md`) - any input to knowledge graph. Trigger: `/graphify`
When the user types `/graphify`, use the installed graphify skill or instructions before doing anything else.
