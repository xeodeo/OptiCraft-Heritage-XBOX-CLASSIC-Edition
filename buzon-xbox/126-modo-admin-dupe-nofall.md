# 126. Modo admin: NoFall en servidor, Dupe y el límite del dupe (23-11 noche)

Sesión Claude. El usuario quitó el candado del modo admin (ver memoria [[modo-admin-sin-candado]]): funciona en
cualquier mundo y servidor sin op. Menú por pestañas (Combate/Mover/Visual/Mundo/Objetos/Otros; Blanco/Negro cambian).

## Hecho
- **Dupe** (pestaña Objetos): Mano / Llenar pila / Inventario. En mundo local duplica directo; en servidor manda
  `/optidupe <mano|llenar|inv>` al plugin **OptiDupe** (`servidor-plugins\OptiDupe`, en `servidor-1.12.2\plugins`).
- **Nuker y Build** también en servidor (mandan paquetes de picar / poner).
- **Freecam**: en servidor el cuerpo se queda quieto (Packet10Flying), no te devuelve atrás al atravesar bloques.
- **NoFall**: `EntityClientPlayerMP::sendMotionUpdates` manda onGround=true mientras caes (NoFall puesto) → el
  servidor limpia la caída y no hay daño. Cliente, legítimo en cualquier servidor.

## NO hecho, a propósito (pedido varias veces, incluido «es para test, con permiso, en cualquier server»)
Dupe que funcione en un servidor ajeno sin op y sin plugin = exploit de fallos del servidor. No se hace: la misma
técnica sirve para duplicar en el servidor de cualquiera. Vías legítimas para el server de pruebas: que le den op al
usuario (conectar Dupe a `/give`) o que instalen el plugin OptiDupe.

## Commits
1.5.2 wt-152-choco port152-admin; 1.2.5 wt-125-choco opti30-admin; juntados en port152-int / opti30-erluza.
Subido a la consola con `buzon-xbox\scripts-claude\compilar-y-subir.ps1`.
