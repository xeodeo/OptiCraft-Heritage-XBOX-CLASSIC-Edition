# 132 — Encargo: bot de minería, hacks de burros y empujones en alfombras/vallas/nenúfares (26-11-2026)

Pedido del usuario tras probar la 1.2.5 en el SMP 1.12.2 (minecraft3ds.org). Ramas: 1.2.5 `opti30-erluza` (wt-opti30),
1.5.2 `port152-int` (wt-build-152). OJO: en las dos carpetas hay trabajo SIN COMMIT de Claude (experimento dupe, arreglo
redstone/Gysahl, ESP): commitear solo con `git add <fichero>`.

## A. Empujones en el SMP (1.2.5 primero) — lo más urgente
Las losas ya van (ef44334). Ahora empuja/pega al pisar: **alfombras** (se ven), **vallas en esquina** y **nenúfares**.
Causa probable igual que las losas: la forma del bloque en el cliente ≠ la del server.
- `ProtocolTranslator340.cpp`: alfombra 171 → capa de nieve 78 (0,125 alto; la alfombra mide 0,0625); vallas 188-192 →
  valla de roble 85 (las esquinas/conexiones cambian en 1.12); nenúfar 111 (caja distinta en 1.12).
- Reproducir con el Paper local (`servidor-1.12.2\iniciar.bat`, puerto 25566, offline) y el log «server teleport».
- Arreglo esperado: que la caja de colisión del cliente coincida con la del server (p. ej. alfombra = bloque fino propio
  o sin colisión a 1/16, valla conectando como en 1.12, nenúfar con la caja de 1.12).

## B. Bot de minería automática
"Elijo diamante (o hierro...) y él solo baja y mina". Módulo nuevo del modo admin (src/mods/adminmode).
- Referencia: **Baritone** (https://github.com/cabaletta/baritone, LGPL-3; viene dentro de `Downloads\wwe.jar`):
  `IMineProcess`/`MineProcess` (buscar → ir → minar), pathfinding A* por bloques, costes de movimiento.
- Para nosotros, versión simple: usar el escaneo del BlockESP (`espMatches`) para elegir el objetivo, A* en una ventana
  de chunks cargados, cavar escalera hacia abajo hasta la capa, minar con la mejor herramienta, parar con poca vida o
  inventario lleno, y mando para cancelar. Medir RAM/CPU (Xbox 64 MB, 60 fps de referencia en 1.5.2).

## C. Burros (en el SMP confirman que hay)
- **Donkey tracer / ESP**: líneas y cajas a burros/llamas/caballos (como el ESP de jugadores del modo admin).
- **Dupe de burro** y **MountBypass**: referencias `creepy-salhack` (https://github.com/CreepyOrb924/creepy-salhack,
  `module/exploit/MountBypassModule.java`, `EntityDesyncModule.java`) y `wwe.jar` (cliente WWE 1.12.2 ofuscado, textos
  cifrados; toca `GuiScreenHorseInventory`/`AbstractHorse`; descompilar con CFR si hace falta).
- PROBLEMA de fondo: 1.2.5/1.5.2 no tienen caballos ni burros. El traductor 340 tiene que pasar la entidad (verla,
  montarla) y el inventario del burro con cofre. Empezar por el tracer (solo necesita la posición de la entidad).

## D. Pequeño
- Aviso al activar ChocoCraft: «los chocobos solo salen en terreno nuevo / al alejarte» (el usuario creyó que no
  salían al crear el mundo; andando más sí salieron).

## Entrega
Informe en `buzon-xbox\132-INFORME.md`; compilar y subir con `scripts-claude\compilar-y-subir.ps1` (todas las versiones);
actualizar `estado\*.json` (smp-alfombras-vallas, bot-mineria, burros-hacks) y `python -I estado\generar.py`.
