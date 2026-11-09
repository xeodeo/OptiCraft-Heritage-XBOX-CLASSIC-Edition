# 38 — Packs de texturas en Xbox (08-10, commit 5c24962)

- Se cargan de `D:\texturepacks\` (carpeta del juego): un `.pak` por pack (`scripts/make_pak.py`
  sobre la carpeta del pack) o una carpeta suelta. Código: `TexturePackList.cpp` (rama Xbox),
  `TexturePackPak.*`.
- Menú: el original, **Mods → Texture Packs** (`GuiTexturePacks`), ahora con cruceta: arriba/abajo
  mueve, A aplica, B sale. El pack en uso sale con el nombre en amarillo.
- Packs de Legacy: `python scripts/xbox/make_legacy_texture_packs.py "<minecraft-legacy original>" <salida>`
  crea "Legacy Console" + los 9 DLC (Halo, Skyrim, MassEffect, Candy, City, Fantasy, Halloween,
  Natural, Plastic). Los x32 se reducen a x16. No se toca `gui/` salvo `gui/items.png`.
- Los DLC son contenido de terceros: **no van en la release pública**, solo para la consola del usuario.
- En ClassicXboxLoader: `OptiCraft\texturepacks\*.pak` (300-750 KB cada uno).
