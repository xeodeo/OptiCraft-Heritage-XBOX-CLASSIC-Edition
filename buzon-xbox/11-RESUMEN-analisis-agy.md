# Resumen de los informes 09 y 10 (agy, 28-09-2026), verificado por Claude

Los informes completos son `09-INFORME-opencode.md` (lo hizo agy, porque OpenCode falló) y `10-INFORME-agy.md`. En el 10, las URL son flojas o genéricas: tómalas como pistas, no como fuentes.

## Verificado con grep
- `SetFlickerFilter` no se usa en el repo: **cierto**.
- LTCG (`/GL`, `/LTCG`) no está en `cmake/xbox*.cmake`: **cierto**.
- El tope de partículas de Legacy es 200 (`ParticleEngine.h:14`). El port ya tiene `PLATFORM_MAX_PARTICLES_PER_LAYER` (`EffectRenderer.cpp:35`); falta revisar qué valor tiene en Xbox.
- El tope de mobs **ya existe** en el port (`PLATFORM_MAX_LIVE_MOBS 40`, `XboxTuning.h:125`). El punto 1 del TOP de agy ya está hecho.
- `EntityItem.cpp` no une los items del suelo: **cierto**. Legacy lo hace en `ItemEntity::mergeWithNeighbours` (`ItemEntity.cpp:135`).

## TOP propuesto (barato primero)
1. `SetFlickerFilter` o `SetSoftDisplayFilter`: una línea. Más nitidez y menos ancho de banda. Probar en la TV del usuario.
2. LTCG (`/GL` + `/LTCG`): solo compilación. Se espera un 10-15% de CPU. Cuidado con el parche SSE2 que se aplica después de enlazar.
3. Unir los items del suelo (`mergeWithNeighbours` de Legacy): menos `entTick` en granjas y explosiones.
4. Display lists en `ModelPart` para mobs (Legacy `ModelPart.cpp:290`). Hay que mirar antes si el port ya compila modelos.
5. Memoria write-combined para vertex buffers (hoy solo el audio usa `XPhysicalAlloc`, `SoundManager_XBOX.cpp:205`).
6. Revisar el tope de partículas en Xbox y compararlo con los 200 de Legacy.
7. Más adelante, de más esfuerzo: texturas swizzled o DXT, audio ADPCM, push buffers, 128 MB en consolas modificadas y LZX/LZ4 en vez de zlib.

## Notas de orquestación
- OpenCode (`--agent opencode`, modelo gratis `opencode/big-pickle` en `~/.config/opencode/opencode.json`) arrancó mostrando la ayuda y se cerró: no hizo nada. Falta investigar por qué.
- agy (Gemini 3.1 Pro) cumplió los dos encargos, unos 15 y 40 min. En el primero, el `worker_done` salió sin `--dispatch-capability` y Orca lo rechazó; lo repitió bien.
