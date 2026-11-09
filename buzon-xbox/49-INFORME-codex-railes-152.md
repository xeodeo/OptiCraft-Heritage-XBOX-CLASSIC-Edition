# Encargo 49 — raíles y vagonetas 1.5.2

## Implementación

| Clase o área | Archivos | Resultado |
|---|---|---|
| API de raíles | `BlockRailBase.h:1`, `BlockRailBase.cpp:1`, `BlockBaseRailLogic.h:1` | API común de detección para rail normal, propulsor, detector y activador; el algoritmo existente de curvas queda reutilizado con el nombre 1.5.2. |
| Raíl propulsor y activador | `BlockRailPowered.h:1`, `BlockRailPowered.cpp:1`, `Block.cpp:1029,1069` | IDs 27 y 157 usan iconos 1.5.2, actualizan su metadato y se autodibujan. |
| Detector | `BlockDetectorRail.h:17`, `BlockDetectorRail.cpp:44` | Expone potencia débil y fuerte como `0` o `15` en la API nueva. |
| Vagonetas | `EntityMinecart.h:51`, `EntityMinecart.cpp:88,356`, `EntityMinecartEmpty.h:3`, `EntityMinecartChest.h:3`, `EntityMinecartFurnace.h:3`, `EntityMinecartTNT.h:3`, `EntityMinecartVariants.cpp:7` | Fábrica de los tipos vacío, cofre, horno y TNT; TNT se enciende en activador y explota al agotarse la mecha. |
| Guardados y render | `EntityList.cpp:90,224`, `RenderMinecart.cpp:102` | Guarda con las entidades 1.5.2 y dibuja el bloque TNT en el carro. |
| Ítem, recetas y creativo | `Item.cpp:641`, `ItemMinecart.cpp:15`, `CraftingManager.cpp:102,110`, `ContainerCreative.cpp:105` | Ítem 407 con `minecartTnt.png`, colocación mediante la fábrica y recetas de activador/TNT. |

## Compatibilidad de mundos antiguos

Al leer una entidad con `id = "Minecart"`, `EntityList::createEntityFromNBT` consulta su NBT `Type` y llama a `EntityMinecart::createMinecart`. Así los carros 1.2.5 existentes se materializan como vacío (0), cofre (1), horno (2) o TNT (3) antes de ejecutar `readFromNBT`. Las entidades nuevas se registran con los nombres e IDs 1.5.2: `MinecartRideable`/42, `MinecartChest`/43, `MinecartFurnace`/44 y `MinecartTNT`/45.

## Compilación

Se ejecutaron, bajo `BUILD.lock`, ambos perfiles configurados:

```text
cmake --build build/xbox-release --target OptiCraft --parallel 2
cmake --build build/xbox-release-exp --target OptiCraft --parallel 2
```

Los dos finalizaron correctamente y dejaron `bin/xbox/OptiCraft.exe` y `bin/xbox-exp/OptiCraft.exe`. No se usó xemu, según el encargo.

## Diff

El commit contiene las clases nuevas de raíles y vagonetas y cambios en 14 archivos existentes; el `git diff --stat` final queda registrado en el commit.
