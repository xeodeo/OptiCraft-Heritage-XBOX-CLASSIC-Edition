# 69. Plan completo de la 1.5.2 (03-11, noche): hecho, en marcha, por probar y lo que falta

> **09-11: el estado al día está en `ESTADO-ACTUAL.md`** (ramas, builds, decisiones, qué probar). Hecho desde el 06-11:
> aldeanos, eventos de entidad, sonidos de aldeano y bruja, spawn como Java, niebla y cielo en Xbox, rendimiento
> (ticks por chunk, texturas animadas). En la tabla 5, el punto 15 «Mayús+clic»: con mando no existe; propuesta Y = mover rápido.

Rama buena: `port152-claude` en `wt-152-codex` (último: `41ab79b`, build en `OptiCraft\1.5.2\` 03-11 16:56).
1.2.5: `xbox-opti-30` en `wt-opti30` (`2c4afa4`, build 03-11 16:12).
Guía de revisión: `55-GUIA-mcp-descompilar-y-revisar.md`. Faltas detalladas: `66-LO-QUE-FALTA-152-COMPLETO.md`.

## 1. Hecho y probado por el usuario en consola (OK)
Redstone, tolva, vagonetas, raíl activador, placas de peso, sensor de luz, soltador, templos, cofre extra, biomas grandes,
esqueleto Wither (Nether) + corazones negros, pigman, pociones, faro (efectos), mapa (dibuja), juntar objetos, cuarzo,
cofre trampa, menú creativo 12 pestañas (bonito), cruceta+A en inventarios (1.2.5 y 1.5.2), yunque (nombre y resultado).

## 2. Arreglado hoy por auditoría (03-11) — ver commits
Arranque (StatList/RecipesMapCloning), cuelgues al cargar mundo/morir (caja del pool borrada en EntityItem),
yunque/faro Mayús+clic (recursión), yunque gratis, faro sin efectos, orbes tras reaparecer, zombi con objeto encantado,
estructuras acumuladas, cuadros al revés, fugas, mapa en negro, marco (textura y orientación), receta vieja del mapa,
teclado detrás de objetos, coste del yunque (%1$d), nombre en cursiva (mal codificado: arreglado por agy 65),
libros encantados sin encantamiento (initEnchantments), idiomas 1.5.2 en los 58.

## 3. Por probar (build 03-11 16:56 `41ab79b`, con diagnósticos TEMP `3b71bea`)
1. Yunque: coste de XP con número (ya no `%1$d`), nombre en cursiva al mirar el objeto.
2. Marco: poner objeto y girarlo. Si falla, el netlog dirá `diag.use` (a qué apunta) y `diag.frame` (interact).
3. Libros encantados con su encantamiento en creativo y cofres; libros escritos «Página 1 de 5».
4. Textos en español (pestañas, faro, libros, menús de consola).
5. Pendiente de antes: cabaña de bruja, esmeraldas, guardar/cargar con mobs nuevos, tirón largo tras mucho creativo.
→ Cuando se cierre lo del marco/yunque: QUITAR los diagnósticos TEMP (`3b71bea`) con un revert.

## 4. En marcha
- **agy encargo 67** (`wt-152-agy`): creativo (tapa del cofre en la pestaña, cultivos que no deben salir,
  marco del End en 3D, buscador con teclado), **cruceta separada del joystick** (arriba cámara, abajo chat,
  izq/der objeto; inventarios igual), vídeo en 480p centrado (sin logo si hace falta), iconos en Controles.
- **Investigación de rendimiento** (subagente): `68-INVESTIGACION-rendimiento-ports-xbox.md` (ClassiCube Xbox,
  Melee-X, nxdk, NV2A). El usuario cree que se puede ganar MUCHO rendimiento: es la siguiente gran línea.

## 5. Lo que falta portar, en orden (cada uno = encargo con worktree propio; Claude revisa contra Java)
| # | Qué | Estado | Quién |
|---|-----|--------|-------|
| 1 | **Aldeanos**: comercio con mando y pantalla partida, aldeas del desierto, reputación, `EntityAIVillagerMate`, `VillageSiege` | **HECHO 06/09-11** (`port152-aldeanos`, informe 72); comercio y desierto confirmados, resto por probar | Claude |
| 2 | **Comandos** (~40) + `GameRules` + modo aventura + `PlayerSelector` + chat 1.5.2 (`GuiNewChat`) | no empezado | Codex/agy |
| 3 | **Wither** + barra de jefe (`BossStatus`, también dragón) | no empezado | agy |
| 4 | **Fuegos artificiales** (cohete, estrella, receta, partículas) | no empezado | agy |
| 5 | **Dispensador 1.5.2** (14 comportamientos) + bloque de comandos | no empezado | Codex |
| 6 | **Generador de mobs** nuevo + vagoneta con generador | no empezado | agy |
| 7 | **Cuero teñido** + caldero lava | no empezado | agy |
| 8 | **Encantamientos**: Espinas, libros en la mesa | no empezado | agy |
| 9 | **Mapas**: zoom/escala, marcadores en marcos | no empezado | agy |
| 10 | **Superplano personalizable** | no empezado | agy |
| 11 | **Marcador** (scoreboard) | con comandos | — |
| 12 | Combate: mensajes de muerte 1.5.2, flechas de fuego, explosiones | no empezado | Codex |
| 13 | Bloques: puertas, cama Nether, troncos tumbados, nieve en capas, vallas, cacao, hojas | no empezado | agy |
| 14 | Mobs: lobo collar, oveja mezcla, Enderman, slime pantano, gólem, gatitos, crías | no empezado | agy |
| 15 | Inventario: Mayús+clic a armadura/horno, repartir pila, XP del horno | no empezado | agy |
| 16 | Nether: cuevas con la semilla de 1.5.2 (`MapGenCavesHell`) | no empezado | Codex |
| 17 | Pantalla: nombre del objeto al cambiar casilla, calaveras en la cabeza, colores § | no empezado | agy |

Reglas para todos: un worktree por agente, candado `BUILD.lock`, compilar las dos variantes, nada de relleno,
`ensureEntityInit()` en entidades, NUNCA `delete` de cajas del pool (`expand/getBoundingBoxFromPool/getAABB`),
comprobar nulls donde Java los comprueba, ItemStack: quién es el dueño (DataWatcher/inventario borran).
Claude audita cada entrega con subagentes buscando cuelgues ANTES de pasarla a la consola.

## 6. Rendimiento (línea paralela)
Partir de `68-INVESTIGACION...`. Medir antes/después con el netlog (`xbox.perf`, `xbox.spike`, `xbox.mem`, `freeKB`).
Investigación hecha: `68-INVESTIGACION-rendimiento-ports-xbox.md` (texturas animadas 5 ms/frame y NO se ven animadas ahora; tick del jugador; autosave; randomBlocks). Por ahora SOLO medir (pedido del usuario 03-11).
Objetivos: memoria libre de la 1.5.2 (9-16 MB → más), tirón de `EntityPlayerSP` (74→250 ms), `DrawPrimitiveUP`
(`upDraws`) → buffers estáticos, entidades por tipo.

## 7. PS3
Otro chat. SDK listo en `Minecraft en xboox clasico\ps3\` (ver su `LEEME.md`).
