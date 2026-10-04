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
| 5 — Clearcoat | `a75e312` | Add a second dielectric GGX coat with matched lobe sampling/PDF | [`evolution_toycar_stage5_clearcoat_128spp.png`](evolution_toycar_stage5_clearcoat_128spp.png) |
| 6 — Fabric sheen | `8670276` | Enable KHR_materials_sheen with Charlie microfiber distribution; paired baseline changes only this lobe | [`baseline`](evolution_toycar_stage6_fabric_baseline_128spp.png) · [`sheen`](evolution_toycar_stage6_fabric_sheen_128spp.png) |
| 7 — Close framing | `a8bf651` | Increase full-scene scale and narrow the camera FOV while retaining the complete car and cloth silhouette | [`baseline`](evolution_toycar_stage7_close_baseline_128spp.png) · [`sheen`](evolution_toycar_stage7_close_sheen_128spp.png) |

The original framing is deliberately retained: it documents why scene-wide
bounding boxes are reliable for CI yet weak for presentation. Stages 2–5 then
isolate framing, camera, lighting, and clearcoat as separate changes.

Stage 6 deliberately returns to the complete ToyCar scene because material 1 is
the draped fabric. Both 640x360 renders use 128 spp and seed `0xC0FFEE`; only
the sheen lobe changes. Automated comparison found 44,926 changed pixels with
a 0.987 channel MAE and a maximum channel delta of 165, while native-resolution
review confirmed that the red grazing highlights follow the cloth folds.

Stage 7 keeps the Stage 6 material comparison but moves the composition closer:
the car body, cloth perimeter, and primary folds occupy most of the frame without
clipping. At 128 spp, the paired renders differ across 98,431 pixels with a
2.760 channel MAE and a maximum channel delta of 177.

## DamagedHelmet: occlusion evolution

| Stage | Commit | Controlled change | Result |
| --- | --- | --- | --- |
| 8 — Occlusion map | `4b28a7f` | Import the glTF occlusion texture/strength and apply it to environment plus later-bounce energy | [`baseline`](evolution_helmet_stage8_baseline_128spp.png) · [`occlusion`](evolution_helmet_stage8_occlusion_128spp.png) |

Both 640x360 images use the same close camera, three-light studio, 128 spp, and
seed `0xC0FFEE`. Occlusion changes 45,111 pixels with a 0.359 channel MAE and a
maximum channel delta of 58; native-resolution review shows the intended subtle
darkening in seams, panel gaps, and recessed parts rather than a global color shift.

## AlphaBlendModeTest: transparency evolution

| Stage | Commit | Controlled change | Result |
| --- | --- | --- | --- |
| 9 — Alpha modes | `979a9d1` | Preserve RGBA and enable MASK cutoff plus stochastic BLEND coverage during BVH traversal | [`opaque baseline`](evolution_alpha_stage9_opaque_128spp.png) · [`alpha`](evolution_alpha_stage9_alpha_128spp.png) |

The official Khronos test asset exposes five authored opacity/cutoff cases in a
single view. Both 640x360 renders use the same camera, studio lights, 128 spp,
and seed `0xC0FFEE`. Alpha processing changes 94,139 pixels with a 4.825 channel
MAE and maximum channel delta of 181; the reviewed result reveals the rear frame
and background through MASK/BLEND texels while OPAQUE remains solid.

## TextureTransformTest: UV mapping evolution

| Stage | Commit | Controlled change | Result |
| --- | --- | --- | --- |
| 10 — Texture transforms | `3273175` | Enable per-texture texCoord, offset, rotation, scale and sampler wrapping | [`baseline`](evolution_texture_stage10_baseline_128spp.png) · [`transformed`](evolution_texture_stage10_transformed_128spp.png) |

The official Khronos test uses six panels to expose offset, rotation, scale and
combined mapping errors. Both 640x360 renders use the same camera, studio lights,
128 spp and seed `0xC0FFEE`. Transform support changes 131,488 pixels with a
16.287 channel MAE and maximum channel delta of 202. Native review confirms the
three U/V/UV success panels and all green arrow targets.

## ClearCoatTest: layered texture evolution

| Stage | Commit | Controlled change | Result |
| --- | --- | --- | --- |
| 11 — Clearcoat textures | pending | Enable factor (R), roughness (G), and independent coat-normal textures | [`baseline`](evolution_clearcoat_stage11_baseline_128spp.png) · [`textures`](evolution_clearcoat_stage11_textures_128spp.png) |

The official Khronos ClearCoatTest contains 37,116 triangles and five labelled
rows for partial coating, roughness variation, base normals, shared normals and
independent coat normals. Both 640x360 renders use the same camera, studio
lights, 128 spp and seed `0xC0FFEE`. Enabling the three texture slots changes
19,676 pixels with a 0.249 channel MAE and maximum channel delta of 135. Both
PNGs were read back and inspected at native resolution. The layered model follows
the clearcoat lineage popularized by the 2012 Disney principled BRDF.

## Rules for future stages

1. Use a new descriptive filename; never replace a reviewed image.
2. Change one major variable per commit.
3. Record resolution, SPP, seed, model, and active material features.
4. Run automated image validation and inspect the PNG at native resolution.
5. Keep CI smoke renders separate from quality/evolution renders.
6. Add the commit hash after the stage is pushed.

This structure makes Git history a visual experiment log rather than a folder
containing only the latest result.
