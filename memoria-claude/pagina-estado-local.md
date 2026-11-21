---
name: pagina-estado-local
description: "Página de estado del port: buzon-xbox/ESTADO-OPTICRAFT.html (local), un .json por tarea en buzon-xbox/estado/ y generar.py"
metadata:
  node_type: memory
  type: project
  originSessionId: 5081e937-c767-46da-b247-a4e0f6c8024e
  modified: 2026-11-20T17:41:54.165Z
---

El usuario pidió (20-11) un HTML que Claude vaya actualizando con el estado de cada cosa de la 1.5.2 y la 1.2.5.
Es un fichero LOCAL: `Desktop\Minecraft en xboox clasico\buzon-xbox\ESTADO-OPTICRAFT.html` (no artifact: [[no-artifacts]]).

Datos: `buzon-xbox\estado\<id>.json` (campos `version` 1.5.2/1.2.5/ambas, `estado`
probado/por_probar/en_marcha/falta/fallo/aparcado, `area`, `nombre`, `nota`, `ref` nº de encargo, `orden`) y
`_meta.json` (`actualizado`, `agy`, `nota`). Plantilla `estado\plantilla.html`.

**How to apply:** cada vez que el usuario confirme algo en la consola, agy entregue o aparezca un fallo: editar o crear el
.json, `python -I generar.py` en `buzon-xbox\estado\` y abrir el HTML con `start "" "<ruta>"`. Va en la rama `contexto`
con el resto del buzón. Relacionado: [[estado-actual]].
