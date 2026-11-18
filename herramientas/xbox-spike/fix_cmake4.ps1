$ErrorActionPreference = 'Stop'
$f2 = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\cmake\xbox.cmake"
$c = [IO.File]::ReadAllText($f2).Replace("`r`n", "`n")
$old = @'
add_custom_target(xbox-data
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/data/assets" "${XBOX_ISO_DIR}/data/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/data/resources" "${XBOX_ISO_DIR}/data/resources"
'@.Replace("`r`n", "`n")
$new = @'
# The game data is not part of the repository; point XBOX_DATA_DIR at a
# folder holding assets/ and resources/ (defaults to <repo>/data).
set(XBOX_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Folder with the game's assets/ and resources/")
add_custom_target(xbox-data
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${XBOX_DATA_DIR}/assets" "${XBOX_ISO_DIR}/data/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${XBOX_DATA_DIR}/resources" "${XBOX_ISO_DIR}/data/resources"
'@.Replace("`r`n", "`n")
if (-not $c.Contains($old)) { throw "xbox-data block not found" }
[IO.File]::WriteAllText($f2, $c.Replace($old, $new))
Write-Output "xbox.cmake: XBOX_DATA_DIR"
