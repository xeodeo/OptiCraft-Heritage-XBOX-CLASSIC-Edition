---
name: xbox-consola-real-sse2
description: Why the Xbox port failed on real hardware but not xemu (unconditional SSE2 in VS2022 CRT) and how real-console testing is done (UDP net log)
metadata:
  node_type: memory
  type: project
  originSessionId: 46825c14-fe35-4a23-8c96-a67eeb09c272
  modified: 2026-09-23T20:37:58.052Z
---

On 2026-09-23 the OptiCraft Xbox port first booted on the user's real (softmodded) console after removing unconditional SSE2 from the VS2022 static CRT. xemu executes SSE2 even though it reports a Pentium III, so it cannot catch these bugs.

**Why:** the Pentium III has SSE1 only; the MS CRT is prebuilt for SSE2 and some paths skip the CPU check (__acrt_fltout cvttsd2si, __libm_sse2_log10_precise, __ceil_default/__floor_default, ldexp/scalbn, _87except).

**How to apply:**
- Fixes live in src/xbox/runtime/XboxP3Math.c (x87 replacements) and scripts/xbox/patch_sse2_moves.ps1 (post-link SSE2→SSE1 move rewrite plus x87 stubs in a code cave).
- Remaining warnings the build prints (frexp, powhlp, handle_exc, num_put wide) are paths assumed unused; revisit if a real-console crash points there.
- Real-console testing: the user launches the `bin/xbox/iso` folder from HDD (not the ISO). The build sends the log over UDP (XBOX_NETLOG_HOST=192.168.0.18, the user's PC) to `xbox-spike/escuchar_log.py` → netlog.txt. The Xbox is at 192.168.0.3 (FTP xbox:xbox); T:\debug.log = E:\TDATA\FFFF4F43\debug.log.
- Launching the game kills the dashboard's FTP, which is why the network log exists.
