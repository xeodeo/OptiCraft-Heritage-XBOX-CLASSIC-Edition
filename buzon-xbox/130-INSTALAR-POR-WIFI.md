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
