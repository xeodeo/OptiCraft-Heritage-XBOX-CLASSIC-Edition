@echo off
setlocal
set XDK=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox
set TOOLS=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-tools\artifacts
set PATH=%XDK%\bin\vc71;%XDK%\bin;%PATH%
set INCLUDE=%XDK%\include
set LIB=%XDK%\lib
cd /d "%~dp0"
if exist out rmdir /s /q out
mkdir out\iso

cl /nologo /c /O2 /D_XBOX /DNDEBUG /MT /W3 /Foout\main.obj main.cpp || exit /b 1
link /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:XBOX /OUT:out\spike.exe out\main.obj xapilib.lib d3d8.lib xgraphics.lib xboxkrnl.lib libcmt.lib || exit /b 1
imagebld /nologo /IN:out\spike.exe /OUT:out\iso\default.xbe /STACK:0x10000 /TESTID:0xFFFF4F43 /TESTREGION:0x80000007 /TESTMEDIATYPES:0x400003FF /LIMITMEM /TESTNAME:"OptiCraft Spike" /NOLIBWARN || exit /b 1
"%TOOLS%\extract-xiso.exe" -c "%~dp0out\iso" "%~dp0out\spike.iso" || exit /b 1
echo BUILD OK
