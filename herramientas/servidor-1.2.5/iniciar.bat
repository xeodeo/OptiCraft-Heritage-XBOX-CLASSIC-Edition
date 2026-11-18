@echo off
rem Servidor Minecraft 1.2.5 portable (Java 8 de esta carpeta, sin instalar nada).
cd /d "%~dp0"
"%~dp0jre8\bin\java.exe" -Xms256M -Xmx1G -jar minecraft_server.1.2.5.jar nogui
pause
