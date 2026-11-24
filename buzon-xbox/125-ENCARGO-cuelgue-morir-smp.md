# 125. Encargo agy: cuelgue al MORIR en el servidor 1.12.2 (consola real, 23-11) — 1.2.5

Carpeta `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-125-mods`, rama `opti30-respawn` (desde
`opti30-erluza` `a5ee300`). Sin push ni lanzador; commits con `git add <ficheros>`; candado `buzon-xbox\BUILD.lock`.

## El fallo (log de la consola real)
En el SMP `minecraft3ds.org` (Paper 1.12.2, entrado con el traductor `ProtocolTranslator340*`), al morir:
GuiGameOver → GuiDownloadTerrain → (none) → (none) → GuiDownloadTerrain → … (DOS cambios de dimensión seguidos al
reaparecer) y luego `crash c0000005 write 00000000` en `RtlpCoalesceFreeBlocks` ← `RtlFreeHeap` ← `free` ←
`~DataWatcher` ← `~Entity` ← `World::updateEntities` (mapa de la build exp). El heap ya estaba corrompido antes:
algo escribió en memoria liberada o borró dos veces (sospecha: el jugador viejo / entidades entre dos
`NetClientHandler::handleRespawn` → `Minecraft::changeWorld1` + `Minecraft::respawn` seguidos; mira también
`WorldClient` entityHash, `isActiveClientEntity` en World.cpp, punteros a entidades en EntityRenderer/RenderGlobal,
el traductor `s.entities`).

## Qué hacer
1. Reproducirlo en xemu SIN tocar el servidor real: banco del encargo 121 en
   `buzon-xbox\scripts-claude\h340\` (`replay_server.py` repite la grabación `smp2.rec` del SMP). Añade al final de la
   repetición dos paquetes Respawn 1.12.2 (id 0x35: dim -1 y luego dim 0, con su Player Position And Look) para
   imitar lo que manda el SMP al morir. Build de prueba con `-DXBOX_AUTOPILOT=ON` y `D:\autoconnect.txt`
   («host puerto 340») para entrar solo (ver `121-INFORME.md`). Alternativa: Paper 1.12.2 local
   `servidor-1.12.2\iniciar.bat` (puerto 25566) y `kill <jugador>` desde su consola.
2. Encontrar el doble borrado / uso tras liberar y arreglarlo de raíz (también vale para la 1.5.2, que comparte el
   código de respawn multijugador).
3. Compilar con `powershell -File "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\scripts-claude\build125-respawn.ps1"`.
Informe `buzon-xbox\125-INFORME-agy.md`; señal de fin `CIENTOVEINTICINCO-LISTO` + commit.
