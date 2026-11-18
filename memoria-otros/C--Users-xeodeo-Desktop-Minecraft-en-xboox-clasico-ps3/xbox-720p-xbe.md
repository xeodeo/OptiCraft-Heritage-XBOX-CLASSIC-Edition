---
name: xbox-720p-xbe
description: Always ship both default.xbe (480) and OptiCraft_720p.xbe; test perf at 480 but keep 720p working
metadata:
  node_type: memory
  type: project
  originSessionId: 1f1fbea3-50a1-408f-826c-f375a335f8f3
  modified: 2026-09-24T02:43:36.444Z
---

Every Xbox build must keep two XBEs in bin/xbox/iso: `default.xbe` (640x480) and `OptiCraft_720p.xbe` (same binary; src/xbox/system/XboxVideoMode.cpp picks 1280x720 when the XBE name contains "720" and the dashboard allows 720p). The cmake post-build step copies it.

**Why:** the user plays on a real Xbox with an Xbox 360 component cable and confirmed (2026-09-23) that 720p works and "looks nice"; they asked to always keep it.

**How to apply:** optimizations are measured at 480p, but check that changes also work at 1280x720 (+~7 MB RAM for the bigger back/depth buffers, GUI scaled). Never drop the 720p copy. Related: [[xbox-consola-real-sse2]].
