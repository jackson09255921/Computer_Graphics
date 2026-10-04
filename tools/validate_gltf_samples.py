#!/usr/bin/env python3
"""Validate downloaded Khronos GLB files against the checked-in manifest."""

import hashlib
import json
import pathlib
import struct
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
ASSETS = ROOT / "LabX" / "data" / "external" / "gltf_samples"


def main():
    entries = json.loads((ASSETS / "manifest.json").read_text(encoding="utf-8-sig"))
    for entry in entries:
        path = ASSETS / entry["file"]
        payload = path.read_bytes()
        if len(payload) != entry["bytes"]:
            raise ValueError(f"{entry['name']}: byte length mismatch")
        if hashlib.sha256(payload).hexdigest() != entry["sha256"]:
            raise ValueError(f"{entry['name']}: SHA-256 mismatch")
        if path.suffix == ".glb":
            if len(payload) < 12 or payload[:4] != b"glTF":
                raise ValueError(f"{entry['name']}: invalid GLB magic")
            version, declared_length = struct.unpack_from("<II", payload, 4)
            if version != 2 or declared_length != len(payload):
                raise ValueError(f"{entry['name']}: invalid GLB v2 header")
        else:
            document = json.loads(payload)
            if not str(document.get("asset", {}).get("version", "")).startswith("2."):
                raise ValueError(f"{entry['name']}: invalid glTF 2.x document")
        for dependency in entry.get("dependencies", []):
            dependency_path = ASSETS / dependency["file"]
            dependency_payload = dependency_path.read_bytes()
            if len(dependency_payload) != dependency["bytes"] or \
                    hashlib.sha256(dependency_payload).hexdigest() != dependency["sha256"]:
                raise ValueError(f"{entry['name']}: dependency mismatch: {dependency['file']}")
        if not (ASSETS / entry["license"]).is_file():
            raise ValueError(f"{entry['name']}: license README missing")
    print(f"validated {len(entries)} Khronos glTF/GLB assets with licenses and SHA-256 digests")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"external asset validation failed: {error}", file=sys.stderr)
        raise SystemExit(1)
