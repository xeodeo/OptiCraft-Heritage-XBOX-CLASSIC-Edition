---
name: xbox-no-deferred-populate
description: "Deferred/incremental chunk population was rejected on Xbox — trees, mobs and snow appeared far too late"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 380bbe79-3835-4160-903e-3ed4e54dbf90
  modified: 2026-09-24T14:18:20.667Z
---

Do not enable PLATFORM_DEFERRED_POPULATE / PLATFORM_INCREMENTAL_POPULATE (or the paced disk-load cap PLATFORM_DEFERRED_DISK_LOADS_PER_TICK) on Xbox again. Tried in commit 5fc5d44 (2026-09-24): on the real console trees, passive mobs and snow took a long time to appear; user: "va re mal ... se demora mucho en aparecer los árboles ... no me gusto". Reverted in the next commit.

**Why:** one column decorated per tick under a 3 ms budget can't keep up on the 733 MHz P3; the visible delay counts as quality loss for the user, who prefers occasional hitches over missing/late decoration. xemu hid the problem (faster host).

**How to apply:** any hitch fix that postpones world content (decoration, mob spawns, chunk data) must be validated on the real console, not only xemu; prefer making the work itself cheaper (exact speedups) over deferring it. Related: [[xbox-plan-optimizacion]].
