---
name: xbox-data-dir-worktrees
description: Compilar Xbox desde un worktree exige -DXBOX_DATA_DIR o el ISO sale sin recursos
metadata:
  node_type: memory
  type: project
  originSessionId: 17d8c769-4034-4dff-b756-c714384a9786
  modified: 2026-10-03T16:57:43.322Z
---

El árbol de datos del juego (`data\`, 804 ficheros, 56 MB) **no está en git**: vive solo en
`Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data`. Los worktrees (`wt-opti30`,
etc.) no lo tienen. `XBOX_DATA_DIR` vale por defecto `${CMAKE_SOURCE_DIR}/data`, así que al
compilar desde un worktree CMake solo avisa y sigue: el ISO sale **de 9 MB sin recursos** y el
juego no arranca.

**Why:** pasó el 03-10-2026 montando la release v1.2; el fallo es silencioso (warning, no error)
y solo se ve por el tamaño del ISO.

**How to apply:** para cualquier build que se vaya a publicar desde un worktree, pasar
`"-DXBOX_DATA_DIR=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data"`
(entre comillas en PowerShell). Comprobar después: ISO de unos 64 MB y `bin\<preset>\iso\data`
con 804 ficheros. Ver [[xbox-builds-prueba-publica]] y [[una-compilacion-a-la-vez]].
