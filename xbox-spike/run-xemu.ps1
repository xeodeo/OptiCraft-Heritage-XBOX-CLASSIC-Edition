param([string]$Iso, [int]$Seconds = 15, [string]$Shot = "$PSScriptRoot\shot.png", [switch]$KeepOpen)
# Boots an xiso in xemu, waits, captures the largest xemu window, reports the
# center pixel color, then closes xemu (unless -KeepOpen).
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Collections.Generic; using System.Runtime.InteropServices;
public class W {
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  public struct RECT { public int L, T, R, B; }
  public static IntPtr Largest(uint pid) {
    IntPtr best = IntPtr.Zero; int bestArea = 0;
    EnumWindows((h, p) => { uint wp; GetWindowThreadProcessId(h, out wp);
      if (wp == pid && IsWindowVisible(h)) { RECT r; GetClientRect(h, out r); int a = (r.R - r.L) * (r.B - r.T);
        if (a > bestArea) { bestArea = a; best = h; } } return true; }, IntPtr.Zero);
    return best; }
}
"@
Get-Process xemu -ErrorAction SilentlyContinue | Stop-Process -Force
$p = Start-Process -FilePath "C:\RetroXeo\emulators\xemu\xemu.exe" -ArgumentList "-dvd_path `"$Iso`"" -PassThru
Start-Sleep -Seconds $Seconds
$p.Refresh()
if ($p.HasExited) { Write-Output "XEMU EXITED code=$($p.ExitCode)"; exit 1 }
$h = [W]::Largest([uint32]$p.Id)
$r = New-Object W+RECT
[W]::GetClientRect($h, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ([Math]::Max(1, $r.R - $r.L)), ([Math]::Max(1, $r.B - $r.T))
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[W]::PrintWindow($h, $hdc, 3) | Out-Null   # PW_CLIENTONLY | PW_RENDERFULLCONTENT
$g.ReleaseHdc($hdc)
$bmp.Save($Shot)
$c = $bmp.GetPixel([int]($bmp.Width / 2), [int]($bmp.Height / 2))
Write-Output "center pixel R=$($c.R) G=$($c.G) B=$($c.B) size=$($bmp.Width)x$($bmp.Height) shot=$Shot"
if (-not $KeepOpen) { Stop-Process -Id $p.Id -Force }
