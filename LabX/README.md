# LabX experiments

`LabX` contains the extensions beyond the original coursework. Experiments are
grouped by topic so each rendering technique can evolve without mixing demo
entry points into the legacy labs.

```text
LabX/
├── animation/     # keyframe and quaternion animation demos
├── assets/        # glTF loading and asset tests
├── curves/        # headless and interactive Bézier tools
├── environment/   # HDR environment loading and sampling
├── gpu/
│   ├── cuda/      # CUDA capability, path tracer, and ReSTIR
│   └── optix/     # native Windows OptiX pipelines
├── pbr/           # GGX, Fresnel, and metallic/roughness model
├── progressive/   # deterministic multi-threaded tile rendering
├── sampling/      # importance sampling, PDFs, and MIS
└── raytracing/    # Whitted and Monte Carlo path tracing demos
```

## Reviewed legacy-mesh renders

Legacy Lab 3/4 ASC meshes are supported by `legacy_asc_demo`. It reads the
course format, triangulates polygon faces, builds the current CPU BVH, frames
the object automatically, and produces a deterministic ray-traced image. The
reviewed bench, drop, glass, and skull PNGs are stored in `LabX/images/`; their
lossless PPM references are exercised by CTest visual regressions.

## GPU profile and constraints

The initial GPU target is an NVIDIA GeForce RTX 4060 Laptop GPU with 8 GiB
VRAM, compute capability 8.9, and a current NVIDIA driver.
GPU experiments therefore use these constraints:

- CUDA architecture `sm_89`, configurable through `CG_CUDA_ARCHITECTURES`.
- Tiled rendering and bounded allocations rather than keeping large duplicated
  scene buffers in VRAM.
- CPU targets remain available and are the default on machines without CUDA.
- GPU tests are opt-in because hosted CI runners generally have no NVIDIA GPU.

The legacy Windows CUDA 11.8 toolkit is older than the current MSVC standard
library accepts. GPU development therefore uses Ubuntu 24.04 on WSL2 with the
repository's `computer-graphics` conda environment and CUDA Toolkit 13.1.2. The
Windows NVIDIA driver is shared with WSL; no Linux display driver is installed.

Create or update the environment inside WSL2:

```bash
conda env create -f environment.yml
conda activate computer-graphics
```

Validate the local CUDA toolchain and device with:

```bash
cmake -S . -B build-cuda -DBUILD_LEGACY_LABS=OFF -DBUILD_CUDA_DEMOS=ON
cmake --build build-cuda --target cuda_capability
ctest --test-dir build-cuda -R cuda_capability --output-on-failure
```

## Planned milestones

1. Physically based materials: GGX, Fresnel, metallic/roughness workflow.
2. Progressive, multi-threaded CPU rendering and convergence snapshots.
3. Importance sampling and multiple importance sampling.
4. Refraction, dielectric materials, HDR environment lighting, and glTF input (implemented).
5. CUDA tiled path tracing sized for the 8 GiB GPU budget (geometry, glTF, matched GGX reflection/BTDF, MIS, and HDR implemented).
6. OptiX indexed-mesh/glTF rendering with recursive GGX reflections, progressive G-buffers, moving-camera and animated-geometry reprojection, and temporally plus spatially reused and resolved ReSTIR DI reservoirs (implemented).
7. Denoising and neural reconstruction only after stable temporal buffers exist.

Every milestone must retain a CPU build, deterministic tests, and a small demo
scene before the next one begins.

## Optional OptiX demos (native Windows)

OptiX on WSL2 still requires an experimental driver-library workaround. Keep
the CPU/CUDA labs in WSL2, but run OptiX on native Windows. After downloading
the separate NVIDIA SDK, point `OPTIX_ROOT` at its extracted root:

```powershell
$env:OPTIX_ROOT = 'D:\github\.deps\optix-sdk-9.1.0'
cmake -S . -B build-optix-windows `
  -DBUILD_LEGACY_LABS=OFF -DBUILD_CUDA_DEMOS=OFF -DBUILD_OPTIX_DEMOS=ON
cmake --build build-optix-windows --config Release `
  --target optix_context_probe optix_triangle_demo
ctest --test-dir build-optix-windows -C Release -R optix_ --output-on-failure
```

`optix_triangle_demo` compiles its raygen, miss, and closest-hit programs to
PTX, builds an indexed triangle geometry acceleration structure and shader
binding table, launches radiance and shadow rays through RTX traversal,
validates visible and shadowed pixels, and writes a PPM. Its default scene is
a two-triangle ground plus an elevated triangle casting a hard shadow; pass an
output path and then a `.gltf` path to upload a scene through the shared loader:

```powershell
.\build-optix-windows\Release\optix_triangle_demo.exe output.ppm model.gltf
```

The renderer writes five diagnostic companions next to `output.ppm`:
`output_normal.ppm`, `output_depth.ppm`, `output_motion.ppm`,
`output_validity.ppm`, and `output_reservoir.ppm`. Temporal validity uses green
for accepted history and red for rejected disocclusions; the reservoir view
visualizes spatial candidate count relative to the frame maximum.
`output.ppm` is a four-light RTX reference and `output_restir.ppm` is the ReSTIR
approximation. Reused temporal and spatial samples are reevaluated at the
receiving hit before their source-reservoir normalization is applied. The demo
averages 64 independent spatial resolves and requires the visible-pixel mean
absolute error against the reference to remain within a deterministic range.

Radiance hits can recursively trace up to two reflection bounces. Imported
metallic factors determine the Fresnel reflection weight, while roughness sets
the width of deterministic GGX half-vector samples. Each pixel and bounce uses
a reproducible hash seed so tests remain stable. The demo launches 32 samples
per pixel into a persistent `float4` GPU accumulation buffer and verifies the
sample count before writing the running average.

Primary hits populate persistent world-normal, linear-depth, albedo, and
motion-vector attachments. Reflection and shadow rays are excluded by payload
depth. Frame 1 moves the camera horizontally; each world-space hit is projected
into the previous camera to produce a non-zero screen-space motion vector.
The default elevated triangle also translates between frames. Its vertices are
uploaded and the existing GAS is updated with `OPTIX_BUILD_OPERATION_UPDATE`;
per-primitive motion maps current hits back to their previous world positions.
The same buffers are copied into deterministic debug visualizations after the
spatial resolve pass, providing the diagnostics needed by a future live viewer
and denoiser.

The renderer splits its 32 samples across two frames. Frame 1 reprojects into
frame 0 with motion vectors, then validates 1% relative depth, normal
similarity, and albedo continuity. Previous-view depth is checked against the
current world-space hit's expected distance from the previous camera. Rejected
pixels clear their history; the moving-camera center pixel must report at least
0.5 pixel motion and retain all 32 samples for the test to pass. The complete
frame must contain both accepted history and rejected foreground pixels,
covering stable surfaces as well as true disocclusions.

Direct lighting uses four point-light candidates. Each primary pixel streams
one weighted candidate per sample into a compact ReSTIR DI reservoir. Frame 1
merges the motion-reprojected Frame 0 reservoir only after the same G-buffer
validation, then traces one visibility ray for the selected light and applies
reservoir normalization. The deterministic test requires `M = 32` at the
static center pixel. A second OptiX raygen pass reads those stable temporal
reservoirs, rejects four-neighbor discontinuities using depth, normal, and
albedo, and writes into a separate race-free spatial buffer. The default scene
must produce `M = 160` at its center. A third raygen pass retraces primary rays,
uses the spatial selection and normalization for one new visibility ray and
keeps recursive GGX reflection shading active. Repeating this with independent
spatial seeds produces the 64-sample ReSTIR comparison image; the reference
pass traces all four light visibility rays at each hit.

Without the SDK, configuration stops immediately with an actionable message;
normal CPU and CUDA builds remain unaffected.
