# INFORME 09 agy

## A) XDK: Características Analizadas

**1. Push Buffers (`BeginPushBuffer` / `RunPushBuffer`)**
- **Uso:** NO usado (el render usa D3DPT_QUADLIST emulando display lists). `xdk\5849\sdk\XDK\xbox\include\D3D8.h:752`
- **Beneficio:** Evita la sobrecarga de la CPU al traducir comandos gráficos; el NV2A recibe los comandos listos.
- **Riesgo:** Alta dependencia del hardware; reescritura masiva del render de OpenGL a macros del NV2A.

**2. Texturas Swizzled / DXT / P8**
- **Uso:** NO usado (el código especifica "Textures are swizzled A8R8G8B8" pero sube los píxeles de forma lineal sin compresión). `xdk\5849\sdk\XDK\xbox\include\D3D8Types.h`
- **Beneficio:** DXT reduce el uso de VRAM a una cuarta parte; el formato swizzle nativo acelera la caché de textura del NV2A (rasterización más rápida).
- **Riesgo:** Requiere pre-procesar las texturas (offline) o consumir tiempo de CPU al cargarlas.

**3. `XPhysicalAlloc` (Write-Combined)**
- **Uso:** SÍ, pero SOLO en audio (`SoundManager_XBOX.cpp:205`). NO en Vertex Buffers. `xdk\5849\sdk\XDK\xbox\include\xboxkrnl.h`
- **Beneficio:** Escribir vértices usando memoria Write-Combined evita contaminar la caché L2 de la CPU. Enorme mejora al reconstruir chunks.
- **Riesgo:** Las lecturas accidentales desde esta memoria por parte de la CPU provocarán bloqueos masivos de rendimiento.

**4. `SetFlickerFilter(0)`**
- **Uso:** NO usado. `xdk\5849\sdk\XDK\xbox\include\D3D8.h:788`
- **Beneficio:** Desactiva el filtro deflicker de hardware, mejorando la nitidez enormemente y reduciendo ancho de banda de memoria.
- **Riesgo:** Posible parpadeo (shimmering) en televisores CRT de tubo (480i).

**5. `XMountUtilityDrive` (Caché Z:)**
- **Uso:** SÍ. `src\xbox\system\XboxWritableRoot.cpp:29`
- **Beneficio/Riesgo:** Ya implementado para la caché temporal del mundo.

**6. Audio ADPCM (`XAudioCreateAdpcmFormat`)**
- **Uso:** NO usado en Xbox (se usa PCM a 16-bits. PS2 sí usa ADPCM). `xdk\5849\sdk\XDK\xbox\include\XAudio.h`
- **Beneficio:** El APU decodifica el audio por hardware, comprimiéndolo a 1/4 del tamaño en RAM.
- **Riesgo:** Hay que recomprimir todos los `.ogg`/.`wav` a formato XMA/ADPCM y configurar loops de hardware.

**7. LTCG (Link Time Code Generation) y Non-Debug Libs**
- **Uso:** NO usado (no presente en scripts CMake).
- **Beneficio:** Optimizaciones globales de todo el programa (inlining entre archivos). 10-15% más de rendimiento de CPU.
- **Riesgo:** Los volcados de memoria y el depurado pierden contexto.

**8. Prioridades de Hilo (`SetThreadPriority`)**
- **Uso:** SÍ. `src\platform\Thread.cpp:211`.
- **Beneficio/Riesgo:** Ya implementado para los hilos secundarios.

---

## B) Legacy Console Edition: Trucos Encontrados

**1. Modelos de Entidades Cacheados en Display Lists**
- **Archivo:** `Minecraft.Client/ModelPart.cpp:290`
- **Complejidad:** Baja
- **Beneficio:** `ModelPart::compile()` compila los cubos que forman el modelo del mob (piernas, cabeza, etc.) en un `glNewList`. En vez de renderizar los quads matemáticamente cada frame, simplemente ejecuta una lista. Salva mucho trabajo de GPU y CPU.

**2. Límites Estrictos de Mobs**
- **Archivo:** `Minecraft.World/MobCategory.h:20` (`MAX_XBOX_ANIMALS_WITH_BREEDING = 70`)
- **Complejidad:** Baja
- **Beneficio:** Los límites estáticos (`Level::canCreateMore`) capan la cantidad de animales y monstruos. PC no tenía límites tan firmes en la 1.2.5. Corta de raíz los problemas de `entTick` y el uso de RAM.

**3. Agrupación de Items en el Suelo**
- **Archivo:** `Minecraft.World/ItemEntity.cpp:135` (`ItemEntity::mergeWithNeighbours()`)
- **Complejidad:** Media
- **Beneficio:** Si dos ítems iguales (ej. piedra) caen cerca, se fusionan en un solo `ItemEntity` sumando el `count`. Reduce drásticamente la carga de redibujado de cajas y el peso físico en las granjas, evitando saturar `entTick`.

**4. Texturas Animadas Pre-calculadas**
- **Archivo:** `Minecraft.Client/AbstractTexturePack.cpp:180` (`getAnimationString()`)
- **Complejidad:** Alta
- **Beneficio:** Legacy omite el costoso `TextureFX` procedural de la 1.2.5 para el agua y lava. En su lugar, usa un sprite sheet gigante y un archivo `.txt` con los índices para rotar los píxeles. Quita la mayor carga del hilo principal de CPU.

**5. Límite Drástico de Partículas**
- **Archivo:** `Minecraft.Client/ParticleEngine.h:14` (`MAX_PARTICLES_PER_LAYER = 200`)
- **Complejidad:** Baja
- **Beneficio:** Reduce el límite de 4000 a 200 partículas. Es una solución de fuerza bruta pero muy efectiva en las limitaciones de llenado (fillrate) de consola.

---

## TOP 8 Optimizaciones (Mayor beneficio vs Menor esfuerzo)

1. **Límites Estrictos de Mobs:** (Legacy) Implementar los límites de `MobCategory`. Es trivial (variables constantes y checks) y elimina el peor caso de lag.
2. **Límite Drástico de Partículas:** (Legacy) Bajar a 200 partículas (trivial).
3. **SetFlickerFilter(0):** (XDK) Solo una línea de código y mejora radical del sharpness e inyección de ancho de banda a la GPU.
4. **Agrupación de Items en el Suelo:** (Legacy) Adaptar el `mergeWithNeighbours()` en `ItemEntity.cpp`. Coste de implementación medio pero inmenso alivio en físicas.
5. **Display Lists en `ModelPart`:** (Legacy) Cambiar `ModelPart::render` para que guarde sus llamadas en una display list. Requiere añadir un id y compilar una sola vez.
6. **LTCG:** (Build) Activar `/LTCG` y `/GL` en el compilador sin cambiar código.
7. **XPhysicalAlloc con Write-Combine:** (XDK) Reemplazar los alojamientos de memoria de los buffers de vértices de los chunks. Salva la caché de CPU.
8. **Texturas Animadas Pre-calculadas:** (Legacy) Alto esfuerzo para crear el sistema y exportar los assets a `.png`, pero la carga que elimina de la CPU en el render de agua lo hace vital.
