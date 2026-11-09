---
name: xdk-sin-instalar
description: Cómo sacar las libs/headers de cualquier XDK de Xbox sin instalarlo (7z abre XDKSetup.exe) y dónde están las 24 versiones
metadata:
  node_type: memory
  type: reference
  originSessionId: 61393428-185c-4352-893f-714f761ccd9c
  modified: 2026-10-15T22:04:53.443Z
---

Las 24 versiones del XDK (2000-2005) están en `Desktop\Minecraft en xboox clasico\xbox-sdks\*.7z`. Método ya probado (OptiCraft, documentado en `wt-pr\docs\XBOX_PORT.md`): dentro del .7z (o del .rar interno) está `XDKSetup<build>.exe`; no se ejecuta, se abre con `7z x XDKSetup<build>.exe -o<destino>` y lo útil queda en `XDK\xbox\lib` y `XDK\xbox\include`. XDKRecovery*.exe / Recovery ISO no sirven (imagen de consola). La 5849 ya extraída en `Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox`; las demás se extraen a `Desktop\AnyXboxClasico\xdk\<build>\` para [[anyxboxclasico]].
