#!/usr/bin/env python3
"""Create a PS2 disc image from an existing build; never compile or repack assets.

Requires pycdlib (install it in your Python environment). From the repository root:
    python3 scripts/create_ps2_iso.py --dry-run
    python3 scripts/create_ps2_iso.py
    python3 scripts/create_ps2_iso.py --output /path/to/new-image.iso

The default input is bin/ps2/usb/MCBETA, produced by build_linux.sh ps2 or
build_windows.ps1 ps2. Existing output files are never overwritten.
"""

import argparse
import io
from pathlib import Path
import re
import struct
import sys


ROOT = Path(__file__).resolve().parent.parent
BOOT_NAME = "SLUS_420.69"
MODULES = ("audsrv.irx", "ps2dev9.irx", "netman.irx", "smap.irx")


def require_file(path):
    if not path.is_file() or path.stat().st_size == 0:
        raise ValueError(f"Missing or empty file: {path}\nRun a full PS2 build with asset staging first.")
    if path.stat().st_size >= 2**32:
        raise ValueError(f"File exceeds this ISO layout's single-file size limit: {path}")


def validate_elf(path):
    require_file(path)
    with path.open("rb") as stream:
        header = stream.read(52)
    if (len(header) != 52 or header[:6] != b"\x7fELF\x01\x01" or
            struct.unpack_from("<HH", header, 16) != (2, 8)):
        raise ValueError(f"Expected a 32-bit little-endian MIPS executable: {path}")


def validate_pak(path):
    require_file(path)
    size = path.stat().st_size
    with path.open("rb") as stream:
        header = stream.read(32)
        if len(header) != 32:
            raise ValueError(f"Truncated assets archive: {path}")
        magic, version, count, table, names, names_size, _, _ = struct.unpack(">4s7I", header)
        if (magic != b"MCPK" or version != 1 or not 0 < count <= 100000 or
                table < 32 or names < table + count * 16 or
                not 0 < names_size <= 16 * 1024 * 1024 or names + names_size > size):
            raise ValueError(f"Invalid MCPK version 1 archive: {path}")
        stream.seek(table)
        records = stream.read(count * 16)
        stream.seek(names)
        strings = stream.read(names_size)
    keys = set()
    for _, name, offset, length in struct.iter_unpack(">4I", records):
        end = strings.find(b"\0", name)
        if name >= names_size or end < 0 or offset < names + names_size or offset + length > size:
            raise ValueError(f"Invalid entry in assets archive: {path}")
        keys.add(strings[name:end].lower())
    for key in (b"assets/terrain.png", b"resources.manifest"):
        if key not in keys:
            raise ValueError(f"Archive is missing {key.decode()}: {path}\nRebuild with asset staging enabled.")


def disc_name(name):
    upper = name.upper()
    if len(upper) > 29 or not re.fullmatch(r"[A-Z0-9_]+\.[A-Z0-9_]+", upper):
        raise ValueError(f"Name cannot be represented safely on this ISO: {name}")
    return upper


def collect_files(app):
    elf, pak = app / "OptiCraft.elf", app / "assets.pak"
    validate_elf(elf)
    validate_pak(pak)
    files = [(elf, f"/{BOOT_NAME};1"), (pak, "/ASSETS.PAK;1")]
    for name in MODULES:
        module = app / "data" / "irx" / name
        require_file(module)
        files.append((module, f"/DATA/IRX/{disc_name(name)};1"))

    # Disc directory enumeration is unreliable; emit an explicit mod manifest.
    mods = []
    directory = app / "mods"
    if directory.is_dir():
        for path in sorted(directory.iterdir()):
            if path.suffix.lower() != ".ochpack":
                continue
            require_file(path)
            name = disc_name(path.name)
            if name in mods:
                raise ValueError(f"Case-insensitive mod filename collision: {path.name}")
            mods.append(name)
            files.append((path, f"/MODS/{name};1"))
    return files, mods


def write_iso(files, mods, output, video_mode):
    try:
        import pycdlib
    except ImportError as error:
        raise RuntimeError("Missing pycdlib. Install it in your Python environment with: python -m pip install pycdlib") from error

    iso = pycdlib.PyCdlib()
    iso.new(interchange_level=2, vol_ident="MINECRAFT_PS2", sys_ident="PLAYSTATION")
    # Keep generated streams alive until pycdlib has finished writing.
    boot = io.BytesIO(f"BOOT2 = cdrom0:\\{BOOT_NAME};1\r\nVER = 1.00\r\nVMODE = {video_mode}\r\n".encode("ascii"))
    packlist = io.BytesIO(("\r\n".join(mods) + "\r\n").encode("ascii"))
    try:
        iso.add_fp(boot, boot.getbuffer().nbytes, iso_path="/SYSTEM.CNF;1")
        iso.add_directory("/DATA")
        iso.add_directory("/DATA/IRX")
        if mods:
            iso.add_directory("/MODS")
            iso.add_fp(packlist, packlist.getbuffer().nbytes, iso_path="/MODS/PACKLIST.TXT;1")
        for source, destination in files:
            iso.add_file(str(source), iso_path=destination)
        output.parent.mkdir(parents=True, exist_ok=True)
        # Exclusive creation also closes the race with another packaging process.
        with output.open("xb") as stream:
            try:
                iso.write_fp(stream)
            except BaseException:
                stream.close()
                output.unlink()  # Only this invocation's incomplete new image.
                raise
    finally:
        iso.close()
        boot.close()
        packlist.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--app-dir", type=Path, default=ROOT / "bin/ps2/usb/MCBETA")
    parser.add_argument("--output", type=Path, default=ROOT / f"bin/ps2/iso/{BOOT_NAME}.Minecraft_PS2.iso")
    parser.add_argument("--video-mode", choices=("NTSC", "PAL"), default="NTSC",
                        help="SYSTEM.CNF video mode; does not change the ELF's compiled video mode")
    parser.add_argument("--dry-run", action="store_true", help="validate inputs and print layout without writing files or requiring pycdlib")
    args = parser.parse_args()
    # Do not resolve the output itself: exclusive open must reject symlinks too.
    output = args.output.expanduser().absolute()
    if output.suffix.lower() != ".iso":
        raise ValueError("Output filename must end with .iso")
    if output.exists() or output.is_symlink():
        raise ValueError(f"Refusing to overwrite {output}; choose a new --output filename.")
    files, mods = collect_files(args.app_dir.expanduser().resolve())
    print(f"Output: {output}")
    print(f"  /SYSTEM.CNF;1 (generated, {args.video_mode})")
    for source, destination in files:
        print(f"  {destination} <- {source}")
    if mods:
        print("  /MODS/PACKLIST.TXT;1 (generated)")
    print("Only runtime files are included; loose saves, settings and logs are excluded.")
    if args.dry_run:
        print("Inputs validated. No files written.")
        return
    write_iso(files, mods, output, args.video_mode)
    print(f"Created: {output} ({output.stat().st_size / (1024 * 1024):.1f} MiB)")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
