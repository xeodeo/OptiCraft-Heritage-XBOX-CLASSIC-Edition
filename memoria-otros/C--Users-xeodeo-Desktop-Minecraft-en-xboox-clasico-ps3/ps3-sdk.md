---
name: ps3-sdk
description: "PS3 port prep — official SDK 3.70 extracted (not installed) in \"Minecraft en xboox clasico\\ps3\\sdk\\cell\"; short path needed; the user will do the port in another chat"
metadata:
  node_type: memory
  type: project
  originSessionId: f120f6d6-e2de-46cc-9ea4-af7ecf956bac
  modified: 2026-10-29T14:24:14.320Z
---

On 2026-10-29 I prepared `Minecraft en xboox clasico\ps3\`. The plan and the steps are in its `LEEME.md`, and the environment is in `entorno-ps3.sh`/`.bat`.

- The SDK 3.70 (gcc 4.1.1 PPU, SNC, libgcm, sce-cgc) was extracted with 7z from the offline installer, the same way as the XDK [[xdk-sin-instalar]].
- A hello world and the gcm `basic` sample both build to `EBOOT.BIN`.
- gcc 4.1.1 breaks on paths with spaces ("illegal usage of if-file-exist"). Use the short path `MINECR~1`.

The user said the port itself will happen "en otro chat". They chose the leaked Sony SDK over PSL1GHT, and I advised against PhyreEngine.
The 1.5.2 pending list is in `buzon-xbox\64-PENDIENTE-152.md`; see [[port152-estado]].

29-10: el port de PS3 arranca en el chat de la carpeta ps3 (memorias de OptiCraft copiadas aquí). El usuario tiene una PS3 real; RPCS3 es opcional.
PS3 del usuario: HEN + webMAN MOD, FTP anónimo en 192.168.0.21 (puede cambiar: buscar puerto 21 con banner webMANftpd). Juego en /dev_hdd0/game/OPTC00001, .pkg en /dev_hdd0/packages. Subida: python ftplib (usar encoding latin-1 para listar).

29-10 HITO: triángulo libgcm corriendo en la PS3 real (HEN). Lo que funcionó:
- La firma del SDK (make_fself / make_fself_npdrm) da error 80010009 en HEN. Hay que firmar NPDRM free con scetool: está en WSL Ubuntu (~/ps3tools/scetool, claves de jevinskie/ps3dotdir en data/), script `ps3\tools\firmar_npdrm.sh` (llamarlo con MSYS_NO_PATHCONV=1 desde Git Bash).
- Formato carpeta: /dev_hdd0/game/OPTC00001/{PARAM.SFO (tools\hacer_sfo.py), ICON0.PNG, USRDIR/EBOOT.BIN}. Para actualizar, basta con subir el EBOOT.BIN por FTP.
- Reiniciar: http://192.168.0.21/restart.ps3. Después el usuario reactiva HEN a mano.
- 29-10: a petición del usuario se borraron todos los .pkg de /dev_hdd0/packages y The Last of Us (NPEA00435, BCES01585DATA2).
- El SMART del disco no se puede leer desde la PS3 (el hipervisor no deja pasar esa orden). Hay que sacar el disco y usar CrystalDiskInfo.

29-10 DECISIÓN: el juego se compila con gcc 13.2 + PSL1GHT (ps3toolchain en WSL, ~/ps3dev). Los shaders van con sce-cgc de Sony y sus docs quedan de consulta. Motivo: el gcc/SNC de Sony no entienden C++11, y su ABI usa punteros de 4 bytes (no se puede enlazar con compiladores modernos). El usuario lo aceptó ("monstruo entre los dos"). Plan y estado: ps3\buzon\01-PLAN-port-ps3.md. Worktree wt-ps3, rama ps3-base (sale de port152-claude).
- Subir a la PS3: `MSYS_NO_PATHCONV=1 python -I ps3	ools\subir_ps3.py <local> /dev_hdd0/...` (con barra). Sin MSYS_NO_PATHCONV, Git Bash convierte /dev_hdd0 en una ruta de Windows y el FTP responde "451 Action aborted".
- Toolchain en WSL: `source ~/ps3tools/env.sh` (ppu-g++ 13.2, PSL1GHT en ~/ps3dev, cgc de NVIDIA instalado). Ejemplo rsxtest: tools\probar_rsxtest.sh.
- PSL1GHT: firmar SIEMPRE el ELF de build/<nombre>.elf (ya pasado por strip + sprxlinker), no el .elf de la raíz. Si se firma sin sprxlinker, la pantalla se queda negra y el programa se cierra.
- 29-10 HITO 2: triángulo con shaders (gcc13+PSL1GHT, cgcomp) funcionando en la PS3 real: ps3\pruebasx-tri. El ejemplo rsxtest de PSL1GHT cuelga la consola real (en RPCS3 va bien): no usarlo de referencia.
- WSL desde Git Bash: `wsl -- bash -c "..."` en línea expande las $variables ANTES de llegar a bash (salen vacías, "division by 0"). Hay que poner todo en un script .sh y llamarlo con `MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash /mnt/c/...script.sh`. Barra de ninja: `ps3	oolsarra_ninja.sh <log>`.
- 29-10 HILOS en PS3: gcc 13 recompilado con --enable-threads=posix sobre las CABECERAS de pthread-embedded, pero las funciones las da `src/ps3/compat/Ps3Pthread.c` con syscalls lv2 directas. La librería PTE y los helpers sprx de PSL1GHT (sysLwMutex*, sysThreadGetId) cuelgan el arranque de C++, porque libgcc bloquea un mutex en crtbegin antes de que se enlacen los sprx. newlib = la del gcc sin hilos. Detalle en ps3\buzon\01-PLAN-port-ps3.md (paso 3b).
- Depurar en RPCS3: el log solo enseña syscalls que FALLAN; las estadísticas "⁂ nombre [n]" al final dicen cuáles se llamaron. Escribir pasos en /dev_hdd0/tmp/*.txt (en RPCS3 es tools\rpcs3\dev_hdd0\tmp). El servidor GDB de RPCS3 no sirvió (corta la conexión).
- Al generar C/C++ desde Python dentro de un heredoc de bash, los "\n" de las cadenas acaban como saltos de línea reales y rompen el código (pasó 4 veces el 29-10). Para cambios de código usar Edit/Write, no python con reemplazos.

**29-10 tarde:** arreglado el cuelgue al romper/recoger bloques (EntityItem::searchForOtherItemsNearby hacía delete de una caja del pool AABB, commit 1193a98). La PS3 manda pulso cada 5 s por UDP 9999 (ps3.wd: stage, frame, get/put); escuchar con ps3/tools/escuchar_log.py -> ps3/builds/netlog.txt. Medido: ~30-37 FPS en el mundo, CPU-bound (RSX esperando). Commit 92da57f activa generación de chunks en el 2º hilo PPU: compilado, NO probado ni subido a la consola. En la PS3 queda el EBOOT 1193a98 + pak 1.5.2.

**29-10 noche (ÚLTIMO):** estado completo y siguientes pasos en `ps3\buzon\00-ESTADO-PS3.md` (leer primero). Rama ps3-base en af61e05, árbol limpio. Funciona en consola: sonido, iconos PS, Legacy Look, cierre XMB limpio, mallado de chunks en 2º hilo PPU. 4 chunks: 50-58 quieto, 34-48 caminando. Siguiente: leer ps3.dirty (churn ~250 secciones/5 s), convertir vértices en el worker, desglosar meshSnapshot, luego SPU con PSL1GHT (spu-gcc 9.5 en ~/ps3dev/spu) (el usuario permite el SDK de Sony filtrado si da fps, ver ps3-sdk-sony-filtrado.md). Primera tarea SPU: ruido del terreno, bit a bit igual que la PPU.

**31-10:** lo más reciente está en [[ps3-estado-31oct]].
