---
name: xeostore-ps3
description: "XeoStore = tienda nativa PS3 (estilo PKGi, hermana de Classic Xbox Store); carpeta, title id, compilación, decisiones"
metadata:
  node_type: memory
  type: project
  originSessionId: 4361e778-9941-4d46-b576-3da270fcabfd
  modified: 2026-10-31T16:44:38.715Z
---

Empezada el 31-10-2026 a pedido del usuario. Carpeta: `C:\Users\xeodeo\Downloads\Desktop\de kevin\Classic PS3 Store`
(plan y estado en `buzon\01-PLAN.md`). Title ID **XEOS00001**, PARAM.SFO con CATEGORY **CB** (sale en la columna Red del
XMB, como PKGi — lo pidió el usuario). Logo hecho por agy (`assets\icon`, `scripts\make_logo.py`).

- Reutiliza el código de Classic Xbox Store (copia de referencia en `src\xbox-original`): Http/mbedTLS, Route (proxies),
  Session (cuenta archive.org opcional), Keyboard, Lang, main.cpp. Capa PS3 nueva: Gfx (RSX), Network, Files, Drives.
- Juegos: colecciones archive.org `PS3-Redump-2024-Individual-Games-Part1..34` (las de emulatorlauncher ps3.json),
  sin cuenta. Catálogo = índice integrado `assets\data\ps3index.tsv` (scripts\make_ps3_index.py: claves redump + Title ID GameTDB).
- Instalar: ZIP → zlib → AES por sector → ISO9660/Joliet → carpeta JB en `/dev_hdd0/GAMES` (store\Ps3Iso.h, probado en PC con
  tools\prueba_extractor.sh). Cola de descargas + apagar al terminar (pedido del usuario). Abrir juegos con webMAN `/mount_ps3/`.
- Compilar: `MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash ".../Classic PS3 Store/scripts/compilar.sh" [IP PC netlog]` → bin\XEOS00001.

Ver [[ps3-sdk]] para el entorno (gcc13+PSL1GHT, scetool, FTP 192.168.0.21).

Estado completo al 17-11: buzon\05-ESTADO-TIENDA.md (qué funciona, qué falta, problema abierto Terraria convertido+update, mapa de código, trampas). La carpeta NO está en git.
