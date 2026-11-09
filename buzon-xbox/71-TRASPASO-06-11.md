# 71. Traspaso 06-11 (para quien siga)

Empieza por aquí, luego `69-PLAN-COMPLETO-152.md` (plan general) y `55-GUIA-mcp-descompilar-y-revisar.md`.

## Estado de las ramas
- **1.5.2 buena**: `port152-claude` en `wt-152-codex`, último `98b72b0`. Build en
  `Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\OptiCraft\1.5.2\` (06-11 13:45; el `assets.pak` es el de
  13:39). Datos del pak: `wt-152-codex\build\xbox-release\port152-data` (rehacer pak con `scripts/make_pak.py`).
- **1.2.5 buena**: `xbox-opti-30` en `wt-opti30`, último `2c4afa4` (doble clic de A con cruceta arreglado). Build 03-11 16:12.
- Worktrees de agentes: `wt-152-agy` (agy, término encargos 58/67/70, todo integrado), `wt-152-idiomas` (agy 65, integrado),
  `wt-152-pociones` (WIP aldeanos `c746c8f`, sin compilar).

## Probado OK por el usuario en consola (1.5.2)
Redstone, tolva, vagonetas, templos, pociones, faro (efectos), yunque (resultado, nombre, coste), mapa, marco del objeto,
corazones negros (Wither), cofre trampa, menú creativo 12 pestañas (sin cultivos, marco End 3D), cruceta+A en inventarios,
iconos de controles (RT/LT), idiomas.

## Hecho el 06-11: CONFIRMADO por el usuario el 06-11 (nombres y huevo bien, teclas «mejoraron»)
La 1.2.5 queda como está (el usuario no quiere el punto 1 de abajo por ahora): se sigue con la 1.5.2.
- Nombres de piedras con pez plateado (`monsterStoneEgg`) y variantes de troncos/tablones/ladrillos/arenisca (Java).
- Iconos del joystick con flecha (stick grande LCE + flecha de `ButtonLeftStick_sides`).
- Cruceta configurable (encargo 70 + corrección de Claude): por defecto IZQ/DER libres, ARRIBA/ABAJO subir/bajar volando en
  creativo, objeto ant./sig. en Blanco/Negro (L1/R1 en PS2), perspectiva en clic stick derecho, chat SIN botón (asignable).
- Chat: B cierra; en un jugador el mensaje se ve («<nombre> msg»).

## 06-11 tarde: aldeanos listos para probar
Rama `port152-aldeanos` (`wt-152-pociones`, `1e9fb58`, sobre `port152-claude`): comercio Java + Legacy (LCE, con
legacyUI), aldeas del desierto, auditoría con 9 arreglos. XBE copiados a `OptiCraft\1.5.2` y `OptiCraft_actualizar`
(15:44). Qué probar: `72-INFORME-aldeanos.md`. Si va bien: merge a `port152-claude`.
El creativo NO se hace Legacy (el usuario lo quiere como está). Rendimiento: propuesta en el chat, primero texturas
animadas (no animan y gastan 5 ms), pendiente de que el usuario diga.

## 09-11
- Niebla arreglada (proyección W-friendly): 1.5.2 `227432b` (port152-aldeanos) y 1.2.5 `36f2e53` (xbox-opti-30). Las dos
  builds en el lanzador (1.2.5 09:55, 1.5.2 09:47 con pak nuevo); los XBE viejos de 1.2.5 en `OptiCraft_respaldo_125_03-11`.
- Sonidos de aldeano (de la 1.6; en 1.5.2 era mudo) `b1c68a7`. Agua quieta = opción de animaciones apagada, no fallo.
- OptiFine 1.5.2: investigación en marcha → `74-INVESTIGACION-optifine-152.md`.

## Lo siguiente (orden propuesto, aceptado por el usuario: «joya»)
1. **Pasar a la 1.2.5** (`wt-opti30`) lo que ya funciona en 1.5.2: cruceta configurable + teclas LSTICK + iconos
   (commits `db76200`/`1657f4b`, `c4cc6d9`, `bf70d05`, `31df245`, `1c053a5`), `EntityPlayer::getHeldItem` (`89fe089`),
   chat (`89fe089`), y el teclado sobre los objetos (`2932dfd`, parte EntityRenderer). OJO: 1.2.5 no tiene creativo 1.5.2.
2. **agy retoma los aldeanos** (encargos 62/63, WIP `c746c8f` en `wt-152-pociones`; rebase sobre `port152-claude`).
3. Comandos + GameRules + modo aventura → Wither → fuegos artificiales → dispensador → generador → cuero → resto (tabla en 69).
4. Rendimiento (`68-INVESTIGACION...`): por ahora SOLO medir. Primero: agua/lava/fuego NO se animan (también en 1.2.5) y
   gastan ~5 ms/frame en `textureStage`.
5. Pantalla partida: el jugador 2 aún tiene la cruceta fija (`XboxSplitScreen.cpp` ~260).

## Lecciones (cumplirlas)
- Toda entrega de Codex/agy se audita con subagentes (cuelgues: `delete` de cajas del pool, nulls que Java comprueba,
  recursión, dueños de `ItemStack*`) ANTES de dársela al usuario. Ya llegaron 3 cuelgues a la consola por no hacerlo.
- Revisar que lo básico sigue (andar con el stick, saltar, inventario) tras tocar controles: agy lo rompió una vez.
- Si una pantalla dibuja iconos con profundidad, los textos/teclado encima deben ir sin depth test.
- Al ediar con python heredoc: `\\xc2` en un str normal escribe U+00C2 (doble codificación). Usar bytes o `\xc2\xa7` en C++.
- El pak manda sobre lo embebido en el XBE (p.ej. `buttons_xbox.png`): si cambia, rehacer el pak.
- Receptor del log: `python -u xbox-spike\escuchar_log.py` (UDP 9999 → `xbox-spike\netlog.txt`). Cargar mundo da un
  aviso `watchdog ... frameBegin` de 3-5 s: es normal.
