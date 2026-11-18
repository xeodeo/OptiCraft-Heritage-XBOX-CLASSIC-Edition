"""Escucha el log del juego que la Xbox envía por UDP (XBOX_NETLOG_HOST).

Muestra cada línea en vivo y la guarda en netlog.txt.
uso: python escuchar_log.py [puerto]
"""
import socket
import sys
import time

port = int(sys.argv[1]) if len(sys.argv) > 1 else 9999
out = __file__.rsplit("\\", 1)[0] + "\\netlog.txt"
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("0.0.0.0", port))
print(f"escuchando UDP {port} -> {out}", flush=True)
with open(out, "a", encoding="utf-8") as f:
    f.write(f"\n==== {time.strftime('%Y-%m-%d %H:%M:%S')} ====\n")
    while True:
        data, addr = s.recvfrom(4096)
        line = data.decode("latin-1", "replace")
        sys.stdout.write(line)
        sys.stdout.flush()
        f.write(line)
        f.flush()
