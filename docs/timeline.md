# Rendering technology timeline

This project grows in historical order where practical. Dates refer to the
original or representative publication, not necessarily the first private
implementation of an engineering technique.

| Year | Technique | Project status |
| --- | --- | --- |
| 1980 | Whitted recursive reflection, refraction, and shadow rays | Implemented on CPU/CUDA; OptiX recursive reflection plus radiance and hard-shadow ray types implemented |
| 1980 | Hierarchical bounding volumes for complex scenes | Implemented on CPU and CUDA |
| 1982 | Cook–Torrance physically based reflectance | Implemented |
| 1986 | Kajiya rendering equation and path tracing | Implemented |
| 1988 | Hardware deferred shading and per-pixel attribute buffers | OptiX primary-hit normal, depth, albedo, and camera/object motion G-buffers plus normal/depth/motion debug views implemented |
| 1991 | Progressive multi-pass global illumination (representative publication) | Implemented on CPU and as 32-spp persistent GPU accumulation in OptiX |
| 1995 | Veach–Guibas multiple importance sampling | Implemented on CPU and CUDA |
| 1998 | High-dynamic-range image-based lighting | Implemented on CPU and CUDA |
| 2002 | Ray tracing on programmable GPU hardware | Implemented baseline |
| 2007 | GGX microfacet reflection and rough transmission | Matched reflection on CPU/CUDA, matched rough BTDF on CUDA, and recursive GGX reflection-direction sampling on OptiX |
| 2010 | NVIDIA OptiX programmable ray tracing engine | Native Windows indexed/glTF GAS, multi-ray-type SBT, Lambert lighting, recursive reflections, hard shadows, and RTX launch implemented |
| 2017 | glTF 2.0 scene and physically based material interchange | Implemented |
| 2020 | ReSTIR direct illumination | CUDA temporal/spatial core plus OptiX reprojection, receiver-target reservoir merging, GAS updates, disocclusion rejection, visibility rays, 64-sample spatial resolve, four-light reference/MAE validation, and debug views implemented |

Multi-threaded tile scheduling and GPU execution evolved across many systems,
so they are tracked as engineering capabilities rather than assigned a single
invention date.

## References

- Whitted, *An Improved Illumination Model for Shaded Display*, 1980.
- Rubin and Whitted, *A 3-Dimensional Representation for Fast Rendering of Complex Scenes*, 1980, DOI 10.1145/965105.807479.
- Cook and Torrance, *A Reflectance Model for Computer Graphics*, 1982.
- Kajiya, *The Rendering Equation*, SIGGRAPH 1986.
- Deering et al., *The Triangle Processor and Normal Vector Shader*, SIGGRAPH 1988, DOI 10.1145/378456.378468.
- Chen et al., *A Progressive Multi-Pass Method for Global Illumination*, SIGGRAPH 1991.
- Veach and Guibas, *Optimally Combining Sampling Techniques for Monte Carlo Rendering*, SIGGRAPH 1995.
- Debevec, *Rendering Synthetic Objects into Real Scenes*, SIGGRAPH 1998.
- Purcell et al., *Ray Tracing on Programmable Graphics Hardware*, 2002.
- Walter et al., *Microfacet Models for Refraction through Rough Surfaces*, 2007.
- Parker et al., *OptiX: A General Purpose Ray Tracing Engine*, SIGGRAPH 2010.
- Khronos Group, *glTF 2.0 Specification*, 2017.
- Bitterli et al., *Spatiotemporal Reservoir Resampling for Real-Time Ray Tracing with Dynamic Direct Lighting*, SIGGRAPH 2020.
