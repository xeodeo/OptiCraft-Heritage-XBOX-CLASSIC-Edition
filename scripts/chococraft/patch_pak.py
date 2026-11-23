#!/usr/bin/env python3
"""Overlay ChocoCraft assets (textures + kweh.ogg) into an existing assets.pak.

Reads an existing MCPK v1 archive without modifying it, merges every file under
--overlay (e.g. scripts/chococraft/) replacing any same-key entries, and writes
a fresh aligned MCPK v1 archive to --output.
"""

import argparse
import os
import struct
import sys
from pathlib import Path

MAGIC = b"MCPK"
VERSION = 1
HEADER_BYTES = 32
ENTRY_BYTES = 16
DATA_ALIGN = 64


def fnv1a32(text: str) -> int:
    value = 0x811C9DC5
    for byte in text.encode("utf-8"):
        value ^= byte
        value = (value * 0x01000193) & 0xFFFFFFFF
    return value


def align(value: int, alignment: int) -> int:
    return (value + alignment - 1) // alignment * alignment


def read_pak(pak_path: Path) -> dict[str, bytes]:
    data = pak_path.read_bytes()
    if len(data) < HEADER_BYTES:
        raise ValueError(f"Truncated pak: {pak_path}")
    magic, version, count, table_off, names_off, names_bytes, _, _ = struct.unpack_from(
        ">4sIIIIIII", data, 0
    )
    if magic != MAGIC or version != VERSION:
        raise ValueError(f"Unsupported pak header in {pak_path}: {magic!r} v{version}")
    names_blob = data[names_off : names_off + names_bytes]
    entries: dict[str, bytes] = {}
    for idx in range(count):
        _, name_off, data_off, size = struct.unpack_from(
            ">IIII", data, table_off + idx * ENTRY_BYTES
        )
        end = names_blob.index(b"\0", name_off)
        key = names_blob[name_off:end].decode("utf-8")
        entries[key] = data[data_off : data_off + size]
    return entries


def collect_overlay(overlay_dir: Path) -> dict[str, bytes]:
    added: dict[str, bytes] = {}
    for root, _, files in os.walk(overlay_dir):
        for name in files:
            full = Path(root) / name
            rel = full.relative_to(overlay_dir).as_posix()
            if not (rel.startswith("assets/") or rel.startswith("resources/")):
                continue
            added[rel] = full.read_bytes()
    return added


def write_pak(entries: dict[str, bytes], output_path: Path) -> tuple[int, int]:
    # Deduplicate case-insensitively (overlay wins)
    lower_to_key: dict[str, str] = {}
    for key in entries:
        lower_to_key[key.lower()] = key

    names = bytearray()
    records = []
    for key in lower_to_key.values():
        payload = entries[key]
        name_off = len(names)
        names += key.encode("utf-8") + b"\0"
        records.append([fnv1a32(key.lower()), name_off, 0, len(payload), key, payload])
    records.sort(key=lambda r: (r[0], r[4]))

    table_off = HEADER_BYTES
    names_off = table_off + len(records) * ENTRY_BYTES
    data_off = align(names_off + len(names), DATA_ALIGN)

    cursor = data_off
    for rec in records:
        rec[2] = cursor
        cursor = align(cursor + rec[3], DATA_ALIGN)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "wb") as out:
        out.write(
            struct.pack(
                ">4sIIIIIII",
                MAGIC,
                VERSION,
                len(records),
                table_off,
                names_off,
                len(names),
                DATA_ALIGN,
                0,
            )
        )
        for h, name_off, off, size, _, _ in records:
            out.write(struct.pack(">IIII", h, name_off, off, size))
        out.write(bytes(names))
        out.write(b"\0" * (data_off - out.tell()))
        for _, _, off, _, _, payload in records:
            out.write(b"\0" * (off - out.tell()))
            out.write(payload)
        out.write(b"\0" * (align(out.tell(), DATA_ALIGN) - out.tell()))
        total = out.tell()
    return len(records), total


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-pak", type=Path, required=True)
    parser.add_argument("--overlay", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    entries = read_pak(args.base_pak)
    overlay = collect_overlay(args.overlay)
    # Remove case-insensitive collisions before merging overlay
    overlay_lower = {k.lower() for k in overlay}
    merged = {k: v for k, v in entries.items() if k.lower() not in overlay_lower}
    merged.update(overlay)
    count, total = write_pak(merged, args.output)
    print(
        f"{args.output}: {count} entries (+{len(overlay)} overlay), {total} bytes ({total / 1048576.0:.2f} MB)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
