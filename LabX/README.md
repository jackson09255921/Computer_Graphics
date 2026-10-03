# LabX experiments

`LabX` contains the extensions beyond the original coursework. Experiments are
grouped by topic so each rendering technique can evolve without mixing demo
entry points into the legacy labs.

```text
LabX/
├── animation/    # keyframe and quaternion animation demos
├── curves/       # headless and interactive Bézier tools
├── gpu/          # optional CUDA/GPU capability and rendering work
└── raytracing/   # Whitted ray tracing and Monte Carlo path tracing demos
```

## GPU profile and constraints

The initial GPU target is an NVIDIA GeForce RTX 4060 Laptop GPU with 8 GiB
VRAM, compute capability 8.9, and a current NVIDIA driver.
GPU experiments therefore use these constraints:

- CUDA architecture `sm_89`, configurable through `CG_CUDA_ARCHITECTURES`.
- Tiled rendering and bounded allocations rather than keeping large duplicated
  scene buffers in VRAM.
- CPU targets remain available and are the default on machines without CUDA.
- GPU tests are opt-in because hosted CI runners generally have no NVIDIA GPU.

The installed CUDA 11.8 toolkit is older than the current MSVC standard library
accepts (MSVC requires CUDA 12.4 or newer), and its Visual Studio integration is
not registered. Upgrade the toolkit and select its Visual Studio integration
component before enabling `BUILD_CUDA_DEMOS`. The driver already supports a
newer CUDA runtime, so this is a compiler-toolchain update rather than a GPU
replacement.

Validate the local CUDA toolchain and device with:

```powershell
cmake -S . -B build-cuda -DBUILD_LEGACY_LABS=OFF -DBUILD_CUDA_DEMOS=ON
cmake --build build-cuda --config Release --target cuda_capability
ctest --test-dir build-cuda -C Release -R cuda_capability --output-on-failure
```

## Planned milestones

1. Physically based materials: GGX, Fresnel, metallic/roughness workflow.
2. Progressive, multi-threaded CPU rendering and convergence snapshots.
3. Importance sampling and multiple importance sampling.
4. Refraction, dielectric materials, HDR environment lighting, and glTF input.
5. CUDA tiled path tracing sized for the 8 GiB GPU budget.
6. OptiX/DXR evaluation, then ReSTIR direct illumination.
7. Denoising and neural reconstruction only after stable temporal buffers exist.

Every milestone must retain a CPU build, deterministic tests, and a small demo
scene before the next one begins.
