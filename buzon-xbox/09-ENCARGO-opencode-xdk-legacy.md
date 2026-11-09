# ENCARGO 09 (OpenCode): qué más aprovechar del XDK y de Legacy

Solo LEER y ANALIZAR. **No modifiques ningún archivo** salvo tu informe.

## Contexto (medido)
- Juego: OptiCraft Heritage (Minecraft 1.2.5 en C++17) portado a la **Xbox original**: Pentium III 733 MHz (SSE1, sin SSE2), 64 MB de RAM compartida, GPU NV2A.
- Repo: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition`.
  - Render: `src\platform\RenderAPI_D3D8_XBOX.cpp` emula display lists de GL sobre D3D8, con vértices compactos de 16/12 bytes y `D3DPT_QUADLIST`.
  - Ajustes: `src\xbox\XboxTuning.h`. Documentación: `docs\XBOX_PORT.md`.
- XDK 5849: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox` (`include\*.h`).
- Legacy Console Edition (4J), solo lectura: `C:\Users\xeodeo\Downloads\minecraft` (`Minecraft.World`, `Minecraft.Client`).
- Medido en consola real: ~60 FPS a 2 chunks, 25-35 MB libres, frame de 16,7 ms con ~10 ms esperando vsync. Hay tirones de CPU al cargar o generar chunks (NBT + zlib en el hilo principal), en `entTick` y en `randomBlocks`.
- **YA analizado, NO repetir:**
  - día/noche sin reconstruir chunks;
  - caché de luz 16³;
  - tope a ticks aleatorios;
  - saltar bloques ocultos al mallar;
  - luz por planos;
  - guardar sin NBT;
  - hilo en el hueco del vsync;
  - bloques comprimidos de Legacy.

## Qué hacer
**A) XDK.** Funciones que el port NO usa y ayudarían. Busca uso en el repo con grep. Candidatas:
- push buffers (`BeginPushBuffer` / `RunPushBuffer`) en vez de display lists;
- texturas swizzled, DXT o P8 para ahorrar RAM;
- `XPhysicalAlloc` o memoria write-combined para vertex buffers;
- vertex shaders y register combiners (niebla y luz en GPU);
- triple buffering;
- `SetFlickerFilter`;
- fences;
- caché de disco `XMountUtilityDrive`;
- audio ADPCM por hardware;
- prioridades de hilo;
- libs sin debug y LTCG.

Para cada una: `archivo.h:línea`, si el repo la usa, beneficio esperado y riesgo.

**B) Legacy.** Trucos de rendimiento NO cubiertos arriba:
- mobs y entidades: display lists, distancia, límites;
- partículas;
- texturas animadas (agua, lava, fuego);
- sonido;
- tile entities;
- unión de items en el suelo;
- IA y pathfinding.

Y funciones de consola que encajen en 1.2.5, con complejidad baja/media/alta. Siempre `archivo:línea`.

## Entrega
- Informe en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\09-INFORME-opencode.md`. Primera línea literal: `# INFORME 09 opencode`.
- Máximo ~1000 palabras. Cierra con un **TOP 8** ordenado por beneficio/esfuerzo.
- No escribas nada en otro sitio. Si algo no está claro, di "no verificado" en vez de inventar.
