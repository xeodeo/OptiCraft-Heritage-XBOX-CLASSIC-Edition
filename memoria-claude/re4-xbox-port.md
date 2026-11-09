---
name: re4-xbox-port
description: "RE4 (GameCube, NWiiRecomp) port to original Xbox lives in RE4 port/nwiirecomp-xbox (branch xbox-port); PC folder untouched"
metadata:
  node_type: memory
  type: project
  originSessionId: 8783a2c8-65f6-448b-80da-94c7308cae1d
  modified: 2026-10-05T22:54:23.213Z
---

Since 2026-10-05 the user wants RE4 (recompiled with NWiiRecomp, runs on PC via Desktop\RE4.bat) on the original Xbox (64 MB, softmodded), using the SAME toolchain as Minecraft (VS2022 cl x86 + XDK 5849, D3D8), not nxdk.

**Why:** the PC version must stay untouched ("crea otra rama para no dañar lo que llevo en pc").

**How to apply:** work only in `C:\Users\xeodeo\Desktop\RE4 port\nwiirecomp-xbox` (git worktree, branch `xbox-port`); start with `RE4 port\buzon\XBOX-ESTADO.md` (how to build, tools, measured status, pending list); keep it updated. The user tests in xemu himself: build the ISO and open xemu with the gdbstub, don't drive the pad. Agy quota ran out on 2026-10-05; the renderer is now Claude's. Live runtime is `export/nWiiRuntime` (not `nWiiRuntime/`). Test ISO goes to `C:\RetroXeo\roms\xbox\RE4 Recomp (prueba).iso`; user opens it with RetroXeo/xemu. Reference port: Melee-X cloned in `RE4 port\referencias\Melee-X`. Fog is the first thing to cut if slow. Use the BUILD.lock rule [[una-compilacion-a-la-vez]].

**Desde el 06-10-2026 hay DOS caminos.** El viejo (recompilacion del binario PowerPC + GX
emulado) sigue en `nwiirecomp-xbox`, rama `xbox-port`: arranca, llega al menu, el mando va; el
arreglo de memoria de las copias del EFB esta en el commit 3882091. El nuevo es compilar la
**decompilacion completa de RE4** (github.com/adonis-singh/re4, clonada en `RE4 port
e4`,
rama `agy-port-msvc`; el clon bueno para el build byte a byte esta en WSL `~/decomp/re4`).
**Empezar siempre por `RE4 portuzon\PLAN-rama-decomp.md`**, que es el indice con todo lo
medido. Resumido: 115/115 ficheros byte a byte con el cc1plus de SN construido del zip del
usuario; el juego compila para x86; el endianness se resuelve con `scalar_storage_order` de GCC
(sin la herramienta de reescritura que necesito Melee-X); y el `Link.Exe` del XDK
(ojo: esta en `binc71\`) enlaza objetos de MinGW GCC con objetos del `cl`. Lo unico que
queda es escribir la capa de SDK: 292 funciones, de las que 95 de GX ya estan en Melee-X.

**08-10-2026 (madrugada).** La rama nativa arranca de verdad: memoria propia, lee ficheros del
disco, audio inicializado, y 18 funciones GX conectadas a un render D3D8 que escribi
(`xbox-nativo/src/gx_render.cpp`, `gx_textura.cpp`, `gx_cache.cpp`). Muere cargando las fuentes
(`mes.cpp:338`) por orden de bytes. **Empezar por `RE4 port/buzon/TRASPASO-08-10.md`**: ahi esta
el plan y el aviso de que el endianness se esta parcheando a mano en vez de con
`scalar_storage_order`, que es la decision pendiente. Hay una skill `re4-xbox` con los comandos. **08-10 noche: empezar por `RE4 port/buzon/TRASPASO-08-10-noche.md`** (GX v2 con TEV por combiners de Melee-X, OSLink nativo, yz2 en C, autopad).
Truco que costo cuatro pruebas: xemu deja la ISO abierta y extract-xiso no la reescribe, asi que
se prueba la ISO vieja creyendo que es nueva (ya arreglado en `compilar_plataforma.ps1`).

**08-10-2026 (mañana): el menú de la tarjeta sale en pantalla** ("No Memory Card found in Slot A",
foto en `RE4 port/buzon/foto-tarjeta-08-10.png`). **Empezar por `RE4 port/buzon/TRASPASO-08-10-tarde.md`**
y `AUDITORIA-08-10.md`. Claves: `scalar_storage_order` es solo de C → se usa `BE<T>`/`DiscPtr<T>`
(`re4/include/be_disc.h`, ~140 structs pendientes); hilos de GC sobre hilos de Xbox con relevo (las
fibras tumban la Xbox con una excepción); FIFO de GX decodificada en `gx_fifo.c`; ciclo de prueba
con `xbox-nativo/scripts/probar_xemu.ps1` y `recompilar_juego.sh`. Nada commiteado aún.
