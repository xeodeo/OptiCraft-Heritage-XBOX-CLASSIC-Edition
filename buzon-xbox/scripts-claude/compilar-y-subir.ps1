# Compila OptiCraft (Xbox) y lo sube SIEMPRE: lanzador (ClassicXboxLoader) + consola por FTP.
# Uso: powershell -File compilar-y-subir.ps1 -Which "152,125" -Presets "xbox-release-exp,xbox-release"
#   152 = wt-build-152 (rama port152-int), 125 = wt-opti30 (rama opti30-erluza).
# Coge buzon-xbox\BUILD.lock. Si la consola no responde (juego abierto), lo dice y reintenta 10 min.
param([string]$Which = "152,125", [string]$Presets = "xbox-release-exp")
$root = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$lock = "$root\buzon-xbox\BUILD.lock"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$loader = "C:\Users\xeodeo\Downloads\ClassicXboxLoader\downloads\OptiCraft_extracted\OptiCraft"
$ftp = "ftp://192.168.0.3/F/Games/OptiCraft"
$env:XBOX_XDK_ROOT = "$root\xdk\5849\sdk\XDK\xbox"
$env:XBOX_MSVC_ROOT = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$env:XBOX_WINSDK_ROOT = "C:\Program Files (x86)\Windows Kits\10"
$all = @{
  "152" = @{ w = "wt-build-152"; ver = "1.5.2"; data = "$root\wt-152-codex\build\xbox-release\port152-data" };
  "125" = @{ w = "wt-opti30";    ver = "1.2.5"; data = "$root\OptiCraftHeritageEdition\data" }
}
# preset -> carpeta bin y nombres en el lanzador
$out = @{
  "xbox-release"     = @{ bin = "xbox";     names = @("OptiCraft.xbe", "OptiCraft_720p.xbe") };
  "xbox-release-exp" = @{ bin = "xbox-exp"; names = @("OptiCraft_exp.xbe", "OptiCraft_exp_720p.xbe") }
}

while ($true) {
  while (Test-Path $lock) { Start-Sleep -Seconds 20 }
  try {
    $fs = [System.IO.File]::Open($lock, 'CreateNew', 'Write')
    $b = [System.Text.Encoding]::UTF8.GetBytes("Claude compilar-y-subir ($Which $Presets) $(Get-Date -Format 'HH:mm')")
    $fs.Write($b, 0, $b.Length); $fs.Close(); break
  } catch { Start-Sleep -Seconds 5 }
}
$subir = @()
try {
  foreach ($k in $Which.Split(",")) {
    $j = $all[$k]
    Push-Location "$root\$($j.w)"
    foreach ($p in $Presets.Split(",")) {
      & $cm --preset $p "-DMC_LOG_LEVEL=1" "-DXBOX_NETLOG_HOST=192.168.0.18" "-DXBOX_DATA_DIR=$($j.data)" "-DXBOX_XDK_ROOT=$root\xdk\5849\sdk\XDK\xbox" 2>&1 | Out-File -Encoding utf8 "$env:TEMP\cfg-$($j.w)-$p.log"
      & $cm --build --preset $p 2>&1 | Out-File -Encoding utf8 "$env:TEMP\bld-$($j.w)-$p.log"
      if ($LASTEXITCODE -ne 0) { "FALLO build $($j.w) $p (ver $env:TEMP\bld-$($j.w)-$p.log)"; continue }
      $src = "$root\$($j.w)\bin\$($out[$p].bin)\iso\default.xbe"
      $bak = "$loader\..\respaldo-$(Get-Date -Format 'ddMM-HHmm')\$($j.ver)"
      New-Item -ItemType Directory -Force $bak | Out-Null
      foreach ($n in $out[$p].names) {
        if (Test-Path "$loader\$($j.ver)\$n") { Copy-Item "$loader\$($j.ver)\$n" $bak }
        Copy-Item $src "$loader\$($j.ver)\$n" -Force
        $subir += ,@("$loader\$($j.ver)\$n", "$($j.ver)/$n")
      }
      "build ok $($j.w) $p -> lanzador $($j.ver)"
    }
    Pop-Location
  }
} finally { Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue }

# Subida a la consola (solo responde con el dashboard abierto).
$fin = (Get-Date).AddMinutes(10)
foreach ($s in $subir) {
  $local = $s[0]; $remote = "$ftp/$($s[1])"
  $size = (Get-Item $local).Length
  while ($true) {
    & curl.exe -s -S -m 120 --user xbox:xbox -T $local $remote
    $list = & curl.exe -s -m 15 --user xbox:xbox ($remote.Substring(0, $remote.LastIndexOf('/') + 1))
    $name = Split-Path $local -Leaf
    if ($list -match "\s$size\s.*\s$([regex]::Escape($name))\s*$" -or ($list | Where-Object { $_ -match "\s$size\s" -and $_ -match [regex]::Escape($name) })) { "subido $($s[1]) ($size)"; break }
    if ((Get-Date) -gt $fin) { "NO SUBIDO $($s[1]): la consola no responde (¿juego abierto?)"; break }
    "consola no responde, reintento en 20 s..."; Start-Sleep -Seconds 20
  }
}
