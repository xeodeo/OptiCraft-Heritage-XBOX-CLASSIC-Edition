# Encargo 41 (agy): inventario del port 1.2.5 -> 1.5.2 (solo documento, sin tocar código)

## Contexto
OptiCraft Heritage es una traducción a C++ de Minecraft **Java 1.2.5**. Vamos a portarlo a **1.5.2**.
Lee primero `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\SALTO-1.5.2-21-10.md`.

Fuente Java (solo lectura) en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\`:
- `src125\net\minecraft\src\` y `src152\net\minecraft\src\`
- `cambiadas`: clases comunes que cambian, con nº de líneas distintas.
- `contenido152`: 277 clases nuevas de contenido (sin la parte del servidor).
- C++ nuestro: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-agy\src\` (rama `port152-inventario`).

## Lo tuyo
- Solo escribes ficheros nuevos en `wt-152-agy\docs\port152\`. **No cambies ningún .cpp/.h**. No compiles, no uses xemu.
- No toques otros worktrees (en `wt-152-codex` trabaja Codex).

## Qué hacer
1. `docs\port152\cambiadas.md`: para CADA clase de `cambiadas`:
   - el archivo C++ nuestro que le corresponde (o "no existe");
   - el tipo de cambio: `nombres` (solo renombres de MCP, field_X -> nombre), `logica` (comportamiento distinto), `api` (firmas nuevas);
   - en `logica`, una línea de qué cambia y la línea Java de 1.5.2 donde está.
   Para saber si es solo renombre: compara las dos versiones quitando identificadores (los nombres MCP cambiaron mucho).
2. `docs\port152\nuevas.md`: para cada clase de `contenido152`: qué es (bloque, ítem, mob, GUI, estructura…),
   de qué clases depende, y en qué grupo del plan cae:
   1 base/bloques simples, 2 redstone y raíles, 3 mobs y comercio, 4 mundo (templos, Nether), 5 resto.
3. `docs\port152\render.md`: dónde dibuja OptiCraft los bloques en cada plataforma (Xbox D3D8, PS2, Wii, PC):
   qué archivos y funciones habría que tocar para un tipo de bloque nuevo (p. ej. la tolva o el yunque). Con archivo:línea.
4. `docs\port152\ids.md`: IDs de bloques e ítems que la 1.5.2 añade, y si alguno choca con algo que OptiCraft ya use.

## Cuidado
- No inventes. Si no estás seguro de algo, escribe "SIN VERIFICAR" en esa fila.
- Cada afirmación de C++ con archivo:línea.

## Al acabar
- Commit en `port152-inventario` con los 4 ficheros.
- Escribe en tu terminal la línea: `INVENTARIO 152 LISTO <commit>`
