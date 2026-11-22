---
name: classic-xbox-store
description: "Classic Xbox Store (nuestra): XBE nativo con HTTPS (mbedTLS) que baja juegos de archive.org; reutilizar su Http.cpp"
metadata:
  type: reference
---

El usuario y Claude hicieron la **Classic Xbox Store**: app nativa de Xbox original que muestra el catálogo de
archive.org, descarga e instala juegos en el disco duro. Código: `C:\Users\xeodeo\Downloads\Desktop\de kevin\Classic Xbox Store\`
(github.com/xeodeo/Classic-Xbox-Store), release v1.0 en `Desktop\release\Classic Xbox Store\`. Tiene **HTTPS completo
en la Xbox**: `src/net/Http.cpp` + `mbedtls_config.h` + `external/mbedtls/` (verifica certificados, SNI, descarga a
disco por trozos). Hay también una «Classic PS3 Store» en `Downloads\Desktop\de kevin\`.

**Why:** 22-11 dije que no teníamos HTTPS en la Xbox y el usuario me corrigió («nosotros la hicimos»).
**How to apply:** para cualquier cosa de red/descargas en OptiCraft (packs, skins, catálogos), reutilizar ese cliente
antes de escribir otro. Encargo 110. Relacionado: [[plataformas-que-hacer]].
