# Rendered validation gallery

These PNG files are reviewed outputs from deterministic experiments. Their
filenames identify the source asset and rendering path. Machine-readable PPM
references live under `tests/visual/references`; CTest regenerates actual
images and produces amplified diff heatmaps when MAE or RMSE exceeds tolerance.

## Legacy ASC meshes through the LabX CPU ray tracer

| Asset | Reviewed output | Coverage |
| --- | --- | --- |
| Lab 3 bench | [`legacy_asc_bench_raytracing.png`](legacy_asc_bench_raytracing.png) | ASC quad triangulation, BVH, thin geometry |
| Lab 3 drop | [`legacy_asc_drop_raytracing.png`](legacy_asc_drop_raytracing.png) | curved silhouette, faceted normals |
| Lab 3 glass | [`legacy_asc_glass_raytracing.png`](legacy_asc_glass_raytracing.png) | narrow stem, multiple scales |
| Lab 3 skull | [`legacy_asc_skull_raytracing.png`](legacy_asc_skull_raytracing.png) | 2,084 triangles, cavities and occlusion |

References are updated only after the corresponding PNG has been rendered and
visually inspected. Tests never overwrite a reviewed image automatically.
