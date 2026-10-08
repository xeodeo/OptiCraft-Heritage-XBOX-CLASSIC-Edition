"""Builds Xbox texture pack folders from a Minecraft Legacy Console Edition tree.

    python make_legacy_texture_packs.py <legacy root> <output texturepacks dir>

<legacy root> is the folder holding Minecraft.Client. The output gets one
folder per pack, in the layout the game reads from D:\\texturepacks\\<name>\\
(terrain.png, gui/items.png, mob/..., pack.txt, pack.png):

  * "Legacy Console": the console edition's own textures (TitleUpdate first,
    then the base res folder).
  * one folder per DLC pack (TitleUpdate/DLC/<name>/Data/x16Data.pck or
    x32Data.pck). x32 packs are halved to 16x16 tiles: the game animates water,
    lava and fire in 16x16 tiles, and it keeps the RAM cost of the default pack.

Only textures are taken; sounds, UI movies and game rules are skipped.

The .pck format (4J's DLC_Creator, Minecraft.Client/Common/DLC/DLCManager.cpp,
big-endian on the Xbox 360):
    u32 version (3)
    u32 parameter type count, then per type: u32 id, u32 wchar count,
        UTF-16 name, padded to the 12-byte struct (wchar[1] + pad)
    u32 file count, then per file: u32 size, u32 type, u32 wchar count,
        UTF-16 name (struct of 16 bytes + extra wchars)
    then per file, in order: u32 parameter count, the parameters, the data.
"""
import io
import os
import shutil
import struct
import sys

from PIL import Image

# Paths taken from a pack, relative to its res/ folder.
# gui/ stays the game's own (menus, HUD): only gui/items.png is taken.
WANTED_DIRS = ("mob", "item", "armor", "art", "misc", "environment")
WANTED_FILES = ("terrain.png", "items.png", "particles.png", "pack.png")


def pck_files(data):
    """Yields (name, bytes) for every file in a .pck blob, recursing into
    embedded texture packs."""
    pos = 0

    def u32():
        nonlocal pos
        value = struct.unpack_from(">I", data, pos)[0]
        pos += 4
        return value

    version = u32()
    if version < 3:
        return
    for _ in range(u32()):
        u32()                      # id
        count = u32()
        pos += 4 + count * 2       # wchar[1] + pad, then the extra wchars

    file_count = u32()
    entries = []
    for _ in range(file_count):
        size = u32()
        ftype = u32()
        count = u32()
        name = data[pos:pos + count * 2].decode("utf-16-be")
        pos += 4 + count * 2
        entries.append((size, ftype, name))

    for size, ftype, name in entries:
        for _ in range(u32()):     # this file's parameters
            u32()
            count = u32()
            pos += 4 + count * 2
        blob = data[pos:pos + size]
        pos += size
        if size > 16 and blob[:4] == b"\x00\x00\x00\x03":
            # An embedded texture pack (e_DLCType_TexturePack): a .pck of its own.
            yield from pck_files(blob)
        else:
            yield name, blob


def target_path(name):
    """Maps a pack file name to our layout, or None to skip it."""
    name = name.replace("\\", "/")
    if name.startswith("res/"):
        name = name[4:]
    lower = name.lower()
    if lower in ("items.png", "gui/items.png"):
        return "gui/items.png"
    if lower in WANTED_FILES:
        return lower
    top = lower.split("/", 1)[0]
    if top in WANTED_DIRS and lower.endswith(".png"):
        return name
    return None


def save_png(blob, out_file, scale):
    os.makedirs(os.path.dirname(out_file), exist_ok=True)
    image = Image.open(io.BytesIO(blob))
    image.load()
    if scale != 1:
        image = image.convert("RGBA")
        image = image.resize((max(1, image.width // scale), max(1, image.height // scale)), Image.BOX)
    image.save(out_file)


def write_pack_txt(folder, first, second):
    with open(os.path.join(folder, "pack.txt"), "w", encoding="utf-8") as f:
        f.write(first + "\n" + second + "\n")


def build_dlc(pck_path, out_dir, name):
    with open(pck_path, "rb") as f:
        data = f.read()
    files = {}
    for fname, blob in pck_files(data):
        target = target_path(fname)
        if target is not None and blob[:8] == b"\x89PNG\r\n\x1a\n":
            files[target] = blob
    if "terrain.png" not in files:
        print(f"  {name}: no terrain.png, skipped")
        return
    terrain = Image.open(io.BytesIO(files["terrain.png"]))
    scale = max(1, terrain.width // 256)
    folder = os.path.join(out_dir, name)
    shutil.rmtree(folder, ignore_errors=True)
    for target, blob in files.items():
        save_png(blob, os.path.join(folder, target), scale)
    write_pack_txt(folder, f"{name} (Legacy DLC)", f"x{16 * scale} reduced to x16" if scale > 1 else "x16")
    print(f"  {name}: {len(files)} textures, terrain {terrain.width}px, scale 1/{scale}")


def copy_tree_pngs(src_root, folder, done):
    for dirpath, _, filenames in os.walk(src_root):
        for fn in filenames:
            rel = os.path.relpath(os.path.join(dirpath, fn), src_root).replace("\\", "/")
            target = target_path(rel)
            if target is None or target in done or not fn.lower().endswith(".png"):
                continue
            with open(os.path.join(dirpath, fn), "rb") as f:
                save_png(f.read(), os.path.join(folder, target), 1)
            done.add(target)


def build_base(res_root, out_dir):
    folder = os.path.join(out_dir, "Legacy Console")
    shutil.rmtree(folder, ignore_errors=True)
    done = set()
    # Newest first: the title update replaces the launch textures.
    for src in (os.path.join(res_root, "TitleUpdate", "res"), res_root):
        if os.path.isdir(src):
            copy_tree_pngs(src, folder, done)
    write_pack_txt(folder, "Legacy Console Edition", "Xbox 360 textures")
    print(f"  Legacy Console: {len(done)} textures")


def main():
    legacy_root, out_dir = sys.argv[1], sys.argv[2]
    res_root = os.path.join(legacy_root, "Minecraft.Client", "Common", "res")
    os.makedirs(out_dir, exist_ok=True)
    build_base(res_root, out_dir)
    dlc_root = os.path.join(res_root, "TitleUpdate", "DLC")
    for name in sorted(os.listdir(dlc_root)) if os.path.isdir(dlc_root) else []:
        data_dir = os.path.join(dlc_root, name, "Data")
        for pck in ("x16Data.pck", "x32Data.pck"):
            path = os.path.join(data_dir, pck)
            if os.path.isfile(path):
                build_dlc(path, out_dir, name)
                break


if __name__ == "__main__":
    main()
