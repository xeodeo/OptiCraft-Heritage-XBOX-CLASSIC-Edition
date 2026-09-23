@echo off
setlocal
call "%~dp0env.bat"
cd /d "%~dp0"
if not exist out mkdir out
%CL17% %CXX17FLAGS% /Foout\game.obj game.cpp || exit /b 1
set INCLUDE=%XDK%\include
%CL71% /nologo /c /O2 /D_XBOX /DNDEBUG /MT /W3 /Foout\xbox_main.obj xbox_main.cpp || exit /b 1
echo COMPILE OK
