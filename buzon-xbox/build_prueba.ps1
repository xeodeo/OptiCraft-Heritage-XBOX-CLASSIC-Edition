param([switch]$Configure)
$env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$root = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraft-prueba-opti"
$data = "C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition/data"
Push-Location $root
try {
    if ($Configure -or -not (Test-Path "$root\build\xbox-release\build.ninja")) {
        & $cm --preset xbox-release "-DXBOX_DATA_DIR=$data" "-DXBOX_NETLOG_HOST=192.168.0.17" "-DMC_LOG_LEVEL=1" "-DXBOX_DEPLOY_DIR=" 2>&1 | Select-Object -Last 3
    }
    & $cm --build --preset xbox-release 2>&1 |
        Where-Object { $_ -notmatch 'LNK4229|LNK4049|LNK4217|^\s*(ignored|encountered)' } |
        Select-String -Pattern ': (fatal )?error|FAILED|sucessfully|unresolved' |
        Select-Object -First 40 | ForEach-Object { $l = $_.Line; if ($l.Length -gt 260) { $l.Substring(0, 260) } else { $l } }
} finally { Pop-Location }
