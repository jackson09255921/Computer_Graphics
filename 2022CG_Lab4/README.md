# Lab 4 — Lighting and Shading

This lab adds a Z-buffer, Blinn–Phong lighting, and three shading frequencies:
Flat, Gouraud, and Phong.

From the repository root, build with CMake, then run with this directory as the
working directory:

```bash
../build/lab4 lab4A.in phong
```

The second argument accepts `flat`, `gouraud`, or `phong`. Input scenes live in
[`Data/`](Data/) and reference models in [`Mesh/`](Mesh/). Press <kbd>Enter</kbd>
to step through the scene commands.
