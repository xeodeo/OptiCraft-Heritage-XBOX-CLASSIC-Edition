ESTADO: TERMINADO

(Lo hizo Claude en lugar de OpenCode: la cuenta de OpenCode no tiene saldo para Gemini.)

- Creados `src/net/minecraft/src/legacy/XboxCraftingScreen.cpp` y `.h` desde `0a9dadf:.../LegacyCraftingScreen.*`,
  con `LegacyCraftingScreen` → `XboxCraftingScreen` en todo (clase, constructores, destructor, métodos, include).
- `#pragma once` en el .h. Las funciones y constantes auxiliares del .cpp están en namespaces anónimos: no chocan
  con el `LegacyCraftingScreen.cpp` de OptiJuegos.
- Constructores disponibles: `XboxCraftingScreen(EntityPlayer*)` (2x2) y `XboxCraftingScreen(EntityPlayer*, World*, x, y, z)` (mesa 3x3).
- No compilado (lo compila agy).
