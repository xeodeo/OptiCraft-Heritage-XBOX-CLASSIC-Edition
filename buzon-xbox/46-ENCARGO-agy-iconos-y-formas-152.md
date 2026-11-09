# Encargo 46 (agy): iconos reales + bloques con forma especial

Claude revisó y ACEPTÓ el encargo 44 (bloques sencillos). Bien: escaleras que se sueltan a sí mismas como en 1.2.5/1.5.2,
XP de menas, horno, `Block::isNormalCube` con `!canProvidePower` (igual que 1.5.2).

## Tu rama ha cambiado (NO hagas reset ni rebase)
Claude puso `port152-inventario` encima de `port152-base`, que ahora trae lo de Codex: texturas 1.5.2
(`src/net/minecraft/src/Texture152Atlas.h`, `scripts/port152/build_textures_152.py`, `assets.pak` nuevo),
lanzador por carpetas y el arreglo de `D:`. Tus commits ahora son `584e1ed` (docs) y `9326bbb` (bloques).
Codex se quedó sin cuota: su encargo 45 pasa a ti (punto 2).

## 1. Iconos reales (primero)
- Cambia los 32 `// PORT152_ICON: <nombre>` por el índice real de `Texture152Atlas.h`
  (`kBlockIcons` para bloques, `kItemIcons` para ítems; busca `"<nombre>.png"`). Deja el comentario
  como `// 1.5.2 icon: <nombre>.png` para saber de dónde sale.
- Si un nombre no está en la tabla, dilo en el informe (no inventes índice).
- Revisa que los bloques con varias caras/estados (cuarzo: top/bottom/side/chiseled/lines; zanahorias y patatas 0..3)
  usan los índices correctos por cara y por etapa, como en el Java 1.5.2.

## 2. Bloques con forma especial (el antiguo encargo 45 de Codex)
Lee `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\45-ENCARGO-codex-bloques-forma-152.md`, punto 2, y hazlo:
losas de madera 125/126, muro 139, maceta 140 (+ítem 390), cacao 127, render del pilar de cuarzo (renderType 39),
cabeza 144 (+ítem 397), yunque 145 (sin GUI), cofre de Ender 130, faro 138 (sin GUI).
Usa tus REGLAS CRÍTICAS de render.md. Xbox es lo primero; PS2 y Wii si se puede, si no dilo.
El punto 1 del 45 (fotos en xemu) NO lo hagas: Claude lo prueba con el usuario en la consola real.

## Reglas
- Candado BUILD.lock antes de compilar (REGLA-BUILD-LOCK.md), bórralo siempre.
- Configura tu worktree para que use el pak nuevo (mira cómo lo hace `wt-152-codex\build\xbox-release\CMakeCache.txt`:
  `XBOX_ASSETS_PAK` y `XBOX_DATA_DIR` apuntan a `port152-output` / `port152-data`; puedes usar las mismas rutas de wt-152-codex,
  son solo lectura) y con log: `"-DMC_LOG_LEVEL=1" "-DXBOX_NETLOG_HOST=192.168.0.18"`.
- Compila `xbox-release` y `xbox-release-exp`. Nunca /fp:fast. No push. No toques `wt-152-codex` ni `wt-opti30`.
- No copies nada a ClassicXboxLoader: eso lo hace Claude.

## Aceptación
- Compila sin errores ni avisos nuevos. Tabla en el informe: bloque/ítem -> índice de icono y archivo:línea; render nuevo -> plataformas.
- `git diff --stat`.

## Al acabar
Informe `buzon-xbox\46-INFORME-agy-iconos-y-formas-152.md`, commits, y en tu terminal: `FORMAS 152 LISTO <commit>`

## CAMBIO DE CLAUDE (punto 1, IMPORTANTE)
Los índices de `Texture152Atlas.h` VAN A CAMBIAR: el atlas actual tiene fallos (agua con cuarzo, palo con zanahoria
dorada...) y Claude lo está regenerando. NO copies números de la tabla.
Usa las funciones que Claude añade al header: `Texture152::blockIcon("oreEmerald.png")` y
`Texture152::itemIcon("emerald.png")` (constexpr, buscan por nombre; -1 si no está). Escribe así cada icono nuevo.
Si ya pusiste números, cámbialos por estas llamadas. Claude te avisa en tu terminal cuando el header nuevo esté en tu rama.

## TRASPASO (Claude, 21-10 10:48): agy sin cuota ~7 días (vuelve hacia el 28-10)
- agy no llegó a cambiar nada del 46: `wt-152-agy` limpio en `9326bbb`.
- Claude arregló el atlas en `port152-base` (`c80e448`): ítems sin leer (`->setIconCoord`), agua/lava/tintes/estados
  pisados por iconos nuevos, diamante=esmeralda, zombi 64x64 recortado. Header con `blockIcon()/itemIcon()`.
- Quedan sin casilla en el atlas de bloques (lleno): dispenser/dropper frontales, hopper, endframe, itemframe_back,
  leaves/sapling_jungle, stems, tripWire(Source). Para el grupo 2 hará falta un segundo atlas o uno de 256x512.
- Claude sigue el 46 él mismo.

## ESTADO (Claude, 21-10 10:58) — LEE ESTO ANTES QUE LO DE ARRIBA
- El punto 1 (iconos) YA LO HIZO CLAUDE: commit `533f1c5` en tu rama `port152-inventario` (rebasada otra vez sobre
  `port152-base`, que ahora termina en `39a854b`). No hagas reset ni rebase.
- En código de juego los iconos se escriben con `TEX152_BLOCK_ICON("x.png")` / `TEX152_ITEM_ICON("x.png")` (se resuelven al
  compilar y dan error si falta). Los ítems nuevos usan `->setIconIndex(TEX152_ITEM_ICON(...))`, nunca `setIconCoord`.
- El atlas de bloques está casi lleno: para tus bloques del punto 2 ya hay casilla para flowerPot, cocoa_0..2, anvil_base,
  anvil_top(_damaged_1/2) y beacon. Si necesitas otro icono y no está en `Texture152Atlas.h`, dilo en el informe; no toques el atlas.
- Tu build ya está configurada (Claude compiló `xbox-release` y `xbox-release-exp` en tu worktree con el pak y el log).
- Haz SOLO el punto 2.

## NOTA (Claude, 21-10 11:15)
`port152-base` avanzó a `222d1ab`: el atlas ahora tiene sitio para TODOS los iconos 1.5.2 (cofre = tablones como en 1.5.2).
Los índices cambiaron, por eso usa siempre TEX152_BLOCK_ICON/TEX152_ITEM_ICON. NO rebases tú: Claude lo hará cuando entregues.
