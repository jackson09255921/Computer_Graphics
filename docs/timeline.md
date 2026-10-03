# Rendering technology timeline

This project grows in historical order where practical. Dates refer to the
original or representative publication, not necessarily the first private
implementation of an engineering technique.

| Year | Technique | Project status |
| --- | --- | --- |
| 1980 | Whitted recursive reflection and refraction | Implemented on CPU and CUDA |
| 1980 | Hierarchical bounding volumes for complex scenes | Implemented on CPU and CUDA |
| 1982 | Cook–Torrance physically based reflectance | Implemented |
| 1986 | Kajiya rendering equation and path tracing | Implemented |
| 1991 | Progressive multi-pass global illumination (representative publication) | Implemented |
| 1995 | Veach–Guibas multiple importance sampling | Implemented on CPU and CUDA |
| 1998 | High-dynamic-range image-based lighting | Implemented on CPU and CUDA |
| 2002 | Ray tracing on programmable GPU hardware | Implemented baseline |
| 2007 | GGX microfacet reflection and rough transmission | Matched reflection on CPU and CUDA; rough transmission sampling implemented |
| 2017 | glTF 2.0 scene and physically based material interchange | Implemented |
| 2020 | ReSTIR direct illumination | Planned |

Multi-threaded tile scheduling and GPU execution evolved across many systems,
so they are tracked as engineering capabilities rather than assigned a single
invention date.

## References

- Whitted, *An Improved Illumination Model for Shaded Display*, 1980.
- Rubin and Whitted, *A 3-Dimensional Representation for Fast Rendering of Complex Scenes*, 1980, DOI 10.1145/965105.807479.
- Cook and Torrance, *A Reflectance Model for Computer Graphics*, 1982.
- Kajiya, *The Rendering Equation*, SIGGRAPH 1986.
- Chen et al., *A Progressive Multi-Pass Method for Global Illumination*, SIGGRAPH 1991.
- Veach and Guibas, *Optimally Combining Sampling Techniques for Monte Carlo Rendering*, SIGGRAPH 1995.
- Debevec, *Rendering Synthetic Objects into Real Scenes*, SIGGRAPH 1998.
- Purcell et al., *Ray Tracing on Programmable Graphics Hardware*, 2002.
- Walter et al., *Microfacet Models for Refraction through Rough Surfaces*, 2007.
- Khronos Group, *glTF 2.0 Specification*, 2017.
