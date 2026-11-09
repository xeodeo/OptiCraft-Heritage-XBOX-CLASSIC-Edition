# Encargo 63 (agy, 2ª terminal): terminar los aldeanos de la 1.5.2

Codex empezó el encargo 62 y se quedó sin cuota a mitad. Tú lo terminas.
Lee ENTERO `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\62-ENCARGO-codex-aldeanos-152.md`
(qué hay que portar y la sección TRASPASO del final) y la guía
`C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`.

## Tu sitio
- Worktree `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-pociones`, rama `port152-pociones`.
  Parte del commit `af44b2e` (lo de Codex, SIN compilar). No hagas reset.
- Hay OTRO agy trabajando a la vez en `wt-152-agy` (menú creativo y cofre trampa). No toques ese worktree,
  ni `wt-152-codex`, ni `GuiContainerCreative`/`ContainerCreative`/`CreativeTabs`/`BlockChest`/`TileEntityChest`.
- Java 1.5.2: `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\src152\net\minecraft\src\`.

## Qué hacer
1. Lee el diff de `af44b2e` entero y compáralo con el Java: termina lo que esté a medias (`EntityVillager`,
   `ContainerMerchant`, `GuiMerchant`, `GuiButtonMerchant`...). Corrige lo que no sea fiel.
2. Lo que falta del 62: menú de comercio con MANDO y en pantalla partida, aldeas del desierto,
   `EntityAIVillagerMate`, `VillageSiege`.
3. Compila `xbox-release` y `xbox-release-exp` (candado `buzon-xbox\BUILD.lock`; el otro agy también compila: espera tu turno).
   Este worktree ya está configurado.
4. `sh "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\comparar.sh" <wt> --commit <hash>` sin `<-- revisar`
   (y sin relleno: Claude lee los métodos principales).

## Al acabar
Informe `buzon-xbox\63-INFORME-agy2-aldeanos-152.md`, commit, y en tu terminal: `ENCARGO 63 LISTO <commit>`.
