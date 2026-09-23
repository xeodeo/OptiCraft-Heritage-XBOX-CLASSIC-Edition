# Compiles ONE source file of the Xbox build with the exact xbox-release flags,
# into a private temp object, so several agents can check their own files in
# parallel without touching the shared ninja build directory.
#
#   powershell -File compile_one.ps1 src\platform\InputBackend_XBOX.cpp [more files...]
#
# Works for files not yet known to CMake too: it borrows the command line of a
# file of the same kind (XDK backend vs. shared/Win32 code) and swaps the source.
param([Parameter(Mandatory = $true, ValueFromRemainingArguments = $true)][string[]]$Sources)
$ErrorActionPreference = 'Stop'
$root = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition"
$db = Get-Content "$root\build\xbox-release\compile_commands.json" -Raw | ConvertFrom-Json
$tmp = Join-Path $env:TEMP ("xbox-compile-one-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $tmp | Out-Null
$failed = 0
foreach ($src in $Sources) {
    $full = (Resolve-Path (Join-Path $root $src) -ErrorAction SilentlyContinue)
    if (-not $full) { $full = Resolve-Path $src }
    $full = $full.Path
    $norm = $full.Replace('\', '/')
    $isXdk = ($norm -match '/src/xbox/' -or $norm -match '_XBOX\.cpp$') -and ($norm -notmatch '/src/xbox/runtime/')
    $entry = $db | Where-Object { $_.file.Replace('\', '/') -ieq $norm } | Select-Object -First 1
    if (-not $entry) {
        $pattern = if ($isXdk) { '_XBOX\.cpp$' } else { '/src/net/minecraft/src/.*\.cpp$' }
        $entry = $db | Where-Object { $_.file.Replace('\', '/') -match $pattern } | Select-Object -First 1
    }
    $cmd = $entry.command
    $obj = Join-Path $tmp ([IO.Path]::GetFileNameWithoutExtension($full) + ".obj")
    # Swap the output object and the source file of the borrowed command line.
    # Paths contain spaces, so replace the borrowed entry's exact source path
    # (in whatever quoting/slash form it appears) instead of matching \S+.
    $cmd = [regex]::Replace($cmd, '/Fo("[^"]*"|\S+)', "/Fo`"$obj`"")
    $cmd = [regex]::Replace($cmd, '/Fd("[^"]*"|\S+)', "/Fd`"$tmp\\`"")
    $srcForms = @($entry.file, $entry.file.Replace('/', '\'), $entry.file.Replace('\', '/')) | Select-Object -Unique
    $swapped = $false
    foreach ($form in $srcForms) {
        foreach ($q in @("`"$form`"", $form)) {
            $i = $cmd.LastIndexOf($q, [StringComparison]::OrdinalIgnoreCase)
            if ($i -ge 0) {
                $cmd = $cmd.Substring(0, $i) + "`"$full`"" + $cmd.Substring($i + $q.Length)
                $swapped = $true
                break
            }
        }
        if ($swapped) { break }
    }
    if (-not $swapped) { Write-Output "FAIL $src"; Write-Output "  compile_one: could not locate the source path in the borrowed command"; $failed++; continue }
    Push-Location $entry.directory
    try {
        $out = cmd /c "$cmd 2>&1"
    } finally { Pop-Location }
    $errs = $out | Select-String ': (fatal )?error C\d+'
    if ($LASTEXITCODE -ne 0 -or $errs) {
        $failed++
        Write-Output "FAIL $src"
        $out | Select-String ': (fatal )?error|: warning C4\d+' | Select-Object -First 40 | ForEach-Object { "  " + $_.Line }
    } else {
        Write-Output "OK   $src"
    }
}
Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
if ($failed) { exit 1 }
