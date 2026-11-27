---
name: opticraft
description: Skill suprema del proyecto OptiCraftHeritageEdition (Minecraft Java 1.2.5 y 1.5.2 porteado a Xbox clásica original y PS2, con derivados PS3/3DS). Inventario completo de repos (locales y remotos), ramas, worktrees, buzón numerado, consolas reales, scripts de compilar/subir, mods portados, SDKs, servidores de prueba y modismos del usuario. Cárgala SIEMPRE y como primer paso antes de tocar cualquier cosa del port (código Minecraft, compilar, probar, consultar estado, portar algo de LCE, mirar logs, actualizar contexto). Debió existir desde el inicio del proyecto.
---

# OptiCraft Heritage Edition — Skill suprema

Base: fork C++ de Minecraft Java **1.2.5** (erluza) y **1.5.2** (port propio) a **Xbox clásica**, con derivaciones a PS2 y PS3. Mas de dos meses de trabajo encima. Esta skill es la fuente única de verdad del proyecto.

## 1. Directorio raíz

**`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\`** (ruta con espacios; siempre comillarla).

```
OptiCraftHeritageEdition\    <-- repo principal, rama HEAD xbox-unificada
wt-build-152\                <-- worktree 1.5.2 activo (port152-int)
wt-opti30\                   <-- worktree 1.2.5 activo (opti30-erluza)
wt-152-rend\                 <-- render/experimentos 1.5.2 (port152-erluza)
wt-152-* (12 mas)            <-- worktrees de ramas port152-* (admin, blq, mp, choco, ui, ...)
wt-125-* (7)                 <-- worktrees de ramas opti30-* (admin, mp, ps2, skin, tex, ui, mods)
wt-agy-152                   <-- worktree de agy (agy-mineria)
wt-build-125                 <-- worktree 1.2.5 (detached HEAD, build)
wt-152-codex                 <-- worktree port152-claude
wt-ps3, wt-ps3-agy           <-- PS3 (ps3-base, ps3-comun)
wt-wifi                      <-- wifi-instalar (lanzador con Update)

buzon-xbox\                  <-- buzon del proyecto: 269 entradas numeradas
contexto-respaldo\           <-- copia local del buzon + memorias (snapshot de la rama contexto)
scripts-claude\              <-- scripts helper (actualizar-contexto.sh, etc)
mcp-diff\                    <-- descompilados y diffs por version (ver seccion 7)

lce-tu19\                    <-- LCE TU19 Xbox One UWP (referencia)
lce-wiiu-reborn\             <-- LCE Wii U Reborn 1.13 (referencia)
lce-ps3-build\               <-- LCE PS3 (referencia)
minecraft-legacy original\   <-- LCE antigua (hasta yunque, sin 1.5)

ps3\                         <-- port PS3 (SDK 3.70, ruta corta MINECR~1)
xbox-sdks\                   <-- 24+ XDK extraidos con 7z (nunca instalar)
xbox-tools\, xbox-spike\     <-- utilidades Xbox (extract-xiso, autopilot, livepad)
xdk\                         <-- XDK activo seleccionado

servidor-1.12.2\             <-- servidor Java 1.12.2 para pruebas MP (usa ProtocolTranslator340)
servidor-1.2.5\              <-- servidor Java 1.2.5 de pruebas
servidor-plugins\            <-- plugins Bukkit/Spigot locales

texturas\                    <-- packs y texturas sueltas
```

## 2. Repo principal `OptiCraftHeritageEdition\`

### Remotes git (3 oficiales + 2 colaboradores)

| Remote | URL | Uso |
|---|---|---|
| `xbox` | `https://github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition.git` | **El que SÍ funciona**. Push/pull aquí. Tiene rama `contexto`. |
| `fork` | `https://github.com/xeodeo/OptiCraftHeritageEdition-XBOX-CLASICO.git` | Fork viejo, **da 403 al pushear**. No usar para push. |
| `origin` | `https://github.com/OptiJuegos/OptiCraftHeritageEdition.git` | Upstream de OptiJuegos (fuente). PRs importantes van aquí. |
| `erluza/*` | fork de colaborador | Base 1.2.5 original. |
| `ramide1/*` | fork de colaborador | 46 commits (arco, dimensiones, multijugador). Encargo 98. |

### Ramas vivas (locales y remotas `xbox/*`)

**Rama HEAD actual del repo principal:** `xbox-unificada` (merge de xbox-port + optimización + OptiJuegos).

**Base de consola (las que acaban en la Xbox real):**
- `port152-int` → `wt-build-152` → **build 1.5.2 oficial**
- `opti30-erluza` → `wt-opti30` → **build 1.2.5 oficial**

**Familia `port152-*` (1.5.2):** `admin, aldeanos, arreglos-83, base, blq, choco, choco2, choco3, claude, controles-84, erluza, graficos-legacy, idiomas, int, inventario, mods, mp, net, off, pociones, portales, ps2, reloj-collar, rendimiento, skin, skins-lce, skins-packs, sub, tex, ui, wither`.

**Familia `opti30-*` (1.2.5):** `admin, choco, choco2, choco3, cruceta, erluza, mods, mp, net, off, ps2, respawn, skin, tex, ui`.

**PS3:** `ps3-base`, `ps3-comun`.

**Otras:** `agy-mineria` (encargos a agy), `wifi-instalar` (lanzador con update), `xbox-async-io`, `xbox-ltcg`, `xbox-online`, `xbox-opti-30`, `xbox-optimizacion`, `xbox-port`, `xbox-upstream-prs`, `main`, `respaldo-*` (1080i, antes-merge-opti, antes-opti-2), `pr/arreglos-dragon-multijugador`, `pr/menu-crafteo-ps2`, `prueba-opti`.

**Rama de respaldo del contexto:** `xbox/contexto` — buzón + memorias snapshoteadas con `scripts-claude\actualizar-contexto.sh` (ver sección 9).

### Worktrees (git worktree list)

| Carpeta | Rama | Qué hay |
|---|---|---|
| `OptiCraftHeritageEdition/` | `xbox-unificada` | Repo principal, integra todo. |
| `wt-build-152/` | `port152-int` | **Build 1.5.2** que va a la Xbox. |
| `wt-opti30/` | `opti30-erluza` | **Build 1.2.5** que va a la Xbox. |
| `wt-152-rend/` | `port152-erluza` | Render/experimentos 1.5.2. |
| `wt-152-codex/` | `port152-claude` | Worktree de experimentos de Claude. |
| `wt-agy-152/` | `agy-mineria` | agy trabajando (AdminMiner y demás). |
| `wt-build-125/` | detached | Build 1.2.5. |
| `wt-152-choco` | `port152-admin` | |
| `wt-152-mods` | `port152-blq` | |
| `wt-152-net` | `port152-mp` | |
| `wt-152-pociones` | `port152-aldeanos` | |
| `wt-152-ps2` | `port152-ps2` | 1.5.2 a PS2 (ya auditado, ver informe 106) |
| `wt-152-skin` | `port152-skin` | |
| `wt-152-sub` | `port152-sub` | |
| `wt-152-tex` | `port152-tex` | |
| `wt-152-ui` | `port152-ui` | |
| `wt-125-choco` | `opti30-admin` | |
| `wt-125-mods` | `opti30-respawn` | |
| `wt-125-net` | `opti30-mp` | |
| `wt-125-ps2` | `opti30-ps2` | |
| `wt-125-skin` | `opti30-skin` | |
| `wt-125-tex` | `opti30-tex` | |
| `wt-125-ui` | `opti30-ui` | |
| `wt-ps3` | `ps3-base` | Base PS3. |
| `wt-ps3-agy` | `ps3-comun` | agy en PS3. |
| `wt-wifi` | `wifi-instalar` | Lanzador con Update por WiFi. |

Regla: `git add` por fichero concreto en worktrees compartidos con agy (`wt-opti30`, `wt-152-rend`, `wt-agy-152`) para no pisar trabajo a medias (memoria `commit-sin-a-con-agy.md`).

## 3. Buzón `buzon-xbox\` (269 entradas)

**Primera y última parada de cualquier sesión.** Estructura:

- `00-CONTEXTO.md` — traspaso rápido para cualquier IA (Claude, agy, OpenCode, ChatGPT). Mantener al día.
- `00-CONTEXTO-antiguo-2026-09-23.md` — histórico (no borrar).
- `PENDIENTES-TODO.md` — lista viva de qué toca.
- `PLATAFORMAS-QUE-HACER.md` — qué falta rehacer por consola (PS2 auditada en informe 106).
- `ESTADO-ACTUAL.md` + `ESTADO-OPTICRAFT.html` + `estado/*.json` — estado por bug/tarea. Regenerar con `estado/generar.py`. Estados: `por_probar`, `en_marcha`, `probado`, `fallo`, `falta`.
- `REGLA-BUILD-LOCK.md` — una build a la vez (candado `BUILD.lock`).
- `BUILD.lock` (si existe) — candado activo.
- `SALTO-1.5.2-21-10.md`, `TRASPASO-hibernado.md` — traspasos largos.
- `PR-N-*.md` + `PR-N-enlace.txt` — tracking de PRs al upstream.
- `01-render.md` ... `112-INFORME-*.md` — **entradas numeradas**:
  - `NN-<tema>.md` para notas propias.
  - `NN-ENCARGO-agy-*.md` + `NN-INFORME-agy.md` para delegaciones a agy (o subagente, u OpenCode).
  - `NN-capturas/` para capturas asociadas.
  - `NN-netlog-*.txt` para logs UDP guardados.
- `scripts-claude/` dentro del buzón (scripts propios del buzón).
- `build_*.ps1` y `montar_release_lanzador.ps1` — compilaciones PowerShell del propio buzón (hay duplicidad con `scripts-claude/` de la raíz; comprobar fecha).
- `web/`, `web.zip`, `portada-reddit.png` — material público.

Cuando una tarea sea larga, dejar el análisis y el plan en `buzon-xbox/NN-<tema>.md` **antes** de implementar, por si se corta la sesión (memoria `anotar-en-buzon.md`).

## 4. Consolas reales y red

| Consola | IP/puerto | Credenciales | Carpeta destino |
|---|---|---|---|
| **Xbox clásica** (FTP) | `192.168.0.3:21` | `xbox` / `xbox` | `F:\Games\OptiCraft\<version>\` (1.2.5, 1.5.2, 1.6.4 cuando toque) |
| **Receptor UDP del log** del juego | `192.168.0.18:9999` | — | PC del usuario, destino del `MC_LOG_*` |
| **PS3** | otra IP (ver skill `ps3`) | — | FTP propio |
| **PS2 real** | — | — | memoria tarjeta/USB (ver `pcsx2-prueba-ps2.md`) |
| **xemu** | local | — | `BUILD.lock` también lo bloquea |
| **ClassicXboxLoader** | lanzador en la Xbox | — | cada build se copia a su subcarpeta; el lanzador las detecta solas |

**Siempre dos XBE por versión y por build:** `default.xbe` (480p) y `OptiCraft_720p.xbe` (720p). El usuario juega 720p. Builds públicas → los dos.

## 5. Compilar y subir

Script estándar: **`scripts-claude\compilar-y-subir.ps1`** (compila 1.5.2 y 1.2.5, normal + experimental = 4 XBE por versión, 8 totales; copia al pack del ClassicXboxLoader; sube por FTP). Cada build genera un **ID corto** (ej. `by2yifc2i`, `b6eonmvyy`) que es el que se usa para hablar de ella.

Otros scripts en el `scripts-claude\` del buzón o de la raíz: `build gcc - release.bat`, `build msvc - release.bat`, `build ps2.bat`, `build wii - game.bat`, `build_prueba.ps1`, `build_public_v12.ps1`, `montar_release_lanzador.ps1`, `pak_tool_gui.bat`, `unpack_assets.bat`.

**Reglas de oro al compilar:**

1. **Nunca `/fp:fast`** (memoria `no-fp-fast.md`) — cambia el terreno de una semilla.
2. **Siempre `-DXBOX_DATA_DIR=...`** o el ISO sale vacío.
3. Compilar **todas las combinaciones**: `1.5.2 normal`, `1.5.2 exp`, `1.2.5 normal`, `1.2.5 exp`.
4. **Candado `BUILD.lock`** activo mientras se compila o se corre xemu.
5. **Dos XBE** por versión (480 + 720).
6. **No tocar firewall** (memoria explícita del usuario: el firewall bloqueó una vez y fue raro, no cambiarlo nunca).
7. **No `--no-verify`** ni skip hooks al commitear.

## 6. Repos de referencia externos

### Legacy Console Edition (LCE) — base para pantallas y contenido 1.6.4

- **TU19 (4J, Xbox One UWP, C++)** → `lce-tu19\`
  Repo: `https://github.com/hugozz26/MinecraftLCE-Xbox` (clon superficial).
  Contenido útil: `Minecraft.World\Recipes.cpp` (tolva, comparador, carro con tolva, fuegos artificiales `FireworksRecipe`+`FireworksMenu*.swf`), `Minecraft.World\` con caballos/riendas/etiquetas/alfombras/paja/dispensador/soltador/bloque de comandos/scoreboard/mapas/comparador/Wither/bruja, `Minecraft.Client\Common\UI\IUIScene_*Menu.cpp` (faro, yunque, tolva, cohetes, caballo, crafteo, creativo, encantar), `Common\Media\Graphics\` (101 PNG), `strings.resx`/`languages.loc`. Es contenido 1.6.4.
  Fuente original (DMCA): `https://github.com/smartcmd/MinecraftConsoles` (retirado 451). Espejo: `https://sourceforge.net/projects/minecraftconsoles.mirror/files/` (ZIP 2,6 GB del 04-05-2026). Mismo código que `lce-tu19\`; no bajar de nuevo.

- **LCE Wii U Reborn (1.13 Aquatic, C++ reconstruido)** → `lce-wiiu-reborn\`
  Repo: `https://github.com/LCEReborn/LCEReborn` (GPL-3, archivado 10-11-2026). Bajado 26-11.
  Útil: `source/game/ui/FireworksMenu`, `IUIScene_CraftingMenu` (menú cohetes 1.5.2), inventario+barra de salto del caballo, modelos caballo/burro con cofre y estandartes (`source/game/client/render/`). Para menú cohetes 1.5.2 y caballos/burros 1.6.4.

- **LCE PS3** → `lce-ps3-build\` (referencia estructural).

- **`minecraft-legacy original\`** — LCE más vieja (hasta yunque, sin 1.5). Referencia extra.

- **Repos LCE borrados o inútiles (21-11-2026)**:
  `GRAnimated/MinecraftLCE` (decomp Switch v1.0.17 ~11 %), `6finality/LegacyEX` (solo launcher), `Moxie-Archive/4jcraft`, `5l41dev/MinecraftLegacy-PS3XBOX`, `chrisGrando/MinecraftLCE-7thGen`, `pcpro27/MinecraftLegacy-PS2`, `Lyall-A/MinecraftConsoles`, `snipecope/Emerald-Legacy-Launcher`, `LCE-Emerald-Launcher`, `DexrnZacAttack/libLCE`, `zugebot/LegacyEditor`.

### Forks del proyecto principal (bolha.dev, Forgejo)

API: `https://bolha.dev/api/v1/repos/search`.

- `erluza/OptiCraftHeritage` — **base** de nuestro port (encargo 87).
- `ramide1/OptiCraftHeritage` — 46 commits (arco, cambio de dimensión, multijugador — encargo 98).
- `toroho/opticrafthorotege` — copia de erluza + rasterizador software, **no sirve**.
- `ramide1/CPPEditionLegacy` — 1.8.9, no sirve.

### Mods portados (siempre descompilados, nunca a mano)

- **Herobrine (iLoveYou, GPL v3)** confirmado por el usuario 22-11.
  Repo: `https://github.com/darkdiplomat/iLoveYou`. Local: `mcp-diff\herobrine-iloveyou\`.
  Altar: netherrack encendido sobre piedra musgosa rodeada de 8 oros, 4 antorchas de redstone alrededor del netherrack.

- **ChocoCraft 1.7.10** (ARR, usuario lo aprobó 22-11) — jar Modrinth descompilado en `mcp-diff\chococraft-1710\`. Implementación nuestra: `wt-build-152\src\mods\chococraft\`.
  Referencia modelo: **ChocoCraft4** (MIT + texturas CC-BY-SA 4.0, citar) en `mcp-diff\chococraft4-ref\`, repo `https://github.com/Mrbysco/ChocoCraft4`.

- **OptiFine 1.5.2** — reutilizado como la LCE, sin avisos de licencia (memoria `optifine-permitido.md`).
  Local: `mcp-diff\optifine152\` y `mcp-diff\srcOF152\`.

- **Nodus 1.5.2** — menú admin tipo Nodus (Water, Jesus, Fly, NoFall, Enderman, etc.).
  Local: `mcp-diff\nodus-152\`. Implementación: `wt-build-152\src\mods\adminmode\`.

### 3DS / otras plataformas

- `mclib` y `Re-Craft-3DS` — referencia para 1.12.2 (encargo 121).
- `mc3ds-modern` — ídem.
- `unistore` — solo idea de tienda.

### Web pública

- `xeodeo.github.io` (rama `gh-pages`) + `ko-fi.com/xeodeo`. Memoria `web-y-kofi.md`.

### Servidores Java reales para pruebas MP

- `servidor-1.12.2\` → usa `ProtocolTranslator340` del cliente 1.5.2 para probar multijugador contra un server 1.12.2.
- `servidor-1.2.5\` → pruebas con el cliente 1.2.5 nativo.
- `servidor-plugins\` → plugins Bukkit/Spigot locales.

## 7. `mcp-diff\` — descompilados y diffs

Carpetas por versión y diffs:

```
mc125-named.jar, mc152-named.jar, mc164-named.jar
src125/  src152/  src164/
l125/    l152/    l164/          <-- LCE equivalentes
n152/                             <-- nodus 1.5.2
contenido152/                     <-- diff de contenido 1.5.2
comunes/  comunes164/            <-- clases comunes
cambiadas/  cambiadas164/        <-- clases cambiadas entre versiones
solo125/  solo152/  solo152v164/  solo164/  <-- clases solo en una
chococraft-1710/, chococraft4-ref/
herobrine-iloveyou/
optifine152/, optifine164/
mclib-ref/
nodus-152/
arnes152/
dec125.log, dec152.log, dec164.log   <-- logs de descompilado
comparar.sh, rehacer.sh               <-- scripts para regenerar
```

## 8. XDK y toolchain

- 24+ versiones de XDK en `xbox-sdks\` (de 2000-06 Alpha 0526 a 5933+). **No instalar nunca**: extraer con `7z x XDKSetup<build>.exe` (memoria `xdk-sin-instalar.md`).
- `xdk\` — XDK activo seleccionado.
- `xbox-tools\` — utilidades (extract-xiso, pak_tool, etc.).
- `xbox-spike\` — experimentos (livepad.py del log UDP, autopilot para capturas automáticas con xemu, netlog.txt).

## 9. Respaldo de contexto (rama `contexto`)

Rama **`xbox/contexto`** del repo `OptiCraftHeritageEdition` tiene todo el buzón + memorias snapshoteadas. Carpeta local gemela: **`contexto-respaldo\`**.

```
contexto-respaldo\
  actualizar-contexto.sh         <-- script que sincroniza todo
  buzon-xbox\                     <-- copia
  claude\                         <-- settings/hooks
  guardados\                      <-- backups
  herramientas\
  mcp-diff\
  memoria-claude\                 <-- copia de ~/.claude/projects/.../memory
  memoria-otros\                  <-- memorias de otras IAs
  README.md
  RESTAURAR.md                    <-- cómo recuperar
```

**Para sincronizar:**

```bash
sh scripts-claude/actualizar-contexto.sh
cd OptiCraftHeritageEdition && git push xbox contexto
```

GitHub fork (`fork/`) **da 403** — nunca pushear ahí. Solo al remote `xbox`.

Memoria `respaldo-contexto.md`.

## 10. Ritual estándar (checklist por sesión)

1. **Al abrir**: leer `buzon-xbox\00-CONTEXTO.md` y `PENDIENTES-TODO.md`. Revisar `estado/` si el usuario pregunta por un bug concreto.
2. **Antes de editar** código Minecraft: comprobar en qué rama/worktree estás. Base de consola = `wt-build-152` (1.5.2) o `wt-opti30` (1.2.5).
3. **Antes de compilar**: ver `BUILD.lock`. Si no está, créalo. `compilar-y-subir.ps1` lo hace.
4. **Al probar** un bug con el usuario en la Xbox real: pídele que arranque el receptor UDP en el PC (`xbox-spike\livepad.py` o el script que tenga) y activa el log si hay flag.
5. **Al cerrar sesión**: actualizar `buzon-xbox\00-CONTEXTO.md`, `PENDIENTES-TODO.md`, `estado/`, y correr `sh scripts-claude/actualizar-contexto.sh` + `git push xbox contexto`.

## 11. Agentes y división de trabajo

- **agy** (Antigravity CLI / Gemini) → tareas largas (ver skill `agy`). Encargo numerado en `buzon-xbox\NN-ENCARGO-agy-*.md` + informe en `NN-INFORME-agy.md`.
- **OpenCode** → también por encargo numerado. `opencode.json` con `allow` para no pedir permisos.
- **Claude Code** (yo) → trabajo corto/medio, máximo 1 subagente propio a la vez (memoria `usar-agy-no-subagentes.md`).
- **subagentes propios** → cuando la tarea es de investigación/búsqueda larga. Resultados → `buzon-xbox\NN-INFORME-subagente-*.md`.

## 12. Modismos y reglas del usuario (no romperlas)

| Modismo / regla | Significado |
|---|---|
| **"mal bien"** | **CONFIRMADO que funciona**, NO es fallo. Ante duda preguntar antes de tocar código. Memoria `mal-bien-modismo.md`. |
| **"ojo con X"** | Cuidado, si lo rompes es gordo. |
| **"relajao"** / **"déjalo donde va"** | No tocar más, revertir si cambiaste. |
| **"dale pues compila"** | Permiso explícito para compilar y subir. |
| Respuestas | **cortas, español**, sin inglés ni tablas largas (memoria `respuestas-cortas-espanol.md`). |
| **Nunca Artifacts** de claude.ai | Salvo que diga literal "artifact". HTML = fichero local. Memorias `no-artifacts.md`, skill `sin-artifacts`. |
| **No `/fp:fast`** nunca | Cambia el terreno de una semilla. |
| **No preguntar en auto/bypass** | Decidir y hacer. Memoria `no-preguntar-decidir.md`. |
| Firewall | Nunca tocarlo. Si bloquea, avisar. |
| Creativo | NO se toca (memoria `dos-interfaces-java-legacy.md`). |
| Modo admin | Sin candado, cualquier server sin op. Memoria `modo-admin-sin-candado.md`. |
| Música | En Xbox/PS2 revertir lo que cambia OptiJuegos (memoria `musica-comportamiento-original.md`). |
| Mods | Descompilar y portar, nunca rehacer a mano. |
| LCE Downloads | Recursos de `Downloads\minecraft` permitidos (memoria `legacy-lce-permitido.md`). |
| Versión base | Java 1.2.5 (no Beta 1.7.3). De LCE solo cosas puntuales. Memoria `version-java-125.md`. |
| 60 fps 1.5.2 | Referencia con niebla y nubes. Toda optimización contra eso (memoria `fps-referencia-152.md`). |
| `git add` con agy | Por fichero concreto, nunca `-a` en worktrees compartidos (`commit-sin-a-con-agy.md`). |

## 13. Estado al 27-11-2026 (resumen rápido)

Build de consola: **1.5.2 `port152-int` ee55f32** + **1.2.5 `opti30-erluza` e4af03f**.

Últimas builds OK: `by2yifc2i` (solo 1.5.2 con Water/Jesus/Enderman), `b6eonmvyy` (1.5.2 + 1.2.5 con fix de lluvia).

Pendientes conocidos: probar lluvia en consola (activar "Clima" + `/weather rain`), probar Jesus/Water split, enderman gaze log, Modrinth slowness (DNS cache + TLS resumption, plan 110), bloque de comandos 1.5.2, vagoneta con generador, chat doble-B + autocompletar MCP, portar Jesus/Water/Enderman a 1.2.5.

Receptor UDP del log murió por OOM del sistema — **no re-arrancar solo**, esperar al usuario.

Para el día a día: memoria `estado-actual.md`.

## 14. Otros ports del mismo usuario (contexto cruzado)

- **RE4 Xbox** → `RE4 port\`, skill `re4-xbox`. Start por `RE4 port\buzon\COMO-VAMOS.md` y `TRASPASO-08-10-noche.md`. Referencia: Melee-X (memoria `melee-x-referencia.md`).
- **BT3 Xbox** (Budokai Tenkaichi 3) → `bt3 port\`, memoria `bt3-xbox-port.md`. Start por `buzon/TRASPASO-21-10.md`. Rama buena `xbox-memoria-pelea` en `bisect\wt-mem`.
- **AnyXboxClasico** → Xbox clásica → PC sin emulador. Desde 06-11 sobre XboxXBERecomp (suffering12, buzon/04). Memoria `anyxboxclasico.md`.
- **Tecmilenio** → skill `tecmi`.

## 15. Cuándo cargar esta skill (triggers)

- Primera acción al abrir conversación sobre Minecraft / OptiCraft / Xbox clásica / PS2 / Wii / consolas del proyecto.
- Antes de tocar `wt-*`, `OptiCraftHeritageEdition\`, `buzon-xbox\`, `mcp-diff\`, `lce-*\`, `contexto-respaldo\`.
- Antes de compilar, subir, probar, portar.
- Antes de contestar "qué repos hay", "dónde está X", "qué falta", "qué ramas", "qué carpeta es esa".
- Al final de sesión, antes de actualizar el contexto.
- Si el usuario dice "continuar", "retomar", "qué iba", "qué estaba haciendo".
