# Builds the autopilot test ISO (bin/xbox-autotest, never deployed), boots it
# in xemu with the gdbstub, waits, then screenshots and dumps the game log.
param([int]$Seconds = 60, [switch]$NoBuild, [string]$Script = "$PSScriptRoot\autopilot.txt")
$ErrorActionPreference = 'Continue'
$env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$root = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition"
$spike = $PSScriptRoot
$bd = "$root\build\xbox-autotest"
$iso = "$root\bin\xbox-autotest\OptiCraft.iso"

Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
if (-not $NoBuild) {
    if (-not (Test-Path "$bd\build.ninja")) {
        & $cm -S $root -B $bd -G Ninja "-DCMAKE_MAKE_PROGRAM=$root\ninja.exe" "-DCMAKE_TOOLCHAIN_FILE=$root\cmake\xbox_toolchain.cmake" `
            -DCMAKE_BUILD_TYPE=Release -DPLATFORM=XBOX -DXBOX_AUTOPILOT=ON -DMC_LOG_LEVEL=1 "-DXBOX_DATA_DIR=C:/Users/xeodeo/Downloads/data" 2>&1 | Select-Object -Last 2
        & $cm --build $bd --target xbox-data 2>&1 | Select-Object -Last 1
    }
    New-Item -ItemType Directory -Force "$root\bin\xbox-autotest\iso" | Out-Null
    Copy-Item $Script "$root\bin\xbox-autotest\iso\autopilot.txt" -Force
    # Force the post-build repack so the new script lands in the ISO.
    if (Test-Path "$root\bin\xbox-autotest\OptiCraft.exe") { Remove-Item "$root\bin\xbox-autotest\OptiCraft.exe" -Force }
    & $cm --build $bd 2>&1 | Out-File -Encoding utf8 "$spike\autotest_build.log"
    $errs = Get-Content "$spike\autotest_build.log" | Select-String ': (fatal )?error (C|LNK)\d+|FAILED:'
    if ($errs) { "BUILD FAILED"; $errs | Select-Object -First 10 | ForEach-Object { $_.Line }; exit 1 }
    "build ok"
}
Start-Process -FilePath "C:\RetroXeo\emulators\xemu\xemu.exe" -ArgumentList "-dvd_path `"$iso`" -gdb tcp:127.0.0.1:1235" | Out-Null
Start-Sleep -Seconds $Seconds
& "$spike\captura.ps1" -Out "$spike\autotest.png" | Out-Null
python "$spike\gdblog.py" "$root\bin\xbox-autotest\OptiCraft.exe.map" 6000 2>&1 | Out-File -Encoding utf8 "$spike\autotest_log.txt"
Get-Content "$spike\autotest_log.txt" | Select-Object -First 1
Get-Content "$spike\autotest_log.txt" | Select-String "xbox\.(player|fpu)|ERROR|\[E\]|\[W\]" | Select-Object -Last 25 | ForEach-Object { $_.Line }
