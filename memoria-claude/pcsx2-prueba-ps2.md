---
name: pcsx2-prueba-ps2
description: PCSX2 del usuario en C:\RetroXeo\emulators\pcsx2 (BIOS lista); cómo compilar la PS2 1.5.2 y lanzarla
metadata:
  type: reference
---

PCSX2 portable en `C:\RetroXeo\emulators\pcsx2` (BIOS europea en `C:\RetroXeo\bios\pcsx2\bios`, HostFs activo).
Lanzar: `pcsx2-qt.exe -fastboot -elf "<carpeta>\OptiCraft.elf"` (lee assets.pak junto al ELF); log en
`logs\emulog.txt`; captura con PrintWindow del proceso `pcsx2-qt`.
Carpeta de prueba del usuario: `Desktop\OptiCraft PS2 prueba\OptiCraftHeritage` (ELF + assets.pak + data\irx).
Compilar PS2 en un worktree: junction `psdevwindows` → la de OptiCraftHeritageEdition, cmake de VS
(`...\CMake\CMake\bin\cmake.exe` + Ninja en PATH), `--preset ps2-release`. El pak: sonidos ADPCM del pak viejo +
assets 1.5.2 de `wt-152-codex\build\xbox-release\port152-data`, con `scripts/make_pak.py`.
Teclas: X=K, O=L, Start=Enter, L1=Q R1=E L2=1 R2=3. Relacionado: [[plataformas-que-hacer]].
