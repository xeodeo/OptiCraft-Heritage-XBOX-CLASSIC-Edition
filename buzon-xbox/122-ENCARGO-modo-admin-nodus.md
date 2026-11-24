# 122. Encargo: mod «Modo admin (pruebas)» con las utilidades de Nodus 1.5.2 (23-11)

El usuario trajo `C:\Users\xeodeo\Downloads\nodus_1.5.2.zip`: **Nodus** (cliente de trucos para Minecraft **1.5.2**,
mayo 2013, por Scetch; Y = menú, U = consola). Copia extraída para leer: `%TEMP%\nodus_inspect\` (minecraft.jar
ofuscado; los .class de Nodus están en la raíz con nombres de 1–2 letras). Descompilar con CFR/Fernflower a una
carpeta nueva solo para leer (no ejecutar nada de ahí).

## Qué portar (mod nuevo en el catálogo, id `adminmode`, apagado por defecto)
Utilidades para PROBAR el juego: Flight, NoFall, FullBright, Freecam, FastBreak, FastPlace, AutoTool, AutoMine,
AutoFish, AutoWalk, AutoEnchant, HighJump, NoSlowDown, NoRender (partículas/clima), BlockESP, ChestFinder, CaveFinder,
AutoSign, Notifications. Comportamiento como Nodus (léelo del jar), adaptado a mando (menú propio estilo LCE, igual que
los demás mods, en las dos interfaces; combo para abrirlo que no choque con los existentes) y pantalla partida.

## Dónde funciona
En cualquier mundo y servidor, sin op (decidido por el usuario el 23-11). Ya existen X-Ray y minimapa: no duplicar.

## Reglas
Ambas versiones (1.5.2 primero, luego 1.2.5); registrar el mod en `ModManager` (y en el apagado de mods del encargo
115); candado `buzon-xbox\BUILD.lock`; `git add <ficheros>`; sin push ni lanzador; 60 fps; 64 MB. Informe
`buzon-xbox\122-INFORME.md`; señal de fin `CIENTOVEINTIDOS-LISTO` + commits.

## PREPARADO POR CLAUDE (23-11 16:30): Nodus ya descompilado
- Código fuente descompilado (Vineflower) en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\nodus-152\`
  (carpeta con MAYÚSCULAS/minúsculas distintas activado: `xk.java` y `XK.java` son clases DIFERENTES; no copies esa
  carpeta a otra normal de Windows o se pisan).
- `xk.java` = enum con el estado de cada módulo (a..Y). `zg.java` (lista «Enabled Mods») dice qué letra es cada uno:
  b Aimbot, c AutoEgg, d AutoEnchant, e AutoFish, g AutoMine, h AutoSign, i AutoTool, j AutoWalk, k BlockESP,
  l BowAimbot, m Build, n CaveFinder, o ChestFinder, p ClickAimbot, q Day, r Derp, s Disable Nametags, t ESP,
  u FastBreak, v FastPlace, w Flood, x Fly, y Forcefield, z Freecam, A FullBright, C Health, D HighJump, F NoFall,
  G NoFlinch, H NoRender, I NoSwing, J Notifications, K Nuker, M NoSlowDown, N Sneak, O Spider, P Sprint, Q Step,
  R Timer, S Tracers, U WallHack, V Water, W Waypoints, X Weather, Y XRay.
- Los efectos están repartidos en ganchos dentro de clases vanilla ofuscadas de 1.5.2: buscar
  `xk.<letra>.getState()` (40 ficheros: Df, es, fJ, fT, Fx, gQ, kM, kv, la, lM, mc, nU, nZ, ob, of, oT, pb, pj, pp,
  pr, pS, pY, qG, S, sT, wL, xL, ys, zA, zC, zH, zi, zj, zK, zl, zr, zv, zx, zz). Para saber qué clase vanilla es cada
  una, compara con `mcp-diff\src152` (mismos métodos/constantes) o con los mapeos MCP 7.51 de 1.5.2.
- NO portar el modo «Swastika» de Build (solo Pole/Wall/Floor). Flood/Timer/AutoSign sobre servidores: solo en propios.

## PAUSADO (Claude, 23-11 por la tarde, a petición del usuario)
El subagente se paró a mitad. Todo su trabajo está guardado en el commit `217e5394` «WIP adminmode» de la rama
`port152-admin` (carpeta `wt-152-choco`, también subido al remoto `xbox`): `src/mods/adminmode/` (AdminHooks.h,
AdminModeMod.*, GuiAdminMenu.*) y ganchos en 25 ficheros (+151 líneas). SIN compilar ni probar. 1.2.5 sin empezar
(`wt-125-choco`, rama `opti30-admin`). Para seguir: retomar desde ese commit.
