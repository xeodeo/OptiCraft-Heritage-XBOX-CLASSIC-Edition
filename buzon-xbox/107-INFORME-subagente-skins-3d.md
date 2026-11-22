# 107 - Skins como en 1.8 (segunda capa 3D, brazo/pierna izquierdos propios, brazos finos)

Ramas: `port152-skin` (wt-152-skin) y `opti30-skin` (wt-125-skin). Mismo cambio en las dos.

## Qué cambia

- **Modelo nuevo `ModelPlayer`** (`src/net/minecraft/src/ModelPlayer.h/.cpp`), copia del 1.8:
  textura 64x64, cabeza+gorro, cuerpo+chaqueta (16,32), brazo dcho (40,16)+manga (40,32),
  brazo izdo (32,48)+manga (48,48), pierna dcha (0,16)+pantalón (0,32), pierna izda (16,48)+pantalón (0,48).
  Capas exteriores con +0.25 (gorro +0.5). Brazos finos (Alex, 3 px) con pivote y=2.5.
  Cada frame, tras `ModelBiped::setRotationAngles`, copia ángulos **y** pivotes de cada parte a su capa.
  Hereda de `ModelBiped`, así que armadura, objeto en la mano, agacharse, montar, dormir, arco,
  bloqueo con espada y el casco-bloque siguen igual (usan `bipedHead/RightArm->postRender`).
- **`RenderPlayer`**: tiene tres modelos: el clásico `ModelBiped` (64x32), `ModelPlayer` ancho y
  `ModelPlayer` fino. En cada `renderPlayer` elige según la textura que se va a ligar a ESE jugador
  (`getTextureForDownloadableImage(skinUrl, textura)`): si es cuadrada (64x64) usa `ModelPlayer`
  (fino si `SkinManager::isSlimSkinTexture(ruta)`), si no el clásico. Al terminar vuelve al clásico.
  Pantalla partida: cada jugador elige con su propia textura. Mobs (zombis, etc.) no se tocan:
  usan su propio `ModelBiped`, igual que antes. Armadura sigue con sus modelos 64x32.
- **Mano en primera persona** (`ItemRenderer` -> `drawFirstPersonHand(player)`): brazo derecho + manga,
  ancho o fino según la skin. También las dos manos del mapa.
- **Ruleta de skins** (`GuiSkinSelector`) y **búsqueda online** (`GuiSearchSkin`): la vista 3D usa
  `ModelPlayer` cuando la textura es 64x64 (fino/ancho), si no el `ModelBiped` de siempre. La búsqueda
  online sube ya la textura 64x64 convertida (nuevo `SkinManager::convertSkinTo64`), y los mensajes
  ya no dicen "convertida a 64x32".
- **HUD / muñeco del inventario**: dibujan al jugador con `RenderManager`, así que salen con el modelo nuevo
  sin cambios.

## Texturas (SkinManager)

- Skins propias/descargadas: además de `<nombre>.png` (original), `<nombre>_32.png` y `_Front.png`, ahora se
  guarda **`<nombre>_64.png`**, que es la que liga el juego (`SkinEntry::modelPath`). Se crea al descargar,
  al instalar y, si falta, al escanear la carpeta (las skins que ya tenía el usuario se convierten solas en el
  primer arranque, una vez). Se borra junto con la skin. `_32.png` se sigue generando por compatibilidad.
  El escaneo y la lista de "cargar skins" ignoran `_64.png`.
- Skins de serie (Steve, Alex, Tennis..., 21 en total, todas 64x64 en el pak): `modelPath` pasa a ser el
  `.png` 64x64 original (ya no el `_32`). Revisado con un script: todas tienen la base opaca, gorro bien,
  9 finas (todas las "Alex" + ninguna más) y 12 anchas.
- Packs (Halo, Mass Effect, Skyrim): son 64x32, siguen con `_32.png` y el modelo clásico (en 1.8 se verían
  igual: el brazo/pierna izdos son el derecho en espejo, que es lo que ya hace `ModelBiped`).
- Brazos finos: `isLikelySlimSkinRgba` (sin cambios). Para las texturas guardadas el resultado se cachea
  por ruta (`isSlimSkinTexture`); si no está en la caché se decodifica la PNG una vez (16 KB).

## Cómo se convierten las skins viejas 64x32 (`makeModern64`, igual que 1.8 `parseUserSkin`)

1. Imagen nueva 64x64 transparente; se copia la 64x32 en la mitad de arriba.
2. Pierna izda (16,48) y brazo izdo (32,48): cada cara (arriba, abajo, fuera/dentro, delante, detrás) copiada de
   la pierna/brazo derechos **en espejo horizontal**, con las 12 copias exactas del `drawImage` de 1.8
   (dentro y fuera intercambiados).
3. Reglas de alfa de 1.8 (también para las 64x64): opaco (0,0)-(32,16); "truco Notch" (solo se vacía si no hay
   ningún píxel transparente) en (32,0)-(64,32); opaco (0,16)-(64,32); truco Notch en las zonas de capa
   (0,32)-(16,48), (16,32)-(40,48), (40,32)-(56,48), (0,48)-(16,64), (48,48)-(64,64); opaco (16,48)-(48,64).
   En una 64x32 convertida esas zonas de capa quedan vacías (no hay segunda capa), como en 1.8.
4. Extra nuestro: si la skin es fina, se dejan transparentes los texeles que un brazo de 3 px nunca lee, para
   que el `_64.png` guardado se siga detectando como fino al volver a cargarlo.

## Rendimiento / memoria

- Una textura 64x64 por skin (16 KB). Sin reservas por frame (la ruta se copia a un `std::string` miembro que
  reutiliza su buffer; la caché de "fino" es un `unordered_map` que solo crece una vez por ruta).
- Coste extra: 5 cajas más por jugador (las capas) y su display list, compilada la primera vez.
- Primera vez que aparece una skin de serie: se decodifica su PNG una vez para saber si es fina (~1 ms).

## Sin probar (no se ha arrancado xemu ni la consola)

- Todo lo visual. Compila en las 4 builds (152/125, normal/exp).
- Skins descargadas por URL en multijugador (`skinUrl` de otros jugadores): `ImageBufferDownload` las deja en
  64x32, así que siguen con el modelo clásico; no se ha tocado.
- Capas transparentes: dependen del alpha test que ya usa el gorro; si alguna capa sale negra/opaca, mirar el
  estado de alpha test en `RenderLiving`.

## Qué mirar en la consola

1. Steve/Alex de serie: Alex con brazos finos (3 px) en tercera persona, en la mano en primera persona y en la ruleta.
2. Una skin moderna con chaqueta/mangas/pantalón (p. ej. buscar "Dream" o cualquiera con capa): capas en relieve
   en cuerpo, brazos y piernas, y el brazo/pierna izdos con su propio dibujo.
3. Una skin vieja 64x32 (p. ej. "Notch"): sin caja negra en la cara, brazo/pierna izdos iguales que antes.
4. Agacharse, andar, golpear, arco, dormir en cama, ir en vagoneta/cerdo, armadura puesta (que no asome la capa por
   dentro de forma rara), objeto en la mano.
5. Pantalla partida con P1 Alex y P2 Steve (cada uno su modelo), muñeco del HUD y del inventario.
6. Pack Halo/Skyrim: deben verse igual que antes (modelo clásico).
7. Skins propias que ya estaban en el disco: en el primer arranque se crea `<nombre>_64.png` en la carpeta de skins.
