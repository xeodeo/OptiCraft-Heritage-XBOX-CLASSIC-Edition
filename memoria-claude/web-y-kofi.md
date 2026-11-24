---
name: web-y-kofi
description: "Web pública (GitHub Pages) y Ko-fi del proyecto; dónde está, cómo se actualiza, y la regla de la portada"
metadata:
  node_type: memory
  type: project
  originSessionId: 4bac41f4-d0be-425d-bc5c-04da57364737
  modified: 2026-11-24T12:43:01.974Z
---

Desde 24-11-2026 OptiCraft tiene **web pública** y **Ko-fi**.

- **Web en vivo:** https://xeodeo.github.io/OptiCraft-Heritage-XBOX-CLASSIC-Edition/ — rama `gh-pages` del repo `xbox`
  (Pages ya activado). Código fuente en `buzon-xbox\web\` (index/progreso/404, fuentes, img). Estilo menú Minecraft de
  consola, 3 idiomas ES/EN/PT (i18n JS). El panel real `ESTADO-OPTICRAFT.html` NO se publica (solo `progreso.html`, curada).
- **Redeploy:** copiar `buzon-xbox\web\` a una carpeta temp, `git add -A && commit && push <repo xbox> gh-pages`
  (rama huérfana, solo la web). Detalle y comandos exactos en `buzon-xbox\128-WEB-Y-KOFI.md`.
- **Ko-fi:** ko-fi.com/xeodeo (tema morado, banner propio OptiCraft, post con portada propia). El README de `xbox/main`
  enlaza Website + Support.

**Regla firme (no la cruzo):** NO publico la portada falsa "Minecraft Xbox Edition" (logos Xbox/Microsoft/Mojang) en la
web ni en Ko-fi — es suplantación de producto oficial en página de donaciones. Está en `buzon-xbox\portada-reddit.png`
para que la suba el usuario en su cuenta si quiere. Lo demás del proyecto (SDK/LCE/decompilado) es decisión suya.

**Capturas:** las reales van en `buzon-xbox\web\img\` (1-2 ya, fase temprana con hover). agy saca más (encargo 127);
al entregarlas, verificar que no sean negras/con menú y **redeploy**.

Ver [[compilar-todas-versiones]] y [[subir-siempre-al-compilar]] para los builds.

**24-11 tarde:** capturas en cuadrícula de 6 por página + visor al hacer clic (13 fotos elegidas por el usuario en
`img/shots/`); fondo **versión A** (zonas que cambian con fundido y se mueven lento: superficie→cueva→obsidiana→Nether→End;
`?fondo=b` = versión que baja con el scroll, al usuario le mareaba); barras con efecto portal del End (texturas del juego en
`img/depth/`) que se llenan al verse. Tarjetas casi opacas: con textura detrás el texto transparente "da vómito".
Sección `#wifi` (instalar desde el celular) está en index.html con `hidden` hasta que exista la release `ultima`.
