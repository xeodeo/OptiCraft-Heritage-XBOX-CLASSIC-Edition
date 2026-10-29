# Genera un PARAM.SFO de homebrew (categoria HG) para la PS3.
# Uso: python hacer_sfo.py <salida> <TITLE_ID> "<titulo>"
import struct, sys

def sfo(title_id, title):
    ents = {  # clave: (formato, valor, tamano maximo)
        "APP_VER": (0x0204, "01.00", 8),
        "ATTRIBUTE": (0x0404, 0, 4),
        "BOOTABLE": (0x0404, 1, 4),
        "CATEGORY": (0x0204, "HG", 4),
        "LICENSE": (0x0204, "OptiCraft", 512),
        "PARENTAL_LEVEL": (0x0404, 0, 4),
        "PS3_SYSTEM_VER": (0x0204, "03.5500", 8),
        "RESOLUTION": (0x0404, 0x3F, 4),
        "SOUND_FORMAT": (0x0404, 0x01, 4),
        "TITLE": (0x0204, title, 128),
        "TITLE_ID": (0x0204, title_id, 16),
        "VERSION": (0x0204, "01.00", 8),
    }
    keys = sorted(ents)
    ktab = b""; dtab = b""; idx = b""
    for k in keys:
        fmt, val, mx = ents[k]
        if fmt == 0x0404:
            raw = struct.pack("<I", val)
        else:
            raw = val.encode("utf-8") + b"\0"
        idx += struct.pack("<HHIII", len(ktab), fmt, len(raw), mx, len(dtab))
        ktab += k.encode() + b"\0"
        dtab += raw.ljust(mx, b"\0")
    ktab = ktab.ljust((len(ktab) + 3) & ~3, b"\0")
    kstart = 20 + len(idx)
    return struct.pack("<4sIIII", b"\0PSF", 0x101, kstart, kstart + len(ktab), len(keys)) + idx + ktab + dtab

if __name__ == "__main__":
    open(sys.argv[1], "wb").write(sfo(sys.argv[2], sys.argv[3]))
