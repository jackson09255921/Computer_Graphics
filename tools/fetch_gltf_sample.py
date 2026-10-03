#!/usr/bin/env python3
"""Fetch one manifest-pinned Khronos GLB for CI or local validation."""

import hashlib
import json
import pathlib
import sys
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parents[1]
ASSETS = ROOT / "LabX" / "data" / "external" / "gltf_samples"


def main():
    if len(sys.argv) != 2:
        raise ValueError("usage: fetch_gltf_sample.py ModelName")
    model = sys.argv[1]
    entries = json.loads((ASSETS / "manifest.json").read_text(encoding="utf-8-sig"))
    entry = next((item for item in entries if item["name"] == model), None)
    if entry is None:
        raise ValueError(f"model is not pinned in the manifest: {model}")
    destination = ASSETS / entry["file"]
    destination.parent.mkdir(parents=True, exist_ok=True)
    url = ("https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/main/Models/"
           f"{model}/glTF-Binary/{model}.glb")
    if not destination.is_file() or destination.stat().st_size != entry["bytes"]:
        with urllib.request.urlopen(url, timeout=120) as response, destination.open("wb") as output:
            while chunk := response.read(1024 * 1024):
                output.write(chunk)
    payload_hash = hashlib.sha256(destination.read_bytes()).hexdigest()
    if payload_hash != entry["sha256"]:
        destination.unlink(missing_ok=True)
        raise ValueError(f"SHA-256 mismatch after downloading {model}")
    print(f"ready: {model} ({destination.stat().st_size} bytes, SHA-256 verified)")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"sample fetch failed: {error}", file=sys.stderr)
        raise SystemExit(1)
