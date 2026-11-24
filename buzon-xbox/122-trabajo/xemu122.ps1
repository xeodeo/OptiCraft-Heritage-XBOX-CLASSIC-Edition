param([int]$Seconds = 420, [string]$Wt = "wt-152-choco", [string]$Data = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-152-codex\build\xbox-release\port152-data", [string]$Tag = "run1")
$lock = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$top = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$root = "$top\$Wt"
$spike = "$top\OptiCraftHeritageEdition\xbox-spike"
$work = "$top\buzon-xbox\122-trabajo"
$out = "$work\xemu-$Wt-$Tag"
$env:XBOX_XDK_ROOT = "$top\xdk\5849\sdk\XDK\xbox"
$env:XBOX_MSVC_ROOT = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$env:XBOX_WINSDK_ROOT = "C:\Program Files (x86)\Windows Kits\10"
New-Item -ItemType Directory -Force $out | Out-Null
while ($true) {
  while (Test-Path $lock) { Start-Sleep -Seconds 20 }
  try {
    $fs = [System.IO.File]::Open($lock, 'CreateNew', 'Write')
    $b = [System.Text.Encoding]::UTF8.GetBytes("Claude subagente 122 modo admin: build autotest + xemu ($Wt) $(Get-Date -Format 'HH:mm')")
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
  Copy-Item "$work\autopilot.txt" "$root\bin\xbox-autotest\iso\autopilot.txt" -Force
  Set-Content -Encoding ascii "$root\bin\xbox-autotest\iso\probe122.txt" "on"
  Get-ChildItem "$root\bin\xbox-autotest\iso" -Filter "probe1*.txt" | Where-Object { $_.Name -ne "probe122.txt" } | Remove-Item -Force
  if (Test-Path "$root\bin\xbox-autotest\OptiCraft.exe") { Remove-Item "$root\bin\xbox-autotest\OptiCraft.exe" -Force }
  & $cm --build $bd 2>&1 | Out-File -Encoding utf8 "$out\build.log"
  "build exit $LASTEXITCODE"
  $iso = Get-ChildItem "$root\bin\xbox-autotest" -Filter *.iso | Select-Object -First 1
  if (-not $iso) { "NO ISO"; exit 1 }
  Start-Process -FilePath "C:\RetroXeo\emulators\xemu\xemu.exe" -ArgumentList "-dvd_path `"$($iso.FullName)`" -gdb tcp:127.0.0.1:1235" | Out-Null
  $t0 = Get-Date
  $i = 0
  while (((Get-Date) - $t0).TotalSeconds -lt $Seconds) {
    Start-Sleep -Seconds 5
    $i++
    if (-not (Get-Process xemu -ErrorAction SilentlyContinue)) { "xemu gone at $i"; break }
    try { & "$spike\captura.ps1" -Out ("$out\shot_{0:D3}.png" -f $i) 2>$null | Out-Null } catch { "capture failed $i" }
    if ($i % 2 -eq 0) {
      $map0 = Get-ChildItem "$root\bin\xbox-autotest" -Filter *.map | Select-Object -First 1
      python "$spike\gdblog.py" $map0.FullName 60000 2>&1 | Out-File -Encoding utf8 "$out\gamelog_live.txt"
      Get-Content "$out\gamelog_live.txt" | Select-String "PROBE" | ForEach-Object { "{0:D3} {1}" -f $i, $_.Line } | Out-File -Append -Encoding utf8 "$out\probe.txt"
      if (Select-String -Path "$out\gamelog_live.txt" -Pattern "\[PROBE\] END" -Quiet) { "probe END at $i"; break }
    }
  }
  $map = Get-ChildItem "$root\bin\xbox-autotest" -Filter *.map | Select-Object -First 1
  python "$spike\gdblog.py" $map.FullName 60000 2>&1 | Out-File -Encoding utf8 "$out\gamelog.txt"
  Get-Content "$out\gamelog.txt" | Select-String "PROBE|ERROR|\[E\]" | Select-Object -Last 60 | ForEach-Object { $_.Line }
} finally {
  Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
  Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
