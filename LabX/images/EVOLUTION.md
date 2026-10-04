# Rendering evolution gallery

This gallery preserves immutable renders so each Git commit changes one visible
variable at a time. Images are never overwritten after review. Every comparison
uses the same model, resolution, sample count, and random seed unless the table
explicitly says otherwise.

## Lantern: material evolution

| Stage | Commit | Controlled change | Result |
| --- | --- | --- | --- |
| 0 — PBR baseline | `b11436a` | Base color, normal, and metallic-roughness maps | [`quality_gltf_lantern_128spp.png`](quality_gltf_lantern_128spp.png) |
| 1 — Emissive surface | `6944a92` | Add emissive factor/texture and surface radiance only | [`quality_gltf_lantern_emissive_128spp.png`](quality_gltf_lantern_emissive_128spp.png) |
| 2 — Emissive lighting | planned | Sample emissive triangles with NEE/MIS so the lamp illuminates its surroundings | — |
| 3 — Showcase lighting | planned | Darker exposure, warm key light, and cool rim light | — |

Stage 0 → 1 proves that the texture is decoded and visible. Stage 1 → 2 will
isolate the difference between an object that merely looks bright and a mesh
light that transports energy into the scene.

## ToyCar: composition and material evolution

| Stage | Commit | Controlled change | Result |
| --- | --- | --- | --- |
| 0 — PBR baseline | `b11436a` | Automatic full-scene bounds and opaque core PBR | [`quality_gltf_toycar_128spp.png`](quality_gltf_toycar_128spp.png) |
| 1 — Transmission | `1ea84e7` | Add KHR transmission/IOR while preserving camera and light | [`quality_gltf_toycar_transmission_128spp.png`](quality_gltf_toycar_transmission_128spp.png) |
| 2 — Subject framing | `d1815ee` | Frame material 0/2 car geometry while material 1 fabric no longer controls bounds | [`evolution_toycar_stage2_subject_framing_128spp.png`](evolution_toycar_stage2_subject_framing_128spp.png) |
| 3 — Three-quarter camera | `66536a6` | Low 3/4 view to expose body curvature and silhouette | [`evolution_toycar_stage3_three_quarter_camera_128spp.png`](evolution_toycar_stage3_three_quarter_camera_128spp.png) |
| 4 — Studio lighting | `538a865` | Warm key, cool fill, and off-camera rim lights with multi-light MIS | [`evolution_toycar_stage4_studio_lighting_128spp.png`](evolution_toycar_stage4_studio_lighting_128spp.png) |
| 5 — Clearcoat and sheen | planned | Add the car-paint coat and fabric grazing reflection | — |

The original framing is deliberately retained: it documents why scene-wide
bounding boxes are reliable for CI yet weak for presentation. The next render
will change framing only before camera, lighting, or BSDF work is introduced.

## Rules for future stages

1. Use a new descriptive filename; never replace a reviewed image.
2. Change one major variable per commit.
3. Record resolution, SPP, seed, model, and active material features.
4. Run automated image validation and inspect the PNG at native resolution.
5. Keep CI smoke renders separate from quality/evolution renders.
6. Add the commit hash after the stage is pushed.

This structure makes Git history a visual experiment log rather than a folder
containing only the latest result.
