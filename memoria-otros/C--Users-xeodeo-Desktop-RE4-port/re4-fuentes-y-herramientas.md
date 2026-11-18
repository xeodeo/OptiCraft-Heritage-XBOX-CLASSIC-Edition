---
name: re4-fuentes-y-herramientas
description: "Qué volcado de RE4 sirve, dónde están CMake/MSVC en esta máquina, y que la build Debug trae símbolos y habla por EXI"
metadata:
  node_type: memory
  type: project
  originSessionId: 6b41f0d9-721b-4fb7-a4d2-78d8b7f5f5c7
  modified: 2026-10-01T22:03:00.858Z
---

Detalles del entorno de `~/Desktop/RE4 port` que cuestan tiempo si se
redescubren:

- **Usar `Resident Evil 4 Debug (Disc 1).iso`.** `RE4 Review.iso` (4.6 GB) no
  tiene cabecera de disco GameCube válida (magic `05050505`, sin ID de juego)
  y no sirve como fuente.
- **La build Debug trae 123 ficheros `.sym`** con los nombres reales de Capcom
  (`cEm06::move`, `CameraControl::CoreDataRead`). `tools/sym2csv.py` los
  convierte al CSV del recompilador; de `Bio4.sym` salen 9889 símbolos. Sin
  esto, las trazas son direcciones desnudas.
- **La build Debug escribe sus diagnósticos por el puerto EXI**
  (`0xCC006810`), no por el log. Si un `OSReport` sale vacío, el texto está
  ahí: reconstruir las escrituras como ASCII. Así se recuperó el mensaje del
  assert de `vi.c`.
- **CMake, Ninja y MSVC no están en el PATH**, pero sí dentro de VS2022
  Community: `Common7/IDE/CommonExtensions/Microsoft/CMake/{CMake/bin,Ninja}`
  y `VC/Tools/MSVC/14.44.35207`. Hay que cargar `VC/Auxiliary/Build/vcvars64.bat`.
- **Los heredoc de Bash en esta máquina se comen las barras invertidas.** Para
  scripts de Python que necesiten `\n` o `\x00` literales, construirlos con
  `chr(92)` / enteros, o escribir el fichero con la herramienta Write.
- **Ninja NO sabe que los `output_*.cpp` dependen de `cpu_context.h`.** Si se
  cambia la cabecera hay que hacer `touch export/output_*.cpp export/main_output.cpp`
  o el .exe se cuelga al arrancar (objetos con la estructura vieja). Ademas el
  runtime existe dos veces: `nwiirecomp/nWiiRuntime` y `export/nWiiRuntime`; se
  compila la de `export`, hay que copiar los cambios.
- Los ficheros del proyecto usan **CRLF**; los parches por bytes deben contarlo.

Contexto del proyecto en [[re4-port-via-nwiirecomp]].
