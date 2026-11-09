# 36 — Luz real en Xbox (lightmap) para la v1.3 (08-10)

## El problema
El backend de Xbox (`wt-opti30\src\platform\RenderAPI_D3D8_XBOX.cpp`) solo usa la etapa de
textura 0. La unidad 1 (el lightmap de Minecraft: luz de cielo y de bloque por vértice, día/noche,
gamma del "Brightness") se ignoraba a propósito. Resultado: todo a plena luz, la noche no oscurece
el terreno, cuevas claras, la barra de brillo no hace nada. La luz suave sí funciona porque va en
el color del vértice.

## Datos que ya trae el juego
- Brillo por vértice: int32 = bloque (16 bits bajos) | cielo (16 altos), valores 0..240.
  Tessellator: offset 28 (stride 32). Terreno compacto PcLegacy: offset 20 (stride 24).
- GL usa la matriz de textura de la unidad 1: uv = (valor + 8) / 256.
- Entidades/mano/tile entities: `OpenGlHelper::setLightmapTextureCoords` = `renderSetMultiTextureCoord`
  (coordenada constante por objeto). Llega también dentro de display lists.
- `EntityRenderer::enableLightmap/disableLightmap` = activar/desactivar Texture2D en la unidad 1.
- El lightmap es una textura 16x16 que se actualiza cada tick (`updateTextureSubImage`).

## Plan
1. Backend: seguir Texture2D de la unidad 1, la textura atada ahí y la coordenada constante.
2. Etapa 1 = lightmap: MODULATE con CURRENT, filtro lineal, clamp.
3. Vértices:
   - `XboxVertex` (UP y listas no compactas) pasa de 24 a 32 bytes: + `lu, lv` float ya en 0..1.
   - `XboxCompactVertex` sigue en **16 bytes**: SHORT3 posición + PBYTE2 luz (el hueco del `w=1`)
     + color + UV. Sin RAM extra para el terreno.
4. Mallas sin brillo por vértice (entidades): coordenada constante por texgen
   (TCI_CAMERASPACEPOSITION + matriz D3DTS_TEXTURE1 con solo traslación).
5. `renderDrawCaptured` debe pasar `hasBrightness` (antes se perdía).
6. Opción **"Fullbright"** en Opciones > Vídeo (Xbox): deja la luz como antes (lightmap apagado).
7. Probar en xemu: día, noche, cueva, antorcha, brillo 0 y 100, Fullbright on/off, mobs de noche,
   agua, mano. Luego consola real (FPS y que el PBYTE2 sin alinear a 4 funcione en el NV2A).

## Riesgo a vigilar
- PBYTE2 en offset 6: xemu lo acepta; confirmar en consola. Si fallara, plan B: vértice compacto
  de 20 bytes (+25 % memoria de terreno).

## Hecho (08-10, commit 5c24962, sin push)
- Lightmap funcionando; el usuario: "lo veo bien la luz". Opción Fullbright en Vídeo.
- Anillo de 3 texturas para el lightmap (evita que la CPU espere a la GPU cada tick).
- PBYTE2 en offset 6: falta confirmarlo en la consola real (el usuario ya jugó con la luz y la vio
  bien, así que en principio va).
