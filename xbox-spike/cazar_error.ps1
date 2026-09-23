# Boots the deployed ISO in xemu with the gdbstub and logs every C++ throw
# straight to throws.log (unbuffered), so it can be read at any moment.
$d = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico"
$log = "$d\xbox-spike\throws.log"
Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Start-Process -FilePath "C:\RetroXeo\emulators\xemu\xemu.exe" -ArgumentList "-dvd_path `"C:\RetroXeo\roms\xbox\OptiCraft.iso`" -gdb tcp:127.0.0.1:1235" | Out-Null
Start-Sleep -Seconds 15
Start-Process -FilePath "python" -ArgumentList "-u", "`"$d\xbox-spike\gdbthrow.py`"", "`"$d\OptiCraftHeritageEdition\bin\xbox\OptiCraft.exe.map`"", "600", "40" `
    -RedirectStandardOutput $log -RedirectStandardError "$log.err" -WindowStyle Hidden | Out-Null
Start-Sleep -Seconds 3
Get-Content $log -ErrorAction SilentlyContinue
Get-Content "$log.err" -ErrorAction SilentlyContinue

