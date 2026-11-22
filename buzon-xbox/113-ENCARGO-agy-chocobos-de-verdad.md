# 113. Encargo agy: chocobos «de verdad» (modelo y texturas de ChocoCraft) + arreglos de montar (1.2.5 y 1.5.2) (22-11)

Visto por el usuario en la consola real (22-11 18:20) con nuestro ChocoCraft hecho a mano (encargo 101):
1. **La cría (bebé): la cabeza no queda bien pegada al cuerpo.**
2. **No parece un chocobo de verdad**: modelo y textura hechos por código. Quiere los de un ChocoCraft real.
3. **El dorado no vuela.**
4. **Montado, se mueve solo demasiado** (sigue con su IA de deambular) y **da igual si tiene silla o no**.
5. **Al planear y aterrizar, el jugador recibe daño por caída**: montado/planeando NO debe hacer daño.
El usuario esperaba que se descompilaran los mods y se portaran.

## Fuente (buscada por Claude, 22-11)
No queda ningún ChocoCraft descargable para 1.2.5 ni 1.5.2 (lo más viejo es 1.7.10, «All Rights Reserved»). Se usa
**ChocoCraft4** (continuación del ChocoCraft de EddieV → ArnoSaxena → clienthax → Slayer5934): código **MIT**, recursos
**CC-BY-SA 4.0** (hay que citarlo). Bajado en `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\chococraft4-ref\`:
- Texturas: `common/src/main/resources/assets/chococraft/textures/entities/chocobos/*.png` (10 colores: amarillo,
  verde, azul, blanco, negro, dorado, rojo, rosa, morado, fuego) y `.../chicobos/*.png` (crías); sillas/alforjas,
  objetos (gysahl, plumas…) en `assets/chococraft/textures/`.
- Modelo: `common/src/main/java/net/chococraft/client/models/entities/AdultChocoboModel.java` (y el de la cría);
  comportamiento (montar, saltar, planear, volar, sillas, cría) en `net/chococraft/common/entity/`.
Añade en `docs/` (o en el README de mods) la línea de créditos y licencia CC-BY-SA de las texturas.

## Qué hacer (ramas `port152-int` y `opti30-erluza`, carpeta que te diga Claude)
- Sustituir modelo y texturas por los de ChocoCraft4 (convertir el modelo de 1.16+ a nuestro `ModelRenderer` de
  1.2.5/1.5.2: mismas cajas, pivotes y animaciones de patas/alas/cabeza; cría con su modelo propio y la cabeza bien
  unida). Texturas al pak de las dos versiones (y al conversor si hace falta).
- Colores y habilidades como ChocoCraft4 (al menos los 6 que ya hay; los 4 nuevos si es razonable): el **dorado
  vuela** (subir/bajar con el mando como el vuelo de creativo, consumo/límite como el original si lo tiene), verde salta
  alto, azul por agua, etc.
- **Montar**: solo con **silla puesta**; al montar, la IA del chocobo se apaga (no deambula ni sigue) y obedece solo
  al jugador; al desmontar vuelve a su modo.
- **Sin daño por caída** para el jugador mientras va montado/planeando ni al aterrizar (ni para el chocobo dorado en
  vuelo); planear con las alas al caer como el original.
- Todo con mando (dos interfaces para el menú, ya restilizado a LCE por Claude/subagente: no rehagas el aspecto del
  menú, solo su contenido si hace falta), pantalla partida, 60 fps, memoria (texturas 64×64/128×64 como mucho).

## Aceptación
Capturas: los 6–10 colores adultos, una cría (cabeza en su sitio), montado con silla, intento sin silla (no monta),
dorado volando, aterrizaje sin daño (vida antes/después). Guardar/cargar. Capturas en `buzon-xbox\113-capturas\`.
Informe `buzon-xbox\113-INFORME-agy.md`; la señal de fin es la palabra CIENTOTRECE-LISTO seguida de los commits.
