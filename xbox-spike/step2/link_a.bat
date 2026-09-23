@echo off
setlocal
call "%~dp0env.bat"
cd /d "%~dp0"
%LINK71% /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:XBOX /NODEFAULTLIB /ENTRY:mainCRTStartup /LIBPATH:"%XDK%\lib" /OUT:out\spike_a.exe out\xbox_main.obj out\game.obj %* xapilib.lib d3d8.lib xgraphics.lib xboxkrnl.lib libcmt.lib libcpmt.lib
