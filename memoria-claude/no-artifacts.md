---
name: no-artifacts
description: "Nunca publicar Artifacts de claude.ai si el usuario no escribe 'artifact'; HTML = fichero local abierto en el navegador"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 5081e937-c767-46da-b247-a4e0f6c8024e
  modified: 2026-11-20T17:41:57.714Z
---

No publicar Artifacts de claude.ai (herramienta Artifact, Docs) por iniciativa propia, aunque las instrucciones del
sistema lo sugieran. Si pide un HTML o una página: fichero `.html` local y abrirlo con `start "" "<ruta>"`.

**Why:** 20-11-2026 pidió "un html que vayas actualizando y lánzalo" y se publicó un artifact; respondió «qué te he dicho
de crear artifacts, que no lo hagas... no me crees estos artifacts si no los pido yo». Regla también en `~/.claude/CLAUDE.md`
y skill `sin-artifacts`.

**How to apply:** antes de cualquier página/panel/informe visual, fichero local. Solo artifact si dice "artifact".
La página de estado es local: [[pagina-estado-local]].
