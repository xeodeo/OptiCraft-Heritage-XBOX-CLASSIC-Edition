# ENCARGO 10 (agy): técnicas de la comunidad de la Xbox original (internet)

Solo INVESTIGAR en internet. **No modifiques ningún archivo** salvo tu informe.

## Contexto (medido)
- Juego: Minecraft 1.2.5 en C++ portado a la **Xbox original** con el XDK oficial 5849 (Direct3D 8). Pentium III 733 MHz, 64 MB de RAM, GPU NV2A.
- Emula display lists de GL sobre D3D8 con vértices compactos y quad lists, y usa doble buffer con vsync.
- Por fotograma quedan ~10 ms libres esperando el vsync.
- Tirones de CPU al cargar o generar chunks (NBT + zlib en el hilo principal).

## Qué buscar (con URL de fuente para cada punto)
1. **Rendimiento:** push buffers frente a DrawVertices, register combiners del NV2A, trucos de vertex shaders, texturas swizzled y en formato DXT/P8, coste de los cambios de estado en D3D8 de Xbox, memoria write-combined.
2. **128 MB en consolas modificadas:** ¿puede una app del XDK usar la RAM extra? ¿Cómo lo hacen XBMC, Kodi o homebrew? ¿Qué llamadas usan?
3. **Disco y descompresión:** caché del disco duro (particiones X/Y/Z), HDD frente a DVD, velocidad de zlib, LZ4 y LZX en un Pentium III.
4. **Un solo núcleo:** hilo de baja prioridad durante el vsync y cómo evitar la fragmentación de memoria.
5. **Postmortems** de ports a Xbox (Quake, Doom, Half-Life 2 Xbox, Halo GDC): trucos aplicables.

Fuentes sugeridas: xboxdevwiki.net, nxdk y pbkit (GitHub), documentación de xemu, xbox-scene, AssemblerGames, r/originalxbox, charlas de la GDC.

## Entrega
- Informe en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\10-INFORME-agy.md`. Primera línea literal: `# INFORME 10 agy`.
- Máximo ~1000 palabras: **TOP 10** técnicas, cada una con qué es, URL, beneficio esperado y dificultad.
- No escribas en el Desktop ni en otra carpeta. Si no encuentras fuente, dilo; no inventes.
