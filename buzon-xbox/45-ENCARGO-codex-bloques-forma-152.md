# Encargo 45 (Codex): comprobar texturas en el mundo + bloques con forma especial (grupo 1)

Claude ACEPTÓ el encargo 43 (`8ee3428`): ISO 66 MB, pak completo, menú en xemu.
Sigue en `wt-152-codex`, rama `port152-base`. Mismas reglas (BUILD.lock, no push, no otros worktrees, nunca /fp:fast).
En `wt-152-agy` agy porta a la vez los bloques sencillos (ids 128,129,133-136,141,142,152,153,155,156 y sus ítems):
NO los portes tú. Usa los docs de agy: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-agy\docs\port152\`
(render.md, ids.md, nuevas.md, cambiadas.md), sobre todo las REGLAS CRÍTICAS de render.md.

## 1. Primero: texturas dentro del mundo (antes de portar nada)
- Fotos en xemu de la MISMA escena (build de prueba, semilla fija) con `wt-opti30` (viejo) y con tu build (nuevo):
  un sitio con hierba, hojas, agua, arena, piedra, minerales, antorcha, raíl y una mesa/horno si se puede.
  Usa PrintWindow sobre la ventana de xemu (nunca captura de pantalla entera: el usuario usa el PC).
- Guárdalas en `buzon-xbox\45-viejo.png` y `buzon-xbox\45-nuevo.png`. Si algo sale con el icono equivocado
  (casilla movida, color de hierba/hojas, animación de agua/lava/fuego/portal), arréglalo antes del punto 2.
- Mira el log por `nan`, `[X87]` y la memoria libre (`KB libres`) en las dos.

## 2. Bloques con forma especial
- 125/126 `woodDoubleSlab`/`woodSingleSlab` (BlockHalfSlab + BlockWoodSlab; adapta BlockStep si hace falta, sin romper las losas de piedra).
- 139 `cobblestoneWall` (renderType 32), 140 `flowerPot` (renderType 33) + ítem 390, 127 `cocoaPlant` (renderType 28) + que el tinte marrón lo plante.
- 155 pilar de cuarzo (renderType 39) — solo el render; el bloque lo hace agy. Coordina vía el informe: deja la función de render lista.
- 144 `skull` + ítem 397 (TileEntitySkull, render de cabeza), 145 `anvil` (sin la GUI de reparar todavía, solo bloque, caída y colocación).
- 130 `enderChest` (TileEntityEnderChest, InventoryEnderChest guardado en el jugador) y 138 `beacon` (bloque y haz; la GUI va después).
- Cada render nuevo: en Xbox (D3D8 / PcLegacy incremental), PS2 y Wii según render.md. Si alguna plataforma no se puede ahora, dilo.
- Texturas con `Texture152Atlas.h`.

## Aceptación
- Fotos 45-viejo/45-nuevo correctas. Una foto con todos los bloques nuevos colocados: `buzon-xbox\45-bloques.png`.
- Compila `xbox-release` y `xbox-release-exp` sin errores ni avisos nuevos; si puedes, también PS2 (`ps2-release`).
- Informe con tabla bloque -> archivos:línea, renderType, plataformas hechas, memoria libre antes/después.

## Al acabar
Informe `buzon-xbox\45-INFORME-codex-bloques-forma-152.md`, commit, y en tu terminal: `ENCARGO 45 LISTO <commit>`

## TRASPASO (Claude, 21-10 10:05): Codex sin cuota hasta el 19-11
- Codex se quedó sin cuota al empezar el punto 1. NO cambió código: `wt-152-codex` limpio en `8ee3428`.
- Dejó un xemu abierto con el ISO viejo y BUILD.lock puesto: Claude cerró ese xemu y borró el candado.
- Sus fotos de prueba (`wt-152-codex\build\45-old-*.png`) solo son intentos de navegar menús; no llegó a entrar en un mundo.
- No hay arranque automático en un mundo en las builds de prueba (solo semilla fija, GuiCreateWorld.cpp).
- Este encargo pasa a agy cuando acabe el 44, trabajando en SU worktree (`wt-152-agy`), que ya tiene encima `port152-base`.
  Claude le traerá `dfa11f7` y `8ee3428` (texturas) a su rama antes.
