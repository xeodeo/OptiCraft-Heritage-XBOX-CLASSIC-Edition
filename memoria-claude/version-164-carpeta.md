---
name: version-164-carpeta
description: When the 1.6.4 port starts it must ship in its own launcher folder OptiCraft\1.6.4, like 1.2.5 and 1.5.2
metadata:
  type: project
---

The 1.6.4 port (after finishing 1.5.2) goes in its own folder in the launcher and on the console:
`ClassicXboxLoader\downloads\OptiCraft_extracted\OptiCraft\1.6.4\` and `F:\Games\OptiCraft\1.6.4\`, with its own
4 XBEs + assets.pak, like the other versions (user 2026-11-23).

**Why:** the launcher detects one folder per version; the user wants each version side by side.

**How to apply:** when starting 1.6.4, create the folder, a new build tree and launcher entry; never overwrite
1.5.2. Related: [[xbox-loader-copy]], [[port152-estado]], [[que-es-de-cada-version]].
