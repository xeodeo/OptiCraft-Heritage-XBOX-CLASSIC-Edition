# Compila el lanzador (preset xbox-launcher) en wt-wifi. Coge y suelta BUILD.lock.
param([string]$Wt = "wt-wifi")
$top = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$lock = "$top\buzon-xbox\BUILD.lock"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$env:XBOX_XDK_ROOT = "$top\xdk\5849\sdk\XDK\xbox"
$env:XBOX_MSVC_ROOT = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$env:XBOX_WINSDK_ROOT = "C:\Program Files (x86)\Windows Kits\10"
if (Test-Path $lock) { "CANDADO OCUPADO: $(Get-Content $lock)"; exit 1 }
Set-Content -Encoding utf8 $lock "Claude: lanzador wifi $Wt $(Get-Date -Format 'HH:mm')"
try {
  Set-Location "$top\$Wt"
  & $cm --preset xbox-launcher "-DXBOX_XDK_ROOT=$top\xdk\5849\sdk\XDK\xbox" 2>&1 | Select-Object -Last 3
  & $cm --build --preset xbox-launcher 2>&1 | Select-String -Pattern "error|warning C4|FAILED|Linking|imagebld" | Select-Object -First 40
  "exit $LASTEXITCODE"
  Get-ChildItem "$top\$Wt\bin\xbox-launcher" -Recurse -Filter *.xbe | ForEach-Object { "$($_.FullName) $($_.Length)" }
} finally { Remove-Item $lock -Force -ErrorAction SilentlyContinue }
