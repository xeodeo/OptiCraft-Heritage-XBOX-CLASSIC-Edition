---
name: ps3-estado-31oct
description: "PS3 al 31-10 — cómo detectar la consola, el escuchador de log, versión subida, qué falta para 60 fps, LCE/MCP, formateo y respaldo IDPS"
metadata:
  node_type: memory
  type: project
  originSessionId: ab1d4717-6399-4290-b485-70c441461bca
  modified: 2026-10-31T16:44:22.674Z
---

Estado al 2026-10-31. Lo anterior está en [[ps3-sdk]]. El traspaso completo está en `ps3\buzon\00-ESTADO-PS3.md`; léelo primero.

**Detectar la PS3**
- IP 192.168.0.21. Puede cambiar: buscar en 192.168.0.x el puerto 21 con el banner `webMANftpd`.
- FTP anónimo. Subir con `MSYS_NO_PATHCONV=1 python -I ps3/tools/subir_ps3.py <C:/local> /dev_hdd0/...`. Las carpetas enteras van con `subir_carpeta.py`.
- Comprobar el md5 con ftplib RETR, usando `encoding='latin-1'`.
- webMAN por HTTP: `http://IP/restart.ps3`, `/dump.ps3?flash`. Bajo HEN, el volcado de flash sale TODO A CEROS y no sirve.
- Detectar que el juego está abierto: la PS3 manda log UDP 9999 a la PC (192.168.0.18).
  - Escuchador: `python -I ps3/tools/escuchar_log.py`, que escribe en `ps3/builds/netlog.txt`. Lanzarlo en segundo plano.
  - Juego cerrado de forma limpia = la última línea es `RSX idle`. Solo entonces subir (ver [[ps3-subidas-juego-abierto]]).
  - El 30-10 el sistema lo mató por poca memoria en la PC. No volver a lanzarlo si el usuario no lo pide, o si no va a probar una versión.

**Compilar**
- `wsl bash -lc "bash '/mnt/c/Users/xeodeo/Desktop/Minecraft en xboox clasico/ps3/tools/compilar_todo_k0.sh'"`. Si sale bien termina con `codigo ninja: 0`.
- Sale en `wt-ps3/bin/ps3/OPTC00001/`.
- Las tareas SPU se compilan con el spu-lv2-gcc de Sony (`wt-ps3/cmake/ps3_spu_build.sh`) y la PPU de PSL1GHT las carga. Ver [[ps3-sdk-sony-filtrado]].

**Versión en la consola**
- Commit 4b170de, EBOOT md5 8fcc8e0de7117628633b197a317ce2b7. Está SIN PROBAR.
- Qué mirar al probarla:
  - fps;
  - colores de mobs y cielo, incluido el tinte rojo al recibir daño;
  - el desglose de `snapshot=` en `[ps3.mesh]`;
  - que no salga "vertex program has no colorConst".
- El mallado SPU solo funciona con Iluminación suave activada.

**Rendimiento y lo que falta para 60 estables a 4 chunks**
- Antes de esta versión: 41-46 fps caminando. El cuello es el hilo principal de la PPU.
- Lo que más gasta:
  - entidades + updateEntities (20-25%);
  - meshPublish (~1.9 ms por sección);
  - meshSnapshot (~1.3 ms por sección);
  - tirones del autoguardado;
  - la etapa "start" sin etiquetar (7-8%).
- Siguientes pasos:
  - marcadores dentro de "start";
  - autoguardado repartido;
  - triple búfer: si la GPU pasa de 16.7 ms, el doble búfer clava el juego a 30 fps;
  - Z-cull y vértices compactos;
  - mallado SPU fase 3 (hojas, líquidos, losas);
  - ruido SPU repartido entre varias SPU y asíncrono.
- El usuario quiere usar las 6 SPU "como God of War 3". Se puede copiar código de 4J/LCE tal cual ([[legacy-lce-permitido]]).

**LCE y MCP**
- No hay un "MCP" para LCE. MCP sirve para descompilar Java; LCE ya es código fuente C++ filtrado, así que no hace falta.
- La filtración de LCE es antigua. No existe una más reciente (tipo 1.13 de PS3) disponible.
- Compilación de LCE para PS3: `Minecraft en xboox clasico\lce-ps3-build\` (gen_list.py, build.py con `-Xfastmath=0`, link.py, make_pkg.py).
  - Se subió como LCEM00001. Nunca se confirmó que arranque.
- La carpeta `minecraft-legacy original` es SOLO LECTURA.

**Sin confirmar**
- La música del XMB (SND0 en ATRAC3plus a 44.1 kHz, `ps3/tools/hacer_snd0.sh`) no sonaba. Falta probar tras reiniciar.
- Creepers y brujas de día: el usuario no dijo dónde los vio.

**Formateo de la consola (31-10)**
- El usuario va a formatear la PS3.
- Respaldo en `ps3\respaldo-consola\`, con copia en `Desktop\USB-PS3\`:
  - IDPS 00000001008800121000D169FCEF730C;
  - PSID B6602DDE59DC5B3EC05BF50CC951846E;
  - act.dat.
- `Desktop\USB-PS3\PS3\webMAN_MOD_1.47.50_Installer.pkg` es para instalar desde el USB.
- Después de formatear, el usuario hace a mano: HEN desde el navegador (ps3xploit.me), instalar webMAN desde el USB y reactivar HEN. Luego Claude lo vuelve a subir todo por FTP: OPTC00001, SND0 y los temas.
- Los .pkg de Gamepad Test, PS3-Health y el pack de temas ya no están en Descargas.
- Falta un volcado de flash válido; necesita PS3 Toolset.
