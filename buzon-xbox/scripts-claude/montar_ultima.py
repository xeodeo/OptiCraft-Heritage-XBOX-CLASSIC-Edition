"""Monta la release "ultima" para instalar por WiFi (buzon-xbox/130-INSTALAR-POR-WIFI.md).

Lee una carpeta OptiCraft completa (default.xbe + data/ + 1.2.5/ + 1.5.2/ ...) y deja en <salida>:
  - manifest.txt: "<ruta> <bytes> <sha256> <asset>" por archivo (+ "# build <texto>")
  - un archivo por entrada, con nombre plano (las '/' pasan a '__'), listo para subir a la release.
Nada de zip: la consola baja y comprueba archivo por archivo (src/xbox/tools/XboxLauncherWifi.cpp).

uso: python montar_ultima.py <carpeta OptiCraft> <salida> [--build "1.5.2 24-11 abc1234"]
"""
import argparse
import hashlib
import os
import shutil
import sys

SKIP_EXT = {".bak", ".part", ".log", ".tmp"}
SKIP_NAMES = {"thumbs.db", "desktop.ini"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("out")
    ap.add_argument("--build", default="")
    a = ap.parse_args()
    if os.path.exists(a.out):
        shutil.rmtree(a.out)
    os.makedirs(a.out)
    lines = []
    if a.build:
        lines.append("# build " + a.build)
    total = 0
    for root, dirs, files in os.walk(a.src):
        dirs.sort()
        for name in sorted(files):
            if name.lower() in SKIP_NAMES or os.path.splitext(name)[1].lower() in SKIP_EXT:
                continue
            full = os.path.join(root, name)
            rel = os.path.relpath(full, a.src).replace("\\", "/")
            if " " in rel:
                sys.exit(f"ruta con espacio (no admitida por la lista): {rel}")
            asset = rel.replace("/", "__")
            h = hashlib.sha256()
            with open(full, "rb") as f:
                for chunk in iter(lambda: f.read(1 << 20), b""):
                    h.update(chunk)
            size = os.path.getsize(full)
            total += size
            shutil.copyfile(full, os.path.join(a.out, asset))
            lines.append(f"{rel} {size} {h.hexdigest()} {asset}")
    with open(os.path.join(a.out, "manifest.txt"), "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"{len(lines)} lineas, {total / 1048576:.1f} MB -> {a.out}")


if __name__ == "__main__":
    main()
