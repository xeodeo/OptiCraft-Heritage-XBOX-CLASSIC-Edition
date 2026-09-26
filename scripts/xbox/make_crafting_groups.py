#!/usr/bin/env python3
"""
Extracts the console crafting-menu classification from the Legacy Console
Edition sources and writes src/net/minecraft/src/legacy/LegacyCraftingGroups.h.

For every crafted item it records:
  - the tab (group) its recipe belongs to (the trailing group letter of each
    add*Recipy call, or the file's letter for the table-driven tool/armour files);
  - its "base type" (sword, pickaxe, stairs...), which is what stacks the
    wood/stone/iron/... versions of one item into a single column.

Usage: python scripts/xbox/make_crafting_groups.py [Minecraft.World folder]
"""

import os
import re
import sys

DEFAULT_SRC = os.path.join(os.path.expanduser('~'), 'Downloads', 'minecraft', 'Minecraft.World')

GROUPS = {'S': 0, 'T': 1, 'F': 2, 'A': 3, 'M': 4, 'V': 5, 'D': 6}
GROUP_NAMES = ['Structure', 'Tool', 'Food', 'Armour', 'Mechanism', 'Transport', 'Decoration']

RECIPE_FILES = ['Recipes.cpp', 'StructureRecipies.cpp', 'FoodRecipies.cpp', 'OreRecipies.cpp',
                'WeaponRecipies.cpp', 'ArmorRecipes.cpp', 'ToolRecipies.cpp', 'ClothDyeRecipes.cpp']
# Files whose products come from ADD_OBJECT tables inside a loop.
TABLE_FILES = {'ToolRecipies.cpp': 'T', 'WeaponRecipies.cpp': 'T', 'ArmorRecipes.cpp': 'A'}
# OreRecipies: every table row is a block <-> 9 items pair, both crafted.
ORE_FILE = 'OreRecipies.cpp'
# Tiles whose constructor does not take the id.
TILE_ID_FALLBACK = {'Tile::cloth': 35, 'Tile::stoneSlabHalf': 44, 'Tile::woodSlabHalf': 126,
                    'Tile::tripWireSource': 131}
CAST = r'(?:\(\s*\w+\s*\*\s*\)\s*)?'


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def read(src, name):
    with open(os.path.join(src, name), encoding='utf-8', errors='replace') as f:
        return strip_comments(f.read())


def parse_ids(src):
    """Name -> numeric id for Tile:: and Item:: objects."""
    ids = {}
    assign = r'::(\w+)\s*=\s*[(\s]*' + CAST + r'[(\s]*new\s+\w+\s*\(\s*(\d+)'
    for m in re.finditer(r'Tile' + assign, read(src, 'Tile.cpp')):
        ids['Tile::' + m.group(1)] = int(m.group(2))
    for m in re.finditer(r'Item' + assign, read(src, 'Item.cpp')):
        ids['Item::' + m.group(1)] = 256 + int(m.group(2))
    for name, value in TILE_ID_FALLBACK.items():
        ids.setdefault(name, value)
    return ids


def parse_base_types(src):
    """Name -> (baseType, material) enum names."""
    out = {}
    for fname, prefix in (('Tile.cpp', 'Tile::'), ('Item.cpp', 'Item::')):
        for line in read(src, fname).split('\n'):
            m = re.search(r'(Tile|Item)::(\w+)\s*=.*setBaseItemTypeAndMaterial\(\s*(?:Item::)?eBaseItemType_(\w+)\s*,\s*(?:Item::)?eMaterial_(\w+)', line)
            if m:
                out[prefix + m.group(2)] = (m.group(3), m.group(4))
    return out


def base_type_names(src):
    text = read(src, 'Item.h')
    body = text[text.index('eBaseItemType_undefined'):]
    body = body[:body.index('eBaseItemType_MAXTYPES')]
    return re.findall(r'eBaseItemType_(\w+)', body)


def calls(text, name):
    for m in re.finditer(name + r'\s*\(', text):
        depth, i = 1, m.end()
        while depth and i < len(text):
            if text[i] == '(':
                depth += 1
            elif text[i] == ')':
                depth -= 1
            i += 1
        yield text[m.end():i - 1]


def parse_recipes(src):
    """List of (outputName, aux or -1, groupLetter)."""
    out = []
    for fname in RECIPE_FILES:
        text = read(src, fname)
        for fn in ('addShapedRecipy', 'addShapelessRecipy'):
            for body in calls(text, fn):
                letters = re.findall(r"L'([A-Z])'\s*$", body.strip())
                if not letters:
                    continue
                m = re.match(r'\s*new\s+ItemInstance\s*\(\s*' + CAST + r'((?:Tile|Item)::\w+)\s*(?:,\s*([^,()]+?)\s*(?:,\s*([^,()]+?)\s*)?)?\)', body)
                if not m:
                    continue
                aux = m.group(3)
                aux_val = int(aux) if aux is not None and re.fullmatch(r'-?\d+', aux) else (-1 if aux is None else None)
                out.append((m.group(1), aux_val, letters[-1], aux))
        if fname == ORE_FILE:
            for m in re.finditer(r'ADD_OBJECT\s*\(\s*map\[\d+\]\s*,\s*(?:new\s+ItemInstance\s*\(\s*)?((?:Tile|Item)::\w+)(?:\s*,\s*\d+\s*,\s*(\w+::\w+))?', text):
                aux = 4 if m.group(2) == 'DyePowderItem::BLUE' else -1
                out.append((m.group(1), aux, 'D', None))
        if fname in TABLE_FILES:
            for m in re.finditer(r'ADD_OBJECT\s*\(\s*map\[(\d+)\]\s*,\s*((?:Tile|Item)::\w+)', text):
                if int(m.group(1)) > 0:
                    out.append((m.group(2), -1, TABLE_FILES[fname], None))
    return out


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SRC
    if not os.path.isdir(src):
        print('[!] Folder not found: %s' % src)
        return 1
    ids = parse_ids(src)
    bases = parse_base_types(src)
    type_names = base_type_names(src)

    rows = {}
    unresolved = []
    for name, aux, letter, raw_aux in parse_recipes(src):
        if name not in ids:
            unresolved.append(name)
            continue
        # Named aux values (e.g. TreeTile::BIRCH_TRUNK) only split variants the
        # port does not have; treat them as "any".
        aux = -1 if aux is None else aux
        base = bases.get(name, ('undefined', 'undefined'))[0]
        key = (ids[name], aux)
        if key not in rows:
            rows[key] = (GROUPS[letter], type_names.index(base) if base in type_names else 0, name)

    repo = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
    out_h = os.path.join(repo, 'src', 'net', 'minecraft', 'src', 'legacy', 'LegacyCraftingGroups.h')
    lines = [
        '// Auto-generated by scripts/xbox/make_crafting_groups.py -- do not edit.',
        '// Console crafting menu: tab and base type of each crafted item.',
        '#pragma once',
        '',
        'enum LegacyCraftingGroup : unsigned char',
        '{',
    ]
    for i, g in enumerate(GROUP_NAMES):
        lines.append('    LEGACY_CRAFT_%s = %d,' % (g.upper(), i))
    lines += ['    LEGACY_CRAFT_GROUP_COUNT = %d' % len(GROUP_NAMES), '};', '']
    lines.append('// baseType 0 = none: the item gets a column of its own.')
    lines.append('struct LegacyCraftingGroupEntry { short id; short aux; unsigned char group; unsigned char baseType; };')
    lines.append('')
    lines.append('static const LegacyCraftingGroupEntry s_legacyCraftingGroups[] = {')
    for (item_id, aux), (group, base, name) in sorted(rows.items()):
        lines.append('    { %d, %d, %d, %d }, // %s%s' % (item_id, aux, group, base, name,
                     (' (%s)' % type_names[base]) if base else ''))
    lines.append('};')
    lines.append('')
    with open(out_h, 'w', newline='\n') as f:
        f.write('\n'.join(lines))
    print('[OK] %s (%d items)' % (out_h, len(rows)))
    if unresolved:
        print('[i] unresolved names: %s' % ', '.join(sorted(set(unresolved))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
