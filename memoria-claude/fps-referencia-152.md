---
name: fps-referencia-152
description: "1.5.2 Xbox runs ~60 fps stable with fog, clouds, sun and moon (09-11); the user does not want to lose it — measure every change against it"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-09T16:28:21.519Z
---

On 2026-11-09 the 1.5.2 Xbox build reached ~60 fps stable with fog, clouds, sun and moon (netlog: median 59.9, min 55,
HUD 0.2-0.4 ms, animated textures ~0 uploads/s). User: «iba a 60 fps muy joya, hermoso, con niebla, nubes y todo, que no se pierda eso».

**Why:** this is the best the port has felt; regressions would be very visible to the user.
**How to apply:** for every new feature/encargo (agy or Claude), compare the netlog (`xbox.perf`, `xbox.spike`) against this
reference before handing builds over; flag any drop. Keep the receiver (`xbox-spike\escuchar_log.py`) running when the user tests.
Related: [[estado-actual]], [[usar-agy-no-subagentes]].
