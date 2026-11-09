# 34 — Release v1.2 montada (03-10, 11:55)

Está en **`C:\Users\xeodeo\Desktop\release\v1.2\`**. **Falta publicarla en GitHub** (eso lo hace
el usuario) y falta el tag.

## Qué hay dentro

| Fichero | Tamaño | Qué es |
|---|---|---|
| `OptiCraft-Xbox-v1.2.zip` | 55 MB | La carpeta `OptiCraft\` para el disco duro de la Xbox, con **los cuatro XBE** y `data\` (804 ficheros) |
| `OptiCraft-Xbox-v1.2.iso` | 64 MB | Versión **normal**, para xemu |
| `OptiCraft-Xbox-v1.2-exp.iso` | 64 MB | Versión **experimental** (async), para xemu |
| `RELEASE.md` | | Notas en español |
| `REDDIT.md` | | Texto para Reddit, español e inglés |

Dentro del zip, en `OptiCraft\`:
- `default.xbe` y `OptiCraft_720p.xbe` — normal (idénticos entre sí: el 720p lo elige el nombre)
- `OptiCraft_exp.xbe` y `OptiCraft_exp_720p.xbe` — experimental
- `default.tbn` (portada) y `data\`

MD5 de lo que se publica:
```
dc48b1d45e82b1789c4a66908c07a10d  OptiCraft-Xbox-v1.2.zip
65a1ff39f63bc333482425462e001d47  OptiCraft-Xbox-v1.2.iso
06c578e99a3ee78f9d40cad4dcc1918f  OptiCraft-Xbox-v1.2-exp.iso
```

## De dónde sale
Rama **`xbox-opti-30`** (`wt-opti30`), HEAD en `5f03a7d` (música como el original).
Son las versiones **públicas**, sin log por red (`MC_LOG_LEVEL=0`, sin `XBOX_NETLOG_HOST`):
presets `xbox-public` y `xbox-public-exp`.

Script que las compila: **`buzon-xbox\build_public_v12.ps1`** (compila) y
**`build_public_v12_data.ps1`** (reconfigura con el árbol de datos y reempaqueta el ISO).

## Un fallo que había que arreglar y conviene recordar

La primera pasada sacó un **ISO de 9 MB en vez de 64**: `XBOX_DATA_DIR` vale por defecto
`${CMAKE_SOURCE_DIR}/data`, y **el worktree `wt-opti30` no tiene carpeta `data\`** (es un árbol
grande que no está en git, vive solo en la carpeta principal `OptiCraftHeritageEdition\data`).
CMake avisa de ello y sigue adelante, así que el ISO sale sin recursos y el juego no arranca.

**Para cualquier build que vaya a publicarse desde un worktree hay que pasar:**
```
-DXBOX_DATA_DIR=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data
```
Comprobación rápida: el ISO tiene que pesar unos **64 MB**, y `bin\<preset>\iso\data` tiene que
tener **804 ficheros**. Si el ISO pesa 9 MB, falta el árbol de datos.

Se comprobó que los dos árboles de datos (el de la carpeta principal y el desplegado en
`Downloads\ClassicXboxLoader\...\OptiCraft\data`) son **idénticos**: 804 ficheros, mismos tamaños.

## Probado
Las dos ISO arrancan en xemu y llegan al menú con el panorama y las texturas bien.
Capturas: `buzon-xbox\34-v12-xemu-normal.png` y `34-v12-xemu-exp.png`.
**No se ha probado el zip en la consola real**; los XBE son el mismo código que el usuario ya
jugó, pero compilado sin el log por red.

## Qué falta
1. **Probar el zip en la consola real** antes de publicar (copiar `OptiCraft\` al disco duro).
2. Commit y `git push xbox xbox-opti-30:main`; después el tag `xbox-v1.2` y
   `git push xbox refs/tags/xbox-v1.2`. **Ojo:** `origin` es OptiJuegos, ahí no se hace push.
   Recuerda que el commit `5f03a7d` todavía no estaba subido.
3. Crear la página de la release en GitHub (lo hace el usuario) con los tres ficheros y el
   texto de `RELEASE.md`.
4. Las **dos PR a OptiJuegos siguen sin enviar** (`PR-1-enlace.txt`, `PR-2-enlace.txt`).

## Nota sobre `assets.pak`
El zip lleva la carpeta `data\` suelta, **no** el `assets.pak`, igual que la v1.1. El juego sabe
montar `D:\assets.pak` si está, pero no se ha publicado nunca así y no se ha probado una release
solo con el pak. Si algún día se quiere, hay que generarlo con `scripts\make_pak.py` y probarlo
en consola real antes.
