# Post-link fixups that turn a VS2022-linked PE into valid imagebld input.
#
# 1. Subsystem -> IMAGE_SUBSYSTEM_XBOX (14). The image is linked as WINDOWS so
#    the linker keeps 4 KB section alignment (the modern CRT has 64-byte
#    aligned sections the XBOX subsystem rejects); imagebld requires XBOX.
#
# 2. Static TLS base. Modern MSVC hard-codes the Win32 TEB slot fs:[2Ch]
#    (ThreadLocalStoragePointer) for thread_local / thread-safe statics. On the
#    Xbox, XAPI keeps the TLS array at fs:[04h] (the XDK defines
#    __tls_array = 4), so every "mov r32, fs:[2Ch]" in executable sections is
#    rewritten to "mov r32, fs:[04h]" (same length).
param([Parameter(Mandatory = $true)][string]$Path, [int]$ExpectedTls = -1)
$bytes = [IO.File]::ReadAllBytes($Path)
$pe = [BitConverter]::ToInt32($bytes, 0x3C)
if ([BitConverter]::ToUInt32($bytes, $pe) -ne 0x00004550) { throw "not a PE file: $Path" }
$numSections = [BitConverter]::ToUInt16($bytes, $pe + 6)
$optSize = [BitConverter]::ToUInt16($bytes, $pe + 20)
$opt = $pe + 24
if ([BitConverter]::ToUInt16($bytes, $opt) -ne 0x10B) { throw "not PE32: $Path" }

$bytes[$opt + 68] = 14
$bytes[$opt + 69] = 0

$patched = 0
$sec = $opt + $optSize
for ($s = 0; $s -lt $numSections; $s++) {
    if ($s -gt 0) { $sec += 40 }
    $chars = [BitConverter]::ToUInt32($bytes, $sec + 36)
    if (($chars -band 0x20000000) -eq 0) { continue }  # IMAGE_SCN_MEM_EXECUTE
    $rawSize = [BitConverter]::ToInt32($bytes, $sec + 16)
    $rawPtr = [BitConverter]::ToInt32($bytes, $sec + 20)
    $end = $rawPtr + $rawSize - 7
    for ($i = $rawPtr; $i -le $end; $i++) {
        if ($bytes[$i] -ne 0x64) { continue }
        $b1 = $bytes[$i + 1]
        $dispAt = -1
        if ($b1 -eq 0xA1) {
            $dispAt = $i + 2                                   # mov eax, fs:[disp32]
        } elseif ($b1 -eq 0x8B -and (($bytes[$i + 2] -band 0xC7) -eq 0x05)) {
            $dispAt = $i + 3                                   # mov r32, fs:[disp32]
        }
        if ($dispAt -lt 0) { continue }
        if ($bytes[$dispAt] -eq 0x2C -and $bytes[$dispAt + 1] -eq 0 -and $bytes[$dispAt + 2] -eq 0 -and $bytes[$dispAt + 3] -eq 0) {
            $bytes[$dispAt] = 0x04
            $patched++
        }
    }
}

if ($ExpectedTls -ge 0 -and $patched -ne $ExpectedTls) {
    throw "TLS patch count $patched != expected $ExpectedTls"
}
[IO.File]::WriteAllBytes($Path, $bytes)
Write-Output "subsystem -> XBOX, TLS fs:[2Ch] -> fs:[04h] patched: $patched ($Path)"
