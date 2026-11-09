# Correcciones al encargo 16 (revisión de Claude)

El unity build **NO se aplicó**. Pruebas:
- `find wt-ltcg/build/xbox-release -name "unity_*"` → **0 archivos**. Cuando funciona, CMake genera `CMakeFiles/OptiCraft.dir/Unity/unity_N_cxx.cxx`.
- El XBE mide exactamente lo mismo que sin unity (4.182.016 bytes).

Además, en el informe:
- los avisos SSE2 salen como `at offset ...`: copia las líneas **literales** de la salida del build;
- "16+ hilos": no lo pudiste medir, no lo pongas.

Si no ves algo, escribe "no verificado".

Qué hacer:
1. Averigua por qué no se activa. Pistas:
   - ¿se define el target `OptiCraft` después del bloque, o en otro `CMakeLists`?
   - ¿las fuentes se añaden después?
   - ¿hace falta `--fresh`?
   - ¿la versión de CMake soporta `UNITY_BUILD`? (`cmake --version`)
   - mira `wt-ltcg\build\xbox-release\build.ninja` y busca "Unity".
2. Arréglalo en `cmake\xbox.cmake` (o en `CMakeLists.txt` si el target se crea ahí; en ese caso mueve el bloque junto al target). Mismas exclusiones y mismas reglas: no toques los archivos de agy.
3. Compila con `--fresh` y confirma que existen los `unity_*.cxx`: cuántos hay y cuántos `.cpp` excluiste.
4. xemu hasta el menú. Informe en `16-INFORME-opencode.md`, con una sección nueva `## Resultado 2` al final. Incluye el tamaño nuevo del XBE y las líneas SSE2 literales.
