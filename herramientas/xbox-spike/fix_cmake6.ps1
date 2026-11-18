$ErrorActionPreference = 'Stop'
$f2 = "C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\cmake\xbox.cmake"
$c = [IO.File]::ReadAllText($f2).Replace("`r`n", "`n")
$anchor = "# Staging data/ into the ISO tree is thousands of copies"
$block = @'
# Optional deploy: copy the finished ISO where the emulator/front end reads it
# (e.g. -DXBOX_DEPLOY_DIR=C:/RetroXeo/roms/xbox). Overwrites the previous copy.
set(XBOX_DEPLOY_DIR "" CACHE PATH "Folder the built OptiCraft.iso is copied to after each build")
if(XBOX_EXTRACT_XISO AND XBOX_DEPLOY_DIR)
    add_custom_command(TARGET OptiCraft POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/bin/xbox/OptiCraft.iso" "${XBOX_DEPLOY_DIR}/OptiCraft.iso"
        COMMENT "Deploying OptiCraft.iso to ${XBOX_DEPLOY_DIR}"
        VERBATIM
    )
endif()

'@.Replace("`r`n", "`n")
if (-not $c.Contains($anchor)) { throw "anchor not found" }
[IO.File]::WriteAllText($f2, $c.Replace($anchor, $block + $anchor))
Write-Output "xbox.cmake: XBOX_DEPLOY_DIR"
