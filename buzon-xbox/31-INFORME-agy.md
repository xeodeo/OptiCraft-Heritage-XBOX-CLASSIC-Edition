
# INFORME 31 agy

## Fallo 1: Círculo soltaba el objeto
Añadí la lógica de `s_suppressedButtons` para enmascarar los botones a la salida de los menús (imitando la lógica que el Jugador 1 tiene en Xbox y la que añadí a PS2).
- **`src/ps2/input/Ps2InputMapper.cpp`** (~línea 66): Modifiqué `Ps2InputMapper::update` guardando la variable estática en `s_suppressedButtons` al salir de un menú para el Jugador 1, permitiendo que `pad.held` no dispare `pad.pressed` ni `pad.released` hasta que el usuario suelte físicamente el botón.
- **`src/client/Ps2SplitScreen.cpp`** (~línea 250): Hice lo propio para la pantalla dividida del Jugador 2 en PS2, ya que este port administra sus propias interrupciones de menú para J2. Al salir de su inventario, el estado se captura y se limpia.

## Fallo 2: Pantalla de Controles de PS2
La razón técnica por la que OptiJuegos desactivó completamente la pantalla interactiva (`LegacyControlsScreen.cpp`) con bloques `#if !PLATFORM_PS2` es porque desde el menú de opciones para PS2 abría incondicionalmente un panel estático llamado `LegacyControllerLayoutScreen`, diseñado en torno a una imagen de fondo de un DualShock.

Para cumplir tu requisito y hacer que PS2 comparta el formato a una sola columna de Xbox:
- **`src/net/minecraft/src/legacy/LegacyHelpOptions.cpp`** (~línea 76): Eliminé el desvío a `LegacyControllerLayoutScreen`. Ahora, al hacer clic en "Controles" en PS2 se llama directamente a `LegacyControlsScreen`, igual que en Xbox.
- **`src/net/minecraft/src/legacy/LegacyControlsScreen.cpp`**:
  - ~Línea 84 (`initGui`): Cambié a `#if PLATFORM_XBOX || PLATFORM_PS2` para usar el diseño de 1 columna con 8 filas.
  - ~Línea 291: Eliminé el `#if !PLATFORM_PS2` que inhabilitaba la asignación de botones y la navegación en la parte inferior de la lista (Prev/Next/Reset).
  - ~Líneas 193/249 (`refreshRowLabels` y `rebuildPage`): Incluí a la PS2 en la habilitación del array de `rowIcons` de la Xbox, que reemplaza el nombre del botón con un icono. Cambié la extracción literal a `#else ps2PadKeyName(...)`. La función `LegacyButtonPrompt::iconFromName` que le seguía ya tenía el soporte del DualShock implementado, devolviendo `Ps2ButtonIcon::Circle`, `Cross`, etc.
  - ~Línea 375: Oculté el dibujo en el fondo del teclado (`LAYOUT_ART`) que salía cortado al habilitar una sola columna en la PS2.

## Git Diff Stat
```text
 src/client/Ps2SplitScreen.cpp                      | 29 ++++++++++++++++++++--
 .../minecraft/src/legacy/LegacyControlsScreen.cpp  | 18 +++++++-------
 src/net/minecraft/src/legacy/LegacyHelpOptions.cpp |  4 ---
 src/ps2/input/Ps2InputMapper.cpp                   | 25 ++++++++++++++++---
 4 files changed, 61 insertions(+), 15 deletions(-)
```

## Compilaciones Reales
**PS2 (ps2-release):**
```text
[4/4] Linking CXX executable bin\ps2-release\OptiCraft.elf
C:/msys64/usr/bin/cmake.exe -E cmake_link_script CMakeFiles/OptiCraft.dir/link.txt --color=
C:\Users\xeodeo\Desktop\MINECR~1\WT-OPT~1\PSDEVW~1\ee\bin\mips64r5900el-ps2-elf-g++.exe -mno-crt0 -Wl,-zmax-page-size=128 -T "C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/wt-opti30/build/ps2-release/ps2_linkfile_gc.ld" CMakeFiles/OptiCraft.dir/src/EntryPoint.cpp.obj CMakeFiles/OptiCraft.dir/src/client/Minecraft.cpp.obj <TRUNCATED_OBJ_FILES> -o bin/ps2-release/OptiCraft.elf  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ports/lib/libvorbisfile.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ports/lib/libvorbis.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ports/lib/libogg.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ports/lib/libz.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/gsKit/lib/libgskit_toolkit.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/gsKit/lib/libgskit.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/gsKit/lib/libdmakit.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libpad.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libmc.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libcdvd.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libpacket.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libgraph.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libdraw.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libmath3d.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libnet.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libpatches.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libfileXio.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libpoweroff.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libhdd.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libkernel.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libdebug.a  C:/Users/xeodeo/Desktop/MINECR~1/WT-OPT~1/PSDEVW~1/ps2sdk/ee/lib/libc.a  -lstdc++  -lm
C:/msys64/usr/bin/cmake.exe -E true
```

**Xbox (xbox-release):**
```text
[5/6] Linking CXX executable bin\xbox-release\OptiCraft.exe
[6/6] Generating bin/xbox-release/default.xbe, bin/xbox-release/OptiCraft.xdb
Microsoft (R) Xbox Image Builder, Version 1.0.5849.0
Copyright (C) Microsoft Corporation 1981-2003. All rights reserved.

Xbox executable image   : c:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30\build\xbox-release\bin\xbox-release\default.xbe
Xbox debug info         : c:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30\build\xbox-release\bin\xbox-release\OptiCraft.xdb
Xbox allowed media types:
     CDROM, DVD_X2, DVD_CD, CD, DVD_5_RO, DVD_9_RO, DVD_5_RW, DVD_9_RW, DONGLE, NONSECURE_HARD_DISK, HARD_DISK, RAM_DRIVE
Xbox sections:
     .text .rdata .data .rsrc
```


## Ampliación (Encargo 31, parte 2)

### A. Mapeo de botones de PS2
Igualé incondicionalmente el mapeo de botones del mando de PS2 a la distribución equivalente a la Xbox, configurándolo por defecto en `GameSettingsBackend_PS2.cpp`. Esto asegura que sea la base tanto al crear un archivo nuevo como al migrar el viejo.

**Tabla de mapeo de PS2 (Antes vs Ahora):**
| Acción en el juego | Botón anterior en PS2 (Dependía de legacyCrafting) | Nuevo botón en PS2 (Igual a Xbox) |
|---|---|---|
| Moverse / Cámara | stick izq / dcho | stick izq / dcho |
| Saltar | Cruz | Cruz (equivalente a A) |
| Soltar objeto | Triángulo o Círculo | Círculo (equivalente a B) |
| Inventario (el normal) | Cuadrado o Triángulo | Triángulo (equivalente a Y) |
| Nuestro crafteo | (Hardcodeado a Triángulo o Cuadrado) | Cuadrado (equivalente a X) |
| Atacar / picar | R2 | R2 (equivalente a RT) |
| Usar / colocar | L2 | L2 (equivalente a LT) |
| Objeto ant. / sig. | L1 / R1 | L1 / R1 (equivalente a Blanco/Negro) |
| Agacharse | L3 | L3 (clic izq) |
| Cambiar cámara (F5) | R3 | R3 (clic dcho) |
| Pausa | START | START |
| F3 / Debug | SELECT | SELECT (equivalente a BACK en Xbox) |

*(Nota: F3 en Xbox se dispara usando el botón Back y en PS2 ya se disparaba usando Select gracias a la correspondencia existente en `Ps2InputMapper.cpp`)*

### B. Opciones de crafteo unificadas ("Console Crafting")
1. **Ocultamiento de opciones Legacy:** En las pantallas `GuiBetaOptions.cpp`, `GuiOptiCraftOptions.cpp` y `LegacyHeritageOptions.cpp`, la casilla "Legacy Crafting" original (el menú de crafteo modificado de PC introducido por OptiJuegos) fue oculta exclusivamente para Xbox y PS2.
2. **Casilla unificada:** En su lugar (y en el espacio de Xbox-Style Crafting en el menú Heritage), ahora aparece una única casilla llamada "Console Crafting". Se sincroniza con la variable `xboxStyleCrafting` en `options.txt` sin romper compatibilidad.
3. **Bloqueo del menú heredado de PC:** Edité `Minecraft.cpp`, `EntityPlayerSP.cpp` y `GuiInventory.cpp` para remover las llamadas incondicionales a `LegacyCraftingScreen` (el menú heredado) si la plataforma es PS2 o Xbox, forzando a que las consolas abran `XboxCraftingScreen` si `xboxStyleCrafting` está activado, o el modesto `GuiCrafting` nativo de Minecraft Vanilla si está desactivado. El menú exclusivo de PC quedó completamente aislado de las consolas.

Ambas plataformas compilaron satisfactoriamente tras los cambios.


### Compilación final y confirmación de cambios
He recompilado desde cero "de verdad" (con el candado de `BUILD.lock` puesto) limpiando las builds intermedias y usando la configuración nativa:
- **Xbox**: `xbox-release` configurado con `XBOX_XDK_ROOT` en PowerShell, generando correctamente su binario.
- **PS2**: `ps2-release` configurado en PowerShell, construyendo directamente a su ruta original.

Los archivos generados tienen las firmas de tiempo esperadas tras aplicar todos los puntos:
```text
Name          LastWriteTime           
----          -------------           
OptiCraft.elf 30/09/2026 2:38:12 p. m.
default.xbe   30/09/2026 2:37:28 p. m.
```

**Salida PS2 (Últimas líneas):**
```
[1226/1227] Linking CXX executable "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30\bin\ps2\OptiCraft.elf"
-- PS2 ELF linked: 5801276 file bytes, 5800200 LOAD bytes; report: C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/wt-opti30/build/ps2-release/OptiCraft.linked-size.txt
label=linked
file_bytes=5801276
load_file_bytes=5800200
load_memory_bytes=6621100
text_bytes=4828832
rodata_bytes=294432
data_bytes=19516
bss_bytes=820780
eh_frame_bytes=518984
gcc_except_table_bytes=132009

-- PS2 ELF packaged: 5801208 file bytes, 5800200 LOAD bytes; report: C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/wt-opti30/build/ps2-release/OptiCraft.size.txt
label=packaged
file_bytes=5801208
load_file_bytes=5800200
```

**Salida Xbox (Últimas líneas):**
```
[5/6] Linking CXX executable bin\xbox-release\OptiCraft.exe
[6/6] Generating bin/xbox-release/default.xbe, bin/xbox-release/OptiCraft.xdb
Microsoft (R) Xbox Image Builder, Version 1.0.5849.0
Copyright (C) Microsoft Corporation 1981-2003. All rights reserved.

Xbox executable image   : c:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30\build\xbox-release\bin\xbox-release\default.xbe
Xbox debug info         : c:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30\build\xbox-release\bin\xbox-release\OptiCraft.xdb
Xbox allowed media types:
     CDROM, DVD_X2, DVD_CD, CD, DVD_5_RO, DVD_9_RO, DVD_5_RW, DVD_9_RW, DONGLE, NONSECURE_HARD_DISK, HARD_DISK, RAM_DRIVE
Xbox sections:
     .text .rdata .data .rsrc
sucessfully created OptiCraft.iso (807 files totalling 65287822 bytes added)
```

