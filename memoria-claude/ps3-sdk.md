---
name: ps3-sdk
description: "PS3 port prep — official SDK 3.70 extracted (not installed) in \"Minecraft en xboox clasico\\ps3\\sdk\\cell\"; short path needed; the user will do the port in another chat"
metadata:
  node_type: memory
  type: project
  originSessionId: f120f6d6-e2de-46cc-9ea4-af7ecf956bac
  modified: 2026-10-29T14:24:14.320Z
---

On 2026-10-29 I prepared `Minecraft en xboox clasico\ps3\`. The plan and the steps are in its `LEEME.md`, and the environment is in `entorno-ps3.sh`/`.bat`.

- The SDK 3.70 (gcc 4.1.1 PPU, SNC, libgcm, sce-cgc) was extracted with 7z from the offline installer, the same way as the XDK [[xdk-sin-instalar]].
- A hello world and the gcm `basic` sample both build to `EBOOT.BIN`.
- gcc 4.1.1 breaks on paths with spaces ("illegal usage of if-file-exist"). Use the short path `MINECR~1`.

The user said the port itself will happen "en otro chat". They chose the leaked Sony SDK over PSL1GHT, and I advised against PhyreEngine.
The 1.5.2 pending list is in `buzon-xbox\64-PENDIENTE-152.md`; see [[port152-estado]].
