# Keep-going full Xbox build; writes full_build.log and prints an error summary.
param([switch]$Fresh, [int]$Top = 30)
$env:XBOX_XDK_ROOT = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox"
$cm = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$root = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition"
$log = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\full_build.log"
Push-Location $root
try {
    if ($Fresh) { & $cm --preset xbox-release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON --fresh 2>&1 | Select-Object -Last 3 }
    & $cm --build --preset xbox-release -- -k 0 2>&1 | Out-File -Encoding utf8 $log
} finally { Pop-Location }
$l = Get-Content $log
"FAILED objs: $(($l | Select-String '^FAILED:.*\.obj').Count)   errors: $(($l | Select-String ': (fatal )?error C').Count)   link: $(($l | Select-String 'LNK\d+').Count)"
$l | Select-String ': (fatal )?error (C|LNK)\d+' | ForEach-Object { ($_.Line -replace '^.*?\\src\\', 'src\' -replace '^.*?include\\', 'INC\' -replace '\(\d+\)', '') } |
    Group-Object | Sort-Object Count -Descending | Select-Object -First $Top |
    ForEach-Object { "{0,5} {1}" -f $_.Count, ($_.Name.Substring(0, [Math]::Min(210, $_.Name.Length))) }
