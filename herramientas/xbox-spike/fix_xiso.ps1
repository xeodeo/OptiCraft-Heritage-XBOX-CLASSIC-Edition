$ErrorActionPreference = 'Stop'
$f2 = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\cmake\xbox.cmake"
$c = [IO.File]::ReadAllText($f2).Replace("`r`n", "`n")

$imgOld = '            "/IN:$<TARGET_FILE:OptiCraft>" "/OUT:${XBOX_XBE}"'
$imgNew = '            "/IN:$<TARGET_FILE:OptiCraft>" "/MAP:$<TARGET_FILE:OptiCraft>.map" "/OUT:${XBOX_XBE}"'
if (-not $c.Contains($imgOld)) { throw "imagebld line not found" }
$c = $c.Replace($imgOld, $imgNew)

$isoOld = '        COMMAND "${XBOX_EXTRACT_XISO}" -c "${XBOX_ISO_DIR}" "${CMAKE_SOURCE_DIR}/bin/xbox/OptiCraft.iso"'
$isoNew = '        COMMAND "${XBOX_EXTRACT_XISO}" -c "${_xbox_iso_dir_native}" "${_xbox_iso_native}"'
if (-not $c.Contains($isoOld)) { throw "xiso line not found" }
$c = $c.Replace($isoOld, $isoNew)

$ifOld = "if(XBOX_EXTRACT_XISO)`n    add_custom_command(TARGET OptiCraft POST_BUILD"
$ifNew = "if(XBOX_EXTRACT_XISO)`n    # extract-xiso mangles forward-slash absolute paths; give it native ones.`n    file(TO_NATIVE_PATH `"`${XBOX_ISO_DIR}`" _xbox_iso_dir_native)`n    file(TO_NATIVE_PATH `"`${CMAKE_SOURCE_DIR}/bin/xbox/OptiCraft.iso`" _xbox_iso_native)`n    add_custom_command(TARGET OptiCraft POST_BUILD"
if (-not $c.Contains($ifOld)) { throw "xiso if block not found" }
$c = $c.Replace($ifOld, $ifNew)

[IO.File]::WriteAllText($f2, $c)
Write-Output "xbox.cmake: imagebld map + native xiso paths"
