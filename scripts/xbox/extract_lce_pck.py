#!/usr/bin/env python3
"""
Extractor for Legacy Console Edition (LCE) DLC skin packs (.pck files).
Follows the format used by 4J Studios in Minecraft.Client Common/DLC/:
  - DLCManager.cpp
  - DLCPack.cpp
  - DLCSkinFile.cpp

Extracts skins (PNG 64x32/64x64), generates 64x32 retro model textures (_32.png)
and 16x32 2D front previews (_Front.png), and writes manifest files (pack.txt).
"""

import os
import sys
import argparse
import struct
import io
import re
from PIL import Image

CURRENT_DLC_VERSION_MIN = 2

# DLC file types from DLCManager.h
TYPE_SKIN = 0
TYPE_CAPE = 1
TYPE_TEXTURE = 2
TYPE_UIDATA = 3
TYPE_PACKCONFIG = 4
TYPE_TEXTUREPACK = 5

def sanitize_filename(name: str) -> str:
    """Creates a clean, cross-platform filename from a skin display name."""
    s = re.sub(r"[^\w\- ]+", "", name).strip()
    s = s.replace(" ", "_")
    return s if s else "skin"

def assemble_front_preview(im: Image.Image) -> Image.Image:
    """
    Assembles a 16x32 front preview image from a 64x32 or 64x64 Minecraft skin.
    Matches assembleFrontPreview() in SkinManager.cpp.
    """
    im = im.convert("RGBA")
    w, h = im.size
    front = Image.new("RGBA", (16, 32), (0, 0, 0, 0))

    # 1. Head front: (8, 8, 16, 16) -> (4, 0)
    head = im.crop((8, 8, 16, 16))
    front.paste(head, (4, 0))

    # 2. Hat/Helmet overlay: (40, 8, 48, 16) -> (4, 0) alpha composite
    hat = im.crop((40, 8, 48, 16))
    front.alpha_composite(hat, (4, 0))

    # 3. Torso front: (20, 20, 28, 32) -> (4, 8)
    torso = im.crop((20, 20, 28, 32))
    front.paste(torso, (4, 8))

    # 4. Right Arm front: (44, 20, 48, 32) -> (0, 8)
    r_arm = im.crop((44, 20, 48, 32))
    front.paste(r_arm, (0, 8))

    # 5. Left Arm front: (12, 8)
    if h == 64:
        l_arm = im.crop((36, 52, 40, 64))
        front.paste(l_arm, (12, 8))
    else:
        l_arm = r_arm.transpose(Image.FLIP_LEFT_RIGHT)
        front.paste(l_arm, (12, 8))

    # 6. Right Leg front: (4, 20, 8, 32) -> (4, 20)
    r_leg = im.crop((4, 20, 8, 32))
    front.paste(r_leg, (4, 20))

    # 7. Left Leg front: (8, 20)
    if h == 64:
        l_leg = im.crop((20, 52, 24, 64))
        front.paste(l_leg, (8, 20))
    else:
        l_leg = r_leg.transpose(Image.FLIP_LEFT_RIGHT)
        front.paste(l_leg, (8, 20))

    return front

def make_retro_32(im: Image.Image) -> Image.Image:
    """
    Converts 64x64 or 64x32 skin to 64x32 retro texture.
    Matches makeRetro32() in SkinManager.cpp.
    """
    im = im.convert("RGBA")
    w, h = im.size
    if h == 32:
        return im

    retro = im.crop((0, 0, 64, 32))
    # Composite modern 64x64 overlays if needed
    torso_ov = im.crop((16, 32, 40, 48))
    retro.alpha_composite(torso_ov, (16, 16))

    r_arm_ov = im.crop((40, 32, 56, 48))
    retro.alpha_composite(r_arm_ov, (40, 16))

    r_leg_ov = im.crop((0, 32, 16, 48))
    retro.alpha_composite(r_leg_ov, (0, 16))

    # Ensure opaque base areas
    retro_data = list(retro.getdata())
    for y in range(32):
        for x in range(64):
            idx = y * 64 + x
            if (y < 16 and x < 32) or (y >= 16):
                r, g, b, _ = retro_data[idx]
                retro_data[idx] = (r, g, b, 255)
    retro.putdata(retro_data)
    return retro

def parse_pck_buffer(data: bytes):
    """
    Parses a PCK byte buffer according to 4J Studios DLC format.
    Returns list of file dicts: {'name', 'type', 'size', 'params', 'data'}.
    """
    if len(data) < 16:
        return []

    # Detect endianness (Xbox 360 / PS3 = Big Endian '>', PC/Durango = Little Endian '<')
    v_be, = struct.unpack_from('>I', data, 0)
    v_le, = struct.unpack_from('<I', data, 0)
    if v_be in (2, 3, 4):
        en = '>'
        enc = 'utf-16be'
        ver = v_be
    elif v_le in (2, 3, 4):
        en = '<'
        enc = 'utf-16le'
        ver = v_le
    else:
        return []

    offset = 4
    n_global_params, = struct.unpack_from(en + 'I', data, offset)
    offset += 4

    global_params = {}
    for _ in range(n_global_params):
        ptype, wch_cnt = struct.unpack_from(en + 'II', data, offset)
        pname_bytes = data[offset + 8 : offset + 8 + wch_cnt * 2]
        pname = pname_bytes.decode(enc, errors='replace').rstrip('\x00')
        global_params[ptype] = pname
        offset += 12 + wch_cnt * 2

    n_files, = struct.unpack_from(en + 'I', data, offset)
    offset += 4

    files = []
    for _ in range(n_files):
        size, ftype, wch_cnt = struct.unpack_from(en + 'III', data, offset)
        fname_bytes = data[offset + 12 : offset + 12 + wch_cnt * 2]
        fname = fname_bytes.decode(enc, errors='replace').rstrip('\x00')
        files.append({'size': size, 'type': ftype, 'name': fname})
        offset += 16 + wch_cnt * 2

    pbTemp = offset
    for fi in files:
        n_file_params, = struct.unpack_from(en + 'I', data, pbTemp)
        pbTemp += 4
        file_params = {}
        for _ in range(n_file_params):
            ptype, wch_cnt = struct.unpack_from(en + 'II', data, pbTemp)
            val_bytes = data[pbTemp + 8 : pbTemp + 8 + wch_cnt * 2]
            val = val_bytes.decode(enc, errors='replace').rstrip('\x00')
            param_name = global_params.get(ptype, f'PARAM_{ptype}')
            file_params[param_name] = val
            pbTemp += 12 + wch_cnt * 2

        fi['params'] = file_params
        fi['data'] = data[pbTemp : pbTemp + fi['size']]
        pbTemp += fi['size']

    all_files = []
    for fi in files:
        if fi['type'] == TYPE_TEXTUREPACK:
            # Child texture pack, recurse
            child_files = parse_pck_buffer(fi['data'])
            all_files.extend(child_files)
        else:
            all_files.append(fi)

    return all_files

def extract_dlc_pack(pack_dir: str, pack_name: str, out_dirs: list[str]):
    """
    Extracts all skins from a DLC pack directory into target out_dirs/skins/packs/<pack_name>/.
    """
    print(f"\n[*] Scanning DLC pack: {pack_name} at {pack_dir}")
    skins_found = []

    # 1. Search for .pck files in pack_dir
    pck_files = []
    for root, _, files in os.walk(pack_dir):
        for f in files:
            if f.lower().endswith(".pck"):
                pck_files.append(os.path.join(root, f))

    for pck_path in pck_files:
        try:
            with open(pck_path, "rb") as f:
                data = f.read()
            files = parse_pck_buffer(data)
            for fi in files:
                if fi['type'] == TYPE_CAPE or 'cape' in fi['name'].lower():
                    continue
                # Is it a skin?
                is_skin = (fi['type'] == TYPE_SKIN) or ('dlcskin' in fi['name'].lower() and fi['name'].lower().endswith('.png'))
                if is_skin and len(fi['data']) > 0:
                    skins_found.append(fi)
        except Exception as e:
            print(f"    [!] Error reading {pck_path}: {e}")

    # 2. Also search for loose PNG skins
    for root, _, files in os.walk(pack_dir):
        for f in files:
            if f.lower().endswith(".png") and "skin" in f.lower() and "cape" not in f.lower():
                # check if not already in skins_found
                full = os.path.join(root, f)
                try:
                    with open(full, "rb") as pf:
                        pdata = pf.read()
                    im = Image.open(io.BytesIO(pdata))
                    if im.size in ((64, 32), (64, 64)):
                        name_base = os.path.splitext(f)[0]
                        skins_found.append({
                            'name': f,
                            'type': TYPE_SKIN,
                            'size': len(pdata),
                            'params': {'DISPLAYNAME': name_base},
                            'data': pdata
                        })
                except Exception:
                    pass

    if not skins_found:
        print(f"    No skins found in pack {pack_name}.")
        return 0

    print(f"    Found {len(skins_found)} skin(s) in {pack_name}.")

    # Process each skin
    processed_skins = []
    seen_safe_names = set()

    for s in skins_found:
        raw_display = s['params'].get('DISPLAYNAME', '')
        if not raw_display:
            raw_display = os.path.splitext(os.path.basename(s['name']))[0]

        # 4J correction for Zap
        if s['name'].endswith('dlcskin00000109.png') or raw_display == 'Zapp':
            raw_display = 'Zap'

        safe_name = sanitize_filename(raw_display)
        # Avoid duplicate filenames in same pack
        if safe_name in seen_safe_names:
            cnt = 2
            while f"{safe_name}_{cnt}" in seen_safe_names:
                cnt += 1
            safe_name = f"{safe_name}_{cnt}"
        seen_safe_names.add(safe_name)

        try:
            im = Image.open(io.BytesIO(s['data'])).convert("RGBA")
            if im.size not in ((64, 32), (64, 64)):
                continue
            im_32 = make_retro_32(im)
            im_front = assemble_front_preview(im)

            processed_skins.append({
                'safe_name': safe_name,
                'display_name': raw_display,
                'img': im,
                'img_32': im_32,
                'img_front': im_front
            })
        except Exception as e:
            print(f"    [!] Failed to process skin image {raw_display}: {e}")

    # Write to each output root
    for out_root in out_dirs:
        pack_dest = os.path.join(out_root, pack_name)
        os.makedirs(pack_dest, exist_ok=True)

        pack_txt_path = os.path.join(pack_dest, "pack.txt")
        manifest_lines = []

        for pskin in processed_skins:
            sname = pskin['safe_name']
            dname = pskin['display_name']

            # 1. Main skin PNG
            main_path = os.path.join(pack_dest, f"{sname}.png")
            pskin['img'].save(main_path, "PNG")

            # 2. Retro 64x32 PNG
            path_32 = os.path.join(pack_dest, f"{sname}_32.png")
            pskin['img_32'].save(path_32, "PNG")

            # 3. 16x32 Front preview PNG
            path_front = os.path.join(pack_dest, f"{sname}_Front.png")
            pskin['img_front'].save(path_front, "PNG")

            manifest_lines.append(f"{sname}|{dname}\n")

        with open(pack_txt_path, "w", encoding="utf-8") as mf:
            mf.writelines(manifest_lines)

        print(f"    [OK] Saved {len(processed_skins)} skins to {pack_dest}")

    return len(processed_skins)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dlc-root", type=str,
                        default=r"C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\minecraft-legacy original\Minecraft.Client\Common\res\TitleUpdate\DLC",
                        help="Root folder containing LCE DLC packs")
    parser.add_argument("--out-dir", type=str, action="append", required=True,
                        help="Output directory (e.g. .../data/assets/skins/packs). Can specify multiple times.")
    args = parser.parse_args()

    dlc_root = os.path.abspath(args.dlc_root)
    if not os.path.isdir(dlc_root):
        print(f"Error: DLC root does not exist: {dlc_root}", file=sys.stderr)
        return 1

    out_dirs = [os.path.abspath(d) for d in args.out_dir]
    for d in out_dirs:
        os.makedirs(d, exist_ok=True)

    print(f"=== LCE Skin Pack Extractor ===")
    print(f"Source: {dlc_root}")
    print(f"Destinations: {out_dirs}")

    # Pack display titles mapping
    PACK_TITLES = {
        "Halo": "Halo",
        "MassEffect": "Mass Effect",
        "Skyrim": "Skyrim",
        "Fantasy": "Fantasy",
        "Candy": "Candy",
        "City": "City",
        "Halloween": "Halloween",
        "Natural": "Natural",
        "Plastic": "Plastic",
    }

    # Iterate over DLC subdirectories
    subdirs = sorted([d for d in os.listdir(dlc_root) if os.path.isdir(os.path.join(dlc_root, d))])
    packs_with_skins = []

    for d in subdirs:
        full_dir = os.path.join(dlc_root, d)
        count = extract_dlc_pack(full_dir, d, out_dirs)
        if count > 0:
            title = PACK_TITLES.get(d, d)
            packs_with_skins.append((d, title, count))

    # Write global packs.txt to each output directory
    for out_root in out_dirs:
        packs_txt = os.path.join(out_root, "packs.txt")
        with open(packs_txt, "w", encoding="utf-8") as f:
            for pdir, ptitle, pcount in packs_with_skins:
                f.write(f"{pdir}|{ptitle}|{pcount}\n")
        print(f"[OK] Wrote global packs manifest: {packs_txt}")

    print(f"\nExtraction complete! Summary:")
    total_skins = 0
    for pdir, ptitle, pcount in packs_with_skins:
        print(f"  - {ptitle} ({pdir}): {pcount} skins")
        total_skins += pcount
    print(f"Total: {total_skins} skins extracted across {len(packs_with_skins)} packs.")

    return 0

if __name__ == "__main__":
    sys.exit(main())
