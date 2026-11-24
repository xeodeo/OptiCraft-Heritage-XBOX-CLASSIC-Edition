---
name: subir-siempre-al-compilar
description: Toda build de Xbox compilada se sube sola al lanzador y a la consola: usar buzon-xbox/scripts-claude/compilar-y-subir.ps1
metadata:
  type: feedback
---

Compilar con `powershell -File buzon-xbox\scripts-claude\compilar-y-subir.ps1 -Which "152,125" -Presets "xbox-release-exp,xbox-release"`:
coge BUILD.lock, compila en wt-build-152 / wt-opti30, copia al lanzador (con respaldo) y sube por FTP, reintentando 10 min.

**Why:** el usuario dijo (23-11) «sube así te quedes sin créditos, asegura que se suba al compilar».
**How to apply:** si el FTP no responde (juego abierto), decirlo y volver a subir en cuanto responda. Ver [[subir-a-consola-ftp]], [[xbox-loader-copy]].
