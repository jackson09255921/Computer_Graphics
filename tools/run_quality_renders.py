#!/usr/bin/env python3
"""Render and publish the slower, presentation-quality validation set."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
import time

from run_validation_pipeline import ROOT, executable, publish_png, run, validate_image


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cpu-build-dir", default="build-jenkins")
    parser.add_argument("--gpu-build-dir", default="build-wsl-gpu")
    parser.add_argument("--config", default="Release")
    parser.add_argument("--cpu-spp", type=int, default=512)
    parser.add_argument("--gpu-spp", type=int, default=128)
    args = parser.parse_args()
    cpu = (ROOT / args.cpu_build_dir).resolve()
    gpu = (ROOT / args.gpu_build_dir).resolve()
    artifacts = cpu / "quality-artifacts"
    gallery = ROOT / "LabX" / "images"
    artifacts.mkdir(parents=True, exist_ok=True)
    results: list[dict[str, object]] = []
    started = time.monotonic()

    def publish(source: pathlib.Path, destination: str, dimensions: tuple[int, int], name: str) -> None:
        begin = time.monotonic()
        result = validate_image(source, dimensions)
        result.update(name=name, seconds=round(time.monotonic() - begin, 3),
                      published=publish_png(source, gallery / destination))
        results.append(result)

    cpu_output = artifacts / f"cpu_pathtracer_{args.cpu_spp}spp.bmp"
    run([str(executable(cpu, "pathtracer_demo", args.config)), str(cpu_output), str(args.cpu_spp)])
    publish(cpu_output, f"quality_cpu_pathtracer_{args.cpu_spp}spp.png", (640, 360), "cpu-pathtracer")

    samples = ROOT / "LabX" / "data" / "external" / "gltf_samples"
    for model in ("Avocado", "BoomBox", "Lantern", "ToyCar", "WaterBottle"):
        source = artifacts / f"gltf_{model.lower()}_{args.gpu_spp}spp.ppm"
        mode = "--showcase-studio" if model == "ToyCar" else "--gltf"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(samples / model / f"{model}.glb"), str(source), str(args.gpu_spp)])
        suffix = "_emissive" if model == "Lantern" else "_subject_framing" if model == "ToyCar" else ""
        destination = (f"evolution_toycar_stage5_clearcoat_{args.gpu_spp}spp.png"
                       if model == "ToyCar" else
                       f"quality_gltf_{model.lower()}{suffix}_{args.gpu_spp}spp.png")
        publish(source, destination,
                (640, 360), f"gltf-{model}")

    toycar = samples / "ToyCar" / "ToyCar.glb"
    for mode, label in (("--fabric-baseline", "baseline"), ("--fabric-sheen", "sheen")):
        source = artifacts / f"toycar_fabric_{label}_{args.gpu_spp}spp.ppm"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(toycar), str(source), str(args.gpu_spp)])
        publish(source, f"evolution_toycar_stage6_fabric_{label}_{args.gpu_spp}spp.png",
                (640, 360), f"toycar-fabric-{label}")

    for mode, label in (("--fabric-close-baseline", "baseline"),
                        ("--fabric-close-sheen", "sheen")):
        source = artifacts / f"toycar_fabric_close_{label}_{args.gpu_spp}spp.ppm"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(toycar), str(source), str(args.gpu_spp)])
        publish(source, f"evolution_toycar_stage7_close_{label}_{args.gpu_spp}spp.png",
                (640, 360), f"toycar-fabric-close-{label}")

    helmet = samples / "DamagedHelmet" / "DamagedHelmet.glb"
    for mode, label in (("--occlusion-baseline", "baseline"), ("--occlusion", "occlusion")):
        source = artifacts / f"damaged_helmet_{label}_{args.gpu_spp}spp.ppm"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(helmet), str(source), str(args.gpu_spp)])
        publish(source, f"evolution_helmet_stage8_{label}_{args.gpu_spp}spp.png",
                (640, 360), f"damaged-helmet-{label}")

    alpha_test = samples / "AlphaBlendModeTest" / "AlphaBlendModeTest.glb"
    for mode, label in (("--alpha-baseline", "opaque"), ("--alpha", "alpha")):
        source = artifacts / f"alpha_modes_{label}_{args.gpu_spp}spp.ppm"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(alpha_test), str(source), str(args.gpu_spp)])
        publish(source, f"evolution_alpha_stage9_{label}_{args.gpu_spp}spp.png",
                (640, 360), f"alpha-modes-{label}")

    transform_test = samples / "TextureTransformTest" / "TextureTransformTest.gltf"
    for mode, label in (("--texture-transform-baseline", "baseline"),
                        ("--texture-transform", "transformed")):
        source = artifacts / f"texture_transform_{label}_{args.gpu_spp}spp.ppm"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(transform_test), str(source), str(args.gpu_spp)])
        publish(source, f"evolution_texture_stage10_{label}_{args.gpu_spp}spp.png",
                (640, 360), f"texture-transform-{label}")

    clearcoat_test = samples / "ClearCoatTest" / "ClearCoatTest.glb"
    for mode, label in (("--clearcoat-texture-baseline", "baseline"),
                        ("--clearcoat-texture", "textures")):
        source = artifacts / f"clearcoat_texture_{label}_{args.gpu_spp}spp.ppm"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(clearcoat_test), str(source), str(args.gpu_spp)])
        publish(source, f"evolution_clearcoat_stage11_{label}_{args.gpu_spp}spp.png",
                (640, 360), f"clearcoat-texture-{label}")

    sheen_source = samples / "SheenTestGrid" / "SheenTestGrid.glb"
    sheen_test = ROOT / "LabX" / "data" / "generated" / "SheenTextureTest.glb"
    run([sys.executable, str(ROOT / "tools" / "generate_sheen_texture_test.py"),
         str(sheen_source), str(sheen_test)])
    for mode, label in (("--sheen-texture-baseline", "baseline"),
                        ("--sheen-texture", "textures")):
        source = artifacts / f"sheen_texture_{label}_{args.gpu_spp}spp.ppm"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(sheen_test), str(source), str(args.gpu_spp)])
        publish(source, f"evolution_sheen_stage12_{label}_{args.gpu_spp}spp.png",
                (640, 360), f"sheen-texture-{label}")

    transmission_test = samples / "TransmissionTest" / "TransmissionTest.glb"
    for mode, label in (("--transmission-texture-baseline", "baseline"),
                        ("--transmission-texture", "textures")):
        source = artifacts / f"transmission_texture_{label}_{args.gpu_spp}spp.ppm"
        run([str(executable(gpu, "cuda_pathtracer", args.config)), mode,
             str(transmission_test), str(source), str(args.gpu_spp)])
        publish(source, f"evolution_transmission_stage13_{label}_{args.gpu_spp}spp.png",
                (640, 360), f"transmission-texture-{label}")

    models = ROOT / "LabX" / "data" / "legacy_converted" / "meshes"
    multi = artifacts / f"cuda_multi_asset_{args.gpu_spp}spp.ppm"
    run([str(executable(gpu, "cuda_pathtracer", args.config)), "--scene", str(multi), str(args.gpu_spp),
         str(models / "bunny_from_obj.gltf"), str(models / "teapot_from_obj.gltf")])
    publish(multi, f"quality_cuda_multi_asset_{args.gpu_spp}spp.png", (1280, 720), "cuda-multi-asset")

    report = {"status": "passed", "cpu_spp": args.cpu_spp, "gpu_spp": args.gpu_spp,
              "elapsed_seconds": round(time.monotonic() - started, 3), "renders": results}
    (artifacts / "quality-report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"quality render pipeline passed: {len(results)} images published to {gallery}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
