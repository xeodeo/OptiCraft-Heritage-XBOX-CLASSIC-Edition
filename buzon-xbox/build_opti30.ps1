# Compila las TRES versiones de la rama buena (wt-opti30): Xbox normal, Xbox
# experimental y PS2. Respeta el candado BUILD.lock y lo borra al terminar.
#
#   powershell -File "buzon-xbox\build_opti30.ps1"
#
# Las carpetas de build ya están configuradas con MC_LOG_LEVEL=1 y la IP del
# log. Si creas una carpeta NUEVA, hay que configurarla antes (ver abajo).
$lock = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock"
if (Test-Path $lock) { "BLOQUEADO: " + (Get-Content $lock); exit 2 }
"Claude opti30 xbox+exp+ps2 $(Get-Date -Format 'HH:mm')" | Out-File -Encoding utf8 $lock
try {
    $env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
    $cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    Push-Location "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30"
    foreach ($p in @("xbox-release", "xbox-release-exp", "ps2-release")) {
        "=== $p"
        & $cm --build --preset $p 2>&1 | Out-File -Encoding utf8 "$env:TEMP\opti30-$p.log"
        Select-String -Path "$env:TEMP\opti30-$p.log" -Pattern ': (fatal )?error|FAILED|sucessfully created|Linking CXX executable' | Select-Object -First 12 | ForEach-Object { $l = $_.Line; if ($l.Length -gt 200) { $l.Substring(0,200) } else { $l } }
    }
} finally {
    Pop-Location
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}

# --- Configurar una carpeta de build NUEVA (solo la primera vez) -------------
# Las comillas de los -D son OBLIGATORIAS: sin ellas PowerShell parte la IP en
# los puntos y CMake falla con "Could not read presets from ...\.168.0.17".
#
#   & $cm --preset xbox-release-exp "-DMC_LOG_LEVEL=1" "-DXBOX_NETLOG_HOST=192.168.0.17"
#
# Para la versión pública (sin log), no se pasa ninguno de los dos.
