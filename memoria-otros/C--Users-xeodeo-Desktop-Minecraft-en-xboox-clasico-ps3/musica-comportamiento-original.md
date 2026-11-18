---
name: musica-comportamiento-original
description: The user wants vanilla 1.2.5 music behaviour on Xbox and PS2; upstream changes to it get reverted behind a platform guard
metadata:
  node_type: memory
  type: feedback
  originSessionId: b9320646-46b1-4ea9-beec-04aa1c771a88
  modified: 2026-10-01T19:50:45.457Z
---

On 2026-10-01 the user said plainly: *"yo quiero mantener el funcionamiento original"*, for Xbox **and** PS2, about music. Restored in commit `5f03a7d` on `xbox-opti-30`:
- the music timer only runs inside a world (`PlayerController::onUpdate`), as Minecraft 1.2.5 does;
- the menu plays only the startup theme, then silence — no random menu music;
- no forced track when entering a world (upstream's `triggerMusicNow()`);
- gaps back to 10-20 minutes on PS2 (upstream's `c90eb44` had cut them to 30-90 s, first track at 2-7 s).

All of it behind `#if !(PLATFORM_XBOX || PLATFORM_PS2)` so PC and Wii keep upstream behaviour.

**Why:** the user treats the port as faithful to the original game. Upstream (OptiJuegos) keeps adding "nicer" behaviour to shared files; on Xbox/PS2 that counts as a regression, the same way late trees or mobs do ([[xbox-no-deferred-populate]]).

**How to apply:** after every merge with `origin/main`, check whether music, content timing or other feel-level behaviour changed on Xbox/PS2, and guard it rather than deleting their code — the user prefers switches. Do not touch sound without permission. Related: [[estado-actual]], [[xbox-plan-optimizacion]].
