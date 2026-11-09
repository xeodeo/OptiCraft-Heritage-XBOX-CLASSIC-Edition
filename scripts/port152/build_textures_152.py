#!/usr/bin/env python3
"""Stage Minecraft 1.5.2 textures, legacy atlases, and an Xbox assets.pak.

The source PNGs live beside the repository in mcp-diff/src152 and are copied
only into the ignored build tree. Existing atlas cells that are pixel-identical
to a 1.5.2 icon keep their index; modern icons that are new or changed fill the
remaining cells. This preserves the current renderer's 16x16 UV/index rules.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import shutil
import sys
from pathlib import Path

from PIL import Image, ImageChops


CELL = 16
ATLAS_SIZE = 256
CELL_COUNT = (ATLAS_SIZE // CELL) ** 2
SKIP_CONSOLE_GUI = {
    "gui/gui.png",
    "gui/icons.png",
    "gui/inventory.png",
}
# These legacy slots still have active users, but 1.5.2 changed the artwork or
# stores the texture as an animation strip rather than a 16x16 icon.
BLOCK_SLOT_OVERRIDES = {
    14: "portal.png", 19: "gravel.png", 31: "fire_0.png", 47: "fire_1.png",
    # Redrawn in 1.5.2 past the near-match threshold, same blocks as 1.2.5:
    # BlockSapling meta 3, BlockStem (111, bent +16), BlockEndPortalFrame
    # side and eye, BlockLeaves jungle (52 + 144).
    30: "sapling_jungle.png", 111: "stem_straight.png", 127: "stem_bent.png",
    159: "endframe_side.png", 174: "endframe_eye.png", 196: "leaves_jungle.png",
    # Still water is one cell; flowing water and lava are 2x2 (TextureWaterFlowFX
    # and TextureLavaFlowFX use tileSize 2 at 206 and 238).
    205: "water.png", 237: "lava.png",
    206: ("water_flow.png", 0, 0), 207: ("water_flow.png", 1, 0),
    222: ("water_flow.png", 0, 1), 223: ("water_flow.png", 1, 1),
    238: ("lava_flow.png", 0, 0), 239: ("lava_flow.png", 1, 0),
    254: ("lava_flow.png", 0, 1), 255: ("lava_flow.png", 1, 1),
}
# The same legacy tile repeated this many times is the atlas filler, not a texture.
PLACEHOLDER_REPEATS = 8
# Mean per-channel difference (0-255) under which a changed 1.5.2 icon is taken
# as the same texture redrawn; above it the cell keeps its 1.2.5 pixels.
NEAR_MATCH_DIFF = 3.0
LEGACY_TILE = "<1.2.5 tile>"
# Old chest faces: 1.5.2 draws chests only as models and gives the chest and the
# locked chest the planks icon (BlockChest.cpp / BlockLockedChest.cpp), so these
# cells go to new icons.
FREED_BLOCK_CELLS = {25, 26, 27, 41, 42, 57, 58}
# Item cells whose icon comes from metadata or item state, not setIconCoord.
DYE_COLORS = ["black", "red", "green", "brown", "blue", "purple", "cyan", "silver",
              "gray", "pink", "lime", "yellow", "lightBlue", "magenta", "orange", "white"]
ITEM_STATE_OVERRIDES = {
    101: "bow_pull_0.png", 117: "bow_pull_1.png", 133: "bow_pull_2.png",
    85: "fishingRod_empty.png",
    140: "potion.png", 141: "potion_contents.png", 154: "potion_splash.png",
    169: "monsterPlacer_overlay.png",
}
OVERFLOW: dict[str, list[str]] = {}
# 1.5.2 leather armour is grey and dyed at draw time (ItemArmor.getColor, default
# 10511680 = 0xA06540), with an undyed overlay on top. The 1.2.5 renderer draws
# it as it is, so it is baked brown here: the item icons and the worn texture.
LEATHER_COLOR = (0xA0, 0x65, 0x40)
LEATHER_ICONS = ["helmetCloth", "chestplateCloth", "leggingsCloth", "bootsCloth"]
LEATHER_ARMOR = ["armor/cloth_1", "armor/cloth_2"]


def bake_leather(base: Image.Image, overlay_path: Path | None) -> Image.Image:
    base = base.convert("RGBA")
    r, g, b, a = base.split()
    r = r.point(lambda v: v * LEATHER_COLOR[0] // 255)
    g = g.point(lambda v: v * LEATHER_COLOR[1] // 255)
    b = b.point(lambda v: v * LEATHER_COLOR[2] // 255)
    baked = Image.merge("RGBA", (r, g, b, a))
    if overlay_path is not None and overlay_path.exists():
        with Image.open(overlay_path) as overlay:
            baked = Image.alpha_composite(baked, overlay.convert("RGBA").resize(baked.size))
    return baked
# Group 1 of the port (buzon-xbox 44/46): these must get a cell before the rest.
PRIORITY_ICONS = [
    "oreEmerald.png", "blockEmerald.png", "blockRedstone.png", "netherquartz.png",
    "quartzblock_bottom.png", "quartzblock_chiseled.png", "quartzblock_chiseled_top.png",
    "quartzblock_lines.png", "quartzblock_lines_top.png", "quartzblock_side.png", "quartzblock_top.png",
    "carrots_0.png", "carrots_1.png", "carrots_2.png", "carrots_3.png",
    "potatoes_0.png", "potatoes_1.png", "potatoes_2.png", "potatoes_3.png",
    "flowerPot.png", "cocoa_0.png", "cocoa_1.png", "cocoa_2.png",
    "anvil_base.png", "anvil_top.png", "anvil_top_damaged_1.png", "anvil_top_damaged_2.png", "beacon.png",
]


def image_key(image: Image.Image) -> bytes:
    return image.convert("RGBA").tobytes()


def icon_diff(a: Image.Image, b: Image.Image) -> float:
    data = ImageChops.difference(a, b).tobytes()
    return sum(data) / len(data)


def load_quadrant(path: Path, qx: int, qy: int) -> Image.Image | None:
    # First 32x32 frame of a 2x2 animation strip, one 16x16 quarter of it.
    with Image.open(path) as image:
        if image.width != 2 * CELL or image.height < 2 * CELL:
            return None
        x, y = qx * CELL, qy * CELL
        return image.crop((x, y, x + CELL, y + CELL)).convert("RGBA")


def load_icon(path: Path) -> Image.Image | None:
    with Image.open(path) as image:
        if image.size == (CELL, CELL):
            return image.convert("RGBA")
        # The source animation strips are vertical, one 16x16 frame per row.
        if image.width == CELL and image.height >= CELL and image.height % CELL == 0:
            return image.crop((0, 0, CELL, CELL)).convert("RGBA")
    return None


def find_named_icon(directory: Path, name: str) -> Path | None:
    wanted = name.casefold()
    return next((p for p in sorted(directory.glob("*.png")) if p.name.casefold() == wanted), None)


def build_atlas(source_dir: Path, old_atlas_path: Path, overrides: dict[int, str], reserved: set[int]) -> tuple[Image.Image, dict[str, int], dict[int, str], list[str], int]:
    source_files = sorted(source_dir.glob("*.png"), key=lambda p: p.name.casefold())
    icons: dict[str, Image.Image] = {}
    aliases: dict[bytes, list[str]] = {}
    for path in source_files:
        icon = load_icon(path)
        if icon is None:
            continue
        key = image_key(icon)
        icons[path.name] = icon
        aliases.setdefault(key, []).append(path.name)

    if source_dir.name == "items":
        for stem in LEATHER_ICONS:
            if f"{stem}.png" in icons:
                icons[f"{stem}.png"] = bake_leather(icons[f"{stem}.png"], source_dir / f"{stem}_overlay.png")
        aliases.clear()
        for name, icon in icons.items():
            aliases.setdefault(image_key(icon), []).append(name)

    old = Image.open(old_atlas_path).convert("RGBA")
    if old.size != (ATLAS_SIZE, ATLAS_SIZE):
        raise ValueError(f"Legacy atlas must be 256x256: {old_atlas_path} is {old.size}")
    atlas = Image.new("RGBA", (ATLAS_SIZE, ATLAS_SIZE), (0, 0, 0, 0))
    name_to_index: dict[str, int] = {}
    index_to_name: dict[int, str] = {}

    # Preserve positions for all exact matches, but paint only 1.5.2 pixels.
    exact_match_count = 0
    for index in range(CELL_COUNT):
        x, y = index % 16 * CELL, index // 16 * CELL
        legacy_tile = old.crop((x, y, x + CELL, y + CELL))
        names = aliases.get(image_key(legacy_tile))
        if names:
            chosen = names[0]
            atlas.paste(icons[chosen], (x, y))
            index_to_name[index] = chosen
            exact_match_count += 1

    # Explicit modern replacements for active legacy slots whose source art
    # changed or became animated in 1.5.2.
    for index, spec in overrides.items():
        filename, quadrant = (spec, None) if isinstance(spec, str) else (spec[0], spec[1:])
        path = find_named_icon(source_dir, filename)
        if path is None:
            raise FileNotFoundError(f"Required 1.5.2 atlas replacement missing: {filename}")
        if quadrant is None:
            # The loaded set, so baked icons (leather) are the ones placed.
            icon = icons.get(path.name) or load_icon(path)
        else:
            icon = load_quadrant(path, *quadrant)
        if icon is None:
            raise ValueError(f"Cannot extract a 16x16 frame from {path}")
        x, y = index % 16 * CELL, index // 16 * CELL
        atlas.paste(icon, (x, y))
        index_to_name[index] = path.name

    # Every other cell that had art in 1.2.5 may still be read by code that
    # computes its index (fluids, metadata offsets, item states), so it keeps
    # its slot: a near-identical 1.5.2 icon if there is one, otherwise its
    # 1.2.5 pixels. Only empty cells and the repeated placeholder tile are
    # handed out to new icons.
    tiles = {}
    for index in range(CELL_COUNT):
        x, y = index % 16 * CELL, index // 16 * CELL
        tiles[index] = old.crop((x, y, x + CELL, y + CELL))
    repeats: dict[bytes, int] = {}
    for tile in tiles.values():
        if tile.getbbox() is not None:
            repeats[image_key(tile)] = repeats.get(image_key(tile), 0) + 1
    placeholders = {index for index, tile in tiles.items()
                    if tile.getbbox() is not None and repeats[image_key(tile)] >= PLACEHOLDER_REPEATS}
    for index, tile in tiles.items():
        if index in index_to_name or tile.getbbox() is None or index in placeholders:
            continue
        if source_dir.name == "blocks" and index in FREED_BLOCK_CELLS:
            continue
        x, y = index % 16 * CELL, index // 16 * CELL
        best_name, best_diff = min(((name, icon_diff(tile, icon)) for name, icon in icons.items()),
                                   key=lambda pair: pair[1])
        if best_diff <= NEAR_MATCH_DIFF:
            atlas.paste(icons[best_name], (x, y))
            index_to_name[index] = best_name
        else:
            atlas.paste(tile, (x, y))
            index_to_name[index] = LEGACY_TILE
    used = set(index_to_name)
    free = iter(index for index in range(CELL_COUNT) if index not in used and index not in reserved)
    image_to_index = {image_key(icons[name]): index for index, name in index_to_name.items() if name in icons}
    # One image can have several filenames. Give every filename the same slot.
    # Icons of blocks already being ported go first: the free cells run out.
    first = [name for name in PRIORITY_ICONS if name in icons]
    order = first + [name for name in icons if name not in first]
    for path_name in order:
        icon = icons[path_name]
        key = image_key(icon)
        index = image_to_index.get(key)
        if index is None:
            index = next(free, None)
            if index is None:
                # No room left: the icon stays out of this atlas and out of the
                # header, so code asking for it by name gets -1.
                OVERFLOW.setdefault(source_dir.name, []).append(path_name)
                continue
            x, y = index % 16 * CELL, index // 16 * CELL
            atlas.paste(icon, (x, y))
            index_to_name[index] = path_name
            image_to_index[key] = index
        name_to_index[path_name] = index

    if len(image_to_index) > CELL_COUNT:
        raise ValueError(f"Atlas overflow: {len(image_to_index)} icons for {CELL_COUNT} cells")
    unmatched_old = []
    for index in range(CELL_COUNT):
        x, y = index % 16 * CELL, index // 16 * CELL
        tile = old.crop((x, y, x + CELL, y + CELL))
        if tile.getbbox() is not None and index not in index_to_name:
            unmatched_old.append(index)
    return atlas, name_to_index, index_to_name, unmatched_old, exact_match_count


def legacy_block_slots(block_cpp: Path) -> set[int]:
    slots: set[int] = set()
    if not block_cpp.exists():
        return slots
    constructor = re.compile(r"\bnew\s+\w+\s*\(\s*\d+\s*,\s*(\d+)")
    for line in block_cpp.read_text(encoding="utf-8").splitlines():
        match = constructor.search(line)
        if match:
            slots.add(int(match.group(1)))
    return slots


def legacy_item_overrides(item_cpp: Path, source_dir: Path) -> tuple[dict[int, str], set[int]]:
    overrides: dict[int, str] = {}
    reserved: set[int] = set()
    if not item_cpp.exists():
        return overrides, reserved
    line_pattern = re.compile(
        r"\b(\w+)\s*=\s*\(new\s+\w+\s*\(.*?(?:\.|->)setIconCoord\(\s*(\d+)\s*,\s*(\d+)\s*\).*?(?:\.|->)setItemName\(\"([^\"]+)\"\)"
    )
    for line in item_cpp.read_text(encoding="utf-8").splitlines():
        match = line_pattern.search(line)
        if not match:
            continue
        variable, x, y, item_name = match.groups()
        index = int(x) + 16 * int(y)
        reserved.add(index)
        candidates = [f"{item_name}.png", f"{variable}.png"]
        if item_name == "dyePowder":
            candidates = ["dyePowder_white.png"]
        if variable == "diamond":
            # 1.2.5 names the diamond item "emerald" (its old language key).
            candidates = ["diamond.png"]
        for candidate in candidates:
            source = find_named_icon(source_dir, candidate)
            if source is not None:
                previous = overrides.get(index)
                if previous is not None and previous.casefold() != source.name.casefold():
                    raise ValueError(f"Conflicting 1.5.2 item replacements for legacy slot {index}")
                overrides[index] = source.name
                break
    # The old engine has a family of record item names sharing one Java name.
    # Their registration variables identify the corresponding 1.5.2 texture.
    for index in reserved:
        if index in overrides:
            continue
        for line in item_cpp.read_text(encoding="utf-8").splitlines():
            match = line_pattern.search(line)
            if not match:
                continue
            variable, x, y, _ = match.groups()
            if int(x) + 16 * int(y) != index:
                continue
            source = find_named_icon(source_dir, f"{variable}.png")
            if source is not None:
                overrides[index] = source.name
                break
    # ItemDye::getIconFromDamage: base + (meta % 8) * 16 + meta / 8.
    dye = re.search(r"dyePowder\s*=.*?setIconCoord\(\s*(\d+)\s*,\s*(\d+)\s*\)", item_cpp.read_text(encoding="utf-8"))
    if dye:
        base = int(dye.group(1)) + 16 * int(dye.group(2))
        for meta, color in enumerate(DYE_COLORS):
            index = base + (meta % 8) * 16 + meta // 8
            reserved.add(index)
            overrides[index] = f"dyePowder_{color}.png"
    for index, name in ITEM_STATE_OVERRIDES.items():
        reserved.add(index)
        overrides[index] = name
    return overrides, reserved


def cpp_identifier(name: str) -> str:
    stem = re.sub(r"\.png$", "", name, flags=re.IGNORECASE)
    parts = re.findall(r"[A-Za-z0-9]+", stem)
    ident = "".join(part[:1].upper() + part[1:] for part in parts) or "Unnamed"
    if ident[0].isdigit():
        ident = "Icon" + ident
    return ident


def write_header(path: Path, blocks: dict[str, int], items: dict[str, int]) -> None:
    lines = [
        "// Generated by scripts/port152/build_textures_152.py; do not edit.",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace Texture152 {",
        "struct IconIndex { const char* name; std::uint16_t index; };",
        "",
    ]
    for label, mapping in (("Block", blocks), ("Item", items)):
        seen: set[str] = set()
        lines.append(f"inline constexpr IconIndex k{label}Icons[] = {{")
        for name, index in sorted(mapping.items(), key=lambda item: (item[1], item[0].casefold())):
            ident = cpp_identifier(name)
            if ident not in seen:
                lines.append(f"    {{\"{name}\", {index}}},")
                seen.add(ident)
        lines.append("};")
        lines.append(f"inline constexpr int k{label}IconCount = sizeof(k{label}Icons) / sizeof(k{label}Icons[0]);")
        lines.append(f"inline constexpr std::uint16_t k{label}AtlasWidth = 256;")
        lines.append(f"inline constexpr std::uint16_t k{label}AtlasHeight = 256;")
        lines.append("")
    lines += [
        "// Atlas index of a 1.5.2 texture by file name (\"oreEmerald.png\"), or -1.",
        "inline constexpr bool sameName(const char* a, const char* b)",
        "{",
        "    while (*a && *a == *b) { ++a; ++b; }",
        "    return *a == *b;",
        "}",
        "inline constexpr int findIcon(const IconIndex* icons, int count, const char* name)",
        "{",
        "    for (int i = 0; i < count; ++i)",
        "        if (sameName(icons[i].name, name)) return icons[i].index;",
        "    return -1;",
        "}",
        "inline constexpr int blockIcon(const char* name) { return findIcon(kBlockIcons, kBlockIconCount, name); }",
        "inline constexpr int itemIcon(const char* name) { return findIcon(kItemIcons, kItemIconCount, name); }",
        "",
        "// Compile-time only: use these in game code, so a lookup never runs per face",
        "// and a name missing from the atlas is a build error.",
        "template <int Index> struct CheckedIcon",
        "{",
        "    static_assert(Index >= 0, \"1.5.2 icon missing from the atlas\");",
        "    static constexpr int value = Index;",
        "};",
        "#define TEX152_BLOCK_ICON(name) (::Texture152::CheckedIcon<::Texture152::blockIcon(name)>::value)",
        "#define TEX152_ITEM_ICON(name) (::Texture152::CheckedIcon<::Texture152::itemIcon(name)>::value)",
        "",
        "} // namespace Texture152", "",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")


def parse_block_report(block_cpp: Path, index_to_icon: dict[int, str]) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    if not block_cpp.exists():
        return rows
    pattern = re.compile(r"\b(\w+)\s*=\s*\(new\s+\w+\s*\(\s*(\d+)\s*,\s*(\d+)")
    for line_no, line in enumerate(block_cpp.read_text(encoding="utf-8").splitlines(), 1):
        match = pattern.search(line)
        if not match:
            continue
        var, block_id, index = match.group(1), int(match.group(2)), int(match.group(3))
        label = re.search(r'->setBlockName\("([^"]+)"\)', line)
        rows.append({
            "id": block_id,
            "block": label.group(1) if label else var,
            "legacyIndex": index,
            "icon152": index_to_icon.get(index, "NO DIRECT ATLAS MATCH"),
            "source": f"src/net/minecraft/src/Block.cpp:{line_no}",
        })
    return rows


def copy_source_textures(source_root: Path, assets: Path, staged: list[str]) -> None:
    for kind in ("blocks", "items"):
        for path in sorted((source_root / "textures" / kind).glob("*.png"), key=lambda p: p.name.casefold()):
            dest = assets / "textures" / kind / path.name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dest)
            staged.append(dest.relative_to(assets.parent).as_posix())

    old_root = source_root.parent / "src125"
    old_files: dict[str, Path] = {}
    if old_root.exists():
        for path in old_root.rglob("*.png"):
            old_files[path.relative_to(old_root).as_posix().casefold()] = path
    for path in sorted(source_root.rglob("*.png"), key=lambda p: p.relative_to(source_root).as_posix().casefold()):
        rel = path.relative_to(source_root).as_posix()
        key = rel.casefold()
        if key.startswith("textures/"):
            continue
        if key in SKIP_CONSOLE_GUI:
            continue
        old_path = old_files.get(key)
        cropped = None
        if old_path is not None:
            with Image.open(path) as new_image, Image.open(old_path) as old_image:
                if new_image.size == old_image.size and image_key(new_image) == image_key(old_image):
                    continue
                if new_image.size != old_image.size:
                    # The 1.2.5 models keep their UV layout: 1.5.2 grew some
                    # textures downwards (zombie 64x64 carries the zombie
                    # villager underneath), so only the old area is usable.
                    if new_image.width == old_image.width and new_image.height > old_image.height:
                        cropped = new_image.crop((0, 0, old_image.width, old_image.height)).convert("RGBA")
                    else:
                        print(f"keeping 1.2.5 {rel}: 1.5.2 size {new_image.size} != {old_image.size}")
                        continue
        dest = assets / Path(rel)
        dest.parent.mkdir(parents=True, exist_ok=True)
        if key[:-4] in LEATHER_ARMOR:
            with Image.open(path) as image:
                cropped = bake_leather(image, path.with_name(path.stem + "_b.png"))
        if cropped is not None:
            cropped.save(dest)
        else:
            shutil.copy2(path, dest)
        staged.append(dest.relative_to(assets.parent).as_posix())


def add_wither_hearts(console_icons: Path, icons_152: Path) -> str:
    """gui/icons.png stays the console one (SKIP_CONSOLE_GUI), but 1.2.5 has no
    wither hearts: copy only those 9x9 cells (x 117..161, normal and hardcore rows)."""
    with Image.open(console_icons) as base, Image.open(icons_152) as new:
        icons = base.convert("RGBA")
        source = new.convert("RGBA")
        for y in (0, 45):
            box = (117, y, 162, y + 9)
            icons.paste(source.crop(box), box[:2])
    icons.save(console_icons, optimize=True)
    return "assets/gui/icons.png"


def read_lang(path: Path) -> tuple[list[str], dict[str, str]]:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    values = {}
    for line in lines:
        if "=" in line and not line.lstrip().startswith("#"):
            key, value = line.split("=", 1)
            values[key.strip()] = value
    return lines, values


def merge_languages(base_lang: Path, old_lang: Path, new_lang: Path, out_lang: Path) -> list[str]:
    """1.5.2 texts for every language OptiCraft ships, keeping OptiCraft's own.

    A key OptiCraft changed from vanilla 1.2.5 keeps OptiCraft's text; a key it
    left as vanilla takes the 1.5.2 text; keys new in 1.5.2 are appended.
    """
    merged_names = []
    for base in sorted(base_lang.glob("*.lang")):
        new = new_lang / base.name
        if not new.exists():
            continue
        base_lines, base_values = read_lang(base)
        _, old_values = read_lang(old_lang / base.name) if (old_lang / base.name).exists() else ([], {})
        _, new_values = read_lang(new)
        out = []
        for line in base_lines:
            if "=" in line and not line.lstrip().startswith("#"):
                key = line.split("=", 1)[0].strip()
                if key in new_values and base_values.get(key) == old_values.get(key, base_values.get(key)):
                    line = f"{key}={new_values[key]}"
            out.append(line)
        added = [f"{key}={value}" for key, value in new_values.items() if key not in base_values]
        if added:
            out.append("")
            out.append("# Minecraft 1.5.2")
            out += added
        target = out_lang / base.name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text("\n".join(out) + "\n", encoding="utf-8")
        merged_names.append(base.name)

    # Update languages.txt with corrected names from 1.5.2 while preserving OptiCraft's 58 supported languages
    base_txt = base_lang / "languages.txt"
    new_txt = new_lang / "languages.txt"
    if base_txt.exists():
        new_languages_map = {}
        if new_txt.exists():
            for line in new_txt.read_text(encoding="utf-8").splitlines():
                if "=" in line and not line.lstrip().startswith("#"):
                    k, v = line.split("=", 1)
                    new_languages_map[k.strip()] = v.strip()
        updated_lines = []
        for line in base_txt.read_text(encoding="utf-8").splitlines():
            if "=" in line and not line.lstrip().startswith("#"):
                k, v = line.split("=", 1)
                k = k.strip()
                v = new_languages_map.get(k, v.strip())
                updated_lines.append(f"{k}={v}")
            else:
                updated_lines.append(line)
        target_txt = out_lang / "languages.txt"
        target_txt.parent.mkdir(parents=True, exist_ok=True)
        target_txt.write_text("\n".join(updated_lines) + "\n", encoding="utf-8")
        merged_names.append("languages.txt")

    return merged_names


def copy_sound3(launcher_assets: Path, resources: Path) -> list[str]:
    """The 1.5 sounds (resources/sound3) are not in the jar: the launcher keeps them
    hashed under assets/objects, listed in indexes/pre-1.6.json."""
    index = json.loads((launcher_assets / "indexes" / "pre-1.6.json").read_text(encoding="utf-8"))["objects"]
    copied = []
    for name, entry in sorted(index.items()):
        if not name.startswith("sound3/"):
            continue
        digest = entry["hash"]
        source = launcher_assets / "objects" / digest[:2] / digest
        if not source.exists():
            raise FileNotFoundError(f"{name}: {source} not downloaded by the launcher")
        dest = resources / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, dest)
        copied.append("resources/" + name)
    return copied


def copy_extra_sounds(extra_root: Path, resources: Path) -> list[str]:
    """Sounds 1.5.2 names but never shipped (villagers stayed silent until 1.6): the 1.6
    files from the launcher's legacy index, renamed to the 1.5.2 keys, kept in the repo."""
    copied = []
    if not extra_root.is_dir():
        return copied
    for source in sorted(extra_root.rglob("*.ogg")):
        rel = source.relative_to(extra_root).as_posix()
        dest = resources / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, dest)
        copied.append("resources/" + rel)
    return copied


def copy_lce_skin_packs(base_data: Path, assets: Path, staged: list[str]) -> list[str]:
    """Copies LCE DLC skin packs into the staged assets/skins/packs directory."""
    copied = []
    base_packs = base_data / "assets/skins/packs"
    dest_packs = assets / "skins/packs"
    if base_packs.is_dir():
        for item in sorted(base_packs.rglob("*")):
            if item.is_file():
                rel = item.relative_to(base_packs)
                dest = dest_packs / rel
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(item, dest)
                p = "assets/skins/packs/" + rel.as_posix()
                staged.append(p)
                copied.append(p)
    return copied


def load_make_pak(script: Path):
    spec = importlib.util.spec_from_file_location("make_pak", script)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Cannot load pak writer: {script}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True, help="OptiCraft worktree root")
    parser.add_argument("--base-data", type=Path, required=True, help="Read-only original data/ tree")
    parser.add_argument("--source", type=Path, required=True, help="mcp-diff/src152")
    parser.add_argument("--output-data", type=Path, required=True, help="Isolated data staging folder under build/")
    parser.add_argument("--output-pak", type=Path, required=True, help="Build/output assets.pak destination")
    parser.add_argument("--header", type=Path, default=Path("src/net/minecraft/src/Texture152Atlas.h"))
    parser.add_argument("--report-json", type=Path, required=True)
    parser.add_argument("--preview", type=Path, required=True)
    parser.add_argument("--launcher-assets", type=Path, default=None,
                        help="Launcher assets/ (indexes/pre-1.6.json + objects/) for the 1.5 sound3 set")
    args = parser.parse_args()

    repo = args.repo.resolve()
    output_data = args.output_data.resolve()
    build_root = (repo / "build").resolve()
    if build_root not in output_data.parents:
        raise ValueError(f"Refusing to stage outside the repository build/: {output_data}")
    if output_data == args.base_data.resolve() or args.base_data.resolve() in output_data.parents:
        raise ValueError("Output data must not be inside the protected base-data tree")
    if not args.base_data.is_dir() or not (args.base_data / "assets").is_dir():
        raise FileNotFoundError(f"Base data/assets not found: {args.base_data}")
    if not (args.source / "textures/blocks").is_dir() or not (args.source / "textures/items").is_dir():
        raise FileNotFoundError(f"Expected 1.5.2 textures under {args.source}")

    block_cpp = repo / "src/net/minecraft/src/Block.cpp"
    item_cpp = repo / "src/net/minecraft/src/Item.cpp"
    item_overrides, item_reserved = legacy_item_overrides(item_cpp, args.source / "textures/items")
    blocks_atlas, block_indices, block_slots, old_block_unmatched, block_exact = build_atlas(
        args.source / "textures/blocks", args.source.parent / "src125/terrain.png",
        BLOCK_SLOT_OVERRIDES, legacy_block_slots(block_cpp)
    )
    items_atlas, item_indices, item_slots, old_item_unmatched, item_exact = build_atlas(
        args.source / "textures/items", args.source.parent / "src125/gui/items.png",
        item_overrides, item_reserved
    )

    if output_data.exists():
        shutil.rmtree(output_data)
    shutil.copytree(args.base_data, output_data)
    assets = output_data / "assets"
    assets.mkdir(parents=True, exist_ok=True)
    blocks_atlas.save(assets / "terrain.png", optimize=True)
    items_atlas.save(assets / "gui/items.png", optimize=True)
    write_header(repo / args.header, block_indices, item_indices)

    staged = ["assets/terrain.png", "assets/gui/items.png"]
    copy_source_textures(args.source, assets, staged)
    sound3 = copy_sound3(args.launcher_assets, output_data / "resources") if args.launcher_assets else []
    staged += sound3
    staged += copy_extra_sounds(repo / "scripts/port152/extra_sounds", output_data / "resources")
    merged_langs = merge_languages(args.base_data / "assets/lang", args.source.parent / "src125/lang",
                                   args.source / "lang", assets / "lang")
    staged += [f"assets/lang/{name}" for name in merged_langs]
    staged.append(add_wither_hearts(assets / "gui/icons.png", args.source / "gui/icons.png"))
    copy_lce_skin_packs(args.base_data, assets, staged)
    block_report = parse_block_report(block_cpp, block_slots)

    args.preview.parent.mkdir(parents=True, exist_ok=True)
    preview = Image.new("RGBA", (ATLAS_SIZE * 2, ATLAS_SIZE), (36, 38, 42, 255))
    preview.paste(blocks_atlas, (0, 0))
    preview.paste(items_atlas, (ATLAS_SIZE, 0))
    preview.save(args.preview, optimize=True)

    make_pak = load_make_pak(repo / "scripts/make_pak.py")
    args.output_pak.parent.mkdir(parents=True, exist_ok=True)
    count, total_bytes = make_pak.build(str(output_data), str(args.output_pak))

    result = {
        "atlas": {"width": 256, "height": 256, "cells": 256},
        "blockIconNames": len(block_indices),
        "blockUniqueImages": len(set(block_indices.values())),
        "blockExactLegacyCells": block_exact,
        "itemIconNames": len(item_indices),
        "itemUniqueImages": len(set(item_indices.values())),
        "itemExactLegacyCells": item_exact,
        "oldUnmatchedNonTransparentBlockCells": old_block_unmatched,
        "oldUnmatchedNonTransparentItemCells": old_item_unmatched,
        "blockIcons": block_indices,
        "itemIcons": item_indices,
        "oldBlocks": block_report,
        "skippedConsoleGui": sorted(SKIP_CONSOLE_GUI),
        "staged1_5Files": staged,
        "iconsWithoutRoom": OVERFLOW,
        "mergedLanguages": len(merged_langs),
        "sound3Files": len(sound3),
        "pakEntries": count,
        "pakBytes": total_bytes,
        "pakPath": str(args.output_pak.resolve()),
    }
    args.report_json.parent.mkdir(parents=True, exist_ok=True)
    args.report_json.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps({key: result[key] for key in ("atlas", "blockIconNames", "blockUniqueImages", "itemIconNames", "itemUniqueImages", "iconsWithoutRoom", "pakEntries", "pakBytes", "pakPath")}, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise
