---
name: commit-sin-a-con-agy
description: "En los worktrees de agy (wt-opti30, wt-152-rend) commitear solo con git add <fichero>, nunca commit -a"
metadata:
  type: feedback
---

Al hacer commit de un cambio de Claude en `wt-opti30` o `wt-152-rend`, mirar antes `git status` y usar
`git add <fichero>` + `git commit`, nunca `git commit -a`.

**Why:** 20-11-2026 un `commit -a` de Claude (flecha del icono de guardado) arrastró 15 ficheros sin commit del encargo
95 de agy; hubo que deshacerlo con `reset --soft` sin perder su árbol.

**How to apply:** compilar siempre desde `wt-build-152`/`wt-build-125` fijados al commit, así el trabajo a medias de agy
no entra en las builds. Relacionado: [[usar-agy-no-subagentes]], [[estado-actual]].
