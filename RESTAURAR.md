# Cómo restaurar el proyecto después de formatear

Esta rama (`contexto`, en `github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition`) guarda TODO lo que no es código:
buzón de trabajo, memorias de Claude, guías de agy, scripts del MCP. El código está en las otras ramas del mismo repo.
Se actualiza con `sh actualizar-contexto.sh` (desde `Desktop\Minecraft en xboox clasico\contexto-respaldo\`).

## 1. Carpetas (todo cuelga de `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\`)
```
git clone https://github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition.git OptiCraftHeritageEdition
cd OptiCraftHeritageEdition
git remote rename origin xbox
git remote add origin https://github.com/OptiJuegos/OptiCraftHeritageEdition.git   # upstream: NUNCA push aquí
git remote add fork https://github.com/xeodeo/OptiCraftHeritageEdition-XBOX-CLASICO.git
git worktree add ../wt-opti30       xbox-opti-30         # 1.2.5 buena
git worktree add ../wt-152-codex    port152-claude       # 1.5.2 buena
git worktree add ../wt-152-pociones port152-aldeanos     # 1.5.2 rama de prueba
git worktree add ../wt-152-rend     port152-graficos-legacy   # rama de agy (o la que diga ESTADO-ACTUAL)
git worktree add ../contexto-respaldo contexto           # esta rama
```
Luego copia `contexto-respaldo\buzon-xbox` a `..\buzon-xbox` y `contexto-respaldo\mcp-diff` a `..\mcp-diff`.

## 2. Memorias de Claude Code
Copia `memoria-claude\*` a
`C:\Users\xeodeo\.claude\projects\C--Users-xeodeo-Desktop-Minecraft-en-xboox-clasico-OptiCraftHeritageEdition\memory\`.
`claude\CLAUDE-global.md` → `C:\Users\xeodeo\.claude\CLAUDE.md`. `claude\skills\*` → `C:\Users\xeodeo\.claude\skills\`.
`claude\COMO-USAR-AGY.md` → Escritorio. Para empezar un chat: «lee buzon-xbox\ESTADO-ACTUAL.md».

## 3. Herramientas
- Visual Studio 2022 Community (C++), CMake de VS, Python 3 (con Pillow), Git, 7-Zip, Java 21.
- **XDK sin instalar**: `7z x XDKSetup5849.exe` en `xdk\5849\` (ver memoria `xdk-sin-instalar.md`; los instaladores
  estaban en `xbox-sdks\`, NO están en GitHub: guárdalos tú en un disco aparte).
- xemu en `C:\RetroXeo\emulators\xemu\xemu.exe` (para autotest/capturas).
- agy (`agy --dangerously-skip-permissions`) y Orca (terminales); ver `claude\COMO-USAR-AGY.md`.
- PS2: `psdevwindows/` (ver `docs/XBOX_PORT.md`, fase 11).

## 4. MCP (Java descompilado con nombres)
Guía completa: `buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`. Resumen:
- `mcp-diff\rehacer.sh` vuelve a crear `src125\` y `src152\` desde los jars originales de Mojang
  (`Downloads\xeolauncher\libraries\com\mojang\minecraft\` o los que baja RetroMCP a `v125\jars`, `v152\jars`).
- Herramientas en `mcp-diff\tools\`: RetroMCP-Java-CLI (mapeos `setup --version 1.2.5|1.5.2`), tiny-remapper 0.10.4,
  vineflower 1.10.1. Java 21.
- `mcp-diff\comparar.sh <worktree> --commit <hash>`: líneas Java vs C++ por clase (marca cascarones < 60 %).
- Arnés Java de la semilla/spawn: `mcp-diff\arnes152\` (ver `buzon-xbox\77-INFORME-semilla-152.md`).
- **OptiFine 1.5.2** (no se sube: código cerrado): `OptiFine_1.5.2_HD_U_D5.zip` del archivo
  `github.com/SynArchive/OptiFine-Archive`; se remapea y descompila igual que el juego a `mcp-diff\srcOF152\`
  (pasos en `buzon-xbox\74-INVESTIGACION-optifine-152.md`; clases tocadas en `mcp-diff\optifine152\clases-tocadas.txt`).

## 5. Lo que NO está en GitHub (guárdalo aparte antes de formatear)
- `minecraft-legacy original\` (fuente de la LCE, se usa como referencia de interfaz).
- `xdk\` y `xbox-sdks\` (XDK de Microsoft).
- Datos del juego: `Downloads\data` y `wt-152-codex\build\xbox-release\port152-data` (o regenerarlos con
  `scripts/port152/build_textures_152.py`, que parte de los jars de Mojang).
- `Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\` (las builds del lanzador; se rehacen compilando).
- Otros proyectos: `Desktop\RE4 port`, `Desktop\port bodokaii 3`, `ps3\`, `lce-ps3-build\`.
