# Encargo 43 (Codex): texturas de la 1.5.2 en el formato de OptiCraft

Sigue en `wt-152-codex`, rama `port152-base`, encima de `924eac1` (aceptado). Mismas reglas:
candado BUILD.lock, no push, no tocar otros worktrees.

## El problema
OptiCraft usa los atlas de la 1.2.5: `terrain.png` (bloques) y `gui/items.png` (ítems), rejillas 16x16 de iconos,
y cada bloque elige su icono con un índice (`blockIndexInTexture`, `getBlockTextureFromSideAndMetadata`...).
La 1.5.2 usa archivos sueltos y un atlas cosido en tiempo de ejecución (TextureMap/Stitcher/Icon):
- `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\textures\blocks\` (257 png)
- `...\mcp-diff\src152\textures\items\` (205 png)
- Atlas 1.2.5 originales: `...\mcp-diff\src125\terrain.png` y `src125\gui\items.png`.
- Datos reales del juego: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data` (no está en git; no lo modifiques).
En consola NO queremos el cosido en tiempo de ejecución (memoria y tiempo de carga).

## Qué hacer
1. Averigua y documenta (archivo:línea) cómo carga y usa OptiCraft `terrain.png` e `items.png` en Xbox, PS2, Wii y PC:
   tamaño del atlas, si admite más de 256 iconos, si los texture packs (`buzon-xbox\38-texture-packs.md`) dependen
   de que el atlas sea 256x256, y cómo empaqueta los datos (`scripts/make_pak.py`, assets.pak).
2. Lista qué iconos de 1.5.2 son NUEVOS (no existían en 1.2.5) para bloques y para ítems, y cuántas casillas
   libres quedan en los atlas 1.2.5. Para eso, empareja cada png de 1.5.2 con su casilla 1.2.5 comparando píxeles.
3. Propón y aplica la solución más barata para consola. Opciones a valorar:
   (a) meter los iconos nuevos en las casillas libres;
   (b) un segundo atlas `terrain2.png` / `items2.png` con su propio índice;
   (c) ampliar el atlas a 256x512.
   Elige con números: memoria de textura en Xbox (64 MB compartidos con NV2A), PS2 (VRAM 4 MB) y Wii.
4. Escribe un script en `scripts/port152/` (Python) que genere los atlas desde los png de 1.5.2 y una tabla
   `nombre_icono -> índice` en un .h generado, para que los bloques nuevos lo usen.
   No metas los png de Mojang en git: el script los lee de `mcp-diff` y escribe en la carpeta de datos de build.
5. No portes todavía ningún bloque; solo la infraestructura de texturas y su tabla.

## Aceptación
- Compila `xbox-release` sin errores ni avisos nuevos.
- Informe con la decisión y sus números, la lista de iconos nuevos y las casillas libres.
- Una imagen del atlas generado en `buzon-xbox\43-atlas.png` para que Claude lo mire.

## Al acabar
- Informe `buzon-xbox\43-INFORME-codex-texturas-152.md`, commit, y en tu terminal: `ENCARGO 43 LISTO <commit>`

## DECISIÓN DEL USUARIO (cambia el punto 3)
- Se traen TODAS las texturas de la 1.5.2: las nuevas y también las de bloques/ítems que ya existían
  (el aspecto debe ser el de 1.5.2 en todo). No se mezclan con las de 1.2.5.
- Hay que generar el `.pak` nuevo de datos con esas texturas (mira `scripts/make_pak.py` y cómo se arma assets.pak
  en el build de Xbox). El .pak va en la carpeta de build/salida, no en git.
- Sigue tocando elegir el formato del atlas (casillas, segundo atlas o atlas más grande) con números de memoria,
  pero ya no hay que conservar las casillas viejas: reordena lo que haga falta, con la tabla nombre -> índice
  para que los bloques existentes apunten a su icono 1.5.2. Cuida que los bloques existentes sigan mostrando
  su textura correcta (compara en el informe una lista de bloques viejos -> icono que usarán).
- Revisa también gui/, particles, armor, mob, environment, misc, etc. de la 1.5.2 (`mcp-diff\src152\`)
  y si cambian respecto a 1.2.5 tráelos al .pak igualmente, salvo lo que rompa la interfaz de consola (dilo).
- Texture packs (38): que sigan funcionando o explica qué haría falta.

## REVISION DE CLAUDE (dfa11f7)
Aceptado lo principal: atlas 256x256 conservando índices viejos (buena decisión), tabla Texture152Atlas.h, pak por entradas
(AssetPak lee por offset, no carga los 54 MB). D9025 /arch:IA32 ya existía (XboxP3Math.c), ok.
Falta arreglar:
1. La ISO pesa 127 MB porque lleva `data/` (56 MB) Y `assets.pak` (54 MB). Resources_XBOX.cpp resuelve primero el pak,
   así que `data/` sobra. Cuando `XBOX_ASSETS_PAK` está puesto, NO copies `data/` a la ISO y borra el `data/` viejo que
   ya quedó en `bin/xbox/iso/` (si no, imagebld lo vuelve a meter). Comprueba antes que el pak tiene TODO lo que hay en
   data/ (assets y resources, también música y sonidos): lista lo que falte.
2. Que funcione igual con los presets `xbox-release-exp` (y el XBE 720p): mismo pak.
3. Di en el informe cuánto pesa la ISO final y cuánto tarda en arrancar el juego en xemu con el pak (solo si xemu
   está libre: candado BUILD.lock).
Al acabar: commit y `ENCARGO 43 LISTO <commit>` de nuevo.
