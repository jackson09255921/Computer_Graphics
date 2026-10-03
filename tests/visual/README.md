# Visual regression tests

Visual tests render deterministic scenes, read the generated image, compare it
with a reviewed reference, and write an amplified difference heatmap. A process
exit code of zero is not sufficient: dimensions, MAE, and RMSE must also pass.

References in `references/` are reviewed assets. Tests must never overwrite
them automatically. To change one:

1. render a candidate into the build directory;
2. convert it to PNG and inspect the image;
3. explain why the visual change is intended;
4. copy the reviewed candidate into `references/` explicitly;
5. run the visual test again and commit the reference with the code change.

The deterministic baselines cover the Bézier demo plus the legacy bench, drop,
glass, and skull ASC assets rendered through the LabX CPU ray tracer. Animation,
CUDA, and OptiX baselines are added incrementally; stochastic and GPU outputs
use non-zero error thresholds and semantic invariants. Reviewed PNG companions
are stored in `LabX/images/` for human inspection.
