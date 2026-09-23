@echo off
setlocal
call "%~dp0env.bat"
cd /d "%~dp0"
if exist out\iso rmdir /s /q out\iso
mkdir out\iso
powershell -NoProfile -ExecutionPolicy Bypass -File patch_subsystem.ps1 -Path out\spike_c.exe || exit /b 1
imagebld /nologo /IN:out\spike_c.exe /OUT:out\iso\default.xbe /STACK:0x40000 /TESTID:0xFFFF4F43 /TESTREGION:0x80000007 /TESTMEDIATYPES:0x400003FF /TESTNAME:"OptiCraft C++17 Spike" /NOLIBWARN /LIMITMEM || exit /b 1
if exist out\spike_c.iso del out\spike_c.iso
"%TOOLS%\extract-xiso.exe" -c "%~dp0out\iso" "%~dp0out\spike_c.iso" || exit /b 1
echo PACK OK
