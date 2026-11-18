---
name: classic-xbox-store
description: "Port nativo a la Xbox clásica (XDK 5849) del loader como tienda: carpeta, plan y hallazgos de archive.org"
metadata:
  node_type: memory
  type: project
  originSessionId: 4f67a06f-d393-4b26-9867-5d49a221aa3d
  modified: 2026-10-22T15:50:56.816Z
---

Proyecto aparte (pedido por el usuario el 21-10-2026): `C:\Users\xeodeo\Downloads\Desktop\de kevin\Classic Xbox Store`, fuera del repo del loader. XBE nativo con el mismo sistema de compilación que OptiCraft (VS2022 cl + Link/imagebld del XDK 5849, `src/runtime` copiado de `wt-opti30`). Compilar: `XBOX_XDK_ROOT=...\xdk\5849\sdk\XDK\xbox`, `cmake --preset xbox` y `cmake --build --preset xbox` → `bin\iso\default.xbe` + `bin\ClassicXboxStore.iso`. Para usar xemu, respetar el `buzon-xbox\BUILD.lock` de OptiCraft.

Plan elegido: la Xbox descarga sola, sin PC puente. Enlace directo a la ISO dentro del ZIP: `https://archive.org/download/<item>/<juego>.zip/<juego>.iso`. Solo funciona con la sesión iniciada (sin sesión: "Item not available"). La ISO es redump de unos 7,8 GB; la partición del juego (XDVDFS, firma `MICROSOFT*XBOX*MEDIA` en 0x18310000) empieza en 0x18300000. No admite peticiones por rangos (siempre 200 desde el byte 0) y va a unos 2 MB/s. FATX tiene un límite de 4 GB por archivo, así que se guarda en trozos menores de 4 GB, se convierte a carpeta y se borran los trozos. El usuario tiene un F: de 120 GB.

Estado al 21-10-2026 noche: hay HTTPS (mbedTLS 3.6.2, raíces en assets/certs), catálogo por letras con arte de MobCat (assets/data/artindex.tsv con scripts/make_art_index.py) e instalador. El instalador baja el **ZIP** con Range y lo descomprime en la Xbox con zlib, no la ISO de dentro: view_archive.php no admite Range y se corta. Si se corta, reanuda desde el byte exacto. Corta la descarga cuando ya tiene el árbol XDVDFS completo. El registro está en E:\TDATA\FFFF4358\cxs_log.txt. XNet recibe por defecto con una ventana de 16 KB (se sube con XNetStartupParams). xemu no pasa de unos 0,5 MB/s de red: la velocidad real solo se mide en la consola. Prueba local: preset `xbox-autotest` + servidor con Range en el PC (xemu lo ve en 10.0.2.2:8000).

Prueba 1 (hecha, va en xemu): espacio de E/F/G montadas con IoCreateSymbolicLink (Partition1/6/7), busca juegos en las carpetas de primer nivel y comprueba DNS + TCP 443 a archive.org. Siguiente: HTTPS con mbedTLS e inicio de sesión.

Repositorio: https://github.com/xeodeo/Classic-Xbox-Store (rama main). Los commits van con `git -c user.name=xeodeo -c user.email=pintolopez89@gmail.com`, **sin ninguna mención a Claude Code** (ni Co-Authored-By), por petición expresa del usuario. La consola está en 192.168.0.3 (FTP xbox:xbox) y la tienda en `F:\Apps\Classic Xbox Store\`; se puede subir el XBE por FTP y comprobarlo con MD5. El FTP solo responde con la consola en el panel, no con la tienda abierta.

**Why:** el usuario quiere la app del loader como tienda dentro de la consola.
**How to apply:** las reglas del usuario vienen de OptiCraft (en español, corto, sin jerga; no subir el XDK ni binarios a git). Ver [[opticraft-xbox-port]].
