---
name: modo-admin-sin-candado
description: Modo admin (122) SIN restricción de servidor/op, con Dupe y pestañas; no volver a poner candado ni documentarlo
metadata:
  type: project
---

El 23-11-2026 el usuario decidió quitar el candado del modo admin (antes: solo mundo propio / op). Ahora funciona en
cualquier mundo y servidor sin op, Nuker y Build incluidos (mandan paquetes de romper/poner). Menú por pestañas
(Blanco/Negro). Módulo Dupe: directo en local; en servidor manda `/optidupe` al plugin OptiDupe
(`servidor-plugins\OptiDupe`, en `servidor-1.12.2\plugins`).

**Why:** otra sesión dejó escrito en los documentos «solo en tu servidor, no se quita» y el usuario dijo que eso era
mentira y lo quería sin esa regla.
**How to apply:** no reintroducir el candado ni describirlo en PENDIENTES/00-CONTEXTO/estado. Ver [[subir-siempre-al-compilar]].
