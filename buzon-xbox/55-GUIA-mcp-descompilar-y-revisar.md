# 55. Guía: cómo se descompila Java (MCP) y cómo se revisa un encargo (26-10-2026)

Para cualquier IA o persona que siga el port 1.2.5 -> 1.5.2. OptiCraft es una traducción a C++ de **Java 1.2.5**
(no Beta 1.7.3). La 1.5.2 se porta traduciendo el Java de 1.5.2 tal cual, clase por clase.

## 1. Fuente Java con nombres MCP
Carpeta: `Minecraft en xboox clasico\mcp-diff\` (fuera del repo).

| Qué | Dónde |
|---|---|
| Java 1.2.5 / 1.5.2 descompilado | `src125\net\minecraft\src\`, `src152\net\minecraft\src\` |
| Rehacer todo desde cero | `sh rehacer.sh` (Git Bash) |
| Revisar un encargo | `sh comparar.sh <worktree> --commit <hash>` |

Cómo se hizo (lo repite `rehacer.sh`):
1. **Jars originales**: los de `Downloads\xeolauncher\libraries\com\mojang\minecraft\` (mismo md5 que Mojang).
   RetroMCP los baja también a `v125\jars\` y `v152\jars\`.
2. **Mapeos**: `RetroMCP-Java-CLI.jar setup --version 1.2.5|1.5.2` deja `vXXX\conf\mappings.tiny`.
   El setup acaba con error (maven.glass-launcher.net no responde); da igual, lo necesario ya está.
3. **Renombrar** con `tiny-remapper` 0.10.4: 1.2.5 `client -> named`, 1.5.2 `official -> named` con `--ignoreConflicts`.
   Librerías: lwjgl y jinput de `v125\libraries`. Salen `mc125-named.jar` y `mc152-named.jar`.
4. **Descompilar** con `vineflower` 1.10.1: `--only=net/minecraft`, `-Xmx2G`.
5. **Listas**: `solo125` (clases que desaparecen), `solo152` (nuevas), `comunes`,
   `cambiadas` ("líneas distintas + clase"), `contenido152` (nuevas sin servidor/red/Realms: lo que hay que portar).

Java: `Downloads\xeolauncher\java\java-runtime-delta\bin\java.exe` (21). No hace falta instalar nada más.
Los nombres `func_XXXXX_x` / `field_XXXXX_x` que quedan son de MCP sin nombre; se dejan igual en C++.

## 2. Cómo se revisa lo que entrega Codex o agy
1. `sh comparar.sh <worktree> --commit <hash>`: líneas Java frente a C++ (.cpp + .h) por clase.
   Marca `<-- revisar` si el C++ es < 60 % del Java. C++ suele salir igual o más largo.
   Por debajo del 60 % casi siempre es un **cascarón** (métodos vacíos, `return 0`, TODO). Se abre y se compara con el Java.
2. Leer el diff a mano en lo delicado: redstone, pistones, guardado (NBT), DataWatcher, render.
3. Reglas que más se rompen (cada una ya dio un fallo real):
   - **Entidades**: el constructor de cada entidad concreta con `entityInit()` propio debe llamar
     `ensureEntityInit();` como PRIMERA línea. Si no, el primer acceso al DataWatcher lanza
     "invalid map<K, T> key" (murciélago, bruja, zombi y esqueleto lo tuvieron; arreglado en `4a1919a`).
     Registrar cada entidad nueva en `EntityList` y cada TileEntity en `TileEntity.cpp`.
   - **Bloques que cambian metadata** necesitan `setRequiresSelfNotify()` para redibujarse (maceta).
   - **setBlock de 1.5.2** (flags 1 vecinos, 2 redibujar, 4 sin redibujar): usar `setBlockNotify152`,
     `setBlockMetadataWithNotify152`, `setBlockToAir152`. Los setters viejos de 4 argumentos SÍ avisan a vecinos.
   - `World::removeBlockTileEntity` borra la TE al momento fuera del tick: copiar lo que haga falta antes.
   - Nada de `/fp:fast` (cambia el terreno de una semilla). Nunca push a `origin`.
4. Compilar las dos (`xbox-release` y `xbox-release-exp`) respetando `buzon-xbox\BUILD.lock`.
   En PowerShell `-DXBOX_VERSION_LABEL=1.5.2` se parte en "1" + ".5.2": compilar desde Git Bash o entrecomillar.
5. Mirar el sello en el menú (arriba a la izquierda) y la línea `BUILD` del log de red (UDP 9999, `escuchar_log.py`).
   La línea "build <fecha>" del log es el `__DATE__` de main_xbox.cpp, NO la hora de la XBE.
6. Copiar las 4 XBE a `ClassicXboxLoader\downloads\OptiCraft_extracted\OptiCraft\<versión>\` y lo cambiado a
   `OptiCraft_actualizar\` con LEEME.txt.

## 3. Ramas y worktrees (26-10)
| Worktree | Rama | Qué |
|---|---|---|
| `wt-opti30` | xbox-opti-30 | 1.2.5 buena; `d07650b` arregla el pistón pegajoso de 1 tick |
| `wt-152-codex` | **port152-claude** (activa) | 1.5.2 en la Xbox: `6c7d7ce` + pistón `caa0d45` + tolva `0bcfe5c` + mundo de agy `32d2c15` |
| `wt-152-codex` | port152-base | encima de 6c7d7ce: commits de Codex del encargo 53 RECHAZADOS (`fae11bb`..`61c32f9`), solo referencia |
| `wt-152-agy` | port152-inventario | 1.5.2: agy trabajando el mundo (encargo 54), sin commit aún |

## 4. Pendiente (en orden)
1. HECHO 26-10: **Pistón pegajoso 1.2.5** con reloj de 1 tick: el bloque jalado no se coloca y se queda el 36 (pistón en movimiento).
   Build de diagnóstico "1.2.5 26-10 09:21" en la Xbox; esperar las líneas `xbox.piston place` / `chunk refused`.
   Luego arreglar, quitar logs (`git checkout` de BlockPistonBase, TileEntityPiston, World, Chunk) y pasar el arreglo a 1.5.2.
2. **Grupo 1 resto (encargo 53)**: Codex lo entregó TRES veces mal (la 3ª rellenó ContainerRepair con ~80 métodos sin uso para pasar el 60 %; `comparar.sh` no basta, hay que leer el método principal). Hay que reasignarlo. Antes: (`8035a9c`, `4f452fa`): yunque, faro,
   marco, libros, mapa vacío, zanahoria con palo. Ver `56-RECHAZO-codex-53.md`.
3. HECHO 26-10 (agy `f6e8b9f`, en port152-claude `32d2c15`, alturas verificadas): **Mundo 1.5.2**: alturas de colinas, esmeraldas, lepismas, templos, cabaña de bruja,
   cuarzo del Nether, cofre bonus, biomas grandes.
4. Después: fuegos artificiales, libro encantado, Wither, comercio con aldeanos, tinte de cuero, clonar mapas,
   Espinas, reglas de juego.
5. Optimización: lag del fuego, tolvas buscando sin parar, tirón de 7 s.
