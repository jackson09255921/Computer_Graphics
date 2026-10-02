# CPU Rasterization & Shading Labs

A from-scratch computer graphics learning project written in C++ and visualized
with OpenGL/GLUT. OpenGL is used only to display points; rasterization,
transformations, clipping, visibility, and shading are implemented on the CPU.

> 以 C++ 從零實作光柵化與即時渲染管線。OpenGL/GLUT 僅負責顯示像素，核心演算法皆由 CPU 完成。

![Rendered scene with Z-buffering](images/image37.png)

## Highlights

- Scan conversion and polygon filling
- Viewport transformation and 2D clipping
- OBJ mesh loading
- Model, view, projection, and viewport transformations
- Perspective division and 3D clipping
- Back-face culling and Z-buffer visibility
- Blinn–Phong lighting
- Flat, Gouraud, and Phong shading

## Labs

| Lab | Focus | Source | Example |
| --- | --- | --- | --- |
| 1 | Interactive drawing primitives | Incorporated into later labs; the standalone source was not preserved | [`image28.png`](images/image28.png) |
| 2 | Polygon filling, viewport transforms, and 2D clipping | [`2022CG_Lab2.cpp`](2022CG_Lab2/2022CG_Lab2.cpp) | [`image35.png`](images/image35.png) |
| 3 | Complete 3D transformation and rasterization pipeline | [`2022CG_Lab3.cpp`](2022CG_Lab3/2022CG_Lab3.cpp) | [`image37.png`](images/image37.png) |
| 4 | Z-buffering and Flat/Gouraud/Phong shading | [`2022CG_Lab4.cpp`](2022CG_Lab4/2022CG_Lab4.cpp) | [`image37.png`](images/image37.png) |

### Pipeline

![Real-time rendering pipeline](images/image40.png)

The Lab 3 renderer transforms mesh vertices through object, world, view,
projection, and screen spaces. It then clips geometry, rejects back-facing
surfaces, and rasterizes the visible polygons.

### Lighting and shading

![Rendered scene with Z-buffering](images/image37.png)

Lab 4 evaluates ambient, diffuse, and specular lighting and supports three
interpolation frequencies:

- **Flat:** one lighting result per triangle
- **Gouraud:** lighting per vertex, followed by color interpolation
- **Phong:** normal interpolation followed by per-pixel lighting

## Build

### Requirements

- A C++17 compiler
- CMake 3.16+
- OpenGL and GLUT/freeglut development packages

Ubuntu/Debian:

```bash
sudo apt install build-essential cmake freeglut3-dev
```

Configure and compile:

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Run

The programs resolve `Data/` and `Mesh/` relative to their corresponding lab
directory. Run each executable with that directory as the working directory.

```bash
cd 2022CG_Lab2
../build/lab2 lab2D.in

cd ../2022CG_Lab3
../build/lab3 Lab3B.in

cd ../2022CG_Lab4
../build/lab4 lab4A.in phong
```

On multi-config generators such as Visual Studio, executables may be under
`build/Release/`. Lab 4 accepts `flat`, `gouraud`, or `phong` as its second
argument.

## Controls

After the window opens, press <kbd>Enter</kbd> to advance through commands from
the selected input file. Some inherited drawing controls are also available:

| Key | Action |
| --- | --- |
| `d` | Draw points/freehand strokes |
| `l` | Draw a line |
| `p` | Draw a polygon |
| `c` | Draw a circle |
| `u` | Undo |
| `s` | Change color |
| `e` | Clear |
| `q` | Quit |

## Repository layout

```text
.
├── 2022CG_Lab2/       # 2D rasterization and clipping
├── 2022CG_Lab3/       # 3D rendering pipeline and OBJ meshes
├── 2022CG_Lab4/       # lighting, shading, and Z-buffering
├── images/            # diagrams and rendered results
└── CMakeLists.txt      # reproducible build configuration
```

## Current limitations

- The renderer is educational and intentionally CPU-bound.
- Source files are monolithic because each lab builds on the previous one.
- The standalone Lab 1 source is not present in the original archive.
- The remaining sample meshes came from the original coursework archive and
  still require a provenance/license review before this project can adopt an
  open-source license. Character and celebrity meshes with unclear rights were
  removed from the public source tree.

## Acknowledgements

This project was developed as a sequence of 2022 computer graphics coursework
labs. The conceptual study also drew from the
[GAMES101 computer graphics course](https://sites.cs.ucsb.edu/~lingqi/teaching/games101.html).
