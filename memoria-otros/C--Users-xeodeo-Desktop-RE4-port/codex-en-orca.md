---
name: codex-en-orca
description: Codex (CLI de OpenAI) está disponible como tercer agente; cómo lanzarlo en Orca y sus límites
metadata:
  node_type: memory
  type: reference
  originSessionId: 361f689a-abcf-4152-80b3-6a3ef3c0b5fa
  modified: 2026-11-11T23:55:17.417Z
---

Desde el 11-11-2026 el usuario tiene Codex (`codex-cli`, en `AppData\Local\Programs\OpenAI\Codex\bin`) además de los agy ([[sesiones-cortas-y-sin-vigilantes]], skill `agy`).

- Lanzarlo en una pestaña de Orca: `orca terminal create --title "<nombre>" --command 'codex -s workspace-write -a never -C "C:\Users\xeodeo\Desktop\RE4 port"'` y mandarle un puntero a un `buzon\ENCARGO-codex-*.md` con `orca terminal send`.
- Árboles suyos en RE4: `re4-codex` y `xbox-nativo-codex` (rama `codex-recorte`).
- Límites vistos: su sandbox no puede ejecutar WSL (no compila con MinGW; comprueba con clang) y dio por bueno código que no compilaba en D3D8. Revisar y compilar siempre su entrega, igual que con agy.
- Rinde bien en diagnóstico razonado con fichero:línea y es honesto separando lo medido de lo estimado.
