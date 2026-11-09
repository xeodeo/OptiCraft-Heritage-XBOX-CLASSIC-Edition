# INFORME 10 agy

Basado en investigaciones sobre la arquitectura de la Xbox original (Pentium III 733MHz, NV2A, 64MB), aquí están las top 10 técnicas aplicables para optimizar el port C++ de Minecraft:

## 1. NV2A Push Buffers (frente a llamadas D3D8 estándar)
*   **Qué es:** Escribir instrucciones de renderizado y vértices directamente en la memoria en el formato nativo del NV2A (comandos push buffer), saltándose la validación del driver D3D8.
*   **URL:** https://www.khronos.org/ / https://mattkc.com/
*   **Beneficio:** Ahorro drástico de ciclos de CPU en el hilo principal de renderizado.
*   **Dificultad:** Alta. Requiere manipular los `DWORD` del hardware de la Xbox (similar a pbkit).

## 2. Particiones de caché del HDD (Unidades X, Y, Z)
*   **Qué es:** Utilizar el disco duro (particiones caché) para el streaming de chunks, aprovechando el "tandem reading" (leer del HDD mientras el DVD hace otra cosa).
*   **URL:** https://www.racketboy.com/ / https://xboxdevwiki.net/
*   **Beneficio:** Evita tirones al cargar regiones y reduce el desgaste de la lectora.
*   **Dificultad:** Media. Hay que redirigir el guardado de chunks temporales a `Z:\`.

## 3. Memoria Write-Combined para Vértices
*   **Qué es:** Alojar los buffers de vértices (como los chunks construidos) en memoria configurada como Write-Combined (WC). Esto evita ensuciar la caché L1/L2 del Pentium III al enviar datos a la GPU.
*   **URL:** https://www.gamedev.net/ / xboxdevwiki.net
*   **Beneficio:** Más ancho de banda de memoria libre y la caché L2 queda para la lógica del juego.
*   **Dificultad:** Baja (solo cambiar el tipo de alloc).

## 4. Hilo de fondo sincronizado con el VSync
*   **Qué es:** Sabiendo que el motor se bloquea ~10ms esperando el VSync, mover el empaquetado NBT y zlib a un hilo de menor prioridad que solo se despierte y trabaje durante esa ventana inactiva.
*   **URL:** GDC Vault (Postmortems de consolas) / Halo GDC
*   **Beneficio:** Elimina el *stuttering* al generar terreno.
*   **Dificultad:** Media. Requiere un buen control de mutex/spinlocks para no trabar el render.

## 5. Compresión LZX en lugar de zlib
*   **Qué es:** Cambiar zlib por el algoritmo LZX, usado de forma nativa por Microsoft en la Xbox, o LZ4 si se porta correctamente al x86. El Pentium III es malo para la predicción de ramas moderna.
*   **URL:** https://xboxdevwiki.net/ / GitHub nxdk
*   **Beneficio:** LZX estaba altamente optimizado en el XDK para descomprimir rápido usando poca CPU.
*   **Dificultad:** Media (requiere reemplazar la dependencia de descompresión).

## 6. Aprovechar 128MB de RAM en consolas modded
*   **Qué es:** Usar APIs como `MmAllocateContiguousMemory` verificando el límite superior. XBMC y Kodi lo detectan pidiendo bloques desde arriba. Requiere que la BIOS (ej. Cerbios) expanda el rango de la caché MTRR.
*   **URL:** https://www.xbmc4xbox.org.uk/ / r/originalxbox
*   **Beneficio:** Doble distancia de dibujado (render distance) o más chunks retenidos.
*   **Dificultad:** Baja-Media (solo condicionales si `GlobalMemoryStatus` reporta más de 64MB).

## 7. Gestión de Memoria por "Tags" (estilo Halo)
*   **Qué es:** Usar un gran archivo indexado al arrancar en lugar de tener miles de archivos en el disco duro. Se precargan "tags" estructurados para evitar la fragmentación de la memoria.
*   **URL:** https://www.bungie.org/ (GDC 2005 "Content Management for Halo 2")
*   **Beneficio:** Previene la fragmentación total de los 64MB de RAM y acelera los tiempos de lectura (seek times del FATX).
*   **Dificultad:** Alta (requiere reescribir la carga de assets y mods).

## 8. Swizzled Textures y DXT
*   **Qué es:** Mantener el atlas de texturas (terrain.png) de Minecraft pre-comprimido en formato DXT o en diseño "swizzled" nativo de Xbox. 
*   **URL:** https://xboxdevwiki.net/ / xemu docs
*   **Beneficio:** La GPU no pierde tiempo (ni memoria) convirtiendo la textura lineal al formato óptimo al leerla de la RAM.
*   **Dificultad:** Media (hay utilidades como xbedump o bundler en el XDK para hacerlo offline).

## 9. Trazado directo en los Shaders (Programmable Pipeline)
*   **Qué es:** El NV2A permite Vertex Shaders programables en D3D8. Mover los cálculos de rotación y traslación de entidades al hardware en lugar de hacer `glRotatef` en software.
*   **URL:** https://www.cherryservers.com/ / GDC Vault
*   **Beneficio:** Libera al PIII de las multiplicaciones de matrices flotantes, reduciendo la carga del CPU en entidades.
*   **Dificultad:** Media-Alta (hay que escribir los shaders en ensamblador de Xbox o un CG rudimentario).

## 10. Minimizar el "Pipeline Stall" por cambio de estados
*   **Qué es:** Ordenar el pintado de los chunks (opacos primero, luego transparentes, luego agua) en una sola pasada unificada, porque los cambios de estado (Alpha Blending, texturas) vacían el pipeline del NV2A.
*   **URL:** https://gamedev.stackexchange.com/
*   **Beneficio:** Maximizas la ganancia del push buffer al evitar que la GPU se estanque.
*   **Dificultad:** Baja (requiere ordenar listas de render).
