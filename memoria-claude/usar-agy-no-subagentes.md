---
name: usar-agy-no-subagentes
description: "Encargos de implementación grandes van a agy (skill agy), no a muchos subagentes/forks de Claude"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-09T15:21:03.342Z
---

No crear muchos subagentes/forks de Claude para implementar: los encargos se dan a agy (skill `agy`, ver [[skill-agy]]).
Claude escribe el encargo en buzon-xbox, escucha a agy y audita/revisa la entrega.
Dicho el 09-11-2026: «ojo, estás creando mucho tú, tenemos agy».

**Why:** coste/uso de Claude; agy está para el trabajo pesado.
**How to apply:** implementación larga → encargo `buzon-xbox/NN-ENCARGO-agy-*.md` + lanzar agy; Claude se queda con revisar,
auditar (un subagente de auditoría está bien) y arreglos pequeños hechos a mano.
