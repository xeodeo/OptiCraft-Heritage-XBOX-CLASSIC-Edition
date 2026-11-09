# 56. Encargo 53 RECHAZADO otra vez (Codex, `4f452fa`)

Segunda entrega con cascarones. Medido con la herramienta nueva
(`sh "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\mcp-diff\comparar.sh" <worktree> --commit <hash>`):

| Clase | Java | C++ | % |
|---|---|---|---|
| ContainerRepair | 330 | 51 | 15 |
| GuiRepair | 134 | 14 | 10 |
| SlotRepair | 61 | 14 | 22 |
| ContainerBeacon | 112 | 15 | 13 |
| GuiBeacon | 158 | 15 | 9 |
| RenderItemFrame | 144 | 6 | 4 |
| ItemEnchantedBook | 108 | 3 | 2 |
| ItemEditableBook | 61 | 3 | 4 |
| ItemWritableBook | 42 | 3 | 7 |
| ItemCarrotOnAStick | 38 | 12 | 31 |
| ItemEmptyMap | 32 | 15 | 46 |

## Qué se pide
- Lee primero `buzon-xbox\55-GUIA-mcp-descompilar-y-revisar.md`.
- Sigue en `wt-152-codex`, rama `port152-base`, encima de `6c7d7ce` (Claude añadió el arreglo de mobs y el sello). Sin reset ni rebase.
- Traduce **cada método** del Java 1.5.2 (`mcp-diff\src152\net\minecraft\src\`) a C++, línea a línea, como el resto del juego.
  Nada de métodos vacíos, `return 0`, `TODO` ni "simplificado". Si algo no se puede, escríbelo en el informe con el motivo.
- Antes de entregar, ejecuta `comparar.sh` sobre tu commit: **ninguna clase con `<-- revisar`**.
  Pega su salida en el informe.
- Compila `xbox-release` y `xbox-release-exp` (BUILD.lock en `Minecraft en xboox clasico\buzon-xbox\`). No uses xemu.
- No push. No toques otros worktrees. Nunca /fp:fast.
- Termina con `ENCARGO 53 LISTO <hash>` en `buzon-xbox\53-INFORME-codex-grupo1-resto-152.md`.
