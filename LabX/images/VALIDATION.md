# Full render validation — 2026-10-05

This audit regenerated the current renderer outputs rather than reviewing stale
PNGs. The quality pipeline produced 29 images: the CPU reference at 512 spp,
CUDA quality and evolution renders at 128 spp, and a 1280x720 multi-object
scene. Every published image passed format, dimension, truncation, and visual
variation checks and was then inspected at native resolution.

## Corrections made during the audit

1. Cached average texture radiance by texture index while building mesh lights.
   BoomBox 8 spp dropped to 4.62 seconds; before the fix, redundant full-texture
   scans per triangle were still running after 4 minutes 15 seconds.
2. Enlarged generic glTF subjects while preserving complete silhouettes.
   Lantern, WaterBottle, Avocado, and BoomBox now occupy useful screen space.
3. Enlarged and respaced the bunny/teapot multi-object scene.
4. Tightened the ToyCar clearcoat presentation camera.
5. Reframed Clearcoat and Sheen grids so every labelled row remains visible.
6. Enlarged AnisotropyDiscTest so directional highlights and roughness labels
   can be inspected directly.

## Controlled comparisons

All comparisons below use 640x360, 128 spp, and deterministic seed `0xC0FFEE`.

| Feature | Changed pixels | Channel MAE | Max delta | Visual decision |
| --- | ---: | ---: | ---: | --- |
| Fabric sheen | 43,007 | 0.941 | 192 | Pass; supporting wide view |
| Close fabric sheen | 95,475 | 2.628 | 170 | Pass; keep as featured ToyCar view |
| Occlusion | 45,056 | 0.359 | 58 | Pass; subtle supporting evidence |
| Alpha MASK/BLEND | 94,377 | 4.823 | 175 | Pass; clear featured comparison |
| Texture transform | 131,663 | 16.283 | 201 | Pass; clearest labelled mapping test |
| Clearcoat textures | 17,133 | 0.217 | 134 | Pass; subtle supporting evidence |
| Sheen textures | 53,905 | 0.770 | 80 | Pass; labelled factor grid is readable |
| Transmission textures | 81,948 | 1.327 | 219 | Pass; keep as featured glass comparison |
| Specular extension | 50,736 | 1.977 | 255 | Pass; labelled F0/F90 response is visible |
| Anisotropic GGX | 92,263 | 1.706 | 157 | Pass after reframing |
| Mesh-light NEE/MIS | 197,526 | 10.909 | 154 | Pass; strongest lighting comparison |

## Automated validation

- CPU CTest: 22/22 passed.
- CUDA non-OptiX CTest: 26/26 passed.
- Jenkins-equivalent validation pipeline: 22 validations passed at 8 spp.
- Khronos assets: 15 files verified by license metadata and SHA-256 digest.
- Published PNG count: 65 across `evolution/`, `quality/`, `ci/`, `legacy/`,
  and `archive/`; no PNG remains loose in the gallery root.

## Remaining limitations

- Clearcoat texture and occlusion effects are physically meaningful but too
  subtle to serve as portfolio thumbnails; they remain evolution evidence.
- 128 spp still leaves visible Monte Carlo grain in darker regions. Adaptive
  sampling and denoising belong to the next quality/performance track.
- Stable glass caustics still require a dedicated refractive-connection,
  photon, or bidirectional technique; ordinary camera path tracing rarely finds
  those paths.
- The CUDA BVH uses a median split and fixed-stack traversal. SAH/wide-BVH and
  memory-layout work remain worthwhile for the 99k-triangle ToyCar scenes.
