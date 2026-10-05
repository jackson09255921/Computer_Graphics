#!/usr/bin/env python3
"""Run every non-interactive Computer Graphics lab validation from one entry point."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import struct
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
import zlib


ROOT = pathlib.Path(__file__).resolve().parents[1]


def executable(build: pathlib.Path, name: str, config: str) -> pathlib.Path:
    suffix = ".exe" if os.name == "nt" else ""
    candidates = (build / config / f"{name}{suffix}", build / f"{name}{suffix}")
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(f"cannot find {name} in {build}")


def run(command: list[str], cwd: pathlib.Path = ROOT) -> str:
    print("+", " ".join(command), flush=True)
    completed = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
    if completed.stdout:
        print(completed.stdout, end="")
    if completed.stderr:
        print(completed.stderr, end="", file=sys.stderr)
    if completed.returncode:
        raise RuntimeError(f"command exited with {completed.returncode}: {' '.join(command)}")
    return completed.stdout


def image_info(path: pathlib.Path) -> tuple[int, int, bytes]:
    payload = path.read_bytes()
    if payload[:2] == b"BM" and len(payload) >= 54:
        width, height = struct.unpack_from("<ii", payload, 18)
        offset = struct.unpack_from("<I", payload, 10)[0]
        pixels = payload[offset:]
        return abs(width), abs(height), pixels
    if payload[:2] in (b"P6", b"P3"):
        tokens: list[bytes] = []
        index = 2
        while len(tokens) < 3:
            while index < len(payload) and payload[index:index + 1].isspace():
                index += 1
            if payload[index:index + 1] == b"#":
                index = payload.find(b"\n", index) + 1
                continue
            end = index
            while end < len(payload) and not payload[end:end + 1].isspace():
                end += 1
            tokens.append(payload[index:end])
            index = end
        while index < len(payload) and payload[index:index + 1].isspace():
            index += 1
        return int(tokens[0]), int(tokens[1]), payload[index:]
    raise ValueError(f"unsupported image format: {path}")


def validate_image(path: pathlib.Path, expected: tuple[int, int]) -> dict[str, object]:
    width, height, pixels = image_info(path)
    if (width, height) != expected:
        raise ValueError(f"{path.name}: expected {expected[0]}x{expected[1]}, got {width}x{height}")
    if len(pixels) < width * height:
        raise ValueError(f"{path.name}: truncated pixel data")
    values = set(pixels)
    if len(values) < 4 or max(values) - min(values) < 16:
        raise ValueError(f"{path.name}: image has insufficient visual variation")
    return {"file": str(path.relative_to(ROOT)), "width": width, "height": height,
            "bytes": path.stat().st_size, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def rgb_pixels(path: pathlib.Path) -> tuple[int, int, bytes]:
    payload = path.read_bytes()
    if payload[:2] == b"BM" and len(payload) >= 54:
        width, signed_height = struct.unpack_from("<ii", payload, 18)
        bits = struct.unpack_from("<H", payload, 28)[0]
        if width <= 0 or signed_height == 0 or bits != 24:
            raise ValueError(f"unsupported BMP layout: {path}")
        height = abs(signed_height)
        offset = struct.unpack_from("<I", payload, 10)[0]
        stride = (width * 3 + 3) & ~3
        rows = []
        for display_y in range(height):
            source_y = height - 1 - display_y if signed_height > 0 else display_y
            row = payload[offset + source_y * stride:offset + source_y * stride + width * 3]
            rows.append(bytes(channel for pixel in range(width)
                              for channel in (row[pixel * 3 + 2], row[pixel * 3 + 1], row[pixel * 3])))
        return width, height, b"".join(rows)
    width, height, pixels = image_info(path)
    if payload[:2] != b"P6" or len(pixels) < width * height * 3:
        raise ValueError(f"PNG publication requires a binary RGB image: {path}")
    return width, height, pixels[:width * height * 3]


def publish_png(source: pathlib.Path, destination: pathlib.Path) -> str:
    width, height, pixels = rgb_pixels(source)
    scanlines = b"".join(b"\0" + pixels[row * width * 3:(row + 1) * width * 3]
                         for row in range(height))
    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(scanlines, 9)) + chunk(b"IEND", b""))
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(png)
    return str(destination.relative_to(ROOT))


def mean_absolute_difference(first: pathlib.Path, second: pathlib.Path) -> float:
    first_width, first_height, first_pixels = rgb_pixels(first)
    second_width, second_height, second_pixels = rgb_pixels(second)
    if (first_width, first_height) != (second_width, second_height):
        raise ValueError("visual comparison dimensions do not match")
    return sum(abs(a - b) for a, b in zip(first_pixels, second_pixels)) / len(first_pixels)


def write_reports(output: pathlib.Path, records: list[dict[str, object]], elapsed: float) -> None:
    report = {"status": "passed", "elapsed_seconds": round(elapsed, 3), "validations": records}
    (output / "validation-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    suite = ET.Element("testsuite", name="computer-graphics-pipeline",
                       tests=str(len(records)), failures="0", time=f"{elapsed:.3f}")
    for record in records:
        ET.SubElement(suite, "testcase", classname="pipeline", name=str(record["name"]),
                      time=f"{float(record['seconds']):.3f}")
    ET.ElementTree(suite).write(output / "validation-report.xml", encoding="utf-8", xml_declaration=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build-jenkins")
    parser.add_argument("--config", default="Release")
    parser.add_argument("--gpu-build-dir")
    parser.add_argument("--spp", type=int, default=8)
    args = parser.parse_args()
    build = (ROOT / args.build_dir).resolve()
    output = build / "validation-artifacts"
    gallery = ROOT / "LabX" / "images"
    output.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, object]] = []
    started = time.monotonic()

    def stage(name: str, action) -> None:
        begin = time.monotonic()
        details = action() or {}
        records.append({"name": name, "seconds": round(time.monotonic() - begin, 3), **details})

    ctest = ["ctest", "--test-dir", str(build), "--output-on-failure"]
    if os.name == "nt":
        ctest[3:3] = ["-C", args.config]
    stage("cpu-ctest", lambda: {"output": run(ctest).strip().splitlines()[-1]})

    renders = [
        ("bezier", "bezier_demo", [str(output / "bezier.bmp")], (960, 540), "pipeline_bezier.png"),
        ("raytracer", "raytracer_demo", [str(output / "raytracer.bmp")], (800, 450), "pipeline_cpu_raytracer.png"),
        ("pathtracer", "pathtracer_demo", [str(output / "pathtracer.bmp"), str(args.spp)], (640, 360),
         "pipeline_cpu_pathtracer_smoke.png"),
    ]
    for name, program, command_args, dimensions, gallery_name in renders:
        def render(program=program, command_args=command_args, dimensions=dimensions, gallery_name=gallery_name):
            run([str(executable(build, program, args.config)), *command_args])
            source = pathlib.Path(command_args[0])
            result = validate_image(source, dimensions)
            result["published"] = publish_png(source, gallery / "ci" / gallery_name)
            return result
        stage(f"render-{name}", render)

    animation_dir = output / "animation"
    def animation() -> dict[str, object]:
        run([str(executable(build, "animation_demo", args.config)), str(animation_dir)])
        frames = sorted(animation_dir.glob("frame_*.bmp"))
        if len(frames) != 60:
            raise ValueError(f"animation: expected 60 frames, got {len(frames)}")
        first = validate_image(frames[0], (640, 360))
        last = validate_image(frames[-1], (640, 360))
        sheet = validate_image(animation_dir / "contact_sheet.bmp", (960, 540))
        if first["sha256"] == last["sha256"]:
            raise ValueError("animation: first and last frames are identical")
        sheet["published"] = publish_png(animation_dir / "contact_sheet.bmp",
                                          gallery / "ci" / "pipeline_animation_contact_sheet.png")
        return {"frames": len(frames), "contact_sheet": sheet}
    stage("render-animation", animation)

    legacy = ROOT / "legacy" / "2022CG_Lab3" / "Mesh"
    for model in ("bench", "drop", "glass", "skull"):
        def render_legacy(model=model):
            target = output / f"legacy_{model}.bmp"
            run([str(executable(build, "legacy_asc_demo", args.config)), str(legacy / f"{model}.asc"), str(target)])
            result = validate_image(target, (512, 512))
            result["published"] = publish_png(target, gallery / "ci" / f"pipeline_legacy_{model}.png")
            return result
        stage(f"legacy-{model}", render_legacy)

    def external_assets() -> dict[str, object]:
        manifest = json.loads((ROOT / "LabX" / "data" / "external" / "gltf_samples" /
                               "manifest.json").read_text(encoding="utf-8-sig"))
        for entry in manifest:
            model = str(entry["name"])
            run([sys.executable, str(ROOT / "tools" / "fetch_gltf_sample.py"), model])
        return {"output": run(
            [sys.executable, str(ROOT / "tools" / "validate_gltf_samples.py")]).strip()}
    stage("external-assets", external_assets)
    samples = ROOT / "LabX" / "data" / "external" / "gltf_samples"
    for model in ("AlphaBlendModeTest", "Avocado", "BoomBox", "BoxTextured", "DamagedHelmet", "Duck",
                  "Lantern", "ToyCar", "WaterBottle"):
        def load_gltf(model=model):
            text = run([str(executable(build, "gltf_demo", args.config)),
                        str(samples / model / f"{model}.glb")])
            return {"output": text.strip()}
        stage(f"gltf-{model}", load_gltf)

    if args.gpu_build_dir:
        gpu = (ROOT / args.gpu_build_dir).resolve()
        command = ["ctest", "--test-dir", str(gpu), "-E", "optix", "--output-on-failure"]
        if os.name == "nt":
            command[3:3] = ["-C", args.config]
        stage("gpu-ctest", lambda: {"output": run(command).strip().splitlines()[-1]})
        target = output / "cuda_multi_asset.ppm"
        models = ROOT / "LabX" / "data" / "legacy_converted" / "meshes"
        def cuda_render():
            run([str(executable(gpu, "cuda_pathtracer", args.config)), "--scene", str(target), str(args.spp),
                 str(models / "bunny_from_obj.gltf"), str(models / "teapot_from_obj.gltf")])
            result = validate_image(target, (1280, 720))
            result["published"] = publish_png(target, gallery / "ci" / "pipeline_cuda_multi_asset_smoke.png")
            return result
        stage("render-cuda-multi-asset", cuda_render)

        mesh_baseline = output / "mesh_light_baseline.ppm"
        mesh_feature = output / "mesh_light_feature.ppm"
        def cuda_mesh_light():
            for mode, target in (("--mesh-light-baseline", mesh_baseline),
                                 ("--mesh-light", mesh_feature)):
                run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
                     str(target), str(args.spp)])
                validate_image(target, (640, 360))
            difference = mean_absolute_difference(mesh_baseline, mesh_feature)
            if difference < 2.0:
                raise ValueError(f"mesh-light visual delta is too small: {difference:.3f}")
            return {
                "mean_absolute_difference": round(difference, 3),
                "baseline": publish_png(mesh_baseline, gallery / "ci" / "pipeline_mesh_light_baseline.png"),
                "feature": publish_png(mesh_feature, gallery / "ci" / "pipeline_mesh_light_nee_mis.png"),
            }
        stage("render-cuda-mesh-light", cuda_mesh_light)

    write_reports(output, records, time.monotonic() - started)
    print(f"pipeline passed: {len(records)} validations; reports in {output}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"validation pipeline failed: {error}", file=sys.stderr)
        raise SystemExit(1)
