# Compila las DOS versiones publicas (sin log por red) de wt-opti30:
#   xbox-public      -> bin\xbox-public      (normal)
#   xbox-public-exp  -> bin\xbox-public-exp  (async / experimental)
# Para la release v1.2. Respeta BUILD.lock.
$lock = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock"
if (Test-Path $lock) { "BLOQUEADO: " + (Get-Content $lock); exit 2 }
"Claude release-v1.2 publicas $(Get-Date -Format 'HH:mm')" | Out-File -Encoding utf8 $lock
try {
    $env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
    $cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    Push-Location "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30"
    foreach ($p in @("xbox-public", "xbox-public-exp")) {
        "=== configure $p"
        & $cm --preset $p 2>&1 | Out-File -Encoding utf8 "$env:TEMP\v12-cfg-$p.log"
        if ($LASTEXITCODE -ne 0) { "CONFIGURE FALLO $p"; Get-Content "$env:TEMP\v12-cfg-$p.log" -Tail 20; continue }
        "=== build $p"
        & $cm --build --preset $p 2>&1 | Out-File -Encoding utf8 "$env:TEMP\v12-build-$p.log"
        if ($LASTEXITCODE -ne 0) { "BUILD FALLO $p" }
        Select-String -Path "$env:TEMP\v12-build-$p.log" -Pattern ': (fatal )?error|FAILED|Linking CXX executable|successfully created|sucessfully created' | Select-Object -First 12 | ForEach-Object { $l = $_.Line; if ($l.Length -gt 200) { $l.Substring(0,200) } else { $l } }
    }
    "=== salidas"
    Get-ChildItem "bin\xbox-public","bin\xbox-public-exp" -Recurse -Filter *.xbe -ErrorAction SilentlyContinue | ForEach-Object { "{0}  {1}  {2:N0}" -f $_.LastWriteTime, $_.FullName, $_.Length }
    Get-ChildItem "bin\xbox-public","bin\xbox-public-exp" -Filter *.iso -ErrorAction SilentlyContinue | ForEach-Object { "{0}  {1}  {2:N0}" -f $_.LastWriteTime, $_.FullName, $_.Length }
} finally {
    Pop-Location
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
