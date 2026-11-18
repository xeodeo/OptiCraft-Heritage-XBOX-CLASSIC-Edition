# Captures the running xemu window (largest visible window of the process).
param([string]$Out = "$PSScriptRoot\captura.png")
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class WCap { public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
  public struct RECT { public int L,T,R,B; }
  public static IntPtr Largest(uint pid){ IntPtr best=IntPtr.Zero; int ba=0; EnumWindows((h,p)=>{uint wp; GetWindowThreadProcessId(h,out wp); if(wp==pid&&IsWindowVisible(h)){RECT r; GetClientRect(h,out r); int a=(r.R-r.L)*(r.B-r.T); if(a>ba){ba=a;best=h;}} return true;},IntPtr.Zero); return best; } }
"@
$p = Get-Process xemu -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $p) { "xemu no esta abierto"; exit 1 }
$h = [WCap]::Largest([uint32]$p.Id)
$r = New-Object WCap+RECT
[WCap]::GetClientRect($h, [ref]$r) | Out-Null
$b = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
$g = [System.Drawing.Graphics]::FromImage($b)
$hdc = $g.GetHdc()
[WCap]::PrintWindow($h, $hdc, 3) | Out-Null
$g.ReleaseHdc($hdc)
$b.Save($Out)
"ok $Out"
