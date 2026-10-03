#!/usr/bin/env python3
"""Convert legacy course OBJ/ASC meshes and IN command streams into LabX data."""

import json
import math
import pathlib
import shlex
import struct

ROOT = pathlib.Path(__file__).resolve().parents[1]
LEGACY = ROOT / "legacy"
OUTPUT = ROOT / "LabX" / "data" / "legacy_converted"


def load_obj(path):
    vertices, triangles = [], []
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        words = raw.split()
        if not words or words[0].startswith("#"):
            continue
        if words[0] == "v" and len(words) >= 4:
            vertices.append(tuple(map(float, words[1:4])))
        elif words[0] == "f" and len(words) >= 4:
            face = []
            for word in words[1:]:
                index = int(word.split("/")[0])
                face.append(index - 1 if index > 0 else len(vertices) + index)
            for corner in range(1, len(face) - 1):
                triangles.append((face[0], face[corner], face[corner + 1]))
    return vertices, triangles


def load_asc(path):
    words = path.read_text(encoding="utf-8", errors="replace").split()
    cursor = 0
    vertex_count, face_count = int(words[cursor]), int(words[cursor + 1])
    cursor += 2
    vertices = []
    for _ in range(vertex_count):
        vertices.append(tuple(float(value) for value in words[cursor:cursor + 3]))
        cursor += 3
    triangles = []
    for _ in range(face_count):
        count = int(words[cursor]); cursor += 1
        face = [int(value) - 1 for value in words[cursor:cursor + count]]; cursor += count
        for corner in range(1, len(face) - 1):
            triangles.append((face[0], face[corner], face[corner + 1]))
    return vertices, triangles


def write_gltf(name, vertices, triangles, source):
    if not vertices or not triangles:
        raise ValueError(f"empty mesh: {source}")
    mesh_dir = OUTPUT / "meshes"
    mesh_dir.mkdir(parents=True, exist_ok=True)
    binary = bytearray()
    for vertex in vertices:
        binary.extend(struct.pack("<3f", *vertex))
    index_offset = len(binary)
    for triangle in triangles:
        if min(triangle) < 0 or max(triangle) >= len(vertices):
            raise ValueError(f"index out of range: {source}")
        binary.extend(struct.pack("<3I", *triangle))
    bin_name = f"{name}.bin"
    (mesh_dir / bin_name).write_bytes(binary)
    minimum = [min(vertex[axis] for vertex in vertices) for axis in range(3)]
    maximum = [max(vertex[axis] for vertex in vertices) for axis in range(3)]
    document = {
        "asset": {"version": "2.0", "generator": "Computer_Graphics legacy converter"},
        "extras": {"source": source.relative_to(ROOT).as_posix()},
        "buffers": [{"uri": bin_name, "byteLength": len(binary)}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": index_offset, "target": 34962},
            {"buffer": 0, "byteOffset": index_offset, "byteLength": len(binary) - index_offset,
             "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(vertices), "type": "VEC3",
             "min": minimum, "max": maximum},
            {"bufferView": 1, "componentType": 5125, "count": len(triangles) * 3, "type": "SCALAR"},
        ],
        "materials": [{"pbrMetallicRoughness": {"baseColorFactor": [0.18, 0.52, 0.92, 1.0],
                                                   "metallicFactor": 0.05, "roughnessFactor": 0.48}}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1, "material": 0}]}],
        "nodes": [{"mesh": 0}], "scenes": [{"nodes": [0]}], "scene": 0,
    }
    (mesh_dir / f"{name}.gltf").write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return {"name": name, "source": source.relative_to(ROOT).as_posix(),
            "vertices": len(vertices), "triangles": len(triangles), "gltf": f"meshes/{name}.gltf"}


def convert_scene(path, available):
    commands, object_refs = [], []
    for line_number, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        stripped = raw.strip()
        if not stripped or stripped.startswith("#"):
            continue
        words = shlex.split(stripped)
        args = []
        for value in words[1:]:
            try:
                number = float(value)
                args.append(int(number) if number.is_integer() else number)
            except ValueError:
                args.append(value)
        command = {"line": line_number, "op": words[0], "args": args}
        if words[0] == "object" and args:
            stem = pathlib.Path(str(args[0])).stem.lower()
            converted = f"../meshes/{stem}_from_obj.gltf" if stem in available else None
            command["convertedMesh"] = converted
            object_refs.append({"legacy": str(args[0]), "converted": converted})
        commands.append(command)
    name = f"{path.parents[1].name.lower()}_{path.stem.lower()}"
    output = {"format": "computer-graphics-legacy-scene-v1",
              "source": path.relative_to(ROOT).as_posix(), "commands": commands,
              "objectReferences": object_refs}
    scene_dir = OUTPUT / "scenes"; scene_dir.mkdir(parents=True, exist_ok=True)
    destination = scene_dir / f"{name}.scene.json"
    destination.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")
    return {"name": name, "source": path.relative_to(ROOT).as_posix(),
            "scene": f"scenes/{destination.name}",
            "unresolvedObjects": sorted({entry["legacy"] for entry in object_refs if not entry["converted"]})}


def main():
    mesh_entries = []
    obj_sources = {}
    for path in sorted(LEGACY.rglob("*.obj")):
        obj_sources.setdefault(path.stem.lower(), path)
    for stem, path in sorted(obj_sources.items()):
        mesh_entries.append(write_gltf(f"{stem}_from_obj", *load_obj(path), path))
    asc_sources = {}
    for path in sorted(LEGACY.rglob("*.asc")):
        asc_sources.setdefault(path.stem.lower(), path)
    for stem, path in sorted(asc_sources.items()):
        mesh_entries.append(write_gltf(f"{stem}_from_asc", *load_asc(path), path))
    scenes = [convert_scene(path, set(obj_sources)) for path in sorted(LEGACY.rglob("*.in"))]
    manifest = {"format": "computer-graphics-converted-assets-v1", "meshes": mesh_entries, "scenes": scenes}
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"converted {len(mesh_entries)} meshes and {len(scenes)} scene command streams into {OUTPUT}")


if __name__ == "__main__":
    main()
