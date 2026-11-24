$lock = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$root = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$env:XBOX_XDK_ROOT = "$root\xdk\5849\sdk\XDK\xbox"
$env:XBOX_MSVC_ROOT = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$env:XBOX_WINSDK_ROOT = "C:\Program Files (x86)\Windows Kits\10"
while ($true) {
  while (Test-Path $lock) { Start-Sleep -Seconds 20 }
  try {
    $fs = [System.IO.File]::Open($lock, 'CreateNew', 'Write')
    $b = [System.Text.Encoding]::UTF8.GetBytes("Claude build fix End (wt-build-125) $(Get-Date -Format 'HH:mm')")
    $fs.Write($b, 0, $b.Length); $fs.Close(); break
  } catch { Start-Sleep -Seconds 5 }
}
"candado cogido $(Get-Date -Format 'HH:mm')"
$jobs = @(
  @{ w = "wt-build-125"; data = "$root\OptiCraftHeritageEdition\data" }
)
try {
  foreach ($j in $jobs) {
    Push-Location "$root\$($j.w)"
    foreach ($p in "xbox-release-exp") {
      & $cm --preset $p "-DMC_LOG_LEVEL=1" "-DXBOX_NETLOG_HOST=192.168.0.18" "-DXBOX_DATA_DIR=$($j.data)" "-DXBOX_XDK_ROOT=$root\xdk\5849\sdk\XDK\xbox" 2>&1 | Out-File -Encoding utf8 "$env:TEMP\cfg-$($j.w)-$p.log"
      "cfg $($j.w) $p exit $LASTEXITCODE"
      & $cm --build --preset $p 2>&1 | Out-File -Encoding utf8 "$env:TEMP\bld-$($j.w)-$p.log"
      "build $($j.w) $p exit $LASTEXITCODE"
    }
    Pop-Location
  }
} finally { Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue }

