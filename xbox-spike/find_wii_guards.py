import os
import re

src_dir = r"C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\OptiCraftHeritageEdition\src"
out_file = r"C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\wii_guards.txt"

pattern = re.compile(r"WII_PLATFORM|PLATFORM_WII")
exclude_dirs = {"wii", "ps2", "xbox"}

results = []

for root, dirs, files in os.walk(src_dir):
    # filter out excluded dirs
    parts = os.path.normpath(root).split(os.sep)
    if any(p in exclude_dirs for p in parts):
        continue
    for f in files:
        if f.endswith((".cpp", ".h")):
            filepath = os.path.join(root, f)
            try:
                with open(filepath, "r", encoding="utf-8", errors="replace") as fp:
                    for lno, line in enumerate(fp, 1):
                        if pattern.search(line):
                            relpath = os.path.relpath(filepath, os.path.dirname(src_dir))
                            results.append(f"{relpath}:{lno}: {line.strip()}")
            except Exception as e:
                pass

with open(out_file, "w", encoding="utf-8") as out:
    for r in results:
        out.write(r + "\n")

print(f"Found {len(results)} occurrences across shared code.")
