---
name: re4-xbox
description: Compilar, arrancar en xemu y diagnosticar el port de Resident Evil 4 a la Xbox clasica (las dos ramas: la nativa de la decompilacion y la vieja de recompilacion). Usala cuando haya que construir el XBE, probar la ISO, leer el log del juego por el gdbstub o averiguar donde se queda parado el arranque.
---

# RE4 en Xbox clasica: compilar, probar y diagnosticar

Hay **dos ramas vivas**. No las confundas.

| | nativa (la buena) | vieja (recompilacion) |
|---|---|---|
| Carpeta | `C:\Users\xeodeo\Desktop\RE4 port\xbox-nativo` | `...\RE4 port\nwiirecomp-xbox\xbox` |
| Que es | la decompilacion de RE4 compilada para x86 | el binario de PowerPC traducido + GX emulado |
| Estado | se juega entero en la Xbox real, con audio (12-11-2026) | llega al menu, lento y sin TEV |
| Empezar por | `RE4 port\buzon\COMO-VAMOS.md` (el RELEVO de arriba) | `RE4 port\buzon\XBOX-ESTADO.md` |

## Regla permanente: dejarlo anotado SIEMPRE (pedido del usuario, 13-11-2026)

Cada vez que se termina un bloque de trabajo (un arreglo subido, una medida de la consola leída, un
diagnóstico que cambia), **antes de responder al usuario**:

1. **`RE4 port\buzon\COMO-VAMOS.md`**: añadir encima de "Fallos abiertos" qué se hizo, qué se midió (con cifras),
   qué quedó sin subir o sin medir, y lo siguiente. Commit en la raíz.
2. **Esta skill** (`~\.claude\skills\re4-xbox\SKILL.md`): si apareció una herramienta nueva, una receta que
   funcionó, una trampa o un dato que cambia cómo se trabaja, anotarlo aquí en su sección. No esperar a que el
   usuario lo pida. Lo que es estado del día va a COMO-VAMOS; lo que es "cómo se hace" va aquí.
3. Commit del código en su repositorio (`xbox-nativo`, `re4`) y guardar el mapa del build
   (`logs\mapas\debug_<commit>.map`).

Si una afirmación de esta skill deja de ser cierta, corregirla en el momento.

## Regla de oro: el candado

**Antes de compilar o de abrir xemu**, coge
`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock`.
Si existe, **espera**: lo tiene otro (agy, OpenCode, el usuario probando). Borralo al terminar.
Los guiones de compilar ya lo hacen solos.

## Compilar

**Rama nativa, solo nuestra capa** (lo normal, tarda ~1 min; reusa los 354 objetos del juego):
```powershell
powershell -ExecutionPolicy Bypass -File "C:\Users\xeodeo\Desktop\RE4 port\xbox-nativo\scripts\compilar_plataforma.ps1"
```
Tiene que imprimir `compilacion de la plataforma ok` / `enlace ok` / `xbe ok` / `iso ok`.
Si anades un fichero fuente nuevo, metelo en `$sinXdk` o `$conXdk` **y** su `.obj` en `$nuevos`.

**Rama nativa entera** (recompila tambien el juego con MinGW, tarda mucho mas):
`...\xbox-nativo\scripts\compilar_nativo.ps1`

**Rama vieja:** `...\nwiirecomp-xbox\xbox\scripts\re4_compilar.ps1`

## Ciclo rapido de prueba (rama nativa, desde el 08-10)

Estado y siguiente paso: el RELEVO de arriba de `RE4 port\buzon\COMO-VAMOS.md`.

1. Si cambiaste codigo del juego (`re4\src`, `re4\include`), recompila esos objetos en WSL:
   `MSYS_NO_PATHCONV=1 wsl -e bash "/mnt/c/Users/xeodeo/Desktop/RE4 port/xbox-nativo/scripts/recompilar_juego.sh" -`
   (`-` = todos, ~2 min en paralelo; o un patron de grep: `'libgpu.h'`). Si tocaste `types.h` o `gx.h`, todos.
2. `powershell -ExecutionPolicy Bypass -File "...\xbox-nativo\scripts\probar_xemu.ps1" log.txt 50`
   compila la plataforma, abre xemu, espera, guarda el log en `xbox-nativo\logs` y resuelve la
   excepcion (EIP y pila contra el .map). Deja BUILD.lock puesto: borralo al terminar.
3. Cuelgue sin excepcion: `python scripts\pila_gdb.py` (para el juego y lista la pila). Si en la
   pila sale `0xc0000005` con el kernel parado, es una excepcion que nadie atrapo.
4. Foto de la ventana de xemu: GetWindowRect + CopyFromScreen en PowerShell y leer el PNG.

Reglas del port: endianness con `DISC_BE`/`DISC_PTR` (`re4\include\be_disc.h`) o `GC_BE32`, nunca
`__builtin_bswap` suelto; direcciones de GameCube con las macros de `types.h` (`GC_MEM_LO/HI`,
`GC_NOT_PTR`, `GC_RELOCATED`, `GC_LOCKED_CACHE`...). En SN todas valen lo original.

## Consola real (lo que de verdad cuenta; xemu NO sirve para medir tiempos)

IP 192.168.0.3, FTP `xbox:xbox`. **El FTP se cierra con un juego abierto**: el usuario tiene que salir al
dashboard. Comprobar: `(Test-NetConnection 192.168.0.3 -Port 21).TcpTestSucceeded`.

Hay **tres carpetas instaladas**, cada una con sus datos (1,4 GB):

| Carpeta en la consola | Fichero local | Variables al compilar |
|---|---|---|
| `F:\Games\RE4 Nativo` (final, sin depuracion) | `bin\consola_final.xbe` | `RE4_NETLOG=1 RE4_SIN_TRACE=1 RE4_FINAL=1` |
| `F:\Games\RE4 Nativo Debug` (textos, barras, log) | `bin\consola_debug.xbe` | `RE4_NETLOG=1 RE4_SIN_TRACE=1` |
| `F:\Games\RE4 Nativo O2` (experimento, NO adoptado) | `bin\consola_final_O2.xbe` | como el final, con el juego en `-O2` |

**Para diagnosticar, que el usuario juegue en Debug.** El final solo deja pasar al log `[NATIVO]`, `[MED]`,
`[TIRON]`, `[GXD] f...` (memoria), la sala que carga (`.das`), el vaciado de caches y cualquier "fallo".

1. Compilar cada una con `compilar_plataforma.ps1` (SIN `RE4_AUTOPAD*`), copiar `bin\iso\default.xbe` a su
   `bin\consola_*.xbe` y `bin\RE4Xbox.map` a `bin\RE4Xbox_consola.map` / `bin\RE4Xbox_consola_final.map`.
   Los objetos del juego son los mismos para las dos: el juego pregunta `xbox_build_final()` al arrancar.
   Si la compilacion falla justo despues de matar xemu, repetir (el ISO sigue cogido un momento).
2. Subir las dos: `powershell -ExecutionPolicy Bypass -File scripts\subir_todo.ps1`.
   Una sola: `python scripts\subir_xbox.py xbe <fichero.xbe> "<carpeta>"`. Datos: `subir_xbox.py datos "<carpeta>"`.
3. Log por UDP 9999 al PC. El escuchador es un proceso suelto (comprobar que vive tras hibernar):
   `python -u "...\Minecraft en xboox clasico\xbox-spike\escuchar_log.py" 9999 >> xbox-nativo\logs\consola_netlog.txt`
4. Medir: linea `[MED]` (una cada 60 fotogramas). 1 unidad = 2^20 ciclos = 1,43 ms. `cuadro` = fotograma entero,
   `dl` listas sin cache, `dlc` con cache (huesos incluidos), `vsync` = parado esperando al refresco (tope del
   juego: 30 fps = 2 refrescos, `cuadro` 24), `espera` = GPU. Comparar siempre a igual `vtx=`.
5. Experimento `-O2` del juego: `wsl -e env RE4_OPT=2 bash recompilar_juego.sh -` (objetos `-O1` guardados en
   `build\game_objs_O1`; **copiar ahi los `.o` nuevos cada vez que se recompile algo del juego** y restaurarlos
   tras compilar la O2). El 12-11 la O2 gano poco y dio un paron al saltar una ventana: no adoptada.

### Leer una sesion de la consola (recetas que funcionaron el 12-11)

El log es `xbox-nativo\logs\consola_netlog.txt` (mas de 10 MB): **nunca leerlo entero**. Cortar la ultima sesion
por la ultima linea `[NATIVO] arrancando` a un temporal y trabajar sobre eso:
- **Salas visitadas**: `grep "FastOpen.*\.das"`.
- **Reparto del fotograma por carga**: agrupar `[MED]` por `vtx/100000` y promediar `cuadro`, `dl`, `dlc`,
  `vsync`; "resto" = `cuadro - dl - dlc - vsync`. Referencia en el pueblo (12-11): 200.000 vertices -> 25-33;
  300.000 -> 32-41; con la cache en el minimo (r101) 300.000-400.000 -> 82-97 con `dl` 31-43.
- **Memoria**: `[GXD] f...`: `libres=` (KB), `aram=` (KB comprometidos), `kb=` (cache de geometria; su
  presupuesto es usado + libres - 5 MB, minimo 3 MB), `vs=` (programas de vertices, tope 512),
  `hit/miss`. Si `libres` baja de ~4 MB aparecen `[VS] ensamblado FALLO hr=8007000e` y `CreateTexture fallo`.
- **Tirones**: `[TIRON] f= cuadro= aplicar= | vs=N(t)[dsk= ens= asm= crt=] rc= tex=`. `ens` = programas de
  vertices ensamblados (39 ms cada uno en la consola); `dsk` = salidos de `Z:\re4_vs.bin` (casi gratis). Un
  `[TIRON]` sin creaciones junto a lecturas de `.das` es una carga de sala, no partida.
- **Camino lento de CPU**: `[MED] ... cpu=N conv= dib=` distintos de 0 y `[VS] prog... A CPU` (el programa no
  cabe en 136 instrucciones: muchas luces con foco/distancia + texgen) o `[VS] por CPU: <motivo>`.
- **Excepcion**: recortar las lineas de `EXCEPCION` y `[PILA]` a un fichero y `python scripts\resolver_excepcion.py
  <fichero>` (usa `bin\RE4Xbox.map`: tiene que ser el mapa del mismo build).
- **`DVD: Read Error : <ruta>`** = fichero que no esta en el disco. Este disco solo trae `ss/Eng` (si falta la
  carpeta de idioma se usa `eng`) y no trae la etapa 3.

### Perfilador de CPU en la consola (desde el 13-11; solo Debug)

Dice qué funciones se comen la CPU. Es lo primero que hay que mirar ante un "va lento": el 13-11 se perdió un
día optimizando la GPU cuando el límite era la CPU.

- `gx_d3d.cpp` `PerfBucle`: un hilo para el hilo del juego ~1000 veces por segundo y busca en su pila el marco
  de la interrupción (EIP, CS=8, EFLAGS). Cada 20.000 muestras escribe `[PERF] ---- ...` y 60 líneas
  `[PERF] dirección n`. La Xbox no tiene `GetThreadContext`.
- Pedir al usuario que se quede **quieto 2-3 minutos** en el sitio malo: cada bloque son ~20 s.
- Resolver: `python scripts\perfil.py <log de la sesión> <bloques 3,4,5> <mapa> [--obj]`.
  **El mapa tiene que ser el del build que generó el log**: se guardan en `logs\mapas\debug_<commit>.map`
  (el de `bin` se pisa en cada compilación; las direcciones del mapa van desplazadas +0x3F0000).
- Para saber qué bloque es qué sitio: agrupar los `[MED]` entre cabeceras `[PERF] ----` (cuadro, vtx, dl, dlc)
  junto con la sala (`FastOpen ... .das`). Receta en awk en COMO-VAMOS (13-11).
- "nucleo" = muestras fuera del XBE (esperas, interrupciones); no se reparte.
- Para ver el punto exacto dentro de una función: desensamblar el `.obj` en WSL
  (`i686-w64-mingw32-objdump -d -M intel src/gxd/gx_d3d.obj`) y cruzar con los desplazamientos de los cubos.

### Otras medidas de Debug (13-11)

- `[CARGA]` / `[CARGA2]` / `[CARGA3]` (cada 60 fotogramas): vértices por la GPU repartidos por número de luces
  del programa, instrucciones × vértice, tipos de luz, y en las listas estáticas vértices antes/después de
  fusionar y ejecuciones con caché FIFO de 16/24. Son cuentas exactas: valen también en xemu.
- `[DLC] ... fallos:` por 120 fotogramas: `no_ent` (lista sin entrada), `host` (lista en orden nativo), `mem`,
  `val` (falló la validación), `tam`. Si `miss` en `[GXD]` es grande, mirar aquí el motivo antes de culpar a la memoria.
- `RE4_MEDIR_HOST=1` al compilar: `[HOST]` lista qué listas nativas se dibujan y si cambian.
- `[SINTEX]` (etapa que pide textura y no la tiene) y `[COPIAPIDE]` (copias del EFB que pide el juego).

### Probar varias salas en xemu de una vez

`powershell -ExecutionPolicy Bypass -File scripts\probar_salas.ps1 "1 03;2 05;2 08" 100 nombre`
(etapa y sala en hexadecimal; `1 00` = el pueblo por el camino normal). Deja `logs\nombre_rXYY.txt` y `.png` de
cada una, imprime excepciones, `[GXD]`, fallos de la caché y el último `[MED]`, y al final devuelve `config.txt`
a la sala de la consola y suelta el candado. Una sala suelta: `python scripts\sala_config.py 4 06` (sin argumentos
vuelve a la de la consola). **Nunca `sed -i` sobre `config.txt`**: quita los CRLF y el juego peta en `ConfigSet`.
El mando automático entra en la sala pero no se mueve ni gira: para ver algo concreto (el agua) hace falta el usuario.

### Lo que se aprendió el 13-11 sobre rendimiento (no repetir)

- **El límite en la consola es la CPU, no la GPU** (`espera=0`: la GPU acaba sin trabajo). Bajar un 28 % la carga
  de vértices de la GPU no movió los fps. Una cuenta teórica con datos de xemu no sustituye al perfil de la consola.
- **Leer memoria de vídeo con la CPU es lentísimo** (búfer de pantalla, vertex buffers: memoria write-combined).
  Leer 256x256 del búfer de pantalla por fotograma era el 15-20 % de la CPU. Copias y reducciones: con la GPU
  (`CopyRects` + dibujo a una textura `D3DUSAGE_RENDERTARGET`), y después marcar todo el estado como sucio.
- **El Pentium III no precarga solo y tiene 128 KB de caché**: en bucles que recorren datos dispersos o grandes,
  `_mm_prefetch` unos vértices por delante (huesos: 38 -> 33 unidades mirando al coche).
- **Búsquedas lineales con `memcmp` en cada cambio de material se pagan** (`BuscarVs` era el 12-14 %): comparar
  primero una huella de 32 bits en una tabla aparte.
- **Una sola lista puede ser el problema**: la lista común de los efectos (cuadrado de 96 bytes en orden nativo)
  se dibuja 185-345 veces por fotograma y la caché la rechazaba. Mirar `fallos:` y contar listas distintas.
- La caché de geometría fusiona vértices repetidos (`RE4_SIN_FUSION` lo apaga). La reordenación de triángulos está APAGADA (`RE4_CON_ORDEN`): rompía el pelo de Ashley (capas semitransparentes: el orden dentro de la lista se ve). No cambiar el orden de dibujo de ninguna malla.
- Las copias del EFB a textura solo están activas para el agua (origen que cabe en el EFB, destino 128 o 256
  potencia de 2). La ventana del coche de la intro pide 512x512 (más alto que la pantalla) y sigue sin dibujarse.
- Si se cambia código del juego (`re4\src`), copiar el `.o` nuevo también a `build\game_objs_O1`.

### Cachés y cambios de sala (13-11)
- Las cachés de geometría y texturas se vacían SOLO al cargar una sala: al abrir `StN/rXXX.das` (`dvd_stub.c`) y
  con módulos `stN...` (`os_stub.c` `caches_por_modulo`). Entre salas del mismo capítulo el módulo de sala no se
  vuelve a enlazar: la señal fiable es el `.das`. Con armas, enemigos e inventario solo se revalida.
- Si `libres` baja mucho dentro de una sala, mirar si faltan vaciados; si hay tramos de `dl` alto tras abrir el
  inventario o aparecer enemigos, mirar si sobran (`grep "cache vaciada\|se revalida\|OSLink"`).
- agy prueba en xemu con el MISMO puerto de gdb (1235): nunca matar xemu ni borrar el candado sin mirar.
  `probar_salas.ps1` espera si hay candado ajeno u otro xemu. Al mandar `/model` u otra orden con `/` por
  `orca terminal send` desde Git Bash: `export MSYS_NO_PATHCONV=1` (si no, llega `C:/Program Files/Git/model`).

### Integrar entregas de agentes (15-11)
- Las ramas de agy acumulan todo su historial (`agy-cache-listas`, `agy-sonido`): **cherry-pick del commit
  entregado**, nunca merge de la rama entera (arrastra cambios rechazados, p. ej. `64e68c8`).
- Si un cambio toca el reparto de constantes del programa de vértices o el texto que genera `GenerarVs`, subir
  `VS_DISCO_VERSION` (si no, `Z:e4_vs.bin` devuelve programas viejos).
- Las fotos de los agentes suelen ser de escenas distintas antes/después: rehacerlas en la misma escena.
- Codex no compila (sin WSL ni clang): compilar su commit con `recompilar_juego.sh` antes de nada.
- Los respaldos por CPU que reescriben código del juego (p. ej. el skinning de huesos v2) tienen que seguir el MISMO
  orden de sumas y las mismas escalas GQR que el original (`Sk1Loop` en `trans.cpp`: carga * 2^-escala, suma la
  traslación también en normales, guarda * 2^escala); si no, difieren en el último bit.
- En código del juego (MinGW) no dividir enteros de 64 bits: genera `__udivdi3`/`__divdi3` de libgcc y el enlace
  con MSVC falla (`LNK2019`). Pasar por `double`. Comprobar con `i686-w64-mingw32-nm <obj> | grep " U "`.
- En el perfil, `_main` +0x5a7/+0x5e7 (`game_main.o`) es el bucle que espera al refresco (tope de 30 fps): no es
  trabajo; descontarlo al repartir la CPU.
- **Los fps van a saltos 30/20/15 y el juego es de paso fijo** (`re4\docs\ritmo-fotogramas.md`): 30 fps exige
  `cuadro` <= ~23 unidades; bajar de 38 a 31 no se nota (las dos cosas son 20 fps). Medir contra ese objetivo.
- **La barra amarilla del medidor del juego** (Debug) es el tramo hasta `RENDER SETUP` (`main.cpp:311`): incluye
  `Render()`, que dibuja el OT del fotograma anterior por nuestra capa. Si se llena, el coste es de dibujo.
- **Umbrales de memoria: xemu tiene varios MB MÁS libres que la consola** (xemu 7-10 MB donde la consola da 2-4 MB).
  Un umbral fijo de "libres" que nunca salta en xemu puede saltar siempre en la consola (16-11: presupuesto de
  texturas a 5,5 MB -> 70-120 texturas recreadas por fotograma). Probar esos cambios en la consola o relativos.
- **Log de `probar_salas` vacío (0 bytes) y sin foto = xemu se CERRÓ** (gdb rechaza la conexión), no es que el
  juego no llegue. Bisectar con los interruptores (`RE4_SIN_*`) sala a sala. El 16-11 fueron las copias NPOT en r101.
  Si cambias un fuente a mitad de un `probar_salas` de varias salas, las siguientes salas pueden salir vacías.
- Para esperar en segundo plano a que acabe otro `probar_salas`, no buscar 'probar_salas' en las líneas de comando
  con un `powershell -c` (se encuentra a sí mismo y espera para siempre): esperar al candado y a que no haya xemu.
- **r101 en xemu (18-11, el usuario)**: al entrar sale el "Playing Manual" (varias páginas) y hay que pulsar B varias
  veces. Antes del 18-11 el mando automático solo lo cerraba si pasaba por r100: las pruebas de r101 se quedaban en el
  manual (`vtx` ~29.000) y nunca vimos su carga real (~360.000). Desde `eb54cf0` pulsa B cada segundo 40 s en cada
  sala (`g_autopad_sala`). Si la foto de `probar_salas` sale con un menú o un manual, el dato no vale.
- **En la consola el límite de r101 era la MEMORIA, no la CPU del juego** (18-11): el perfil daba 77 % decodificando
  listas que no cabían en la caché y < 3 % de lógica del juego. xemu tiene más memoria libre y no lo ve (r101 `dl` 1-4
  en xemu, 30-54 en la consola). Antes de optimizar código, mirar `libres`, `aram=`, `kb=` y `[DLCPRIME-TEX] cache=`
  (si está en el tope, hay recreación de texturas: esperas en `XPhysicalAlloc` en el perfil).
- **ARAM al disco** (`ar_stub.c`, `[ARAMSWP]`, `RE4_SIN_ARAM_SWAP`): bloques no usados van a `Z:\re4_aram.swp` al cambiar
  de sala y 3 s después. El MEZCLADOR también lee la ARAM (`ax_dsound.cpp` `AramRead`) y en zonas no esperadas: cualquier
  cambio aquí tiene que mantener "marcar `g_aram_mezclador`, LUEGO mirar `g_aram_pag`" y el estado 3 (saliendo).
- **Caché de programas de vértices** (`gx_vs.inl`): vive en `D:\re4_vs.bin` (carpeta del juego) desde `9c36021`; tope
  `VS_INDEX_MAX` 4096 (con 1024 estaba LLENA en la consola y nada nuevo se guardaba: tirones de 39 ms por programa en
  cada evento, siempre). Tras una sesión larga del usuario: `python scripts\vs_juntar.py` (consola en el dashboard)
  junta las de las tres carpetas y las reparte. Mirar `[VS] cache en disco: N programas` al arrancar.
- **clang** (`RE4_CLANG=1 RE4_PGO=re4.profdata`, objetos en `build\game_objs_pgo`, `scripts\recompilar_juego_clang.py`):
  usar **sin LTO** (`RE4_LTO=0`). Con ThinLTO el pueblo da una excepción reproducible (`DrawOTag`) que no aparece con
  ninguna mitad de objetos por separado. Si se cambia código del juego, recompilar también con clang.
- **Antes de copiar `bin\iso\default.xbe` a `consola_*.xbe`, comprobar que la compilación dijo `xbe ok`** (18-11: un
  error de compilación dejó el XBE viejo y se copió tres veces sin darme cuenta; las tres con el mismo md5).
- `probar_salas.ps1` NO repite la escena entre dos ejecuciones (el mando automático no es determinista): sirve para
  ver que nada se rompe, no para comparar antes/después pixel a pixel.

### Lo que se aprendió el 18-11 (rendimiento: dónde está el coste de verdad)
- **El juego cuesta < 3 % de la CPU; el coste es nuestra TRADUCCIÓN GX -> D3D por dibujo.** En GameCube la GPU lee
  las listas directamente; aquí cada dibujo pasa por la CPU (preparar plantillas, MandarPixel, MandarConstantes,
  LazySetStateVB, ShadowVertexShaderState, FastCopyToWC de índices). Hay ~1.200 dibujos por fotograma
  (`[MED] draw=t(N)`: N = dibujos) y Melee-X mide ~31 µs por dibujo en la Xbox: el objetivo es BAJAR N (Prime-X
  4.000 -> 400) y el coste por dibujo, no micro-optimizar el juego.
- **Orden de diagnóstico**: (1) memoria (`libres`, `aram=`, `kb=`, `[DLCPRIME-TEX] cache=` en el tope = recreación de
  texturas), (2) perfil por bloques en la Debug quieto 2-3 min, (3) contar en xemu (cuentas exactas).
- **Contadores nuevos**: `[ARAMSWP]` (ARAM al disco), `[ANILLOVB]` (anillo: dibujos, vueltas, esperas),
  `[DLCMEM] expulsiones= reintentos= aplazadas=`, `[DLCPRIME-TEX] creadas= pool= reusos=`, `[HGPU-POR]` (por qué una
  plantilla no va por huesos en GPU; `GPU=` las que sí). `[HMED2] plantillas=0` NO significa que no haya huesos en GPU.
- **Interruptores nuevos** (`compilar_plataforma.ps1`): `RE4_SIN_ARAM_SWAP`, `RE4_SIN_TEX_POOL`, `RE4_TEX_MAX=N`,
  `RE4_SIN_DLC_PRIME`, `RE4_SIN_ANILLO_VB`, `RE4_TEX_FIRMA_CADA_CUADRO`.
- **Referencias** en `RE4 port\referencias\`: `Prime-X` (Metroid Prime en Xbox: `docs\performance.md`), `Melee-X`
  (`docs\fps-plan.md`, `renderer.md`), `PuebloCTR` (RE4 en New 3DS, MISMA descompilación: 37 parches de juego en
  `overlay\port\whole\patches\03*.patch`, casi todos "mismos resultados"). Antes de inventar, mirar si ya lo resolvieron.
- **Subir con la consola en el dashboard** y, tras cada sesión larga del usuario, `python scripts\vs_juntar.py`.
  Tras hibernar el PC, el escuchador UDP muere: comprobar `escuchar_log` en los procesos y relanzarlo.
- **Esperas en `XPhysicalAlloc` sin texturas nuevas = VB recreados**: mirar `[DLCMEM] expulsiones=` y los KB de
  `plantilla stat= dyn= push=` (deben ser ~0 en una sala estable). `[VALPLT]` da el motivo del fallo de validación
  (1 muestra de la lista, 2 descriptor, 3-5 arrays de color/uv, 6 sin array, 7 otro paso de posición). 19-11: el
  motivo 7 (mismo modelo con paso 6 y 8) ya no expulsa (`771d346`, contador `paso=`).
- Compilar desde **Bash** (`RE4_NETLOG=1 RE4_SIN_TRACE=1 powershell -ExecutionPolicy Bypass -File scripts/compilar_plataforma.ps1`):
  un bloque de varias líneas en la herramienta PowerShell se ejecutó dos veces en paralelo y chocó con su propio candado.
- **Quién llama a una función caliente** (desde `e734b81`): `python scripts\perfil_llamadores.py <log> <bloques> <mapa>
  [fun1,fun2] [--top N]`. Fiable para funciones hoja (FastCopyToWC, memmove, memset); en funciones grandes sale basura
  (direcciones viejas en sus variables locales). Así se vio que FastCopyToWC venía de `RunPushBuffer` (pushbuffers con
  malloc que D3D copia en cada dibujo): un pushbuffer tiene que estar en memoria contigua para ejecutarse en su sitio.
- objdump no desensambla bien los `.obj` de MSVC (`$LN` cortan las funciones): usar `dumpbin /disasm`.
- **El usuario quiere ver los fps primero**: no meterse en otra cosa (bordes, etc.) mientras el lag no esté resuelto,
  aunque lo pida de paso; si una tarea secundaria se complica, aparcarla y avisar.
- **No hacer** (el usuario): bajar calidad (distancia de dibujo, assets de PS2, recortes). Ni que una sala vaya mal al
  entrar y bien después: lo que libere memoria tiene que hacerse durante la carga.

### Trampas vistas el 12-11
- **xemu no sirve para medir tiempos** (el mismo codigo da `fill` 119 o 364). Sirve para: que no pete, la imagen,
  contar cosas (`[TIRON]`, `libres`, programas). Los tiempos, solo en la consola.
- Las caches (texturas y listas) se vacian en `OSLink`/`OSUnlink` (`os_stub.c`): es la unica senal de cambio de
  sala. Si se quita, la memoria se acaba entre salas.
- El build Debug con `RE4_TRAZA_MUESTRA` dibuja 1 fotograma de cada 120 sin caches (parpadea la ropa de Leon).
- Los informes de agy dan por medido lo estimado y han llegado a citar lineas de log que no existen: comprobar
  cada cita con grep y ejecutar siempre hasta el pueblo antes de integrar.
- La memoria fija se mide con `i686-w64-mingw32-size <objeto>` (columna bss), no con huecos del `.map`.

## Probar otra sala en xemu

Cambiar `[ROOM]` y `[STAGE]` en `bin\iso\re4\files\Debug\config.txt` (p. ej. `0x04` = r104), compilar con
`RE4_AUTOPAD=1` y `RE4_AUTOPAD_SALTA=1`: entra solo en esa sala. **Devolver `[ROOM] 0x20` al acabar** (ese
fichero es el mismo que se sube a la consola). Sin tocarlo, el mando automatico llega al pueblo (r100).
Foto de xemu sin traerlo al frente: `PrintWindow(hwnd, hdc, 2)` (CopyFromScreen pilla la ventana de encima).

## Arrancar en xemu

```powershell
Start-Process "C:\RetroXeo\emulators\xemu\xemu.exe" `
  -WorkingDirectory "C:\RetroXeo\emulators\xemu" `
  -ArgumentList '-dvd_path "<ruta de la ISO>" -gdb tcp:127.0.0.1:1235'
```
ISOs: nativa `...\xbox-nativo\bin\RE4_Nativo.iso`; vieja `...\nwiirecomp-xbox\xbox\bin\RE4.iso`.
**El gdbstub en el 1235 es obligatorio**: sin el no se puede leer el log.

## Leer el log del juego (esto es lo que de verdad diagnostica)

El XBE escribe en un anillo de 64 KB en memoria (`g_xboxLogRing`). Se lee desde el PC:
```powershell
python "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\gdblog.py" `
       "<ruta del .map>" 65000 > log.txt
```
Mapas: nativo `...\xbox-nativo\bin\RE4Xbox.map`; viejo `...\nwiirecomp-xbox\xbox\bin\RE4Xbox.exe.map`.

Escuchador continuo (cada N segundos, solo lineas nuevas):
```powershell
python "...\xbox-nativo\scripts\escuchar_log.py" "...\bin\RE4Xbox.map" 15
```

## Que mirar en el log (rama nativa)

```bash
grep -c EXCEPCION log.txt            # 1 = sigue petando
grep "^\[SDK\]" log.txt | tail -3    # hasta que llamada llego
grep "\*$" log.txt | tail -20        # las funciones del SDK que alcanzo por primera vez
grep "\[DVD\]\|\[GX\]" log.txt       # ficheros leidos y estado del render
```
- `[NATIVO] arrancando, entro a mainCRTStartup()` = el main() del juego se ejecuta.
- `[SDK] <n> <funcion> *` = llamada numero n; el `*` es la primera vez de esa funcion.
- `[GX] render listo` = `GXInit` creo el dispositivo D3D8.
- `[NATIVO] EXCEPCION ...` = el juego se mato; mira las 3 lineas de antes para saber donde.

**El marcador del progreso es el numero de llamadas antes de la excepcion.**
Historico: 290 (sin memoria) -> 373 (memoria) -> 381 (DVD leyendo).

## Meter trazas o bloques de codigo: NUNCA por heredoc

Insertar codigo C con `python - <<E`, `cat <<E` o `sed` desde bash convierte el `\n` de las cadenas
en un salto de linea real (`missing terminating " character`) y se pierde una compilacion y una
prueba en xemu. Paso el 09-11-2026 cinco veces seguidas. El metodo es:

0. (13-11, otra vez) Los ficheros de `src` llevan CRLF: un guion que busca texto con `\n` no encuentra nada.
   Para cambios de varias zonas: guion Python escrito con **Write** que lea en binario, normalice `\r\n` -> `\n`,
   compruebe con `assert s.count(a) == 1` cada sustitución y vuelva a poner los CRLF al guardar.
0. Tampoco sustituciones con `python - <<E` que lleven `\n` o rutas con `\`: paso otra vez el 12-11-2026
   (tres veces). Para cambios en ficheros existentes, herramienta **Edit**.
1. Escribe el bloque con la herramienta **Write** en el scratchpad (texto literal, sin escapes).
2. Colocalo con `python ~/.claude/skills/re4-xbox/insertar.py <destino> <bloque> --despues "ancla"`
   (`--antes`, `--linea N`, `--ocurrencia K`). Conserva CRLF/LF y se niega si hay una cadena partida.
3. Cambios pequeños en una linea: herramienta **Edit**, no `sed`.
4. **Comprueba que compila antes de lanzar xemu**: `recompilar_juego.sh` imprime los `error:`; si
   sale alguno, el objeto viejo sigue ahi y la prueba no vale.

## Lineas rojas

- **Nunca vaciar ni borrar ficheros de codigo** para que algo compile. Antes de creerte un
  numero: `find <carpeta> -name "*.c" -size 0` y `git diff --stat | tail -1`.
- Los cambios en el codigo del juego (`re4\src`) van **envueltos en `#if defined(__MINGW32__)`**
  o `_MSC_VER`, sin romper la rama original (el build byte a byte de WSL da 115/115).
- No tocar `nwiirecomp-xbox` cuando se trabaja en la nativa, y al reves.
- Reparto de ficheros entre agentes: `RE4 port\buzon\REPARTO-agy-claude.md`.

## Cuando algo no cuadra

- `gdblog.py` dice "xemu no responde": xemu cerrado o sin `-gdb`.
- El log sale vacio: el `.map` no corresponde al XBE que esta corriendo (recompila o usa el mapa
  bueno).
- El enlace falla con `LNK1181 ... .obj,`: una coma colada en `bin\link_claude.rsp`; se regenera
  solo al volver a compilar.
- Avisos `LNK4006 second definition ignored`: son normales, vienen de `/FORCE:MULTIPLE` (los
  modulos REL del juego traen simbolos repetidos).
