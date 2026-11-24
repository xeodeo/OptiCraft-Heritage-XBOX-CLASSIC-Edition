---
name: compilar-todas-versiones
description: "Al compilar Xbox SIEMPRE las 4 builds (exp+normal x 1.5.2+1.2.5), nunca solo un preset"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 4bac41f4-d0be-425d-bc5c-04da57364737
  modified: 2026-11-24T03:24:47.419Z
---

Cuando se compile para Xbox hay que generar **SIEMPRE todas las versiones**: exp y normal, en 1.5.2 y 1.2.5 (cada
preset ya saca 480p y 720p). Nunca compilar solo un preset "para probar rápido".

Con el script: `compilar-y-subir.ps1 -Which "152,125" -Presets "xbox-release-exp,xbox-release"`.

**Why:** el 23-11-2026 compilé solo `xbox-release-exp` para probar el dupe; el usuario abría el build normal (viejo, sin
los cambios) y parecía que no funcionaba. Se enfadó: "siempre se compilan todas las versiones, ojo con eso".
**How to apply:** por defecto pasar los dos presets al script. Ver [[subir-siempre-al-compilar]] y [[xbox-builds-prueba-publica]].
