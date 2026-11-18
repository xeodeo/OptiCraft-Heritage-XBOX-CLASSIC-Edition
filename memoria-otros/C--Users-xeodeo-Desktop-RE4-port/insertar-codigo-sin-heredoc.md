---
name: insertar-codigo-sin-heredoc
description: "Para meter trazas o bloques de C en un fuente, Write + insertar.py; nunca heredocs de bash ni python inline"
metadata:
  node_type: memory
  type: feedback
  originSessionId: c4712196-64a8-4ad0-989d-fccb2f9260ca
  modified: 2026-11-10T02:04:22.347Z
---

Al insertar codigo C (trazas con `OSReport("...\n")`) no pasar el texto por el shell: ni `python - <<E`, ni `cat <<E`, ni `sed` con `\n`. Escribir el bloque con la herramienta Write y colocarlo con `python ~/.claude/skills/re4-xbox/insertar.py <destino> <bloque> --despues "ancla"`; cambios de una linea con Edit. Y comprobar que `recompilar_juego.sh` no imprime `error:` antes de lanzar xemu.

**Why:** el 09-11-2026 el `\n` de las cadenas se convirtio en salto de linea real cinco veces seguidas ("missing terminating \" character"); cada vez se perdio una compilacion y una prueba en xemu con el objeto viejo. El usuario pidio un metodo fijo para que no vuelva a pasar.

**How to apply:** en cualquier proyecto donde se editen fuentes C/C++ desde Git Bash en Windows; el guion y la regla estan en la skill [[re4-xbox]] (seccion "Meter trazas o bloques de codigo").
