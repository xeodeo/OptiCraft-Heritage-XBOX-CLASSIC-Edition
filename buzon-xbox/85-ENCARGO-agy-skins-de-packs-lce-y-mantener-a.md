# 85. Encargo agy: skins de los packs de la LCE en el selector, y mantener A de 1 en 1 desde el principio (09-11)

## Dónde
- 1.5.2: worktree `wt-152-rend`, rama NUEVA `port152-skins-packs` (ya creada por Claude desde `d8db2d0`). SIN reset.
- 1.2.5: después, cherry-pick a `wt-opti30` rama `opti30-cruceta` (adapta lo que difiera). Tú puedes tocar `wt-opti30`
  SOLO para eso, al final, sin reset.
- NO toques `wt-152-pociones` ni `wt-152-codex`.

## 1. Skins de los packs de la LCE (pedido del usuario: «los packs de skins de los otros packs, en ambas versiones»)
- Fuente (solo leer, nunca modificar): `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\minecraft-legacy original\
  Minecraft.Client\Common\res\TitleUpdate\DLC\<Pack>\` (`Data\x16Data.pck`/`x32Data.pck`, `TexturePack.pck` en Halo,
  MassEffect, Skyrim). El formato `.pck` lo lee la LCE en `Minecraft.Client\Common\DLC\DLCManager.cpp`, `DLCPack.cpp`,
  `DLCSkinFile.cpp`: escribe un extractor en Python (`scripts/xbox/extract_lce_pck.py`) siguiendo ese código (tipos de
  fichero, nombres, propiedades como `DISPLAYNAME` de cada skin). Busca también skins sueltas en esos packs.
- Saca todas las skins (PNG 64x32/64x64) con su nombre visible a `skins/packs/<Pack>/` dentro de los datos del juego:
  1.5.2 `wt-152-codex\build\xbox-release\port152-data\` (+ que el script de datos `scripts/port152/build_textures_152.py`
  las copie, reproducible) y 1.2.5 los datos que use `wt-opti30` (mira su CMakeCache `XBOX_DATA_DIR`). Rehaz los paks
  (`python -I scripts/make_pak.py <datos> <salida>`), y di tamaño antes/después (memoria: no pasarse).
- `SkinManager` / `GuiSkinSelector`: cada pack de la LCE es una pestaña más (además de «Skins originales» y
  «Personalizado»), con su nombre (Halo, Mass Effect, Skyrim, Fantasy...), en las DOS interfaces (carrusel LCE con Legacy
  UI y la pantalla de antes sin ella). Guardar/cargar la skin elegida de un pack (id estable, p.ej. `pack:Halo:<nombre>`),
  J1 y J2. Las vistas previas `_32`/`_Front` genéralas en el script (no en la consola).
- Sin coste de fps fuera del selector; cargar texturas de skins solo al mostrarlas (no todas al arrancar), y liberarlas.

## 2. Mantener A: de 1 en 1 desde el principio (corrección del usuario)
Hoy (commits 84b4059/316988d, `src/xbox/...` y `GuiContainer`): el primer toque de A hace el clic normal y, con el cursor
vacío, ya coge la pila entera, así que mantener no añade nada. El usuario quiere que al MANTENER A coja de 1 en 1 rápido
desde el principio. Hazlo así: el toque corto sigue igual; si A sigue pulsado a los 0,35 s y ese primer clic cogió una
pila de esa misma casilla, devuélvela a la casilla (como Java al volver a clicar) y empieza la repetición de 1 en 1
(creativo: suma 1; supervivencia: pasa 1 de la casilla al cursor). J1 y J2. Sin duplicar ni perder objetos (revisa con
un cofre: total de objetos igual antes y después).

## Reglas
- 60 fps de referencia intactos. Dos interfaces. Texto sin depth test sobre modelos. Restaurar estados de render.
- `BUILD.lock`. 1.5.2: `cmake --build build/xbox-release` y `build/xbox-release-exp` en `wt-152-rend` (si falla por
  objetos de otra rama, reconstruye limpio). 1.2.5: `powershell -File buzon-xbox\build_opti30.ps1` (incluye PS2).
- NO copies al lanzador. Informe `buzon-xbox\85-INFORME-agy.md` (qué packs y cuántas skins, tamaño de paks, qué probar).
- Señal final literal: `ENCARGO 85 LISTO <commit>`
