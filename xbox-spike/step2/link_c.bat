@echo off
setlocal
call "%~dp0env.bat"
cd /d "%~dp0"
rem Option B: full modern MSVC runtime, XAPI only for Win32 services. All objects built with VS2022.
set INCLUDE=
set MASM32="%MSVC%\bin\Hostx64\x86\ml.exe"
powershell -NoProfile -ExecutionPolicy Bypass -File gen_imports.ps1 || exit /b 1
%MASM32% /nologo /c /coff /Foout\xbox_imports.obj xbox_imports.asm || exit /b 1
%CL17% %CXX17FLAGS% /Foout\xbox_entry.obj xbox_entry.cpp || exit /b 1
%CL17% %CXX17FLAGS% /GR /Foout\game2.obj game2.cpp || exit /b 1
%CL17% %CXX17FLAGS% /Foout\seh_probe.obj seh_probe.cpp || exit /b 1
set INCLUDE=%XDK%\include
%CL71% /nologo /c /O2 /D_XBOX /DNDEBUG /MT /Foout\seh_probe71.obj seh_probe71.cpp || exit /b 1
set INCLUDE=
%CL17% %CXX17FLAGS% /Foout\xbox_crt_shim.obj xbox_crt_shim.cpp || exit /b 1
%CL17% %CXX17FLAGS% /I"%XDK%\include" /Foout\xbox_main17.obj xbox_main.cpp || exit /b 1
%LINK71% /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:XboxEntry /MAP:out\spike_c.map /OUT:out\spike_c.exe out\xbox_entry.obj out\xbox_crt_shim.obj out\xbox_imports.obj out\xbox_main17.obj out\game.obj out\game2.obj out\seh_probe.obj out\seh_probe71.obj %* "%MSVC%\lib\x86\libcmt.lib" "%MSVC%\lib\x86\libcpmt.lib" "%MSVC%\lib\x86\libvcruntime.lib" "%UCRT_LIB%\libucrt.lib" "%MSVC%\lib\x86\legacy_stdio_definitions.lib" "%MSVC%\lib\x86\legacy_stdio_wide_specifiers.lib" "%XDK%\lib\xapilib.lib" "%XDK%\lib\d3d8.lib" "%XDK%\lib\xgraphics.lib" "%XDK%\lib\xboxkrnl.lib" || exit /b 1
echo LINK OK
