@echo off
setlocal
call "%~dp0env.bat"
cd /d "%~dp0"
rem Old XDK CRT first (owns startup + XAPI init), modern MSVC runtime libs only fill remaining gaps.
%LINK71% /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:XBOX /NODEFAULTLIB /ENTRY:mainCRTStartup /OUT:out\spike_b.exe out\xbox_main.obj out\game.obj %* "%XDK%\lib\xapilib.lib" "%XDK%\lib\d3d8.lib" "%XDK%\lib\xgraphics.lib" "%XDK%\lib\xboxkrnl.lib" "%XDK%\lib\libcmt.lib" "%MSVC%\lib\x86\libcpmt.lib" "%MSVC%\lib\x86\libvcruntime.lib" "%UCRT_LIB%\libucrt.lib"
