# Monta la release con el LANZADOR (default.xbe que deja elegir version).
#   powershell -File "buzon-xbox\montar_release_lanzador.ps1" -Version v1.2
# Necesita compilados: xbox-launcher, xbox-public y xbox-public-exp (wt-opti30).
# Saca en Desktop\release\<Version>\: el zip (carpeta OptiCraft\ para el disco
# duro) y una ISO con las cuatro versiones para xemu.
param([string]$Version = "v1.2")
$root = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$wt = "$root\wt-opti30"
$out = "C:\Users\xeodeo\Desktop\release\$Version"
$stage = "$env:TEMP\opticraft-release-$Version\OptiCraft"

$launcher = "$wt\bin\xbox-launcher\iso\default.xbe"
$normal = "$wt\bin\xbox-public\iso\default.xbe"
$exp = "$wt\bin\xbox-public-exp\iso\default.xbe"
foreach ($f in $launcher, $normal, $exp, "$wt\bin\xbox-public\iso\data") {
    if (-not (Test-Path $f)) { "FALTA: $f"; exit 1 }
}
$nData = (Get-ChildItem "$wt\bin\xbox-public\iso\data" -Recurse -File).Count
if ($nData -lt 800) { "data\ incompleto ($nData ficheros, deben ser ~804): falta XBOX_DATA_DIR"; exit 1 }

Remove-Item -Recurse -Force (Split-Path $stage) -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $stage | Out-Null
robocopy "$wt\bin\xbox-public\iso\data" "$stage\data" /E /NFL /NDL /NJH /NJS | Out-Null
Copy-Item $launcher "$stage\default.xbe"
Copy-Item $normal "$stage\OptiCraft.xbe"
Copy-Item $normal "$stage\OptiCraft_720p.xbe"
Copy-Item $exp "$stage\OptiCraft_exp.xbe"
Copy-Item $exp "$stage\OptiCraft_exp_720p.xbe"
Copy-Item "$wt\bin\xbox-public\iso\default.tbn" "$stage\default.tbn"

New-Item -ItemType Directory -Force $out | Out-Null
$zip = "$out\OptiCraft-Xbox-$Version.zip"
$iso = "$out\OptiCraft-Xbox-$Version.iso"
Remove-Item $zip, $iso -ErrorAction SilentlyContinue
Compress-Archive -Path $stage -DestinationPath $zip
$xiso = Get-ChildItem "$root\xbox-tools\artifacts" -Filter extract-xiso.exe -Recurse | Select-Object -First 1
& $xiso.FullName -c $stage $iso | Select-Object -Last 1
Get-ChildItem $out | ForEach-Object { "{0,-34} {1,12:N0}" -f $_.Name, $_.Length }
Get-FileHash -Algorithm MD5 $zip, $iso | ForEach-Object { "$($_.Hash.ToLower())  $(Split-Path -Leaf $_.Path)" }
