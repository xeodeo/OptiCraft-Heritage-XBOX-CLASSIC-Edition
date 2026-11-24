param([int]$Seconds = 260, [string]$Mode = "on", [string]$Wt = "wt-125-choco", [string]$Data = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data")
$lock = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$top = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$root = "$top\$Wt"
$spike = "$top\OptiCraftHeritageEdition\xbox-spike"
$out = "C:\Users\xeodeo\AppData\Local\Temp\claude\ab7fa9eb85ea2df0\5081e937-c767-46da-b247-a4e0f6c8024e\scratchpad\xemu119-$Wt-$Mode"
$env:XBOX_XDK_ROOT = "$top\xdk\5849\sdk\XDK\xbox"
$env:XBOX_MSVC_ROOT = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$env:XBOX_WINSDK_ROOT = "C:\Program Files (x86)\Windows Kits\10"
New-Item -ItemType Directory -Force $out | Out-Null
while ($true) {
  while (Test-Path $lock) { Start-Sleep -Seconds 20 }
  try {
    $fs = [System.IO.File]::Open($lock, 'CreateNew', 'Write')
    $b = [System.Text.Encoding]::UTF8.GetBytes("Claude subagente 119 chocobos: build autotest + xemu ($Wt) $(Get-Date -Format 'HH:mm')")
    $fs.Write($b, 0, $b.Length); $fs.Close(); break
  } catch { Start-Sleep -Seconds 5 }
}
"candado cogido $(Get-Date -Format 'HH:mm')"
try {
  Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
  $bd = "$root\build\xbox-autotest"
  if (-not (Test-Path "$bd\build.ninja")) {
    & $cm -S $root -B $bd -G Ninja "-DCMAKE_MAKE_PROGRAM=$root\ninja.exe" "-DCMAKE_TOOLCHAIN_FILE=$root\cmake\xbox_toolchain.cmake" `
      -DCMAKE_BUILD_TYPE=Release -DPLATFORM=XBOX -DXBOX_BRINGUP=OFF -DXBOX_ENABLE_SOUND=ON -DXBOX_ENABLE_NETWORK=ON -DXBOX_AUTOPILOT=ON -DMC_LOG_LEVEL=1 "-DXBOX_DATA_DIR=$Data" "-DXBOX_XDK_ROOT=$top\xdk\5849\sdk\XDK\xbox" 2>&1 | Out-File -Encoding utf8 "$out\cfg.log"
  }
  New-Item -ItemType Directory -Force "$root\bin\xbox-autotest\iso" | Out-Null
  Set-Content -Encoding ascii "$root\bin\xbox-autotest\iso\autopilot.txt" "0 -"
  Set-Content -Encoding ascii "$root\bin\xbox-autotest\iso\probe119.txt" $Mode
  if (Test-Path "$root\bin\xbox-autotest\OptiCraft.exe") { Remove-Item "$root\bin\xbox-autotest\OptiCraft.exe" -Force }
  & $cm --build $bd 2>&1 | Out-File -Encoding utf8 "$out\build.log"
  "build exit $LASTEXITCODE"
  $iso = Get-ChildItem "$root\bin\xbox-autotest" -Filter *.iso | Select-Object -First 1
  if (-not $iso) { "NO ISO"; exit 1 }
  Start-Process -FilePath "C:\RetroXeo\emulators\xemu\xemu.exe" -ArgumentList "-dvd_path `"$($iso.FullName)`" -gdb tcp:127.0.0.1:1235" | Out-Null
  $t0 = Get-Date
  $i = 0
  while (((Get-Date) - $t0).TotalSeconds -lt $Seconds) {
    Start-Sleep -Seconds 6
    $i++
    if (-not (Get-Process xemu -ErrorAction SilentlyContinue)) { "xemu gone at $i"; break }
    try { & "$spike\captura.ps1" -Out ("$out\shot_{0:D3}.png" -f $i) 2>$null | Out-Null } catch { "capture failed $i" }
    $map1 = Get-ChildItem "$root\bin\xbox-autotest" -Filter *.map | Select-Object -First 1; python "$spike\gdblog.py" $map1.FullName 60000 2>&1 | Out-File -Encoding utf8 "$out\gamelog_live.txt"
    if ($i % 3 -eq 0) { $map0 = Get-ChildItem "$root\bin\xbox-autotest" -Filter *.map | Select-Object -First 1; python "$spike\gdblog.py" $map0.FullName 60000 2>&1 | Select-String "PROBE" | ForEach-Object { $_.Line } | Out-File -Append -Encoding utf8 "$out\probe.txt" }
  }
  $map = Get-ChildItem "$root\bin\xbox-autotest" -Filter *.map | Select-Object -First 1
  python "$spike\gdblog.py" $map.FullName 60000 2>&1 | Out-File -Encoding utf8 "$out\gamelog.txt"
  Get-Content "$out\gamelog.txt" | Select-String "PROBE|ERROR|\[E\]" | Select-Object -Last 40 | ForEach-Object { $_.Line }
} finally {
  Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
  Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
