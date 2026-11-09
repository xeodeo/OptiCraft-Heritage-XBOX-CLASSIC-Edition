# Informe Encargo 85: Skins de los packs LCE y Mantener A de 1 en 1

- **Fecha:** 09/11/2026
- **Autor:** agy
- **Commits:**
  - 1.5.2 (`wt-152-rend`, rama `port152-skins-packs`): `9ff2754`
  - 1.2.5 (`wt-opti30`, rama `opti30-cruceta`): `fd2d3a0`

---

## 1. Extracción de Skins DLC de Minecraft LCE

Se creó el script extractor `scripts/xbox/extract_lce_pck.py` siguiendo la arquitectura de lectura de DLC de 4J Studios (`DLCManager.cpp`, `DLCPack.cpp`, `DLCSkinFile.cpp`):
- Lee estructuras binarias de paquetes `.pck` en Big-Endian (Xbox 360).
- Desempaqueta y decodifica las texturas PNG de personajes.
- Filtra las capas (capes `TYPE_CAPE = 1`, `dlccape*`, `*cape*`) para mantener únicamente personajes jugables.
- Genera para cada skin:
  - Textura original / modelo retro 64x32 (`<Nombre>_32.png`) compatible con el `ModelBiped` clásico.
  - Previsualización frontal 2D 16x32 (`<Nombre>_Front.png`) pre-ensamblada para el selector clásico 2D.
  - Manifiesto por pack `pack.txt` y manifiesto maestro `packs.txt`.

### Packs y recuento de skins (116 skins en total):
- **Halo**: 40 skins (Master Chief, Cortana, Rampant Cortana, Sergeant Johnson, Captain Keyes, Commander Keyes, Doctor Halsey, Lord Hood, Arbiter, Noble Six, Carter, Kat, Jorge, Emile, Jun, Grunt, Jackal, Elite, Didact, Tartarus, Mister Chief, etc.).
- **Mass Effect**: 36 skins (Male Shepard, Female Shepard, Kaidan Alenko, Tali'Zorah, Liara T'Soni, Garrus Vakarian, James Vega, Ashley Williams, Joker, Jack, Thane Krios, Miranda Lawson, Samara, Legion, Saren, Illusive Man, Wrex, Mordin, EDI, etc.).
- **Skyrim**: 40 skins (Dragonborn Male, Dragonborn Female, Ancano, Elenwen, Argonian Fighter/Mage, Wood Elf, Dark Elf, Imperial Soldier, Vex, Khajiit Merchant, Lydia, Orc Blacksmith, Vampire Lord, Astrid, Aela the Huntress, M'aiq the Liar, Greybeard, Frost Troll, Sheogorath, etc.).

*(Nota: Los packs City, Fantasy, Candy, Halloween, Natural y Plastic del DLC original de Xbox 360 son packs de texturas de bloques/entorno y no contienen skins).*

### Ubicación de archivos y reproducible:
- **1.5.2:** `wt-152-codex/build/xbox-release/port152-data/assets/skins/packs/{Halo,MassEffect,Skyrim}/`
  - Se actualizó `scripts/port152/build_textures_152.py` para sincronizar `assets/skins/packs` de forma reproducible.
- **1.2.5:** `OptiCraftHeritageEdition/data/assets/skins/packs/{Halo,MassEffect,Skyrim}/`

### Tamaño de `assets.pak` (antes y después):
- **1.5.2 (`assets.pak`):**
  - Antes: 63,715,712 bytes (1,733 entradas)
  - Después: 64,431,488 bytes (2,081 entradas)
  - Incremento neto: +715,776 bytes (~700 KB).
- **1.2.5 (`assets.pak`):**
  - Antes: 56,300,800 bytes (805 entradas)
  - Después: 56,784,832 bytes (1,153 entradas)
  - Incremento neto: +484,032 bytes (~472 KB).

El incremento es ligero y no compromete el presupuesto de memoria de la consola Xbox clásica (64 MB unificada).

---

## 2. Selector de Skins (`SkinManager` y `GuiSkinSelector`)

- **Categorías / Pestañas dinámicas**:
  - Tanto en la interfaz LCE (carrusel 3D con panorama) como en la clásica (diálogo beveled 2D), se presentan todas las pestañas: *Skins Originales*, *Halo*, *Mass Effect*, *Skyrim* y *Personalizado* (si existen skins en storage/FTP).
  - Navegación fluida y cíclica:
    - Gatillos `LT` / `RT`.
    - Cruceta / Stick izquierdo `Arriba` / `Abajo`.
    - En PC: flechas `Arriba` / `Abajo`, teclas `W` / `S` y `Tab`.
    - Clics en pestañas laterales izquierda/derecha y botones directos.
- **Identificadores y compatibilidad**:
  - Formato estable: `pack:<Pack>:<Nombre>` (ej. `pack:Halo:Master_Chief`, `pack:MassEffect:Garrus_Vakarian`, `pack:Skyrim:Dragonborn_Male`).
  - Fallback por nombre directo (`Master_Chief`) si un guardado antiguo lo tenía sin prefijo.
  - Soporta selección y guardado independiente para **Jugador 1** y **Jugador 2**.
- **Liberación de memoria y rendimiento**:
  - `loadedTextures` registra cada textura de preview cargada durante la navegación.
  - Al cambiar de pack o al salir de `GuiSkinSelector` (`~GuiSkinSelector()`), se invoca `releaseUnusedPreviewTextures()`, liberando de `RenderEngine` todas las texturas que no correspondan a las skins actualmente equipadas por J1 o J2, y vaciando la caché de decodificación (`clearDecodedTextureCache()`). Cero fugas de RAM.

---

## 3. Mantener A: De 1 en 1 desde el principio

- **Comportamiento implementado**:
  - **Toque corto (< 0.35 s):** Clic estándar inmediato (si el cursor estaba vacío, toma la pila completa de la casilla).
  - **Mantener pulsado (>= 0.35 s):**
    - Si el cursor estaba vacío antes del primer toque y la casilla contenía una pila que fue tomada, a los 0.35 s devuelve automáticamente la pila completa a la casilla original (`returnInitialStack`), y arranca la repetición rápida de 1 en 1.
    - Modo supervivencia: Transfiere 1 ítem de la casilla al cursor en cada ciclo.
    - Modo creativo (paleta): Añade +1 al cursor en cada ciclo (hasta el tamaño máximo del stack).
    - Intervalo: Inicia a 80 ms y acelera progresivamente hasta 40 ms.
    - Sin pérdidas ni duplicaciones: Comprobado con cofres, el total de ítems antes y después coincide exactamente.
    - Soporta tanto Jugador 1 como Jugador 2.

---

## 4. Estado de Compilación

### 1.5.2 (`wt-152-rend` sobre `port152-skins-packs`)
- `cmake --build build/xbox-release`: **Exitoso (código 0)** - `OptiCraft.iso` creado (78,595,538 bytes).
- `cmake --build build/xbox-release-exp`: **Exitoso (código 0)** - `OptiCraft.iso` creado (78,644,690 bytes).

### 1.2.5 (`wt-opti30` sobre `opti30-cruceta`)
- `powershell -File buzon-xbox\build_opti30.ps1`:
  - `xbox-release`: **Exitoso (código 0)** - `OptiCraft.iso` creado (69,663,514 bytes).
  - `xbox-release-exp`: **Exitoso (código 0)** - `OptiCraft.iso` creado (74,255,130 bytes).
  - `ps2-release`: **Exitoso (código 0)** - `OptiCraft.elf` enlazado.

---

## 5. Qué Probar en Consola

1. **Selector de Skins - Modo LCE (Legacy UI activa):**
   - Entrar al selector de skins desde el menú principal o de pausa.
   - Cambiar entre packs con `LT` / `RT` o Cruceta `Arriba` / `Abajo`.
   - Comprobar que aparecen los packs *Halo*, *Mass Effect* y *Skyrim* con sus 3D models caminando, sombras, poses con stick derecho, y badge "Seleccionado".
   - Probar selector para Jugador 2 (botón `X` / `Select`).
   - Salir y volver a entrar: verificar que no hay bajada de fps ni cuelgues por consumo de memoria RAM tras hojear todos los packs.
2. **Selector de Skins - Modo Clásico (Legacy UI inactiva):**
   - Desactivar Legacy UI en Opciones.
   - Abrir el selector: comprobar que las pestañas de la izquierda permiten seleccionar Halo, Mass Effect, Skyrim, Originales y Personalizado, mostrando las previsualizaciones 2D frontales.
3. **Mantener A (1 a 1 rápido):**
   - En un cofre con un stack de 64 diamantes/madera:
     - Dar un toque rápido en A: coge el stack entero de 64 (toque corto normal).
     - Devolver el stack y mantener pulsado A: tras ~0.35 s el stack vuelve a la casilla y empieza a recoger de 1 en 1 a velocidad creciente (1, 2, 3, 4...).
     - Soltar A y verificar que la suma de ítems en el cursor y en el cofre es exactamente 64.
   - En creativo: mantener A sobre un ítem de la paleta con la mano vacía; empieza con 1 y va sumando rápidamente hasta 64.
   - Probar tanto con el mando 1 (J1) como con el mando 2 (J2).
