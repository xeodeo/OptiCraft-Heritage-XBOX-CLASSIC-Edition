#!/usr/bin/env python3
"""Inserta un bloque de codigo en un fichero fuente SIN pasar el texto por el shell.

Por que existe: meter trazas con heredocs de bash o `python - <<E` convierte el `\\n` de una cadena
de C en un salto de linea real ("missing terminating \" character") y se pierde una compilacion y
una prueba en xemu cada vez. Aqui el bloque se escribe ANTES con la herramienta Write (texto
literal) y este guion solo lo coloca.

Uso:
  python insertar.py <destino> <bloque.txt> --despues "texto ancla"   (tras la linea que lo contiene)
  python insertar.py <destino> <bloque.txt> --antes   "texto ancla"
  python insertar.py <destino> <bloque.txt> --linea N                 (tras la linea N)
  ... --ocurrencia K   si el ancla sale varias veces (1 = la primera); sin ella debe ser unica.

Conserva los finales de linea del destino (CRLF/LF) y se niega si el bloque trae una cadena
partida en dos lineas.
"""
import argparse
import sys


def cadena_partida(linea):
    """True si la linea deja una comilla doble sin cerrar (fuera de comentarios //)."""
    dentro = False
    i = 0
    while i < len(linea):
        c = linea[i]
        if dentro:
            if c == "\\":
                i += 2
                continue
            if c == '"':
                dentro = False
        else:
            if c == "/" and linea[i:i + 2] == "//":
                break
            if c == "'":            # caracter: 'x' o '\x'
                i += 4 if linea[i + 1:i + 2] == "\\" else 3
                continue
            if c == '"':
                dentro = True
        i += 1
    return dentro


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("destino")
    ap.add_argument("bloque")
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--despues")
    g.add_argument("--antes")
    g.add_argument("--linea", type=int)
    ap.add_argument("--ocurrencia", type=int, default=0)
    a = ap.parse_args()

    bloque = open(a.bloque, encoding="utf-8", newline="").read().replace("\r\n", "\n")
    if not bloque.endswith("\n"):
        bloque += "\n"
    for n, l in enumerate(bloque.split("\n"), 1):
        if cadena_partida(l):
            sys.exit(f"ERROR: el bloque tiene una cadena sin cerrar en su linea {n}: {l.strip()[:80]}")

    texto = open(a.destino, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in texto else "\n"
    lineas = texto.replace("\r\n", "\n").split("\n")

    if a.linea is not None:
        if not 0 <= a.linea <= len(lineas):
            sys.exit(f"ERROR: la linea {a.linea} no existe ({len(lineas)} lineas)")
        pos = a.linea
    else:
        ancla = a.despues if a.despues is not None else a.antes
        donde = [i for i, l in enumerate(lineas) if ancla in l]
        if not donde:
            sys.exit(f"ERROR: el ancla no aparece: {ancla!r}")
        if a.ocurrencia:
            if a.ocurrencia > len(donde):
                sys.exit(f"ERROR: solo hay {len(donde)} apariciones del ancla")
            i = donde[a.ocurrencia - 1]
        elif len(donde) > 1:
            sys.exit(f"ERROR: el ancla aparece {len(donde)} veces (lineas {[d + 1 for d in donde[:8]]}); usa --ocurrencia")
        else:
            i = donde[0]
        pos = i + 1 if a.despues is not None else i

    nuevas = bloque.rstrip("\n").split("\n")
    lineas[pos:pos] = nuevas
    open(a.destino, "w", encoding="utf-8", newline="").write(nl.join(lineas))
    print(f"ok: {len(nuevas)} lineas insertadas en {a.destino} tras la linea {pos}")


if __name__ == "__main__":
    main()
