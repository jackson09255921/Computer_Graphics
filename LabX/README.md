# LabX experiments

`LabX` 是原始 OpenGL 課程以外的現代電腦圖學實驗區。所有 offline renderer
都能 headless 執行，並以 deterministic 測試、固定 seed 與審核過的 PNG 驗證。

```text
LabX/
├── animation/       # keyframe and quaternion animation demos
├── assets/          # glTF/GLB and legacy ASC loaders
├── curves/          # headless and interactive Bézier tools
├── data/            # converted legacy data and licensed validation assets
├── environment/     # RGBE HDR loading and importance sampling
├── gpu/
│   ├── cuda/        # CUDA path tracer and ReSTIR foundations
│   └── optix/       # OptiX RTX, reprojection and reservoir experiments
├── images/          # reviewed pipeline, quality and evolution renders
├── pbr/             # GGX, Fresnel and material tests
├── progressive/     # deterministic tiled rendering
├── sampling/        # PDFs, importance sampling and MIS
├── visual/          # image validation and regression comparison
└── raytracing/      # CPU Whitted and Monte Carlo renderers
```

## Implemented stages

1. CPU geometry, Bézier curves, ray tracing and BVH.
2. Monte Carlo path tracing, progressive tiles and deterministic scheduling.
3. GGX metallic/roughness, dielectric reflection/transmission and MIS.
4. RGBE HDR environment loading, filtering and importance sampling.
5. glTF/GLB geometry, transforms, normals, UVs and embedded PNG/JPEG images.
6. CUDA tiled path tracing and flattened GPU BVH traversal.
7. Base-color, normal, metallic-roughness and emissive maps.
8. Transmission/IOR, clearcoat and fabric sheen material extensions.
9. Multi-light studio MIS, material-aware framing and close camera studies.
10. Occlusion maps, `doubleSided`, RGBA, alpha MASK and stochastic BLEND.
11. External images, sampler wrapping, TEXCOORD_0/1 and independent
    `KHR_texture_transform` mappings.
12. Clearcoat factor, roughness and normal textures with independent mappings.
13. Sheen color and alpha-channel roughness textures with independent mappings.
14. Red-channel transmission textures over the rough dielectric path.
15. Specular factor/color and alpha/RGB textures with F0/F90 separation.
16. Anisotropic GGX strength, rotation, RGB maps and matched sampling/PDF.
17. OptiX indexed geometry, recursive GGX, temporal reprojection and ReSTIR DI.

The reviewed progression is recorded in
[`images/EVOLUTION.md`](images/EVOLUTION.md). Every comparison uses a new file
instead of overwriting an already reviewed result.

## GPU target and WSL2

The primary CUDA target is an NVIDIA GeForce RTX 4060 Laptop GPU with 8 GiB
VRAM and compute capability 8.9. The renderer therefore uses bounded tile
buffers and avoids unnecessary duplicate scene storage.

Development uses Ubuntu 24.04 under WSL2 and the repository Conda environment:

```bash
conda env create -f environment.yml
conda activate computer-graphics

cmake -S . -B build-wsl-gpu -G Ninja \
  -DBUILD_LEGACY_LABS=OFF \
  -DBUILD_CUDA_DEMOS=ON \
  -DCG_CUDA_ARCHITECTURES=89 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-wsl-gpu --parallel
ctest --test-dir build-wsl-gpu --output-on-failure -E '^optix_'
```

The Windows NVIDIA driver is shared with WSL. Do not install a Linux display
driver inside the distro.

## Asset corpus

`data/external/gltf_samples` contains manifest entries and upstream license
READMEs for Khronos validation assets. Large GLBs are downloaded by:

```powershell
./tools/download_gltf_samples.ps1
```

The current corpus includes Avocado, BoomBox, BoxTextured, DamagedHelmet, Duck,
Lantern, ToyCar, WaterBottle, AlphaBlendModeTest and TextureTransformTest.
Validation checks glTF/GLB 2.x data, dependency files, byte sizes, SHA-256
digests and license files.

Legacy `.obj` and `.asc` inputs are normalized under `data/legacy_converted`.
The converter produces glTF 2.0 geometry plus scene-command JSON, and
`converted_asset_tests` loads every generated mesh through the shared loader.

## Validation contract

- Portable build: 22 CTest cases.
- CUDA build: 26 non-OptiX CTest cases.
- Visual regressions compare deterministic PPM/BMP output and emit diff maps.
- Pipeline PNGs must pass format, dimensions, truncation and variation checks.
- Quality references use 512 spp on CPU and 128 spp on CUDA.
- Reviewed evolution images are inspected at native resolution before commit.
- Jenkins archives JSON/JUnit reports and render artifacts.

```bash
python tools/run_validation_pipeline.py \
  --build-dir build-jenkins \
  --gpu-build-dir build-wsl-gpu

python tools/run_quality_renders.py \
  --cpu-build-dir build-jenkins \
  --gpu-build-dir build-wsl-gpu
```

## OptiX status

The optional native Windows OptiX path supports triangle GAS, SBT records,
radiance/shadow rays, recursive GGX, progressive accumulation, G-buffers,
camera and geometry motion vectors, temporal history validation, and temporal
plus spatial ReSTIR DI reservoirs. It emits beauty, ReSTIR, normal, depth,
motion, validity and reservoir images.

WSL2 remains the primary environment for normal CUDA work. OptiX runtime tests
may fail there when `libnvoptix.so` is unavailable even if SDK compilation
succeeds, so native Windows is the recommended OptiX runtime.

## Next material stages

1. Clearcoat, sheen and transmission texture variants.
2. Specular, anisotropy and iridescence extensions.
3. Volume thickness and colored absorption.
4. Emissive triangle sampling, power-weighted selection, NEE and mesh-light MIS (complete).

The next lighting milestone is stable dielectric transmission plus a controlled
glass-caustic validation scene before adaptive sampling and denoising.

Sparse accessors, compressed geometry, skinning and morph targets are separate
future asset-pipeline tracks.
