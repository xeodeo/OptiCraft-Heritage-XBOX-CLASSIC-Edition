---
name: ps3
description: Trabajar con la PS3 real del usuario (HEN + webMAN) - saber en qué estado está, encontrarla en la red, subir EBOOT/pkg/carpetas por FTP, escuchar el log del juego, compilar OptiCraft para PS3 y recuperar la consola tras formatear. Usala siempre que haya que tocar la PS3, subirle algo, comprobar si está encendida o si hay un juego abierto, o compilar/probar el port de PS3.
---

# PS3 del usuario

Responder SIEMPRE en español y corto, incluidas las descripciones de los comandos.
Proyecto: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\ps3`. El código está en `..\wt-ps3`, en la rama `ps3-base`.
El traspaso con el estado del port está en `ps3\buzon\00-ESTADO-PS3.md`; léelo primero.

## 1. Saber en qué estado está (sin tocarla)
```
python -I "C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/ps3/tools/estado_ps3.py"
```
El script solo abre y cierra conexiones. Qué puede salir:
- **No responde:** la consola está apagada, sin red, ha cambiado de IP, o HEN no está activado. webMAN solo arranca con HEN, que hay que activar a mano en cada encendido.
  - Para buscarla, recorre 192.168.0.x buscando el puerto 21 con el banner `webMANftpd`.
- **Banner `220 webMANftpd 1.47.xx`:** está encendida, con HEN activo y FTP disponible.
- **¿Hay un juego abierto?**
  - De OptiCraft se sabe por el netlog: un pulso reciente significa ABIERTO, y `RSX idle` como última línea significa cerrado limpio.
  - De cualquier otro juego o app NO se puede saber. Si hay duda, PREGUNTAR.
  - No uses páginas de webMAN para averiguarlo, porque algunas lanzan "Refreshing XML".

## 2. Reglas duras
- **Nada de FTP ni de órdenes a webMAN** (`install.ps3`, `mount`, `refresh`, `restart`) con un juego o app abiertos. La consola se ha colgado dos veces por eso.
- El escuchador del log se come memoria del PC; el sistema ya lo mató una vez. Lánzalo solo cuando el usuario vaya a probar o lo pida.
- No uses `-ffast-math`: la misma semilla tiene que dar el mismo mundo.
- La carpeta `minecraft-legacy original` (el LCE filtrado) es de SOLO LECTURA. Copia lo que necesites a wt-ps3.
- Los cambios en C/C++ se hacen con Edit/Write, nunca con python ni heredocs, porque rompen los `\n`.

## 3. Pasar archivos
- Un fichero (con barra de progreso y comprobación del tamaño):
  `MSYS_NO_PATHCONV=1 python -I ps3/tools/subir_ps3.py C:/ruta/local /dev_hdd0/ruta/remota`
  - El origen va como `C:/...`, no como `/c/...`.
  - Sin `MSYS_NO_PATHCONV`, el FTP devuelve "451".
- Una carpeta entera: `ps3/tools/subir_carpeta.py`.
- Comprueba el md5 bajando el fichero con ftplib RETR, usando `encoding='latin-1'`.
- Destinos:
  - OptiCraft va en `/dev_hdd0/game/OPTC00001/` (`PARAM.SFO`, `ICON0.PNG`, `SND0.AT3`, `USRDIR/EBOOT.BIN`, assets.pak 1.5.2).
  - Los `.pkg` van en `/dev_hdd0/packages`; el usuario los instala desde el XMB.
  - El pak de la 1.5.2 es el de la Xbox 1.5.2 (`ClassicXboxLoader\...\OptiCraft\1.5.2\assets.pak`).
- Un título nuevo o un SND0 nuevo solo aparece en el XMB después de reiniciar la consola. El usuario reactiva HEN después.

## 4. Log del juego (netlog)
- La PS3 manda log por UDP 9999 al PC (192.168.0.18).
- Escuchador: `python -I ps3/tools/escuchar_log.py`, en segundo plano, que escribe en `ps3/builds/netlog.txt`.
- Líneas útiles:
  - `[ps3.wd] fps= frameMs= waitMs=` (pulso cada 5 s);
  - `[ps3.prof]`, `[ps3.mesh]`, `[ps3.spumesh]`, `[ps3.spu]`, `[ps3.dirty]`.
- Antes de subir, espera a que la última línea sea `RSX idle`.
- Si sale `frameMs=33` clavado, es la cuantización del vsync (la GPU pasa de 16.7 ms), no la CPU.

## 5. Compilar OptiCraft PS3
```
wsl bash -lc "bash '/mnt/c/Users/xeodeo/Desktop/Minecraft en xboox clasico/ps3/tools/compilar_todo_k0.sh'"
```
- Si sale bien termina con `codigo ninja: 0  ficheros que fallan: 0`. El resultado queda en `wt-ps3/bin/ps3/OPTC00001/`.
- Toolchain: gcc 13 + PSL1GHT en WSL (`~/ps3dev`), con su propio pthread, y firma NPDRM con scetool (`tools/firmar_npdrm.sh`).
- SPU: las tareas se compilan con el `spu-lv2-gcc` del SDK de Sony filtrado (`ps3/sdk/cell`) mediante `wt-ps3/cmake/ps3_spu_build.sh`.
  - La PPU de PSL1GHT las carga con `sysSpuImageImport`.
  - La PPU de Sony NO se puede mezclar: su ABI usa punteros de 4 bytes.
- Una compilación a la vez.
- Al terminar, haz commit en `ps3-base` con el mensaje en español.

## 6. webMAN útil (solo con el juego cerrado)
- `http://IP/restart.ps3`: reinicia. Después hay que reactivar HEN a mano.
- `http://IP/dump.ps3?flash`: bajo HEN sale TODO A CEROS, no sirve. Para la flash hace falta PS3 Toolset.

## 7. Datos de la consola y formateo
- El respaldo está en `ps3\respaldo-consola\`, con copia en `Desktop\USB-PS3\`.
  - IDPS `00000001008800121000D169FCEF730C`.
  - PSID `B6602DDE59DC5B3EC05BF50CC951846E`.
  - También está `act.dat`.
- Formatear NO cambia el IDPS ni el PSID. Sí borra HEN, webMAN, los pkg, las licencias y las partidas.
- Después de formatear, el usuario hace a mano:
  1. Activar HEN desde el navegador (ps3xploit.me).
  2. Instalar webMAN desde el USB: `Desktop\USB-PS3\PS3\webMAN_MOD_*_Installer.pkg`.
  3. Reiniciar y activar HEN.

  Después Claude vuelve a subir por FTP OptiCraft, el SND0 y lo que pida.

## 8. Otros proyectos en la misma PS3
- LCE compilado: `lce-ps3-build\` → `LCEM00001`. Nunca se confirmó que arranque.
- XeoStore (tienda tipo PKGi): `Downloads\Desktop\de kevin\Classic PS3 Store`, `XEOS00001`.
- Mando Xbox: `ps3\mandos\salida`, sin probar.
