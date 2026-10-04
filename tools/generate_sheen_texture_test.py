#!/usr/bin/env python3
"""Derive a reproducible sheen-texture GLB from Khronos SheenTestGrid."""

from __future__ import annotations

import argparse
import json
import pathlib
import struct
import zlib


def png_rgba(width: int, height: int, pixel) -> bytes:
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            rows.extend(pixel(x, y))

    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))


def pad(data: bytes, byte: bytes) -> bytes:
    return data + byte * ((-len(data)) % 4)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()

    raw = args.source.read_bytes()
    if raw[:4] != b"glTF":
        raise ValueError("source must be a binary glTF")
    offset = 12
    json_length, json_type = struct.unpack_from("<II", raw, offset)
    offset += 8
    document = json.loads(raw[offset:offset + json_length].decode("utf-8").rstrip(" \0"))
    offset += json_length
    bin_length, bin_type = struct.unpack_from("<II", raw, offset)
    offset += 8
    binary = bytearray(raw[offset:offset + bin_length])
    if json_type != 0x4E4F534A or bin_type != 0x004E4942:
        raise ValueError("unexpected GLB chunk types")

    size = 64
    color = png_rgba(size, size, lambda x, y: (
        255 if (x // 16 + y // 16) % 2 == 0 else 32,
        48 if (x // 16 + y // 16) % 2 == 0 else 220,
        80 if (x // 16 + y // 16) % 2 == 0 else 255,
        255))
    roughness = png_rgba(size, size, lambda x, _y: (255, 255, 255, 20 + 235 * x // (size - 1)))

    document.setdefault("bufferViews", [])
    document.setdefault("images", [])
    document.setdefault("textures", [])
    texture_indices = []
    for name, image in (("Stage12SheenColor", color), ("Stage12SheenRoughness", roughness)):
        while len(binary) % 4:
            binary.append(0)
        view = len(document["bufferViews"])
        document["bufferViews"].append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(image)})
        binary.extend(image)
        image_index = len(document["images"])
        document["images"].append({"name": name, "bufferView": view, "mimeType": "image/png"})
        texture_indices.append(len(document["textures"]))
        document["textures"].append({"source": image_index})

    for material in document.get("materials", []):
        sheen = material.get("extensions", {}).get("KHR_materials_sheen")
        if sheen is not None:
            sheen["sheenColorTexture"] = {"index": texture_indices[0]}
            sheen["sheenRoughnessTexture"] = {"index": texture_indices[1]}

    document["buffers"][0]["byteLength"] = len(binary)
    document.setdefault("asset", {})["generator"] = "Computer_Graphics Stage 12 sheen texture validator"
    json_chunk = pad(json.dumps(document, separators=(",", ":")).encode("utf-8"), b" ")
    bin_chunk = pad(bytes(binary), b"\0")
    total = 12 + 8 + len(json_chunk) + 8 + len(bin_chunk)
    output = (struct.pack("<4sII", b"glTF", 2, total) +
              struct.pack("<II", len(json_chunk), 0x4E4F534A) + json_chunk +
              struct.pack("<II", len(bin_chunk), 0x004E4942) + bin_chunk)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(output)
    print(f"generated {args.output} ({len(output)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
