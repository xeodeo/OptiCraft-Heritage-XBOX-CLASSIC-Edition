---
name: xbox-plan-optimizacion
description: Where the Xbox performance work stands - the experimental async build is validated on console; what still causes hitches
metadata:
  node_type: memory
  type: project
  originSessionId: 1f1fbea3-50a1-408f-826c-f375a335f8f3
  modified: 2026-10-01T19:53:02.779Z
---

State 2026-10-01: the experimental build (`XBOX_ASYNC_CHUNK_IO`, see [[xbox-builds-prueba-publica]]) is **validated on the real console** — the user called it "joya, lo vi estable". An 11-minute run: no crash, median 57 FPS, hitches ≥80 ms down to 4.2/min (13/min before the async work), `chunkUnload` median 28 ms / max 70 ms (was 94-118 on 29-09). Watch free RAM: it ends around 16.8 MB, lower than the 24 MB of 29-09.

Remaining hitch sources, in order: render (not covered by the named sub-phases), `worldTick`, entity ticks (~34 ms). Next measurements, both agreed but not started:
1. split the `chunkUnload` profiler phase into adopt / populate / requests / save / scan — it is the whole chunk-provider tick hook, not just unloading;
2. entity ticks per mob type — keep the same mob count and AI.

Closed as impossible: LTCG (XDK linker LNK1262) and unity build (~60 name clashes).

**Why:** the user prefers occasional hitches over late content (trees/mobs/snow), wants switches over deleted code, and has agy/OpenCode (free models) do the bulk analysis and code.

**How to apply:** measure with `xbox.spike` / `xbox.async` from the console netlog before and after; xemu does not show real hitches. Branch and status: `buzon-xbox\ESTADO-ACTUAL.md` ([[estado-actual]]). Related: [[xbox-no-deferred-populate]].
