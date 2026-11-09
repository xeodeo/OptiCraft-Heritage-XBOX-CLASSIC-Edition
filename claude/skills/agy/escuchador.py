"""Escuchador de agy: avisa solo cuando agy entrega, se atasca o cambia de tarea.

Uso (con el tool Monitor, timeout al maximo y rearmado al expirar):
    python -I escuchador.py --hecho "FASE 2 LISTA [0-9a-f]{7,}" --clave render-nativo [--informe RUTA] [--cada 60]

- Vuelve a listar las terminales EN CADA VUELTA (los handle caducan).
- Elige la terminal de agy (agentIdentity antigravity) cuya pantalla contiene --clave
  (si solo hay una de agy, esa).
- Imprime una linea (= un aviso) solo cuando:
    LISTO      la pantalla muestra el texto de --hecho (o aparece --informe), y sale.
    ATASCADO   permiso pedido, paginador, encuesta, "trust", cuota agotada, error de handle.
    PARADO     esta en el prompt ">" sin trabajar (termino o espera algo).
    AVANCE     lo que dice cambio, como mucho una vez cada --avance segundos (por defecto 600).
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time

ORCA = r"C:\Users\xeodeo\AppData\Local\Programs\orca\resources\bin\orca.exe"
RUIDO = re.compile(r"^[\s─━-]*$|for shortcuts|esc to cancel|Gemini \d|Tip: ")

ap = argparse.ArgumentParser()
ap.add_argument("--hecho", required=True, help="regex que marca la entrega en su pantalla")
ap.add_argument("--clave", default="", help="texto para reconocer SU terminal (carpeta, encargo)")
ap.add_argument("--informe", default="", help="fichero cuya aparicion tambien cuenta como entrega")
ap.add_argument("--handle", default="", help="handle inicial (se mantiene mientras exista)")
ap.add_argument("--cada", type=int, default=60)
ap.add_argument("--avance", type=int, default=600)
ap.add_argument("--agente", default="antigravity", help="agentIdentity de la terminal (antigravity = agy, codex = Codex)")
a = ap.parse_args()
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
hecho = re.compile(a.hecho)


def say(msg):
    print(msg[:300], flush=True)


def orca(*args):
    try:
        out = subprocess.run([ORCA, *args, "--json"], capture_output=True, text=True, timeout=60,
                             encoding="utf-8", errors="replace").stdout
        return json.loads(out)
    except Exception:
        return None


def lineas_de(obj):
    """Todas las listas de strings 'lines' del JSON de terminal read --screen."""
    res = []
    if isinstance(obj, dict):
        for k, v in obj.items():
            if k in ("lines", "tail") and isinstance(v, list) and all(isinstance(x, str) for x in v):
                res.extend(v)
            else:
                res.extend(lineas_de(v))
    elif isinstance(obj, list):
        for v in obj:
            res.extend(lineas_de(v))
    return res


def terminales_agy():
    j = orca("terminal", "list")
    hs = []

    def walk(o):
        if isinstance(o, dict):
            if o.get("agentIdentity") == a.agente and isinstance(o.get("handle"), str):
                hs.append(o["handle"])
            for v in o.values():
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)
    walk(j)
    return hs


prev, ultimo_avance, avisos = "", 0.0, set()
fijo = a.handle
while True:
    if a.informe and os.path.exists(a.informe):
        say(f"[agy] LISTO: existe {a.informe}")
        sys.exit(0)
    pantallas = {}
    for h in terminales_agy():
        ls = lineas_de(orca("terminal", "read", "--terminal", h, "--screen"))
        if ls:
            pantallas[h] = ls
    elegida = fijo if fijo in pantallas else None
    for h, ls in pantallas.items():
        if elegida:
            break
        if a.clave and any(a.clave in x for x in ls):
            elegida = h
            break
    if elegida is None and len(pantallas) == 1:
        elegida = next(iter(pantallas))
    if elegida is None:
        if "sin" not in avisos:
            say(f"[agy] ATASCADO?: no encuentro su terminal ({len(pantallas)} de agy, clave '{a.clave}')")
            avisos.add("sin")
        time.sleep(a.cada)
        continue
    avisos.discard("sin")
    fijo = elegida  # la clave puede salirse de la pantalla: nos quedamos con esta terminal
    ls = pantallas[elegida]
    texto = "\n".join(ls)
    m = hecho.search(texto)
    if m:
        say(f"[agy] LISTO: {m.group(0)}")
        sys.exit(0)
    for patron, nombre in ((r"Requesting permission|Run this command\?", "pide permiso (Enter = si)"),
                           (r"pgup/pgdown", "en el paginador (mandar ESC)"),
                           (r"How's the CLI", "encuesta (mandar 0)"),
                           (r"Do you trust", "pregunta trust (mandar Enter)"),
                           (r"quota reached|Resets in", "sin cuota"),
                           (r"terminal_handle_stale", "handle caducado")):
        if re.search(patron, texto):
            if nombre not in avisos:
                say(f"[agy] ATASCADO: {nombre}")
                avisos.add(nombre)
        else:
            avisos.discard(nombre)
    utiles = [x.strip() for x in ls if not RUIDO.search(x)]
    trabajando = any(re.search(r"esc to cancel|[⣾⣽⣻⢿⡿⣟⣯⣷]|Running|queued messages|task\(s\)|running$", x) for x in ls[-12:])
    if not trabajando and utiles and utiles[-1] == ">":
        if "parado" not in avisos:
            cola = " | ".join(u for u in utiles[-6:-1])
            say(f"[agy] PARADO en el prompt: {cola}")
            avisos.add("parado")
    else:
        avisos.discard("parado")
    cur = " | ".join(utiles[-2:])
    if cur != prev and time.time() - ultimo_avance >= a.avance:
        say(f"[agy] AVANCE: {cur}")
        ultimo_avance = time.time()
        prev = cur
    time.sleep(a.cada)
