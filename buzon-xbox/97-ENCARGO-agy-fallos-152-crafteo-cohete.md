# 97. Encargo agy: fallos de la 1.5.2 vistos en la consola real (21-11)

Rama `port152-erluza` (`wt-152-rend`, último `1e036e1`). El punto 5 también en 1.2.5 (`opti30-erluza`, `wt-opti30`,
último `90faed9`) si Java 1.2.5 lo hace distinto. Sin reset, sin push, sin copiar al lanzador. Reglas de siempre:
BUILD.lock, 60 fps, memoria (64 MB), creativo NO se toca (salvo el punto 2: que la mesa funcione en creativo),
terreno igual, dos interfaces (Java y Legacy), pantalla partida. Commits solo con `git add <fichero>`.

## 1. CRASH al lanzar un fuego artificial (consola real)
Al lanzar un cohete (crafteado en el menú de crafteo de consola) la consola se colgó:
`[crash] Unexpected error: bound must be positive` (x2) y después `main stuck ... phase=frameBegin`. Extracto en
`buzon-xbox\97-netlog-crash-cohete.txt`. Es `Random::nextInt(0)` (`src/java/Random.cpp:128`). Los `nextInt` de
`EntityFireworkStarterFX`, `SoundPool` y `EntityFireworkRocket` ya están protegidos: busca el que no lo está
(cohete sin NBT o con `Explosions` vacío, `Flight` 0, estrella sin `Colors`, preset del crafteo de consola con el tinte
comodín -1, render del cohete, sonido...). Reprodúcelo en xemu crafteando el cohete EXACTAMENTE por el menú de
consola (cohete solo, cohete con estrella, cada forma) y lanzándolo desde el suelo y el dispensador. Que una excepción
así nunca vuelva a colgar el juego.

## 2. Menú de crafteo de consola (`XboxCraftingScreen`, `LegacyCraftingGroups.h`, `make_crafting_groups.py`)
- Cosas raras: duplicados (p. ej. el faro sale dos veces) y objetos en la categoría equivocada. Revisa TODAS las
  recetas: cada una una sola vez y en su categoría. Referencia: la LCE original
  (`minecraft-legacy original\Minecraft.World\Recipes.cpp` y `*Recipies.cpp`: cada `addShapedRecipy` termina con su
  letra de grupo, p. ej. `L'D'` = decoración). Para lo que la LCE no tiene (1.5: tolva, comparador, sensor de luz,
  raíl activador, carro con tolva/TNT, faro, fuegos artificiales...), ponlo donde la LCE de la 1.5 (TU14+) lo pone;
  si no lo sabes seguro, en la categoría lógica (redstone → mecanismos, carros → transporte) y anótalo en el informe.
- ~~Mesa en creativo~~: el usuario corrigió (21-11): SÍ funciona. No tocar.
- Estrella de fuego artificial: el icono del resultado SIEMPRE muestra el saco de tinta, aunque craftea bien. Que
  muestre la estrella (con su color) y el cohete lo que toque.
- Lista final de categorías en el informe. Capturas de cada categoría.

## 3. Faro: los textos del efecto salen debajo del icono
Cobra bien. Pero en la pantalla del faro el texto de cada efecto se dibuja debajo/encima del icono. Igual que Java
1.5.2 (`GuiBeacon`), en las dos interfaces, 480 y 720p.

## 4. Zombi pigman
- A veces sale uno con una textura rara (sin pack de texturas). Mira `RenderPigZombie`/textura `/mob/pigzombie.png`,
  la armadura/objeto en mano, o el zombi aldeano/hijo de 1.5.2 usando la textura equivocada.
- Al pegar a uno, no se enfadan todos los de alrededor. En Java 1.5.2 `EntityPigZombie.attackEntityFrom` enfada a
  todos los pigman en 32 bloques (`becomeAngryAt`). Igual que Java.

## 5. Romper hielo en el Nether pone agua (probablemente las dos versiones)
Compara `BlockIce.harvestBlock` con `mcp-diff/src152` y `src125`: si Java evapora el agua en el Nether
(`worldProvider.isHellWorld`), hazlo igual; si Java 1.2.5 de verdad pone agua, deja la 1.2.5 como Java y dilo.

## Aceptación
- Cohete: 20 lanzamientos de todas las variantes (crafteo de consola y Java), suelo y dispensador, sin crash.
- Crafteo: sin duplicados, categorías revisadas, icono de la estrella correcto.
- Faro, pigman y hielo como Java. Capturas en `buzon-xbox\97-capturas\`. 60 fps igual.

## Informe
`buzon-xbox\97-INFORME-agy.md`. Señal: `ENCARGO 97 LISTO` + commits.
