# Rewrites SSE2 data-movement instructions in the linked PE to SSE1 forms.
#
# The Xbox CPU is a Pentium III: SSE, no SSE2. The VS2022 static CRT is built
# for SSE2 and uses it without a CPU check in plain copies, mostly
# `xorps xmm0,xmm0 / movlpd [mem],xmm0` (zero 8 bytes) and movq/movsd pairs
# that copy a 64-bit value through xmm0. Emulators run those; the console
# raises #UD on the first one (the title never shows a frame). Each copy has an
# SSE1 twin of the same length once the prefix byte becomes a NOP:
#
#   66 0F 12/13  movlpd          -> 90 0F 12/13  movlps
#   F3 0F 7E     movq xmm,m64    -> 90 0F 12     movlps xmm,m64
#   66 0F D6     movq m64,xmm    -> 90 0F 13     movlps m64,xmm
#   F2 0F 10/11  movsd  (memory) -> 90 0F 12/13  movlps
#   F2 0F 10/11  movsd  xmm,xmm  -> 90 0F 28/29  movaps
#   66 0F 28/29  movapd          -> 90 0F 28/29  movaps
#   66 0F 10/11  movupd          -> 90 0F 10/11  movups
#   66 0F 54/55/56/57 and/andn/or/xorpd -> 90 0F ... *ps (bitwise, identical)
#   66 0F 6E/7E  movd (memory)   -> F3 0F 10/11  movss (same zero-extension)
#
# cvttsd2si r32,[ebp+disp] (the UCRT float formatter) has no SSE1 form. It
# becomes `call stub` + NOPs; the stub, written into the XboxSse2Cave code cave
# (src/xbox/runtime/XboxP3Math.c), truncates with the x87 unit instead.
#
# Loads through movlps keep the upper xmm half instead of zeroing it; the
# patched code only uses the low 64 bits. Functions that pick an SSE2 path at
# run time from CPUID (the *_pentium4 libm entry points, memmove/memset, stb's
# SIMD kernels, ...) are left alone: on the console they are never entered.
# Remaining SSE2 arithmetic outside those is listed as a warning; the known
# cases are replaced at source level in src/xbox/runtime/XboxP3Math.c.
param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][string]$Map,
    [Parameter(Mandatory = $true)][string]$Dumpbin
)
$ErrorActionPreference = 'Stop'

# --- Map: function start addresses -> name ----------------------------------
$addrs = New-Object System.Collections.Generic.List[uint32]
$names = New-Object System.Collections.Generic.List[string]
$symRe = [regex]'^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s'
foreach ($line in [IO.File]::ReadLines($Map)) {
    $m = $symRe.Match($line)
    if (-not $m.Success) { continue }
    $a = [Convert]::ToUInt32($m.Groups[2].Value, 16)
    if ($a -lt 0x400000) { continue }
    $addrs.Add($a); $names.Add($m.Groups[1].Value)
}
$addrArr = $addrs.ToArray(); $nameArr = $names.ToArray()
[Array]::Sort($addrArr, $nameArr)
function Get-FunctionName([uint32]$va) {
    $i = [Array]::BinarySearch($addrArr, $va)
    if ($i -lt 0) { $i = (-bnot $i) - 1 }
    if ($i -lt 0) { return '?' }
    return $nameArr[$i]
}
$dispatched = [regex]'pentium4|_filter_x86_sse2|_simd|memmove|memset|memcpy|strchr|strrchr|wcschr|strnlen|wcsnlen|__std_'

# --- PE sections for VA -> file offset --------------------------------------
$bytes = [IO.File]::ReadAllBytes($Path)
$pe = [BitConverter]::ToInt32($bytes, 0x3C)
$numSections = [BitConverter]::ToUInt16($bytes, $pe + 6)
$optSize = [BitConverter]::ToUInt16($bytes, $pe + 20)
$imageBase = [BitConverter]::ToUInt32($bytes, $pe + 24 + 28)
$sections = @()
$sec = $pe + 24 + $optSize
for ($s = 0; $s -lt $numSections; $s++) {
    $sections += [pscustomobject]@{
        Va = [BitConverter]::ToUInt32($bytes, $sec + 12) + $imageBase
        Size = [BitConverter]::ToUInt32($bytes, $sec + 16)
        Raw = [BitConverter]::ToUInt32($bytes, $sec + 20)
    }
    $sec += 40
}
function Get-FileOffset([uint32]$va) {
    foreach ($s in $sections) {
        if ($va -ge $s.Va -and $va -lt $s.Va + $s.Size) { return [int]($s.Raw + ($va - $s.Va)) }
    }
    return -1
}

# --- Disassemble and patch --------------------------------------------------
$sse1 = @{}
'movaps movups movss movlps movhps movhlps movlhps xorps andps andnps orps addss subss mulss divss sqrtss maxss minss addps subps mulps divps sqrtps maxps minps rcpps rsqrtps rcpss rsqrtss cmpps cmpss comiss ucomiss cvtsi2ss cvtss2si cvttss2si shufps unpcklps unpckhps movmskps ldmxcsr stmxcsr movntps'.Split(' ') | ForEach-Object { $sse1[$_] = $true }
$insRe = [regex]'^\s+([0-9A-F]{8}): (\w+)\s*(.*)$'
$patched = 0
$remaining = @{}
$caveVa = [uint32]0
foreach ($line in [IO.File]::ReadLines($Map)) {
    $m = $symRe.Match($line)
    if ($m.Success -and $m.Groups[1].Value -eq '_XboxSse2Cave') { $caveVa = [Convert]::ToUInt32($m.Groups[2].Value, 16); break }
}
$caveUsed = 0
$caveSize = 512
# x87 truncating conversion of [ebp+disp] into r32; flags and FPU control
# word preserved. Returns the stub's VA.
function Add-TruncateStub([int]$reg, [bool]$disp32, [byte[]]$disp) {
    $code = New-Object System.Collections.Generic.List[byte]
    $code.AddRange([byte[]](0x9C, 0x83, 0xEC, 0x08, 0xD9, 0x3C, 0x24, 0xD9, 0x7C, 0x24, 0x02,
                            0x66, 0x81, 0x4C, 0x24, 0x02, 0x00, 0x0C, 0xD9, 0x6C, 0x24, 0x02))
    if ($disp32) { $code.AddRange([byte[]](0xDD, 0x85)) } else { $code.AddRange([byte[]](0xDD, 0x45)) }
    $code.AddRange($disp)
    $code.AddRange([byte[]](0xDB, 0x5C, 0x24, 0x04, 0xD9, 0x2C, 0x24, 0x8B, [byte](0x44 -bor ($reg -shl 3)), 0x24, 0x04,
                            0x83, 0xC4, 0x08, 0x9D, 0xC3))
    if ($script:caveVa -eq 0 -or $script:caveUsed + $code.Count -gt $script:caveSize) { throw "XboxSse2Cave missing or full" }
    $va = $script:caveVa + $script:caveUsed
    $o = Get-FileOffset $va
    for ($k = 0; $k -lt $code.Count; $k++) { $script:bytes[$o + $k] = $code[$k] }
    $script:caveUsed += $code.Count
    return $va
}

$disasm = & $Dumpbin /nologo /disasm:nobytes $Path
foreach ($line in $disasm) {
    if (-not $line.Contains('xmm') -and -not $line.Contains('cvttsd2si')) { continue }
    $m = $insRe.Match($line)
    if (-not $m.Success) { continue }
    $op = $m.Groups[2].Value
    if ($sse1.ContainsKey($op)) { continue }
    $va = [Convert]::ToUInt32($m.Groups[1].Value, 16)
    $fn = Get-FunctionName $va
    if ($dispatched.IsMatch($fn)) { continue }
    $operands = $m.Groups[3].Value
    $isMem = $operands.Contains('ptr')
    $o = Get-FileOffset $va
    $b0 = $bytes[$o]; $b1 = $bytes[$o + 1]; $b2 = $bytes[$o + 2]
    $ok = $false
    if ($b1 -eq 0x0F) {
        switch ($op) {
            'movlpd' { if ($b0 -eq 0x66 -and ($b2 -eq 0x12 -or $b2 -eq 0x13)) { $bytes[$o] = 0x90; $ok = $true } }
            'movq' {
                if ($isMem -and $b0 -eq 0xF3 -and $b2 -eq 0x7E) { $bytes[$o] = 0x90; $bytes[$o + 2] = 0x12; $ok = $true }
                elseif ($isMem -and $b0 -eq 0x66 -and $b2 -eq 0xD6) { $bytes[$o] = 0x90; $bytes[$o + 2] = 0x13; $ok = $true }
            }
            'movsd' {
                if ($b0 -eq 0xF2 -and ($b2 -eq 0x10 -or $b2 -eq 0x11)) {
                    $bytes[$o] = 0x90
                    if ($isMem) { $bytes[$o + 2] = $b2 + 2 } else { $bytes[$o + 2] = $b2 + 0x18 }
                    $ok = $true
                }
            }
            { $_ -in 'movapd', 'movupd', 'andpd', 'andnpd', 'orpd', 'xorpd' } {
                if ($b0 -eq 0x66) { $bytes[$o] = 0x90; $ok = $true }
            }
            'cvttsd2si' {
                $modrm = $bytes[$o + 3]
                $mod = $modrm -shr 6; $rm = $modrm -band 7; $reg = ($modrm -shr 3) -band 7
                if ($b0 -eq 0xF2 -and $b2 -eq 0x2C -and $rm -eq 5 -and ($mod -eq 1 -or $mod -eq 2)) {
                    $len = if ($mod -eq 2) { 8 } else { 5 }
                    $disp = [byte[]]$bytes[($o + 4)..($o + $len - 1)]
                    $stub = Add-TruncateStub $reg ($mod -eq 2) $disp
                    $rel = [int]([int64]$stub - ([int64]$va + 5))
                    $bytes[$o] = 0xE8
                    [BitConverter]::GetBytes($rel).CopyTo($bytes, $o + 1)
                    for ($k = 5; $k -lt $len; $k++) { $bytes[$o + $k] = 0x90 }
                    $ok = $true
                }
            }
            'movd' {
                if ($isMem -and $b0 -eq 0x66 -and $b2 -eq 0x6E) { $bytes[$o] = 0xF3; $bytes[$o + 2] = 0x10; $ok = $true }
                elseif ($isMem -and $b0 -eq 0x66 -and $b2 -eq 0x7E) { $bytes[$o] = 0xF3; $bytes[$o + 2] = 0x11; $ok = $true }
            }
        }
    }
    if ($ok) { $patched++ } else { $remaining[$fn] = "$($remaining[$fn]) $op".Trim() }
}

[IO.File]::WriteAllBytes($Path, $bytes)
Write-Output "SSE2 -> SSE1 moves patched: $patched ($Path)"
foreach ($k in ($remaining.Keys | Sort-Object)) {
    Write-Output "warning: SSE2 left in $k : $($remaining[$k])"
}
