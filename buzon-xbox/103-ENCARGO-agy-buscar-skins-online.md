# 103. Encargo agy: buscar y descargar skins por nombre de usuario desde la consola (1.2.5 y 1.5.2) (22-11)

Pedido por el usuario: que el jugador pueda buscar una skin por nombre de usuario (como el comando /skin de los
servidores), descargarla desde la consola, que quede guardada en la carpeta de skins y que aparezca en la rueda de
skins como «Skins personalizadas». Depende de internet (si no hay red: mensaje claro, sin colgarse).

Ramas: 1.5.2 `port152-erluza` (`wt-152-rend`) y 1.2.5 `opti30-erluza` (`wt-opti30`). Sin reset, sin push, sin copiar
al lanzador; commits con `git add <fichero>`; candado `buzon-xbox\BUILD.lock` (lo comparte otro agy). Reglas: 60 fps,
64 MB, dos interfaces (Java y Legacy) para la pantalla nueva, mando completo (teclado en pantalla), pantalla partida.

## Fuente de skins (MEDIDO por Claude, 22-11)
La Xbox no puede hacer HTTPS fácilmente. Estas URL responden por **HTTP plano** con el PNG (200 image/png):
- `http://minotar.net/skin/<usuario>` ← principal (nombre → skin directamente).
- `http://crafatar.com/skins/<uuid-sin-guiones>` y `http://textures.minecraft.net/texture/<hash>` (respaldo).
NO sirven por HTTP (redirigen a https): `api.mojang.com`, `mc-heads.net`, `mineskin.eu`; `s3.amazonaws.com/MinecraftSkins` da 404.
Usa la red que ya tiene el port (multijugador / `ThreadDownloadImage` de Java para skins, DNS de la Xbox).
Tiempo de espera corto (≤ 8 s), descarga en un hilo (sin congelar la pantalla), tamaño máximo (p. ej. 64 KB).

## Qué hacer
1. Pantalla «Buscar skin»: escribir el nombre (teclado en pantalla), vista previa del jugador con esa skin (como el
   muñeco del inventario), botones Guardar / Usar / Cancelar. Errores: «Usuario no encontrado», «Sin conexión».
2. Conversión: las skins modernas son 64×64 (y algunas «slim»/Alex de brazos finos); la 1.2.5 y la 1.5.2 usan 64×32.
   Pasarla a 64×32 (mitad de arriba; si la de abajo trae capas de ropa, ignorarlas o fusionar la capa del cuerpo si es
   fácil) y avisar si era slim. Validar que sea PNG de verdad.
3. Guardar en la carpeta de skins del jugador (la misma que usa la rueda; mira cómo se cargan las skins del lanzador
   `skins/` y las LCE) con el nombre del usuario, y que aparezca en la rueda en un grupo «Skins personalizadas»
   (también las que el jugador copie a mano por FTP). Borrar una skin personalizada desde la rueda.
4. Opcional si sobra: lista de «skins populares» sugeridas (nombres conocidos) para elegir sin escribir.

## Aceptación
En xemu con red: buscar 3 usuarios reales, guardar, verlas en la rueda y en el juego (1ª y 3ª persona), reiniciar y
siguen. Sin red: mensaje, sin cuelgue. 60 fps al volver al juego; `freeKB` antes/después. Capturas en
`buzon-xbox\103-capturas\`. Informe `buzon-xbox\103-INFORME-agy.md`; la señal de fin es la palabra CIENTOTRES-LISTO
seguida de los commits.
