---
name: repos-3ds-referencia
description: "Repos de la comunidad 3DS usados/mirados (traductor 1.12.2, packs, unistore) y para qué sirve cada uno"
metadata:
  node_type: memory
  type: reference
  originSessionId: c46a1df5-dec7-4cf9-b04c-fa56d20303a1
  modified: 2026-11-24T15:11:28.890Z
---

Para el traductor 1.2.5↔1.12.2 (entrar al SMP minecraft3ds.org, informe 121) la referencia fue:
- Re-Craft-3DS https://github.com/SomeRandoLameo/Re-Craft-3DS (cliente 1.12 C++ para 3DS, GPL-3, archivado)
- mclib-3ds https://github.com/SomeRandoLameo/mclib-3ds ← port de plushmonkey/mclib https://github.com/plushmonkey/mclib
  (protocolo Java 1.10–1.12.2: paquetes 340, chunks con paleta, compresión); ejemplo https://github.com/plushmonkey/Terracotta
Anotado en `buzon-xbox\121-ENCARGO-servidores-1.8.9-multiversion.md` (§ REFERENCIA ENCONTRADA).

Packs: https://github.com/wyndchyme/mc3ds-modern (archivado, encargo 123).

Minecraft-3DS-Community (mirado 24-11): https://github.com/Minecraft-3DS-Community/Minecraft-3ds-unistore es una tienda
de Universal-Updater con ~100 mods (packs, shaders, skins) de la edición 3DS oficial, parcheada en binario: NO trae
renderizador/chunks útiles para rendimiento. Lo aprovechable: la IDEA de tienda (índice + iconos) para el Update del
lanzador ([[subir-a-consola-ftp]], buzon 130) y quizá packs/texturas. GamePatches = tabla de direcciones de la ROM 3DS.

24-11 tarde: el juego oficial de 3DS = fork de Pocket 0.15.4 (~1.1.5), última 1.9.19, sin online. El .rar del usuario
(Downloads\00040000001B8700.rar) es el mod NuMC3DS (code.ips + romfs, sin code.bin): no se puede "MCP-ear". Para
contenido nuevo: 1.6.4 desde MCP + Cuberite (C++ Apache) para 1.8-1.12. Idea pendiente: tienda en el lanzador tipo
UniStore. Detalle: buzon 131.
