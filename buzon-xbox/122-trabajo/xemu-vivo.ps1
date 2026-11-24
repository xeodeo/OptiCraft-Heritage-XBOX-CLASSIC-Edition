# Compila el build de pruebas (autotest + autopilot), abre xemu con GDB y lo deja abierto para
# manejarlo en vivo con OptiCraftHeritageEdition\xbox-spike\livepad.py. Coge el BUILD.lock;
# borrarlo al cerrar xemu (o usar -Release para soltarlo cuando xemu se cierre).
param([string]$Wt = "wt-build-152", [switch]$NoMinimap)
$top = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$lock = "$top\buzon-xbox\BUILD.lock"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$root = "$top\$Wt"
$env:XBOX_XDK_ROOT = "$top\xdk\5849\sdk\XDK\xbox"
$env:XBOX_MSVC_ROOT = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$env:XBOX_WINSDK_ROOT = "C:\Program Files (x86)\Windows Kits\10"
if (Test-Path $lock) { "CANDADO OCUPADO: $(Get-Content $lock)"; exit 1 }
Set-Content -Encoding utf8 $lock "Claude: xemu en vivo (livepad) $Wt $(Get-Date -Format 'HH:mm')"
$iso = "$root\bin\xbox-autotest\iso"
Copy-Item "$top\buzon-xbox\122-trabajo\autopilot.txt" "$iso\autopilot.txt" -Force
$nm = "$iso\nominimap.txt"
if ($NoMinimap) { Set-Content -Encoding ascii $nm "on" } elseif (Test-Path $nm) { Remove-Item $nm -Force }
Get-ChildItem $iso -Filter "probe1*.txt" | Remove-Item -Force
if (Test-Path "$root\bin\xbox-autotest\OptiCraft.exe") { Remove-Item "$root\bin\xbox-autotest\OptiCraft.exe" -Force }
& $cm --build "$root\build\xbox-autotest" 2>&1 | Out-File -Encoding utf8 "$top\buzon-xbox\122-trabajo\vivo-build.log"
"build exit $LASTEXITCODE"
if ($LASTEXITCODE -ne 0) { Remove-Item $lock -Force; exit 1 }
$isoFile = Get-ChildItem "$root\bin\xbox-autotest" -Filter *.iso | Select-Object -First 1
Start-Process -FilePath "C:\RetroXeo\emulators\xemu\xemu.exe" -ArgumentList "-dvd_path `"$($isoFile.FullName)`" -gdb tcp:127.0.0.1:1235" | Out-Null
"xemu abierto; map: $((Get-ChildItem "$root\bin\xbox-autotest" -Filter *.map | Select-Object -First 1).FullName)"
