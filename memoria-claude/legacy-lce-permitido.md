---
name: legacy-lce-permitido
description: "User allows taking assets/ideas from the Legacy Console Edition source in Downloads\\minecraft (read-only); button icons and console crafting already done (v1.1, 965efdb)"
metadata:
  node_type: memory
  type: feedback
  originSessionId: b9320646-46b1-4ea9-beec-04aa1c771a88
  modified: 2026-11-06T20:07:43.644Z
---

(06-11: the LCE source now lives in `Desktop\Minecraft en xboox clasico\minecraft-legacy original\`; `Downloads\minecraft` no longer exists.)

On 2026-09-24 the user reversed the earlier "don't use the 4J/Legacy code" rule: "eso de clean room es de él, a mí no me interesa". Assets and code in `C:\Users\xeodeo\Downloads\minecraft` (LCE, ~Java 1.2-1.3 era, Xbox 360/PS3/etc.) may be taken or used as a guide, but that folder must never be modified (the user also rejected me copying files into `Downloads\data`).

**Why:** the clean-room claim belongs to the upstream OptiJuegos project, not this Xbox port. The user wants the look to be "más Legacy" ("copia más código").

**How to apply:**
- Done and pushed in 965efdb (release folder v1.1 replaced; tag xbox-v1.1 moved to 965efdb on 2026-09-25): Xbox button icons (`scripts/xbox/make_button_atlas.py`), console crafting menu `legacy/LegacyCraftingScreen.*` with Legacy 1280x720 scene geometry/colours (`make_crafting_groups.py`, `make_crafting_icons.py`). Generated PNGs are also embedded in the XBE because the user often copies only XBEs.
- Controls as on the 360: Y = normal inventory, X = crafting 2x2, workbench = 3x3. In the port X had no gameplay action before.
- Legacy chunk/lighting ideas are analysed and ranked in buzon-xbox/00-CONTEXTO.md §7, not started.
- Keep [[traspaso-contexto-ia]] (00-CONTEXTO.md) in sync.
