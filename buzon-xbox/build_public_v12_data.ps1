# Reconfigura las dos publicas con el arbol de datos bueno y reempaqueta el ISO.
$lock = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\buzon-xbox\BUILD.lock"
if (Test-Path $lock) { "BLOQUEADO: " + (Get-Content $lock); exit 2 }
"Claude release-v1.2 datos+iso $(Get-Date -Format 'HH:mm')" | Out-File -Encoding utf8 $lock
try {
    $env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
    $cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    $datos = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\data"
    Push-Location "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30"
    foreach ($p in @("xbox-public", "xbox-public-exp")) {
        "=== configure $p"
        & $cm --preset $p "-DXBOX_DATA_DIR=$datos" 2>&1 | Select-String -Pattern "Xbox build:|Configuring done|Generating done|CMake Error" | ForEach-Object { $_.Line }
        "=== build $p"
        & $cm --build --preset $p 2>&1 | Out-File -Encoding utf8 "$env:TEMP\v12b-$p.log"
        if ($LASTEXITCODE -ne 0) { "BUILD FALLO $p" }
        Select-String -Path "$env:TEMP\v12b-$p.log" -Pattern ': (fatal )?error|FAILED|sucessfully created|successfully created' | Select-Object -First 6 | ForEach-Object { $_.Line }
    }
    "=== resultado"
    foreach ($d in @("bin\xbox-public","bin\xbox-public-exp")) {
        $n = (Get-ChildItem "$d\iso\data" -Recurse -File -ErrorAction SilentlyContinue | Measure-Object).Count
        $iso = Get-Item "$d\OptiCraft.iso" -ErrorAction SilentlyContinue
        "{0}: data={1} ficheros, iso={2:N0} bytes ({3})" -f $d, $n, $iso.Length, $iso.LastWriteTime
    }
} finally {
    Pop-Location
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
