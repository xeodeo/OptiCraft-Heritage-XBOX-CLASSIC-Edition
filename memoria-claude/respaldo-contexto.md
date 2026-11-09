---
name: respaldo-contexto
description: "Backup of non-code context (buzon, memories, agy guides, MCP scripts) in branch \"contexto\" of the xbox remote; rerun actualizar-contexto.sh"
metadata:
  node_type: memory
  type: reference
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-09T17:02:28.351Z
---

Since 2026-11-09 the context lives in branch `contexto` of `github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition`
(remote `xbox`), folder `Desktop\Minecraft en xboox clasico\contexto-respaldo\` (own git repo). `sh actualizar-contexto.sh`
copies buzon-xbox, these memories, COMO-USAR-AGY.md, global CLAUDE.md, skills agy/orca-cli, MCP scripts/lists/tools and
pushes. `RESTAURAR.md` there explains rebuilding everything after a format (worktrees, XDK, MCP via rehacer.sh, OptiFine mirror).
Code: all branches pushed to `xbox` (`git push xbox --all`); remote `fork` answers 403 from this PC. Never push to `origin`.

**How to apply:** when the user asks to "subir todo", push branches to `xbox` and rerun the script. Not uploaded (tell the
user to keep them on a disk): LCE source, XDK/xbox-sdks, game data dirs, OptiFine jars, other ports.
