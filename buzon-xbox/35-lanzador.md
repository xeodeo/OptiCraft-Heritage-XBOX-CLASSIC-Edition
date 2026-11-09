# 35 — Lanzador de versiones (08-10)

`default.xbe` es ahora un **lanzador** (estilo launcher clásico de Minecraft en Java):
panorama arriba, panel con la descripción de la versión, y abajo sobre tierra el desplegable
**Version** + botón **Play**. Arranca con `XLaunchNewImage` uno de:
`OptiCraft.xbe`, `OptiCraft_720p.xbe`, `OptiCraft_exp.xbe`, `OptiCraft_exp_720p.xbe`.
Siempre abre en la normal (lo pidió el usuario). Si falta un XBE, sale en gris y avisa.

- Código: `wt-opti30\src\xbox\tools\XboxLauncher.cpp` (commit `6c10231`). Lee los PNG de
  `D:\data\assets` (fuente, gui, legacy\panorama.png, legacy\title.png).
- Compilar: preset **`xbox-launcher`** → `bin\xbox-launcher\iso\default.xbe` (316 KB).
- Los builds del juego ahora dejan también `OptiCraft[_exp].xbe` y `OptiCraft[_exp]_720p.xbe`.
- Montar la release: `buzon-xbox\montar_release_lanzador.ps1 -Version v1.2`.
- Probado en xemu: abre, la lista funciona, elegir + A arranca el juego y llega al menú.
  Captura: `35-lanzador-xemu.png`. **Falta probar en consola real.**
- Para probar con teclado en xemu hay que poner `port1 = 'keyboard'` en `xemu.toml`
  (normalmente es el mando físico). Enter NO es START en el teclado de xemu; A sí es A.
  Ojo: si el usuario escribe con xemu en primer plano, su Enter le llega al juego.

## Release v1.2 rehecha con el lanzador
`Desktop\release\v1.2\`: zip (OptiCraft\ con lanzador + 4 XBE + data) y **una sola ISO** con
todo. La anterior está en `Desktop\release\v1.2-sin-lanzador\`. RELEASE.md actualizado.
MD5: zip `41d1a09f02bca9bcb4fe387b68858cfc`, iso `865704db8a394f3683f3151a06594947`.

**Ojo:** en el remoto `xbox` YA existe un tag `xbox-v1.2` que apunta a `965efdb` (el commit de la
v1.1). Al publicar hay que moverlo a la rama nueva (borrar y volver a crear), con permiso del usuario.

## ClassicXboxLoader
`default.xbe` = lanzador; el normal de prueba (con log) pasó a `OptiCraft.xbe`.
Respaldo: `OptiCraft_respaldo_08-10_lanzador`. `OptiCraft_1080i.xbe` sigue siendo viejo.

## 21-10: una carpeta por versión (commit en `port152-base`, wt-152-codex)
El lanzador ya no tiene lista fija: busca carpetas junto a `default.xbe` y lista los
`OptiCraft[_exp][_720p].xbe` que tenga cada una, por orden de nombre (`version.txt` opcional = descripción).
Al arrancar un XBE de una subcarpeta, D: pasa a ser esa carpeta: cada versión lee solo sus datos.
ClassicXboxLoader `OptiCraft\`: `default.xbe` (lanzador) + `data\assets` (5 PNG del lanzador),
`1.2.5\` (4 XBE + 1080i viejo + assets.pak + data + texturepacks), `1.5.2\` (OptiCraft[_720p].xbe + assets.pak).
Lanzador viejo en `OptiCraft_respaldo_21-10_lanzador_viejo`. Falta pasar este lanzador a `xbox-opti-30`.
