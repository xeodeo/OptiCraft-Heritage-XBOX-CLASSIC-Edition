---
name: no-fp-fast
description: Never use /fp:fast or any optimization that can change terrain generation results
metadata:
  type: feedback
---

Never enable `/fp:fast` (or any float-relaxing flag or trick) where it could change world generation.

**Why:** 2026-10-08 the user answered "esto nunca que dañe el terreno" when offered /fp:fast for speed: the same seed must keep giving the same terrain.

**How to apply:** performance work may change scheduling, memory, rendering, but never the numeric results of terrain/biome/structure generation. Compiler flag history is in `buzon-xbox\37-rendimiento-v13.md` (/Gw + /OPT:ICF added; LTCG and unity build impossible). Related: [[xbox-plan-optimizacion]].
