---
name: lce-repos-referencia
description: "Fuentes de la LCE (4J) en GitHub: cuál sirve (TU19 con recetas 1.5/1.6) y cuáles no; URLs por si hacen falta"
metadata:
  type: reference
---

**Sirve y está bajada:** https://github.com/hugozz26/MinecraftLCE-Xbox (LCE **TU19**, C++, para Xbox One UWP) en
`Desktop\Minecraft en xboox clasico\lce-tu19\` (clon superficial). Tiene `Minecraft.World/Recipes.cpp` con tolva,
comparador, carro con tolva, fuegos artificiales (`FireworksRecipe`, menú propio `FireworksMenu*.swf`) y la letra de
grupo de cada receta: referencia para las categorías del crafteo de consola de la 1.5.2. Nuestra
`minecraft-legacy original\` es más vieja (llega al yunque, sin 1.5).

Qué más trae la TU19 (contenido 1.6.4): `Minecraft.World` con caballos, riendas, etiquetas, alfombras, paja,
dispensador/soltador, bloque de comandos, marcador (Scoreboard/Team), mapas, tolvas, comparador, Wither, bruja;
`Minecraft.Client/Common/UI/IUIScene_*Menu.cpp` (faro, yunque, tolva, fuegos artificiales, caballo, crafteo, creativo,
encantar...) para las pantallas Legacy; `Common/Media/Graphics` (101 PNG) y `strings.resx`/`languages.loc`.

Origen de la TU19: https://github.com/smartcmd/MinecraftConsoles (retirado por DMCA, 451); espejo en
https://sourceforge.net/projects/minecraftconsoles.mirror/files/ (zip 2,6 GB del 04-05-2026, port a PC con teclado).
Es el mismo código que `lce-tu19` (su .sln se llama MinecraftConsoles.sln): no hace falta bajarlo.

**No sirven (borradas o sin bajar, 21-11-2026):**
- https://github.com/GRAnimated/MinecraftLCE — decomp de Switch v1.0.17, ~11 %, menú de crafteo vacío.
- https://github.com/6finality/LegacyEX — solo README + releases para el "Legacy Emerald Launcher".
- Otros vistos: Moxie-Archive/4jcraft (filtración, versión ?), 5l41dev/MinecraftLegacy-PS3XBOX ("fuente 2016"),
  chrisGrando/MinecraftLCE-7thGen (360/PS3/Vita), pcpro27/MinecraftLegacy-PS2, Lyall-A/MinecraftConsoles (parches),
  snipecope/Emerald-Legacy-Launcher y LCE-Emerald-Launcher (launchers), DexrnZacAttack/libLCE, zugebot/LegacyEditor.

**Forks de OptiCraft en bolha.dev (Forgejo, API /api/v1/repos/search):** erluza/OptiCraftHeritage (encargo 87),
ramide1/OptiCraftHeritage (46 commits propios: arco, cambio de dimensión, multijugador → encargo 98),
toroho/opticrafthorotege (copia de erluza + rasterizador software, no sirve), ramide1/CPPEditionLegacy (1.8.9, no sirve).

Relacionado: [[legacy-lce-permitido]], [[que-es-de-cada-version]].

**Herobrine original (confirmado por el usuario 22-11):** plugin de tkelly → https://github.com/darkdiplomat/iLoveYou (GPL v3), bajado en `mcp-diff\herobrine-iloveyou\`. Altar: netherrack encendido sobre piedra musgosa rodeada de 8 oros, 4 antorchas de redstone alrededor del netherrack.
