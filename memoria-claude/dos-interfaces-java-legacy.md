---
name: dos-interfaces-java-legacy
description: "Cada pantalla nueva (creativo, comercio de aldeanos...) debe tener versión Java y versión Legacy de consola según la opción legacyUI"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 03fd57b9-4231-4aeb-8c94-333661eea053
  modified: 2026-11-06T20:08:50.623Z
---

Toda pantalla del juego que se porte o haga nueva debe existir en las dos interfaces: la de Java normal y la Legacy
clásica (como la LCE), elegida por `GameSettings::legacyUI` (igual que `LegacyCraftingScreen` vs crafteo Java).
Pedido el 06-11 para el comercio de aldeanos. EXCEPCIÓN: el menú creativo se queda como está (el de la 1.2.5 le parece
perfecto); no hacer creativo Legacy.

**Why:** el usuario quiere que el modo Legacy se vea como la consola en todo, no solo en el crafteo.
**How to apply:** al diseñar una pantalla, mirar primero la LCE en
`Desktop\Minecraft en xboox clasico\minecraft-legacy original\Minecraft.Client\Common\UI\` (p.ej. `UIScene_TradingMenu`,
`IUIScene_CreativeMenu`) y hacer la versión Legacy junto con la Java. Ver [[legacy-lce-permitido]].
