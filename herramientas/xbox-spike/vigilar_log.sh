#!/bin/sh
# Every 30 s, dump the game log from the running xemu into vigilado.txt.
MAP="C:/Users/xeodeo/Desktop/Minecraft en xboox clasico/OptiCraftHeritageEdition/bin/xbox/OptiCraft.exe.map"
cd "$(dirname "$0")"
while tasklist | grep -qi xemu.exe; do
  timeout 20 python gdblog.py "$MAP" 20000 > vigilado_tmp.txt 2>&1 && mv vigilado_tmp.txt vigilado.txt
  sleep 30
done
echo "xemu cerrado"
