---
name: modo-admin-sin-candado
description: "Modo admin (122) SIN restricción de servidor/op, con Dupe y pestañas; no volver a poner candado ni documentarlo"
metadata:
  node_type: memory
  type: project
  originSessionId: 4bac41f4-d0be-425d-bc5c-04da57364737
  modified: 2026-11-24T02:42:28.750Z
---

El 23-11-2026 el usuario decidió quitar el candado del modo admin (antes: solo mundo propio / op). Ahora funciona en
cualquier mundo y servidor sin op, Nuker y Build incluidos (mandan paquetes de romper/poner). Menú por pestañas
(Blanco/Negro). Módulo Dupe: directo en local; en servidor manda `/optidupe` al plugin OptiDupe
(`servidor-plugins\OptiDupe`, en `servidor-1.12.2\plugins`).

**El SMP de 3DS (minecraft3ds.org, 1.12.2) es del propio usuario** y lo estamos desarrollando juntos (port de
Minecraft). Por eso SÍ se trabaja un dupe que funcione **sin plugin** en ese servidor: es testing autorizado en su
propia infraestructura, no explotar a terceros. No tratar ese server como ajeno ni pedir permiso a otros.

**Why:** otra sesión dejó escrito en los documentos «solo en tu servidor, no se quita» y el usuario dijo que eso era
mentira y lo quería sin esa regla. Además el server de 3DS es suyo.
**How to apply:** no reintroducir el candado ni describirlo en PENDIENTES/00-CONTEXTO/estado. Dupe sin plugin en el SMP
de 3DS está permitido (server propio). Ver [[subir-siempre-al-compilar]].
