import os
import re

src_dir = r"C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\src"
pattern = re.compile(r"WII_PLATFORM|PLATFORM_WII")
exclude_dirs = {"wii", "ps2", "xbox"}

# Domains to categorize:
# render: files touched by agy-render
# storage: files touched by agy-storage
# input: files touched by agy-input
# platform: our domain!

solo_wii = []

for root, dirs, files in os.walk(src_dir):
    parts = os.path.normpath(root).split(os.sep)
    if any(p in exclude_dirs for p in parts):
        continue
    for f in files:
        if f.endswith((".cpp", ".h")):
            filepath = os.path.join(root, f)
            with open(filepath, "r", encoding="utf-8", errors="replace") as fp:
                lines = fp.readlines()
            for lno, line in enumerate(lines, 1):
                if pattern.search(line):
                    # Check if line also has PS2 or XBOX
                    has_ps2 = "PS2" in line
                    has_xbox = "XBOX" in line
                    relpath = os.path.relpath(filepath, os.path.dirname(src_dir))
                    if not has_ps2 and not has_xbox:
                        solo_wii.append((relpath, lno, line.strip()))

out_file = r"C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\solo_wii_guards.txt"
with open(out_file, "w", encoding="utf-8") as out:
    for path, lno, line in solo_wii:
        out.write(f"{path}:{lno}: {line}\n")

print(f"Total solo-Wii lines: {len(solo_wii)}")

# Group by file
files_map = {}
for path, lno, line in solo_wii:
    files_map.setdefault(path, []).append((lno, line))

for path in sorted(files_map.keys()):
    print(f"{path} ({len(files_map[path])} occurrences)")
