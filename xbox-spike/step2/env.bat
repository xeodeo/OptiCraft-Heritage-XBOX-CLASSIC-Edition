@echo off
set XDK=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xdk\5849\sdk\XDK\xbox
set TOOLS=C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-tools\artifacts
set MSVC=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
set UCRT_INC=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt
set UCRT_LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x86
set CL17="%MSVC%\bin\Hostx64\x86\cl.exe"
set LINK17="%MSVC%\bin\Hostx64\x86\link.exe"
set CL71="%XDK%\bin\vc71\CL.Exe"
set LINK71="%XDK%\bin\vc71\Link.Exe"
set PATH=%XDK%\bin\vc71;%XDK%\bin;%PATH%
rem Pentium III: SSE only, no SSE2. No security cookies, no FH4, no thread-safe statics (need TLS/kernel support we don't have yet).
set CXX17FLAGS=/nologo /c /std:c++17 /O2 /arch:SSE /GS- /Gy /EHsc /MT /d2FH4- /Zc:threadSafeInit- /D_XBOX /DNDEBUG /I"%MSVC%\include" /I"%UCRT_INC%"
