# Encargo 125: autopilot build of wt-125-mods that auto-joins the respawn replay
# server, boots it in xemu and keeps it running (netlog to the PC on UDP 9999).
param([int]$Seconds = 150, [switch]$NoBuild)
$ErrorActionPreference = 'Continue'
$top = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$root = "$top\wt-125-mods"
$lock = "$top\buzon-xbox\BUILD.lock"
$env:XBOX_XDK_ROOT = "$top\xdk\5849\sdk\XDK\xbox"
$env:XBOX_MSVC_ROOT = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$env:XBOX_WINSDK_ROOT = "C:\Program Files (x86)\Windows Kits\10"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$bd = "$root\build\xbox-autotest"
$bin = "$root\bin\xbox-autotest"
$iso = "$bin\OptiCraft.iso"

while ($true) {
  while (Test-Path $lock) { Start-Sleep -Seconds 15 }
  try { $fs = [System.IO.File]::Open($lock, 'CreateNew', 'Write'); $b = [System.Text.Encoding]::UTF8.GetBytes("Claude repro125 $(Get-Date -Format 'HH:mm')"); $fs.Write($b,0,$b.Length); $fs.Close(); break } catch { Start-Sleep 5 }
}
try {
  Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
  if (-not $NoBuild) {
    & $cm -S $root -B $bd -G Ninja "-DCMAKE_MAKE_PROGRAM=$root\ninja.exe" "-DCMAKE_TOOLCHAIN_FILE=$root\cmake\xbox_toolchain.cmake" `
      -DCMAKE_BUILD_TYPE=Release -DPLATFORM=XBOX -DXBOX_BRINGUP=OFF -DXBOX_ENABLE_SOUND=ON -DXBOX_ENABLE_NETWORK=ON -DXBOX_AUTOPILOT=ON -DMC_LOG_LEVEL=1 "-DXBOX_NETLOG_HOST=192.168.0.18" `
      "-DXBOX_DATA_DIR=$top\OptiCraftHeritageEdition\data" "-DXBOX_XDK_ROOT=$env:XBOX_XDK_ROOT" 2>&1 | Select-Object -Last 2
    New-Item -ItemType Directory -Force "$bin\iso" | Out-Null
    Set-Content -Encoding ascii "$bin\iso\autoconnect.txt" "192.168.0.18 25567 340"
    Set-Content -Encoding ascii "$bin\iso\autopilot.txt" "0 -"
    if (Test-Path "$bin\OptiCraft.exe") { Remove-Item "$bin\OptiCraft.exe" -Force }
    & $cm --build $bd 2>&1 | Out-File -Encoding utf8 "$env:TEMP\repro125_build.log"
    $errs = Get-Content "$env:TEMP\repro125_build.log" | Select-String ': (fatal )?error (C|LNK)\d+|FAILED:'
    if ($errs) { "BUILD FAILED"; $errs | Select-Object -First 10 | ForEach-Object { $_.Line }; exit 1 }
    "build ok"
  }
  Start-Process -FilePath "C:\RetroXeo\emulators\xemu\xemu.exe" -ArgumentList "-dvd_path `"$iso`" -gdb tcp:127.0.0.1:1235" | Out-Null
  Start-Sleep -Seconds $Seconds
} finally { Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue }
"xemu started; leave it running"
