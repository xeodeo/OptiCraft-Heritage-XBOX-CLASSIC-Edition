# Configures (fresh) and builds an Xbox CMake preset, printing only useful lines.
param([string]$Preset = "xbox-bringup", [switch]$Fresh, [int]$Tail = 40)
$env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$root = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition"
Push-Location $root
try {
    if ($Fresh) {
        & $cm --preset $Preset --fresh 2>&1 | Select-Object -Last 4
    }
    & $cm --build --preset $Preset 2>&1 |
        Where-Object { $_ -notmatch 'LNK4229|LNK4049|LNK4217|^\s*(ignored|encountered)' } |
        ForEach-Object { if ($_.Length -gt 240) { $_.Substring(0, 240) } else { $_ } } |
        Select-Object -Last $Tail
} finally {
    Pop-Location
}
