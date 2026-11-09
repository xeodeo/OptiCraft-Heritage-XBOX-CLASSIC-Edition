# REGLA: una sola compilación a la vez (pedido del usuario, 28-09)

Antes de compilar o de usar xemu, CUALQUIER agente (Claude, agy, OpenCode) debe:
1. Mirar si existe `C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock`.
   - Si existe, **espera** (revisa cada 30 s). No compiles ni abras xemu.
2. Si no existe, créalo con una línea: `<quién> <qué> <hora>`. Ejemplo: `opencode unity-build 15:10`.
3. Compila y prueba en xemu.
4. **Borra `BUILD.lock` al terminar**, también si la compilación falla.

PowerShell:
```powershell
$L = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock"
while (Test-Path $L) { Start-Sleep 30 }
"opencode unity-build $(Get-Date -Format HH:mm)" | Set-Content $L
try { <compilar y probar> } finally { Remove-Item $L -ErrorAction SilentlyContinue }
```
