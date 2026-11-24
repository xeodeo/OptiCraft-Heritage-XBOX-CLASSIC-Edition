# Web pública + Ko-fi de OptiCraft (24-11-2026)

Todo lo montado hoy para la página pública del proyecto y las donaciones. Déjalo al día si cambia.

## Web pública (GitHub Pages)
- **URL en vivo:** https://xeodeo.github.io/OptiCraft-Heritage-XBOX-CLASSIC-Edition/
- **Fuente:** rama `gh-pages` del repo `xbox` (github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition). Pages ya activado (Deploy from a branch → gh-pages → / root).
- **Código fuente local:** `buzon-xbox\web\` (es lo que se sube). Páginas:
  - `index.html` — menú estilo Minecraft de consola: paisaje de bloques paneando, logo, splash, botones GUI, 3 idiomas (ES/EN/PT con selector, i18n por JS), botón Donar (Ko-fi), secciones progreso/capturas/qué trae/novedades/descargas/apoya.
  - `progreso.html` — página de progreso PÚBLICA y curada (barras por áreas). **NO** es el panel real.
  - `404.html` — con cara de creeper.
  - `fonts/` — `minecraft.woff2/woff` (South-Paw, sin acentos) + `monocraft.ttf` (fallback para acentos U+00A1–024F; por eso las tildes se ven bien).
  - `img/` — `1.png`,`2.png` capturas reales (fases tempranas, con hover "Fase temprana"); `3–6` pendientes de agy; `landscape.svg` (fondo), `tiles/` (dirt/grass/stone/planks/cobble), `creeper.svg`, `cover.png` (portada propia OptiCraft Heritage).
- **El panel real (`ESTADO-OPTICRAFT.html`) NO está en web/** a propósito. Sigue solo en `buzon-xbox\ESTADO-OPTICRAFT.html`.
- Botones de descarga → **Releases** del repo. "Cómo instalar" → README del repo.

### Cómo actualizar la web (redeploy)
Editar en `buzon-xbox\web\`, luego:
```
T="<carpeta temp>/ghpages"   # cualquier carpeta temporal
cp -r buzon-xbox/web/. "$T"/
cd "$T" && git add -A && git commit -m "web: <cambio>" && git push "https://github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition.git" gh-pages
```
(La rama gh-pages es huérfana, solo la web. No mezclar con el código.)

## Ko-fi (donaciones)
- Cuenta: **ko-fi.com/xeodeo** (id de página `O8M52AR1WG`). Ese link ES la página de donar (el botón grande abre el pago). No hay URL "solo pago" aparte en el plan gratis.
- Hecho por mí: tema **morado**, **banner propio** "OptiCraft Heritage" (generado, nuestro), un **post con imagen** usando la portada propia OptiCraft (cover256). Tip por defecto $5.
- PayPal: lo conectó el usuario.
- **Portada "Minecraft Xbox Edition" (de Reddit/SuperNitro58):** YO NO la subo (usa logos oficiales Xbox/Microsoft/Mojang → suplantación de producto oficial en página de donaciones). El usuario puede subirla él: está en `buzon-xbox\portada-reddit.png` (sacada de `xbox/main:scripts/xbox/media/xbox-cover-front.png`). Es decisión suya hacerlo en su cuenta.

## Capturas (agy, encargo 127) — EN MARCHA
- Encargo: `buzon-xbox\127-ENCARGO-agy-capturas-web.md`. agy abre xemu con `wt-build-152\bin\xbox-exp\OptiCraft.iso`, mundo tutorial, y deja `3.png`–`6.png` en `buzon-xbox\web\img\`. Informe: `127-INFORME-agy-capturas.md`. Terminal Orca: `term_d703c564...`.
- **PENDIENTE al terminar agy:** verificar que los PNG no sean negros/con menú, y **redeploy** (ver arriba). Luego en `index.html` los 4 `<div class="shot">Próximamente</div>` se cambian por `<figure class="shot"><img src="img/N.png"><figcaption data-i18n="cap.early">Fase temprana</figcaption></figure>` si son de fase temprana, o sin caption si quedan bien.

## Idiomas
- i18n en `index.html` (dict `I18N` es/en/pt + `data-i18n`). `progreso.html` de momento solo ES; si se quiere multiidioma, replicar el mismo patrón.
