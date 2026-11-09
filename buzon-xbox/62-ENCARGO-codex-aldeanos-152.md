# Encargo 62 (Codex): aldeanos de la 1.5.2 — comercio y aldeas del desierto

Claude ACEPTÓ el 61 (`7f52b98`, en la rama buena como `d7f926e`). Buen trabajo y buen informe.

## Tu sitio
- `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-pociones`, rama `port152-pociones`.
  Al empezar, con el árbol limpio: `git reset --hard port152-claude`.
- NO toques `wt-152-codex` ni `wt-152-agy`. agy hace yunque/faro/marco/libros y luego menú creativo/cofre trampa:
  no toques esas clases. Si necesitas una pestaña creativa para algo nuevo, déjalo anotado en el informe.
- Guía: `buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`. Lista general: `buzon-xbox\60-LO-QUE-FALTA-152.md`.
- Java 1.5.2: `mcp-diff\src152\net\minecraft\src\`.

## Qué portar (traducción fiel, método a método)
1. **Comercio**: `IMerchant`, `NpcMerchant`, `MerchantRecipe`, `MerchantRecipeList` (NBT), `InventoryMerchant`,
   `ContainerMerchant`, `SlotMerchantResult`, `GuiMerchant` + `GuiButtonMerchant`, `EntityAITradePlayer`,
   `EntityAILookAtTradePlayer`, y `EntityVillager` 1.5.2 completo (ofertas por profesión con esmeraldas,
   `addDefaultEquipmentAndRecipies`, desbloqueo de ofertas nuevas, reputación, `Offers` en NBT, partículas,
   no comerciar de noche/heridos, bebés).
   El menú con el MANDO como los demás (`src/net/minecraft/src/legacy/`, `GuiContainer` con cursor) y en pantalla
   partida (`displayPlayerScreen`). Textura `gui/trading.png` (comprueba que esté en el pak; si no, añádela al
   script `scripts/port152/build_textures_152.py` y dilo).
2. **Aldeas**: `Village` (reputación por jugador, `noBreedTicks`), `VillageCollection` como `WorldSavedData`
   (`villages.dat`), `EntityAIVillagerMate` (profesión al azar del bebé), `VillageSiege` (`initCreature`).
3. **Aldeas del desierto**: `ComponentVillageStartPiece` / `StructureVillagePieces` / `ComponentVillage*` con
   `getBiomeSpecificBlock` (arenisca) y los aldeanos de cada casa (bibliotecario, sacerdote, carnicero, herrero con
   su cofre). Que el terreno de una semilla no cambie fuera de las aldeas.

## Rendimiento
La consola tiene 64 MB y la 1.5.2 ya va con ~9 MB libres. Mide y anota cuánto ocupa una lista de ofertas y cuánto
cuesta el tick del aldeano. Nada de búsquedas por tick que el Java no haga.

## Reglas
Candado `buzon-xbox\BUILD.lock`, compilar `xbox-release` y `xbox-release-exp`, nunca /fp:fast, no push, no
ClassicXboxLoader, no xemu. Nada de métodos de relleno.

## Al acabar
Informe `buzon-xbox\62-INFORME-codex-aldeanos-152.md` y en tu terminal: `ENCARGO 62 LISTO <commit>`.

## TRASPASO (Claude, 26-10): Codex se quedó sin cuota a mitad
- Lo hecho está guardado en `wt-152-pociones`, commit `af44b2e` (WIP, SIN compilar). 31 archivos, +936 líneas.
- Hecho a medias: `EntityVillager` 74 %, `MerchantRecipe(List)`, `InventoryMerchant`, `ContainerMerchant` 54 %,
  `GuiMerchant` 51 %, `GuiButtonMerchant` 31 %, `SlotMerchantResult`, `EntityAITradePlayer`, `Village`,
  `VillageCollection`, `MapDataBase`, `EntityPlayer(SP)` (displayGUIMerchant).
- Falta: terminar menú y botones del comercio (mando + pantalla partida), aldeas del desierto
  (`ComponentVillage*` con `getBiomeSpecificBlock`), `EntityAIVillagerMate`, `VillageSiege`, compilar y el informe.
- Quien siga: parte de `af44b2e`, lee el diff entero antes de seguir, nada de relleno.

## TRASPASO 2 (Claude, 26-10 tarde): agy2 (encargo 63) también se quedó sin cuota
- Guardado en `wt-152-pociones`, commit `c746c8f` encima de `af44b2e` (WIP, SIN compilar).
