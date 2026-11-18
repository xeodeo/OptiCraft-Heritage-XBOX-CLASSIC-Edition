# Boots the autopilot test ISO in xemu, lets it create the fixed-seed world and walk, then
# keeps the terrain fingerprints ("genhash x z hash") that arrived in netlog.txt.
#   powershell -File genhash_run.ps1 -Out <file> [-Seconds 90]
# Needs: bin\xbox-autotest built with XBOX_AUTOPILOT=ON and XBOX_NETLOG_HOST set to this PC,
# the UDP listener (escuchar_log.py) running, and xemu network on (NAT).
param([string]$Out, [int]$Seconds = 90)
$log = "$PSScriptRoot\netlog.txt"
$iso = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\wt-opti30\bin\xbox-autotest\OptiCraft.iso"
$start = (Get-Content $log | Measure-Object -Line).Lines
Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Process -FilePath "C:\RetroXeo\emulators\xemu\xemu.exe" -ArgumentList "-dvd_path `"$iso`"" | Out-Null
Start-Sleep -Seconds $Seconds
Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1
$lines = Get-Content $log | Select-Object -Skip $start
$hashes = $lines | Select-String -Pattern "\[genhash\] (-?\d+) (-?\d+) ([0-9a-f]{8})" | ForEach-Object {
    "{0} {1} {2}" -f $_.Matches[0].Groups[1].Value, $_.Matches[0].Groups[2].Value, $_.Matches[0].Groups[3].Value }
$hashes | Sort-Object -Unique | Set-Content -Encoding ascii $Out
"{0} fingerprints -> {1}" -f (Get-Content $Out | Measure-Object -Line).Lines, $Out
