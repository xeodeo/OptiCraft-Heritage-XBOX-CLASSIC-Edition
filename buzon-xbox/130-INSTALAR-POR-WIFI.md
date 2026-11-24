# 130 — Instalar / actualizar por WiFi desde el celular (24-11-2026)

Pedido del usuario: "instalación desde celular", que la página de GitHub pueda pasar la última build a la consola,
como hace Yakara (yakaracolombia.github.io/esp32-online-tool/blueretro.html: flashea un ESP32 por Web Serial/USB).
"Nosotros WiFi", "pasar la última build", "nada de zip roto".

## Por qué no se empuja desde la web
- Una página https (GitHub Pages) no puede abrir FTP ni sockets, y fetch a http://192.168.x.x lo bloquea el navegador
  (contenido mixto / Private Network Access). Navegar (abrir el enlace) sí se puede.
- Web Serial/USB (lo de Yakara) no sirve: la Xbox no se conecta por USB al celular.

## Diseño: la consola tira, el celular manda
1. **Release "ultima"** en GitHub (rolling): un fichero por archivo, SIN zip, + `manifest.txt`:
   `<ruta en la Xbox> <bytes> <sha256> <nombre del asset>` (los assets no admiten '/', van como `1.5.2__OptiCraft.xbe`).
2. **Lanzador (default.xbe)**:
   - Al arrancar levanta la red y muestra abajo `WiFi: http://<IP>`.
   - Servidor HTTP mínimo en el puerto 80 (en el bucle, sin hilos que molesten): `/` página con botones,
     `/instalar?v=1.5.2` arranca la instalación, `/estado` progreso.
   - Instalación en un hilo: baja `manifest.txt` por HTTPS (XboxTls, sigue redirecciones de GitHub), luego cada
     archivo a `<ruta>.part`, comprueba tamaño + SHA-256, y solo cuando TODOS están bien renombra (el viejo a `.bak`).
     Si algo falla no toca la instalación que funciona.
   - Barra de progreso en la tele. También botón en el propio lanzador (sin celular).
3. **Web**: sección "Instalar desde el celular": pasos + campo IP (se guarda) + botones por versión que abren
   `http://<IP>/instalar?v=...`. Mismo WiFi.

## Estado (24-11, tarde)
- [x] worktree `wt-wifi` rama `wifi-instalar` desde `port152-int`, commit `388c55cc`
- [x] lanzador: red + IP en pantalla + servidor HTTP (puerto 80): `/`, `/instalar?v=`, `/estado` (JSON)
- [x] lanzador: descarga HTTP/HTTPS con redirecciones, manifest, SHA-256, `.part` → renombrado; salta lo idéntico
- [x] pad: Y = actualizar la versión elegida (o "todo" si no hay ninguna)
- [x] `scripts-claude\montar_ultima.py <carpeta OptiCraft> <salida> --build "..."` (manifest + assets planos)
- [x] `scripts-claude\compilar-lanzador-wifi.ps1` (preset xbox-launcher en wt-wifi, coge el candado)
- [x] PROBADO en xemu: ISO con lanzador + `D:\wifi-base.txt` = `http://10.0.2.2:8000/` (servidor python en el PC),
      xemu.toml `[net.nat] forward_ports = [{host=8080, guest=80}]` (se restauró después; copia en xemu.toml.bak-wifi).
      Desde el navegador del PC http://127.0.0.1:8080/ → Instalar 1.5.2 → bajó 2 archivos (6 MB), SHA OK,
      "Prueba OK" (desde ISO escribe en T:\wifi-prueba porque D: es de solo lectura). xemu NAT va lento (~100 KB/s).
- [x] web local: sección `#wifi` "Instalar desde el celular" (IP guardada + botones). **NO publicada** hasta que haya
      release `ultima` y un lanzador con esto en la consola.
- [ ] Falta: decidir qué builds van (públicas sin log), montar y SUBIR la release `ultima` (permiso del usuario; no hay gh,
      hace falta token o subirla a mano), meter el lanzador nuevo en la carpeta OptiCraft y en la consola,
      probar en la consola real (HDD: D: escribible), y entonces publicar la sección de la web.
- FTP directo desde una web: imposible (los navegadores no hablan FTP ni sockets; Chrome quitó FTP en 2021).
  Si se quiere "subir por FTP desde el celular" hace falta una app (APK) o el PC.

## 24-11 noche: sin celular, como la Classic Xbox Store (commit `4a45a074`)
- El usuario: "que no necesite el celular, que lo descargue él solo como la Classic Xbox Store".
- Botón **Update** al lado de Play (izq/der para llegar; A pulsa; Y sigue de atajo). Al subir la red, el lanzador
  baja solo la lista y marca **"Update!" / "New build!"** si hay algo nuevo para la versión elegida, si no "Up to date".
- `D:\instalado.txt` = lo instalado (ruta, tamaño, SHA-256): la comprobación no hashea todo cada vez
  (solo la primera vez, para lo que no tiene registro).
- Probado en xemu: release local con un archivo nuevo → "New build!" sin tocar nada → Update → bajó SOLO ese
  archivo → "Up to date". Textos de la tele en inglés (como el resto del lanzador).
- La página del celular (puerto 80) sigue existiendo como extra.

## 24-11 mediodía: consola vacía → baja la versión que elijas (commit en `wifi-instalar`, subido)
- Sin nada instalado, la lista muestra las versiones de la release como `1.2.5 (get)` / `1.5.2 (get)` y Play pasa a
  **Download**. PROBADO en xemu con el usuario: lanzador solo (sin carpetas) → eligió 1.5.2 → bajó XBE + version.txt,
  comprobados → pasó a "1.5.2" instalada, "Up to date", y 1.2.5 sigue como "(get)".
- Página del celular APAGADA (`XBOX_LAUNCHER_WEB=0`) y sin IP en pantalla: el usuario no la quiere.
- Pruebas desde ISO: `D:\wifi-base.txt` (URL de la release local) y `D:\wifi-limpiar.txt` (vacía T:\wifi-prueba al arrancar).

## Qué baja y qué falta comprobar (aclarado con el usuario, 24-11)
- Baja TODO lo que tenga la carpeta de esa versión en la release: OptiCraft.xbe, _720p, _exp, _exp_720p, assets.pak,
  texturepacks\, skins\, version.txt, default.tbn. Las 4 variantes aparecen luego en la lista (normal/720p/Exp/Exp 720p).
- Update = solo los archivos que cambiaron (normalmente los XBE; assets.pak y packs casi nunca).
- Primera instalación de 1.5.2 completa ≈ 100 MB. En xemu la red NAT va a ~100 KB/s (≈15 min); en la consola real más rápido.
- La prueba de xemu SOLO llevaba OptiCraft.xbe + version.txt (rápida): esa 1.5.2 de prueba no tiene assets.pak y no
  arrancaría. Pendiente: probar una carpeta completa (xemu o consola) y la descarga desde GitHub real.

## Pendientes para usarlo de verdad
1. OK del usuario para crear/subir la release `ultima` (no hay `gh` ni token aquí: token o subida a mano). Montarla con
   `python scripts-claude\montar_ultima.py <carpeta OptiCraft con builds PÚBLICAS> <salida> --build "..."`.
2. Meter este lanzador (`wt-wifi\bin\xbox-launcher\iso\default.xbe`) en la carpeta OptiCraft y pasarlo a la consola una vez.
3. Probar en la consola real: instalar desde vacío, Update, que D: (disco duro) se escriba bien.
4. Unir la rama `wifi-instalar` a `port152-int` (y el lanzador de 1.2.5 si se compila aparte).
